/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsconv.h>
#include <qsencoder.h>
#include <qsinit.h>
#include <qsosutil.h>
#include <qsstl.h>

#include <tchar.h>
#include <windows.h>
#include <winsock.h>

#include <vector>
#include <algorithm>

#ifdef QS_KCONVERT
#	include <kctrl.h>
#endif

using namespace qs;

#if defined _WIN32_WCE && _WIN32_WCE >= 211
#include <initguid.h>
DEFINE_GUID(IID_IMultiLanguage, 0x275c23e1, 0x3747, 0x11d0,
	0x9f, 0xea, 0x00, 0xaa, 0x00, 0x3f, 0x86, 0x46);
DEFINE_GUID(CLSID_CMultiLanguage, 0x275c23e2, 0x3747, 0x11d0,
	0x9f, 0xea, 0x00, 0xaa, 0x00, 0x3f, 0x86, 0x46);
#endif

namespace qs {
struct ConverterFactoryImpl;
}


/****************************************************************************
 *
 * Simple Conversion
 *
 */

#ifdef QS_KCONVERT
inline bool isLeadByte(unsigned char c)
{
	return (c >= 0x81 && c <= 0x9f) || (c >= 0xe0 && c <= 0xfc);
}
#endif

QSEXPORTPROC string_ptr qs::wcs2mbs(const WCHAR* pwszSrc)
{
	return wcs2mbs(pwszSrc, static_cast<size_t>(-1), 0);
}

QSEXPORTPROC string_ptr qs::wcs2mbs(const WCHAR* pwszSrc,
									size_t nLen)
{
	return wcs2mbs(pwszSrc, nLen, 0);
}

QSEXPORTPROC string_ptr qs::wcs2mbs(const WCHAR* pwszSrc,
									size_t nLen,
									size_t* pnLen)
{
#ifdef QS_KCONVERT
	if (nLen == static_cast<size_t>(-1))
		nLen = wcslen(pwszSrc);
	const WCHAR* pwszSrcEnd = pwszSrc + nLen;
	string_ptr str(allocString(nLen*2 + 1));
	CHAR* p = str.get();
	while (pwszSrc < pwszSrcEnd) {
		WORD sjis = unicode2sjis_char(*pwszSrc++);
		if (sjis & 0xff00)
			*p++ = (sjis >> 8) & 0xff;
		*p++ = sjis & 0xff;
	}
	*p = '\0';
	if (pnLen)
		*pnLen = p - str.get();
	return str;
#else
	int nSize = ::WideCharToMultiByte(CP_ACP, 0, pwszSrc, nLen, 0, 0, 0, 0);
	string_ptr str(allocString(nSize + 1));
	nSize = ::WideCharToMultiByte(CP_ACP, 0,
		pwszSrc, nLen, str.get(), nSize, 0, 0);
	str[nSize] = '\0';
	if (pnLen)
		*pnLen = nSize;
	return str;
#endif
}

QSEXPORTPROC tstring_ptr qs::wcs2tcs(const WCHAR* pwszSrc)
{
	return wcs2tcs(pwszSrc, static_cast<size_t>(-1), 0);
}

QSEXPORTPROC tstring_ptr qs::wcs2tcs(const WCHAR* pwszSrc,
									 size_t nLen)
{
	return wcs2tcs(pwszSrc, nLen, 0);
}

QSEXPORTPROC tstring_ptr qs::wcs2tcs(const WCHAR* pwszSrc,
									 size_t nLen,
									 size_t* pnLen)
{
#ifdef UNICODE
	if (pnLen)
		*pnLen = nLen == static_cast<size_t>(-1) ? wcslen(pwszSrc) : nLen;
	return allocWString(pwszSrc, nLen);
#else
	return wcs2mbs(pwszSrc, nLen, pnLen);
#endif
}

QSEXPORTPROC wstring_ptr qs::mbs2wcs(const CHAR* pszSrc)
{
	return mbs2wcs(pszSrc, static_cast<size_t>(-1), 0);
}

QSEXPORTPROC wstring_ptr qs::mbs2wcs(const CHAR* pszSrc,
									 size_t nLen)
{
	return mbs2wcs(pszSrc, nLen, 0);
}

QSEXPORTPROC wstring_ptr qs::mbs2wcs(const CHAR* pszSrc,
									 size_t nLen,
									 size_t* pnLen)
{
#ifdef QS_KCONVERT
	if (nLen == static_cast<size_t>(-1))
		nLen = strlen(pszSrc);
	const CHAR* pszSrcEnd = pszSrc + nLen;
	wstring_ptr wstr(allocWString(nLen + 1));
	WCHAR* p = wstr.get();
	while (pszSrc < pszSrcEnd) {
		WORD sjis = static_cast<unsigned char>(*pszSrc++);
		if (isLeadByte(static_cast<unsigned char>(sjis))) {
			sjis <<= 8;
			sjis |= static_cast<unsigned char>(*pszSrc++);
		}
		*p++ = sjis2unicode_char(sjis);
	}
	*p = L'\0';
	if (pnLen)
		*pnLen = p - wstr.get();
	return wstr;
#else
	int nSize = ::MultiByteToWideChar(CP_ACP, 0, pszSrc, nLen, 0, 0);
	wstring_ptr wstr(allocWString(nSize + 1));
	nSize = ::MultiByteToWideChar(CP_ACP, 0, pszSrc, nLen, wstr.get(), nSize);
	wstr[nSize] = L'\0';
	if (pnLen)
		*pnLen = nSize;
	return wstr;
#endif
}

QSEXPORTPROC tstring_ptr qs::mbs2tcs(const CHAR* pszSrc)
{
	return mbs2tcs(pszSrc, static_cast<size_t>(-1), 0);
}

