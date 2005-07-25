/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsconv.h>
#include <qsstl.h>
#include <qsstring.h>

#include <memory>
#include <algorithm>
#include <vector>
#include <cstring>

using namespace qs;


/****************************************************************************
 *
 * String Allocation
 *
 */

typedef std::__sgi_alloc string_alloc;

QSEXPORTPROC string_ptr qs::allocString(size_t nSize)
{
	nSize = (nSize + 1)*sizeof(CHAR) + sizeof(size_t);
	void* p = string_alloc::allocate(nSize);
	*static_cast<size_t*>(p) = nSize;
	return static_cast<STRING>(static_cast<char*>(p) + sizeof(size_t));
}

QSEXPORTPROC string_ptr qs::allocString(const CHAR* psz)
{
	return allocString(psz, -1);
}

QSEXPORTPROC string_ptr qs::allocString(const CHAR* psz,
										size_t nSize)
{
	if (nSize == -1)
		nSize = strlen(psz);
	string_ptr str(allocString(nSize));
	strncpy(str.get(), psz, nSize);
	*(str.get() + nSize) = '\0';
	return str;
}

QSEXPORTPROC string_ptr qs::reallocString(string_ptr str,
										  size_t nSize)
{
	string_ptr strNew(allocString(nSize));
	size_t nLen = QSMIN(str.get() ? strlen(str.get()) : 0, nSize);
	strncpy(strNew.get(), str.get(), nLen);
	*(strNew.get() + nLen) = '\0';
	return strNew;
}

QSEXPORTPROC void qs::freeString(STRING str)
{
	if (str) {
		void* p = reinterpret_cast<char*>(str) - sizeof(size_t);
		string_alloc::deallocate(p, *static_cast<size_t*>(p));
	}
}

QSEXPORTPROC xstring_ptr qs::allocXString(size_t nSize)
{
	return static_cast<XSTRING>(malloc((nSize + 1)*sizeof(CHAR)));
}

QSEXPORTPROC xstring_ptr qs::allocXString(const CHAR* psz)
{
	return allocXString(psz, -1);
}

QSEXPORTPROC xstring_ptr qs::allocXString(const CHAR* psz,
										  size_t nSize)
{
	if (nSize == -1)
		nSize = strlen(psz);
	xstring_ptr str(allocXString(nSize));
	if (!str.get())
		return 0;
	strncpy(str.get(), psz, nSize);
	*(str.get() + nSize) = '\0';
	return str;
}

QSEXPORTPROC xstring_ptr qs::reallocXString(xstring_ptr str,
											 size_t nSize)
{
	xstring_ptr strNew(allocXString(nSize));
	if (!strNew.get())
		return 0;
	size_t nLen = QSMIN(str.get() ? strlen(str.get()) : 0, nSize);
	strncpy(strNew.get(), str.get(), nLen);
	*(strNew.get() + nLen) = '\0';
	return strNew;
}

QSEXPORTPROC void qs::freeXString(XSTRING str)
{
	free(str);
}

QSEXPORTPROC wstring_ptr qs::allocWString(size_t nSize)
{
	nSize = (nSize + 1)*sizeof(WCHAR) + sizeof(size_t);
	void* p = string_alloc::allocate(nSize);
	*static_cast<size_t*>(p) = nSize;
	return reinterpret_cast<WSTRING>(static_cast<char*>(p) + sizeof(size_t));
}


QSEXPORTPROC wxstring_ptr qs::allocWXString(size_t nSize)
{
	return static_cast<WXSTRING>(malloc((nSize + 1)*sizeof(WCHAR)));
}

QSEXPORTPROC wstring_ptr qs::allocWString(const WCHAR* pwsz)
{
	return allocWString(pwsz, -1);
}

QSEXPORTPROC wxstring_ptr qs::allocWXString(const WCHAR* pwsz)
{
	return allocWXString(pwsz, -1);
}

