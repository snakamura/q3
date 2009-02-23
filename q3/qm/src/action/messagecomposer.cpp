/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#pragma warning(disable:4786)

#include <qmaccount.h>
#include <qmapplication.h>
#include <qmdocument.h>
#include <qmfolder.h>
#include <qmmacro.h>
#include <qmmessage.h>
#include <qmsecurity.h>

#include <qscrypto.h>
#include <qsinit.h>
#include <qslog.h>
#include <qsregex.h>

#include "messagecomposer.h"
#include "../model/addressbook.h"
#include "../model/message.h"
#include "../model/recentaddress.h"
#include "../pgp/pgp.h"
#include "../uimodel/folderselectionmodel.h"
#include "../uimodel/securitymodel.h"

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
									 AccountSelectionModel* pAccountSelectionModel,
									 SecurityModel* pSecurityModel) :
	bDraft_(bDraft),
	pDocument_(pDocument),
	pPasswordManager_(pPasswordManager),
	pProfile_(pProfile),
	hwnd_(hwnd),
	pAccountSelectionModel_(pAccountSelectionModel),
	pSecurityModel_(pSecurityModel)
{
	assert(pDocument);
	assert(pPasswordManager);
	assert(pProfile);
	assert(hwnd);
	assert(pSecurityModel);
}

qm::MessageComposer::~MessageComposer()
{
}

