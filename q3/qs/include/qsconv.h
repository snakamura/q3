/*
 * $Id: qsconv.h,v 1.1.1.1 2003/04/29 08:07:34 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
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

QSEXPORTPROC STRING wcs2mbs(const WCHAR* pwszSrc);
QSEXPORTPROC STRING wcs2mbs(const WCHAR* pwszSrc, size_t nLen);
QSEXPORTPROC STRING wcs2mbs(const WCHAR* pwszSrc, size_t nLen, size_t* pnLen);
QSEXPORTPROC TSTRING wcs2tcs(const WCHAR* pwszSrc);
QSEXPORTPROC TSTRING wcs2tcs(const WCHAR* pwszSrc, size_t nLen);
QSEXPORTPROC TSTRING wcs2tcs(const WCHAR* pwszSrc, size_t nLen, size_t* pnLen);
QSEXPORTPROC WSTRING mbs2wcs(const CHAR* pszSrc);
QSEXPORTPROC WSTRING mbs2wcs(const CHAR* pszSrc, size_t nLen);
QSEXPORTPROC WSTRING mbs2wcs(const CHAR* pszSrc, size_t nLen, size_t* pnLen);
QSEXPORTPROC TSTRING mbs2tcs(const CHAR* pszSrc);
QSEXPORTPROC TSTRING mbs2tcs(const CHAR* pszSrc, size_t nLen);
QSEXPORTPROC TSTRING mbs2tcs(const CHAR* pszSrc, size_t nLen, size_t* pnLen);
QSEXPORTPROC WSTRING tcs2wcs(const TCHAR* ptszSrc);
QSEXPORTPROC WSTRING tcs2wcs(const TCHAR* ptszSrc, size_t nLen);
QSEXPORTPROC WSTRING tcs2wcs(const TCHAR* ptszSrc, size_t nLen, size_t* pnLen);
QSEXPORTPROC STRING tcs2mbs(const TCHAR* ptszSrc);
QSEXPORTPROC STRING tcs2mbs(const TCHAR* ptszSrc, size_t nLen);
QSEXPORTPROC STRING tcs2mbs(const TCHAR* ptszSrc, size_t nLen, size_t* pnLen);

#ifdef UNICODE
#define W2T(w, t) const TCHAR* t = w
#else
#define W2T(w, t) \
	string_ptr<TSTRING> tstr##t; \
	if (w) { \
		tstr##t.reset(wcs2tcs(w)); \
		if (!tstr##t.get()) \
			return QSTATUS_OUTOFMEMORY; \
	} \
	const TCHAR* t = tstr##t.get()
#endif

#ifdef UNICODE
#define W2T_STATUS(w, t) const TCHAR* t = w
#else
#define W2T_STATUS(w, t) \
	string_ptr<TSTRING> tstr##t; \
	if (w) { \
		tstr##t.reset(wcs2tcs(w)); \
		if (!tstr##t.get()) \
			status = QSTATUS_OUTOFMEMORY; \
	} \
	const TCHAR* t = tstr##t.get()
#endif

#ifdef UNICODE
#define T2W(t, w) const WCHAR* w = t
#else
#define T2W(t, w) \
	string_ptr<WSTRING> wstr##w; \
	if (t) { \
		wstr##w.reset(tcs2wcs(t)); \
		if (!wstr##w.get()) \
			return QSTATUS_OUTOFMEMORY; \
	} \
	const WCHAR* w = wstr##w.get()
#endif

#ifdef UNICODE
#define T2W_STATUS(t, w) const WCHAR* w = t
#else
#define T2W_STATUS(t, w) \
	string_ptr<WSTRING> wstr##w; \
	if (t) { \
		wstr##w.reset(tcs2wcs(t)); \
		if (!wstr##w.get()) \
			status = QSTATUS_OUTOFMEMORY; \
	} \
	const WCHAR* w = wstr##w.get()
#endif

#ifdef UNICODE
#define A2T(a, t) \
	string_ptr<TSTRING> tstr##t; \
	if (a) { \
		tstr##t.reset(mbs2tcs(a)); \
		if (!tstr##t.get()) \
			return QSTATUS_OUTOFMEMORY; \
	} \
	const TCHAR* t = tstr##t.get()
#else
#define A2T(a, t) const TCHAR* t = a
#endif

#ifdef UNICODE
#define A2T_STATUS(a, t) \
	string_ptr<TSTRING> tstr##t; \
	if (a) { \
		tstr##t.reset(mbs2tcs(a)); \
		if (!tstr##t.get()) \
			status = QSTATUS_OUTOFMEMORY; \
	} \
	const TCHAR* t = tstr##t.get()
#else
#define A2T_STATUS(a, t) const TCHAR* t = a
#endif

#ifdef UNICODE
#define T2A(t, a) \
	string_ptr<STRING> str##a; \
	if (t) { \
		str##a.reset(tcs2mbs(t)); \
		if (!str##a.get()) \
			return QSTATUS_OUTOFMEMORY; \
	} \
	const CHAR* a = str##a.get()
#else
#define T2A(t, a) const CHAR* a = t
#endif

#ifdef UNICODE
#define T2A_STATUS(t, a) \
	string_ptr<STRING> str##a; \
	if (t) { \
		str##a.reset(tcs2mbs(t)); \
		if (!str##a.get()) \
			status = QSTATUS_OUTOFMEMORY; \
	} \
	const CHAR* a = str##a.get()
#else
#define T2A_STATUS(t, a) const CHAR* a = t
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
	virtual QSTATUS encode(const WCHAR* pwsz, size_t* pnLen,
		STRING* pstr, size_t* pnResultLen) = 0;
	virtual QSTATUS decode(const CHAR* psz, size_t* pnLen,
		WSTRING* pwstr, size_t* pnResultLen) = 0;
};


/****************************************************************************
 *
 * ConverterFactory
 *
 */

