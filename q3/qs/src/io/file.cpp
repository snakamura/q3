/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsconv.h>
#include <qsfile.h>
#include <qsosutil.h>
#include <qsstring.h>

#include <algorithm>

#include <windows.h>
#include <tchar.h>

using namespace qs;

#if defined _WIN32_WCE && _WIN32_WCE <= 300
namespace {
const DWORD INVALID_SET_FILE_POINTER = 0xffffffff;
}
#endif


/****************************************************************************
 *
 * File
 *
 */

qs::File::~File()
{
}

wstring_ptr qs::File::getTempFileName(const WCHAR* pwszDir)
{
	assert(pwszDir);
	
	wstring_ptr wstrPath(allocWString(wcslen(pwszDir) + 33));
	
	WCHAR* pwszPath = wstrPath.get();
	wcscpy(pwszPath, pwszDir);
	WCHAR* p = pwszPath + wcslen(pwszDir);
	if (*(p - 1) != L'\\') {
		*p++ = L'\\';
		*p = L'\0';
	}
	
	for (int n = 0; ; ++n) {
		::_snwprintf(p, 32, L"tmp%x", n);
		W2T(pwszPath, ptszPath);
		if (::GetFileAttributes(ptszPath) == 0xffffffff)
			break;
	}
	
	return wstrPath;
}

bool qs::File::removeDirectory(const WCHAR* pwszDir)
{
	assert(pwszDir);
	assert(*(pwszDir + wcslen(pwszDir) - 1) != L'\\');
	
	wstring_ptr wstrPathBase(concat(pwszDir, L"\\*.*"));
	tstring_ptr tstrPathBase(wcs2tcs(wstrPathBase.get()));
	
	WIN32_FIND_DATA fd;
	AutoFindHandle hFind(::FindFirstFile(tstrPathBase.get(), &fd));
	
	size_t nLen = _tcslen(tstrPathBase.get()) - 3;
	assert(_tcscmp(tstrPathBase.get() + nLen - 1, _T("\\*.*")) == 0);
	*(tstrPathBase.get() + nLen) = _T('\0');
	
	if (hFind.get()) {
		tstring_ptr tstrPath(allocTString(nLen + MAX_PATH + 10));
		_tcscpy(tstrPath.get(), tstrPathBase.get());
		TCHAR* pFileName = tstrPath.get() + nLen;
		do {
			if (_tcscmp(fd.cFileName, _T(".")) == 0 ||
				_tcscmp(fd.cFileName, _T("..")) == 0)
				continue;
			if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				T2W(fd.cFileName, pwszFileName);
				size_t nLen = wcslen(wstrPathBase.get()) - 3;
				wstring_ptr wstrPath(allocWString(
					nLen + wcslen(pwszFileName) + 10));
				wcsncpy(wstrPath.get(), wstrPathBase.get(), nLen);
				wcscpy(wstrPath.get() + nLen, pwszFileName);
				if (!File::removeDirectory(wstrPath.get()))
					return false;
			}
			else {
				_tcscpy(pFileName, fd.cFileName);
				if (!::DeleteFile(tstrPath.get()))
					return false;
			}
		} while (::FindNextFile(hFind.get(), &fd));
	}
	hFind.close();
	
	assert(*(tstrPathBase.get() + nLen - 1) == _T('\\'));
	*(tstrPathBase.get() + nLen - 1) = _T('\0');
	
	return ::RemoveDirectory(tstrPathBase.get()) != 0;
}


/****************************************************************************
 *
 * BinaryFileImpl
 *
 */

struct qs::BinaryFileImpl
{
	enum {
		BUFFER_SIZE	= 256
	};
	
	bool open(const WCHAR* pwszPath,
			  unsigned int nMode);
	bool close();
	bool mapBuffer();
	bool flushBuffer();
	
	HANDLE hFile_;
	DWORD dwPosition_;
	bool bWritten_;
	auto_ptr_array<unsigned char> pBuf_;
	unsigned char* pBufEnd_;
	unsigned char* pCurrent_;
};