QSEXPORTPROC tstring_ptr qs::mbs2tcs(const CHAR* pszSrc,
									 size_t nLen)
{
	return mbs2tcs(pszSrc, nLen, 0);
}

QSEXPORTPROC tstring_ptr qs::mbs2tcs(const CHAR* pszSrc,
									 size_t nLen,
									 size_t* pnLen)
{
#ifdef UNICODE
	return mbs2wcs(pszSrc, nLen, pnLen);
#else
	if (pnLen)
		*pnLen = nLen == static_cast<size_t>(-1) ? strlen(pszSrc) : nLen;
	return allocString(pszSrc, nLen);
#endif
}

QSEXPORTPROC wstring_ptr qs::tcs2wcs(const TCHAR* ptszSrc)
{
	return tcs2wcs(ptszSrc, static_cast<size_t>(-1), 0);
}

QSEXPORTPROC wstring_ptr qs::tcs2wcs(const TCHAR* ptszSrc,
									 size_t nLen)
{
	return tcs2wcs(ptszSrc, nLen, 0);
}

QSEXPORTPROC wstring_ptr qs::tcs2wcs(const TCHAR* ptszSrc,
									 size_t nLen,
									 size_t* pnLen)
{
#ifdef UNICODE
	if (pnLen)
		*pnLen = nLen == static_cast<size_t>(-1) ? _tcslen(ptszSrc) : nLen;
	return allocWString(ptszSrc, nLen);
#else
	return mbs2wcs(ptszSrc, nLen, pnLen);
#endif
}

QSEXPORTPROC string_ptr qs::tcs2mbs(const TCHAR* ptszSrc)
{
	return tcs2mbs(ptszSrc, static_cast<size_t>(-1), 0);
}

QSEXPORTPROC string_ptr qs::tcs2mbs(const TCHAR* ptszSrc,
									size_t nLen)
{
	return tcs2mbs(ptszSrc, nLen, 0);
}

QSEXPORTPROC string_ptr qs::tcs2mbs(const TCHAR* ptszSrc,
									size_t nLen,
									size_t* pnLen)
{
#ifdef UNICODE
	return wcs2mbs(ptszSrc, nLen, pnLen);
#else
	if (pnLen)
		*pnLen = nLen == static_cast<size_t>(-1) ? _tcslen(ptszSrc) : nLen;
	return allocString(ptszSrc, nLen);
#endif
}


/****************************************************************************
 *
 * Converter
 *
 */

qs::Converter::~Converter()
{
}

xstring_size_ptr qs::Converter::encode(const WCHAR* pwsz,
									   size_t* pnLen)
									   QNOTHROW()
{
	XStringBuffer<XSTRING> buf;
	*pnLen = encode(pwsz, *pnLen, &buf);
	if (*pnLen == -1)
		return xstring_size_ptr();
	return buf.getXStringSize();
}

wxstring_size_ptr qs::Converter::decode(const CHAR* psz,
										size_t* pnLen)
										QNOTHROW()
{
	XStringBuffer<WXSTRING> buf;
	*pnLen = decode(psz, *pnLen, &buf);
	if (*pnLen == -1)
		return wxstring_size_ptr();
	return buf.getXStringSize();
}

size_t qs::Converter::encode(const WCHAR* pwsz,
							 size_t nLen,
							 XStringBuffer<XSTRING>* pBuf)
							 QNOTHROW()
{
	return encodeImpl(pwsz, nLen, pBuf);
}

size_t qs::Converter::decode(const CHAR* psz,
							 size_t nLen,
							 XStringBuffer<WXSTRING>* pBuf)
							 QNOTHROW()
{
	return decodeImpl(psz, nLen, pBuf);
}


/****************************************************************************
 *
 * ConverterFactoryImpl
 *
 */

struct qs::ConverterFactoryImpl
{
	typedef std::vector<ConverterFactory*> FactoryMap;
	static FactoryMap* pMap__;
	static class InitializerImpl : public Initializer
	{
	public:
		InitializerImpl();
		virtual ~InitializerImpl();
	
	public:
		virtual bool init();
		virtual void term();
	} init__;
};

ConverterFactoryImpl::FactoryMap* qs::ConverterFactoryImpl::pMap__;
ConverterFactoryImpl::InitializerImpl qs::ConverterFactoryImpl::init__;

qs::ConverterFactoryImpl::InitializerImpl::InitializerImpl()
{
}

qs::ConverterFactoryImpl::InitializerImpl::~InitializerImpl()
{
}

bool qs::ConverterFactoryImpl::InitializerImpl::init()
{
	ConverterFactoryImpl::pMap__ = new FactoryMap();
	return true;
}

void qs::ConverterFactoryImpl::InitializerImpl::term()
{
	delete ConverterFactoryImpl::pMap__;
	ConverterFactoryImpl::pMap__ = 0;
}


/****************************************************************************
 *
 * ConverterFactory
 *
 */

qs::ConverterFactory::ConverterFactory()
{
}

qs::ConverterFactory::~ConverterFactory()
{
}

std::auto_ptr<Converter> qs::ConverterFactory::getInstance(const WCHAR* pwszName)
{
	assert(pwszName);
	
	wstring_ptr wstrLowerName(tolower(pwszName));
	
	typedef ConverterFactoryImpl::FactoryMap Map;
	Map* pMap = ConverterFactoryImpl::pMap__;
	
	Map::iterator it = pMap->begin();
	while (it != pMap->end()) {
		if ((*it)->isSupported(wstrLowerName.get()))
			break;
		++it;
	}
	if (it == pMap->end())
		return 0;
	
	return (*it)->createInstance(pwszName);
}

void qs::ConverterFactory::registerFactory(ConverterFactory* pFactory)
{
	assert(pFactory);
	ConverterFactoryImpl::pMap__->push_back(pFactory);
}

