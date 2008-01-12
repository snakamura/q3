/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#ifndef __TEXTMODEL_H__
#define __TEXTMODEL_H__

#include <qs.h>

#include <vector>


namespace qs {

/****************************************************************************
 *
 * TextModelUndoManager
 *
 */

class TextModelUndoManager
{
public:
	class Item
	{
	public:
		Item(size_t nStartLine,
			 size_t nStartChar,
			 size_t nEndLine,
			 size_t nEndChar,
			 size_t nCharLine,
			 size_t nCaretPos,
			 wstring_ptr wstrText);
		~Item();
	
	public:
		size_t getStartLine() const;
		size_t getStartChar() const;
		size_t getEndLine() const;
		size_t getEndChar() const;
		size_t getCaretLine() const;
		size_t getCaretChar() const;
		const WCHAR* getText() const;
	
	private:
		Item(const Item&);
		Item& operator=(const Item&);
	
	private:
		size_t nStartLine_;
		size_t nStartChar_;
		size_t nEndLine_;
		size_t nEndChar_;
		size_t nCaretLine_;
		size_t nCaretChar_;
		wstring_ptr wstrText_;
	};

public:
	TextModelUndoManager();
	~TextModelUndoManager();

public:
	void pushUndoItem(size_t nStartLine,
					  size_t nStartChar,
					  size_t nEndLine,
					  size_t nEndChar,
					  size_t nCaretLine,
					  size_t nCaretChar,
					  wstring_ptr wstrText,
					  bool bClearRedo);
	Item* popUndoItem();
	bool hasUndoItem() const;
	void pushRedoItem(size_t nStartLine,
					  size_t nStartChar,
					  size_t nEndLine,
					  size_t nEndChar,
					  size_t nCaretLine,
					  size_t nCaretChar,
					  wstring_ptr wstrText);
	Item* popRedoItem();
	bool hasRedoItem() const;
	void clearRedoItems();
	void clear();

private:
	TextModelUndoManager(const TextModelUndoManager&);
	TextModelUndoManager& operator=(const TextModelUndoManager&);

private:
	typedef std::vector<Item*> ItemList;

private:
	ItemList listUndo_;
	ItemList listRedo_;
};

}

#endif // __TEXTMODEL_H__
