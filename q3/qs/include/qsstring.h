/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QSSTRING_H__
#define __QSSTRING_H__

#include <qs.h>
#include <qsstl.h>

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
typedef CHAR* XSTRING;
typedef WCHAR* WXSTRING;

#ifdef UNICODE
//typedef WCHAR TCHAR;
typedef WSTRING TSTRING;
#else
//typedef CHAR TCHAR;
typedef STRING TSTRING;
#endif

template<class String> class basic_string_ptr;
typedef basic_string_ptr<STRING> string_ptr;
typedef basic_string_ptr<WSTRING> wstring_ptr;
typedef basic_string_ptr<TSTRING> tstring_ptr;

template<class XString> class basic_xstring_ptr;
typedef basic_xstring_ptr<XSTRING> xstring_ptr;
typedef basic_xstring_ptr<WXSTRING> wxstring_ptr;

template<class XString> class basic_xstring_size_ptr;
typedef basic_xstring_size_ptr<XSTRING> xstring_size_ptr;
typedef basic_xstring_size_ptr<WXSTRING> wxstring_size_ptr;


/**
 * Allocate string.
 *
 * @param nSize [in] Size to allocate.
 * @return Allocated string. Cannot be null.
 * @exception std::bad_alloc When allocation failed.
 */
QSEXPORTPROC string_ptr allocString(size_t nSize);

/**
 * Allocate string.
 *
 * @param psz [in] Allocate string.
 * @return Allocated string. Cannot be null.
 * @exception std::bad_alloc Out of memory.
 */
QSEXPORTPROC string_ptr allocString(const CHAR* psz);

/**
 * Allocate string.
 *
 * @param psz [in] Allocate string.
 * @param nSize [in] Allocate size.
 * @return Allocated string. Can not be null.
 * @exception std::bad_alloc Out of memory.
 */
QSEXPORTPROC string_ptr allocString(const CHAR* psz,
									size_t nSize);

/**
 * Reallocate string.
 *
 * @param str [in] Reallocate string.
 * @param nSize [in] Allocate size.
 * @return Reallocated string. Can not be null.
 * @exception std::bad_alloc Out of memory.
 */
QSEXPORTPROC string_ptr reallocString(string_ptr str,
									  size_t nSize);

/**
 * Deallocate string.
 *
 * @param str [in] Deallocate string.
 */
QSEXPORTPROC void freeString(STRING str) QNOTHROW();

/**
 * Allocate string.
 *
 * @param nSize [in] Size to allocate.
 * @return Allocated string. null if allocation failed.
 */
QSEXPORTPROC xstring_ptr allocXString(size_t nSize) QNOTHROW();

/**
 * Allocate string.
 *
 * @param psz [in] Allocate string.
 * @return Allocated string. null if out of memory.
 */
QSEXPORTPROC xstring_ptr allocXString(const CHAR* psz) QNOTHROW();

/**
 * Allocate string.
 *
 * @param psz [in] Allocate string.
 * @param nSize [in] Allocate size.
 * @return Allocated string. null if out of memory.
 */
QSEXPORTPROC xstring_ptr allocXString(const CHAR* psz,
									  size_t nSize)
									  QNOTHROW();

/**
 * Reallocate string.
 *
 * @param str [in] Reallocate string.
 * @param nSize [in] Allocate size.
 * @return Reallocated string. null if out of memory.
 */
QSEXPORTPROC xstring_ptr reallocXString(xstring_ptr str,
										size_t nSize)
										QNOTHROW();

/**
 * Deallocate string.
 *
 * @param str [in] Deallocate string.
 */
QSEXPORTPROC void freeXString(XSTRING str) QNOTHROW();

/**
 * Allocate string with the specified size.
 *
 * @param nSize Allocate size.
 * @return Allocated string. Can not be null.
 * @exception std::bad_alloc Out of memory.
 */
QSEXPORTPROC wstring_ptr allocWString(size_t nSize);