void qs::ConverterFactory::unregisterFactory(ConverterFactory* pFactory)
{
	assert(pFactory);
	
	typedef ConverterFactoryImpl::FactoryMap Map;
	Map* pMap = ConverterFactoryImpl::pMap__;
	
	Map::iterator it = std::remove(pMap->begin(), pMap->end(), pFactory);
	assert(it != pMap->end());
	pMap->erase(it, pMap->end());
}


/****************************************************************************
 *
 * UTF7ConverterImpl
 *
 */

struct qs::UTF7ConverterImpl
{
	bool decode(const CHAR* p,
				const CHAR* pEnd,
				WCHAR** ppDst);
	
	bool bModified_;
	bool bEncoded_;
};

bool qs::UTF7ConverterImpl::decode(const CHAR* p,
								   const CHAR* pEnd,
								   WCHAR** ppDst)
{
	Base64Encoder encoder(false);
	
	size_t n = 0;
	size_t nEncodeLen = pEnd - p;
	malloc_ptr<unsigned char> pBuf(
		static_cast<unsigned char*>(malloc(nEncodeLen + 4)));
	if (!pBuf.get())
		return false;
	for (n = 0; n < nEncodeLen; ++n) {
		if (bModified_ && p[n] == ',')
			*(pBuf.get() + n) = '/';
		else
			*(pBuf.get() + n) = p[n];
	}
	for (; nEncodeLen % 4; ++nEncodeLen)
		*(pBuf.get() + nEncodeLen) = '=';
	
	malloc_size_ptr<unsigned char> decode(encoder.decode(pBuf.get(), nEncodeLen));
	if (!decode.get())
		return false;
	WCHAR* pDst = *ppDst;
	for (n = 0; n < decode.size(); n += 2)
		*pDst++ = ntohs(*reinterpret_cast<const short*>(decode.get() + n));
	*ppDst = pDst;
	
	return true;
}


/****************************************************************************
 *
 * UTF7Converter
 *
 */

qs::UTF7Converter::UTF7Converter(bool bModified) :
	pImpl_(0)
{
	pImpl_ = new UTF7ConverterImpl();
	pImpl_->bModified_ = bModified;
	pImpl_->bEncoded_ = false;
}

qs::UTF7Converter::~UTF7Converter()
{
	delete pImpl_;
	pImpl_ = 0;
}

size_t qs::UTF7Converter::encodeImpl(const WCHAR* pwsz,
									 size_t nLen,
									 XStringBuffer<XSTRING>* pBuf)
{
	assert(pwsz);
	assert(pBuf);
	
	const CHAR cIn = pImpl_->bModified_ ? '&' : '+';
	const CHAR cOut = '-';
	
	XStringBufferLock<XSTRING> lock(pBuf, nLen*4 + 10);
	CHAR* pLock = lock.get();
	if (!pLock)
		return -1;
	CHAR* p = pLock;
	
	Base64Encoder encoder(false);
	
	const WCHAR* pSrc = pwsz;
	const WCHAR* pSrcEnd = pwsz + nLen;
	const WCHAR* pEncodeBegin = 0;
	do {
		if (pEncodeBegin) {
			if (pSrc == pSrcEnd || (0x20 <= *pSrc && *pSrc <= 0x7e)) {
				size_t n = 0;
				*p++ = cIn;
				size_t nEncodeLen = pSrc - pEncodeBegin;
				malloc_ptr<unsigned char> pEncode(
					static_cast<unsigned char*>(malloc(nEncodeLen*sizeof(WCHAR) + 1)));
				if (!pEncode.get())
					return -1;
				for (n = 0; n < nEncodeLen; ++n, ++pEncodeBegin)
					*reinterpret_cast<WCHAR*>(pEncode.get() + n*2) = htons(*pEncodeBegin);
				*(pEncode.get() + nEncodeLen*2) = 0;
				malloc_size_ptr<unsigned char> encoded(encoder.encode(
					pEncode.get(), nEncodeLen*sizeof(WCHAR)));
				if (!encoded.get())
					return -1;
				for (n = 0; n < encoded.size() && encoded[n] != '='; ++n) {
					if (pImpl_->bModified_ && encoded[n] == '/')
						*p++ = ',';
					else
						*p++ = encoded[n];
				}
				*p++ = cOut;
				pEncodeBegin = 0;
			}
		}
		if (!pEncodeBegin && pSrc != pSrcEnd) {
			if (*pSrc == static_cast<WCHAR>(cIn)) {
				*p++ = cIn;
				*p++ = cOut;
			}
			else if (0x20 <= *pSrc && *pSrc <= 0x7e) {
				*p++ = static_cast<char>(*pSrc);
			}
			else {
				pEncodeBegin = pSrc;
			}
		}
		++pSrc;
	} while (pSrc <= pSrcEnd);
	*p = '\0';
	
	lock.unlock(p - pLock);
	
	return nLen;
}

