/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsconv.h>
#include <qsosutil.h>
#include <qsstream.h>
#include <qsstring.h>

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
	bool open(const WCHAR* pwszPath);
	
	HANDLE hFile_;
};

bool qs::FileInputStreamImpl::open(const WCHAR* pwszPath)
{
	W2T(pwszPath, ptszPath);
	AutoHandle hFile(::CreateFile(ptszPath, GENERIC_READ,
		FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0));
	if (!hFile.get())
		return false;
	hFile_ = hFile.release();
	
	return true;
}


/****************************************************************************
 *
 * FileInputStream
 *
 */

qs::FileInputStream::FileInputStream(const WCHAR* pwszPath) :
	pImpl_(0)
{
	std::auto_ptr<FileInputStreamImpl> pImpl(new FileInputStreamImpl());
	pImpl->hFile_ = 0;
	
	if (!pImpl->open(pwszPath))
		return;
	
	pImpl_ = pImpl.release();
}

qs::FileInputStream::~FileInputStream()
{
	if (pImpl_) {
		close();
		delete pImpl_;
		pImpl_ = 0;
	}
}

bool qs::FileInputStream::operator!() const
{
	return pImpl_ == 0;
}

bool qs::FileInputStream::close()
{
	bool b = true;
	if (pImpl_->hFile_) {
		b = ::CloseHandle(pImpl_->hFile_) != 0;
		pImpl_->hFile_ = 0;
	}
	return b;
}

size_t qs::FileInputStream::read(unsigned char* p,
								 size_t nRead)
{
	assert(p);
	
	if (nRead == 0)
		return 0;
	
	size_t nSize = 0;
	
	while (nRead != 0) {
		DWORD dwRead = 0;
		if (!::ReadFile(pImpl_->hFile_, p, nRead, &dwRead, 0))
			return -1;
		if (dwRead == 0) {
			break;
		}
		else {
			nSize += dwRead;
			nRead -= dwRead;
			p += dwRead;
		}
	}
	return nSize;
}


/****************************************************************************
 *
 * ByteInputStreamImpl
 *
 */

struct qs::ByteInputStreamImpl
{
	malloc_ptr<unsigned char> pBuf_;
	const unsigned char* pBegin_;
	const unsigned char* pEnd_;
	const unsigned char* p_;
};


/****************************************************************************
 *
 * ByteInputStream
 *
 */

qs::ByteInputStream::ByteInputStream(const unsigned char* p,
									 size_t nLen,
									 bool bCopy) :
	pImpl_(0)
{
	assert(p);
	
	malloc_ptr<unsigned char> pBuf;
	if (bCopy) {
		pBuf.reset(static_cast<unsigned char*>(malloc(nLen)));
		if (!pBuf.get())
			return;
		memcpy(pBuf.get(), p, nLen);
	}
	
	pImpl_ = new ByteInputStreamImpl();
	pImpl_->pBuf_ = pBuf;
	pImpl_->pBegin_ = bCopy ? pImpl_->pBuf_.get() : p;
	pImpl_->pEnd_ = pImpl_->pBegin_ + nLen;
	pImpl_->p_ = pImpl_->pBegin_;
}

qs::ByteInputStream::ByteInputStream(malloc_ptr<unsigned char> p,
									 size_t nLen) :
	pImpl_(0)
{
	assert(p.get());
	
	pImpl_ = new ByteInputStreamImpl();
	pImpl_->pBuf_ = p;
	pImpl_->pBegin_ = pImpl_->pBuf_.get();
	pImpl_->pEnd_ = pImpl_->pBegin_ + nLen;
	pImpl_->p_ = pImpl_->pBegin_;
}

qs::ByteInputStream::~ByteInputStream()
{
	delete pImpl_;
	pImpl_ = 0;
}

bool qs::ByteInputStream::operator!() const
{
	return pImpl_ == 0;
}

