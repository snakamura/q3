/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsstream.h>
#include <qserror.h>
#include <qsstring.h>
#include <qsconv.h>
#include <qsnew.h>

#include <windows.h>

using namespace qs;


/****************************************************************************
 *
 * OutputStream
 *
 */

qs::OutputStream::~OutputStream()
{
}


/****************************************************************************
 *
 * FileOutputStreamImpl
 *
 */

struct qs::FileOutputStreamImpl
{
	QSTATUS open(const WCHAR* pwszPath);
	
	HANDLE hFile_;
};

QSTATUS qs::FileOutputStreamImpl::open(const WCHAR* pwszPath)
{
	DECLARE_QSTATUS();
	
	W2T(pwszPath, ptszPath);
	hFile_ = ::CreateFile(ptszPath, GENERIC_WRITE, FILE_SHARE_READ,
		0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	if (hFile_ == INVALID_HANDLE_VALUE) {
		hFile_ = 0;
		return QSTATUS_FAIL;
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * FileOutputStream
 *
 */

qs::FileOutputStream::FileOutputStream(const WCHAR* pwszPath, QSTATUS* pstatus) :
	pImpl_(0)
{
	assert(pstatus);
	
	*pstatus = QSTATUS_SUCCESS;
	
	DECLARE_QSTATUS();
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	pImpl_->hFile_ = 0;
	
	status = pImpl_->open(pwszPath);
	CHECK_QSTATUS_SET(pstatus);
}

qs::FileOutputStream::~FileOutputStream()
{
	if (pImpl_) {
		close();
		delete pImpl_;
		pImpl_ = 0;
	}
}

QSTATUS qs::FileOutputStream::close()
{
	BOOL b = TRUE;
	if (pImpl_->hFile_) {
		b = ::CloseHandle(pImpl_->hFile_);
		pImpl_->hFile_ = 0;
	}
	return b ? QSTATUS_SUCCESS : QSTATUS_FAIL;
}

QSTATUS qs::FileOutputStream::write(const unsigned char* p, size_t nWrite)
{
	assert(p);
	
	if (nWrite == 0)
		return QSTATUS_SUCCESS;
	
	while (nWrite != 0) {
		DWORD dwWritten = 0;
		if (!::WriteFile(pImpl_->hFile_, p, nWrite, &dwWritten, 0))
			return QSTATUS_FAIL;
		nWrite -= dwWritten;
		p += dwWritten;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::FileOutputStream::flush()
{
	return ::FlushFileBuffers(pImpl_->hFile_) ? QSTATUS_SUCCESS : QSTATUS_FAIL;
}

/****************************************************************************
 *
 * ByteOutputStreamImpl
 *
 */

struct qs::ByteOutputStreamImpl
{
	enum {
		BUFFER_SIZE	= 128
	};
	
	QSTATUS allocBuffer(size_t nSize);
	
	unsigned char* pBuf_;
	unsigned char* pBufEnd_;
	unsigned char* p_;
};

QSTATUS qs::ByteOutputStreamImpl::allocBuffer(size_t nSize)
{
	size_t nNewSize = 0;
	if (pBuf_ == pBufEnd_) {
		nNewSize = QSMAX(static_cast<size_t>(BUFFER_SIZE), nSize);
	}
	else {
		nNewSize = pBufEnd_ - pBuf_;
		nNewSize += QSMAX(nNewSize, nSize);
	}
	
	malloc_ptr<unsigned char> pNew(
		static_cast<unsigned char*>(realloc(pBuf_, nNewSize)));
	if (!pNew.get())
		return QSTATUS_OUTOFMEMORY;
	p_ = pNew.get() + (p_ - pBuf_);
	pBufEnd_ = pNew.get() + nNewSize;
	pBuf_ = pNew.release();
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * ByteOutputStream
 *
 */

qs::ByteOutputStream::ByteOutputStream(QSTATUS* pstatus)
{
	assert(pstatus);
	
	*pstatus = QSTATUS_SUCCESS;
	
	DECLARE_QSTATUS();
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	pImpl_->pBuf_ = 0;
	pImpl_->pBufEnd_ = 0;
	pImpl_->p_ = 0;
}

qs::ByteOutputStream::~ByteOutputStream()
{
	if (pImpl_) {
		free(pImpl_->pBuf_);
		delete pImpl_;
		pImpl_ = 0;
	}
}

const unsigned char* qs::ByteOutputStream::getBuffer() const
{
	return pImpl_->pBuf_;
}

unsigned char* qs::ByteOutputStream::releaseBuffer()
{
	unsigned char* p = pImpl_->pBuf_;
	pImpl_->pBuf_ = 0;
	pImpl_->pBufEnd_ = 0;
	pImpl_->p_ = 0;
	return p;
}

size_t qs::ByteOutputStream::getLength() const
{
	return pImpl_->p_ - pImpl_->pBuf_;
}

QSTATUS qs::ByteOutputStream::close()
{
	return QSTATUS_SUCCESS;
}

QSTATUS qs::ByteOutputStream::write(const unsigned char* p, size_t nWrite)
{
	DECLARE_QSTATUS();
	
	if (static_cast<size_t>(pImpl_->pBufEnd_ - pImpl_->p_) < nWrite) {
		status = pImpl_->allocBuffer(nWrite);
		CHECK_QSTATUS();
	}
	memcpy(pImpl_->p_, p, nWrite);
	pImpl_->p_ += nWrite;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::ByteOutputStream::flush()
{
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * BufferedOutputStreamImpl
 *
 */

struct qs::BufferedOutputStreamImpl
{
	enum {
		BUFFER_SIZE	= 4096
	};
	
	QSTATUS flushBuffer();
	
	OutputStream* pOutputStream_;
	bool bDelete_;
	unsigned char* pBuf_;
	unsigned char* p_;
};

QSTATUS qs::BufferedOutputStreamImpl::flushBuffer()
{
	DECLARE_QSTATUS();
	
	if (p_ != pBuf_) {
		status = pOutputStream_->write(pBuf_, p_ - pBuf_);
		CHECK_QSTATUS();
		p_ = pBuf_;
	}
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * BufferedOutputStream
 *
 */

qs::BufferedOutputStream::BufferedOutputStream(OutputStream* pOutputStream,
	bool bDelete, QSTATUS* pstatus)
{
	assert(pOutputStream);
	assert(pstatus);
	
	*pstatus = QSTATUS_SUCCESS;
	
	DECLARE_QSTATUS();
	
	malloc_ptr<unsigned char> pBuf(
		static_cast<unsigned char*>(malloc(BufferedOutputStreamImpl::BUFFER_SIZE)));
	if (!pBuf.get()) {
		*pstatus = QSTATUS_OUTOFMEMORY;
		return;
	}
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	pImpl_->pOutputStream_ = pOutputStream;
	pImpl_->bDelete_ = bDelete;
	pImpl_->pBuf_ = pBuf.release();
	pImpl_->p_ = pImpl_->pBuf_;
}

qs::BufferedOutputStream::~BufferedOutputStream()
{
	if (pImpl_) {
		close();
		if (pImpl_->bDelete_)
			delete pImpl_->pOutputStream_;
		free(pImpl_->pBuf_);
		delete pImpl_;
		pImpl_ = 0;
	}
}

QSTATUS qs::BufferedOutputStream::close()
{
	DECLARE_QSTATUS();
	
	status = flush();
	CHECK_QSTATUS();
	
	return pImpl_->pOutputStream_->close();
}

QSTATUS qs::BufferedOutputStream::write(const unsigned char* p, size_t nWrite)
{
	assert(p);
	
	DECLARE_QSTATUS();
	
	if (static_cast<size_t>(pImpl_->pBuf_ + BufferedOutputStreamImpl::BUFFER_SIZE - pImpl_->p_) >= nWrite) {
		memcpy(pImpl_->p_, p, nWrite);
		pImpl_->p_ += nWrite;
	}
	else {
		status = pImpl_->flushBuffer();
		CHECK_QSTATUS();
		
		if (nWrite > BufferedOutputStreamImpl::BUFFER_SIZE/2) {
			status = pImpl_->pOutputStream_->write(p, nWrite);
			CHECK_QSTATUS();
		}
		else {
			memcpy(pImpl_->p_, p, nWrite);
			pImpl_->p_ += nWrite;
		}
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::BufferedOutputStream::flush()
{
	DECLARE_QSTATUS();
	
	status = pImpl_->flushBuffer();
	CHECK_QSTATUS();
	
	return pImpl_->pOutputStream_->flush();
}
