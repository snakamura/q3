/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmaccount.h>
#include <qmfolder.h>
#include <qmmessage.h>
#include <qmmessageholder.h>

#include <qsassert.h>
#include <qsthread.h>

#include <algorithm>

#include "imap4driver.h"
#include "offlinejob.h"
#include "option.h"

#pragma warning(disable:4786)

using namespace qmimap4;
using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * Imap4Driver
 *
 */

qmimap4::Imap4Driver::Imap4Driver(Account* pAccount,
								  const Security* pSecurity) :
	pAccount_(pAccount),
	pSecurity_(pSecurity),
	bOffline_(true)
{
	pOfflineJobManager_.reset(new OfflineJobManager(pAccount_->getPath()));
}

qmimap4::Imap4Driver::~Imap4Driver()
{
}

bool qmimap4::Imap4Driver::init()
{
	return true;
}

bool qmimap4::Imap4Driver::save()
{
	Lock<CriticalSection> lock(cs_);
	
	return pOfflineJobManager_->save(pAccount_->getPath());
}

bool qmimap4::Imap4Driver::isSupport(Account::Support support)
{
	switch (support) {
	case Account::SUPPORT_REMOTEFOLDER:
		return true;
	case Account::SUPPORT_LOCALFOLDERDOWNLOAD:
		return false;
	case Account::SUPPORT_LOCALFOLDERGETMESSAGE:
		return false;
	default:
		assert(false);
		return false;
	}
}

void qmimap4::Imap4Driver::setOffline(bool bOffline)
{
	Lock<CriticalSection> lock(cs_);
	
	if (!bOffline_ && bOffline)
		pSessionCache_.reset(0);
	
	bOffline_ = bOffline;
}

std::auto_ptr<NormalFolder> qmimap4::Imap4Driver::createFolder(SubAccount* pSubAccount,
															   const WCHAR* pwszName,
															   Folder* pParent)
{
	assert(pSubAccount);
	assert(pwszName);
	
	Lock<CriticalSection> lock(cs_);
	
	wstring_ptr wstrRootFolder(pAccount_->getProperty(L"Imap4", L"RootFolder", L""));
	
	if (!prepareSessionCache(pSubAccount))
		return false;
	
	SessionCacher cacher(pSessionCache_.get(), 0);
	Imap4* pImap4 = cacher.get();
	if (!pImap4)
		return 0;
	
	WCHAR cSeparator = L'/';
	wstring_ptr wstrFullName;
	if (pParent) {
		wstring_ptr wstrParentName(pParent->getFullName());
		cSeparator = pParent->getSeparator();
		ConcatW c[] = {
			{ wstrRootFolder.get(),	-1	},
			{ L"/",					1	},
			{ wstrParentName.get(),	-1	},
			{ &cSeparator,			1	},
			{ pwszName,				-1	}
		};
		bool bChildOfRoot = pParent->isFlag(Folder::FLAG_CHILDOFROOT) &&
			*wstrRootFolder.get() != L'\0';
		wstrFullName = concat(c + (bChildOfRoot ? 0 : 2),
			countof(c) - (bChildOfRoot ? 0 : 2));
	}
	else {
		ConcatW c[] = {
			{ wstrRootFolder.get(),	-1	},
			{ L"/",					1	},
			{ pwszName,				-1	}
		};
		bool bChildOfRoot = *wstrRootFolder.get() != L'\0';
		wstrFullName = concat(c + (bChildOfRoot ? 0 : 2),
			countof(c) - (bChildOfRoot ? 0 : 2));
	}
	
	if (!pImap4->create(wstrFullName.get()))
		return false;
	
	wstring_ptr wstrName;
	size_t nFullNameLen = wcslen(wstrFullName.get());
	if (wstrFullName[nFullNameLen - 1] == cSeparator) {
		wstrFullName[nFullNameLen - 1] = L'\0';
		
		wstrName = allocWString(pwszName, wcslen(pwszName) - 1);
		pwszName = wstrName.get();
	}
	
	FolderUtil folderUtil(pAccount_);
	
	struct ListProcessHook : public ProcessHook
	{
		ListProcessHook(const WCHAR* pwszName,
						const FolderUtil& folderUtil) :
			pwszName_(pwszName),
			folderUtil_(folderUtil),
			nFlags_(0),
			cSeparator_(L'\0'),
			bFound_(false)
		{
		};
		
		virtual bool processListResponse(ResponseList* pList)
		{
			if (Util::isEqualFolderName(pList->getMailbox(), pwszName_, pList->getSeparator())) {
				bFound_ = true;
				wstring_ptr wstrName;
				folderUtil_.getFolderData(pList->getMailbox(),
					pList->getSeparator(), pList->getAttributes(), &wstrName, &nFlags_);
				cSeparator_ = pList->getSeparator();
			}
			return true;
		}
		
		const WCHAR* pwszName_;
		const FolderUtil& folderUtil_;
		unsigned int nFlags_;
		WCHAR cSeparator_;
		bool bFound_;
	} hook(wstrFullName.get(), folderUtil);
	
	Hook h(pCallback_.get(), &hook);
	if (!pImap4->list(false, L"", wstrFullName.get()))
		return false;
	if (!hook.bFound_)
		return false;
	
	cacher.release();
	
	return new NormalFolder(pAccount_->generateFolderId(), pwszName,
		hook.cSeparator_, hook.nFlags_, 0, 0, 0, 0, 0, pParent, pAccount_);
}

bool qmimap4::Imap4Driver::removeFolder(SubAccount* pSubAccount,
										NormalFolder* pFolder)
{
	assert(pSubAccount);
	assert(pFolder);
	
	Lock<CriticalSection> lock(cs_);
	
	if (!prepareSessionCache(pSubAccount))
		return false;
	
	SessionCacher cacher(pSessionCache_.get(), 0);
	Imap4* pImap4 = cacher.get();
	if (!pImap4)
		return 0;
	
	wstring_ptr wstrName(Util::getFolderName(pFolder));
	
	if (!pImap4->remove(wstrName.get()))
		return false;
	
	cacher.release();
	
	return true;
}

bool qmimap4::Imap4Driver::renameFolder(SubAccount* pSubAccount,
										NormalFolder* pFolder,
										const WCHAR* pwszName)
{
	assert(pSubAccount);
	assert(pFolder);
	assert(pwszName);
	
	Lock<CriticalSection> lock(cs_);
	
	if (!prepareSessionCache(pSubAccount))
		return false;
	
	SessionCacher cacher(pSessionCache_.get(), 0);
	Imap4* pImap4 = cacher.get();
	if (!pImap4)
		return 0;
	
	wstring_ptr wstrOldName(Util::getFolderName(pFolder));
	
	wstring_ptr wstrNewName(allocWString(wstrOldName.get(),
		wcslen(wstrOldName.get()) + wcslen(pwszName) + 1));
	WCHAR* p = wcsrchr(wstrNewName.get(), pFolder->getSeparator());
	p = p ? p + 1 : wstrNewName.get();
	wcscpy(p, pwszName);
	
	if (!pImap4->rename(wstrOldName.get(), wstrNewName.get()))
		return false;
	
	cacher.release();
	
	return true;
}

bool qmimap4::Imap4Driver::createDefaultFolders(Account::FolderList* pList)
{
	assert(pList);
	return true;
}

