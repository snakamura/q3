/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QSWINDOW_H__
#define __QSWINDOW_H__

#include <qs.h>
#include <qsstring.h>
#include <qsosutil.h>

#include <windows.h>
#include <commctrl.h>

#ifndef _WIN32_WCE
#	ifndef WM_THEMECHANGED
#		define WM_THEMECHANGED 0x031A
#	endif
#else
#	define WM_CONTEXTMENU  0x007B
#	define WM_NCDESTROY    0x0082
#	ifndef ListView_SetCheckState
#		define ListView_SetCheckState(hwndLV, i, fCheck) \
			ListView_SetItemState(hwndLV, i, INDEXTOSTATEIMAGEMASK((fCheck)?2:1), LVIS_STATEIMAGEMASK)
#	endif
#endif


namespace qs {

class Window;
class DefWindowProcHolder;
	class WindowBase;
		class FrameWindow;
		class SplitterWindow;
		class ImeWindow;
#ifdef QS_KANJIIN
		class KanjiinWindow;
#endif
#if _WIN32_WCE >= 200 && (_WIN32_WCE < 300 || !defined _WIN32_WCE_PSPC)
		class CommandBand;
#endif
class CommandUpdate;
	class CommandUpdateMenu;
class CommandHandler;

class NotifyHandler;
class OwnerDrawHandler;
class ModalHandler;
class ModalHandlerInvoker;
class Accelerator;
class WindowHandler;
class DefaultWindowHandlerBase;
	class DefaultWindowHandler;
		class FrameWindow;
		class SplitterWindow;
#ifdef QS_KANJIIN
		class KanjiinWindow;
#endif
#if _WIN32_WCE >= 200 && (_WIN32_WCE < 300 || !defined _WIN32_WCE_PSPC)
		class CommandBand;
#endif
class SplitterWindowHandler;
class SplitterWindowEvent;
class DisableRedraw;
class Cursor;
class WaitCursor;

class Action;
class InitThread;
class Profile;


/****************************************************************************
 *
 * Global functions
 *
 */

QSEXPORTPROC int messageBox(HINSTANCE hInstResource,
							UINT nId);
QSEXPORTPROC int messageBox(HINSTANCE hInstResource,
							UINT nId,
							UINT nType);
QSEXPORTPROC int messageBox(HINSTANCE hInstResource,
							UINT nId,
							HWND hwnd);
QSEXPORTPROC int messageBox(HINSTANCE hInstResource,
							UINT nId,
							UINT nType,
							HWND hwnd);
QSEXPORTPROC int messageBox(HINSTANCE hInstResource,
							UINT nId,
							UINT nType,
							HWND hwnd,
							const WCHAR* pwszTitle,
							ModalHandler* pModalHandler);
QSEXPORTPROC int messageBox(const WCHAR* pwszMessage);
QSEXPORTPROC int messageBox(const WCHAR* pwszMessage,
							UINT nType);
QSEXPORTPROC int messageBox(const WCHAR* pwszMessage,
							HWND hwnd);
QSEXPORTPROC int messageBox(const WCHAR* pwszMessage,
							UINT nType,
							HWND hwnd);
QSEXPORTPROC int messageBox(const WCHAR* pwszMessage,
							UINT nType,
							HWND hwnd,
							const WCHAR* pwszTitle,
							ModalHandler* pModalHandler);


/****************************************************************************
 *
 * Window
 *
 */

class QSEXPORTCLASS Window
{
public:
	explicit Window(HWND hwnd);
	Window(const Window& window);
	~Window();

public:
	operator HWND() const;
	Window& operator=(HWND hwnd);
	Window& operator=(const Window& window);
	bool operator!() const;

public:
	HWND getHandle() const;
	bool updateWindow();
	bool destroyWindow();
	bool showWindow();
	bool showWindow(int nShow);
	bool isVisible() const;
	bool isIconic() const;
	DWORD getStyle() const;
	DWORD setStyle(DWORD dwStyle,
				   DWORD dwMask);
	bool enableWindow();
	bool enableWindow(bool bEnable);
	bool isWindowEnabled() const;
	HWND getWindow(UINT nCmd) const;
	
	bool getClientRect(RECT* pRect) const;
	bool getWindowRect(RECT* pRect) const;
	bool setWindowPos(HWND hwnd,
					  int x,
					  int y,
					  int cx,
					  int cy,
					  UINT nFlags);
	static HDWP beginDeferWindowPos(int nNumWindows);
	HDWP deferWindowPos(HDWP hdwp,
						HWND hwnd,
						int x,
						int y,
						int cx,
						int cy,
						UINT nFlags);
	static bool endDeferWindowPos(HDWP hdwp);
	bool moveWindow(int x,
					int y,
					int cx,
					int cy);
	bool moveWindow(int x,
					int y,
					int cx,
					int cy,
					bool bRepaint);
	
	bool screenToClient(POINT* pPoint) const;
	bool screenToClient(RECT* pRect) const;
	bool clientToScreen(POINT* pPoint) const;
	bool clientToScreen(RECT* pRect) const;
	
	bool centerWindow();
	bool centerWindow(HWND hwnd);
	
	HWND getParent() const;
	HWND setParent(HWND hwndParent);
	HWND getParentFrame() const;
	HWND getParentPopup() const;
	
#ifndef _WIN32_WCE
	bool getWindowPlacement(WINDOWPLACEMENT* pwp) const;
	bool setWindowPlacement(const WINDOWPLACEMENT& wp);
	bool redrawWindow(const RECT* pRect,
					  HRGN hrgn,
					  UINT nFlags);
#endif
	
	static HWND getFocus();
	bool hasFocus() const;
	HWND setFocus();
	
	static HWND getActiveWindow();
	static HWND getActiveFrame();
	bool isActive() const;
	HWND setActiveWindow();
	
	static HWND getForegroundWindow();
	bool setForegroundWindow();
	
#ifndef _WIN32_WCE
	bool flashWindow(bool bInvert);
#endif
	
	bool invalidate();
	bool invalidate(bool bErase);
	bool invalidateRect(const RECT& rect);
	bool invalidateRect(const RECT& rect,
						bool bErase);
	bool validate();
	bool validateRect(const RECT& rect);
	
