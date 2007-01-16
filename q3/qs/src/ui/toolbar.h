/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __TOOLBAR_H__
#define __TOOLBAR_H__

#include <qssax.h>
#include <qsstring.h>
#include <qstoolbar.h>
#include <qswindow.h>

#include <vector>


namespace qs {

class Toolbar;
class ToolbarItem;
	class ToolbarButton;
	class ToolbarSeparator;
class ToolbarContentHandler;
class ToolbarNotifyHandler;

class MenuManager;


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
	ToolbarCookie* create(HWND hwnd,
						  WindowBase* pParent,
						  const MenuManager* pMenuManager,
						  HIMAGELIST hImageList) const;
	void destroy(ToolbarCookie* pCookie) const;

public:
	void add(std::auto_ptr<ToolbarItem> pItem);
	const ToolbarItem* getItem(UINT nAction) const;

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
	virtual UINT getAction() const = 0;
	virtual const WCHAR* getDropDown() const = 0;
	virtual const WCHAR* getToolTip() const = 0;
	virtual bool isSeparator() const = 0;

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
	virtual UINT getAction() const;
	virtual const WCHAR* getDropDown() const;
	virtual const WCHAR* getToolTip() const;
	virtual bool isSeparator() const;

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
	virtual UINT getAction() const;
	virtual const WCHAR* getDropDown() const;
	virtual const WCHAR* getToolTip() const;
	virtual bool isSeparator() const;

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
						  size_t nItemCount,
						  ActionParamMap* pActionParamMap);
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
	const ActionItem* getActionItem(const WCHAR* pwszAction) const;

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
	ActionParamMap* pActionParamMap_;
	State state_;
	Toolbar* pToolbar_;
	UINT nDummyId_;
};


/****************************************************************************
 *
 * ToolbarNotifyHandler
 *
 */

class ToolbarNotifyHandler : public NotifyHandler
{
public:
	ToolbarNotifyHandler(const Toolbar* pToolbar,
						 const MenuManager* pMenuManager,
						 HWND hwndFrame);
	virtual ~ToolbarNotifyHandler();

public:
	virtual LRESULT onNotify(NMHDR* pnmhdr,
							 bool* pbHandled);

private:
	LRESULT onDropDown(NMHDR* pnmhdr,
					   bool* pbHandled);
#ifndef _WIN32_WCE
	LRESULT onGetDispInfo(NMHDR* pnmhdr,
						  bool* pbHandled);
#endif

private:
	ToolbarNotifyHandler(const ToolbarNotifyHandler&);
	ToolbarNotifyHandler& operator=(const ToolbarNotifyHandler&);

private:
	const Toolbar* pToolbar_;
	const MenuManager* pMenuManager_;
	HWND hwndFrame_;
};

}

#endif // __TOOLBAR_H__
