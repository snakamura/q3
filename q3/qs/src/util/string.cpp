/*
 * $Id: string.cpp,v 1.1.1.1 2003/04/29 08:07:37 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsstring.h>
#include <qserror.h>
#include <qsconv.h>
#include <qsstl.h>
#include <qsnew.h>

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

QSEXPORTPROC STRING qs::allocString(size_t nSize)
{
	nSize = (nSize + 1)*sizeof(CHAR);
	void* p = string_alloc::allocate(nSize + sizeof(int));
	if (!p)
		return 0;
	*static_cast<int*>(p) = nSize;
	return static_cast<STRING>(static_cast<char*>(p) + sizeof(int));
}

QSEXPORTPROC STRING qs::allocString(const CHAR* psz)
{
	return allocString(psz, -1);
}

QSEXPORTPROC STRING qs::allocString(const CHAR* psz, size_t nSize)
{
	if (nSize == static_cast<size_t>(-1))
		nSize = strlen(psz);
	STRING str = allocString(nSize);
	if (!str)
		return 0;
	strncpy(str, psz, nSize);
	*(str + nSize) = '\0';
	return str;
}

QSEXPORTPROC STRING qs::reallocString(STRING str, size_t nSize)
{
	STRING strNew = allocString(nSize);
	if (strNew) {
		nSize = QSMIN(strlen(str), nSize);
		strncpy(strNew, str, nSize);
		*(strNew + nSize) = '\0';
	}
	freeString(str);
	return strNew;
}

QSEXPORTPROC void qs::freeString(STRING str)
{
	if (str) {
		void* p = reinterpret_cast<char*>(str) - sizeof(int);
		string_alloc::deallocate(p, *static_cast<int*>(p));
	}
}

QSEXPORTPROC WSTRING qs::allocWString(size_t nSize)
{
	nSize = (nSize + 1)*sizeof(WCHAR);
	void* p = string_alloc::allocate(nSize + sizeof(int));
	if (!p)
		return 0;
	*static_cast<int*>(p) = nSize;
	return reinterpret_cast<WSTRING>(static_cast<char*>(p) + sizeof(int));
}

QSEXPORTPROC WSTRING qs::allocWString(const WCHAR* pwsz)
{
	return allocWString(pwsz, -1);
}

QSEXPORTPROC WSTRING qs::allocWString(const WCHAR* pwsz, size_t nSize)
{
	if (nSize == static_cast<size_t>(-1))
		nSize = wcslen(pwsz);
	WSTRING wstr = allocWString(nSize);
	if (!wstr)
		return 0;
	wcsncpy(wstr, pwsz, nSize);
	*(wstr + nSize) = L'\0';
	return wstr;
}

QSEXPORTPROC WSTRING qs::reallocWString(WSTRING wstr, size_t nSize)
{
	WSTRING wstrNew = allocWString(nSize);
	if (wstrNew) {
		nSize = QSMIN(wcslen(wstr), nSize);
		wcsncpy(wstrNew, wstr, nSize);
		*(wstrNew + nSize) = L'\0';
	}
	freeWString(wstr);
	return wstrNew;
}

QSEXPORTPROC void qs::freeWString(WSTRING wstr)
{
	if (wstr) {
		void* p = reinterpret_cast<char*>(wstr) - sizeof(int);
		string_alloc::deallocate(p, *static_cast<int*>(p));
	}
}


/****************************************************************************
 *
 * String Utilities
 *
 */

QSEXPORTPROC QSTATUS qs::loadString(HINSTANCE hInstResource,
	UINT nId, WSTRING* pwstr)
{
	assert(pwstr);
	*pwstr = 0;
	
	int nSize = 128;
	string_ptr<TSTRING> tstr(allocTString(nSize));
	if (!tstr.get())
		return QSTATUS_OUTOFMEMORY;
	int nLen = 0;
	while (true) {
		nLen = ::LoadString(hInstResource, nId, tstr.get(), nSize);
		if (nLen == 0)
			return QSTATUS_FAIL;
		else if (nLen < nSize)
			break;
		nSize += 128;
		tstr.reset(reallocTString(tstr.get(), nSize));
		if (!tstr.get())
			return QSTATUS_OUTOFMEMORY;
	}
	
#ifdef UNICODE
	*pwstr = tstr.release();
#else
	*pwstr = tcs2wcs(tstr.get());
#endif
	return QSTATUS_SUCCESS;
}