QSEXPORTPROC wstring_ptr qs::allocWString(const WCHAR* pwsz,
										  size_t nSize)
{
	if (nSize == -1)
		nSize = wcslen(pwsz);
	wstring_ptr wstr(allocWString(nSize));
	wcsncpy(wstr.get(), pwsz, nSize);
	*(wstr.get() + nSize) = L'\0';
	return wstr;
}

QSEXPORTPROC wxstring_ptr qs::allocWXString(const WCHAR* pwsz,
											size_t nSize)
{
	if (nSize == -1)
		nSize = wcslen(pwsz);
	wxstring_ptr wstr(allocWXString(nSize));
	if (!wstr.get())
		return 0;
	wcsncpy(wstr.get(), pwsz, nSize);
	*(wstr.get() + nSize) = L'\0';
	return wstr;
}

QSEXPORTPROC wstring_ptr qs::reallocWString(wstring_ptr wstr,
											size_t nSize)
{
	wstring_ptr wstrNew(allocWString(nSize));
	size_t nLen = QSMIN(wstr.get() ? wcslen(wstr.get()) : 0, nSize);
	wcsncpy(wstrNew.get(), wstr.get(), nLen);
	*(wstrNew.get() + nLen) = L'\0';
	return wstrNew;
}

QSEXPORTPROC wxstring_ptr qs::reallocWXString(wxstring_ptr wstr,
											  size_t nSize)
{
	wxstring_ptr wstrNew(allocWXString(nSize));
	if (!wstrNew.get())
		return 0;
	size_t nLen = QSMIN(wstr.get() ? wcslen(wstr.get()) : 0, nSize);
	wcsncpy(wstrNew.get(), wstr.get(), nLen);
	*(wstrNew.get() + nLen) = L'\0';
	return wstrNew;
}

QSEXPORTPROC void qs::freeWString(WSTRING wstr)
{
	if (wstr) {
		void* p = reinterpret_cast<char*>(wstr) - sizeof(size_t);
		string_alloc::deallocate(p, *static_cast<size_t*>(p));
	}
}

QSEXPORTPROC void qs::freeWXString(WXSTRING wstr)
{
	free(wstr);
}


/****************************************************************************
 *
 * String Utilities
 *
 */

QSEXPORTPROC wstring_ptr qs::loadString(HINSTANCE hInstResource,
										UINT nId)
{
	int nSize = 128;
	tstring_ptr tstr(allocTString(nSize));
	int nLen = 0;
	while (true) {
		nLen = ::LoadString(hInstResource, nId, tstr.get(), nSize);
		if (nLen == 0)
			return 0;
		else if (nLen < nSize - 1)
			break;
		nSize += 128;
		tstr = reallocTString(tstr, nSize);
	}
	
#ifdef UNICODE
	return tstr;
#else
	return tcs2wcs(tstr.get());
#endif
}

QSEXPORTPROC string_ptr qs::tolower(const CHAR* psz)
{
	return tolower(psz, -1);
}

QSEXPORTPROC string_ptr qs::tolower(const CHAR* psz,
									size_t nLen)
{
	if (nLen == -1)
		nLen = strlen(psz);
	
	string_ptr str(allocString(nLen + 1));
	CHAR* p = str.get();
	for (const CHAR* pSrc = psz; pSrc < psz + nLen; ++pSrc)
		*p++ = ::tolower(*pSrc);
	*p = '\0';
	
	return str;
}

QSEXPORTPROC string_ptr qs::toupper(const CHAR* psz)
{
	return toupper(psz, -1);
}

QSEXPORTPROC string_ptr qs::toupper(const CHAR* psz,
									size_t nLen)
{
	if (nLen == -1)
		nLen = strlen(psz);
	
	string_ptr str(allocString(nLen + 1));
	CHAR* p = str.get();
	for (const CHAR* pSrc = psz; pSrc < psz + nLen; ++pSrc)
		*p++ = ::toupper(*pSrc);
	*p = '\0';
	
	return str;
}

QSEXPORTPROC wstring_ptr qs::tolower(const WCHAR* pwsz)
{
	return tolower(pwsz, -1);
}

