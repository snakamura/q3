/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
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

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * MessageComposer
 *
 */

qm::MessageComposer::MessageComposer(bool bDraft, Document* pDocument,
	Profile* pProfile, HWND hwnd, FolderModel* pFolderModel) :
	bDraft_(bDraft),
	pDocument_(pDocument),
	pProfile_(pProfile),
	hwnd_(hwnd),
	pFolderModel_(pFolderModel)
{
}

qm::MessageComposer::~MessageComposer()
{
}

QSTATUS qm::MessageComposer::compose(Account* pAccount,
	SubAccount* pSubAccount, Message* pMessage, unsigned int nFlags) const
{
	assert(pAccount || pFolderModel_);
	assert((!pAccount && !pSubAccount) ||
		(pAccount && !pSubAccount) || (pAccount && pSubAccount));
	assert(pMessage);
	
	DECLARE_QSTATUS();
	
	if (!pAccount) {
		assert(pFolderModel_);
		
		UnstructuredParser account(&status);
		CHECK_QSTATUS();
		Part::Field f;
		status = pMessage->getField(L"X-QMAIL-Account", &account, &f);
		CHECK_QSTATUS();
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
		return QSTATUS_FAIL;
	if (!pSubAccount) {
		UnstructuredParser subaccount(&status);
		CHECK_QSTATUS();
		Part::Field f;
		status = pMessage->getField(L"X-QMAIL-SubAccount", &subaccount, &f);
		CHECK_QSTATUS();
		if (f == Part::FIELD_EXIST)
			pSubAccount = pAccount->getSubAccount(subaccount.getValue());
		else
			pSubAccount = pAccount->getCurrentSubAccount();
	}
	
	Folder* pFolder = pAccount->getFolderByFlag(
		bDraft_ ? Folder::FLAG_DRAFTBOX : Folder::FLAG_OUTBOX);
	if (!pFolder || pFolder->getType() != Folder::TYPE_NORMAL)
		return QSTATUS_FAIL;
	
	bool bResent = false;
	status = PartUtil(*pMessage).isResent(&bResent);
	CHECK_QSTATUS();
	
	AddressParser from(pSubAccount->getSenderName(),
		pSubAccount->getSenderAddress(), &status);
	CHECK_QSTATUS();
	status = pMessage->setField(bResent ? L"Resent-From" : L"From", from);
	CHECK_QSTATUS();
	
	Time time(Time::getCurrentTime());
	DateParser date(time, &status);
	CHECK_QSTATUS();
	status = pMessage->setField(bResent ? L"Resent-Date" : L"Date", date);
	CHECK_QSTATUS();
	
	if (pSubAccount->isAddMessageId()) {
		WCHAR wsz[256];
		swprintf(wsz, L"%u%04d%02d%02d%02d%02d%02d",
			::GetCurrentProcessId(), time.wYear, time.wMonth,
			time.wDay, time.wHour, time.wMinute, time.wSecond);
		
		string_ptr<WSTRING> wstrMessageId(concat(
			wsz, pSubAccount->getSenderAddress()));
		if (!wstrMessageId.get())
			return QSTATUS_OUTOFMEMORY;
		
		MessageIdParser messageId(wstrMessageId.get(), &status);
		CHECK_QSTATUS();
		status = pMessage->setField(bResent ? L"Resent-Message-Id" : L"Message-Id", messageId);
		CHECK_QSTATUS();
	}
	
	const WCHAR* pwszMacro = 0;
	UnstructuredParser macro(&status);
	CHECK_QSTATUS();
	Part::Field f;
	status = pMessage->getField(L"X-QMAIL-Macro", &macro, &f);
	CHECK_QSTATUS();
	if (f == Part::FIELD_EXIST)
		pwszMacro = macro.getValue();
	status = pMessage->removeField(L"X-QMAIL-Macro");
	CHECK_QSTATUS();
	
	string_ptr<WSTRING> wstrVersion;
	status = Application::getApplication().getVersion(true, &wstrVersion);
	CHECK_QSTATUS();
	UnstructuredParser mailer(wstrVersion.get(), L"utf-8", &status);
	CHECK_QSTATUS();
	status = pMessage->replaceField(L"X-Mailer", mailer);
	CHECK_QSTATUS();
	
	status = pMessage->sortHeader();
	CHECK_QSTATUS();
	
	const Security* pSecurity = pDocument_->getSecurity();
	const SMIMEUtility* pSMIMEUtility = pSecurity->getSMIMEUtility();
	if (pSMIMEUtility && nFlags != 0) {
		SMIMECallbackImpl callback(pSecurity,
			pDocument_->getAddressBook(), &status);
		CHECK_QSTATUS();
		
		if (nFlags & FLAG_SIGN) {
			// TODO
			// Read from profile whether make multipart/signed or not
			bool bMultipart = false;
			Certificate* pCertificate = pSubAccount->getCertificate();
			PrivateKey* pPrivateKey = pSubAccount->getPrivateKey();
			if (pCertificate && pPrivateKey) {
				string_ptr<STRING> strMessage;
				status = pSMIMEUtility->sign(pMessage, bMultipart,
					pPrivateKey, pCertificate, &callback, &strMessage);
				CHECK_QSTATUS();
				status = pMessage->create(strMessage.get(),
					-1, Message::FLAG_NONE);
				CHECK_QSTATUS();
			}
		}
		if (nFlags & FLAG_ENCRYPT) {
			std::auto_ptr<Cipher> pCipher;
			status = CryptoUtil<Cipher>::getInstance(L"des3", &pCipher);
			CHECK_QSTATUS();
			string_ptr<STRING> strMessage;
			status = pSMIMEUtility->encrypt(pMessage,
				pCipher.get(), &callback, &strMessage);
			CHECK_QSTATUS();
			status = pMessage->create(strMessage.get(),
				-1, Message::FLAG_NONE);
			CHECK_QSTATUS();
		}
	}
	
	status = pAccount->appendMessage(static_cast<NormalFolder*>(pFolder), *pMessage,
		MessageHolder::FLAG_SEEN | (bDraft_ ? MessageHolder::FLAG_DRAFT : 0));
	CHECK_QSTATUS();
	status = pFolder->saveMessageHolders();
	CHECK_QSTATUS();
	
	if (pwszMacro) {
		MacroParser parser(MacroParser::TYPE_MESSAGE, &status);
		CHECK_QSTATUS();
		Macro* p = 0;
		status = parser.parse(pwszMacro, &p);
		CHECK_QSTATUS();
		std::auto_ptr<Macro> pMacro(p);
		
		MacroContext::Init init = {
			0,
			0,
			pAccount,
			pDocument_,
			hwnd_,
			pProfile_,
			false,
			0,
			0
		};
		MacroContext context(init, &status);
		CHECK_QSTATUS();
		
		MacroValuePtr pValue;
		status = pMacro->value(&context, &pValue);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * AddressBook::SMIMECallbackImpl
 *
 */

qm::SMIMECallbackImpl::SMIMECallbackImpl(const Security* pSecurity,
	AddressBook* pAddressBook, QSTATUS* pstatus) :
	pSecurity_(pSecurity),
	pAddressBook_(pAddressBook)
{
}

qm::SMIMECallbackImpl::~SMIMECallbackImpl()
{
}

QSTATUS qm::SMIMECallbackImpl::getContent(Part* pPart, STRING* pstrContent)
{
	assert(pPart);
	assert(pstrContent);
	
	DECLARE_QSTATUS();
	
	const WCHAR* pwszFields[] = {
		L"X-QMAIL-Signature",
		L"X-QMAIL-SubAccount",
		L"X-QMAIL-EnvelopeFrom"
	};
	for (int n = 0; n < countof(pwszFields); ++n) {
		status = pPart->removeField(pwszFields[n]);
		CHECK_QSTATUS();
	}
	
	status = pPart->getContent(pstrContent);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::SMIMECallbackImpl::getCertificate(
	const WCHAR* pwszAddress, Certificate** ppCertificate)
{
	assert(pwszAddress);
	assert(ppCertificate);
	
	DECLARE_QSTATUS();
	
	*ppCertificate = 0;
	
	const AddressBookEntry* pEntry = 0;
	status = pAddressBook_->getEntry(pwszAddress, &pEntry);
	CHECK_QSTATUS();
	
	const WCHAR* pwszCertificate = 0;
	
	bool bEnd = false;
	const AddressBookEntry::AddressList& l = pEntry->getAddresses();
	AddressBookEntry::AddressList::const_iterator itA = l.begin();
	while (itA != l.end() && !bEnd) {
		const AddressBookAddress* pAddress = *itA;
		if (_wcsicmp(pAddress->getAddress(), pwszAddress) == 0) {
			pwszCertificate = pAddress->getCertificate();
			bEnd = true;
		}
		++itA;
	}
	
	if (pwszCertificate) {
		status = pSecurity_->getCertificate(pwszCertificate, ppCertificate);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}
