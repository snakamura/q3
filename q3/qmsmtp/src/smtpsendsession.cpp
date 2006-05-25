/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
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
#include <qsthread.h>

#include <algorithm>

#include "main.h"
#include "resourceinc.h"
#include "smtpsendsession.h"
#include "ui.h"

using namespace qmsmtp;
using namespace qm;
using namespace qs;

#define HANDLE_ERROR() \
	do { \
		reportError(); \
		return false; \
	} while (false) \


/****************************************************************************
 *
 * SmtpSendSession
 *
 */

qmsmtp::SmtpSendSession::SmtpSendSession() :
	pAccount_(0),
	pSubAccount_(0),
	pLogger_(0),
	pSessionCallback_(0)
{
}

qmsmtp::SmtpSendSession::~SmtpSendSession()
{
}

bool qmsmtp::SmtpSendSession::init(Document* pDocument,
								   Account* pAccount,
								   SubAccount* pSubAccount,
								   Profile* pProfile,
								   Logger* pLogger,
								   SendSessionCallback* pCallback)
{
	assert(pAccount);
	assert(pSubAccount);
	assert(pCallback);
	
	pAccount_ = pAccount;
	pSubAccount_ = pSubAccount;
	pLogger_ = pLogger;
	pSessionCallback_ = pCallback;
	
	pCallback_.reset(new CallbackImpl(pSubAccount_,
		pDocument->getSecurity(), pSessionCallback_));
	
	return true;
}

void qmsmtp::SmtpSendSession::term()
{
}

bool qmsmtp::SmtpSendSession::connect()
{
	assert(!pSmtp_.get());
	
	Log log(pLogger_, L"qmsmtp::SmtpSendSession");
	log.debug(L"Connecting to the server...");
	
	pSmtp_.reset(new Smtp(pSubAccount_->getTimeout(), pCallback_.get(),
		pCallback_.get(), pCallback_.get(), pLogger_));
	
	Smtp::Secure secure = Smtp::SECURE_NONE;
	switch (pSubAccount_->getSecure(Account::HOST_SEND)) {
	case SubAccount::SECURE_SSL:
		secure = Smtp::SECURE_SSL;
		break;
	case SubAccount::SECURE_STARTTLS:
		secure = Smtp::SECURE_STARTTLS;
		break;
	}
	
	if (!pSmtp_->connect(pSubAccount_->getHost(Account::HOST_SEND),
		pSubAccount_->getPort(Account::HOST_SEND), secure))
		HANDLE_ERROR();
	
	log.debug(L"Connected to the server.");
	
	return true;
}

void qmsmtp::SmtpSendSession::disconnect()
{
	assert(pSmtp_.get());
	
	Log log(pLogger_, L"qmsmtp::SmtpSendSession");
	log.debug(L"Disconnecting from the server...");
	
	pSmtp_->disconnect();
	
	log.debug(L"Disconnected from the server.");
}

