/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __ACTIVERULE_H__
#define __ACTIVERULE_H__

#include <qm.h>
#include <qmaccount.h>
#include <qmfolder.h>


namespace qm {

class ActiveRuleInvoker;

class ActionInvoker;
class Document;
class SecurityModel;


/****************************************************************************
 *
 * ActiveRuleInvoker
 *
 */

class ActiveRuleInvoker :
	public DefaultAccountManagerHandler,
	public DefaultAccountHandler,
	public FolderHook
{
public:
	ActiveRuleInvoker(Document* pDocument,
					  SecurityModel* pSecurityModel,
					  const ActionInvoker* pActionInvoker,
					  HWND hwnd,
					  qs::Profile* pProfile);
	virtual ~ActiveRuleInvoker();

public:
	virtual void accountListChanged(const AccountManagerEvent& event);

public:
	virtual void folderListChanged(const FolderListChangedEvent& event);

public:
	virtual unsigned int messageAdded(NormalFolder* pFolder,
									  const MessageHolderList& l,
									  unsigned int nOpFlags);

private:
	unsigned int applyRules(Folder* pFolder,
							const MessageHolderList& listMessageHolder,
							bool bBackground);
	void addHandlers(Account* pAccount);
	void removeHandlers(Account* pAccount);
	void addHook(Folder* pFolder);
	void removeHook(Folder* pFolder);

private:
	ActiveRuleInvoker(const ActiveRuleInvoker&);
	ActiveRuleInvoker& operator=(const ActiveRuleInvoker&);

private:
	Document* pDocument_;
	SecurityModel* pSecurityModel_;
	const ActionInvoker* pActionInvoker_;
	HWND hwnd_;
	qs::Profile* pProfile_;
};

}

#endif // __ACTIVERULE_H__
