/*
 * $Id: writer.cpp,v 1.1.1.1 2003/04/29 08:07:35 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsstream.h>
#include <qserror.h>
#include <qsconv.h>
#include <qsnew.h>

using namespace qs;


/****************************************************************************
 *
 * Writer
 *
 */

qs::Writer::~Writer()
{
}


/****************************************************************************
 *
 * OutputStreamWriterImpl
 *
 */

struct qs::OutputStreamWriterImpl
{
	enum {
		BUFFER_SIZE	= 128
	};
	
	QSTATUS flushBuffer();
	QSTATUS write(const WCHAR* p, size_t nSize);
	
	OutputStream* pOutputStream_;
	bool bDelete_;
	Converter* pConverter_;
	WCHAR* pBuf_;
	WCHAR* pBufEnd_;
	WCHAR* pCurrent_;
};

QSTATUS qs::OutputStreamWriterImpl::flushBuffer()
{
	DECLARE_QSTATUS();
	
	if (pCurrent_ != pBuf_) {
		string_ptr<STRING> str;
		size_t nLen = pCurrent_ - pBuf_;
		size_t nEncodedLen = 0;
		status = pConverter_->encode(pBuf_, &nLen, &str, &nEncodedLen);
		CHECK_QSTATUS();
		
		status = pOutputStream_->write(
			reinterpret_cast<unsigned char*>(str.get()), nEncodedLen);
		CHECK_QSTATUS();
		
		if (nLen != static_cast<size_t>(pCurrent_ - pBuf_))
			memmove(pBuf_, pCurrent_, (pCurrent_ - pBuf_ - nLen)*sizeof(WCHAR));
		pCurrent_ -= nLen;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::OutputStreamWriterImpl::write(const WCHAR* p, size_t nSize)
{
	assert(p);
	assert(nSize > static_cast<size_t>(pBufEnd_ - pCurrent_));
	
	DECLARE_QSTATUS();
	
	while (nSize != 0) {
		if (pCurrent_ == pBuf_) {
			string_ptr<STRING> str;
			size_t nLen = nSize;
			size_t nEncodedLen = 0;
			status = pConverter_->encode(p, &nLen, &str, &nEncodedLen);
			CHECK_QSTATUS();
			assert(nSize - nLen < BUFFER_SIZE);
			
			status = pOutputStream_->write(
				reinterpret_cast<unsigned char*>(str.get()), nEncodedLen);
			CHECK_QSTATUS();
			
			if (nLen != nSize)
				memcpy(pBuf_, p + nLen, nSize - nLen);
			pCurrent_ = pBuf_ + (nSize - nLen);
			
			nSize = 0;
		}
		else {
			size_t nRest = pCurrent_ - pBuf_;
			size_t nLen = QSMAX(static_cast<size_t>(pBufEnd_ - pCurrent_), nSize);
			memcpy(pCurrent_, p, nLen);
			
			WCHAR* pCurrentOld = pCurrent_;
			status = flushBuffer();
			CHECK_QSTATUS();
			
			if (pCurrent_ != pBuf_ &&
				static_cast<size_t>(pCurrentOld - pCurrent_) >= nRest) {
				p += pCurrentOld - pCurrent_ - nRest;
				nSize -= pCurrentOld - pCurrent_ - nRest;
				pCurrent_ = pBuf_;
			}
			else {
				p += nLen;
				nSize -= nLen;
			}
		}
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * OutputStreamWriter
 *
 */

qs::OutputStreamWriter::OutputStreamWriter(OutputStream* pOutputStream,
	bool bDelete, const WCHAR* pwszEncoding, QSTATUS* pstatus) :
	pImpl_(0)
{
	assert(pOutputStream);
	assert(pstatus);
	
	*pstatus = QSTATUS_SUCCESS;
	
	DECLARE_QSTATUS();
	
	malloc_ptr<WCHAR> pBuf(static_cast<WCHAR*>(
		malloc(OutputStreamWriterImpl::BUFFER_SIZE*sizeof(WCHAR))));
	if (!pBuf.get()) {
		*pstatus = QSTATUS_OUTOFMEMORY;
		return;
	}
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	pImpl_->pOutputStream_ = pOutputStream;
	pImpl_->bDelete_ = bDelete;
	pImpl_->pConverter_ = 0;
	pImpl_->pBuf_ = pBuf.release();
	pImpl_->pBufEnd_ = pImpl_->pBuf_ + OutputStreamWriterImpl::BUFFER_SIZE;
	pImpl_->pCurrent_ = pImpl_->pBuf_;
	
	if (!pwszEncoding)
		pwszEncoding = getSystemEncoding();
	status = ConverterFactory::getInstance(
		pwszEncoding, &pImpl_->pConverter_);
	CHECK_QSTATUS_SET(pstatus);
	if (!pImpl_->pConverter_) {
		*pstatus = QSTATUS_FAIL;
		return;
	}
}

qs::OutputStreamWriter::~OutputStreamWriter()
{
	if (pImpl_) {
		if (pImpl_->bDelete_)
			delete pImpl_->pOutputStream_;
		delete pImpl_->pConverter_;
		free(pImpl_->pBuf_);
		delete pImpl_;
		pImpl_ = 0;
	}
}

QSTATUS qs::OutputStreamWriter::close()
{
	DECLARE_QSTATUS();
	
	status = pImpl_->flushBuffer();
	CHECK_QSTATUS();
	
	status = pImpl_->pOutputStream_->close();
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::OutputStreamWriter::write(const WCHAR* p, size_t nWrite)
{
	assert(p);
	
	DECLARE_QSTATUS();
	
	if (static_cast<size_t>(pImpl_->pBufEnd_ - pImpl_->pCurrent_) >= nWrite) {
		memcpy(pImpl_->pCurrent_, p, nWrite*sizeof(WCHAR));
		pImpl_->pCurrent_ += nWrite;
	}
	else {
		status = pImpl_->flushBuffer();
		CHECK_QSTATUS();
		
		if (nWrite > static_cast<size_t>(pImpl_->pBufEnd_ - pImpl_->pCurrent_)) {
			status = pImpl_->write(p, nWrite);
		}
		else {
			memcpy(pImpl_->pCurrent_, p, nWrite*sizeof(WCHAR));
			pImpl_->pCurrent_ += nWrite;
		}
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * StringWriterImpl
 *
 */

struct qs::StringWriterImpl
{
	enum {
		BUFFER_SIZE	= 256
	};
	
	QSTATUS allocBuffer(size_t nSize);
	
	WSTRING wstr_;
	WCHAR* pEnd_;
	WCHAR* pCurrent_;
};

QSTATUS qs::StringWriterImpl::allocBuffer(size_t nSize)
{
	size_t nNewSize = 0;
	if (!wstr_) {
		nNewSize = QSMAX(static_cast<size_t>(BUFFER_SIZE), nSize);
	}
	else {
		nNewSize = pEnd_ - wstr_;
		nNewSize += QSMAX(nNewSize, nSize);
	}
	
	WSTRING wstr = wstr_;
	if (!wstr_)
		wstr_ = allocWString(nNewSize);
	else
		wstr_ = reallocWString(wstr_, nNewSize);
	if (!wstr_)
		return QSTATUS_OUTOFMEMORY;
	pEnd_ = wstr_ + nNewSize;
	pCurrent_ = wstr_ + (pCurrent_ - wstr);
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * StringWriter
 *
 */

qs::StringWriter::StringWriter(QSTATUS* pstatus) :
	pImpl_(0)
{
	assert(pstatus);
	
	*pstatus = QSTATUS_SUCCESS;
	
	DECLARE_QSTATUS();
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	pImpl_->wstr_ = 0;
	pImpl_->pEnd_ = 0;
	pImpl_->pCurrent_ = 0;
}

qs::StringWriter::~StringWriter()
{
	if (pImpl_) {
		freeWString(pImpl_->wstr_);
		delete pImpl_;
		pImpl_ = 0;
	}
}

QSTATUS qs::StringWriter::close()
{
	return QSTATUS_SUCCESS;
}

QSTATUS qs::StringWriter::write(const WCHAR* p, size_t nWrite)
{
	DECLARE_QSTATUS();
	
	if (static_cast<size_t>(pImpl_->pEnd_ - pImpl_->pCurrent_) < nWrite) {
		status = pImpl_->allocBuffer(nWrite);
		CHECK_QSTATUS();
	}
	memcpy(pImpl_->pCurrent_, p, nWrite*sizeof(WCHAR));
	pImpl_->pCurrent_ += nWrite;
	
	return QSTATUS_SUCCESS;
	
}


/****************************************************************************
 *
 * BufferedWriterImpl
 *
 */

struct qs::BufferedWriterImpl
{
	enum {
		BUFFER_SIZE	= 4096
	};
	
	QSTATUS flushBuffer();
	
	Writer* pWriter_;
	bool bDelete_;
	WCHAR* pBuf_;
	WCHAR* pBufEnd_;
	WCHAR* pCurrent_;
};

QSTATUS qs::BufferedWriterImpl::flushBuffer()
{
	DECLARE_QSTATUS();
	
	if (pCurrent_ != pBuf_) {
		status = pWriter_->write(pBuf_, pCurrent_ - pBuf_);
		CHECK_QSTATUS();
		
		pCurrent_ = pBuf_;
	}
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * BufferedWriter
 *
 */

qs::BufferedWriter::BufferedWriter(Writer* pWriter,
	bool bDelete, QSTATUS* pstatus) :
	pImpl_(0)
{
	assert(pstatus);
	
	*pstatus = QSTATUS_SUCCESS;
	
	DECLARE_QSTATUS();
	
	malloc_ptr<WCHAR> pBuf(static_cast<WCHAR*>(
		malloc(BufferedWriterImpl::BUFFER_SIZE*sizeof(WCHAR))));
	if (!pBuf.get()) {
		*pstatus = QSTATUS_OUTOFMEMORY;
		return;
	}
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	pImpl_->pWriter_ = pWriter;
	pImpl_->bDelete_ = bDelete;
	pImpl_->pBuf_ = pBuf.release();
	pImpl_->pBufEnd_ = pImpl_->pBuf_ + BufferedWriterImpl::BUFFER_SIZE;
	pImpl_->pCurrent_ = pImpl_->pBuf_;
}

qs::BufferedWriter::~BufferedWriter()
{
	if (pImpl_) {
		if (pImpl_->bDelete_)
			delete pImpl_->pWriter_;
		free(pImpl_->pBuf_);
		delete pImpl_;
		pImpl_ = 0;
	}
}

QSTATUS qs::BufferedWriter::newLine()
{
	return write(L"\r\n", 2);
}

QSTATUS qs::BufferedWriter::close()
{
	DECLARE_QSTATUS();
	
	status = pImpl_->flushBuffer();
	CHECK_QSTATUS();
	
	return pImpl_->pWriter_->close();
}

QSTATUS qs::BufferedWriter::write(const WCHAR* p, size_t nWrite)
{
	assert(p);
	
	DECLARE_QSTATUS();
	
	if (static_cast<size_t>(pImpl_->pBufEnd_ - pImpl_->pCurrent_) >= nWrite) {
		memcpy(pImpl_->pCurrent_, p, nWrite*sizeof(WCHAR));
		pImpl_->pCurrent_ += nWrite;
	}
	else {
		status = pImpl_->flushBuffer();
		CHECK_QSTATUS();
		
		if (nWrite > BufferedWriterImpl::BUFFER_SIZE/2) {
			status = pImpl_->pWriter_->write(p, nWrite);
			CHECK_QSTATUS();
		}
		else {
			memcpy(pImpl_->pCurrent_, p, nWrite*sizeof(WCHAR));
			pImpl_->pCurrent_ += nWrite;
		}
	}
	return QSTATUS_SUCCESS;
}
