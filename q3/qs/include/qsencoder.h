/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QSENCODER_H__
#define __QSENCODER_H__

#include <qs.h>
#include <qsstl.h>


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
	/**
	 * Encode the specified buffer.
	 *
	 * @param pSrc [in] Buffer.
	 * @param nSrcLen [in] Buffer length.
	 * @return Encoded buffer. null if failed.
	 */
	virtual malloc_size_ptr<unsigned char> encode(const unsigned char* pSrc,
												  size_t nSrcLen)
												  QNOTHROW() = 0;
	
	/**
	 * Decode the specified buffer.
	 *
	 * @param pSrc [in] Buffer.
	 * @param nSrcLen [in] Buffer length.
	 * @return Decoded buffer. null if failed.
	 */
	virtual malloc_size_ptr<unsigned char> decode(const unsigned char* pSrc,
												  size_t nSrcLen)
												  QNOTHROW() = 0;
};


/****************************************************************************
 *
 * EncoderFactory
 *
 */

class QSEXPORTCLASS EncoderFactory
{
protected:
	EncoderFactory();

public:
	virtual ~EncoderFactory();

public:
	/**
	 * Create instance of encoder.
	 *
	 * @param pwszName [in] Encoding name.
	 * @return Created encoder. null if encoder is not found or error occured.
	 * @exception std::bad_alloc Out of memory.
	 */
	static std::auto_ptr<Encoder> getInstance(const WCHAR* pwszName);

protected:
	/**
	 * Get encoding name.
	 *
	 * @return Encoding name.
	 */
	virtual const WCHAR* getName() const = 0;
	
	/**
	 * Create instance of encoder.
	 *
	 * @return Created encoder. null if encoder is not found or error occured.
	 * @exception std::bad_alloc Out of memory.
	 */
	virtual std::auto_ptr<Encoder> createInstance() = 0;

protected:
	/**
	 * Register encoder factory.
	 *
	 * @param pFactory [in] Factory.
	 * @exception std::bad_alloc Out of memory.
	 */
	static void registerFactory(EncoderFactory* pFactory);
	
	/**
	 * Unregister encoder factory.
	 *
	 * @param pFactory [in] Factory.
	 */
	static void unregisterFactory(EncoderFactory* pFactory);

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
	/**
	 * Create instance.
	 *
	 * @param bFold Specify make folding or not.
	 */
	explicit Base64Encoder(bool bFold);
	
	virtual ~Base64Encoder();

public:
	virtual malloc_size_ptr<unsigned char> encode(const unsigned char* pSrc,
												  size_t nSrcLen)
												  QNOTHROW();
	virtual malloc_size_ptr<unsigned char> decode(const unsigned char* pSrc,
												  size_t nSrcLen)
												  QNOTHROW();

public:
	/**
	 * Encode. Sufficient buffer must be allocated.
	 *
	 * @param pSrc [in] Buffer to be encoded.
	 * @param nSrcLen [in] Buffer length.
	 * @param bFold [in] Make folding or not.
	 * @param pDst [in] Buffer that decoded result to be written.
	 * @param pnDstLen [out] Writte size.
	 */
	static void encode(const unsigned char* pSrc,
					   size_t nSrcLen,
					   bool bFold,
					   unsigned char* pDst,
					   size_t* pnDstLen);

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
	Base64EncoderFactory();
	virtual ~Base64EncoderFactory();

protected:
	virtual const WCHAR* getName() const;
	virtual std::auto_ptr<Encoder> createInstance();

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
	BEncoderFactory();
	virtual ~BEncoderFactory();

protected:
	virtual const WCHAR* getName() const;
	virtual std::auto_ptr<Encoder> createInstance();

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
	explicit QuotedPrintableEncoder(bool bQ);
	virtual ~QuotedPrintableEncoder();

public:
	virtual malloc_size_ptr<unsigned char> encode(const unsigned char* pSrc,
												  size_t nSrcLen)
												  QNOTHROW();
	virtual malloc_size_ptr<unsigned char> decode(const unsigned char* pSrc,
												  size_t nSrcLen)
												  QNOTHROW();

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
	QuotedPrintableEncoderFactory();
	virtual ~QuotedPrintableEncoderFactory();

protected:
	virtual const WCHAR* getName() const;
	virtual std::auto_ptr<Encoder> createInstance();

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
	QEncoderFactory();
	virtual ~QEncoderFactory();

protected:
	virtual const WCHAR* getName() const;
	virtual std::auto_ptr<Encoder> createInstance();

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
	UuencodeEncoder();
	virtual ~UuencodeEncoder();

public:
	virtual malloc_size_ptr<unsigned char> encode(const unsigned char* pSrc,
												  size_t nSrcLen)
												  QNOTHROW();
	virtual malloc_size_ptr<unsigned char> decode(const unsigned char* pSrc,
												  size_t nSrcLen)
												  QNOTHROW();

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
	UuencodeEncoderFactory();
	virtual ~UuencodeEncoderFactory();

protected:
	virtual const WCHAR* getName() const;
	virtual std::auto_ptr<Encoder> createInstance();

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
	XUuencodeEncoderFactory();
	virtual ~XUuencodeEncoderFactory();

protected:
	virtual const WCHAR* getName() const;
	virtual std::auto_ptr<Encoder> createInstance();

private:
	XUuencodeEncoderFactory(const XUuencodeEncoderFactory&);
	XUuencodeEncoderFactory& operator=(const XUuencodeEncoderFactory&);
};

}

#endif // __QSENCODER_H__
