/*
 * $Id: file.cpp,v 1.1.1.1 2003/04/29 08:07:35 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsfile.h>
#include <qsconv.h>
#include <qsstring.h>
#include <qserror.h>
#include <qsosutil.h>
#include <qsnew.h>

#include <algorithm>

#include <windows.h>
#include <tchar.h>

using namespace qs;

#ifdef _WIN32_WCE
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

QSTATUS qs::File::getTempFileName(const WCHAR* pwszDir, WSTRING* pwstrPath)
{
	assert(pwszDir);
	assert(pwstrPath);
	
	*pwstrPath = 0;
	
	string_ptr<WSTRING> wstrPath(allocWString(wcslen(pwszDir) + 33));
	if (!wstrPath.get())
		return QSTATUS_OUTOFMEMORY;
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
	*pwstrPath = wstrPath.release();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::File::removeDirectory(const WCHAR* pwszDir)
{
	assert(pwszDir);
	assert(*(pwszDir + wcslen(pwszDir) - 1) != L'\\');
	
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrPathBase(concat(pwszDir, L"\\*.*"));
	if (!wstrPathBase.get())
		return QSTATUS_OUTOFMEMORY;
	
	string_ptr<TSTRING> tstrPathBase(wcs2tcs(wstrPathBase.get()));
	if (!tstrPathBase.get())
		return QSTATUS_OUTOFMEMORY;
	
	WIN32_FIND_DATA fd;
	AutoFindHandle hFind(::FindFirstFile(tstrPathBase.get(), &fd));
	
	size_t nLen = _tcslen(tstrPathBase.get()) - 3;
	assert(_tcscmp(tstrPathBase.get() + nLen - 1, _T("\\*.*")) == 0);
	*(tstrPathBase.get() + nLen) = _T('\0');
	
	if (hFind.get()) {
		string_ptr<TSTRING> tstrPath(allocTString(nLen + MAX_PATH + 10));
		_tcscpy(tstrPath.get(), tstrPathBase.get());
		TCHAR* pFileName = tstrPath.get() + nLen;
		do {
			if (_tcscmp(fd.cFileName, _T(".")) == 0 ||
				_tcscmp(fd.cFileName, _T("..")) == 0)
				continue;
			if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				T2W(fd.cFileName, pwszFileName);
				size_t nLen = wcslen(wstrPathBase.get()) - 3;
				string_ptr<WSTRING> wstrPath(allocWString(nLen + wcslen(pwszFileName) + 10));
				wcsncpy(wstrPath.get(), wstrPathBase.get(), nLen);
				wcscpy(wstrPath.get() + nLen, pwszFileName);
				status = File::removeDirectory(wstrPath.get());
				CHECK_QSTATUS();
			}
			else {
				_tcscpy(pFileName, fd.cFileName);
				if (!::DeleteFile(tstrPath.get()))
					return QSTATUS_FAIL;
			}
		} while (::FindNextFile(hFind.get(), &fd));
	}
	
	assert(*(tstrPathBase.get() + nLen - 1) == _T('\\'));
	*(tstrPathBase.get() + nLen - 1) = _T('\0');
	
	return ::RemoveDirectory(tstrPathBase.get()) ? QSTATUS_SUCCESS : QSTATUS_FAIL;
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
	
	QSTATUS open(const WCHAR* pwszPath, unsigned int nMode);
	QSTATUS close();
	QSTATUS mapBuffer();
	QSTATUS flushBuffer();
	
	HANDLE hFile_;
	DWORD dwPosition_;
	bool bWritten_;
	unsigned char* pBuf_;
	unsigned char* pBufEnd_;
	unsigned char* pCurrent_;
};

QSTATUS qs::BinaryFileImpl::open(const WCHAR* pwszPath, unsigned int nMode)
{
	assert(pwszPath);
	assert(!hFile_);
	assert(dwPosition_ == 0);
	assert(!bWritten_);
	assert(pBuf_);
	assert(pBufEnd_ == pBuf_);
	assert(pCurrent_ == pBuf_);
	
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
	hFile_ = ::CreateFile(ptszPath, dwMode, dwShare, 0, dwDescription,
		FILE_ATTRIBUTE_NORMAL, 0);
	if (hFile_ == INVALID_HANDLE_VALUE) {
		hFile_ = 0;
		return QSTATUS_FAIL;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::BinaryFileImpl::close()
{
	DECLARE_QSTATUS();
	
	if (hFile_) {
		status = flushBuffer();
		if (!::CloseHandle(hFile_) && status != QSTATUS_SUCCESS)
			status = QSTATUS_FAIL;
		hFile_ = 0;
	}
	return status;
}

QSTATUS qs::BinaryFileImpl::mapBuffer()
{
	DECLARE_QSTATUS();
	
	status = flushBuffer();
	CHECK_QSTATUS();
	assert(pBufEnd_ == pBuf_);
	assert(pCurrent_ = pBuf_);
	
	while (pBufEnd_ != pBuf_ + BUFFER_SIZE) {
		DWORD dwRead = 0;
		if (!::ReadFile(hFile_, pBufEnd_, pBuf_ + BUFFER_SIZE - pBufEnd_, &dwRead, 0))
			return QSTATUS_FAIL;
		if (dwRead == 0)
			break;
		pBufEnd_ += dwRead;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::BinaryFileImpl::flushBuffer()
{
	if (bWritten_) {
		assert(pCurrent_ != pBuf_);
		assert(pBufEnd_ != pBuf_);
		
		if (::SetFilePointer(hFile_, dwPosition_, 0, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
			return QSTATUS_FAIL;
		
		unsigned char* p = pBuf_;
		while (p != pBufEnd_) {
			DWORD dwWritten = 0;
			if (!::WriteFile(hFile_, p, pBufEnd_ - p, &dwWritten, 0))
				return QSTATUS_FAIL;
			p += dwWritten;
		}
		bWritten_ = false;
	}
	
	DWORD dwNewPos = ::SetFilePointer(hFile_,
		dwPosition_ + (pCurrent_ - pBuf_), 0, FILE_BEGIN);
	if (dwNewPos == INVALID_SET_FILE_POINTER)
		return QSTATUS_FAIL;
	dwPosition_ = dwNewPos;
	
	pCurrent_ = pBuf_;
	pBufEnd_ = pBuf_;
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * BinaryFile
 *
 */