size_t qs::UTF7Converter::decodeImpl(const CHAR* psz,
									 size_t nLen,
									 XStringBuffer<WXSTRING>* pBuf)
{
	assert(psz);
	assert(pBuf);
	
	const CHAR cIn = pImpl_->bModified_ ? '&' : '+';
	const CHAR cOut = '-';
	
	XStringBufferLock<WXSTRING> lock(pBuf, nLen + 1);
	WCHAR* pLock = lock.get();
	if (!pLock)
		return -1;
	WCHAR* pDst = pLock;
	
	const CHAR* pSrc = psz;
	const CHAR* pSrcEnd = psz + nLen;
	const CHAR* pEncodeBegin = 0;
	if (pImpl_->bEncoded_)
		pEncodeBegin = psz;
	
	while (pSrc != pSrcEnd) {
		if (pEncodeBegin) {
			if (*pSrc == cOut) {
				if (!pImpl_->decode(pEncodeBegin, pSrc, &pDst))
					return -1;
				pEncodeBegin = 0;
			}
		}
		else {
			if (*pSrc == cIn) {
				if (pSrc + 1 == pSrcEnd) {
					break;
				}
				else if (*(pSrc + 1) == cOut) {
					*pDst++ = cIn;
					++pSrc;
				}
				else {
					pEncodeBegin = pSrc + 1;
				}
			}
			else {
				*pDst++ = static_cast<WCHAR>(*pSrc);
			}
		}
		++pSrc;
	}
	
	size_t nEaten = 0;
	if (pEncodeBegin) {
		const CHAR* pEncodeEnd = pEncodeBegin;
		while (pEncodeEnd + 8 < pSrc)
			pEncodeEnd += 8;
		if (pEncodeEnd != pEncodeBegin) {
			if (!pImpl_->decode(pEncodeBegin, pEncodeEnd, &pDst))
				return -1;
		}
		pImpl_->bEncoded_ = true;
		nEaten = pEncodeEnd - psz;
	}
	else {
		pImpl_->bEncoded_ = false;
		nEaten = pSrc - psz;
	}
	*pDst = L'\0';
	
	lock.unlock(pDst - pLock);
	
	return nEaten;
}


/****************************************************************************
 *
 * UTF7ConverterFactory
 *
 */

qs::UTF7ConverterFactory::UTF7ConverterFactory()
{
	registerFactory(this);
}

qs::UTF7ConverterFactory::~UTF7ConverterFactory()
{
	unregisterFactory(this);
}

bool qs::UTF7ConverterFactory::isSupported(const WCHAR* pwszName)
{
	assert(pwszName);
	return wcsicmp(pwszName, L"utf-7") == 0;
}

std::auto_ptr<Converter> qs::UTF7ConverterFactory::createInstance(const WCHAR* pwszName)
{
	return new UTF7Converter(false);
}


/****************************************************************************
 *
 * UTF8Converter
 *
 */

qs::UTF8Converter::UTF8Converter()
{
}

qs::UTF8Converter::~UTF8Converter()
{
}

size_t qs::UTF8Converter::encodeImpl(const WCHAR* pwsz,
									 size_t nLen,
									 XStringBuffer<XSTRING>* pBuf)
{
	assert(pwsz);
	assert(pBuf);
	
	XStringBufferLock<XSTRING> lock(pBuf, nLen*3 + 10);
	CHAR* pLock = lock.get();
	if (!pLock)
		return -1;
	CHAR* p = pLock;
	
	const WCHAR* pEnd = pwsz + nLen;
	while (pwsz != pEnd) {
		if (*pwsz < 0x80) {
			*p++ = *pwsz & 0x7f;
		}
		else if (*pwsz < 0x0800) {
			*p++ = 0xc0 + ((*pwsz >> 6) & 0x1f);
			*p++ = 0x80 + (*pwsz & 0x3f);
		}
		else {
			*p++ = 0xe0 + ((*pwsz >> 12) & 0x0f);
			*p++ = 0x80 + ((*pwsz >> 6) & 0x3f);
			*p++ = 0x80 + (*pwsz & 0x3f);
		}
		++pwsz;
	}
	*p = '\0';
	
	lock.unlock(p - pLock);
	
	return nLen;
}

size_t qs::UTF8Converter::decodeImpl(const CHAR* psz,
									 size_t nLen,
									 XStringBuffer<WXSTRING>* pBuf)
{
	assert(psz);
	assert(pBuf);
	
	XStringBufferLock<WXSTRING> lock(pBuf, nLen + 10);
	WCHAR* pLock = lock.get();
	if (!pLock)
		return -1;
	WCHAR* pDst = pLock;
	
	const CHAR* pSrc = psz;
	const CHAR* pSrcEnd = psz + nLen;
	while (pSrc != pSrcEnd) {
		if ((*pSrc & 0x80) == 0) {
			*pDst++ = static_cast<WCHAR>(*pSrc);
		}
		else if ((*pSrc & 0x40) == 0) {
			// Error
		}
		else if ((*pSrc & 0x20) == 0) {
			if (pSrc + 1 == pSrcEnd)
				break;
			
			WCHAR c = (static_cast<WCHAR>(*pSrc) & 0x1f) << 6;
			++pSrc;
			if ((*pSrc & 0xc0) == 0x80) {
				c |= static_cast<WCHAR>(*pSrc) & 0x3f;
			}
			else {
				// Error
				--pSrc;
			}
			*pDst++ = c;
		}
		else if ((*pSrc & 0x10) == 0) {
			if (pSrc + 1 == pSrcEnd || pSrc + 2 == pSrcEnd)
				break;
			
			WCHAR c = (static_cast<WCHAR>(*pSrc) & 0x0f) << 12;
			++pSrc;
			if ((*pSrc & 0xc0) == 0x80) {
				c |= (static_cast<WCHAR>(*pSrc) & 0x3f) << 6;
				++pSrc;
				if ((*pSrc & 0xc0) == 0x80) {
					c |= static_cast<WCHAR>(*pSrc) & 0x3f;
				}
				else {
					// Error
					--pSrc;
				}
			}
			else {
				// Error
				--pSrc;
			}
			*pDst++ = c;
		}
		else {
			// Error
			*pDst++ = static_cast<WCHAR>(*pSrc);
		}
		++pSrc;
	}
	*pDst = L'\0';
	
	lock.unlock(pDst - pLock);
	
	return pSrc - psz;
}


/****************************************************************************
 *
 * UTF8ConverterFactory
 *
 */

qs::UTF8ConverterFactory::UTF8ConverterFactory()
{
	registerFactory(this);
}

