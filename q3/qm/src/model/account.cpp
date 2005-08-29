/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#pragma warning(disable:4786)

#include <qmaccount.h>
#include <qmfilenames.h>
#include <qmfolder.h>
#include <qmmessageholder.h>
#include <qmmessageoperation.h>
#include <qmpassword.h>
#include <qmpgp.h>
#include <qmprotocoldriver.h>
#include <qmsecurity.h>
#include <qmjunk.h>

#include <qsconv.h>
#include <qsfile.h>
#include <qsosutil.h>
#include <qsprofile.h>
#include <qsstream.h>
#include <qsthread.h>

#include <algorithm>

#include "account.h"
#include "messageindex.h"
#include "messagestore.h"
#include "undo.h"
#include "../junk/junk.h"
#include "../util/confighelper.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * AccountImpl
 *
 */

class qm::AccountImpl
{
public:
	class CallByFolderCallback
	{
	public:
		virtual bool callback(NormalFolder* pFolder,
							  const MessageHolderList& l) = 0;
	};

public:
	typedef std::vector<AccountHandler*> AccountHandlerList;
	typedef std::vector<MessageHolderHandler*> MessageHolderHandlerList;

public:
	bool loadFolders();
	bool saveFolders() const;
	
	bool loadSubAccounts();
	bool saveSubAccounts(bool bForce) const;
	
	bool getMessage(MessageHolder* pmh,
					unsigned int nFlags,
					Message* pMessage);
	bool appendMessage(NormalFolder* pFolder,
					   const CHAR* pszMessage,
					   size_t nLen,
					   const Message& msgHeader,
					   unsigned int nFlags,
					   unsigned int nSize,
					   UndoItemList* pUndoItemList,
					   MessagePtr* pptr);
	bool removeMessages(NormalFolder* pFolder,
						const MessageHolderList& l,
						bool bDirect,
						MessageOperationCallback* pCallback,
						UndoItemList* pUndoItemList);
	bool copyMessages(NormalFolder* pFolderFrom,
					  NormalFolder* pFolderTo,
					  const MessageHolderList& l,
					  unsigned int nFlags,
					  MessageOperationCallback* pCallback,
					  UndoItemList* pUndoItemList);
	bool setMessagesFlags(NormalFolder* pFolder,
						  const MessageHolderList& l,
						  unsigned int nFlags,
						  unsigned int nMask,
						  UndoItemList* pUndoItemList);
	
	bool getDataList(MessageStore::DataList* pList) const;
	
	bool processSMIME(const SMIMEUtility* pSMIMEUtility,
					  SMIMEUtility::Type type,
					  Message* pMessage);
	bool processPGP(const PGPUtility* pPGPUtility,
					PGPUtility::Type type,
					Message* pMessage);
	
	void fireCurrentSubAccountChanged();
	void fireSubAccountListChanged();
	void fireFolderListChanged(const FolderListChangedEvent& event);
	void fireAccountDestroyed();

public:
	static bool createTemporaryMessage(MessageHolder* pmh,
									   Message* pMessage);
	static bool callByFolder(const MessageHolderList& l,
							 CallByFolderCallback* pCallback);

private:
	bool createDefaultFolders();

public:
	Account* pThis_;
	wstring_ptr wstrName_;
	wstring_ptr wstrPath_;
	wstring_ptr wstrClass_;
	wstring_ptr wstrType_[Account::HOST_SIZE];
	wstring_ptr wstrMessageStorePath_;
	bool bMultiMessageStore_;
	bool bStoreDecodedMessage_;
	Profile* pProfile_;
	const Security* pSecurity_;
	PasswordManager* pPasswordManager_;
	JunkFilter* pJunkFilter_;
	Account::SubAccountList listSubAccount_;
	SubAccount* pCurrentSubAccount_;
	Account::FolderList listFolder_;
	std::auto_ptr<MessageStore> pMessageStore_;
	std::auto_ptr<MessageIndex> pMessageIndex_;
	std::auto_ptr<ProtocolDriver> pProtocolDriver_;
	AccountHandlerList listAccountHandler_;
	MessageHolderHandlerList listMessageHolderHandler_;
	CriticalSection csLock_;
	bool bDeletedAsSeen_;
#ifndef NDEBUG
	unsigned int nLock_;
#endif
};

bool qm::AccountImpl::loadFolders()
{
	wstring_ptr wstrPath(concat(wstrPath_.get(), L"\\", FileNames::FOLDERS_XML));
	
	W2T(wstrPath.get(), ptszPath);
	if (::GetFileAttributes(ptszPath) != 0xffffffff) {
		FileInputStream stream(wstrPath.get());
		if (!stream)
			return false;
		BufferedInputStream bufferedStream(&stream, false);
		XMLReader reader;
		FolderContentHandler handler(pThis_, &listFolder_);
		reader.setContentHandler(&handler);
		InputSource source(&bufferedStream);
		if (!reader.parse(&source))
			return false;
	}
	else {
		if (!createDefaultFolders())
			return false;
	}
	
	if (!pThis_->getFolderByBoxFlag(Folder::FLAG_SEARCHBOX)) {
		std::auto_ptr<QueryFolder> pFolder(new QueryFolder(
			pThis_->generateFolderId(), L"Search", L'/',
			Folder::FLAG_LOCAL | Folder::FLAG_SEARCHBOX,
			0, 0, L"macro", L"@False()", 0, false, 0, pThis_));
		listFolder_.push_back(pFolder.get());
		pFolder.release();
	}
	
	return true;
}

bool qm::AccountImpl::saveFolders() const
{
	wstring_ptr wstrPath(concat(wstrPath_.get(), L"\\", FileNames::FOLDERS_XML));
	Account::FolderList listFolder(listFolder_);
	std::sort(listFolder.begin(), listFolder.end(), FolderLess());
	return ConfigSaver<const Account::FolderList&, FolderWriter>::save(listFolder, wstrPath.get());
}

bool qm::AccountImpl::loadSubAccounts()
{
	wstring_ptr wstrPath(concat(wstrPath_.get(), L"\\", FileNames::ACCOUNT_XML));
	
	std::auto_ptr<XMLProfile> pAccountProfile(new XMLProfile(wstrPath.get()));
	if (!pAccountProfile->load())
		return false;
	
	pProfile_ = pAccountProfile.get();
	std::auto_ptr<SubAccount> pDefaultSubAccount(
		new SubAccount(pThis_, pAccountProfile, L""));
	
	listSubAccount_.push_back(pDefaultSubAccount.get());
	pDefaultSubAccount.release();
	
	ConcatW c[] = {
		{ wstrPath_.get(),		-1	},
		{ L"\\",				1	},
		{ FileNames::ACCOUNT,	-1	},
		{ L"_*",				2	},
		{ FileNames::XML_EXT,	-1	}
	};
	wstring_ptr wstrFind(concat(c, countof(c)));
	W2T(wstrFind.get(), ptszFind);
	
	size_t nOffset = wcslen(FileNames::ACCOUNT) + 1;
	
	WIN32_FIND_DATA fd;
	AutoFindHandle hFind(::FindFirstFile(ptszFind, &fd));
	if (hFind.get()) {
		do {
			T2W(fd.cFileName, pwszFileName);
			if (wcscmp(pwszFileName, FileNames::ACCOUNT_XML) == 0)
				continue;
			
			const WCHAR* p = wcschr(pwszFileName, L'.');
			assert(p);
			wstring_ptr wstrName(allocWString(
				pwszFileName + nOffset, p - pwszFileName - nOffset));
			wstring_ptr wstrPath(concat(
				wstrPath_.get(), L"\\", pwszFileName));
			
			std::auto_ptr<XMLProfile> pProfile(new XMLProfile(wstrPath.get()));
			if (pProfile->load()) {
				std::auto_ptr<SubAccount> pSubAccount(
					new SubAccount(pThis_, pProfile, wstrName.get()));
				listSubAccount_.push_back(pSubAccount.get());
				pSubAccount.release();
			}
		} while (::FindNextFile(hFind.get(), &fd));
	}
	
	return true;
}

bool qm::AccountImpl::saveSubAccounts(bool bForce) const
{
	for (Account::SubAccountList::const_iterator it = listSubAccount_.begin(); it != listSubAccount_.end(); ++it) {
		if (!(*it)->save(bForce))
			return false;
	}
	return true;
}

bool qm::AccountImpl::getMessage(MessageHolder* pmh,
								 unsigned int nFlags,
								 Message* pMessage)
{
	assert(pmh);
	assert(pMessage);
	assert((nFlags & Account::GETMESSAGEFLAG_METHOD_MASK) != 0);
	assert(pThis_->isLocked());
	
#ifndef NDEBUG
	Message::Flag flag = pMessage->getFlag();
	switch (nFlags & Account::GETMESSAGEFLAG_METHOD_MASK) {
	case Account::GETMESSAGEFLAG_ALL:
		assert(flag != Message::FLAG_NONE);
		break;
	case Account::GETMESSAGEFLAG_HEADER:
		assert(flag != Message::FLAG_NONE &&
			flag != Message::FLAG_HEADERONLY &&
			flag != Message::FLAG_TEXTONLY &&
			flag != Message::FLAG_HTMLONLY);
		break;
	case Account::GETMESSAGEFLAG_TEXT:
		assert(flag != Message::FLAG_NONE &&
			flag != Message::FLAG_TEXTONLY &&
			flag != Message::FLAG_HTMLONLY);
		break;
	case Account::GETMESSAGEFLAG_HTML:
		assert(flag != Message::FLAG_NONE &&
			flag != Message::FLAG_HTMLONLY);
		break;
	case Account::GETMESSAGEFLAG_POSSIBLE:
		break;
	default:
		assert(false);
		break;
	}
#endif
	
	bool bLoadFromStore = false;
	Message::Flag msgFlag = Message::FLAG_NONE;
	unsigned int nPartialMask = pmh->getFlags() & MessageHolder::FLAG_PARTIAL_MASK;
	switch (nFlags & Account::GETMESSAGEFLAG_METHOD_MASK) {
	case Account::GETMESSAGEFLAG_ALL:
		bLoadFromStore = nPartialMask == 0;
		msgFlag = Message::FLAG_NONE;
		break;
	case Account::GETMESSAGEFLAG_HEADER:
		bLoadFromStore = nPartialMask == 0 ||
			nPartialMask == MessageHolder::FLAG_HTMLONLY ||
			nPartialMask == MessageHolder::FLAG_TEXTONLY ||
			nPartialMask == MessageHolder::FLAG_HEADERONLY;
		msgFlag = Message::FLAG_HEADERONLY;
		break;
	case Account::GETMESSAGEFLAG_TEXT:
		bLoadFromStore = nPartialMask == 0 ||
			nPartialMask == MessageHolder::FLAG_HTMLONLY ||
			nPartialMask == MessageHolder::FLAG_TEXTONLY;
		msgFlag = nPartialMask == 0 ? Message::FLAG_NONE :
			nPartialMask == MessageHolder::FLAG_HTMLONLY ?
			Message::FLAG_HTMLONLY : Message::FLAG_TEXTONLY;
		break;
	case Account::GETMESSAGEFLAG_HTML:
		bLoadFromStore = nPartialMask == 0 ||
			nPartialMask == MessageHolder::FLAG_HTMLONLY;
		msgFlag = nPartialMask == 0 ? Message::FLAG_NONE : Message::FLAG_HTMLONLY;
		break;
	case Account::GETMESSAGEFLAG_POSSIBLE:
		bLoadFromStore = true;
		msgFlag = nPartialMask == 0 ? Message::FLAG_NONE :
			nPartialMask == MessageHolder::FLAG_INDEXONLY ? Message::FLAG_TEMPORARY :
			nPartialMask == MessageHolder::FLAG_HEADERONLY ? Message::FLAG_HEADERONLY :
			nPartialMask == MessageHolder::FLAG_TEXTONLY ? Message::FLAG_TEXTONLY :
			/*nPartialMask == MessageHolder::FLAG_HTMLONLY ?*/ Message::FLAG_HTMLONLY/* : 0*/;
		break;
	default:
		assert(false);
		break;
	}
	if ((pmh->getFolder()->isFlag(Folder::FLAG_LOCAL) &&
		(!pProtocolDriver_->isSupport(Account::SUPPORT_LOCALFOLDERGETMESSAGE) ||
		!pmh->getFolder()->isFlag(Folder::FLAG_SYNCABLE))))
		bLoadFromStore = true;
	else if (pmh->isFlag(MessageHolder::FLAG_LOCAL))
		bLoadFromStore = true;
	
	bool bGet = false;
	bool bMadeSeen = false;
	if (!bLoadFromStore) {
		struct GetMessageCallbackImpl : public ProtocolDriver::GetMessageCallback
		{
			GetMessageCallbackImpl(MessageHolder* pmh,
								   Message* pMessage) :
				pmh_(pmh),
				pMessage_(pMessage),
				bGet_(false),
				bMadeSeen_(false)
			{
			}
			
			virtual bool message(const CHAR* pszMessage,
								 size_t nLen,
								 Message::Flag flag,
								 bool bMadeSeen)
			{
				bGet_ = flag != Message::FLAG_EMPTY;
				if (bGet_) {
					if (!pMessage_->create(pszMessage, nLen, flag))
						return false;
					
					NormalFolder* pFolder = pmh_->getFolder();
					if (pFolder->isFlag(Folder::FLAG_CACHEWHENREAD)) {
						Account* pAccount = pFolder->getAccount();
						if (!pAccount->updateMessage(pmh_, pszMessage, nLen))
							return false;
						
						unsigned int nFlag = 0;
						switch (flag) {
						case Message::FLAG_HEADERONLY:
							nFlag = MessageHolder::FLAG_HEADERONLY;
							break;
						case Message::FLAG_TEXTONLY:
							nFlag = MessageHolder::FLAG_TEXTONLY;
							break;
						case Message::FLAG_HTMLONLY:
							nFlag = MessageHolder::FLAG_HTMLONLY;
							break;
						}
						pmh_->setFlags(nFlag, MessageHolder::FLAG_PARTIAL_MASK);
					}
				}
				bMadeSeen_ = bMadeSeen;
				return true;
			}
			
			MessageHolder* pmh_;
			Message* pMessage_;
			bool bGet_;
			bool bMadeSeen_;
		} callback(pmh, pMessage);
		
		if (!pProtocolDriver_->getMessage(pmh, nFlags, &callback))
			return false;
		bGet = callback.bGet_;
		bMadeSeen = callback.bMadeSeen_;
		if (!bGet) {
			if (nFlags & Account::GETMESSAGEFLAG_NOFALLBACK)
				return false;
		}
	}
	
	if (!bGet) {
		const MessageHolder::MessageBoxKey& key = pmh->getMessageBoxKey();
		if (key.nOffset_ != -1) {
			unsigned int nLength =
				(nFlags & Account::GETMESSAGEFLAG_METHOD_MASK) == Account::GETMESSAGEFLAG_HEADER ?
				key.nHeaderLength_ : key.nLength_;
			if (!pMessageStore_->load(key.nOffset_, nLength, pMessage))
				return false;
			pMessage->setFlag(msgFlag);
		}
		else {
			if (pMessage->getFlag() == Message::FLAG_EMPTY) {
				if (!createTemporaryMessage(pmh, pMessage))
					return false;
			}
		}
	}
	
	if ((nFlags & Account::GETMESSAGEFLAG_MAKESEEN) &&
		!bMadeSeen &&
		!pmh->isFlag(MessageHolder::FLAG_SEEN)) {
		MessageHolderList l(1, pmh);
		if (!setMessagesFlags(pmh->getFolder(), l,
			MessageHolder::FLAG_SEEN, MessageHolder::FLAG_SEEN, 0))
			return false;
	}
	
	return true;
}

