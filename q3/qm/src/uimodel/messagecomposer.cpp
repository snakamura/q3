/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmaccount.h>
#include <qmapplication.h>
#include <qmdocument.h>
#include <qmfolder.h>
#include <qmmacro.h>
#include <qmmessage.h>
#include <qmpassword.h>
#include <qmpgp.h>
#include <qmsecurity.h>

#include <qscrypto.h>

#include "foldermodel.h"
#include "messagecomposer.h"
#include "securitymodel.h"
#include "../model/addressbook.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * MessageComposer
 *
 */

qm::MessageComposer::MessageComposer(bool bDraft,
									 Document* pDocument,
									 PasswordManager* pPasswordManager,
									 Profile* pProfile,
									 HWND hwnd,
									 FolderModel* pFolderModel,
									 SecurityModel* pSecurityModel) :
	bDraft_(bDraft),
	pDocument_(pDocument),
	pPasswordManager_(pPasswordManager),
	pProfile_(pProfile),
	hwnd_(hwnd),
	pFolderModel_(pFolderModel),
	pSecurityModel_(pSecurityModel)
{
}

qm::MessageComposer::~MessageComposer()
{
}

bool qm::MessageComposer::compose(Account* pAccount,
								  SubAccount* pSubAccount,
								  Message* pMessage,
								  unsigned int nFlags,
								  MessagePtr* pptr) const
{
	assert(pAccount || pFolderModel_);
	assert((!pAccount && !pSubAccount) ||
		(pAccount && !pSubAccount) || (pAccount && pSubAccount));
	assert(pMessage);
	
	if (!pAccount) {
		assert(pFolderModel_);
		
		UnstructuredParser account;
		Part::Field f = pMessage->getField(L"X-QMAIL-Account", &account);
		if (f == Part::FIELD_EXIST) {
			pAccount = pDocument_->getAccount(account.getValue());
		}
		else {
			std::pair<Account*, Folder*> p(pFolderModel_->getCurrent());
			pAccount = p.first ? p.first : p.second ? p.second->getAccount() : 0;
		}
	}
	if (!pAccount)
		return false;
	if (!pSubAccount) {
		UnstructuredParser subaccount;
		Part::Field f = pMessage->getField(L"X-QMAIL-SubAccount", &subaccount);
		if (f == Part::FIELD_EXIST)
			pSubAccount = pAccount->getSubAccount(subaccount.getValue());
		else
			pSubAccount = pAccount->getCurrentSubAccount();
	}
	
	Folder* pFolder = pAccount->getFolderByFlag(
		bDraft_ ? Folder::FLAG_DRAFTBOX : Folder::FLAG_OUTBOX);
	if (!pFolder || pFolder->getType() != Folder::TYPE_NORMAL)
		return false;
	
	bool bResent = PartUtil(*pMessage).isResent();
	
	AddressParser from(pSubAccount->getSenderName(),
		pSubAccount->getSenderAddress());
	if (!pMessage->setField(bResent ? L"Resent-From" : L"From", from))
		return false;
	
	Time time(Time::getCurrentTime());
	DateParser date(time);
	if (!pMessage->setField(bResent ? L"Resent-Date" : L"Date", date))
		return false;
	
	if (pSubAccount->isAddMessageId()) {
		WCHAR wsz[256];
		swprintf(wsz, L"%u%04d%02d%02d%02d%02d%02d",
			::GetCurrentProcessId(), time.wYear, time.wMonth,
			time.wDay, time.wHour, time.wMinute, time.wSecond);
		
		wstring_ptr wstrMessageId(concat(wsz, pSubAccount->getSenderAddress()));
		
		MessageIdParser messageId(wstrMessageId.get());
		if (!pMessage->setField(bResent ? L"Resent-Message-Id" : L"Message-Id", messageId))
			return false;
	}
	
	const WCHAR* pwszMacro[] = { 0, 0 };
	UnstructuredParser draftMacro;
	if (pMessage->getField(L"X-QMAIL-DraftMacro", &draftMacro) == Part::FIELD_EXIST)
		pwszMacro[0] = draftMacro.getValue();
	pMessage->removeField(L"X-QMAIL-DraftMacro");
	UnstructuredParser macro;
	if (!bDraft_) {
		if (pMessage->getField(L"X-QMAIL-Macro", &macro) == Part::FIELD_EXIST)
			pwszMacro[1] = macro.getValue();
		pMessage->removeField(L"X-QMAIL-Macro");
	}
	
	bool bWithOSVersion = pProfile_->getInt(L"Global", L"XMailerWithOSVersion", 1) != 0;
	wstring_ptr wstrVersion(Application::getApplication().getVersion(L' ', bWithOSVersion));
	UnstructuredParser mailer(wstrVersion.get(), L"utf-8");
	if (!pMessage->replaceField(L"X-Mailer", mailer))
		return false;
	
	if (!pMessage->sortHeader())
		return false;
	
	const Security* pSecurity = pDocument_->getSecurity();
	const SMIMEUtility* pSMIMEUtility = pSecurity->getSMIMEUtility();
	if (pSMIMEUtility && (nFlags & FLAG_SMIMESIGN || nFlags & FLAG_SMIMEENCRYPT)) {
		std::auto_ptr<Certificate> pSelfCertificate;
		if (pProfile_->getInt(L"Security", L"EncryptForSelf", 0))
			pSelfCertificate = pSubAccount->getCertificate(pPasswordManager_);
		SMIMECallbackImpl callback(pSecurity, pDocument_->getAddressBook(), pSelfCertificate.get());
		
		if (nFlags & FLAG_SMIMESIGN) {
			bool bMultipart = (nFlags & FLAG_SMIMEENCRYPT) == 0 &&
				pProfile_->getInt(L"Security", L"MultipartSigned", 1) != 0;
			std::auto_ptr<Certificate> pCertificate(pSubAccount->getCertificate(pPasswordManager_));
			std::auto_ptr<PrivateKey> pPrivateKey(pSubAccount->getPrivateKey(pPasswordManager_));
			if (!pCertificate.get() || !pPrivateKey.get())
				return false;
			xstring_ptr strMessage(pSMIMEUtility->sign(pMessage,
				bMultipart, pPrivateKey.get(), pCertificate.get()));
			if (!strMessage.get())
				return false;
			if (!pMessage->create(strMessage.get(), -1, Message::FLAG_NONE))
				return false;
		}
		if (nFlags & FLAG_SMIMEENCRYPT) {
			std::auto_ptr<Cipher> pCipher(Cipher::getInstance(L"des3"));
			xstring_ptr strMessage(pSMIMEUtility->encrypt(
				pMessage, pCipher.get(), &callback));
			if (!strMessage.get())
				return false;
			if (!pMessage->create(strMessage.get(), -1, Message::FLAG_NONE))
				return false;
		}
	}
	const PGPUtility* pPGPUtility = pSecurity->getPGPUtility();
	if (pPGPUtility && (nFlags & FLAG_PGPSIGN || nFlags & FLAG_PGPENCRYPT)) {
		const WCHAR* pwszUserId = pSubAccount->getSenderAddress();
		wstring_ptr wstrPassword;
		PasswordState state = PASSWORDSTATE_ONETIME;
		if (nFlags & FLAG_PGPSIGN) {
			PGPPasswordCondition condition(pwszUserId);
			wstrPassword = pPasswordManager_->getPassword(condition, false, &state);
			if (!wstrPassword.get())
				return false;
		}
		
		bool bMime = (nFlags & FLAG_PGPMIME) != 0;
		if (nFlags & FLAG_PGPSIGN && nFlags & FLAG_PGPENCRYPT) {
			const WCHAR* pwszUserId = pSubAccount->getSenderAddress();
			xstring_ptr strMessage(pPGPUtility->signAndEncrypt(
				pMessage, bMime, pwszUserId, wstrPassword.get()));
			if (!strMessage.get())
				return false;
			if (!pMessage->create(strMessage.get(), -1, Message::FLAG_NONE))
				return false;
		}
		else if (nFlags & FLAG_PGPSIGN) {
			const WCHAR* pwszUserId = pSubAccount->getSenderAddress();
			xstring_ptr strMessage(pPGPUtility->sign(pMessage,
				bMime, pwszUserId, wstrPassword.get()));
			if (!strMessage.get())
				return false;
			if (!pMessage->create(strMessage.get(), -1, Message::FLAG_NONE))
				return false;
		}
		else if (nFlags & FLAG_PGPENCRYPT) {
			xstring_ptr strMessage(pPGPUtility->encrypt(pMessage, bMime));
			if (!strMessage.get())
				return false;
			if (!pMessage->create(strMessage.get(), -1, Message::FLAG_NONE))
				return false;
		}
		
		if (state == PASSWORDSTATE_SESSION || state == PASSWORDSTATE_SAVE) {
			PGPPasswordCondition condition(pwszUserId);
			pPasswordManager_->setPassword(condition,
				wstrPassword.get(), state == PASSWORDSTATE_SAVE);
		}
	}
	
	if (!pAccount->appendMessage(static_cast<NormalFolder*>(pFolder), *pMessage,
		MessageHolder::FLAG_SEEN | (bDraft_ ? MessageHolder::FLAG_DRAFT : 0), pptr))
		return false;
	if (!pAccount->flushMessageStore() || !pFolder->saveMessageHolders())
		return false;
	
	for (int n = 0; n < countof(pwszMacro); ++n) {
		if (pwszMacro[n]) {
			MacroParser parser(MacroParser::TYPE_MESSAGE);
			std::auto_ptr<Macro> pMacro(parser.parse(pwszMacro[n]));
			if (!pMacro.get())
				return false;
			
			MacroContext context(0, 0, MessageHolderList(), pAccount, pDocument_,
				hwnd_, pProfile_, false, 0, pSecurityModel_->getSecurityMode(), 0, 0);
			MacroValuePtr pValue(pMacro->value(&context));
		}
	}
	
	return true;
}

