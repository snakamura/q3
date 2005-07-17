/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsfile.h>
#include <qstextutil.h>
#include <qsthread.h>

#include "password.h"
#include "../util/confighelper.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * PasswordManagerImpl
 *
 */

struct qm::PasswordManagerImpl
{
	PasswordManagerImpl(const WCHAR* pwszPath);
	bool load();
	
	PasswordManager* pThis_;
	PasswordManagerCallback* pCallback_;
	PasswordManager::PasswordList listPassword_;
	qs::CriticalSection cs_;
	ConfigHelper<PasswordManager, PasswordContentHandler, PasswordWriter> helper_;
};

qm::PasswordManagerImpl::PasswordManagerImpl(const WCHAR* pwszPath) :
	helper_(pwszPath)
{
}

bool qm::PasswordManagerImpl::load()
{
	PasswordContentHandler handler(&listPassword_);
	return helper_.load(pThis_, &handler);
}


/****************************************************************************
 *
 * PasswordManager
 *
 */

qm::PasswordManager::PasswordManager(const WCHAR* pwszPath,
									 PasswordManagerCallback* pCallback) :
	pImpl_(0)
{
	pImpl_ = new PasswordManagerImpl(pwszPath);
	pImpl_->pThis_ = this;
	pImpl_->pCallback_ = pCallback;
	pImpl_->load();
}

qm::PasswordManager::~PasswordManager()
{
	clear();
	delete pImpl_;
}

wstring_ptr qm::PasswordManager::getPassword(const PasswordCondition& condition,
											 bool bPermanentOnly,
											 PasswordState* pState) const
{
	Lock<CriticalSection> lock(pImpl_->cs_);
	
	wstring_ptr wstrPassword;
	PasswordState state = PASSWORDSTATE_ONETIME;
	
	PasswordList::const_iterator it = pImpl_->listPassword_.begin();
	while (it != pImpl_->listPassword_.end() && !(*it)->visit(condition))
		++it;
	if (bPermanentOnly) {
		if (it != pImpl_->listPassword_.end() && (*it)->isPermanent())
			wstrPassword = allocWString((*it)->getPassword());
	}
	else {
		if (it != pImpl_->listPassword_.end())
			wstrPassword = allocWString((*it)->getPassword());
		else if (pState)
			state = pImpl_->pCallback_->getPassword(condition, &wstrPassword);
	}
	
	if (pState)
		*pState = wstrPassword.get() ? state : PASSWORDSTATE_NONE;
	
	return wstrPassword;
}

void qm::PasswordManager::setPassword(const PasswordCondition& condition,
									  const WCHAR* pwszPassword,
									  bool bPermanent)
{
	Lock<CriticalSection> lock(pImpl_->cs_);
	
	PasswordList::iterator it = pImpl_->listPassword_.begin();
	while (it != pImpl_->listPassword_.end() && !(*it)->visit(condition))
		++it;
	if (it == pImpl_->listPassword_.end()) {
		std::auto_ptr<Password> pPassword(condition.createPassword(
			pwszPassword, bPermanent));
		pImpl_->listPassword_.push_back(pPassword.get());
		pPassword.release();
	}
	else {
		(*it)->set(pwszPassword, bPermanent);
	}
}

void qm::PasswordManager::removePassword(const PasswordCondition& condition)
{
	Lock<CriticalSection> lock(pImpl_->cs_);
	
	PasswordList::iterator it = pImpl_->listPassword_.begin();
	while (it != pImpl_->listPassword_.end() && !(*it)->visit(condition))
		++it;
	if (it != pImpl_->listPassword_.end()) {
		delete *it;
		pImpl_->listPassword_.erase(it);
	}
}

bool qm::PasswordManager::save(bool bForce) const
{
	return pImpl_->helper_.save(this) || bForce;
}

void qm::PasswordManager::clear()
{
	std::for_each(pImpl_->listPassword_.begin(),
		pImpl_->listPassword_.end(), qs::deleter<Password>());
}

const PasswordManager::PasswordList& qm::PasswordManager::getPasswords() const
{
	return pImpl_->listPassword_;
}


/****************************************************************************
 *
 * PasswordManagerCallback
 *
 */

qm::PasswordManagerCallback::~PasswordManagerCallback()
{
}


/****************************************************************************
 *
 * Password
 *
 */

qm::Password::Password(const WCHAR* pwszPassword,
					   bool bPermanent) :
	bPermanent_(bPermanent)
{
	wstrPassword_ = allocWString(pwszPassword);
}

qm::Password::~Password()
{
}