bool qm::AccountImpl::appendMessage(NormalFolder* pFolder,
									const CHAR* pszMessage,
									size_t nLen,
									const Message& msgHeader,
									unsigned int nFlags,
									unsigned int nSize,
									UndoItemList* pUndoItemList,
									MessagePtr* pptr)
{
	assert(pFolder);
	assert(pszMessage);
	assert((nFlags & ~MessageHolder::FLAG_USER_MASK) == 0);
	
	Lock<Account> lock(*pThis_);
	
	std::auto_ptr<UndoItem> pUndoItem;
	if (pFolder->isFlag(Folder::FLAG_LOCAL)) {
		MessageHolder* pmh = pThis_->storeMessage(pFolder,
			pszMessage, nLen, &msgHeader, -1, nFlags, nSize, false);
		if (!pmh)
			return false;
		if (pptr)
			*pptr = MessagePtr(pmh);
		if (pUndoItemList)
			pUndoItem.reset(new DeleteUndoItem(MessageHolderList(1, pmh)));
	}
	else {
		if (!pProtocolDriver_->appendMessage(pFolder, pszMessage, nLen, nFlags))
			return false;
	}
	if (pUndoItemList)
		pUndoItemList->add(pUndoItem);
	
	return true;
}

bool qm::AccountImpl::removeMessages(NormalFolder* pFolder,
									 const MessageHolderList& l,
									 bool bDirect,
									 MessageOperationCallback* pCallback,
									 UndoItemList* pUndoItemList)
{
	assert(pFolder);
	assert(pThis_->isLocked());
	assert(std::find_if(l.begin(), l.end(),
		std::not1(
			std::bind2nd(
				binary_compose_f_gx_hy(
					std::equal_to<Folder*>(),
					std::mem_fun(&MessageHolder::getFolder),
					std::identity<Folder*>()),
				pFolder))) == l.end());
	
	if (l.empty())
		return true;
	
	if (!bDirect)
		bDirect = pFolder->isFlag(Folder::FLAG_TRASHBOX) ||
			pFolder->isFlag(Folder::FLAG_JUNKBOX);
	
	NormalFolder* pTrash = 0;
	if (!bDirect)
		pTrash = static_cast<NormalFolder*>(
			pThis_->getFolderByBoxFlag(Folder::FLAG_TRASHBOX));
	
	if (pTrash) {
		if (!copyMessages(pFolder, pTrash, l, Account::COPYFLAG_MOVE, pCallback, pUndoItemList))
			return false;
	}
	else {
		if (pCallback)
			pCallback->setCount(l.size());
		
		if (pFolder->isFlag(Folder::FLAG_LOCAL)) {
			if (pCallback && l.size() > 1) {
				pCallback->setCancelable(false);
				pCallback->show();
			}
			
			if (!pThis_->unstoreMessages(l, pCallback))
				return false;
		}
		else {
			if (!pProtocolDriver_->removeMessages(pFolder, l))
				return false;
			
			if (pCallback)
				pCallback->step(l.size());
		}
		if (pUndoItemList)
			pUndoItemList->add(std::auto_ptr<UndoItem>());
	}
	
	return true;
}

bool qm::AccountImpl::copyMessages(NormalFolder* pFolderFrom,
								   NormalFolder* pFolderTo,
								   const MessageHolderList& l,
								   unsigned int nFlags,
								   MessageOperationCallback* pCallback,
								   UndoItemList* pUndoItemList)
{
	assert(pFolderFrom);
	assert(pFolderTo);
	assert(pThis_->isLocked());
	assert(std::find_if(l.begin(), l.end(),
		std::not1(
			std::bind2nd(
				binary_compose_f_gx_hy(
					std::equal_to<NormalFolder*>(),
					std::mem_fun(&MessageHolder::getFolder),
					std::identity<NormalFolder*>()),
				pFolderFrom))) == l.end());
	assert(pFolderFrom->getAccount() == pThis_);
	
	bool bMove = (nFlags & Account::COPYFLAG_MOVE) != 0;
	if (l.empty() || (bMove && pFolderFrom == pFolderTo))
		return true;
	
	if (nFlags & Account::COPYFLAG_MANAGEJUNK &&
		pJunkFilter_ &&
		pJunkFilter_->getFlags() & JunkFilter::FLAG_MANUALLEARN) {
		unsigned int nJunkOperation = 0;
		if (pFolderTo->isFlag(Folder::FLAG_JUNKBOX) &&
			!pFolderFrom->isFlag(Folder::FLAG_JUNKBOX))
			nJunkOperation = JunkFilter::OPERATION_ADDJUNK |
				(bMove ? JunkFilter::OPERATION_REMOVECLEAN : 0);
		else if (pFolderFrom->isFlag(Folder::FLAG_JUNKBOX) &&
			!pFolderTo->isFlag(Folder::FLAG_JUNKBOX))
			nJunkOperation = JunkFilter::OPERATION_ADDCLEAN |
				(bMove ? JunkFilter::OPERATION_REMOVEJUNK : 0);
		if (nJunkOperation != 0) {
			for (MessageHolderList::const_iterator it = l.begin(); it != l.end(); ++it)
				JunkFilterUtil::manage(pJunkFilter_, *it, nJunkOperation);
			pJunkFilter_->save(false);
		}
	}
	
	// TODO
	// Take care of local messages in remote folder
	
	if (pCallback)
		pCallback->setCount(l.size());
	
	Account* pAccountTo = pFolderTo->getAccount();
	
	bool bLocalCopy = true;
	if (bMove && pAccountTo == pThis_)
		bLocalCopy = pFolderFrom->isFlag(Folder::FLAG_LOCAL) !=
			pFolderTo->isFlag(Folder::FLAG_LOCAL);
	
	if (bLocalCopy) {
		if (pCallback && l.size() > 1)
			pCallback->show();
		
		for (MessageHolderList::size_type n = 0; n < l.size(); ++n) {
			MessageHolder* pmh = l[n];
			Message msg;
			if (!pmh->getMessage(Account::GETMESSAGEFLAG_ALL | Account::GETMESSAGEFLAG_NOFALLBACK,
				0, SECURITYMODE_NONE, &msg))
				return false;
			if (!pAccountTo->appendMessage(pFolderTo, msg,
				pmh->getFlags() & MessageHolder::FLAG_USER_MASK, pUndoItemList, 0))
				return false;
			
			if (pCallback) {
				if (n % 10 == 0 && pCallback->isCanceled())
					break;
				pCallback->step(1);
			}
		}
		if (bMove) {
			if (!removeMessages(pFolderFrom, l, true, 0, pUndoItemList))
				return false;
		}
	}
	else {
		std::auto_ptr<UndoItem> pUndoItem;
		if (pFolderFrom->isFlag(Folder::FLAG_LOCAL)) {
			assert(bMove);
			if (!pFolderFrom->moveMessages(l, pFolderTo))
				return false;
			if (pUndoItemList)
				pUndoItem.reset(new MoveUndoItem(l, pFolderFrom));
		}
		else {
			if (!pProtocolDriver_->copyMessages(l, pFolderFrom, pFolderTo, bMove))
				return false;
		}
		if (pUndoItemList)
			pUndoItemList->add(pUndoItem);
		
		if (pCallback)
			pCallback->step(l.size());
	}
	
	return true;
}

bool qm::AccountImpl::setMessagesFlags(NormalFolder* pFolder,
									   const MessageHolderList& l,
									   unsigned int nFlags,
									   unsigned int nMask,
									   UndoItemList* pUndoItemList)
{
	assert(pFolder);
	assert(pThis_->isLocked());
	assert(std::find_if(l.begin(), l.end(),
		std::not1(
			std::bind2nd(
				binary_compose_f_gx_hy(
					std::equal_to<Folder*>(),
					std::mem_fun(&MessageHolder::getFolder),
					std::identity<Folder*>()),
				pFolder))) == l.end());
	
	if (l.empty())
		return true;
	
	std::auto_ptr<SetFlagsUndoItem> pUndoItem;
	if (pFolder->isFlag(Folder::FLAG_LOCAL)) {
		if (pUndoItemList)
			pUndoItem.reset(new SetFlagsUndoItem());
		for (MessageHolderList::const_iterator it = l.begin(); it != l.end(); ++it) {
			MessageHolder* pmh = *it;
			unsigned int nOldFlags = pmh->getFlags();
			pmh->setFlags(nFlags, nMask);
			if (pUndoItem.get())
				pUndoItem->add(pmh, nOldFlags, nMask);
		}
	}
	else {
		if (!pProtocolDriver_->setMessagesFlags(pFolder, l, nFlags, nMask))
			return false;
	}
	if (pUndoItemList)
		pUndoItemList->add(pUndoItem);
	
	return true;
}

bool qm::AccountImpl::getDataList(MessageStore::DataList* pList) const
{
	for (Account::FolderList::const_iterator it = listFolder_.begin(); it != listFolder_.end(); ++it) {
		Folder* pFolder = *it;
		if (pFolder->getType() == Folder::TYPE_NORMAL) {
			if (!pFolder->loadMessageHolders())
				return false;
			
			unsigned int nCount = pFolder->getCount();
			pList->reserve(pList->size() + nCount);
			for (unsigned int n = 0; n < nCount; ++n) {
				MessageHolder* pmh = pFolder->getMessage(n);
				const MessageHolder::MessageIndexKey& indexKey = pmh->getMessageIndexKey();
				const MessageHolder::MessageBoxKey& boxKey = pmh->getMessageBoxKey();
				MessageStore::Data data = {
					boxKey.nOffset_,
					boxKey.nLength_,
					indexKey.nKey_,
					indexKey.nLength_
				};
				pList->push_back(data);
			}
		}
	}
	return true;
}

bool qm::AccountImpl::processSMIME(const SMIMEUtility* pSMIMEUtility,
								   SMIMEUtility::Type type,
								   Message* pMessage)
{
	xstring_size_ptr strMessage;
	wstring_ptr wstrSignedBy;
	unsigned int nSecurity = pMessage->getSecurity();
	switch (type) {
	case SMIMEUtility::TYPE_SIGNED:
	case SMIMEUtility::TYPE_MULTIPARTSIGNED:
		{
			unsigned int nVerify = 0;
			strMessage = pSMIMEUtility->verify(*pMessage,
				pSecurity_->getCA(), &nVerify, &wstrSignedBy);
			if (!strMessage.get())
				return false;
			
			if (nVerify == SMIMEUtility::VERIFY_OK)
				nSecurity |= Message::SECURITY_VERIFIED;
			if (nVerify & SMIMEUtility::VERIFY_FAILED)
				nSecurity |= Message::SECURITY_VERIFICATIONFAILED;
			if (nVerify & SMIMEUtility::VERIFY_ADDRESSNOTMATCH)
				nSecurity |= Message::SECURITY_ADDRESSNOTMATCH;
		}
		break;
	case SMIMEUtility::TYPE_ENVELOPED:
		{
			SubAccount* pSubAccount = pThis_->getCurrentSubAccount();
			std::auto_ptr<PrivateKey> pPrivateKey(pSubAccount->getPrivateKey(pPasswordManager_));
			std::auto_ptr<Certificate> pCertificate(pSubAccount->getCertificate(pPasswordManager_));
			if (pPrivateKey.get() && pCertificate.get()) {
				strMessage = pSMIMEUtility->decrypt(*pMessage, pPrivateKey.get(), pCertificate.get());
				if (!strMessage.get())
					return false;
				nSecurity |= Message::SECURITY_DECRYPTED;
			}
		}
		break;
	case SMIMEUtility::TYPE_ENVELOPEDORSIGNED:
		assert(false);
		break;
	default:
		break;
	}
	if (!strMessage.get())
		return false;
	
	if (!pMessage->create(strMessage.get(), strMessage.size(), Message::FLAG_NONE, nSecurity))
		return false;
	
	if (wstrSignedBy.get())
		pMessage->setParam(L"SignedBy", wstrSignedBy.get());
	
	return true;
}

