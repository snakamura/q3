/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsconv.h>
#include <qsencoder.h>
#include <qserror.h>
#include <qsinit.h>
#include <qsnew.h>
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

QSEXPORTPROC STRING qs::wcs2mbs(const WCHAR* pwszSrc)
{
	return wcs2mbs(pwszSrc, static_cast<size_t>(-1), 0);
}

QSEXPORTPROC STRING qs::wcs2mbs(const WCHAR* pwszSrc, size_t nLen)
{
	return wcs2mbs(pwszSrc, nLen, 0);
}

QSEXPORTPROC STRING qs::wcs2mbs(const WCHAR* pwszSrc, size_t nLen, size_t* pnLen)
{
#ifdef QS_KCONVERT
	if (nLen == static_cast<size_t>(-1))
		nLen = wcslen(pwszSrc);
	const WCHAR* pwszSrcEnd = pwszSrc + nLen;
	string_ptr<STRING> str(allocString(nLen*2 + 1));
	if (!str.get())
		return 0;
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
	return str.release();
#else
	int nSize = ::WideCharToMultiByte(CP_ACP, 0, pwszSrc, nLen, 0, 0, 0, 0);
	string_ptr<STRING> str(allocString(nSize + 1));
	if (!str.get())
		return 0;
	nSize = ::WideCharToMultiByte(CP_ACP, 0, pwszSrc, nLen, str.get(), nSize, 0, 0);
	str[nSize] = '\0';
	if (pnLen)
		*pnLen = nSize;
	return str.release();
#endif
}

QSEXPORTPROC TSTRING qs::wcs2tcs(const WCHAR* pwszSrc)
{
	return wcs2tcs(pwszSrc, static_cast<size_t>(-1), 0);
}

QSEXPORTPROC TSTRING qs::wcs2tcs(const WCHAR* pwszSrc, size_t nLen)
{
	return wcs2tcs(pwszSrc, nLen, 0);
}

QSEXPORTPROC TSTRING qs::wcs2tcs(const WCHAR* pwszSrc, size_t nLen, size_t* pnLen)
{
#ifdef UNICODE
	if (pnLen)
		*pnLen = nLen == static_cast<size_t>(-1) ? wcslen(pwszSrc) : nLen;
	return allocWString(pwszSrc, nLen);
#else
	return wcs2mbs(pwszSrc, nLen, pnLen);
#endif
}

QSEXPORTPROC WSTRING qs::mbs2wcs(const CHAR* pszSrc)
{
	return mbs2wcs(pszSrc, static_cast<size_t>(-1), 0);
}

QSEXPORTPROC WSTRING qs::mbs2wcs(const CHAR* pszSrc, size_t nLen)
{
	return mbs2wcs(pszSrc, nLen, 0);
}

QSEXPORTPROC WSTRING qs::mbs2wcs(const CHAR* pszSrc, size_t nLen, size_t* pnLen)
{
#ifdef QS_KCONVERT
	if (nLen == static_cast<size_t>(-1))
		nLen = strlen(pszSrc);
	const CHAR* pszSrcEnd = pszSrc + nLen;
	string_ptr<WSTRING> wstr(allocWString(nLen + 1));
	if (!wstr.get())
		return 0;
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
	return wstr.release();
#else
	int nSize = ::MultiByteToWideChar(CP_ACP, 0, pszSrc, nLen, 0, 0);
	string_ptr<WSTRING> wstr(allocWString(nSize + 1));
	if (!wstr.get())
		return 0;
	nSize = ::MultiByteToWideChar(CP_ACP, 0, pszSrc, nLen, wstr.get(), nSize);
	wstr[nSize] = L'\0';
	if (pnLen)
		*pnLen = nSize;
	return wstr.release();
#endif
}

QSEXPORTPROC TSTRING qs::mbs2tcs(const CHAR* pszSrc)
{
	return mbs2tcs(pszSrc, static_cast<size_t>(-1), 0);
}

QSEXPORTPROC TSTRING qs::mbs2tcs(const CHAR* pszSrc, size_t nLen)
{
	return mbs2tcs(pszSrc, nLen, 0);
}

