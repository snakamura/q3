/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __TEMPLATEMANAGER_H__
#define __TEMPLATEMANAGER_H__

#include <qs.h>


namespace qm {

class TemplateManager;

class Account;
class Folder;
class Template;


/****************************************************************************
 *
 * TemplateManager
 *
 */

class TemplateManager
{
public:
	typedef std::vector<qs::WSTRING> NameList;

public:
	TemplateManager(const WCHAR* pwszPath, qs::QSTATUS* pstatus);
	~TemplateManager();

public:
	qs::QSTATUS getTemplate(Account* pAccount, Folder* pFolder,
		const WCHAR* pwszName, const Template** ppTemplate) const;
	qs::QSTATUS getTemplateNames(Account* pAccount,
		const WCHAR* pwszPrefix, NameList* pList) const;

private:
	TemplateManager(const TemplateManager&);
	TemplateManager& operator=(const TemplateManager&);

private:
	class Item
	{
	public:
		Item(const WCHAR* pwszPath, const FILETIME& ft,
			Template* pTemplate, qs::QSTATUS* pstatus);
		~Item();
	
	public:
		const WCHAR* getPath() const;
		const FILETIME& getFileTime() const;
		Template* getTemplate() const;
	
	private:
		Item(const Item&);
		Item& operator=(const Item&);
	
	private:
		qs::WSTRING wstrPath_;
		FILETIME ft_;
		Template* pTemplate_;
	};

private:
	typedef std::vector<Item*> ItemList;

private:
	qs::WSTRING wstrPath_;
	mutable ItemList listItem_;
};

}

#endif // __TEMPLATEMANAGER_H__
