/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 */

#ifdef QMZIP

#include <qsfile.h>
#include <qsosutil.h>

extern "C" {
#define API
#include <zip/api.h>
#undef API
}

#include "zip.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * ZipFile
 *
 */

namespace {

int WINAPI print(LPSTR, unsigned long nLen)
{
	return static_cast<int>(nLen);
}

int WINAPI comment(LPSTR)
{
	return 0;
}

}

qm::ZipFile::ZipFile(const WCHAR* pwszFileName,
					 const PathList& listPath,
					 const WCHAR* pwszTempDir)
{
	wstring_ptr wstrDir(File::getTempFileName(pwszTempDir));
	if (!File::createDirectory(wstrDir.get()))
		return;
	wstrDir_ = wstrDir;
	
	wstring_ptr wstrPath(concat(wstrDir_.get(), L"\\", pwszFileName));
	if (!zip(wstrPath.get(), listPath))
		return;
	wstrPath_ = wstrPath;
}

qm::ZipFile::~ZipFile()
{
	if (wstrDir_.get())
		File::removeDirectory(wstrDir_.get());
}

const WCHAR* qm::ZipFile::getPath() const
{
	return wstrPath_.get();
}

bool qm::ZipFile::zip(const WCHAR* pwszPath,
					  const PathList& listPath)
{
	assert(pwszPath);
	assert(!listPath.empty());
	
	Library library(L"zip32.dll");
	if (!library)
		return false;
	
	typedef int (EXPENTRY *PFN_ZPINIT)(LPZIPUSERFUNCTIONS);
	typedef BOOL (EXPENTRY *PFN_ZPSETOPTIONS)(LPZPOPT);
	typedef int (EXPENTRY *PFN_ZPARCHIVE)(ZCL);
	
	PFN_ZPINIT pfnZpInit = reinterpret_cast<PFN_ZPINIT>(
		::GetProcAddress(library, "ZpInit"));
	PFN_ZPSETOPTIONS pfnZpSetOptions = reinterpret_cast<PFN_ZPSETOPTIONS>(
		::GetProcAddress(library, "ZpSetOptions"));
	PFN_ZPARCHIVE pfnZpArchive = reinterpret_cast<PFN_ZPARCHIVE>(
		::GetProcAddress(library, "ZpArchive"));
	if (!pfnZpInit || !pfnZpSetOptions || !pfnZpArchive)
		return false;
	
	ZIPUSERFUNCTIONS userFunctions = { &print, &comment, 0, 0 };
	if (!(*pfnZpInit)(&userFunctions))
		return false;
	
	ZPOPT opt = { 0 };
	opt.fJunkDir = TRUE;
	(*pfnZpSetOptions)(&opt);
	
	size_t nPathLen = 0;
	for (PathList::const_iterator it = listPath.begin(); it != listPath.end(); ++it) {
		string_ptr strPath(wcs2mbs(*it));
		nPathLen += strlen(strPath.get()) + 1;
	}
	
	size_t nIndexLen = sizeof(char*)*listPath.size();
	malloc_ptr<char> pBuf(static_cast<char*>(allocate(nIndexLen + nPathLen)));
	if (!pBuf.get())
		return false;
	char** ppIndex = reinterpret_cast<char**>(pBuf.get());
	char* p = pBuf.get() + nIndexLen;
	
	for (PathList::const_iterator it = listPath.begin(); it != listPath.end(); ++it) {
		string_ptr strPath(wcs2mbs(*it));
		size_t nLen = strlen(strPath.get());
		strcpy(p, strPath.get());
		*ppIndex = p;
		p += nLen;
		*p = '\0';
		++p;
		++ppIndex;
	}
	
	string_ptr strPath(wcs2mbs(pwszPath));
	ZCL zcl = {
		static_cast<int>(listPath.size()),
		strPath.get(),
		reinterpret_cast<char**>(pBuf.get())
	};
	if ((*pfnZpArchive)(zcl) != 0)
		return false;
	
	return true;
}

#endif // QMZIP