bool qm::AccountImpl::processPGP(const PGPUtility* pPGPUtility,
								 PGPUtility::Type type,
								 Message* pMessage)
{
	PasswordState state = PASSWORDSTATE_ONETIME;
	wstring_ptr wstrPassword;
	if (type == PGPUtility::TYPE_MIMEENCRYPTED ||
		type == PGPUtility::TYPE_INLINEENCRYPTED) {
		PGPPasswordCondition condition(pThis_->getCurrentSubAccount()->getSenderAddress());
		wstrPassword = pPasswordManager_->getPassword(condition, false, &state);
		if (!wstrPassword.get())
			return false;
	}
	
	xstring_size_ptr strMessage;
	unsigned int nVerify = 0;
	wstring_ptr wstrSignedBy;
	unsigned int nSecurity = pMessage->getSecurity();
	switch (type) {
	case PGPUtility::TYPE_MIMEENCRYPTED:
		strMessage = pPGPUtility->decryptAndVerify(*pMessage,
			true, wstrPassword.get(), &nVerify, &wstrSignedBy);
		if (!strMessage.get())
			return false;
		nSecurity |= Message::SECURITY_DECRYPTED;
		break;
	case PGPUtility::TYPE_MIMESIGNED:
		strMessage = pPGPUtility->verify(*pMessage, true, &nVerify, &wstrSignedBy);
		if (!strMessage.get())
			return false;
		break;
	case PGPUtility::TYPE_INLINEENCRYPTED:
		strMessage = pPGPUtility->decryptAndVerify(
			*pMessage, false, wstrPassword.get(), &nVerify, &wstrSignedBy);
		if (!strMessage.get())
			return false;
		nSecurity |= Message::SECURITY_DECRYPTED;
		break;
	case PGPUtility::TYPE_INLINESIGNED:
		strMessage = pPGPUtility->verify(*pMessage, false, &nVerify, &wstrSignedBy);
		if (!strMessage.get())
			return false;
		break;
	}
	
	if (state == PASSWORDSTATE_SESSION || state == PASSWORDSTATE_SAVE) {
		PGPPasswordCondition condition(pThis_->getCurrentSubAccount()->getSenderAddress());
		pPasswordManager_->setPassword(condition,
			wstrPassword.get(), state == PASSWORDSTATE_SAVE);
	}
	
	if (nVerify == PGPUtility::VERIFY_OK)
		nSecurity |= Message::SECURITY_VERIFIED;
	if (nVerify & PGPUtility::VERIFY_FAILED)
		nSecurity |= Message::SECURITY_VERIFICATIONFAILED;
	if (nVerify & PGPUtility::VERIFY_ADDRESSNOTMATCH)
		nSecurity |= Message::SECURITY_ADDRESSNOTMATCH;
	
	if (!pMessage->create(strMessage.get(), strMessage.size(), Message::FLAG_NONE, nSecurity))
		return false;
	
	if (wstrSignedBy.get())
		pMessage->setParam(L"SignedBy", wstrSignedBy.get());
	
	return true;
}

void qm::AccountImpl::fireCurrentSubAccountChanged()
{
	AccountEvent event(pThis_);
	for (AccountHandlerList::const_iterator it = listAccountHandler_.begin(); it != listAccountHandler_.end(); ++it)
		(*it)->currentSubAccountChanged(event);
}

void qm::AccountImpl::fireSubAccountListChanged()
{
	AccountEvent event(pThis_);
	for (AccountHandlerList::const_iterator it = listAccountHandler_.begin(); it != listAccountHandler_.end(); ++it)
		(*it)->subAccountListChanged(event);
}

void qm::AccountImpl::fireFolderListChanged(const FolderListChangedEvent& event)
{
	for (AccountHandlerList::const_iterator it = listAccountHandler_.begin(); it != listAccountHandler_.end(); ++it)
		(*it)->folderListChanged(event);
}

void qm::AccountImpl::fireAccountDestroyed()
{
	AccountHandlerList l(listAccountHandler_);
	AccountEvent event(pThis_);
	for (AccountHandlerList::const_iterator it = l.begin(); it != l.end(); ++it)
		(*it)->accountDestroyed(event);
}

bool qm::AccountImpl::createTemporaryMessage(MessageHolder* pmh, Message* pMessage)
{
	StringBuffer<WSTRING> buf;
	
	Time time;
	pmh->getDate(&time);
	wstring_ptr wstrDate(time.format(L"Date: %W, %D %M1 %Y4 %h:%m:%s %z\n", Time::FORMAT_ORIGINAL));
	buf.append(wstrDate.get());
	
	struct {
		const WCHAR* pwszName_;
		wstring_ptr (MessageHolder::*pfn_)() const;
		const WCHAR* pwszPrefix_;
		const WCHAR* pwszSuffix_;
	} fields[] = {
		{ L"From",			&MessageHolder::getFrom,		L"\"",	L"\" <unknown@unknown-host.unknown-domain>"	},
		{ L"To",			&MessageHolder::getTo,			L"\"",	L"\" <unknown@unknown-host.unknown-domain>"	},
		{ L"Subject",		&MessageHolder::getSubject,		0,		0											},
		{ L"Message-Id",	&MessageHolder::getMessageId,	L"<",	L">"										}
	};
	for (int n = 0; n < countof(fields); ++n) {
		wstring_ptr wstrValue((pmh->*fields[n].pfn_)());
		if (*wstrValue.get()) {
			buf.append(fields[n].pwszName_);
			buf.append(L": ");
			if (fields[n].pwszPrefix_)
				buf.append(fields[n].pwszPrefix_);
			buf.append(wstrValue.get());
			if (fields[n].pwszSuffix_)
				buf.append(fields[n].pwszSuffix_);
			buf.append(L"\n");
		}
	}
	
	MessageCreator creator(MessageCreator::FLAG_RECOVERHEADER, SECURITYMODE_NONE);
	if (!creator.createHeader(pMessage, buf.getCharArray(), buf.getLength()))
		return false;
	pMessage->setFlag(Message::FLAG_TEMPORARY);
	
	return true;
}

bool qm::AccountImpl::callByFolder(const MessageHolderList& l,
								   CallByFolderCallback* pCallback)
{
	if (l.empty())
		return true;
	
	NormalFolder* pFolder = l.front()->getFolder();
	MessageHolderList::const_iterator it = std::find_if(
		l.begin(), l.end(),
		std::not1(
			std::bind2nd(
				binary_compose_f_gx_hy(
					std::equal_to<NormalFolder*>(),
					std::mem_fun(&MessageHolder::getFolder),
					std::identity<NormalFolder*>()),
				pFolder)));
	if (it == l.end()) {
		if (!pCallback->callback(pFolder, l))
			return false;
	}
	else {
		MessageHolderList listSort(l);
		std::sort(listSort.begin(), listSort.end(),
			binary_compose_f_gx_hy(
				std::less<NormalFolder*>(),
				std::mem_fun(&MessageHolder::getFolder),
				std::mem_fun(&MessageHolder::getFolder)));
		
		MessageHolderList listCopy;
		for (MessageHolderList::const_iterator it = listSort.begin(); it != listSort.end(); ) {
			NormalFolder* pFolder = (*it)->getFolder();
			MessageHolderList::const_iterator itBegin = it;
			for (++it; it != listSort.end() && (*it)->getFolder() == pFolder; ++it)
				;
			listCopy.assign(itBegin, it);
			if (!pCallback->callback(pFolder, listCopy))
				return false;
		}
	}
	
	return true;
}

bool qm::AccountImpl::createDefaultFolders()
{
	Account::FolderList l;
	if (!pProtocolDriver_->createDefaultFolders(&l))
		return false;
	
	listFolder_.reserve(listFolder_.size() + l.size());
	
	for (Account::FolderList::const_iterator it = l.begin(); it != l.end(); ++it) {
		Folder* pFolder = *it;
		if (pFolder->getType() == Folder::TYPE_NORMAL)
			pProtocolDriver_->setDefaultFolderParams(static_cast<NormalFolder*>(pFolder));
		listFolder_.push_back(pFolder);
	}
	
	return true;
}


/****************************************************************************
 *
 * Account
 *
 */

qm::Account::Account(const WCHAR* pwszPath,
					 const Security* pSecurity,
					 PasswordManager* pPasswordManager,
					 JunkFilter* pJunkFilter) :
	pImpl_(0)
{
	assert(pwszPath);
	assert(*pwszPath);
	assert(pwszPath[wcslen(pwszPath) - 1] != L'\\');
	assert(pSecurity);
	
	wstring_ptr wstrPath(allocWString(pwszPath));
	
	const WCHAR* pName = wcsrchr(pwszPath, L'\\');
	assert(pName);
	wstring_ptr wstrName(allocWString(pName + 1));
	
	pImpl_ = new AccountImpl();
	pImpl_->pThis_ = this;
	pImpl_->wstrName_ = wstrName;
	pImpl_->wstrPath_ = wstrPath;
	pImpl_->pProfile_ = 0;
	pImpl_->pSecurity_ = pSecurity;
	pImpl_->pPasswordManager_ = pPasswordManager;
	pImpl_->pCurrentSubAccount_ = 0;
	pImpl_->bDeletedAsSeen_ = false;
#ifndef NDEBUG
	pImpl_->nLock_ = 0;
#endif
	
	if (!pImpl_->loadSubAccounts()) {
		// TODO
	}
	assert(!pImpl_->listSubAccount_.empty());
	
	Profile* pProfile = pImpl_->pProfile_;
	
	int nBlockSize = pProfile->getInt(L"Global", L"BlockSize", -1);
	if (nBlockSize != -1)
		nBlockSize *= 1024*1024;
	
	int nIndexBlockSize = pProfile->getInt(L"Global", L"IndexBlockSize", -1);
	if (nIndexBlockSize == 0)
		nIndexBlockSize = -1;
	else if (nIndexBlockSize != -1)
		nIndexBlockSize *= 1024*1024;
	
	wstring_ptr wstrMessageStorePath(
		pProfile->getString(L"Global", L"MessageStorePath", L""));
	if (*wstrMessageStorePath.get())
		pImpl_->wstrMessageStorePath_ = wstrMessageStorePath;
	else
		pImpl_->wstrMessageStorePath_ = allocWString(pwszPath);
	
	pImpl_->bMultiMessageStore_ = nBlockSize == 0;
	if (pImpl_->bMultiMessageStore_)
		pImpl_->pMessageStore_.reset(new MultiMessageStore(
			pImpl_->wstrMessageStorePath_.get(),
			pwszPath, nIndexBlockSize));
	else
		pImpl_->pMessageStore_.reset(new SingleMessageStore(
			pImpl_->wstrMessageStorePath_.get(),
			nBlockSize, pwszPath, nIndexBlockSize));
	
	pImpl_->bStoreDecodedMessage_ = pImpl_->bMultiMessageStore_ &&
		pProfile->getInt(L"Global", L"StoreDecodedMessage", 0) != 0;
	
	size_t nIndexMaxSize = pProfile->getInt(L"Global", L"IndexMaxSize", -1);
	pImpl_->pMessageIndex_.reset(new MessageIndex(pImpl_->pMessageStore_.get(), nIndexMaxSize));
	
	pImpl_->wstrClass_ = pProfile->getString(L"Global", L"Class", L"mail");
	pImpl_->wstrType_[HOST_SEND] = pProfile->getString(L"Send", L"Type", L"smtp");
	pImpl_->wstrType_[HOST_RECEIVE] = pProfile->getString(L"Receive", L"Type", L"pop3");
	
	pImpl_->pProtocolDriver_ = ProtocolFactory::getDriver(this, pSecurity);
	pImpl_->bDeletedAsSeen_ = pImpl_->pProtocolDriver_->isSupport(SUPPORT_DELETEDMESSAGE);
	
	pImpl_->pJunkFilter_ = isSupport(SUPPORT_JUNKFILTER) ? pJunkFilter : 0;
	
	if (!pImpl_->loadFolders()) {
		// TODO
	}
	
	wstring_ptr wstrSubAccount(pProfile->getString(L"Global", L"SubAccount", L""));
	pImpl_->pCurrentSubAccount_ = getSubAccount(wstrSubAccount.get());
	if (!pImpl_->pCurrentSubAccount_)
		pImpl_->pCurrentSubAccount_ = pImpl_->listSubAccount_.front();
	
	if (!pImpl_->pProtocolDriver_->init()) {
		// TODO
	}
	pImpl_->pProtocolDriver_->setSubAccount(pImpl_->pCurrentSubAccount_);
}

qm::Account::~Account()
{
	if (pImpl_) {
		std::for_each(pImpl_->listSubAccount_.begin(),
			pImpl_->listSubAccount_.end(), deleter<SubAccount>());
		std::for_each(pImpl_->listFolder_.begin(),
			pImpl_->listFolder_.end(), deleter<Folder>());
		delete pImpl_;
		pImpl_ = 0;
	}
}

const WCHAR* qm::Account::getName() const
{
	return pImpl_->wstrName_.get();
}

const WCHAR* qm::Account::getPath() const
{
	return pImpl_->wstrPath_.get();
}

const WCHAR* qm::Account::getClass() const
{
	return pImpl_->wstrClass_.get();
}

const WCHAR* qm::Account::getType(Host host) const
{
	return pImpl_->wstrType_[host].get();
}

bool qm::Account::isSupport(Support support) const
{
	return pImpl_->pProtocolDriver_->isSupport(support);
}

const WCHAR* qm::Account::getMessageStorePath() const
{
	return pImpl_->wstrMessageStorePath_.get();
}

bool qm::Account::isMultiMessageStore() const
{
	return pImpl_->bMultiMessageStore_;
}

bool qm::Account::isStoreDecodedMessage() const
{
	return pImpl_->bStoreDecodedMessage_;
}

void qm::Account::setStoreDecodedMessage(bool bStore) const
{
	pImpl_->bStoreDecodedMessage_ = bStore;
}

int qm::Account::getProperty(const WCHAR* pwszSection,
							 const WCHAR* pwszKey,
							 int nDefault) const
{
	assert(pwszSection);
	assert(pwszKey);
	assert(!pImpl_->listSubAccount_.empty());
	
	SubAccount* pSubAccount = pImpl_->listSubAccount_.front();
	return pSubAccount->getProperty(pwszSection, pwszKey, nDefault);
}

void qm::Account::setProperty(const WCHAR* pwszSection,
							  const WCHAR* pwszKey,
							  int nValue)
{
	assert(pwszSection);
	assert(pwszKey);
	assert(!pImpl_->listSubAccount_.empty());
	
	SubAccount* pSubAccount = pImpl_->listSubAccount_.front();
	pSubAccount->setProperty(pwszSection, pwszKey, nValue);
}

wstring_ptr qm::Account::getProperty(const WCHAR* pwszSection,
									 const WCHAR* pwszName,
									 const WCHAR* pwszDefault) const
{
	assert(pwszSection);
	assert(pwszName);
	assert(!pImpl_->listSubAccount_.empty());
	
	SubAccount* pSubAccount = pImpl_->listSubAccount_.front();
	return pSubAccount->getProperty(pwszSection, pwszName, pwszDefault);
}