	HDC beginPaint(PAINTSTRUCT* pps);
	bool endPaint(const PAINTSTRUCT& ps);
	HDC getDC() const;
	int releaseDC(HDC hdc) const;
	
	HFONT getFont() const;
	void setFont(HFONT hfont);
	void setFont(HFONT hfont,
				 bool bRedraw);
	
	bool createCaret(int nWidth,
					 int nHeight);
	bool createCaret(int nWidth,
					 int nHeight,
					 HBITMAP hBitmap);
	static bool destroyCaret();
	bool showCaret();
	bool hideCaret();
	static bool setCaretPos(const POINT& pt);
	static bool getCaretPos(POINT* pPoint);
	
	bool getScrollInfo(int nBar,
					   SCROLLINFO* psi) const;
	bool setScrollInfo(int nBar,
					   const SCROLLINFO& si);
	bool setScrollInfo(int nBar,
					   const SCROLLINFO& si,
					   bool bRedraw);
	int getScrollPos(int nBar) const;
	bool setScrollPos(int nBar,
					  int nPos);
	bool setScrollPos(int nBar,
					  int nPos,
					  bool bRedraw);
	bool getScrollRange(int nBar,
						int* pnMinPos,
						int* pnMaxPos) const;
	bool setScrollRange(int nBar,
						int nMinPos,
						int nMaxPos);
	bool setScrollRange(int nBar,
						int nMinPos,
						int nMaxPos,
						bool bRedraw);
	int scrollWindow(int x,
					 int y);
	int scrollWindow(int x,
					 int y,
					 const RECT* prcScroll,
					 const RECT* prcClip,
					 HRGN hrgnUpdate,
					 RECT* prcUpdate,
					 UINT nFlags);
	
	static HWND getCapture();
	static bool releaseCapture();
	bool isCapture() const;
	HWND setCapture();
	
	wstring_ptr getClassName() const;
	
	wstring_ptr getWindowText() const;
	bool setWindowText(const WCHAR* pwszText);
	int getWindowTextLength() const;
	
	long getWindowLong(int n) const;
	long setWindowLong(int n,
					   long l);
	
	UINT setTimer(UINT nId,
				  UINT nTimeout);
	UINT setTimer(UINT nId,
				  UINT nTimeout,
				  TIMERPROC proc);
	bool killTimer(UINT nId);
	
#ifndef _WIN32_WCE
	HMENU getMenu() const;
	bool setMenu(HMENU hmenu);
	
	void dragAcceptFiles(bool bAccept);
#endif
	
	bool postMessage(UINT uMsg);
	bool postMessage(UINT uMsg,
					 WPARAM wParam);
	bool postMessage(UINT uMsg,
					 WPARAM wParam,
					 LPARAM lParam);
	LRESULT sendMessage(UINT uMsg);
	LRESULT sendMessage(UINT uMsg,
						WPARAM wParam);
	LRESULT sendMessage(UINT uMsg,
						WPARAM wParam,
						LPARAM lParam);
	
	bool peekMessage(MSG* pMsg);
	bool peekMessage(MSG* pMsg,
					 UINT uMsgFilterMin,
					 UINT uMsgFilterMax,
					 UINT nRemoveMsg);
	
