/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __MENUS_H__
#define __MENUS_H__

#include <qm.h>
#include <qmaccount.h>
#include <qmfolder.h>

#include <qs.h>
#include <qsaction.h>
#include <qsmime.h>
#include <qsstring.h>

#include <vector>
#include <utility>


namespace qm {

class AttachmentMenu;
class EncodingMenu;
class FilterMenu;
class GoRoundMenu;
class MoveMenu;
class ScriptMenu;
class SortMenu;
class SubAccountMenu;
class TemplateMenu;
	class CreateTemplateMenu;
	class ViewTemplateMenu;

class Document;
class Filter;
class FilterManager;
class FolderModel;
class GoRound;
class GoRoundCourse;
class Message;
class MessageHolder;
class ScriptManager;
class TemplateManager;
class ViewModelManager;


/****************************************************************************
 *
 * AttachmentMenu
 *
 */

class AttachmentMenu
{
public:
	enum {
		MAX_ATTACHMENT = 100
	};

public:
	explicit AttachmentMenu(qs::QSTATUS* pstatus);
	~AttachmentMenu();

public:
	qs::QSTATUS getPart(unsigned int nId, Message* pMessage,
		qs::WSTRING* pwstrName, const qs::Part** ppPart) const;
	qs::QSTATUS createMenu(HMENU hmenu, const MessagePtrList& l);

private:
	AttachmentMenu(const AttachmentMenu&);
	AttachmentMenu& operator=(const AttachmentMenu&);

private:
	typedef std::vector<std::pair<unsigned int, MessageHolder*> > List;

private:
	List list_;
};


/****************************************************************************
 *
 * EncodingMenu
 *
 */

class EncodingMenu
{
public:
	enum {
		MAX_ENCODING = 100
	};

public:
	EncodingMenu(qs::Profile* pProfile, qs::QSTATUS* pstatus);
	~EncodingMenu();

public:
	const WCHAR* getEncoding(unsigned int nId) const;
	qs::QSTATUS createMenu(HMENU hmenu);

private:
	qs::QSTATUS load(qs::Profile* pProfile);

private:
	EncodingMenu(const EncodingMenu&);
	EncodingMenu& operator=(const EncodingMenu&);

private:
	typedef std::vector<qs::WSTRING> EncodingList;

private:
	EncodingList listEncoding_;
	bool bMenuCreated_;
};


/****************************************************************************
 *
 * FilterMenu
 *
 */

class FilterMenu
{
public:
	enum {
		MAX_FILTER = 100
	};

public:
	FilterMenu(FilterManager* pFilterManager, qs::QSTATUS* pstatus);
	~FilterMenu();

public:
	qs::QSTATUS getFilter(unsigned int nId, const Filter** ppFilter) const;
	qs::QSTATUS createMenu(HMENU hmenu);

private:
	FilterMenu(const FilterMenu&);
	FilterMenu& operator=(const FilterMenu&);

private:
	FilterManager* pFilterManager_;
};


/****************************************************************************
 *
 * GoRoundMenu
 *
 */

class GoRoundMenu
{
public:
	enum {
		MAX_COURSE = 100
	};

public:
	GoRoundMenu(GoRound* pGoRound, qs::QSTATUS* pstatus);
	~GoRoundMenu();

public:
	qs::QSTATUS getCourse(unsigned int nId,
		const GoRoundCourse** ppCourse) const;
	qs::QSTATUS createMenu(HMENU hmenu);

private:
	GoRoundMenu(const GoRoundMenu&);
	GoRoundMenu& operator=(const GoRoundMenu&);

private:
	GoRound* pGoRound_;
};


/****************************************************************************
 *
 * MoveMenu
 *
 */

class MoveMenu
{
public:
	enum {
		MAX_FOLDER = 999
	};

public:
	explicit MoveMenu(qs::QSTATUS* pstatus);
	~MoveMenu();

public:
	NormalFolder* getFolder(unsigned int nId) const;
	qs::QSTATUS createMenu(HMENU hmenu, Account* pAccount,
		bool bShowHidden, const qs::ActionMap& actionMap);

private:
	static bool isMovableFolder(const Folder* pFolder);
	static bool hasSelectableChildNormalFolder(
		Account::FolderList::const_iterator first,
		Account::FolderList::const_iterator last);
	static qs::QSTATUS formatName(const Folder* pFolder,
		unsigned int n, qs::WSTRING* pwstrName);

private:
	MoveMenu(const MoveMenu&);
	MoveMenu& operator=(const MoveMenu&);

private:
	struct MenuInserter
	{
		MenuInserter(HMENU hmenu, Folder* pFolder);
		
