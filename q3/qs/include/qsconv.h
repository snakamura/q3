/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QSCONV_H__
#define __QSCONV_H__

#include <qs.h>
#include <qsstring.h>

#include <mlang.h>


namespace qs {

/****************************************************************************
 *
 * Simple Conversion
 *
 */

/**
 * Convert unicode string to native string.
 *
 * @param pwszSrc [in] Unicode string.
 * @return Native string. Can not be null.
 * @exception std::bad_alloc Out of memory.
 */
QSEXPORTPROC string_ptr wcs2mbs(const WCHAR* pwszSrc);

/**
 * Convert unicode string to native string.
 *
 * @param pwszSrc [in] Unicode string.
 * @param nLen [in] Unicode string length.
 * @return Native string. Can not be null.
 * @exception std::bad_alloc Out of memory.
 */
QSEXPORTPROC string_ptr wcs2mbs(const WCHAR* pwszSrc,
								size_t nLen);

/**
 * Convert unicode string to native string.
 *
 * @param pwszSrc [in] Unicode string.
 * @param nLen [in] Unicode string length.
 * @param pnLen [out] Converted native string length.
 * @return Native string. Can not be null.
 * @exception std::bad_alloc Out of memory.
 */
QSEXPORTPROC string_ptr wcs2mbs(const WCHAR* pwszSrc,
								size_t nLen,
								size_t* pnLen);

/**
 * Convert unicode string to program string.
 *
 * @param pwszSrc [in] Unicode string.
 * @return Program string. Can not be null.
 * @exception std::bad_alloc Out of memory.
 */
QSEXPORTPROC tstring_ptr wcs2tcs(const WCHAR* pwszSrc);

/**
 * Convert unicode string to program string.
 *
 * @param pwszSrc [in] Unicode string.
 * @param nLen [in] Unicode string length.
 * @return Program string. Can not be null.
 * @exception std::bad_alloc Out of memory.
 */
QSEXPORTPROC tstring_ptr wcs2tcs(const WCHAR* pwszSrc,
								 size_t nLen);

/**
 * Convert unicode string to program string.
 *
 * @param pwszSrc [in] Unicode string.
 * @param nLen [in] Unicode string length.
 * @param pnLen [out] Converted program string length.
 * @return Program string. Can not be null.
 * @exception std::bad_alloc Out of memory.
 */
QSEXPORTPROC tstring_ptr wcs2tcs(const WCHAR* pwszSrc,
								 size_t nLen,
								 size_t* pnLen);

/**
 * Convert native string to unicode string.
 *
 * @param pszSrc [in] Navite string.
 * @return Unicode string. Can not be null.
 * @exception std::bad_alloc Out of memory.
 */
QSEXPORTPROC wstring_ptr mbs2wcs(const CHAR* pszSrc);

/**
 * Convert native string to unicode string.
 *
 * @param pszSrc [in] Navite string.
 * @param nLen [in] Native string length.
 * @return Unicode string. Can not be null.
 * @exception std::bad_alloc Out of memory.
 */
QSEXPORTPROC wstring_ptr mbs2wcs(const CHAR* pszSrc,
								 size_t nLen);

/**
 * Convert native string to unicode string.
 *
 * @param pszSrc [in] Navite string.
 * @param nLen [in] Native string length.
 * @param pnLen [out] Converted unicode string length.
 * @return Unicode string. Can not be null.
 * @exception std::bad_alloc Out of memory.
 */
QSEXPORTPROC wstring_ptr mbs2wcs(const CHAR* pszSrc,
								 size_t nLen,
								 size_t* pnLen);

/**
 * Convert native string to program string.
 *
 * @param pszSrc [in] Native string.
 * @return Program string. Can not be null.
 * @exception std::bad_alloc Out of memory.
 */
QSEXPORTPROC tstring_ptr mbs2tcs(const CHAR* pszSrc);

/**
 * Convert native string to program string.
 *
 * @param pszSrc [in] Native string.
 * @param nLen [in] Native string length.
 * @return Program string. Can not be null.
 * @exception std::bad_alloc Out of memory.
 */
QSEXPORTPROC tstring_ptr mbs2tcs(const CHAR* pszSrc,
								 size_t nLen);

/**
 * Convert native string to program string.
 *
 * @param pszSrc [in] Native string.
 * @param nLen [in] Native string length.
 * @param pnLen [out] Converted program string length.
 * @return Program string. Can not be null.
 * @exception std::bad_alloc Out of memory.
 */
QSEXPORTPROC tstring_ptr mbs2tcs(const CHAR* pszSrc,
								 size_t nLen,
								 size_t* pnLen);

/**
 * Convert program string to unicode string.
 *
 * @param ptszSrc [in] Program string.
 * @return Unicode string. Can not be null.
 * @exception std::bad_alloc Out of memory.
 */
QSEXPORTPROC wstring_ptr tcs2wcs(const TCHAR* ptszSrc);

/**
 * Convert program string to unicode string.
 *
 * @param ptszSrc [in] Program string.
 * @param nLen [in] Program string length.
 * @return Unicode string. Can not be null.
 * @exception std::bad_alloc Out of memory.
 */
QSEXPORTPROC wstring_ptr tcs2wcs(const TCHAR* ptszSrc,
								 size_t nLen);

/**
 * Convert program string to unicode string.
 *
 * @param ptszSrc [in] Program string.
 * @param nLen [in] Program string length.
 * @param pnLen [out] Converted unicode string length.
 * @return Unicode string. Can not be null.
 * @exception std::bad_alloc Out of memory.
 */
QSEXPORTPROC wstring_ptr tcs2wcs(const TCHAR* ptszSrc,
								 size_t nLen,
								 size_t* pnLen);

/**
 * Convert program string to native string.
 *
 * @param ptszSrc [in] Program string.
 * @return Native string. Can not be null.
 * @exception std::bad_alloc Out of memory.
 */
QSEXPORTPROC string_ptr tcs2mbs(const TCHAR* ptszSrc);

/**
 * Convert program string to native string.
 *
 * @param ptszSrc [in] Program string.
 * @param nLen [in] Program string length.
 * @return Native string. Can not be null.
 * @exception std::bad_alloc Out of memory.
 */
QSEXPORTPROC string_ptr tcs2mbs(const TCHAR* ptszSrc,
								size_t nLen);

/**
 * Convert program string to native string.
 *
 * @param ptszSrc [in] Program string.
 * @param nLen [in] Program string length.
 * @param pnLen [out] Converted native string length.
 * @return Native string. Can not be null.
 * @exception std::bad_alloc Out of memory.
 */
QSEXPORTPROC string_ptr tcs2mbs(const TCHAR* ptszSrc,
								size_t nLen,
								size_t* pnLen);

#ifdef UNICODE
#define W2T(w, t) const TCHAR* t = w
#else
#define W2T(w, t) \
	tstring_ptr tstr##t; \
	if (w) \
		tstr##t = wcs2tcs(w); \
	const TCHAR* t = tstr##t.get()
#endif

#ifdef UNICODE
#define T2W(t, w) const WCHAR* w = t
#else
#define T2W(t, w) \
	wstring_ptr wstr##w; \
	if (t) \
		wstr##w = tcs2wcs(t); \
	const WCHAR* w = wstr##w.get()
#endif

#ifdef UNICODE
#define A2T(a, t) \
	tstring_ptr tstr##t; \
	if (a) \
		tstr##t = mbs2tcs(a); \
	const TCHAR* t = tstr##t.get()
#else
#define A2T(a, t) const TCHAR* t = a
#endif

#ifdef UNICODE
#define T2A(t, a) \
	string_ptr str##a; \
	if (t) \
		str##a = tcs2mbs(t); \
	const CHAR* a = str##a.get()
#else
#define T2A(t, a) const CHAR* a = t
#endif


/****************************************************************************
 *
 * Converter
 *
 */

class QSEXPORTCLASS Converter
{
public:
	virtual ~Converter();

public:
	xstring_size_ptr encode(const WCHAR* pwsz,
							size_t* pnLen)
							QNOTHROW();
	wxstring_size_ptr decode(const CHAR* psz,
							 size_t* pnLen)
							 QNOTHROW();
	size_t encode(const WCHAR* pwsz,
				  size_t nLen,
				  XStringBuffer<XSTRING>* pBuf)
				  QNOTHROW();
	size_t decode(const CHAR* psz,
				  size_t nLen,
				  XStringBuffer<WXSTRING>* pBuf)
				  QNOTHROW();

protected:
	virtual size_t encodeImpl(const WCHAR* pwsz,
							  size_t nLen,
							  XStringBuffer<XSTRING>* pBuf)
							  QNOTHROW() = 0;
	
