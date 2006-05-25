/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <windows.h>

#include <kctrl.h>

#include "convja.h"

using namespace qs;
using namespace qsconvja;


/****************************************************************************
 *
 * ShiftJISConverter
 *
 */

qsconvja::ShiftJISConverter::ShiftJISConverter()
{
}

qsconvja::ShiftJISConverter::~ShiftJISConverter()
{
}

size_t qsconvja::ShiftJISConverter::encodeImpl(const WCHAR* pwsz,
											   size_t nLen,
											   XStringBuffer<XSTRING>* pBuf)
											   QNOTHROW()
{
	XStringBufferLock<XSTRING> lock(pBuf, nLen*2);
	CHAR* pLock = lock.get();
	if (!pLock)
		return -1;
	CHAR* p = pLock;
	
	const WCHAR* pEnd = pwsz + nLen;
	while (pwsz < pEnd) {
		WORD sjis = unicode2sjis_char(*pwsz++);
		if (sjis & 0xff00)
			*p++ = (sjis >> 8) & 0xff;
		*p++ = sjis & 0xff;
	}
	
	lock.unlock(p - pLock);
	
	return nLen;
}

size_t qsconvja::ShiftJISConverter::decodeImpl(const CHAR* psz,
											   size_t nLen,
											   XStringBuffer<WXSTRING>* pBuf)
											   QNOTHROW()
{
	XStringBufferLock<WXSTRING> lock(pBuf, nLen);
	WCHAR* pLock = lock.get();
	if (!pLock)
		return -1;
	WCHAR* p = pLock;
	
	const CHAR* pBegin = psz;
	const CHAR* pEnd = psz + nLen;
	while (psz < pEnd) {
		WORD sjis = static_cast<unsigned char>(*psz);
		if (isLeadByte(static_cast<unsigned char>(sjis))) {
			if (psz + 1 == pEnd)
				break;
			++psz;
			sjis <<= 8;
			sjis |= static_cast<unsigned char>(*psz++);
		}
		else {
			++psz;
		}
		*p++ = sjis2unicode_char(sjis);
	}
	
	lock.unlock(p - pLock);
	
	return psz - pBegin;
}

bool qsconvja::ShiftJISConverter::isLeadByte(unsigned char c)
{
	return (c >= 0x81 && c <= 0x9f) || (c >= 0xe0 && c <= 0xfc);
}


/****************************************************************************
 *
 * ShiftJISConverterFactory
 *
 */

qsconvja::ShiftJISConverterFactory::ShiftJISConverterFactory()
{
	registerFactory(this);
}

qsconvja::ShiftJISConverterFactory::~ShiftJISConverterFactory()
{
	unregisterFactory(this);
}

bool qsconvja::ShiftJISConverterFactory::isSupported(const WCHAR* pwszName)
{
	assert(pwszName);
	return _wcsicmp(pwszName, L"shift_jis") == 0 ||
		_wcsicmp(pwszName, L"x-sjis") == 0;
}

std::auto_ptr<Converter> qsconvja::ShiftJISConverterFactory::createInstance(const WCHAR* pwszName)
{
	return std::auto_ptr<Converter>(new ShiftJISConverter());
}


/****************************************************************************
 *
 * ISO2022JPConverter
 *
 */

qsconvja::ISO2022JPConverter::ISO2022JPConverter() :
	mode_(MODE_ASCII)
{
}

qsconvja::ISO2022JPConverter::~ISO2022JPConverter()
{
}

