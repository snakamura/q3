/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmaccount.h>
#include <qmdocument.h>
#include <qmfilenames.h>
#include <qmmessage.h>
#include <qmsecurity.h>

#include <qsmime.h>
#include <qsprofile.h>
#include <qsstl.h>
#include <qstextutil.h>

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#pragma warning(disable:4786)

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * SubAccountImpl
 *
 */

struct qm::SubAccountImpl
{
	typedef std::vector<WSTRING> AddressList;
	
	void load();
	
	SubAccount* pThis_;
	Account* pAccount_;
	wstring_ptr wstrName_;
	wstring_ptr wstrIdentity_;
	wstring_ptr wstrSenderName_;
	wstring_ptr wstrSenderAddress_;
	wstring_ptr wstrHost_[Account::HOST_SIZE];
	short nPort_[Account::HOST_SIZE];
	wstring_ptr wstrUserName_[Account::HOST_SIZE];
	wstring_ptr wstrPassword_[Account::HOST_SIZE];
	SubAccount::Secure secure_[Account::HOST_SIZE];
	bool bLog_[Account::HOST_SIZE];
	long nTimeout_;
	bool bConnectReceiveBeforeSend_;
	bool bTreatAsSent_;
	bool bAddMessageId_;
	unsigned int nSslOption_;
	SubAccount::DialupType dialupType_;
	wstring_ptr wstrDialupEntry_;
	bool bDialupShowDialog_;
	unsigned int nDialupDisconnectWait_;
	std::auto_ptr<PrivateKey> pPrivateKey_;
	std::auto_ptr<Certificate> pCertificate_;
	AddressList listMyAddress_;
	std::auto_ptr<Profile> pProfile_;
	wstring_ptr wstrSyncFilterName_;
};