const WCHAR* qm::Password::getPassword() const
{
	return wstrPassword_.get();
}

bool qm::Password::isPermanent() const
{
	return bPermanent_;
}

void qm::Password::set(const WCHAR* pwszPassword,
					   bool bPermanent)
{
	wstrPassword_ = allocWString(pwszPassword);
	bPermanent_ = bPermanent;
}


/****************************************************************************
 *
 * PasswordVisitor
 *
 */

qm::PasswordVisitor::~PasswordVisitor()
{
}


/****************************************************************************
 *
 * PasswordCondition
 *
 */

qm::PasswordCondition::~PasswordCondition()
{
}

bool qm::PasswordCondition::visit(const AccountPassword& password) const
{
	return false;
}

bool qm::PasswordCondition::visit(const FilePassword& password) const
{
	return false;
}

bool qm::PasswordCondition::visit(const PGPPassword& password) const
{
	return false;
}


/****************************************************************************
 *
 * AccountPassword
 *
 */

qm::AccountPassword::AccountPassword(const WCHAR* pwszAccount,
									 const WCHAR* pwszSubAccount,
									 Account::Host host,
									 const WCHAR* pwszPassword,
									 bool bPermanent) :
	Password(pwszPassword, bPermanent),
	host_(host)
{
	wstrAccount_ = allocWString(pwszAccount);
	wstrSubAccount_ = allocWString(pwszSubAccount);
}

qm::AccountPassword::~AccountPassword()
{
}

const WCHAR* qm::AccountPassword::getAccount() const
{
	return wstrAccount_.get();
}

const WCHAR* qm::AccountPassword::getSubAccount() const
{
	return wstrSubAccount_.get();
}

Account::Host qm::AccountPassword::getHost() const
{
	return host_;
}

bool qm::AccountPassword::visit(const PasswordVisitor& visitor) const
{
	return visitor.visit(*this);
}


/****************************************************************************
 *
 * AccountPasswordCondition
 *
 */

qm::AccountPasswordCondition::AccountPasswordCondition(Account* pAccount,
													   SubAccount* pSubAccount,
													   Account::Host host) :
	pAccount_(pAccount),
	pSubAccount_(pSubAccount),
	host_(host)
{
}

qm::AccountPasswordCondition::~AccountPasswordCondition()
{
}

std::auto_ptr<Password> qm::AccountPasswordCondition::createPassword(const WCHAR* pwszPassword,
																	 bool bPermanent) const
{
	return std::auto_ptr<Password>(new AccountPassword(pAccount_->getName(),
		pSubAccount_->getName(), host_, pwszPassword, bPermanent));
}

wstring_ptr qm::AccountPasswordCondition::getHint() const
{
	StringBuffer<WSTRING> buf;
	
	buf.append(L'[');
	buf.append(pAccount_->getName());
	if (*pSubAccount_->getName()) {
		buf.append(L'/');
		buf.append(pSubAccount_->getName());
	}
	buf.append(L"] ");
	buf.append(pSubAccount_->getUserName(host_));
	
	return buf.getString();
}

bool qm::AccountPasswordCondition::visit(const AccountPassword& password) const
{
	return password.getHost() == host_ &&
		wcscmp(password.getAccount(), pAccount_->getName()) == 0 &&
		wcscmp(password.getSubAccount(), pSubAccount_->getName()) == 0;
}


/****************************************************************************
 *
 * FilePassword
 *
 */

qm::FilePassword::FilePassword(const WCHAR* pwszPath,
							   const WCHAR* pwszPassword,
							   bool bPermanent) :
	Password(pwszPassword, bPermanent)
{
	wstrPath_ = allocWString(pwszPath);
}

qm::FilePassword::~FilePassword()
{
}

const WCHAR* qm::FilePassword::getPath() const
{
	return wstrPath_.get();
}

bool qm::FilePassword::visit(const PasswordVisitor& visitor) const
{
	return visitor.visit(*this);
}


/****************************************************************************
 *
 * FilePasswordCondition
 *
 */

qm::FilePasswordCondition::FilePasswordCondition(const WCHAR* pwszPath) :
	pwszPath_(pwszPath)
{
}

qm::FilePasswordCondition::~FilePasswordCondition()
{
}

std::auto_ptr<Password> qm::FilePasswordCondition::createPassword(const WCHAR* pwszPassword,
																  bool bPermanent) const
{
	return std::auto_ptr<Password>(new FilePassword(pwszPath_, pwszPassword, bPermanent));
}

wstring_ptr qm::FilePasswordCondition::getHint() const
{
	return allocWString(pwszPath_);
}

