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
	bool open(const WCHAR* pwszPath);
	
	HANDLE hFile_;
};

bool qs::FileOutputStreamImpl::open(const WCHAR* pwszPath)
{
	W2T(pwszPath, ptszPath);
	AutoHandle hFile(::CreateFile(ptszPath, GENERIC_WRITE,
		FILE_SHARE_READ, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0));
	if (!hFile.get())
		return false;
	hFile_ = hFile.release();
	
	return true;
}


/****************************************************************************
 *
 * FileOutputStream
 *
 */

qs::FileOutputStream::FileOutputStream(const WCHAR* pwszPath) :
	pImpl_(0)
{
	std::auto_ptr<FileOutputStreamImpl> pImpl(new FileOutputStreamImpl());
	pImpl->hFile_ = 0;
	
	if (!pImpl->open(pwszPath))
		return;
	pImpl_ = pImpl.release();
}

qs::FileOutputStream::~FileOutputStream()
{
	if (pImpl_) {
		close();
		delete pImpl_;
		pImpl_ = 0;
	}
}

bool qs::FileOutputStream::operator!() const
{
	return pImpl_ == 0;
}

bool qs::FileOutputStream::close()
{
	bool b = true;
	if (pImpl_->hFile_) {
		b = ::CloseHandle(pImpl_->hFile_) != 0;
		pImpl_->hFile_ = 0;
	}
	return b;
}

size_t qs::FileOutputStream::write(const unsigned char* p,
								   size_t nWrite)
{
	assert(p);
	
	if (nWrite == 0)
		return 0;
	
	while (nWrite != 0) {
		DWORD dwWritten = 0;
		if (!::WriteFile(pImpl_->hFile_, p, static_cast<DWORD>(nWrite), &dwWritten, 0))
			return -1;
		nWrite -= dwWritten;
		p += dwWritten;
	}
	
	return nWrite;
}

bool qs::FileOutputStream::flush()
{
//	return ::FlushFileBuffers(pImpl_->hFile_) != 0;
	return true;
}

/****************************************************************************
 *
 * ByteOutputStreamImpl
 *
 */

struct qs::ByteOutputStreamImpl
{
	enum {
		BUFFER_SIZE	= 1024
	};
	
	bool allocBuffer(size_t nSize);
	
	unsigned char* pBuf_;
	unsigned char* pBufEnd_;
	unsigned char* p_;
};

bool qs::ByteOutputStreamImpl::allocBuffer(size_t nSize)
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
		return false;
	p_ = pNew.get() + (p_ - pBuf_);
	pBufEnd_ = pNew.get() + nNewSize;
	pBuf_ = pNew.release();
	
	return true;
}


/****************************************************************************
 *
 * ByteOutputStream
 *
 */

