/*
 * $Id: qsdialog.h,v 1.3 2003/06/01 16:27:36 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QSDIALOG_H__
#define __QSDIALOG_H__

#include <qs.h>
#include <qswindow.h>

namespace qs {

class Window;
	class DialogBase;
		class Dialog;
			class DefaultDialog;
		class PropertyPage;
	class PropertySheetBase;
class DialogHandler;
	class DefaultDialogHandler;
class FileDialog;
#ifdef _WIN32_WCE
class BrowseFolderDialog;
#endif


/****************************************************************************
 *
 * DialogBase
 *
 */

class QSEXPORTCLASS DialogBase :
	public Window,
	public DefWindowProcHolder
{
protected:
	DialogBase(bool bDeleteThis, QSTATUS* pstatus);

public:
	virtual ~DialogBase();

public:
	QSTATUS setDialogHandler(DialogHandler* pDialogHandler,
		bool bDeleteHandler);
	
	QSTATUS addCommandHandler(CommandHandler* pch);
	QSTATUS removeCommandHandler(CommandHandler* pch);
	
	QSTATUS addNotifyHandler(NotifyHandler* pnh);
	QSTATUS removeNotifyHandler(NotifyHandler* pnh);
	
	QSTATUS addOwnerDrawHandler(OwnerDrawHandler* podh);
	QSTATUS removeOwnerDrawHandler(OwnerDrawHandler* podh);

public:
	virtual LRESULT defWindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

public:
	static QSTATUS processDialogMessage(MSG* pMsg, bool* pbProcessed);

private:
	DialogBase(const DialogBase&);
	DialogBase& operator=(const DialogBase&);

private:
	class DialogBaseImpl* pImpl_;

friend INT_PTR CALLBACK dialogProc(HWND, UINT, WPARAM, LPARAM);
friend INT_PTR CALLBACK propertyPageProc(HWND, UINT, WPARAM, LPARAM);
};


/****************************************************************************
 *
 * Dialog
 *
 */

class QSEXPORTCLASS Dialog : public DialogBase
{
public:
	Dialog(HINSTANCE hInstResource, UINT nId,
		bool bDeleteThis, QSTATUS* pstatus);
	~Dialog();

public:
	QSTATUS doModal(HWND hwndParent,
		ModalHandler* pModalHandler, int* pnRet);
	QSTATUS create(HWND hwndParent);
	QSTATUS endDialog(int nCode);

private:
	Dialog(const Dialog&);
	Dialog& operator=(const Dialog&);

private:
	struct DialogImpl* pImpl_;
};


/****************************************************************************
 *
 * DialogHandler
 *
 */

class QSEXPORTCLASS DialogHandler
{
public:
	virtual ~DialogHandler();

public:
	virtual DialogBase* getDialogBase() const = 0;
	virtual QSTATUS setDialogBase(DialogBase* pDialogBase) = 0;
	virtual INT_PTR dialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam) = 0;
	virtual void initProcResult() = 0;
	virtual INT_PTR getProcResult() const = 0;
};

#define BEGIN_DIALOG_HANDLER() \
	initProcResult(); \
	bool bProcessed = true; \
	LRESULT lResult = 0; \
	switch (uMsg) { \

#define END_DIALOG_HANDLER() \
	default: \
		bProcessed = false; \
		break; \
	} \
	if (bProcessed) { \
		switch (uMsg) { \
		case WM_CHARTOITEM: \
		case WM_COMPAREITEM: \
		case WM_CTLCOLORBTN: \
		case WM_CTLCOLORDLG: \
		case WM_CTLCOLOREDIT: \
		case WM_CTLCOLORLISTBOX: \
		case WM_CTLCOLORSCROLLBAR: \
		case WM_CTLCOLORSTATIC: \
		case WM_INITDIALOG: \
		case WM_QUERYDRAGICON: \
		case WM_VKEYTOITEM: \
			return lResult; \
		default: \
			getDialogBase()->setWindowLong(DWL_MSGRESULT, lResult); \
			return getProcResult(); \
		} \
	} \


/****************************************************************************
 *
 * DefaultDialogHandler
 *
 */

class QSEXPORTCLASS DefaultDialogHandler :
	public DialogHandler,
	public DefaultWindowHandlerBase
{
public:
	explicit DefaultDialogHandler(QSTATUS* pstatus);
	virtual ~DefaultDialogHandler();

public:
	virtual DialogBase* getDialogBase() const;
	virtual QSTATUS setDialogBase(DialogBase* pDialogBase);
	virtual INT_PTR dialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual void initProcResult();
	virtual INT_PTR getProcResult() const;

public:
	virtual DefWindowProcHolder* getDefWindowProcHolder();

protected:
	void setProcResult(INT_PTR nResult);

private:
	DefaultDialogHandler(const DefaultDialogHandler&);
	DefaultDialogHandler& operator=(const DefaultDialogHandler&);

private:
	DialogBase* pDialogBase_;
	INT_PTR nResult_;
};


/****************************************************************************
 *
 * PropertySheetBase
 *
 */

