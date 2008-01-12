/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#include <qmdocument.h>

#include <boost/bind.hpp>

#include "activesync.h"
#include "../sync/syncqueue.h"


/****************************************************************************
 *
 * ActiveSyncInvoker
 *
 */

qm::ActiveSyncInvoker::ActiveSyncInvoker(Document* pDocument,
										 SyncQueue* pSyncQueue) :
	pDocument_(pDocument),
	pSyncQueue_(pSyncQueue)
{
	const Document::AccountList& l = pDocument_->getAccounts();
	std::for_each(l.begin(), l.end(),
		boost::bind(&ActiveSyncInvoker::setHook, _1, this));
	pDocument_->addAccountManagerHandler(this);
}

qm::ActiveSyncInvoker::~ActiveSyncInvoker()
{
	const Document::AccountList& l = pDocument_->getAccounts();
	std::for_each(l.begin(), l.end(),
		boost::bind(&ActiveSyncInvoker::setHook, _1, static_cast<AccountHook*>(0)));
	pDocument_->removeAccountManagerHandler(this);
}

void qm::ActiveSyncInvoker::messageAppended(NormalFolder* pFolder,
											unsigned int nAppendFlags)
{
	if (isSyncTarget(pFolder, nAppendFlags))
		sync(pFolder);
}

void qm::ActiveSyncInvoker::messageRemoved(NormalFolder* pFolder,
										   unsigned int nRemoveFlags)
{
	if (isSyncSource(pFolder, nRemoveFlags))
		sync(pFolder);
}

void qm::ActiveSyncInvoker::messageCopied(NormalFolder* pFolderFrom,
										  NormalFolder* pFolderTo,
										  unsigned int nCopyFlags)
{
	if (nCopyFlags & Account::COPYFLAG_MOVE &&
		isSyncSource(pFolderFrom, nCopyFlags))
		sync(pFolderFrom);
	if (isSyncTarget(pFolderTo, nCopyFlags))
		sync(pFolderTo);
}

void qm::ActiveSyncInvoker::accountListChanged(const AccountManagerEvent& event)
{
	switch (event.getType()) {
	case AccountManagerEvent::TYPE_ALL:
		{
			const Document::AccountList& l = pDocument_->getAccounts();
			std::for_each(l.begin(), l.end(),
				boost::bind(&ActiveSyncInvoker::setHook, _1, this));
		}
		break;
	case AccountManagerEvent::TYPE_ADD:
		setHook(event.getAccount(), this);
		break;
	case AccountManagerEvent::TYPE_REMOVE:
		setHook(event.getAccount(), 0);
		break;
	default:
		assert(false);
		break;
	}
}

void qm::ActiveSyncInvoker::sync(NormalFolder* pFolder)
{
	if (!pDocument_->isOffline())
		pSyncQueue_->pushFolder(pFolder, false);
}

bool qm::ActiveSyncInvoker::isSyncSource(NormalFolder* pFolder,
										 unsigned int nOpFlags) const
{
	return isSync(pFolder, nOpFlags);
}

bool qm::ActiveSyncInvoker::isSyncTarget(NormalFolder* pFolder,
										 unsigned int nOpFlags) const
{
	return isSync(pFolder, nOpFlags);
}

bool qm::ActiveSyncInvoker::isSync(NormalFolder* pFolder,
								   unsigned int nOpFlags) const
{
	assert(pFolder);
	
	return nOpFlags & Account::OPFLAG_ACTIVE &&
		!pFolder->isFlag(Folder::FLAG_LOCAL) &&
		pFolder->isFlag(Folder::FLAG_SYNCABLE) &&
		pFolder->isFlag(Folder::FLAG_ACTIVESYNC);
}

void qm::ActiveSyncInvoker::setHook(Account* pAccount,
									AccountHook* pHook)
{
	assert(pAccount);
	
	if (pAccount->isSupport(Account::SUPPORT_REMOTEFOLDER))
		pAccount->setHook(pHook);
}
