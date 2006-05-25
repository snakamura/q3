/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include "main.h"
#include "rssdriver.h"
#include "resourceinc.h"

using namespace qmrss;
using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * RssDriver
 *
 */

const unsigned int qmrss::RssDriver::nSupport__ =
	Account::SUPPORT_LOCALFOLDERSYNC |
	Account::SUPPORT_LOCALFOLDERDOWNLOAD |
	Account::SUPPORT_EXTERNALLINK;

const WCHAR* qmrss::RssDriver::pwszParamNames__[] = {
	L"URL",
	L"UserName",
	L"Password",
	L"MakeMultipart",
	L"UseDescriptionAsContent",
	L"UpdateIfModified",
	L"Cookie"
};

const WCHAR* qmrss::RssDriver::pwszParamValues__[] = {
	L"",
	L"",
	L"",
	L"true",
	L"false",
	L"false",
	L""
};

qmrss::RssDriver::RssDriver(Account* pAccount) :
	pAccount_(pAccount)
{
}

qmrss::RssDriver::~RssDriver()
{
}

bool qmrss::RssDriver::isSupport(Account::Support support)
{
	return (nSupport__ & support) != 0;
}

bool qmrss::RssDriver::createDefaultFolders(Account::FolderList* pList)
{
	assert(pList);
	
	struct {
		UINT nId_;
		unsigned int nFlags_;
	} folders[] = {
		{ IDS_FOLDER_TRASH,		Folder::FLAG_LOCAL | Folder::FLAG_TRASHBOX | Folder::FLAG_IGNOREUNSEEN	}
	};
	
	pList->reserve(countof(folders));
	
	for (int n = 0; n < countof(folders); ++n) {
		wstring_ptr wstrName(loadString(getResourceHandle(), folders[n].nId_));
		NormalFolder* pFolder = new NormalFolder(n + 1, wstrName.get(),
			L'/', folders[n].nFlags_, 0, 0, 0, 0, 0, 0, pAccount_);
		pList->push_back(pFolder);
	}
	
	return true;
}

std::pair<const WCHAR**, size_t> qmrss::RssDriver::getFolderParamNames(Folder* pFolder)
{
	if (pFolder->isFlag(Folder::FLAG_SYNCABLE))
		return std::pair<const WCHAR**, size_t>(pwszParamNames__, countof(pwszParamNames__));
	else
		return std::pair<const WCHAR**, size_t>(0, 0);
}

void qmrss::RssDriver::setDefaultFolderParams(NormalFolder* pFolder)
{
	if (pFolder->isFlag(Folder::FLAG_SYNCABLE)) {
		for (int n = 0; n < countof(pwszParamNames__); ++n)
			pFolder->setParam(pwszParamNames__[n], pwszParamValues__[n]);
	}
}


/****************************************************************************
*
* RssFactory
*
*/

RssFactory qmrss::RssFactory::factory__;

qmrss::RssFactory::RssFactory()
{
	registerFactory(L"rss", this);
}

qmrss::RssFactory::~RssFactory()
{
	unregisterFactory(L"rss");
}

std::auto_ptr<ProtocolDriver> qmrss::RssFactory::createDriver(Account* pAccount,
															  PasswordCallback* pPasswordCallback,
															  const Security* pSecurity)
{
	assert(pAccount);
	assert(pPasswordCallback);
	assert(pSecurity);
	
	return std::auto_ptr<ProtocolDriver>(new RssDriver(pAccount));
}