bool qmimap4::Imap4Driver::getRemoteFolders(SubAccount* pSubAccount,
											RemoteFolderList* pList)
{
	assert(pSubAccount);
	assert(pList);
	
	Lock<CriticalSection> lock(cs_);
	
	FolderUtil::saveSpecialFolders(pAccount_);
	
	FolderListGetter getter(pAccount_, pSubAccount, pSecurity_);
	if (!getter.update())
		return false;
	getter.getFolders(pList);
	
	return true;
}

bool qmimap4::Imap4Driver::getMessage(SubAccount* pSubAccount,
									  MessageHolder* pmh,
									  unsigned int nFlags,
									  xstring_ptr* pstrMessage,
									  Message::Flag* pFlag,
									  bool* pbMadeSeen)
{
	assert(pSubAccount);
	assert(pmh);
	assert(pstrMessage);
	assert(pFlag);
	assert(pbMadeSeen);
	assert(!pmh->getFolder()->isFlag(Folder::FLAG_LOCAL));
	assert(!pmh->isFlag(MessageHolder::FLAG_LOCAL));
	
	pstrMessage->reset(0);
	*pFlag = Message::FLAG_EMPTY;
	*pbMadeSeen = false;
	
	if (bOffline_)
		return true;
	
	Lock<CriticalSection> lock(cs_);
	
	int nOption = pSubAccount->getProperty(L"Imap4", L"Option", 0xff);
	
	if (!prepareSessionCache(pSubAccount))
		return false;
	
	SessionCacher cacher(pSessionCache_.get(), pmh->getFolder());
	Imap4* pImap4 = cacher.get();
	if (!pImap4)
		return false;
	
	struct BodyProcessHook : public ProcessHook
	{
		BodyProcessHook(unsigned int nUid,
						bool bHeaderOnly,
						MessageHolder* pmh,
						xstring_ptr* pstrMessage,
						Message::Flag* pFlag) :
			nUid_(nUid),
			bHeaderOnly_(bHeaderOnly),
			pmh_(pmh),
			pstrMessage_(pstrMessage),
			pFlag_(pFlag)
		{
		}
		
		virtual bool processFetchResponse(ResponseFetch* pFetch)
		{
			unsigned int nUid = 0;
			FetchDataBody* pBody = 0;
			unsigned int nMask = MessageHolder::FLAG_SEEN |
				MessageHolder::FLAG_REPLIED | MessageHolder::FLAG_DRAFT |
				MessageHolder::FLAG_DELETED | MessageHolder::FLAG_MARKED;
			
			const ResponseFetch::FetchDataList& l = pFetch->getFetchDataList();
			for (ResponseFetch::FetchDataList::const_iterator it = l.begin(); it != l.end(); ++it) {
				switch ((*it)->getType()) {
				case FetchData::TYPE_BODY:
					pBody = static_cast<FetchDataBody*>(*it);
					break;
				case FetchData::TYPE_UID:
					nUid = static_cast<FetchDataUid*>(*it)->getUid();
					break;
				case FetchData::TYPE_FLAGS:
					pmh_->setFlags(Util::getMessageFlagsFromImap4Flags(
						static_cast<FetchDataFlags*>(*it)->getSystemFlags(),
						static_cast<FetchDataFlags*>(*it)->getCustomFlags()), nMask);
					break;
				}
			}
			
			if (nUid == nUid_ && pBody) {
				FetchDataBody::Section s = pBody->getSection();
				if (((s == FetchDataBody::SECTION_NONE && !bHeaderOnly_) ||
					(s == FetchDataBody::SECTION_HEADER && bHeaderOnly_)) &&
					pBody->getPartPath().empty()) {
					// TODO
					xstring_ptr strContent(allocXString(pBody->getContent()));
					if (!strContent.get())
						return false;
					*pstrMessage_ = strContent;
					*pFlag_ = bHeaderOnly_ ? Message::FLAG_HEADERONLY : Message::FLAG_NONE;
				}
			}
			
			return true;
		}
		
		unsigned int nUid_;
		bool bHeaderOnly_;
		MessageHolder* pmh_;
		xstring_ptr* pstrMessage_;
		Message::Flag* pFlag_;
	};
	
	struct BodyStructureProcessHook : public ProcessHook
	{
		BodyStructureProcessHook(unsigned int nUid) :
			nUid_(nUid),
			pBodyStructure_(0)
		{
		}
		
		FetchDataBodyStructure* getBodyStructure() const
		{
			return pBodyStructure_.get();
		}
		
		virtual bool processFetchResponse(ResponseFetch* pFetch)
		{
			unsigned int nUid = 0;
			FetchDataBodyStructure* pBodyStructure = 0;
			
			const ResponseFetch::FetchDataList& l = pFetch->getFetchDataList();
			for (ResponseFetch::FetchDataList::const_iterator it = l.begin(); it != l.end(); ++it) {
				switch ((*it)->getType()) {
				case FetchData::TYPE_BODYSTRUCTURE:
					pBodyStructure = static_cast<FetchDataBodyStructure*>(*it);
					break;
				case FetchData::TYPE_UID:
					nUid = static_cast<FetchDataUid*>(*it)->getUid();
					break;
				}
			}
			
			if (nUid == nUid_ && pBodyStructure) {
				pBodyStructure_.reset(pBodyStructure);
				pFetch->detach(pBodyStructure);
			}
			
			return true;
		}
		
		unsigned int nUid_;
		std::auto_ptr<FetchDataBodyStructure> pBodyStructure_;
	};
	
	struct BodyListProcessHook : public ProcessHook
	{
		typedef Util::BodyList BodyList;
		typedef Util::PartList PartList;
		
		BodyListProcessHook(unsigned int nUid,
							FetchDataBodyStructure* pBodyStructure,
							const PartList& listPart,
							unsigned int nPartCount,
							MessageHolder* pmh,
							xstring_ptr* pstrMessage,
							Message::Flag* pFlag) :
			nUid_(nUid),
			pBodyStructure_(pBodyStructure),
			listPart_(listPart),
			nPartCount_(nPartCount),
			pmh_(pmh),
			pstrMessage_(pstrMessage),
			pFlag_(pFlag)
		{
		}
		
		virtual bool processFetchResponse(ResponseFetch* pFetch)
		{
			unsigned int nUid = 0;
			BodyList listBody;
			
			unsigned int nMask = MessageHolder::FLAG_SEEN |
				MessageHolder::FLAG_REPLIED | MessageHolder::FLAG_DRAFT |
				MessageHolder::FLAG_DELETED | MessageHolder::FLAG_MARKED;
			
			const ResponseFetch::FetchDataList& l = pFetch->getFetchDataList();
			for (ResponseFetch::FetchDataList::const_iterator it = l.begin(); it != l.end(); ++it) {
				switch ((*it)->getType()) {
				case FetchData::TYPE_BODY:
					{
						FetchDataBody* pBody = static_cast<FetchDataBody*>(*it);
						bool bAdd = false;
						if (pBody->getSection() == FetchDataBody::SECTION_HEADER) {
							bAdd = true;
						}
						else if (pBody->getSection() == FetchDataBody::SECTION_NONE ||
							pBody->getSection() == FetchDataBody::SECTION_MIME) {
							const FetchDataBody::PartPath& path = pBody->getPartPath();
							PartList::const_iterator part = std::find_if(
								listPart_.begin(), listPart_.end(),
								unary_compose_f_gx(
									PathEqual(&path[0], path.size()),
									std::select2nd<PartList::value_type>()));
							bAdd = part != listPart_.end();
						}
						if (bAdd)
							listBody.push_back(pBody);
					}
					break;
				case FetchData::TYPE_UID:
					nUid = static_cast<FetchDataUid*>(*it)->getUid();
					break;
				case FetchData::TYPE_FLAGS:
					pmh_->setFlags(Util::getMessageFlagsFromImap4Flags(
						static_cast<FetchDataFlags*>(*it)->getSystemFlags(),
						static_cast<FetchDataFlags*>(*it)->getCustomFlags()), nMask);
					break;
				}
			}
			
			if (listBody.size() == nPartCount_ && nUid == nUid_) {
				xstring_ptr strContent(Util::getContentFromBodyStructureAndBodies(listPart_, listBody));
				if (!strContent.get())
					return false;
				*pstrMessage_ = strContent;
				*pFlag_ = Message::FLAG_TEXTONLY;
			}
			
			return true;
		}
		
		unsigned int nUid_;
		FetchDataBodyStructure* pBodyStructure_;
		const PartList& listPart_;
		unsigned int nPartCount_;
		MessageHolder* pmh_;
		xstring_ptr* pstrMessage_;
		Message::Flag* pFlag_;
	};
	
	SingleRange range(pmh->getId(), true);
	
	bool bBodyStructure = false;
	bool bHeaderOnly = false;
	bool bHtml = false;
	
	switch (nFlags & Account::GETMESSAGEFLAG_METHOD_MASK) {
	case Account::GETMESSAGEFLAG_ALL:
		break;
	case Account::GETMESSAGEFLAG_HEADER:
		bHeaderOnly = true;
		break;
	case Account::GETMESSAGEFLAG_TEXT:
		bBodyStructure = pmh->isFlag(MessageHolder::FLAG_MULTIPART);
		break;
	case Account::GETMESSAGEFLAG_HTML:
		bBodyStructure = pmh->isFlag(MessageHolder::FLAG_MULTIPART);
		bHtml = true;
		break;
	default:
		assert(false);
		return false;
	}
	
	if (bBodyStructure) {
		BodyStructureProcessHook hook(pmh->getId());
		Hook h(pCallback_.get(), &hook);
		if (!pImap4->getBodyStructure(range))
			return false;
		FetchDataBodyStructure* pBodyStructure = hook.getBodyStructure();
		if (pBodyStructure) {
			Util::PartList listPart;
			Util::PartListDeleter deleter(listPart);
			unsigned int nPath = 0;
			Util::getPartsFromBodyStructure(pBodyStructure, &nPath, &listPart);
			
			if (!listPart.empty()) {
				string_ptr strArg;
				unsigned int nPartCount = 0;
				bool bAll = false;
				Util::getFetchArgFromPartList(listPart,
					bHtml ? Util::FETCHARG_HTML : Util::FETCHARG_TEXT,
					false, (nOption & OPTION_TRUSTBODYSTRUCTURE) == 0,
					&strArg, &nPartCount, &bAll);
				
				BodyListProcessHook hook(pmh->getId(), pBodyStructure,
					listPart, nPartCount, pmh, pstrMessage, pFlag);
				Hook h(pCallback_.get(), &hook);
				
				if (!pImap4->fetch(range, strArg.get()))
					return false;
			}
		}
	}
	else {
		if (bHeaderOnly) {
			BodyProcessHook hook(pmh->getId(), true, pmh, pstrMessage, pFlag);
			Hook h(pCallback_.get(), &hook);
			if (!pImap4->getHeader(range,
				(nFlags & Account::GETMESSAGEFLAG_MAKESEEN) == 0))
				return false;
		}
		else {
			BodyProcessHook hook(pmh->getId(), false, pmh, pstrMessage, pFlag);
			Hook h(pCallback_.get(), &hook);
			if (!pImap4->getMessage(range,
				(nFlags & Account::GETMESSAGEFLAG_MAKESEEN) == 0))
				return false;
		}
	}
	
	cacher.release();
	
	*pbMadeSeen = *pFlag != Message::FLAG_EMPTY;
	
	return true;
}

