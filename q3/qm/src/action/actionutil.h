/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#ifndef __ACTIONUTIL_H__
#define __ACTIONUTIL_H__

#include <qm.h>
#include <qmmessageholderlist.h>

#include <qsaction.h>
#include <qsprofile.h>


namespace qm {

class ActionUtil;
class ActionParamUtil;
class FolderActionUtil;
class MessageActionUtil;
#ifdef QMTABWINDOW
class TabActionUtil;
#endif

class Account;
class AccountManager;
class Folder;
class FolderModelBase;
class MessageModel;
#ifdef QMTABWINDOW
class TabModel;
#endif
class ViewModel;

/****************************************************************************
 *
 * ActionUtil
 *
 */

class ActionUtil
{
public:
	static void info(HWND hwnd,
					 UINT nMessage);
	static void error(HWND hwnd,
					  UINT nMessage);
	static void error(HWND hwnd,
					  const WCHAR* pwszMessage);
};


/****************************************************************************
 *
 * ActionParamUtil
 *
 */

class ActionParamUtil
{
public:
	static const WCHAR* getString(const qs::ActionParam* pParam,
								  size_t n);
	static unsigned int getNumber(const qs::ActionParam* pParam,
								  size_t n);
	static unsigned int getIndex(const qs::ActionParam* pParam,
								 size_t n);
	static std::pair<const WCHAR*, unsigned int> getStringOrIndex(const qs::ActionParam* pParam,
																  size_t n);
};


/****************************************************************************
 *
 * FolderActionUtil
 *
 */

class FolderActionUtil
{
public:
	static std::pair<Account*, Folder*> getCurrent(const FolderModelBase* pModel);
	static Account* getAccount(const FolderModelBase* pModel);
	static Folder* getFolder(const FolderModelBase* pModel);
	static Account* getAccount(const AccountManager* pAccountManager,
							   const FolderModelBase* pModel,
							   qs::Profile* pProfile,
							   const WCHAR* pwszClass);

private:
	static qs::wstring_ptr getDefaultKey(const WCHAR* pwszClass);
};


/****************************************************************************
 *
 * MessageActionUtil
 *
 */

class MessageActionUtil
{
public:
	static void select(ViewModel* pViewModel,
					   unsigned int nIndex,
					   MessageModel* pMessageModel);
	static void select(ViewModel* pViewModel,
					   unsigned int nIndex,
					   bool bDelay);
	static void selectNextUndeleted(ViewModel* pViewModel,
									unsigned int nIndex,
									const MessageHolderList& listExclude,
									MessageModel* pMessageModel);
};


#ifdef QMTABWINDOW
/****************************************************************************
 *
 * TabActionUtil
 *
 */

class TabActionUtil
{
public:
	static int getCurrent(TabModel* pModel);
};
#endif

}

#endif // __ACTIONUTIL_H__