void qm::Account::setProperty(const WCHAR* pwszSection,
							  const WCHAR* pwszName,
							  const WCHAR* pwszValue)
{
	assert(pwszSection);
	assert(pwszName);
	assert(pwszValue);
	assert(!pImpl_->listSubAccount_.empty());
	
	SubAccount* pSubAccount = pImpl_->listSubAccount_.front();
	pSubAccount->setProperty(pwszSection, pwszName, pwszValue);
}

SubAccount* qm::Account::getSubAccount(const WCHAR* pwszName) const
{
	SubAccountList::const_iterator it = std::find_if(
		pImpl_->listSubAccount_.begin(), pImpl_->listSubAccount_.end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				string_equal<WCHAR>(),
				std::mem_fun(&SubAccount::getName),
				std::identity<const WCHAR*>()),
			pwszName));
	return it != pImpl_->listSubAccount_.end() ? *it : 0;
}

SubAccount* qm::Account::getSubAccountByIdentity(const WCHAR* pwszIdentity) const
{
	SubAccountList::const_iterator it = std::find_if(
		pImpl_->listSubAccount_.begin(), pImpl_->listSubAccount_.end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				string_equal<WCHAR>(),
				std::mem_fun(&SubAccount::getIdentity),
				std::identity<const WCHAR*>()),
			pwszIdentity));
	return it != pImpl_->listSubAccount_.end() ? *it : 0;
}

const Account::SubAccountList& qm::Account::getSubAccounts() const
{
	return pImpl_->listSubAccount_;
}

void qm::Account::addSubAccount(std::auto_ptr<SubAccount> pSubAccount)
{
	assert(!getSubAccount(pSubAccount->getName()));
	
	pImpl_->listSubAccount_.push_back(pSubAccount.get());
	pSubAccount.release();
	pImpl_->fireSubAccountListChanged();
}

void qm::Account::removeSubAccount(SubAccount* pSubAccount)
{
	assert(*pSubAccount->getName());
	assert(std::find(pImpl_->listSubAccount_.begin(),
		pImpl_->listSubAccount_.end(), pSubAccount) !=
		pImpl_->listSubAccount_.end());
	
	SubAccountList::iterator it = std::remove(pImpl_->listSubAccount_.begin(),
		pImpl_->listSubAccount_.end(), pSubAccount);
	assert(it != pImpl_->listSubAccount_.end());
	pImpl_->listSubAccount_.erase(it, pImpl_->listSubAccount_.end());
	
	if (getCurrentSubAccount() == pSubAccount)
		setCurrentSubAccount(pImpl_->listSubAccount_.front());
	
	if (!pSubAccount->deletePermanent()) {
		// TODO LOG
	}
	delete pSubAccount;
}

bool qm::Account::renameSubAccount(SubAccount* pSubAccount,
								   const WCHAR* pwszName)
{
	if (getSubAccount(pwszName))
		return false;
	
	if (!pSubAccount->setName(pwszName))
		return false;
	pImpl_->fireSubAccountListChanged();
	
	return true;
}

SubAccount* qm::Account::getCurrentSubAccount() const
{
	return pImpl_->pCurrentSubAccount_;
}

void qm::Account::setCurrentSubAccount(SubAccount* pSubAccount)
{
	pImpl_->pCurrentSubAccount_ = pSubAccount;
	pImpl_->pProtocolDriver_->setSubAccount(pSubAccount);
	
	pImpl_->fireCurrentSubAccountChanged();
}

Folder* qm::Account::getFolder(const WCHAR* pwszName) const
{
	assert(pwszName);
	
	const FolderList& l = pImpl_->listFolder_;
	for (FolderList::const_iterator it = l.begin(); it != l.end(); ++it) {
		wstring_ptr wstrName((*it)->getFullName());
		if (wcscmp(wstrName.get(), pwszName) == 0)
			return *it;
	}
	return 0;
}

Folder* qm::Account::getFolder(Folder* pParent,
							   const WCHAR* pwszName) const
{
	FolderList::const_iterator it = std::find_if(
		pImpl_->listFolder_.begin(), pImpl_->listFolder_.end(),
		unary_compose_f_gx_hy(
			binary_and_t<bool>(),
			std::bind2nd(
				binary_compose_f_gx_hy(
					std::equal_to<Folder*>(),
					std::mem_fun(&Folder::getParentFolder),
					std::identity<Folder*>()),
				pParent),
			std::bind2nd(
				binary_compose_f_gx_hy(
					string_equal<WCHAR>(),
					std::mem_fun(&Folder::getName),
					std::identity<const WCHAR*>()),
				pwszName)));
	return it != pImpl_->listFolder_.end() ? *it : 0;
}

Folder* qm::Account::getFolderById(unsigned int nId) const
{
	const FolderList& l = pImpl_->listFolder_;
	FolderList::const_iterator it = std::find_if(
		l.begin(), l.end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				std::equal_to<unsigned int>(),
				std::mem_fun(&Folder::getId),
				std::identity<unsigned int>()),
			nId));
	return it != l.end() ? *it : 0;
}

Folder* qm::Account::getFolderByBoxFlag(unsigned int nBoxFlag) const
{
	assert((nBoxFlag & ~Folder::FLAG_BOX_MASK) == 0);
	
	const FolderList& l = pImpl_->listFolder_;
	FolderList::const_iterator it = std::find_if(
		l.begin(), l.end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				contains<unsigned int>(),
				std::mem_fun(&Folder::getFlags),
				std::identity<unsigned int>()),
			nBoxFlag));
	if (it == l.end())
		return 0;
	
	Folder::Type type = nBoxFlag != Folder::FLAG_SEARCHBOX ?
		Folder::TYPE_NORMAL : Folder::TYPE_QUERY;
	if ((*it)->getType() != type)
		return 0;
	
	return *it;
}

Folder* qm::Account::getFolderByParam(const WCHAR* pwszName,
									  const WCHAR* pwszValue) const
{
	FolderList::const_iterator it = std::find_if(
		pImpl_->listFolder_.begin(), pImpl_->listFolder_.end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				string_equal<WCHAR>(),
				std::bind2nd(
					std::mem_fun(&Folder::getParam),
					pwszName),
				std::identity<const WCHAR*>()),
			pwszValue));
	return it != pImpl_->listFolder_.end() ? *it : 0;
}

const Account::FolderList& qm::Account::getFolders() const
{
	return pImpl_->listFolder_;
}

void qm::Account::getNormalFolders(const WCHAR* pwszName,
								   bool bRecursive,
								   NormalFolderList* pList) const
{
	assert(pList);
	
	Folder* pNamedFolder = 0;
	if (pwszName)
		pNamedFolder = getFolder(pwszName);
	
	const FolderList& l = pImpl_->listFolder_;
	
	if (pNamedFolder) {
		if (bRecursive) {
			for (FolderList::const_iterator it = l.begin(); it != l.end(); ++it) {
				Folder* pFolder = *it;
				
				if (pFolder->getType() == Folder::TYPE_NORMAL &&
					!pFolder->isHidden() &&
					(pFolder == pNamedFolder ||
					pNamedFolder->isAncestorOf(pFolder)))
					pList->push_back(static_cast<NormalFolder*>(pFolder));
			}
		}
		else {
			if (pNamedFolder->getType() == Folder::TYPE_NORMAL)
				pList->push_back(static_cast<NormalFolder*>(pNamedFolder));
		}
	}
	else {
		for (FolderList::const_iterator it = l.begin(); it != l.end(); ++it) {
			Folder* pFolder = *it;
			if (pFolder->getType() == Folder::TYPE_NORMAL &&
				!pFolder->isFlag(Folder::FLAG_TRASHBOX) &&
				!pFolder->isFlag(Folder::FLAG_JUNKBOX))
				pList->push_back(static_cast<NormalFolder*>(pFolder));
		}
	}
}

NormalFolder* qm::Account::createNormalFolder(const WCHAR* pwszName,
											  Folder* pParent,
											  bool bRemote,
											  bool bSyncable)
{
	if (getFolder(pParent, pwszName))
		return 0;
	else if (pParent && pParent->isFlag(Folder::FLAG_NOINFERIORS))
		return 0;
	
	WCHAR cSeparator = pParent ? pParent->getSeparator() : L'/';
	
	std::auto_ptr<NormalFolder> pNormalFolder;
	if (bRemote) {
		const WCHAR* p = wcschr(pwszName, cSeparator);
		if (p && *(p + 1) != L'\0')
			return 0;
		
		pNormalFolder = pImpl_->pProtocolDriver_->createFolder(pwszName, pParent);
	}
	else {
		if (wcschr(pwszName, cSeparator))
			return 0;
		
		unsigned int nFlags = Folder::FLAG_LOCAL | (bSyncable ? Folder::FLAG_SYNCABLE : 0);
		pNormalFolder.reset(new NormalFolder(generateFolderId(),
			pwszName, cSeparator, nFlags, 0, 0, 0, 0, 0, pParent, this));
	}
	if (!pNormalFolder.get())
		return 0;
	
	pImpl_->pProtocolDriver_->setDefaultFolderParams(pNormalFolder.get());
	pImpl_->listFolder_.push_back(pNormalFolder.get());
	NormalFolder* pFolder = pNormalFolder.release();
	
	FolderListChangedEvent event(this, FolderListChangedEvent::TYPE_ADD, pFolder);
	pImpl_->fireFolderListChanged(event);
	
	return pFolder;
}

QueryFolder* qm::Account::createQueryFolder(const WCHAR* pwszName,
											Folder* pParent,
											const WCHAR* pwszDriver,
											const WCHAR* pwszCondition,
											const WCHAR* pwszTargetFolder,
											bool bRecursive)
{
	if (getFolder(pParent, pwszName))
		return 0;
	
	WCHAR cSeparator = pParent ? pParent->getSeparator() : L'/';
	if (wcschr(pwszName, cSeparator))
		return 0;
	
	std::auto_ptr<QueryFolder> pQueryFolder(new QueryFolder(
		generateFolderId(), pwszName, cSeparator, Folder::FLAG_LOCAL, 0, 0,
		pwszDriver, pwszCondition, pwszTargetFolder, bRecursive, pParent, this));
	
	pImpl_->listFolder_.push_back(pQueryFolder.get());
	QueryFolder* pFolder = pQueryFolder.release();
	
	FolderListChangedEvent event(this, FolderListChangedEvent::TYPE_ADD, pFolder);
	pImpl_->fireFolderListChanged(event);
	
	return pFolder;
}

bool qm::Account::removeFolder(Folder* pFolder)
{
	while (true) {
		FolderList::iterator it = std::find(pImpl_->listFolder_.begin(),
			pImpl_->listFolder_.end(), pFolder);
		assert(it != pImpl_->listFolder_.end());
		++it;
		if (it == pImpl_->listFolder_.end() || !pFolder->isAncestorOf(*it))
			break;
		if (!removeFolder(*it))
			return false;
	}
	
	if (pFolder->getType() == Folder::TYPE_NORMAL &&
		!pFolder->isFlag(Folder::FLAG_LOCAL)) {
		if (!pImpl_->pProtocolDriver_->removeFolder(static_cast<NormalFolder*>(pFolder)))
			return false;
	}
	
	FolderList::iterator it = std::find(pImpl_->listFolder_.begin(),
		pImpl_->listFolder_.end(), pFolder);
	assert(it != pImpl_->listFolder_.end());
	pImpl_->listFolder_.erase(it);
	
	FolderListChangedEvent event(this, FolderListChangedEvent::TYPE_REMOVE, pFolder);
	pImpl_->fireFolderListChanged(event);
	
	if (!pFolder->deletePermanent()) {
		// TODO
	}
	delete pFolder;
	
	return true;
}

bool qm::Account::renameFolder(Folder* pFolder,
							   const WCHAR* pwszName)
{
	assert(pFolder);
	assert(pwszName);
	
	if (wcschr(pwszName, pFolder->getSeparator()) ||
		getFolder(pFolder->getParentFolder(), pwszName))
		return false;
	
	if (pFolder->getType() == Folder::TYPE_NORMAL &&
		!pFolder->isFlag(Folder::FLAG_LOCAL)) {
		if (!pImpl_->pProtocolDriver_->renameFolder(
			static_cast<NormalFolder*>(pFolder), pwszName))
			return false;
	}
	
	pFolder->setName(pwszName);
	
	FolderListChangedEvent event(this, FolderListChangedEvent::TYPE_RENAME, pFolder);
	pImpl_->fireFolderListChanged(event);
	
	return true;
}

bool qm::Account::moveFolder(Folder* pFolder,
							 Folder* pParent,
							 const WCHAR* pwszName)
{
	assert(pFolder);
	
	if (pFolder->getParentFolder() == pParent)
		return true;
	else if (pParent && pParent->getAccount() != pFolder->getAccount())
		return false;
	else if (pParent == pFolder || (pParent && pFolder->isAncestorOf(pParent)))
		return false;
	else if ((!pwszName && getFolder(pParent, pFolder->getName())) ||
		(pwszName && getFolder(pParent, pwszName)))
		return false;
	
	if (pFolder->isFlag(Folder::FLAG_LOCAL)) {
		for (FolderList::const_iterator it = pImpl_->listFolder_.begin(); it != pImpl_->listFolder_.end(); ++it) {
			if (pFolder->isAncestorOf(*it) && !(*it)->isFlag(Folder::FLAG_LOCAL))
				return false;
		}
	}
	else {
		if (pParent && pParent->isFlag(Folder::FLAG_LOCAL))
			return false;
		
		assert(pFolder->getType() == Folder::TYPE_NORMAL);
		assert(!pParent || pParent->getType() == Folder::TYPE_NORMAL);
		
		if (!pImpl_->pProtocolDriver_->moveFolder(
			static_cast<NormalFolder*>(pFolder),
			static_cast<NormalFolder*>(pParent),
			pwszName))
			return false;
	}
	
	if (pwszName)
		pFolder->setName(pwszName);
	pFolder->setParentFolder(pParent);
	
	FolderListChangedEvent event(this, FolderListChangedEvent::TYPE_ALL, 0);
	pImpl_->fireFolderListChanged(event);
	
	return true;
}

