/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __SYNCDIALOG_H__
#define __SYNCDIALOG_H__

#include <qm.h>

#include <qs.h>
#include <qsdevicecontext.h>
#include <qsdialog.h>
#include <qsprofile.h>
#include <qsthread.h>

#include "../sync/syncmanager.h"


namespace qm {

class SyncDialogManager;
class SyncDialog;
class SyncStatusWindow;
class SyncDialogThread;


/****************************************************************************
 *
 * SyncDialogManager
 *
 */

class SyncDialogManager
{
public:
	SyncDialogManager(qs::Profile* pProfile, qs::QSTATUS* pstatus);
	~SyncDialogManager();

public:
	qs::QSTATUS open(SyncDialog** ppSyncDialog);

private:
	SyncDialogManager(const SyncDialogManager&);
	SyncDialogManager& operator=(const SyncDialogManager&);

private:
	qs::Profile* pProfile_;
	SyncDialogThread* pThread_;
};


/****************************************************************************
 *
 * SyncDialog
 *
 */

class SyncDialog :
	public qs::Dialog,
	public qs::DefaultDialogHandler,
	public qs::DefaultCommandHandler
{
public:
	enum {
		WM_SYNCDIALOG_SHOWDIALUPDIALOG = WM_APP + 1001
	};

public:
	SyncDialog(qs::Profile* pProfile, qs::QSTATUS* pstatus);
	virtual ~SyncDialog();

public:
	SyncManagerCallback* getSyncManagerCallback() const;

public:
	void show();
	void hide();
	void setMessage(const WCHAR* pwszMessage);
	unsigned int getCanceledTime() const;
	void resetCanceledTime();
	qs::QSTATUS addError(const WCHAR* pwszError);
	bool hasError() const;

public:
	virtual INT_PTR dialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

public:
	virtual LRESULT onCommand(WORD nCode, WORD nId);

protected:
	LRESULT onClose();
	LRESULT onDestroy();
	LRESULT onInitDialog(HWND hwndFocus, LPARAM lParam);
	LRESULT onSize(UINT nFlags, int cx, int cy);
	LRESULT onShowDialupDialog(WPARAM wParam, LPARAM lParam);

private:
	LRESULT onCancel();
	LRESULT onHide();

private:
	void layout();
	void layout(int cx, int cy);
	qs::QSTATUS showDialupDialog(RASDIALPARAMS* prdp, bool* pbCancel);

private:
	SyncDialog(const SyncDialog&);
	SyncDialog& operator=(const SyncDialog&);

private:
	SyncDialogManager* pManager_;
	qs::Profile* pProfile_;
	SyncStatusWindow* pStatusWindow_;
	bool bShowError_;
	qs::CriticalSection csError_;
	volatile unsigned int nCanceledTime_;
};


/****************************************************************************
 *
 * SyncStatusWindow
 *
 */

class SyncStatusWindow :
	public qs::WindowBase,
	public qs::DefaultWindowHandler,
	public SyncManagerCallback
{
private:
	class Item
	{
	public:
		struct Progress
		{
			unsigned int nMin_;
			unsigned int nMax_;
			unsigned int nPos_;
		};
	
	public:
		Item(unsigned int nId, qs::QSTATUS* pstatus);
		~Item();
	
	public:
		unsigned int getId() const;
		const Progress& getProgress(bool bSub) const;
		const WCHAR* getMessage() const;
	
	public:
		void setPos(bool bSub, unsigned int nPos);
		void setRange(bool bSub, unsigned int nMin, unsigned int nMax);
		qs::QSTATUS setAccount(Account* pAccount, SubAccount* pSubAccount);
		qs::QSTATUS setFolder(Folder* pFolder);
		qs::QSTATUS setMessage(const WCHAR* pwszMessage);
	
	private:
		qs::QSTATUS updateMessage();
	
	private:
		unsigned int nId_;
		Progress main_;
		Progress sub_;
		Account* pAccount_;
		SubAccount* pSubAccount_;
		Folder* pFolder_;
		qs::WSTRING wstrOriginalMessage_;
		qs::WSTRING wstrMessage_;
	};

private:
	typedef std::vector<Item*> ItemList;

public:
	SyncStatusWindow(SyncDialog* pSyncDialog, qs::QSTATUS* pstatus);
	virtual ~SyncStatusWindow();

public:
	virtual qs::QSTATUS getWindowClass(WNDCLASS* pwc);
	virtual LRESULT windowProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

public:
	virtual qs::QSTATUS start();
	virtual void end();
	virtual qs::QSTATUS startThread(unsigned int nId);
	virtual void endThread(unsigned int nId);
	virtual qs::QSTATUS setPos(unsigned int nId,
		bool bSub, unsigned int nPos);
	virtual qs::QSTATUS setRange(unsigned int nId, bool bSub,
		unsigned int nMin, unsigned int nMax);
	virtual qs::QSTATUS setAccount(unsigned int nId,
		Account* pAccount, SubAccount* pSubAccount);
	virtual qs::QSTATUS setFolder(unsigned int nId, Folder* pFolder);
	virtual qs::QSTATUS setMessage(unsigned int nId, const WCHAR* pwszMessage);
	virtual qs::QSTATUS addError(unsigned int nId, const SessionErrorInfo& info);
	virtual bool isCanceled(unsigned int nId, bool bForce);
	virtual qs::QSTATUS showDialupDialog(
		RASDIALPARAMS* prdp, bool* pbCancel);

protected:
	LRESULT onCreate(CREATESTRUCT* pCreateStruct);
	LRESULT onPaint();
	LRESULT onSize(UINT nFlags, int cx, int cy);
	LRESULT onVScroll(UINT nCode, UINT nPos, HWND hwnd);

private:
	int getItemHeight() const;
	void updateScrollBar();
	void paintItem(qs::DeviceContext* pdc,
		const RECT& rect, const Item* pItem);
	void paintProgress(qs::DeviceContext* pdc,
		const RECT& rect, const Item::Progress& progress);
	ItemList::iterator getItem(unsigned int nId);

private:
	SyncStatusWindow(const SyncStatusWindow&);
	SyncStatusWindow& operator=(const SyncStatusWindow&);

private:
	SyncDialog* pSyncDialog_;
	ItemList listItem_;
	qs::CriticalSection cs_;
	int nFontHeight_;
	qs::WSTRING wstrFinished_;
	qs::WSTRING wstrCancel_;
	int nCancelWidth_;
};


/****************************************************************************
 *
 * SyncDialogThread
 *
 */

class SyncDialogThread : public qs::Thread
{
public:
	SyncDialogThread(qs::Profile* pProfile, qs::QSTATUS* pstatus);
	virtual ~SyncDialogThread();

public:
	SyncDialog* getDialog();
	void stop();

public:
	virtual unsigned int run();

private:
	SyncDialogThread(const SyncDialogThread&);
	SyncDialogThread& operator=(const SyncDialogThread&);

private:
	qs::Profile* pProfile_;
	SyncDialog* pDialog_;
	qs::Event* pEvent_;
};

}

#endif // __SYNCDIALOG_H__
