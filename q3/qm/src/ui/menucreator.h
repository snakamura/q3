/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __MENUCREATOR_H__
#define __MENUCREATOR_H__

#include <qm.h>
#include <qmaccount.h>
#include <qmfolder.h>
#include <qmmessageholder.h>

#include <qs.h>
#include <qsaction.h>
#include <qsmenu.h>
#include <qsmime.h>
#include <qsstring.h>

#include <vector>
#include <utility>

#include "resourceinc.h"


namespace qm {

class AttachmentMenuCreator;
class EncodingMenuCreator;
class FilterMenuCreator;
class GoRoundMenuCreator;
class InsertTextMenuCreator;
class MoveMenuCreator;
class RecentsMenuCreator;
class ScriptMenuCreator;
class SortMenuCreator;
class SubAccountMenuCreator;
class TemplateMenuCreator;
	class CreateTemplateMenuCreator;
	class ViewTemplateMenuCreator;
class ActionParamHelper;
class MenuCreatorUtil;

class AccountManager;
class Filter;
class FilterManager;
class FixedFormText;
class FixedFormTextManager;
class FolderModel;
class FolderModelBase;
class GoRound;
class GoRoundCourse;
class Message;
class MessageHolder;
class MessageSelectionModel;
class Recents;
class ScriptManager;
class SecurityModel;
class TemplateManager;
class URI;
class ViewModelManager;


enum {
	DMI_MESSAGE_ATTACHMENT	= 1,
	DMI_MESSAGE_CREATE,
	DMI_MESSAGE_CREATEEXTERNAL,
	DMI_MESSAGE_MOVE,
	DMI_MESSAGE_RECENTS,
	DMI_TOOL_ENCODING,
	DMI_TOOL_GOROUND,
	DMI_TOOL_INSERTTEXT,
	DMI_TOOL_SCRIPT,
	DMI_TOOL_SUBACCOUNT,
	DMI_VIEW_ENCODING,
	DMI_VIEW_FILTER,
	DMI_VIEW_SORT,
	DMI_VIEW_TEMPLATE
};

const qs::DynamicMenuItem dynamicMenuItems[] = {
	{ L"MessageAttachment",		DMI_MESSAGE_ATTACHMENT,		DMI_MESSAGE_ATTACHMENT		},
	{ L"MessageCreate",			DMI_MESSAGE_CREATE,			DMI_MESSAGE_CREATE			},
	{ L"MessageCreateExternal",	DMI_MESSAGE_CREATEEXTERNAL,	DMI_MESSAGE_CREATEEXTERNAL	},
	{ L"MessageMove",			DMI_MESSAGE_MOVE,			DMI_MESSAGE_MOVE			},
	{ L"MessageRecents",		DMI_MESSAGE_RECENTS,		DMI_MESSAGE_RECENTS			},
	{ L"ToolEncoding",			DMI_TOOL_ENCODING,			DMI_TOOL_ENCODING			},
	{ L"ToolGoRound",			DMI_TOOL_GOROUND,			DMI_TOOL_GOROUND			},
	{ L"ToolInsertText",		DMI_TOOL_INSERTTEXT,		DMI_TOOL_INSERTTEXT			},
	{ L"ToolScript",			DMI_TOOL_SCRIPT,			DMI_TOOL_SCRIPT				},
	{ L"ToolSubAccount",		DMI_TOOL_SUBACCOUNT,		DMI_TOOL_SUBACCOUNT			},
	{ L"ViewEncoding",			DMI_VIEW_ENCODING,			DMI_VIEW_ENCODING			},
	{ L"ViewFilter",			DMI_VIEW_FILTER,			DMI_VIEW_FILTER				},
	{ L"ViewSort",				DMI_VIEW_SORT,				DMI_VIEW_SORT				},
	{ L"ViewTemplate",			DMI_VIEW_TEMPLATE,			DMI_VIEW_TEMPLATE			},
};


/****************************************************************************
 *
 * ActionParamHelper
 *
 */

class ActionParamHelper
{
public:
	explicit ActionParamHelper(qs::ActionParamMap* pActionParamMap);
	~ActionParamHelper();

public:
	unsigned int add(unsigned int nMaxParamCount,
					 std::auto_ptr<qs::ActionParam> pParam);
	void clear();

private:
	ActionParamHelper(const ActionParamHelper&);
	ActionParamHelper& operator=(const ActionParamHelper&);

private:
	typedef std::vector<unsigned int> IdList;

private:
	qs::ActionParamMap* pActionParamMap_;
	IdList listId_;
};


/****************************************************************************
 *
 * AttachmentMenuCreator
 *
 */

class AttachmentMenuCreator : public qs::DynamicMenuCreator
{
public:
	enum {
		DATA = DMI_MESSAGE_ATTACHMENT
	};

public:
	AttachmentMenuCreator(MessageSelectionModel* pMessageSelectionModel,
						  SecurityModel* pSecurityModel,
						  qs::ActionParamMap* pActionParamMap);
	~AttachmentMenuCreator();

public:
	virtual UINT createMenu(HMENU hmenu,
							UINT nIndex);
	virtual DWORD getMenuItemData() const;

private:
	AttachmentMenuCreator(const AttachmentMenuCreator&);
	AttachmentMenuCreator& operator=(const AttachmentMenuCreator&);

private:
	MessageSelectionModel* pMessageSelectionModel_;
	SecurityModel* pSecurityModel_;
	ActionParamHelper helper_;
};


/****************************************************************************
 *
 * EncodingMenuCreator
 *
 */

class EncodingMenuCreator : public qs::DynamicMenuCreator
{
public:
	enum {
		DATA_VIEW	= DMI_VIEW_ENCODING,
		DATA_TOOL	= DMI_TOOL_ENCODING
	};

public:
	EncodingMenuCreator(qs::Profile* pProfile,
						bool bView,
						qs::ActionParamMap* pActionParamMap);
	~EncodingMenuCreator();

public:
	virtual UINT createMenu(HMENU hmenu,
							UINT nIndex);
	virtual DWORD getMenuItemData() const;

private:
	EncodingMenuCreator(const EncodingMenuCreator&);
	EncodingMenuCreator& operator=(const EncodingMenuCreator&);

private:
	qs::Profile* pProfile_;
	bool bView_;
	ActionParamHelper helper_;
};


/****************************************************************************
 *
 * FilterMenuCreator
 *
 */

class FilterMenuCreator : public qs::DynamicMenuCreator
{
public:
	enum {
		DATA = DMI_VIEW_FILTER
	};

public:
	FilterMenuCreator(FilterManager* pFilterManager,
					  qs::ActionParamMap* pActionParamMap);
	~FilterMenuCreator();

public:
	virtual UINT createMenu(HMENU hmenu,
							UINT nIndex);
	virtual DWORD getMenuItemData() const;

private:
	FilterMenuCreator(const FilterMenuCreator&);
	FilterMenuCreator& operator=(const FilterMenuCreator&);

private:
	FilterManager* pFilterManager_;
	ActionParamHelper helper_;
};


/****************************************************************************
 *
 * GoRoundMenuCreator
 *
 */

class GoRoundMenuCreator : public qs::DynamicMenuCreator
{
public:
	enum {
		DATA = DMI_TOOL_GOROUND
	};

public:
	GoRoundMenuCreator(GoRound* pGoRound,
					   qs::ActionParamMap* pActionParamMap);
	~GoRoundMenuCreator();

public:
	virtual UINT createMenu(HMENU hmenu,
							UINT nIndex);
	virtual DWORD getMenuItemData() const;

private:
	GoRoundMenuCreator(const GoRoundMenuCreator&);
	GoRoundMenuCreator& operator=(const GoRoundMenuCreator&);

private:
	GoRound* pGoRound_;
	ActionParamHelper helper_;
};


/****************************************************************************
 *
 * InsertTextMenuCreator
 *
 */

class InsertTextMenuCreator : public qs::DynamicMenuCreator
{
public:
	enum {
		DATA = DMI_TOOL_INSERTTEXT
	};

public:
	InsertTextMenuCreator(FixedFormTextManager* pManager,
						  qs::ActionParamMap* pActionParamMap);
	~InsertTextMenuCreator();

public:
	virtual UINT createMenu(HMENU hmenu,
							UINT nIndex);
	virtual DWORD getMenuItemData() const;

private:
	InsertTextMenuCreator(const InsertTextMenuCreator&);
	InsertTextMenuCreator& operator=(const InsertTextMenuCreator&);

private:
	FixedFormTextManager* pManager_;
	ActionParamHelper helper_;
};


/****************************************************************************
 *
 * MoveMenuCreator
 *
 */

class MoveMenuCreator : public qs::DynamicMenuCreator
{
public:
	enum {
		DATA = DMI_MESSAGE_MOVE
	};

public:
	MoveMenuCreator(FolderModelBase* pFolderModel,
					MessageSelectionModel* pMessageSelectionModel,
					qs::ActionParamMap* pActionParamMap);
	~MoveMenuCreator();

public:
	virtual UINT createMenu(HMENU hmenu,
							UINT nIndex);
	virtual DWORD getMenuItemData() const;

private:
	static bool isMovableFolder(const Folder* pFolder);
	static bool hasSelectableChildNormalFolder(Account::FolderList::const_iterator first,
											   Account::FolderList::const_iterator last);
	static qs::wstring_ptr formatName(const Folder* pFolder,
									  int* pnMnemonic);

private:
	MoveMenuCreator(const MoveMenuCreator&);
	MoveMenuCreator& operator=(const MoveMenuCreator&);

private:
	struct MenuInserter
	{
		MenuInserter(HMENU hmenu,
					 Folder* pFolder);
		