/**
 * Allocate string.
 *
 * @param pwsz [in] Allocate string.
 * @return Allocated string. Can not be null.
 * @exception std::bad_alloc Out of memory.
 */
QSEXPORTPROC wstring_ptr allocWString(const WCHAR* pwsz);

/**
 * Allocate string.
 *
 * @param pwsz [in] Allocate string.
 * @param nSize [in] Allocate size.
 * @return Allocated string. Can not be null.
 * @exception std::bad_alloc Out of memory.
 */
QSEXPORTPROC wstring_ptr allocWString(const WCHAR* pwsz,
									  size_t nSize);

/**
 * Reallocate string.
 *
 * @param wstr [in] Reallocate string.
 * @param nSize [in] Allocate size.
 * @return Reallocated string. Can not be null.
 * @exception std::bad_alloc Out of memory.
 */
QSEXPORTPROC wstring_ptr reallocWString(wstring_ptr wstr,
										size_t nSize);

/**
 * Deallocate string.
 *
 * @param wstr [in] Deallocate string.
 */
QSEXPORTPROC void freeWString(WSTRING wstr) QNOTHROW();

/**
 * Allocate string with the specified size.
 *
 * @param nSize [in] Allocate size.
 * @return Allocated string. null if out of memory.
 */
QSEXPORTPROC wxstring_ptr allocWXString(size_t nSize) QNOTHROW();

/**
 * Allocate string.
 *
 * @param pwsz [in] Allocate string.
 * @return Allocated string. null if out of memory.
 */
QSEXPORTPROC wxstring_ptr allocWXString(const WCHAR* pwsz) QNOTHROW();

/**
 * Allocate string.
 *
 * @param psz [in] Allocate string.
 * @param nSize [in] Allocate size.
 * @return Allocated string. null if out of memory.
 */
QSEXPORTPROC wxstring_ptr allocWXString(const WCHAR* pwsz,
										size_t nSize)
										QNOTHROW();

/**
 * Reallocate string.
 *
 * @param wstr [in] Reallocate string.
 * @param nSize [in] Allocate size.
 * @return Reallocated string. null if out of memory.
 */
QSEXPORTPROC wxstring_ptr reallocWXString(wxstring_ptr wstr,
										  size_t nSize)
										  QNOTHROW();

/**
 * Deallocate string.
 *
 * @param wstr [in] Deallocate string.
 */
QSEXPORTPROC void freeWXString(WXSTRING wstr) QNOTHROW();


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

/**
 * Load string from the resource.
 *
 * @param hInstResource [in] Instance handle of resource.
 * @param nId [in] Resource id.
 * @return Loaded string. null if error occured.
 * @exception std::bad_alloc Out of memory.
 */
QSEXPORTPROC wstring_ptr loadString(HINSTANCE hInstResource,
									UINT nId);

/**
 * Make string lower.
 *
 * @param psz [in] String.
 * @return Lowered string. Can not be null.
 * @exception std::bad_alloc Out of memory.
 */
QSEXPORTPROC string_ptr tolower(const CHAR* psz);

/**
 * Make string lower.
 *
 * @param psz [in] String.
 * @param nLen [in] String length.
 * @return Lowered string. Can not be null.
 * @exception std::bad_alloc Out of memory.
 */
QSEXPORTPROC string_ptr tolower(const CHAR* psz,
								size_t nLen);

/**
 * Make string upper.
 *
 * @param psz [in] String.
 * @return Uppered string. Can not be null.
 * @exception std::bad_alloc Out of memory.
 */
QSEXPORTPROC string_ptr toupper(const CHAR* psz);

/**
 * Make string upper.
 *
 * @param psz [in] String.
 * @param nLen [in] String length.
 * @return Uppered string. Can not be null.
 * @exception std::bad_alloc Out of memory.
 */
QSEXPORTPROC string_ptr toupper(const CHAR* psz,
								size_t nLen);