bool qm::MessageComposer::compose(Account* pAccount,
								  SubAccount* pSubAccount,
								  const WCHAR* pwszPath,
								  unsigned int nFlags) const
{
	assert(pwszPath);
	
	FileInputStream stream(pwszPath);
	if (!stream)
		return false;
	BufferedInputStream bufferedStream(&stream, false);
	InputStreamReader reader(&bufferedStream, false, getSystemEncoding());
	if (!reader)
		return false;
	
	XStringBuffer<WXSTRING> buf;
	while (true) {
		XStringBufferLock<WXSTRING> lock(&buf, 1024);
		
		size_t nRead = reader.read(lock.get(), 1024);
		if (nRead == -1)
			return false;
		else if (nRead == 0)
			break;
		
		lock.unlock(nRead);
	}
	if (!reader.close())
		return false;
	
	if (buf.getLength() != 0) {
		MessageCreator creator(MessageCreator::FLAG_ADDCONTENTTYPE |
			MessageCreator::FLAG_EXPANDALIAS |
			MessageCreator::FLAG_EXTRACTATTACHMENT |
			MessageCreator::FLAG_ENCODETEXT,
			pSecurityModel_->getSecurityMode());
		std::auto_ptr<Message> pMessage(creator.createMessage(
			pDocument_, buf.getCharArray(), buf.getLength()));
		if (!pMessage.get())
			return false;
		
		if (!compose(0, 0, pMessage.get(), nFlags, 0))
			return false;
	}
	
	return true;
}


