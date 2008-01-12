/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#pragma warning(disable:4786)

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

#include <boost/bind.hpp>

#ifdef QS_KCONVERT
#	include <kctrl.h>
#endif

using namespace qs;

#if defined _WIN32_WCE && _WIN32_WCE >= 0x211
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
	return wcs2mbs(pwszSrc, -1, 0);
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
	if (nLen == -1)
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
	int nSize = ::WideCharToMultiByte(CP_ACP, 0,
		pwszSrc, static_cast<int>(nLen), 0, 0, 0, 0);
	string_ptr str(allocString(nSize + 1));
	nSize = ::WideCharToMultiByte(CP_ACP, 0, pwszSrc,
		static_cast<int>(nLen), str.get(), nSize, 0, 0);
	str[nSize] = '\0';
	if (pnLen)
		*pnLen = nSize;
	return str;
#endif
}

QSEXPORTPROC tstring_ptr qs::wcs2tcs(const WCHAR* pwszSrc)
{
	return wcs2tcs(pwszSrc, -1, 0);
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
		*pnLen = nLen == -1 ? wcslen(pwszSrc) : nLen;
	return allocWString(pwszSrc, nLen);
#else
	return wcs2mbs(pwszSrc, nLen, pnLen);
#endif
}

QSEXPORTPROC wstring_ptr qs::mbs2wcs(const CHAR* pszSrc)
{
	return mbs2wcs(pszSrc, -1, 0);
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
	if (nLen == -1)
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
	int nSize = ::MultiByteToWideChar(CP_ACP, 0,
		pszSrc, static_cast<int>(nLen), 0, 0);
	wstring_ptr wstr(allocWString(nSize + 1));
	nSize = ::MultiByteToWideChar(CP_ACP, 0, pszSrc,
		static_cast<int>(nLen), wstr.get(), nSize);
	wstr[nSize] = L'\0';
	if (pnLen)
		*pnLen = nSize;
	return wstr;
#endif
}

QSEXPORTPROC tstring_ptr qs::mbs2tcs(const CHAR* pszSrc)
{
	return mbs2tcs(pszSrc, -1, 0);
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
		*pnLen = nLen == -1 ? strlen(pszSrc) : nLen;
	return allocString(pszSrc, nLen);
#endif
}

QSEXPORTPROC wstring_ptr qs::tcs2wcs(const TCHAR* ptszSrc)
{
	return tcs2wcs(ptszSrc, -1, 0);
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
		*pnLen = nLen == -1 ? _tcslen(ptszSrc) : nLen;
	return allocWString(ptszSrc, nLen);
#else
	return mbs2wcs(ptszSrc, nLen, pnLen);
#endif
}

QSEXPORTPROC string_ptr qs::tcs2mbs(const TCHAR* ptszSrc)
{
	return tcs2mbs(ptszSrc, -1, 0);
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
		*pnLen = nLen == -1 ? _tcslen(ptszSrc) : nLen;
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
	
	static const WCHAR* lookupAlias(const WCHAR* pwszAlias);
	typedef std::vector<std::pair<WSTRING, WSTRING> > AliasMap;
	static AliasMap mapAlias__;
};

ConverterFactoryImpl::FactoryMap* qs::ConverterFactoryImpl::pMap__;
ConverterFactoryImpl::InitializerImpl qs::ConverterFactoryImpl::init__;
ConverterFactoryImpl::AliasMap qs::ConverterFactoryImpl::mapAlias__;