/**
 * Make string lower.
 *
 * @param pwsz [in] String.
 * @return Lowered string. Can not be null.
 * @exception std::bad_alloc Out of memory.
 */
QSEXPORTPROC wstring_ptr tolower(const WCHAR* pwsz);

/**
 * Make string lower.
 *
 * @param pwsz [in] String.
 * @param nLen [in] String length.
 * @return Lowered string. Can not be null.
 * @exception std::bad_alloc Out of memory.
 */
QSEXPORTPROC wstring_ptr tolower(const WCHAR* pwsz,
								 size_t nLen);

/**
 * Make string upper.
 *
 * @param pwsz [in] String.
 * @return Uppered string. Can not be null.
 * @exception std::bad_alloc Out of memory.
 */
QSEXPORTPROC wstring_ptr toupper(const WCHAR* pwsz);

/**
 * Make string upper.
 *
 * @param pwsz [in] String.
 * @param nLen [in] String length.
 * @return Uppered string. Can not be null.
 * @exception std::bad_alloc Out of memory.
 */
QSEXPORTPROC wstring_ptr toupper(const WCHAR* pwsz,
								 size_t nLen);

/**
 * Trim whitespace.
 *
 * @param pwsz [in] String.
 * @return Trimmed string. Can not be null.
 * @exception std::bad_alloc Out of memory.
 */
QSEXPORTPROC wstring_ptr trim(const WCHAR* pwsz);

/**
 * Trim whitespace.
 *
 * @param pwsz [in] String.
 * @param nLen [in] String length.
 * @return Trimmed string. Can not be null.
 * @exception std::bad_alloc Out of memory.
 */
QSEXPORTPROC wstring_ptr trim(const WCHAR* pwsz,
							  size_t nLen);


struct QSEXPORTCLASS Concat
{
	const CHAR* p_;
	size_t nLen_;
};

/**
 * Concatinate strings.
 *
 * @param psz1 [in] String.
 * @param psz2 [in] String.
 * @return Concatinated string. Can not be null.
 * @exception std::bad_alloc Out of memory.
 */
QSEXPORTPROC string_ptr concat(const CHAR* psz1,
							   const CHAR* psz2);

/**
 * Concatinate strings.
 *
 * @param psz1 [in] String.
 * @param nLen1 [in] String length.
 * @param psz2 [in] String.
 * @param nLen2 [in] String length.
 * @return Concatinated string. Can not be null.
 * @exception std::bad_alloc Out of memory.
 */
QSEXPORTPROC string_ptr concat(const CHAR* psz1,
							   size_t nLen1,
							   const CHAR* psz2,
							   size_t nLen2);

/**
 * Concatinate strings.
 *
 * @param psz1 [in] String.
 * @param psz2 [in] String.
 * @param psz3 [in] String.
 * @return Concatinated string. Can not be null.
 * @exception std::bad_alloc Out of memory.
 */
QSEXPORTPROC string_ptr concat(const CHAR* psz1,
							   const CHAR* psz2,
							   const CHAR* psz3);

/**
 * Concatinate strings.
 *
 * @param psz1 [in] String.
 * @param nLen1 [in] String length.
 * @param psz2 [in] String.
 * @param nLen2 [in] String length.
 * @param psz3 [in] String.
 * @param nLen3 [in] String length.
 * @return Concatinated string. Can not be null.
 * @exception std::bad_alloc Out of memory.
 */
QSEXPORTPROC string_ptr concat(const CHAR* psz1,
							   size_t nLen1,
							   const CHAR* psz2,
							   size_t nLen2,
							   const CHAR* psz3,
							   size_t nLen3);

/**
 * Concatinate strings.
 *
 * @param pConcat [in] concatinated data.
 * @param nSize [in] Data size.
 * @return Concatinated string. Can not be null.
 * @exception std::bad_alloc Out of memory.
 */
