/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
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
class FontDialog;

class InitThread;


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
	explicit DialogBase(bool bDeleteThis);

public:
	virtual ~DialogBase();

public:
	void setDialogHandler(DialogHandler* pDialogHandler,
						  bool bDeleteHandler);
	
	void addCommandHandler(CommandHandler* pch);
	void removeCommandHandler(CommandHandler* pch);
	
	void addNotifyHandler(NotifyHandler* pnh);
	void removeNotifyHandler(NotifyHandler* pnh);
	
	void addOwnerDrawHandler(OwnerDrawHandler* podh);
	void removeOwnerDrawHandler(OwnerDrawHandler* podh);
	
	InitThread* getInitThread() const;

public:
	virtual LRESULT defWindowProc(UINT uMsg,
								  WPARAM wParam,
								  LPARAM lParam);

public:
	static bool processDialogMessage(MSG* pMsg);

private:
	DialogBase(const DialogBase&);
	DialogBase& operator=(const DialogBase&);

private:
	class DialogBaseImpl* pImpl_;

#if defined _WIN32_WCE && !defined _WIN32_WCE_EMULATION
friend class WindowDestroy;
#endif
friend INT_PTR CALLBACK dialogProc(HWND,
								   UINT,
								   WPARAM,
								   LPARAM);
friend INT_PTR CALLBACK propertyPageProc(HWND,
										 UINT,
										 WPARAM,
										 LPARAM);
};


/****************************************************************************
 *
 * Dialog
 *
 */

class QSEXPORTCLASS Dialog : public DialogBase
{
public:
	Dialog(HINSTANCE hInstResource,
		   UINT nId,
		   bool bDeleteThis);
	~Dialog();

public:
	INT_PTR doModal(HWND hwndParent);
	INT_PTR doModal(HWND hwndParent,
					ModalHandler* pModalHandler);
	bool create(HWND hwndParent);
	bool endDialog(int nCode);

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
	virtual void setDialogBase(DialogBase* pDialogBase) = 0;
	virtual INT_PTR dialogProc(UINT uMsg,
							   WPARAM wParam,
							   LPARAM lParam) = 0;
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
			getDialogBase()->setWindowLong(DWLP_MSGRESULT, lResult); \
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
	DefaultDialogHandler();
	virtual ~DefaultDialogHandler();

public:
	virtual DialogBase* getDialogBase() const;
	virtual void setDialogBase(DialogBase* pDialogBase);
	virtual INT_PTR dialogProc(UINT uMsg,
							   WPARAM wParam,
							   LPARAM lParam);
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
 * DefaultDialog
 *
 */

class QSEXPORTCLASS DefaultDialog :
	public Dialog,
	public DefaultDialogHandler,
	public CommandHandler
{
protected:
	DefaultDialog(HINSTANCE hInst,
				  UINT nId);

public:
	virtual ~DefaultDialog();

protected:
	void init(bool bDoneButton);

public:
	virtual INT_PTR dialogProc(UINT uMsg,
							   WPARAM wParam,
							   LPARAM lParam);

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

protected:
	virtual LRESULT onDestroy();
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

protected:
	virtual LRESULT onOk();
	virtual LRESULT onCancel();

private:
	DefaultDialog(const DefaultDialog&);
	DefaultDialog& operator=(const DefaultDialog&);
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
					  const WCHAR* pwszTitle,
					  bool bDeleteThis);
	~PropertySheetBase();

public:
	INT_PTR doModal(HWND hwndParent);
	INT_PTR doModal(HWND hwndParent,
					ModalHandler* pModalHandler);
	
	void add(PropertyPage* pPage);
	void setStartPage(int nPage);
	PropertyPage* getPage(int nPage);
	PROPSHEETHEADER& getHeader();
	
	void init();
	bool isDialogMessage(MSG* pMsg);

public:
	static bool processDialogMessage(MSG* pMsg);

private:
	PropertySheetBase(const PropertySheetBase&);
	PropertySheetBase& operator=(const PropertySheetBase&);

private:
	class PropertySheetBaseImpl* pImpl_;

friend int CALLBACK propertySheetProc(HWND,
									  UINT,
									  LPARAM);
};


/****************************************************************************
 *
 * PropertyPage
 *
 */

class QSEXPORTCLASS PropertyPage : public DialogBase
{
public:
	PropertyPage(HINSTANCE hInstResource,
				 UINT nId,
				 bool bDeleteThis);
	virtual ~PropertyPage();

public:
	PropertySheetBase* getSheet() const;

private:
	HPROPSHEETPAGE create(PropertySheetBase* pSheet);

private:
	PropertyPage(const PropertyPage&);
	PropertyPage& operator=(PropertyPage&);

private:
	struct PropertyPageImpl* pImpl_;

friend class PropertySheetBase;
};


/****************************************************************************
 *
 * DefaultPropertyPage
 *
 */

class QSEXPORTCLASS DefaultPropertyPage :
	public PropertyPage,
	public DefaultDialogHandler,
	public CommandHandler,
	public NotifyHandler
{
protected:
	DefaultPropertyPage(HINSTANCE hInst,
						UINT nId);

public:
	virtual ~DefaultPropertyPage();

public:
	virtual INT_PTR dialogProc(UINT uMsg,
							   WPARAM wParam,
							   LPARAM lParam);

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

public:
	virtual LRESULT onNotify(NMHDR* pnmhdr,
							 bool* pbHandled);

protected:
	virtual LRESULT onDestroy();
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

protected:
	virtual LRESULT onOk();

private:
	LRESULT onApply(NMHDR* pnmhdr,
					bool* pbHandled);

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
	FileDialog(bool bOpen,
			   const WCHAR* pwszFilter,
			   const WCHAR* pwszInitialDir,
			   const WCHAR* pwszDefaultExt,
			   const WCHAR* pwszFileName,
			   DWORD dwFlags);
	~FileDialog();

public:
	const WCHAR* getPath() const;
	int doModal(HWND hwndParent);
	int doModal(HWND hwndParent,
				ModalHandler* pModalHandler);

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
					   const WCHAR* pwszInitialPath);
	virtual ~BrowseFolderDialog();

public:
	const WCHAR* getPath() const;

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

protected:
	virtual LRESULT onDestroy();
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

protected:
	virtual LRESULT onOk();

private:
	LRESULT onNewFolder();

private:
	BrowseFolderDialog(const BrowseFolderDialog&);
	BrowseFolderDialog& operator=(const BrowseFolderDialog&);

private:
	struct BrowseFolderDialogImpl* pImpl_;
};

#endif


/****************************************************************************
 *
 * FontDialog
 *
 */

class QSEXPORTCLASS FontDialog : public DefaultDialog
{
public:
	explicit FontDialog(const LOGFONT& lf);
	virtual ~FontDialog();

public:
	const LOGFONT& getLogFont() const;

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

protected:
	virtual LRESULT onOk();

private:
	FontDialog(const FontDialog&);
	FontDialog& operator=(const FontDialog&);

private:
	LOGFONT lf_;
};

}

#endif // __QSDIALOG_H__