qs::ByteOutputStream::ByteOutputStream()
{
	pImpl_ = new ByteOutputStreamImpl();
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

const unsigned char* qs::ByteOutputStream::getBuffer()
{
	if (!pImpl_->pBuf_) {
		if (!pImpl_->allocBuffer(1))
			return 0;
	}
	
	return pImpl_->pBuf_;
}

malloc_ptr<unsigned char> qs::ByteOutputStream::releaseBuffer()
{
	if (!pImpl_->pBuf_) {
		if (!pImpl_->allocBuffer(1))
			return malloc_ptr<unsigned char>(0);
	}
	
	unsigned char* p = pImpl_->pBuf_;
	pImpl_->pBuf_ = 0;
	pImpl_->pBufEnd_ = 0;
	pImpl_->p_ = 0;
	return malloc_ptr<unsigned char>(p);
}

malloc_size_ptr<unsigned char> qs::ByteOutputStream::releaseSizeBuffer()
{
	if (!pImpl_->pBuf_) {
		if (!pImpl_->allocBuffer(1))
			return malloc_size_ptr<unsigned char>();
	}
	
	unsigned char* p = pImpl_->pBuf_;
	size_t nLen = pImpl_->p_ - pImpl_->pBuf_;
	pImpl_->pBuf_ = 0;
	pImpl_->pBufEnd_ = 0;
	pImpl_->p_ = 0;
	return malloc_size_ptr<unsigned char>(p, nLen);
}

size_t qs::ByteOutputStream::getLength() const
{
	return pImpl_->p_ - pImpl_->pBuf_;
}

bool qs::ByteOutputStream::reserve(size_t nLength)
{
	if (static_cast<size_t>(pImpl_->pBufEnd_ - pImpl_->p_) < nLength)
		return pImpl_->allocBuffer(nLength);
	else
		return true;
}

bool qs::ByteOutputStream::close()
{
	return true;
}

size_t qs::ByteOutputStream::write(const unsigned char* p,
								   size_t nWrite)
{
	if (static_cast<size_t>(pImpl_->pBufEnd_ - pImpl_->p_) < nWrite) {
		if (!pImpl_->allocBuffer(nWrite))
			return -1;
	}
	memcpy(pImpl_->p_, p, nWrite);
	pImpl_->p_ += nWrite;
	
	return nWrite;
}

bool qs::ByteOutputStream::flush()
{
	return true;
}


/****************************************************************************
 *
 * XStringOutputStream
 *
 */

qs::XStringOutputStream::XStringOutputStream()
{
}

qs::XStringOutputStream::~XStringOutputStream()
{
}

xstring_ptr qs::XStringOutputStream::getXString()
{
	unsigned char c = 0;
	if (stream_.write(&c, 1) != 1)
		return 0;
	
	malloc_ptr<unsigned char> p(stream_.releaseBuffer());
	return xstring_ptr(reinterpret_cast<XSTRING>(p.release()));
}

bool qs::XStringOutputStream::reserve(size_t nLength)
{
	return stream_.reserve(nLength);
}

bool qs::XStringOutputStream::close()
{
	return stream_.close();
}

size_t qs::XStringOutputStream::write(const unsigned char* p,
									  size_t nWrite)
{
	return stream_.write(p, nWrite);
}

bool qs::XStringOutputStream::flush()
{
	return stream_.flush();
}


/****************************************************************************
 *
 * BufferedOutputStreamImpl
 *
 */

struct qs::BufferedOutputStreamImpl
{
	enum {
		BUFFER_SIZE	= 8192
	};
	
	bool flushBuffer();
	
	OutputStream* pOutputStream_;
	bool bDelete_;
	auto_ptr_array<unsigned char> pBuf_;
	unsigned char* p_;
};

bool qs::BufferedOutputStreamImpl::flushBuffer()
{
	if (p_ != pBuf_.get()) {
		if (pOutputStream_->write(pBuf_.get(), p_ - pBuf_.get()) == -1)
			return false;
		p_ = pBuf_.get();
	}
	return true;
}


/****************************************************************************
 *
 * BufferedOutputStream
 *
 */

qs::BufferedOutputStream::BufferedOutputStream(OutputStream* pOutputStream,
											   bool bDelete)
{
	assert(pOutputStream);
	
	auto_ptr_array<unsigned char> pBuf(
		new unsigned char[BufferedOutputStreamImpl::BUFFER_SIZE]);
	
	pImpl_ = new BufferedOutputStreamImpl();
	pImpl_->pOutputStream_ = pOutputStream;
	pImpl_->bDelete_ = bDelete;
	pImpl_->pBuf_ = pBuf;
	pImpl_->p_ = pImpl_->pBuf_.get();
}

qs::BufferedOutputStream::~BufferedOutputStream()
{
	if (pImpl_) {
		close();
		if (pImpl_->bDelete_)
			delete pImpl_->pOutputStream_;
		delete pImpl_;
		pImpl_ = 0;
	}
}

bool qs::BufferedOutputStream::close()
{
	if (!flush())
		return false;
	return pImpl_->pOutputStream_->close();
}

size_t qs::BufferedOutputStream::write(const unsigned char* p,
									   size_t nWrite)
{
	assert(p);
	
	if (static_cast<size_t>(pImpl_->pBuf_.get() + BufferedOutputStreamImpl::BUFFER_SIZE - pImpl_->p_) >= nWrite) {
		memcpy(pImpl_->p_, p, nWrite);
		pImpl_->p_ += nWrite;
	}
	else {
		if (!pImpl_->flushBuffer())
			return -1;
		
		if (nWrite > BufferedOutputStreamImpl::BUFFER_SIZE/2) {
			if (pImpl_->pOutputStream_->write(p, nWrite) == -1)
				return -1;
		}
		else {
			memcpy(pImpl_->p_, p, nWrite);
			pImpl_->p_ += nWrite;
		}
	}
	
	return nWrite;
}

bool qs::BufferedOutputStream::flush()
{
	if (!pImpl_->flushBuffer())
		return false;
	return pImpl_->pOutputStream_->flush();
}
