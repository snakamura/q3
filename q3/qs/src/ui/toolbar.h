/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
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
	Toolbar(const WCHAR* pwszName, QSTATUS* pstatus);
	~Toolbar();

public:
	const WCHAR* getName() const;
	QSTATUS create(HWND hwnd, HIMAGELIST hImageList) const;

public:
	QSTATUS add(ToolbarItem* pItem);

private:
	Toolbar(const Toolbar&);
	Toolbar& operator=(const Toolbar&);

private:
	typedef std::vector<ToolbarItem*> ItemList;

private:
	WSTRING wstrName_;
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
	explicit ToolbarItem(QSTATUS* pstatus);

public:
	virtual ~ToolbarItem();

public:
	virtual QSTATUS create(HWND hwnd) = 0;

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
	ToolbarButton(int nImage, const WCHAR* pwszText, const WCHAR* pwszToolTip,
		UINT nAction, const WCHAR* pwszDropDown, QSTATUS* pstatus);
	virtual ~ToolbarButton();

public:
	virtual QSTATUS create(HWND hwnd);

private:
	ToolbarButton(const ToolbarButton&);
	ToolbarButton& operator=(const ToolbarButton&);

private:
	int nImage_;
	WSTRING wstrText_;
	WSTRING wstrToolTip_;
	UINT nAction_;
	WSTRING wstrDropDown_;
};


/****************************************************************************
 *
 * ToolbarSeparator
 *
 */

class ToolbarSeparator : public ToolbarItem
{
public:
	explicit ToolbarSeparator(QSTATUS* pstatus);
	virtual ~ToolbarSeparator();

public:
	virtual QSTATUS create(HWND hwnd);

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
		const ActionItem* pItem, size_t nItemCount, QSTATUS* pstatus);
	virtual ~ToolbarContentHandler();

public:
	virtual QSTATUS startElement(const WCHAR* pwszNamespaceURI,
		const WCHAR* pwszLocalName, const WCHAR* pwszQName,
		const Attributes& attributes);
	virtual QSTATUS endElement(const WCHAR* pwszNamespaceURI,
		const WCHAR* pwszLocalName, const WCHAR* pwszQName);
	virtual QSTATUS characters(const WCHAR* pwsz,
		size_t nStart, size_t nLength);

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