bool qs::BinaryFileImpl::open(const WCHAR* pwszPath,
							  unsigned int nMode)
{
	assert(pwszPath);
	assert(!hFile_);
	assert(dwPosition_ == 0);
	assert(!bWritten_);
	assert(pBuf_.get());
	assert(pBufEnd_ == pBuf_.get());
	assert(pCurrent_ == pBuf_.get());
	
	assert(nMode & BinaryFile::MODE_READ || nMode & BinaryFile::MODE_WRITE);
	
	DWORD dwMode = 0;
	DWORD dwDescription = 0;
	DWORD dwShare = FILE_SHARE_READ;
	
	if (nMode & BinaryFile::MODE_CREATE)
		dwDescription = CREATE_ALWAYS;
	
	if (nMode & BinaryFile::MODE_WRITE) {
		dwMode |= GENERIC_WRITE;
		if (dwDescription == 0)
			dwDescription = OPEN_ALWAYS;
		dwShare = 0;
	}
	if (nMode & BinaryFile::MODE_READ) {
		dwMode |= GENERIC_READ;
		if (dwDescription == 0)
			dwDescription = OPEN_EXISTING;
	}
	
	W2T(pwszPath, ptszPath);
	AutoHandle hFile(::CreateFile(ptszPath, dwMode, dwShare,
		0, dwDescription, FILE_ATTRIBUTE_NORMAL, 0));
	if (!hFile.get())
		return false;
	hFile_ = hFile.release();
	
	return true;
}

bool qs::BinaryFileImpl::close()
{
	if (hFile_) {
		if (!flushBuffer())
			return false;
		if (!::CloseHandle(hFile_))
			return false;
		hFile_ = 0;
	}
	return true;
}

bool qs::BinaryFileImpl::mapBuffer()
{
	if (!flushBuffer())
		return false;
	assert(pBufEnd_ == pBuf_.get());
	assert(pCurrent_ = pBuf_.get());
	
	while (pBufEnd_ != pBuf_.get() + BUFFER_SIZE) {
		DWORD dwRead = 0;
		if (!::ReadFile(hFile_, pBufEnd_,
			pBuf_.get() + BUFFER_SIZE - pBufEnd_, &dwRead, 0))
			return false;
		if (dwRead == 0)
			break;
		pBufEnd_ += dwRead;
	}
	
	return true;
}