bool qmimap4::Imap4Driver::setMessagesFlags(SubAccount* pSubAccount,
											NormalFolder* pFolder,
											const MessageHolderList& l,
											unsigned int nFlags,
											unsigned int nMask)
{
	assert(pSubAccount);
	assert(pFolder);
	assert(!l.empty());
	assert(std::find_if(l.begin(), l.end(),
		std::not1(
			std::bind2nd(
				binary_compose_f_gx_hy(
					std::equal_to<Folder*>(),
					std::mem_fun(&MessageHolder::getFolder),
					std::identity<Folder*>()),
				pFolder))) == l.end());
	
	MessageHolderList listUpdate;
	listUpdate.reserve(l.size());
	for (MessageHolderList::const_iterator it = l.begin(); it != l.end(); ++it) {
		MessageHolder* pmh = *it;
		if ((pmh->getFlags() & nMask) != nFlags) {
			if (pmh->isFlag(MessageHolder::FLAG_LOCAL))
				pmh->setFlags(nFlags, nMask);
			else
				listUpdate.push_back(pmh);
		}
	}
	if (listUpdate.empty())
		return true;
	
	if (bOffline_) {
		Util::UidList listUid;
		Util::createUidList(listUpdate, &listUid);
		
		if (!listUid.empty()) {
			wstring_ptr wstrFolder(pFolder->getFullName());
			std::auto_ptr<SetFlagsOfflineJob> pJob(new SetFlagsOfflineJob(
				wstrFolder.get(), listUid, nFlags, nMask));
			pOfflineJobManager_->add(pJob);
		}
		
		for (MessageHolderList::iterator it = listUpdate.begin(); it != listUpdate.end(); ++it)
			(*it)->setFlags(nFlags, nMask);
	}
	else {
		std::auto_ptr<MultipleRange> pRange(Util::createRange(listUpdate));
		
		Lock<CriticalSection> lock(cs_);
		
		if (!prepareSessionCache(pSubAccount))
			return false;
		
		SessionCacher cacher(pSessionCache_.get(), pFolder);
		Imap4* pImap4 = cacher.get();
		if (!pImap4)
			return false;
		
		if (!setFlags(pImap4, *pRange, pFolder, listUpdate, nFlags, nMask))
			return false;
		
		cacher.release();
	}
	
	return true;
}

bool qmimap4::Imap4Driver::appendMessage(SubAccount* pSubAccount,
										 NormalFolder* pFolder,
										 const CHAR* pszMessage,
										 unsigned int nFlags)
{
	assert(pSubAccount);
	assert(pFolder);
	assert(pszMessage);
	
	Lock<Account> lock(*pAccount_);
	
	if (bOffline_) {
		MessageHolder* pmh = pAccount_->storeMessage(pFolder, pszMessage, 0,
			-1, nFlags | MessageHolder::FLAG_LOCAL, -1, false);
		if (!pmh)
			return false;
		
		wstring_ptr wstrFolder(pFolder->getFullName());
		std::auto_ptr<AppendOfflineJob> pJob(
			new AppendOfflineJob(wstrFolder.get(), pmh->getId()));
		pOfflineJobManager_->add(pJob);
	}
	else {
		Lock<CriticalSection> lock(cs_);
		
		if (!prepareSessionCache(pSubAccount))
			return false;
		
		SessionCacher cacher(pSessionCache_.get(), 0);
		Imap4* pImap4 = cacher.get();
		if (!pImap4)
			return false;
		
		wstring_ptr wstrFolderName(Util::getFolderName(pFolder));
		
		Flags flags(Util::getImap4FlagsFromMessageFlags(nFlags));
		if (!pImap4->append(wstrFolderName.get(), pszMessage, flags))
			return false;
		
		cacher.release();
	}
	
	return true;
}

