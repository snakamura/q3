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
	void pushFolder(NormalFolder* pFolder);
	void pushFolders(const Account::NormalFolderList& listFolder);

private:
	void sync();
	void clear();

private:
	SyncQueue(const SyncQueue&);
	SyncQueue& operator=(const SyncQueue&);

private:
	enum {
		WM_SYNCQUEUE_SYNC = WM_APP + 1501
	};
	
	enum {
		TIMER_ID	= 1001,
		TIMER_DELAY	= 500
	};

private:
	class WindowHandler : public qs::DefaultWindowHandler
	{
	public:
		explicit WindowHandler(SyncQueue* pSyncQueue);
	
	public:
		virtual LRESULT windowProc(UINT uMsg,
								   WPARAM wParam,
								   LPARAM lParam);
	
	protected:
		LRESULT onTimer(UINT_PTR nId);
		LRESULT onSync(WPARAM wParam,
					   LPARAM lParam);
	
	private:
		SyncQueue* pSyncQueue_;
	};
	friend class WindowHandler;

private:
	typedef std::vector<qs::WSTRING> FolderNameList;

private:
	SyncManager* pSyncManager_;
	Document* pDocument_;
	SyncDialogManager* pSyncDialogManager_;
	FolderNameList listFolderName_;
	qs::CriticalSection cs_;
	qs::WindowBase* pWindow_;
};

}

#endif // __SYNCQUEUE_H__
