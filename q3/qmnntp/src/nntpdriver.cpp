/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmaccount.h>
#include <qmfolder.h>
#include <qmmessage.h>
#include <qmmessageholder.h>

#include <qsassert.h>

#include "main.h"
#include "nntp.h"
#include "nntpdriver.h"
#include "resourceinc.h"

using namespace qmnntp;
using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * NntpDriver
 *
 */

const unsigned int qmnntp::NntpDriver::nSupport__ =
	Account::SUPPORT_LOCALFOLDERSYNC |
	Account::SUPPORT_LOCALFOLDERGETMESSAGE |
	Account::SUPPORT_JUNKFILTER;

qmnntp::NntpDriver::NntpDriver(Account* pAccount,
							   PasswordCallback* pPasswordCallback,
							   const Security* pSecurity) :
	pAccount_(pAccount),
	pPasswordCallback_(pPasswordCallback),
	pSecurity_(pSecurity),
	pNntp_(0),
	pCallback_(0),
	pLogger_(0),
	pSubAccount_(0),
	bOffline_(true),
	nForceDisconnect_(0),
	nLastUsedTime_(0)
{
}

qmnntp::NntpDriver::~NntpDriver()
{
	clearSession();
}

bool qmnntp::NntpDriver::isSupport(Account::Support support)
{
	return (nSupport__ & support) != 0;
}

void qmnntp::NntpDriver::setOffline(bool bOffline)
{
	if (!bOffline_ && bOffline)
		clearSession();
	bOffline_ = bOffline;
}

void qmnntp::NntpDriver::setSubAccount(qm::SubAccount* pSubAccount)
{
	if (pSubAccount != pSubAccount_) {
		clearSession();
		pSubAccount_ = pSubAccount;
		nForceDisconnect_ = pSubAccount_->getProperty(L"Nntp", L"ForceDisconnect", 0);
	}
}

bool qmnntp::NntpDriver::createDefaultFolders(Account::FolderList* pList)
{
	assert(pList);
	
	struct {
		UINT nId_;
		unsigned int nFlags_;
	} folders[] = {
		{ IDS_FOLDER_OUTBOX,	Folder::FLAG_LOCAL | Folder::FLAG_OUTBOX | Folder::FLAG_DRAFTBOX		},
		{ IDS_FOLDER_POSTED,	Folder::FLAG_LOCAL | Folder::FLAG_SENTBOX								},
		{ IDS_FOLDER_TRASH,		Folder::FLAG_LOCAL | Folder::FLAG_TRASHBOX | Folder::FLAG_IGNOREUNSEEN	},
#ifndef _WIN32_WCE
		{ IDS_FOLDER_JUNK,		Folder::FLAG_LOCAL | Folder::FLAG_JUNKBOX | Folder::FLAG_IGNOREUNSEEN	}
#endif
	};
	
	pList->reserve(countof(folders));
	
	for (int n = 0; n < countof(folders); ++n) {
		wstring_ptr wstrName(loadString(getResourceHandle(), folders[n].nId_));
		NormalFolder* pFolder = new NormalFolder(n + 1, wstrName.get(),
			L'/', folders[n].nFlags_, 0, 0, 0, 0, 0, 0, pAccount_);
		pList->push_back(pFolder);
	}
	
	return true;
}

bool qmnntp::NntpDriver::getMessage(MessageHolder* pmh,
									unsigned int nFlags,
									GetMessageCallback* pCallback)
{
	assert(pmh);
	assert(pCallback);
	
	if (bOffline_)
		return true;
	
	if (!prepareSession(pmh->getFolder()))
		return false;
	
	xstring_size_ptr strMessage;
	if (!pNntp_->getMessage(pmh->getId(),
		Nntp::GETMESSAGEFLAG_ARTICLE, &strMessage, pmh->getSize())) {
		clearSession();
		return false;
	}
	if (!strMessage.get())
		return false;
	
	if (!pCallback->message(strMessage.get(), strMessage.size(), Message::FLAG_NONE, false))
		return false;
	
	return true;
}

bool qmnntp::NntpDriver::prepareSession(NormalFolder* pFolder)
{
	bool bConnect = !pNntp_.get();
	if (!bConnect)
		bConnect = isForceDisconnect();
	
	if (bConnect) {
		clearSession();
		
		std::auto_ptr<Logger> pLogger;
		std::auto_ptr<DefaultCallback> pCallback;
		std::auto_ptr<Nntp> pNntp;
		
		if (pSubAccount_->isLog(Account::HOST_RECEIVE))
			pLogger = pAccount_->openLogger(Account::HOST_RECEIVE);
		
		pCallback.reset(new DefaultCallback(pSubAccount_, pPasswordCallback_, pSecurity_));
		
		pNntp.reset(new Nntp(pSubAccount_->getTimeout(), pCallback.get(),
			pCallback.get(), pCallback.get(), pLogger.get()));
		if (!pNntp->connect(pSubAccount_->getHost(Account::HOST_RECEIVE),
			pSubAccount_->getPort(Account::HOST_RECEIVE),
			pSubAccount_->getSecure(Account::HOST_RECEIVE) == SubAccount::SECURE_SSL))
			return false;
		
		pNntp_ = pNntp;
		pCallback_ = pCallback;
		pLogger_ = pLogger;
	}
	
	assert(pNntp_.get());
	
	wstring_ptr wstrGroup(pFolder->getFullName());
	
	const WCHAR* pwszGroup = pNntp_->getGroup();
	if (!pwszGroup || wcscmp(wstrGroup.get(), pwszGroup) != 0) {
		if (!pNntp_->group(wstrGroup.get()))
			return false;
	}
	
	nLastUsedTime_ = ::GetTickCount();
	
	return true;
}

void qmnntp::NntpDriver::clearSession()
{
	if (pNntp_.get() && !isForceDisconnect())
		pNntp_->disconnect();
	pNntp_.reset(0);
	pCallback_.reset(0);
	pLogger_.reset(0);
}

bool qmnntp::NntpDriver::isForceDisconnect() const
{
	return nForceDisconnect_ != 0 && nLastUsedTime_ + nForceDisconnect_*1000 < ::GetTickCount();
}


/****************************************************************************
 *
 * NntpFactory
 *
 */

NntpFactory qmnntp::NntpFactory::factory__;

qmnntp::NntpFactory::NntpFactory()
{
	registerFactory(L"nntp", this);
}

qmnntp::NntpFactory::~NntpFactory()
{
	unregisterFactory(L"nntp");
}

std::auto_ptr<ProtocolDriver> qmnntp::NntpFactory::createDriver(Account* pAccount,
																PasswordCallback* pPasswordCallback,
																const Security* pSecurity)
{
	return std::auto_ptr<ProtocolDriver>(new NntpDriver(
		pAccount, pPasswordCallback, pSecurity));
}
