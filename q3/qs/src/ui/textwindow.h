/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __TEXTWINDOW_H__
#define __TEXTWINDOW_H__

#include <qs.h>

#include <vector>


namespace qs {

/****************************************************************************
 *
 * TextWindowUndoManager
 *
 */

class TextWindowUndoManager
{
public:
	class Item
	{
	public:
		Item(unsigned int nStartLine,
			 unsigned int nStartChar,
			 unsigned int nEndLine,
			 unsigned int nEndChar,
			 unsigned int nCharLine,
			 unsigned int nCaretPos,
			 wstring_ptr wstrText);
		~Item();
	
	public:
		unsigned int getStartLine() const;
		unsigned int getStartChar() const;
		unsigned int getEndLine() const;
		unsigned int getEndChar() const;
		unsigned int getCaretLine() const;
		unsigned int getCaretChar() const;
		const WCHAR* getText() const;
	
	private:
		Item(const Item&);
		Item& operator=(const Item&);
	
	private:
		unsigned int nStartLine_;
		unsigned int nStartChar_;
		unsigned int nEndLine_;
		unsigned int nEndChar_;
		unsigned int nCaretLine_;
		unsigned int nCaretChar_;
		wstring_ptr wstrText_;
	};

public:
	TextWindowUndoManager();
	~TextWindowUndoManager();

public:
	void pushUndoItem(unsigned int nStartLine,
					  unsigned int nStartChar,
					  unsigned int nEndLine,
					  unsigned int nEndChar,
					  unsigned int nCaretLine,
					  unsigned int nCaretChar,
					  wstring_ptr wstrText,
					  bool bClearRedo);
	Item* popUndoItem();
	bool hasUndoItem() const;
	void pushRedoItem(unsigned int nStartLine,
					  unsigned int nStartChar,
					  unsigned int nEndLine,
					  unsigned int nEndChar,
					  unsigned int nCaretLine,
					  unsigned int nCaretChar,
					  wstring_ptr wstrText);
	Item* popRedoItem();
	bool hasRedoItem() const;
	void clearRedoItems();
	void clear();

private:
	TextWindowUndoManager(const TextWindowUndoManager&);
	TextWindowUndoManager& operator=(const TextWindowUndoManager&);

private:
	typedef std::vector<Item*> ItemList;

private:
	ItemList listUndo_;
	ItemList listRedo_;
};


/****************************************************************************
 *
 * TextWindowRuler
 *
 */

class TextWindowRuler :
	public WindowBase,
	public DefaultWindowHandler
{
public:
	TextWindowRuler(TextWindowImpl* pImpl);
	virtual ~TextWindowRuler();

public:
	void update();

public:
	virtual LRESULT windowProc(UINT uMsg,
							   WPARAM wParam,
							   LPARAM lParam);

protected:
	LRESULT onEraseBkgnd(HDC hdc);
	LRESULT onPaint();

private:
	TextWindowRuler(const TextWindowRuler&);
	TextWindowRuler& operator=(const TextWindowRuler&);

private:
	TextWindowImpl* pImpl_;
	int nPos_;
};

}

#endif // __TEXTWINDOW_H__