		HMENU hmenu_;
		Folder* pFolder_;
		unsigned int nCount_;
		int nMnemonic_;
	};

private:
	FolderModelBase* pFolderModel_;
	MessageSelectionModel* pMessageSelectionModel_;
	ActionParamHelper helper_;
};


/****************************************************************************
 *
 * RecentsMenuCreator
 *
 */

class RecentsMenuCreator : public qs::DynamicMenuCreator
{
public:
	enum {
		DATA = DMI_MESSAGE_RECENTS
	};

public:
	RecentsMenuCreator(Recents* pRecents,
					   AccountManager* pAccountManager,
					   qs::ActionParamMap* pActionParamMap);
	~RecentsMenuCreator();

public:
	virtual UINT createMenu(HMENU hmenu,
							UINT nIndex);
	virtual DWORD getMenuItemData() const;

private:
	struct URIComp : public std::binary_function<const URI*, const URI*, bool>
	{
		bool operator()(const URI* pLhs,
						const URI* pRhs);
	};

private:
	RecentsMenuCreator(const RecentsMenuCreator&);
	RecentsMenuCreator& operator=(const RecentsMenuCreator&);

private:
	Recents* pRecents_;
	AccountManager* pAccountManager_;
	ActionParamHelper helper_;
};


/****************************************************************************
 *
 * ScriptMenuCreator
 *
 */

class ScriptMenuCreator : public qs::DynamicMenuCreator
{
public:
	enum {
		DATA = DMI_TOOL_SCRIPT
	};

public:
	ScriptMenuCreator(ScriptManager* pScriptManager,
					  qs::ActionParamMap* pActionParamMap);
	~ScriptMenuCreator();

public:
	virtual UINT createMenu(HMENU hmenu,
							UINT nIndex);
	virtual DWORD getMenuItemData() const;

private:
	ScriptMenuCreator(const ScriptMenuCreator&);
	ScriptMenuCreator& operator=(const ScriptMenuCreator&);

private:
	ScriptManager* pScriptManager_;
	ActionParamHelper helper_;
};


/****************************************************************************
 *
 * SortMenu
 *
 */

class SortMenuCreator : public qs::DynamicMenuCreator
{
public:
	enum {
		DATA = DMI_VIEW_SORT
	};

public:
	SortMenuCreator(ViewModelManager* pViewModelManager,
					qs::ActionParamMap* pActionParamMap);
	~SortMenuCreator();

public:
	virtual UINT createMenu(HMENU hmenu,
							UINT nIndex);
	virtual DWORD getMenuItemData() const;

private:
	SortMenuCreator(const SortMenuCreator&);
	SortMenuCreator& operator=(const SortMenuCreator&);

private:
	ViewModelManager* pViewModelManager_;
	ActionParamHelper helper_;
};


/****************************************************************************
 *
 * SubAccountMenuCreator
 *
 */

class SubAccountMenuCreator : public qs::DynamicMenuCreator
{
public:
	enum {
		DATA = DMI_TOOL_SUBACCOUNT
	};

public:
	SubAccountMenuCreator(FolderModel* pFolderModel,
						  qs::ActionParamMap* pActionParamMap);
	~SubAccountMenuCreator();

public:
	virtual UINT createMenu(HMENU hmenu,
							UINT nIndex);
	virtual DWORD getMenuItemData() const;

private:
	SubAccountMenuCreator(const SubAccountMenuCreator&);
	SubAccountMenuCreator& operator=(const SubAccountMenuCreator&);

private:
	FolderModel* pFolderModel_;
	ActionParamHelper helper_;
};


/****************************************************************************
 *
 * TemplateMenuCreator
 *
 */

class TemplateMenuCreator : public qs::DynamicMenuCreator
{
protected:
	TemplateMenuCreator(const TemplateManager* pTemplateManager,
						FolderModelBase* pFolderModel,
						qs::ActionParamMap* pActionParamMap);
	~TemplateMenuCreator();

public:
	virtual UINT createMenu(HMENU hmenu,
							UINT nIndex);

protected:
	virtual const WCHAR* getPrefix() const = 0;
	virtual UINT getBaseId() const = 0;
	virtual UINT getMax() const = 0;

private:
	TemplateMenuCreator(const TemplateMenuCreator&);
	TemplateMenuCreator& operator=(const TemplateMenuCreator&);

private:
	const TemplateManager* pTemplateManager_;
	FolderModelBase* pFolderModel_;
	ActionParamHelper helper_;
};


/****************************************************************************
 *
 * CreateTemplateMenuCreator
 *
 */

class CreateTemplateMenuCreator : public TemplateMenuCreator
{
public:
	enum {
		DATA			= DMI_MESSAGE_CREATE,
		DATA_EXTERNAL	= DMI_MESSAGE_CREATEEXTERNAL
	};

public:
	CreateTemplateMenuCreator(const TemplateManager* pTemplateManager,
							  FolderModelBase* pFolderModel,
							  qs::ActionParamMap* pActionParamMap,
							  bool bExternalEditor);
	~CreateTemplateMenuCreator();

public:
	virtual DWORD getMenuItemData() const;

protected:
	virtual const WCHAR* getPrefix() const;
	virtual UINT getBaseId() const;
	virtual UINT getMax() const;

private:
	CreateTemplateMenuCreator(const CreateTemplateMenuCreator&);
	CreateTemplateMenuCreator& operator=(const CreateTemplateMenuCreator&);

private:
	bool bExternalEditor_;
};


/****************************************************************************
 *
 * ViewTemplateMenuCreator
 *
 */

class ViewTemplateMenuCreator : public TemplateMenuCreator
{
public:
	enum {
		DATA = DMI_VIEW_TEMPLATE
	};

public:
	ViewTemplateMenuCreator(const TemplateManager* pTemplateManager,
							FolderModelBase* pFolderModel,
							qs::ActionParamMap* pActionParamMap);
	~ViewTemplateMenuCreator();

public:
	virtual DWORD getMenuItemData() const;

protected:
	virtual const WCHAR* getPrefix() const;
	virtual UINT getBaseId() const;
	virtual UINT getMax() const;

private:
	ViewTemplateMenuCreator(const ViewTemplateMenuCreator&);
	ViewTemplateMenuCreator& operator=(const ViewTemplateMenuCreator&);
};


/****************************************************************************
 *
 * MenuCreatorUtil
 *
 */

class MenuCreatorUtil
{
public:
	static void insertMenuItem(HMENU hmenu,
							   UINT nIndex,
							   UINT nId,
							   const WCHAR* pwszText,
							   DWORD dwData);
	static void removeMenuItems(HMENU hmenu,
								UINT nIndex,
								DWORD dwData);
	static void setMenuItemData(HMENU hmenu,
								UINT nIndex,
								DWORD dwData);
};

}

#endif // __MENUCREATOR_H__
