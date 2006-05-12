/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmaccount.h>
#include <qmfolder.h>
#include <qmmessageholder.h>

#include <qsassert.h>

#include "main.h"
#include "pop3driver.h"
#include "resourceinc.h"

using namespace qmpop3;
using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * Pop3Driver
 *
 */

const unsigned int qmpop3::Pop3Driver::nSupport__ =
	Account::SUPPORT_LOCALFOLDERDOWNLOAD | Account::SUPPORT_JUNKFILTER;

const WCHAR* qmpop3::Pop3Driver::pwszParamNames__[] = {
	L"To"
};

qmpop3::Pop3Driver::Pop3Driver(Account* pAccount) :
	pAccount_(pAccount)
{
}

qmpop3::Pop3Driver::~Pop3Driver()
{
}

bool qmpop3::Pop3Driver::isSupport(Account::Support support)
{
	return (nSupport__ & support) != 0;
}

bool qmpop3::Pop3Driver::createDefaultFolders(Account::FolderList* pList)
{
	assert(pList);
	
	struct {
		UINT nId_;
		unsigned int nFlags_;
	} folders[] = {
		{ IDS_FOLDER_INBOX,		Folder::FLAG_LOCAL | Folder::FLAG_INBOX | Folder::FLAG_SYNCABLE			},
		{ IDS_FOLDER_OUTBOX,	Folder::FLAG_LOCAL | Folder::FLAG_OUTBOX | Folder::FLAG_DRAFTBOX		},
		{ IDS_FOLDER_SENTBOX,	Folder::FLAG_LOCAL | Folder::FLAG_SENTBOX								},
		{ IDS_FOLDER_TRASH,		Folder::FLAG_LOCAL | Folder::FLAG_TRASHBOX | Folder::FLAG_IGNOREUNSEEN	},
#ifndef _WIN32_WCE
		{ IDS_FOLDER_JUNK,		Folder::FLAG_LOCAL | Folder::FLAG_JUNKBOX | Folder::FLAG_IGNOREUNSEEN	}
#endif
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

std::pair<const WCHAR**, size_t> qmpop3::Pop3Driver::getFolderParamNames(Folder* pFolder)
{
	if (!(pFolder->getFlags() & Folder::FLAG_BOX_MASK))
		return std::pair<const WCHAR**, size_t>(pwszParamNames__, countof(pwszParamNames__));
	else
		return std::pair<const WCHAR**, size_t>(0, 0);
}


/****************************************************************************
 *
 * Pop3Factory
 *
 */

Pop3Factory qmpop3::Pop3Factory::factory__;

qmpop3::Pop3Factory::Pop3Factory()
{
	registerFactory(L"pop3", this);
}

qmpop3::Pop3Factory::~Pop3Factory()
{
	unregisterFactory(L"pop3");
}

std::auto_ptr<ProtocolDriver> qmpop3::Pop3Factory::createDriver(Account* pAccount,
																PasswordCallback* pPasswordCallback,
																const Security* pSecurity)
{
	assert(pAccount);
	assert(pPasswordCallback);
	assert(pSecurity);
	
	return std::auto_ptr<ProtocolDriver>(new Pop3Driver(pAccount));
}