QSEXPORTPROC TSTRING qs::mbs2tcs(const CHAR* pszSrc, size_t nLen, size_t* pnLen)
{
#ifdef UNICODE
	return mbs2wcs(pszSrc, nLen, pnLen);
#else
	if (pnLen)
		*pnLen = nLen == static_cast<size_t>(-1) ? strlen(pszSrc) : nLen;
	return allocString(pszSrc, nLen);
#endif
}

QSEXPORTPROC WSTRING qs::tcs2wcs(const TCHAR* ptszSrc)
{
	return tcs2wcs(ptszSrc, static_cast<size_t>(-1), 0);
}

QSEXPORTPROC WSTRING qs::tcs2wcs(const TCHAR* ptszSrc, size_t nLen)
{
	return tcs2wcs(ptszSrc, nLen, 0);
}

QSEXPORTPROC WSTRING qs::tcs2wcs(const TCHAR* ptszSrc, size_t nLen, size_t* pnLen)
{
#ifdef UNICODE
	if (pnLen)
		*pnLen = nLen == static_cast<size_t>(-1) ? _tcslen(ptszSrc) : nLen;
	return allocWString(ptszSrc, nLen);
#else
	return mbs2wcs(ptszSrc, nLen, pnLen);
#endif
}

QSEXPORTPROC STRING qs::tcs2mbs(const TCHAR* ptszSrc)
{
	return tcs2mbs(ptszSrc, static_cast<size_t>(-1), 0);
}

QSEXPORTPROC STRING qs::tcs2mbs(const TCHAR* ptszSrc, size_t nLen)
{
	return tcs2mbs(ptszSrc, nLen, 0);
}

QSEXPORTPROC STRING qs::tcs2mbs(const TCHAR* ptszSrc, size_t nLen, size_t* pnLen)
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
		virtual QSTATUS init();
		virtual QSTATUS term();
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

