/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
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

class PasswordManager;


/****************************************************************************
 *
 * SyncDialogManager
 *
 */

class SyncDialogManager
{
public:
	SyncDialogManager(qs::Profile* pProfile,
					  PasswordManager* pPasswordManager);
	~SyncDialogManager();

public:
	SyncDialog* open();
	bool save() const;

private:
	SyncDialogManager(const SyncDialogManager&);
	SyncDialogManager& operator=(const SyncDialogManager&);

private:
	qs::Profile* pProfile_;
	PasswordManager* pPasswordManager_;
	std::auto_ptr<SyncDialogThread> pThread_;
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
	enum Flag {
		FLAG_NONE				= 0x00,
		FLAG_SHOWDIALOG			= 0x01,
		FLAG_NOTIFYNEWMESSAGE	= 0x02
	};

public:
	SyncDialog(qs::Profile* pProfile,
			   PasswordManager* pPasswordManager);
	virtual ~SyncDialog();

public:
	SyncManagerCallback* getSyncManagerCallback() const;

public:
	void show();
	void hide();
	void setMessage(const WCHAR* pwszMessage);
	unsigned int getCanceledTime() const;
	void resetCanceledTime();
	void addError(const WCHAR* pwszError);
	bool hasError() const;
	void enableCancel(bool bEnable);
	PasswordCallback::Result getPassword(SubAccount* pSubAccount,
										 Account::Host host,
										 qs::wstring_ptr* pwstrPassword);
	void setPassword(SubAccount* pSubAccount,
					 Account::Host host,
					 const WCHAR* pwszPassword,
					 bool bPermanent);
	bool showDialupDialog(RASDIALPARAMS* prdp);
	qs::wstring_ptr selectDialupEntry();
	void notifyNewMessage() const;
	bool save() const;

public:
	virtual INT_PTR dialogProc(UINT uMsg,
							   WPARAM wParam,
							   LPARAM lParam);

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

protected:
	LRESULT onClose();
	LRESULT onDestroy();
	LRESULT onInitDialog(HWND hwndFocus,
						 LPARAM lParam);
	LRESULT onSize(UINT nFlags,
				   int cx,
				   int cy);

private:
	LRESULT onCancel();
	LRESULT onEsc();
	LRESULT onHide();

private:
	void layout();
	void layout(int cx,
				int cy);

private:
	SyncDialog(const SyncDialog&);
	SyncDialog& operator=(const SyncDialog&);

private:
	qs::Profile* pProfile_;
	PasswordManager* pPasswordManager_;
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
		Item(unsigned int nId,
			 unsigned int nParam);
		~Item();
	
	public:
		unsigned int getId() const;
		unsigned int getParam() const;
		const Progress& getProgress(bool bSub) const;
		const WCHAR* getMessage() const;
	
	public:
		void setPos(bool bSub,
					unsigned int nPos);
		void setRange(bool bSub,
					  unsigned int nMin,
					  unsigned int nMax);
		void setAccount(Account* pAccount,
						SubAccount* pSubAccount);
		void setFolder(Folder* pFolder);
		void setMessage(const WCHAR* pwszMessage);
	
	private:
		void updateMessage();
	
	private:
		unsigned int nId_;
		unsigned int nParam_;
		Progress main_;
		Progress sub_;
		Account* pAccount_;
		SubAccount* pSubAccount_;
		Folder* pFolder_;
		qs::wstring_ptr wstrOriginalMessage_;
		qs::wstring_ptr wstrMessage_;
	};

private:
	typedef std::vector<Item*> ItemList;

public:
	explicit SyncStatusWindow(SyncDialog* pSyncDialog);
	virtual ~SyncStatusWindow();

public:
	virtual void getWindowClass(WNDCLASS* pwc);
	virtual LRESULT windowProc(UINT uMsg,
							   WPARAM wParam,
							   LPARAM lParam);

public:
	virtual void start(unsigned int nParam);
	virtual void end();
	virtual void startThread(unsigned int nId,
							 unsigned int nParam);
	virtual void endThread(unsigned int nId);
	virtual void setPos(unsigned int nId,
						bool bSub,
						unsigned int nPos);
	virtual void setRange(unsigned int nId,
						  bool bSub,
						  unsigned int nMin,
						  unsigned int nMax);
	virtual void setAccount(unsigned int nId,
							Account* pAccount,
							SubAccount* pSubAccount);
	virtual void setFolder(unsigned int nId,
						   Folder* pFolder);
	virtual void setMessage(unsigned int nId,
							const WCHAR* pwszMessage);
	virtual void addError(unsigned int nId,
						  const SessionErrorInfo& info);
	virtual bool isCanceled(unsigned int nId, bool bForce);
	virtual PasswordCallback::Result getPassword(SubAccount* pSubAccount,
												 Account::Host host,
												 qs::wstring_ptr* pwstrPassword);
	virtual void setPassword(SubAccount* pSubAccount,
							 Account::Host host,
							 const WCHAR* pwszPassword,
							 bool bPermanent);
	virtual qs::wstring_ptr selectDialupEntry();
	virtual bool showDialupDialog(RASDIALPARAMS* prdp);
	virtual void notifyNewMessage(unsigned int nId);

protected:
	LRESULT onCreate(CREATESTRUCT* pCreateStruct);
	LRESULT onPaint();
	LRESULT onSize(UINT nFlags,
				   int cx,
				   int cy);
	LRESULT onVScroll(UINT nCode,
					  UINT nPos,
					  HWND hwnd);

private:
	int getItemHeight() const;
	void updateScrollBar();
	void paintItem(qs::DeviceContext* pdc,
				   const RECT& rect,
				   const Item* pItem);
	void paintProgress(qs::DeviceContext* pdc,
					   const RECT& rect,
					   const Item::Progress& progress);
	ItemList::iterator getItem(unsigned int nId);

private:
	SyncStatusWindow(const SyncStatusWindow&);
	SyncStatusWindow& operator=(const SyncStatusWindow&);

private:
	SyncDialog* pSyncDialog_;
	ItemList listItem_;
	bool bNewMessage_;
	qs::CriticalSection cs_;
	int nFontHeight_;
	qs::wstring_ptr wstrFinished_;
	qs::wstring_ptr wstrCancel_;
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
	SyncDialogThread(qs::Profile* pProfile,
					 PasswordManager* pPasswordManager);
	virtual ~SyncDialogThread();

public:
	SyncDialog* getDialog();
	void stop();

public:
	virtual void run();

private:
	SyncDialogThread(const SyncDialogThread&);
	SyncDialogThread& operator=(const SyncDialogThread&);

private:
	qs::Profile* pProfile_;
	PasswordManager* pPasswordManager_;
	SyncDialog* pDialog_;
	std::auto_ptr<qs::Event> pEvent_;
};

}

#endif // __SYNCDIALOG_H__
