/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QSTEXTWINDOW_H__
#define __QSTEXTWINDOW_H__

#include <qs.h>
#include <qswindow.h>


namespace qs {

class TextModel;
	class AbstractTextModel;
		class EditableTextModel;
		class ReadOnlyTextModel;
class TextModelHandler;
class TextModelEvent;
class TextWindow;
class TextWindowLinkHandler;

class Profile;
class Reader;


/****************************************************************************
 *
 * TextModel
 *
 */

class QSEXPORTCLASS TextModel
{
public:
	class Line
	{
	public:
		Line(const WCHAR* pwszText,
			 size_t nLen);
		~Line();
	
	public:
		const WCHAR* getText() const;
		size_t getLength() const;
	
	private:
		const WCHAR* pwszText_;
		size_t nLen_;
	};

public:
	virtual ~TextModel();

public:
	virtual size_t getLineCount() const = 0;
	virtual Line getLine(size_t nLine) const = 0;
	virtual bool isEditable() const = 0;
	virtual void update(unsigned int nStartLine,
						unsigned int nStartChar,
						unsigned int nEndLine,
						unsigned int nEndChar,
						const WCHAR* pwsz,
						size_t nLen,
						unsigned int* pnLine,
						unsigned int* pnChar) = 0;
	virtual void addTextModelHandler(TextModelHandler* pHandler) = 0;
	virtual void removeTextModelHandler(TextModelHandler* pHandler) = 0;
};


/****************************************************************************
 *
 * AbstractTextModel
 *
 */

class QSEXPORTCLASS AbstractTextModel : public TextModel
{
public:
	AbstractTextModel();
	virtual ~AbstractTextModel();

public:
	virtual void addTextModelHandler(TextModelHandler* pHandler);
	virtual void removeTextModelHandler(TextModelHandler* pHandler);

public:
	void fireTextUpdated(unsigned int nStartLine,
						 unsigned int nOldEndLine,
						 unsigned int nNewEndLine);
	void fireTextSet();

private:
	AbstractTextModel(const AbstractTextModel&);
	AbstractTextModel& operator=(const AbstractTextModel&);

private:
	struct AbstractTextModelImpl* pImpl_;
};


/****************************************************************************
 *
 * EditableTextModel
 *
 */

class QSEXPORTCLASS EditableTextModel : public AbstractTextModel
{
public:
	EditableTextModel();
	virtual ~EditableTextModel();

public:
	wxstring_ptr getText() const;
	bool setText(const WCHAR* pwszText,
				 size_t nLen);

public:
	virtual size_t getLineCount() const;
	virtual Line getLine(size_t nLine) const;
	virtual bool isEditable() const;
	virtual void update(unsigned int nStartLine,
						unsigned int nStartChar,
						unsigned int nEndLine,
						unsigned int nEndChar,
						const WCHAR* pwsz,
						size_t nLen,
						unsigned int* pnLine,
						unsigned int* pnChar);

private:
	EditableTextModel(const EditableTextModel&);
	EditableTextModel& operator=(const EditableTextModel&);

private:
	struct EditableTextModelImpl* pImpl_;
};


/****************************************************************************
 *
 * ReadOnlyTextModel
 *
 */

class QSEXPORTCLASS ReadOnlyTextModel : public AbstractTextModel
{
public:
	ReadOnlyTextModel();
	virtual ~ReadOnlyTextModel();

public:
	std::pair<const WCHAR*, size_t> getText() const;
	bool setText(const WCHAR* pwszText,
				 size_t nLen);
	bool loadText(std::auto_ptr<Reader> pReader,
				  bool bAsync);
	void cancelLoad();

public:
	virtual size_t getLineCount() const;
	virtual Line getLine(size_t nLine) const;
	virtual bool isEditable() const;
	virtual void update(unsigned int nStartLine,
						unsigned int nStartChar,
						unsigned int nEndLine,
						unsigned int nEndChar,
						const WCHAR* pwsz,
						size_t nLen,
						unsigned int* pnLine,
						unsigned int* pnChar);

private:
	ReadOnlyTextModel(const ReadOnlyTextModel&);
	ReadOnlyTextModel& operator=(const ReadOnlyTextModel&);

private:
	class ReadOnlyTextModelImpl* pImpl_;
};


/****************************************************************************
 *
 * TextModelHandler
 *
 */

class QSEXPORTCLASS TextModelHandler
{
public:
	virtual ~TextModelHandler();

public:
	virtual void textUpdated(const TextModelEvent& event) = 0;
	virtual void textSet(const TextModelEvent& event) = 0;
};


/****************************************************************************
 *
 * TextModelEvent
 *
 */

class QSEXPORTCLASS TextModelEvent
{
public:
	TextModelEvent(TextModel* pTextModel,
				   unsigned int nStartLine,
				   unsigned int nOldEndLine,
				   unsigned int nNewEndLine);
	~TextModelEvent();

public:
	TextModel* getTextModel() const;
	unsigned int getStartLine() const;
	unsigned int getOldEndLine() const;
	unsigned int getNewEndLine() const;

private:
	TextModelEvent(const TextModelEvent&);
	TextModelEvent& operator=(const TextModelEvent&);

private:
	TextModel* pTextModel_;
	unsigned int nStartLine_;
	unsigned int nOldEndLine_;
	unsigned int nNewEndLine_;
};


/****************************************************************************
 *
 * TextWindow
 *
 */

class QSEXPORTCLASS TextWindow :
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
		SCROLL_VERTICALPOS		= 0x0007,
		SCROLL_VERTICAL_MASK	= 0x000f,
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
		MOVECARET_WORDLEFT,
		MOVECARET_WORDRIGHT,
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
	
