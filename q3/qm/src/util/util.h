/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __UTIL_H__
#define __UTIL_H__

#include <qm.h>
#include <qmaccount.h>

#include <qsregex.h>
#include <qsstring.h>


namespace qm {

class Util;
class RegexValue;

class AccountManager;


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
	static std::pair<Account*, Folder*> getAccountOrFolder(AccountManager* pAccountManager,
														   const WCHAR* pwsz);
	
	static unsigned int getMessageCount(Account* pAccount);
	static unsigned int getUnseenMessageCount(Account* pAccount);
	
	static bool hasFilesOrURIs(IDataObject* pDataObject);
	static void getFilesOrURIs(IDataObject* pDataObject,
							   PathList* pList);
};


/****************************************************************************
 *
 * RegexValue
 *
 */

class RegexValue
{
public:
	RegexValue();
	RegexValue(const WCHAR* pwszRegex,
			   std::auto_ptr<qs::RegexPattern> pRegex);
	RegexValue(const RegexValue& regex);
	~RegexValue();

public:
	RegexValue& operator=(const RegexValue& regex);
	const qs::RegexPattern* operator->() const;

public:
	const WCHAR* getRegex() const;
	const qs::RegexPattern* getRegexPattern() const;
	bool setRegex(const WCHAR* pwszRegex);
	void setRegex(const WCHAR* pwszRegex,
				  std::auto_ptr<qs::RegexPattern> pRegex);
	void assign(RegexValue& regex);

private:
	qs::wstring_ptr wstrRegex_;
	std::auto_ptr<qs::RegexPattern> pRegex_;
};

}

#endif // __UTIL_H__