const WCHAR* qs::ConverterFactoryImpl::lookupAlias(const WCHAR* pwszAlias)
{
	AliasMap::value_type alias(const_cast<WSTRING>(pwszAlias), 0);
	AliasMap::const_iterator it = std::lower_bound(
		mapAlias__.begin(), mapAlias__.end(), alias,
		boost::bind(string_less<WCHAR>(),
			boost::bind(&AliasMap::value_type::first, _1),
			boost::bind(&AliasMap::value_type::first, _2)));
	return it != mapAlias__.end() && wcscmp((*it).first, pwszAlias) == 0? (*it).second : 0;
}

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
	typedef ConverterFactoryImpl::AliasMap AliasMap;
	AliasMap& m = ConverterFactoryImpl::mapAlias__;
	for (AliasMap::iterator it = m.begin(); it != m.end(); ++it) {
		freeWString((*it).first);
		freeWString((*it).second);
	}
	m.clear();
	
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
	
	const WCHAR* pwsz = ConverterFactoryImpl::lookupAlias(wstrLowerName.get());
	if (pwsz) {
		pwszName = pwsz;
		wstrLowerName = tolower(pwszName);
	}
	
	typedef ConverterFactoryImpl::FactoryMap Map;
	Map* pMap = ConverterFactoryImpl::pMap__;
	
	Map::iterator it = pMap->begin();
	while (it != pMap->end()) {
		if ((*it)->isSupported(wstrLowerName.get()))
			break;
		++it;
	}
	if (it == pMap->end())
		return std::auto_ptr<Converter>(0);
	
	return (*it)->createInstance(pwszName);
}

void qs::ConverterFactory::addAlias(const WCHAR* pwszAlias,
									const WCHAR* pwszName)
{
	typedef ConverterFactoryImpl::AliasMap AliasMap;
	AliasMap::value_type alias(const_cast<WSTRING>(pwszAlias), 0);
	AliasMap::iterator it = std::lower_bound(
		ConverterFactoryImpl::mapAlias__.begin(),
		ConverterFactoryImpl::mapAlias__.end(), alias,
		boost::bind(string_less<WCHAR>(),
			boost::bind(&AliasMap::value_type::first, _1),
			boost::bind(&AliasMap::value_type::first, _2)));
	if (it != ConverterFactoryImpl::mapAlias__.end() &&
		wcscmp((*it).first, pwszAlias) == 0)
		return;
	
	wstring_ptr wstrAlias(allocWString(pwszAlias));
	wstring_ptr wstrName(allocWString(pwszName));
	ConverterFactoryImpl::mapAlias__.insert(it,
		AliasMap::value_type(wstrAlias.get(), wstrName.get()));
	wstrAlias.release();
	wstrName.release();
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
	enum Type {
		TYPE_D,
		TYPE_O,
		TYPE_E
	};
	
	bool decode(const CHAR* p,
				const CHAR* pEnd,
				WCHAR** ppDst);
	bool isEncodedChar(CHAR c) const;
	bool isEncodingNeeded(WCHAR c) const;
	
	static Type getType(WCHAR c);
	
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
		static_cast<unsigned char*>(allocate(nEncodeLen + 4)));
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

bool qs::UTF7ConverterImpl::isEncodedChar(CHAR c) const
{
	return bModified_ ? c != '&' : Base64Encoder::isEncodedChar(c);
}

bool qs::UTF7ConverterImpl::isEncodingNeeded(WCHAR c) const
{
	if (bModified_)
		return c <= 0x1f || 0x7f <= c;
	else
		return getType(c) == TYPE_E && c != L' ' && c != L'\t' && c != L'\n' && c != L'\r';
}