qs::BinaryFile::BinaryFile(const WCHAR* pwszPath, unsigned int nMode,
	size_t nBufferSize, QSTATUS* pstatus)
{
	assert(pwszPath);
	assert(pstatus);
	
	if (nBufferSize == 0)
		nBufferSize = BinaryFileImpl::BUFFER_SIZE;
	
	DECLARE_QSTATUS();
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	pImpl_->hFile_ = 0;
	pImpl_->dwPosition_ = 0;
	pImpl_->bWritten_ = false;
	pImpl_->pBuf_ = 0;
	pImpl_->pBufEnd_ = 0;
	pImpl_->pCurrent_ = 0;
	
	malloc_ptr<unsigned char> pBuf(static_cast<unsigned char*>(malloc(nBufferSize)));
	if (!pBuf.get()) {
		*pstatus = QSTATUS_OUTOFMEMORY;
		return;
	}
	pImpl_->pBuf_ = pBuf.release();
	pImpl_->pBufEnd_ = pImpl_->pBuf_;
	pImpl_->pCurrent_ = pImpl_->pBuf_;
	
	status = pImpl_->open(pwszPath, nMode);
	CHECK_QSTATUS_SET(pstatus);
}

qs::BinaryFile::~BinaryFile()
{
	if (pImpl_) {
		pImpl_->close();
		
		free(pImpl_->pBuf_);
		delete pImpl_;
		pImpl_ = 0;
	}
}

