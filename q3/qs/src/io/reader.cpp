/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsconv.h>
#include <qsstream.h>
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
		BUFFER_SIZE	= 8192
	};
	
	bool mapBuffer();
	
	InputStream* pInputStream_;
	bool bDelete_;
	std::auto_ptr<Converter> pConverter_;
	CHAR* pBuf_;
	CHAR* pBufEnd_;
	wxstring_ptr wstr_;
	WCHAR* pwEnd_;
	WCHAR* pwCurrent_;
	bool bSkipBOM_;
};

bool qs::InputStreamReaderImpl::mapBuffer()
{
	assert(pwCurrent_ == pwEnd_);
	
	size_t nRead = pInputStream_->read(
		reinterpret_cast<unsigned char*>(pBufEnd_),
		BUFFER_SIZE - (pBufEnd_ - pBuf_));
	if (nRead == -1)
		return false;
	
	CHAR* p = pBufEnd_;
	for (size_t n = 0; n < nRead; ++n) {
		CHAR c = *(pBufEnd_ + n);
		if (c != '\r')
			*p++ = c;
	}
	pBufEnd_ = p;
	
	size_t nLen = pBufEnd_ - pBuf_;
	if (nLen != 0) {
		wxstring_size_ptr decoded(pConverter_->decode(pBuf_, &nLen));
		if (!decoded.get())
			return false;
		wstr_.reset(decoded.release());
		pwEnd_ = wstr_.get() + decoded.size();
		pwCurrent_ = wstr_.get();
		
		if (bSkipBOM_ && decoded.size() != 0) {
			if (*pwCurrent_ == 0xfeff)
				++pwCurrent_;
			bSkipBOM_ = false;
		}
		
		if (pBuf_ + nLen != pBufEnd_) {
			if (nRead == 0)
				return false;
			size_t nRest = (pBufEnd_ - pBuf_) - nLen;
			memmove(pBuf_, pBuf_ + nLen, nRest);
			pBufEnd_ = pBuf_ + nRest;
		}
		else {
			pBufEnd_ = pBuf_;
		}
	}
	else {
		wstr_.reset(0);
		pwEnd_ = 0;
		pwCurrent_ = 0;
	}
	
	return true;
}


/****************************************************************************
 *
 * InputStreamReader
 *
 */

qs::InputStreamReader::InputStreamReader(InputStream* pInputStream,
										 bool bDelete,
										 const WCHAR* pwszEncoding) :
	pImpl_(0)
{
	assert(pInputStream);
	
	malloc_ptr<CHAR> pBuf(static_cast<CHAR*>(
		malloc(InputStreamReaderImpl::BUFFER_SIZE*sizeof(CHAR))));
	if (!pBuf.get())
		return;
	
	if (!pwszEncoding)
		pwszEncoding = getSystemEncoding();
	std::auto_ptr<Converter> pConverter(ConverterFactory::getInstance(pwszEncoding));
	if (!pConverter.get())
		return;
	
	pImpl_ = new InputStreamReaderImpl();
	pImpl_->pInputStream_ = pInputStream;
	pImpl_->bDelete_ = bDelete;
	pImpl_->pConverter_ = pConverter;
	pImpl_->pBuf_ = pBuf.release();
	pImpl_->pBufEnd_ = pImpl_->pBuf_;
	pImpl_->pwEnd_ = 0;
	pImpl_->pwCurrent_ = 0;
	pImpl_->bSkipBOM_ = _wcsnicmp(pwszEncoding, L"utf-", 4) == 0 ||
		_wcsnicmp(pwszEncoding, L"ucs-", 4) == 0;
}

qs::InputStreamReader::~InputStreamReader()
{
	if (pImpl_) {
		if (pImpl_->bDelete_)
			delete pImpl_->pInputStream_;
		free(pImpl_->pBuf_);
		delete pImpl_;
		pImpl_ = 0;
	}
}

bool qs::InputStreamReader::operator!() const
{
	return pImpl_ == 0;
}

bool qs::InputStreamReader::close()
{
	return pImpl_->pInputStream_->close();
}

