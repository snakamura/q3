/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __SYNCQUEUE_H__
#define __SYNCQUEUE_H__

#include <qm.h>
#include <qmaccount.h>

#include <qsstring.h>
#include <qsthread.h>
#include <qswindow.h>

#include "syncmanager.h"


namespace qm {

class SyncQueue;

class Document;
class NormalFolder;
class SyncDialogManager;
class SyncManager;


/****************************************************************************
 *
 * SyncQueue
 *
 */

class SyncQueue
{
public:
	SyncQueue(SyncManager* pSyncManager,
			  Document* pDocument,
			  SyncDialogManager* pSyncDialogManager);
	~SyncQueue();

public:
	void pushFolder(NormalFolder* pFolder,
					bool bCancelable);

private:
	void sync();
	void clear();
	void getFolders(Account::NormalFolderList* pList);

private:
	SyncQueue(const SyncQueue&);
	SyncQueue& operator=(const SyncQueue&);

private:
	enum {
		WM_SYNCQUEUE_SYNC = WM_APP + 1501
	};

private:
	class DynamicSyncData : public SyncData
	{
	public:
		DynamicSyncData(Document* pDocument,
						SyncQueue* pSyncQueue);
		virtual ~DynamicSyncData();
	
	public:
		virtual void getItems(ItemListList* pList);
	
	private:
		DynamicSyncData(const DynamicSyncData&);
		DynamicSyncData& operator=(const DynamicSyncData&);
	
	private:
		SyncQueue* pSyncQueue_;
	};
	friend class DynamicSyncData;
	
	class WindowHandler : public qs::DefaultWindowHandler
	{
	public:
		explicit WindowHandler(SyncQueue* pSyncQueue);
	
	public:
		virtual LRESULT windowProc(UINT uMsg,
								   WPARAM wParam,
								   LPARAM lParam);
	
	protected:
		LRESULT onSync(WPARAM wParam,
					   LPARAM lParam);
	
	private:
		SyncQueue* pSyncQueue_;
	};
	friend class WindowHandler;

private:
	typedef std::vector<std::pair<qs::WSTRING, bool> > FolderList;

private:
	SyncManager* pSyncManager_;
	Document* pDocument_;
	SyncDialogManager* pSyncDialogManager_;
	FolderList listFolder_;
	bool bSyncing_;
	qs::CriticalSection cs_;
	qs::WindowBase* pWindow_;
};

}

#endif // __SYNCQUEUE_H__