bool qmimap4::Imap4Driver::removeMessages(SubAccount* pSubAccount,
										  NormalFolder* pFolder,
										  const MessageHolderList& l)
{
	assert(pSubAccount);
	assert(pFolder);
	assert(!l.empty());
	assert(std::find_if(l.begin(), l.end(),
		std::not1(
			std::bind2nd(
				binary_compose_f_gx_hy(
					std::equal_to<Folder*>(),
					std::mem_fun(&MessageHolder::getFolder),
					std::identity<Folder*>()),
				pFolder))) == l.end());
	
	return setMessagesFlags(pSubAccount, pFolder, l,
		MessageHolder::FLAG_DELETED, MessageHolder::FLAG_DELETED);
}

bool qmimap4::Imap4Driver::copyMessages(SubAccount* pSubAccount,
										const MessageHolderList& l,
										NormalFolder* pFolderFrom,
										NormalFolder* pFolderTo,
										bool bMove)
{
	assert(pSubAccount);
	assert(!l.empty());
	assert(pFolderFrom);
	assert(pFolderTo);
	assert(pAccount_->isLocked());
	assert(std::find_if(l.begin(), l.end(),
		std::not1(
			std::bind2nd(
				binary_compose_f_gx_hy(
					std::equal_to<Folder*>(),
					std::mem_fun(&MessageHolder::getFolder),
					std::identity<Folder*>()),
				pFolderFrom))) == l.end());
	
	MessageHolderList listUpdate;
	listUpdate.reserve(l.size());
	Util::UidList listLocalUid;
	listLocalUid.reserve(l.size());
	for (MessageHolderList::const_iterator it = l.begin(); it != l.end(); ++it) {
		MessageHolder* pmh = *it;
		if (pmh->isFlag(MessageHolder::FLAG_LOCAL))
			listLocalUid.push_back(pmh->getId());
		else
			listUpdate.push_back(pmh);
	}
	
	if (!pOfflineJobManager_->copyJobs(pFolderFrom, pFolderTo, listLocalUid, bMove))
		return false;
	
	if (listUpdate.empty())
		return true;
	
	if (bOffline_) {
		CopyOfflineJob::ItemList listItemTo;
		listItemTo.reserve(listUpdate.size());
		
		for (MessageHolderList::iterator it = listUpdate.begin(); it != listUpdate.end(); ++it) {
			MessageHolder* pmh = *it;
			MessageHolder* pmhClone = pAccount_->cloneMessage(pmh, pFolderTo);
			pmhClone->setFlags(MessageHolder::FLAG_LOCAL, MessageHolder::FLAG_LOCAL);
			if (bMove)
				pmh->setFlags(MessageHolder::FLAG_DELETED,
					MessageHolder::FLAG_DELETED);
			CopyOfflineJob::Item item = {
				pmhClone->getId(),
				pmhClone->getFlags()
			};
			listItemTo.push_back(item);
		}
		
		Util::UidList listUidFrom;
		Util::createUidList(listUpdate, &listUidFrom);
		
		assert(listUidFrom.size() == listItemTo.size());
		
		if (!listUidFrom.empty()) {
			wstring_ptr wstrFolderFrom(pFolderFrom->getFullName());
			wstring_ptr wstrFolderTo(pFolderTo->getFullName());
			std::auto_ptr<CopyOfflineJob> pJob(new CopyOfflineJob(
				wstrFolderFrom.get(), wstrFolderTo.get(), listUidFrom, listItemTo, bMove));
			pOfflineJobManager_->add(pJob);
		}
	}
	else {
		std::auto_ptr<MultipleRange> pRange(Util::createRange(listUpdate));
		
		Lock<CriticalSection> lock(cs_);
		
		if (!prepareSessionCache(pSubAccount))
			return false;
		
		SessionCacher cacher(pSessionCache_.get(), pFolderFrom);
		Imap4* pImap4 = cacher.get();
		if (!pImap4)
			return false;
		
		wstring_ptr wstrFolderName(Util::getFolderName(pFolderTo));
		if (!pImap4->copy(*pRange, wstrFolderName.get()))
			return false;
		
		cacher.release();
		
		if (bMove) {
			if (!setFlags(pImap4, *pRange, pFolderFrom, listUpdate,
				MessageHolder::FLAG_DELETED, MessageHolder::FLAG_DELETED))
				return false;
		}
	}
	
	return true;
}

bool qmimap4::Imap4Driver::clearDeletedMessages(SubAccount* pSubAccount,
												NormalFolder* pFolder)
{
	assert(pSubAccount);
	assert(pFolder);
	
	if (bOffline_) {
		wstring_ptr wstrFolder(pFolder->getFullName());
		std::auto_ptr<ExpungeOfflineJob> pJob(
			new ExpungeOfflineJob(wstrFolder.get()));
		pOfflineJobManager_->add(pJob);
	}
	else {
		Lock<CriticalSection> lock(cs_);
		
		if (!prepareSessionCache(pSubAccount))
			return false;
		
		SessionCacher cacher(pSessionCache_.get(), pFolder);
		Imap4* pImap4 = cacher.get();
		if (!pImap4)
			return false;
		
		struct ExpungeProcessHook : public ProcessHook
		{
			virtual bool processExpungeResponse(ResponseExpunge* pExpunge)
			{
				unsigned int n = pExpunge->getExpunge() - 1;
				unsigned int nMessage = n;
				for (UidList::iterator it = listUid_.begin(); it != listUid_.end(); ++it) {
					if ((*it).first <= n)
						++nMessage;
				}
				listUid_.push_back(UidList::value_type(n, nMessage));
				
				return true;
			}
			
			typedef std::vector<std::pair<unsigned int, unsigned int> > UidList;
			UidList listUid_;
		} hook;
		
		Hook h(pCallback_.get(), &hook);
		if (!pImap4->expunge())
			return false;
		
		cacher.release();
		
		Lock<Account> lock2(*pAccount_);
		
		MessageHolderList l;
		l.resize(hook.listUid_.size());
		for (ExpungeProcessHook::UidList::size_type n = 0; n < hook.listUid_.size(); ++n)
			l[n] = pFolder->getMessage(hook.listUid_[n].second);
		
		for (MessageHolderList::const_iterator it = l.begin(); it != l.end(); ++it) {
			if (!pAccount_->unstoreMessage(*it))
				return false;
		}
	}
	
	return true;
}

OfflineJobManager* qmimap4::Imap4Driver::getOfflineJobManager() const
{
	return pOfflineJobManager_.get();
}

