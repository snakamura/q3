/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmaccount.h>
#include <qmdocument.h>
#include <qmfilenames.h>
#include <qmmessage.h>
#include <qmsecurity.h>

#include <qserror.h>
#include <qsmime.h>
#include <qsnew.h>
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
	
	QSTATUS load();
	
	SubAccount* pThis_;
	Account* pAccount_;
	WSTRING wstrName_;
	WSTRING wstrIdentity_;
	WSTRING wstrSenderName_;
	WSTRING wstrSenderAddress_;
	WSTRING wstrHost_[Account::HOST_SIZE];
	short nPort_[Account::HOST_SIZE];
	WSTRING wstrUserName_[Account::HOST_SIZE];
	WSTRING wstrPassword_[Account::HOST_SIZE];
	bool bSsl_[Account::HOST_SIZE];
	bool bLog_[Account::HOST_SIZE];
	long nTimeout_;
	bool bConnectReceiveBeforeSend_;
	bool bTreatAsSent_;
	bool bAddMessageId_;
	bool bAllowUnverifiedCertificate_;
	SubAccount::DialupType dialupType_;
	WSTRING wstrDialupEntry_;
	bool bDialupShowDialog_;
	unsigned int nDialupDisconnectWait_;
	PrivateKey* pPrivateKey_;
	Certificate* pCertificate_;
	AddressList listMyAddress_;
	Profile* pProfile_;
	WSTRING wstrSyncFilterName_;
};