QSEXPORTPROC wstring_ptr qs::tolower(const WCHAR* pwsz,
									 size_t nLen)
{
	if (nLen == -1)
		nLen = wcslen(pwsz);
	
	wstring_ptr wstr(allocWString(nLen + 1));
	WCHAR* p = wstr.get();
	for (const WCHAR* pSrc = pwsz; pSrc < pwsz + nLen; ++pSrc)
		*p++ = ::towlower(*pSrc);
	*p = L'\0';
	
	return wstr;
}

QSEXPORTPROC wstring_ptr qs::toupper(const WCHAR* pwsz)
{
	return toupper(pwsz, -1);
}

QSEXPORTPROC wstring_ptr qs::toupper(const WCHAR* pwsz,
									 size_t nLen)
{
	if (nLen == -1)
		nLen = wcslen(pwsz);
	
	wstring_ptr wstr(allocWString(nLen + 1));
	WCHAR* p = wstr.get();
	for (const WCHAR* pSrc = pwsz; pSrc < pwsz + nLen; ++pSrc)
		*p++ = ::towupper(*pSrc);
	*p = L'\0';
	
	return wstr;
}

QSEXPORTPROC wstring_ptr qs::trim(const WCHAR* pwsz)
{
	return trim(pwsz, -1);
}

QSEXPORTPROC wstring_ptr qs::trim(const WCHAR* pwsz,
								  size_t nLen)
{
	if (nLen == -1)
		nLen = wcslen(pwsz);
	if (nLen == 0)
		return allocWString(L"");
	
	const WCHAR* p = pwsz;
	while (p < pwsz + nLen && *p == L' ')
		++p;
	if (p == pwsz + nLen)
		return allocWString(L"");
	
	const WCHAR* pEnd = pwsz + nLen - 1;
	while (*pEnd == L' ')
		--pEnd;
	assert(pEnd > pwsz);
	
	return allocWString(p, pEnd - p + 1);
}

QSEXPORTPROC string_ptr qs::concat(const CHAR* psz1,
								   const CHAR* psz2)
{
	return concat(psz1, -1, psz2, -1);
}

QSEXPORTPROC string_ptr qs::concat(const CHAR* psz1,
								   size_t nLen1,
								   const CHAR* psz2,
								   size_t nLen2)
{
	const Concat c[] = {
		{ psz1, nLen1 },
		{ psz2, nLen2 }
	};
	return concat(c, countof(c));
}

QSEXPORTPROC string_ptr qs::concat(const CHAR* psz1,
								   const CHAR* psz2,
								   const CHAR* psz3)
{
	return concat(psz1, -1, psz2, -1, psz3, -1);
}

QSEXPORTPROC string_ptr qs::concat(const CHAR* psz1,
								   size_t nLen1,
								   const CHAR* psz2,
								   size_t nLen2,
								   const CHAR* psz3,
								   size_t nLen3)
{
	const Concat c[] = {
		{ psz1, nLen1 },
		{ psz2, nLen2 },
		{ psz3, nLen3 }
	};
	return concat(c, countof(c));
}

QSEXPORTPROC string_ptr qs::concat(const Concat* pConcat,
								   size_t nSize)
{
	size_t n = 0;
	size_t nLen = 0;
	for (n = 0; n < nSize; ++n) {
		if (pConcat[n].nLen_ == -1)
			nLen += strlen(pConcat[n].p_);
		else
			nLen += pConcat[n].nLen_;
	}
	
	string_ptr str(allocString(nLen + 1));
	CHAR* p = str.get();
	for (n = 0; n < nSize; ++n) {
		size_t nCharLen = 0;
		if (pConcat[n].nLen_ == -1)
			nCharLen = strlen(pConcat[n].p_);
		else
			nCharLen = pConcat[n].nLen_;
		strncpy(p, pConcat[n].p_, nCharLen);
		p += nCharLen;
	}
	*p = '\0';
	
	return str;
}

