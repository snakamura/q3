/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsconv.h>
#include <qsosutil.h>
#include <qsstl.h>

#include "tempfilecleaner.h"

#pragma warning(disable:4786)

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
}

void qm::TempFileCleaner::add(const WCHAR* pwszPath)
{
	tstring_ptr tstrPath(wcs2tcs(pwszPath));
	
	List::iterator it = std::find_if(list_.begin(), list_.end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				string_equal<TCHAR>(),
				std::select1st<List::value_type>(),
				std::identity<const TCHAR*>()),
			tstrPath.get()));
	if (it != list_.end()) {
		freeTString((*it).first);
		list_.erase(it);
	}
	
	AutoHandle hFile(::CreateFile(tstrPath.get(), GENERIC_READ, 0, 0,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0));
	if (hFile.get()) {
		FILETIME ft;
		BOOL b = ::GetFileTime(hFile.get(), 0, 0, &ft);
		hFile.close();
		if (b) {
			list_.push_back(List::value_type(tstrPath.get(), ft));
			tstrPath.release();
		}
	}
}

void qm::TempFileCleaner::clean(TempFileCleanerCallback* pCallback)
{
	for (List::iterator it = list_.begin(); it != list_.end(); ++it) {
		AutoHandle hFile(::CreateFile((*it).first, GENERIC_READ, 0, 0,
			OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0));
		if (hFile .get()) {
			FILETIME ft;
			BOOL b = ::GetFileTime(hFile.get(), 0, 0, &ft);
			hFile.close();
			if (b) {
				bool bDelete = true;
				if (::CompareFileTime(&(*it).second, &ft) != 0) {
					wstring_ptr wstrPath(tcs2wcs((*it).first));
					bDelete = pCallback->confirmDelete(wstrPath.get());
				}
				if (bDelete)
					::DeleteFile((*it).first);
			}
		}
		freeTString((*it).first);
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