bool qm::FilePasswordCondition::visit(const FilePassword& password) const
{
	return wcscmp(password.getPath(), pwszPath_) == 0;
}


/****************************************************************************
 *
 * PGPPassword
 *
 */

qm::PGPPassword::PGPPassword(const WCHAR* pwszUserId,
							 const WCHAR* pwszPassword,
							 bool bPermanent) :
	Password(pwszPassword, bPermanent)
{
	wstrUserId_ = allocWString(pwszUserId);
}

qm::PGPPassword::~PGPPassword()
{
}

const WCHAR* qm::PGPPassword::getUserId() const
{
	return wstrUserId_.get();
}

bool qm::PGPPassword::visit(const PasswordVisitor& visitor) const
{
	return visitor.visit(*this);
}


/****************************************************************************
 *
 * PGPPasswordCondition
 *
 */

qm::PGPPasswordCondition::PGPPasswordCondition(const WCHAR* pwszUserId) :
	pwszUserId_(pwszUserId)
{
}

qm::PGPPasswordCondition::~PGPPasswordCondition()
{
}

std::auto_ptr<Password> qm::PGPPasswordCondition::createPassword(const WCHAR* pwszPassword,
																 bool bPermanent) const
{
	return std::auto_ptr<Password>(new PGPPassword(pwszUserId_, pwszPassword, bPermanent));
}

wstring_ptr qm::PGPPasswordCondition::getHint() const
{
	return allocWString(pwszUserId_);
}

bool qm::PGPPasswordCondition::visit(const PGPPassword& password) const
{
	return wcscmp(password.getUserId(), pwszUserId_) == 0;
}


/****************************************************************************
 *
 * PasswordContentHandler
 *
 */

qm::PasswordContentHandler::PasswordContentHandler(PasswordManager::PasswordList* pList) :
	pList_(pList),
	state_(STATE_ROOT)
{
}

qm::PasswordContentHandler::~PasswordContentHandler()
{
}

bool qm::PasswordContentHandler::startElement(const WCHAR* pwszNamespaceURI,
											  const WCHAR* pwszLocalName,
											  const WCHAR* pwszQName,
											  const Attributes& attributes)
{
	if (wcscmp(pwszLocalName, L"passwords") == 0) {
		if (state_ != STATE_ROOT)
			return false;
		if (attributes.getLength() != 0)
			return false;
		state_ = STATE_PASSWORDS;
	}
	else if (wcscmp(pwszLocalName, L"accountPassword") == 0) {
		if (state_ != STATE_PASSWORDS)
			return false;
		
		const WCHAR* pwszAccount = 0;
		const WCHAR* pwszSubAccount = 0;
		const WCHAR* pwszType = 0;
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrName, L"account") == 0)
				pwszAccount = attributes.getValue(n);
			else if (wcscmp(pwszAttrName, L"subaccount") == 0)
				pwszSubAccount = attributes.getValue(n);
			else if (wcscmp(pwszAttrName, L"type") == 0)
				pwszType = attributes.getValue(n);
			else
				return false;
		}
		if (!pwszAccount || !pwszSubAccount || !pwszType)
			return false;
		Account::Host host = wcscmp(pwszType, L"receive") == 0 ?
			Account::HOST_RECEIVE : Account::HOST_SEND;
		
		assert(!pPassword_.get());
		pPassword_.reset(new AccountPassword(pwszAccount, pwszSubAccount, host, L"", true));
		
		state_ = STATE_CONDITION;
	}
	else if (wcscmp(pwszLocalName, L"filePassword") == 0) {
		if (state_ != STATE_PASSWORDS)
			return false;
		
		const WCHAR* pwszPath = 0;
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrName, L"path") == 0)
				pwszPath = attributes.getValue(n);
			else
				return false;
		}
		if (!pwszPath)
			return false;
		
		assert(!pPassword_.get());
		pPassword_.reset(new FilePassword(pwszPath, L"", true));
		
		state_ = STATE_CONDITION;
	}
	else if (wcscmp(pwszLocalName, L"pgpPassword") == 0) {
		if (state_ != STATE_PASSWORDS)
			return false;
		
		const WCHAR* pwszUserId = 0;
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrName, L"userid") == 0)
				pwszUserId = attributes.getValue(n);
			else
				return false;
		}
		if (!pwszUserId)
			return false;
		
		assert(!pPassword_.get());
		pPassword_.reset(new PGPPassword(pwszUserId, L"", true));
		
		state_ = STATE_CONDITION;
	}
	else if (wcscmp(pwszLocalName, L"password") == 0) {
		if (state_ != STATE_CONDITION)
			return false;
		if (attributes.getLength() != 0)
			return false;
		state_ = STATE_PASSWORD;
	}
	else {
		return false;
	}
	return true;
}