		HMENU hmenu_;
		Folder* pFolder_;
		unsigned int nCount_;
	};

private:
	typedef std::vector<NormalFolder*> MenuMap;

private:
	MenuMap mapMenu_;
};


/****************************************************************************
 *
 * ScriptMenu
 *
 */

class ScriptMenu
{
public:
	enum {
		MAX_SCRIPT = 100
	};

public:
	ScriptMenu(ScriptManager* pScriptManager, qs::QSTATUS* pstatus);
	~ScriptMenu();

public:
	const WCHAR* getScript(unsigned int nId) const;
	ScriptManager* getScriptManager() const;
	qs::QSTATUS createMenu(HMENU hmenu);

private:
	void clear();

private:
	ScriptMenu(const ScriptMenu&);
	ScriptMenu& operator=(const ScriptMenu&);

private:
	typedef std::vector<qs::WSTRING> List;

private:
	ScriptManager* pScriptManager_;
	List list_;
};


/****************************************************************************
 *
 * SortMenu
 *
 */

class SortMenu
{
public:
	enum {
		MAX_SORT = 100
	};

public:
	SortMenu(ViewModelManager* pViewModelManager, qs::QSTATUS* pstatus);
	~SortMenu();

public:
	unsigned int getSort(unsigned int nId) const;
	qs::QSTATUS createMenu(HMENU hmenu);

private:
	SortMenu(const SortMenu&);
	SortMenu& operator=(const SortMenu&);

private:
	ViewModelManager* pViewModelManager_;
};


/****************************************************************************
 *
 * SubAccountMenu
 *
 */

class SubAccountMenu
{
public:
	enum {
		MAX_SUBACCOUNT = 100
	};

public:
	SubAccountMenu(FolderModel* pFolderModel, qs::QSTATUS* pstatus);
	~SubAccountMenu();

public:
	const WCHAR* getName(unsigned int nId) const;
	qs::QSTATUS createMenu(HMENU hmenu);

private:
	SubAccountMenu(const SubAccountMenu&);
	SubAccountMenu& operator=(const SubAccountMenu&);

private:
	FolderModel* pFolderModel_;
};


/****************************************************************************
 *
 * TemplateMenu
 *
 */

class TemplateMenu
{
public:
	enum {
		MAX_TEMPLATE = 100
	};

protected:
	TemplateMenu(const TemplateManager* pTemplateManager,
		qs::QSTATUS* pstatus);
	~TemplateMenu();

public:
	const WCHAR* getTemplate(unsigned int nId) const;
	qs::QSTATUS createMenu(HMENU hmenu, Account* pAccount);

protected:
	virtual const WCHAR* getPrefix() const = 0;
	virtual UINT getId() const = 0;
	virtual UINT getNoneId() const = 0;
	virtual int getBase() const = 0;

private:
	void clear();

private:
	TemplateMenu(const TemplateMenu&);
	TemplateMenu& operator=(const TemplateMenu&);

private:
	typedef std::vector<qs::WSTRING> List;

private:
	const TemplateManager* pTemplateManager_;
	List list_;
};


/****************************************************************************
 *
 * CreateTemplateMenu
 *
 */

class CreateTemplateMenu : public TemplateMenu
{
public:
	CreateTemplateMenu(const TemplateManager* pTemplateManager,
		bool bExternalEditor, qs::QSTATUS* pstatus);
	~CreateTemplateMenu();

protected:
	virtual const WCHAR* getPrefix() const;
	virtual UINT getId() const;
	virtual UINT getNoneId() const;
	virtual int getBase() const;

private:
	CreateTemplateMenu(const CreateTemplateMenu&);
	CreateTemplateMenu& operator=(const CreateTemplateMenu&);

private:
	bool bExternalEditor_;
};


/****************************************************************************
 *
 * ViewTemplateMenu
 *
 */

class ViewTemplateMenu : public TemplateMenu
{
public:
	ViewTemplateMenu(const TemplateManager* pTemplateManager,
		qs::QSTATUS* pstatus);
	~ViewTemplateMenu();

protected:
	virtual const WCHAR* getPrefix() const;
	virtual UINT getId() const;
	virtual UINT getNoneId() const;
	virtual int getBase() const;

private:
	ViewTemplateMenu(const ViewTemplateMenu&);
	ViewTemplateMenu& operator=(const ViewTemplateMenu&);
};

}

#endif // __MENUS_H__