bool qs::ByteInputStream::close()
{
	return true;
}

size_t qs::ByteInputStream::read(unsigned char* p,
								 size_t nRead)
{
	assert(p);
	
	if (nRead == 0)
		return 0;
	
	if (nRead > static_cast<size_t>(pImpl_->pEnd_ - pImpl_->p_))
		nRead = pImpl_->pEnd_ - pImpl_->p_;
	
	memcpy(p, pImpl_->p_, nRead);
	pImpl_->p_ += nRead;
	
	return nRead;
}


/****************************************************************************
 *
 * BufferedInputStreamImpl
 *
 */

struct qs::BufferedInputStreamImpl
{
	enum {
		BUFFER_SIZE	= 8192
	};
	
	bool mapBuffer();
	
	InputStream* pInputStream_;
	bool bDelete_;
	auto_ptr_array<unsigned char> pBuf_;
	unsigned char* pBufEnd_;
	unsigned char* p_;
};

bool qs::BufferedInputStreamImpl::mapBuffer()
{
	assert(p_ == pBufEnd_);
	
	size_t nRead = pInputStream_->read(pBuf_.get(), BUFFER_SIZE);
	if (nRead == -1)
		return false;
	pBufEnd_ = pBuf_.get() + nRead;
	p_ = pBuf_.get();
	
	return true;
}


/****************************************************************************
 *
 * BufferedInputStream
 *
 */

qs::BufferedInputStream::BufferedInputStream(InputStream* pInputStream,
											 bool bDelete) :
	pImpl_(0)
{
	assert(pInputStream);
	
	auto_ptr_array<unsigned char> pBuf(
		new unsigned char[BufferedInputStreamImpl::BUFFER_SIZE]);
	
	pImpl_ = new BufferedInputStreamImpl();
	pImpl_->pInputStream_ = pInputStream;
	pImpl_->bDelete_ = bDelete;
	pImpl_->pBuf_ = pBuf;
	pImpl_->pBufEnd_ = pImpl_->pBuf_.get();
	pImpl_->p_ = pImpl_->pBuf_.get();
}

qs::BufferedInputStream::~BufferedInputStream()
{
	if (pImpl_) {
		if (pImpl_->bDelete_)
			delete pImpl_->pInputStream_;
		delete pImpl_;
		pImpl_ = 0;
	}
}

bool qs::BufferedInputStream::close()
{
	return pImpl_->pInputStream_->close();
}

size_t qs::BufferedInputStream::read(unsigned char* p,
									 size_t nRead)
{
	assert(p);
	
	if (nRead == 0)
		return 0;
	
	size_t nSize = 0;
	
	if (static_cast<size_t>(pImpl_->pBufEnd_ - pImpl_->p_) > nRead) {
		memcpy(p, pImpl_->p_, nRead);
		pImpl_->p_ += nRead;
		nSize = nRead;
	}
	else {
		if (pImpl_->p_ != pImpl_->pBufEnd_) {
			size_t n = pImpl_->pBufEnd_ - pImpl_->p_;
			memcpy(p, pImpl_->p_, n);
			pImpl_->p_ = pImpl_->pBufEnd_;
			nRead -= n;
			nSize = n;
			p += n;
		}
		
		if (nRead >= BufferedInputStreamImpl::BUFFER_SIZE/2) {
			size_t n = pImpl_->pInputStream_->read(p, nRead);
			if (n == -1)
				return -1;
			nSize += n;
		}
		else {
			if (!pImpl_->mapBuffer())
				return -1;
			if (static_cast<size_t>(pImpl_->pBufEnd_ - pImpl_->p_) < nRead)
				nRead = pImpl_->pBufEnd_ - pImpl_->p_;
			if (nRead != 0) {
				memcpy(p, pImpl_->p_, nRead);
				pImpl_->p_ += nRead;
				nSize += nRead;
			}
		}
	}
	
	return nSize;
}
