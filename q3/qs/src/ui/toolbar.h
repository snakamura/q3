/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __TOOLBAR_H__
#define __TOOLBAR_H__

#include <qssax.h>
#include <qsstring.h>
#include <qstoolbar.h>

#include <vector>


namespace qs {

class Toolbar;
class ToolbarItem;
	class ToolbarButton;
	class ToolbarSeparator;
class ToolbarContentHandler;


/****************************************************************************
 *
 * Toolbar
 *
 */

class Toolbar
{
public:
	Toolbar(const WCHAR* pwszName,
			bool bShowText);
	~Toolbar();

public:
	const WCHAR* getName() const;
	bool create(HWND hwnd,
				HIMAGELIST hImageList) const;

public:
	void add(std::auto_ptr<ToolbarItem> pItem);

private:
	Toolbar(const Toolbar&);
	Toolbar& operator=(const Toolbar&);

private:
	typedef std::vector<ToolbarItem*> ItemList;

private:
	wstring_ptr wstrName_;
	bool bShowText_;
	ItemList listItem_;
};


/****************************************************************************
 *
 * ToolbarItem
 *
 */

class ToolbarItem
{
protected:
	ToolbarItem();

public:
	virtual ~ToolbarItem();

public:
	virtual bool create(HWND hwnd,
						bool bShowText) = 0;

private:
	ToolbarItem(const ToolbarItem&);
	ToolbarItem& operator=(const ToolbarItem&);
};


/****************************************************************************
 *
 * ToolbarButton
 *
 */

class ToolbarButton : public ToolbarItem
{
public:
	ToolbarButton(int nImage,
				  const WCHAR* pwszText,
				  const WCHAR* pwszToolTip,
				  UINT nAction,
				  const WCHAR* pwszDropDown);
	virtual ~ToolbarButton();

public:
	virtual bool create(HWND hwnd,
						bool bShowText);

private:
	ToolbarButton(const ToolbarButton&);
	ToolbarButton& operator=(const ToolbarButton&);

private:
	int nImage_;
	wstring_ptr wstrText_;
	wstring_ptr wstrToolTip_;
	UINT nAction_;
	wstring_ptr wstrDropDown_;
};


/****************************************************************************
 *
 * ToolbarSeparator
 *
 */

class ToolbarSeparator : public ToolbarItem
{
public:
	ToolbarSeparator();
	virtual ~ToolbarSeparator();

public:
	virtual bool create(HWND hwnd,
						bool bShowText);

private:
	ToolbarSeparator(const ToolbarSeparator&);
	ToolbarSeparator& operator=(const ToolbarSeparator&);
};


/****************************************************************************
 *
 * ToolbarContentHandler
 *
 */

class ToolbarContentHandler : public DefaultHandler
{
public:
	typedef std::vector<Toolbar*> ToolbarList;

public:
	ToolbarContentHandler(ToolbarList* pListToolbar,
						  const ActionItem* pItem,
						  size_t nItemCount);
	virtual ~ToolbarContentHandler();

public:
	virtual bool startElement(const WCHAR* pwszNamespaceURI,
							  const WCHAR* pwszLocalName,
							  const WCHAR* pwszQName,
							  const Attributes& attributes);
	virtual bool endElement(const WCHAR* pwszNamespaceURI,
							const WCHAR* pwszLocalName,
							const WCHAR* pwszQName);
	virtual bool characters(const WCHAR* pwsz,
							size_t nStart,
							size_t nLength);

private:
	UINT getActionId(const WCHAR* pwszAction);

private:
	ToolbarContentHandler(const ToolbarContentHandler&);
	ToolbarContentHandler& operator=(const ToolbarContentHandler&);

private:
	enum State {
		STATE_ROOT,
		STATE_TOOLBARS,
		STATE_TOOLBAR,
		STATE_BUTTON,
		STATE_SEPARATOR
	};

private:
	ToolbarList* pListToolbar_;
	const ActionItem* pActionItem_;
	size_t nActionItemCount_;
	State state_;
	Toolbar* pToolbar_;
};

}

#endif // __TOOLBAR_H__
