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
#include <qmsecurity.h>

#include <qscrypto.h>

#include "messagecomposer.h"
#include "../model/addressbook.h"
#include "../ui/foldermodel.h"
#include "../ui/securitymodel.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * MessageComposer
 *
 */

qm::MessageComposer::MessageComposer(bool bDraft,
									 Document* pDocument,
									 Profile* pProfile,
									 HWND hwnd,
									 FolderModel* pFolderModel,
									 SecurityModel* pSecurityModel) :
	bDraft_(bDraft),
	pDocument_(pDocument),
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
								  unsigned int nFlags) const
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
			pAccount = pFolderModel_->getCurrentAccount();
			if (!pAccount) {
				Folder* pFolder = pFolderModel_->getCurrentFolder();
				if (pFolder)
					pAccount = pFolder->getAccount();
			}
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
	
	const WCHAR* pwszMacro = 0;
	UnstructuredParser macro;
	Part::Field f = pMessage->getField(L"X-QMAIL-Macro", &macro);
	if (f == Part::FIELD_EXIST)
		pwszMacro = macro.getValue();
	pMessage->removeField(L"X-QMAIL-Macro");
	
	wstring_ptr wstrVersion(Application::getApplication().getVersion(true));
	UnstructuredParser mailer(wstrVersion.get(), L"utf-8");
	if (!pMessage->replaceField(L"X-Mailer", mailer))
		return false;
	
	if (!pMessage->sortHeader())
		return false;
	
	const Security* pSecurity = pDocument_->getSecurity();
	const SMIMEUtility* pSMIMEUtility = pSecurity->getSMIMEUtility();
	if (pSMIMEUtility && nFlags != 0) {
		SMIMECallbackImpl callback(pSecurity, pDocument_->getAddressBook());
		
		if (nFlags & FLAG_SIGN) {
			// TODO
			// Read from profile whether make multipart/signed or not
			bool bMultipart = false;
			Certificate* pCertificate = pSubAccount->getCertificate();
			PrivateKey* pPrivateKey = pSubAccount->getPrivateKey();
			if (pCertificate && pPrivateKey) {
				xstring_ptr strMessage(pSMIMEUtility->sign(pMessage,
					bMultipart, pPrivateKey, pCertificate, &callback));
				if (!strMessage.get())
					return false;
				if (!pMessage->create(strMessage.get(), -1, Message::FLAG_NONE))
					return false;
			}
		}
		if (nFlags & FLAG_ENCRYPT) {
			std::auto_ptr<Cipher> pCipher(Cipher::getInstance(L"des3"));
			xstring_ptr strMessage(pSMIMEUtility->encrypt(
				pMessage, pCipher.get(), &callback));
			if (!strMessage.get())
				return false;
			if (!pMessage->create(strMessage.get(), -1, Message::FLAG_NONE))
				return false;
		}
	}
	
	if (!pAccount->appendMessage(static_cast<NormalFolder*>(pFolder), *pMessage,
		MessageHolder::FLAG_SEEN | (bDraft_ ? MessageHolder::FLAG_DRAFT : 0)))
		return false;
	if (!pFolder->saveMessageHolders())
		return false;
	
	if (pwszMacro) {
		MacroParser parser(MacroParser::TYPE_MESSAGE);
		std::auto_ptr<Macro> pMacro(parser.parse(pwszMacro));
		if (!pMacro.get())
			return false;
		
		MacroContext context(0, 0, pAccount, pDocument_, hwnd_, pProfile_,
			false, pSecurityModel_->isDecryptVerify(), 0, 0);
		MacroValuePtr pValue(pMacro->value(&context));
	}
	
	return true;
}


/****************************************************************************
 *
 * AddressBook::SMIMECallbackImpl
 *
 */

qm::SMIMECallbackImpl::SMIMECallbackImpl(const Security* pSecurity,
										 AddressBook* pAddressBook) :
	pSecurity_(pSecurity),
	pAddressBook_(pAddressBook)
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
		return 0;
	
	const AddressBookEntry::AddressList& l = pEntry->getAddresses();
	for (AddressBookEntry::AddressList::const_iterator itA = l.begin(); itA != l.end(); ++itA) {
		const AddressBookAddress* pAddress = *itA;
		if (_wcsicmp(pAddress->getAddress(), pwszAddress) == 0)
			return pSecurity_->getCertificate(pAddress->getCertificate());
	}
	return 0;
}
