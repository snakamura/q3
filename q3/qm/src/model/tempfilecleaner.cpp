/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#pragma warning(disable:4786)

#include <qsconv.h>
#include <qsosutil.h>
#include <qsstl.h>

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
	std::for_each(list_.begin(), list_.end(),
		unary_compose_f_gx(string_free<WSTRING>(),
			std::select1st<List::value_type>()));
}

void qm::TempFileCleaner::add(const WCHAR* pwszPath)
{
	List::iterator it = std::find_if(list_.begin(), list_.end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				string_equal<WCHAR>(),
				std::select1st<List::value_type>(),
				std::identity<const WCHAR*>()),
			pwszPath));
	if (it != list_.end()) {
		freeWString((*it).first);
		list_.erase(it);
	}
	
	W2T(pwszPath, ptszPath);
	WIN32_FIND_DATA fd;
	AutoFindHandle hFind(::FindFirstFile(ptszPath, &fd));
	if (hFind.get()) {
		wstring_ptr wstrPath(allocWString(pwszPath));
		list_.push_back(List::value_type(wstrPath.get(), fd.ftLastWriteTime));
		wstrPath.release();
	}
}

bool qm::TempFileCleaner::isModified(const WCHAR* pwszPath) const
{
	List::const_iterator it = std::find_if(list_.begin(), list_.end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				string_equal<WCHAR>(),
				std::select1st<List::value_type>(),
				std::identity<const WCHAR*>()),
			pwszPath));
	if (it == list_.end())
		return false;
	
	W2T(pwszPath, ptszPath);
	WIN32_FIND_DATA fd;
	AutoFindHandle hFind(::FindFirstFile(ptszPath, &fd));
	return hFind.get() && ::CompareFileTime(&(*it).second, &fd.ftLastWriteTime) != 0;
}

void qm::TempFileCleaner::clean(TempFileCleanerCallback* pCallback)
{
	for (List::iterator it = list_.begin(); it != list_.end(); ++it) {
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
	list_.clear();
}


/****************************************************************************
 *
 * TempFileCleanerCallback
 *
 */

qm::TempFileCleanerCallback::~TempFileCleanerCallback()
{
}