bool qmimap4::Imap4Driver::search(SubAccount* pSubAccount,
								  NormalFolder* pFolder,
								  const WCHAR* pwszCondition,
								  const WCHAR* pwszCharset,
								  bool bUseCharset,
								  MessageHolderList* pList)
{
	assert(pSubAccount);
	assert(pFolder);
	assert(pwszCondition);
	assert(pList);
	
	if (!bOffline_) {
		Lock<CriticalSection> lock(cs_);
		
		if (!prepareSessionCache(pSubAccount))
			return false;
		
		SessionCacher cacher(pSessionCache_.get(), pFolder);
		Imap4* pImap4 = cacher.get();
		if (!pImap4)
			return false;
		
		struct SearchProcessHook : public ProcessHook
		{
			SearchProcessHook(NormalFolder* pFolder,
							  MessageHolderList* pList) :
				pFolder_(pFolder),
				pList_(pList)
			{
			}
			
			virtual bool processSearchResponse(ResponseSearch* pSearch)
			{
				assert(pFolder_->getAccount()->isLocked());
				
				if (!pFolder_->loadMessageHolders())
					return false;
				
				const ResponseSearch::ResultList& l = pSearch->getResult();
				for (ResponseSearch::ResultList::const_iterator it = l.begin(); it != l.end(); ++it) {
					MessageHolder* pmh = pFolder_->getMessageHolderById(*it);
					if (pmh)
						pList_->push_back(pmh);
				}
				
				return true;
			}
			
			NormalFolder* pFolder_;
			MessageHolderList* pList_;
		} hook(pFolder, pList);
		
		Hook h(pCallback_.get(), &hook);
		if (!pImap4->search(pwszCondition, pwszCharset, bUseCharset, true))
			return false;
		
		cacher.release();
	}
	
	return true;
}

bool qmimap4::Imap4Driver::prepareSessionCache(SubAccount* pSubAccount)
{
	if (!pSessionCache_.get() || pSessionCache_->getSubAccount() != pSubAccount) {
		pSessionCache_.reset(0);
		pCallback_.reset(new CallbackImpl(pSubAccount, pSecurity_));
		
		int nMaxSession = pAccount_->getProperty(L"Imap4", L"MaxSession", 5);
		pSessionCache_.reset(new SessionCache(pAccount_,
			pSubAccount, pCallback_.get(), nMaxSession));
	}
	
	return true;
}

bool qmimap4::Imap4Driver::setFlags(Imap4* pImap4,
									const Range& range,
									NormalFolder* pFolder,
									const MessageHolderList& l,
									unsigned int nFlags,
									unsigned int nMask)
{
	Flags flags(Util::getImap4FlagsFromMessageFlags(nFlags));
	Flags mask(Util::getImap4FlagsFromMessageFlags(nMask));
	FlagProcessHook hook(pFolder);
	Hook h(pCallback_.get(), &hook);
	if (!pImap4->setFlags(range, flags, mask))
		return false;
	
	// Some server doesn't contain UID in a response to this STORE command
	// If so, FlagProcessHook cannot set new flags to MessageHolder.
	// So I'll update flags here to ensure flags updated.
	for (MessageHolderList::const_iterator it = l.begin(); it != l.end(); ++it)
		(*it)->setFlags(nFlags, nMask);
	
	return true;
}


/****************************************************************************
 *
 * Imap4Driver::ProcessHook
 *
 */

qmimap4::Imap4Driver::ProcessHook::~ProcessHook()
{
}

bool qmimap4::Imap4Driver::ProcessHook::processFetchResponse(ResponseFetch* pFetch)
{
	return true;
}

bool qmimap4::Imap4Driver::ProcessHook::processListResponse(ResponseList* pList)
{
	return true;
}

bool qmimap4::Imap4Driver::ProcessHook::processExpungeResponse(ResponseExpunge* pExpunge)
{
	return true;
}

bool qmimap4::Imap4Driver::ProcessHook::processSearchResponse(ResponseSearch* pSearch)
{
	return true;
}


/****************************************************************************
 *
 * Imap4Driver::FlagProcessHook
 *
 */

qmimap4::Imap4Driver::FlagProcessHook::FlagProcessHook(NormalFolder* pFolder) :
	pFolder_(pFolder)
{
}

qmimap4::Imap4Driver::FlagProcessHook::~FlagProcessHook()
{
}

bool qmimap4::Imap4Driver::FlagProcessHook::processFetchResponse(ResponseFetch* pFetch)
{
	unsigned int nUid = 0;
	unsigned int nFlags = 0;
	
	int nCount = 0;
	
	const ResponseFetch::FetchDataList& l = pFetch->getFetchDataList();
	for (ResponseFetch::FetchDataList::const_iterator it = l.begin(); it != l.end(); ++it) {
		switch ((*it)->getType()) {
		case FetchData::TYPE_FLAGS:
			nFlags = Util::getMessageFlagsFromImap4Flags(
				static_cast<FetchDataFlags*>(*it)->getSystemFlags(),
				static_cast<FetchDataFlags*>(*it)->getCustomFlags());
			++nCount;
			break;
		case FetchData::TYPE_UID:
			nUid = static_cast<FetchDataUid*>(*it)->getUid();
			++nCount;
			break;
		}
	}
	
	if (nCount == 2) {
		MessagePtr ptr(pFolder_->getMessageById(nUid));
		MessagePtrLock mpl(ptr);
		if (mpl)
			mpl->setFlags(nFlags, nFlags);
	}
	
	return true;
}


/****************************************************************************
 *
 * Imap4Driver::CallbackImpl
 *
 */

qmimap4::Imap4Driver::CallbackImpl::CallbackImpl(SubAccount* pSubAccount,
												 const Security* pSecurity) :
	AbstractCallback(pSubAccount, pSecurity),
	pProcessHook_(0)
{
}

qmimap4::Imap4Driver::CallbackImpl::~CallbackImpl()
{
}

void qmimap4::Imap4Driver::CallbackImpl::setProcessHook(ProcessHook* pProcessHook)
{
	pProcessHook_ = pProcessHook;
}

bool qmimap4::Imap4Driver::CallbackImpl::isCanceled(bool bForce) const
{
	return false;
}

void qmimap4::Imap4Driver::CallbackImpl::initialize()
{
}

void qmimap4::Imap4Driver::CallbackImpl::lookup()
{
}

void qmimap4::Imap4Driver::CallbackImpl::connecting()
{
}

void qmimap4::Imap4Driver::CallbackImpl::connected()
{
}

void qmimap4::Imap4Driver::CallbackImpl::authenticating()
{
}

void qmimap4::Imap4Driver::CallbackImpl::setRange(unsigned int nMin,
												  unsigned int nMax)
{
}

void qmimap4::Imap4Driver::CallbackImpl::setPos(unsigned int nPos)
{
}