QSEXPORTPROC string_ptr concat(const Concat* pConcat,
							   size_t nSize);


struct QSEXPORTCLASS ConcatW
{
	const WCHAR* p_;
	size_t nLen_;
};

/**
 * Concatinate strings.
 *
 * @param pwsz1 [in] String.
 * @param pwsz2 [in] String.
 * @return Concatinated string. Can not be null.
 * @exception std::bad_alloc Out of memory.
 */
QSEXPORTPROC wstring_ptr concat(const WCHAR* pwsz1,
								const WCHAR* pwsz2);

/**
 * Concatinate strings.
 *
 * @param pwsz1 [in] String.
 * @param nLen1 [in] String length.
 * @param pwsz2 [in] String.
 * @param nLen2 [in] String length.
 * @return Concatinated string. Can not be null.
 * @exception std::bad_alloc Out of memory.
 */
QSEXPORTPROC wstring_ptr concat(const WCHAR* pwsz1,
								size_t nLen1,
								const WCHAR* pwsz2,
								size_t nLen2);

/**
 * Concatinate strings.
 *
 * @param pwsz1 [in] String.
 * @param pwsz2 [in] String.
 * @param pwsz3 [in] String.
 * @return Concatinated string. Can not be null.
 * @exception std::bad_alloc Out of memory.
 */
QSEXPORTPROC wstring_ptr concat(const WCHAR* pwsz1,
								const WCHAR* pwsz2,
								const WCHAR* pwsz3);

/**
 * Concatinate strings.
 *
 * @param pwsz1 [in] String.
 * @param nLen1 [in] String length.
 * @param pwsz2 [in] String.
 * @param nLen2 [in] String length.
 * @param pwsz3 [in] String.
 * @param nLen3 [in] String length.
 * @return Concatinated string. Can not be null.
 * @exception std::bad_alloc Out of memory.
 */
QSEXPORTPROC wstring_ptr concat(const WCHAR* pwsz1,
								size_t nLen1,
								const WCHAR* pwsz2,
								size_t nLen2,
								const WCHAR* pwsz3,
								size_t nLen3);

/**
 * Concatinate strings.
 *
 * @param pConcat [in] concatinated data.
 * @param nSize [in] Data size.
 * @return Concatinated string. Can not be null.
 * @exception std::bad_alloc Out of memory.
 */
QSEXPORTPROC wstring_ptr concat(const ConcatW* pConcat,
								size_t nSize);


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
	static CHAR toLower(CHAR c);
	static int compare(const CHAR* lhs,
					   const CHAR* rhs);
	static int compare(const CHAR* lhs,
					   const CHAR* rhs,
					   size_t nLen);
	static int compareIgnoreCase(const CHAR* lhs,
								 const CHAR* rhs);
	static int compareIgnoreCase(const CHAR* lhs,
								 const CHAR* rhs,
								 size_t nLen);
};

template<>
struct CharTraits<WCHAR>
{
	static size_t getLength(const WCHAR* pwsz);
	static const WCHAR* getEmptyBuffer();
	static WCHAR toLower(WCHAR c);
	static int compare(const WCHAR* lhs,
					   const WCHAR* rhs);
	static int compare(const WCHAR* lhs,
					   const WCHAR* rhs,
					   size_t nLen);
	static int compareIgnoreCase(const WCHAR* lhs,
								 const WCHAR* rhs);
	static int compareIgnoreCase(const WCHAR* lhs,
								 const WCHAR* rhs,
								 size_t nLen);
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
	
	static string_ptr allocString(size_t nLen);
	static string_ptr allocString(const CHAR* psz,
								  size_t nLen);
	static string_ptr reallocString(string_ptr str,
									size_t nLen);
	static void freeString(STRING str);
};

template<>
struct StringTraits<WSTRING>
{
	typedef WCHAR char_type;
	
