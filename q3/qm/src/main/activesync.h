/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __ACTIVESYNC_H__
#define __ACTIVESYNC_H__

#include <qm.h>
#include <qmaccount.h>


namespace qm {

class ActiveSyncInvoker;

class Document;
class NormalFolder;
class SyncManager;
class SyncDialogManager;


/****************************************************************************
 *
 * ActiveSyncInvoker
 *
 */

class ActiveSyncInvoker :
	public AccountHook,
	public AccountManagerHandler
{
public:
	ActiveSyncInvoker(Document* pDocument,
					  SyncManager* pSyncManager,
					  SyncDialogManager* pSyncDialogManager);
	virtual ~ActiveSyncInvoker();

public:
	virtual void messageAppended(NormalFolder* pFolder,
								 unsigned int nAppendFlags);
	virtual void messageCopied(NormalFolder* pFolderFrom,
							   NormalFolder* pFolderTo,
							   unsigned int nCopyFlags);

public:
	virtual void accountListChanged(const AccountManagerEvent& event);

private:
	void sync(NormalFolder* pFolder);

private:
	ActiveSyncInvoker(const ActiveSyncInvoker&);
	ActiveSyncInvoker& operator=(const ActiveSyncInvoker&);

private:
	Document* pDocument_;
	SyncManager* pSyncManager_;
	SyncDialogManager* pSyncDialogManager_;
};

}

#endif // __ACTIVESYNC_H__
