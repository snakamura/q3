/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include "rssdriver.h"

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

const WCHAR* qmrss::RssDriver::pwszParams__[] = {
	L"URL"
};

qmrss::RssDriver::RssDriver(Account* pAccount) :
	pAccount_(pAccount)
{
}

qmrss::RssDriver::~RssDriver()
{
}

bool qmrss::RssDriver::init()
{
	return true;
}

bool qmrss::RssDriver::save()
{
	return true;
}

bool qmrss::RssDriver::isSupport(Account::Support support)
{
	return (nSupport__ & support) != 0;
}

void qmrss::RssDriver::setOffline(bool bOffline)
{
}

std::auto_ptr<NormalFolder> qmrss::RssDriver::createFolder(SubAccount* pSubAccount,
														   const WCHAR* pwszName,
														   Folder* pParent)
{
	assert(false);
	return std::auto_ptr<NormalFolder>(0);
}

bool qmrss::RssDriver::removeFolder(SubAccount* pSubAccount,
									NormalFolder* pFolder)
{
	assert(false);
	return false;
}

bool qmrss::RssDriver::renameFolder(SubAccount* pSubAccount,
									NormalFolder* pFolder,
									const WCHAR* pwszName)
{
	assert(false);
	return false;
}

bool qmrss::RssDriver::createDefaultFolders(Account::FolderList* pList)
{
	assert(pList);
	
	struct {
		const WCHAR* pwszName_;
		unsigned int nFlags_;
	} folders[] = {
//		{ L"Inbox",		Folder::FLAG_LOCAL | Folder::FLAG_INBOX | Folder::FLAG_SYNCABLE		},
//		{ L"Outbox",	Folder::FLAG_LOCAL | Folder::FLAG_OUTBOX | Folder::FLAG_DRAFTBOX	},
//		{ L"Sentbox",	Folder::FLAG_LOCAL | Folder::FLAG_SENTBOX							},
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

bool qmrss::RssDriver::getRemoteFolders(SubAccount* pSubAccount,
										RemoteFolderList* pList)
{
	assert(false);
	return false;
}

std::pair<const WCHAR**, size_t> qmrss::RssDriver::getFolderParamNames()
{
	return std::pair<const WCHAR**, size_t>(pwszParams__, countof(pwszParams__));
}

bool qmrss::RssDriver::getMessage(SubAccount* pSubAccount,
								  MessageHolder* pmh,
								  unsigned int nFlags,
								  xstring_ptr* pstrMessage,
								  Message::Flag* pFlag,
								  bool* pbMadeSeen)
{
	assert(false);
	return false;
}

bool qmrss::RssDriver::setMessagesFlags(SubAccount* pSubAccount,
										NormalFolder* pFolder,
										const MessageHolderList& l,
										unsigned int nFlags,
										unsigned int nMask)
{
	assert(false);
	return false;
}

bool qmrss::RssDriver::appendMessage(SubAccount* pSubAccount,
									 NormalFolder* pFolder,
									 const CHAR* pszMessage,
									 unsigned int nFlags)
{
	assert(false);
	return false;
}

bool qmrss::RssDriver::removeMessages(SubAccount* pSubAccount,
									  NormalFolder* pFolder,
									  const MessageHolderList& l)
{
	assert(false);
	return false;
}

bool qmrss::RssDriver::copyMessages(SubAccount* pSubAccount,
									const MessageHolderList& l,
									NormalFolder* pFolderFrom,
									NormalFolder* pFolderTo,
									bool bMove)
{
	assert(false);
	return false;
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
															  const Security* pSecurity)
{
	assert(pAccount);
	assert(pSecurity);
	
	return std::auto_ptr<ProtocolDriver>(new RssDriver(pAccount));
}
