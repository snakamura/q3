/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmdocument.h>
#include <qmrule.h>

#include "activerule.h"
#include "../uimodel/securitymodel.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * ActiveRuleInvoker
 *
 */

qm::ActiveRuleInvoker::ActiveRuleInvoker(Document* pDocument,
										 SecurityModel* pSecurityModel,
										 HWND hwnd,
										 Profile* pProfile) :
	pDocument_(pDocument),
	pSecurityModel_(pSecurityModel),
	hwnd_(hwnd),
	pProfile_(pProfile)
{
	const Document::AccountList& l = pDocument_->getAccounts();
	std::for_each(l.begin(), l.end(),
		std::bind1st(std::mem_fun(&ActiveRuleInvoker::addHandlers), this));
	pDocument_->addAccountManagerHandler(this);
}

qm::ActiveRuleInvoker::~ActiveRuleInvoker()
{
	const Document::AccountList& l = pDocument_->getAccounts();
	std::for_each(l.begin(), l.end(),
		std::bind1st(std::mem_fun(&ActiveRuleInvoker::removeHandlers), this));
	pDocument_->removeAccountManagerHandler(this);
}

void qm::ActiveRuleInvoker::accountListChanged(const AccountManagerEvent& event)
{
	switch (event.getType()) {
	case AccountManagerEvent::TYPE_ALL:
		break;
	case AccountManagerEvent::TYPE_ADD:
		addHandlers(event.getAccount());
		break;
	case AccountManagerEvent::TYPE_REMOVE:
		removeHandlers(event.getAccount());
		break;
	default:
		assert(false);
		break;
	}
}

void qm::ActiveRuleInvoker::folderListChanged(const FolderListChangedEvent& event)
{
	switch (event.getType()) {
	case FolderListChangedEvent::TYPE_ALL:
		removeHandlers(event.getAccount());
		addHandlers(event.getAccount());
		break;
	case FolderListChangedEvent::TYPE_ADD:
		addHook(event.getFolder());
		break;
	case FolderListChangedEvent::TYPE_REMOVE:
		removeHook(event.getFolder());
		break;
	case FolderListChangedEvent::TYPE_RENAME:
	case FolderListChangedEvent::TYPE_MOVE:
	case FolderListChangedEvent::TYPE_FLAGS:
		break;
	default:
		assert(false);
		break;
	}
}

unsigned int qm::ActiveRuleInvoker::messageAdded(NormalFolder* pFolder,
												 const MessageHolderList& l,
												 unsigned int nOpFlags)
{
	if (!(nOpFlags & Account::OPFLAG_ACTIVE))
		return Account::RESULTFLAG_NONE;
	return applyRules(pFolder, l, (nOpFlags & Account::OPFLAG_BACKGROUND) != 0);
}

unsigned int qm::ActiveRuleInvoker::applyRules(Folder* pFolder,
											   const MessageHolderList& listMessageHolder,
											   bool bBackground)
{
	RuleManager* pRuleManager = pDocument_->getRuleManager();
	unsigned int nResultFlags = 0;
	if (!pRuleManager->applyActive(pFolder, listMessageHolder,
		pDocument_, hwnd_, pProfile_, pSecurityModel_->getSecurityMode(),
		bBackground, &nResultFlags))
		nResultFlags |= Account::RESULTFLAG_ALL;
	return nResultFlags;
}

void qm::ActiveRuleInvoker::addHandlers(Account* pAccount)
{
	const Account::FolderList& l = pAccount->getFolders();
	std::for_each(l.begin(), l.end(),
		std::bind1st(std::mem_fun(&ActiveRuleInvoker::addHook), this));
	pAccount->addAccountHandler(this);
}

void qm::ActiveRuleInvoker::removeHandlers(Account* pAccount)
{
	const Account::FolderList& l = pAccount->getFolders();
	std::for_each(l.begin(), l.end(),
		std::bind1st(std::mem_fun(&ActiveRuleInvoker::removeHook), this));
	pAccount->removeAccountHandler(this);
}

void qm::ActiveRuleInvoker::addHook(Folder* pFolder)
{
	if (pFolder->getType() == Folder::TYPE_NORMAL)
		static_cast<NormalFolder*>(pFolder)->setHook(this);
}

void qm::ActiveRuleInvoker::removeHook(Folder* pFolder)
{
	if (pFolder->getType() == Folder::TYPE_NORMAL)
		static_cast<NormalFolder*>(pFolder)->setHook(0);
}
