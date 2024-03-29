/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#ifndef __QMMAINWINDOW_H__
#define __QMMAINWINDOW_H__

#include <qm.h>
#include <qmaction.h>

#include <qskeymap.h>
#include <qsprofile.h>
#include <qswindow.h>


namespace qm {

class MainWindow;

class EditFrameWindowManager;
class FolderModel;
class SecurityModel;


/****************************************************************************
 *
 * MainWindow
 *
 */

class QMEXPORTCLASS MainWindow : public qs::FrameWindow
{
public:
	explicit MainWindow(qs::Profile* pProfile);
	virtual ~MainWindow();

public:
	FolderModel* getFolderModel() const;
	SecurityModel* getSecurityModel() const;
	const ActionInvoker* getActionInvoker() const;
	const EditFrameWindowManager* getEditFrameWindowManager() const;
	bool isShowingModalDialog() const;
	void initialShow(bool bHidden);
	void layout();
	void reloadProfiles();
	bool save(bool bForce);
	
#ifndef _WIN32_WCE_PSPC
	void show();
	void hide();
	bool isHidden() const;
	void activate();
#endif
	
	bool isShowToolbar() const;
	void setShowToolbar(bool bShow);
	bool isShowStatusBar() const;
	void setShowStatusBar(bool bShow);
	bool isShowFolderWindow() const;
	void setShowFolderWindow(bool bShow);
	bool isShowPreviewWindow() const;
	void setShowPreviewWindow(bool bShow);

public:
	virtual void processIdle();

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
	virtual std::auto_ptr<qs::ActionParam> getActionParam(UINT nId);
	virtual qs::Accelerator* getAccelerator();
	virtual LRESULT windowProc(UINT uMsg,
							   WPARAM wParam,
							   LPARAM lParam);

protected:
	LRESULT onActivate(UINT nFlags,
					   HWND hwnd,
					   bool bMinimized);
	LRESULT onClose();
	LRESULT onCopyData(HWND hwnd,
					   COPYDATASTRUCT* pData);
	LRESULT onCreate(CREATESTRUCT* pCreateStruct);
	LRESULT onDestroy();
#ifndef _WIN32_WCE
	LRESULT onEndSession(bool bEnd,
						 int nOption);
#endif
#ifndef _WIN32_WCE
	LRESULT onQueryEndSession(int nOption);
#endif
	LRESULT onSize(UINT nFlags,
				   int cx,
				   int cy);

private:
	MainWindow(const MainWindow&);
	MainWindow& operator=(const MainWindow&);

private:
	class MainWindowImpl* pImpl_;
};

}

#endif // __QMMAINWINDOW_H__
