/*
 * $Id: qstextviewwindow.h,v 1.1.1.1 2003/04/29 08:07:35 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#if 0

#ifndef __QSTEXTVIEWWINDOW_H__
#define __QSTEXTVIEWWINDOW_H__

#include <qswindow.h>


namespace qs {

class Reader;

/****************************************************************************
 *
 * TextViewWindow
 *
 */

class QSEXPORTCLASS TextViewWindow :
	public WindowBase,
	public DefaultWindowHandler
{
public:
	enum Scroll {
		SCROLL_TOP				= 0x0001,
		SCROLL_BOTTOM			= 0x0002,
		SCROLL_LINEUP			= 0x0003,
		SCROLL_LINEDOWN			= 0x0004,
		SCROLL_PAGEUP			= 0x0005,
		SCROLL_PAGEDOWN			= 0x0006,
		SCROLL_VIRTICALPOS		= 0x0007,
		SCROLL_VIRTICAL_MASK	= 0x000f,
		SCROLL_LEFT				= 0x0100,
		SCROLL_RIGHT			= 0x0200,
		SCROLL_CHARLEFT			= 0x0300,
		SCROLL_CHARRIGHT		= 0x0400,
		SCROLL_PAGELEFT			= 0x0500,
		SCROLL_PAGERIGHT		= 0x0600,
		SCROLL_HORIZONTALPOS	= 0x0700,
		SCROLL_HORIZONTAL_MASK	= 0x0f00
	};
	
	enum MoveCaret {
		MOVECARET_CHARLEFT,
		MOVECARET_CHARRIGHT,
		MOVECARET_LINESTART,
		MOVECARET_LINEEND,
		MOVECARET_LINEUP,
		MOVECARET_LINEDOWN,
		MOVECARET_PAGEUP,
		MOVECARET_PAGEDOWN,
		MOVECARET_DOCSTART,
		MOVECARET_DOCEND,
		MOVECARET_POS
	};
	
	enum Select {
		SELECT_SELECT,
		SELECT_CLEAR,
		SELECT_NONE
	};
	
	struct Option
	{
		enum Mask {
			MASK_FONT			= 0x0001,
			MASK_CHARINLINE		= 0x0002,
			MASK_LINESPACING	= 0x0004,
			MASK_NEWLINE		= 0x0008,
			MASK_TAB			= 0x0010,
			MASK_MARGIN			= 0x0020,
			MASK_COLOR			= 0x0040,
			MASK_SCROLLBAR		= 0x0080,
			MASK_CARET			= 0x0100
		};
		unsigned int nMask_;
		LOGFONT logfont_;
		int nCharInLine_;
		int nLineSpacing_;
		bool bShowNewLine_;
		int nTabWidth_;
		WCHAR cTabChar_;
		int nMarginLeft_;
		int nMarginTop_;
		int nMarginRight_;
		int nMarginBottom_;
		COLORREF crForeground_;
		COLORREF crBackground_;
		bool bShowVerticalScrollBar_;
		bool bShowHorizontalScrollBar_;
		bool bShowCaret_;
	};

public:
	TextViewWindow(bool bDeleteThis, QSTATUS* pstatus);
	virtual ~TextViewWindow();

public:
	QSTATUS getOption(Option* pOption);
	QSTATUS setOption(const Option& option);
	
	QSTATUS getText(const WCHAR** ppwszText, size_t* pnLen) const;
	QSTATUS setText(const WCHAR* pwszText, size_t nLen);
	QSTATUS appendText(const WCHAR* pwszText, size_t nLen);
	QSTATUS clearText();
	QSTATUS loadText(Reader* pReader, bool bAsync);
	QSTATUS cancelLoad();
	
	QSTATUS getSelectedText(const WCHAR** ppwszText, size_t* pnLen) const;
	QSTATUS select(const WCHAR* pwszBegin, size_t nLen);
	
	QSTATUS getScrollPos(int* pnPos);
	QSTATUS scroll(Scroll scroll, int nPos, bool bRepeat);
	
	QSTATUS moveCaret(MoveCaret moveCaret,
		const WCHAR* pText, bool bRepeat, Select select);

public:
	virtual QSTATUS getWindowClass(WNDCLASS* pwc);
	virtual QSTATUS preCreateWindow(CREATESTRUCT* pCreateStruct);
	virtual LRESULT windowProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

protected:
	LRESULT onCreate(CREATESTRUCT* pCreateStruct);
	LRESULT onEraseBkgnd(HDC hdc);
	LRESULT onHScroll(UINT nCode, UINT nPos, HWND hwnd);
	LRESULT onKeyDown(UINT nKey, UINT nRepeat, UINT nFlags);
	LRESULT onKillFocus(HWND hwnd);
	LRESULT onLButtonDown(UINT nFlags, const POINT& pt);
	LRESULT onLButtonUp(UINT nFlags, const POINT& pt);
	LRESULT onMouseMove(UINT nFlags, const POINT& pt);
#if !defined _WIN32_WCE || _WIN32_WCE >= 211
	LRESULT onMouseWheel(UINT nFlags, short nDelta, const POINT& pt);
#endif
	LRESULT onPaint();
	LRESULT onSetFocus(HWND hwnd);
	LRESULT onSize(UINT nFlags, int cx, int cy);
	LRESULT onTimer(UINT nId);
	LRESULT onVScroll(UINT nCode, UINT nPos, HWND hwnd);

private:
	TextViewWindow(const TextViewWindow&);
	TextViewWindow& operator=(const TextViewWindow&);

private:
	struct TextViewWindowImpl* pImpl_;
};

}

#endif // __QSTEXTVIEWWINDOW_H__

#endif // 0
