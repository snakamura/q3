/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsstream.h>
#include <qserror.h>
#include <qsnew.h>
#include <qsstring.h>
#include <qsconv.h>

#include <windows.h>

using namespace qs;


/****************************************************************************
 *
 * InputStream
 *
 */

qs::InputStream::~InputStream()
{
}


/****************************************************************************
 *
 * FileInputStreamImpl
 *
 */

struct qs::FileInputStreamImpl
{
	QSTATUS open(const WCHAR* pwszPath);
	
	HANDLE hFile_;
};

QSTATUS qs::FileInputStreamImpl::open(const WCHAR* pwszPath)
{
	DECLARE_QSTATUS();
	
	W2T(pwszPath, ptszPath);
	hFile_ = ::CreateFile(ptszPath, GENERIC_READ, FILE_SHARE_READ, 0,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (hFile_ == INVALID_HANDLE_VALUE) {
		hFile_ = 0;
		return QSTATUS_FAIL;
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * FileInputStream
 *
 */

qs::FileInputStream::FileInputStream(const WCHAR* pwszPath, QSTATUS* pstatus) :
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

qs::FileInputStream::~FileInputStream()
{
	if (pImpl_) {
		close();
		delete pImpl_;
		pImpl_ = 0;
	}
}

QSTATUS qs::FileInputStream::close()
{
	BOOL b = TRUE;
	if (pImpl_->hFile_) {
		b = ::CloseHandle(pImpl_->hFile_);
		pImpl_->hFile_ = 0;
	}
	return b ? QSTATUS_SUCCESS : QSTATUS_FAIL;
}

QSTATUS qs::FileInputStream::read(
	unsigned char* p, size_t nRead, size_t* pnRead)
{
	assert(p);
	assert(pnRead);
	
	*pnRead = 0;
	
	if (nRead == 0)
		return QSTATUS_SUCCESS;
	
	DECLARE_QSTATUS();
	
	while (nRead != 0) {
		DWORD dwRead = 0;
		if (!::ReadFile(pImpl_->hFile_, p, nRead, &dwRead, 0))
			return QSTATUS_FAIL;
		if (dwRead == 0) {
			if (*pnRead == 0)
				*pnRead = -1;
			return QSTATUS_SUCCESS;
		}
		else {
			*pnRead += dwRead;
			nRead -= dwRead;
			p += dwRead;
		}
	}
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * ByteInputStreamImpl
 *
 */

struct qs::ByteInputStreamImpl
{
	const unsigned char* pBuf_;
	const unsigned char* pBufEnd_;
	const unsigned char* p_;
};


/****************************************************************************
 *
 * ByteInputStream
 *
 */

qs::ByteInputStream::ByteInputStream(
	const unsigned char* p, size_t nLen, QSTATUS* pstatus) :
	pImpl_(0)
{
	assert(p);
	assert(pstatus);
	
	*pstatus = QSTATUS_SUCCESS;
	
	DECLARE_QSTATUS();
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	pImpl_->pBuf_ = p;
	pImpl_->pBufEnd_ = p + nLen;
	pImpl_->p_ = p;
}

qs::ByteInputStream::~ByteInputStream()
{
	delete pImpl_;
	pImpl_ = 0;
}

QSTATUS qs::ByteInputStream::close()
{
	return QSTATUS_SUCCESS;
}

QSTATUS qs::ByteInputStream::read(
	unsigned char* p, size_t nRead, size_t* pnRead)
{
	assert(p);
	assert(pnRead);
	
	*pnRead = 0;
	
	if (nRead == 0)
		return QSTATUS_SUCCESS;
	
	if (nRead > static_cast<size_t>(pImpl_->pBufEnd_ - pImpl_->p_))
		nRead = pImpl_->pBufEnd_ - pImpl_->p_;
	
	if (nRead != 0) {
		*pnRead = nRead;
		memcpy(p, pImpl_->p_, nRead);
		pImpl_->p_ += nRead;
	}
	else {
		*pnRead = -1;
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * BufferedInputStreamImpl
 *
 */

struct qs::BufferedInputStreamImpl
{
	enum {
		BUFFER_SIZE	= 4096
	};
	
	QSTATUS mapBuffer();
	
	InputStream* pInputStream_;
	bool bDelete_;
	unsigned char* pBuf_;
	unsigned char* pBufEnd_;
	unsigned char* p_;
};

QSTATUS qs::BufferedInputStreamImpl::mapBuffer()
{
	assert(p_ == pBufEnd_);
	
	DECLARE_QSTATUS();
	
	size_t nRead = 0;
	status = pInputStream_->read(pBuf_, BUFFER_SIZE, &nRead);
	CHECK_QSTATUS();
	if (nRead == -1)
		nRead = 0;
	pBufEnd_ = pBuf_ + nRead;
	p_ = pBuf_;
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * BufferedInputStream
 *
 */

qs::BufferedInputStream::BufferedInputStream(InputStream* pInputStream,
	bool bDelete, QSTATUS* pstatus) :
	pImpl_(0)
{
	assert(pInputStream);
	assert(pstatus);
	
	*pstatus = QSTATUS_SUCCESS;
	
	DECLARE_QSTATUS();
	
	malloc_ptr<unsigned char> pBuf(
		static_cast<unsigned char*>(malloc(BufferedInputStreamImpl::BUFFER_SIZE)));
	if (!pBuf.get()) {
		*pstatus = QSTATUS_OUTOFMEMORY;
		return;
	}
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	pImpl_->pInputStream_ = pInputStream;
	pImpl_->bDelete_ = bDelete;
	pImpl_->pBuf_ = pBuf.release();
	pImpl_->pBufEnd_ = pImpl_->pBuf_;
	pImpl_->p_ = pImpl_->pBuf_;
}

qs::BufferedInputStream::~BufferedInputStream()
{
	if (pImpl_) {
		if (pImpl_->bDelete_)
			delete pImpl_->pInputStream_;
		free(pImpl_->pBuf_);
		delete pImpl_;
		pImpl_ = 0;
	}
}

QSTATUS qs::BufferedInputStream::close()
{
	return pImpl_->pInputStream_->close();
}

QSTATUS qs::BufferedInputStream::read(
	unsigned char* p, size_t nRead, size_t* pnRead)
{
	assert(p);
	assert(pnRead);
	
	*pnRead = 0;
	
	if (nRead == 0)
		return QSTATUS_SUCCESS;
	
	if (static_cast<size_t>(pImpl_->pBufEnd_ - pImpl_->p_) > nRead) {
		memcpy(p, pImpl_->p_, nRead);
		pImpl_->p_ += nRead;
		*pnRead = nRead;
	}
	else {
		if (pImpl_->p_ != pImpl_->pBufEnd_) {
			size_t n = pImpl_->pBufEnd_ - pImpl_->p_;
			memcpy(p, pImpl_->p_, n);
			pImpl_->p_ = pImpl_->pBufEnd_;
			nRead -= n;
			*pnRead = n;
			p += n;
		}
		
		DECLARE_QSTATUS();
		
		if (nRead >= BufferedInputStreamImpl::BUFFER_SIZE/2) {
			size_t n = 0;
			status = pImpl_->pInputStream_->read(p, nRead, &n);
			CHECK_QSTATUS();
			if (n == -1) {
				if (*pnRead == 0)
					*pnRead = -1;
			}
			else {
				*pnRead += n;
			}
		}
		else {
			status = pImpl_->mapBuffer();
			CHECK_QSTATUS();
			if (static_cast<size_t>(pImpl_->pBufEnd_ - pImpl_->p_) < nRead)
				nRead = pImpl_->pBufEnd_ - pImpl_->p_;
			if (nRead != 0) {
				memcpy(p, pImpl_->p_, nRead);
				pImpl_->p_ += nRead;
				*pnRead += nRead;
			}
			else {
				if (*pnRead == 0)
					*pnRead = -1;
			}
		}
	}
	assert(*pnRead != 0);
	
	return QSTATUS_SUCCESS;
}
