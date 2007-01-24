/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QMADDRESSBOOKWINDOW_H__
#define __QMADDRESSBOOKWINDOW_H__

#include <qm.h>

#include <qsprofile.h>
#include <qsthread.h>
#include <qswindow.h>


namespace qm {

class AddressBookFrameWindow;
class AddressBookCategoryWindow;
class AddressBookListWindow;

class AddressBookFrameWindowManager;
class AddressBookModel;
class AddressBookSelectionModel;
class UIManager;


/****************************************************************************
 *
 * AddressBookFrameWindow
 *
 */

class QMEXPORTCLASS AddressBookFrameWindow : public qs::FrameWindow
{
public:
	AddressBookFrameWindow(AddressBookFrameWindowManager* pManager,
						   qs::Profile* pProfile);
	virtual ~AddressBookFrameWindow();

public:
	void initialShow();
	bool tryClose(bool bAsync);
	
	bool isShowToolbar() const;
	void setShowToolbar(bool bShow);
	bool isShowStatusBar() const;
	void setShowStatusBar(bool bShow);
	
	void reloadProfiles();

protected:
	virtual bool getToolbarButtons(Toolbar* pToolbar);
	virtual bool createToolbarButtons(void* pCreateParam,
									  HWND hwndToolbar);
#ifdef _WIN32_WCE
	virtual UINT getBarId(int n) const;
	virtual bool getCommandBandsRestoreInfo(int n,
											COMMANDBANDSRESTOREINFO* pcbri) const;
	virtual bool setCommandBandsRestoreInfo(int n,
											const COMMANDBANDSRESTOREINFO& cbri);
#endif
	virtual HMENU getMenuHandle(void* pCreateParam);
	virtual UINT getIconId();
	virtual const qs::DynamicMenuItem* getDynamicMenuItem(unsigned int nId) const;
	virtual qs::DynamicMenuCreator* getDynamicMenuCreator(const qs::DynamicMenuItem* pItem);

public:
	virtual void getWindowClass(WNDCLASS* pwc);
	virtual bool preCreateWindow(CREATESTRUCT* pCreateStruct);
	virtual qs::Action* getAction(UINT nId);
	virtual const qs::ActionParam* getActionParam(UINT nId);
	virtual qs::Accelerator* getAccelerator();
	virtual LRESULT windowProc(UINT uMsg,
							   WPARAM wParam,
							   LPARAM lParam);

protected:
	LRESULT onActivate(UINT nFlags,
					   HWND hwnd,
					   bool bMinimized);
	LRESULT onClose();
	LRESULT onCreate(CREATESTRUCT* pCreateStruct);
	LRESULT onDestroy();
	LRESULT onSize(UINT nFlags,
				   int cx,
				   int cy);
	LRESULT onAddressBookFrameWindowClose(WPARAM wParam,
										  LPARAM lParam);

private:
	AddressBookFrameWindow(const AddressBookFrameWindow&);
	AddressBookFrameWindow& operator=(const AddressBookFrameWindow&);

private:
	class AddressBookFrameWindowImpl* pImpl_;
};


/****************************************************************************
 *
 * AddressBookListWindow
 *
 */

class AddressBookListWindow :
	public qs::WindowBase,
	public qs::DefaultWindowHandler
{
public:
	AddressBookListWindow(qs::WindowBase* pParentWindow,
						  AddressBookModel* pAddressBookModel,
						  qs::Profile* pProfile);
	virtual ~AddressBookListWindow();

public:
	AddressBookSelectionModel* getSelectionModel() const;
	
	void reloadProfiles();

public:
	virtual qs::wstring_ptr getSuperClass();
	virtual bool preCreateWindow(CREATESTRUCT* pCreateStruct);
	virtual qs::Accelerator* getAccelerator();
	virtual LRESULT windowProc(UINT uMsg,
							   WPARAM wParam,
							   LPARAM lParam);

protected:
	LRESULT onContextMenu(HWND hwnd,
						  const POINT& pt);
	LRESULT onCreate(CREATESTRUCT* pCreateStruct);
	LRESULT onDestroy();
	LRESULT onLButtonDblClk(UINT nFlags,
							const POINT& pt);
	LRESULT onLButtonDown(UINT nFlags,
						  const POINT& pt);

private:
	AddressBookListWindow(const AddressBookListWindow&);
	AddressBookListWindow& operator=(const AddressBookListWindow&);

private:
	class AddressBookListWindowImpl* pImpl_;
};


/****************************************************************************
 *
 * AddressBookThread
 *
 */

class AddressBookThread : public qs::Thread
{
public:
	AddressBookThread(AddressBookFrameWindowManager* pManager,
					  std::auto_ptr<AddressBookModel> pAddressBookModel,
					  UIManager* pUIManager,
					  qs::Profile* pProfile);
	virtual ~AddressBookThread();

public:
	AddressBookFrameWindow* create();

public:
	virtual void run();

private:
	AddressBookThread(const AddressBookThread&);
	AddressBookThread& operator=(const AddressBookThread&);

private:
	struct AddressBookThreadImpl* pImpl_;
};

}

#endif // __QMADDRESSBOOKWINDOW_H__