bool qmimap4::Imap4Driver::CallbackImpl::response(Response* pResponse)
{
#define BEGIN_PROCESS_RESPONSE() \
	switch (pResponse->getType()) { \

#define END_PROCESS_RESPONSE() \
	} \

#define PROCESS_RESPONSE(type, name) \
	case Response::TYPE_##type: \
		if (pProcessHook_) { \
			if (!pProcessHook_->process##name##Response( \
				static_cast<Response##name*>(pResponse))) \
				return false; \
		} \
		break; \
	
	BEGIN_PROCESS_RESPONSE()
		PROCESS_RESPONSE(EXPUNGE, Expunge)
		PROCESS_RESPONSE(FETCH, Fetch)
		PROCESS_RESPONSE(LIST, List)
		PROCESS_RESPONSE(SEARCH, Search)
	END_PROCESS_RESPONSE()
	
	return true;
}


/****************************************************************************
 *
 * Imap4Driver::Hook
 *
 */

qmimap4::Imap4Driver::Hook::Hook(CallbackImpl* pCallback,
	ProcessHook* pProcessHook) :
	pCallback_(pCallback)
{
	pCallback_->setProcessHook(pProcessHook);
}

qmimap4::Imap4Driver::Hook::~Hook()
{
	pCallback_->setProcessHook(0);
}


/****************************************************************************
 *
 * Imap4Factory
 *
 */

Imap4Factory qmimap4::Imap4Factory::factory__;

qmimap4::Imap4Factory::Imap4Factory()
{
	registerFactory(L"imap4", this);
}

qmimap4::Imap4Factory::~Imap4Factory()
{
	unregisterFactory(L"imap4");
}

std::auto_ptr<ProtocolDriver> qmimap4::Imap4Factory::createDriver(Account* pAccount,
																  const qm::Security* pSecurity)
{
	assert(pAccount);
	assert(pSecurity);
	
	return new Imap4Driver(pAccount, pSecurity);
}


/****************************************************************************
 *
 * FolderUtil
 *
 */

qmimap4::FolderUtil::FolderUtil(Account* pAccount)
{
	wstrRootFolder_ = pAccount->getProperty(L"Imap4", L"RootFolder", L"");
	
	struct {
		const WCHAR* pwszKey_;
		const WCHAR* pwszDefault_;
	} folders[] = {
		{ L"OutboxFolder",		L"Outbox"	},
		{ L"DraftboxFolder",	L"Outbox"	},
		{ L"SentboxFolder",		L"Sentbox"	},
		{ L"TrashFolder",		L"Trash"	}
	};
	for (int n = 0; n < countof(folders); ++n)
		wstrSpecialFolders_[n] = pAccount->getProperty(L"Imap4",
			folders[n].pwszKey_, folders[n].pwszDefault_);
}

qmimap4::FolderUtil::~FolderUtil()
{
}

bool qmimap4::FolderUtil::isRootFolderSpecified() const
{
	return *wstrRootFolder_.get() != L'\0';
}

const WCHAR* qmimap4::FolderUtil::getRootFolder() const
{
	return wstrRootFolder_.get();
}

void qmimap4::FolderUtil::getFolderData(const WCHAR* pwszName,
										WCHAR cSeparator,
										unsigned int nAttributes,
										wstring_ptr* pwstrName,
										unsigned int* pnFlags) const
{
	assert(pwszName);
	assert(pwstrName);
	assert(pnFlags);
	
	pwstrName->reset(0);
	*pnFlags = 0;
	
	bool bChildOfRootFolder = false;
	size_t nRootFolderLen = wcslen(wstrRootFolder_.get());
	if (nRootFolderLen != 0) {
		bChildOfRootFolder = wcsncmp(pwszName, wstrRootFolder_.get(), nRootFolderLen) == 0 &&
			*(pwszName + nRootFolderLen) == cSeparator;
		if (bChildOfRootFolder)
			pwszName += nRootFolderLen + 1;
	}
	if (!*pwszName)
		return;
	
	wstring_ptr wstr(allocWString(pwszName));
	size_t nLen = wcslen(wstr.get());
	if (nLen != 1 && *(wstr.get() + nLen - 1) == cSeparator)
		*(wstr.get() + nLen - 1) = L'\0';
	
	unsigned int nFlags = Util::getFolderFlagsFromAttributes(nAttributes);
	if (!(nFlags & Folder::FLAG_NOSELECT))
		nFlags |= Folder::FLAG_SYNCABLE;
	if (bChildOfRootFolder)
		nFlags |= Folder::FLAG_CHILDOFROOT;
	
	if (_wcsnicmp(wstr.get(), L"Inbox", 5) == 0 &&
		(*(wstr.get() + 5) == L'\0' || *(wstr.get() + 5) == cSeparator))
		wcsncpy(wstr.get(), L"Inbox", 5);
	
	struct {
		const WCHAR* pwszName_;
		unsigned int nFlags_;
	} flags[] = {
		{ L"Inbox",						Folder::FLAG_INBOX | Folder::FLAG_NORENAME	},
		{ wstrSpecialFolders_[0].get(),	Folder::FLAG_OUTBOX							},
		{ wstrSpecialFolders_[1].get(),	Folder::FLAG_DRAFTBOX						},
		{ wstrSpecialFolders_[2].get(),	Folder::FLAG_SENTBOX						},
		{ wstrSpecialFolders_[3].get(),	Folder::FLAG_TRASHBOX						}
	};
	for (int n = 0; n < countof(flags); ++n) {
		if (Util::isEqualFolderName(wstr.get(), flags[n].pwszName_, cSeparator))
			nFlags |= flags[n].nFlags_;
	}
	
	*pwstrName = wstr;
	*pnFlags = nFlags;
}

void qmimap4::FolderUtil::saveSpecialFolders(Account* pAccount)
{
	struct {
		Folder::Flag flag_;
		const WCHAR* pwszKey_;
	} flags[] = {
		{ Folder::FLAG_OUTBOX,		L"OutboxFolder"		},
		{ Folder::FLAG_DRAFTBOX,	L"DraftboxFolder"	},
		{ Folder::FLAG_SENTBOX,		L"SentboxFolder"	},
		{ Folder::FLAG_TRASHBOX,	L"TrashFolder"		}
	};
	
	const Account::FolderList& l = pAccount->getFolders();
	for (Account::FolderList::const_iterator it = l.begin(); it != l.end(); ++it) {
		Folder* pFolder = *it;
		unsigned int nBoxFlags = pFolder->getFlags() & Folder::FLAG_BOX_MASK;
		for (int n = 0; n < countof(flags); ++n) {
			if (nBoxFlags & flags[n].flag_) {
				wstring_ptr wstrName(pFolder->getFullName());
				pAccount->setProperty(L"Imap4", flags[n].pwszKey_, wstrName.get());
			}
		}
	}
}


/****************************************************************************
 *
 * FolderListGetter
 *
 */

qmimap4::FolderListGetter::FolderListGetter(Account* pAccount,
											SubAccount* pSubAccount,
											const Security* pSecurity) :
	pAccount_(pAccount),
	pSubAccount_(pSubAccount),
	pSecurity_(pSecurity)
{
	pFolderUtil_.reset(new FolderUtil(pAccount_));
}

qmimap4::FolderListGetter::~FolderListGetter()
{
	if (pImap4_.get()) {
		pImap4_->disconnect();
		pImap4_.reset(0);
	}
	
	std::for_each(listNamespace_.begin(), listNamespace_.end(),
		unary_compose_f_gx(
			string_free<WSTRING>(),
			std::select1st<NamespaceList::value_type>()));
	std::for_each(listFolderData_.begin(), listFolderData_.end(),
		unary_compose_f_gx(
			string_free<WSTRING>(),
			mem_data_ref(&FolderData::wstrMailbox_)));
	std::for_each(listFolder_.begin(), listFolder_.end(),
		unary_compose_f_gx(
			deleter<Folder>(),
			std::select1st<FolderList::value_type>()));
}

bool qmimap4::FolderListGetter::update()
{
	return connect() &&
		listNamespaces() &&
		listFolders();
}

void qmimap4::FolderListGetter::getFolders(Imap4Driver::RemoteFolderList* pList)
{
	assert(pList);
	
	pList->resize(listFolder_.size());
	std::copy(listFolder_.begin(), listFolder_.end(), pList->begin());
	listFolder_.clear();
}

bool qmimap4::FolderListGetter::connect()
{
	pCallback_.reset(new CallbackImpl(this, pSecurity_));
	
	if (pSubAccount_->isLog(Account::HOST_RECEIVE))
		pLogger_ = pAccount_->openLogger(Account::HOST_RECEIVE);
	
	pImap4_.reset(new Imap4(pSubAccount_->getTimeout(), pCallback_.get(),
		pCallback_.get(), pCallback_.get(), pLogger_.get()));
	
	Imap4::Ssl ssl = Util::getSsl(pSubAccount_);
	return pImap4_->connect(pSubAccount_->getHost(Account::HOST_RECEIVE),
		pSubAccount_->getPort(Account::HOST_RECEIVE), ssl);
}

bool qmimap4::FolderListGetter::listNamespaces()
{
	int nUseNamespace = pSubAccount_->getProperty(L"Imap4", L"UseNamespace", 1);
	
	pCallback_->setNamespaceList(&listNamespace_);
	if (nUseNamespace && pImap4_->getCapability() & Imap4::CAPABILITY_NAMESPACE) {
		if (!pImap4_->namespaceList())
			return false;
	}
	else {
		if (!pImap4_->list(false, L"", L""))
			return false;
	}
	
	if (pFolderUtil_->isRootFolderSpecified() && !listNamespace_.empty()) {
		wstring_ptr wstrInbox(allocWString(L"Inbox"));
		listNamespace_.push_back(std::make_pair(
			wstrInbox.get(), listNamespace_.front().second));
		wstrInbox.release();
	}
	
	return true;
}

bool qmimap4::FolderListGetter::listFolders()
{
	pCallback_->setFolderDataList(&listFolderData_);
	
	for (NamespaceList::iterator itNS = listNamespace_.begin(); itNS != listNamespace_.end(); ++itNS) {
		if (!pImap4_->list(false, L"", (*itNS).first))
			return false;
		if (!pImap4_->list(false, (*itNS).first, L"*"))
			return false;
	}
	
	std::sort(listFolderData_.begin(), listFolderData_.end(), FolderDataLess());
	
	unsigned int nId = pAccount_->generateFolderId();
	for (FolderDataList::iterator itFD = listFolderData_.begin(); itFD != listFolderData_.end(); ++itFD) {
		const FolderData& data = *itFD;
		
		Folder* pFolder = getFolder(data.wstrMailbox_,
			data.cSeparator_, data.nFlags_, &nId);
	}
	
	return true;
}

Folder* qmimap4::FolderListGetter::getFolder(const WCHAR* pwszName,
											 WCHAR cSeparator,
											 unsigned int nFlags,
											 unsigned int* pnId)
{
	assert(pwszName);
	
	Folder* pFolder = 0;
	
	for (FolderList::const_iterator itF = listFolder_.begin(); itF != listFolder_.end(); ++itF) {
		wstring_ptr wstrName((*itF).first->getFullName());
		if (wcscmp(wstrName.get(), pwszName) == 0) {
			pFolder = (*itF).first;
			break;
		}
	}
	
	if (!pFolder) {
		Folder* pParent = 0;
		const WCHAR* pName = 0;
		if (cSeparator != '\0')
			pName = wcsrchr(pwszName, cSeparator);
		if (pName) {
			wstring_ptr wstrParentName(allocWString(pwszName, pName - pwszName));
			pParent = getFolder(wstrParentName.get(), cSeparator, Folder::FLAG_NOSELECT, pnId);
			++pName;
		}
		else {
			pName = pwszName;
		}
		
		bool bNew = false;
		pFolder = pAccount_->getFolder(pwszName);
		if (pFolder) {
			// TODO
			// What happen if this folder is local folder or
			// this folder is query folder
			pAccount_->setFolderFlags(pFolder, nFlags, ~Folder::FLAG_USER_MASK);
		}
		else {
			std::auto_ptr<NormalFolder> pNormalFolder(new NormalFolder(
				(*pnId)++, pName, cSeparator, nFlags, 0, 0, 0, 0, 0, pParent, pAccount_));
			pFolder = pNormalFolder.release();
			bNew = true;
		}
		listFolder_.push_back(std::make_pair(pFolder, bNew));
	}
	
	return pFolder;
}


/****************************************************************************
 *
 * FolderListGetter::FolderDataLess
 *
 */

bool qmimap4::FolderListGetter::FolderDataLess::operator()(
	const FolderData& lhs, const FolderData& rhs) const
{
	const WCHAR* pLhs = lhs.wstrMailbox_;
	const WCHAR* pRhs = rhs.wstrMailbox_;
	while (*pLhs && *pLhs == *pRhs) {
		++pLhs;
		++pRhs;
	}
	if (*pLhs == *pRhs)
		return false;
	else if (!*pLhs)
		return true;
	else if (!*pRhs)
		return false;
	else if (*pLhs == lhs.cSeparator_)
		return true;
	else if (*pRhs == rhs.cSeparator_)
		return false;
	else
		return *pLhs < *pRhs;
}


/****************************************************************************
 *
 * FolderListGetter::CallbackImpl
 *
 */

qmimap4::FolderListGetter::CallbackImpl::CallbackImpl(FolderListGetter* pGetter,
													  const Security* pSecurity) :
	AbstractCallback(pGetter->pSubAccount_, pSecurity),
	pGetter_(pGetter),
	pListNamespace_(0),
	pListFolderData_(0)
{
}

qmimap4::FolderListGetter::CallbackImpl::~CallbackImpl()
{
}

void qmimap4::FolderListGetter::CallbackImpl::setNamespaceList(NamespaceList* pListNamespace)
{
	pListNamespace_ = pListNamespace;
	pListFolderData_ = 0;
}

void qmimap4::FolderListGetter::CallbackImpl::setFolderDataList(FolderDataList* pListFolderData)
{
	pListFolderData_ = pListFolderData;
	pListNamespace_ = 0;
}

bool qmimap4::FolderListGetter::CallbackImpl::isCanceled(bool bForce) const
{
	// TODO
	return false;
}

void qmimap4::FolderListGetter::CallbackImpl::initialize()
{
	// TODO
}

void qmimap4::FolderListGetter::CallbackImpl::lookup()
{
	// TODO
}

void qmimap4::FolderListGetter::CallbackImpl::connecting()
{
	// TODO
}

void qmimap4::FolderListGetter::CallbackImpl::connected()
{
	// TODO
}

void qmimap4::FolderListGetter::CallbackImpl::authenticating()
{
	// TODO
}

void qmimap4::FolderListGetter::CallbackImpl::setRange(unsigned int nMin,
													   unsigned int nMax)
{
	// TODO
}

void qmimap4::FolderListGetter::CallbackImpl::setPos(unsigned int nPos)
{
	// TODO
}

bool qmimap4::FolderListGetter::CallbackImpl::response(Response* pResponse)
{
	switch (pResponse->getType()) {
	case Response::TYPE_NAMESPACE:
		return processNamespace(static_cast<ResponseNamespace*>(pResponse));
	case Response::TYPE_LIST:
		return processList(static_cast<ResponseList*>(pResponse));
	default:
		break;
	}
	return true;
}

bool qmimap4::FolderListGetter::CallbackImpl::processNamespace(ResponseNamespace* pNamespace)
{
	if (pListNamespace_) {
		const WCHAR* pwszRootFolder = pGetter_->pFolderUtil_->getRootFolder();
		typedef ResponseNamespace::NamespaceList NSList;
		const NSList* pLists[] = {
			&pNamespace->getPersonal(),
			&pNamespace->getOthers(),
			&pNamespace->getShared()
		};
		for (int n = 0; n < countof(pLists); ++n) {
			const NSList& l = (*pLists)[n];
			for (NSList::const_iterator it = l.begin(); it != l.end(); ++it) {
				wstring_ptr wstr;
				const WCHAR* pwsz = (*it).first;
				if (!*pwsz && *pwszRootFolder) {
					WCHAR wsz[] = { (*it).second, L'\0' };
					wstr = concat(pwszRootFolder, wsz);
				}
				else {
					wstr = allocWString(pwsz);
				}
				pListNamespace_->push_back(std::make_pair(wstr.get(), (*it).second));
				wstr.release();
			}
		}
	}
	
	return true;
}

bool qmimap4::FolderListGetter::CallbackImpl::processList(ResponseList* pList)
{
	if (pListNamespace_) {
		const WCHAR* pwszRootFolder = pGetter_->pFolderUtil_->getRootFolder();
		WCHAR wszSeparator[] = { pList->getSeparator(), L'\0' };
		wstring_ptr wstr;
		if (pwszRootFolder && *pwszRootFolder)
			wstr = concat(pwszRootFolder, wszSeparator, pList->getMailbox());
		else
			wstr = allocWString(pList->getMailbox());
		pListNamespace_->push_back(std::make_pair(wstr.get(), pList->getSeparator()));
		wstr.release();
	}
	else {
		wstring_ptr wstrName;
		unsigned int nFlags = 0;
		pGetter_->pFolderUtil_->getFolderData(pList->getMailbox(),
			pList->getSeparator(), pList->getAttributes(), &wstrName, &nFlags);
		if (wstrName.get()) {
			FolderData data = {
				wstrName.get(),
				pList->getSeparator(),
				nFlags
			};
			pListFolderData_->push_back(data);
			wstrName.release();
		}
	}
	
	return true;
}


/****************************************************************************
 *
 * SessionCache
 *
 */

qmimap4::SessionCache::SessionCache(Account* pAccount,
									SubAccount* pSubAccount,
									AbstractCallback* pCallback,
									size_t nMaxSession) :
	pAccount_(pAccount),
	pSubAccount_(pSubAccount),
	pCallback_(pCallback),
	nMaxSession_(nMaxSession),
	bReselect_(true),
	nForceDisconnect_(0)
{
	bReselect_ = pSubAccount->getProperty(L"Imap4", L"Reselect", 1) != 0;
	nForceDisconnect_ = pSubAccount->getProperty(L"Imap4", L"ForceDisconnect", 0);
	listSession_.reserve(nMaxSession);
}

qmimap4::SessionCache::~SessionCache()
{
	for (SessionList::iterator it = listSession_.begin(); it != listSession_.end(); ++it) {
		if ((*it).pImap4_->checkConnection())
			(*it).pImap4_->disconnect();
		delete (*it).pImap4_;
		delete (*it).pLogger_;
	}
}

SubAccount* qmimap4::SessionCache::getSubAccount() const
{
	return pSubAccount_;
}

bool qmimap4::SessionCache::getSession(NormalFolder* pFolder,
									   Session* pSession)
{
	assert(pSession);
	
	std::auto_ptr<Logger> pLogger;
	std::auto_ptr<Imap4> pImap4;
	unsigned int nLastUsedTime = 0;
	unsigned int nLastSelectedTime = 0;
	SessionList::iterator it = std::find_if(
		listSession_.begin(), listSession_.end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				std::equal_to<NormalFolder*>(),
				mem_data_ref(&Session::pFolder_),
				std::identity<NormalFolder*>()),
			pFolder));
	if (it != listSession_.end()) {
		pImap4.reset((*it).pImap4_);
		pLogger.reset((*it).pLogger_);
		nLastUsedTime = (*it).nLastUsedTime_;
		nLastSelectedTime = (*it).nLastSelectedTime_;
		listSession_.erase(it);
	}
	else {
		if (listSession_.size() >= nMaxSession_) {
			it = listSession_.begin();
			pImap4.reset((*it).pImap4_);
			pLogger.reset((*it).pLogger_);
			nLastUsedTime = (*it).nLastUsedTime_;
			listSession_.erase(it);
			
			if (!pFolder)
				pImap4.reset(0);
		}
	}
	
	if (pImap4.get()) {
		if ((nForceDisconnect_ != 0 && (*it).nLastUsedTime_ + nForceDisconnect_*1000 < ::GetTickCount()) ||
			!pImap4->checkConnection())
			pImap4.reset(0);
	}
	
	if (!pImap4.get()) {
		if (pSubAccount_->isLog(Account::HOST_RECEIVE))
			pLogger = pAccount_->openLogger(Account::HOST_RECEIVE);
		
		pImap4.reset(new Imap4(pSubAccount_->getTimeout(),
			pCallback_, pCallback_, pCallback_, pLogger.get()));
		Imap4::Ssl ssl = Util::getSsl(pSubAccount_);
		if (!pImap4->connect(pSubAccount_->getHost(Account::HOST_RECEIVE),
			pSubAccount_->getPort(Account::HOST_RECEIVE), ssl))
			return false;
		
		nLastSelectedTime = 0;
	}
	
	if (pFolder && isNeedSelect(pFolder, nLastSelectedTime)) {
		wstring_ptr wstrName(Util::getFolderName(pFolder));
		if (!pImap4->select(wstrName.get()))
			return false;
		nLastSelectedTime = ::GetTickCount();
	}
	
	pSession->pFolder_ = pFolder;
	pSession->pImap4_ = pImap4.release();
	pSession->pLogger_ = pLogger.release();
	pSession->nLastUsedTime_ = 0;
	pSession->nLastSelectedTime_ = nLastSelectedTime;
	
	return true;
}