bool qmsmtp::SmtpSendSession::sendMessage(Message* pMessage)
{
	assert(pMessage);
	
	Log log(pLogger_, L"qmsmtp::SmtpSendSession");
	log.debug(L"Sending message...");
	
	pCallback_->setMessage(IDS_SENDMESSAGE);
	
	PartUtil util(*pMessage);
	bool bResent = util.isResent();
	
	wstring_ptr wstrEnvelopeFrom = pSubAccount_->getPropertyString(L"Smtp", L"EnvelopeFrom");
	if (!*wstrEnvelopeFrom.get()) {
		wstrEnvelopeFrom.reset(0);
		
		AddressParser envelopeFrom;
		if (pMessage->getField(L"X-QMAIL-EnvelopeFrom", &envelopeFrom) == Part::FIELD_EXIST) {
			wstrEnvelopeFrom = envelopeFrom.getAddress();
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
				AddressListParser address(AddressListParser::FLAG_DISALLOWGROUP);
				if (pMessage->getField(*(ppwszFields + n), &address) == Part::FIELD_EXIST) {
					const AddressListParser::AddressList& l = address.getAddressList();
					if (!l.empty())
						wstrEnvelopeFrom = l.front()->getAddress();
				}
			}
		}
	}
	if (!wstrEnvelopeFrom.get())
		return false;
	
	std::vector<STRING> vecAddress;
	struct Deleter
	{
		Deleter(std::vector<STRING>& v) :
			v_(v)
		{
		}
		
		~Deleter()
		{
			std::for_each(v_.begin(), v_.end(), string_free<STRING>());
		}
		
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
		AddressListParser address;
		if (pMessage->getField(*(ppwszFields + n), &address) == Part::FIELD_EXIST) {
			bool bReplace = false;
			
			const AddressListParser::AddressList& l = address.getAddressList();
			for (AddressListParser::AddressList::const_iterator it = l.begin(); it != l.end(); ++it) {
				AddressParser* pAddress = *it;
				AddressListParser* pGroup = pAddress->getGroup();
				if (pGroup) {
					const AddressListParser::AddressList& groups = pGroup->getAddressList();
					for (AddressListParser::AddressList::const_iterator itG = groups.begin(); itG != groups.end(); ++itG) {
						wstring_ptr wstrAddress((*itG)->getAddress());
						string_ptr strAddress(wcs2mbs(wstrAddress.get()));
						vecAddress.push_back(strAddress.get());
						strAddress.release();
					}
					pGroup->removeAllAddresses();
					bReplace = true;
				}
				else {
					wstring_ptr wstrAddress(pAddress->getAddress());
					string_ptr strAddress(wcs2mbs(wstrAddress.get()));
					vecAddress.push_back(strAddress.get());
					strAddress.release();
				}
			}
			
			if (bReplace) {
				if (!pMessage->replaceField(*(ppwszFields + n), address))
					return false;
			}
		}
	}
	
	const WCHAR* pwszRemoveFields[] = {
		L"Bcc",
		L"Resent-Bcc",
		L"X-QMAIL-EnvelopeFrom"
	};
	for (int m = 0; m < countof(pwszRemoveFields); ++m)
		pMessage->removeField(*(pwszRemoveFields + m));
	
	xstring_size_ptr strContent(pMessage->getContent());
	
	string_ptr strEnvelopeFrom(wcs2mbs(wstrEnvelopeFrom.get()));
	Smtp::SendMessageData data = {
		strEnvelopeFrom.get(),
		const_cast<const CHAR**>(&vecAddress[0]),
		vecAddress.size(),
		strContent.get(),
		strContent.size()
	};
	if (!pSmtp_->sendMessage(data))
		HANDLE_ERROR();
	
	return true;
}

void qmsmtp::SmtpSendSession::reportError()
{
	assert(pSmtp_.get());
	
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
			{ Smtp::SMTP_ERROR_STARTTLS,	IDS_ERROR_STARTTLS	},
			{ Smtp::SMTP_ERROR_RSET,		IDS_ERROR_RSET		}
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
		}
	};
	
	unsigned int nError = pSmtp_->getLastError();
	unsigned int nMasks[] = {
		Smtp::SMTP_ERROR_MASK_HIGHLEVEL,
		Smtp::SMTP_ERROR_MASK_LOWLEVEL,
	};
	wstring_ptr wstrDescriptions[countof(maps)];
	for (int n = 0; n < countof(maps); ++n) {
		for (int m = 0; m < countof(maps[n]) && !wstrDescriptions[n].get(); ++m) {
			if (maps[n][m].nError_ != 0 && (nError & nMasks[n]) == maps[n][m].nError_)
				wstrDescriptions[n] = loadString(getResourceHandle(), maps[n][m].nId_);
		}
	}
	
	wstring_ptr wstrMessage(loadString(getResourceHandle(), IDS_ERROR_MESSAGE));
	wstring_ptr wstrSocketDescription(SocketBase::getErrorDescription(
		static_cast<SocketBase::Error>(nError & SocketBase::SOCKET_ERROR_MASK_SOCKET)));
	
	const WCHAR* pwszDescription[] = {
		wstrDescriptions[0].get(),
		wstrDescriptions[1].get(),
		wstrSocketDescription.get(),
		pSmtp_->getLastErrorResponse()
	};
	SessionErrorInfo info(pAccount_, pSubAccount_, 0, wstrMessage.get(),
		nError, pwszDescription, countof(pwszDescription));
	pSessionCallback_->addError(info);
}


/****************************************************************************
 *
 * SmtpSendSession::CallbackImpl
 *
 */

qmsmtp::SmtpSendSession::CallbackImpl::CallbackImpl(SubAccount* pSubAccount,
													const Security* pSecurity,
													SendSessionCallback* pSessionCallback) :
	DefaultSSLSocketCallback(pSubAccount, Account::HOST_SEND, pSecurity),
	pSubAccount_(pSubAccount),
	pSessionCallback_(pSessionCallback),
	state_(PASSWORDSTATE_ONETIME)
{
}

qmsmtp::SmtpSendSession::CallbackImpl::~CallbackImpl()
{
}

