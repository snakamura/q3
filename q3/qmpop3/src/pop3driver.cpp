/*
 * $Id: pop3driver.cpp,v 1.1.1.1 2003/04/29 08:07:34 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmaccount.h>
#include <qmfolder.h>
#include <qmmessageholder.h>

#include <qsassert.h>
#include <qserror.h>
#include <qsnew.h>

#include "pop3driver.h"

using namespace qmpop3;
using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * Pop3Driver
 *
 */

qmpop3::Pop3Driver::Pop3Driver(Account* pAccount, QSTATUS* pstatus) :
	pAccount_(pAccount)
{
	assert(pstatus);
	
	*pstatus = QSTATUS_SUCCESS;
}

qmpop3::Pop3Driver::~Pop3Driver()
{
}

bool qmpop3::Pop3Driver::isSupport(Account::Support support)
{
	switch (support) {
	case Account::SUPPORT_REMOTEFOLDER:
		return false;
	case Account::SUPPORT_LOCALFOLDERDOWNLOAD:
		return true;
	default:
		assert(false);
		return false;
	}
}

QSTATUS qmpop3::Pop3Driver::setOffline(bool bOffline)
{
	return QSTATUS_SUCCESS;
}

QSTATUS qmpop3::Pop3Driver::save()
{
	return QSTATUS_SUCCESS;
}

QSTATUS qmpop3::Pop3Driver::createFolder(SubAccount* pSubAccount,
	const WCHAR* pwszName, Folder* pParent, NormalFolder** ppFolder)
{
	assert(false);
	return QSTATUS_FAIL;
}

QSTATUS qmpop3::Pop3Driver::createDefaultFolders(
	Folder*** pppFolder, size_t* pnCount)
{
	DECLARE_QSTATUS();
	
	*pppFolder = 0;
	*pnCount = 0;
	
	struct {
		const WCHAR* pwszName_;
		unsigned int nFlags_;
	} folders[] = {
		{ L"Inbox",		Folder::FLAG_LOCAL | Folder::FLAG_INBOX | Folder::FLAG_SYNCABLE		},
		{ L"Outbox",	Folder::FLAG_LOCAL | Folder::FLAG_OUTBOX | Folder::FLAG_DRAFTBOX	},
		{ L"Sentbox",	Folder::FLAG_LOCAL | Folder::FLAG_SENTBOX							},
		{ L"Trash",		Folder::FLAG_LOCAL | Folder::FLAG_TRASHBOX							}
	};
	
	malloc_ptr<Folder*> pFolder(static_cast<Folder**>(
		malloc(countof(folders)*sizeof(Folder*))));
	if (!pFolder.get())
		return QSTATUS_OUTOFMEMORY;
	
	for (int n = 0; n < countof(folders); ++n) {
		NormalFolder::Init init;
		init.nId_ = n;
		init.pwszName_ = folders[n].pwszName_;
		init.cSeparator_ = L'/';
		init.nFlags_ = folders[n].nFlags_;
		init.nCount_ = 0;
		init.nUnseenCount_ = 0;
		init.pParentFolder_ = 0;
		init.pAccount_ = pAccount_;
		init.nValidity_ = 0;
		init.nDownloadCount_ = 0;
		
		NormalFolder* p = 0;
		status = newQsObject(init, &p);
		CHECK_QSTATUS();
		*(pFolder.get() + n) = p;
	}
	
	*pppFolder = pFolder.release();
	*pnCount = countof(folders);
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmpop3::Pop3Driver::getRemoteFolders(SubAccount* pSubAccount,
	std::pair<Folder*, bool>** ppFolder, size_t* pnCount)
{
	assert(pSubAccount);
	assert(ppFolder);
	assert(pnCount);
	
	*ppFolder = 0;
	*pnCount = 0;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmpop3::Pop3Driver::getMessage(SubAccount* pSubAccount,
	MessageHolder* pmh, unsigned int nFlags, Message* pMessage,
	bool* pbGet, bool* pbMadeSeen)
{
	assert(pSubAccount);
	assert(pmh);
	assert(pMessage);
	assert(pbGet);
	assert(pbMadeSeen);
	
	*pbGet = false;
	*pbMadeSeen = false;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmpop3::Pop3Driver::setMessagesFlags(SubAccount* pSubAccount,
	NormalFolder* pFolder, const Folder::MessageHolderList& l,
	unsigned int nFlags, unsigned int nMask)
{
	assert(pSubAccount);
	assert(pFolder);
	assert(!l.empty());
	assert(std::find_if(l.begin(), l.end(),
		std::not1(
			std::bind2nd(
				binary_compose_f_gx_hy(
					std::equal_to<Folder*>(),
					std::mem_fun(&MessageHolder::getFolder),
					std::identity<Folder*>()),
				pFolder))) == l.end());
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmpop3::Pop3Driver::appendMessage(SubAccount* pSubAccount,
	NormalFolder* pFolder, const CHAR* pszMessage, unsigned int nFlags)
{
	assert(pSubAccount);
	assert(pFolder);
	assert(pszMessage);
	
	return QSTATUS_FAIL;
}

QSTATUS qmpop3::Pop3Driver::removeMessages(SubAccount* pSubAccount,
	NormalFolder* pFolder, const Folder::MessageHolderList& l)
{
	assert(pSubAccount);
	assert(pFolder);
	assert(!l.empty());
	assert(std::find_if(l.begin(), l.end(),
		std::not1(
			std::bind2nd(
				binary_compose_f_gx_hy(
					std::equal_to<Folder*>(),
					std::mem_fun(&MessageHolder::getFolder),
					std::identity<Folder*>()),
				pFolder))) == l.end());
	
	// TODO
	
	return QSTATUS_FAIL;
}

QSTATUS qmpop3::Pop3Driver::copyMessages(SubAccount* pSubAccount,
	const Folder::MessageHolderList& l, NormalFolder* pFolderFrom,
	NormalFolder* pFolderTo, bool bMove)
{
	assert(!l.empty());
	assert(pFolderFrom);
	assert(pFolderTo);
	assert(std::find_if(l.begin(), l.end(),
		std::not1(
			std::bind2nd(
				binary_compose_f_gx_hy(
					std::equal_to<Folder*>(),
					std::mem_fun(&MessageHolder::getFolder),
					std::identity<Folder*>()),
				pFolderFrom))) == l.end());
	
	return QSTATUS_FAIL;
}

QSTATUS qmpop3::Pop3Driver::clearDeletedMessages(
	SubAccount* pSubAccount, NormalFolder* pFolder)
{
	return QSTATUS_FAIL;
}


/****************************************************************************
 *
 * Pop3Factory
 *
 */

Pop3Factory qmpop3::Pop3Factory::factory__;

qmpop3::Pop3Factory::Pop3Factory()
{
	regist(L"pop3", this);
}

qmpop3::Pop3Factory::~Pop3Factory()
{
	unregist(L"pop3");
}

QSTATUS qmpop3::Pop3Factory::createDriver(
	Account* pAccount, ProtocolDriver** ppProtocolDriver)
{
	assert(pAccount);
	assert(ppProtocolDriver);
	
	DECLARE_QSTATUS();
	
	Pop3Driver* pDriver = 0;
	status = newQsObject(pAccount, &pDriver);
	CHECK_QSTATUS();
	
	*ppProtocolDriver = pDriver;
	
	return QSTATUS_SUCCESS;
}
