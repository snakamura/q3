/*
 * $Id: reader.cpp,v 1.1.1.1 2003/04/29 08:07:35 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsstream.h>
#include <qserror.h>
#include <qsconv.h>
#include <qsnew.h>
#include <qsstring.h>

using namespace qs;


/****************************************************************************
 *
 * Reader
 *
 */
qs::Reader::~Reader()
{
}


/****************************************************************************
 *
 * InputStreamReaderImpl
 *
 */

struct qs::InputStreamReaderImpl
{
	enum {
		BUFFER_SIZE	= 128
	};
	
	QSTATUS mapBuffer();
	
	InputStream* pInputStream_;
	bool bDelete_;
	Converter* pConverter_;
	CHAR* pBuf_;
	CHAR* pBufEnd_;
	WSTRING wstr_;
	WCHAR* pwEnd_;
	WCHAR* pwCurrent_;
};

QSTATUS qs::InputStreamReaderImpl::mapBuffer()
{
	assert(pwCurrent_ == pwEnd_);
	
	DECLARE_QSTATUS();
	
	size_t nRead = 0;
	status = pInputStream_->read(reinterpret_cast<unsigned char*>(pBufEnd_),
		BUFFER_SIZE - (pBufEnd_ - pBuf_), &nRead);
	CHECK_QSTATUS();
	
	if (nRead != -1) {
		CHAR* p = pBufEnd_;
		for (size_t n = 0; n < nRead; ++n) {
			CHAR c = *(pBufEnd_ + n);
			if (c != '\r')
				*p++ = c;
		}
		pBufEnd_ = p;
	}
	else {
		pBufEnd_ = pBuf_;
	}
	
	freeWString(wstr_);
	
	size_t nLen = pBufEnd_ - pBuf_;
	if (nLen != 0) {
		size_t nDecodeLen = 0;
		status = pConverter_->decode(pBuf_, &nLen, &wstr_, &nDecodeLen);
		CHECK_QSTATUS();
		pwEnd_ = wstr_ + nDecodeLen;
		pwCurrent_ = wstr_;
		
		if (pBuf_ + nLen != pBufEnd_) {
			if (nRead == -1)
				return QSTATUS_FAIL;
			size_t nRest = (pBufEnd_ - pBuf_) - nLen;
			memmove(pBuf_, pBuf_ + nLen, nRest);
			pBufEnd_ = pBuf_ + nRest;
		}
		else {
			pBufEnd_ = pBuf_;
		}
	}
	else {
		wstr_ = 0;
		pwEnd_ = 0;
		pwCurrent_ = 0;
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * InputStreamReader
 *
 */

qs::InputStreamReader::InputStreamReader(InputStream* pInputStream,
	bool bDelete, const WCHAR* pwszEncoding, QSTATUS* pstatus) :
	pImpl_(0)
{
	assert(pInputStream);
	assert(pstatus);
	
	*pstatus = QSTATUS_SUCCESS;
	
	DECLARE_QSTATUS();
	
	malloc_ptr<CHAR> pBuf(static_cast<CHAR*>(
		malloc(InputStreamReaderImpl::BUFFER_SIZE*sizeof(CHAR))));
	if (!pBuf.get()) {
		*pstatus = QSTATUS_OUTOFMEMORY;
		return;
	}
	
	std::auto_ptr<Converter> pConverter;
	if (!pwszEncoding)
		pwszEncoding = getSystemEncoding();
	status = ConverterFactory::getInstance(pwszEncoding, &pConverter);
	CHECK_QSTATUS_SET(pstatus);
	if (!pConverter.get()) {
		*pstatus = QSTATUS_FAIL;
		return;
	}
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	pImpl_->pInputStream_ = pInputStream;
	pImpl_->bDelete_ = bDelete;
	pImpl_->pConverter_ = pConverter.release();
	pImpl_->pBuf_ = pBuf.release();
	pImpl_->pBufEnd_ = pImpl_->pBuf_;
	pImpl_->wstr_ = 0;
	pImpl_->pwEnd_ = 0;
	pImpl_->pwCurrent_ = 0;
}

qs::InputStreamReader::~InputStreamReader()
{
	if (pImpl_) {
		if (pImpl_->bDelete_)
			delete pImpl_->pInputStream_;
		delete pImpl_->pConverter_;
		free(pImpl_->pBuf_);
		freeWString(pImpl_->wstr_);
		delete pImpl_;
		pImpl_ = 0;
	}
}

QSTATUS qs::InputStreamReader::close()
{
	return pImpl_->pInputStream_->close();
}

QSTATUS qs::InputStreamReader::read(WCHAR* p, size_t nRead, size_t* pnRead)
{
	assert(p);
	assert(pnRead);
	
	*pnRead = 0;
	
	DECLARE_QSTATUS();
	
	while (nRead != 0) {
		if (static_cast<size_t>(pImpl_->pwEnd_ - pImpl_->pwCurrent_) >= nRead) {
			memcpy(p, pImpl_->pwCurrent_, nRead*sizeof(WCHAR));
			pImpl_->pwCurrent_ += nRead;
			*pnRead += nRead;
			nRead = 0;
		}
		else {
			if (pImpl_->pwEnd_ != pImpl_->pwCurrent_) {
				size_t nLen = static_cast<size_t>(pImpl_->pwEnd_ - pImpl_->pwCurrent_);
				memcpy(p, pImpl_->pwCurrent_, nLen*sizeof(WCHAR));
				p += nLen;
				pImpl_->pwCurrent_ += nLen;
				*pnRead += nLen;
				nRead -= nLen;
			}
			assert(nRead != 0);
			
			status = pImpl_->mapBuffer();
			CHECK_QSTATUS();
			if (!pImpl_->wstr_) {
				if (*pnRead == 0)
					*pnRead = -1;
				break;
			}
		}
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * StringReaderImpl
 *
 */

struct qs::StringReaderImpl
{
	QSTATUS init(const WCHAR* pwsz, size_t nLen);
	
	WSTRING wstr_;
	const WCHAR* pEnd_;
	const WCHAR* pCurrent_;
};

QSTATUS qs::StringReaderImpl::init(const WCHAR* pwsz, size_t nLen)
{
	wstr_ = 0;
	pEnd_ = 0;
	pCurrent_ = 0;
	
	if (nLen == static_cast<size_t>(-1))
		nLen = wcslen(pwsz);
	string_ptr<WSTRING> wstr(allocWString(pwsz, nLen));
	if (!wstr.get())
		return QSTATUS_OUTOFMEMORY;
	
	wstr_ = wstr.release();
	pEnd_ = wstr_ + nLen;
	pCurrent_ = wstr_;
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * StringReader
 *
 */

qs::StringReader::StringReader(const WCHAR* pwsz, QSTATUS* pstatus) :
	pImpl_(0)
{
	assert(pstatus);
	
	*pstatus = QSTATUS_SUCCESS;
	
	DECLARE_QSTATUS();
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	status = pImpl_->init(pwsz, static_cast<size_t>(-1));
	CHECK_QSTATUS_SET(pstatus);
}

qs::StringReader::StringReader(const WCHAR* pwsz, size_t nLen, QSTATUS* pstatus)
{
	assert(pstatus);
	
	DECLARE_QSTATUS();
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	status = pImpl_->init(pwsz, nLen);
	CHECK_QSTATUS_SET(pstatus);
}

qs::StringReader::~StringReader()
{
	if (pImpl_) {
		freeWString(pImpl_->wstr_);
		delete pImpl_;
		pImpl_ = 0;
	}
}

QSTATUS qs::StringReader::close()
{
	return QSTATUS_SUCCESS;
}

QSTATUS qs::StringReader::read(WCHAR* p, size_t nRead, size_t* pnRead)
{
	assert(p);
	assert(pnRead);
	
	if (nRead > static_cast<size_t>(pImpl_->pEnd_ - pImpl_->pCurrent_))
		nRead = pImpl_->pEnd_ - pImpl_->pCurrent_;
	if (nRead != 0) {
		memcpy(p, pImpl_->pCurrent_, nRead*sizeof(WCHAR));
		*pnRead = nRead;
		pImpl_->pCurrent_ += nRead;
	}
	else {
		*pnRead = -1;
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * BufferedReaderImpl
 *
 */

struct qs::BufferedReaderImpl
{
	enum {
		BUFFER_SIZE	= 4096
	};
	
	QSTATUS mapBuffer();
	
	Reader* pReader_;
	bool bDelete_;
	WCHAR* pBuf_;
	WCHAR* pBufEnd_;
	WCHAR* pCurrent_;
};

QSTATUS qs::BufferedReaderImpl::mapBuffer()
{
	assert(pCurrent_ == pBufEnd_);
	
	DECLARE_QSTATUS();
	
	size_t nRead = 0;
	status = pReader_->read(pBuf_, BUFFER_SIZE, &nRead);
	CHECK_QSTATUS();
	
	if (nRead != -1)
		pBufEnd_ = pBuf_ + nRead;
	else
		pBufEnd_ = pBuf_;
	pCurrent_ = pBuf_;
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * BufferedReader
 *
 */

qs::BufferedReader::BufferedReader(Reader* pReader,
	bool bDelete, QSTATUS* pstatus) :
	pImpl_(0)
{
	assert(pstatus);
	
	*pstatus = QSTATUS_SUCCESS;
	
	DECLARE_QSTATUS();
	
	malloc_ptr<WCHAR> pBuf(static_cast<WCHAR*>(
		malloc(BufferedReaderImpl::BUFFER_SIZE*sizeof(WCHAR))));
	if (!pBuf.get()) {
		*pstatus = QSTATUS_OUTOFMEMORY;
		return;
	}
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	pImpl_->pReader_ = pReader;
	pImpl_->bDelete_ = bDelete;
	pImpl_->pBuf_ = pBuf.release();
	pImpl_->pBufEnd_ = pImpl_->pBuf_;
	pImpl_->pCurrent_ = pImpl_->pBuf_;
}

qs::BufferedReader::~BufferedReader()
{
	if (pImpl_) {
		if (pImpl_->bDelete_)
			delete pImpl_->pReader_;
		free(pImpl_->pBuf_);
		delete pImpl_;
		pImpl_ = 0;
	}
}

QSTATUS qs::BufferedReader::readLine(WSTRING* pwstr)
{
	assert(pwstr);
	
	*pwstr = 0;
	
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstr;
	size_t nLen = 0;
	while (true) {
		WCHAR* p = pImpl_->pCurrent_;
		if (pImpl_->pCurrent_ != pImpl_->pBufEnd_) {
			while (p < pImpl_->pBufEnd_ && *p != L'\r' && *p != L'\n')
				++p;
			
			size_t nNewLen = nLen + (p - pImpl_->pCurrent_);
			if (!wstr.get())
				wstr.reset(allocWString(pImpl_->pCurrent_, nNewLen));
			else
				wstr.reset(reallocWString(wstr.release(), nNewLen));
			if (!wstr.get())
				return QSTATUS_OUTOFMEMORY;
			if (nLen != 0) {
				wcsncpy(wstr.get() + nLen, pImpl_->pCurrent_, nNewLen - nLen);
				*(wstr.get() + nNewLen) = L'\0';
			}
			nLen = nNewLen;
		}
		
		if (p != pImpl_->pBufEnd_) {
			pImpl_->pCurrent_ = p + 1;
			if (*p == L'\r') {
				if (pImpl_->pCurrent_ == pImpl_->pBufEnd_) {
					status = pImpl_->mapBuffer();
					CHECK_QSTATUS();
					
					if (pImpl_->pCurrent_ != pImpl_->pBufEnd_ &&
						*pImpl_->pCurrent_ == L'\n')
						++pImpl_->pCurrent_;
				}
				else {
					if (*pImpl_->pCurrent_ == L'\n')
						++pImpl_->pCurrent_;
				}
			}
			break;
		}
		else {
			pImpl_->pCurrent_ = pImpl_->pBufEnd_;
			
			status = pImpl_->mapBuffer();
			CHECK_QSTATUS();
			
			if (pImpl_->pBuf_ == pImpl_->pBufEnd_)
				break;
		}
	}
	
	*pwstr = wstr.release();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::BufferedReader::close()
{
	return pImpl_->pReader_->close();
}

QSTATUS qs::BufferedReader::read(WCHAR* p, size_t nRead, size_t* pnRead)
{
	assert(p);
	assert(pnRead);
	
	*pnRead = 0;
	
	DECLARE_QSTATUS();
	
	if (static_cast<size_t>(pImpl_->pBufEnd_ - pImpl_->pCurrent_) >= nRead) {
		memcpy(p, pImpl_->pCurrent_, nRead*sizeof(WCHAR));
		*pnRead = nRead;
		pImpl_->pCurrent_ += nRead;
	}
	else {
		if (pImpl_->pCurrent_ != pImpl_->pBufEnd_) {
			size_t n = pImpl_->pBufEnd_ - pImpl_->pCurrent_;
			memcpy(p, pImpl_->pCurrent_, n*sizeof(WCHAR));
			nRead -= n;
			pImpl_->pCurrent_ = pImpl_->pBufEnd_;
			*pnRead = n;
			p += n;
		}
		
		if (nRead > BufferedReaderImpl::BUFFER_SIZE/2) {
		}
		else {
			status = pImpl_->mapBuffer();
			CHECK_QSTATUS();
			
			if (static_cast<size_t>(pImpl_->pBufEnd_ - pImpl_->pCurrent_) < nRead)
				nRead = pImpl_->pBufEnd_ - pImpl_->pCurrent_;
			if (nRead != 0) {
				memcpy(p, pImpl_->pCurrent_, nRead*sizeof(WCHAR));
				pImpl_->pCurrent_ += nRead;
				*pnRead += nRead;
			}
			else {
				if (*pnRead == 0)
					*pnRead = -1;
			}
		}
	}
	
	return QSTATUS_SUCCESS;
}
