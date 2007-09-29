/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __SYNCUTIL_H__
#define __SYNCUTIL_H__

#include <qm.h>

#include <qs.h>

#include "../sync/syncmanager.h"


namespace qm {

class SyncUtil;

class Document;
class GoRoundCourse;
class NormalFolder;
class SyncData;
class SyncDialogManager;
class SyncManager;


/****************************************************************************
 *
 * SyncUtil
 *
 */

class SyncUtil
{
public:
	static bool syncFolder(SyncManager* pSyncManager,
						   Document* pDocument,
						   SyncDialogManager* pSyncDialogManager,
						   SyncData::Type type,
						   NormalFolder* pFolder,
						   unsigned int nFlags);
	static bool syncFolders(SyncManager* pSyncManager,
							Document* pDocument,
							SyncDialogManager* pSyncDialogManager,
							SyncData::Type type,
							const Account::NormalFolderList& listFolder,
							unsigned int nFlags);
	static bool sync(SyncManager* pSyncManager,
					 Document* pDocument,
					 SyncDialogManager* pSyncDialogManager,
					 SyncData::Type type,
					 Account* pAccount,
					 bool bSend,
					 bool bReceive,
					 bool bSelectSyncFilter,
					 HWND hwnd);
	static bool send(SyncManager* pSyncManager,
					 Document* pDocument,
					 SyncDialogManager* pSyncDialogManager,
					 SyncData::Type type,
					 Account* pAccount,
					 SubAccount* pSubAccount,
					 const WCHAR* pwszMessageId);
	static bool applyRules(SyncManager* pSyncManager,
						   Document* pDocument,
						   SyncDialogManager* pSyncDialogManager,
						   const Account::FolderList& listFolder);
	static bool goRound(SyncManager* pSyncManager,
						Document* pDocument,
						SyncDialogManager* pSyncDialogManager,
						SyncData::Type type,
						const GoRoundCourse* pCourse);
	static bool syncData(SyncManager* pSyncManager,
						 SyncDialogManager* pSyncDialogManager,
						 const SubAccount* pSubAccount,
						 std::auto_ptr<SyncData> pSyncData);

private:
	static void setDialup(SyncData* pSyncData,
						  const SubAccount* pSubAccount);
};

}

#endif // __SYNCUTIL_H__