class QSEXPORTCLASS PropertySheetBase : public Window
{
public:
	PropertySheetBase(HINSTANCE hInstResource,
		const WCHAR* pwszTitle, bool bDeleteThis, QSTATUS* pstatus);
	~PropertySheetBase();

public:
	QSTATUS doModal(HWND hwndParent,
		ModalHandler* pModalHandler, int* pnRet);
	
	QSTATUS add(PropertyPage* pPage);
	QSTATUS setStartPage(int nPage);
	PropertyPage* getPage(int nPage);
	
	void init();
	bool isDialogMessage(MSG* pMsg);

public:
	static QSTATUS processDialogMessage(MSG* pMsg, bool* pbProcessed);

private:
	PropertySheetBase(const PropertySheetBase&);
	PropertySheetBase& operator=(const PropertySheetBase&);

private:
	class PropertySheetBaseImpl* pImpl_;

friend int CALLBACK propertySheetProc(HWND, UINT, LPARAM);
};


/****************************************************************************
 *
 * PropertyPage
 *
 */

class QSEXPORTCLASS PropertyPage : public DialogBase
{
public:
	PropertyPage(HINSTANCE hInstResource, UINT nId,
		bool bDeleteThis, QSTATUS* pstatus);
	virtual ~PropertyPage();

private:
	QSTATUS create(PropertySheetBase* pSheet, HPROPSHEETPAGE* phpsp);

private:
	PropertyPage(const PropertyPage&);
	PropertyPage& operator=(PropertyPage&);

private:
	struct PropertyPageImpl* pImpl_;

friend class PropertySheetBase;
};


/****************************************************************************
 *
 * DefaultDialog
 *
 */

class QSEXPORTCLASS DefaultDialog :
	public Dialog,
	public DefaultDialogHandler,
	public DefaultCommandHandler
{
protected:
	DefaultDialog(HINSTANCE hInst, UINT nId, QSTATUS* pstatus);

public:
	virtual ~DefaultDialog();

protected:
	void init(bool bDoneButton);

public:
	virtual INT_PTR dialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

public:
	virtual LRESULT onCommand(WORD nCode, WORD nId);

protected:
	virtual LRESULT onDestroy();
	virtual LRESULT onInitDialog(HWND hwndFocus, LPARAM lParam);

protected:
	virtual LRESULT onOk();
	virtual LRESULT onCancel();

private:
	DefaultDialog(const DefaultDialog&);
	DefaultDialog& operator=(const DefaultDialog&);
};


/****************************************************************************
 *
 * DefaultPropertyPage
 *
 */

class QSEXPORTCLASS DefaultPropertyPage :
	public PropertyPage,
	public DefaultDialogHandler,
	public DefaultCommandHandler,
	public NotifyHandler
{
protected:
	DefaultPropertyPage(HINSTANCE hInst, UINT nId, QSTATUS* pstatus);

public:
	virtual ~DefaultPropertyPage();

public:
	virtual INT_PTR dialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

public:
	virtual LRESULT onCommand(WORD nCode, WORD nId);

public:
	virtual LRESULT onNotify(NMHDR* pnmhdr, bool* pbHandled);

protected:
	virtual LRESULT onDestroy();
	virtual LRESULT onInitDialog(HWND hwndFocus, LPARAM lParam);

protected:
	virtual LRESULT onOk();

private:
	LRESULT onApply(NMHDR* pnmhdr, bool* pbHandled);

private:
	DefaultPropertyPage(const DefaultPropertyPage&);
	DefaultPropertyPage& operator=(const DefaultPropertyPage&);
};


/****************************************************************************
 *
 * FileDialog
 *
 */

class QSEXPORTCLASS FileDialog
{
public:
	FileDialog(bool bOpen, const WCHAR* pwszFilter,
		const WCHAR* pwszInitialDir, const WCHAR* pwszDefaultExt,
		const WCHAR* pwszFileName, DWORD dwFlags, QSTATUS* pstatus);
	~FileDialog();

public:
	const WCHAR* getPath() const;
	QSTATUS doModal(HWND hwndParent,
		ModalHandler* pModalHandler, int* pnRet);

private:
	FileDialog(const FileDialog&);
	FileDialog& operator=(const FileDialog&);

private:
	struct FileDialogImpl* pImpl_;
};


#ifdef _WIN32_WCE
/****************************************************************************
 *
 * BrowseFolderDialog
 *
 */

class QSEXPORTCLASS BrowseFolderDialog : public DefaultDialog
{
public:
	BrowseFolderDialog(const WCHAR* pwszTitle,
		const WCHAR* pwszInitialPath, QSTATUS* pstatus);
	virtual ~BrowseFolderDialog();

public:
	const WCHAR* getPath() const;

protected:
	virtual LRESULT onDestroy();
	virtual LRESULT onInitDialog(HWND hwndFocus, LPARAM lParam);

protected:
	virtual LRESULT onOk();

private:
	BrowseFolderDialog(const BrowseFolderDialog&);
	BrowseFolderDialog& operator=(const BrowseFolderDialog&);

private:
	struct BrowseFolderDialogImpl* pImpl_;
};
#endif

}

#endif // __QSDIALOG_H__
