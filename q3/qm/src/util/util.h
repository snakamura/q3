/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __UTIL_H__
#define __UTIL_H__

#include <qm.h>
#include <qmaccount.h>

#include <qsstring.h>


namespace qm {

class Util;

class Document;


/****************************************************************************
 *
 * Util
 *
 */

class Util
{
public:
	typedef std::vector<qs::WSTRING> PathList;

public:
	static qs::wstring_ptr convertLFtoCRLF(const WCHAR* pwsz);
	static qs::wstring_ptr convertCRLFtoLF(const WCHAR* pwsz);
	
	static qs::wstring_ptr formatAccount(Account* pAccount);
	static qs::wstring_ptr formatFolder(Folder* pFolder);
	static qs::wstring_ptr formatFolders(const Account::FolderList& l,
										 const WCHAR* pwszSeparator);
	static std::pair<Account*, Folder*> getAccountOrFolder(Document* pDocument,
														   const WCHAR* pwsz);
	
	static bool hasFilesOrURIs(IDataObject* pDataObject);
	static void getFilesOrURIs(IDataObject* pDataObject,
							   PathList* pList);
};

}

#endif // __UTIL_H__
