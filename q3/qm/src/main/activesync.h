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
class SyncQueue;


/****************************************************************************
 *
 * ActiveSyncInvoker
 *
 */

class ActiveSyncInvoker :
	public AccountHook,
	public DefaultAccountManagerHandler
{
public:
	ActiveSyncInvoker(Document* pDocument,
					  SyncQueue* pSyncQueue);
	virtual ~ActiveSyncInvoker();

public:
	virtual void messageAppended(NormalFolder* pFolder,
								 unsigned int nAppendFlags);
	virtual void messageRemoved(NormalFolder* pFolder,
								unsigned int nRemoveFlags);
	virtual void messageCopied(NormalFolder* pFolderFrom,
							   NormalFolder* pFolderTo,
							   unsigned int nCopyFlags);

public:
	virtual void accountListChanged(const AccountManagerEvent& event);

private:
	void sync(NormalFolder* pFolder);
	bool isSyncSource(NormalFolder* pFolder,
					  unsigned int nOpFlags) const;
	bool isSyncTarget(NormalFolder* pFolder,
					  unsigned int nOpFlags) const;
	bool isSync(NormalFolder* pFolder,
				unsigned int nOpFlags) const;

private:
	static void setHook(Account* pAccount,
						AccountHook* pHook);

private:
	ActiveSyncInvoker(const ActiveSyncInvoker&);
	ActiveSyncInvoker& operator=(const ActiveSyncInvoker&);

private:
	Document* pDocument_;
	SyncQueue* pSyncQueue_;
};

}

#endif // __ACTIVESYNC_H__