	static wstring_ptr allocString(size_t nLen);
	static wstring_ptr allocString(const WCHAR* psz,
								   size_t nLen);
	static wstring_ptr reallocString(wstring_ptr str,
									 size_t nLen);
	static void freeString(WSTRING str);
};


/****************************************************************************
 *
 * XStringTraits
 *
 */

template<class XString>
struct XStringTraits
{
};

template<>
struct XStringTraits<XSTRING>
{
	typedef CHAR char_type;
	
	static xstring_ptr allocXString(size_t nLen) QNOTHROW();
	static xstring_ptr allocXString(const CHAR* psz,
									size_t nLen)
									QNOTHROW();
	static xstring_ptr reallocXString(xstring_ptr str,
									  size_t nLen)
									  QNOTHROW();
	static void freeXString(XSTRING str);
};

template<>
struct XStringTraits<WXSTRING>
{
	typedef WCHAR char_type;
	
	static wxstring_ptr allocXString(size_t nLen) QNOTHROW();
	static wxstring_ptr allocXString(const WCHAR* psz,
									 size_t nLen)
									 QNOTHROW();
	static wxstring_ptr reallocXString(wxstring_ptr str,
									   size_t nLen)
									   QNOTHROW();
	static void freeXString(WXSTRING str);
};


/****************************************************************************
 *
 * basic_string_ptr
 *
 */

template<class String>
class basic_string_ptr
{
public:
	typedef StringTraits<String>::char_type Char;

public:
	basic_string_ptr();
	basic_string_ptr(String str);
	basic_string_ptr(basic_string_ptr& s);
	~basic_string_ptr();

public:
	basic_string_ptr& operator=(basic_string_ptr& s);
	Char operator[](size_t n) const;
	Char& operator[](size_t n);

public:
	String get() const;
	String release();
	void reset(String str);
	basic_string_ptr* getThis();

private:
	String str_;
};


/****************************************************************************
 *
 * basic_xstring_ptr
 *
 */

template<class XString>
class basic_xstring_ptr
{
public:
	typedef XStringTraits<XString>::char_type Char;

public:
	basic_xstring_ptr();
	basic_xstring_ptr(XString str);
	basic_xstring_ptr(basic_xstring_ptr& s);
	~basic_xstring_ptr();

public:
	basic_xstring_ptr& operator=(basic_xstring_ptr& s);
	Char operator[](size_t n) const;
	Char& operator[](size_t n);

public:
	XString get() const;
	XString release();
	void reset(XString str);
	basic_xstring_ptr* getThis();

private:
	XString str_;
};


/****************************************************************************
 *
 * basic_xstring_size_ptr
 *
 */

template<class XString>
class basic_xstring_size_ptr
{
public:
	typedef XStringTraits<XString>::char_type Char;

public:
	basic_xstring_size_ptr();
	basic_xstring_size_ptr(XString str,
						   size_t nSize);
	basic_xstring_size_ptr(basic_xstring_ptr<XString> str,
						   size_t nSize);
	basic_xstring_size_ptr(basic_xstring_size_ptr& s);
	~basic_xstring_size_ptr();

public:
	basic_xstring_size_ptr& operator=(basic_xstring_size_ptr& s);
	Char operator[](size_t n) const;
	Char& operator[](size_t n);

public:
	XString get() const;
	XString release();
	void reset(XString str,
			   size_t nSize);
	size_t size() const;

private:
	XString str_;
	size_t nSize_;
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
struct string_equal : public std::binary_function<const Char*, const Char*, bool>
{
	bool operator()(const Char* plhs,
					const Char* prhs) const;
};


/****************************************************************************
 *
 * string_equal_i
 *
 */

template<class Char>
struct string_equal_i : public std::binary_function<const Char*, const Char*, bool>
{
	bool operator()(const Char* plhs,
					const Char* prhs) const;
};


/****************************************************************************
 *
 * string_less
 *
 */

template<class Char>
struct string_less : public std::binary_function<const Char*, const Char*, bool>
{
	bool operator()(const Char* plhs,
					const Char* prhs) const;
};


/****************************************************************************
 *
 * string_less_i
 *
 */

template<class Char>
struct string_less_i : public std::binary_function<const Char*, const Char*, bool>
{
	bool operator()(const Char* plhs,
					const Char* prhs) const;
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
	/**
	 * Create instance.
	 *
	 * @exception std::bad_alloc Out of memory.
	 */
	StringBuffer();
	
