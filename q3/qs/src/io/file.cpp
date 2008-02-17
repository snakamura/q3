/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#include <qsconv.h>
#include <qsfile.h>
#include <qsinit.h>
#include <qslog.h>
#include <qsosutil.h>
#include <qsstl.h>
#include <qsstring.h>

#include <algorithm>
#include <vector>

#include <windows.h>
#include <tchar.h>

using namespace qs;

#if defined _WIN32_WCE && _WIN32_WCE <= 0x300
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

bool qs::File::isFileExisting(const WCHAR* pwszPath)
{
	W2T(pwszPath, ptszPath);
	DWORD dw = ::GetFileAttributes(ptszPath);
	return dw != 0xffffffff && !(dw & FILE_ATTRIBUTE_DIRECTORY);
}

bool qs::File::isDirectoryExisting(const WCHAR* pwszPath)
{
	W2T(pwszPath, ptszPath);
	DWORD dw = ::GetFileAttributes(ptszPath);
	return dw != 0xffffffff && (dw & FILE_ATTRIBUTE_DIRECTORY);
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
		if (!isFileExisting(pwszPath))
			break;
	}
	
	return wstrPath;
}

bool qs::File::createDirectory(const WCHAR* pwszDir)
{
	assert(pwszDir);
	assert(*(pwszDir + wcslen(pwszDir) - 1) != L'\\');
	
	W2T(pwszDir, ptszDir);
	
	DWORD dwAttributes = ::GetFileAttributes(ptszDir);
	if (dwAttributes == 0xffffffff)
		;
	else if (dwAttributes & FILE_ATTRIBUTE_DIRECTORY)
		return true;
	else
		return false;
	
	if (::CreateDirectory(ptszDir, 0))
		return true;
	
	const WCHAR* p = wcsrchr(pwszDir, L'\\');
	if (!p)
		return false;
	
	wstring_ptr wstrParentDir(allocWString(pwszDir, p - pwszDir));
	if (!createDirectory(wstrParentDir.get()))
		return false;
	
	return ::CreateDirectory(ptszDir, 0) != 0;
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

bool qs::File::isDirectoryEmpty(const WCHAR* pwszDir)
{
	assert(pwszDir);
	assert(*(pwszDir + wcslen(pwszDir) - 1) != L'\\');
	
	wstring_ptr wstrFind(concat(pwszDir, L"\\*.*"));
	W2T(wstrFind.get(), ptszFind);
	WIN32_FIND_DATA fd;
	AutoFindHandle hFind(::FindFirstFile(ptszFind, &fd));
	if (!hFind.get())
		return false;
	
	do {
		if (_tcscmp(fd.cFileName, _T(".")) == 0 ||
			_tcscmp(fd.cFileName, _T("..")) == 0)
			continue;
		return false;
	} while (::FindNextFile(hFind.get(), &fd));
	
	return true;
}

bool qs::File::isDeviceName(const WCHAR* pwszName)
{
	assert(pwszName);
	
	const WCHAR* pwszDeviceNames[] = {
		L"CON",
		L"PRN",
		L"AUX",
		L"NUL",
		L"COM1",
		L"COM2",
		L"COM3",
		L"COM4",
		L"COM5",
		L"COM6",
		L"COM7",
		L"COM8",
		L"COM9",
		L"LPT1",
		L"LPT2",
		L"LPT3",
		L"LPT4",
		L"LPT5",
		L"LPT6",
		L"LPT7",
		L"LPT8",
		L"LPT9",
		L"CLOCK$"
	};
	for (int n = 0; n < countof(pwszDeviceNames); ++n) {
		const WCHAR* p = pwszDeviceNames[n];
		size_t nLen = wcslen(p);
		if (_wcsicmp(pwszName, p) == 0 ||
			(_wcsnicmp(pwszName, p, nLen) == 0 && pwszName[nLen] == L'.'))
			return true;
	}
	return false;
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
	
	static LARGE_INTEGER convertPosition(File::Offset n);
	static File::Offset convertPosition(DWORD nLow,
										LONG nHigh);
	
	size_t nBufferSize_;
	HANDLE hFile_;
	wstring_ptr wstrPath_;
	File::Offset nPosition_;
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
	assert(nPosition_ == 0);
	assert(!bWritten_);
	assert(pBuf_.get());
	assert(pBufEnd_ == pBuf_.get());
	assert(pCurrent_ == pBuf_.get());
	
	assert(nMode & BinaryFile::MODE_READ || nMode & BinaryFile::MODE_WRITE);
	
	DWORD dwMode = 0;
	DWORD dwDescription = 0;
	
	if (nMode & BinaryFile::MODE_CREATE)
		dwDescription = CREATE_ALWAYS;
	
	if (nMode & BinaryFile::MODE_WRITE) {
		dwMode |= GENERIC_WRITE;
		if (dwDescription == 0)
			dwDescription = OPEN_ALWAYS;
	}
	if (nMode & BinaryFile::MODE_READ) {
		dwMode |= GENERIC_READ;
		if (dwDescription == 0)
			dwDescription = OPEN_EXISTING;
	}
	
	W2T(pwszPath, ptszPath);
	AutoHandle hFile(::CreateFile(ptszPath, dwMode, FILE_SHARE_READ,
		0, dwDescription, FILE_ATTRIBUTE_NORMAL, 0));
	if (!hFile.get()) {
		Log log(InitThread::getInitThread().getLogger(), L"qs::BinaryFileImpl");
		log.errorf(L"Could not open file: %s, %x", pwszPath, ::GetLastError());
		return false;
	}
	hFile_ = hFile.release();
	wstrPath_ = allocWString(pwszPath);
	
	return true;
}

bool qs::BinaryFileImpl::close()
{
	if (hFile_) {
		if (!flushBuffer())
			return false;
		if (!::CloseHandle(hFile_)) {
			Log log(InitThread::getInitThread().getLogger(), L"qs::BinaryFileImpl");
			log.errorf(L"Could not close file: %s, %x", wstrPath_.get(), ::GetLastError());
			return false;
		}
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
	
	while (pBufEnd_ != pBuf_.get() + nBufferSize_) {
		DWORD dwRead = 0;
		if (!::ReadFile(hFile_, pBufEnd_,
			static_cast<DWORD>(pBuf_.get() + nBufferSize_ - pBufEnd_), &dwRead, 0)) {
			Log log(InitThread::getInitThread().getLogger(), L"qs::BinaryFileImpl");
			log.errorf(L"Could not read file: %s, %x", wstrPath_.get(), ::GetLastError());
			return false;
		}
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
		
		LARGE_INTEGER pos(convertPosition(nPosition_));
		if (::SetFilePointer(hFile_, pos.LowPart, &pos.HighPart, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
			return false;
		
		unsigned char* p = pBuf_.get();
		while (p != pBufEnd_) {
			DWORD dwWritten = 0;
			if (!::WriteFile(hFile_, p, static_cast<DWORD>(pBufEnd_ - p), &dwWritten, 0)) {
				Log log(InitThread::getInitThread().getLogger(), L"qs::BinaryFileImpl");
				log.errorf(L"Could not write file: %s, %x", wstrPath_.get(), ::GetLastError());
				return false;
			}
			p += dwWritten;
		}
		bWritten_ = false;
	}
	
	if (pCurrent_ != pBuf_.get()) {
		LARGE_INTEGER pos(convertPosition(nPosition_ + (pCurrent_ - pBuf_.get())));
		DWORD dwNewPos = ::SetFilePointer(hFile_, pos.LowPart, &pos.HighPart, FILE_BEGIN);
		if (dwNewPos == INVALID_SET_FILE_POINTER) {
			Log log(InitThread::getInitThread().getLogger(), L"qs::BinaryFileImpl");
			log.errorf(L"Could not set file pointer: %s, %x", wstrPath_.get(), ::GetLastError());
			return false;
		}
		nPosition_ = convertPosition(dwNewPos, pos.HighPart);
	}
	
	pCurrent_ = pBuf_.get();
	pBufEnd_ = pBuf_.get();
	
	return true;
}

LARGE_INTEGER qs::BinaryFileImpl::convertPosition(File::Offset n)
{
	LARGE_INTEGER li;
	li.QuadPart = n;
	return li;
}

File::Offset qs::BinaryFileImpl::convertPosition(DWORD nLow,
												 LONG nHigh)
{
	LARGE_INTEGER li;
	li.LowPart = nLow;
	li.HighPart = nHigh;
	return li.QuadPart;
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
	pImpl->nBufferSize_ = nBufferSize;
	pImpl->hFile_ = 0;
	pImpl->nPosition_ = 0;
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
		
		if (nRead > pImpl_->nBufferSize_/2) {
			if (!pImpl_->flushBuffer())
				return -1;
			assert(pImpl_->pBufEnd_ == pImpl_->pBuf_.get());
			assert(pImpl_->pCurrent_ == pImpl_->pBuf_.get());
			
			while (nRead != 0) {
				DWORD dwRead = 0;
				if (!::ReadFile(pImpl_->hFile_, p, static_cast<DWORD>(nRead), &dwRead, 0)) {
					Log log(InitThread::getInitThread().getLogger(), L"qs::BinaryFileImpl");
					log.errorf(L"Could not read file: %s, %x", pImpl_->wstrPath_.get(), ::GetLastError());
					return -1;
				}
				if (dwRead == 0)
					break;
				nSize += dwRead;
				p += dwRead;
				nRead -= dwRead;
				pImpl_->nPosition_ += dwRead;
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
	
	size_t nWritten = nWrite;
	
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
		
		if (nWrite > pImpl_->nBufferSize_/2) {
			if (!pImpl_->flushBuffer())
				return -1;
			assert(pImpl_->pBufEnd_ == pImpl_->pBuf_.get());
			assert(pImpl_->pCurrent_ == pImpl_->pBuf_.get());
			
			size_t n = nWrite;
			while (n != 0) {
				DWORD dwWritten = 0;
				if (!::WriteFile(pImpl_->hFile_, p, static_cast<DWORD>(n), &dwWritten, 0)) {
					Log log(InitThread::getInitThread().getLogger(), L"qs::BinaryFileImpl");
					log.errorf(L"Could not write file: %s, %x", pImpl_->wstrPath_.get(), ::GetLastError());
					return -1;
				}
				p += dwWritten;
				n -= dwWritten;
				pImpl_->nPosition_ += dwWritten;
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
	
	return nWritten;
}

bool qs::BinaryFile::flush()
{
	if (!pImpl_->flushBuffer())
		return false;
	
//	return ::FlushFileBuffers(pImpl_->hFile_) != 0;
	return true;
}

File::Offset qs::BinaryFile::getPosition()
{
	return pImpl_->nPosition_ + (pImpl_->pCurrent_ - pImpl_->pBuf_.get());
}

File::Offset qs::BinaryFile::setPosition(Offset nPosition,
										 SeekOrigin seekOrigin)
{
	if (pImpl_->pBuf_.get() != pImpl_->pBufEnd_) {
		Offset nNewPos = 0;
		switch (seekOrigin) {
		case SEEKORIGIN_BEGIN:
			nNewPos = nPosition;
			break;
		case SEEKORIGIN_END:
			{
				Offset nSize = getSize();
				if (nSize == -1)
					return -1;
				nNewPos = nSize + nPosition;
			}
			break;
		case SEEKORIGIN_CURRENT:
			nNewPos = pImpl_->nPosition_ + (pImpl_->pCurrent_ - pImpl_->pBuf_.get()) + nPosition;
			break;
		default:
			assert(false);
			return -1;
		}
		if (pImpl_->nPosition_ <= nNewPos &&
			nNewPos <= pImpl_->nPosition_ + (pImpl_->pBufEnd_ - pImpl_->pBuf_.get())) {
			pImpl_->pCurrent_ = pImpl_->pBuf_.get() + (nNewPos - pImpl_->nPosition_);
			return getPosition();
		}
	}
	
	DWORD dwMethod = seekOrigin == SEEKORIGIN_BEGIN ? FILE_BEGIN :
		seekOrigin == SEEKORIGIN_END ? FILE_END : FILE_CURRENT;
	if (!pImpl_->flushBuffer())
		return -1;
	assert(pImpl_->pBufEnd_ == pImpl_->pBuf_.get());
	assert(pImpl_->pCurrent_ == pImpl_->pBuf_.get());
	LARGE_INTEGER pos(BinaryFileImpl::convertPosition(nPosition));
	DWORD dwNewPos = ::SetFilePointer(pImpl_->hFile_, pos.LowPart, &pos.HighPart, dwMethod);
	if (dwNewPos == INVALID_SET_FILE_POINTER) {
		Log log(InitThread::getInitThread().getLogger(), L"qs::BinaryFileImpl");
		log.errorf(L"Could not set file pointer: %s, %x", pImpl_->wstrPath_.get(), ::GetLastError());
		return -1;
	}
	pImpl_->nPosition_ = BinaryFileImpl::convertPosition(dwNewPos, pos.HighPart);
	
	return getPosition();
}

bool qs::BinaryFile::setEndOfFile()
{
	Offset nPosition = getPosition();
	
	if (!pImpl_->flushBuffer())
		return false;
	assert(pImpl_->pBufEnd_ == pImpl_->pBuf_.get());
	assert(pImpl_->pCurrent_ == pImpl_->pBuf_.get());
	
	LARGE_INTEGER pos(BinaryFileImpl::convertPosition(nPosition));
	DWORD dwNewPos = ::SetFilePointer(pImpl_->hFile_, pos.LowPart, &pos.HighPart, FILE_BEGIN);
	if (dwNewPos == INVALID_SET_FILE_POINTER) {
		Log log(InitThread::getInitThread().getLogger(), L"qs::BinaryFileImpl");
		log.errorf(L"Could not set file pointer: %s, %x", pImpl_->wstrPath_.get(), ::GetLastError());
		return false;
	}
	pImpl_->nPosition_ = BinaryFileImpl::convertPosition(dwNewPos, pos.HighPart);
	
	if (!::SetEndOfFile(pImpl_->hFile_)) {
		Log log(InitThread::getInitThread().getLogger(), L"qs::BinaryFileImpl");
		log.errorf(L"Could not set end of file: %s, %x", pImpl_->wstrPath_.get(), ::GetLastError());
	}
	return true;
}

File::Offset qs::BinaryFile::getSize()
{
	DWORD dwSizeHigh = 0;
	DWORD dwSizeLow = ::GetFileSize(pImpl_->hFile_, &dwSizeHigh);
	if (dwSizeLow == -1 && ::GetLastError() != NO_ERROR) {
		Log log(InitThread::getInitThread().getLogger(), L"qs::BinaryFileImpl");
		log.errorf(L"Could not get file size: %s, %x", pImpl_->wstrPath_.get(), ::GetLastError());
		return -1;
	}
	return QSMAX(BinaryFileImpl::convertPosition(dwSizeLow, dwSizeHigh),
		pImpl_->nPosition_ + (pImpl_->pCurrent_ - pImpl_->pBuf_.get()));
}


/****************************************************************************
 *
 * DividedFileImpl
 *
 */

struct qs::DividedFileImpl
{
	typedef std::vector<BinaryFile*> FileList;
	
	BinaryFile* getFile(size_t n);
	wstring_ptr getPath(size_t n) const;
	
	wstring_ptr wstrPath_;
	unsigned int nBlockSize_;
	unsigned int nMode_;
	size_t nBufferSize_;
	FileList listFile_;
	File::Offset nPosition_;
};

BinaryFile* qs::DividedFileImpl::getFile(size_t n)
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

wstring_ptr qs::DividedFileImpl::getPath(size_t n) const
{
	const WCHAR* pFileName = wcsrchr(wstrPath_.get(), L'\\');
	if (!pFileName)
		pFileName = wstrPath_.get();
	const WCHAR* pExt = wcschr(pFileName, L'.');
	if (!pExt)
		pExt = pFileName + wcslen(pFileName);
	
	WCHAR wsz[16];
	_snwprintf(wsz, countof(wsz), L"%03u", static_cast<unsigned int>(n));
	
	return concat(wstrPath_.get(), pExt - wstrPath_.get(), wsz, -1, pExt, -1);
}


/****************************************************************************
 *
 * DividedFile
 *
 */

qs::DividedFile::DividedFile(const WCHAR* pwszPath,
							 unsigned int nBlockSize,
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
	for (DividedFileImpl::FileList::iterator it = pImpl_->listFile_.begin(); it != pImpl_->listFile_.end(); ++it) {
		BinaryFile* pFile = *it;
		if (pFile) {
			if (!pFile->close())
				bFail = true;
			delete pFile;
			*it = 0;
		}
	}
	
	return !bFail;
}

size_t qs::DividedFile::read(unsigned char* p,
							 size_t nRead)
{
	assert(p);
	
	if (nRead == 0)
		return 0;
	
	size_t nStart = static_cast<size_t>(pImpl_->nPosition_/pImpl_->nBlockSize_);
	size_t nEnd = static_cast<size_t>((pImpl_->nPosition_ + nRead - 1)/pImpl_->nBlockSize_);
	size_t nReadAll = 0;
	for (size_t n = nStart; n <= nEnd; ++n) {
		BinaryFile* pFile = pImpl_->getFile(n);
		if (!pFile)
			return -1;
		
		size_t nReadSize = pImpl_->nBlockSize_;
		Offset nPosition = 0;
		if (n == nStart) {
			nReadSize = QSMIN(nRead,
				static_cast<size_t>((n + 1)*static_cast<Offset>(pImpl_->nBlockSize_) - pImpl_->nPosition_));
			nPosition = pImpl_->nPosition_ - nStart*static_cast<Offset>(pImpl_->nBlockSize_);
		}
		else if (n == nEnd) {
			nReadSize = nRead;
		}
		assert(nPosition < static_cast<Offset>(pImpl_->nBlockSize_));
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
	
	size_t nStart = static_cast<size_t>(pImpl_->nPosition_/pImpl_->nBlockSize_);
	size_t nEnd = static_cast<size_t>((pImpl_->nPosition_ + nWrite - 1)/pImpl_->nBlockSize_);
	size_t nWriteAll = 0;
	for (size_t n = nStart; n <= nEnd; ++n) {
		BinaryFile* pFile = pImpl_->getFile(n);
		if (!pFile)
			return -1;
		
		size_t nWriteSize = pImpl_->nBlockSize_;
		Offset nPosition = 0;
		if (n == nStart) {
			nWriteSize = QSMIN(nWrite,
				static_cast<size_t>((n + 1)*static_cast<Offset>(pImpl_->nBlockSize_) - pImpl_->nPosition_));
			nPosition = pImpl_->nPosition_ - nStart*static_cast<Offset>(pImpl_->nBlockSize_);
		}
		else if (n == nEnd) {
			nWriteSize = nWrite;
		}
		assert(nPosition < static_cast<Offset>(pImpl_->nBlockSize_));
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
	
	return nWriteAll;
}

bool qs::DividedFile::flush()
{
	bool bFail = false;
	for (DividedFileImpl::FileList::iterator it = pImpl_->listFile_.begin(); it != pImpl_->listFile_.end(); ++it) {
		BinaryFile* pFile = *it;
		if (pFile && !pFile->flush())
			bFail = true;
	}
	return !bFail;
}

File::Offset qs::DividedFile::getPosition()
{
	return pImpl_->nPosition_;
}

File::Offset qs::DividedFile::setPosition(Offset nPosition,
										  SeekOrigin seekOrigin)
{
	assert(seekOrigin == SEEKORIGIN_BEGIN);
	pImpl_->nPosition_ = nPosition;
	return getPosition();
}

bool qs::DividedFile::setEndOfFile()
{
	size_t nFile = static_cast<size_t>(pImpl_->nPosition_/pImpl_->nBlockSize_);
	
	BinaryFile* pFile = pImpl_->getFile(nFile);
	if (!pFile)
		return false;
	
	if (pFile->setPosition(
		pImpl_->nPosition_ - nFile*static_cast<Offset>(pImpl_->nBlockSize_),
		SEEKORIGIN_BEGIN) == -1)
		return false;
	if (!pFile->setEndOfFile())
		return false;
	
	DividedFileImpl::FileList& l = pImpl_->listFile_;
	for (size_t n = nFile + 1; n < l.size(); ++n)
		delete l[n];
	l.erase(l.begin() + nFile + 1, l.end());
	
	for (size_t n = nFile + 1; ; ++n) {
		wstring_ptr wstrPath(pImpl_->getPath(n));
		if (!File::isFileExisting(wstrPath.get()))
			break;
		W2T(wstrPath.get(), ptszPath);
		::DeleteFile(ptszPath);
	}
	
	return true;
}

File::Offset qs::DividedFile::getSize()
{
	WIN32_FIND_DATA fd;
	unsigned int n = 0;
	while (true) {
		wstring_ptr wstrPath(pImpl_->getPath(n));
		W2T(wstrPath.get(), ptszPath);
		AutoFindHandle hFind(::FindFirstFile(ptszPath, &fd));
		if (!hFind.get())
			break;
		++n;
	}
	if (n == 0)
		return 0;
	
	return static_cast<Offset>(pImpl_->nBlockSize_)*(n - 1) + fd.nFileSizeLow;
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
	Log log(InitThread::getInitThread().getLogger(), L"qs::TemporaryFileRenamer");
	
#ifdef UNICODE
	const TCHAR* ptszOriginalPath = pImpl_->wstrOriginalPath_.get();
	const TCHAR* ptszTemporaryPath = pImpl_->wstrTemporaryPath_.get();
#else
	const TCHAR* ptszOriginalPath = pImpl_->tstrOriginalPath_.get();
	const TCHAR* ptszTemporaryPath = pImpl_->tstrTemporaryPath_.get();
#endif
	if (::GetFileAttributes(ptszOriginalPath) != 0xffffffff) {
		if (!::DeleteFile(ptszOriginalPath) && ::GetLastError() != ERROR_FILE_NOT_FOUND) {
			T2W(ptszOriginalPath, pwszPath);
			log.errorf(L"Could not delete file: %s, %x", pwszPath, ::GetLastError());
			return false;
		}
	}
	if (!::MoveFile(ptszTemporaryPath, ptszOriginalPath)) {
		T2W(ptszTemporaryPath, pwszFromPath);
		T2W(ptszOriginalPath, pwszToPath);
		log.errorf(L"Could not move file: %s, %s, %x", pwszFromPath, pwszToPath, ::GetLastError());
		return false;
	}
	
	pImpl_->bRenamed_ = true;
	
	return true;
}
