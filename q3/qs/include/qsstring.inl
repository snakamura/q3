/*
 * $Id: qsstring.inl,v 1.2 2003/05/30 08:02:05 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QSSTRING_INL__
#define __QSSTRING_INL__

#include <qserror.h>
#include <qsassert.h>
#include <qswce.h>

#include <algorithm>


/****************************************************************************
 *
 * CharTraits<CHAR>
 *
 */

template<>
inline size_t qs::CharTraits<CHAR>::getLength(const CHAR* psz)
{
	return strlen(psz);
}

template<>
inline const CHAR* qs::CharTraits<CHAR>::getEmptyBuffer()
{
	return "";
}

template<>
inline int qs::CharTraits<CHAR>::compare(const CHAR* lhs, const CHAR* rhs)
{
	return strcmp(lhs, rhs);
}

template<>
inline int qs::CharTraits<CHAR>::compare(const CHAR* lhs, const CHAR* rhs, size_t nLen)
{
	return strncmp(lhs, rhs, nLen);
}

template<>
inline int qs::CharTraits<CHAR>::compareIgnoreCase(const CHAR* lhs, const CHAR* rhs)
{
	return _stricmp(lhs, rhs);
}

template<>
inline int qs::CharTraits<CHAR>::compareIgnoreCase(const CHAR* lhs, const CHAR* rhs, size_t nLen)
{
	return _strnicmp(lhs, rhs, nLen);
}


/****************************************************************************
 *
 * CharTraits<WCHAR>
 *
 */

template<>
inline size_t qs::CharTraits<WCHAR>::getLength(const WCHAR* pwsz)
{
	return wcslen(pwsz);
}

template<>
inline const WCHAR* qs::CharTraits<WCHAR>::getEmptyBuffer()
{
	return L"";
}

template<>
inline int qs::CharTraits<WCHAR>::compare(const WCHAR* lhs, const WCHAR* rhs)
{
	return wcscmp(lhs, rhs);
}

template<>
inline int qs::CharTraits<WCHAR>::compare(const WCHAR* lhs, const WCHAR* rhs, size_t nLen)
{
	return wcsncmp(lhs, rhs, nLen);
}

template<>
inline int qs::CharTraits<WCHAR>::compareIgnoreCase(const WCHAR* lhs, const WCHAR* rhs)
{
	return _wcsicmp(lhs, rhs);
}

template<>
inline int qs::CharTraits<WCHAR>::compareIgnoreCase(const WCHAR* lhs, const WCHAR* rhs, size_t nLen)
{
	return _wcsnicmp(lhs, rhs, nLen);
}


/****************************************************************************
 *
 * StringTraits<STRING>
 *
 */

template<>
inline qs::STRING qs::StringTraits<qs::STRING>::allocString(size_t nLen)
{
	return qs::allocString(nLen);
}

template<>
inline qs::STRING qs::StringTraits<qs::STRING>::allocString(
	const CHAR* psz, size_t nLen)
{
	return qs::allocString(psz, nLen);
}

template<>
inline qs::STRING qs::StringTraits<qs::STRING>::reallocString(
	STRING str, size_t nLen)
{
	return qs::reallocString(str, nLen);
}

template<>
inline void qs::StringTraits<qs::STRING>::freeString(STRING str)
{
	qs::freeString(str);
}


/****************************************************************************
 *
 * StringTraits<WSTRING>
 *
 */

template<>
inline qs::WSTRING qs::StringTraits<qs::WSTRING>::allocString(size_t nLen)
{
	return qs::allocWString(nLen);
}

template<>
inline qs::WSTRING qs::StringTraits<qs::WSTRING>::allocString(
	const WCHAR* psz, size_t nLen)
{
	return qs::allocWString(psz, nLen);
}

template<>
inline qs::WSTRING qs::StringTraits<qs::WSTRING>::reallocString(
	WSTRING str, size_t nLen)
{
	return qs::reallocWString(str, nLen);
}

template<>
inline void qs::StringTraits<qs::WSTRING>::freeString(WSTRING str)
{
	qs::freeWString(str);
}


/****************************************************************************
 *
 * string_ptr
 *
 */

template<class String>
qs::string_ptr<String>::string_ptr() :
	str_(0)
{
}

template<class String>
qs::string_ptr<String>::string_ptr(String str) :
	str_(str)
{
}

template<class String>
qs::string_ptr<String>::string_ptr(string_ptr& s) :
	str_(s.release())
{
}

template<class String>
qs::string_ptr<String>::~string_ptr()
{
	StringTraits<String>::freeString(str_);
}

template<class String>
qs::string_ptr<String>& qs::string_ptr<String>::operator=(string_ptr& s)
{
	if (&s != this && s.get() != str_)
		str_ = s.release();
	return *this;
}

template<class String>
String* qs::string_ptr<String>::operator&()
{
	assert(!str_);
	return &str_;
}