QSTATUS qm::SubAccountImpl::load()
{
	DECLARE_QSTATUS();
	
#define LOAD_STRING(section, key, default, name) \
	status = pProfile_->getString(section, key, default, &##name); \
	CHECK_QSTATUS()
	
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
	int _##tempname##_ = 0; \
	status = pProfile_->getInt(section, key, default, &_##tempname##_); \
	CHECK_QSTATUS(); \
	##name = static_cast<type>(_##tempname##_)
	
	LOAD_INT(L"Send",		L"Port",						25,		nPort_[Account::HOST_SEND],		short,					nSendPort					);
	LOAD_INT(L"Receive",	L"Port",						110,	nPort_[Account::HOST_RECEIVE],	short,					nReceivePort				);
	LOAD_INT(L"Send",		L"Ssl",							0,		bSsl_[Account::HOST_SEND],		bool,					nSendSsl					);
	LOAD_INT(L"Receive",	L"Ssl",							0,		bSsl_[Account::HOST_RECEIVE],	bool,					nReceiveSsl					);
	LOAD_INT(L"Send",		L"Log",							0,		bLog_[Account::HOST_SEND],		bool,					nSendLog					);
	LOAD_INT(L"Receive",	L"Log",							0,		bLog_[Account::HOST_RECEIVE],	bool,					nReceiveLog					);
	LOAD_INT(L"Global",		L"Timeout",						60,		nTimeout_,						long,					nTimeout					);
	LOAD_INT(L"Global",		L"ConnectReceiveBeforeSend",	0,		bConnectReceiveBeforeSend_,		bool,					nConnectReceiveBeforeSend	);
	LOAD_INT(L"Global",		L"TreatAsSent",					1,		bTreatAsSent_,					bool,					nTreatAsSent				);
	LOAD_INT(L"Global",		L"AddMessageId",				1,		bAddMessageId_,					bool,					nAddMessageId				);
	LOAD_INT(L"Global",		L"AllowUnverifiedCertificate",	0,		bAllowUnverifiedCertificate_,	bool,					nAllowUnverifiedCertificate	);
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
		WSTRING& wstrPassword = wstrPassword_[entries[n].host_];
		if (!*wstrPassword) {
			string_ptr<WSTRING> wstrEncodedPassword;
			status = pProfile_->getString(entries[n].pwszSection_,
				L"EncodedPassword", 0, &wstrEncodedPassword);
			CHECK_QSTATUS();
			string_ptr<WSTRING> wstr;
			status = TextUtil::decodePassword(
				wstrEncodedPassword.get(), &wstr);
			CHECK_QSTATUS();
			freeWString(wstrPassword);
			wstrPassword = wstr.release();
		}
	}
	
	string_ptr<WSTRING> wstrMyAddress;
	status = pProfile_->getString(L"Global", L"MyAddress", 0, &wstrMyAddress);
	CHECK_QSTATUS();
	status = pThis_->setMyAddress(wstrMyAddress.get());
	CHECK_QSTATUS();
	
	if (Security::isEnabled()) {
		{
			std::auto_ptr<PrivateKey> pPrivateKey;
			status = CryptoUtil<PrivateKey>::getInstance(&pPrivateKey);
			CHECK_QSTATUS();
			ConcatW c[] = {
				{ pAccount_->getPath(),		-1	},
				{ L"\\",					1	},
				{ FileNames::KEY,			-1	},
				{ *wstrName_ ? L"_" : L"",	-1	},
				{ wstrName_,				-1	},
				{ FileNames::PEM_EXT,		-1	}
			};
			string_ptr<WSTRING> wstrPath(concat(c, countof(c)));
			if (!wstrPath.get())
				return QSTATUS_OUTOFMEMORY;
			status = pPrivateKey->load(wstrPath.get(),
				PrivateKey::FILETYPE_PEM, 0);
			if (status == QSTATUS_SUCCESS)
				pPrivateKey_ = pPrivateKey.release();
		}
		
		{
			std::auto_ptr<Certificate> pCertificate;
			status = CryptoUtil<Certificate>::getInstance(&pCertificate);
			CHECK_QSTATUS();
			ConcatW c[] = {
				{ pAccount_->getPath(),		-1	},
				{ L"\\",					1	},
				{ FileNames::CERT,			-1	},
				{ *wstrName_ ? L"_" : L"",	-1	},
				{ wstrName_,				-1	},
				{ FileNames::PEM_EXT,		-1	}
			};
			string_ptr<WSTRING> wstrPath(concat(c, countof(c)));
			if (!wstrPath.get())
				return QSTATUS_OUTOFMEMORY;
			status = pCertificate->load(wstrPath.get(),
				Certificate::FILETYPE_PEM, 0);
			if (status == QSTATUS_SUCCESS)
				pCertificate_ = pCertificate.release();
		}
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * SubAccount
 *
 */

qm::SubAccount::SubAccount(Account* pAccount, Profile* pProfile,
	const WCHAR* pwszName, QSTATUS* pstatus)
{
	assert(pAccount);
	assert(pProfile);
	assert(pwszName);
	assert(pstatus);
	
	*pstatus = QSTATUS_SUCCESS;
	
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrName(allocWString(pwszName));
	if (!wstrName.get()) {
		*pstatus = QSTATUS_OUTOFMEMORY;
		return;
	}
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	pImpl_->pThis_ = this;
	pImpl_->pAccount_ = pAccount;
	pImpl_->wstrName_ = wstrName.release();
	pImpl_->wstrIdentity_ = 0;
	pImpl_->wstrSenderName_ = 0;
	pImpl_->wstrSenderAddress_ = 0;
	pImpl_->wstrHost_[Account::HOST_SEND] = 0;
	pImpl_->wstrHost_[Account::HOST_RECEIVE] = 0;
	pImpl_->nPort_[Account::HOST_SEND] = 0;
	pImpl_->nPort_[Account::HOST_RECEIVE] = 0;
	pImpl_->wstrUserName_[Account::HOST_SEND] = 0;
	pImpl_->wstrUserName_[Account::HOST_RECEIVE] = 0;
	pImpl_->wstrPassword_[Account::HOST_SEND] = 0;
	pImpl_->wstrPassword_[Account::HOST_RECEIVE] = 0;
	pImpl_->bSsl_[Account::HOST_SEND] = false;
	pImpl_->bSsl_[Account::HOST_RECEIVE] = false;
	pImpl_->bLog_[Account::HOST_SEND] = false;
	pImpl_->bLog_[Account::HOST_RECEIVE] = false;
	pImpl_->nTimeout_ = 60;
	pImpl_->bConnectReceiveBeforeSend_ = false;
	pImpl_->bTreatAsSent_ = true;
	pImpl_->bAddMessageId_ = true;
	pImpl_->bAllowUnverifiedCertificate_ = false;
	pImpl_->dialupType_ = SubAccount::DIALUPTYPE_NEVER;
	pImpl_->wstrDialupEntry_ = 0;
	pImpl_->bDialupShowDialog_ = false;
	pImpl_->nDialupDisconnectWait_ = 0;
	pImpl_->pPrivateKey_ = 0;
	pImpl_->pCertificate_ = 0;
	pImpl_->pProfile_ = pProfile;
	pImpl_->wstrSyncFilterName_ = 0;
	
	status = pImpl_->load();
	CHECK_QSTATUS_SET(pstatus);
}

qm::SubAccount::~SubAccount()
{
	if (pImpl_) {
		WSTRING* pwstrs[] = {
			&pImpl_->wstrName_,
			&pImpl_->wstrIdentity_,
			&pImpl_->wstrSenderName_,
			&pImpl_->wstrSenderAddress_,
			&pImpl_->wstrHost_[Account::HOST_SEND],
			&pImpl_->wstrHost_[Account::HOST_RECEIVE],
			&pImpl_->wstrUserName_[Account::HOST_SEND],
			&pImpl_->wstrUserName_[Account::HOST_RECEIVE],
			&pImpl_->wstrPassword_[Account::HOST_SEND],
			&pImpl_->wstrPassword_[Account::HOST_RECEIVE],
			&pImpl_->wstrDialupEntry_
		};
		for (int n = 0; n < countof(pwstrs); ++n)
			freeWString(*pwstrs[n]);
		std::for_each(pImpl_->listMyAddress_.begin(),
			pImpl_->listMyAddress_.end(), string_free<WSTRING>());
		delete pImpl_->pProfile_;
		freeWString(pImpl_->wstrSyncFilterName_);
		
		delete pImpl_->pPrivateKey_;
		delete pImpl_->pCertificate_;
		
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
	return pImpl_->wstrName_;
}

const WCHAR* qm::SubAccount::getIdentity() const
{
	return pImpl_->wstrIdentity_;
}

QSTATUS qm::SubAccount::setIdentity(const WCHAR* pwszIdentity)
{
	string_ptr<WSTRING> wstrIdentity(allocWString(pwszIdentity));
	if (!wstrIdentity.get())
		return QSTATUS_OUTOFMEMORY;
	
	freeWString(pImpl_->wstrIdentity_);
	pImpl_->wstrIdentity_ = wstrIdentity.release();
	
	return QSTATUS_SUCCESS;
}

const WCHAR* qm::SubAccount::getSenderName() const
{
	return pImpl_->wstrSenderName_;
}

QSTATUS qm::SubAccount::setSenderName(const WCHAR* pwszName)
{
	string_ptr<WSTRING> wstrName(allocWString(pwszName));
	if (!wstrName.get())
		return QSTATUS_OUTOFMEMORY;
	
	freeWString(pImpl_->wstrSenderName_);
	pImpl_->wstrSenderName_ = wstrName.release();
	
	return QSTATUS_SUCCESS;
}

const WCHAR* qm::SubAccount::getSenderAddress() const
{
	return pImpl_->wstrSenderAddress_;
}

QSTATUS qm::SubAccount::setSenderAddress(const WCHAR* pwszAddress)
{
	string_ptr<WSTRING> wstrAddress(allocWString(pwszAddress));
	if (!wstrAddress.get())
		return QSTATUS_OUTOFMEMORY;
	
	freeWString(pImpl_->wstrSenderAddress_);
	pImpl_->wstrSenderAddress_ = wstrAddress.release();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::SubAccount::getMyAddress(WSTRING* pwstrAddress) const
{
	assert(pwstrAddress);
	
	DECLARE_QSTATUS();
	
	*pwstrAddress = 0;
	
	StringBuffer<WSTRING> buf(&status);
	CHECK_QSTATUS();
	SubAccountImpl::AddressList::const_iterator it = pImpl_->listMyAddress_.begin();
	while (it != pImpl_->listMyAddress_.end()) {
		if (buf.getLength() != 0) {
			status = buf.append(L", ");
			CHECK_QSTATUS();
		}
		status = buf.append(*it);
		CHECK_QSTATUS();
		++it;
	}
	
	*pwstrAddress = buf.getString();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::SubAccount::setMyAddress(const WCHAR* pwszAddress)
{
	DECLARE_QSTATUS();
	
	std::for_each(pImpl_->listMyAddress_.begin(),
		pImpl_->listMyAddress_.end(), string_free<WSTRING>());
	pImpl_->listMyAddress_.clear();
	
	STLWrapper<SubAccountImpl::AddressList> wrapper(pImpl_->listMyAddress_);
	const WCHAR* p = pwszAddress;
	const WCHAR* pEnd = wcschr(p, L',');
	while (true) {
		size_t nLen = pEnd ? pEnd - p : static_cast<size_t>(-1);
		string_ptr<WSTRING> wstr(trim(p, nLen));
		if (!wstr.get())
			return QSTATUS_OUTOFMEMORY;
		if (wcslen(wstr.get()) != 0) {
			status = wrapper.push_back(wstr.get());
			CHECK_QSTATUS();
			wstr.release();
		}
		
		if (!pEnd)
			break;
		
		p = pEnd + 1;
		pEnd = wcschr(p, L',');
	}
	
	return QSTATUS_SUCCESS;
}

bool qm::SubAccount::isMyAddress(const WCHAR* pwszMailbox, const WCHAR* pwszHost) const
{
	typedef SubAccountImpl::AddressList List;
	const List& l = pImpl_->listMyAddress_;
	List::const_iterator it = l.begin();
	while (it != l.end()) {
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
		++it;
	}
	return false;
}

bool qm::SubAccount::isMyAddress(const AddressListParser& address) const
{
	typedef AddressListParser::AddressList List;
	const List& l = address.getAddressList();
	List::const_iterator it = l.begin();
	while (it != l.end()) {
		const AddressParser* pAddress = *it;
		const AddressListParser* pGroup = pAddress->getGroup();
		if (pGroup) {
			const List& l = pGroup->getAddressList();
			List::const_iterator it = l.begin();
			while (it != l.end()) {
				if (isMyAddress((*it)->getMailbox(), (*it)->getHost()))
					return true;
				++it;
			}
		}
		else {
			if (isMyAddress(pAddress->getMailbox(), pAddress->getHost()))
				return true;
		}
		++it;
	}
	return false;
}

const WCHAR* qm::SubAccount::getHost(Account::Host host) const
{
	return pImpl_->wstrHost_[host];
}

QSTATUS qm::SubAccount::setHost(Account::Host host, const WCHAR* pwszHost)
{
	string_ptr<WSTRING> wstrHost(allocWString(pwszHost));
	if (!wstrHost.get())
		return QSTATUS_OUTOFMEMORY;
	
	freeWString(pImpl_->wstrHost_[host]);
	pImpl_->wstrHost_[host] = wstrHost.release();
	
	return QSTATUS_SUCCESS;
}

short qm::SubAccount::getPort(Account::Host host) const
{
	return pImpl_->nPort_[host];
}

void qm::SubAccount::setPort(Account::Host host, short nPort)
{
	pImpl_->nPort_[host] = nPort;
}

const WCHAR* qm::SubAccount::getUserName(Account::Host host) const
{
	return pImpl_->wstrUserName_[host];
}

QSTATUS qm::SubAccount::setUserName(Account::Host host, const WCHAR* pwszUserName)
{
	string_ptr<WSTRING> wstrUserName(allocWString(pwszUserName));
	if (!wstrUserName.get())
		return QSTATUS_OUTOFMEMORY;
	
	freeWString(pImpl_->wstrUserName_[host]);
	pImpl_->wstrUserName_[host] = wstrUserName.release();
	
	return QSTATUS_SUCCESS;
}

const WCHAR* qm::SubAccount::getPassword(Account::Host host) const
{
	return pImpl_->wstrPassword_[host];
}

QSTATUS qm::SubAccount::setPassword(Account::Host host, const WCHAR* pwszPassword)
{
	string_ptr<WSTRING> wstrPassword(allocWString(pwszPassword));
	if (!wstrPassword.get())
		return QSTATUS_OUTOFMEMORY;
	
	freeWString(pImpl_->wstrPassword_[host]);
	pImpl_->wstrPassword_[host] = wstrPassword.release();
	
	return QSTATUS_SUCCESS;
}

bool qm::SubAccount::isSsl(Account::Host host) const
{
	return pImpl_->bSsl_[host];
}

void qm::SubAccount::setSsl(Account::Host host, bool bSsl)
{
	pImpl_->bSsl_[host] = bSsl;
}

bool qm::SubAccount::isLog(Account::Host host) const
{
	return pImpl_->bLog_[host];
}

void qm::SubAccount::setLog(Account::Host host, bool bLog)
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

bool qm::SubAccount::isAllowUnverifiedCertificate() const
{
	return pImpl_->bAllowUnverifiedCertificate_;
}

void qm::SubAccount::setAllowUnverifiedCertificate(bool bAllow) const
{
	pImpl_->bAllowUnverifiedCertificate_ = bAllow;
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
	return pImpl_->wstrDialupEntry_;
}

QSTATUS qm::SubAccount::setDialupEntry(const WCHAR* pwszEntry)
{
	string_ptr<WSTRING> wstrEntry(allocWString(pwszEntry));
	if (!wstrEntry.get())
		return QSTATUS_OUTOFMEMORY;
	
	freeWString(pImpl_->wstrDialupEntry_);
	pImpl_->wstrDialupEntry_ = wstrEntry.release();
	
	return QSTATUS_SUCCESS;
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
	return pImpl_->pPrivateKey_;
}

Certificate* qm::SubAccount::getCertificate() const
{
	return pImpl_->pCertificate_;
}

QSTATUS qm::SubAccount::getProperty(const WCHAR* pwszSection,
	const WCHAR* pwszKey, int nDefault, int* pnValue) const
{
	assert(pwszSection);
	assert(pwszKey);
	assert(pnValue);
	
	DECLARE_QSTATUS();
	
	*pnValue = 0;
	
	if (!*pnValue) {
		status = pImpl_->pProfile_->getInt(pwszSection,
			pwszKey, nDefault, pnValue);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::SubAccount::setProperty(const WCHAR* pwszSection,
	const WCHAR* pwszKey, int nValue)
{
	assert(pwszSection);
	assert(pwszKey);
	
	DECLARE_QSTATUS();
	
	status = pImpl_->pProfile_->setInt(pwszSection, pwszKey, nValue);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::SubAccount::getProperty(const WCHAR* pwszSection,
	const WCHAR* pwszKey, const WCHAR* pwszDefault, WSTRING* pwstrValue) const
{
	assert(pwszSection);
	assert(pwszKey);
	assert(pwstrValue);
	
	DECLARE_QSTATUS();
	
	*pwstrValue = 0;
	
	if (wcscmp(pwszSection, L"Global") == 0) {
		if (wcscmp(pwszKey, L"Path") == 0) {
			*pwstrValue = allocWString(pImpl_->pAccount_->getPath());
			if (!*pwstrValue)
				return QSTATUS_OUTOFMEMORY;
		}
	}
	
	if (!*pwstrValue) {
		status = pImpl_->pProfile_->getString(
			pwszSection, pwszKey, pwszDefault, pwstrValue);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::SubAccount::setProperty(const WCHAR* pwszSection,
	const WCHAR* pwszKey, const WCHAR* pwszValue)
{
	assert(pwszSection);
	assert(pwszKey);
	assert(pwszValue);
	
	DECLARE_QSTATUS();
	
	status = pImpl_->pProfile_->setString(pwszSection, pwszKey, pwszValue);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

const WCHAR* qm::SubAccount::getSyncFilterName() const
{
	return pImpl_->wstrSyncFilterName_;
}

QSTATUS qm::SubAccount::setSyncFilterName(const WCHAR* pwszName)
{
	string_ptr<WSTRING> wstrName(allocWString(pwszName));
	if (!wstrName.get())
		return QSTATUS_OUTOFMEMORY;
	pImpl_->wstrSyncFilterName_ = wstrName.release();
	return QSTATUS_SUCCESS;
}

QSTATUS qm::SubAccount::isSelf(const Message& msg, bool* pbSelf) const
{
	assert(pbSelf);
	
	DECLARE_QSTATUS();
	
	*pbSelf = false;
	
	if (pImpl_->bTreatAsSent_) {
		AddressListParser from(AddressListParser::FLAG_DISALLOWGROUP, &status);
		CHECK_QSTATUS();
		Part::Field field;
		status = msg.getField(L"From", &from, &field);
		CHECK_QSTATUS();
		if (field == Part::FIELD_EXIST) {
			const AddressListParser::AddressList& listFrom = from.getAddressList();
			if (!listFrom.empty()) {
				AddressParser* pFrom = listFrom.front();
				if (isMyAddress(pFrom->getMailbox(), pFrom->getHost())) {
					AddressListParser sender(AddressListParser::FLAG_DISALLOWGROUP, &status);
					CHECK_QSTATUS();
					status = msg.getField(L"Sender", &sender, &field);
					CHECK_QSTATUS();
					if (field == Part::FIELD_EXIST) {
						const AddressListParser::AddressList& listSender =
							sender.getAddressList();
						if (!listSender.empty()) {
							AddressParser* pSender = listSender.front();
							*pbSelf = wcsicmp(pFrom->getMailbox(), pSender->getMailbox()) == 0 &&
								wcsicmp(pFrom->getHost(), pSender->getHost()) == 0;
						}
					}
					else {
						*pbSelf = true;
					}
				}
			}
			if (*pbSelf) {
				const WCHAR* pwszFields[] = {
					L"Posted",
					L"X-ML-Name",
					L"Mailing-List"
				};
				for (int n = 0; n < countof(pwszFields) && *pbSelf; ++n) {
					bool bHas = false;
					status = msg.hasField(pwszFields[n], &bHas);
					CHECK_QSTATUS();
					if (bHas)
						*pbSelf = false;
				}
			}
		}
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::SubAccount::save() const
{
	DECLARE_QSTATUS();
	
#define SAVE_STRING(section, key, name) \
	status = pImpl_->pProfile_->setString(section, key, pImpl_->##name); \
	CHECK_QSTATUS(); \
	
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
	status = pImpl_->pProfile_->setInt(section, key, pImpl_->##name); \
	CHECK_QSTATUS(); \
	
	SAVE_INT(L"Send",		L"Port",						nPort_[Account::HOST_SEND]		);
	SAVE_INT(L"Receive",	L"Port",						nPort_[Account::HOST_RECEIVE]	);
	SAVE_INT(L"Global",		L"Timeout",						nTimeout_						);
	SAVE_INT(L"Send",		L"Ssl",							bSsl_[Account::HOST_SEND]		);
	SAVE_INT(L"Receive",	L"Ssl",							bSsl_[Account::HOST_RECEIVE]	);
	SAVE_INT(L"Send",		L"Log",							bLog_[Account::HOST_SEND]		);
	SAVE_INT(L"Receive",	L"Log",							bLog_[Account::HOST_RECEIVE]	);
	SAVE_INT(L"Global",		L"ConnectReceiveBeforeSend",	bConnectReceiveBeforeSend_		);
	SAVE_INT(L"Global",		L"TreatAsSent",					bTreatAsSent_					);
	SAVE_INT(L"Global",		L"AddMessageId",				bAddMessageId_					);
	SAVE_INT(L"Global",		L"AllowUnverifiedCertificate",	bAllowUnverifiedCertificate_	);
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
		string_ptr<WSTRING> wstrEncodedPassword;
		status = TextUtil::encodePassword(
			pImpl_->wstrPassword_[entries[n].host_],
			&wstrEncodedPassword);
		CHECK_QSTATUS();
		status = pImpl_->pProfile_->setString(entries[n].pwszSection_,
			L"EncodedPassword", wstrEncodedPassword.get());
		CHECK_QSTATUS();
		status = pImpl_->pProfile_->setString(
			entries[n].pwszSection_, L"Password", L"");
		CHECK_QSTATUS();
	}
	
	string_ptr<WSTRING> wstrMyAddress;
	status = getMyAddress(&wstrMyAddress);
	CHECK_QSTATUS();
	status = pImpl_->pProfile_->setString(L"Global", L"MyAddress", wstrMyAddress.get());
	CHECK_QSTATUS();
	
	return pImpl_->pProfile_->save();
}

QSTATUS qm::SubAccount::setName(const WCHAR* pwszName)
{
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrName(allocWString(pwszName));
	if (!wstrName.get())
		return QSTATUS_OUTOFMEMORY;
	
	ConcatW c[] = {
		{ pImpl_->pAccount_->getPath(),	-1	},
		{ L"\\",						1	},
		{ FileNames::ACCOUNT,			-1	},
		{ L"_",							1	},
		{ wstrName.get(),				-1	},
		{ FileNames::XML_EXT,			-1	}
	};
	string_ptr<WSTRING> wstrPath(concat(c, countof(c)));
	if (!wstrPath.get())
		return QSTATUS_OUTOFMEMORY;
	
	status = pImpl_->pProfile_->rename(wstrPath.get());
	CHECK_QSTATUS();
	
	freeWString(pImpl_->wstrName_);
	pImpl_->wstrName_ = wstrName.release();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::SubAccount::deletePermanent()
{
	return pImpl_->pProfile_->deletePermanent();
}
