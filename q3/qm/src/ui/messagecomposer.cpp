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
	
	AddressParser from(pSubAccount->getSenderName(),
		pSubAccount->getSenderAddress(), &status);
	CHECK_QSTATUS();
	status = pMessage->setField(L"From", from);
	CHECK_QSTATUS();
	
	Time time(Time::getCurrentTime());
	DateParser date(time, &status);
	CHECK_QSTATUS();
	status = pMessage->setField(L"Date", date);
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
		status = pMessage->setField(L"Message-Id", messageId);
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
	
	const SMIMEUtility* pSMIMEUtility =
		pDocument_->getSecurity()->getSMIMEUtility();
	if (pSMIMEUtility) {
		UnstructuredParser signature(&status);
		CHECK_QSTATUS();
		UnstructuredParser subaccount(&status);
		CHECK_QSTATUS();
		struct
		{
			const WCHAR* pwszName_;
			FieldParser* pField_;
			Part::Field field_;
		} fields[] = {
			{ L"X-QMAIL-Signature",		&signature	},
			{ L"X-QMAIL-SubAccount",	&subaccount	}
		};
		if (nFlags) {
			for (int n = 0; n < countof(fields); ++n) {
				status = pMessage->getField(fields[n].pwszName_,
					fields[n].pField_, &fields[n].field_);
				CHECK_QSTATUS();
				status = pMessage->removeField(fields[n].pwszName_);
				CHECK_QSTATUS();
			}
		}
		
		if (nFlags & FLAG_SIGN) {
			// TODO
			// Read from profile whether make multipart/signed or not
			bool bMultipart = false;
			Certificate* pCertificate = pSubAccount->getCertificate();
			PrivateKey* pPrivateKey = pSubAccount->getPrivateKey();
			if (pCertificate && pPrivateKey) {
				string_ptr<STRING> strMessage;
				status = pSMIMEUtility->sign(*pMessage, bMultipart,
					pPrivateKey, pCertificate, &strMessage);
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
			AddressBook* pAddressBook = pDocument_->getAddressBook();
			SMIMECallback* pCallback = pAddressBook->getSMIMECallback();
			string_ptr<STRING> strMessage;
			status = pSMIMEUtility->encrypt(*pMessage,
				pCipher.get(), pCallback, &strMessage);
			CHECK_QSTATUS();
			status = pMessage->create(strMessage.get(),
				-1, Message::FLAG_NONE);
			CHECK_QSTATUS();
		}
		
		if (nFlags) {
			for (int n = 0; n < countof(fields); ++n) {
				if (fields[n].field_ == Part::FIELD_EXIST) {
					status = pMessage->setField(
						fields[n].pwszName_, *fields[n].pField_);
					CHECK_QSTATUS();
				}
			}
		}
	}
	
	status = static_cast<NormalFolder*>(pFolder)->appendMessage(*pMessage,
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