template<class String>
qs::string_ptr<String>::Char qs::string_ptr<String>::operator[](
	size_t n) const
{
	return str_[n];
}

template<class String>
qs::string_ptr<String>::Char& qs::string_ptr<String>::operator[](
	size_t n)
{
	return str_[n];
}

template<class String>
String qs::string_ptr<String>::get() const
{
	return str_;
}

template<class String>
String qs::string_ptr<String>::release()
{
	String str = str_;
	str_ = 0;
	return str;
}

template<class String>
void qs::string_ptr<String>::reset(String str)
{
	StringTraits<String>::freeString(str_);
	str_ = str;
}

template<class String>
qs::string_ptr<String>* qs::string_ptr<String>::getThis()
{
	return this;
}


/****************************************************************************
 *
 * string_hash
 *
 */

template<class Char>
int qs::string_hash<Char>::operator()(const Char* p) const
{
	return *p;
}


/****************************************************************************
 *
 * string_equal
 *
 */

template<class Char>
bool qs::string_equal<Char>::operator()(const Char* plhs, const Char* prhs) const
{
	if (plhs)
		return prhs && CharTraits<Char>::compare(plhs, prhs) == 0;
	else
		return !prhs;
}


/****************************************************************************
 *
 * string_equal_i
 *
 */

template<class Char>
bool qs::string_equal_i<Char>::operator()(const Char* plhs, const Char* prhs) const
{
	if (plhs)
		return prhs && CharTraits<Char>::compareIgnoreCase(plhs, prhs) == 0;
	else
		return !prhs;
}


/****************************************************************************
 *
 * string_less
 *
 */

template<class Char>
bool qs::string_less<Char>::operator()(const Char* plhs, const Char* prhs) const
{
	return CharTraits<Char>::compare(plhs, prhs) < 0;
}


/****************************************************************************
 *
 * string_less_i
 *
 */

template<class Char>
bool qs::string_less_i<Char>::operator()(const Char* plhs, const Char* prhs) const
{
	return CharTraits<Char>::compareIgnoreCase(plhs, prhs) < 0;
}


/****************************************************************************
 *
 * string_free
 *
 */

template<class String>
void* qs::string_free<String>::operator()(String str) const
{
	StringTraits<String>::freeString(str);
	return 0;
}


/****************************************************************************
 *
 * StringBuffer
 *
 */

template<class String>
qs::StringBuffer<String>::StringBuffer(QSTATUS* pstatus)
{
	assert(pstatus);
	*pstatus = init(0, 0);
}

template<class String>
qs::StringBuffer<String>::StringBuffer(size_t nLen, QSTATUS* pstatus)
{
	assert(pstatus);
	*pstatus = init(0, nLen);
}

template<class String>
qs::StringBuffer<String>::StringBuffer(const Char* psz, QSTATUS* pstatus)
{
	assert(pstatus);
	*pstatus = init(psz, static_cast<size_t>(-1));
}

template<class String>
qs::StringBuffer<String>::StringBuffer(const Char* psz,
	size_t nLen, QSTATUS* pstatus)
{
	assert(pstatus);
	*pstatus = init(psz, nLen);
}

template<class String>
qs::StringBuffer<String>::~StringBuffer()
{
	assert(str_);
	StringTraits<String>::freeString(str_);
}

template<class String>
String qs::StringBuffer<String>::getString()
{
	assert(str_);
	String str = str_;
	init(0, 0);
	return str;
}

template<class String>
const qs::StringBuffer<String>::Char* qs::StringBuffer<String>::getCharArray() const
{
	assert(str_);
	return str_;
}

template<class String>
qs::QSTATUS qs::StringBuffer<String>::getLength() const
{
	return pEnd_ - str_;
}

template<class String>
qs::StringBuffer<String>::Char qs::StringBuffer<String>::get(size_t n) const
{
	assert(n < getLength());
	return str_[n];
}

template<class String>
qs::QSTATUS qs::StringBuffer<String>::append(const Char c)
{
	return append(&c, 1);
}

template<class String>
qs::QSTATUS qs::StringBuffer<String>::append(const Char* psz)
{
	return append(psz, static_cast<size_t>(-1));
}

template<class String>
qs::QSTATUS qs::StringBuffer<String>::append(const Char* psz, size_t nLen)
{
	if (nLen == static_cast<size_t>(-1))
		nLen = CharTraits<Char>::getLength(psz);
	
	if (nLen == 0)
		return QSTATUS_SUCCESS;
	
	if ((pEnd_ - str_) + nLen > nLen_) {
		DECLARE_QSTATUS();
		status = allocBuffer(nLen);
		CHECK_QSTATUS();
	}
	memcpy(pEnd_, psz, nLen*sizeof(Char));
	pEnd_ += nLen;
	*pEnd_ = Char();
	
	return QSTATUS_SUCCESS;
}

