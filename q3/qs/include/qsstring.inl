/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QSSTRING_INL__
#define __QSSTRING_INL__

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
inline CHAR qs::CharTraits<CHAR>::toLower(CHAR c)
{
	return ::tolower(c);
}

template<>
inline int qs::CharTraits<CHAR>::compare(const CHAR* lhs,
										 const CHAR* rhs)
{
	return strcmp(lhs, rhs);
}

template<>
inline int qs::CharTraits<CHAR>::compare(const CHAR* lhs,
										 const CHAR* rhs,
										 size_t nLen)
{
	return strncmp(lhs, rhs, nLen);
}

template<>
inline int qs::CharTraits<CHAR>::compareIgnoreCase(const CHAR* lhs,
												   const CHAR* rhs)
{
	return _stricmp(lhs, rhs);
}

template<>
inline int qs::CharTraits<CHAR>::compareIgnoreCase(const CHAR* lhs,
												   const CHAR* rhs,
												   size_t nLen)
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
inline WCHAR qs::CharTraits<WCHAR>::toLower(WCHAR c)
{
	return ::towlower(c);
}

template<>
inline int qs::CharTraits<WCHAR>::compare(const WCHAR* lhs,
										  const WCHAR* rhs)
{
	return wcscmp(lhs, rhs);
}

template<>
inline int qs::CharTraits<WCHAR>::compare(const WCHAR* lhs,
										  const WCHAR* rhs,
										  size_t nLen)
{
	return wcsncmp(lhs, rhs, nLen);
}

template<>
inline int qs::CharTraits<WCHAR>::compareIgnoreCase(const WCHAR* lhs,
													const WCHAR* rhs)
{
	return _wcsicmp(lhs, rhs);
}

template<>
inline int qs::CharTraits<WCHAR>::compareIgnoreCase(const WCHAR* lhs,
													const WCHAR* rhs,
													size_t nLen)
{
	return _wcsnicmp(lhs, rhs, nLen);
}


/****************************************************************************
 *
 * StringTraits<STRING>
 *
 */

template<>
inline qs::string_ptr qs::StringTraits<qs::STRING>::allocString(size_t nLen)
{
	return qs::allocString(nLen);
}

template<>
inline qs::string_ptr qs::StringTraits<qs::STRING>::allocString(const CHAR* psz,
																size_t nLen)
{
	return qs::allocString(psz, nLen);
}

template<>
inline qs::string_ptr qs::StringTraits<qs::STRING>::reallocString(string_ptr str,
																  size_t nLen)
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
inline qs::wstring_ptr qs::StringTraits<qs::WSTRING>::allocString(size_t nLen)
{
	return qs::allocWString(nLen);
}

template<>
inline qs::wstring_ptr qs::StringTraits<qs::WSTRING>::allocString(const WCHAR* psz,
																  size_t nLen)
{
	return qs::allocWString(psz, nLen);
}

template<>
inline qs::wstring_ptr qs::StringTraits<qs::WSTRING>::reallocString(wstring_ptr str,
																	size_t nLen)
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
 * XStringTraits<XSTRING>
 *
 */

template<>
inline qs::xstring_ptr qs::XStringTraits<qs::XSTRING>::allocXString(size_t nLen) QNOTHROW()
{
	return qs::allocXString(nLen);
}

template<>
inline qs::xstring_ptr qs::XStringTraits<qs::XSTRING>::allocXString(const CHAR* psz,
																	size_t nLen)
																	QNOTHROW()
{
	return qs::allocXString(psz, nLen);
}

template<>
inline qs::xstring_ptr qs::XStringTraits<qs::XSTRING>::reallocXString(xstring_ptr str,
																	  size_t nLen)
																	  QNOTHROW()
{
	return qs::reallocXString(str, nLen);
}

template<>
inline void qs::XStringTraits<qs::XSTRING>::freeXString(XSTRING str)
{
	qs::freeXString(str);
}


/****************************************************************************
 *
 * XStringTraits<WXSTRING>
 *
 */

