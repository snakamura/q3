/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
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
class InsertTextMenu;
class MoveMenu;
class RecentsMenu;
class ScriptMenu;
class SortMenu;
class SubAccountMenu;
class TemplateMenu;
	class CreateTemplateMenu;
	class ViewTemplateMenu;

class Document;
class Filter;
class FilterManager;
class FixedFormText;
class FixedFormTextManager;
class FolderModel;
class GoRound;
class GoRoundCourse;
class Message;
class MessageHolder;
class ScriptManager;
class SecurityModel;
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
	explicit AttachmentMenu(SecurityModel* pSecurityModel);
	~AttachmentMenu();

public:
	bool getPart(unsigned int nId,
				 Message* pMessage,
				 qs::wstring_ptr* pwstrName,
				 const qs::Part** ppPart) const;
	bool createMenu(HMENU hmenu,
					const MessagePtr& ptr);

private:
	AttachmentMenu(const AttachmentMenu&);
	AttachmentMenu& operator=(const AttachmentMenu&);

private:
	typedef std::vector<std::pair<unsigned int, MessageHolder*> > List;

private:
	SecurityModel* pSecurityModel_;
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
	explicit EncodingMenu(qs::Profile* pProfile);
	~EncodingMenu();

public:
	const WCHAR* getEncoding(unsigned int nId) const;
	bool createMenu(HMENU hmenu);

private:
	void load(qs::Profile* pProfile);

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
	explicit FilterMenu(FilterManager* pFilterManager);
	~FilterMenu();

public:
	const Filter* getFilter(unsigned int nId) const;
	bool createMenu(HMENU hmenu);

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
	explicit GoRoundMenu(GoRound* pGoRound);
	~GoRoundMenu();

public:
	const GoRoundCourse* getCourse(unsigned int nId) const;
	bool createMenu(HMENU hmenu);

private:
	GoRoundMenu(const GoRoundMenu&);
	GoRoundMenu& operator=(const GoRoundMenu&);

private:
	GoRound* pGoRound_;
};


/****************************************************************************
 *
 * InsertTextMenu
 *
 */

class InsertTextMenu
{
public:
	explicit InsertTextMenu(FixedFormTextManager* pManager);
	~InsertTextMenu();

public:
	const FixedFormText* getText(unsigned int nId) const;
	bool createMenu(HMENU hmenu);

private:
	InsertTextMenu(const InsertTextMenu&);
	InsertTextMenu& operator=(const InsertTextMenu&);

private:
	FixedFormTextManager* pManager_;
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
	MoveMenu();
	~MoveMenu();

public:
	NormalFolder* getFolder(unsigned int nId) const;
	bool createMenu(HMENU hmenu,
					Account* pAccount,
					bool bShowHidden,
					const qs::ActionMap& actionMap);

private:
	static bool isMovableFolder(const Folder* pFolder);
	static bool hasSelectableChildNormalFolder(Account::FolderList::const_iterator first,
											   Account::FolderList::const_iterator last);
	static qs::wstring_ptr formatName(const Folder* pFolder,
									  unsigned int n);

private:
	MoveMenu(const MoveMenu&);
	MoveMenu& operator=(const MoveMenu&);

private:
	struct MenuInserter
	{
		MenuInserter(HMENU hmenu,
					 Folder* pFolder);
		
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
 * RecentsMenu
 *
 */

class RecentsMenu
{
public:
	enum {
		MAX_RECENTS = 100
	};

public:
	explicit RecentsMenu(Document* pDocument);
	~RecentsMenu();

public:
	const WCHAR* getURI(unsigned int nId) const;
	bool createMenu(HMENU hmenu);

private:
	void clear();

private:
	struct URIComp : public std::binary_function<const WCHAR*, const WCHAR*, bool>
	{
		bool operator()(const WCHAR* pwszLhs,
						const WCHAR* pwszRhs);
	};

private:
	RecentsMenu(const RecentsMenu&);
	RecentsMenu& operator=(const RecentsMenu&);

private:
	typedef std::vector<qs::WSTRING> URIList;

private:
	Document* pDocument_;
	URIList listURI_;
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
	explicit ScriptMenu(ScriptManager* pScriptManager);
	~ScriptMenu();

public:
	const WCHAR* getScript(unsigned int nId) const;
	ScriptManager* getScriptManager() const;
	bool createMenu(HMENU hmenu);

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
	explicit SortMenu(ViewModelManager* pViewModelManager);
	~SortMenu();

public:
	unsigned int getSort(unsigned int nId) const;
	bool createMenu(HMENU hmenu);

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
	explicit SubAccountMenu(FolderModel* pFolderModel);
	~SubAccountMenu();

public:
	const WCHAR* getName(unsigned int nId) const;
	bool createMenu(HMENU hmenu);

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
	explicit TemplateMenu(const TemplateManager* pTemplateManager);
	~TemplateMenu();

public:
	const WCHAR* getTemplate(unsigned int nId) const;
	bool createMenu(HMENU hmenu, Account* pAccount);

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
					   bool bExternalEditor);
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
	explicit ViewTemplateMenu(const TemplateManager* pTemplateManager);
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