void qm::Account::setFolderFlags(Folder* pFolder,
								 unsigned int nFlags,
								 unsigned int nMask)
{
	unsigned int nOldFlags = pFolder->getFlags();
	if ((nOldFlags & nMask) != nFlags) {
		pFolder->setFlags(nFlags, nMask);
		
		FolderListChangedEvent event(this,
			FolderListChangedEvent::TYPE_FLAGS, pFolder,
			nOldFlags, pFolder->getFlags());
		pImpl_->fireFolderListChanged(event);
	}
}

bool qm::Account::updateFolders()
{
	typedef ProtocolDriver::RemoteFolderList List;
	List l;
	if (!pImpl_->pProtocolDriver_->getRemoteFolders(&l))
		return false;
	
	struct Deleter
	{
		typedef ProtocolDriver::RemoteFolderList List;
		
		Deleter(const List& l) :
			l_(l),
			bReleased_(false)
		{
		}
		
		~Deleter()
		{
			if (bReleased_)
				return;
			for (List::const_iterator it = l_.begin(); it != l_.end(); ++it) {
				if ((*it).second)
					delete (*it).first;
			}
		}
		
		void release()
		{
			bReleased_ = true;
		}
		
		const List& l_;
		bool bReleased_;
	} deleter(l);
	
	pImpl_->listFolder_.reserve(pImpl_->listFolder_.size() + l.size());
	for (List::const_iterator itR = l.begin(); itR != l.end(); ++itR) {
		if ((*itR).second) {
			NormalFolder* pFolder = static_cast<NormalFolder*>((*itR).first);
			pImpl_->pProtocolDriver_->setDefaultFolderParams(pFolder);
			pImpl_->listFolder_.push_back(pFolder);
		}
	}
	deleter.release();
	
	FolderList listDelete;
	struct Deleter2
	{
		Deleter2(const FolderList& l) :
			l_(l)
		{
		}
		
		~Deleter2()
		{
			std::for_each(l_.begin(), l_.end(), qs::deleter<Folder>());
		}
		
		const FolderList& l_;
	} deleter2(listDelete);
	
	std::sort(l.begin(), l.end(), RemoteFolderLess());
	for (FolderList::iterator itF = pImpl_->listFolder_.begin(); itF != pImpl_->listFolder_.end(); ) {
		std::pair<Folder*, bool> p(*itF, false);
		ProtocolDriver::RemoteFolderList::const_iterator itR = std::lower_bound(
			l.begin(), l.end(), p, RemoteFolderLess());
		if (itR != l.end() && (*itR).first == *itF) {
			++itF;
		}
		else if ((*itF)->getType() == Folder::TYPE_NORMAL &&
			!((*itF)->getFlags() & Folder::FLAG_LOCAL)) {
			if (!(*itF)->deletePermanent()) {
				// TODO
			}
			listDelete.push_back(*itF);
			itF = pImpl_->listFolder_.erase(itF);
		}
		else {
			++itF;
		}
	}
	
	FolderListChangedEvent event(this, FolderListChangedEvent::TYPE_ALL, 0);
	pImpl_->fireFolderListChanged(event);
	
	return true;
}

std::pair<const WCHAR**, size_t> qm::Account::getFolderParamNames() const
{
	return pImpl_->pProtocolDriver_->getFolderParamNames();
}

void  qm::Account::setOffline(bool bOffline)
{
	pImpl_->pProtocolDriver_->setOffline(bOffline);
}

bool qm::Account::compact(MessageOperationCallback* pCallback)
{
	assert(pCallback);
	
	Lock<Account> lock(*this);
	
	if (!save(false))
		return false;
	
	MessageStore::DataList listData;
	if (!pImpl_->getDataList(&listData))
		return false;
	
	if (!pImpl_->pMessageStore_->compact(&listData, pCallback))
		return false;
	
	MessageStore::DataList::size_type d = 0;
	for (FolderList::const_iterator it = pImpl_->listFolder_.begin(); it != pImpl_->listFolder_.end(); ++it) {
		Folder* pFolder = *it;
		if (pFolder->getType() == Folder::TYPE_NORMAL) {
			unsigned int nCount = pFolder->getCount();
			for (unsigned int n = 0; n < nCount; ++n) {
				const MessageStore::Data& data = listData[d++];
				
				MessageHolder* pmh = pFolder->getMessage(n);
				
				MessageHolder::MessageIndexKey indexKey = pmh->getMessageIndexKey();
				if (indexKey.nKey_ != data.nIndexKey_)
					pImpl_->pMessageIndex_->remove(indexKey.nKey_);
				indexKey.nKey_ = data.nIndexKey_;
				
				MessageHolder::MessageBoxKey boxKey = pmh->getMessageBoxKey();
				boxKey.nOffset_ = data.nOffset_;
				
				pmh->setKeys(indexKey, boxKey);
			}
		}
	}
	
	if (!save(false))
		return false;
	
	if (!pImpl_->pMessageStore_->freeUnused())
		return false;
	
	return true;
}

bool qm::Account::salvage(NormalFolder* pFolder,
						  MessageOperationCallback* pCallback)
{
	assert(pFolder);
	assert(pCallback);
	
	Lock<Account> lock(*this);
	
	pCallback->setCancelable(false);
	pCallback->show();
	
	MessageStore::DataList listData;
	if (!pImpl_->getDataList(&listData))
		return false;
	
	class CallbackImpl : public MessageStoreSalvageCallback
	{
	public:
		CallbackImpl(NormalFolder* pFolder,
					 MessageOperationCallback* pCallback) :
			pFolder_(pFolder),
			pCallback_(pCallback)
		{
		}
		
	public:
		virtual void setCount(size_t nCount)
		{
			pCallback_->setCount(nCount);
		}
		
		virtual void step(size_t nStep)
		{
			pCallback_->step(nStep);
		}
		
		virtual bool salvage(const Message& msg)
		{
			Account* pAccount = pFolder_->getAccount();
			return pAccount->appendMessage(pFolder_, msg, 0, 0, 0);
		}
	
	private:
		NormalFolder* pFolder_;
		MessageOperationCallback* pCallback_;
	} callback(pFolder, pCallback);
	
	if (!pImpl_->pMessageStore_->salvage(listData, &callback))
		return false;
	if (!save(false))
		return false;
	
	return true;
}

bool qm::Account::check(AccountCheckCallback* pCallback)
{
	assert(pCallback);
	
	Lock<Account> lock(*this);
	
	MessageHolderList listMessageHolder;
	for (FolderList::const_iterator it = pImpl_->listFolder_.begin(); it != pImpl_->listFolder_.end(); ++it) {
		Folder* pFolder = *it;
		if (pFolder->getType() == Folder::TYPE_NORMAL) {
			if (!pFolder->loadMessageHolders())
				return false;
			
			unsigned int nCount = pFolder->getCount();
			listMessageHolder.reserve(listMessageHolder.size() + nCount);
			for (unsigned int n = 0; n < nCount; ++n)
				listMessageHolder.push_back(pFolder->getMessage(n));
		}
	}
	
	pCallback->setCancelable(false);
	pCallback->setCount(listMessageHolder.size());
	pCallback->show();
	
	class CallbackImpl : public MessageStoreCheckCallback
	{
	public:
		typedef std::vector<MessageHolder::MessageIndexKey> KeyList;
	
	public:
		CallbackImpl(MessageIndex* pMessageIndex,
					 const MessageHolderList& listMessageHolder,
					 AccountCheckCallback* pCallback) :
			pMessageIndex_(pMessageIndex),
			listMessageHolder_(listMessageHolder),
			listKey_(listMessageHolder.size()),
			pCallback_(pCallback),
			bIgnoreError_(false)
		{
		}
	
	public:
		void apply()
		{
			for (KeyList::size_type n = 0; n < listKey_.size(); ++n) {
				MessageHolder* pmh = listMessageHolder_[n];
				const MessageHolder::MessageIndexKey& indexKey = pmh->getMessageIndexKey();
				const MessageHolder::MessageIndexKey& newIndexKey = listKey_[n];
				if (newIndexKey.nKey_ != -1) {
					if (newIndexKey.nKey_ != indexKey.nKey_ ||
						newIndexKey.nLength_ != indexKey.nLength_) {
						pMessageIndex_->remove(indexKey.nKey_);
						pmh->setKeys(newIndexKey, pmh->getMessageBoxKey());
					}
				}
				else {
					pMessageIndex_->remove(indexKey.nKey_);
					pmh->getFolder()->removeMessages(MessageHolderList(1, pmh));
				}
			}
		}
	
	public:
		virtual unsigned int getCount()
		{
			return static_cast<unsigned int>(listMessageHolder_.size());
		}
		
		virtual bool getHeader(unsigned int n,
							   Message* pMessage)
		{
			return listMessageHolder_[n]->getMessage(
				Account::GETMESSAGEFLAG_HEADER | Account::GETMESSAGEFLAG_NOFALLBACK,
				0, SECURITYMODE_NONE, pMessage);
		}
		
		virtual void setKey(unsigned int n,
							unsigned int nKey,
							unsigned int nLength)
		{
			assert(n < listKey_.size());
			listKey_[n].nKey_ = nKey;
			listKey_[n].nLength_ = nLength;
			
			pCallback_->step(1);
		}
		
		virtual bool isIgnoreError(unsigned int n)
		{
			MessageHolder* pmh = listMessageHolder_[n];
			if (!pmh->getFolder()->isFlag(Folder::FLAG_LOCAL))
				return false;
			
			if (bIgnoreError_)
				return true;
			
			AccountCheckCallback::Ignore ignore = pCallback_->isIgnoreError(pmh);
			switch (ignore) {
			case AccountCheckCallback::IGNORE_FALSE:
				return false;
			case AccountCheckCallback::IGNORE_TRUE:
				return true;
			case AccountCheckCallback::IGNORE_ALL:
				bIgnoreError_ = true;
				return true;
			default:
				assert(false);
				return false;
			}
		}
	
	private:
		MessageIndex* pMessageIndex_;
		const MessageHolderList& listMessageHolder_;
		KeyList listKey_;
		AccountCheckCallback* pCallback_;
		bool bIgnoreError_;
	} callback(pImpl_->pMessageIndex_.get(), listMessageHolder, pCallback);
	
	if (!pImpl_->pMessageStore_->check(&callback))
		return false;
	callback.apply();
	if (!save(false))
		return false;
	
	return true;
}

bool qm::Account::save(bool bForce) const
{
	Lock<Account> lock(*this);
	
	pImpl_->pProfile_->setString(L"Global", L"SubAccount",
		pImpl_->pCurrentSubAccount_->getName());
	pImpl_->pProfile_->setInt(L"Global", L"StoreDecodedMessage", pImpl_->bStoreDecodedMessage_);
	
	if (!saveMessages(bForce))
		return false;
	if (!pImpl_->saveFolders() && !bForce)
		return false;
	if (!pImpl_->saveSubAccounts(bForce))
		return false;
	if (!pImpl_->pProtocolDriver_->save(bForce))
		return false;
	
	return true;
}

bool qm::Account::saveMessages(bool bForce) const
{
	if (!flushMessageStore() && !bForce)
		return false;
	
	for (FolderList::const_iterator it = pImpl_->listFolder_.begin(); it != pImpl_->listFolder_.end(); ++it) {
		if (!(*it)->saveMessageHolders() && !bForce)
			return false;
	}
	
	return true;
}

bool qm::Account::flushMessageStore() const
{
	Lock<Account> lock(*this);
	return pImpl_->pMessageStore_->flush();
}

bool qm::Account::importMessage(NormalFolder* pFolder,
								const CHAR* pszMessage,
								size_t nLen,
								unsigned int nFlags)
{
	assert(pFolder);
	assert(pszMessage);
	
	Message header;
	if (!header.createHeader(pszMessage, nLen))
		return false;
	
	unsigned int nMessageFlags = 0;
	NumberParser flags(NumberParser::FLAG_HEX);
	Part::Field field = header.getField(L"X-QMAIL-Flags", &flags);
	if (field == Part::FIELD_EXIST) {
		switch (nFlags) {
		case Account::IMPORTFLAG_NORMALFLAGS:
			nMessageFlags = flags.getValue() & MessageHolder::FLAG_USER_MASK;
			break;
		case Account::IMPORTFLAG_IGNOREFLAGS:
			break;
		case Account::IMPORTFLAG_QMAIL20FLAGS:
			{
				enum OldFlag {
					SEEN		= 0x00000001,
					REPLIED		= 0x00000002,
					FORWARDED	= 0x00000004,
					SENT		= 0x00000010,
					DRAFT		= 0x00000020,
					MARKED		= 0x00000080,
					TOME		= 0x00002000,
					CCME		= 0x00004000
				};
				struct {
					OldFlag oldFlag_;
					MessageHolder::Flag flag_;
				} map[] = {
					{ SEEN,			MessageHolder::FLAG_SEEN		},
					{ REPLIED,		MessageHolder::FLAG_REPLIED		},
					{ FORWARDED,	MessageHolder::FLAG_FORWARDED	},
					{ SENT,			MessageHolder::FLAG_SENT		},
					{ DRAFT,		MessageHolder::FLAG_DRAFT		},
					{ MARKED,		MessageHolder::FLAG_MARKED		},
					{ TOME,			MessageHolder::FLAG_TOME		},
					{ CCME,			MessageHolder::FLAG_CCME		}
				};
				for (int n = 0; n < countof(map); ++n) {
					if (flags.getValue() & map[n].oldFlag_)
						nMessageFlags |= map[n].flag_;
				}
			}
			break;
		}
	}
	if (field != Part::FIELD_NOTEXIST)
		header.removeField(L"X-QMAIL-Flags");
	
	return pImpl_->appendMessage(pFolder, pszMessage, nLen, header, nMessageFlags, -1, 0, 0);
}

bool qm::Account::appendMessage(NormalFolder* pFolder,
								const Message& msg,
								unsigned int nFlags,
								UndoItemList* pUndoItemList,
								MessagePtr* pptr)
{
	assert(pFolder);
	assert(msg.getFlag() != Message::FLAG_EMPTY);
	
	xstring_size_ptr strMessage(msg.getContent());
	if (!strMessage.get())
		return false;
	
	return pImpl_->appendMessage(pFolder, strMessage.get(),
		strMessage.size(), msg, nFlags, -1, pUndoItemList, pptr);
}

