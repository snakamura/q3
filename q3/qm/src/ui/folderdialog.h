/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __FOLDERDIALOG_H__
#define __FOLDERDIALOG_H__

#include <qm.h>
#include <qmaccount.h>

#include <qsprofile.h>

#include "dialogs.h"
#include "propertypages.h"

namespace qm {

class CreateFolderDialog;
class FolderPropertyPage;
class FolderConditionPage;
class FolderParameterPage;
class ParameterDialog;

class Folder;
class QueryFolder;
class SearchUI;


/****************************************************************************
 *
 * CreateFolderDialog
 *
 */

class CreateFolderDialog : public DefaultDialog
{
public:
	enum Type {
		TYPE_LOCALFOLDER,
		TYPE_REMOTEFOLDER,
		TYPE_QUERYFOLDER
	};
	
	enum Flag {
		FLAG_ALLOWREMOTE	= 0x01
	};

public:
	CreateFolderDialog(Type type,
					   unsigned int nFlags);
	virtual ~CreateFolderDialog();

public:
	Type getType() const;
	const WCHAR* getName() const;

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

protected:
	virtual LRESULT onOk();

private:
	LRESULT onNameChange();

private:
	void updateState();

private:
	CreateFolderDialog(const CreateFolderDialog&);
	CreateFolderDialog& operator=(const CreateFolderDialog&);

private:
	Type type_;
	unsigned int nFlags_;
	qs::wstring_ptr wstrName_;
};


/****************************************************************************
 *
 * FolderPropertyPage
 *
 */

class FolderPropertyPage : public DefaultPropertyPage
{
public:
	FolderPropertyPage(const Account::FolderList& l);
	virtual ~FolderPropertyPage();

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

protected:
	virtual LRESULT onOk();

private:
	FolderPropertyPage(const FolderPropertyPage&);
	FolderPropertyPage& operator=(const FolderPropertyPage&);

private:
	const Account::FolderList& listFolder_;
};


/****************************************************************************
 *
 * FolderConditionPage
 *
 */

class FolderConditionPage : public DefaultPropertyPage
{
public:
	FolderConditionPage(QueryFolder* pFolder,
						qs::Profile* pProfile);
	virtual ~FolderConditionPage();

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

protected:
	virtual LRESULT onOk();

private:
	void initDriver();
	void initFolder();

private:
	FolderConditionPage(const FolderConditionPage&);
	FolderConditionPage& operator=(const FolderConditionPage&);

private:
	typedef std::vector<SearchUI*> UIList;

private:
	QueryFolder* pFolder_;
	qs::Profile* pProfile_;
	UIList listUI_;
};


/****************************************************************************
 *
 * FolderParameterPage
 *
 */

class FolderParameterPage : public DefaultPropertyPage
{
public:
	FolderParameterPage(Folder* pFolder,
						const WCHAR** ppwszParams,
						size_t nParamCount);
	virtual ~FolderParameterPage();

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

protected:
	virtual LRESULT onOk();

public:
	virtual LRESULT onNotify(NMHDR* pnmhdr,
							 bool* pbHandled);

private:
	LRESULT onEdit();
	LRESULT onParameterDblClk(NMHDR* pnmhdr,
							  bool* pbHandled);
	LRESULT onParameterItemChanged(NMHDR* pnmhdr,
								   bool* pbHandled);

private:
	void edit();
	void updateState();

private:
	FolderParameterPage(const FolderParameterPage&);
	FolderParameterPage& operator=(const FolderParameterPage&);

private:
	Folder* pFolder_;
	const WCHAR** ppwszParams_;
	size_t nParamCount_;
};


/****************************************************************************
 *
 * ParameterDialog
 *
 */

class ParameterDialog : public DefaultDialog
{
public:
	ParameterDialog(const WCHAR* pwszName,
					const WCHAR* pwszValue);
	virtual ~ParameterDialog();

public:
	const WCHAR* getValue() const;

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

protected:
	virtual LRESULT onOk();

private:
	ParameterDialog(const ParameterDialog&);
	ParameterDialog& operator=(const ParameterDialog&);

private:
	qs::wstring_ptr wstrName_;
	qs::wstring_ptr wstrValue_;
};

}

#endif // __FOLDERDIALOG_H__