	virtual size_t decodeImpl(const CHAR* psz,
							  size_t nLen,
							  XStringBuffer<WXSTRING>* pBuf)
							  QNOTHROW() = 0;
};


/****************************************************************************
 *
 * ConverterFactory
 *
 */

class QSEXPORTCLASS ConverterFactory
{
protected:
	ConverterFactory();

public:
	virtual ~ConverterFactory();

public:
	/**
	 * Create instance of converter.
	 *
	 * @param pwszName [in] Encoding name.
	 * @return Created converter. null if converter is not found or error occured.
	 * @exception std::bad_alloc Out of memory.
	 */
	static std::auto_ptr<Converter> getInstance(const WCHAR* pwszName);

protected:
	/**
	 * Check if the specified encoding is supported or not.
	 *
	 * @param pwszName [in] Encoding name.
	 * @return ture if supported, false otherwise.
	 */
	virtual bool isSupported(const WCHAR* pwszName) = 0;
	
	/**
	 * Create instance of converter.
	 *
	 * @param pwszName [in] Encoding name.
	 * @return Created converter. null if converter is not found or error occured.
	 * @exception std::bad_alloc Out of memory.
	 */
	virtual std::auto_ptr<Converter> createInstance(const WCHAR* pwszName) = 0;

protected:
	/**
	 * Register converter factory.
	 *
	 * @param pFactory [in] Factory.
	 * @exception std::bad_alloc Out of memory.
	 */
	static void registerFactory(ConverterFactory* pFactory);
	
