/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QMEDITWINDOW_H__
#define __QMEDITWINDOW_H__

#include <qm.h>
#include <qmaction.h>

#include <qsprofile.h>
#include <qstextwindow.h>
#include <qswindow.h>


namespace qm {

class EditFrameWindow;
class EditWindow;

class AttachmentSelectionModel;
class EditFrameWindowManager;
class EditMessage;
class EditMessageHolder;
class EditWindowItem;


/****************************************************************************
 *
 * EditFrameWindow
 *
 */

class QMEXPORTCLASS EditFrameWindow : public qs::FrameWindow
{
public:
	EditFrameWindow(EditFrameWindowManager* pManager,
					qs::Profile* pProfile);
	virtual ~EditFrameWindow();

public:
	EditWindow* getEditWindow() const;
	const ActionInvoker* getActionInvoker() const;
	void initialShow();
	void close();
	bool tryClose();
	
	bool isShowToolbar() const;
	void setShowToolbar(bool bShow);
	bool isShowStatusBar() const;
	void setShowStatusBar(bool bShow);

protected:
	virtual bool getToolbarButtons(Toolbar* pToolbar);
	virtual bool createToolbarButtons(void* pCreateParam,
									  HWND hwndToolbar);
#if defined _WIN32_WCE && (_WIN32_WCE < 300 || !defined _WIN32_WCE_PSPC)
	virtual UINT getBarId(int n) const;
	virtual bool getCommandBandsRestoreInfo(int n,
											COMMANDBANDSRESTOREINFO* pcbri) const;
	virtual bool setCommandBandsRestoreInfo(int n,
											const COMMANDBANDSRESTOREINFO& cbri);
#endif
	virtual HMENU getMenuHandle(void* pCreateParam);
	virtual UINT getIconId();

public:
	virtual void getWindowClass(WNDCLASS* pwc);
	virtual bool preCreateWindow(CREATESTRUCT* pCreateStruct);
	virtual qs::Action* getAction(UINT nId);
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
	LRESULT onInitMenuPopup(HMENU hmenu,
						    UINT nIndex,
						    bool bSysMenu);
	LRESULT onSize(UINT nFlags,
				   int cx,
				   int cy);

private:
	EditFrameWindow(const EditFrameWindow&);
	EditFrameWindow& operator=(const EditFrameWindow&);

private:
	class EditFrameWindowImpl* pImpl_;
};


/****************************************************************************
 *
 * EditWindow
 *
 */

class EditWindow :
	public qs::WindowBase,
	public qs::DefaultWindowHandler
{
public:
	explicit EditWindow(qs::Profile* pProfile);
	virtual ~EditWindow();

public:
	EditMessageHolder* getEditMessageHolder() const;
	AttachmentSelectionModel* getAttachmentSelectionModel() const;
	qs::TextWindow* getTextWindow() const;
	EditWindowItem* getItemByNumber(unsigned int nNumber) const;
	EditWindowItem* getFocusedItem() const;
	void saveFocusedItem();
	void restoreFocusedItem();
	bool isHeaderEdit() const;
	void setHeaderEdit(bool bHeaderEdit);

public:
	virtual qs::Accelerator* getAccelerator();
	virtual LRESULT windowProc(UINT uMsg,
							   WPARAM wParam,
							   LPARAM lParam);

protected:
	LRESULT onCreate(CREATESTRUCT* pCreateStruct);
	LRESULT onDestroy();
	LRESULT onSize(UINT nFlags,
				   int cx,
				   int cy);

private:
	EditWindow(const EditWindow&);
	EditWindow& operator=(const EditWindow&);

private:
	class EditWindowImpl* pImpl_;
};


/****************************************************************************
 *
 * HeaderEditWindow
 *
 */

class HeaderEditWindow :
	public qs::WindowBase,
	public qs::DefaultWindowHandler
{
public:
	explicit HeaderEditWindow(qs::Profile* pProfile);
	virtual ~HeaderEditWindow();

public:
	void setEditMessage(EditMessage* pEditMessage,
						bool bReset);
	void releaseEditMessage(EditMessage* pEditMessage);
	void updateEditMessage(EditMessage* pEditMessage);
	int getHeight() const;
	void layout();
	EditWindowItem* getFocusedItem() const;
	EditWindowItem* getInitialFocusedItem() const;
	EditWindowItem* getNextFocusItem(EditWindowItem* pItem) const;
	EditWindowItem* getPrevFocusItem(EditWindowItem* pItem) const;
	EditWindowItem* getItemByNumber(unsigned int nNumber) const;
	AttachmentSelectionModel* getAttachmentSelectionModel() const;

public:
	virtual void getWindowClass(WNDCLASS* pwc);
	virtual LRESULT windowProc(UINT uMsg,
							   WPARAM wParam,
							   LPARAM lParam);

protected:
	LRESULT onCreate(CREATESTRUCT* pCreateStruct);
	LRESULT onCtlColorStatic(HDC hdc,
							 HWND hwnd);
	LRESULT onDestroy();
	LRESULT onSize(UINT nFlags,
				   int cx,
				   int cy);

private:
	HeaderEditWindow(const HeaderEditWindow&);
	HeaderEditWindow& operator=(const HeaderEditWindow&);

private:
	class HeaderEditWindowImpl* pImpl_;
};

}

#endif // __QMEDITWINDOW_H__