	HWND getDlgItem(int nDlgItem) const;
	int getDlgItemInt(int nDlgItem) const;
	int getDlgItemInt(int nDlgItem,
					  bool* pbTranslated,
					  bool bSigned) const;
	bool setDlgItemInt(int nDlgItem,
					   int nValue);
	bool setDlgItemInt(int nDlgItem,
					   int nValue,
					   bool bSigned);
	wstring_ptr getDlgItemText(int nDlgItem) const;
	bool setDlgItemText(int nDlgItem,
						const WCHAR* pwszText);
	LRESULT sendDlgItemMessage(int nDlgItem,
							   UINT uMsg);
	LRESULT sendDlgItemMessage(int nDlgItem,
							   UINT uMsg,
							   WPARAM wParam);
	LRESULT sendDlgItemMessage(int nDlgItem,
							   UINT uMsg,
							   WPARAM wParam,
							   LPARAM lParam);
	bool isDialogMessage(MSG* pMsg);
	
#if defined _WIN32_WCE && _WIN32_WCE >= 300 && defined _WIN32_WCE_PSPC
	bool tapAndHold(const POINT& pt);
#endif

public:
	void setHandle(HWND hwnd);

private:
	HWND hwnd_;
};


/****************************************************************************
 *
 * DefWindowProcHolder
 *
 */

class QSEXPORTCLASS DefWindowProcHolder
{
public:
	virtual ~DefWindowProcHolder();

public:
	virtual LRESULT defWindowProc(UINT uMsg,
								  WPARAM wParam,
								  LPARAM lParam) = 0;
};


/****************************************************************************
 *
 * WindowBase
 *
 */

class QSEXPORTCLASS WindowBase :
	public Window,
	public DefWindowProcHolder
{
public:
	explicit WindowBase(bool bDeleteThis);
	virtual ~WindowBase();

public:
	void setWindowHandler(WindowHandler* pWindowHandler,
						  bool bDeleteHandler);
	bool create(const WCHAR* pwszClassName,
				const WCHAR* pwszTitle,
				DWORD dwStyle,
				const RECT& rect,
				HWND hwndParent,
				DWORD dwExStyle,
				const WCHAR* pwszSuperClass,
				UINT nId,
				void* pParam);
	bool create(const WCHAR* pwszClassName,
				const WCHAR* pwszTitle,
				DWORD dwStyle,
				int x,
				int y,
				int cx,
				int cy,
				HWND hwndParent,
				DWORD dwExStyle,
				const WCHAR* pwszSuperClass,
				UINT nId,
				void* pParam);
	
	bool subclassWindow(HWND hwnd);
	bool unsubclassWindow();
	
	void addCommandHandler(CommandHandler* pch);
	void removeCommandHandler(CommandHandler* pch);
	
	void addNotifyHandler(NotifyHandler* pnh);
	void removeNotifyHandler(NotifyHandler* pnh);
	
	void addOwnerDrawHandler(OwnerDrawHandler* podh);
	void removeOwnerDrawHandler(OwnerDrawHandler* podh);
	
	Accelerator* getAccelerator() const;
	bool preTranslateAccelerator(const MSG& msg);
	bool isFrame() const;
	
	InitThread* getInitThread() const;

public:
	virtual LRESULT defWindowProc(UINT uMsg,
								  WPARAM wParam,
								  LPARAM lParam);

public:
	static bool translateAccelerator(const MSG& msg);

private:
	WindowBase(const WindowBase&);
	WindowBase& operator=(const WindowBase&);

private:
	class WindowBaseImpl* pImpl_;

friend class WindowBaseImpl;
#if defined _WIN32_WCE && !defined _WIN32_WCE_EMULATION
friend class WindowDestroy;
#endif
friend LRESULT CALLBACK windowProc(HWND,
								   UINT,
								   WPARAM,
								   LPARAM);
};


/****************************************************************************
 *
 * CommandUpdate
 *
 */

class QSEXPORTCLASS CommandUpdate
{
public:
	virtual ~CommandUpdate();

public:
	virtual UINT getId() const = 0;
	virtual void setEnable();
	virtual void setEnable(bool bEnable) = 0;
	virtual void setCheck();
	virtual void setCheck(bool bCheck) = 0;
	virtual void setText(const WCHAR* pwszText);
	virtual void setText(const WCHAR* pwszText,
						 bool bWithoutAccel) = 0;
	virtual void setText(HINSTANCE hInstResource,
						 UINT nId);
	virtual void setText(HINSTANCE hInstResource,
						 UINT nId,
						 bool bWithoutAccel);
	virtual wstring_ptr getText() const = 0;
	virtual void updateText() = 0;
};


/****************************************************************************
 *
 * CommandUpdateMenu
 *
 */

class QSEXPORTCLASS CommandUpdateMenu : public CommandUpdate
{
public:
	CommandUpdateMenu(HMENU hmenu,
					  UINT nId);
	virtual ~CommandUpdateMenu();

public:
	HMENU getMenu() const;

public:
	virtual UINT getId() const;
	virtual void setEnable(bool bEnable);
	virtual void setCheck(bool bCheck);
	virtual void setText(const WCHAR* pwszText,
						 bool bWithoutAccel);
	virtual wstring_ptr getText() const;
	virtual void updateText();

private:
	CommandUpdateMenu(const CommandUpdateMenu&);
	CommandUpdateMenu& operator=(const CommandUpdateMenu&);

private:
	HMENU hmenu_;
	UINT nId_;
};


/****************************************************************************
 *
 * CommandUpdateToolbar
 *
 */

class QSEXPORTCLASS CommandUpdateToolbar : public CommandUpdate
{
public:
	CommandUpdateToolbar(HWND hwnd,
						 UINT nId);
	virtual ~CommandUpdateToolbar();

public:
	virtual UINT getId() const;
	virtual void setEnable(bool bEnable);
	virtual void setCheck(bool bCheck);
	virtual void setText(const WCHAR* pwszText,
						 bool bWithoutAccel);
	virtual wstring_ptr getText() const;
	virtual void updateText();

private:
	CommandUpdateToolbar(const CommandUpdateToolbar&);
	CommandUpdateToolbar& operator=(const CommandUpdateToolbar&);

private:
	HWND hwnd_;
	UINT nId_;
};


/****************************************************************************
 *
 * CommandHandler
 *
 */

class QSEXPORTCLASS CommandHandler
{
public:
	virtual ~CommandHandler();

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId) = 0;
};

#define BEGIN_COMMAND_HANDLER() \
	if (false) \
		; \

#define END_COMMAND_HANDLER() \

#define HANDLE_COMMAND_ID(id, handler) \
	else if (nId == id) \
		return handler(); \

#define HANDLE_COMMAND_CODE(code, handler) \
	else if (nCode == code) \
		return handler(); \

#define HANDLE_COMMAND_ID_CODE(id, code, handler) \
	else if (nId == id && nCode == code) \
		return handler(); \

#define HANDLE_COMMAND_ID_RANGE(idFrom, idTo, handler) \
	else if (idFrom <= nId && nId <= idTo) \
		return handler(nId); \

#define HANDLE_COMMAND_ID_RANGE_CODE(idFrom, idTo, code, handler) \
	else if (idFrom <= nId && nId <= idTo && nCode == code) \
		return handler(nId); \

#define BEGIN_UPDATE_COMMAND_ID_HANDLER() \
	switch (pcu->getId()) { \

#define END_UPDATE_COMMAND_ID_HANDLER() \
	} \

#define HANDLE_UPDATE_COMMAND_ID(id, handler) \
	case id: \
		handler(pcu); \
		break; \


/****************************************************************************
 *
 * NotifyHandler
 *
 */

class QSEXPORTCLASS NotifyHandler
{
public:
	virtual ~NotifyHandler();

public:
	virtual LRESULT onNotify(NMHDR* pnmhdr,
							 bool* pbHandled) = 0;
};

#define BEGIN_NOTIFY_HANDLER() \
	if (false) { \
	} \

#define END_NOTIFY_HANDLER() \

#define HANDLE_NOTIFY(code_, id, handler) \
	else if (pnmhdr->code == code_ && pnmhdr->idFrom == id) { \
		return handler(pnmhdr, pbHandled); \
	} \

#define HANDLE_NOTIFY_CODE(code_, handler) \
	else if (pnmhdr->code == code_) { \
		return handler(pnmhdr, pbHandled); \
	} \


/****************************************************************************
 *
 * OwnerDrawHandler
 *
 */

class QSEXPORTCLASS OwnerDrawHandler
{
public:
	virtual ~OwnerDrawHandler();

public:
	virtual void onDrawItem(DRAWITEMSTRUCT* pDrawItemStruct) = 0;
	virtual void onMeasureItem(MEASUREITEMSTRUCT* pMeasureItemStruct) = 0;
};


/****************************************************************************
 *
 * ModalHandler
 *
 */