QSTATUS qs::BinaryFile::close()
{
	return pImpl_->close();
}

QSTATUS qs::BinaryFile::read(unsigned char* p, size_t nRead, size_t* pnRead)
{
	assert(p);
	assert(pnRead);
	
	*pnRead = 0;
	
	DECLARE_QSTATUS();
	
	if (static_cast<size_t>(pImpl_->pBufEnd_ - pImpl_->pCurrent_) >= nRead) {
		memcpy(p, pImpl_->pCurrent_, nRead);
		pImpl_->pCurrent_ += nRead;
		*pnRead = nRead;
	}
	else {
		if (pImpl_->pCurrent_ != pImpl_->pBufEnd_) {
			size_t n = pImpl_->pBufEnd_ - pImpl_->pCurrent_;
			memcpy(p, pImpl_->pCurrent_, n);
			p += n;
			nRead -= n;
			*pnRead = n;
			pImpl_->pCurrent_ = pImpl_->pBufEnd_;
		}
		
		if (nRead > BinaryFileImpl::BUFFER_SIZE/2) {
			status = pImpl_->flushBuffer();
			CHECK_QSTATUS();
			assert(pImpl_->pBufEnd_ == pImpl_->pBuf_);
			assert(pImpl_->pCurrent_ == pImpl_->pBuf_);
			
			while (nRead != 0) {
				DWORD dwRead = 0;
				if (!::ReadFile(pImpl_->hFile_, p, nRead, &dwRead, 0))
					return QSTATUS_FAIL;
				if (dwRead == 0) {
					if (*pnRead == 0)
						*pnRead = -1;
					break;
				}
				*pnRead += dwRead;
				p += dwRead;
				nRead -= dwRead;
				pImpl_->dwPosition_ += dwRead;
			}
		}
		else {
			status = pImpl_->mapBuffer();
			CHECK_QSTATUS();
			if (static_cast<size_t>(pImpl_->pBufEnd_ - pImpl_->pCurrent_) < nRead)
				nRead = pImpl_->pBufEnd_ - pImpl_->pCurrent_;
			if (nRead != 0) {
				memcpy(p, pImpl_->pCurrent_, nRead);
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

QSTATUS qs::BinaryFile::write(const unsigned char* p, size_t nWrite)
{
	assert(p);
	
	DECLARE_QSTATUS();
	
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
			status = pImpl_->flushBuffer();
			CHECK_QSTATUS();
			assert(pImpl_->pBufEnd_ == pImpl_->pBuf_);
			assert(pImpl_->pCurrent_ == pImpl_->pBuf_);
			
			while (nWrite != 0) {
				DWORD dwWritten = 0;
				if (!::WriteFile(pImpl_->hFile_, p, nWrite, &dwWritten, 0))
					return QSTATUS_FAIL;
				p += dwWritten;
				nWrite -= dwWritten;
				pImpl_->dwPosition_ += dwWritten;
			}
		}
		else {
			status = pImpl_->mapBuffer();
			CHECK_QSTATUS();
			memcpy(pImpl_->pCurrent_, p, nWrite);
			pImpl_->pCurrent_ += nWrite;
			if (pImpl_->pCurrent_ > pImpl_->pBufEnd_)
				pImpl_->pBufEnd_ = pImpl_->pCurrent_;
			pImpl_->bWritten_ = true;
		}
	}
	return QSTATUS_SUCCESS;
}

QSTATUS qs::BinaryFile::flush()
{
	DECLARE_QSTATUS();
	
	status = pImpl_->flushBuffer();
	CHECK_QSTATUS();
	
	return ::FlushFileBuffers(pImpl_->hFile_) ? QSTATUS_SUCCESS : QSTATUS_FAIL;
}

QSTATUS qs::BinaryFile::getPosition(int* pnPosition)
{
	assert(pnPosition);
	*pnPosition = pImpl_->dwPosition_ + (pImpl_->pCurrent_ - pImpl_->pBuf_);
	return QSTATUS_SUCCESS;
}

QSTATUS qs::BinaryFile::setPosition(int nPosition, SeekOrigin seekOrigin)
{
	DECLARE_QSTATUS();
	
	if (pImpl_->pBuf_ != pImpl_->pBufEnd_) {
		DWORD dwNewPos = 0;
		switch (seekOrigin) {
		case SEEKORIGIN_BEGIN:
			dwNewPos = nPosition;
			break;
		case SEEKORIGIN_END:
			{
				size_t nSize = 0;
				status = getSize(&nSize);
				CHECK_QSTATUS();
				dwNewPos = nSize + nPosition;
			}
			break;
		case SEEKORIGIN_CURRENT:
			dwNewPos = pImpl_->dwPosition_ + (pImpl_->pCurrent_ - pImpl_->pBuf_) + nPosition;
			break;
		default:
			assert(false);
			return QSTATUS_FAIL;
		}
		if (pImpl_->dwPosition_ <= dwNewPos &&
			dwNewPos <= pImpl_->dwPosition_ + (pImpl_->pBufEnd_ - pImpl_->pBuf_)) {
			pImpl_->pCurrent_ = pImpl_->pBuf_ + (dwNewPos - pImpl_->dwPosition_);
			return QSTATUS_SUCCESS;
		}
	}
	
	DWORD dwMethod = seekOrigin == SEEKORIGIN_BEGIN ? FILE_BEGIN :
		seekOrigin == SEEKORIGIN_END ? FILE_END : FILE_CURRENT;
	status = pImpl_->flushBuffer();
	CHECK_QSTATUS();
	assert(pImpl_->pBufEnd_ == pImpl_->pBuf_);
	assert(pImpl_->pCurrent_ == pImpl_->pBuf_);
	DWORD dwNewPos = ::SetFilePointer(pImpl_->hFile_, nPosition, 0, dwMethod);
	if (dwNewPos == INVALID_SET_FILE_POINTER)
		return QSTATUS_FAIL;
	pImpl_->dwPosition_ = dwNewPos;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::BinaryFile::setEndOfFile()
{
	DECLARE_QSTATUS();
	
	int nPosition = 0;
	status = getPosition(&nPosition);
	CHECK_QSTATUS();
	
	status = pImpl_->flushBuffer();
	CHECK_QSTATUS();
	assert(pImpl_->pBufEnd_ == pImpl_->pBuf_);
	assert(pImpl_->pCurrent_ == pImpl_->pBuf_);
	
	DWORD dwNewPos = ::SetFilePointer(pImpl_->hFile_, nPosition, 0, FILE_BEGIN);
	if (dwNewPos == INVALID_SET_FILE_POINTER)
		return QSTATUS_FAIL;
	pImpl_->dwPosition_ = dwNewPos;
	
	return ::SetEndOfFile(pImpl_->hFile_) ? QSTATUS_SUCCESS : QSTATUS_FAIL;
}

QSTATUS qs::BinaryFile::getSize(size_t* pnSize)
{
	assert(pnSize);
	
	DWORD dwSize = ::GetFileSize(pImpl_->hFile_, 0);
	if (dwSize == -1)
		return QSTATUS_FAIL;
	*pnSize = QSMAX(dwSize, pImpl_->dwPosition_ + (pImpl_->pCurrent_ - pImpl_->pBuf_));
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * DividedFileImpl
 *
 */

struct qs::DividedFileImpl
{
	typedef std::vector<BinaryFile*> FileList;
	
	QSTATUS getFile(unsigned int n, BinaryFile** ppFile);
	QSTATUS getPath(unsigned int n, WSTRING* pwstrPath) const;
	
	WSTRING wstrPath_;
	size_t nBlockSize_;
	unsigned int nMode_;
	size_t nBufferSize_;
	FileList listFile_;
	int nPosition_;
};

QSTATUS qs::DividedFileImpl::getFile(unsigned int n, BinaryFile** ppFile)
{
	assert(ppFile);
	
	DECLARE_QSTATUS();
	
	if (listFile_.size() <= n) {
		status = STLWrapper<FileList>(listFile_).resize(n + 1);
		CHECK_QSTATUS();
	}
	
	BinaryFile*& pFile = listFile_[n];
	if (!pFile) {
		string_ptr<WSTRING> wstrPath;
		status = getPath(n, &wstrPath);
		CHECK_QSTATUS();
		status = newQsObject(wstrPath.get(), nMode_, nBufferSize_, &pFile);
		CHECK_QSTATUS();
	}
	*ppFile = pFile;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::DividedFileImpl::getPath(unsigned int n, WSTRING* pwstrPath) const
{
	assert(pwstrPath);
	
	DECLARE_QSTATUS();
	
	const WCHAR* pFileName = wcsrchr(wstrPath_, L'\\');
	if (!pFileName)
		pFileName = wstrPath_;
	const WCHAR* pExt = wcschr(pFileName, L'.');
	if (!pExt)
		pExt = pFileName + wcslen(pFileName);
	
	WCHAR wsz[16];
	swprintf(wsz, L"%03u", n);
	
	StringBuffer<WSTRING> buf(wstrPath_, pExt - wstrPath_, &status);
	CHECK_QSTATUS();
	status = buf.append(wsz);
	CHECK_QSTATUS();
	status = buf.append(pExt);
	CHECK_QSTATUS();
	
	*pwstrPath = buf.getString();
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * DividedFile
 *
 */

qs::DividedFile::DividedFile(const WCHAR* pwszPath, size_t nBlockSize,
	unsigned int nMode, size_t nBufferSize, QSTATUS* pstatus) :
	pImpl_(0)
{
	assert(pstatus);
	
	*pstatus = QSTATUS_SUCCESS;
	
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrPath(allocWString(pwszPath));
	if (!wstrPath.get()) {
		*pstatus = QSTATUS_OUTOFMEMORY;
		return;
	}
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	pImpl_->wstrPath_ = wstrPath.release();
	pImpl_->nBlockSize_ = nBlockSize;
	pImpl_->nMode_ = nMode;
	pImpl_->nBufferSize_ = nBufferSize;
	pImpl_->nPosition_ = 0;
}

qs::DividedFile::~DividedFile()
{
	if (pImpl_) {
		close();
		
		freeWString(pImpl_->wstrPath_);
		delete pImpl_;
		pImpl_ = 0;
	}
}

QSTATUS qs::DividedFile::close()
{
	bool bFail = false;
	DividedFileImpl::FileList::iterator it = pImpl_->listFile_.begin();
	while (it != pImpl_->listFile_.end()) {
		BinaryFile* pFile = *it;
		if (pFile) {
			if (pFile->close() != QSTATUS_SUCCESS)
				bFail = true;
			delete pFile;
			*it = 0;
		}
		++it;
	}
	
	return !bFail ? QSTATUS_SUCCESS : QSTATUS_FAIL;
}

QSTATUS qs::DividedFile::read(unsigned char* p, size_t nRead, size_t* pnRead)
{
	assert(p);
	assert(pnRead);
	
	*pnRead = 0;
	
	if (nRead == 0)
		return QSTATUS_SUCCESS;
	
	DECLARE_QSTATUS();
	
	unsigned int nStart = pImpl_->nPosition_/pImpl_->nBlockSize_;
	unsigned int nEnd = (pImpl_->nPosition_ + nRead - 1)/pImpl_->nBlockSize_;
	size_t nReadAll = 0;
	for (unsigned int n = nStart; n <= nEnd; ++n) {
		BinaryFile* pFile = 0;
		status = pImpl_->getFile(n, &pFile);
		CHECK_QSTATUS();
		
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
		status = pFile->setPosition(nPosition, SEEKORIGIN_BEGIN);
		CHECK_QSTATUS();
		size_t nFileRead = 0;
		status = pFile->read(p, nReadSize, &nFileRead);
		CHECK_QSTATUS();
		if (nFileRead != -1) {
			p += nFileRead;
			nRead -= nFileRead;
			nReadAll += nFileRead;
		}
		if (nFileRead != nReadSize)
			break;
	}
	pImpl_->nPosition_ += nReadAll;
	*pnRead = nReadAll == 0 ? -1 : nReadAll;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::DividedFile::write(const unsigned char* p, size_t nWrite)
{
	assert(p);
	
	DECLARE_QSTATUS();
	
	unsigned int nStart = pImpl_->nPosition_/pImpl_->nBlockSize_;
	unsigned int nEnd = (pImpl_->nPosition_ + nWrite - 1)/pImpl_->nBlockSize_;
	unsigned int nWriteAll = 0;
	for (unsigned int n = nStart; n <= nEnd; ++n) {
		BinaryFile* pFile = 0;
		status = pImpl_->getFile(n, &pFile);
		CHECK_QSTATUS();
		
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
		status = pFile->setPosition(nPosition, SEEKORIGIN_BEGIN);
		CHECK_QSTATUS();
		status = pFile->write(p, nWriteSize);
		CHECK_QSTATUS();
		p += nWriteSize;
		nWrite -= nWriteSize;
		nWriteAll += nWriteSize;
	}
	pImpl_->nPosition_ += nWriteAll;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::DividedFile::flush()
{
	bool bFail = false;
	DividedFileImpl::FileList::iterator it = pImpl_->listFile_.begin();
	while (it != pImpl_->listFile_.end()) {
		BinaryFile* pFile = *it;
		if (pFile && pFile->flush() != QSTATUS_SUCCESS)
			bFail = true;
		++it;
	}
	
	return !bFail ? QSTATUS_SUCCESS : QSTATUS_FAIL;
}

QSTATUS qs::DividedFile::getPosition(int* pnPosition)
{
	assert(pnPosition);
	*pnPosition = pImpl_->nPosition_;
	return QSTATUS_SUCCESS;
}

QSTATUS qs::DividedFile::setPosition(int nPosition, SeekOrigin seekOrigin)
{
	assert(seekOrigin == SEEKORIGIN_BEGIN);
	pImpl_->nPosition_ = nPosition;
	return QSTATUS_SUCCESS;
}

QSTATUS qs::DividedFile::setEndOfFile()
{
	DECLARE_QSTATUS();
	
	unsigned int nFile = pImpl_->nPosition_/pImpl_->nBlockSize_;
	
	BinaryFile* pFile = 0;
	status = pImpl_->getFile(nFile, &pFile);
	CHECK_QSTATUS();
	
	status = pFile->setPosition(pImpl_->nPosition_ - nFile*pImpl_->nBlockSize_,
		SEEKORIGIN_BEGIN);
	CHECK_QSTATUS();
	status = pFile->setEndOfFile();
	CHECK_QSTATUS();
	
	DividedFileImpl::FileList& l = pImpl_->listFile_;
	for (unsigned int n = nFile + 1; n < l.size(); ++n)
		delete l[n];
	l.erase(l.begin() + nFile + 1, l.end());
	
	for (n = nFile + 1; ; ++n) {
		string_ptr<WSTRING> wstrPath;
		status = pImpl_->getPath(n, &wstrPath);
		CHECK_QSTATUS();
		W2T(wstrPath.get(), ptszPath);
		if (::GetFileAttributes(ptszPath) == 0xffffffff)
			break;
		::DeleteFile(ptszPath);
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::DividedFile::getSize(size_t* pnSize)
{
	assert(false);
	assert(pnSize);
	*pnSize = 0;
	return QSTATUS_SUCCESS;
}