bool qm::Account::removeMessages(const MessageHolderList& l,
								 Folder* pFolder,
								 bool bDirect,
								 MessageOperationCallback* pCallback,
								 UndoItemList* pUndoItemList)
{
	assert(isLocked());
	
	struct CallByFolderCallbackImpl : public AccountImpl::CallByFolderCallback
	{
		CallByFolderCallbackImpl(AccountImpl* pImpl,
								 bool bDirect,
								 MessageOperationCallback* pCallback,
								 UndoItemList* pUndoItemList) :
			pImpl_(pImpl),
			bDirect_(bDirect),
			pCallback_(pCallback),
			pUndoItemList_(pUndoItemList)
		{
		}
		
		virtual bool callback(NormalFolder* pFolder,
							  const MessageHolderList& l)
		{
			return pImpl_->removeMessages(pFolder, l, bDirect_, pCallback_, pUndoItemList_);
		}
		
		AccountImpl* pImpl_;
		bool bDirect_;
		MessageOperationCallback* pCallback_;
		UndoItemList* pUndoItemList_;
	} callback(pImpl_, bDirect, pCallback, pUndoItemList);
	
	if (!AccountImpl::callByFolder(l, &callback))
		return false;
	
	if (pFolder && pFolder->getType() == Folder::TYPE_QUERY)
		static_cast<QueryFolder*>(pFolder)->removeMessages(l);
	
	return true;
}

bool qm::Account::copyMessages(const MessageHolderList& l,
							   Folder* pFolderFrom,
							   NormalFolder* pFolderTo,
							   unsigned int nFlags,
							   MessageOperationCallback* pCallback,
							   UndoItemList* pUndoItemList)
{
	assert(pFolderTo);
	assert(isLocked());
	
	struct CallByFolderCallbackImpl : public AccountImpl::CallByFolderCallback
	{
		CallByFolderCallbackImpl(AccountImpl* pImpl,
								 NormalFolder* pFolderTo,
								 unsigned int nFlags,
								 MessageOperationCallback* pCallback,
								 UndoItemList* pUndoItemList) :
			pImpl_(pImpl),
			pFolderTo_(pFolderTo),
			nFlags_(nFlags),
			pCallback_(pCallback),
			pUndoItemList_(pUndoItemList)
		{
		}
		
		virtual bool callback(NormalFolder* pFolder,
							  const MessageHolderList& l)
		{
			return pImpl_->copyMessages(pFolder, pFolderTo_,
				l, nFlags_, pCallback_, pUndoItemList_);
		}
		
		AccountImpl* pImpl_;
		NormalFolder* pFolderTo_;
		unsigned int nFlags_;
		MessageOperationCallback* pCallback_;
		UndoItemList* pUndoItemList_;
	} callback(pImpl_, pFolderTo, nFlags, pCallback, pUndoItemList);
	
	if (!AccountImpl::callByFolder(l, &callback))
		return false;
	
	if (nFlags & COPYFLAG_MOVE &&
		pFolderFrom && pFolderFrom->getType() == Folder::TYPE_QUERY)
		static_cast<QueryFolder*>(pFolderFrom)->removeMessages(l);
	
	return true;
}

bool qm::Account::setMessagesFlags(const MessageHolderList& l,
								   unsigned int nFlags,
								   unsigned int nMask,
								   UndoItemList* pUndoItemList)
{
	assert(isLocked());
	
	struct CallByFolderCallbackImpl : public AccountImpl::CallByFolderCallback
	{
		CallByFolderCallbackImpl(AccountImpl* pImpl,
								 unsigned int nFlags,
								 unsigned int nMask,
								 UndoItemList* pUndoItemList) :
			pImpl_(pImpl),
			nFlags_(nFlags),
			nMask_(nMask),
			pUndoItemList_(pUndoItemList)
		{
		}
		
		virtual bool callback(NormalFolder* pFolder,
							  const MessageHolderList& l)
		{
			return pImpl_->setMessagesFlags(pFolder, l, nFlags_, nMask_, pUndoItemList_);
		}
		
		AccountImpl* pImpl_;
		unsigned int nFlags_;
		unsigned int nMask_;
		UndoItemList* pUndoItemList_;
	} callback(pImpl_, nFlags, nMask, pUndoItemList);
	
	return AccountImpl::callByFolder(l, &callback);
}

bool qm::Account::deleteMessagesCache(const MessageHolderList& l)
{
	assert(isLocked());
	
	for (MessageHolderList::const_iterator it = l.begin(); it != l.end(); ++it) {
		MessageHolder* pmh = *it;
		
		if (!pmh->getFolder()->isFlag(Folder::FLAG_LOCAL)) {
			MessageHolder::MessageBoxKey key = pmh->getMessageBoxKey();
			if (key.nOffset_ != -1) {
				if (!pImpl_->pMessageStore_->free(key.nOffset_, key.nLength_, -1, 0))
					return false;
				
				key.nOffset_ = -1;
				key.nLength_ = -1;
				key.nHeaderLength_ = -1;
				MessageHolder::MessageIndexKey indexKey = { -1, 0 };
				pmh->setKeys(indexKey, key);
				pmh->setFlags(MessageHolder::FLAG_INDEXONLY,
					MessageHolder::FLAG_PARTIAL_MASK);
			}
		}
	}
	
	return true;
}

bool qm::Account::isSeen(const MessageHolder* pmh) const
{
	return isSeen(pmh->getFlags());
}

bool qm::Account::isSeen(unsigned int nFlags) const
{
	return nFlags & MessageHolder::FLAG_SEEN ||
		(pImpl_->bDeletedAsSeen_ && nFlags & MessageHolder::FLAG_DELETED);
}

void qm::Account::addAccountHandler(AccountHandler* pHandler)
{
	assert(std::find(pImpl_->listAccountHandler_.begin(),
		pImpl_->listAccountHandler_.end(), pHandler) ==
		pImpl_->listAccountHandler_.end());
	pImpl_->listAccountHandler_.push_back(pHandler);
}

void qm::Account::removeAccountHandler(AccountHandler* pHandler)
{
	AccountImpl::AccountHandlerList& l = pImpl_->listAccountHandler_;
	AccountImpl::AccountHandlerList::iterator it =
		std::remove(l.begin(), l.end(), pHandler);
	assert(it != l.end());
	l.erase(it, l.end());
}

void qm::Account::addMessageHolderHandler(MessageHolderHandler* pHandler)
{
	Lock<Account> lock(*this);
	
	assert(std::find(pImpl_->listMessageHolderHandler_.begin(),
		pImpl_->listMessageHolderHandler_.end(), pHandler) ==
		pImpl_->listMessageHolderHandler_.end());
	pImpl_->listMessageHolderHandler_.push_back(pHandler);
}

void qm::Account::removeMessageHolderHandler(MessageHolderHandler* pHandler)
{
	Lock<Account> lock(*this);
	
	AccountImpl::MessageHolderHandlerList& l = pImpl_->listMessageHolderHandler_;
	AccountImpl::MessageHolderHandlerList::iterator it =
		std::remove(l.begin(), l.end(), pHandler);
	assert(it != l.end());
	l.erase(it, l.end());
}

void qm::Account::lock() const
{
	pImpl_->csLock_.lock();
#ifndef NDEBUG
	++pImpl_->nLock_;
#endif
}

void qm::Account::unlock() const
{
#ifndef NDEBUG
	--pImpl_->nLock_;
#endif
	pImpl_->csLock_.unlock();
}

#ifndef NDEBUG
bool qm::Account::isLocked() const
{
	return pImpl_->nLock_ != 0;
}

unsigned int qm::Account::getLockCount() const
{
	return pImpl_->nLock_;
}
#endif

void qm::Account::deletePermanent(bool bDeleteContent)
{
	if (bDeleteContent) {
		pImpl_->pMessageIndex_.reset(0);
		pImpl_->pMessageStore_.reset(0);
		pImpl_->pProtocolDriver_.reset(0);
		
		wstring_ptr wstrMessageStorePath(
			pImpl_->pProfile_->getString(L"Global", L"MessageStorePath", L""));
		if (*wstrMessageStorePath.get()) {
			if (!File::removeDirectory(wstrMessageStorePath.get())) {
				// TODO LOG
			}
		}
		
		if (!File::removeDirectory(pImpl_->wstrPath_.get())) {
			// TODO LOG
		}
	}
	
	pImpl_->fireAccountDestroyed();
}

wstring_ptr qm::Account::getIndex(unsigned int nKey,
								  unsigned int nLength,
								  MessageIndexName name) const
{
	assert(isLocked());
	return pImpl_->pMessageIndex_->get(nKey, nLength, name);
}

bool qm::Account::getMessage(MessageHolder* pmh,
							 unsigned int nFlags,
							 unsigned int nSecurityMode,
							 Message* pMessage)
{
	bool bSMIME = Security::isSMIMEEnabled() && nSecurityMode & SECURITYMODE_SMIME;
	bool bPGP = Security::isPGPEnabled() && nSecurityMode & SECURITYMODE_PGP;
	
	if ((bSMIME || bPGP) && pmh->isFlag(MessageHolder::FLAG_ENVELOPED)) {
		nFlags &= ~GETMESSAGEFLAG_METHOD_MASK;
		nFlags |= GETMESSAGEFLAG_ALL;
	}
	
	if (!pImpl_->getMessage(pmh, nFlags, pMessage))
		return false;
	
	bool bProcessed = false;
	if (bSMIME) {
		const SMIMEUtility* pSMIMEUtility = pImpl_->pSecurity_->getSMIMEUtility();
		SMIMEUtility::Type type = pSMIMEUtility->getType(pMessage->getContentType());
		if  (type != SMIMEUtility::TYPE_NONE) {
			if (pMessage->getFlag() != Message::FLAG_NONE) {
				pMessage->clear();
				if (!pImpl_->getMessage(pmh, GETMESSAGEFLAG_ALL, pMessage))
					return false;
			}
			type = pSMIMEUtility->getType(*pMessage);
		}
		
		while  (type != SMIMEUtility::TYPE_NONE) {
			if (!pImpl_->processSMIME(pSMIMEUtility, type, pMessage))
				break;
			type = pSMIMEUtility->getType(*pMessage);
			bProcessed = true;
		}
	}
	if (bPGP) {
		const PGPUtility* pPGPUtility = pImpl_->pSecurity_->getPGPUtility();
		PGPUtility::Type type = pPGPUtility->getType(pMessage->getContentType());
		if  (type == PGPUtility::TYPE_MIMEENCRYPTED ||
			 type == PGPUtility::TYPE_MIMESIGNED) {
			if (pMessage->getFlag() != Message::FLAG_NONE) {
				pMessage->clear();
				if (!pImpl_->getMessage(pmh, GETMESSAGEFLAG_ALL, pMessage))
					return false;
			}
			type = pPGPUtility->getType(*pMessage, false);
		}
		else {
			type = pPGPUtility->getType(*pMessage, true);
		}
		
		if (type == PGPUtility::TYPE_INLINEENCRYPTED ||
			type == PGPUtility::TYPE_INLINESIGNED) {
			pImpl_->processPGP(pPGPUtility, type, pMessage);
			bProcessed = true;
		}
		else {
			while  (type != PGPUtility::TYPE_NONE) {
				if (!pImpl_->processPGP(pPGPUtility, type, pMessage))
					break;
				type = pPGPUtility->getType(*pMessage, false);
				bProcessed = true;
			}
		}
	}
	
	unsigned int nSecurity = pMessage->getSecurity();
	if (nSecurity & Message::SECURITY_VERIFY_MASK) {
		unsigned int n = 0;
		if (nSecurity & Message::SECURITY_VERIFY_ERROR_MASK)
			n = nSecurity >> 12;
		
		WCHAR wsz[32];
		swprintf(wsz, L"%d", n);
		pMessage->setParam(L"Verification", wsz);
	}
	
	if (bProcessed && pImpl_->bStoreDecodedMessage_) {
		unsigned int nOffset = pmh->getMessageBoxKey().nOffset_;
		if (nOffset != -1)
			pImpl_->pMessageStore_->saveDecoded(nOffset, *pMessage);
	}
	
	return true;
}

void qm::Account::fireMessageHolderChanged(MessageHolder* pmh,
										   unsigned int nOldFlags,
										   unsigned int nNewFlags)
{
	assert(isLocked());
	
	typedef AccountImpl::MessageHolderHandlerList List;
	MessageHolderEvent event(pmh, nOldFlags, nNewFlags);
	for (List::const_iterator it = pImpl_->listMessageHolderHandler_.begin(); it != pImpl_->listMessageHolderHandler_.end(); ++it)
		(*it)->messageHolderChanged(event);
}

void qm::Account::fireMessageHolderDestroyed(MessageHolder* pmh)
{
	assert(isLocked());
	
	typedef AccountImpl::MessageHolderHandlerList List;
	MessageHolderEvent event(pmh);
	for (List::const_iterator it = pImpl_->listMessageHolderHandler_.begin(); it != pImpl_->listMessageHolderHandler_.end(); ++it)
		(*it)->messageHolderDestroyed(event);
}

ProtocolDriver* qm::Account::getProtocolDriver() const
{
	return pImpl_->pProtocolDriver_.get();
}