	/**
	 * Create instance with the specifid size.
	 *
	 * @param nLen [in] Initial size.
	 * @exception std::bad_alloc Out of memory.
	 */
	explicit StringBuffer(size_t nLen);
	
	/**
	 * Create instance with the specified string.
	 *
	 * @param psz [in] String.
	 * @exception std::bad_alloc Out of memory.
	 */
	explicit StringBuffer(const Char* psz);
	
	/**
	 * Create instance with the specified string and size.
	 *
	 * @param psz [in] String.
	 * @param nLen [in] Initial size.
	 * @exception std::bad_alloc Out of memory.
	 */
	StringBuffer(const Char* psz,
				 size_t nLen);
	
	~StringBuffer();

public:
	/**
	 * Get internal string. After calling this method,
	 * this string buffer is reset empty.
	 *
	 * @return Internal string. Can not be null.
	 * @exception std::bad_alloc Out of memory.
	 */
	basic_string_ptr<String> getString();
	
	/**
	 * Get internal character array.
	 *
	 * @return Character array. Can not be null.
	 */
	const Char* getCharArray() const QNOTHROW();
	
	/**
	 * Get buffer length.
	 *
	 * @return Buffer length.
	 */
	size_t getLength() const QNOTHROW();
	
	/**
	 * Get character of the specified position.
	 *
	 * @param n [in] Position.
	 * @return Character.
	 */
	Char get(size_t n) const QNOTHROW();
	
	/**
	 * Append the specified character.
	 *
	 * @param c [in] Character.
	 * @exception std::bad_alloc Out of memory.
	 */
	void append(const Char c);
	
	/**
	 * Append the specified string.
	 *
	 * @param psz [in] String.
	 * @exception std::bad_alloc Out of memory.
	 */
	void append(const Char* psz);
	
	/**
	 * Append the specified string.
	 *
	 * @param psz [in] String.
	 * @param nLen [in] Length.
	 * @exception std::bad_alloc Out of memory.
	 */
	void append(const Char* psz,
				size_t nLen);
	
	/**
	 * Insert the specified character at the specified position.
	 *
	 * @param nPos [in] Position.
	 * @param c [in] Character.
	 * @exception std::bad_alloc Out of memory.
	 */
	void insert(size_t nPos,
				const Char c);
	
	/**
	 * Insert the specified string at the specified position.
	 *
	 * @param nPos [in] Position.
	 * @param psz [in] String.
	 * @exception std::bad_alloc Out of memory.
	 */
	void insert(size_t nPos,
				const Char* psz);
	
	/**
	 * Insert the specified string and length at the specified position.
	 *
	 * @param nPos [in] Position.
	 * @param psz [in] String.
	 * @param nLen [in] Length.
	 * @exception std::bad_alloc Out of memory.
	 */
	void insert(size_t nPos,
				const Char* psz,
				size_t nLen);
	
	/**
	 * Empty buffer.
	 */
	void remove() QNOTHROW();
	
	/**
	 * Remove the character at the specified position.
	 *
	 * @param nPos [in] Position.
	 */
	void remove(size_t nPos) QNOTHROW();
	
	/**
	 * Remove the characters from nStart through nEnd.
	 *
	 * @param nStart [in] Start position.
	 * @param nEnd [in] End position.
	 */
	void remove(size_t nStart,
				size_t nEnd)
				QNOTHROW();
	