template<>
inline qs::wxstring_ptr qs::XStringTraits<qs::WXSTRING>::allocXString(size_t nLen) QNOTHROW()
{
	return qs::allocWXString(nLen);
}

template<>
inline qs::wxstring_ptr qs::XStringTraits<qs::WXSTRING>::allocXString(const WCHAR* psz,
																	  size_t nLen)
																	  QNOTHROW()
{
	return qs::allocWXString(psz, nLen);
}

template<>
inline qs::wxstring_ptr qs::XStringTraits<qs::WXSTRING>::reallocXString(wxstring_ptr str,
																		size_t nLen)
																		QNOTHROW()
{
	return qs::reallocWXString(str, nLen);
}

template<>
inline void qs::XStringTraits<qs::WXSTRING>::freeXString(WXSTRING str)
{
	qs::freeWXString(str);
}


/****************************************************************************
 *
 * basic_string_ptr
 *
 */

template<class String>
qs::basic_string_ptr<String>::basic_string_ptr() :
	str_(0)
{
}

template<class String>
qs::basic_string_ptr<String>::basic_string_ptr(String str) :
	str_(str)
{
}

template<class String>
qs::basic_string_ptr<String>::basic_string_ptr(basic_string_ptr& s) :
	str_(s.release())
{
}

template<class String>
qs::basic_string_ptr<String>::~basic_string_ptr()
{
	StringTraits<String>::freeString(str_);
}

template<class String>
qs::basic_string_ptr<String>& qs::basic_string_ptr<String>::operator=(basic_string_ptr& s)
{
	if (s.get() != str_)
		reset(s.release());
	return *this;
}

template<class String>
qs::basic_string_ptr<String>::Char qs::basic_string_ptr<String>::operator[](size_t n) const
{
	return str_[n];
}

template<class String>
qs::basic_string_ptr<String>::Char& qs::basic_string_ptr<String>::operator[](size_t n)
{
	return str_[n];
}

template<class String>
String qs::basic_string_ptr<String>::get() const
{
	return str_;
}

template<class String>
String qs::basic_string_ptr<String>::release()
{
	String str = str_;
	str_ = 0;
	return str;
}

template<class String>
void qs::basic_string_ptr<String>::reset(String str)
{
	StringTraits<String>::freeString(str_);
	str_ = str;
}

template<class String>
qs::basic_string_ptr<String>* qs::basic_string_ptr<String>::getThis()
{
	return this;
}


/****************************************************************************
 *
 * basic_xstring_ptr
 *
 */

template<class XString>
qs::basic_xstring_ptr<XString>::basic_xstring_ptr() :
	str_(0)
{
}

template<class XString>
qs::basic_xstring_ptr<XString>::basic_xstring_ptr(XString str) :
	str_(str)
{
}

template<class XString>
qs::basic_xstring_ptr<XString>::basic_xstring_ptr(basic_xstring_ptr& s) :
	str_(s.release())
{
}

template<class XString>
qs::basic_xstring_ptr<XString>::~basic_xstring_ptr()
{
	XStringTraits<XString>::freeXString(str_);
}

template<class XString>
qs::basic_xstring_ptr<XString>& qs::basic_xstring_ptr<XString>::operator=(basic_xstring_ptr& s)
{
	if (s.get() != str_)
		reset(s.release());
	return *this;
}

template<class XString>
qs::basic_xstring_ptr<XString>::Char qs::basic_xstring_ptr<XString>::operator[](size_t n) const
{
	return str_[n];
}

template<class XString>
qs::basic_xstring_ptr<XString>::Char& qs::basic_xstring_ptr<XString>::operator[](size_t n)
{
	return str_[n];
}

template<class XString>
XString qs::basic_xstring_ptr<XString>::get() const
{
	return str_;
}

template<class XString>
XString qs::basic_xstring_ptr<XString>::release()
{
	XString str = str_;
	str_ = 0;
	return str;
}

template<class XString>
void qs::basic_xstring_ptr<XString>::reset(XString str)
{
	XStringTraits<XString>::freeXString(str_);
	str_ = str;
}