/****************************************************************************
 *
 * AddressBook::SMIMECallbackImpl
 *
 */

qm::SMIMECallbackImpl::SMIMECallbackImpl(const Security* pSecurity,
										 AddressBook* pAddressBook,
										 const Certificate* pSelfCertificate) :
	pSecurity_(pSecurity),
	pAddressBook_(pAddressBook),
	pSelfCertificate_(pSelfCertificate)
{
}

qm::SMIMECallbackImpl::~SMIMECallbackImpl()
{
}

std::auto_ptr<Certificate> qm::SMIMECallbackImpl::getCertificate(const WCHAR* pwszAddress)
{
	assert(pwszAddress);
	
	const AddressBookEntry* pEntry = pAddressBook_->getEntry(pwszAddress);
	if (!pEntry)
		return std::auto_ptr<Certificate>(0);
	
	const AddressBookEntry::AddressList& l = pEntry->getAddresses();
	for (AddressBookEntry::AddressList::const_iterator itA = l.begin(); itA != l.end(); ++itA) {
		const AddressBookAddress* pAddress = *itA;
		if (_wcsicmp(pAddress->getAddress(), pwszAddress) == 0) {
			const WCHAR* pwszCertificate = pAddress->getCertificate();
			if (pwszCertificate)
				return pSecurity_->getCertificate(pwszCertificate);
		}
	}
	return std::auto_ptr<Certificate>(0);
}

const Certificate* qm::SMIMECallbackImpl::getSelfCertificate()
{
	return pSelfCertificate_;
}