bool qm::PasswordContentHandler::endElement(const WCHAR* pwszNamespaceURI,
											const WCHAR* pwszLocalName,
											const WCHAR* pwszQName)
{
	if (wcscmp(pwszLocalName, L"passwords") == 0) {
		assert(state_ == STATE_PASSWORDS);
		state_ = STATE_ROOT;
	}
	else if (wcscmp(pwszLocalName, L"accountPassword") == 0 ||
		wcscmp(pwszLocalName, L"filePassword") == 0 ||
		wcscmp(pwszLocalName, L"pgpPassword") == 0) {
		assert(state_ == STATE_CONDITION);
		assert(pPassword_.get());
		pList_->push_back(pPassword_.get());
		pPassword_.release();
		state_ = STATE_PASSWORDS;
	}
	else if (wcscmp(pwszLocalName, L"password") == 0) {
		assert(state_ == STATE_PASSWORD);
		assert(pPassword_.get());
		wstring_ptr wstrPassword(TextUtil::decodePassword(buffer_.getCharArray()));
		pPassword_->set(wstrPassword.get(), true);
		buffer_.remove();
		state_ = STATE_CONDITION;
	}
	else {
		return false;
	}
	return true;
}

bool qm::PasswordContentHandler::characters(const WCHAR* pwsz,
											size_t nStart,
											size_t nLength)
{
	if (state_ == STATE_PASSWORD) {
		buffer_.append(pwsz + nStart, nLength);
	}
	else {
		const WCHAR* p = pwsz + nStart;
		for (size_t n = 0; n < nLength; ++n, ++p) {
			if (*p != L' ' && *p != L'\t' && *p != '\n')
				return false;
		}
	}
	return true;
}


/****************************************************************************
 *
 * PasswordWriter
 *
 */

qm::PasswordWriter::PasswordWriter(Writer* pWriter) :
	handler_(pWriter)
{
}

qm::PasswordWriter::~PasswordWriter()
{
}

bool qm::PasswordWriter::write(const PasswordManager* pManager)
{
	if (!handler_.startDocument())
		return false;
	if (!handler_.startElement(0, 0, L"passwords", DefaultAttributes()))
		return false;
	
	const PasswordManager::PasswordList& l = pManager->getPasswords();
	for (PasswordManager::PasswordList::const_iterator it = l.begin(); it != l.end(); ++it) {
		const Password* pPassword = *it;
		if (pPassword->isPermanent()) {
			struct PasswordVisitorImpl : public PasswordVisitor
			{
				PasswordVisitorImpl(PasswordWriter* pWriter) :
					pWriter_(pWriter)
				{
				}
				
				virtual bool visit(const AccountPassword& password) const
				{
					SimpleAttributes::Item item[] = {
						{ L"account",		password.getAccount()												},
						{ L"subaccount",	password.getSubAccount()											},
						{ L"type",			password.getHost() == Account::HOST_RECEIVE ? L"receive" : L"send"	}
					};
					SimpleAttributes attrs(item, countof(item));
					return pWriter_->write(password, L"accountPassword", attrs);
				}
				
				virtual bool visit(const FilePassword& password) const
				{
					SimpleAttributes attrs(L"path", password.getPath());
					return pWriter_->write(password, L"filePassword", attrs);
				}
				
				virtual bool visit(const PGPPassword& password) const
				{
					SimpleAttributes attrs(L"userid", password.getUserId());
					return pWriter_->write(password, L"pgpPassword", attrs);
				}
				
				PasswordWriter* pWriter_;
			} visitor(this);
			if (!pPassword->visit(visitor))
				return false;
		}
	}
	
	if (!handler_.endElement(0, 0, L"passwords"))
		return false;
	if (!handler_.endDocument())
		return false;
	
	return true;
}

bool qm::PasswordWriter::write(const Password& password,
							   const WCHAR* pwszName,
							   const qs::Attributes& attrs)
{
	wstring_ptr wstrPassword(TextUtil::encodePassword(password.getPassword()));
	return handler_.startElement(0, 0, pwszName, attrs) &&
		handler_.startElement(0, 0, L"password", DefaultAttributes()) &&
		handler_.characters(wstrPassword.get(), 0, wcslen(wstrPassword.get())) &&
		handler_.endElement(0, 0, L"password") &&
		handler_.endElement(0, 0, pwszName);
}
