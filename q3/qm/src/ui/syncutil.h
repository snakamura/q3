/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
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
						   NormalFolder* pFolder);
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
					 SubAccount* pSubAccount);
	static bool goRound(SyncManager* pSyncManager,
						Document* pDocument,
						SyncDialogManager* pSyncDialogManager,
						HWND hwnd,
						unsigned int nCallbackParam,
						const GoRoundCourse* pCourse);
};

}

#endif // __SYNCUTIL_H__
