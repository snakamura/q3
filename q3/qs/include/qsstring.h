/*
 * $Id: qsstring.h,v 1.1.1.1 2003/04/29 08:07:35 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QSSTRING_H__
#define __QSSTRING_H__

#include <qs.h>

#include <functional>


namespace qs {

/****************************************************************************
 *
 * String Allocation
 *
 */

//typedef char CHAR;
//typedef unsigned short WCHAR;
typedef CHAR* STRING;
typedef WCHAR* WSTRING;

#ifdef UNICODE
//typedef WCHAR TCHAR;
typedef WSTRING TSTRING;
#else
//typedef CHAR TCHAR;
typedef STRING TSTRING;
#endif

QSEXPORTPROC STRING allocString(size_t nSize);
QSEXPORTPROC STRING allocString(const CHAR* psz);
QSEXPORTPROC STRING allocString(const CHAR* psz, size_t nSize);
QSEXPORTPROC STRING reallocString(STRING str, size_t nSize);
QSEXPORTPROC void freeString(STRING str);

QSEXPORTPROC WSTRING allocWString(size_t nSize);
QSEXPORTPROC WSTRING allocWString(const WCHAR* pwsz);
QSEXPORTPROC WSTRING allocWString(const WCHAR* pwsz, size_t nSize);
QSEXPORTPROC WSTRING reallocWString(WSTRING wstr, size_t nSize);
QSEXPORTPROC void freeWString(WSTRING wstr);

#ifdef UNICODE
#	define allocTString allocWString
#	define reallocTString reallocWString
#	define freeTString freeWString
#else
#	define allocTString allocString
#	define reallocTString reallocString
#	define freeTString freeString
#endif


/****************************************************************************
 *
 * String Utilities
 *
 */

QSEXPORTPROC QSTATUS loadString(
	HINSTANCE hInstResource, UINT nId, WSTRING* pwstr);

QSEXPORTPROC STRING tolower(const CHAR* psz);
QSEXPORTPROC STRING tolower(const CHAR* psz, size_t nLen);
QSEXPORTPROC STRING toupper(const CHAR* psz);
QSEXPORTPROC STRING toupper(const CHAR* psz, size_t nLen);
QSEXPORTPROC WSTRING tolower(const WCHAR* pwsz);
QSEXPORTPROC WSTRING tolower(const WCHAR* pwsz, size_t nLen);
QSEXPORTPROC WSTRING toupper(const WCHAR* pwsz);
QSEXPORTPROC WSTRING toupper(const WCHAR* pwsz, size_t nLen);

QSEXPORTPROC WSTRING trim(const WCHAR* pwsz);
QSEXPORTPROC WSTRING trim(const WCHAR* pwsz, size_t nLen);

struct QSEXPORTCLASS Concat
{
	const CHAR* p_;
	size_t nLen_;
};

QSEXPORTPROC STRING concat(const CHAR* psz1, const CHAR* psz2);
QSEXPORTPROC STRING concat(const CHAR* psz1, size_t nLen1,
	const CHAR* psz2, size_t nLen2);
QSEXPORTPROC STRING concat(const CHAR* psz1,
	const CHAR* psz2, const CHAR* psz3);
QSEXPORTPROC STRING concat(const CHAR* psz1, size_t nLen1,
	const CHAR* psz2, size_t nLen2, const CHAR* psz3, size_t nLen3);
//QSEXPORTPROC STRING concat(const CHAR** ppsz, size_t nSize);
QSEXPORTPROC STRING concat(const Concat* pConcat, size_t nSize);

struct QSEXPORTCLASS ConcatW
{
	const WCHAR* p_;
	size_t nLen_;
};

QSEXPORTPROC WSTRING concat(const WCHAR* pwsz1, const WCHAR* pwsz2);
QSEXPORTPROC WSTRING concat(const WCHAR* pwsz1, size_t nLen1,
	const WCHAR* pwsz2, size_t nLen2);
