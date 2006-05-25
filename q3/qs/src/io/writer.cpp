/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsconv.h>
#include <qsstream.h>

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
		BUFFER_SIZE	= 8192
	};
	
	bool flushBuffer();
	bool write(const WCHAR* p,
			   size_t nSize);
	bool writeToStream(const unsigned char* p,
					   size_t nLen);
	
	OutputStream* pOutputStream_;
	bool bDelete_;
	std::auto_ptr<Converter> pConverter_;
	WCHAR* pBuf_;
	WCHAR* pBufEnd_;
	WCHAR* pCurrent_;
};

bool qs::OutputStreamWriterImpl::flushBuffer()
{
	if (pCurrent_ != pBuf_) {
		size_t nLen = pCurrent_ - pBuf_;
		xstring_size_ptr encoded(pConverter_->encode(pBuf_, &nLen));
		if (!encoded.get())
			return false;
		
		if (!writeToStream(reinterpret_cast<unsigned char*>(encoded.get()), encoded.size()))
			return false;
		
		if (nLen != static_cast<size_t>(pCurrent_ - pBuf_))
			memmove(pBuf_, pCurrent_, (pCurrent_ - pBuf_ - nLen)*sizeof(WCHAR));
		pCurrent_ -= nLen;
	}
	
	return true;
}

