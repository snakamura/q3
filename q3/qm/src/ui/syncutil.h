/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __SYNCUTIL_H__
#define __SYNCUTIL_H__

#include <qm.h>

#include <qs.h>


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
						   HWND hwnd,
						   unsigned int nCallbackParam,
						   NormalFolder* pFolder,
						   unsigned int nFlags);
	static bool syncFolders(SyncManager* pSyncManager,
							Document* pDocument,
							SyncDialogManager* pSyncDialogManager,
							HWND hwnd,
							unsigned int nCallbackParam,
							const Account::NormalFolderList& listFolder,
							unsigned int nFlags);
	static bool sync(SyncManager* pSyncManager,
					 Document* pDocument,
					 SyncDialogManager* pSyncDialogManager,
					 HWND hwnd,
					 unsigned int nCallbackParam,
					 Account* pAccount,
					 bool bSend,
					 bool bReceive,
					 bool bSelectSyncFilter);
	static bool send(SyncManager* pSyncManager,
					 Document* pDocument,
					 SyncDialogManager* pSyncDialogManager,
					 HWND hwnd,
					 unsigned int nCallbackParam,
					 Account* pAccount,
					 SubAccount* pSubAccount,
					 const WCHAR* pwszMessageId);
	static bool goRound(SyncManager* pSyncManager,
						Document* pDocument,
						SyncDialogManager* pSyncDialogManager,
						HWND hwnd,
						unsigned int nCallbackParam,
						const GoRoundCourse* pCourse);
};

}

#endif // __SYNCUTIL_H__