void qm::SubAccountImpl::load()
{
#define LOAD_STRING(section, key, default, name) \
	this->name = pProfile_->getString(section, key, default);
	
	LOAD_STRING(L"Global",	L"Identity",		0,	wstrIdentity_						);
	LOAD_STRING(L"Global",	L"SenderName",		0,	wstrSenderName_						);
	LOAD_STRING(L"Global",	L"SenderAddress",	0,	wstrSenderAddress_					);
	LOAD_STRING(L"Send",	L"Host",			0,	wstrHost_[Account::HOST_SEND]		);
	LOAD_STRING(L"Receive",	L"Host",			0,	wstrHost_[Account::HOST_RECEIVE]	);
	LOAD_STRING(L"Send",	L"UserName",		0,	wstrUserName_[Account::HOST_SEND]	);
	LOAD_STRING(L"Receive",	L"UserName",		0,	wstrUserName_[Account::HOST_RECEIVE]);
	LOAD_STRING(L"Send",	L"Password",		0,	wstrPassword_[Account::HOST_SEND]	);
	LOAD_STRING(L"Receive",	L"Password",		0,	wstrPassword_[Account::HOST_RECEIVE]);
	LOAD_STRING(L"Receive",	L"SyncFilterName",	0,	wstrSyncFilterName_					);
	LOAD_STRING(L"Dialup",	L"Entry",			0,	wstrDialupEntry_					);

#pragma warning(disable:4800)
#define LOAD_INT(section, key, default, name, type, tempname) \
	int _##tempname##_ = pProfile_->getInt(section, key, default); \
	name = static_cast<type>(_##tempname##_)
	
	LOAD_INT(L"Send",		L"Port",						25,		nPort_[Account::HOST_SEND],		short,					nSendPort					);
	LOAD_INT(L"Receive",	L"Port",						110,	nPort_[Account::HOST_RECEIVE],	short,					nReceivePort				);
	LOAD_INT(L"Send",		L"Secure",						0,		secure_[Account::HOST_SEND],	SubAccount::Secure,		nSendSecure					);
	LOAD_INT(L"Receive",	L"Secure",						0,		secure_[Account::HOST_RECEIVE],	SubAccount::Secure,		nReceiveSecure				);
	LOAD_INT(L"Send",		L"Log",							0,		bLog_[Account::HOST_SEND],		bool,					nSendLog					);
	LOAD_INT(L"Receive",	L"Log",							0,		bLog_[Account::HOST_RECEIVE],	bool,					nReceiveLog					);
	LOAD_INT(L"Global",		L"Timeout",						60,		nTimeout_,						long,					nTimeout					);
	LOAD_INT(L"Global",		L"ConnectReceiveBeforeSend",	0,		bConnectReceiveBeforeSend_,		bool,					nConnectReceiveBeforeSend	);
	LOAD_INT(L"Global",		L"TreatAsSent",					1,		bTreatAsSent_,					bool,					nTreatAsSent				);
	LOAD_INT(L"Global",		L"AddMessageId",				1,		bAddMessageId_,					bool,					nAddMessageId				);
	LOAD_INT(L"Global",		L"SslOption",					0,		nSslOption_,					unsigned int,			nSslOption					);
	LOAD_INT(L"Dialup",		L"Type",						0,		dialupType_,					SubAccount::DialupType,	dialupType					);
	LOAD_INT(L"Dialup",		L"ShowDialog",					0,		bDialupShowDialog_,				bool,					bDialupShowDialog			);
	LOAD_INT(L"Dialup",		L"DisconnectWait",				0,		nDialupDisconnectWait_,			unsigned int,			nDialupDisconnectWait		);
#pragma warning(default:4800)
	
	struct {
		Account::Host host_;
		const WCHAR* pwszSection_;
	} entries[] = {
		{ Account::HOST_SEND,		L"Send"		},
		{ Account::HOST_RECEIVE,	L"Receive"	}
	};
	for (int n = 0; n < countof(entries); ++n) {
		wstring_ptr& wstrPassword = wstrPassword_[entries[n].host_];
		if (!*wstrPassword.get()) {
			wstring_ptr wstrEncodedPassword(pProfile_->getString(
				entries[n].pwszSection_, L"EncodedPassword", 0));
			wstring_ptr wstr(TextUtil::decodePassword(wstrEncodedPassword.get()));
			wstrPassword = wstr;
		}
	}
	
	wstring_ptr wstrMyAddress(pProfile_->getString(L"Global", L"MyAddress", 0));
	pThis_->setMyAddress(wstrMyAddress.get());
	
	if (Security::isEnabled()) {
		{
			std::auto_ptr<PrivateKey> pPrivateKey(PrivateKey::getInstance());
			ConcatW c[] = {
				{ pAccount_->getPath(),				-1	},
				{ L"\\",							1	},
				{ FileNames::KEY,					-1	},
				{ *wstrName_.get() ? L"_" : L"",	-1	},
				{ wstrName_.get(),					-1	},
				{ FileNames::PEM_EXT,				-1	}
			};
			wstring_ptr wstrPath(concat(c, countof(c)));
			if (pPrivateKey->load(wstrPath.get(),
				PrivateKey::FILETYPE_PEM, 0))
				pPrivateKey_ = pPrivateKey;
		}
		
		{
			std::auto_ptr<Certificate> pCertificate(Certificate::getInstance());
			ConcatW c[] = {
				{ pAccount_->getPath(),				-1	},
				{ L"\\",							1	},
				{ FileNames::CERT,					-1	},
				{ *wstrName_.get() ? L"_" : L"",	-1	},
				{ wstrName_.get(),					-1	},
				{ FileNames::PEM_EXT,				-1	}
			};
			wstring_ptr wstrPath(concat(c, countof(c)));
			if (pCertificate->load(wstrPath.get(),
				Certificate::FILETYPE_PEM, 0))
				pCertificate_ = pCertificate;
		}
	}
}


/****************************************************************************
 *
 * SubAccount
 *
 */

qm::SubAccount::SubAccount(Account* pAccount,
						   std::auto_ptr<Profile> pProfile,
						   const WCHAR* pwszName)
{
	assert(pAccount);
	assert(pProfile.get());
	assert(pwszName);
	
	wstring_ptr wstrName(allocWString(pwszName));
	
	pImpl_ = new SubAccountImpl();
	pImpl_->pThis_ = this;
	pImpl_->pAccount_ = pAccount;
	pImpl_->wstrName_ = wstrName;
	pImpl_->nPort_[Account::HOST_SEND] = 0;
	pImpl_->nPort_[Account::HOST_RECEIVE] = 0;
	pImpl_->secure_[Account::HOST_SEND] = SECURE_NONE;
	pImpl_->secure_[Account::HOST_RECEIVE] = SECURE_NONE;
	pImpl_->bLog_[Account::HOST_SEND] = false;
	pImpl_->bLog_[Account::HOST_RECEIVE] = false;
	pImpl_->nTimeout_ = 60;
	pImpl_->bConnectReceiveBeforeSend_ = false;
	pImpl_->bTreatAsSent_ = true;
	pImpl_->bAddMessageId_ = true;
	pImpl_->nSslOption_ = 0;
	pImpl_->dialupType_ = SubAccount::DIALUPTYPE_NEVER;
	pImpl_->bDialupShowDialog_ = false;
	pImpl_->nDialupDisconnectWait_ = 0;
	pImpl_->pProfile_ = pProfile;
	
	pImpl_->load();
}

qm::SubAccount::~SubAccount()
{
	if (pImpl_) {
		std::for_each(pImpl_->listMyAddress_.begin(),
			pImpl_->listMyAddress_.end(), string_free<WSTRING>());
		delete pImpl_;
		pImpl_ = 0;
	}
}

Account* qm::SubAccount::getAccount() const
{
	return pImpl_->pAccount_;
}

const WCHAR* qm::SubAccount::getName() const
{
	return pImpl_->wstrName_.get();
}

const WCHAR* qm::SubAccount::getIdentity() const
{
	return pImpl_->wstrIdentity_.get();
}

void qm::SubAccount::setIdentity(const WCHAR* pwszIdentity)
{
	pImpl_->wstrIdentity_ = allocWString(pwszIdentity);
}

const WCHAR* qm::SubAccount::getSenderName() const
{
	return pImpl_->wstrSenderName_.get();
}

void qm::SubAccount::setSenderName(const WCHAR* pwszName)
{
	pImpl_->wstrSenderName_ = allocWString(pwszName);
}

const WCHAR* qm::SubAccount::getSenderAddress() const
{
	return pImpl_->wstrSenderAddress_.get();
}

void qm::SubAccount::setSenderAddress(const WCHAR* pwszAddress)
{
	pImpl_->wstrSenderAddress_ = allocWString(pwszAddress);
}

wstring_ptr qm::SubAccount::getMyAddress() const
{
	StringBuffer<WSTRING> buf;
	for (SubAccountImpl::AddressList::const_iterator it = pImpl_->listMyAddress_.begin(); it != pImpl_->listMyAddress_.end(); ++it) {
		if (buf.getLength() != 0)
			buf.append(L", ");
		buf.append(*it);
	}
	return buf.getString();
}

void qm::SubAccount::setMyAddress(const WCHAR* pwszAddress)
{
	std::for_each(pImpl_->listMyAddress_.begin(),
		pImpl_->listMyAddress_.end(), string_free<WSTRING>());
	pImpl_->listMyAddress_.clear();
	
	const WCHAR* p = pwszAddress;
	const WCHAR* pEnd = wcschr(p, L',');
	while (true) {
		size_t nLen = pEnd ? pEnd - p : static_cast<size_t>(-1);
		wstring_ptr wstr(trim(p, nLen));
		if (wcslen(wstr.get()) != 0) {
			pImpl_->listMyAddress_.push_back(wstr.get());
			wstr.release();
		}
		
		if (!pEnd)
			break;
		
		p = pEnd + 1;
		pEnd = wcschr(p, L',');
	}
}

bool qm::SubAccount::isMyAddress(const WCHAR* pwszMailbox,
								 const WCHAR* pwszHost) const
{
	typedef SubAccountImpl::AddressList List;
	const List& l = pImpl_->listMyAddress_;
	for (List::const_iterator it = l.begin(); it != l.end(); ++it) {
		const WCHAR* p = wcsrchr(*it, L'@');
		if (p) {
			if (wcslen(pwszMailbox) == static_cast<size_t>(p - *it) &&
				_wcsnicmp(*it, pwszMailbox, p - *it) == 0 &&
				_wcsicmp(p + 1, pwszHost) == 0)
				return true;
		}
		else {
			if (_wcsicmp(*it, pwszMailbox) == 0)
				return true;
		}
	}
	return false;
}

bool qm::SubAccount::isMyAddress(const AddressListParser& address) const
{
	typedef AddressListParser::AddressList List;
	const List& l = address.getAddressList();
	for (List::const_iterator it = l.begin(); it != l.end(); ++it) {
		const AddressParser* pAddress = *it;
		const AddressListParser* pGroup = pAddress->getGroup();
		if (pGroup) {
			const List& l = pGroup->getAddressList();
			for (List::const_iterator it = l.begin(); it != l.end(); ++it) {
				if (isMyAddress((*it)->getMailbox(), (*it)->getHost()))
					return true;
			}
		}
		else {
			if (isMyAddress(pAddress->getMailbox(), pAddress->getHost()))
				return true;
		}
	}
	return false;
}

const WCHAR* qm::SubAccount::getHost(Account::Host host) const
{
	return pImpl_->wstrHost_[host].get();
}

void qm::SubAccount::setHost(Account::Host host,
							 const WCHAR* pwszHost)
{
	pImpl_->wstrHost_[host] = allocWString(pwszHost);
}

short qm::SubAccount::getPort(Account::Host host) const
{
	return pImpl_->nPort_[host];
}

void qm::SubAccount::setPort(Account::Host host,
							 short nPort)
{
	pImpl_->nPort_[host] = nPort;
}

const WCHAR* qm::SubAccount::getUserName(Account::Host host) const
{
	return pImpl_->wstrUserName_[host].get();
}

void qm::SubAccount::setUserName(Account::Host host,
								 const WCHAR* pwszUserName)
{
	pImpl_->wstrUserName_[host] = allocWString(pwszUserName);
}

const WCHAR* qm::SubAccount::getPassword(Account::Host host) const
{
	return pImpl_->wstrPassword_[host].get();
}

void qm::SubAccount::setPassword(Account::Host host,
								 const WCHAR* pwszPassword)
{
	pImpl_->wstrPassword_[host] = allocWString(pwszPassword);
}

SubAccount::Secure qm::SubAccount::getSecure(Account::Host host) const
{
	return pImpl_->secure_[host];
}

void qm::SubAccount::setSecure(Account::Host host,
							   Secure secure)
{
	pImpl_->secure_[host] = secure;
}

bool qm::SubAccount::isLog(Account::Host host) const
{
	return pImpl_->bLog_[host];
}

void qm::SubAccount::setLog(Account::Host host,
							bool bLog)
{
	pImpl_->bLog_[host] = bLog;
}

long qm::SubAccount::getTimeout() const
{
	return pImpl_->nTimeout_;
}

void qm::SubAccount::setTimeout(long nTimeout)
{
	pImpl_->nTimeout_ = nTimeout;
}

bool qm::SubAccount::isConnectReceiveBeforeSend() const
{
	return pImpl_->bConnectReceiveBeforeSend_;
}

void qm::SubAccount::setConnectReceiveBeforeSend(bool bConnectReceiveBeforeSend)
{
	pImpl_->bConnectReceiveBeforeSend_ = bConnectReceiveBeforeSend;
}

bool qm::SubAccount::isTreatAsSent() const
{
	return pImpl_->bTreatAsSent_;
}

void qm::SubAccount::setTreatAsSent(bool bTreatAsSent)
{
	pImpl_->bTreatAsSent_ = bTreatAsSent;
}

bool qm::SubAccount::isAddMessageId() const
{
	return pImpl_->bAddMessageId_;
}

void qm::SubAccount::setAddMessageId(bool bAddMessageId)
{
	pImpl_->bAddMessageId_ = bAddMessageId;
}

unsigned int qm::SubAccount::getSslOption() const
{
	return pImpl_->nSslOption_;
}

void qm::SubAccount::setSslOption(unsigned int nSslOption)
{
	pImpl_->nSslOption_ = nSslOption;
}

SubAccount::DialupType qm::SubAccount::getDialupType() const
{
	return pImpl_->dialupType_;
}

void qm::SubAccount::setDialupType(DialupType type)
{
	pImpl_->dialupType_ = type;
}

const WCHAR* qm::SubAccount::getDialupEntry() const
{
	return pImpl_->wstrDialupEntry_.get();
}

void qm::SubAccount::setDialupEntry(const WCHAR* pwszEntry)
{
	pImpl_->wstrDialupEntry_ = allocWString(pwszEntry);
}

bool qm::SubAccount::isDialupShowDialog() const
{
	return pImpl_->bDialupShowDialog_;
}

void qm::SubAccount::setDialupShowDialog(bool bShow)
{
	pImpl_->bDialupShowDialog_ = bShow;
}

unsigned int qm::SubAccount::getDialupDisconnectWait() const
{
	return pImpl_->nDialupDisconnectWait_;
}

void qm::SubAccount::setDialupDisconnectWait(unsigned int nWait)
{
	pImpl_->nDialupDisconnectWait_ = nWait;
}

PrivateKey* qm::SubAccount::getPrivateKey() const
{
	return pImpl_->pPrivateKey_.get();
}

Certificate* qm::SubAccount::getCertificate() const
{
	return pImpl_->pCertificate_.get();
}

int qm::SubAccount::getProperty(const WCHAR* pwszSection,
								const WCHAR* pwszKey,
								int nDefault) const
{
	assert(pwszSection);
	assert(pwszKey);
	
	return pImpl_->pProfile_->getInt(pwszSection, pwszKey, nDefault);
}

void qm::SubAccount::setProperty(const WCHAR* pwszSection,
								 const WCHAR* pwszKey,
								 int nValue)
{
	assert(pwszSection);
	assert(pwszKey);
	
	pImpl_->pProfile_->setInt(pwszSection, pwszKey, nValue);
}

wstring_ptr qm::SubAccount::getProperty(const WCHAR* pwszSection,
										const WCHAR* pwszKey,
										const WCHAR* pwszDefault) const
{
	assert(pwszSection);
	assert(pwszKey);
	
	if (wcscmp(pwszSection, L"Global") == 0) {
		if (wcscmp(pwszKey, L"Path") == 0)
			return allocWString(pImpl_->pAccount_->getPath());
	}
	
	return pImpl_->pProfile_->getString(pwszSection, pwszKey, pwszDefault);
}

void qm::SubAccount::setProperty(const WCHAR* pwszSection,
								 const WCHAR* pwszKey,
								 const WCHAR* pwszValue)
{
	assert(pwszSection);
	assert(pwszKey);
	assert(pwszValue);
	
	pImpl_->pProfile_->setString(pwszSection, pwszKey, pwszValue);
}

const WCHAR* qm::SubAccount::getSyncFilterName() const
{
	return pImpl_->wstrSyncFilterName_.get();
}

void qm::SubAccount::setSyncFilterName(const WCHAR* pwszName)
{
	pImpl_->wstrSyncFilterName_ = allocWString(pwszName);
}

bool qm::SubAccount::isSelf(const Message& msg) const
{
	bool bSelf = false;
	
	if (pImpl_->bTreatAsSent_) {
		AddressListParser from(AddressListParser::FLAG_DISALLOWGROUP);
		if (msg.getField(L"From", &from) == Part::FIELD_EXIST) {
			const AddressListParser::AddressList& listFrom = from.getAddressList();
			if (!listFrom.empty()) {
				AddressParser* pFrom = listFrom.front();
				if (isMyAddress(pFrom->getMailbox(), pFrom->getHost())) {
					AddressListParser sender(AddressListParser::FLAG_DISALLOWGROUP);
					if (msg.getField(L"Sender", &sender) == Part::FIELD_EXIST) {
						const AddressListParser::AddressList& listSender = sender.getAddressList();
						if (!listSender.empty()) {
							AddressParser* pSender = listSender.front();
							bSelf = wcsicmp(pFrom->getMailbox(), pSender->getMailbox()) == 0 &&
								wcsicmp(pFrom->getHost(), pSender->getHost()) == 0;
						}
					}
					else {
						bSelf = true;
					}
				}
			}
			if (bSelf) {
				const WCHAR* pwszFields[] = {
					L"Posted",
					L"X-ML-Name",
					L"Mailing-List"
				};
				for (int n = 0; n < countof(pwszFields) && bSelf; ++n) {
					if (msg.hasField(pwszFields[n]))
						bSelf = false;
				}
			}
		}
	}
	
	return bSelf;
}

bool qm::SubAccount::save() const
{
#define SAVE_STRING(section, key, name) \
	pImpl_->pProfile_->setString(section, key, pImpl_->name.get());
	
	SAVE_STRING(L"Global",	L"Identity",		wstrIdentity_						);
	SAVE_STRING(L"Global",	L"SenderName",		wstrSenderName_						);
	SAVE_STRING(L"Global",	L"SenderAddress",	wstrSenderAddress_					);
	SAVE_STRING(L"Send",	L"Host",			wstrHost_[Account::HOST_SEND]		);
	SAVE_STRING(L"Receive",	L"Host",			wstrHost_[Account::HOST_RECEIVE]	);
	SAVE_STRING(L"Send",	L"UserName",		wstrUserName_[Account::HOST_SEND]	);
	SAVE_STRING(L"Receive",	L"UserName",		wstrUserName_[Account::HOST_RECEIVE]);
	SAVE_STRING(L"Receive",	L"SyncFilterName",	wstrSyncFilterName_					);
	SAVE_STRING(L"Dialup",	L"Entry",			wstrDialupEntry_					);
	
#define SAVE_INT(section, key, name) \
	pImpl_->pProfile_->setInt(section, key, pImpl_->name);
	
	SAVE_INT(L"Send",		L"Port",						nPort_[Account::HOST_SEND]		);
	SAVE_INT(L"Receive",	L"Port",						nPort_[Account::HOST_RECEIVE]	);
	SAVE_INT(L"Global",		L"Timeout",						nTimeout_						);
	SAVE_INT(L"Send",		L"Secure",						secure_[Account::HOST_SEND]		);
	SAVE_INT(L"Receive",	L"Secure",						secure_[Account::HOST_RECEIVE]	);
	SAVE_INT(L"Send",		L"Log",							bLog_[Account::HOST_SEND]		);
	SAVE_INT(L"Receive",	L"Log",							bLog_[Account::HOST_RECEIVE]	);
	SAVE_INT(L"Global",		L"ConnectReceiveBeforeSend",	bConnectReceiveBeforeSend_		);
	SAVE_INT(L"Global",		L"TreatAsSent",					bTreatAsSent_					);
	SAVE_INT(L"Global",		L"AddMessageId",				bAddMessageId_					);
	SAVE_INT(L"Global",		L"SslOption",					nSslOption_						);
	SAVE_INT(L"Dialup",		L"Type",						dialupType_						);
	SAVE_INT(L"Dialup",		L"ShowDialog",					bDialupShowDialog_				);
	SAVE_INT(L"Dialup",		L"DisconnectWait",				nDialupDisconnectWait_			);
	
	struct {
		Account::Host host_;
		const WCHAR* pwszSection_;
	} entries[] = {
		{ Account::HOST_SEND,		L"Send"		},
		{ Account::HOST_RECEIVE,	L"Receive"	}
	};
	for (int n = 0; n < countof(entries); ++n) {
		wstring_ptr wstrEncodedPassword(TextUtil::encodePassword(
			pImpl_->wstrPassword_[entries[n].host_].get()));
		pImpl_->pProfile_->setString(entries[n].pwszSection_,
			L"EncodedPassword", wstrEncodedPassword.get());
		pImpl_->pProfile_->setString(entries[n].pwszSection_, L"Password", L"");
	}
	
	wstring_ptr wstrMyAddress(getMyAddress());
	pImpl_->pProfile_->setString(L"Global", L"MyAddress", wstrMyAddress.get());
	
	return pImpl_->pProfile_->save();
}

bool qm::SubAccount::setName(const WCHAR* pwszName)
{
	wstring_ptr wstrName(allocWString(pwszName));
	
	ConcatW c[] = {
		{ pImpl_->pAccount_->getPath(),	-1	},
		{ L"\\",						1	},
		{ FileNames::ACCOUNT,			-1	},
		{ L"_",							1	},
		{ wstrName.get(),				-1	},
		{ FileNames::XML_EXT,			-1	}
	};
	wstring_ptr wstrPath(concat(c, countof(c)));
	
	if (!pImpl_->pProfile_->rename(wstrPath.get()))
		return false;
	
	pImpl_->wstrName_ = wstrName;
	
	return true;
}

bool qm::SubAccount::deletePermanent()
{
	return pImpl_->pProfile_->deletePermanent();
}