void qmsmtp::SmtpSendSession::CallbackImpl::setMessage(UINT nId)
{
	wstring_ptr wstrMessage(loadString(getResourceHandle(), nId));
	pSessionCallback_->setMessage(wstrMessage.get());
}

bool qmsmtp::SmtpSendSession::CallbackImpl::isCanceled(bool bForce) const
{
	return pSessionCallback_->isCanceled(bForce);
}

void qmsmtp::SmtpSendSession::CallbackImpl::initialize()
{
	setMessage(IDS_INITIALIZE);
}

void qmsmtp::SmtpSendSession::CallbackImpl::lookup()
{
	setMessage(IDS_LOOKUP);
}

void qmsmtp::SmtpSendSession::CallbackImpl::connecting()
{
	setMessage(IDS_CONNECTING);
}

void qmsmtp::SmtpSendSession::CallbackImpl::connected()
{
	setMessage(IDS_CONNECTED);
}

bool qmsmtp::SmtpSendSession::CallbackImpl::getUserInfo(wstring_ptr* pwstrUserName,
														wstring_ptr* pwstrPassword)
{
	assert(pwstrUserName);
	assert(pwstrPassword);
	
	const WCHAR* pwszUserName = pSubAccount_->getUserName(Account::HOST_SEND);
	if (!pwszUserName || !*pwszUserName)
		return true;
	*pwstrUserName = allocWString(pwszUserName);
	
	state_ = pSessionCallback_->getPassword(
		pSubAccount_, Account::HOST_SEND, pwstrPassword);
	
	return state_ != PASSWORDSTATE_NONE;
}

void qmsmtp::SmtpSendSession::CallbackImpl::setPassword(const WCHAR* pwszPassword)
{
	if (state_ == PASSWORDSTATE_SESSION || state_ == PASSWORDSTATE_SAVE)
		pSessionCallback_->setPassword(pSubAccount_, Account::HOST_SEND,
			pwszPassword, state_ == PASSWORDSTATE_SAVE);
}

wstring_ptr qmsmtp::SmtpSendSession::CallbackImpl::getLocalHost()
{
	return pSubAccount_->getPropertyString(L"Smtp", L"LocalHost");
}

wstring_ptr qmsmtp::SmtpSendSession::CallbackImpl::getAuthMethods()
{
	return pSubAccount_->getPropertyString(L"Smtp", L"AuthMethods");
}

void qmsmtp::SmtpSendSession::CallbackImpl::authenticating()
{
	setMessage(IDS_AUTHENTICATING);
}

void qmsmtp::SmtpSendSession::CallbackImpl::setRange(size_t nMin,
													 size_t nMax)
{
	pSessionCallback_->setSubRange(nMin, nMax);
}

void qmsmtp::SmtpSendSession::CallbackImpl::setPos(size_t nPos)
{
	pSessionCallback_->setSubPos(nPos);
}


/****************************************************************************
 *
 * SmtpSendSessionUI
 *
 */

qmsmtp::SmtpSendSessionUI::SmtpSendSessionUI()
{
}

qmsmtp::SmtpSendSessionUI::~SmtpSendSessionUI()
{
}

const WCHAR* qmsmtp::SmtpSendSessionUI::getClass()
{
	return L"mail";
}

wstring_ptr qmsmtp::SmtpSendSessionUI::getDisplayName()
{
	return loadString(getResourceHandle(), IDS_SMTP);
}

short qmsmtp::SmtpSendSessionUI::getDefaultPort(bool bSecure)
{
	return bSecure ? 465 : 25;
}

bool qmsmtp::SmtpSendSessionUI::isSupported(Support support)
{
	return true;
}

std::auto_ptr<PropertyPage> qmsmtp::SmtpSendSessionUI::createPropertyPage(SubAccount* pSubAccount)
{
	return std::auto_ptr<PropertyPage>(new SendPage(pSubAccount));
}


/****************************************************************************
 *
 * SmtpSendSessionFactory
 *
 */

SmtpSendSessionFactory qmsmtp::SmtpSendSessionFactory::factory__;

qmsmtp::SmtpSendSessionFactory::SmtpSendSessionFactory()
{
	registerFactory(L"smtp", this);
}

qmsmtp::SmtpSendSessionFactory::~SmtpSendSessionFactory()
{
	unregisterFactory(L"smtp");
}

std::auto_ptr<SendSession> qmsmtp::SmtpSendSessionFactory::createSession()
{
	return std::auto_ptr<SendSession>(new SmtpSendSession());
}

std::auto_ptr<SendSessionUI> qmsmtp::SmtpSendSessionFactory::createUI()
{
	return std::auto_ptr<SendSessionUI>(new SmtpSendSessionUI());
}