bool qm::MessageComposer::compose(Message* pMessage,
								  unsigned int nMessageSecurity,
								  Account* pAccount,
								  SubAccount* pSubAccount,
								  MessagePtr* pptr) const
{
	assert(pAccount || pAccountSelectionModel_);
	assert((!pAccount && !pSubAccount) ||
		(pAccount && !pSubAccount) || (pAccount && pSubAccount));
	assert(pMessage);
	
	if (!pAccount) {
		pAccount = getAccount(*pMessage);
		if (!pAccount)
			return false;
	}
	if (!pSubAccount) {
		pSubAccount = getSubAccount(pAccount, *pMessage);
		if (!pSubAccount)
			return false;
	}
	
	NormalFolder* pFolder = 0;
	UnstructuredParser outbox;
	if (pMessage->getField(L"X-QMAIL-Outbox", &outbox) == Part::FIELD_EXIST) {
		Folder* p = pAccount->getFolder(outbox.getValue());
		if (p && p->getType() == Folder::TYPE_NORMAL)
			pFolder = static_cast<NormalFolder*>(p);
	}
	if (!pFolder)
		pFolder = static_cast<NormalFolder*>(pAccount->getFolderByBoxFlag(
			bDraft_ ? Folder::FLAG_DRAFTBOX : Folder::FLAG_OUTBOX));
	if (!pFolder)
		return false;
	
	bool bResent = PartUtil(*pMessage).isResent();
	
	AddressParser from(pSubAccount->getSenderName(),
		pSubAccount->getSenderAddress());
	if (!pMessage->setField(bResent ? L"Resent-From" : L"From", from))
		return false;
	
	const WCHAR* pwszReplyTo = pSubAccount->getReplyTo();
	if (pwszReplyTo && *pwszReplyTo) {
		AddressParser replyTo(0, pwszReplyTo);
		if (!pMessage->setField(L"Reply-To", replyTo))
			return false;
	}
	
	Time time(Time::getCurrentTime());
	DateParser date(time);
	if (!pMessage->setField(bResent ? L"Resent-Date" : L"Date", date))
		return false;
	
	if (pSubAccount->isAddMessageId()) {
		WCHAR wsz[256];
		_snwprintf(wsz, countof(wsz), L"%u%04d%02d%02d%02d%02d%02d",
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
	
	bool bWithOSVersion = pProfile_->getInt(L"Global", L"XMailerWithOSVersion") != 0;
	wstring_ptr wstrVersion(Application::getApplication().getVersion(L' ', bWithOSVersion));
	UnstructuredParser mailer(wstrVersion.get(), L"utf-8");
	if (!pMessage->setField(L"X-Mailer", mailer))
		return false;
	
	if (!pMessage->sortHeader())
		return false;
	
	if (!processSMIME(pMessage, nMessageSecurity, pSubAccount))
		return false;
	if (!processPGP(pMessage, nMessageSecurity, pSubAccount))
		return false;
	
	if (!pAccount->appendMessage(static_cast<NormalFolder*>(pFolder), *pMessage,
		MessageHolder::FLAG_SEEN | (bDraft_ ? MessageHolder::FLAG_DRAFT : 0),
		0, Account::OPFLAG_ACTIVE, 0, pptr))
		return false;
	if (!pAccount->flushMessageStore() || !pFolder->saveMessageHolders())
		return false;
	
	for (int n = 0; n < countof(pwszMacro); ++n) {
		if (pwszMacro[n]) {
			std::auto_ptr<Macro> pMacro(MacroParser().parse(pwszMacro[n]));
			if (!pMacro.get())
				return false;
			
			MacroContext context(0, 0, pAccount, pSubAccount,
				MessageHolderList(), pFolder, pDocument_, 0, hwnd_, pProfile_, 0,
				MacroContext::FLAG_UITHREAD | MacroContext::FLAG_UI | MacroContext::FLAG_MODIFY,
				pSecurityModel_->getSecurityMode(), 0, 0);
			MacroValuePtr pValue(pMacro->value(&context));
		}
	}
	
	RecentAddress* pRecentAddress = pDocument_->getRecentAddress();
	pRecentAddress->add(*pMessage);
	
	return true;
}

bool qm::MessageComposer::compose(const WCHAR* pwszMessage,
								  size_t nLen,
								  unsigned int nMessageSecurity) const
{
	assert(pwszMessage);
	assert(pAccountSelectionModel_);
	
	if (nLen == -1)
		nLen = wcslen(pwszMessage);
	
	BMFindString<WSTRING> bmfs(L"\n\n");
	const WCHAR* pBody = bmfs.find(pwszMessage);
	size_t nHeaderLen = pBody ? pBody - pwszMessage + 2 : nLen;
	MessageCreator headerCreator;
	std::auto_ptr<Message> pHeader(headerCreator.createMessage(pwszMessage, nHeaderLen));
	if (!pHeader.get())
		return false;
	
	Account* pAccount = getAccount(*pHeader.get());
	if (!pAccount)
		return false;
	SubAccount* pSubAccount = getSubAccount(pAccount, *pHeader.get());
	if (!pSubAccount)
		return false;
	
	const unsigned int nFlags = MessageCreator::FLAG_ADDCONTENTTYPE |
		MessageCreator::FLAG_EXPANDALIAS |
		MessageCreator::FLAG_EXTRACTATTACHMENT |
		MessageCreator::FLAG_ENCODETEXT |
		MessageCreator::FLAG_ADDSIGNATURE;
	wstring_ptr wstrExcludePattern(pProfile_->getString(L"Global", L"ExcludeArchive"));
	const WCHAR* pwszTempDir = Application::getApplication().getTemporaryFolder();
	MessageCreator creator(nFlags, pSecurityModel_->getSecurityMode(),
		pSubAccount->getTransferEncodingFor8Bit(), pDocument_->getURIResolver(),
		pDocument_->getSignatureManager(), pAccount, wstrExcludePattern.get(), pwszTempDir);
	std::auto_ptr<Message> pMessage(creator.createMessage(pwszMessage, nLen));
	if (!pMessage.get())
		return false;
	
	return compose(pMessage.get(), nMessageSecurity, 0, 0, 0);
}

bool qm::MessageComposer::compose(const WCHAR* pwszPath,
								  unsigned int nMessageSecurity) const
{
	assert(pwszPath);
	assert(pAccountSelectionModel_);
	
	FileInputStream stream(pwszPath);
	if (!stream)
		return false;
	BufferedInputStream bufferedStream(&stream, false);
	InputStreamReader reader(&bufferedStream, false, getSystemEncoding());
	if (!reader)
		return false;
	
	XStringBuffer<WXSTRING> buf;
	const size_t nBufferSize = 8192;
	while (true) {
		XStringBufferLock<WXSTRING> lock(&buf, nBufferSize);
		if (!lock.get())
			return false;
		
		size_t nRead = reader.read(lock.get(), nBufferSize);
		if (nRead == -1)
			return false;
		else if (nRead == 0)
			break;
		
		lock.unlock(nRead);
	}
	if (!reader.close())
		return false;
	
	if (buf.getLength() != 0) {
		if (!compose(buf.getCharArray(), buf.getLength(), nMessageSecurity))
			return false;
	}
	
	return true;
}

bool qm::MessageComposer::isAttachmentArchiving(const WCHAR* pwszFileOrURI) const
{
	assert(pwszFileOrURI);
	
	if (MessageCreator::isAttachmentURI(pwszFileOrURI))
		return false;
	
	wstring_ptr wstrPattern(pProfile_->getString(L"Global", L"ExcludeArchive"));
	return !*wstrPattern.get() ||
		Regex::search(wstrPattern.get(), pwszFileOrURI).first == 0;
}

Account* qm::MessageComposer::getAccount(const Message& header) const
{
	assert(pAccountSelectionModel_);
	
	UnstructuredParser account;
	if (header.getField(L"X-QMAIL-Account", &account) == Part::FIELD_EXIST)
		return pDocument_->getAccount(account.getValue());
	else
		return pAccountSelectionModel_->getAccount();
}

SubAccount* qm::MessageComposer::getSubAccount(Account* pAccount,
											   const Message& header) const
{
	if (!pAccount)
		return 0;
	
	UnstructuredParser subaccount;
	if (header.getField(L"X-QMAIL-SubAccount", &subaccount) == Part::FIELD_EXIST)
		return pAccount->getSubAccount(subaccount.getValue());
	else
		return pAccount->getCurrentSubAccount();
}

bool qm::MessageComposer::processSMIME(Message* pMessage,
									   unsigned int nMessageSecurity,
									   SubAccount* pSubAccount) const
{
	Log log(InitThread::getInitThread().getLogger(), L"qm::MessageComposer");
	
	if (!(nMessageSecurity & MESSAGESECURITY_SMIMESIGN) &&
		!(nMessageSecurity & MESSAGESECURITY_SMIMEENCRYPT))
		return true;
	
	const Security* pSecurity = pDocument_->getSecurity();
	const SMIMEUtility* pSMIMEUtility = pSecurity->getSMIMEUtility();
	if (!pSMIMEUtility) {
		log.error(L"S/MIME is not supported.");
		return false;
	}
	
	if (nMessageSecurity & MESSAGESECURITY_SMIMESIGN) {
		bool bMultipart = !(nMessageSecurity & MESSAGESECURITY_SMIMEENCRYPT) &&
			(nMessageSecurity & MESSAGESECURITY_SMIMEMULTIPARTSIGNED);
		std::auto_ptr<Certificate> pCertificate(pSubAccount->getCertificate(pPasswordManager_));
		if (!pCertificate.get()) {
			log.errorf(L"Could not get the certificate for the account: %s/%s.",
				pSubAccount->getAccount()->getName(), pSubAccount->getName());
			return false;
		}
		std::auto_ptr<PrivateKey> pPrivateKey(pSubAccount->getPrivateKey(pPasswordManager_));
		if (!pPrivateKey.get()) {
			log.errorf(L"Could not get the private key for the account: %s/%s.",
				pSubAccount->getAccount()->getName(), pSubAccount->getName());
			return false;
		}
		xstring_size_ptr strMessage(pSMIMEUtility->sign(pMessage,
			bMultipart, pPrivateKey.get(), pCertificate.get()));
		if (!strMessage.get()) {
			log.error(L"Failed to sign with S/MIME.");
			return false;
		}
		if (!pMessage->create(strMessage.get(), strMessage.size(), Message::FLAG_NONE))
			return false;
	}
	if (nMessageSecurity & MESSAGESECURITY_SMIMEENCRYPT) {
		std::auto_ptr<Certificate> pSelfCertificate;
		if (nMessageSecurity & MESSAGESECURITY_SMIMEENCRYPTFORSELF)
			pSelfCertificate = pSubAccount->getCertificate(pPasswordManager_);
		SMIMECallbackImpl callback(pSecurity, pDocument_->getAddressBook(), pSelfCertificate.get());
		
		std::auto_ptr<Cipher> pCipher(Cipher::getInstance(L"des3"));
		xstring_size_ptr strMessage(pSMIMEUtility->encrypt(
			pMessage, pCipher.get(), &callback));
		if (!strMessage.get()) {
			log.error(L"Failed to encrypt with S/MIME.");
			return false;
		}
		if (!pMessage->create(strMessage.get(), strMessage.size(), Message::FLAG_NONE))
			return false;
	}
	
	return true;
}

bool qm::MessageComposer::processPGP(Message* pMessage,
									 unsigned int nMessageSecurity,
									 SubAccount* pSubAccount) const
{
	Log log(InitThread::getInitThread().getLogger(), L"qm::MessageComposer");
	
	if (!(nMessageSecurity & MESSAGESECURITY_PGPSIGN) &&
		!(nMessageSecurity & MESSAGESECURITY_PGPENCRYPT))
		return true;
	
	const Security* pSecurity = pDocument_->getSecurity();
	const PGPUtility* pPGPUtility = pSecurity->getPGPUtility();
	if (!pPGPUtility) {
		log.error(L"PGP is not supported.");
		return false;
	}
	
	const WCHAR* pwszUserId = pSubAccount->getSenderAddress();
	PGPPassphraseCallbackImpl passphraseCallback(pPasswordManager_, pwszUserId);
	
	bool bMime = (nMessageSecurity & MESSAGESECURITY_PGPMIME) != 0;
	if (nMessageSecurity & MESSAGESECURITY_PGPSIGN &&
		nMessageSecurity & MESSAGESECURITY_PGPENCRYPT) {
		const WCHAR* pwszUserId = pSubAccount->getSenderAddress();
		xstring_size_ptr strMessage(pPGPUtility->signAndEncrypt(
			pMessage, bMime, pwszUserId, &passphraseCallback));
		if (!strMessage.get()) {
			log.error(L"Failed to sign and encrypt with PGP.");
			return false;
		}
		if (!pMessage->create(strMessage.get(), strMessage.size(), Message::FLAG_NONE))
			return false;
	}
	else if (nMessageSecurity & MESSAGESECURITY_PGPSIGN) {
		const WCHAR* pwszUserId = pSubAccount->getSenderAddress();
		xstring_size_ptr strMessage(pPGPUtility->sign(pMessage,
			bMime, pwszUserId, &passphraseCallback));
		if (!strMessage.get()) {
			log.error(L"Failed to sign with PGP.");
			return false;
		}
		if (!pMessage->create(strMessage.get(), strMessage.size(), Message::FLAG_NONE))
			return false;
	}
	else if (nMessageSecurity & MESSAGESECURITY_PGPENCRYPT) {
		xstring_size_ptr strMessage(pPGPUtility->encrypt(pMessage, bMime));
		if (!strMessage.get()) {
			log.error(L"Failed to encrypt with PGP.");
			return false;
		}
		if (!pMessage->create(strMessage.get(), strMessage.size(), Message::FLAG_NONE))
			return false;
	}
	
	passphraseCallback.save();
	
	return true;
}


/****************************************************************************
 *
 * SMIMECallbackImpl
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
