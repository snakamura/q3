/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmaccount.h>

using namespace qm;


/****************************************************************************
 *
 * AccountManager
 *
 */

qm::AccountManager::~AccountManager()
{
}


/****************************************************************************
 *
 * AccountManagerHandler
 *
 */

qm::AccountManagerHandler::~AccountManagerHandler()
{
}


/****************************************************************************
 *
 * AccountManagerEvent
 *
 */

qm::AccountManagerEvent::AccountManagerEvent(AccountManager* pAccountManager,
											 Type type,
											 Account* pAccount) :
	pAccountManager_(pAccountManager),
	type_(type),
	pAccount_(pAccount)
{
}

qm::AccountManagerEvent::~AccountManagerEvent()
{
}

AccountManager* qm::AccountManagerEvent::getAccountManager() const
{
	return pAccountManager_;
}

AccountManagerEvent::Type qm::AccountManagerEvent::getType() const
{
	return type_;
}

Account* qm::AccountManagerEvent::getAccount() const
{
	return pAccount_;
}
