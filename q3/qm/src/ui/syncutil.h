/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
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
	static qs::QSTATUS syncFolder(SyncManager* pSyncManager,
		Document* pDocument, SyncDialogManager* pSyncDialogManager,
		HWND hwnd, unsigned int nCallbackParam, NormalFolder* pFolder);
	static qs::QSTATUS send(SyncManager* pSyncManager, Document* pDocument,
		SyncDialogManager* pSyncDialogManager, HWND hwnd,
		unsigned int nCallbackParam, Account* pAccount, SubAccount* pSubAccount);
	static qs::QSTATUS createGoRoundData(const GoRoundCourse* pCourse,
		Document* pDocument, SyncData* pData);
};

}

#endif // __SYNCUTIL_H__