size_t qs::InputStreamReader::read(WCHAR* p,
								   size_t nRead)
{
	assert(p);
	
	size_t nSize = 0;
	
	while (nRead != 0) {
		if (static_cast<size_t>(pImpl_->pwEnd_ - pImpl_->pwCurrent_) >= nRead) {
			memcpy(p, pImpl_->pwCurrent_, nRead*sizeof(WCHAR));
			pImpl_->pwCurrent_ += nRead;
			nSize += nRead;
			nRead = 0;
		}
		else {
			if (pImpl_->pwEnd_ != pImpl_->pwCurrent_) {
				size_t nLen = static_cast<size_t>(pImpl_->pwEnd_ - pImpl_->pwCurrent_);
				memcpy(p, pImpl_->pwCurrent_, nLen*sizeof(WCHAR));
				p += nLen;
				pImpl_->pwCurrent_ += nLen;
				nSize += nLen;
				nRead -= nLen;
			}
			assert(nRead != 0);
			
			if (!pImpl_->mapBuffer())
				return -1;
			if (!pImpl_->wstr_.get())
				break;
		}
	}
	
	return nSize;
}


/****************************************************************************
 *
 * StringReaderImpl
 *
 */

struct qs::StringReaderImpl
{
	bool init(const WCHAR* pwsz,
			  size_t nLen,
			  bool bCopy);
	bool init(wxstring_ptr wstr,
			  size_t nLen);
	
	wxstring_ptr wstr_;
	const WCHAR* pBegin_;
	const WCHAR* pEnd_;
	const WCHAR* pCurrent_;
};

bool qs::StringReaderImpl::init(const WCHAR* pwsz,
								size_t nLen,
								bool bCopy)
{
	pBegin_ = 0;
	pEnd_ = 0;
	pCurrent_ = 0;
	
	if (nLen == static_cast<size_t>(-1))
		nLen = wcslen(pwsz);
	
	if (bCopy) {
		wxstring_ptr wstr(allocWXString(pwsz, nLen));
		if (!wstr.get())
			return false;
		return init(wstr, nLen);
	}
	else {
		pBegin_ = pwsz;
		pEnd_ = pBegin_ + nLen;
		pCurrent_ = pBegin_;
	}
	
	return true;
}

bool qs::StringReaderImpl::init(wxstring_ptr wstr,
								size_t nLen)
{
	pBegin_ = 0;
	pEnd_ = 0;
	pCurrent_ = 0;
	
	if (nLen == static_cast<size_t>(-1))
		nLen = wcslen(wstr.get());
	
	wstr_ = wstr;
	pBegin_ = wstr_.get();
	pEnd_ = pBegin_ + nLen;
	pCurrent_ = pBegin_;
	
	return true;
}


/****************************************************************************
 *
 * StringReader
 *
 */

qs::StringReader::StringReader(const WCHAR* pwsz,
							   bool bCopy) :
	pImpl_(0)
{
	std::auto_ptr<StringReaderImpl> pImpl(new StringReaderImpl());
	if (!pImpl->init(pwsz, -1, bCopy))
		return;
	pImpl_ = pImpl.release();
}

qs::StringReader::StringReader(const WCHAR* pwsz,
							   size_t nLen,
							   bool bCopy) :
	pImpl_(0)
{
	std::auto_ptr<StringReaderImpl> pImpl(new StringReaderImpl());
	if (!pImpl->init(pwsz, nLen, bCopy))
		return;
	pImpl_ = pImpl.release();
}

qs::StringReader::StringReader(wxstring_ptr wstr) :
	pImpl_(0)
{
	std::auto_ptr<StringReaderImpl> pImpl(new StringReaderImpl());
	if (!pImpl->init(wstr, -1))
		return;
	pImpl_ = pImpl.release();
}

qs::StringReader::StringReader(wxstring_ptr wstr,
							   size_t nLen) :
	pImpl_(0)
{
	std::auto_ptr<StringReaderImpl> pImpl(new StringReaderImpl());
	if (!pImpl->init(wstr, nLen))
		return;
	pImpl_ = pImpl.release();
}

qs::StringReader::~StringReader()
{
	delete pImpl_;
	pImpl_ = 0;
}

bool qs::StringReader::operator!() const
{
	return pImpl_ == 0;
}

bool qs::StringReader::close()
{
	return true;
}

size_t qs::StringReader::read(WCHAR* p,
							  size_t nRead)
{
	assert(p);
	
	size_t nSize = 0;
	
	if (nRead > static_cast<size_t>(pImpl_->pEnd_ - pImpl_->pCurrent_))
		nRead = pImpl_->pEnd_ - pImpl_->pCurrent_;
	if (nRead != 0) {
		memcpy(p, pImpl_->pCurrent_, nRead*sizeof(WCHAR));
		nSize = nRead;
		pImpl_->pCurrent_ += nRead;
	}
	
	return nSize;
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
	
	bool mapBuffer();
	
	Reader* pReader_;
	bool bDelete_;
	auto_ptr_array<WCHAR> pBuf_;
	WCHAR* pBufEnd_;
	WCHAR* pCurrent_;
};