qs::UTF8ConverterFactory::~UTF8ConverterFactory()
{
	unregisterFactory(this);
}

bool qs::UTF8ConverterFactory::isSupported(const WCHAR* pwszName)
{
	assert(pwszName);
	return wcsicmp(pwszName, L"utf-8") == 0;
}

std::auto_ptr<Converter> qs::UTF8ConverterFactory::createInstance(const WCHAR* pwszName)
{
	return new UTF8Converter();
}


#ifdef QS_KCONVERT

/****************************************************************************
 *
 * ShiftJISConverter
 *
 */

qs::ShiftJISConverter::ShiftJISConverter()
{
}

qs::ShiftJISConverter::~ShiftJISConverter()
{
}

Converter::Encoded qs::ShiftJISConverter::encode(const WCHAR* pwsz,
												 size_t* pnLen)
{
	assert(pwsz);
	assert(pnLen);
	
	QTRY {
		size_t nLen = 0;
		string_ptr<STRING> str(wcs2mbs(pwsz, *pnLen, &nLen));
		return Encoded(str, nLen);
	}
	QCATCH_ALL() {
		return Encoded();
	}
}

Converter::Decoded qs::ShiftJISConverter::decode(const CHAR* psz,
												 size_t* pnLen)
{
	assert(psz);
	assert(pnLen);
	
	string_ptr<WSTRING> wstr(allocWStringNT(*pnLen + 1));
	if (!wstr.get())
		return Decoded();
	
	const CHAR* pSrc = psz;
	const CHAR* pSrcEnd = psz + *pnLen;
	WCHAR* pDst = wstr.get();
	while (pSrc != pSrcEnd) {
		WORD sjis = static_cast<unsigned char>(*pSrc++);
		if (isLeadByte(static_cast<unsigned char>(sjis))) {
			if (pSrc == pSrcEnd)
				break;
			sjis <<= 8;
			sjis |= static_cast<unsigned char>(*pSrc++);
		}
		*pDst++ = sjis2unicode_char(sjis);
	}
	*pDst = L'\0';
	
	*pnLen = pSrc - psz;
	
	return Decoded(wstr, pDst - wstr.get());
}


/****************************************************************************
 *
 * ShiftJISConverterFactory
 *
 */

qs::ShiftJISConverterFactory::ShiftJISConverterFactory() QTHROW1(std::bad_alloc)
{
	registerFactory(this);
}

qs::ShiftJISConverterFactory::~ShiftJISConverterFactory()
{
	unregisterFactory(this);
}

bool qs::ShiftJISConverterFactory::isSupported(const WCHAR* pwszName)
{
	assert(pwszName);
	return wcsicmp(pwszName, L"shift_jis") == 0 ||
		wcsicmp(pwszName, L"x-sjis") == 0;
}

std::auto_ptr<Converter> qs::ShiftJISConverterFactory::createInstance(const WCHAR* pwszName)
																	  QTHROW1(std::bad_alloc)
{
	return new ShiftJISConverter();
}


/****************************************************************************
 *
 * ISO2022JPConverterImpl
 *
 */

struct qs::ISO2022JPConverterImpl
{
	enum Mode {
		MODE_ASCII,
		MODE_KANJI,
		MODE_KANA
	};
	
	static WORD sjis2jis(WORD w);
	static WORD jis2sjis(WORD w);
	static WORD han2zen(unsigned char b, unsigned char bNext, bool* pbDakuten);
	static bool isLeadByte(unsigned char b);
	static bool isHankakuKana(unsigned char b);
	
	Mode mode_;
};