class QSEXPORTCLASS ModalHandler
{
public:
	virtual ~ModalHandler();

public:
	virtual void preModalDialog(HWND hwndParent) = 0;
	virtual void postModalDialog(HWND hwndParent) = 0;
};


/****************************************************************************
 *
 * ModalHandlerInvoker
 *
 */

class QSEXPORTCLASS ModalHandlerInvoker
{
public:
	ModalHandlerInvoker(ModalHandler* pModalHandler,
						HWND hwnd);
	~ModalHandlerInvoker();

private:
	ModalHandlerInvoker(const ModalHandlerInvoker&);
	ModalHandlerInvoker& operator=(const ModalHandlerInvoker&);

private:
	ModalHandler* pModalHandler_;
	HWND hwnd_;
};


/****************************************************************************
 *
 * WindowHandler
 *
 */

class QSEXPORTCLASS WindowHandler
{
public:
	virtual ~WindowHandler();

public:
	virtual void setWindowBase(WindowBase* pWindowBase) = 0;
	
	virtual wstring_ptr getSuperClass() = 0;
	virtual void getWindowClass(WNDCLASS* pwc) = 0;
	virtual bool getWindowClass(const WCHAR* pwszSuperClass,
								WNDCLASS* pwc,
								WNDPROC* pproc) = 0;
	
	virtual bool preCreateWindow(CREATESTRUCT* pCreateStruct) = 0;
	virtual bool preSubclassWindow() = 0;
	
	virtual Action* getAction(UINT nId) = 0;
	virtual Accelerator* getAccelerator() = 0;
	virtual bool preTranslateAccelerator(const MSG& msg) = 0;
	virtual bool isFrame() const = 0;
	
	virtual LRESULT windowProc(UINT uMsg,
							   WPARAM wParam,
							   LPARAM lParam) = 0;
};

#define BEGIN_MESSAGE_HANDLER() \
	bool bProcessed = true; \
	LRESULT lResult = 0; \
	switch (uMsg) { \

#define END_MESSAGE_HANDLER() \
	default: \
		bProcessed = false; \
		break; \
	} \
	if (bProcessed) \
		return lResult; \

#define HANDLE_ACTIVATE() \
	case WM_ACTIVATE: \
		lResult = onActivate(LOWORD(wParam), reinterpret_cast<HWND>(lParam), HIWORD(wParam) != 0); \
		break; \

#ifndef _WIN32_WCE
#define HANDLE_ACTIVATEAPP() \
	case WM_ACTIVATEAPP: \
		lResult = onActivateApp(wParam != 0, lParam); \
		break; \

#endif

#define HANDLE_CHAR() \
	case WM_CHAR: \
		lResult = onChar(wParam, lParam & 0xffff, lParam); \
		break; \

#define HANDLE_CLOSE() \
	case WM_CLOSE: \
		lResult = onClose(); \
		break; \

#define HANDLE_COMMAND() \
	case WM_COMMAND: \
		lResult = onCommand(HIWORD(wParam), LOWORD(wParam), reinterpret_cast<HWND>(lParam)); \
		break; \

#define HANDLE_CONTEXTMENU() \
	case WM_CONTEXTMENU: \
		lResult = onContextMenu(reinterpret_cast<HWND>(wParam), \
			Point(static_cast<short>(LOWORD(lParam)), \
				static_cast<short>(HIWORD(lParam)))); \
		break; \

#define HANDLE_COPY() \
	case WM_COPY: \
		lResult = onCopy(); \
		break; \

#define HANDLE_COPYDATA() \
	case WM_COPYDATA: \
		lResult = onCopyData(reinterpret_cast<HWND>(wParam), \
			reinterpret_cast<COPYDATASTRUCT*>(lParam)); \
		break; \

#define HANDLE_CREATE() \
	case WM_CREATE: \
		lResult = onCreate(reinterpret_cast<CREATESTRUCT*>(lParam)); \
		break; \

#define HANDLE_CTLCOLOREDIT() \
	case WM_CTLCOLOREDIT: \
		lResult = onCtlColorEdit(reinterpret_cast<HDC>(wParam), \
			reinterpret_cast<HWND>(lParam)); \
		break; \

#define HANDLE_CTLCOLORSTATIC() \
	case WM_CTLCOLORSTATIC: \
		lResult = onCtlColorStatic(reinterpret_cast<HDC>(wParam), \
			reinterpret_cast<HWND>(lParam)); \
		break; \

#define HANDLE_CUT() \
	case WM_CUT: \
		lResult = onCut(); \
		break; \

#define HANDLE_DESTROY() \
	case WM_DESTROY: \
		lResult = onDestroy(); \
		break; \

#ifndef _WIN32_WCE
#define HANDLE_DROPFILES() \
	case WM_DROPFILES: \
		lResult = onDropFiles(reinterpret_cast<HDROP>(wParam)); \
		break; \

#define HANDLE_ENDSESSION() \
	case WM_ENDSESSION: \
		lResult = onEndSession(wParam != 0, lParam); \
		break; \

#endif

#define HANDLE_ERASEBKGND() \
	case WM_ERASEBKGND: \
		lResult = onEraseBkgnd(reinterpret_cast<HDC>(wParam)); \
		break; \

#define HANDLE_HOTKEY() \
	case WM_HOTKEY: \
		lResult = onHotKey(wParam, LOWORD(lParam), HIWORD(lParam)); \
		break; \

#define HANDLE_HSCROLL() \
	case WM_HSCROLL: \
		lResult = onHScroll(LOWORD(wParam), HIWORD(wParam), \
			reinterpret_cast<HWND>(lParam)); \
		break; \

#define HANDLE_IME_CHAR() \
	case WM_IME_CHAR: \
		lResult = onImeChar(wParam, lParam & 0xffff, lParam); \
		break; \

#define HANDLE_IME_COMPOSITION() \
	case WM_IME_COMPOSITION: \
		lResult = onImeComposition(wParam, lParam); \
		break; \

#define HANDLE_IME_ENDCOMPOSITION() \
	case WM_IME_ENDCOMPOSITION: \
		lResult = onImeEndComposition(); \
		break; \