size_t qsconvja::ISO2022JPConverter::encodeImpl(const WCHAR* pwsz,
												size_t nLen,
												XStringBuffer<XSTRING>* pBuf)
												QNOTHROW()
{
	const CHAR szKanji[] = { 0x1b, '$', 'B', '\0' };
	const CHAR szAscii[] = { 0x1b, '(', 'B', '\0' };
	
	const WCHAR* pBegin = pwsz;
	const WCHAR* pEnd = pwsz + nLen;
	while (pwsz < pEnd) {
		WORD sjis = unicode2sjis_char(*pwsz);
		if (sjis & 0xff00) {
			if (mode_ == MODE_ASCII) {
				if (!pBuf->append(szKanji))
					return -1;
				mode_ = MODE_KANJI;
			}
			WORD jis = Util::sjis2jis(sjis);
			if (!pBuf->append(HIBYTE(jis)) ||
				!pBuf->append(LOBYTE(jis)))
				return -1;
		}
		else if (Util::isHalfWidthKatakana(static_cast<unsigned char>(sjis & 0xff))) {
			if (mode_ == MODE_ASCII) {
				if (!pBuf->append(szKanji))
					return -1;
				mode_ = MODE_KANJI;
			}
			if (pwsz + 1 == pEnd)
				break;
			WORD sjisNext = unicode2sjis_char(*(pwsz + 1));
			bool bNext = false;
			WORD jis = Util::sjis2jis(Util::han2zen(static_cast<unsigned char>(sjis & 0xff),
				sjisNext & 0xff00 ? 0 : static_cast<unsigned char>(sjisNext & 0xff), &bNext));
			if (bNext)
				++pwsz;
			if (!pBuf->append(HIBYTE(jis)) ||
				!pBuf->append(LOBYTE(jis)))
				return -1;
		}
		else {
			if (mode_ == MODE_KANJI) {
				if (!pBuf->append(szAscii))
					return -1;
				mode_ = MODE_ASCII;
			}
			if (!pBuf->append(static_cast<unsigned char>(sjis & 0xff)))
				return -1;
		}
		++pwsz;
	}
	if (mode_ == MODE_KANJI) {
		if (!pBuf->append(szAscii))
			return -1;
		mode_ = MODE_ASCII;
	}
	
	return pwsz - pBegin;
}

size_t qsconvja::ISO2022JPConverter::decodeImpl(const CHAR* psz,
												size_t nLen,
												XStringBuffer<WXSTRING>* pBuf)
												QNOTHROW()
{
	const CHAR szKanji1[] = { 0x1b, '$', 'B', '\0' };
	const CHAR szKanji2[] = { 0x1b, '$', '@', '\0' };
	const CHAR szAscii1[] = { 0x1b, '(', 'B', '\0' };
	const CHAR szAscii2[] = { 0x1b, '(', 'J', '\0' };
	const CHAR szKana[] = { 0x1b, L'(', 'I', '\0' };
	
	const CHAR* pBegin = psz;
	const CHAR* pEnd = psz + nLen;
	while (psz < pEnd) {
		if (*psz == 0x1b) {
			if (psz + 2 >= pEnd)
				break;
			if (strncmp(psz, szKanji1, 3) == 0 || strncmp(psz, szKanji2, 3) == 0) {
				psz += 3;
				mode_ = MODE_KANJI;
			}
			else if (strncmp(psz, szAscii1, 3) == 0 || strncmp(psz, szAscii2, 3) == 0) {
				psz += 3;
				mode_ = MODE_ASCII;
			}
			else if (strncmp(psz, szKana, 3) == 0) {
				psz += 3;
				mode_ = MODE_KANA;
			}
			else {
				if (!pBuf->append(static_cast<WCHAR>(*psz)))
					return -1;
				++psz;
			}
		}
		else {
			WORD sjis = 0;
			bool bIncomplete = false;
			switch (mode_) {
			case MODE_ASCII:
				sjis = static_cast<WORD>(*psz);
				break;
			case MODE_KANA:
				sjis = Util::jis2sjis(static_cast<WORD>(*psz));
				break;
			case MODE_KANJI:
				if (psz + 1 == pEnd) {
					bIncomplete = true;
					break;
				}
				sjis = Util::jis2sjis(static_cast<WORD>(*psz) << 8 |
					static_cast<WORD>(*(psz + 1)));
				++psz;
				break;
			default:
				assert(false);
				return -1;
			}
			if (bIncomplete)
				break;
			if (!pBuf->append(sjis2unicode_char(sjis)))
				return -1;
			++psz;
		}
	}
	
	return psz - pBegin;
}


