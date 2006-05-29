/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __UTIL_H__
#define __UTIL_H__

#include <qslog.h>
#include <qsstl.h>
#include <qsstream.h>

#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/pkcs7.h>
#include <openssl/x509.h>


namespace qscrypto {

/****************************************************************************
 *
 * BIOPtr
 *
 */

class BIOPtr
{
public:
	BIOPtr(BIO* p) : p_(p) {}
	~BIOPtr() { if (p_) BIO_free(p_); }

public:
	BIO* get() const { return p_; }

private:
	BIOPtr(BIOPtr&);
	BIOPtr& operator=(BIOPtr&);

private:
	BIO* p_;
};


/****************************************************************************
 *
 * PKCS7Ptr
 *
 */

class PKCS7Ptr
{
public:
	PKCS7Ptr(PKCS7* p) : p_(p) {}
	~PKCS7Ptr() { if (p_) PKCS7_free(p_); }

public:
	PKCS7* get() const { return p_; }

private:
	PKCS7Ptr(const PKCS7Ptr&);
	PKCS7Ptr& operator=(const PKCS7Ptr&);

private:
	PKCS7* p_;
};


/****************************************************************************
 *
 * X509Ptr
 *
 */

class X509Ptr
{
public:
	X509Ptr(X509* p) : p_(p) {}
	~X509Ptr() { if (p_) X509_free(p_); }

public:
	X509* get() const { return p_; }

private:
	X509Ptr(const X509Ptr&);
	X509Ptr& operator=(const X509Ptr&);

private:
	X509* p_;
};


/****************************************************************************
 *
 * X509StackPtr
 *
 */

class X509StackPtr
{
public:
	X509StackPtr(STACK_OF(X509)* p, bool bPop) : p_(p), bPop_(bPop) {}
	~X509StackPtr()
	{
		if (p_) {
			if (bPop_)
				sk_X509_pop_free(p_, X509_free);
			else
				sk_X509_free(p_);
		}
	}

public:
	STACK_OF(X509)* get() const { return p_; }

private:
	X509StackPtr(const X509StackPtr&);
	X509StackPtr& operator=(const X509StackPtr&);

private:
	STACK_OF(X509)* p_;
	bool bPop_;
};


/****************************************************************************
 *
 * EVP_PKEYPtr
 *
 */

class EVP_PKEYPtr
{
public:
	EVP_PKEYPtr(EVP_PKEY* p) : p_(p) {}
	~EVP_PKEYPtr() { if (p_) EVP_PKEY_free(p_); }

public:
	EVP_PKEY* get() const { return p_; }

private:
	EVP_PKEYPtr(const EVP_PKEYPtr&);
	EVP_PKEYPtr& operator=(const EVP_PKEYPtr&);

private:
	EVP_PKEY* p_;
};


/****************************************************************************
 *
 * Util
 *
 */

class Util
{
public:
	static qs::malloc_size_ptr<unsigned char> loadFromStream(qs::InputStream* pStream);
	static void logError(qs::Log& log,
						 const WCHAR* pwszMessage);
};

}

#endif // __UTIL_H__
