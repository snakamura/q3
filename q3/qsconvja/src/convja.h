/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __CONVJA_H__
#define __CONVJA_H__

#include <qsconv.h>


namespace qsconvja {

/****************************************************************************
 *
 * ShiftJISConverter
 *
 */

class ShiftJISConverter : public qs::Converter
{
public:
	ShiftJISConverter();
	virtual ~ShiftJISConverter();

protected:
	virtual size_t encodeImpl(const WCHAR* pwsz,
							  size_t nLen,
							  qs::XStringBuffer<qs::XSTRING>* pBuf)
							  QNOTHROW();
	virtual size_t decodeImpl(const CHAR* psz,
							  size_t nLen,
							  qs::XStringBuffer<qs::WXSTRING>* pBuf)
							  QNOTHROW();

public:
	static bool isLeadByte(unsigned char c);

private:
	ShiftJISConverter(const ShiftJISConverter&);
	ShiftJISConverter& operator=(const ShiftJISConverter&);
};


/****************************************************************************
 *
 * ShiftJISConverterFactory
 *
 */

class ShiftJISConverterFactory : public qs::ConverterFactory
{
public:
	ShiftJISConverterFactory();
	virtual ~ShiftJISConverterFactory();

protected:
	virtual bool isSupported(const WCHAR* pwszName);
	virtual std::auto_ptr<qs::Converter> createInstance(const WCHAR* pwszName);

private:
	ShiftJISConverterFactory(const ShiftJISConverterFactory&);
	ShiftJISConverterFactory& operator=(const ShiftJISConverterFactory&);
};


/****************************************************************************
 *
 * ISO2022JPConverter
 *
 */

class ISO2022JPConverter : public qs::Converter
{
public:
	ISO2022JPConverter();
	virtual ~ISO2022JPConverter();

protected:
	virtual size_t encodeImpl(const WCHAR* pwsz,
							  size_t nLen,
							  qs::XStringBuffer<qs::XSTRING>* pBuf)
							  QNOTHROW();
	virtual size_t decodeImpl(const CHAR* psz,
							  size_t nLen,
							  qs::XStringBuffer<qs::WXSTRING>* pBuf)
							  QNOTHROW();

private:
	enum Mode {
		MODE_ASCII,
		MODE_KANJI,
		MODE_KANA
	};

private:
	ISO2022JPConverter(const ISO2022JPConverter&);
	ISO2022JPConverter& operator=(const ISO2022JPConverter&);

private:
	Mode mode_;
};


/****************************************************************************
 *
 * ISO2022JPConverterFactory
 *
 */

class ISO2022JPConverterFactory : public qs::ConverterFactory
{
public:
	ISO2022JPConverterFactory();
	virtual ~ISO2022JPConverterFactory();

protected:
	virtual bool isSupported(const WCHAR* pwszName);
	virtual std::auto_ptr<qs::Converter> createInstance(const WCHAR* pwszName);

private:
	ISO2022JPConverterFactory(const ISO2022JPConverterFactory&);
	ISO2022JPConverterFactory& operator=(const ISO2022JPConverterFactory&);
};


/****************************************************************************
 *
 * EUCJPConverter
 *
 */

class EUCJPConverter : public qs::Converter
{
public:
	EUCJPConverter();
	virtual ~EUCJPConverter();

protected:
	virtual size_t encodeImpl(const WCHAR* pwsz,
							  size_t nLen,
							  qs::XStringBuffer<qs::XSTRING>* pBuf)
							  QNOTHROW();
	virtual size_t decodeImpl(const CHAR* psz,
							  size_t nLen,
							  qs::XStringBuffer<qs::WXSTRING>* pBuf)
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

class EUCJPConverterFactory : public qs::ConverterFactory
{
public:
	EUCJPConverterFactory();
	virtual ~EUCJPConverterFactory();

protected:
	virtual bool isSupported(const WCHAR* pwszName);
	virtual std::auto_ptr<qs::Converter> createInstance(const WCHAR* pwszName);

private:
	EUCJPConverterFactory(const EUCJPConverterFactory&);
	EUCJPConverterFactory& operator=(const EUCJPConverterFactory&);
};


/****************************************************************************
 *
 * Util
 *
 */

class Util
{
public:
	static WORD sjis2jis(WORD sjis);
	static WORD jis2sjis(WORD jis);
	static WORD han2zen(unsigned char sjis,
						unsigned char sjisNext,
						bool* pbDakuten);
	static bool isHalfWidthKatakana(unsigned char sjis);
};

}

#endif // __CONVJA_H__
