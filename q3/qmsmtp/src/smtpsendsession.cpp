/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmaccount.h>
#include <qmdocument.h>
#include <qmfolder.h>
#include <qmmessage.h>
#include <qmmessageholder.h>
#include <qmsecurity.h>

#include <qsconv.h>
#include <qsnew.h>
#include <qsthread.h>

#include <algorithm>

#include "main.h"
#include "resourceinc.h"
#include "smtpsendsession.h"
#include "ui.h"

using namespace qmsmtp;
using namespace qm;
using namespace qs;

#define CHECK_QSTATUS_ERROR() \
	if (status != QSTATUS_SUCCESS) { \
		reportError(); \
		return status; \
	} \


/****************************************************************************
 *
 * SmtpSendSession
 *
 */

qmsmtp::SmtpSendSession::SmtpSendSession(QSTATUS* pstatus) :
	pSmtp_(0),
	pCallback_(0),
	pAccount_(0),
	pSubAccount_(0),
	pLogger_(0),
	pSessionCallback_(0)
{
	assert(pstatus);
	
	*pstatus = QSTATUS_SUCCESS;
}

qmsmtp::SmtpSendSession::~SmtpSendSession()
{
	delete pSmtp_;
	delete pCallback_;
}

QSTATUS qmsmtp::SmtpSendSession::init(Document* pDocument,
	Account* pAccount, SubAccount* pSubAccount, Profile* pProfile,
	Logger* pLogger, SendSessionCallback* pCallback)
{
	assert(pAccount);
	assert(pSubAccount);
	assert(pCallback);
	
	DECLARE_QSTATUS();
	
	pAccount_ = pAccount;
	pSubAccount_ = pSubAccount;
	pLogger_ = pLogger;
	pSessionCallback_ = pCallback;
	
	status = newQsObject(pSubAccount_, pDocument->getSecurity(),
		pSessionCallback_, &pCallback_);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmsmtp::SmtpSendSession::connect()
{
	assert(!pSmtp_);
	
	DECLARE_QSTATUS();
	
	Log log(pLogger_, L"qmsmtp::SmtpSendSession");
	status = log.debug(L"Connecting to the server...");
	CHECK_QSTATUS();
	
	Smtp::Option option = {
		pSubAccount_->getTimeout(),
		pCallback_,
		pCallback_,
		pCallback_,
		pLogger_
	};
	status = newQsObject(option, &pSmtp_);
	CHECK_QSTATUS();
	
	Smtp::Ssl ssl = Smtp::SSL_NONE;
	if (pSubAccount_->isSsl(Account::HOST_SEND)) {
		ssl = Smtp::SSL_SSL;
	}
	else {
		int nStartTls = 0;
		status = pSubAccount_->getProperty(L"Smtp", L"STARTTLS", 0, &nStartTls);
		CHECK_QSTATUS();
		if (nStartTls)
			ssl = Smtp::SSL_STARTTLS;
	}
	
	status = pSmtp_->connect(pSubAccount_->getHost(Account::HOST_SEND),
		pSubAccount_->getPort(Account::HOST_SEND), ssl);
	CHECK_QSTATUS_ERROR();
	
	status = log.debug(L"Connected to the server.");
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmsmtp::SmtpSendSession::disconnect()
{
	assert(pSmtp_);
	
	DECLARE_QSTATUS();
	
	Log log(pLogger_, L"qmsmtp::SmtpSendSession");
	status = log.debug(L"Disconnecting from the server...");
	CHECK_QSTATUS();
	
	status = pSmtp_->disconnect();
	CHECK_QSTATUS_ERROR();
	
	status = log.debug(L"Disconnected from the server.");
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmsmtp::SmtpSendSession::sendMessage(Message* pMessage)
{
	assert(pMessage);
	
	DECLARE_QSTATUS();
	
	Log log(pLogger_, L"qmsmtp::SmtpSendSession");
	status = log.debug(L"Sending message...");
	CHECK_QSTATUS();
	
	status = pCallback_->setMessage(IDS_SENDMESSAGE);
	CHECK_QSTATUS();
	
	PartUtil util(*pMessage);
	bool bResent = false;
	status = util.isResent(&bResent);
	CHECK_QSTATUS();
	
	string_ptr<WSTRING> wstrEnvelopeFrom;
	Part::Field field = Part::FIELD_ERROR;
	AddressParser envelopeFrom(0, &status);
	CHECK_QSTATUS();
	status = pMessage->getField(L"X-QMAIL-EnvelopeFrom", &envelopeFrom, &field);
	CHECK_QSTATUS();
	if (field == Part::FIELD_EXIST) {
		status = envelopeFrom.getAddress(&wstrEnvelopeFrom);
		CHECK_QSTATUS();
	}
	else {
		const WCHAR* pwszNormalFields[] = {
			L"Sender",
			L"From"
		};
		const WCHAR* pwszResentFields[] = {
			L"Resent-Sender",
			L"Resent-From"
		};
		const WCHAR** ppwszFields = bResent ? pwszResentFields : pwszNormalFields;
		for (int n = 0; n < 2 && !wstrEnvelopeFrom.get(); ++n) {
			AddressListParser address(0, &status);
			CHECK_QSTATUS();
			status = pMessage->getField(*(ppwszFields + n), &address, &field);
			CHECK_QSTATUS();
			if (field == Part::FIELD_EXIST) {
				const AddressListParser::AddressList& l = address.getAddressList();
				if (!l.empty()) {
					status = l.front()->getAddress(&wstrEnvelopeFrom);
					CHECK_QSTATUS();
				}
			}
		}
	}
	if (!wstrEnvelopeFrom.get())
		return QSTATUS_FAIL;
	
	std::vector<STRING> vecAddress;
	struct Deleter
	{
		Deleter(std::vector<STRING>& v) : v_(v) {}
		~Deleter()
			{ std::for_each(v_.begin(), v_.end(), string_free<STRING>()); }
		std::vector<STRING>& v_;
	} deleter(vecAddress);
	const WCHAR* pwszNormalFields[] = {
		L"To",
		L"Cc",
		L"Bcc"
	};
	const WCHAR* pwszResentFields[] = {
		L"Resent-To",
		L"Resent-Cc",
		L"Resent-Bcc"
	};
	const WCHAR** ppwszFields = bResent ? pwszResentFields : pwszNormalFields;
	for (int n = 0; n < 3; ++n) {
		AddressListParser address(0, &status);
		CHECK_QSTATUS();
		status = pMessage->getField(*(ppwszFields + n), &address, &field);
		CHECK_QSTATUS();
		if (field == Part::FIELD_EXIST) {
			bool bReplace = false;
			
			const AddressListParser::AddressList& l = address.getAddressList();
			AddressListParser::AddressList::const_iterator it = l.begin();
			while (it != l.end()) {
				AddressParser* pAddress = *it;
				AddressListParser* pGroup = pAddress->getGroup();
				if (pGroup) {
					const AddressListParser::AddressList& groups = pGroup->getAddressList();
					AddressListParser::AddressList::const_iterator itG = groups.begin();
					while (itG != groups.end()) {
						string_ptr<WSTRING> wstrAddress;
						status = (*itG)->getAddress(&wstrAddress);
						CHECK_QSTATUS();
						string_ptr<STRING> strAddress(wcs2mbs(wstrAddress.get()));
						if (!strAddress.get())
							return QSTATUS_OUTOFMEMORY;
						status = STLWrapper<std::vector<STRING> >(
							vecAddress).push_back(strAddress.get());
						CHECK_QSTATUS();
						strAddress.release();
						++itG;
					}
					pGroup->removeAllAddresses();
					bReplace = true;
				}
				else {
					string_ptr<WSTRING> wstrAddress;
					status = pAddress->getAddress(&wstrAddress);
					CHECK_QSTATUS();
					string_ptr<STRING> strAddress(wcs2mbs(wstrAddress.get()));
					if (!strAddress.get())
						return QSTATUS_OUTOFMEMORY;
					status = STLWrapper<std::vector<STRING> >(
						vecAddress).push_back(strAddress.get());
					CHECK_QSTATUS();
					strAddress.release();
				}
				++it;
			}
			
			if (bReplace) {
				status = pMessage->replaceField(*(ppwszFields + n), address);
				CHECK_QSTATUS();
			}
		}
	}
	
	const WCHAR* pwszRemoveFields[] = {
		L"Bcc",
		L"Resent-Bcc",
		L"X-QMAIL-EnvelopeFrom"
	};
	for (int m = 0; m < countof(pwszRemoveFields); ++m) {
		status = pMessage->removeField(*(pwszRemoveFields + m));
		CHECK_QSTATUS();
	}
	
	string_ptr<STRING> strContent;
	status = pMessage->getContent(&strContent);
	CHECK_QSTATUS();
	
	string_ptr<STRING> strEnvelopeFrom(wcs2mbs(wstrEnvelopeFrom.get()));
	if (!strEnvelopeFrom.get())
		return QSTATUS_OUTOFMEMORY;
	Smtp::SendMessageData data = {
		strEnvelopeFrom.get(),
		const_cast<const CHAR**>(&vecAddress[0]),
		vecAddress.size(),
		strContent.get(),
		strlen(strContent.get())
	};
	status = pSmtp_->sendMessage(data);
	CHECK_QSTATUS_ERROR();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmsmtp::SmtpSendSession::reportError()
{
	assert(pSmtp_);
	
	DECLARE_QSTATUS();
	
	struct
	{
		unsigned int nError_;
		UINT nId_;
	} maps[][12] = {
		{
			{ Smtp::SMTP_ERROR_GREETING,	IDS_ERROR_GREETING	},
			{ Smtp::SMTP_ERROR_HELO,		IDS_ERROR_HELO		},
			{ Smtp::SMTP_ERROR_EHLO,		IDS_ERROR_EHLO		},
			{ Smtp::SMTP_ERROR_AUTH,		IDS_ERROR_AUTH		},
			{ Smtp::SMTP_ERROR_MAIL,		IDS_ERROR_MAIL		},
			{ Smtp::SMTP_ERROR_RCPT,		IDS_ERROR_RCPT		},
			{ Smtp::SMTP_ERROR_DATA,		IDS_ERROR_DATA		},
			{ Smtp::SMTP_ERROR_STARTTLS,	IDS_ERROR_STARTTLS	}
		},
		{
			{ Smtp::SMTP_ERROR_INITIALIZE,		IDS_ERROR_INITIALIZE	},
			{ Smtp::SMTP_ERROR_CONNECT,			IDS_ERROR_CONNECT		},
			{ Smtp::SMTP_ERROR_RESPONSE,		IDS_ERROR_RESPONSE		},
			{ Smtp::SMTP_ERROR_INVALIDSOCKET,	IDS_ERROR_INVALIDSOCKET	},
			{ Smtp::SMTP_ERROR_TIMEOUT,			IDS_ERROR_TIMEOUT		},
			{ Smtp::SMTP_ERROR_SELECT,			IDS_ERROR_SELECT		},
			{ Smtp::SMTP_ERROR_DISCONNECT,		IDS_ERROR_DISCONNECT	},
			{ Smtp::SMTP_ERROR_RECEIVE,			IDS_ERROR_RECEIVE		},
			{ Smtp::SMTP_ERROR_SEND,			IDS_ERROR_SEND			},
			{ Smtp::SMTP_ERROR_OTHER,			IDS_ERROR_OTHER			},
			{ Smtp::SMTP_ERROR_SSL,				IDS_ERROR_SSL			}
		},
		{
			{ Socket::SOCKET_ERROR_SOCKET,			IDS_ERROR_SOCKET_SOCKET			},
			{ Socket::SOCKET_ERROR_CLOSESOCKET,		IDS_ERROR_SOCKET_CLOSESOCKET	},
			{ Socket::SOCKET_ERROR_LOOKUPNAME,		IDS_ERROR_SOCKET_LOOKUPNAME		},
			{ Socket::SOCKET_ERROR_CONNECT,			IDS_ERROR_SOCKET_CONNECT		},
			{ Socket::SOCKET_ERROR_CONNECTTIMEOUT,	IDS_ERROR_SOCKET_CONNECTTIMEOUT	},
			{ Socket::SOCKET_ERROR_RECV,			IDS_ERROR_SOCKET_RECV			},
			{ Socket::SOCKET_ERROR_RECVTIMEOUT,		IDS_ERROR_SOCKET_RECVTIMEOUT	},
			{ Socket::SOCKET_ERROR_SEND,			IDS_ERROR_SOCKET_SEND			},
			{ Socket::SOCKET_ERROR_SENDTIMEOUT,		IDS_ERROR_SOCKET_SENDTIMEOUT	},
			{ Socket::SOCKET_ERROR_CANCEL,			IDS_ERROR_SOCKET_CANCEL			},
			{ Socket::SOCKET_ERROR_UNKNOWN,			IDS_ERROR_SOCKET_UNKNOWN		}
		}
	};
	
	unsigned int nError = pSmtp_->getLastError();
	unsigned int nMasks[] = {
		Smtp::SMTP_ERROR_MASK_HIGHLEVEL,
		Smtp::SMTP_ERROR_MASK_LOWLEVEL,
		Socket::SOCKET_ERROR_MASK_SOCKET
	};
	string_ptr<WSTRING> wstrDescriptions[countof(maps)];
	for (int n = 0; n < countof(maps); ++n) {
		for (int m = 0; m < countof(maps[n]) && !wstrDescriptions[n].get(); ++m) {
			if (maps[n][m].nError_ != 0 &&
				(nError & nMasks[n]) == maps[n][m].nError_) {
				status = loadString(getResourceHandle(),
					maps[n][m].nId_, &wstrDescriptions[n]);
				CHECK_QSTATUS();
			}
		}
	}
	
	string_ptr<WSTRING> wstrMessage;
	status = loadString(getResourceHandle(), IDS_ERROR_MESSAGE, &wstrMessage);
	CHECK_QSTATUS();
	
	const WCHAR* pwszDescription[] = {
		wstrDescriptions[0].get(),
		wstrDescriptions[1].get(),
		wstrDescriptions[2].get(),
		pSmtp_->getLastErrorResponse()
	};
	SessionErrorInfo info(pAccount_, pSubAccount_, 0, wstrMessage.get(),
		nError, pwszDescription, countof(pwszDescription));
	status = pSessionCallback_->addError(info);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * SmtpSendSession::CallbackImpl
 *
 */

qmsmtp::SmtpSendSession::CallbackImpl::CallbackImpl(
	SubAccount* pSubAccount, const Security* pSecurity,
	SendSessionCallback* pSessionCallback, QSTATUS* pstatus) :
	DefaultSSLSocketCallback(pSubAccount, Account::HOST_SEND, pSecurity),
	pSubAccount_(pSubAccount),
	pSessionCallback_(pSessionCallback)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qmsmtp::SmtpSendSession::CallbackImpl::~CallbackImpl()
{
}

QSTATUS qmsmtp::SmtpSendSession::CallbackImpl::setMessage(UINT nId)
{
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrMessage;
	status = loadString(getResourceHandle(), nId, &wstrMessage);
	CHECK_QSTATUS();
	
	return pSessionCallback_->setMessage(wstrMessage.get());
}

bool qmsmtp::SmtpSendSession::CallbackImpl::isCanceled(bool bForce) const
{
	return pSessionCallback_->isCanceled(bForce);
}

QSTATUS qmsmtp::SmtpSendSession::CallbackImpl::initialize()
{
	return setMessage(IDS_INITIALIZE);
}

QSTATUS qmsmtp::SmtpSendSession::CallbackImpl::lookup()
{
	return setMessage(IDS_LOOKUP);
}

QSTATUS qmsmtp::SmtpSendSession::CallbackImpl::connecting()
{
	return setMessage(IDS_CONNECTING);
}

QSTATUS qmsmtp::SmtpSendSession::CallbackImpl::connected()
{
	return setMessage(IDS_CONNECTED);
}

QSTATUS qmsmtp::SmtpSendSession::CallbackImpl::getUserInfo(
	WSTRING* pwstrUserName, WSTRING* pwstrPassword)
{
	assert(pwstrUserName);
	assert(pwstrPassword);
	
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrUserName(
		allocWString(pSubAccount_->getUserName(Account::HOST_SEND)));
	if (!wstrUserName.get())
		return QSTATUS_OUTOFMEMORY;
	string_ptr<WSTRING> wstrPassword(
		allocWString(pSubAccount_->getPassword(Account::HOST_SEND)));
	if (!wstrPassword.get())
		return QSTATUS_OUTOFMEMORY;
	
	*pwstrUserName = wstrUserName.release();
	*pwstrPassword = wstrPassword.release();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmsmtp::SmtpSendSession::CallbackImpl::setPassword(
	const WCHAR* pwszPassword)
{
	// TODO
	return QSTATUS_SUCCESS;
}

QSTATUS qmsmtp::SmtpSendSession::CallbackImpl::getLocalHost(
	WSTRING* pwstrLocalHost)
{
	assert(pwstrLocalHost);
	return pSubAccount_->getProperty(L"Smtp",
		L"LocalHost", L"", pwstrLocalHost);
}

QSTATUS qmsmtp::SmtpSendSession::CallbackImpl::getAuthMethods(
	WSTRING* pwstrAuthMethods)
{
	assert(pwstrAuthMethods);
	return pSubAccount_->getProperty(L"Smtp",
		L"AuthMethods", L"", pwstrAuthMethods);
}

QSTATUS qmsmtp::SmtpSendSession::CallbackImpl::authenticating()
{
	return setMessage(IDS_AUTHENTICATING);
}

QSTATUS qmsmtp::SmtpSendSession::CallbackImpl::setRange(
	unsigned int nMin, unsigned int nMax)
{
	return pSessionCallback_->setSubRange(nMin, nMax);
}

QSTATUS qmsmtp::SmtpSendSession::CallbackImpl::setPos(unsigned int nPos)
{
	return pSessionCallback_->setSubPos(nPos);
}


/****************************************************************************
 *
 * SmtpSendSessionUI
 *
 */

qmsmtp::SmtpSendSessionUI::SmtpSendSessionUI(QSTATUS* pstatus)
{
}

qmsmtp::SmtpSendSessionUI::~SmtpSendSessionUI()
{
}

const WCHAR* qmsmtp::SmtpSendSessionUI::getClass()
{
	return L"mail";
}

QSTATUS qmsmtp::SmtpSendSessionUI::getDisplayName(WSTRING* pwstrName)
{
	assert(pwstrName);
	return loadString(getResourceHandle(), IDS_SMTP, pwstrName);
}

short qmsmtp::SmtpSendSessionUI::getDefaultPort()
{
	return 25;
}

QSTATUS qmsmtp::SmtpSendSessionUI::createPropertyPage(
	SubAccount* pSubAccount, PropertyPage** ppPage)
{
	assert(ppPage);
	
	DECLARE_QSTATUS();
	
	*ppPage = 0;
	
	std::auto_ptr<SendPage> pPage;
	status = newQsObject(pSubAccount, &pPage);
	CHECK_QSTATUS();
	
	*ppPage = pPage.release();
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * SmtpSendSessionFactory
 *
 */

SmtpSendSessionFactory qmsmtp::SmtpSendSessionFactory::factory__;

qmsmtp::SmtpSendSessionFactory::SmtpSendSessionFactory()
{
	regist(L"smtp", this);
}

qmsmtp::SmtpSendSessionFactory::~SmtpSendSessionFactory()
{
	unregist(L"smtp");
}

QSTATUS qmsmtp::SmtpSendSessionFactory::createSession(
	SendSession** ppSendSession)
{
	assert(ppSendSession);
	
	DECLARE_QSTATUS();
	
	SmtpSendSession* pSession = 0;
	status = newQsObject(&pSession);
	CHECK_QSTATUS();
	
	*ppSendSession = pSession;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmsmtp::SmtpSendSessionFactory::createUI(SendSessionUI** ppUI)
{
	assert(ppUI);
	
	DECLARE_QSTATUS();
	
	SmtpSendSessionUI* pUI = 0;
	status = newQsObject(&pUI);
	CHECK_QSTATUS();
	
	*ppUI = pUI;
	
	return QSTATUS_SUCCESS;
}