QSEXPORTPROC STRING qs::tolower(const CHAR* psz)
{
	return tolower(psz, static_cast<size_t>(-1));
}

QSEXPORTPROC STRING qs::tolower(const CHAR* psz, size_t nLen)
{
	if (nLen == static_cast<size_t>(-1))
		nLen = strlen(psz);
	
	STRING str = allocString(nLen + 1);
	if (!str)
		return 0;
	
	CHAR* p = str;
	for (const CHAR* pSrc = psz; pSrc < psz + nLen; ++pSrc)
		*p++ = ::tolower(*pSrc);
	*p = '\0';
	
	return str;
}

QSEXPORTPROC STRING qs::toupper(const CHAR* psz)
{
	return toupper(psz, static_cast<size_t>(-1));
}

QSEXPORTPROC STRING qs::toupper(const CHAR* psz, size_t nLen)
{
	if (nLen == static_cast<size_t>(-1))
		nLen = strlen(psz);
	
	STRING str = allocString(nLen + 1);
	if (!str)
		return 0;
	
	CHAR* p = str;
	for (const CHAR* pSrc = psz; pSrc < psz + nLen; ++pSrc)
		*p++ = ::toupper(*pSrc);
	*p = '\0';
	
	return str;
}

QSEXPORTPROC WSTRING qs::tolower(const WCHAR* pwsz)
{
	return tolower(pwsz, static_cast<size_t>(-1));
}

QSEXPORTPROC WSTRING qs::tolower(const WCHAR* pwsz, size_t nLen)
{
	if (nLen == static_cast<size_t>(-1))
		nLen = wcslen(pwsz);
	
	WSTRING wstr = allocWString(nLen + 1);
	if (!wstr)
		return 0;
	
	WCHAR* p = wstr;
	for (const WCHAR* pSrc = pwsz; pSrc < pwsz + nLen; ++pSrc)
		*p++ = ::towlower(*pSrc);
	*p = L'\0';
	
	return wstr;
}

QSEXPORTPROC WSTRING qs::toupper(const WCHAR* pwsz)
{
	return toupper(pwsz, static_cast<size_t>(-1));
}

QSEXPORTPROC WSTRING qs::toupper(const WCHAR* pwsz, size_t nLen)
{
	if (nLen == static_cast<size_t>(-1))
		nLen = wcslen(pwsz);
	
	WSTRING wstr = allocWString(nLen + 1);
	if (!wstr)
		return 0;
	
	WCHAR* p = wstr;
	for (const WCHAR* pSrc = pwsz; pSrc < pwsz + nLen; ++pSrc)
		*p++ = ::towupper(*pSrc);
	*p = L'\0';
	
	return wstr;
}

QSEXPORTPROC WSTRING qs::trim(const WCHAR* pwsz)
{
	return trim(pwsz, static_cast<size_t>(-1));
}