void qmimap4::SessionCache::releaseSession(Session session)
{
	assert(listSession_.size() < nMaxSession_);
	session.nLastUsedTime_ = ::GetTickCount();
	listSession_.push_back(session);
}

bool qmimap4::SessionCache::isNeedSelect(NormalFolder* pFolder,
										 unsigned int nLastSelectedTime)
{
	if (bReselect_) {
		// TODO
		// Take care of GetTickCount is reset after 47.9 days.
		return nLastSelectedTime == 0 ||
			nLastSelectedTime < pFolder->getLastSyncTime();
	}
	else {
		return nLastSelectedTime == 0;
	}
}


/****************************************************************************
 *
 * SessionCacher
 *
 */

qmimap4::SessionCacher::SessionCacher(SessionCache* pCache,
									  NormalFolder* pFolder) :
	pCache_(pCache)
{
	session_.pFolder_ = 0;
	session_.pImap4_ = 0;
	session_.pLogger_ = 0;
	session_.nLastSelectedTime_ = 0;
	
	if (!pCache->getSession(pFolder, &session_)) {
		assert(!session_.pFolder_);
		assert(!session_.pImap4_);
		assert(!session_.pLogger_);
	}
}

qmimap4::SessionCacher::~SessionCacher()
{
	delete session_.pImap4_;
	delete session_.pLogger_;
}

Imap4* qmimap4::SessionCacher::get() const
{
	return session_.pImap4_;
}

void qmimap4::SessionCacher::release()
{
	if (session_.pImap4_) {
		pCache_->releaseSession(session_);
		session_.pFolder_ = 0;
		session_.pImap4_ = 0;
		session_.pLogger_ = 0;
	}
}
