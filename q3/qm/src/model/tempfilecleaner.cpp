/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
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

qm::TempFileCleaner::TempFileCleaner(QSTATUS* pstatus)
{
}

qm::TempFileCleaner::~TempFileCleaner()
{
}

QSTATUS qm::TempFileCleaner::add(const WCHAR* pwszPath)
{
	DECLARE_QSTATUS();
	
	string_ptr<TSTRING> tstrPath(wcs2tcs(pwszPath));
	if (!tstrPath.get())
		return QSTATUS_OUTOFMEMORY;
	
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
			status = STLWrapper<List>(list_).push_back(
				List::value_type(tstrPath.get(), ft));
			CHECK_QSTATUS();
			tstrPath.release();
		}
	}
	
	return QSTATUS_SUCCESS;
}

void qm::TempFileCleaner::clean(TempFileCleanerCallback* pCallback)
{
	List::iterator it = list_.begin();
	while (it != list_.end()) {
		AutoHandle hFile(::CreateFile((*it).first, GENERIC_READ, 0, 0,
			OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0));
		if (hFile .get()) {
			FILETIME ft;
			BOOL b = ::GetFileTime(hFile.get(), 0, 0, &ft);
			hFile.close();
			if (b) {
				bool bDelete = true;
				if (::CompareFileTime(&(*it).second, &ft) != 0) {
					string_ptr<WSTRING> wstrPath(tcs2wcs((*it).first));
					if (wstrPath.get())
						bDelete = pCallback->confirmDelete(wstrPath.get());
					else
						bDelete = false;
				}
				if (bDelete)
					::DeleteFile((*it).first);
			}
		}
		freeTString((*it).first);
		++it;
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