template<class String>
qs::QSTATUS qs::StringBuffer<String>::insert(size_t nPos, const Char c)
{
	return insert(nPos, &c, 1);
}

template<class String>
qs::QSTATUS qs::StringBuffer<String>::insert(size_t nPos, const Char* psz)
{
	return insert(nPos, psz, static_cast<size_t>(-1));
}

template<class String>
qs::QSTATUS qs::StringBuffer<String>::insert(
	size_t nPos, const Char* psz, size_t nLen)
{
	assert(nPos <= static_cast<size_t>(pEnd_ - str_));
	
	if (nLen == static_cast<size_t>(-1))
		nLen = CharTraits<Char>::getLength(psz);
	
	// TODO
	// Performance improvement
	if ((pEnd_ - str_) + nLen > nLen_) {
		DECLARE_QSTATUS();
		status = allocBuffer(nLen);
		CHECK_QSTATUS();
	}
	
	memmove(str_ + nPos + nLen, str_ + nPos, (pEnd_ - str_ - nPos)*sizeof(Char));
	memcpy(str_ + nPos, psz, nLen*sizeof(Char));
	pEnd_ += nLen;
	*pEnd_ = Char();
	
	return QSTATUS_SUCCESS;
}

template<class String>
qs::QSTATUS qs::StringBuffer<String>::remove()
{
	return remove(0, static_cast<size_t>(-1));
}

template<class String>
qs::QSTATUS qs::StringBuffer<String>::remove(size_t nPos)
{
	return remove(nPos, nPos + 1);
}

template<class String>
qs::QSTATUS qs::StringBuffer<String>::remove(size_t nStart, size_t nEnd)
{
	assert(nStart <= static_cast<size_t>(pEnd_ - str_));
	
	if (str_ && nStart != nEnd) {
		if (nEnd >= static_cast<size_t>(pEnd_ - str_)) {
			pEnd_ = str_ + nStart;
		}
		else {
			memmove(str_ + nStart, str_ + nEnd, (pEnd_ - str_ - nEnd)*sizeof(Char));
			pEnd_ -= nEnd - nStart;
		}
		*pEnd_ = Char();
	}
	
	return QSTATUS_SUCCESS;
}

template<class String>
qs::QSTATUS qs::StringBuffer<String>::init(const Char* psz, size_t nLen)
{
	str_ = 0;
	nLen_ = 0;
	pEnd_ = 0;
	
	if (!psz || nLen == 0) {
		str_ = StringTraits<String>::allocString(nLen);
		if (!str_)
			return QSTATUS_OUTOFMEMORY;
		*str_ = Char();
		pEnd_ = str_;
	}
	else {
		if (nLen == static_cast<size_t>(-1))
			nLen = CharTraits<Char>::getLength(psz);
		str_ = StringTraits<String>::allocString(psz, nLen);
		if (!str_)
			return QSTATUS_OUTOFMEMORY;
		pEnd_ = str_ + nLen;
	}
	nLen_ = nLen;

	assert(str_);
	
	return QSTATUS_SUCCESS;
}