bool qs::BinaryFileImpl::flushBuffer()
{
	if (bWritten_) {
		assert(pCurrent_ != pBuf_.get());
		assert(pBufEnd_ != pBuf_.get());
		
		if (::SetFilePointer(hFile_, dwPosition_, 0, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
			return false;
		
		unsigned char* p = pBuf_.get();
		while (p != pBufEnd_) {
			DWORD dwWritten = 0;
			if (!::WriteFile(hFile_, p, pBufEnd_ - p, &dwWritten, 0))
				return false;
			p += dwWritten;
		}
		bWritten_ = false;
	}
	
	DWORD dwNewPos = ::SetFilePointer(hFile_,
		dwPosition_ + (pCurrent_ - pBuf_.get()), 0, FILE_BEGIN);
	if (dwNewPos == INVALID_SET_FILE_POINTER)
		return false;
	dwPosition_ = dwNewPos;
	
	pCurrent_ = pBuf_.get();
	pBufEnd_ = pBuf_.get();
	
	return true;
}


/****************************************************************************
 *
 * BinaryFile
 *
 */

qs::BinaryFile::BinaryFile(const WCHAR* pwszPath,
						   unsigned int nMode,
						   size_t nBufferSize)
{
	assert(pwszPath);
	
	if (nBufferSize == 0)
		nBufferSize = BinaryFileImpl::BUFFER_SIZE;
	
	auto_ptr_array<unsigned char> pBuf(new unsigned char[nBufferSize]);
	
	std::auto_ptr<BinaryFileImpl> pImpl(new BinaryFileImpl());
	pImpl->hFile_ = 0;
	pImpl->dwPosition_ = 0;
	pImpl->bWritten_ = false;
	pImpl->pBuf_ = pBuf;
	pImpl->pBufEnd_ = pImpl->pBuf_.get();
	pImpl->pCurrent_ = pImpl->pBuf_.get();
	
	if (!pImpl->open(pwszPath, nMode))
		return;
	
	pImpl_ = pImpl.release();
}

qs::BinaryFile::~BinaryFile()
{
	if (pImpl_) {
		pImpl_->close();
		delete pImpl_;
		pImpl_ = 0;
	}
}

bool qs::BinaryFile::operator!() const
{
	return pImpl_ == 0;
}

bool qs::BinaryFile::close()
{
	return pImpl_->close();
}

size_t qs::BinaryFile::read(unsigned char* p,
							size_t nRead)
{
	assert(p);
	
	size_t nSize = 0;
	
	if (static_cast<size_t>(pImpl_->pBufEnd_ - pImpl_->pCurrent_) >= nRead) {
		memcpy(p, pImpl_->pCurrent_, nRead);
		pImpl_->pCurrent_ += nRead;
		nSize = nRead;
	}
	else {
		if (pImpl_->pCurrent_ != pImpl_->pBufEnd_) {
			size_t n = pImpl_->pBufEnd_ - pImpl_->pCurrent_;
			memcpy(p, pImpl_->pCurrent_, n);
			p += n;
			nRead -= n;
			nSize = n;
			pImpl_->pCurrent_ = pImpl_->pBufEnd_;
		}
		
		if (nRead > BinaryFileImpl::BUFFER_SIZE/2) {
			if (!pImpl_->flushBuffer())
				return -1;
			assert(pImpl_->pBufEnd_ == pImpl_->pBuf_.get());
			assert(pImpl_->pCurrent_ == pImpl_->pBuf_.get());
			
			while (nRead != 0) {
				DWORD dwRead = 0;
				if (!::ReadFile(pImpl_->hFile_, p, nRead, &dwRead, 0))
					return -1;
				if (dwRead == 0)
					break;
				nSize += dwRead;
				p += dwRead;
				nRead -= dwRead;
				pImpl_->dwPosition_ += dwRead;
			}
		}
		else {
			if (!pImpl_->mapBuffer())
				return -1;
			if (static_cast<size_t>(pImpl_->pBufEnd_ - pImpl_->pCurrent_) < nRead)
				nRead = pImpl_->pBufEnd_ - pImpl_->pCurrent_;
			if (nRead != 0) {
				memcpy(p, pImpl_->pCurrent_, nRead);
				pImpl_->pCurrent_ += nRead;
				nSize += nRead;
			}
		}
	}
	
	return nSize;
}

size_t qs::BinaryFile::write(const unsigned char* p,
							 size_t nWrite)
{
	assert(p);
	
	if (static_cast<size_t>(pImpl_->pBufEnd_ - pImpl_->pCurrent_) >= nWrite) {
		memcpy(pImpl_->pCurrent_, p, nWrite);
		pImpl_->pCurrent_ += nWrite;
		pImpl_->bWritten_ = true;
	}
	else {
		if (pImpl_->pCurrent_ != pImpl_->pBufEnd_) {
			size_t n = pImpl_->pBufEnd_ - pImpl_->pCurrent_;
			memcpy(pImpl_->pCurrent_, p, n);
			p += n;
			nWrite -= n;
			pImpl_->pCurrent_ = pImpl_->pBufEnd_;
			pImpl_->bWritten_ = true;
		}
		
		if (nWrite > BinaryFileImpl::BUFFER_SIZE/2) {
			if (!pImpl_->flushBuffer())
				return -1;
			assert(pImpl_->pBufEnd_ == pImpl_->pBuf_.get());
			assert(pImpl_->pCurrent_ == pImpl_->pBuf_.get());
			
			size_t n = nWrite;
			while (n != 0) {
				DWORD dwWritten = 0;
				if (!::WriteFile(pImpl_->hFile_, p, n, &dwWritten, 0))
					return -1;
				p += dwWritten;
				n -= dwWritten;
				pImpl_->dwPosition_ += dwWritten;
			}
		}
		else {
			if (!pImpl_->mapBuffer())
				return -1;
			memcpy(pImpl_->pCurrent_, p, nWrite);
			pImpl_->pCurrent_ += nWrite;
			if (pImpl_->pCurrent_ > pImpl_->pBufEnd_)
				pImpl_->pBufEnd_ = pImpl_->pCurrent_;
			pImpl_->bWritten_ = true;
		}
	}
	
	return nWrite;
}

bool qs::BinaryFile::flush()
{
	if (!pImpl_->flushBuffer())
		return false;
	
	return ::FlushFileBuffers(pImpl_->hFile_) != 0;
}

int qs::BinaryFile::getPosition()
{
	return pImpl_->dwPosition_ + (pImpl_->pCurrent_ - pImpl_->pBuf_.get());
}

int qs::BinaryFile::setPosition(int nPosition,
								SeekOrigin seekOrigin)
{
	if (pImpl_->pBuf_.get() != pImpl_->pBufEnd_) {
		DWORD dwNewPos = 0;
		switch (seekOrigin) {
		case SEEKORIGIN_BEGIN:
			dwNewPos = nPosition;
			break;
		case SEEKORIGIN_END:
			{
				size_t nSize = getSize();
				if (nSize == -1)
					return -1;
				dwNewPos = nSize + nPosition;
			}
			break;
		case SEEKORIGIN_CURRENT:
			dwNewPos = pImpl_->dwPosition_ +
				(pImpl_->pCurrent_ - pImpl_->pBuf_.get()) + nPosition;
			break;
		default:
			assert(false);
			return -1;
		}
		if (pImpl_->dwPosition_ <= dwNewPos &&
			dwNewPos <= pImpl_->dwPosition_ + (pImpl_->pBufEnd_ - pImpl_->pBuf_.get())) {
			pImpl_->pCurrent_ = pImpl_->pBuf_.get() + (dwNewPos - pImpl_->dwPosition_);
			return getPosition();
		}
	}
	
	DWORD dwMethod = seekOrigin == SEEKORIGIN_BEGIN ? FILE_BEGIN :
		seekOrigin == SEEKORIGIN_END ? FILE_END : FILE_CURRENT;
	if (!pImpl_->flushBuffer())
		return -1;
	assert(pImpl_->pBufEnd_ == pImpl_->pBuf_.get());
	assert(pImpl_->pCurrent_ == pImpl_->pBuf_.get());
	DWORD dwNewPos = ::SetFilePointer(pImpl_->hFile_, nPosition, 0, dwMethod);
	if (dwNewPos == INVALID_SET_FILE_POINTER)
		return -1;
	pImpl_->dwPosition_ = dwNewPos;
	
	return getPosition();
}

bool qs::BinaryFile::setEndOfFile()
{
	int nPosition = getPosition();
	
	if (!pImpl_->flushBuffer())
		return false;
	assert(pImpl_->pBufEnd_ == pImpl_->pBuf_.get());
	assert(pImpl_->pCurrent_ == pImpl_->pBuf_.get());
	
	DWORD dwNewPos = ::SetFilePointer(pImpl_->hFile_, nPosition, 0, FILE_BEGIN);
	if (dwNewPos == INVALID_SET_FILE_POINTER)
		return false;
	pImpl_->dwPosition_ = dwNewPos;
	
	return ::SetEndOfFile(pImpl_->hFile_) != 0;
}

size_t qs::BinaryFile::getSize()
{
	DWORD dwSize = ::GetFileSize(pImpl_->hFile_, 0);
	if (dwSize == -1)
		return -1;
	return QSMAX(dwSize, pImpl_->dwPosition_ + (pImpl_->pCurrent_ - pImpl_->pBuf_.get()));
}


/****************************************************************************
 *
 * DividedFileImpl
 *
 */

struct qs::DividedFileImpl
{
	typedef std::vector<BinaryFile*> FileList;
	
	BinaryFile* getFile(unsigned int n);
	wstring_ptr getPath(unsigned int n) const;
	
	wstring_ptr wstrPath_;
	size_t nBlockSize_;
	unsigned int nMode_;
	size_t nBufferSize_;
	FileList listFile_;
	int nPosition_;
};

BinaryFile* qs::DividedFileImpl::getFile(unsigned int n)
{
	if (listFile_.size() <= n)
		listFile_.resize(n + 1);
	
	BinaryFile*& pFile = listFile_[n];
	if (!pFile) {
		wstring_ptr wstrPath(getPath(n));
		std::auto_ptr<BinaryFile> p(new BinaryFile(
			wstrPath.get(), nMode_, nBufferSize_));
		if (!*p)
			return 0;
		pFile = p.release();
	}
	
	return pFile;
}

wstring_ptr qs::DividedFileImpl::getPath(unsigned int n) const
{
	const WCHAR* pFileName = wcsrchr(wstrPath_.get(), L'\\');
	if (!pFileName)
		pFileName = wstrPath_.get();
	const WCHAR* pExt = wcschr(pFileName, L'.');
	if (!pExt)
		pExt = pFileName + wcslen(pFileName);
	
	WCHAR wsz[16];
	swprintf(wsz, L"%03u", n);
	
	return concat(wstrPath_.get(), pExt - wstrPath_.get(), wsz, -1, pExt, -1);
}


/****************************************************************************
 *
 * DividedFile
 *
 */

qs::DividedFile::DividedFile(const WCHAR* pwszPath,
							 size_t nBlockSize,
							 unsigned int nMode,
							 size_t nBufferSize) :
	pImpl_(0)
{
	wstring_ptr wstrPath(allocWString(pwszPath));
	
	pImpl_ = new DividedFileImpl();
	pImpl_->wstrPath_ = wstrPath;
	pImpl_->nBlockSize_ = nBlockSize;
	pImpl_->nMode_ = nMode;
	pImpl_->nBufferSize_ = nBufferSize;
	pImpl_->nPosition_ = 0;
}

qs::DividedFile::~DividedFile()
{
	if (pImpl_) {
		close();
		delete pImpl_;
		pImpl_ = 0;
	}
}

bool qs::DividedFile::close()
{
	bool bFail = false;
	DividedFileImpl::FileList::iterator it = pImpl_->listFile_.begin();
	while (it != pImpl_->listFile_.end()) {
		BinaryFile* pFile = *it;
		if (pFile) {
			if (!pFile->close())
				bFail = true;
			delete pFile;
			*it = 0;
		}
		++it;
	}
	
	return !bFail;
}

size_t qs::DividedFile::read(unsigned char* p,
							 size_t nRead)
{
	assert(p);
	
	if (nRead == 0)
		return 0;
	
	unsigned int nStart = pImpl_->nPosition_/pImpl_->nBlockSize_;
	unsigned int nEnd = (pImpl_->nPosition_ + nRead - 1)/pImpl_->nBlockSize_;
	size_t nReadAll = 0;
	for (unsigned int n = nStart; n <= nEnd; ++n) {
		BinaryFile* pFile = pImpl_->getFile(n);
		if (!pFile)
			return -1;
		
		size_t nReadSize = pImpl_->nBlockSize_;
		int nPosition = 0;
		if (n == nStart) {
			nReadSize = QSMIN((n + 1)*pImpl_->nBlockSize_ - pImpl_->nPosition_, nRead);
			nPosition = pImpl_->nPosition_ - nStart*pImpl_->nBlockSize_;
		}
		else if (n == nEnd) {
			nReadSize = nRead;
		}
		assert(static_cast<size_t>(nPosition) < pImpl_->nBlockSize_);
		assert(nReadSize <= pImpl_->nBlockSize_);
		if (pFile->setPosition(nPosition, SEEKORIGIN_BEGIN) == -1)
			return -1;
		size_t nFileRead = pFile->read(p, nReadSize);
		if (nFileRead == -1)
			return -1;
		p += nFileRead;
		nRead -= nFileRead;
		nReadAll += nFileRead;
		if (nFileRead != nReadSize)
			break;
	}
	pImpl_->nPosition_ += nReadAll;
	
	return nReadAll;
}

size_t qs::DividedFile::write(const unsigned char* p,
							  size_t nWrite)
{
	assert(p);
	
	unsigned int nStart = pImpl_->nPosition_/pImpl_->nBlockSize_;
	unsigned int nEnd = (pImpl_->nPosition_ + nWrite - 1)/pImpl_->nBlockSize_;
	unsigned int nWriteAll = 0;
	for (unsigned int n = nStart; n <= nEnd; ++n) {
		BinaryFile* pFile = pImpl_->getFile(n);
		if (!pFile)
			return -1;
		
		size_t nWriteSize = pImpl_->nBlockSize_;
		int nPosition = 0;
		if (n == nStart) {
			nWriteSize = QSMIN((n + 1)*pImpl_->nBlockSize_ - pImpl_->nPosition_, nWrite);
			nPosition = pImpl_->nPosition_ - nStart*pImpl_->nBlockSize_;
		}
		else if (n == nEnd) {
			nWriteSize = nWrite;
		}
		assert(static_cast<size_t>(nPosition) < pImpl_->nBlockSize_);
		assert(nWriteSize <= pImpl_->nBlockSize_);
		if (pFile->setPosition(nPosition, SEEKORIGIN_BEGIN) == -1)
			return -1;
		if (pFile->write(p, nWriteSize) == -1)
			return -1;
		p += nWriteSize;
		nWrite -= nWriteSize;
		nWriteAll += nWriteSize;
	}
	pImpl_->nPosition_ += nWriteAll;
	
	return nWrite;
}

bool qs::DividedFile::flush()
{
	bool bFail = false;
	DividedFileImpl::FileList::iterator it = pImpl_->listFile_.begin();
	while (it != pImpl_->listFile_.end()) {
		BinaryFile* pFile = *it;
		if (pFile && !pFile->flush())
			bFail = true;
		++it;
	}
	
	return !bFail;
}

int qs::DividedFile::getPosition()
{
	return pImpl_->nPosition_;
}

int qs::DividedFile::setPosition(int nPosition,
								 SeekOrigin seekOrigin)
{
	assert(seekOrigin == SEEKORIGIN_BEGIN);
	pImpl_->nPosition_ = nPosition;
	return getPosition();
}

bool qs::DividedFile::setEndOfFile()
{
	unsigned int nFile = pImpl_->nPosition_/pImpl_->nBlockSize_;
	
	BinaryFile* pFile = pImpl_->getFile(nFile);
	if (!pFile)
		return false;
	
	if (pFile->setPosition(pImpl_->nPosition_ - nFile*pImpl_->nBlockSize_,
		SEEKORIGIN_BEGIN) == -1)
		return false;
	if (!pFile->setEndOfFile())
		return false;
	
	DividedFileImpl::FileList& l = pImpl_->listFile_;
	for (unsigned int n = nFile + 1; n < l.size(); ++n)
		delete l[n];
	l.erase(l.begin() + nFile + 1, l.end());
	
	for (unsigned int n = nFile + 1; ; ++n) {
		wstring_ptr wstrPath(pImpl_->getPath(n));
		W2T(wstrPath.get(), ptszPath);
		if (::GetFileAttributes(ptszPath) == 0xffffffff)
			break;
		::DeleteFile(ptszPath);
	}
	
	return true;
}

size_t qs::DividedFile::getSize()
{
	assert(false);
	return -1;
}


/****************************************************************************
 *
 * TemporaryFileRenamerImpl
 *
 */

struct qs::TemporaryFileRenamerImpl
{
	wstring_ptr wstrOriginalPath_;
	wstring_ptr wstrTemporaryPath_;
#ifndef UNICODE
	tstring_ptr tstrOriginalPath_;
	tstring_ptr tstrTemporaryPath_;
#endif
	bool bRenamed_;
};


/****************************************************************************
 *
 * TemporaryFileRenamer
 *
 */

qs::TemporaryFileRenamer::TemporaryFileRenamer(const WCHAR* pwszPath) :
	pImpl_(0)
{
	wstring_ptr wstrOriginalPath(allocWString(pwszPath));
	wstring_ptr wstrTemporaryPath(concat(pwszPath, L".tmp"));
	
#ifndef UNICODE
	tstring_ptr tstrOriginalPath(wcs2tcs(wstrOriginalPath.get()));
	tstring_ptr tstrTemporaryPath(wcs2tcs(wstrTemporaryPath.get()));
#endif
	
	pImpl_ = new TemporaryFileRenamerImpl();
	pImpl_->wstrOriginalPath_ = wstrOriginalPath;
	pImpl_->wstrTemporaryPath_ = wstrTemporaryPath;
#ifndef UNICODE
	pImpl_->tstrOriginalPath_ = tstrOriginalPath;
	pImpl_->tstrTemporaryPath_ = tstrTemporaryPath;
#endif
	pImpl_->bRenamed_ = false;
}

qs::TemporaryFileRenamer::~TemporaryFileRenamer()
{
	if (!pImpl_->bRenamed_) {
#ifdef UNICODE
		const TCHAR* ptszPath = pImpl_->wstrTemporaryPath_.get();
#else
		const TCHAR* ptszPath = pImpl_->tstrTemporaryPath_.get();
#endif
		::DeleteFile(ptszPath);
	}
	delete pImpl_;
	pImpl_ = 0;
}

const WCHAR* qs::TemporaryFileRenamer::getPath() const
{
	return pImpl_->wstrTemporaryPath_.get();
}

bool qs::TemporaryFileRenamer::rename()
{
#ifdef UNICODE
	const TCHAR* ptszOriginalPath = pImpl_->wstrOriginalPath_.get();
	const TCHAR* ptszTemporaryPath = pImpl_->wstrTemporaryPath_.get();
#else
	const TCHAR* ptszOriginalPath = pImpl_->tstrOriginalPath_.get();
	const TCHAR* ptszTemporaryPath = pImpl_->tstrTemporaryPath_.get();
#endif
	if (::GetFileAttributes(ptszOriginalPath) != 0xffffffff) {
		if (!::DeleteFile(ptszOriginalPath))
			return false;
	}
	if (!::MoveFile(ptszTemporaryPath, ptszOriginalPath))
		return false;
	
	pImpl_->bRenamed_ = true;
	
	return true;
}
