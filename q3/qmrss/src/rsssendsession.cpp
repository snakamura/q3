/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include "main.h"
#include "resourceinc.h"
#include "rsssendsession.h"
#include "ui.h"

using namespace qmrss;
using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * RssSendSession
 *
 */

qmrss::RssSendSession::RssSendSession()
{
}

qmrss::RssSendSession::~RssSendSession()
{
}

bool qmrss::RssSendSession::init(Document* pDocument,
								 Account* pAccount,
								 SubAccount* pSubAccount,
								 Profile* pProfile,
								 Logger* pLogger,
								 SendSessionCallback* pCallback)
{
	return false;
}

void qmrss::RssSendSession::term()
{
}

bool qmrss::RssSendSession::connect()
{
	return false;
}

void qmrss::RssSendSession::disconnect()
{
}

bool qmrss::RssSendSession::sendMessage(Message* pMessage)
{
	return false;
}


/****************************************************************************
*
* RssSendSessionUI
*
*/

qmrss::RssSendSessionUI::RssSendSessionUI()
{
}

qmrss::RssSendSessionUI::~RssSendSessionUI()
{
}

const WCHAR* qmrss::RssSendSessionUI::getClass()
{
	return L"rss";
}

wstring_ptr qmrss::RssSendSessionUI::getDisplayName()
{
	return loadString(getResourceHandle(), IDS_RSSSEND);
}

short qmrss::RssSendSessionUI::getDefaultPort()
{
	return 0;
}

std::auto_ptr<PropertyPage> qmrss::RssSendSessionUI::createPropertyPage(SubAccount* pSubAccount)
{
	return std::auto_ptr<PropertyPage>(new SendPage(pSubAccount));
}


/****************************************************************************
*
* RssSendSessionFactory
*
*/

RssSendSessionFactory qmrss::RssSendSessionFactory::factory__;

qmrss::RssSendSessionFactory::RssSendSessionFactory()
{
	registerFactory(L"rss", this);
}

qmrss::RssSendSessionFactory::~RssSendSessionFactory()
{
	unregisterFactory(L"rss");
}

std::auto_ptr<SendSession> qmrss::RssSendSessionFactory::createSession()
{
	return std::auto_ptr<SendSession>(new RssSendSession());
}

std::auto_ptr<SendSessionUI> qmrss::RssSendSessionFactory::createUI()
{
	return std::auto_ptr<SendSessionUI>(new RssSendSessionUI());
}
