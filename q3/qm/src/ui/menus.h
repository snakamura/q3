/*
 * $Id: menus.h,v 1.2 2003/05/07 07:25:21 snakamura Exp $
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
class MoveMenu;
class ScriptMenu;
class TemplateMenu;
	class CreateTemplateMenu;
	class ViewTemplateMenu;

class Document;
class Message;
class MessageHolder;
class ScriptManager;
class TemplateManager;


/****************************************************************************
 *
 * AttachmentMenu
 *
 */

class AttachmentMenu
{
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
	EncodingMenu(qs::Profile* pProfile, qs::QSTATUS* pstatus);
	virtual ~EncodingMenu();

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
 * MoveMenu
 *
 */

class MoveMenu
{
public:
	explicit MoveMenu(qs::QSTATUS* pstatus);
	~MoveMenu();

public:
	NormalFolder* getFolder(unsigned int nId) const;
	qs::QSTATUS createMenu(HMENU hmenu,
		Account* pAccount, const qs::ActionMap& actionMap);

private:
	static bool isMovableFolder(const Folder* pFolder);
	static bool hasSelectableChildNormalFolder(
		Account::FolderList::const_iterator first,
		Account::FolderList::const_iterator last);
	static bool isDescendant(const Folder* pParent, const Folder* pChild);
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
 * TemplateMenu
 *
 */

class TemplateMenu
{
protected:
	TemplateMenu(const TemplateManager* pTemplateManager,
		qs::QSTATUS* pstatus);

public:
	virtual ~TemplateMenu();

public:
	const WCHAR* getTemplate(unsigned int nId) const;
	qs::QSTATUS createMenu(HMENU hmenu);

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
	virtual ~CreateTemplateMenu();

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
	virtual ~ViewTemplateMenu();

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