WORD qs::ISO2022JPConverterImpl::sjis2jis(WORD w)
{
	BYTE bHigh = HIBYTE(w);
	BYTE bLow = LOBYTE(w);
	
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

WORD qs::ISO2022JPConverterImpl::jis2sjis(WORD w)
{
	BYTE bHigh = HIBYTE(w);
	BYTE bLow = LOBYTE(w);
	
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

WORD qs::ISO2022JPConverterImpl::han2zen(unsigned char b,
	unsigned char bNext, bool* pbDakuten)
{
	assert(isHankakuKana(b));
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
	if (b == 0xde) {
		// Dakuten
		return 0x814a;
	}
	else if (b == 0xdf) {
		// Handakuten
		return 0x814b;
	}
	else if (bNext == 0xde) {
		// Dakuten
		if (b == 0xb3) {
			*pbDakuten = true;
			return 0x8394;
		}
		else if ((0xb6 <= b && b <= 0xc4) || (0xca <= b && b <= 0xce)) {
			*pbDakuten = true;
			return hanZen[b - 0xa0] + 0x01;
		}
		else {
			return hanZen[b - 0xa0];
		}
	}
	else if (bNext == 0xdf) {
		// Handakuten
		if (0xca <= b && b <= 0xce) {
			*pbDakuten = true;
			return hanZen[b - 0xa0] + 0x02;
		}
		else {
			return hanZen[b - 0xa0];
		}
	}
	else {
		return hanZen[b - 0xa0];
	}
}

bool qs::ISO2022JPConverterImpl::isLeadByte(unsigned char b)
{
	return (b >= 0x81 && b <= 0x9f) || (b >= 0xe0 && b <= 0xfc);
}

bool qs::ISO2022JPConverterImpl::isHankakuKana(unsigned char b)
{
	return 0xa0 <= b && b <= 0xdf;
}


/****************************************************************************
 *
 * ISO2022JPConverter
 *
 */

qs::ISO2022JPConverter::ISO2022JPConverter() QTHROW1(std::bad_alloc)
{
	pImpl_ = new ISO2022JPConverterImpl();
	pImpl_->mode_ = ISO2022JPConverterImpl::MODE_ASCII;
}

qs::ISO2022JPConverter::~ISO2022JPConverter()
{
	delete pImpl_;
	pImpl_ = 0;
}

Converter::Encoded qs::ISO2022JPConverter::encode(const WCHAR* pwsz,
												  size_t* pnLen)
{
	assert(pwsz);
	assert(pnLen);
	
	string_ptr<STRING> strSJIS;
	QTRY {
		strSJIS = wcs2mbs(pwsz, *pnLen);
	}
	QCATCH_ALL() {
		return Encoded();
	}
	
	const CHAR szKanji[] = { 0x1b, '$', 'B', '\0' };
	const CHAR szAscii[] = { 0x1b, '(', 'B', '\0' };
	ISO2022JPConverterImpl::Mode mode = ISO2022JPConverterImpl::MODE_ASCII;
	
	size_t nJISLen = *pnLen;
	string_ptr<STRING> strJIS(allocStringNT(nJISLen));
	if (!strJIS.get())
		return Encoded();
	
	CHAR* pSJIS = strSJIS.get();
	CHAR* pSJISEnd = pSJIS + strlen(pSJIS);
	CHAR* p = strJIS.get();
	while (pSJIS != pSJISEnd) {
		if (static_cast<size_t>(p - strJIS.get() + 10) > nJISLen) {
			int n = p - strJIS.get();
			strJIS.reset(reallocStringNT(strJIS, nJISLen*2));
			if (!strJIS.get())
				return Encoded();
			p = strJIS.get() + n;
		}
		unsigned char c = *pSJIS;
		if (ISO2022JPConverterImpl::isLeadByte(c)) {
			if (mode == ISO2022JPConverterImpl::MODE_ASCII) {
				strcpy(p, szKanji);
				p += 3;
				mode = ISO2022JPConverterImpl::MODE_KANJI;
			}
			++pSJIS;
			WORD w = c << 8;
			w |= static_cast<unsigned char>(*pSJIS);
			w = ISO2022JPConverterImpl::sjis2jis(w);
			*p++ = HIBYTE(w);
			*p++ = LOBYTE(w);
		}
		else if (ISO2022JPConverterImpl::isHankakuKana(c)) {
			if (mode == ISO2022JPConverterImpl::MODE_ASCII) {
				strcpy(p, szKanji);
				p += 3;
				mode = ISO2022JPConverterImpl::MODE_KANJI;
			}
			bool bNext = false;
			WORD w = ISO2022JPConverterImpl::sjis2jis(
				ISO2022JPConverterImpl::han2zen(c, *(pSJIS + 1), &bNext));
			if (bNext)
				++pSJIS;
			*p++ = HIBYTE(w);
			*p++ = LOBYTE(w);
		}
		else {
			if (mode == ISO2022JPConverterImpl::MODE_KANJI) {
				strcpy(p, szAscii);
				p += 3;
				mode = ISO2022JPConverterImpl::MODE_ASCII;
			}
			*p++ = c;
		}
		++pSJIS;
	}
	if (mode == ISO2022JPConverterImpl::MODE_KANJI) {
		strcpy(p, szAscii);
		p += 3;
		mode = ISO2022JPConverterImpl::MODE_ASCII;
	}
	*p = '\0';
	
	return Encoded(strJIS, p - strJIS.get());
}

Converter::Decoded qs::ISO2022JPConverter::decode(const CHAR* psz,
												  size_t* pnLen)
{
	assert(psz);
	assert(pnLen);
	
	const CHAR szKanji1[] = { 0x1b, '$', 'B', '\0' };
	const CHAR szKanji2[] = { 0x1b, '$', '@', '\0' };
	const CHAR szAscii1[] = { 0x1b, '(', 'B', '\0' };
	const CHAR szAscii2[] = { 0x1b, '(', 'J', '\0' };
	const CHAR szKana[] = { 0x1b, L'(', 'I', '\0' };
	
	string_ptr<STRING> strSJIS(allocStringNT(*pnLen + 1));
	if (!strSJIS.get())
		return Decoded();
	
	const CHAR* pSrc = psz;
	const CHAR* pSrcEnd = psz + *pnLen;
	CHAR* pSJIS = strSJIS.get();
	while (pSrc != pSrcEnd) {
		unsigned char c = *pSrc;
		if (c == 0x1b) {
			if (pSrc + 2 >= pSrcEnd)
				break;
			
			if (strncmp(pSrc, szKanji1, 3) == 0 ||
				strncmp(pSrc, szKanji2, 3) == 0) {
				pSrc += 2;
				pImpl_->mode_ = ISO2022JPConverterImpl::MODE_KANJI;
			}
			else if (strncmp(pSrc, szAscii1, 3) == 0 ||
				strncmp(pSrc, szAscii2, 3) == 0) {
				pSrc += 2;
				pImpl_->mode_ = ISO2022JPConverterImpl::MODE_ASCII;
			}
			else if (strncmp(pSrc, szKana, 3) == 0) {
				pSrc += 2;
				pImpl_->mode_ = ISO2022JPConverterImpl::MODE_KANA;
			}
			else {
				*pSJIS++ = c;
			}
		}
		else if (pImpl_->mode_ == ISO2022JPConverterImpl::MODE_ASCII) {
			*pSJIS++ = c;
		}
		else if (pImpl_->mode_ == ISO2022JPConverterImpl::MODE_KANA) {
			WORD w = ISO2022JPConverterImpl::jis2sjis(c);
			*pSJIS++ = LOBYTE(w);
		}
		else {
			if (pSrc + 1 == pSrcEnd)
				break;
			
			WORD w = ISO2022JPConverterImpl::jis2sjis(static_cast<WORD>(c) << 8 |
				static_cast<unsigned char>(*(++pSrc)));
			*pSJIS++ = HIBYTE(w);
			*pSJIS++ = LOBYTE(w);
		}
		++pSrc;
	}
	*pSJIS = '\0';
	
	*pnLen = pSrc - psz;
	
	QTRY {
		size_t nLen = 0;
		string_ptr<WSTRING> wstr(mbs2wcs(strSJIS.get(), pSJIS - strSJIS.get(), &nLen);
		return Decoded(wstr, nLen);
	}
	QCATCH_ALL() {
		return Decoded();
	}
}


/****************************************************************************
 *
 * ISO2022JPConverterFactory
 *
 */

qs::ISO2022JPConverterFactory::ISO2022JPConverterFactory() QTHROW1(std::bad_alloc)
{
	registerFactory(this);
}

qs::ISO2022JPConverterFactory::~ISO2022JPConverterFactory()
{
	unregisterFactory(this);
}

bool qs::ISO2022JPConverterFactory::isSupported(const WCHAR* pwszName)
{
	return wcsnicmp(pwszName, L"iso-2022-jp", 11) == 0;
}

std::auto_ptr<Converter> qs::ISO2022JPConverterFactory::createInstance(const WCHAR* pwszName)
																	   QTHROW1(std::bad_alloc)
{
	return new ISO2022JPConverter();
}


/****************************************************************************
 *
 * EUCJPConverter
 *
 */

qs::EUCJPConverter::EUCJPConverter()
{
}

qs::EUCJPConverter::~EUCJPConverter()
{
}

Converter::Encoded qs::EUCJPConverter::encode(const WCHAR* pwsz,
											  size_t* pnLen)
{
	assert(pwsz);
	assert(pnLen);
	
	string_ptr<STRING> strSJIS;
	QTRY {
		strSJIS = wcs2mbs(pwsz, *pnLen);
	}
	QCATCH_ALL() {
		return Encoded();
	}
	size_t nSJISLen = strlen(strSJIS.get());
	
	string_ptr<STRING> strEUC(allocStringNT(nSJISLen*2 + 1));
	if (!strEUC.get())
		return Encoded();
	
	const CHAR* pSJIS = strSJIS.get();
	const CHAR* pSJISEnd = pSJIS + nSJISLen;
	CHAR* pEUC = strEUC.get();
	while (pSJIS != pSJISEnd) {
		unsigned char c = *pSJIS;
		if (ISO2022JPConverterImpl::isLeadByte(c)) {
			++pSJIS;
			if (pSJIS == pSJISEnd)
				break;
			unsigned char cNext = *pSJIS;
			WORD w = c << 8;
			w |= cNext;
			w = ISO2022JPConverterImpl::sjis2jis(w);
			*pEUC++ = static_cast<unsigned char>(HIBYTE(w) | 0x80);
			*pEUC++ = static_cast<unsigned char>(LOBYTE(w) | 0x80);
		}
		else if (ISO2022JPConverterImpl::isHankakuKana(c)) {
			*pEUC++ = static_cast<unsigned char>(0x8e);
			*pEUC++ = c;
		}
		else {
			*pEUC++ = *pSJIS;
		}
		++pSJIS;
	}
	*pEUC = '\0';
	
	return Encoded(strEUC, pEUC - strEUC.get());
}

Converter::Decoded qs::EUCJPConverter::decode(const CHAR* psz,
											  size_t* pnLen)
{
	assert(psz);
	assert(pnLen);
	
	string_ptr<STRING> strSJIS(allocStringNT(*pnLen + 1));
	if (!strSJIS.get())
		return Decoded();
	
	const CHAR* pSrc = psz;
	const CHAR* pSrcEnd = psz + *pnLen;
	CHAR* pSJIS = strSJIS.get();
	while (pSrc != pSrcEnd) {
		unsigned char c = *pSrc;
		if (c == 0x8e) {
			if (pSrc + 1 == pSrcEnd)
				break;
			++pSrc;
			*pSJIS++ = *pSrc;
		}
		else if (c & 0x80) {
			if (pSrc + 1 == pSrcEnd)
				break;
			++pSrc;
			unsigned char cNext = *pSrc;
			WORD w = ISO2022JPConverterImpl::jis2sjis(
				static_cast<WORD>(c & 0x7f) << 8 | (cNext & 0x7f));
			*pSJIS++ = HIBYTE(w);
			*pSJIS++ = LOBYTE(w);
		}
		else {
			*pSJIS++ = c;
		}
		++pSrc;
	}
	*pSJIS = '\0';
	
	*pnLen = pSrc - psz;
	
	QTRY {
		size_t nLen = 0;
		string_ptr<WSTRING> wstr(mbs2wcs(strSJIS.get(), pSJIS - strSJIS.get(), &nLen));
		return Decoded(wstr, nLen);
	}
	QCATCH_ALL() {
		return Decoded();
	}
}


/****************************************************************************
 *
 * EUCJPConverterFactory
 *
 */

qs::EUCJPConverterFactory::EUCJPConverterFactory() QTHROW1(std::bad_alloc)
{
	registerFactory(this);
}

qs::EUCJPConverterFactory::~EUCJPConverterFactory()
{
	unregisterFactory(this);
}

bool qs::EUCJPConverterFactory::isSupported(const WCHAR* pwszName)
{
	return wcsicmp(pwszName, L"euc-jp") == 0;
}

std::auto_ptr<Converter> qs::EUCJPConverterFactory::createInstance(const WCHAR* pwszName)
																   QTHROW1(std::bad_alloc)
{
	return new EUCJPConverter();
}

#endif // QS_KCONVERT


/****************************************************************************
 *
 * MLangConverterImpl
 *
 */

struct qs::MLangConverterImpl
{
	IMultiLanguage* pMultiLanguage_;
	DWORD dwEncoding_;
};


/****************************************************************************
 *
 * MLangConverter
 *
 */

qs::MLangConverter::MLangConverter(IMultiLanguage* pMultiLanguage,
								   DWORD dwEncoding)
{
	assert(pMultiLanguage);
	assert(dwEncoding != 0);
	
	pImpl_ = new MLangConverterImpl();
	pImpl_->pMultiLanguage_ = pMultiLanguage;
	pImpl_->dwEncoding_ = dwEncoding;
	pImpl_->pMultiLanguage_->AddRef();
}

qs::MLangConverter::~MLangConverter()
{
	if (pImpl_) {
		pImpl_->pMultiLanguage_->Release();
		delete pImpl_;
		pImpl_ = 0;
	}
}

size_t qs::MLangConverter::encodeImpl(const WCHAR* pwsz,
									  size_t nLen,
									  XStringBuffer<XSTRING>* pBuf)
{
	assert(pwsz);
	assert(pBuf);
	
	xstring_ptr str;
	if (nLen == 0)
		return 0;
	
	HRESULT hr = S_OK;
	
	DWORD dwMode = 0;
	UINT nSrcLen = nLen;
	UINT nDstLen = 0;
	hr = pImpl_->pMultiLanguage_->ConvertStringFromUnicode(&dwMode,
		pImpl_->dwEncoding_, const_cast<WCHAR*>(pwsz), &nSrcLen, 0, &nDstLen);
	if (FAILED(hr))
		return -1;
	else if (hr == S_FALSE)
		return 0;
	
	XStringBufferLock<XSTRING> lock(pBuf, nDstLen + 1);
	CHAR* pLock = lock.get();
	if (!pLock)
		return -1;
	
	dwMode = 0;
	hr = pImpl_->pMultiLanguage_->ConvertStringFromUnicode(&dwMode,
		pImpl_->dwEncoding_, const_cast<WCHAR*>(pwsz), &nSrcLen, pLock, &nDstLen);
	if (hr != S_OK)
		return -1;
	
	lock.unlock(nDstLen);
	
	return nSrcLen;
}

size_t qs::MLangConverter::decodeImpl(const CHAR* psz,
									  size_t nLen,
									  XStringBuffer<WXSTRING>* pBuf)
{
	assert(psz);
	assert(pBuf);
	
	wxstring_ptr wstr;
	if (nLen == 0)
		return 0;
	
	HRESULT hr = S_OK;
	
	DWORD dwMode = 0;
	UINT nSrcLen = nLen;
	UINT nDstLen = 0;
	hr = pImpl_->pMultiLanguage_->ConvertStringToUnicode(&dwMode,
		pImpl_->dwEncoding_, const_cast<CHAR*>(psz), &nSrcLen, 0, &nDstLen);
	if (FAILED(hr))
		return -1;
	else if (hr == S_FALSE)
		return 0;
	
	XStringBufferLock<WXSTRING> lock(pBuf, nDstLen + 1);
	WCHAR* pLock = lock.get();
	if (!pLock)
		return -1;
	
	if (hr == S_OK) {
		dwMode = 0;
		hr = pImpl_->pMultiLanguage_->ConvertStringToUnicode(&dwMode,
			pImpl_->dwEncoding_, const_cast<CHAR*>(psz), &nSrcLen, pLock, &nDstLen);
		if (hr != S_OK)
			return -1;
	}
	
	lock.unlock(nDstLen);
	
	return nSrcLen;
}


/****************************************************************************
 *
 * MLangConverterFactoryImpl
 *
 */

struct qs::MLangConverterFactoryImpl
{
	IMultiLanguage* pMultiLanguage_;
	
	DWORD getEncoding(const WCHAR* pwszName);
};

DWORD qs::MLangConverterFactoryImpl::getEncoding(const WCHAR* pwszName)
{
	BSTRPtr bstrCharset(::SysAllocString(pwszName));
	MIMECSETINFO charset;
	HRESULT hr = pMultiLanguage_->GetCharsetInfo(bstrCharset.get(), &charset);
	return FAILED(hr) ? 0 : charset.uiInternetEncoding;
}


/****************************************************************************
 *
 * MLangConverterFactory
 *
 */

qs::MLangConverterFactory::MLangConverterFactory()
{
	pImpl_ = new MLangConverterFactoryImpl();
	pImpl_->pMultiLanguage_ = 0;
	
	HRESULT hr = ::CoCreateInstance(CLSID_CMultiLanguage, 0, CLSCTX_ALL,
		IID_IMultiLanguage, reinterpret_cast<void**>(&pImpl_->pMultiLanguage_));
	if (FAILED(hr))
		return;
	
	registerFactory(this);
}

qs::MLangConverterFactory::~MLangConverterFactory()
{
	unregisterFactory(this);
	
	if (pImpl_) {
		if (pImpl_->pMultiLanguage_)
			pImpl_->pMultiLanguage_->Release();
		delete pImpl_;
		pImpl_ = 0;
	}
}

bool qs::MLangConverterFactory::isSupported(const WCHAR* pwszName)
{
	assert(pwszName);
	
	if (!pImpl_->pMultiLanguage_)
		return false;
	
	DWORD dwEncoding = pImpl_->getEncoding(pwszName);
	if (dwEncoding == 0)
		return false;
	
	return pImpl_->pMultiLanguage_->IsConvertible(dwEncoding, 1200) == S_OK;
}

std::auto_ptr<Converter> qs::MLangConverterFactory::createInstance(const WCHAR* pwszName)
{
	DWORD dwEncoding = pImpl_->getEncoding(pwszName);
	if (dwEncoding == 0)
		return 0;
	return new MLangConverter(pImpl_->pMultiLanguage_, dwEncoding);
}