	enum Find {
		FIND_MATCHCASE		= 0x01,
		FIND_REGEX			= 0x02,
		FIND_PREVIOUS		= 0x04,
		FIND_REFORMED		= 0x08,
	};
	
	enum DeleteTextFlag {
		DELETETEXTFLAG_DELETECHAR,
		DELETETEXTFLAG_DELETEBACKWARDCHAR,
		DELETETEXTFLAG_DELETEWORD,
		DELETETEXTFLAG_DELETEBACKWARDWORD
	};

public:
	TextWindow(TextModel* pTextModel,
			   Profile* pProfile,
			   const WCHAR* pwszSection,
			   bool bDeleteThis);
	virtual ~TextWindow();

public:
	TextModel* getTextModel() const;
	void setTextModel(TextModel* pTextModel);
	bool insertText(const WCHAR* pwszText,
					size_t nLen);
	bool deleteText(DeleteTextFlag flag);
	bool isSelected() const;
	wstring_ptr getSelectedText() const;
	bool selectAll();
	bool canSelectAll() const;
	bool cut();
	bool canCut() const;
	bool copy();
	bool canCopy() const;
	bool paste();
	bool canPaste() const;
	bool undo();
	bool canUndo() const;
	bool redo();
	bool canRedo() const;
	bool find(const WCHAR* pwszFind,
			  unsigned int nFlags);
	bool replace(const WCHAR* pwszFind,
				 const WCHAR* pwszReplace,
				 unsigned int nFlags);
	void reform();
	void scroll(Scroll scroll,
				int nPos,
				bool bRepeat);
	void moveCaret(MoveCaret moveCaret,
				   unsigned int nLine,
				   unsigned int nChar,
				   bool bRepeat,
				   Select select,
				   bool bScroll);
	void deselectAll();
	bool openLink();
	TextWindowLinkHandler* getLinkHandler() const;
	void setLinkHandler(TextWindowLinkHandler* pLinkHandler);
	