#define HANDLE_IME_STARTCOMPOSITION() \
	case WM_IME_STARTCOMPOSITION: \
		lResult = onImeStartComposition(); \
		break; \

#define HANDLE_INITDIALOG() \
	case WM_INITDIALOG: \
		lResult = onInitDialog(reinterpret_cast<HWND>(wParam), lParam); \
		break;

#ifndef _WIN32_WCE
#define HANDLE_INITMENU() \
	case WM_INITMENU: \
		lResult = onInitMenu(reinterpret_cast<HMENU>(wParam)); \
		break; \

#endif

#define HANDLE_INITMENUPOPUP() \
	case WM_INITMENUPOPUP: \
		lResult = onInitMenuPopup(reinterpret_cast<HMENU>(wParam), \
			LOWORD(lParam), HIWORD(lParam) != 0); \
		break; \

#define HANDLE_KEYDOWN() \
	case WM_KEYDOWN: \
		lResult = onKeyDown(wParam, lParam & 0xffff, lParam); \
		break; \

#define HANDLE_KEYUP() \
	case WM_KEYUP: \
		lResult = onKeyUp(wParam, lParam & 0xffff, lParam); \
		break; \

#define HANDLE_KILLFOCUS() \
	case WM_KILLFOCUS: \
		lResult = onKillFocus(reinterpret_cast<HWND>(wParam)); \
		break; \

#define HANDLE_LBUTTONDBLCLK() \
	case WM_LBUTTONDBLCLK: \
		lResult = onLButtonDblClk(wParam, \
			Point(static_cast<short>(LOWORD(lParam)), \
				static_cast<short>(HIWORD(lParam)))); \
		break; \

#define HANDLE_LBUTTONDOWN() \
	case WM_LBUTTONDOWN: \
		lResult = onLButtonDown(wParam, \
			Point(static_cast<short>(LOWORD(lParam)), \
				static_cast<short>(HIWORD(lParam)))); \
		break; \

#define HANDLE_LBUTTONUP() \
	case WM_LBUTTONUP: \
		lResult = onLButtonUp(wParam, \
			Point(static_cast<short>(LOWORD(lParam)), \
				static_cast<short>(HIWORD(lParam)))); \
		break; \

#if !defined _WIN32_WCE || _WIN32_WCE >= 400
#define HANDLE_MBUTTONDBLCLK() \
	case WM_MBUTTONDBLCLK: \
		lResult = onMButtonDblClk(wParam, \
			Point(static_cast<short>(LOWORD(lParam)), \
				static_cast<short>(HIWORD(lParam)))); \
		break; \

#define HANDLE_MBUTTONDOWN() \
	case WM_MBUTTONDOWN: \
		lResult = onMButtonDown(wParam, \
			Point(static_cast<short>(LOWORD(lParam)), \
				static_cast<short>(HIWORD(lParam)))); \
		break; \

#define HANDLE_MBUTTONUP() \
	case WM_MBUTTONUP: \
		lResult = onMButtonUp(wParam, \
			Point(static_cast<short>(LOWORD(lParam)), \
				static_cast<short>(HIWORD(lParam)))); \
		break; \

#endif

#define HANDLE_MOUSEACTIVATE() \
	case WM_MOUSEACTIVATE: \
		lResult = onMouseActivate(reinterpret_cast<HWND>(wParam), \
			LOWORD(lParam), HIWORD(lParam)); \
		break; \

#define HANDLE_MOUSEMOVE() \
	case WM_MOUSEMOVE: \
		lResult = onMouseMove(wParam, \
			Point(static_cast<short>(LOWORD(lParam)), \
				static_cast<short>(HIWORD(lParam)))); \
		break; \

#if !defined _WIN32_WCE || _WIN32_WCE >= 211
#define HANDLE_MOUSEWHEEL() \
	case WM_MOUSEWHEEL: \
		lResult = onMouseWheel(LOWORD(wParam), HIWORD(wParam), \
			Point(static_cast<short>(LOWORD(lParam)), \
				static_cast<short>(HIWORD(lParam)))); \
		break; \

#endif

#ifndef _WIN32_WCE
#define HANDLE_NCHITTEST() \
	case WM_NCHITTEST: \
		lResult = onNcHitTest(Point(static_cast<short>(LOWORD(lParam)), \
			static_cast<short>(HIWORD(lParam)))); \
		break; \

#define HANDLE_NCPAINT() \
	case WM_NCPAINT: \
		lResult = onNcPaint(reinterpret_cast<HRGN>(wParam)); \
		break; \

#endif

#define HANDLE_PAINT() \
	case WM_PAINT: \
		lResult = onPaint(); \
		break; \

#define HANDLE_PASTE() \
	case WM_PASTE: \
		lResult = onPaste(); \
		break; \

#ifndef _WIN32_WCE
#define HANDLE_QUERYENDSESSION() \
	case WM_QUERYENDSESSION: \
		lResult = onQueryEndSession(lParam); \
		break; \

#endif

#define HANDLE_RBUTTONDBLCLK() \
	case WM_RBUTTONDBLCLK: \
		lResult = onRButtonDblClk(wParam, \
			Point(static_cast<short>(LOWORD(lParam)), \
				static_cast<short>(HIWORD(lParam)))); \
		break; \

#define HANDLE_RBUTTONDOWN() \
	case WM_RBUTTONDOWN: \
		lResult = onRButtonDown(wParam, \
			Point(static_cast<short>(LOWORD(lParam)), \
				static_cast<short>(HIWORD(lParam)))); \
		break; \

#define HANDLE_RBUTTONUP() \
	case WM_RBUTTONUP: \
		lResult = onRButtonUp(wParam, \
			Point(static_cast<short>(LOWORD(lParam)), \
				static_cast<short>(HIWORD(lParam)))); \
		break; \

#define HANDLE_SETCURSOR() \
	case WM_SETCURSOR: \
		lResult = onSetCursor(reinterpret_cast<HWND>(wParam), \
			LOWORD(lParam), HIWORD(lParam)); \
		break; \

#define HANDLE_SETFOCUS() \
	case WM_SETFOCUS: \
		lResult = onSetFocus(reinterpret_cast<HWND>(wParam)); \
		break; \