QSEXPORTPROC wstring_ptr qs::concat(const WCHAR* pwsz1,
									const WCHAR* pwsz2)
{
	return concat(pwsz1, -1, pwsz2, -1);
}

QSEXPORTPROC wstring_ptr qs::concat(const WCHAR* pwsz1,
									size_t nLen1,
									const WCHAR* pwsz2,
									size_t nLen2)
{
	const ConcatW c[] = {
		{ pwsz1, nLen1 },
		{ pwsz2, nLen2 }
	};
	return concat(c, countof(c));
}

QSEXPORTPROC wstring_ptr qs::concat(const WCHAR* pwsz1,
									const WCHAR* pwsz2,
									const WCHAR* pwsz3)
{
	return concat(pwsz1, -1, pwsz2, -1, pwsz3, -1);
}

QSEXPORTPROC wstring_ptr qs::concat(const WCHAR* pwsz1,
									size_t nLen1,
									const WCHAR* pwsz2,
									size_t nLen2,
									const WCHAR* pwsz3,
									size_t nLen3)
{
	const ConcatW c[] = {
		{ pwsz1, nLen1 },
		{ pwsz2, nLen2 },
		{ pwsz3, nLen3 }
	};
	return concat(c, countof(c));
}

QSEXPORTPROC wstring_ptr qs::concat(const ConcatW* pConcat,
									size_t nSize)
{
	size_t n = 0;
	size_t nLen = 0;
	for (n = 0; n < nSize; ++n) {
		if (pConcat[n].nLen_ == -1)
			nLen += wcslen(pConcat[n].p_);
		else
			nLen += pConcat[n].nLen_;
	}
	
	wstring_ptr wstr(allocWString(nLen + 1));
	WCHAR* p = wstr.get();
	for (n = 0; n < nSize; ++n) {
		size_t nCharLen = 0;
		if (pConcat[n].nLen_ == -1)
			nCharLen = wcslen(pConcat[n].p_);
		else
			nCharLen = pConcat[n].nLen_;
		wcsncpy(p, pConcat[n].p_, nCharLen);
		p += nCharLen;
	}
	*p = L'\0';
	
	return wstr;
}


/****************************************************************************
 *
 * StringTokenizerImpl
 *
 */

struct qs::StringTokenizerImpl
{
	StringTokenizerImpl();
	~StringTokenizerImpl();
	
	typedef std::vector<WSTRING> TokenList;
	TokenList listToken_;
};

qs::StringTokenizerImpl::StringTokenizerImpl()
{
}

qs::StringTokenizerImpl::~StringTokenizerImpl()
{
	std::for_each(listToken_.begin(),
		listToken_.end(), string_free<WSTRING>());
}


/****************************************************************************
 *
 * StringTokenizer
 *
 */

qs::StringTokenizer::StringTokenizer(const WCHAR* pwsz,
									 const WCHAR* pwszDelimiter) :
	pImpl_(0)
{
	assert(pwsz);
	
	std::auto_ptr<StringTokenizerImpl> pImpl(new StringTokenizerImpl());
	
	StringBuffer<WSTRING> buffer;
	
	WCHAR c = L'\0';
	bool bInQuote = false;
	do {
		c = *pwsz++;
		if (c == L'\"') {
			bInQuote = !bInQuote;
		}
		else if (c == L'\\') {
			c = *pwsz++;
			buffer.append(c);
		}
		else if ((c == L'\0' || wcschr(pwszDelimiter, c)) && !bInQuote) {
			wstring_ptr wstr(buffer.getString());
			pImpl_->listToken_.push_back(wstr.get());
			wstr.release();
		}
		else {
			buffer.append(c);
		}
	} while (c != L'\0');
	
	pImpl_ = pImpl.release();
}

qs::StringTokenizer::~StringTokenizer()
{
	delete pImpl_;
}

size_t qs::StringTokenizer::getCount() const
{
	return pImpl_->listToken_.size();
}

const WCHAR* qs::StringTokenizer::get(unsigned int n) const
{
	assert(n < getCount());
	return pImpl_->listToken_[n];
}