bool qs::BufferedReaderImpl::mapBuffer()
{
	assert(pCurrent_ == pBufEnd_);
	
	size_t nRead = pReader_->read(pBuf_.get(), BUFFER_SIZE);
	if (nRead == -1)
		return false;
	
	pBufEnd_ = pBuf_.get() + nRead;
	pCurrent_ = pBuf_.get();
	
	return true;
}


/****************************************************************************
 *
 * BufferedReader
 *
 */

qs::BufferedReader::BufferedReader(Reader* pReader,
								   bool bDelete) :
	pImpl_(0)
{
	auto_ptr_array<WCHAR> pBuf(new WCHAR[BufferedReaderImpl::BUFFER_SIZE]);
	
	pImpl_ = new BufferedReaderImpl();
	pImpl_->pReader_ = pReader;
	pImpl_->bDelete_ = bDelete;
	pImpl_->pBuf_ = pBuf;
	pImpl_->pBufEnd_ = pImpl_->pBuf_.get();
	pImpl_->pCurrent_ = pImpl_->pBuf_.get();
}

qs::BufferedReader::~BufferedReader()
{
	if (pImpl_) {
		if (pImpl_->bDelete_)
			delete pImpl_->pReader_;
		delete pImpl_;
		pImpl_ = 0;
	}
}

bool qs::BufferedReader::readLine(wxstring_ptr* pwstr)
{
	wxstring_ptr wstr;
	size_t nLen = 0;
	while (true) {
		WCHAR* p = pImpl_->pCurrent_;
		if (pImpl_->pCurrent_ != pImpl_->pBufEnd_) {
			while (p < pImpl_->pBufEnd_ && *p != L'\r' && *p != L'\n')
				++p;
			
			size_t nNewLen = nLen + (p - pImpl_->pCurrent_);
			if (!wstr.get())
				wstr = allocWXString(pImpl_->pCurrent_, nNewLen);
			else
				wstr = reallocWXString(wstr, nNewLen);
			if (!wstr.get())
				return false;
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
					if (!pImpl_->mapBuffer())
						return false;
					
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
			
			if (!pImpl_->mapBuffer())
				return false;
			
			if (pImpl_->pBuf_.get() == pImpl_->pBufEnd_)
				break;
		}
	}
	
	*pwstr = wstr;
	
	return true;
}

bool qs::BufferedReader::close()
{
	return pImpl_->pReader_->close();
}

size_t qs::BufferedReader::read(WCHAR* p,
								size_t nRead)
{
	assert(p);
	
	size_t nSize = 0;
	
	if (static_cast<size_t>(pImpl_->pBufEnd_ - pImpl_->pCurrent_) >= nRead) {
		memcpy(p, pImpl_->pCurrent_, nRead*sizeof(WCHAR));
		nSize = nRead;
		pImpl_->pCurrent_ += nRead;
	}
	else {
		if (pImpl_->pCurrent_ != pImpl_->pBufEnd_) {
			size_t n = pImpl_->pBufEnd_ - pImpl_->pCurrent_;
			memcpy(p, pImpl_->pCurrent_, n*sizeof(WCHAR));
			nRead -= n;
			pImpl_->pCurrent_ = pImpl_->pBufEnd_;
			nSize = n;
			p += n;
		}
		
		if (nRead > BufferedReaderImpl::BUFFER_SIZE/2) {
			size_t n = pImpl_->pReader_->read(p, nRead);
			if (n == -1)
				return -1;
			nSize += n;
		}
		else {
			if (!pImpl_->mapBuffer())
				return -1;
			
			if (static_cast<size_t>(pImpl_->pBufEnd_ - pImpl_->pCurrent_) < nRead)
				nRead = pImpl_->pBufEnd_ - pImpl_->pCurrent_;
			if (nRead != 0) {
				memcpy(p, pImpl_->pCurrent_, nRead*sizeof(WCHAR));
				pImpl_->pCurrent_ += nRead;
				nSize += nRead;
			}
		}
	}
	
	return nSize;
}