#define HANDLE_SETTINGCHANGE() \
	case WM_SETTINGCHANGE: \
		lResult = onSettingChange(wParam, lParam); \
		break; \

#define HANDLE_SHOWWINDOW() \
	case WM_SHOWWINDOW: \
		lResult = onShowWindow(wParam != 0, lParam); \
		break; \

#define HANDLE_SIZE() \
	case WM_SIZE: \
		lResult = onSize(wParam, LOWORD(lParam), HIWORD(lParam)); \
		break; \

#define HANDLE_SYSCHAR() \
	case WM_SYSCHAR: \
		lResult = onSysChar(wParam, lParam & 0xffff, lParam); \
		break; \

#define HANDLE_SYSKEYDOWN() \
	case WM_SYSKEYDOWN: \
		lResult = onSysKeyDown(wParam, lParam & 0xffff, lParam); \
		break; \

#define HANDLE_SYSKEYUP() \
	case WM_SYSKEYUP: \
		lResult = onSysKeyUp(wParam, lParam & 0xffff, lParam); \
		break; \

#ifndef _WIN32_WCE
#define HANDLE_THEMECHANGED() \
	case WM_THEMECHANGED: \
		lResult = onThemeChanged(); \
		break; \

#endif

#define HANDLE_TIMER() \
	case WM_TIMER: \
		lResult = onTimer(wParam); \
		break; \

#define HANDLE_VSCROLL() \
	case WM_VSCROLL: \
		lResult = onVScroll(LOWORD(wParam), HIWORD(wParam), \
			reinterpret_cast<HWND>(lParam)); \
		break; \

#define HANDLE_WINDOWPOSCHANGED() \
	case WM_WINDOWPOSCHANGED: \
		lResult = onWindowPosChanged(reinterpret_cast<WINDOWPOS*>(lParam)); \
		break; \

#ifndef _WIN32_WCE
#define HANDLE_WINDOWPOSCHANGING() \
	case WM_WINDOWPOSCHANGING: \
		lResult = onWindowPosChanging(reinterpret_cast<WINDOWPOS*>(lParam)); \
		break; \

#endif

#define HANDLE_MESSAGE(message, handler) \
	case message: \
		lResult = handler(wParam, lParam); \
		break; \

#define BEGIN_REGISTERED_MESSAGE_HANDLER() \
	bool bRegisteredProcessed = true; \
	LRESULT lRegisteredResult = 0; \
	if (false) { \
	} \

#define END_REGISTERED_MESSAGE_HANDLER() \
	else { \
		bRegisteredProcessed = false; \
	} \
	if (bRegisteredProcessed) \
		return lRegisteredResult; \

#define HANDLE_REGISTERED_MESSAGE(message, handler) \
	else if (uMsg == message) { \
		lRegisteredResult = handler(wParam, lParam); \
	} \


/****************************************************************************
 *
 * DefaultWindowHandlerBase
 *
 */

class QSEXPORTCLASS DefaultWindowHandlerBase
{
public:
	virtual ~DefaultWindowHandlerBase();

public:
	virtual DefWindowProcHolder* getDefWindowProcHolder() = 0;

protected:
	LRESULT onActivate(UINT nFlags,
					   HWND hwnd,
					   bool bMinimized);
#ifndef _WIN32_WCE
	LRESULT onActivateApp(bool bActivate,
						  DWORD dwThreadId);
#endif
	LRESULT onChar(UINT nChar,
				   UINT nRepeat,
				   UINT nFlags);
	LRESULT onClose();
	LRESULT onCommand(UINT nCode,
					  UINT nId,
					  HWND hwnd);
	LRESULT onContextMenu(HWND hwnd,
						  const POINT& pt);
	LRESULT onCopy();
	LRESULT onCopyData(HWND hwnd,
					   COPYDATASTRUCT* pData);
	LRESULT onCreate(CREATESTRUCT* pCreateStruct);
	LRESULT onCtlColorEdit(HDC hdc,
						   HWND hwnd);
	LRESULT onCtlColorStatic(HDC hdc,
							 HWND hwnd);
	LRESULT onCut();
	LRESULT onDestroy();
	LRESULT onEraseBkgnd(HDC hdc);
#ifndef _WIN32_WCE
	LRESULT onDropFiles(HDROP hdrop);
	LRESULT onEndSession(bool bEnd,
						 int nOption);
#endif
	LRESULT onHotKey(UINT nId,
					 UINT nModifier,
					 UINT nKey);
	LRESULT onHScroll(UINT nCode,
					  UINT nPos,
					  HWND hwnd);
	LRESULT onImeChar(UINT nChar,
					  UINT nRepeat,
					  UINT nFlags);
	LRESULT onImeComposition(UINT nChar,
							 UINT nFlags);
	LRESULT onImeEndComposition();
	LRESULT onImeStartComposition();
	LRESULT onInitDialog(HWND hwndFocus,
						 LPARAM lParam);
#ifndef _WIN32_WCE
	LRESULT onInitMenu(HMENU hmenu);
#endif
	LRESULT onInitMenuPopup(HMENU hmenu,
							UINT nIndex,
							bool bSysMenu);
	LRESULT onKeyDown(UINT nKey,
					  UINT nRepeat,
					  UINT nFlags);
	LRESULT onKeyUp(UINT nKey,
					UINT nRepeat,
					UINT nFlags);
	LRESULT onKillFocus(HWND hwnd);
	LRESULT onLButtonDblClk(UINT nFlags,
							const POINT& pt);
	LRESULT onLButtonDown(UINT nFlags,
						  const POINT& pt);
	LRESULT onLButtonUp(UINT nFlags,
						const POINT& pt);
#if !defined _WIN32_WCE || _WIN32_WCE >= 400
	LRESULT onMButtonDblClk(UINT nFlags,
							const POINT& pt);
	LRESULT onMButtonDown(UINT nFlags,
						  const POINT& pt);
	LRESULT onMButtonUp(UINT nFlags,
						const POINT& pt);
#endif
#ifndef _WIN32_WCE
	LRESULT onMouseActivate(HWND hwnd,
							UINT nHitTest,
							UINT uMsg);
#endif
	LRESULT onMouseMove(UINT nFlags,
						const POINT& pt);
#if !defined _WIN32_WCE || _WIN32_WCE >= 211
	LRESULT onMouseWheel(UINT nFlags,
						 short nDelta,
						 const POINT& pt);
#endif
#ifndef _WIN32_WCE
	LRESULT onNcHitTest(const POINT& pt);
	LRESULT onNcPaint(HRGN hrgn);
#endif
	LRESULT onPaint();
	LRESULT onPaste();
#ifndef _WIN32_WCE
	LRESULT onQueryEndSession(int nOption);
#endif
	LRESULT onRButtonDblClk(UINT nFlags,
							const POINT& pt);
	LRESULT onRButtonDown(UINT nFlags,
						  const POINT& pt);
	LRESULT onRButtonUp(UINT nFlags,
						const POINT& pt);
	LRESULT onSetCursor(HWND hwnd,
						UINT nHitTest,
						UINT nMessage);
	LRESULT onSetFocus(HWND hwnd);
	LRESULT onSettingChange(WPARAM wParam,
							LPARAM lParam);
	LRESULT onShowWindow(bool bShow,
						 UINT nStatus);
	LRESULT onSize(UINT nFlags,
				   int cx,
				   int cy);
	LRESULT onSysChar(UINT nKey,
					  UINT nRepeat,
					  UINT nFlags);
	LRESULT onSysKeyDown(UINT nKey,
						 UINT nRepeat,
						 UINT nFlags);
	LRESULT onSysKeyUp(UINT nKey,
					   UINT nRepeat,
					   UINT nFlags);
#ifndef _WIN32_WCE
	LRESULT onThemeChanged();
#endif
	LRESULT onTimer(UINT nId);
	LRESULT onVScroll(UINT nCode,
					  UINT nPos,
					  HWND hwnd);
	LRESULT onWindowPosChanged(WINDOWPOS* pWindowPos);
#ifndef _WIN32_WCE
	LRESULT onWindowPosChanging(WINDOWPOS* pWindowPos);
#endif
};