QSTATUS qs::ConverterFactoryImpl::InitializerImpl::init()
{
	DECLARE_QSTATUS();
	
	status = newObject(&ConverterFactoryImpl::pMap__);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::ConverterFactoryImpl::InitializerImpl::term()
{
	delete ConverterFactoryImpl::pMap__;
	ConverterFactoryImpl::pMap__ = 0;
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * ConverterFactory
 *
 */

qs::ConverterFactory::ConverterFactory(QSTATUS* pstatus)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qs::ConverterFactory::~ConverterFactory()
{
}

QSTATUS qs::ConverterFactory::getInstance(
	const WCHAR* pwszName, Converter** ppConverter)
{
	assert(pwszName);
	assert(ppConverter);
	
	DECLARE_QSTATUS();
	
	*ppConverter = 0;
	
	string_ptr<WSTRING> wstrLowerName(tolower(pwszName));
	if (!wstrLowerName.get())
		return QSTATUS_OUTOFMEMORY;
	
	typedef ConverterFactoryImpl::FactoryMap Map;
	Map* pMap = ConverterFactoryImpl::pMap__;
	
	Map::iterator it = pMap->begin();
	while (it != pMap->end()) {
		bool bSupported = false;
		status = (*it)->isSupported(wstrLowerName.get(), &bSupported);
		CHECK_QSTATUS();
		if (bSupported)
			break;
		++it;
	}
	if (it == pMap->end())
		return QSTATUS_SUCCESS;
	return (*it)->createInstance(pwszName, ppConverter);
}

QSTATUS qs::ConverterFactory::getInstance(const WCHAR* pwszName,
	std::auto_ptr<Converter>* papConverter)
{
	assert(papConverter);
	
	DECLARE_QSTATUS();
	
	Converter* pConverter = 0;
	status = getInstance(pwszName, &pConverter);
	CHECK_QSTATUS();
	
	papConverter->reset(pConverter);
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::ConverterFactory::regist(ConverterFactory* pFactory)
{
	assert(pFactory);
	return STLWrapper<ConverterFactoryImpl::FactoryMap>
		(*ConverterFactoryImpl::pMap__).push_back(pFactory);
}

QSTATUS qs::ConverterFactory::unregist(ConverterFactory* pFactory)
{
	assert(pFactory);
	
	typedef ConverterFactoryImpl::FactoryMap Map;
	Map* pMap = ConverterFactoryImpl::pMap__;
	
	Map::iterator it = std::remove(pMap->begin(), pMap->end(), pFactory);
	assert(it != pMap->end());
	pMap->erase(it, pMap->end());
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * UTF7ConverterImpl
 *
 */

struct qs::UTF7ConverterImpl
{
	QSTATUS decode(const CHAR* p, const CHAR* pEnd, WCHAR** ppDst);
	
	bool bModified_;
	bool bEncoded_;
};

QSTATUS qs::UTF7ConverterImpl::decode(
	const CHAR* p, const CHAR* pEnd, WCHAR** ppDst)
{
	DECLARE_QSTATUS();
	
	Base64Encoder encoder(false, &status);
	CHECK_QSTATUS();
	
	size_t n = 0;
	size_t nEncodeLen = pEnd - p;
	malloc_ptr<unsigned char> pBuf(
		static_cast<unsigned char*>(malloc(nEncodeLen + 4)));
	if (!pBuf.get())
		return QSTATUS_OUTOFMEMORY;
	for (n = 0; n < nEncodeLen; ++n) {
		if (bModified_ && p[n] == ',')
			*(pBuf.get() + n) = '/';
		else
			*(pBuf.get() + n) = p[n];
	}
	for (; nEncodeLen % 4; ++nEncodeLen)
		*(pBuf.get() + nEncodeLen) = '=';
	unsigned char* pDecode = 0;
	size_t nDecodeLen = 0;
	status = encoder.decode(pBuf.get(), nEncodeLen, &pDecode, &nDecodeLen);
	CHECK_QSTATUS();
	WCHAR* pDst = *ppDst;
	for (n = 0; n < nDecodeLen; n += 2)
		*pDst++ = ntohs(*reinterpret_cast<const short*>(pDecode + n));
	*ppDst = pDst;
	free(pDecode);
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * UTF7Converter
 *
 */

qs::UTF7Converter::UTF7Converter(bool bModified, QSTATUS* pstatus) :
	pImpl_(0)
{
	assert(pstatus);
	
	*pstatus = QSTATUS_SUCCESS;
	
	DECLARE_QSTATUS();
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	pImpl_->bModified_ = bModified;
	pImpl_->bEncoded_ = false;
}

qs::UTF7Converter::~UTF7Converter()
{
	delete pImpl_;
	pImpl_ = 0;
}

QSTATUS qs::UTF7Converter::encode(const WCHAR* pwsz, size_t* pnLen,
	STRING* pstr, size_t* pnResultLen)
{
	assert(pwsz);
	assert(pnLen);
	assert(pstr);
	
	DECLARE_QSTATUS();
	
	const CHAR cIn = pImpl_->bModified_ ? '&' : '+';
	const CHAR cOut = '-';
	
	string_ptr<STRING> str(allocString(*pnLen*4 + 10));
	if (!str.get())
		return QSTATUS_OUTOFMEMORY;
	
	Base64Encoder encoder(false, &status);
	CHECK_QSTATUS();
	
	const WCHAR* pSrc = pwsz;
	const WCHAR* pSrcEnd = pwsz + *pnLen;
	CHAR* p = str.get();
	const WCHAR* pEncodeBegin = 0;
	do {
		if (pEncodeBegin) {
			if (pSrc == pSrcEnd || (0x20 <= *pSrc && *pSrc <= 0x7e)) {
				size_t n = 0;
				*p++ = cIn;
				size_t nEncodeLen = pSrc - pEncodeBegin;
				malloc_ptr<unsigned char> pBuf(
					static_cast<unsigned char*>(malloc(nEncodeLen*sizeof(WCHAR) + 1)));
				if (!pBuf.get())
					return QSTATUS_OUTOFMEMORY;
				for (n = 0; n < nEncodeLen; ++n, ++pEncodeBegin)
					*reinterpret_cast<WCHAR*>(pBuf.get() + n*2) = htons(*pEncodeBegin);
				*(pBuf.get() + nEncodeLen*2) = 0;
				unsigned char* pEncoded = 0;
				size_t nEncodedLen = 0;
				status = encoder.encode(pBuf.get(), nEncodeLen*sizeof(WCHAR),
					&pEncoded, &nEncodedLen);
				CHECK_QSTATUS();
				for (n = 0; n < nEncodedLen && pEncoded[n] != '='; ++n) {
					if (pImpl_->bModified_ && pEncoded[n] == '/')
						*p++ = ',';
					else
						*p++ = pEncoded[n];
				}
				free(pEncoded);
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
	
	if (pnResultLen)
		*pnResultLen = p - str.get();
	*pstr = str.release();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::UTF7Converter::decode(const CHAR* psz, size_t* pnLen,
	WSTRING* pwstr, size_t* pnResultLen)
{
	assert(psz);
	assert(pnLen);
	assert(pwstr);
	
	DECLARE_QSTATUS();
	
	const CHAR cIn = pImpl_->bModified_ ? '&' : '+';
	const CHAR cOut = '-';
	
	string_ptr<WSTRING> wstr(allocWString(*pnLen + 1));
	if (!wstr.get())
		return QSTATUS_OUTOFMEMORY;
	
	const CHAR* pSrc = psz;
	const CHAR* pSrcEnd = psz + *pnLen;
	WCHAR* pDst = wstr.get();
	const CHAR* pEncodeBegin = 0;
	if (pImpl_->bEncoded_)
		pEncodeBegin = psz;
	
	while (pSrc != pSrcEnd) {
		if (pEncodeBegin) {
			if (*pSrc == cOut) {
				status = pImpl_->decode(pEncodeBegin, pSrc, &pDst);
				CHECK_QSTATUS();
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
	if (pEncodeBegin) {
		const CHAR* pEncodeEnd = pEncodeBegin;
		while (pEncodeEnd + 8 < pSrc)
			pEncodeEnd += 8;
		if (pEncodeEnd != pEncodeBegin) {
			status = pImpl_->decode(pEncodeBegin, pEncodeEnd, &pDst);
			CHECK_QSTATUS();
		}
		pImpl_->bEncoded_ = true;
		*pnLen = pEncodeEnd - psz;
	}
	else {
		pImpl_->bEncoded_ = false;
		*pnLen = pSrc - psz;
	}
	*pDst = L'\0';
	
	if (pnResultLen)
		*pnResultLen = pDst - wstr.get();
	*pwstr = wstr.release();
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * UTF7ConverterFactory
 *
 */

qs::UTF7ConverterFactory::UTF7ConverterFactory(QSTATUS* pstatus) :
	ConverterFactory(pstatus)
{
	assert(pstatus);
	
	if (*pstatus != QSTATUS_SUCCESS)
		return;
	
	*pstatus = regist(this);
}

qs::UTF7ConverterFactory::~UTF7ConverterFactory()
{
	unregist(this);
}

QSTATUS qs::UTF7ConverterFactory::isSupported(
	const WCHAR* pwszName, bool* pbSupported)
{
	assert(pwszName);
	assert(pbSupported);
	
	*pbSupported = wcsicmp(pwszName, L"utf-7") == 0;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::UTF7ConverterFactory::createInstance(
	const WCHAR* pwszName, Converter** ppConverter)
{
	assert(ppConverter);
	
	UTF7Converter* pConverter = 0;
	QSTATUS status = newQsObject(false, &pConverter);
	*ppConverter = pConverter;
	
	return status;
}


/****************************************************************************
 *
 * UTF8Converter
 *
 */

qs::UTF8Converter::UTF8Converter(QSTATUS* pstatus)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qs::UTF8Converter::~UTF8Converter()
{
}

QSTATUS qs::UTF8Converter::encode(const WCHAR* pwsz, size_t* pnLen,
	STRING* pstr, size_t* pnResultLen)
{
	assert(pwsz);
	assert(pstr);
	
	string_ptr<STRING> str(allocString(*pnLen*3 + 10));
	if (!str.get())
		return QSTATUS_OUTOFMEMORY;
	
	const WCHAR* pEnd = pwsz + *pnLen;
	CHAR* p = str.get();
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
	
	if (pnResultLen)
		*pnResultLen = p - str.get();
	*pstr = str.release();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::UTF8Converter::decode(const CHAR* psz, size_t* pnLen,
	WSTRING* pwstr, size_t* pnResultLen)
{
	assert(psz);
	assert(pwstr);
	
	string_ptr<WSTRING> wstr(allocWString(*pnLen + 10));
	if (!wstr.get())
		return QSTATUS_OUTOFMEMORY;
	
	const CHAR* pSrc = psz;
	const CHAR* pSrcEnd = psz + *pnLen;
	WCHAR* pDst = wstr.get();
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
	
	*pnLen = pSrc - psz;
	if (pnResultLen)
		*pnResultLen = pDst - wstr.get();
	*pwstr = wstr.release();
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * UTF8ConverterFactory
 *
 */

qs::UTF8ConverterFactory::UTF8ConverterFactory(QSTATUS* pstatus) :
	ConverterFactory(pstatus)
{
	assert(pstatus);
	
	if (*pstatus != QSTATUS_SUCCESS)
		return;
	
	*pstatus = regist(this);
}

qs::UTF8ConverterFactory::~UTF8ConverterFactory()
{
	unregist(this);
}

QSTATUS qs::UTF8ConverterFactory::isSupported(
	const WCHAR* pwszName, bool* pbSupported)
{
	assert(pwszName);
	assert(pbSupported);
	
	*pbSupported = wcsicmp(pwszName, L"utf-8") == 0;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::UTF8ConverterFactory::createInstance(
	const WCHAR* pwszName, Converter** ppConverter)
{
	assert(ppConverter);
	
	UTF8Converter* pConverter = 0;
	QSTATUS status = newQsObject(&pConverter);
	*ppConverter = pConverter;
	
	return status;
}


#ifdef QS_KCONVERT

/****************************************************************************
 *
 * ShiftJISConverter
 *
 */

qs::ShiftJISConverter::ShiftJISConverter(QSTATUS* pstatus)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qs::ShiftJISConverter::~ShiftJISConverter()
{
}

QSTATUS qs::ShiftJISConverter::encode(const WCHAR* pwsz, size_t* pnLen,
	STRING* pstr, size_t* pnResultLen)
{
	assert(pwsz);
	assert(pnLen);
	assert(pstr);
	
	string_ptr<STRING> str(wcs2mbs(pwsz, *pnLen, pnResultLen));
	if (!str.get())
		return QSTATUS_OUTOFMEMORY;
	*pstr = str.release();
	return QSTATUS_SUCCESS;
}

QSTATUS qs::ShiftJISConverter::decode(const CHAR* psz, size_t* pnLen,
	WSTRING* pwstr, size_t* pnResultLen)
{
	assert(psz);
	assert(pnLen);
	assert(pwstr);
	
	string_ptr<WSTRING> wstr(allocWString(*pnLen + 1));
	if (!wstr.get())
		return QSTATUS_OUTOFMEMORY;
	
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
	if (pnResultLen)
		*pnResultLen = pDst - wstr.get();
	*pwstr = wstr.release();
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * ShiftJISConverterFactory
 *
 */

qs::ShiftJISConverterFactory::ShiftJISConverterFactory(QSTATUS* pstatus) :
	ConverterFactory(pstatus)
{
	assert(pstatus);
	
	if (*pstatus != QSTATUS_SUCCESS)
		return;
	
	*pstatus = regist(this);
}

qs::ShiftJISConverterFactory::~ShiftJISConverterFactory()
{
	unregist(this);
}

QSTATUS qs::ShiftJISConverterFactory::isSupported(
	const WCHAR* pwszName, bool* pbSupported)
{
	assert(pwszName);
	assert(pbSupported);
	
	*pbSupported = wcsicmp(pwszName, L"shift_jis") == 0 ||
		wcsicmp(pwszName, L"x-sjis") == 0;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::ShiftJISConverterFactory::createInstance(
	const WCHAR* pwszName, Converter** ppConverter)
{
	assert(ppConverter);
	
	ShiftJISConverter* pConverter = 0;
	QSTATUS status = newQsObject(&pConverter);
	*ppConverter = pConverter;
	
	return status;
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

qs::ISO2022JPConverter::ISO2022JPConverter(QSTATUS* pstatus)
{
	assert(pstatus);
	
	*pstatus = QSTATUS_SUCCESS;
	
	DECLARE_QSTATUS();
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	pImpl_->mode_ = ISO2022JPConverterImpl::MODE_ASCII;
}

qs::ISO2022JPConverter::~ISO2022JPConverter()
{
	delete pImpl_;
	pImpl_ = 0;
}

QSTATUS qs::ISO2022JPConverter::encode(const WCHAR* pwsz, size_t* pnLen,
	STRING* pstr, size_t* pnResultLen)
{
	assert(pwsz);
	assert(pstr);
	
	string_ptr<STRING> strSJIS(wcs2mbs(pwsz, *pnLen));
	if (!strSJIS.get())
		return QSTATUS_OUTOFMEMORY;
	
	const CHAR szKanji[] = { 0x1b, '$', 'B', '\0' };
	const CHAR szAscii[] = { 0x1b, '(', 'B', '\0' };
	ISO2022JPConverterImpl::Mode mode = ISO2022JPConverterImpl::MODE_ASCII;
	
	size_t nJISLen = *pnLen;
	string_ptr<STRING> strJIS(allocString(nJISLen));
	
	CHAR* pSJIS = strSJIS.get();
	CHAR* pSJISEnd = pSJIS + strlen(pSJIS);
	CHAR* p = strJIS.get();
	while (pSJIS != pSJISEnd) {
		if (static_cast<size_t>(p - strJIS.get() + 10) > nJISLen) {
			int n = p - strJIS.get();
			strJIS.reset(reallocString(strJIS.release(), nJISLen*2));
			if (!strJIS.get())
				return QSTATUS_OUTOFMEMORY;
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
	
	if (pnResultLen)
		*pnResultLen = p - strJIS.get();
	*pstr = strJIS.release();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::ISO2022JPConverter::decode(const CHAR* psz, size_t* pnLen,
	WSTRING* pwstr, size_t* pnResultLen)
{
	assert(psz);
	assert(pwstr);
	
	const CHAR szKanji1[] = { 0x1b, '$', 'B', '\0' };
	const CHAR szKanji2[] = { 0x1b, '$', '@', '\0' };
	const CHAR szAscii1[] = { 0x1b, '(', 'B', '\0' };
	const CHAR szAscii2[] = { 0x1b, '(', 'J', '\0' };
	const CHAR szKana[] = { 0x1b, L'(', 'I', '\0' };
	
	string_ptr<STRING> strSJIS(allocString(*pnLen + 1));
	if (!strSJIS.get())
		return QSTATUS_OUTOFMEMORY;
	
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
	
	string_ptr<WSTRING> wstr(mbs2wcs(strSJIS.get(), pSJIS - strSJIS.get(), pnResultLen));
	if (!wstr.get())
		return QSTATUS_OUTOFMEMORY;
	*pwstr = wstr.release();
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * ISO2022JPConverterFactory
 *
 */

qs::ISO2022JPConverterFactory::ISO2022JPConverterFactory(QSTATUS* pstatus) :
	ConverterFactory(pstatus)
{
	assert(pstatus);
	
	if (*pstatus != QSTATUS_SUCCESS)
		return;
	
	*pstatus = regist(this);
}

qs::ISO2022JPConverterFactory::~ISO2022JPConverterFactory()
{
	unregist(this);
}

QSTATUS qs::ISO2022JPConverterFactory::isSupported(
	const WCHAR* pwszName, bool* pbSupported)
{
	assert(pwszName);
	assert(pbSupported);
	
	*pbSupported = wcsnicmp(pwszName, L"iso-2022-jp", 11) == 0;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::ISO2022JPConverterFactory::createInstance(
	const WCHAR* pwszName, Converter** ppConverter)
{
	assert(ppConverter);
	
	ISO2022JPConverter* pConverter = 0;
	QSTATUS status = newQsObject(&pConverter);
	*ppConverter = pConverter;
	
	return status;
}


/****************************************************************************
 *
 * EUCJPConverter
 *
 */

qs::EUCJPConverter::EUCJPConverter(QSTATUS* pstatus)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qs::EUCJPConverter::~EUCJPConverter()
{
}

QSTATUS qs::EUCJPConverter::encode(const WCHAR* pwsz, size_t* pnLen,
	STRING* pstr, size_t* pnResultLen)
{
	assert(pwsz);
	assert(pstr);
	
	string_ptr<STRING> strSJIS(wcs2mbs(pwsz, *pnLen));
	if (!strSJIS.get())
		return QSTATUS_OUTOFMEMORY;
	size_t nSJISLen = strlen(strSJIS.get());
	
	string_ptr<STRING> strEUC(allocString(nSJISLen*2 + 1));
	if (!strEUC.get())
		return QSTATUS_OUTOFMEMORY;
	
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
	
	if (pnResultLen)
		*pnResultLen = pEUC - strEUC.get();
	*pstr = strEUC.release();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::EUCJPConverter::decode(const CHAR* psz, size_t* pnLen,
	WSTRING* pwstr, size_t* pnResultLen)
{
	assert(psz);
	assert(pwstr);
	
	string_ptr<STRING> strSJIS(allocString(*pnLen + 1));
	if (!strSJIS.get())
		return QSTATUS_OUTOFMEMORY;
	
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
	
	string_ptr<WSTRING> wstr(mbs2wcs(strSJIS.get(), pSJIS - strSJIS.get(), pnResultLen));
	if (!wstr.get())
		return QSTATUS_OUTOFMEMORY;
	*pwstr = wstr.release();
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * EUCJPConverterFactory
 *
 */

qs::EUCJPConverterFactory::EUCJPConverterFactory(QSTATUS* pstatus) :
	ConverterFactory(pstatus)
{
	assert(pstatus);
	
	if (*pstatus != QSTATUS_SUCCESS)
		return;
	
	*pstatus = regist(this);
}

qs::EUCJPConverterFactory::~EUCJPConverterFactory()
{
	unregist(this);
}

QSTATUS qs::EUCJPConverterFactory::isSupported(
	const WCHAR* pwszName, bool* pbSupported)
{
	assert(pwszName);
	assert(pbSupported);
	
	*pbSupported = wcsicmp(pwszName, L"euc-jp") == 0;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::EUCJPConverterFactory::createInstance(
	const WCHAR* pwszName, Converter** ppConverter)
{
	assert(ppConverter);
	
	EUCJPConverter* pConverter = 0;
	QSTATUS status = newQsObject(&pConverter);
	*ppConverter = pConverter;
	
	return status;
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
	DWORD dwEncoding, QSTATUS* pstatus)
{
	assert(pMultiLanguage);
	assert(dwEncoding != 0);
	assert(pstatus);
	
	*pstatus = QSTATUS_SUCCESS;
	
	DECLARE_QSTATUS();
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	pImpl_->pMultiLanguage_ = pMultiLanguage;
	pImpl_->dwEncoding_ = dwEncoding;
	pImpl_->pMultiLanguage_->AddRef();
}

qs::MLangConverter::~MLangConverter()
{
	if (pImpl_) {
		pImpl_->pMultiLanguage_->Release();
		delete pImpl_;
	}
}

QSTATUS qs::MLangConverter::encode(const WCHAR* pwsz, size_t* pnLen,
	STRING* pstr, size_t* pnResultLen)
{
	assert(pwsz);
	assert(pnLen);
	assert(pstr);
	
	UINT nLen = 0;
	string_ptr<STRING> str;
	if (*pnLen == 0) {
		str.reset(allocString(""));
		if (!str.get())
			return QSTATUS_OUTOFMEMORY;
	}
	else {
		HRESULT hr = S_OK;
		
		DWORD dwMode = 0;
		UINT nSrcLen = *pnLen;
		hr = pImpl_->pMultiLanguage_->ConvertStringFromUnicode(
			&dwMode, pImpl_->dwEncoding_,
			const_cast<WCHAR*>(pwsz), &nSrcLen, 0, &nLen);
		if (FAILED(hr))
			return QSTATUS_FAIL;
		else if (hr == S_FALSE)
			nLen = 0;
		
		str.reset(allocString(nLen + 1));
		if (!str.get())
			return QSTATUS_OUTOFMEMORY;
		
		if (hr == S_OK) {
			dwMode = 0;
			hr = pImpl_->pMultiLanguage_->ConvertStringFromUnicode(
				&dwMode, pImpl_->dwEncoding_,
				const_cast<WCHAR*>(pwsz), pnLen, str.get(), &nLen);
			if (hr != S_OK)
				return QSTATUS_FAIL;
		}
		
		*(str.get() + nLen) = '\0';
	}
	*pstr = str.release();
	if (pnResultLen)
		*pnResultLen = nLen;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::MLangConverter::decode(const CHAR* psz, size_t* pnLen,
	WSTRING* pwstr, size_t* pnResultLen)
{
	assert(psz);
	assert(pnLen);
	assert(pwstr);
	
	UINT nLen = 0;
	string_ptr<WSTRING> wstr;
	if (*pnLen == 0) {
		wstr.reset(allocWString(L""));
		if (!wstr.get())
			return QSTATUS_OUTOFMEMORY;
	}
	else {
		HRESULT hr = S_OK;
		
		DWORD dwMode = 0;
		UINT nSrcLen = *pnLen;
		hr = pImpl_->pMultiLanguage_->ConvertStringToUnicode(
			&dwMode, pImpl_->dwEncoding_,
			const_cast<CHAR*>(psz), &nSrcLen, 0, &nLen);
		if (FAILED(hr))
			return QSTATUS_FAIL;
		else if (hr == S_FALSE)
			nLen = 0;
		
		wstr.reset(allocWString(nLen + 1));
		if (!wstr.get())
			return QSTATUS_OUTOFMEMORY;
		
		if (hr == S_OK) {
			dwMode = 0;
			hr = pImpl_->pMultiLanguage_->ConvertStringToUnicode(
				&dwMode, pImpl_->dwEncoding_,
				const_cast<CHAR*>(psz), pnLen, wstr.get(), &nLen);
			if (hr != S_OK)
				return QSTATUS_FAIL;
		}
		*(wstr.get() + nLen) = L'\0';
	}
	*pwstr = wstr.release();
	if (pnResultLen)
		*pnResultLen = nLen;
	
	return QSTATUS_SUCCESS;
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

qs::MLangConverterFactory::MLangConverterFactory(QSTATUS* pstatus) :
	ConverterFactory(pstatus)
{
	assert(pstatus);
	
	if (*pstatus != QSTATUS_SUCCESS)
		return;
	
	DECLARE_QSTATUS();
	
	status = regist(this);
	CHECK_QSTATUS_SET(pstatus);
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	pImpl_->pMultiLanguage_ = 0;
	
	HRESULT hr = ::CoCreateInstance(CLSID_CMultiLanguage, 0, CLSCTX_ALL,
		IID_IMultiLanguage, reinterpret_cast<void**>(&pImpl_->pMultiLanguage_));
	if (FAILED(hr)) {
		*pstatus = QSTATUS_FAIL;
		return;
	}
}

qs::MLangConverterFactory::~MLangConverterFactory()
{
	unregist(this);
	
	if (pImpl_) {
		if (pImpl_->pMultiLanguage_)
			pImpl_->pMultiLanguage_->Release();
		delete pImpl_;
	}
}

QSTATUS qs::MLangConverterFactory::isSupported(
	const WCHAR* pwszName, bool* pbSupported)
{
	assert(pwszName);
	assert(pbSupported);
	
	*pbSupported = false;
	
	if (pImpl_->pMultiLanguage_) {
		DWORD dwEncoding = pImpl_->getEncoding(pwszName);
		if (dwEncoding != 0) {
			HRESULT hr = pImpl_->pMultiLanguage_->IsConvertible(dwEncoding, 1200);
			if (FAILED(hr))
				return QSTATUS_FAIL;
			*pbSupported = hr == S_OK;
		}
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::MLangConverterFactory::createInstance(
	const WCHAR* pwszName, Converter** ppConverter)
{
	assert(ppConverter);
	
	DECLARE_QSTATUS();
	
	DWORD dwEncoding = pImpl_->getEncoding(pwszName);
	if (dwEncoding == 0)
		return QSTATUS_FAIL;
	
	std::auto_ptr<MLangConverter> pConverter;
	status = newQsObject(pImpl_->pMultiLanguage_, dwEncoding, &pConverter);
	CHECK_QSTATUS();
	*ppConverter = pConverter.release();
	
	return QSTATUS_SUCCESS;
}