template<class XString>
qs::basic_xstring_ptr<XString>* qs::basic_xstring_ptr<XString>::getThis()
{
	return this;
}


/****************************************************************************
 *
 * basic_xstring_size_ptr
 *
 */

template<class XString>
qs::basic_xstring_size_ptr<XString>::basic_xstring_size_ptr() :
	str_(0),
	nSize_(-1)
{
}

template<class XString>
qs::basic_xstring_size_ptr<XString>::basic_xstring_size_ptr(XString str,
													   size_t nSize) :
	str_(str),
	nSize_(nSize)
{
}

template<class XString>
qs::basic_xstring_size_ptr<XString>::basic_xstring_size_ptr(basic_xstring_ptr<XString> str,
															size_t nSize) :
	str_(str.release()),
	nSize_(nSize)
{
}

template<class XString>
qs::basic_xstring_size_ptr<XString>::basic_xstring_size_ptr(basic_xstring_size_ptr& s) :
	str_(s.release()),
	nSize_(s.nSize_)
{
}

template<class XString>
qs::basic_xstring_size_ptr<XString>::~basic_xstring_size_ptr()
{
	XStringTraits<XString>::freeXString(str_);
}

template<class XString>
qs::basic_xstring_size_ptr<XString>& qs::basic_xstring_size_ptr<XString>::operator=(basic_xstring_size_ptr& s)
{
	if (s.get() != str_)
		reset(s.release(), s.nSize_);
	return *this;
}

template<class XString>
qs::basic_xstring_size_ptr<XString>::Char qs::basic_xstring_size_ptr<XString>::operator[](size_t n) const
{
	return str_[n];
}

template<class XString>
qs::basic_xstring_size_ptr<XString>::Char& qs::basic_xstring_size_ptr<XString>::operator[](size_t n)
{
	return str_[n];
}

template<class XString>
XString qs::basic_xstring_size_ptr<XString>::get() const
{
	return str_;
}

template<class XString>
XString qs::basic_xstring_size_ptr<XString>::release()
{
	XString str = str_;
	str_ = 0;
	return str;
}

template<class XString>
void qs::basic_xstring_size_ptr<XString>::reset(XString str,
												size_t nSize)
{
	XStringTraits<XString>::freeXString(str_);
	str_ = str;
	nSize_ = nSize;
}