	/**
	  * Reserve space.
	  *
	  * @param nSize reserve size.
	  */
	void reserve(size_t nSize);

private:
	void init(const Char* pwsz,
			  size_t nLen);
	void allocBuffer(size_t nLen);

private:
	StringBuffer(const StringBuffer&);
	StringBuffer& operator=(const StringBuffer&);

private:
	basic_string_ptr<String> str_;
	size_t nLen_;
	Char* pEnd_;
};


/****************************************************************************
 *
 * XStringBuffer
 *
 */

template<class XString>
class QSEXPORTCLASS XStringBuffer
{
public:
	typedef XStringTraits<XString>::char_type Char;

public:
	/**
	 * Create instance.
	 */
	XStringBuffer();
	
	~XStringBuffer();

public:
	/**
	 * Get internal string. After calling this method,
	 * this string buffer is reset empty.
	 *
	 * @return Internal string. null if failed.
	 */
	basic_xstring_ptr<XString> getXString();
	
	/**
	 * Get internal string. After calling this method,
	 * this string buffer is reset empty.
	 *
	 * @return Internal string. null if failed.
	 */
	basic_xstring_size_ptr<XString> getXStringSize();
	
	/**
	 * Get internal character array.
	 *
	 * @return Character array. Can not be null.
	 */
	const Char* getCharArray() const;
	
	/**
	 * Get buffer length.
	 *
	 * @return Buffer length.
	 */
	size_t getLength() const;
	
	/**
	 * Get character of the specified position.
	 *
	 * @param n [in] Position.
	 * @return Character.
	 */
	Char get(size_t n) const;
	
	/**
	 * Append the specified character.
	 *
	 * @param c [in] Character.
	 * @return true if success, false otherwise.
	 */
	bool append(const Char c);
	
	/**
	 * Append the specified string.
	 *
	 * @param psz [in] String.
	 * @return true if success, false otherwise.
	 */
	bool append(const Char* psz);
	
	/**
	 * Append the specified string.
	 *
	 * @param psz [in] String.
	 * @param nLen [in] Length.
	 * @return true if success, false otherwise.
	 */
	bool append(const Char* psz,
				size_t nLen);
	
	/**
	 * Insert the specified character at the specified position.
	 *
	 * @param nPos [in] Position.
	 * @param c [in] Character.
	 * @return true if success, false otherwise.
	 */
	bool insert(size_t nPos,
				const Char c);
	
	/**
	 * Insert the specified string at the specified position.
	 *
	 * @param nPos [in] Position.
	 * @param psz [in] String.
	 * @return true if success, false otherwise.
	 */
	bool insert(size_t nPos,
				const Char* psz);
	
	/**
	 * Insert the specified string and length at the specified position.
	 *
	 * @param nPos [in] Position.
	 * @param psz [in] String.
	 * @param nLen [in] Length.
	 * @return true if success, false otherwise.
	 */
	bool insert(size_t nPos,
				const Char* psz,
				size_t nLen);
	
	/**
	 * Empty buffer.
	 */
	void remove();
	
	/**
	 * Remove the character at the specified position.
	 *
	 * @param nPos [in] Position.
	 */
	void remove(size_t nPos);
	
	/**
	 * Remove the characters from nStart through nEnd.
	 *
	 * @param nStart [in] Start position.
	 * @param nEnd [in] End position.
	 */
	void remove(size_t nStart,
				size_t nEnd);
	
	/**
	  * Reserve space.
	  *
	  * @param nSize reserve size.
	  * @return true if success, false otherwise.
	  */
	bool reserve(size_t nSize);
	
	/**
	 * Reserve buffer and return buffer directly.
	 * getLength() + nSize is allocated,
	 * and getCharArray() + getLength() is returned.
	 *
	 * @param nSize reserve size.
	 * @return buffer if success, null otherwise.
	 */
	Char* lockBuffer(size_t nSize);
	