bool qs::OutputStreamWriterImpl::write(const WCHAR* p,
									   size_t nSize)
{
	assert(p);
	assert(nSize > static_cast<size_t>(pBufEnd_ - pCurrent_));
	
	while (nSize != 0) {
		if (pCurrent_ == pBuf_) {
			size_t nLen = nSize;
			xstring_size_ptr encoded(pConverter_->encode(p, &nLen));
			if (!encoded.get())
				return false;
			assert(nSize - nLen < BUFFER_SIZE);
			
			if (!writeToStream(reinterpret_cast<unsigned char*>(encoded.get()), encoded.size()))
				return false;
			
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
			if (!flushBuffer())
				return false;
			
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
	
	return true;
}

bool qs::OutputStreamWriterImpl::writeToStream(const unsigned char* p,
											   size_t nLen)
{
	const unsigned char* pszNewLine = reinterpret_cast<const unsigned char*>("\r\n");
	
	const unsigned char* pBegin = p;
	const unsigned char* pEnd = p + nLen;
	while (true) {
		if (p == pEnd || *p == '\r' || *p == '\n') {
			if (p != pBegin) {
				if (pOutputStream_->write(pBegin, p - pBegin) == -1)
					return false;
			}
			if (p == pEnd) {
				break;
			}
			else if (*p == '\n') {
				if (pOutputStream_->write(pszNewLine, 2) == -1)
					return false;
			}
			pBegin = p + 1;
		}
		++p;
	}
	
	return true;
}


/****************************************************************************
 *
 * OutputStreamWriter
 *
 */

qs::OutputStreamWriter::OutputStreamWriter(OutputStream* pOutputStream,
										   bool bDelete,
										   const WCHAR* pwszEncoding) :
	pImpl_(0)
{
	assert(pOutputStream);
	
	malloc_ptr<WCHAR> pBuf(static_cast<WCHAR*>(
		malloc(OutputStreamWriterImpl::BUFFER_SIZE*sizeof(WCHAR))));
	if (!pBuf.get())
		return;
	
	if (!pwszEncoding)
		pwszEncoding = getSystemEncoding();
	std::auto_ptr<Converter> pConverter(ConverterFactory::getInstance(pwszEncoding));
	if (!pConverter.get())
		return;
	
	pImpl_ = new OutputStreamWriterImpl();
	pImpl_->pOutputStream_ = pOutputStream;
	pImpl_->bDelete_ = bDelete;
	pImpl_->pConverter_ = pConverter;
	pImpl_->pBuf_ = pBuf.release();
	pImpl_->pBufEnd_ = pImpl_->pBuf_ + OutputStreamWriterImpl::BUFFER_SIZE;
	pImpl_->pCurrent_ = pImpl_->pBuf_;
}

qs::OutputStreamWriter::~OutputStreamWriter()
{
	if (pImpl_) {
		if (pImpl_->bDelete_)
			delete pImpl_->pOutputStream_;
		free(pImpl_->pBuf_);
		delete pImpl_;
		pImpl_ = 0;
	}
}

bool qs::OutputStreamWriter::operator!() const
{
	return pImpl_ == 0;
}

bool qs::OutputStreamWriter::close()
{
	if (!pImpl_->flushBuffer())
		return false;
	return pImpl_->pOutputStream_->close();
}

size_t qs::OutputStreamWriter::write(const WCHAR* p,
									 size_t nWrite)
{
	assert(p);
	
	if (static_cast<size_t>(pImpl_->pBufEnd_ - pImpl_->pCurrent_) >= nWrite) {
		memcpy(pImpl_->pCurrent_, p, nWrite*sizeof(WCHAR));
		pImpl_->pCurrent_ += nWrite;
	}
	else {
		if (!pImpl_->flushBuffer())
			return -1;
		
		if (nWrite > static_cast<size_t>(pImpl_->pBufEnd_ - pImpl_->pCurrent_)) {
			if (!pImpl_->write(p, nWrite))
				return -1;
		}
		else {
			memcpy(pImpl_->pCurrent_, p, nWrite*sizeof(WCHAR));
			pImpl_->pCurrent_ += nWrite;
		}
	}
	
	return nWrite;
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
	
	bool allocBuffer(size_t nSize);
	
	wxstring_ptr wstr_;
	WCHAR* pEnd_;
	WCHAR* pCurrent_;
};

bool qs::StringWriterImpl::allocBuffer(size_t nSize)
{
	size_t nNewSize = 0;
	if (!wstr_.get()) {
		nNewSize = QSMAX(static_cast<size_t>(BUFFER_SIZE), nSize);
	}
	else {
		nNewSize = pEnd_ - wstr_.get();
		nNewSize += QSMAX(nNewSize, nSize);
	}
	
	size_t nLen = pCurrent_ - wstr_.get();
	if (!wstr_.get())
		wstr_ = allocWXString(nNewSize);
	else
		wstr_ = reallocWXString(wstr_, nNewSize);
	if (!wstr_.get())
		return false;
	pEnd_ = wstr_.get() + nNewSize;
	pCurrent_ = wstr_.get() + nLen;
	
	return true;
}


/****************************************************************************
 *
 * StringWriter
 *
 */

qs::StringWriter::StringWriter() :
	pImpl_(0)
{
	pImpl_ = new StringWriterImpl();
	pImpl_->pEnd_ = 0;
	pImpl_->pCurrent_ = 0;
}

qs::StringWriter::~StringWriter()
{
	delete pImpl_;
	pImpl_ = 0;
}

bool qs::StringWriter::close()
{
	return true;
}

size_t qs::StringWriter::write(const WCHAR* p, size_t nWrite)
{
	if (static_cast<size_t>(pImpl_->pEnd_ - pImpl_->pCurrent_) < nWrite) {
		if (!pImpl_->allocBuffer(nWrite))
			return -1;
	}
	memcpy(pImpl_->pCurrent_, p, nWrite*sizeof(WCHAR));
	pImpl_->pCurrent_ += nWrite;
	
	return nWrite;
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
	
	bool flushBuffer();
	
	Writer* pWriter_;
	bool bDelete_;
	auto_ptr_array<WCHAR> pBuf_;
	WCHAR* pBufEnd_;
	WCHAR* pCurrent_;
};

bool qs::BufferedWriterImpl::flushBuffer()
{
	if (pCurrent_ != pBuf_.get()) {
		if (pWriter_->write(pBuf_.get(), pCurrent_ - pBuf_.get()) == -1)
			return false;
		pCurrent_ = pBuf_.get();
	}
	return true;
}


/****************************************************************************
 *
 * BufferedWriter
 *
 */

qs::BufferedWriter::BufferedWriter(Writer* pWriter,
								   bool bDelete) :
	pImpl_(0)
{
	auto_ptr_array<WCHAR> pBuf(new WCHAR[BufferedWriterImpl::BUFFER_SIZE]);
	
	pImpl_ = new BufferedWriterImpl();
	pImpl_->pWriter_ = pWriter;
	pImpl_->bDelete_ = bDelete;
	pImpl_->pBuf_ = pBuf;
	pImpl_->pBufEnd_ = pImpl_->pBuf_.get() + BufferedWriterImpl::BUFFER_SIZE;
	pImpl_->pCurrent_ = pImpl_->pBuf_.get();
}

qs::BufferedWriter::~BufferedWriter()
{
	if (pImpl_) {
		if (pImpl_->bDelete_)
			delete pImpl_->pWriter_;
		delete pImpl_;
		pImpl_ = 0;
	}
}

bool qs::BufferedWriter::newLine()
{
	return write(L"\r\n", 2) == 2;
}

bool qs::BufferedWriter::close()
{
	if (!pImpl_->flushBuffer())
		return false;
	return pImpl_->pWriter_->close();
}

size_t qs::BufferedWriter::write(const WCHAR* p,
								 size_t nWrite)
{
	assert(p);
	
	if (static_cast<size_t>(pImpl_->pBufEnd_ - pImpl_->pCurrent_) >= nWrite) {
		memcpy(pImpl_->pCurrent_, p, nWrite*sizeof(WCHAR));
		pImpl_->pCurrent_ += nWrite;
	}
	else {
		if (!pImpl_->flushBuffer())
			return -1;
		
		if (nWrite > BufferedWriterImpl::BUFFER_SIZE/2) {
			if (pImpl_->pWriter_->write(p, nWrite) == -1)
				return -1;
		}
		else {
			memcpy(pImpl_->pCurrent_, p, nWrite*sizeof(WCHAR));
			pImpl_->pCurrent_ += nWrite;
		}
	}
	return nWrite;
}