template<class XString>
size_t qs::basic_xstring_size_ptr<XString>::size() const
{
	return nSize_;
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
bool qs::string_equal<Char>::operator()(const Char* plhs,
										const Char* prhs) const
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
bool qs::string_equal_i<Char>::operator()(const Char* plhs,
										  const Char* prhs) const
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
bool qs::string_less<Char>::operator()(const Char* plhs,
									   const Char* prhs) const
{
	return CharTraits<Char>::compare(plhs, prhs) < 0;
}


/****************************************************************************
 *
 * string_less_i
 *
 */

template<class Char>
bool qs::string_less_i<Char>::operator()(const Char* plhs,
										 const Char* prhs) const
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
qs::StringBuffer<String>::StringBuffer()
{
	init(0, 0);
}

template<class String>
qs::StringBuffer<String>::StringBuffer(size_t nLen)
{
	init(0, nLen);
}

template<class String>
qs::StringBuffer<String>::StringBuffer(const Char* psz)
{
	init(psz, -1);
}

template<class String>
qs::StringBuffer<String>::StringBuffer(const Char* psz,
									   size_t nLen)
{
	init(psz, nLen);
}

template<class String>
qs::StringBuffer<String>::~StringBuffer()
{
}

template<class String>
qs::basic_string_ptr<String> qs::StringBuffer<String>::getString()
{
	if (!str_.get())
		init(CharTraits<Char>::getEmptyBuffer(), 0);
	basic_string_ptr<String> str(str_);
	init(0, 0);
	return str;
}

template<class String>
const qs::StringBuffer<String>::Char* qs::StringBuffer<String>::getCharArray() const QNOTHROW()
{
	return str_.get() ? str_.get() : CharTraits<Char>::getEmptyBuffer();
}

template<class String>
size_t qs::StringBuffer<String>::getLength() const QNOTHROW()
{
	return pEnd_ - str_.get();
}

template<class String>
qs::StringBuffer<String>::Char qs::StringBuffer<String>::get(size_t n) const QNOTHROW()
{
	assert(n < getLength());
	return str_.get()[n];
}

template<class String>
void qs::StringBuffer<String>::append(const Char c)
{
	append(&c, 1);
}

template<class String>
void qs::StringBuffer<String>::append(const Char* psz)
{
	append(psz, -1);
}

template<class String>
void qs::StringBuffer<String>::append(const Char* psz,
									  size_t nLen)
{
	if (nLen == -1)
		nLen = CharTraits<Char>::getLength(psz);
	if (nLen == 0)
		return;
	
	if ((pEnd_ - str_.get()) + nLen > nLen_)
		allocBuffer(nLen);
	memcpy(pEnd_, psz, nLen*sizeof(Char));
	pEnd_ += nLen;
	*pEnd_ = Char();
}

template<class String>
void qs::StringBuffer<String>::insert(size_t nPos,
									  const Char c)
{
	insert(nPos, &c, 1);
}

template<class String>
void qs::StringBuffer<String>::insert(size_t nPos,
									  const Char* psz)
{
	insert(nPos, psz, -1);
}

template<class String>
void qs::StringBuffer<String>::insert(size_t nPos,
									  const Char* psz,
									  size_t nLen)
{
	assert(nPos <= getLength());
	
	if (nLen == -1)
		nLen = CharTraits<Char>::getLength(psz);
	
	// TODO
	// Performance improvement
	if ((pEnd_ - str_.get()) + nLen > nLen_)
		allocBuffer(nLen);
	
	memmove(str_.get() + nPos + nLen, str_.get() + nPos,
		(pEnd_ - str_.get() - nPos)*sizeof(Char));
	memcpy(str_.get() + nPos, psz, nLen*sizeof(Char));
	pEnd_ += nLen;
	*pEnd_ = Char();
}

template<class String>
void qs::StringBuffer<String>::remove() QNOTHROW()
{
	remove(0, -1);
}

template<class String>
void qs::StringBuffer<String>::remove(size_t nPos) QNOTHROW()
{
	remove(nPos, nPos + 1);
}

template<class String>
void qs::StringBuffer<String>::remove(size_t nStart,
									  size_t nEnd)
									  QNOTHROW()
{
	assert(nStart <= getLength());
	
	if (str_.get() && nStart != nEnd) {
		if (nEnd >= getLength()) {
			pEnd_ = str_.get() + nStart;
		}
		else {
			memmove(str_.get() + nStart, str_.get() + nEnd, (pEnd_ - str_.get() - nEnd)*sizeof(Char));
			pEnd_ -= nEnd - nStart;
		}
		*pEnd_ = Char();
	}
}

template<class String>
void qs::StringBuffer<String>::reserve(size_t nSize)
{
	if (nSize > nLen_)
		allocBuffer(nSize);
}

template<class String>
void qs::StringBuffer<String>::init(const Char* psz,
									size_t nLen)
{
	str_.reset(0);
	nLen_ = 0;
	pEnd_ = 0;
	
	if (nLen == -1)
		nLen = psz ? CharTraits<Char>::getLength(psz) : 0;
	
	if (!psz) {
		if (nLen != 0) {
			str_ = StringTraits<String>::allocString(nLen);
			*str_.get() = Char();
			pEnd_ = str_.get();
		}
	}
	else {
		str_ = StringTraits<String>::allocString(psz, nLen);
		pEnd_ = str_.get() + nLen;
	}
	nLen_ = nLen;
}

template<class String>
void qs::StringBuffer<String>::allocBuffer(size_t nLen)
{
	size_t nEnd = pEnd_ - str_.get();
	
	basic_string_ptr<String> str;
	size_t nNewLen = nLen_;
	if (nNewLen == 0)
		nNewLen = QSMAX(size_t(16), nLen);
	else
		nNewLen += QSMAX(nNewLen*2, nLen);
	str = StringTraits<String>::reallocString(str_, nNewLen);
	nLen_ = nNewLen;
	pEnd_ = str.get() + nEnd;
	str_ = str;
}


/****************************************************************************
 *
 * XStringBuffer
 *
 */

template<class XString>
qs::XStringBuffer<XString>::XStringBuffer()
{
	init(0, 0);
}

template<class XString>
qs::XStringBuffer<XString>::~XStringBuffer()
{
}

template<class XString>
qs::basic_xstring_ptr<XString> qs::XStringBuffer<XString>::getXString()
{
	if (!str_.get()) {
		if (!init(CharTraits<Char>::getEmptyBuffer(), 0))
			return 0;
	}
	basic_xstring_ptr<XString> str(str_);
	init(0, 0);
	return str;
}

template<class XString>
qs::basic_xstring_size_ptr<XString> qs::XStringBuffer<XString>::getXStringSize()
{
	if (!str_.get()) {
		if (!init(CharTraits<Char>::getEmptyBuffer(), 0))
			return basic_xstring_size_ptr<XString>();
	}
	basic_xstring_size_ptr<XString> str(str_, pEnd_ - str_.get());
	init(0, 0);
	return str;
}

template<class XString>
const qs::XStringBuffer<XString>::Char* qs::XStringBuffer<XString>::getCharArray() const
{
	return str_.get() ? str_.get() : CharTraits<Char>::getEmptyBuffer();
}

template<class XString>
size_t qs::XStringBuffer<XString>::getLength() const
{
	return pEnd_ - str_.get();
}

template<class XString>
qs::XStringBuffer<XString>::Char qs::XStringBuffer<XString>::get(size_t n) const
{
	assert(n < getLength());
	return str_.get()[n];
}

template<class XString>
bool qs::XStringBuffer<XString>::append(const Char c)
{
	return append(&c, 1);
}

template<class XString>
bool qs::XStringBuffer<XString>::append(const Char* psz)
{
	return append(psz, -1);
}

template<class XString>
bool qs::XStringBuffer<XString>::append(const Char* psz,
										size_t nLen)
{
	if (nLen == -1)
		nLen = CharTraits<Char>::getLength(psz);
	if (nLen == 0)
		return true;
	
	if ((pEnd_ - str_.get()) + nLen > nLen_) {
		if (!allocBuffer(nLen))
			return false;
	}
	memcpy(pEnd_, psz, nLen*sizeof(Char));
	pEnd_ += nLen;
	*pEnd_ = Char();
	
	return true;
}

template<class XString>
bool qs::XStringBuffer<XString>::insert(size_t nPos,
										const Char c)
{
	return insert(nPos, &c, 1);
}

template<class XString>
bool qs::XStringBuffer<XString>::insert(size_t nPos,
										const Char* psz)
{
	return insert(nPos, psz, -1);
}

template<class XString>
bool qs::XStringBuffer<XString>::insert(size_t nPos,
										const Char* psz,
										size_t nLen)
{
	assert(nPos <= getLength());
	
	if (nLen == -1)
		nLen = CharTraits<Char>::getLength(psz);
	
	// TODO
	// Performance improvement
	if ((pEnd_ - str_.get()) + nLen > nLen_) {
		if (!allocBuffer(nLen))
			return false;
	}
	
	memmove(str_.get() + nPos + nLen, str_.get() + nPos,
		(pEnd_ - str_.get() - nPos)*sizeof(Char));
	memcpy(str_.get() + nPos, psz, nLen*sizeof(Char));
	pEnd_ += nLen;
	*pEnd_ = Char();
	
	return true;
}

template<class XString>
void qs::XStringBuffer<XString>::remove()
{
	remove(0, -1);
}

template<class XString>
void qs::XStringBuffer<XString>::remove(size_t nPos)
{
	remove(nPos, nPos + 1);
}

template<class XString>
void qs::XStringBuffer<XString>::remove(size_t nStart,
										size_t nEnd)
{
	assert(nStart <= getLength());
	
	if (str_.get() && nStart != nEnd) {
		if (nEnd >= getLength()) {
			pEnd_ = str_.get() + nStart;
		}
		else {
			memmove(str_.get() + nStart, str_.get() + nEnd, (pEnd_ - str_.get() - nEnd)*sizeof(Char));
			pEnd_ -= nEnd - nStart;
		}
		*pEnd_ = Char();
	}
}

template<class XString>
bool qs::XStringBuffer<XString>::reserve(size_t nSize)
{
	if (nSize > nLen_)
		return allocBuffer(nSize);
	else
		return true;
}

template<class XString>
qs::XStringBuffer<XString>::Char* qs::XStringBuffer<XString>::lockBuffer(size_t nSize)
{
	size_t nNewSize = (pEnd_ - str_.get()) + nSize;
	if (nNewSize > nLen_) {
		if (!allocBuffer(nNewSize))
			return 0;
	}
	return pEnd_;
}

template<class XString>
void qs::XStringBuffer<XString>::unlockBuffer(size_t nSize)
{
	if (nSize == -1) {
		while (*pEnd_)
			++pEnd_;
	}
	else {
		pEnd_ += nSize;
		*pEnd_ = Char();
	}
}

template<class XString>
bool qs::XStringBuffer<XString>::init(const Char* psz,
									  size_t nLen)
{
	str_.reset(0);
	nLen_ = 0;
	pEnd_ = 0;
	
	if (nLen == -1)
		nLen = psz ? CharTraits<Char>::getLength(psz) : 0;
	
	if (!psz) {
		if (nLen != 0) {
			str_ = XStringTraits<XString>::allocXString(nLen);
			if (!str_.get())
				return false;
			*str_.get() = Char();
			pEnd_ = str_.get();
		}
	}
	else {
		str_ = XStringTraits<XString>::allocXString(psz, nLen);
		if (!str_.get())
			return false;
		pEnd_ = str_.get() + nLen;
	}
	nLen_ = nLen;
	
	return true;
}

template<class XString>
bool qs::XStringBuffer<XString>::allocBuffer(size_t nLen)
{
	size_t nEnd = pEnd_ - str_.get();
	
	basic_xstring_ptr<XString> str;
	size_t nNewLen = nLen_;
	if (nNewLen == 0)
		nNewLen = QSMAX(size_t(16), nLen);
	else
		nNewLen += QSMAX(nNewLen*2, nLen);
	str = XStringTraits<XString>::reallocXString(str_, nNewLen);
	if (!str.get())
		return false;
	nLen_ = nNewLen;
	pEnd_ = str.get() + nEnd;
	str_ = str;
	
	return true;
}


/****************************************************************************
 *
 * XStringBufferLock
 *
 */

template<class XString>
qs::XStringBufferLock<XString>::XStringBufferLock(XStringBuffer<XString>* pBuf,
												  size_t nSize) :
	pBuf_(pBuf),
	p_(0)
{
	p_ = pBuf_->lockBuffer(nSize);
}

template<class XString>
qs::XStringBufferLock<XString>::~XStringBufferLock()
{
	if (p_)
		pBuf_->unlockBuffer(0);
}

template<class XString>
qs::XStringBuffer<XString>::Char* qs::XStringBufferLock<XString>::get() const
{
	return p_;
}

template<class XString>
void qs::XStringBufferLock<XString>::unlock(size_t nSize)
{
	pBuf_->unlockBuffer(nSize);
	p_ = 0;
}


/****************************************************************************
 *
 * BMFindString
 *
 */

template<class String>
qs::BMFindString<String>::BMFindString(const Char* pszPattern) :
	nFlags_(0)
{
	init(pszPattern, -1, 0);
}

template<class String>
qs::BMFindString<String>::BMFindString(const Char* pszPattern,
									   size_t nLen) :
	nFlags_(0)
{
	init(pszPattern, nLen, 0);
}

template<class String>
qs::BMFindString<String>::BMFindString(const Char* pszPattern,
									   size_t nLen,
									   unsigned int nFlags) :
	nFlags_(0)
{
	init(pszPattern, nLen, nFlags);
}

template<class String>
qs::BMFindString<String>::~BMFindString()
{
}

template<class String>
const qs::BMFindString<String>::Char* qs::BMFindString<String>::find(const Char* psz) const
{
	return find(psz, -1);
}

template<class String>
const qs::BMFindString<String>::Char* qs::BMFindString<String>::find(const Char* psz,
																	 size_t nLen) const
{
	assert(psz);
	
	if (nLen == -1)
		nLen = CharTraits<Char>::getLength(psz);
	
	int nPatternLen = CharTraits<Char>::getLength(strPattern_.get());
	int n = nPatternLen - 1;
	while (n < static_cast<int>(nLen)) {
		int m = nPatternLen - 1;
		while (m >= 0 && isEqual(getChar(psz, nLen, n), strPattern_.get()[m])) {
			--n;
			--m;
		}
		if (m < 0)
			break;
		n += QSMAX(*(skip_ + (getChar(psz, nLen, n) & 0xff)), *(pNext_.get() + m));
	}
	return n < static_cast<int>(nLen) ? nFlags_ & FLAG_REVERSE ?
		psz + nLen - (n + nPatternLen + 1) : psz + (n + 1) : 0;
}

template<class String>
void qs::BMFindString<String>::init(const Char* pszPattern,
									size_t nLen,
									unsigned int nFlags)
{
	if (nLen == -1)
		nLen = CharTraits<Char>::getLength(pszPattern);
	
	nFlags_ = nFlags;
	
	if (nFlags_ & FLAG_IGNORECASE)
		strPattern_ = tolower(pszPattern, nLen);
	else
		strPattern_ = StringTraits<String>::allocString(pszPattern, nLen);
	
	if (nFlags_ & FLAG_REVERSE)
		std::reverse(strPattern_.get(), strPattern_.get() + nLen);
	
	createSkipTable(strPattern_.get(), nLen, skip_);
	pNext_ = createNextTable(strPattern_.get(), nLen);
}

template<class String>
void qs::BMFindString<String>::createSkipTable(const Char* pszPattern,
											   size_t nLen,
											   size_t* pSkip)
{
	for (int n = 0; n < 256; ++n)
		*(pSkip + n) = nLen;
	if (nLen != 0) {
		for (size_t m = 0; m < nLen - 1; ++m)
			*(pSkip + (pszPattern[m] & 0xff)) = nLen - m - 1;
	}
}

template<class String>
qs::auto_ptr_array<size_t> qs::BMFindString<String>::createNextTable(const Char* pszPattern,
																	 size_t nLen)
{
	auto_ptr_array<size_t> pNext(new size_t[nLen]);
	
	size_t n = 0;
	for (n = 0; n < nLen; ++n)
		*(pNext.get() + n) = nLen*2 - n - 1;
	auto_ptr_array<size_t> pTemp(new size_t[nLen]);
	for (int m = nLen - 1; m >= 0; --m) {
		*(pTemp.get() + m) = n;
		while (n != nLen && pszPattern[n] != pszPattern[m]) {
			*(pNext.get() + n) = QSMIN(*(pNext.get() + n), nLen - m - 1);
			n = *(pTemp.get() + n);
		}
		--n;
	}
	size_t l = n;
	for (n = 0; n < nLen; ++n) {
		*(pNext.get() + n) = QSMIN(*(pNext.get() + n), nLen + l - n);
		if (n >= l)
			l = *(pTemp.get() + l);
	}
	
	return pNext;
}

template<class String>
bool qs::BMFindString<String>::isEqual(Char lhs, Char rhs) const
{
	return nFlags_ & FLAG_IGNORECASE ?
		CharTraits<Char>::compareIgnoreCase(&lhs, &rhs, 1) == 0 :
		lhs == rhs;
}

template<class String>
qs::BMFindString<String>::Char qs::BMFindString<String>::getChar(const Char* psz,
																 size_t nLen,
																 size_t n) const
{
	Char c = nFlags_ & FLAG_REVERSE ? *(psz + nLen - n - 1) : *(psz + n);
	return nFlags_ & FLAG_IGNORECASE ? CharTraits<Char>::toLower(c) : c;
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