template<class String>
qs::QSTATUS qs::StringBuffer<String>::allocBuffer(size_t nLen)
{
	String str = 0;
	size_t nNewLen = nLen_;
	if (nNewLen == 0)
		nNewLen = QSMAX(size_t(16), nLen);
	else
		nNewLen += QSMAX(nNewLen*2, nLen);
	assert(str_);
	str = StringTraits<String>::reallocString(str_, nNewLen);
	if (!str)
		return QSTATUS_OUTOFMEMORY;
	nLen_ = nNewLen;
	pEnd_ = str + (pEnd_ - str_);
	str_ = str;
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * BMFindString
 *
 */

template<class String>
qs::BMFindString<String>::BMFindString(
	const Char* pszPattern, QSTATUS* pstatus) :
	pNext_(0),
	strPattern_(0),
	nFlags_(0)
{
	assert(pstatus);
	*pstatus = init(pszPattern, -1, 0);
}

template<class String>
qs::BMFindString<String>::BMFindString(
	const Char* pszPattern, size_t nLen, QSTATUS* pstatus) :
	pNext_(0),
	strPattern_(0),
	nFlags_(0)
{
	assert(pstatus);
	*pstatus = init(pszPattern, nLen, 0);
}

template<class String>
qs::BMFindString<String>::BMFindString(const Char* pszPattern,
	size_t nLen, unsigned int nFlags, QSTATUS* pstatus) :
	pNext_(0),
	strPattern_(0),
	nFlags_(0)
{
	assert(pstatus);
	*pstatus = init(pszPattern, nLen, nFlags);
}

template<class String>
qs::BMFindString<String>::~BMFindString()
{
	StringTraits<String>::freeString(strPattern_);
	free(pNext_);
}

template<class String>
const qs::BMFindString<String>::Char* qs::BMFindString<String>::find(
	const Char* psz) const
{
	return find(psz, static_cast<size_t>(-1));
}

template<class String>
const qs::BMFindString<String>::Char* qs::BMFindString<String>::find(
	const Char* psz, size_t nLen) const
{
	assert(psz);
	
	if (nLen == static_cast<size_t>(-1))
		nLen = CharTraits<Char>::getLength(psz);
	
	int nPatternLen = CharTraits<Char>::getLength(strPattern_);
	int n = nPatternLen - 1;
	while (n < static_cast<int>(nLen)) {
		int m = nPatternLen - 1;
		while (m >= 0 && isEqual(getChar(psz, nLen, n), strPattern_[m])) {
			--n;
			--m;
		}
		if (m < 0)
			break;
		n += QSMAX(*(skip_ + (getChar(psz, nLen, n) & 0xff)), *(pNext_ + m));
	}
	return n < static_cast<int>(nLen) ? nFlags_ & FLAG_REVERSE ?
		psz + nLen - (n + nPatternLen + 1) : psz + (n + 1) : 0;
}

template<class String>
qs::QSTATUS qs::BMFindString<String>::init(
	const Char* pszPattern, size_t nLen, unsigned int nFlags)
{
	DECLARE_QSTATUS();
	
	if (nLen == -1)
		nLen = CharTraits<Char>::getLength(pszPattern);
	
	nFlags_ = nFlags;
	
	if (nFlags_ & FLAG_IGNORECASE)
		strPattern_ = tolower(pszPattern, nLen);
	else
		strPattern_ = StringTraits<String>::allocString(pszPattern, nLen);
	if (!strPattern_)
		return QSTATUS_OUTOFMEMORY;
	if (nFlags_ & FLAG_REVERSE)
		std::reverse(strPattern_, strPattern_ + nLen);
	
	status = createSkipTable();
	CHECK_QSTATUS();
	
	status = createNextTable();
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

template<class String>
qs::QSTATUS qs::BMFindString<String>::createSkipTable()
{
	int nLen = CharTraits<Char>::getLength(strPattern_);
	for (int n = 0; n < 256; ++n)
		*(skip_ + n) = nLen;
	for (n = 0; n < nLen - 1; ++n)
		*(skip_ + (strPattern_[n] & 0xff)) = nLen - n - 1;
	
	return QSTATUS_SUCCESS;
}

template<class String>
qs::QSTATUS qs::BMFindString<String>::createNextTable()
{
	int nLen = CharTraits<Char>::getLength(strPattern_);
	malloc_ptr<int> pNext(static_cast<int*>(malloc(nLen*sizeof(int))));
	if (!pNext.get())
		return QSTATUS_OUTOFMEMORY;
	
	for (int n = 0; n < nLen; ++n)
		*(pNext.get() + n) = nLen*2 - n - 1;
	malloc_ptr<int> pTemp(static_cast<int*>(malloc(nLen*sizeof(int))));
	if (!pTemp.get())
		return QSTATUS_OUTOFMEMORY;
	for (int m = nLen - 1; m >= 0; --m) {
		*(pTemp.get() + m) = n;
		while (n != nLen && strPattern_[n] != strPattern_[m]) {
			*(pNext.get() + n) = QSMIN(*(pNext.get() + n), nLen - m - 1);
			n = *(pTemp.get() + n);
		}
		--n;
	}
	int l = n;
	for (n = 0; n < nLen; ++n) {
		*(pNext.get() + n) = QSMIN(*(pNext.get() + n), nLen + l - n);
		if (n >= l)
			l = *(pTemp.get() + l);
	}
	
	pNext_ = pNext.release();
	
	return QSTATUS_SUCCESS;
}

template<class String>
bool qs::BMFindString<String>::isEqual(Char lhs, Char rhs) const
{
	return nFlags_ & FLAG_IGNORECASE ?
		CharTraits<Char>::compareIgnoreCase(&lhs, &rhs, 1) == 0 :
		lhs == rhs;
}

template<class String>
qs::BMFindString<String>::Char qs::BMFindString<String>::getChar(
	const Char* psz, size_t nLen, size_t n) const
{
	return nFlags_ & FLAG_REVERSE ? *(psz + nLen - n - 1) : *(psz + n);
}


/****************************************************************************
 *
 * StringListFree
 *
 */

template<class List>
qs::StringListFree<List>::StringListFree(List& l) :
	p_(&l)
{
}

template<class List>
qs::StringListFree<List>::~StringListFree()
{
	if (p_)
		std::for_each(p_->begin(), p_->end(),
			string_free<List::value_type>());
}

template<class List>
void qs::StringListFree<List>::release()
{
	p_ = 0;
}

#endif // __QSSTRING_INL__
