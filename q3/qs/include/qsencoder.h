/*
 * $Id: qsencoder.h,v 1.1.1.1 2003/04/29 08:07:34 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QSENCODER_H__
#define __QSENCODER_H__

#include <qs.h>


namespace qs {

/****************************************************************************
 *
 * Encoder
 *
 */

class QSEXPORTCLASS Encoder
{
public:
	virtual ~Encoder();

public:
	virtual QSTATUS encode(const unsigned char* pSrc, size_t nSrcLen,
		unsigned char** ppDst, size_t* pnDstLen) = 0;
	virtual QSTATUS decode(const unsigned char* pSrc, size_t nSrcLen,
		unsigned char** ppDst, size_t* pnDstLen) = 0;
};


/****************************************************************************
 *
 * EncoderFactory
 *
 */

class QSEXPORTCLASS EncoderFactory
{
protected:
	explicit EncoderFactory(QSTATUS* pstatus);

public:
	virtual ~EncoderFactory();

public:
	static QSTATUS getInstance(const WCHAR* pwszName, Encoder** ppEncoder);
	static QSTATUS getInstance(const WCHAR* pwszName,
		std::auto_ptr<Encoder>* papEncoder);

protected:
	virtual const WCHAR* getName() const = 0;
	virtual QSTATUS createInstance(Encoder** ppEncoder) = 0;

protected:
	static QSTATUS regist(EncoderFactory* pFactory);
	static QSTATUS unregist(EncoderFactory* pFactory);

private:
	EncoderFactory(const EncoderFactory&);
	EncoderFactory& operator=(const EncoderFactory&);
};


/****************************************************************************
 *
 * Base64Encoder
 *
 */

class QSEXPORTCLASS Base64Encoder : public Encoder
{
public:
	enum {
		FOLD_LENGTH = 72
	};

public:
	Base64Encoder(bool bFold, QSTATUS* pstatus);
	virtual ~Base64Encoder();

public:
	virtual QSTATUS encode(const unsigned char* pSrc, size_t nSrcLen,
		unsigned char** ppDst, size_t* pnDstLen);
	virtual QSTATUS decode(const unsigned char* pSrc, size_t nSrcLen,
		unsigned char** ppDst, size_t* pnDstLen);

public:
	static void encode(const unsigned char* pSrc, size_t nSrcLen,
		bool bFold, unsigned char* pDst, size_t* pDstLen);

private:
	Base64Encoder(const Base64Encoder&);
	Base64Encoder& operator=(const Base64Encoder&);

private:
	bool bFold_;
};


/****************************************************************************
 *
 * Base64EncoderFactory
 *
 */

class Base64EncoderFactory : public EncoderFactory
{
public:
	Base64EncoderFactory(QSTATUS* pstatus);
	virtual ~Base64EncoderFactory();

protected:
	virtual const WCHAR* getName() const;
	virtual QSTATUS createInstance(Encoder** ppEncoder);

private:
	Base64EncoderFactory(const Base64EncoderFactory&);
	Base64EncoderFactory& operator=(const Base64EncoderFactory&);
};


/****************************************************************************
 *
 * BEncoderFactory
 *
 */

class BEncoderFactory : public EncoderFactory
{
public:
	BEncoderFactory(QSTATUS* pstatus);
	virtual ~BEncoderFactory();

protected:
	virtual const WCHAR* getName() const;
	virtual QSTATUS createInstance(Encoder** ppEncoder);

private:
	BEncoderFactory(const BEncoderFactory&);
	BEncoderFactory& operator=(const BEncoderFactory&);
};


/****************************************************************************
 *
 * QuotedPrintableEncoder
 *
 */

class QSEXPORTCLASS QuotedPrintableEncoder : public Encoder
{
public:
	QuotedPrintableEncoder(bool bQ, QSTATUS* pstatus);
	virtual ~QuotedPrintableEncoder();

public:
	virtual QSTATUS encode(const unsigned char* pSrc, size_t nSrcLen,
		unsigned char** ppDst, size_t* pnDstLen);
	virtual QSTATUS decode(const unsigned char* pSrc, size_t nSrcLen,
		unsigned char** ppDst, size_t* pnDstLen);

private:
	QuotedPrintableEncoder(const QuotedPrintableEncoder&);
	QuotedPrintableEncoder& operator=(const QuotedPrintableEncoder&);

private:
	bool bQ_;
};


/****************************************************************************
 *
 * QuotedPrintableEncoderFactory
 *
 */

class QuotedPrintableEncoderFactory : public EncoderFactory
{
public:
	QuotedPrintableEncoderFactory(QSTATUS* pstatus);
	virtual ~QuotedPrintableEncoderFactory();

protected:
	virtual const WCHAR* getName() const;
	virtual QSTATUS createInstance(Encoder** ppEncoder);

private:
	QuotedPrintableEncoderFactory(const QuotedPrintableEncoderFactory&);
	QuotedPrintableEncoderFactory& operator=(const QuotedPrintableEncoderFactory&);
};


/****************************************************************************
 *
 * QEncoderFactory
 *
 */

class QEncoderFactory : public EncoderFactory
{
public:
	QEncoderFactory(QSTATUS* pstatus);
	virtual ~QEncoderFactory();

protected:
	virtual const WCHAR* getName() const;
	virtual QSTATUS createInstance(Encoder** ppEncoder);

private:
	QEncoderFactory(const QEncoderFactory&);
	QEncoderFactory& operator=(const QEncoderFactory&);
};


/****************************************************************************
 *
 * UuencodeEncoder
 *
 */

class QSEXPORTCLASS UuencodeEncoder : public Encoder
{
public:
	explicit UuencodeEncoder(QSTATUS* pstatus);
	virtual ~UuencodeEncoder();

public:
	virtual QSTATUS encode(const unsigned char* pSrc, size_t nSrcLen,
		unsigned char** ppDst, size_t* pnDstLen);
	virtual QSTATUS decode(const unsigned char* pSrc, size_t nSrcLen,
		unsigned char** ppDst, size_t* pnDstLen);

private:
	UuencodeEncoder(const UuencodeEncoder&);
	UuencodeEncoder& operator=(const UuencodeEncoder&);
};



/****************************************************************************
 *
 * UuencodeEncoderFactory
 *
 */

class UuencodeEncoderFactory : public EncoderFactory
{
public:
	UuencodeEncoderFactory(QSTATUS* pstatus);
	virtual ~UuencodeEncoderFactory();

protected:
	virtual const WCHAR* getName() const;
	virtual QSTATUS createInstance(Encoder** ppEncoder);

private:
	UuencodeEncoderFactory(const UuencodeEncoderFactory&);
	UuencodeEncoderFactory& operator=(const UuencodeEncoderFactory&);
};


/****************************************************************************
 *
 * XUuencodeEncoderFactory
 *
 */

class XUuencodeEncoderFactory : public EncoderFactory
{
public:
	XUuencodeEncoderFactory(QSTATUS* pstatus);
	virtual ~XUuencodeEncoderFactory();

protected:
	virtual const WCHAR* getName() const;
	virtual QSTATUS createInstance(Encoder** ppEncoder);

private:
	XUuencodeEncoderFactory(const XUuencodeEncoderFactory&);
	XUuencodeEncoderFactory& operator=(const XUuencodeEncoderFactory&);
};

}

#endif // __QSENCODER_H__