class QSEXPORTCLASS ConverterFactory
{
protected:
	explicit ConverterFactory(QSTATUS* pstatus);

public:
	virtual ~ConverterFactory();

public:
	static QSTATUS getInstance(const WCHAR* pwszName, Converter** ppConverter);
	static QSTATUS getInstance(const WCHAR* pwszName,
		std::auto_ptr<Converter>* papConverter);

protected:
	virtual QSTATUS isSupported(const WCHAR* pwszName, bool* pbSupported) = 0;
	virtual QSTATUS createInstance(const WCHAR* pwszName,
		Converter** ppConverter) = 0;

protected:
	static QSTATUS regist(ConverterFactory* pFactory);
	static QSTATUS unregist(ConverterFactory* pFactory);

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
	UTF7Converter(bool bModified, QSTATUS* pstatus);
	virtual ~UTF7Converter();

public:
	virtual QSTATUS encode(const WCHAR* pwsz, size_t* pnLen,
		STRING* pstr, size_t* pnResultLen);
	virtual QSTATUS decode(const CHAR* psz, size_t* pnLen,
		WSTRING* pwstr, size_t* pnResultLen);

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
	UTF7ConverterFactory(QSTATUS* pstatus);
	virtual ~UTF7ConverterFactory();

protected:
	virtual QSTATUS isSupported(const WCHAR* pwszName, bool* pbSupported);
	virtual QSTATUS createInstance(const WCHAR* pwszName,
		Converter** ppConverter);

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
	UTF8Converter(QSTATUS* pstatus);
	virtual ~UTF8Converter();

public:
	virtual QSTATUS encode(const WCHAR* pwsz, size_t* pnLen,
		STRING* pstr, size_t* pnResultLen);
	virtual QSTATUS decode(const CHAR* psz, size_t* pnLen,
		WSTRING* pwstr, size_t* pnResultLen);

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
	UTF8ConverterFactory(QSTATUS* pstatus);
	virtual ~UTF8ConverterFactory();

protected:
	virtual QSTATUS isSupported(const WCHAR* pwszName, bool* pbSupported);
	virtual QSTATUS createInstance(const WCHAR* pwszName,
		Converter** ppConverter);

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
	ShiftJISConverter(QSTATUS* pstatus);
	virtual ~ShiftJISConverter();

public:
	virtual QSTATUS encode(const WCHAR* pwsz, size_t* pnLen,
		STRING* pstr, size_t* pnResultLen);
	virtual QSTATUS decode(const CHAR* psz, size_t* pnLen,
		WSTRING* pwstr, size_t* pnResultLen);

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
	ShiftJISConverterFactory(QSTATUS* pstatus);
	virtual ~ShiftJISConverterFactory();

protected:
	virtual QSTATUS isSupported(const WCHAR* pwszName, bool* pbSupported);
	virtual QSTATUS createInstance(const WCHAR* pwszName,
		Converter** ppConverter);

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
	ISO2022JPConverter(QSTATUS* pstatus);
	virtual ~ISO2022JPConverter();

public:
	virtual QSTATUS encode(const WCHAR* pwsz, size_t* pnLen,
		STRING* pstr, size_t* pnResultLen);
	virtual QSTATUS decode(const CHAR* psz, size_t* pnLen,
		WSTRING* pwstr, size_t* pnResultLen);

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
	ISO2022JPConverterFactory(QSTATUS* pstatus);
	virtual ~ISO2022JPConverterFactory();

protected:
	virtual QSTATUS isSupported(const WCHAR* pwszName, bool* pbSupported);
	virtual QSTATUS createInstance(const WCHAR* pwszName,
		Converter** ppConverter);

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
	EUCJPConverter(QSTATUS* pstatus);
	virtual ~EUCJPConverter();

public:
	virtual QSTATUS encode(const WCHAR* pwsz, size_t* pnLen,
		STRING* pstr, size_t* pnResultLen);
	virtual QSTATUS decode(const CHAR* psz, size_t* pnLen,
		WSTRING* pwstr, size_t* pnResultLen);

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
	EUCJPConverterFactory(QSTATUS* pstatus);
	virtual ~EUCJPConverterFactory();

protected:
	virtual QSTATUS isSupported(const WCHAR* pwszName, bool* pbSupported);
	virtual QSTATUS createInstance(const WCHAR* pwszName,
		Converter** ppConverter);

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
		DWORD dwEncoding, QSTATUS* pstatus);
	virtual ~MLangConverter();

public:
	virtual QSTATUS encode(const WCHAR* pwsz, size_t* pnLen,
		STRING* pstr, size_t* pnResultLen);
	virtual QSTATUS decode(const CHAR* psz, size_t* pnLen,
		WSTRING* pwstr, size_t* pnResultLen);

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
	MLangConverterFactory(QSTATUS* pstatus);
	virtual ~MLangConverterFactory();

protected:
	virtual QSTATUS isSupported(const WCHAR* pwszName, bool* pbSupported);
	virtual QSTATUS createInstance(const WCHAR* pwszName,
		Converter** ppConverter);

private:
	MLangConverterFactory(const MLangConverterFactory&);
	MLangConverterFactory& operator=(const MLangConverterFactory&);

private:
	struct MLangConverterFactoryImpl* pImpl_;
};

}

#endif // __QSCONV_H__