MessageHolder* qm::Account::storeMessage(NormalFolder* pFolder,
										 const CHAR* pszMessage,
										 size_t nLen,
										 const Message* pHeader,
										 unsigned int nId,
										 unsigned int nFlags,
										 unsigned int nSize,
										 bool bIndexOnly)
{
	assert(pFolder);
	assert(isLocked());
	assert(pszMessage);
	assert((nFlags & ~(MessageHolder::FLAG_USER_MASK |
		MessageHolder::FLAG_PARTIAL_MASK | MessageHolder::FLAG_LOCAL)) == 0);
	
	if (nLen == -1)
		nLen = strlen(pszMessage);
	if (nSize == -1)
		nSize = static_cast<unsigned int>(nLen);
	
	Message header;
	if (!pHeader) {
		if (!header.createHeader(pszMessage, nLen))
			return 0;
		pHeader = &header;
	}
	
	if (pHeader->isMultipart())
		nFlags |= MessageHolder::FLAG_MULTIPART;
	
	if (Security::isSMIMEEnabled()) {
		const SMIMEUtility* pSMIMEUtility = pImpl_->pSecurity_->getSMIMEUtility();
		SMIMEUtility::Type type = pSMIMEUtility->getType(pHeader->getContentType());
		if (type != SMIMEUtility::TYPE_NONE)
			nFlags |= MessageHolder::FLAG_ENVELOPED;
	}
	if (Security::isPGPEnabled()) {
		const PGPUtility* pPGPUtility = pImpl_->pSecurity_->getPGPUtility();
		PGPUtility::Type type = pPGPUtility->getType(pHeader->getContentType());
		if (type != PGPUtility::TYPE_NONE)
			nFlags |= MessageHolder::FLAG_ENVELOPED;
	}
	
	unsigned int nOffset = -1;
	unsigned int nLength = 0;
	unsigned int nHeaderLength = 0;
	unsigned int nIndexKey = -1;
	unsigned int nIndexLength = 0;
	if (!pImpl_->pMessageStore_->save(pszMessage, nLen, pHeader, bIndexOnly,
		&nOffset, &nLength, &nHeaderLength, &nIndexKey, &nIndexLength))
		return 0;
	
	SubAccount* pSubAccount = getCurrentSubAccount();
	const WCHAR* pwszFields[] = {
		L"To",
		L"Cc"
	};
	MessageHolder::Flag flags[] = {
		MessageHolder::FLAG_TOME,
		MessageHolder::FLAG_CCME
	};
	for (int n = 0; n < countof(pwszFields); ++n) {
		AddressListParser address;
		Part::Field field = pHeader->getField(pwszFields[n], &address);
		if (field == Part::FIELD_EXIST) {
			if (pSubAccount->isMyAddress(address))
				nFlags |= flags[n];
		}
	}
	
	const Time* pTime = 0;
	Time time;
	DateParser date;
	Part::Field f = pHeader->getField(L"Date", &date);
	if (f == Part::FIELD_EXIST) {
		pTime = &date.getTime();
	}
	else {
		time = Time::getCurrentTime();
		pTime = &time;
	}
	
	if (nId == static_cast<unsigned int>(-1)) {
		nId = pFolder->generateId();
		if (nId == -1)
			return 0;
	}
	
	MessageHolder::Init init = {
		nId,
		nFlags,
		MessageDate::getDate(*pTime),
		MessageDate::getTime(*pTime),
		nSize,
		nIndexKey,
		nIndexLength,
		nOffset,
		nLength,
		nHeaderLength
	};
	
	std::auto_ptr<MessageHolder> pmh(new MessageHolder(pFolder, init));
	MessageHolder* p = pmh.get();
	if (!pFolder->appendMessage(pmh))
		return 0;
	return p;
}

bool qm::Account::unstoreMessages(const MessageHolderList& l,
								  MessageOperationCallback* pCallback)
{
	if (l.empty())
		return true;
	
	NormalFolder* pFolder = l.front()->getFolder();
	
	typedef std::vector<std::pair<MessageHolder::MessageBoxKey, MessageHolder::MessageIndexKey> > KeyList;
	KeyList listKey;
	listKey.reserve(l.size());
	
	Lock<Account> lock(*this);
	
	for (MessageHolderList::const_iterator it = l.begin(); it != l.end(); ++it) {
		MessageHolder* pmh = *it;
		assert(pmh->getFolder() == pFolder);
		listKey.push_back(std::make_pair(pmh->getMessageBoxKey(), pmh->getMessageIndexKey()));
	}
	
	pFolder->removeMessages(l);
	
	for (KeyList::const_iterator it = listKey.begin(); it != listKey.end(); ++it) {
		const MessageHolder::MessageBoxKey& boxKey = (*it).first;
		const MessageHolder::MessageIndexKey& indexKey = (*it).second;
		if (!pImpl_->pMessageStore_->free(boxKey.nOffset_,
			boxKey.nLength_, indexKey.nKey_, indexKey.nLength_)) {
			// TODO LOG
		}
		pImpl_->pMessageIndex_->remove(indexKey.nKey_);
		
		if (pCallback)
			pCallback->step(1);
	}
	
	return true;
}

MessageHolder* qm::Account::cloneMessage(MessageHolder* pmh,
										 NormalFolder* pFolderTo)
{
	assert(pmh);
	assert(pFolderTo);
	assert(isLocked());
	
	const MessageHolder::MessageBoxKey& key = pmh->getMessageBoxKey();
	if (key.nOffset_ == -1) {
		MessageHolder::Init init;
		pmh->getInit(&init);
		init.nId_ = pFolderTo->generateId();
		if (init.nId_ == -1)
			return 0;
		std::auto_ptr<MessageHolder> pmhNew(new MessageHolder(pFolderTo, init));
		MessageHolder* p = pmhNew.get();
		if (!pFolderTo->appendMessage(pmhNew))
			return 0;
		return p;
	}
	else {
		Message msg;
		if (!pmh->getMessage(GETMESSAGEFLAG_POSSIBLE, 0, SECURITYMODE_NONE, &msg))
			return 0;
		xstring_size_ptr strContent(msg.getContent());
		if (!strContent.get())
			return 0;
		unsigned int nId = pFolderTo->generateId();
		if (nId == -1)
			return 0;
		unsigned int nFlags = pmh->getFlags() & (MessageHolder::FLAG_USER_MASK |
			MessageHolder::FLAG_PARTIAL_MASK | MessageHolder::FLAG_LOCAL);
		return storeMessage(pFolderTo, strContent.get(), strContent.size(),
			&msg, nId, nFlags, pmh->getSize(), false);
	}
}

bool qm::Account::updateMessage(MessageHolder* pmh,
								const CHAR* pszMessage,
								size_t nLen)
{
	assert(pmh);
	assert(pszMessage);
	
	Lock<Account> lock(*this);
	
	MessageHolder::MessageBoxKey boxKey;
	MessageHolder::MessageIndexKey indexKey;
	if (!pImpl_->pMessageStore_->save(pszMessage, nLen, 0, false,
		&boxKey.nOffset_, &boxKey.nLength_, &boxKey.nHeaderLength_,
		&indexKey.nKey_, &indexKey.nLength_))
		return false;
	
	MessageHolder::MessageIndexKey oldIndexKey = pmh->getMessageIndexKey();
	pImpl_->pMessageIndex_->remove(oldIndexKey.nKey_);
	
	MessageHolder::MessageBoxKey oldBoxKey = pmh->getMessageBoxKey();
	
	pmh->setKeys(indexKey, boxKey);
	
	if (!pImpl_->pMessageStore_->free(oldBoxKey.nOffset_,
		oldBoxKey.nLength_, oldIndexKey.nKey_, oldIndexKey.nLength_))
		return false;
	
	return true;
}

unsigned int qm::Account::generateFolderId() const
{
	unsigned int nId = 0;
	for (FolderList::const_iterator it = pImpl_->listFolder_.begin(); it != pImpl_->listFolder_.end(); ++it)
		nId = QSMAX(nId, (*it)->getId());
	return nId + 1;
}

std::auto_ptr<Logger> qm::Account::openLogger(Host host) const
{
	Time time(Time::getCurrentTime());
	WCHAR wszName[128];
	swprintf(wszName, L"%s-%04d%02d%02d%02d%02d%02d%03d-%u.log",
		getType(host), time.wYear, time.wMonth, time.wDay,
		time.wHour, time.wMinute, time.wSecond, time.wMilliseconds,
		::GetCurrentThreadId());
	
	wstring_ptr wstrDir(concat(getPath(), L"\\log"));
	W2T(wstrDir.get(), ptszDir);
	if (::GetFileAttributes(ptszDir) == 0xffffffff) {
		if (!::CreateDirectory(ptszDir, 0))
			return std::auto_ptr<Logger>(0);
	}
	
	wstring_ptr wstrPath(concat(wstrDir.get(), L"\\", wszName));
	std::auto_ptr<FileLogHandler> pLogHandler(new FileLogHandler(wstrPath.get()));
	std::auto_ptr<Logger> pLogger(new Logger(pLogHandler.get(), true, Logger::LEVEL_DEBUG));
	pLogHandler.release();
	return pLogger;
}


/****************************************************************************
 *
 * AccountHandler
 *
 */

qm::AccountHandler::~AccountHandler()
{
}


/****************************************************************************
 *
 * DefaultAccountHandler
 *
 */

qm::DefaultAccountHandler::DefaultAccountHandler()
{
}

qm::DefaultAccountHandler::~DefaultAccountHandler()
{
}

void qm::DefaultAccountHandler::currentSubAccountChanged(const AccountEvent& event)
{
}

void qm::DefaultAccountHandler::subAccountListChanged(const AccountEvent& event)
{
}

void qm::DefaultAccountHandler::folderListChanged(const FolderListChangedEvent& event)
{
}

void qm::DefaultAccountHandler::accountDestroyed(const AccountEvent& event)
{
}


/****************************************************************************
 *
 * AccountEvent
 *
 */

qm::AccountEvent::AccountEvent(Account* pAccount) :
	pAccount_(pAccount)
{
}

qm::AccountEvent::~AccountEvent()
{
}

Account* qm::AccountEvent::getAccount() const
{
	return pAccount_;
}


/****************************************************************************
 *
 * FolderListChangedEvent
 *
 */

qm::FolderListChangedEvent::FolderListChangedEvent(Account* pAccount,
												   Type type,
												   Folder* pFolder) :
	pAccount_(pAccount),
	type_(type),
	pFolder_(pFolder)
{
}

qm::FolderListChangedEvent::FolderListChangedEvent(Account* pAccount,
												   Type type,
												   Folder* pFolder,
												   unsigned int nOldFlags,
												   unsigned int nNewFlags) :
	pAccount_(pAccount),
	type_(type),
	pFolder_(pFolder),
	nOldFlags_(nOldFlags),
	nNewFlags_(nNewFlags)
{
}

qm::FolderListChangedEvent::~FolderListChangedEvent()
{
}

Account* qm::FolderListChangedEvent::getAccount() const
{
	return pAccount_;
}

FolderListChangedEvent::Type qm::FolderListChangedEvent::getType() const
{
	return type_;
}

Folder* qm::FolderListChangedEvent::getFolder() const
{
	return pFolder_;
}

unsigned int qm::FolderListChangedEvent::getOldFlags() const
{
	return nOldFlags_;
}

unsigned int qm::FolderListChangedEvent::getNewFlags() const
{
	return nNewFlags_;
}


/****************************************************************************
 *
 * AccountCheckCallback
 *
 */

qm::AccountCheckCallback::~AccountCheckCallback()
{
}


/****************************************************************************
 *
 * FolderContentHandler
 *
 */

qm::FolderContentHandler::FolderContentHandler(Account* pAccount,
											   Account::FolderList* pList) :
	pAccount_(pAccount),
	pList_(pList),
	state_(STATE_ROOT),
	bNormal_(true),
	nItem_(0),
	nId_(0),
	nParentId_(0),
	nFlags_(0),
	nCount_(0),
	nUnseenCount_(0),
	cSeparator_(L'\0'),
	nValidity_(0),
	nDownloadCount_(0),
	nDeletedCount_(0),
	bRecursive_(false)
{
}

qm::FolderContentHandler::~FolderContentHandler()
{
	std::for_each(listParam_.begin(), listParam_.end(),
		unary_compose_fx_gx(string_free<WSTRING>(), string_free<WSTRING>()));
}

bool qm::FolderContentHandler::startElement(const WCHAR* pwszNamespaceURI,
											const WCHAR* pwszLocalName,
											const WCHAR* pwszQName,
											const Attributes& attributes)
{
	struct {
		const WCHAR* pwszName_;
		unsigned int nAcceptStates_;
		State state_;
	} items[] = {
		{ L"id",			STATE_NORMALFOLDER | STATE_QUERYFOLDER,	STATE_ID			},
		{ L"parent",		STATE_NORMALFOLDER | STATE_QUERYFOLDER,	STATE_PARENT		},
		{ L"flags",			STATE_NORMALFOLDER | STATE_QUERYFOLDER,	STATE_FLAGS			},
		{ L"count",			STATE_NORMALFOLDER | STATE_QUERYFOLDER,	STATE_COUNT			},
		{ L"unseenCount",	STATE_NORMALFOLDER | STATE_QUERYFOLDER,	STATE_UNSEENCOUNT	},
		{ L"separator",		STATE_NORMALFOLDER | STATE_QUERYFOLDER,	STATE_SEPARATOR		},
		{ L"name",			STATE_NORMALFOLDER | STATE_QUERYFOLDER,	STATE_NAME			},
		{ L"validity",		STATE_NORMALFOLDER,						STATE_VALIDITY		},
		{ L"downloadCount",	STATE_NORMALFOLDER,						STATE_DOWNLOADCOUNT	},
		{ L"deletedCount",	STATE_NORMALFOLDER,						STATE_DELETEDCOUNT	},
		{ L"driver",		STATE_QUERYFOLDER,						STATE_DRIVER		},
		{ L"condition",		STATE_QUERYFOLDER,						STATE_CONDITION		},
		{ L"targetFolder",	STATE_QUERYFOLDER,						STATE_TARGETFOLDER	},
		{ L"recursive",		STATE_QUERYFOLDER,						STATE_RECURSIVE		},
		{ L"params",		STATE_NORMALFOLDER | STATE_QUERYFOLDER,	STATE_PARAMS		}
	};
	
	if (wcscmp(pwszLocalName, L"folders") == 0) {
		if (state_ != STATE_ROOT)
			return false;
		if (attributes.getLength() != 0)
			return false;
		state_ = STATE_FOLDERS;
	}
	else if (wcscmp(pwszLocalName, L"normalFolder") == 0) {
		if (state_ != STATE_FOLDERS)
			return false;
		if (attributes.getLength() != 0)
			return false;
		
		bNormal_ = true;
		nItem_ = 0;
		state_ = STATE_NORMALFOLDER;
	}
	else if (wcscmp(pwszLocalName, L"queryFolder") == 0) {
		if (state_ != STATE_FOLDERS)
			return false;
		if (attributes.getLength() != 0)
			return false;
		
		bNormal_ = false;
		nItem_ = 0;
		state_ = STATE_QUERYFOLDER;
	}
	else if (wcscmp(pwszLocalName, L"param") == 0) {
		if (state_ != STATE_PARAMS)
			return false;
		
		const WCHAR* pwszName = 0;
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrName, L"name") == 0)
				pwszName = attributes.getValue(n);
			else
				return false;
		}
		if (!pwszName)
			return false;
		
		assert(!wstrParamName_.get());
		wstrParamName_ = allocWString(pwszName);
		
		state_ = STATE_PARAM;
	}
	else {
		int n = 0;
		while (n < countof(items)) {
			if (wcscmp(pwszLocalName, items[n].pwszName_) == 0) {
				if (!processItemStartElement(items[n].nAcceptStates_,
					items[n].state_, attributes))
					return false;
				break;
			}
			++n;
		}
		if (n == countof(items))
			return false;
	}
	
	return true;
}