	/**
	 * Release buffer locked by lockBuffer.
	 *
	 * @param nSize New buffer size. if -1, buffer must be terminated by zero.
	 */
	void unlockBuffer(size_t nSize);

private:
	bool init(const Char* pwsz,
			  size_t nLen);
	bool allocBuffer(size_t nLen);

private:
	XStringBuffer(const XStringBuffer&);
	XStringBuffer& operator=(const XStringBuffer&);

private:
	basic_xstring_ptr<XString> str_;
	size_t nLen_;
	Char* pEnd_;
};


/****************************************************************************
 *
 * XStringBufferLock
 *
 */

template<class XString>
class XStringBufferLock
{
public:
	XStringBufferLock(XStringBuffer<XString>* pBuf,
					  size_t nSize);
	~XStringBufferLock();

public:
	XStringBuffer<XString>::Char* get() const;

public:
	void unlock(size_t nSize);

private:
	XStringBuffer<XString>* pBuf_;
	XStringBuffer<XString>::Char* p_;
};


/****************************************************************************
 *
 * StringTokenizer
 *
 */

class QSEXPORTCLASS StringTokenizer
{
public:
	/**
	 * Create instance with the specified string and delimiter.
	 *
	 * @param pwsz [in] String.
	 * @param pwszDelimiter [in] Delimiter.
	 *
	 * @exception std::bad_alloc Out of memory.
	 */
	StringTokenizer(const WCHAR* pwsz,
					const WCHAR* pwszDelimiter);
	
	~StringTokenizer();

public:
	/**
	 * Get token count.
	 *
	 * @return Token count.
	 */
	unsigned int getCount() const;
	
	/**
	 * Get the token of the specified position.
	 *
	 * @param n [in] Position.
	 * @return Token.
	 */
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
	/**
	 * Create instance with the specified pattern.
	 *
	 * @param pszPattern [in] Pattern.
	 * @exception std::bad_alloc Out of memory.
	 */
	explicit BMFindString(const Char* pszPattern);
	
	/**
	 * Create instance with the specified pattern and pattern length.
	 *
	 * @param pszPattern [in] Pattern.
	 * @param nLen [in] Pattern length.
	 * @exception std::bad_alloc Out of memory.
	 */
	BMFindString(const Char* pszPattern,
				 size_t nLen);
	
	/**
	 * Create instance with the specified pattern and pattern length and flags.
	 *
	 * @param pszPattern [in] Pattern.
	 * @param nLen [in] Pattern length.
	 * @param nFlags [in] Flags.
	 * @exception std::bad_alloc Out of memory.
	 */
	BMFindString(const Char* pszPattern,
				 size_t nLen,
				 unsigned int nFlags);
	
	~BMFindString();

public:
	/**
	 * Find the pattern in the specified string.
	 *
	 * @param psz [in] String.
	 * @return Pointer to found string. null if not found.
	 */
	const Char* find(const Char* psz) const;
	
	/**
	 * Find the pattern in the specified string and length.
	 *
	 * @param psz [in] String.
	 * @param nLen [in] Length.
	 * @return Pointer to found string. null if not found.
	 */
	const Char* find(const Char* psz,
					 size_t nLen) const;

private:
	void init(const Char* pszPattern,
			  size_t nLen,
			  unsigned int nFlags);
	bool isEqual(Char lhs,
				 Char rhs) const;
	Char getChar(const Char* psz,
				 size_t nLen,
				 size_t n) const;

private:
	static void createSkipTable(const Char* pszPattern,
								size_t nLen,
								size_t* pSkip);
	static auto_ptr_array<size_t> createNextTable(const Char* pszPattern,
												  size_t nLen);

private:
	BMFindString(const BMFindString&);
	BMFindString& operator=(const BMFindString&);

private:
	size_t skip_[256];
	auto_ptr_array<size_t> pNext_;
	basic_string_ptr<String> strPattern_;
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