QSEXPORTPROC WSTRING concat(const WCHAR* pwsz1,
	const WCHAR* pwsz2, const WCHAR* pwsz3);
QSEXPORTPROC WSTRING concat(const WCHAR* pwsz1, size_t nLen1,
	const WCHAR* pwsz2, size_t nLen2, const WCHAR* pwsz3, size_t nLen3);
//QSEXPORTPROC WSTRING concat(const WCHAR** ppwsz, size_t nSize);
QSEXPORTPROC WSTRING concat(const ConcatW* pConcat, size_t nSize);


/****************************************************************************
 *
 * CharTraits
 *
 */

template<class Char>
struct CharTraits
{
};

template<>
struct CharTraits<CHAR>
{
	static size_t getLength(const CHAR* psz);
	static const CHAR* getEmptyBuffer();
	static int compare(const CHAR* lhs, const CHAR* rhs);
	static int compare(const CHAR* lhs, const CHAR* rhs, size_t nLen);
	static int compareIgnoreCase(const CHAR* lhs, const CHAR* rhs);
	static int compareIgnoreCase(const CHAR* lhs, const CHAR* rhs, size_t nLen);
};

template<>
struct CharTraits<WCHAR>
{
	static size_t getLength(const WCHAR* pwsz);
	static const WCHAR* getEmptyBuffer();
	static int compare(const WCHAR* lhs, const WCHAR* rhs);
	static int compare(const WCHAR* lhs, const WCHAR* rhs, size_t nLen);
	static int compareIgnoreCase(const WCHAR* lhs, const WCHAR* rhs);
	static int compareIgnoreCase(const WCHAR* lhs, const WCHAR* rhs, size_t nLen);
};


/****************************************************************************
 *
 * StringTraits
 *
 */

template<class String>
struct StringTraits
{
};

template<>
struct StringTraits<STRING>
{
	typedef CHAR char_type;
	
	static STRING allocString(size_t nLen);
	static STRING allocString(const CHAR* psz, size_t nLen);
	static STRING reallocString(STRING str, size_t nLen);
	static void freeString(STRING str);
};

template<>
struct StringTraits<WSTRING>
{
	typedef WCHAR char_type;
	
	static WSTRING allocString(size_t nLen);
	static WSTRING allocString(const WCHAR* psz, size_t nLen);
	static WSTRING reallocString(WSTRING str, size_t nLen);
	static void freeString(WSTRING str);
};


/****************************************************************************
 *
 * string_ptr
 *
 */

template<class String>
class string_ptr
{
public:
	typedef StringTraits<String>::char_type Char;

public:
	string_ptr();
	string_ptr(String str);
	string_ptr(string_ptr& s);
	~string_ptr();

public:
	string_ptr& operator=(string_ptr& s);
	String* operator&();
	Char operator[](size_t n) const;
	Char& operator[](size_t n);

public:
	String get() const;
	String release();
	void reset(String str);
	string_ptr* getThis();

private:
	String str_;
};


/****************************************************************************
 *
 * string_hash
 *
 */

template<class Char>
struct string_hash : public std::unary_function<const Char*, int>
{
	int operator()(const Char* p) const;
};


/****************************************************************************
 *
 * string_equal
 *
 */

template<class Char>
struct string_equal :
	public std::binary_function<const Char*, const Char*, bool>
{
	bool operator()(const Char* plhs, const Char* prhs) const;
};


/****************************************************************************
 *
 * string_equal_i
 *
 */

template<class Char>
struct string_equal_i :
	public std::binary_function<const Char*, const Char*, bool>
{
	bool operator()(const Char* plhs, const Char* prhs) const;
};


/****************************************************************************
 *
 * string_less
 *
 */

template<class Char>
struct string_less :
	public std::binary_function<const Char*, const Char*, bool>
{
	bool operator()(const Char* plhs, const Char* prhs) const;
};


/****************************************************************************
 *
 * string_less_i
 *
 */