/****************************************************************************
 *
 * DefaultWindowHandler
 *
 */

class QSEXPORTCLASS DefaultWindowHandler :
	public WindowHandler,
	public DefaultWindowHandlerBase
{
public:
	DefaultWindowHandler();
	virtual ~DefaultWindowHandler();

public:
	WindowBase* getWindowBase() const;

public:
	virtual void setWindowBase(WindowBase* pWindowBase);
	virtual wstring_ptr getSuperClass();
	virtual void getWindowClass(WNDCLASS* pwc);
	virtual bool getWindowClass(const WCHAR* pwszSuperClass,
								WNDCLASS* pwc,
								WNDPROC* pproc);
	virtual bool preCreateWindow(CREATESTRUCT* pCreateStruct);
	virtual bool preSubclassWindow();
	virtual Action* getAction(UINT nId);
	virtual Accelerator* getAccelerator();
	virtual bool preTranslateAccelerator(const MSG& msg);
	virtual bool isFrame() const;
	virtual LRESULT windowProc(UINT uMsg,
							   WPARAM wParam,
							   LPARAM lParam);

public:
	virtual DefWindowProcHolder* getDefWindowProcHolder();

private:
	WindowBase* pWindowBase_;
};


/****************************************************************************
 *
 * MessageLoop
 *
 */

class QSEXPORTCLASS MessageLoop
{
public:
	MessageLoop();
	~MessageLoop();

public:
	void run();

public:
	void addFrame(FrameWindow* pFrameWindow);
	void removeFrame(FrameWindow* pFrameWindow);

public:
	static MessageLoop& getMessageLoop();

private:
	MessageLoop(const MessageLoop&);
	MessageLoop& operator=(const MessageLoop&);

private:
	class MessageLoopImpl* pImpl_;
};


/****************************************************************************
 *
 * FrameWindow
 *
 */

class QSEXPORTCLASS FrameWindow :
	public WindowBase,
	public DefaultWindowHandler
{
public:
	struct Toolbar
	{
		TBBUTTON* ptbButton_;
		int nSize_;
		int nId_;
		int nBitmapId_;
		int nBitmapCount_;
	};

public:
	FrameWindow(HINSTANCE hInstResource,
				bool bDeleteThis);
	virtual ~FrameWindow();

public:
	HWND getToolbar() const;
	int getToolbarHeight() const;
	void adjustWindowSize(LPARAM lParam);

public:
	virtual void processIdle();

protected:
	bool save();

protected:
	virtual bool getToolbarButtons(Toolbar* pToolbar);
	virtual bool createToolbarButtons(void* pCreateParam,
									  HWND hwndToolbar);
#ifdef _WIN32_WCE
	virtual UINT getBarId(int n) const = 0;
	virtual bool getCommandBandsRestoreInfo(int n,
											COMMANDBANDSRESTOREINFO* pcbri) const = 0;
	virtual bool setCommandBandsRestoreInfo(int n,
											const COMMANDBANDSRESTOREINFO& cbri) = 0;
#endif
	virtual HMENU getMenuHandle(void* pCreateParam);
	virtual UINT getMenuId();
	virtual UINT getIconId();

public:
	virtual bool isFrame() const;
	virtual LRESULT windowProc(UINT uMsg,
							   WPARAM wParam,
							   LPARAM lParam);

protected:
	LRESULT onActivate(UINT nFlags,
					   HWND hwnd,
					   bool bMinimized);
	LRESULT onCreate(CREATESTRUCT* pCreateStruct);
	LRESULT onDestroy();
	LRESULT onInitMenuPopup(HMENU hmenu,
							UINT nIndex,
							bool bSysMenu);
	LRESULT onSettingChange(WPARAM wParam,
							LPARAM lParam);

private:
	FrameWindow(const FrameWindow&);
	FrameWindow& operator=(const FrameWindow&);

private:
	struct FrameWindowImpl* pImpl_;
};


/****************************************************************************
 *
 * SplitterWindow
 *
 */

