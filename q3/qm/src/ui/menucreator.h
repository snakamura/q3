/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
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

class MenuCreator;
	class AddressBookMenuCreator;
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
class MacroMenuCreator;
class MacroDynamicMenuItem;
class MacroDynamicMenuMap;
class ActionParamHelper;
class MenuCreatorUtil;
class MenuCreatorList;
class MenuCreatorListCallback;

class AccountManager;
class AccountSelectionModel;
class AddressBookModel;
class AddressBookSelectionModel;
class Filter;
class FilterManager;
class FixedFormText;
class FixedFormTextManager;
class FolderModel;
class FolderModelBase;
class GoRound;
class GoRoundCourse;
class Macro;
class Message;
class MessageHolder;
class MessageSelectionModel;
class Recents;
class ScriptManager;
class SecurityModel;
class TemplateManager;
class URI;
class ViewModelManager;


/****************************************************************************
 *
 * MenuCreator
 *
 */

class MenuCreator : public qs::DynamicMenuCreator
{
public:
	virtual ~MenuCreator();

public:
	virtual const WCHAR* getName() const = 0;
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
 * AddressBookMenuCreator
 *
 */

class AddressBookMenuCreator : public MenuCreator
{
public:
	AddressBookMenuCreator(AddressBookModel* pModel,
						   AddressBookSelectionModel* pSelectionModel,
						   qs::ActionParamMap* pActionParamMap);
	virtual ~AddressBookMenuCreator();

public:
	virtual UINT createMenu(HMENU hmenu,
							UINT nIndex,
							const qs::DynamicMenuItem* pItem);

public:
	virtual const WCHAR* getName() const;

private:
	AddressBookMenuCreator(const AddressBookMenuCreator&);
	AddressBookMenuCreator& operator=(const AddressBookMenuCreator&);

private:
	AddressBookModel* pModel_;
	AddressBookSelectionModel* pSelectionModel_;
	ActionParamHelper helper_;
};


/****************************************************************************
 *
 * AttachmentMenuCreator
 *
 */

class AttachmentMenuCreator : public MenuCreator
{
public:
	AttachmentMenuCreator(MessageSelectionModel* pMessageSelectionModel,
						  SecurityModel* pSecurityModel,
						  qs::ActionParamMap* pActionParamMap);
	virtual ~AttachmentMenuCreator();

public:
	virtual UINT createMenu(HMENU hmenu,
							UINT nIndex,
							const qs::DynamicMenuItem* pItem);

public:
	virtual const WCHAR* getName() const;

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

class EncodingMenuCreator : public MenuCreator
{
public:
	EncodingMenuCreator(qs::Profile* pProfile,
						bool bView,
						qs::ActionParamMap* pActionParamMap);
	~EncodingMenuCreator();

public:
	virtual UINT createMenu(HMENU hmenu,
							UINT nIndex,
							const qs::DynamicMenuItem* pItem);

public:
	virtual const WCHAR* getName() const;

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

class FilterMenuCreator : public MenuCreator
{
public:
	FilterMenuCreator(FilterManager* pFilterManager,
					  qs::ActionParamMap* pActionParamMap);
	~FilterMenuCreator();

public:
	virtual UINT createMenu(HMENU hmenu,
							UINT nIndex,
							const qs::DynamicMenuItem* pItem);

public:
	virtual const WCHAR* getName() const;

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

class GoRoundMenuCreator : public MenuCreator
{
public:
	GoRoundMenuCreator(GoRound* pGoRound,
					   qs::ActionParamMap* pActionParamMap);
	~GoRoundMenuCreator();

public:
	virtual UINT createMenu(HMENU hmenu,
							UINT nIndex,
							const qs::DynamicMenuItem* pItem);

public:
	virtual const WCHAR* getName() const;

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

class InsertTextMenuCreator : public MenuCreator
{
public:
	InsertTextMenuCreator(FixedFormTextManager* pManager,
						  qs::ActionParamMap* pActionParamMap);
	~InsertTextMenuCreator();

public:
	virtual UINT createMenu(HMENU hmenu,
							UINT nIndex,
							const qs::DynamicMenuItem* pItem);

public:
	virtual const WCHAR* getName() const;

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

class MoveMenuCreator : public MenuCreator
{
public:
	MoveMenuCreator(FolderModelBase* pFolderModel,
					MessageSelectionModel* pMessageSelectionModel,
					qs::ActionParamMap* pActionParamMap);
	~MoveMenuCreator();

public:
	virtual UINT createMenu(HMENU hmenu,
							UINT nIndex,
							const qs::DynamicMenuItem* pItem);

public:
	virtual const WCHAR* getName() const;

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

class RecentsMenuCreator : public MenuCreator
{
public:
	RecentsMenuCreator(Recents* pRecents,
					   AccountManager* pAccountManager,
					   qs::ActionParamMap* pActionParamMap);
	~RecentsMenuCreator();

public:
	virtual UINT createMenu(HMENU hmenu,
							UINT nIndex,
							const qs::DynamicMenuItem* pItem);

public:
	virtual const WCHAR* getName() const;

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

class ScriptMenuCreator : public MenuCreator
{
public:
	ScriptMenuCreator(ScriptManager* pScriptManager,
					  qs::ActionParamMap* pActionParamMap);
	~ScriptMenuCreator();

public:
	virtual UINT createMenu(HMENU hmenu,
							UINT nIndex,
							const qs::DynamicMenuItem* pItem);

public:
	virtual const WCHAR* getName() const;

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

class SortMenuCreator : public MenuCreator
{
public:
	SortMenuCreator(ViewModelManager* pViewModelManager,
					qs::ActionParamMap* pActionParamMap);
	~SortMenuCreator();

public:
	virtual UINT createMenu(HMENU hmenu,
							UINT nIndex,
							const qs::DynamicMenuItem* pItem);

public:
	virtual const WCHAR* getName() const;

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

class SubAccountMenuCreator : public MenuCreator
{
public:
	SubAccountMenuCreator(FolderModel* pFolderModel,
						  qs::ActionParamMap* pActionParamMap);
	~SubAccountMenuCreator();

public:
	virtual UINT createMenu(HMENU hmenu,
							UINT nIndex,
							const qs::DynamicMenuItem* pItem);

public:
	virtual const WCHAR* getName() const;

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

class TemplateMenuCreator : public MenuCreator
{
protected:
	TemplateMenuCreator(const TemplateManager* pTemplateManager,
						FolderModelBase* pFolderModel,
						qs::ActionParamMap* pActionParamMap);
	~TemplateMenuCreator();

public:
	virtual UINT createMenu(HMENU hmenu,
							UINT nIndex,
							const qs::DynamicMenuItem* pItem);

protected:
	virtual const WCHAR* getPrefix() const = 0;
	virtual UINT getBaseId() const = 0;
	virtual unsigned int getMax() const = 0;

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
	CreateTemplateMenuCreator(const TemplateManager* pTemplateManager,
							  FolderModelBase* pFolderModel,
							  bool bExternalEditor,
							  qs::ActionParamMap* pActionParamMap);
	~CreateTemplateMenuCreator();

public:
	virtual const WCHAR* getName() const;

protected:
	virtual const WCHAR* getPrefix() const;
	virtual UINT getBaseId() const;
	virtual unsigned int getMax() const;

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
	ViewTemplateMenuCreator(const TemplateManager* pTemplateManager,
							FolderModelBase* pFolderModel,
							qs::ActionParamMap* pActionParamMap);
	~ViewTemplateMenuCreator();

public:
	virtual const WCHAR* getName() const;

protected:
	virtual const WCHAR* getPrefix() const;
	virtual UINT getBaseId() const;
	virtual unsigned int getMax() const;

private:
	ViewTemplateMenuCreator(const ViewTemplateMenuCreator&);
	ViewTemplateMenuCreator& operator=(const ViewTemplateMenuCreator&);
};


/****************************************************************************
 *
 * MacroMenuCreator
 *
 */

class MacroMenuCreator : public qs::DynamicMenuCreator
{
private:
	typedef std::vector<std::pair<const WCHAR*, const WCHAR*> > ItemList;

public:
	MacroMenuCreator(Document* pDocument,
					 MessageSelectionModel* pMessageSelectionModel,
					 SecurityModel* pSecurityModel,
					 qs::Profile* pProfile,
					 const qs::ActionItem* pActionItem,
					 size_t nActionItemCount,
					 qs::ActionParamMap* pActionParamMap);
	MacroMenuCreator(Document* pDocument,
					 AccountSelectionModel* pAccountSelectionModel,
					 SecurityModel* pSecurityModel,
					 qs::Profile* pProfile,
					 const qs::ActionItem* pActionItem,
					 size_t nActionItemCount,
					 qs::ActionParamMap* pActionParamMap);
	~MacroMenuCreator();

public:
	virtual UINT createMenu(HMENU hmenu,
							UINT nIndex,
							const qs::DynamicMenuItem* pItem);

private:
	qs::wstring_ptr evalMacro(const Macro* pMacro) const;
	const qs::ActionItem* getActionItem(const WCHAR* pwszAction) const;

private:
	static void parseItems(WCHAR* pwsz,
						   ItemList* pList);

private:
	MacroMenuCreator(const MacroMenuCreator&);
	MacroMenuCreator& operator=(const MacroMenuCreator&);

private:
	Document* pDocument_;
	MessageSelectionModel* pMessageSelectionModel_;
	AccountSelectionModel* pAccountSelectionModel_;
	SecurityModel* pSecurityModel_;
	qs::Profile* pProfile_;
	const qs::ActionItem* pActionItem_;
	size_t nActionItemCount_;
	ActionParamHelper helper_;
};


/****************************************************************************
 *
 * MacroDynamicMenuItem
 *
 */

class MacroDynamicMenuItem : public qs::DynamicMenuItem
{
public:
	MacroDynamicMenuItem(unsigned int nId,
						 const WCHAR* pwszName,
						 const WCHAR* pwszParam);
	virtual ~MacroDynamicMenuItem();

public:
	const Macro* getMacro() const;

private:
	MacroDynamicMenuItem(const MacroDynamicMenuItem&);
	MacroDynamicMenuItem& operator=(const MacroDynamicMenuItem&);

private:
	std::auto_ptr<Macro> pMacro_;
};


/****************************************************************************
 *
 * MacroDynamicMenuMap
 *
 */

class MacroDynamicMenuMap : public qs::DynamicMenuMap
{
public:
	MacroDynamicMenuMap();
	virtual ~MacroDynamicMenuMap();

protected:
	virtual std::auto_ptr<qs::DynamicMenuItem> createItem(unsigned int nId,
														  const WCHAR* pwszName,
														  const WCHAR* pwszParam) const;

private:
	MacroDynamicMenuMap(const MacroDynamicMenuMap&);
	MacroDynamicMenuMap& operator=(const MacroDynamicMenuMap&);
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


/****************************************************************************
 *
 * MenuCreatorList
 *
 */

class MenuCreatorList
{
public:
	explicit MenuCreatorList(MenuCreatorListCallback* pCallback);
	~MenuCreatorList();

public:
	void add(std::auto_ptr<MenuCreator> pMenuCreator);
	qs::DynamicMenuCreator* get(const qs::DynamicMenuItem* pItem) const;

private:
	MenuCreatorList(const MenuCreatorList&);
	MenuCreatorList& operator=(const MenuCreatorList&);

private:
	typedef std::vector<MenuCreator*> List;

private:
	MenuCreatorListCallback* pCallback_;
	List list_;
	mutable std::auto_ptr<MacroMenuCreator> pMacroMenuCreator_;
};


/****************************************************************************
 *
 * MenuCreatorListCallback
 *
 */

class MenuCreatorListCallback
{
public:
	virtual ~MenuCreatorListCallback();

public:
	virtual std::auto_ptr<MacroMenuCreator> createMacroMenuCreator() = 0;
};

}

#endif // __MENUCREATOR_H__