/****************************************************************************
 *
 * ISO2022JPConverterFactory
 *
 */

qsconvja::ISO2022JPConverterFactory::ISO2022JPConverterFactory()
{
	registerFactory(this);
}

qsconvja::ISO2022JPConverterFactory::~ISO2022JPConverterFactory()
{
	unregisterFactory(this);
}

bool qsconvja::ISO2022JPConverterFactory::isSupported(const WCHAR* pwszName)
{
	return _wcsnicmp(pwszName, L"iso-2022-jp", 11) == 0;
}

std::auto_ptr<Converter> qsconvja::ISO2022JPConverterFactory::createInstance(const WCHAR* pwszName)
{
	return std::auto_ptr<Converter>(new ISO2022JPConverter());
}


/****************************************************************************
 *
 * EUCJPConverter
 *
 */

qsconvja::EUCJPConverter::EUCJPConverter()
{
}

qsconvja::EUCJPConverter::~EUCJPConverter()
{
}

size_t qsconvja::EUCJPConverter::encodeImpl(const WCHAR* pwsz,
											size_t nLen,
											XStringBuffer<XSTRING>* pBuf)
											QNOTHROW()
{
	XStringBufferLock<XSTRING> lock(pBuf, nLen*2);
	CHAR* pLock = lock.get();
	if (!pLock)
		return -1;
	CHAR* p = pLock;
	
	const WCHAR* pEnd = pwsz + nLen;
	while (pwsz < pEnd) {
		WORD sjis = unicode2sjis_char(*pwsz++);
		if (sjis & 0xff00) {
			WORD jis = Util::sjis2jis(sjis);
			*p++ = HIBYTE(jis) | 0x80;
			*p++ = LOBYTE(jis) | 0x80;
		}
		else if (Util::isHalfWidthKatakana(static_cast<unsigned char>(sjis & 0xff))) {
			*p++ = static_cast<unsigned char>(0x8e);
			*p++ = static_cast<unsigned char>(sjis & 0xff);
		}
		else {
			*p++ = static_cast<unsigned char>(sjis & 0xff);
		}
	}
	
	lock.unlock(p - pLock);
	
	return nLen;
}

size_t qsconvja::EUCJPConverter::decodeImpl(const CHAR* psz,
											size_t nLen,
											XStringBuffer<WXSTRING>* pBuf)
											QNOTHROW()
{
	XStringBufferLock<WXSTRING> lock(pBuf, nLen);
	WCHAR* pLock = lock.get();
	if (!pLock)
		return -1;
	WCHAR* p = pLock;
	
	const CHAR* pBegin = psz;
	const CHAR* pEnd = psz + nLen;
	while (psz < pEnd) {
		WORD sjis = 0;
		if (*psz == 0x8e) {
			if (psz + 1 == pEnd)
				break;
			sjis = static_cast<WORD>(*(psz + 1));
			psz += 2;
		}
		else if (*psz & 0x80) {
			if (psz + 1 == pEnd)
				break;
			sjis = Util::jis2sjis(static_cast<WORD>(*psz & 0x7f) << 8 |
				static_cast<WORD>(*(psz + 1) & 0x7f));
			psz += 2;
		}
		else {
			sjis = static_cast<WORD>(*psz);
			++psz;
		}
		*p++ = sjis2unicode_char(sjis);
	}
	
	lock.unlock(p - pLock);
	
	return psz - pBegin;
}


/****************************************************************************
 *
 * EUCJPConverterFactory
 *
 */

qsconvja::EUCJPConverterFactory::EUCJPConverterFactory()
{
	registerFactory(this);
}

qsconvja::EUCJPConverterFactory::~EUCJPConverterFactory()
{
	unregisterFactory(this);
}

bool qsconvja::EUCJPConverterFactory::isSupported(const WCHAR* pwszName)
{
	return _wcsicmp(pwszName, L"euc-jp") == 0;
}