QSEXPORTPROC WSTRING qs::trim(const WCHAR* pwsz, size_t nLen)
{
	if (nLen == static_cast<size_t>(-1))
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

QSEXPORTPROC STRING qs::concat(const CHAR* psz1, const CHAR* psz2)
{
	return concat(psz1, -1, psz2, -1);
}

QSEXPORTPROC STRING qs::concat(const CHAR* psz1, size_t nLen1,
	const CHAR* psz2, size_t nLen2)
{
	const Concat c[] = {
		{ psz1, nLen1 },
		{ psz2, nLen2 }
	};
	return concat(c, countof(c));
}

QSEXPORTPROC STRING qs::concat(const CHAR* psz1,
	const CHAR* psz2, const CHAR* psz3)
{
	return concat(psz1, -1, psz2, -1, psz3, -1);
}

QSEXPORTPROC STRING qs::concat(const CHAR* psz1, size_t nLen1,
	const CHAR* psz2, size_t nLen2, const CHAR* psz3, size_t nLen3)
{
	const Concat c[] = {
		{ psz1, nLen1 },
		{ psz2, nLen2 },
		{ psz3, nLen3 }
	};
	return concat(c, countof(c));
}

QSEXPORTPROC STRING qs::concat(const Concat* pConcat, size_t nSize)
{
	size_t n = 0;
	int nLen = 0;
	for (n = 0; n < nSize; ++n) {
		if (pConcat[n].nLen_ == -1)
			nLen += strlen(pConcat[n].p_);
		else
			nLen += pConcat[n].nLen_;
	}
	STRING str = allocString(nLen + 1);
	if (str) {
		CHAR* p = str;
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
	}
	return str;
}

QSEXPORTPROC WSTRING qs::concat(const WCHAR* pwsz1, const WCHAR* pwsz2)
{
	return concat(pwsz1, -1, pwsz2, -1);
}

QSEXPORTPROC WSTRING qs::concat(const WCHAR* pwsz1, size_t nLen1,
	const WCHAR* pwsz2, size_t nLen2)
{
	const ConcatW c[] = {
		{ pwsz1, nLen1 },
		{ pwsz2, nLen2 }
	};
	return concat(c, countof(c));
}

QSEXPORTPROC WSTRING qs::concat(const WCHAR* pwsz1,
	const WCHAR* pwsz2, const WCHAR* pwsz3)
{
	return concat(pwsz1, -1, pwsz2, -1, pwsz3, -1);
}

QSEXPORTPROC WSTRING qs::concat(const WCHAR* pwsz1, size_t nLen1,
	const WCHAR* pwsz2, size_t nLen2, const WCHAR* pwsz3, size_t nLen3)
{
	const ConcatW c[] = {
		{ pwsz1, nLen1 },
		{ pwsz2, nLen2 },
		{ pwsz3, nLen3 }
	};
	return concat(c, countof(c));
}

QSEXPORTPROC WSTRING qs::concat(const ConcatW* pConcat, size_t nSize)
{
	size_t n = 0;
	int nLen = 0;
	for (n = 0; n < nSize; ++n) {
		if (pConcat[n].nLen_ == -1)
			nLen += wcslen(pConcat[n].p_);
		else
			nLen += pConcat[n].nLen_;
	}
	WSTRING wstr = allocWString(nLen + 1);
	if (wstr) {
		WCHAR* p = wstr;
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
	}
	return wstr;
}


/****************************************************************************
 *
 * StringTokenizerImpl
 *
 */

struct qs::StringTokenizerImpl
{
	typedef std::vector<WSTRING> TokenList;
	TokenList listToken_;
};


/****************************************************************************
 *
 * StringTokenizer
 *
 */

qs::StringTokenizer::StringTokenizer(const WCHAR* pwsz,
	const WCHAR* pwszDelimiter, QSTATUS* pstatus)
{
	assert(pwsz);
	assert(pstatus);
	
	DECLARE_QSTATUS();
	
	*pstatus = QSTATUS_SUCCESS;
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	
	StringBuffer<WSTRING> buffer(&status);
	CHECK_QSTATUS_SET(pstatus);
	
	STLWrapper<std::vector<WSTRING> > wrapper(pImpl_->listToken_);
	
	WCHAR c = L'\0';
	bool bInQuote = false;
	do {
		c = *pwsz++;
		if (c == L'\"') {
			bInQuote = !bInQuote;
		}
		else if (c == L'\\') {
			c = *pwsz++;
			status = buffer.append(c);
			CHECK_QSTATUS_SET(pstatus);
		}
		else if ((c == L'\0' || wcschr(pwszDelimiter, c)) && !bInQuote) {
			string_ptr<WSTRING> wstr(buffer.getString());
			status = wrapper.push_back(wstr.get());
			CHECK_QSTATUS_SET(pstatus);
			wstr.release();
		}
		else {
			status = buffer.append(c);
			CHECK_QSTATUS_SET(pstatus);
		}
	} while (c != L'\0');
}

qs::StringTokenizer::~StringTokenizer()
{
	if (pImpl_) {
		std::for_each(pImpl_->listToken_.begin(),
			pImpl_->listToken_.end(), string_free<WSTRING>());
		delete pImpl_;
		pImpl_ = 0;
	}
}

unsigned int qs::StringTokenizer::getCount() const
{
	return pImpl_->listToken_.size();
}

const WCHAR* qs::StringTokenizer::get(unsigned int n) const
{
	assert(n < getCount());
	return pImpl_->listToken_[n];
}
