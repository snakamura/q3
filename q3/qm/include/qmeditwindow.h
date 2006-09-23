/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QMEDITWINDOW_H__
#define __QMEDITWINDOW_H__

#include <qm.h>

#include <qsprofile.h>
#include <qstextwindow.h>
#include <qswindow.h>


namespace qm {

class EditFrameWindow;
class EditWindow;
class HeaderEditWindow;

class ActionInvoker;
class AttachmentSelectionModel;
class EditFrameWindowManager;
class EditMessage;
class EditMessageHolder;
class EditWindowItem;
template<class Item> class FocusController;


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
	void layout();
	void reloadProfiles();
	
	bool isShowToolbar() const;
	void setShowToolbar(bool bShow);
	bool isShowStatusBar() const;
	void setShowStatusBar(bool bShow);

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
	FocusController<EditWindowItem>* getFocusController() const;
	void saveFocusedItem();
	void restoreFocusedItem();
	bool isHeaderEdit() const;
	void setHeaderEdit(bool bHeaderEdit);
	void layout();
	void reloadProfiles();

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
	void layout(const RECT& rect);
	EditWindowItem* getFocusedItem() const;
	EditWindowItem* getInitialFocusedItem() const;
	EditWindowItem* getNextFocusItem(EditWindowItem* pItem) const;
	EditWindowItem* getPrevFocusItem(EditWindowItem* pItem) const;
	EditWindowItem* getItemByNumber(unsigned int nNumber) const;
	AttachmentSelectionModel* getAttachmentSelectionModel() const;
	void reloadProfiles();

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

private:
	HeaderEditWindow(const HeaderEditWindow&);
	HeaderEditWindow& operator=(const HeaderEditWindow&);

private:
	class HeaderEditWindowImpl* pImpl_;
};

}

#endif // __QMEDITWINDOW_H__