class QSEXPORTCLASS SplitterWindow :
	public WindowBase,
	public DefaultWindowHandler
{
public:
	SplitterWindow(int nColumn,
				   int nRow,
				   bool bDeleteThis,
				   SplitterWindowHandler* pHandler);
	virtual ~SplitterWindow();

public:
	void add(int nColumn,
			 int nRow,
			 Window* pWindow);
	void setPane(int nColumn,
				 int nRow,
				 Window* pWindow);
	void showPane(int nColumn,
				  int nRow,
				  bool bShow);
	bool isShowPane(int nColumn,
					int nRow);
	
	int getColumnWidth(int nColumn);
	void setColumnWidth(int nColumn,
						int nWidth);
	
	int getRowHeight(int nRow);
	void setRowHeight(int nRow,
					  int nHeight);

public:
	virtual void getWindowClass(WNDCLASS* pwc);
	virtual LRESULT windowProc(UINT uMsg,
							   WPARAM wParam,
							   LPARAM lParam);

protected:
	LRESULT onLButtonDown(UINT nFlags,
						  const POINT& pt);
	LRESULT onLButtonUp(UINT nFlags,
						const POINT& pt);
	LRESULT onMouseMove(UINT nFlags,
						const POINT& pt);
	LRESULT onSize(UINT nFlags,
				   int cx,
				   int cy);

private:
	SplitterWindow(const SplitterWindow&);
	SplitterWindow& operator=(const SplitterWindow&);

private:
	struct SplitterWindowImpl* pImpl_;
};


/****************************************************************************
 *
 * SplitterWindowHandler
 *
 */

class QSEXPORTCLASS SplitterWindowHandler
{
public:
	virtual ~SplitterWindowHandler();

public:
	virtual void sizeChanged(const SplitterWindowEvent& event) = 0;
};


/****************************************************************************
 *
 * SplitterWindowEvent
 *
 */

class QSEXPORTCLASS SplitterWindowEvent
{
public:
	SplitterWindowEvent(SplitterWindow* pSplitterWindow);
	~SplitterWindowEvent();

public:
	SplitterWindow* getSplitterWindow() const;

private:
	SplitterWindowEvent(const SplitterWindowEvent&);
	SplitterWindowEvent& operator=(const SplitterWindowEvent&);

private:
	SplitterWindow* pSplitterWindow_;
};


/****************************************************************************
 *
 * ImeWindow
 *
 */

class QSEXPORTCLASS ImeWindow :
	public WindowBase,
	public DefaultWindowHandler
{
public:
	ImeWindow(Profile* pProfile,
			  const WCHAR* pwszSection,
			  const WCHAR* pwszKey,
			  bool bDeleteThis);
	virtual ~ImeWindow();

public:
	virtual LRESULT windowProc(UINT uMsg,
							   WPARAM wParam,
							   LPARAM lParam);

protected:
	LRESULT onCreate(CREATESTRUCT* pCreateStruct);
	LRESULT onDestroy();
	LRESULT onKillFocus(HWND hwnd);
	LRESULT onSetFocus(HWND hwnd);

private:
	ImeWindow(const ImeWindow&);
	ImeWindow& operator=(const ImeWindow&);

private:
	Profile* pProfile_;
	const WCHAR* pwszSection_;
	const WCHAR* pwszKey_;
	bool bIme_;
};


#ifdef QS_KANJIIN

/****************************************************************************
 *
 * KanjiinWindow
 *
 */

class QSEXPORTCLASS KanjiinWindow :
	public WindowBase,
	public DefaultWindowHandler
{
public:
	explicit KanjiinWindow(bool bDeleteThis);
	virtual ~KanjiinWindow();

public:
	virtual LRESULT windowProc(UINT uMsg,
							   WPARAM wParam,
							   LPARAM lParam);

protected:
	LRESULT onChar(UINT nChar,
				   UINT nRepeat,
				   UINT nFlags);
	LRESULT onCommand(UINT nCode,
					  UINT nId,
					  HWND hwnd);
	LRESULT onSysChar(UINT nKey,
					  UINT nRepeat,
					  UINT nFlags);
	LRESULT onSysKeyDown(UINT nKey,
						 UINT nRepeat,
						 UINT nFlags);

private:
	struct KanjiinWindowImpl* pImpl_;
};

#endif // QS_KANJIIN


#if _WIN32_WCE >= 200

/****************************************************************************
 *
 * CommandBand
 *
 */

class QSEXPORTCLASS CommandBand :
	public WindowBase,
	public DefaultWindowHandler
{
public:
	explicit CommandBand(bool bDeleteThis);
	virtual ~CommandBand();

public:
	virtual LRESULT windowProc(UINT uMsg,
							   WPARAM wParam,
							   LPARAM lParam);

protected:
	LRESULT onSize(UINT nFlags,
				   int cx,
				   int cy);

private:
	CommandBand(const CommandBand&);
	CommandBand& operator=(const CommandBand&);
};

#endif // _WIN32_WCE >= 200


/****************************************************************************
 *
 * DisableRedraw
 *
 */

class QSEXPORTCLASS DisableRedraw
{
public:
	explicit DisableRedraw(HWND hwnd);
	~DisableRedraw();

private:
	DisableRedraw(const DisableRedraw&);
	DisableRedraw& operator=(const DisableRedraw&);

private:
	HWND hwnd_;
};


/****************************************************************************
 *
 * Cursor
 *
 */

class QSEXPORTCLASS Cursor
{
public:
	explicit Cursor(HCURSOR cursor);
	~Cursor();

public:
	void reset();

private:
	Cursor(const Cursor&);
	Cursor& operator=(const Cursor&);

private:
	HCURSOR hcursor_;
	bool bReset_;
};


/****************************************************************************
 *
 * WaitCursor
 *
 */

class QSEXPORTCLASS WaitCursor
{
public:
	WaitCursor();
	~WaitCursor();

public:
	void reset();

private:
	WaitCursor(const WaitCursor&);
	WaitCursor& operator=(const WaitCursor&);

private:
	Cursor cursor_;
};

}

#include <qswindow.inl>

#endif // __QSWINDOW_H__