template<class Char>
struct string_less_i :
	public std::binary_function<const Char*, const Char*, bool>
{
	bool operator()(const Char* plhs, const Char* prhs) const;
};


/****************************************************************************
 *
 * string_free
 *
 */

template<class String>
struct string_free : public std::unary_function<String, void*>
{
	void* operator()(String str) const;
};


/****************************************************************************
 *
 * StringBuffer
 *
 */

template<class String>
class QSEXPORTCLASS StringBuffer
{
public:
	typedef StringTraits<String>::char_type Char;

public:
	explicit StringBuffer(QSTATUS* pstatus);
	StringBuffer(size_t nLen, QSTATUS* pstatus);
	StringBuffer(const Char* psz, QSTATUS* pstatus);
	StringBuffer(const Char* psz, size_t nLen, QSTATUS* pstatus);
	~StringBuffer();

public:
	String getString();
	const Char* getCharArray() const;
	size_t getLength() const;
	Char get(size_t n) const;
	QSTATUS append(const Char c);
	QSTATUS append(const Char* psz);
	QSTATUS append(const Char* psz, size_t nLen);
	QSTATUS insert(size_t nPos, const Char c);
	QSTATUS insert(size_t nPos, const Char* psz);
	QSTATUS insert(size_t nPos, const Char* psz, size_t nLen);
	QSTATUS remove();
	QSTATUS remove(size_t nPos);
	QSTATUS remove(size_t nStart, size_t nEnd);

private:
	QSTATUS init(const Char* pwsz, size_t nLen);
	QSTATUS allocBuffer(size_t nLen);

private:
	StringBuffer(const StringBuffer&);
	StringBuffer& operator=(const StringBuffer&);

private:
	String str_;
	size_t nLen_;
	Char* pEnd_;
};


/****************************************************************************
 *
 * StringTokenizer
 *
 */

class QSEXPORTCLASS StringTokenizer
{
public:
	StringTokenizer(const WCHAR* pwsz,
		const WCHAR* pwszDelimiter, QSTATUS* pstatus);
	~StringTokenizer();

public:
	unsigned int getCount() const;
	const WCHAR* get(unsigned int n) const;

private:
	StringTokenizer(const StringTokenizer&);
	StringTokenizer& operator=(const StringTokenizer&);

private:
	struct StringTokenizerImpl* pImpl_;
};


/****************************************************************************
 *
 * BMFindString
 *
 */

template<class String>
class BMFindString
{
public:
	enum Flag {
		FLAG_IGNORECASE	= 0x01,
		FLAG_REVERSE	= 0x02
	};

public:
	typedef StringTraits<String>::char_type Char;

public:
	BMFindString(const Char* pszPattern, QSTATUS* pstatus);
	BMFindString(const Char* pszPattern, size_t nLen, QSTATUS* pstatus);
	BMFindString(const Char* pszPattern, size_t nLen,
		unsigned int nFlags, QSTATUS* pstatus);
	~BMFindString();
	
public:
	const Char* find(const Char* psz) const;
	const Char* find(const Char* psz, size_t nLen) const;

private:
	QSTATUS init(const Char* pszPattern, size_t nLen, unsigned int nFlags);
	QSTATUS createSkipTable();
	QSTATUS createNextTable();
	bool isEqual(Char lhs, Char rhs) const;
	Char getChar(const Char* psz, size_t nLen, size_t n) const;

private:
	BMFindString(const BMFindString&);
	BMFindString& operator=(const BMFindString&);

private:
	int skip_[256];
	int* pNext_;
	String strPattern_;
	unsigned int nFlags_;
};


/****************************************************************************
 *
 * StringListFree
 *
 */

template<class List>
class StringListFree
{
public:
	StringListFree(List& l);
	~StringListFree();

public:
	void release();

private:
	StringListFree(const StringListFree&);
	StringListFree& operator=(const StringListFree&);

private:
	List* p_;
};

}

#include <qsstring.inl>

#endif // __QSSTRING_H__
