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

#include "pop3driver.h"

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

qmpop3::Pop3Driver::Pop3Driver(Account* pAccount) :
	pAccount_(pAccount)
{
}

qmpop3::Pop3Driver::~Pop3Driver()
{
}

bool qmpop3::Pop3Driver::init()
{
#ifndef _WIN32_WCE
	// For compatibility.
	// Remove in the future version.
	if (!pAccount_->getFolderByBoxFlag(Folder::FLAG_JUNKBOX)) {
		NormalFolder* pJunkbox = pAccount_->createNormalFolder(L"Junk", 0, false, false);
		if (pJunkbox)
			pAccount_->setFolderFlags(pJunkbox, Folder::FLAG_JUNKBOX, Folder::FLAG_JUNKBOX);
	}
#endif
	
	return true;
}

bool qmpop3::Pop3Driver::save(bool bForce)
{
	return true;
}

bool qmpop3::Pop3Driver::isSupport(Account::Support support)
{
	return (nSupport__ & support) != 0;
}

void qmpop3::Pop3Driver::setOffline(bool bOffline)
{
}

void qmpop3::Pop3Driver::setSubAccount(SubAccount* pSubAccount)
{
}

std::auto_ptr<NormalFolder> qmpop3::Pop3Driver::createFolder(const WCHAR* pwszName,
															 Folder* pParent)
{
	assert(false);
	return std::auto_ptr<NormalFolder>(0);
}

bool qmpop3::Pop3Driver::removeFolder(NormalFolder* pFolder)
{
	assert(false);
	return false;
}

bool qmpop3::Pop3Driver::renameFolder(NormalFolder* pFolder,
									  const WCHAR* pwszName)
{
	assert(false);
	return false;
}

bool qmpop3::Pop3Driver::moveFolder(NormalFolder* pFolder,
									NormalFolder* pParent,
									const WCHAR* pwszName)
{
	assert(false);
	return false;
}

bool qmpop3::Pop3Driver::createDefaultFolders(Account::FolderList* pList)
{
	assert(pList);
	
	struct {
		const WCHAR* pwszName_;
		unsigned int nFlags_;
	} folders[] = {
		{ L"Inbox",		Folder::FLAG_LOCAL | Folder::FLAG_INBOX | Folder::FLAG_SYNCABLE		},
		{ L"Outbox",	Folder::FLAG_LOCAL | Folder::FLAG_OUTBOX | Folder::FLAG_DRAFTBOX	},
		{ L"Sentbox",	Folder::FLAG_LOCAL | Folder::FLAG_SENTBOX							},
		{ L"Trash",		Folder::FLAG_LOCAL | Folder::FLAG_TRASHBOX							},
#ifndef _WIN32_WCE
		{ L"Junk",		Folder::FLAG_LOCAL | Folder::FLAG_JUNKBOX							}
#endif
	};
	
	pList->reserve(countof(folders));
	
	for (int n = 0; n < countof(folders); ++n) {
		NormalFolder* pFolder = new NormalFolder(n + 1, folders[n].pwszName_,
			L'/', folders[n].nFlags_, 0, 0, 0, 0, 0, 0, pAccount_);
		pList->push_back(pFolder);
	}
	
	return true;
}

bool qmpop3::Pop3Driver::getRemoteFolders(RemoteFolderList* pList)
{
	assert(false);
	return false;
}

std::pair<const WCHAR**, size_t> qmpop3::Pop3Driver::getFolderParamNames()
{
	return std::pair<const WCHAR**, size_t>(0, 0);
}

void qmpop3::Pop3Driver::setDefaultFolderParams(NormalFolder* pFolder)
{
}

bool qmpop3::Pop3Driver::getMessage(MessageHolder* pmh,
									unsigned int nFlags,
									GetMessageCallback* pCallback)
{
	assert(false);
	return false;
}

bool qmpop3::Pop3Driver::setMessagesFlags(NormalFolder* pFolder,
										  const MessageHolderList& l,
										  unsigned int nFlags,
										  unsigned int nMask)
{
	assert(false);
	return false;
}

bool qmpop3::Pop3Driver::appendMessage(NormalFolder* pFolder,
									   const CHAR* pszMessage,
									   size_t nLen,
									   unsigned int nFlags)
{
	assert(false);
	return false;
}

bool qmpop3::Pop3Driver::removeMessages(NormalFolder* pFolder,
										const MessageHolderList& l)
{
	assert(false);
	return false;
}

bool qmpop3::Pop3Driver::copyMessages(const MessageHolderList& l,
									  NormalFolder* pFolderFrom,
									  NormalFolder* pFolderTo,
									  bool bMove)
{
	assert(false);
	return false;
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