std::auto_ptr<Converter> qsconvja::EUCJPConverterFactory::createInstance(const WCHAR* pwszName)
{
	return std::auto_ptr<Converter>(new EUCJPConverter());
}


/****************************************************************************
 *
 * Util
 *
 */

WORD qsconvja::Util::sjis2jis(WORD sjis)
{
	BYTE bHigh = HIBYTE(sjis);
	BYTE bLow = LOBYTE(sjis);
	
	bHigh -= (bHigh <= 0x9f) ? 0x70 : 0xb0;
	bHigh <<= 1;
	if (bLow < 0x9f) {
		bLow -= (bLow < 0x7f) ? 0x1f : 0x20;
		--bHigh;
	}
	else {
		bLow -= 0x7e;
	}
	return (bHigh << 8) | bLow ;
}

WORD qsconvja::Util::jis2sjis(WORD jis)
{
	BYTE bHigh = HIBYTE(jis);
	BYTE bLow = LOBYTE(jis);
	
	if (bHigh & 0x01) {
		bHigh >>= 1;
		bHigh += 0x71;
		bLow += 0x1f;
		if (bLow >= 0x7f) {
			++bLow;
		}
	}
	else {
		bHigh >>= 1;
		bHigh += 0x70;
		bLow += 0x7e;
	}
	if (bHigh > 0x9f) {
		bHigh += 0x40;
	}
	return (bHigh << 8) | bLow ;
}

WORD qsconvja::Util::han2zen(unsigned char sjis,
							 unsigned char sjisNext,
							 bool* pbDakuten)
{
	assert(isHalfWidthKatakana(sjis));
	assert(pbDakuten);
	
	const WORD hanZen[] = {
		0x8140, 0x8142, 0x8175, 0x8176, 0x8141, 0x8145, 0x8392, 0x8340,
		0x8342, 0x8344, 0x8346, 0x8348, 0x8383, 0x8385, 0x8387, 0x8362,
		0x815B, 0x8341, 0x8343, 0x8345, 0x8347, 0x8349, 0x834A, 0x834C,
		0x834E, 0x8350, 0x8352, 0x8354, 0x8356, 0x8358, 0x835A, 0x835C,
		0x835E, 0x8360, 0x8363, 0x8365, 0x8367, 0x8369, 0x836A, 0x836B,
		0x836C, 0x836D, 0x836E, 0x8371, 0x8374, 0x8377, 0x837A, 0x837D,
		0x837E, 0x8380, 0x8381, 0x8382, 0x8384, 0x8386, 0x8388, 0x8389,
		0x838A, 0x838B, 0x838C, 0x838D, 0x838F, 0x8393, 0x814A, 0x814B,
	};
	
	*pbDakuten = false;
	if (sjis == 0xde) {
		// Dakuten
		return 0x814a;
	}
	else if (sjis == 0xdf) {
		// Handakuten
		return 0x814b;
	}
	else if (sjisNext == 0xde) {
		// Dakuten
		if (sjis == 0xb3) {
			*pbDakuten = true;
			return 0x8394;
		}
		else if ((0xb6 <= sjis && sjis <= 0xc4) || (0xca <= sjis && sjis <= 0xce)) {
			*pbDakuten = true;
			return hanZen[sjis - 0xa0] + 0x01;
		}
		else {
			return hanZen[sjis - 0xa0];
		}
	}
	else if (sjisNext == 0xdf) {
		// Handakuten
		if (0xca <= sjis && sjis <= 0xce) {
			*pbDakuten = true;
			return hanZen[sjis - 0xa0] + 0x02;
		}
		else {
			return hanZen[sjis - 0xa0];
		}
	}
	else {
		return hanZen[sjis - 0xa0];
	}
}

bool qsconvja::Util::isHalfWidthKatakana(unsigned char sjis)
{
	return 0xa0 <= sjis && sjis <= 0xdf;
}
