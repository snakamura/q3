/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
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
	TemplateManager(const WCHAR* pwszPath);
	~TemplateManager();

public:
	const Template* getTemplate(Account* pAccount,
								Folder* pFolder,
								const WCHAR* pwszName) const;
	void getTemplateNames(const WCHAR* pwszClass,
						  const WCHAR* pwszPrefix,
						  NameList* pList) const;

private:
	TemplateManager(const TemplateManager&);
	TemplateManager& operator=(const TemplateManager&);

private:
	class Item
	{
	public:
		Item(const WCHAR* pwszPath,
			 const FILETIME& ft,
			 std::auto_ptr<Template> pTemplate);
		~Item();
	
	public:
		const WCHAR* getPath() const;
		const FILETIME& getFileTime() const;
		const Template* getTemplate() const;
	
	private:
		Item(const Item&);
		Item& operator=(const Item&);
	
	private:
		qs::wstring_ptr wstrPath_;
		FILETIME ft_;
		std::auto_ptr<Template> pTemplate_;
	};

private:
	typedef std::vector<Item*> ItemList;

private:
	qs::wstring_ptr wstrPath_;
	mutable ItemList listItem_;
};

}

#endif // __TEMPLATEMANAGER_H__
