/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
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

qmpop3::Pop3Driver::Pop3Driver(Account* pAccount) :
	pAccount_(pAccount)
{
}

qmpop3::Pop3Driver::~Pop3Driver()
{
}

bool qmpop3::Pop3Driver::init()
{
	return true;
}

bool qmpop3::Pop3Driver::save()
{
	return true;
}

bool qmpop3::Pop3Driver::isSupport(Account::Support support)
{
	switch (support) {
	case Account::SUPPORT_REMOTEFOLDER:
		return false;
	case Account::SUPPORT_LOCALFOLDERDOWNLOAD:
		return true;
	case Account::SUPPORT_LOCALFOLDERGETMESSAGE:
		return false;
	default:
		assert(false);
		return false;
	}
}

void qmpop3::Pop3Driver::setOffline(bool bOffline)
{
}

std::auto_ptr<NormalFolder> qmpop3::Pop3Driver::createFolder(SubAccount* pSubAccount,
															 const WCHAR* pwszName,
															 Folder* pParent)
{
	assert(false);
	return std::auto_ptr<NormalFolder>(0);
}

bool qmpop3::Pop3Driver::removeFolder(SubAccount* pSubAccount,
									  NormalFolder* pFolder)
{
	assert(false);
	return false;
}

bool qmpop3::Pop3Driver::renameFolder(SubAccount* pSubAccount,
									  NormalFolder* pFolder,
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
		{ L"Trash",		Folder::FLAG_LOCAL | Folder::FLAG_TRASHBOX							}
	};
	
	pList->reserve(countof(folders));
	
	for (int n = 0; n < countof(folders); ++n) {
		NormalFolder* pFolder = new NormalFolder(n + 1, folders[n].pwszName_,
			L'/', folders[n].nFlags_, 0, 0, 0, 0, 0, 0, pAccount_);
		pList->push_back(pFolder);
	}
	
	return true;
}

bool qmpop3::Pop3Driver::getRemoteFolders(SubAccount* pSubAccount,
										  RemoteFolderList* pList)
{
	assert(false);
	return false;
}

bool qmpop3::Pop3Driver::getMessage(SubAccount* pSubAccount,
									MessageHolder* pmh,
									unsigned int nFlags,
									xstring_ptr* pstrMessage,
									Message::Flag* pFlag,
									bool* pbMadeSeen)
{
	assert(false);
	return false;
}

bool qmpop3::Pop3Driver::setMessagesFlags(SubAccount* pSubAccount,
										  NormalFolder* pFolder,
										  const MessageHolderList& l,
										  unsigned int nFlags,
										  unsigned int nMask)
{
	assert(false);
	return false;
}

bool qmpop3::Pop3Driver::appendMessage(SubAccount* pSubAccount,
									   NormalFolder* pFolder,
									   const CHAR* pszMessage,
									   unsigned int nFlags)
{
	assert(false);
	return false;
}

bool qmpop3::Pop3Driver::removeMessages(SubAccount* pSubAccount,
										NormalFolder* pFolder,
										const MessageHolderList& l)
{
	assert(false);
	return false;
}

bool qmpop3::Pop3Driver::copyMessages(SubAccount* pSubAccount,
									  const MessageHolderList& l,
									  NormalFolder* pFolderFrom,
									  NormalFolder* pFolderTo,
									  bool bMove)
{
	assert(false);
	return false;
}

bool qmpop3::Pop3Driver::clearDeletedMessages(SubAccount* pSubAccount,
											  NormalFolder* pFolder)
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
																const Security* pSecurity)
{
	assert(pAccount);
	assert(pSecurity);
	
	return std::auto_ptr<ProtocolDriver>(new Pop3Driver(pAccount));
}