bool qm::FolderContentHandler::endElement(const WCHAR* pwszNamespaceURI,
										 const WCHAR* pwszLocalName,
										 const WCHAR* pwszQName)
{
	if (wcscmp(pwszLocalName, L"folders") == 0) {
		assert(state_ == STATE_FOLDERS);
		state_ = STATE_ROOT;
	}
	else if (wcscmp(pwszLocalName, L"normalFolder") == 0) {
		assert(state_ == STATE_NORMALFOLDER);
		
		if (nItem_ != 10 && nItem_ != 11)
			return false;
		
		Folder* pParent = nParentId_ != 0 ? getFolder(nParentId_) : 0;
		std::auto_ptr<NormalFolder> pFolder(new NormalFolder(nId_,
			wstrName_.get(), cSeparator_, nFlags_, nCount_, nUnseenCount_,
			nValidity_, nDownloadCount_, nDeletedCount_, pParent, pAccount_));
		pFolder->setParams(listParam_);
		pList_->push_back(pFolder.get());
		pFolder.release();
		wstrName_.reset(0);
		
		state_ = STATE_FOLDERS;
	}
	else if (wcscmp(pwszLocalName, L"queryFolder") == 0) {
		assert(state_ == STATE_QUERYFOLDER);
		
		if (nItem_ != 11 && nItem_ != 12)
			return false;
		
		Folder* pParent = nParentId_ != 0 ? getFolder(nParentId_) : 0;
		std::auto_ptr<QueryFolder> pFolder(new QueryFolder(nId_,
			wstrName_.get(), cSeparator_, nFlags_, nCount_,
			nUnseenCount_, wstrDriver_.get(), wstrCondition_.get(),
			wstrTargetFolder_.get(), bRecursive_, pParent, pAccount_));
		pFolder->setParams(listParam_);
		pList_->push_back(pFolder.get());
		pFolder.release();
		
		wstrName_.reset(0);
		wstrDriver_.reset(0);
		wstrCondition_.reset(0);
		wstrTargetFolder_.reset(0);
		
		state_ = STATE_FOLDERS;
	}
	else if (wcscmp(pwszLocalName, L"id") == 0) {
		assert(state_ == STATE_ID);
		
		if (!getNumber(&nId_))
			return false;
		
		state_ = bNormal_ ? STATE_NORMALFOLDER : STATE_QUERYFOLDER;
	}
	else if (wcscmp(pwszLocalName, L"parent") == 0) {
		assert(state_ == STATE_PARENT);
		
		if (!getNumber(&nParentId_))
			return false;
		
		state_ = bNormal_ ? STATE_NORMALFOLDER : STATE_QUERYFOLDER;
	}
	else if (wcscmp(pwszLocalName, L"flags") == 0) {
		assert(state_ == STATE_FLAGS);
		
		if (!getNumber(&nFlags_))
			return false;
		
		state_ = bNormal_ ? STATE_NORMALFOLDER : STATE_QUERYFOLDER;
	}
	else if (wcscmp(pwszLocalName, L"count") == 0) {
		assert(state_ == STATE_COUNT);
		
		if (!getNumber(&nCount_))
			return false;
		
		state_ = bNormal_ ? STATE_NORMALFOLDER : STATE_QUERYFOLDER;
	}
	else if (wcscmp(pwszLocalName, L"unseenCount") == 0) {
		assert(state_ == STATE_UNSEENCOUNT);
		
		if (!getNumber(&nUnseenCount_))
			return false;
		
		state_ = bNormal_ ? STATE_NORMALFOLDER : STATE_QUERYFOLDER;
	}
	else if (wcscmp(pwszLocalName, L"separator") == 0) {
		assert(state_ == STATE_SEPARATOR);
		
		if (buffer_.getLength() > 1)
			return false;
		
		cSeparator_ = *buffer_.getCharArray();
		buffer_.remove();
		
		state_ = bNormal_ ? STATE_NORMALFOLDER : STATE_QUERYFOLDER;
	}
	else if (wcscmp(pwszLocalName, L"name") == 0) {
		assert(state_ == STATE_NAME);
		
		if (buffer_.getLength() == 0)
			return false;
		
		assert(!wstrName_.get());
		wstrName_ = buffer_.getString();
		
		state_ = bNormal_ ? STATE_NORMALFOLDER : STATE_QUERYFOLDER;
	}
	else if (wcscmp(pwszLocalName, L"validity") == 0) {
		assert(state_ == STATE_VALIDITY);
		assert(bNormal_);
		
		if (!getNumber(&nValidity_))
			return false;
		
		state_ = STATE_NORMALFOLDER;
	}
	else if (wcscmp(pwszLocalName, L"downloadCount") == 0) {
		assert(state_ == STATE_DOWNLOADCOUNT);
		assert(bNormal_);
		
		if (!getNumber(&nDownloadCount_))
			return false;
		
		state_ = STATE_NORMALFOLDER;
	}
	else if (wcscmp(pwszLocalName, L"deletedCount") == 0) {
		assert(state_ == STATE_DELETEDCOUNT);
		assert(bNormal_);
		
		if (!getNumber(&nDeletedCount_))
			return false;
		
		state_ = STATE_NORMALFOLDER;
	}
	else if (wcscmp(pwszLocalName, L"driver") == 0) {
		assert(state_ == STATE_DRIVER);
		assert(!bNormal_);
		
		assert(!wstrDriver_.get());
		wstrDriver_ = buffer_.getString();
		
		state_ = STATE_QUERYFOLDER;
	}
	else if (wcscmp(pwszLocalName, L"condition") == 0) {
		assert(state_ == STATE_CONDITION);
		assert(!bNormal_);
		
		assert(!wstrCondition_.get());
		wstrCondition_ = buffer_.getString();
		
		state_ = STATE_QUERYFOLDER;
	}
	else if (wcscmp(pwszLocalName, L"targetFolder") == 0) {
		assert(state_ == STATE_TARGETFOLDER);
		assert(!bNormal_);
		
		assert(!wstrTargetFolder_.get());
		wstring_ptr wstrTargetFolder(buffer_.getString());
		if (*wstrTargetFolder.get())
			wstrTargetFolder_ = wstrTargetFolder;
		
		state_ = STATE_QUERYFOLDER;
	}
	else if (wcscmp(pwszLocalName, L"recursive") == 0) {
		assert(state_ == STATE_RECURSIVE);
		
		unsigned int nRecursive = 0;
		if (!getNumber(&nRecursive))
			return false;
		bRecursive_ = nRecursive != 0;
		
		state_ = STATE_QUERYFOLDER;
	}
	else if (wcscmp(pwszLocalName, L"params") == 0) {
		assert(state_ == STATE_PARAMS);
		state_ = bNormal_ ? STATE_NORMALFOLDER : STATE_QUERYFOLDER;
	}
	else if (wcscmp(pwszLocalName, L"param") == 0) {
		assert(state_ == STATE_PARAM);
		
		assert(wstrParamName_.get());
		wstring_ptr wstrValue(buffer_.getString());
		listParam_.push_back(Folder::ParamList::value_type(
			wstrParamName_.get(), wstrValue.get()));
		wstrParamName_.release();
		wstrValue.release();
		
		state_ = STATE_PARAMS;
	}
	else {
		return false;
	}
	
	return true;
}

bool qm::FolderContentHandler::characters(const WCHAR* pwsz,
										  size_t nStart,
										  size_t nLength)
{
	if (state_ == STATE_ID ||
		state_ == STATE_PARENT ||
		state_ == STATE_FLAGS ||
		state_ == STATE_COUNT ||
		state_ == STATE_UNSEENCOUNT ||
		state_ == STATE_SEPARATOR ||
		state_ == STATE_NAME ||
		state_ == STATE_VALIDITY ||
		state_ == STATE_DOWNLOADCOUNT ||
		state_ == STATE_DELETEDCOUNT ||
		state_ == STATE_DRIVER ||
		state_ == STATE_CONDITION ||
		state_ == STATE_TARGETFOLDER ||
		state_ == STATE_RECURSIVE ||
		state_ == STATE_PARAM) {
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

bool qm::FolderContentHandler::processItemStartElement(unsigned int nAcceptStates,
													   State state,
													   const Attributes& attributes)
{
	if ((state_ & nAcceptStates) == 0)
		return false;
	if (attributes.getLength() != 0)
		return false;
	
	++nItem_;
	state_ = state;
	
	return true;
}

Folder* qm::FolderContentHandler::getFolder(unsigned int nId) const
{
	Account::FolderList::const_iterator it = std::find_if(
		pList_->begin(), pList_->end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				std::equal_to<unsigned int>(),
				std::mem_fun(&Folder::getId),
				std::identity<unsigned int>()),
			nId));
	return it != pList_->end() ? *it : 0;
}

bool qm::FolderContentHandler::getNumber(unsigned int* pn)
{
	if (buffer_.getLength() == 0)
		return false;
	
	WCHAR* pEnd = 0;
	*pn = wcstol(buffer_.getCharArray(), &pEnd, 10);
	buffer_.remove();
	if (*pEnd)
		return false;
	
	return true;
}


/****************************************************************************
 *
 * FolderWriter
 *
 */

qm::FolderWriter::FolderWriter(Writer* pWriter) :
	handler_(pWriter)
{
}

qm::FolderWriter::~FolderWriter()
{
}

bool qm::FolderWriter::write(const Account::FolderList& l)
{
	DefaultAttributes attrs;
	
	if (!handler_.startDocument())
		return false;
	if (!handler_.startElement(0, 0, L"folders", attrs))
		return false;
	
	for (Account::FolderList::const_iterator it = l.begin(); it != l.end(); ++it) {
		Folder* pFolder = *it;
		const WCHAR* pwszQName = 0;
		switch (pFolder->getType()) {
		case Folder::TYPE_NORMAL:
			pwszQName = L"normalFolder";
			break;
		case Folder::TYPE_QUERY:
			pwszQName = L"queryFolder";
			break;
		default:
			assert(false);
			break;
		}
		if (!handler_.startElement(0, 0, pwszQName, attrs))
			return false;
		
		if (!HandlerHelper::numberElement(&handler_, L"id", pFolder->getId()))
			return false;
		Folder* pParent = pFolder->getParentFolder();
		if (!HandlerHelper::numberElement(&handler_, L"parent", pParent ? pParent->getId() : 0))
			return false;
		if (!HandlerHelper::numberElement(&handler_, L"flags", pFolder->getFlags()))
			return false;
		if (!HandlerHelper::numberElement(&handler_, L"count", pFolder->getCount()))
			return false;
		if (!HandlerHelper::numberElement(&handler_, L"unseenCount", pFolder->getUnseenCount()))
			return false;
		WCHAR cSeparator = pFolder->getSeparator();
		if (!HandlerHelper::textElement(&handler_, L"separator", &cSeparator,
			cSeparator == L'\0' ? 0 : 1))
			return false;
		if (!HandlerHelper::textElement(&handler_, L"name", pFolder->getName(), -1))
			return false;
		switch (pFolder->getType()) {
		case Folder::TYPE_NORMAL:
			{
				NormalFolder* pNormalFolder = static_cast<NormalFolder*>(pFolder);
				if (!HandlerHelper::numberElement(&handler_, L"validity", pNormalFolder->getValidity()))
					return false;
				if (!HandlerHelper::numberElement(&handler_, L"downloadCount", pNormalFolder->getDownloadCount()))
					return false;
				if (!HandlerHelper::numberElement(&handler_, L"deletedCount", pNormalFolder->getDeletedCount()))
					return false;
			}
			break;
		case Folder::TYPE_QUERY:
			{
				QueryFolder* pQueryFolder = static_cast<QueryFolder*>(pFolder);
				if (!HandlerHelper::textElement(&handler_, L"driver", pQueryFolder->getDriver(), -1))
					return false;
				if (!HandlerHelper::textElement(&handler_, L"condition", pQueryFolder->getCondition(), -1))
					return false;
				const WCHAR* pwszTargetFolder = pQueryFolder->getTargetFolder();
				if (!pwszTargetFolder)
					pwszTargetFolder = L"";
				if (!HandlerHelper::textElement(&handler_, L"targetFolder", pwszTargetFolder, -1))
					return false;
				if (!HandlerHelper::numberElement(&handler_, L"recursive", pQueryFolder->isRecursive()))
					return false;
			}
			break;
		default:
			assert(false);
			break;
		}
		
		const Folder::ParamList& listParam = pFolder->getParams();
		if (!listParam.empty()) {
			if (!handler_.startElement(0, 0, L"params", attrs))
				return false;
			for (Folder::ParamList::const_iterator it = listParam.begin(); it != listParam.end(); ++it) {
				SimpleAttributes attrs(L"name", (*it).first);
				if (!handler_.startElement(0, 0, L"param", attrs) ||
					!handler_.characters((*it).second, 0, wcslen((*it).second)) ||
					!handler_.endElement(0, 0, L"param"))
					return false;
			}
			if (!handler_.endElement(0, 0, L"params"))
				return false;
		}
		
		if (!handler_.endElement(0, 0, pwszQName))
			return false;
	}
	
	if (!handler_.endElement(0, 0, L"folders"))
		return false;
	if (!handler_.endDocument())
		return false;
	
	return true;
}