UTF7ConverterImpl::Type qs::UTF7ConverterImpl::getType(WCHAR c)
{
	if ((L'a' <= c && c <= L'z') ||
		(L'A' <= c && c <= L'Z') ||
		(L'0' <= c && c <= L'9'))
		return TYPE_D;
	
	const WCHAR* pszD = L"\'(),-./:?";
	const WCHAR* pszO = L"!\"#$%&*;<=>@[]^_\'{|}";
	
	if (wcschr(pszD, c))
		return TYPE_D;
	else if (wcschr(pszO, c))
		return TYPE_O;
	else
		return TYPE_E;
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
			if (pSrc == pSrcEnd || !pImpl_->isEncodingNeeded(*pSrc)) {
				size_t n = 0;
				*p++ = cIn;
				size_t nEncodeLen = pSrc - pEncodeBegin;
				malloc_ptr<unsigned char> pEncode(
					static_cast<unsigned char*>(allocate(nEncodeLen*sizeof(WCHAR) + 1)));
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
			else if (!pImpl_->isEncodingNeeded(*pSrc)) {
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
			if (*pSrc == cOut || !pImpl_->isEncodedChar(*pSrc)) {
				if (!pImpl_->decode(pEncodeBegin, pSrc, &pDst))
					return -1;
				if (*pSrc != cOut)
					*pDst++ = static_cast<WCHAR>(*pSrc);
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
	return _wcsicmp(pwszName, L"utf-7") == 0 ||
		_wcsicmp(pwszName, L"utf7") == 0;
}

std::auto_ptr<Converter> qs::UTF7ConverterFactory::createInstance(const WCHAR* pwszName)
{
	return std::auto_ptr<Converter>(new UTF7Converter(false));
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
	return _wcsicmp(pwszName, L"utf-8") == 0 ||
		_wcsicmp(pwszName, L"utf8") == 0;
}

std::auto_ptr<Converter> qs::UTF8ConverterFactory::createInstance(const WCHAR* pwszName)
{
	return std::auto_ptr<Converter>(new UTF8Converter());
}


/****************************************************************************
 *
 * MLangConverterImpl
 *
 */

struct qs::MLangConverterImpl
{
	IMultiLanguage* pMultiLanguage_;
	DWORD dwEncoding_;
	DWORD dwMode_;
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
	pImpl_->dwMode_ = 0;
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
	
	DWORD dwMode = pImpl_->dwMode_;
	UINT nSrcLen = static_cast<UINT>(nLen);
	UINT nDstLen = 0;
	hr = pImpl_->pMultiLanguage_->ConvertStringFromUnicode(&dwMode,
		pImpl_->dwEncoding_, const_cast<WCHAR*>(pwsz), &nSrcLen, 0, &nDstLen);
	if (hr != S_OK)
		return -1;
	
	XStringBufferLock<XSTRING> lock(pBuf, nDstLen + 1);
	CHAR* pLock = lock.get();
	if (!pLock)
		return -1;
	
	dwMode = pImpl_->dwMode_;
	hr = pImpl_->pMultiLanguage_->ConvertStringFromUnicode(&dwMode,
		pImpl_->dwEncoding_, const_cast<WCHAR*>(pwsz), &nSrcLen, pLock, &nDstLen);
	if (hr != S_OK)
		return -1;
	pImpl_->dwMode_ = dwMode;
	
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
	
	DWORD dwMode = pImpl_->dwMode_;
	UINT nSrcLen = static_cast<UINT>(nLen);
	UINT nDstLen = 0;
	hr = pImpl_->pMultiLanguage_->ConvertStringToUnicode(&dwMode,
		pImpl_->dwEncoding_, const_cast<CHAR*>(psz), &nSrcLen, 0, &nDstLen);
	if (hr != S_OK)
		return -1;
	
	XStringBufferLock<WXSTRING> lock(pBuf, nDstLen + 1);
	WCHAR* pLock = lock.get();
	if (!pLock)
		return -1;
	
	if (hr == S_OK) {
		dwMode = pImpl_->dwMode_;
		hr = pImpl_->pMultiLanguage_->ConvertStringToUnicode(&dwMode,
			pImpl_->dwEncoding_, const_cast<CHAR*>(psz), &nSrcLen, pLock, &nDstLen);
		if (hr != S_OK)
			return -1;
		pImpl_->dwMode_ = dwMode;
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
		return std::auto_ptr<Converter>(0);
	return std::auto_ptr<Converter>(new MLangConverter(pImpl_->pMultiLanguage_, dwEncoding));
}
