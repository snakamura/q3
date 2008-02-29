/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#pragma warning(disable:4786)

#include <qsconv.h>
#include <qsfile.h>
#include <qsosutil.h>
#include <qsstl.h>

#include <boost/bind.hpp>

#include "tempfilecleaner.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * TempFileCleaner
 *
 */

qm::TempFileCleaner::TempFileCleaner()
{
}

qm::TempFileCleaner::~TempFileCleaner()
{
	std::for_each(listFile_.begin(), listFile_.end(),
		boost::bind(&freeWString, boost::bind(&FileList::value_type::first, _1)));
	std::for_each(listDirectory_.begin(), listDirectory_.end(), &freeWString);
}

void qm::TempFileCleaner::addFile(const WCHAR* pwszPath)
{
	assert(pwszPath);
	
	FileList::iterator it = std::find_if(
		listFile_.begin(), listFile_.end(),
		boost::bind(string_equal<WCHAR>(),
			boost::bind(&FileList::value_type::first, _1), pwszPath));
	if (it != listFile_.end()) {
		freeWString((*it).first);
		listFile_.erase(it);
	}
	
	W2T(pwszPath, ptszPath);
	WIN32_FIND_DATA fd;
	AutoFindHandle hFind(::FindFirstFile(ptszPath, &fd));
	if (hFind.get()) {
		wstring_ptr wstrPath(allocWString(pwszPath));
		listFile_.push_back(FileList::value_type(wstrPath.get(), fd.ftLastWriteTime));
		wstrPath.release();
	}
}

void qm::TempFileCleaner::addDirectory(const WCHAR* pwszPath)
{
	assert(pwszPath);
	
	wstring_ptr wstrPath(allocWString(pwszPath));
	listDirectory_.push_back(wstrPath.get());
	wstrPath.release();
}

bool qm::TempFileCleaner::isModified(const WCHAR* pwszPath) const
{
	assert(pwszPath);
	
	FileList::const_iterator it = std::find_if(
		listFile_.begin(), listFile_.end(),
		boost::bind(string_equal<WCHAR>(),
			boost::bind(&FileList::value_type::first, _1), pwszPath));
	if (it == listFile_.end())
		return false;
	
	W2T(pwszPath, ptszPath);
	WIN32_FIND_DATA fd;
	AutoFindHandle hFind(::FindFirstFile(ptszPath, &fd));
	return hFind.get() && ::CompareFileTime(&(*it).second, &fd.ftLastWriteTime) != 0;
}

void qm::TempFileCleaner::clean(TempFileCleanerCallback* pCallback)
{
	for (FileList::iterator it = listFile_.begin(); it != listFile_.end(); ++it) {
		wstring_ptr wstrPath((*it).first);
		W2T(wstrPath.get(), ptszPath);
		WIN32_FIND_DATA fd;
		AutoFindHandle hFind(::FindFirstFile(ptszPath, &fd));
		if (hFind.get()) {
			hFind.close();
			if (::CompareFileTime(&(*it).second, &fd.ftLastWriteTime) == 0 ||
				pCallback->confirmDelete(wstrPath.get()))
				::DeleteFile(ptszPath);
		}
	}
	listFile_.clear();
	
	for (DirectoryList::iterator it = listDirectory_.begin(); it != listDirectory_.end(); ++it) {
		wstring_ptr wstrPath(*it);
		File::removeDirectory(wstrPath.get());
	}
	listDirectory_.clear();
}


/****************************************************************************
 *
 * TempFileCleanerCallback
 *
 */

qm::TempFileCleanerCallback::~TempFileCleanerCallback()
{
}
