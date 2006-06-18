/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmdocument.h>

#include "activesync.h"
#include "../ui/syncutil.h"


/****************************************************************************
 *
 * ActiveSyncInvoker
 *
 */

qm::ActiveSyncInvoker::ActiveSyncInvoker(Document* pDocument,
										 SyncManager* pSyncManager,
										 SyncDialogManager* pSyncDialogManager) :
	pDocument_(pDocument),
	pSyncManager_(pSyncManager),
	pSyncDialogManager_(pSyncDialogManager)
{
	const Document::AccountList& l = pDocument_->getAccounts();
	std::for_each(l.begin(), l.end(),
		std::bind2nd(std::mem_fun(&Account::setHook), this));
	pDocument_->addAccountManagerHandler(this);
}

qm::ActiveSyncInvoker::~ActiveSyncInvoker()
{
	const Document::AccountList& l = pDocument_->getAccounts();
	std::for_each(l.begin(), l.end(),
		std::bind2nd(std::mem_fun(&Account::setHook), 0));
	pDocument_->removeAccountManagerHandler(this);
}

void qm::ActiveSyncInvoker::messageAppended(NormalFolder* pFolder,
											unsigned int nAppendFlags)
{
	if (nAppendFlags & Account::OPFLAG_ACTIVE)
		sync(pFolder);
}

void qm::ActiveSyncInvoker::messageCopied(NormalFolder* pFolderFrom,
										  NormalFolder* pFolderTo,
										  unsigned int nCopyFlags)
{
	if (nCopyFlags & Account::OPFLAG_ACTIVE)
		sync(pFolderTo);
}

void qm::ActiveSyncInvoker::accountListChanged(const AccountManagerEvent& event)
{
	switch (event.getType()) {
	case AccountManagerEvent::TYPE_ALL:
		{
			const Document::AccountList& l = pDocument_->getAccounts();
			std::for_each(l.begin(), l.end(),
				std::bind2nd(std::mem_fun(&Account::setHook), this));
		}
		break;
	case AccountManagerEvent::TYPE_ADD:
		event.getAccount()->setHook(this);
		break;
	case AccountManagerEvent::TYPE_REMOVE:
		event.getAccount()->setHook(0);
		break;
	default:
		assert(false);
		break;
	}
}

void qm::ActiveSyncInvoker::sync(NormalFolder* pFolder)
{
	if (pDocument_->isOffline())
		return;
	
	if (pFolder->isFlag(Folder::FLAG_SYNCABLE) &&
		pFolder->isFlag(Folder::FLAG_SYNCWHENOPEN)) {
		SyncUtil::syncFolder(pSyncManager_, pDocument_,
			pSyncDialogManager_, SyncData::TYPE_ACTIVE, pFolder, 0);
	}
}