	COLORREF getForegroundColor() const;
	void setForegroundColor(COLORREF cr);
	COLORREF getBackgroundColor() const;
	void setBackgroundColor(COLORREF cr);
	unsigned int getLineSpacing() const;
	void setLineSpacing(unsigned int nLineSpacing);
	unsigned int getCharInLine() const;
	void setCharInLine(unsigned int nCharInLine);
	unsigned int getTabWidth() const;
	void setTabWidth(unsigned int nTabWidth);
	void getMargin(unsigned int* pnTop,
				   unsigned int* pnBottom,
				   unsigned int* pnLeft,
				   unsigned int* pnRight) const;
	void setMargin(unsigned int nTop,
				   unsigned int nBottom,
				   unsigned int nLeft,
				   unsigned int nRight);
	bool isShowNewLine() const;
	void setShowNewLine(bool bShowNewLine);
	bool isShowTab() const;
	void setShowTab(bool bShowTab);
	bool isShowScrollBar(bool bHorizontal) const;
	void setShowScrollBar(bool bHorizontal,
						  bool bShowScrollBar);
	bool isShowCaret() const;
	void setShowCaret(bool bShowCaret);
	const WCHAR* getQuote(unsigned int n) const;
	void setQuote(unsigned int n,
				  const WCHAR* pwszQuote);
	COLORREF getQuoteColor(unsigned int n) const;
	void setQuoteColor(unsigned int n,
					   COLORREF cr);
	unsigned int getReformLineLength() const;
	void setReformLineLength(unsigned int nReformLineLength);
	const WCHAR* getReformQuote() const;
	void setReformQuote(const WCHAR* pwszReformQuote);

public:
	virtual void getWindowClass(WNDCLASS* pwc);
	virtual bool preCreateWindow(CREATESTRUCT* pCreateStruct);
	virtual LRESULT windowProc(UINT uMsg,
							   WPARAM wParam,
							   LPARAM lParam);

protected:
	LRESULT onChar(UINT nChar,
				   UINT nRepeat,
				   UINT nFlags);
	LRESULT onCopy();
	LRESULT onCreate(CREATESTRUCT* pCreateStruct);
	LRESULT onCut();
	LRESULT onDestroy();
	LRESULT onEraseBkgnd(HDC hdc);
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
	LRESULT onKeyDown(UINT nKey,
					  UINT nRepeat,
					  UINT nFlags);
	LRESULT onKillFocus(HWND hwnd);
	LRESULT onLButtonDblClk(UINT nFlags,
							const POINT& pt);
	LRESULT onLButtonDown(UINT nFlags,
						  const POINT& pt);
	LRESULT onLButtonUp(UINT nFlags,
						const POINT& pt);
	LRESULT onMouseMove(UINT nFlags,
						const POINT& pt);
#if !defined _WIN32_WCE || _WIN32_WCE >= 211
	LRESULT onMouseWheel(UINT nFlags,
						 short nDelta,
						 const POINT& pt);
#endif
#ifndef _WIN32_WCE
	LRESULT onNcPaint(HRGN hrgn);
#endif
	LRESULT onPaint();
	LRESULT onPaste();
	LRESULT onSetCursor(HWND hwnd,
						UINT nHitTest,
						UINT nMessage);
	LRESULT onSetFocus(HWND hwnd);
	LRESULT onSize(UINT nFlags,
				   int cx,
				   int cy);
#ifndef _WIN32_WCE
	LRESULT onThemeChanged();
#endif
	LRESULT onTimer(UINT nId);
	LRESULT onVScroll(UINT nCode,
					  UINT nPos,
					  HWND hwnd);

private:
	TextWindow(const TextWindow&);
	TextWindow& operator=(const TextWindow&);

private:
	class TextWindowImpl* pImpl_;
};


/****************************************************************************
 *
 * TextWindowLinkHandler
 *
 */

class QSEXPORTCLASS TextWindowLinkHandler
{
public:
	virtual ~TextWindowLinkHandler();

public:
	virtual bool openLink(const WCHAR* pwszURL) = 0;
};

}

#endif // __QSTEXTWINDOW_H__
