/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmapplication.h>
#include <qmfilenames.h>

#include <qsfile.h>
#include <qstextutil.h>
#include <qsthread.h>

#include "password.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * PasswordManagerImpl
 *
 */

struct qm::PasswordManagerImpl
{
	bool load();
	
	PasswordManager::PasswordList listPassword_;
	qs::CriticalSection cs_;
};

bool qm::PasswordManagerImpl::load()
{
	wstring_ptr wstrPath(Application::getApplication().getProfilePath(FileNames::PASSWORDS_XML));
	
	XMLReader reader;
	PasswordContentHandler handler(&listPassword_);
	reader.setContentHandler(&handler);
	return reader.parse(wstrPath.get());
}


/****************************************************************************
 *
 * PasswordManager
 *
 */

qm::PasswordManager::PasswordManager() :
	pImpl_(0)
{
	pImpl_ = new PasswordManagerImpl();
	pImpl_->load();
}

qm::PasswordManager::~PasswordManager()
{
	if (pImpl_) {
		std::for_each(pImpl_->listPassword_.begin(),
			pImpl_->listPassword_.end(), qs::deleter<Password>());
		delete pImpl_;
	}
}

wstring_ptr qm::PasswordManager::getPassword(const PasswordCondition& condition,
											 bool bPermanentOnly) const
{
	Lock<CriticalSection> lock(pImpl_->cs_);
	
	PasswordList::const_iterator it = pImpl_->listPassword_.begin();
	while (it != pImpl_->listPassword_.end() && !(*it)->visit(condition))
		++it;
	if (it == pImpl_->listPassword_.end())
		return 0;
	if (bPermanentOnly && !(*it)->isPermanent())
		return 0;
	return allocWString((*it)->getPassword());
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

bool qm::PasswordManager::save() const
{
	wstring_ptr wstrPath(Application::getApplication().getProfilePath(FileNames::PASSWORDS_XML));
	
	TemporaryFileRenamer renamer(wstrPath.get());
	
	FileOutputStream os(renamer.getPath());
	if (!os)
		return false;
	OutputStreamWriter writer(&os, false, L"utf-8");
	if (!writer)
		return false;
	BufferedWriter bufferedWriter(&writer, false);
	
	PasswordWriter passwordWriter(&bufferedWriter);
	if (!passwordWriter.write(this))
		return false;
	
	if (!bufferedWriter.close())
		return false;
	
	if (!renamer.rename())
		return false;
	
	return true;
}

const PasswordManager::PasswordList& qm::PasswordManager::getPasswords() const
{
	return pImpl_->listPassword_;
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

qm::AccountPasswordCondition::AccountPasswordCondition(const WCHAR* pwszAccount,
													   const WCHAR* pwszSubAccount,
													   Account::Host host) :
	pwszAccount_(pwszAccount),
	pwszSubAccount_(pwszSubAccount),
	host_(host)
{
}

qm::AccountPasswordCondition::~AccountPasswordCondition()
{
}

std::auto_ptr<Password> qm::AccountPasswordCondition::createPassword(const WCHAR* pwszPassword,
																	 bool bPermanent) const
{
	return std::auto_ptr<Password>(new AccountPassword(pwszAccount_,
		pwszSubAccount_, host_, pwszPassword, bPermanent));
}

bool qm::AccountPasswordCondition::visit(const AccountPassword& password) const
{
	return password.getHost() == host_ &&
		wcscmp(password.getAccount(), pwszAccount_) == 0 &&
		string_equal<WCHAR>().operator()(password.getSubAccount(), pwszSubAccount_);
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
		
		state_ = STATE_ACCOUNTPASSWORD;
	}
	else if (wcscmp(pwszLocalName, L"password") == 0) {
		if (state_ != STATE_ACCOUNTPASSWORD)
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
	else if (wcscmp(pwszLocalName, L"accountPassword") == 0) {
		assert(state_ == STATE_ACCOUNTPASSWORD);
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
		state_ = STATE_ACCOUNTPASSWORD;
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