	/**
	 * Unregister converter factory.
	 *
	 * @param pFactory [in] Factory.
	 */
	static void unregisterFactory(ConverterFactory* pFactory);

private:
	ConverterFactory(const ConverterFactory&);
	ConverterFactory& operator=(const ConverterFactory&);
};


/****************************************************************************
 *
 * UTF7Converter
 *
 */

class QSEXPORTCLASS UTF7Converter : public Converter
{
public:
	explicit UTF7Converter(bool bModified);
	virtual ~UTF7Converter();

protected:
	virtual size_t encodeImpl(const WCHAR* pwsz,
							  size_t nLen,
							  XStringBuffer<XSTRING>* pBuf)
							  QNOTHROW();
	virtual size_t decodeImpl(const CHAR* psz,
							  size_t nLen,
							  XStringBuffer<WXSTRING>* pBuf)
							  QNOTHROW();

private:
	UTF7Converter(const UTF7Converter&);
	UTF7Converter& operator=(const UTF7Converter&);

private:
	struct UTF7ConverterImpl* pImpl_;
};


/****************************************************************************
 *
 * UTF7ConverterFactory
 *
 */

class UTF7ConverterFactory : public ConverterFactory
{
public:
	UTF7ConverterFactory();
	virtual ~UTF7ConverterFactory();

protected:
	virtual bool isSupported(const WCHAR* pwszName);
	virtual std::auto_ptr<Converter> createInstance(const WCHAR* pwszName);

private:
	UTF7ConverterFactory(const UTF7ConverterFactory&);
	UTF7ConverterFactory& operator=(const UTF7ConverterFactory&);
};


/****************************************************************************
 *
 * UTF8Converter
 *
 */

class QSEXPORTCLASS UTF8Converter : public Converter
{
public:
	UTF8Converter();
	virtual ~UTF8Converter();

protected:
	virtual size_t encodeImpl(const WCHAR* pwsz,
							  size_t nLen,
							  XStringBuffer<XSTRING>* pBuf)
							  QNOTHROW();
	virtual size_t decodeImpl(const CHAR* psz,
							  size_t nLen,
							  XStringBuffer<WXSTRING>* pBuf)
							  QNOTHROW();

private:
	UTF8Converter(const UTF8Converter&);
	UTF8Converter& operator=(const UTF8Converter&);
};


/****************************************************************************
 *
 * UTF8ConverterFactory
 *
 */

class UTF8ConverterFactory : public ConverterFactory
{
public:
	UTF8ConverterFactory();
	virtual ~UTF8ConverterFactory();

protected:
	virtual bool isSupported(const WCHAR* pwszName);
	virtual std::auto_ptr<Converter> createInstance(const WCHAR* pwszName);

private:
	UTF8ConverterFactory(const UTF8ConverterFactory&);
	UTF8ConverterFactory& operator=(const UTF8ConverterFactory&);
};


#ifdef QS_KCONVERT

/****************************************************************************
 *
 * ShiftJISConverter
 *
 */

class QSEXPORTCLASS ShiftJISConverter : public Converter
{
public:
	ShiftJISConverter();
	virtual ~ShiftJISConverter();

protected:
	virtual size_t encodeImpl(const WCHAR* pwsz,
							  size_t nLen,
							  XStringBuffer<XSTRING>* pBuf)
							  QNOTHROW();
	virtual size_t decodeImpl(const CHAR* psz,
							  size_t nLen,
							  XStringBuffer<WXSTRING>* pBuf)
							  QNOTHROW();

private:
	ShiftJISConverter(const ShiftJISConverter&);
	ShiftJISConverter& operator=(const ShiftJISConverter&);
};


/****************************************************************************
 *
 * ShiftJISConverterFactory
 *
 */

class ShiftJISConverterFactory : public ConverterFactory
{
public:
	ShiftJISConverterFactory();
	virtual ~ShiftJISConverterFactory();

protected:
	virtual bool isSupported(const WCHAR* pwszName);
	virtual std::auto_ptr<Converter> createInstance(const WCHAR* pwszName);

private:
	ShiftJISConverterFactory(const ShiftJISConverterFactory&);
	ShiftJISConverterFactory& operator=(const ShiftJISConverterFactory&);
};


/****************************************************************************
 *
 * ISO2022JPConverter
 *
 */

class QSEXPORTCLASS ISO2022JPConverter : public Converter
{
public:
	ISO2022JPConverter();
	virtual ~ISO2022JPConverter();

protected:
	virtual size_t encodeImpl(const WCHAR* pwsz,
							  size_t nLen,
							  XStringBuffer<XSTRING>* pBuf)
							  QNOTHROW();
	virtual size_t decodeImpl(const CHAR* psz,
							  size_t nLen,
							  XStringBuffer<WXSTRING>* pBuf)
							  QNOTHROW();

private:
	ISO2022JPConverter(const ISO2022JPConverter&);
	ISO2022JPConverter& operator=(const ISO2022JPConverter&);

private:
	struct ISO2022JPConverterImpl* pImpl_;
};


/****************************************************************************
 *
 * ISO2022JPConverterFactory
 *
 */

class ISO2022JPConverterFactory : public ConverterFactory
{
public:
	ISO2022JPConverterFactory();
	virtual ~ISO2022JPConverterFactory();

protected:
	virtual bool isSupported(const WCHAR* pwszName);
	virtual std::auto_ptr<Converter> createInstance(const WCHAR* pwszName);

private:
	ISO2022JPConverterFactory(const ISO2022JPConverterFactory&);
	ISO2022JPConverterFactory& operator=(const ISO2022JPConverterFactory&);
};


/****************************************************************************
 *
 * EUCJPConverter
 *
 */

class QSEXPORTCLASS EUCJPConverter : public Converter
{
public:
	EUCJPConverter();
	virtual ~EUCJPConverter();

protected:
	virtual size_t encodeImpl(const WCHAR* pwsz,
							  size_t nLen,
							  XStringBuffer<XSTRING>* pBuf)
							  QNOTHROW();
	virtual size_t decodeImpl(const CHAR* psz,
							  size_t nLen,
							  XStringBuffer<WXSTRING>* pBuf)
							  QNOTHROW();

private:
	EUCJPConverter(const EUCJPConverter&);
	EUCJPConverter& operator=(const EUCJPConverter&);
};


/****************************************************************************
 *
 * EUCJPConverterFactory
 *
 */

class EUCJPConverterFactory : public ConverterFactory
{
public:
	EUCJPConverterFactory();
	virtual ~EUCJPConverterFactory();

protected:
	virtual bool isSupported(const WCHAR* pwszName);
	virtual std::auto_ptr<Converter> createInstance(const WCHAR* pwszName);

private:
	EUCJPConverterFactory(const EUCJPConverterFactory&);
	EUCJPConverterFactory& operator=(const EUCJPConverterFactory&);
};

#endif // QS_KCONVERT


/****************************************************************************
 *
 * MLangConverter
 *
 */

class QSEXPORTCLASS MLangConverter : public Converter
{
public:
	MLangConverter(IMultiLanguage* pMultiLanguage,
				   DWORD dwEncoding);
	virtual ~MLangConverter();

protected:
	virtual size_t encodeImpl(const WCHAR* pwsz,
							  size_t nLen,
							  XStringBuffer<XSTRING>* pBuf)
							  QNOTHROW();
	virtual size_t decodeImpl(const CHAR* psz,
							  size_t nLen,
							  XStringBuffer<WXSTRING>* pBuf)
							  QNOTHROW();

private:
	MLangConverter(const MLangConverter&);
	MLangConverter& operator=(const MLangConverter&);

private:
	struct MLangConverterImpl* pImpl_;
};


/****************************************************************************
 *
 * MLangConverterFactory
 *
 */

class MLangConverterFactory : public ConverterFactory
{
public:
	MLangConverterFactory();
	virtual ~MLangConverterFactory();

protected:
	virtual bool isSupported(const WCHAR* pwszName);
	virtual std::auto_ptr<Converter> createInstance(const WCHAR* pwszName);

private:
	MLangConverterFactory(const MLangConverterFactory&);
	MLangConverterFactory& operator=(const MLangConverterFactory&);

private:
	struct MLangConverterFactoryImpl* pImpl_;
};

}

#endif // __QSCONV_H__
