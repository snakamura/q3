/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmaccount.h>
#include <qmfolder.h>
#include <qmmessage.h>
#include <qmmessageholder.h>

#include <qsassert.h>
#include <qserror.h>
#include <qsnew.h>
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

qmimap4::Imap4Driver::Imap4Driver(Account* pAccount, QSTATUS* pstatus) :
	pAccount_(pAccount),
	pSessionCache_(0),
	pCallback_(0),
	pOfflineJobManager_(0),
	bOffline_(true),
	nForceOnline_(0)
{
	assert(pstatus);
	
	DECLARE_QSTATUS();
	
	*pstatus = QSTATUS_SUCCESS;
	
	status = newQsObject(pAccount_->getPath(), &pOfflineJobManager_);
	CHECK_QSTATUS_SET(pstatus);
}

qmimap4::Imap4Driver::~Imap4Driver()
{
	delete pOfflineJobManager_;
	delete pSessionCache_;
	delete pCallback_;
}

bool qmimap4::Imap4Driver::isSupport(Account::Support support)
{
	switch (support) {
	case Account::SUPPORT_REMOTEFOLDER:
		return true;
	case Account::SUPPORT_LOCALFOLDERDOWNLOAD:
		return false;
	default:
		assert(false);
		return false;
	}
}

QSTATUS qmimap4::Imap4Driver::setOffline(bool bOffline)
{
	Lock<CriticalSection> lock(cs_);
	
	if (!bOffline_ && bOffline && nForceOnline_ == 0) {
		delete pSessionCache_;
		pSessionCache_ = 0;
	}
	bOffline_ = bOffline;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Imap4Driver::setForceOnline(bool bOnline)
{
	Lock<CriticalSection> lock(cs_);
	
	if (bOnline) {
		++nForceOnline_;
	}
	else {
		if (--nForceOnline_ == 0 && bOffline_) {
			delete pSessionCache_;
			pSessionCache_ = 0;
		}
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Imap4Driver::save()
{
	Lock<CriticalSection> lock(cs_);
	
	return pOfflineJobManager_->save(pAccount_->getPath());
}

QSTATUS qmimap4::Imap4Driver::createFolder(SubAccount* pSubAccount,
	const WCHAR* pwszName, Folder* pParent, NormalFolder** ppFolder)
{
	assert(pSubAccount);
	assert(pwszName);
	assert(ppFolder);
	
	DECLARE_QSTATUS();
	
	*ppFolder = 0;
	
	Lock<CriticalSection> lock(cs_);
	
	string_ptr<WSTRING> wstrRootFolder;
	status = pAccount_->getProperty(
		L"Imap4", L"RootFolder", 0, &wstrRootFolder);
	CHECK_QSTATUS();
	
	status = prepareSessionCache(pSubAccount);
	CHECK_QSTATUS();
	
	Imap4* pImap4 = 0;
	SessionCacher cacher(pSessionCache_, 0, &pImap4, &status);
	CHECK_QSTATUS();
	
	WCHAR cSeparator = L'/';
	string_ptr<WSTRING> wstrFullName;
	if (pParent) {
		string_ptr<WSTRING> wstrParentName;
		status = pParent->getFullName(&wstrParentName);
		CHECK_QSTATUS();
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
		wstrFullName.reset(concat(c + (bChildOfRoot ? 0 : 2),
			countof(c) - (bChildOfRoot ? 0 : 2)));
	}
	else {
		ConcatW c[] = {
			{ wstrRootFolder.get(),	-1	},
			{ L"/",					1	},
			{ pwszName,				-1	}
		};
		bool bChildOfRoot = *wstrRootFolder.get() != L'\0';
		wstrFullName.reset(concat(c + (bChildOfRoot ? 0 : 2),
			countof(c) - (bChildOfRoot ? 0 : 2)));
	}
	if (!wstrFullName.get())
		return QSTATUS_OUTOFMEMORY;
	
	status = pImap4->create(wstrFullName.get());
	CHECK_QSTATUS();
	
	string_ptr<WSTRING> wstrName;
	size_t nFullNameLen = wcslen(wstrFullName.get());
	if (wstrFullName[nFullNameLen - 1] == cSeparator) {
		wstrFullName[nFullNameLen - 1] = L'\0';
		
		wstrName.reset(allocWString(pwszName, wcslen(pwszName) - 1));
		if (!wstrName.get())
			return QSTATUS_OUTOFMEMORY;
		pwszName = wstrName.get();
	}
	
	FolderUtil folderUtil(pAccount_, &status);
	CHECK_QSTATUS();
	
	struct ListProcessHook : public ProcessHook
	{
		ListProcessHook(const WCHAR* pwszName, const FolderUtil& folderUtil) :
			pwszName_(pwszName),
			folderUtil_(folderUtil),
			nFlags_(0),
			cSeparator_(L'\0'),
			bFound_(false)
		{
		};
		
		virtual QSTATUS processListResponse(ResponseList* pList)
		{
			DECLARE_QSTATUS();
			
//			if (wcscmp(pList->getMailbox(), pwszName_) == 0 ||
//				(_wcsnicmp(pList->getMailbox(), L"Inbox", 5) == 0 &&
//				_wcsnicmp(pwszName_, L"Inbox", 5) == 0 &&
//				wcscmp(pList->getMailbox() + 5, pwszName_ + 5) == 0)) {
			if (Util::isEqualFolderName(pList->getMailbox(), pwszName_, pList->getSeparator())) {
				bFound_ = true;
				string_ptr<WSTRING> wstrName;
				status = folderUtil_.getFolderData(pList->getMailbox(),
					pList->getSeparator(), pList->getAttributes(), &wstrName, &nFlags_);
				cSeparator_ = pList->getSeparator();
			}
			
			return QSTATUS_SUCCESS;
		}
		
		const WCHAR* pwszName_;
		const FolderUtil& folderUtil_;
		unsigned int nFlags_;
		WCHAR cSeparator_;
		bool bFound_;
	} hook(wstrFullName.get(), folderUtil);
	
	Hook h(pCallback_, &hook);
	status = pImap4->list(false, L"", wstrFullName.get());
	CHECK_QSTATUS();
	if (!hook.bFound_)
		return QSTATUS_FAIL;
	
	cacher.release();
	
	NormalFolder::Init init;
	init.nId_ = pAccount_->generateFolderId();
	init.pwszName_ = pwszName;
	init.cSeparator_ = hook.cSeparator_;
	init.nFlags_ = hook.nFlags_;
	init.nCount_ = 0;
	init.nUnseenCount_ = 0;
	init.pParentFolder_ = pParent;
	init.pAccount_ = pAccount_;
	status = newQsObject(init, ppFolder);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Imap4Driver::createDefaultFolders(
	Folder*** pppFolder, size_t* pnCount)
{
	assert(pppFolder);
	assert(pnCount);
	
	*pppFolder = 0;
	*pnCount = 0;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Imap4Driver::getRemoteFolders(SubAccount* pSubAccount,
	std::pair<Folder*, bool>** ppFolder, size_t* pnCount)
{
	assert(ppFolder);
	assert(pnCount);
	
	DECLARE_QSTATUS();
	
	*ppFolder = 0;
	*pnCount = 0;
	
	Lock<CriticalSection> lock(cs_);
	
	FolderListGetter getter(pAccount_, pSubAccount, &status);
	CHECK_QSTATUS();
	status = getter.getFolders(ppFolder, pnCount);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Imap4Driver::getMessage(SubAccount* pSubAccount,
	MessageHolder* pmh, unsigned int nFlags, Message* pMessage,
	bool* pbGet, bool* pbMadeSeen)
{
	assert(pSubAccount);
	assert(pmh);
	assert(pMessage);
	assert(pbGet);
	assert(pbMadeSeen);
	assert(!pmh->getFolder()->isFlag(Folder::FLAG_LOCAL));
	assert(!pmh->isFlag(MessageHolder::FLAG_LOCAL));
	
	DECLARE_QSTATUS();
	
	*pbGet = false;
	*pbMadeSeen = false;
	
	if (bOffline_ &&
		(nForceOnline_ == 0 ||
		(nFlags & Account::GETMESSAGEFLAG_FORCEONLINE) == 0))
		return QSTATUS_SUCCESS;
	
	Lock<CriticalSection> lock(cs_);
	
	int nOption = 0;
	status = pSubAccount->getProperty(L"Imap4", L"Option", 0xff, &nOption);
	CHECK_QSTATUS();
	
	status = prepareSessionCache(pSubAccount);
	CHECK_QSTATUS();
	
	Imap4* pImap4 = 0;
	SessionCacher cacher(pSessionCache_, pmh->getFolder(), &pImap4, &status);
	CHECK_QSTATUS();
	
	struct BodyProcessHook : public ProcessHook
	{
		BodyProcessHook(unsigned int nUid, bool bHeaderOnly,
			MessageHolder* pmh, Message* pMessage) :
			nUid_(nUid),
			bHeaderOnly_(bHeaderOnly),
			pmh_(pmh),
			pMessage_(pMessage)
		{
		}
		
		virtual QSTATUS processFetchResponse(ResponseFetch* pFetch)
		{
			DECLARE_QSTATUS();
			
			unsigned int nUid = 0;
			FetchDataBody* pBody = 0;
			unsigned int nMask = MessageHolder::FLAG_SEEN |
				MessageHolder::FLAG_REPLIED | MessageHolder::FLAG_DRAFT |
				MessageHolder::FLAG_DELETED | MessageHolder::FLAG_MARKED;
			
			const ResponseFetch::FetchDataList& l =
				pFetch->getFetchDataList();
			ResponseFetch::FetchDataList::const_iterator it = l.begin();
			while (it != l.end()) {
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
				++it;
			}
			
			if (nUid == nUid_ && pBody) {
				FetchDataBody::Section s = pBody->getSection();
				if (((s == FetchDataBody::SECTION_NONE && !bHeaderOnly_) ||
					(s == FetchDataBody::SECTION_HEADER && bHeaderOnly_)) &&
					pBody->getPartPath().empty()) {
					status = pMessage_->create(
						pBody->getContent(), static_cast<size_t>(-1),
						bHeaderOnly_ ? Message::FLAG_HEADERONLY : Message::FLAG_NONE);
					CHECK_QSTATUS();
				}
			}
			
			return QSTATUS_SUCCESS;
		}
		
		unsigned int nUid_;
		bool bHeaderOnly_;
		MessageHolder* pmh_;
		Message* pMessage_;
	};
	
	struct BodyStructureProcessHook : public ProcessHook
	{
		BodyStructureProcessHook(unsigned int nUid) :
			nUid_(nUid),
			pBodyStructure_(0)
		{
		}
		
		~BodyStructureProcessHook()
		{
			delete pBodyStructure_;
		}
		
		FetchDataBodyStructure* getBodyStructure() const
		{
			return pBodyStructure_;
		}
		
		virtual QSTATUS processFetchResponse(ResponseFetch* pFetch)
		{
			DECLARE_QSTATUS();
			
			unsigned int nUid = 0;
			FetchDataBodyStructure* pBodyStructure = 0;
			
			const ResponseFetch::FetchDataList& l =
				pFetch->getFetchDataList();
			ResponseFetch::FetchDataList::const_iterator it = l.begin();
			while (it != l.end()) {
				switch ((*it)->getType()) {
				case FetchData::TYPE_BODYSTRUCTURE:
					pBodyStructure = static_cast<FetchDataBodyStructure*>(*it);
					break;
				case FetchData::TYPE_UID:
					nUid = static_cast<FetchDataUid*>(*it)->getUid();
					break;
				}
				++it;
			}
			
			if (nUid == nUid_ && pBodyStructure) {
				pBodyStructure_ = pBodyStructure;
				pFetch->detach(pBodyStructure);
			}
			
			return QSTATUS_SUCCESS;
		}
		
		unsigned int nUid_;
		FetchDataBodyStructure* pBodyStructure_;
	};
	
	struct BodyListProcessHook : public ProcessHook
	{
		typedef Util::BodyList BodyList;
		typedef Util::PartList PartList;
		
		BodyListProcessHook(unsigned int nUid,
			FetchDataBodyStructure* pBodyStructure, const PartList& listPart,
			unsigned int nPartCount, MessageHolder* pmh, Message* pMessage) :
			nUid_(nUid),
			pBodyStructure_(pBodyStructure),
			listPart_(listPart),
			nPartCount_(nPartCount),
			pmh_(pmh),
			pMessage_(pMessage)
		{
		}
		
		virtual QSTATUS processFetchResponse(ResponseFetch* pFetch)
		{
			DECLARE_QSTATUS();
			
			unsigned int nUid = 0;
			BodyList listBody;
			
			unsigned int nMask = MessageHolder::FLAG_SEEN |
				MessageHolder::FLAG_REPLIED | MessageHolder::FLAG_DRAFT |
				MessageHolder::FLAG_DELETED | MessageHolder::FLAG_MARKED;
			
			const ResponseFetch::FetchDataList& l =
				pFetch->getFetchDataList();
			ResponseFetch::FetchDataList::const_iterator it = l.begin();
			while (it != l.end()) {
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
						if (bAdd) {
							status = STLWrapper<BodyList>(listBody).push_back(pBody);
							CHECK_QSTATUS();
						}
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
				++it;
			}
			
			if (listBody.size() == nPartCount_ && nUid == nUid_) {
				string_ptr<STRING> strContent;
				status = Util::getContentFromBodyStructureAndBodies(
					listPart_, listBody, &strContent);
				CHECK_QSTATUS();
				status = pMessage_->create(strContent.get(),
					static_cast<size_t>(-1), Message::FLAG_TEXTONLY);
				CHECK_QSTATUS();
			}
			
			return QSTATUS_SUCCESS;
		}
		
		unsigned int nUid_;
		FetchDataBodyStructure* pBodyStructure_;
		const PartList& listPart_;
		unsigned int nPartCount_;
		MessageHolder* pmh_;
		Message* pMessage_;
	};
	
	SingleRange range(pmh->getId(), true, &status);
	CHECK_QSTATUS();
	
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
		return QSTATUS_FAIL;
	}
	
	if (bBodyStructure) {
		BodyStructureProcessHook hook(pmh->getId());
		Hook h(pCallback_, &hook);
		status = pImap4->getBodyStructure(range);
		CHECK_QSTATUS();
		FetchDataBodyStructure* pBodyStructure = hook.getBodyStructure();
		if (pBodyStructure) {
			Util::PartList listPart;
			Util::PartListDeleter deleter(listPart);
			unsigned int nPath = 0;
			status = Util::getPartsFromBodyStructure(
				pBodyStructure, &nPath, &listPart);
			CHECK_QSTATUS();
			
			if (!listPart.empty()) {
				string_ptr<STRING> strArg;
				unsigned int nPartCount = 0;
				bool bAll = false;
				status = Util::getFetchArgFromPartList(listPart,
					bHtml ? Util::FETCHARG_HTML : Util::FETCHARG_TEXT,
					false, (nOption & OPTION_TRUSTBODYSTRUCTURE) == 0,
					&strArg, &nPartCount, &bAll);
				CHECK_QSTATUS();
				
				BodyListProcessHook hook(pmh->getId(), pBodyStructure,
					listPart, nPartCount, pmh, pMessage);
				Hook h(pCallback_, &hook);
				
				status = pImap4->fetch(range, strArg.get());
				CHECK_QSTATUS();
			}
		}
	}
	else {
		if (bHeaderOnly) {
			BodyProcessHook hook(pmh->getId(), true, pmh, pMessage);
			Hook h(pCallback_, &hook);
			status = pImap4->getHeader(range,
				(nFlags & Account::GETMESSAGEFLAG_MAKESEEN) == 0);
			CHECK_QSTATUS();
		}
		else {
			BodyProcessHook hook(pmh->getId(), false, pmh, pMessage);
			Hook h(pCallback_, &hook);
			status = pImap4->getMessage(range,
				(nFlags & Account::GETMESSAGEFLAG_MAKESEEN) == 0);
			CHECK_QSTATUS();
		}
	}
	
	cacher.release();
	
	*pbGet = pMessage->getFlag() != Message::FLAG_EMPTY;
	*pbMadeSeen = *pbGet;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Imap4Driver::setMessagesFlags(SubAccount* pSubAccount,
	NormalFolder* pFolder, const Folder::MessageHolderList& l,
	unsigned int nFlags, unsigned int nMask)
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
	
	DECLARE_QSTATUS();
	
	Folder::MessageHolderList listUpdate;
	status = STLWrapper<Folder::MessageHolderList>(
		listUpdate).reserve(l.size());
	CHECK_QSTATUS();
	Folder::MessageHolderList::const_iterator it = l.begin();
	while (it != l.end()) {
		MessageHolder* pmh = *it;
		if ((pmh->getFlags() & nMask) != nFlags) {
			if (pmh->isFlag(MessageHolder::FLAG_LOCAL))
				pmh->setFlags(nFlags, nMask);
			else
				listUpdate.push_back(pmh);
		}
		++it;
	}
	if (listUpdate.empty())
		return QSTATUS_SUCCESS;
	
	if (bOffline_) {
		Util::UidList listUid;
		status = Util::createUidList(listUpdate, &listUid);
		CHECK_QSTATUS();
		
		if (!listUid.empty()) {
			string_ptr<WSTRING> wstrFolder;
			status = pFolder->getFullName(&wstrFolder);
			CHECK_QSTATUS();
			std::auto_ptr<SetFlagsOfflineJob> pJob;
			status = newQsObject(wstrFolder.get(), listUid, nFlags, nMask, &pJob);
			CHECK_QSTATUS();
			status = pOfflineJobManager_->add(pJob.get());
			CHECK_QSTATUS();
			pJob.release();
		}
		
		Folder::MessageHolderList::iterator it = listUpdate.begin();
		while (it != listUpdate.end())
			(*it++)->setFlags(nFlags, nMask);
	}
	else {
		std::auto_ptr<MultipleRange> pRange;
		status = Util::createRange(listUpdate, &pRange);
		CHECK_QSTATUS();
		
		Lock<CriticalSection> lock(cs_);
		
		status = prepareSessionCache(pSubAccount);
		CHECK_QSTATUS();
		
		Imap4* pImap4 = 0;
		SessionCacher cacher(pSessionCache_, pFolder, &pImap4, &status);
		CHECK_QSTATUS();
		
		status = setFlags(pImap4, *pRange, pFolder, listUpdate, nFlags, nMask);
		CHECK_QSTATUS();
		
		cacher.release();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Imap4Driver::appendMessage(SubAccount* pSubAccount,
	NormalFolder* pFolder, const CHAR* pszMessage, unsigned int nFlags)
{
	assert(pSubAccount);
	assert(pFolder);
	assert(pszMessage);
	
	DECLARE_QSTATUS();
	
	Lock<Folder> lock(*pFolder);
	
	if (bOffline_) {
		MessageHolder* pmh = 0;
		status = pAccount_->storeMessage(pFolder, pszMessage, 0,
			-1, nFlags | MessageHolder::FLAG_LOCAL, -1, false, &pmh);
		CHECK_QSTATUS();
		
		string_ptr<WSTRING> wstrFolder;
		status = pFolder->getFullName(&wstrFolder);
		CHECK_QSTATUS();
		std::auto_ptr<AppendOfflineJob> pJob;
		status = newQsObject(wstrFolder.get(), pmh->getId(), &pJob);
		CHECK_QSTATUS();
		status = pOfflineJobManager_->add(pJob.get());
		CHECK_QSTATUS();
		pJob.release();
	}
	else {
		Lock<CriticalSection> lock(cs_);
		
		status = prepareSessionCache(pSubAccount);
		CHECK_QSTATUS();
		
		Imap4* pImap4 = 0;
		SessionCacher cacher(pSessionCache_, pFolder, &pImap4, &status);
		CHECK_QSTATUS();
		
		string_ptr<WSTRING> wstrFolderName;
		status = Util::getFolderName(pFolder, &wstrFolderName);
		CHECK_QSTATUS();
		
		Flags flags(Util::getImap4FlagsFromMessageFlags(nFlags), &status);
		CHECK_QSTATUS();
		status = pImap4->append(wstrFolderName.get(), pszMessage, flags);
		CHECK_QSTATUS();
		
		cacher.release();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Imap4Driver::removeMessages(SubAccount* pSubAccount,
	NormalFolder* pFolder, const Folder::MessageHolderList& l)
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

QSTATUS qmimap4::Imap4Driver::copyMessages(SubAccount* pSubAccount,
	const Folder::MessageHolderList& l, NormalFolder* pFolderFrom,
	NormalFolder* pFolderTo, bool bMove)
{
	assert(pSubAccount);
	assert(!l.empty());
	assert(pFolderFrom);
	assert(pFolderTo);
	assert(std::find_if(l.begin(), l.end(),
		std::not1(
			std::bind2nd(
				binary_compose_f_gx_hy(
					std::equal_to<Folder*>(),
					std::mem_fun(&MessageHolder::getFolder),
					std::identity<Folder*>()),
				pFolderFrom))) == l.end());
	
	DECLARE_QSTATUS();
	
	Folder::MessageHolderList listUpdate;
	status = STLWrapper<Folder::MessageHolderList>(
		listUpdate).reserve(l.size());
	CHECK_QSTATUS();
	Util::UidList listLocalUid;
	status = STLWrapper<Util::UidList>(listLocalUid).reserve(l.size());
	CHECK_QSTATUS();
	Folder::MessageHolderList::const_iterator it = l.begin();
	while (it != l.end()) {
		MessageHolder* pmh = *it;
		if (pmh->isFlag(MessageHolder::FLAG_LOCAL))
			listLocalUid.push_back(pmh->getId());
		else
			listUpdate.push_back(pmh);
		++it;
	}
	
	status = pOfflineJobManager_->copyJobs(
		pFolderFrom, pFolderTo, listLocalUid, bMove);
	CHECK_QSTATUS();
	
	if (listUpdate.empty())
		return QSTATUS_SUCCESS;
	
	if (bOffline_) {
		CopyOfflineJob::ItemList listItemTo;
		status = STLWrapper<CopyOfflineJob::ItemList>(
			listItemTo).reserve(listUpdate.size());
		CHECK_QSTATUS();
		
		Lock<Folder> lock(*pFolderTo);
		
		Folder::MessageHolderList::iterator it = listUpdate.begin();
		while (it != listUpdate.end()) {
			MessageHolder* pmh = *it;
			MessageHolder* pmhClone = 0;
			status = pAccount_->cloneMessage(pmh, pFolderTo, &pmhClone);
			CHECK_QSTATUS();
			pmhClone->setFlags(MessageHolder::FLAG_LOCAL,
				MessageHolder::FLAG_LOCAL);
			if (bMove)
				pmh->setFlags(MessageHolder::FLAG_DELETED,
					MessageHolder::FLAG_DELETED);
			CopyOfflineJob::Item item = { pmhClone->getId(), pmhClone->getFlags() };
			listItemTo.push_back(item);
			++it;
		}
		
		Util::UidList listUidFrom;
		status = Util::createUidList(listUpdate, &listUidFrom);
		CHECK_QSTATUS();
		
		assert(listUidFrom.size() == listItemTo.size());
		
		if (!listUidFrom.empty()) {
			string_ptr<WSTRING> wstrFolderFrom;
			status = pFolderFrom->getFullName(&wstrFolderFrom);
			CHECK_QSTATUS();
			string_ptr<WSTRING> wstrFolderTo;
			status = pFolderTo->getFullName(&wstrFolderTo);
			CHECK_QSTATUS();
			std::auto_ptr<CopyOfflineJob> pJob;
			status = newQsObject(wstrFolderFrom.get(),
				wstrFolderTo.get(), listUidFrom, listItemTo, bMove, &pJob);
			CHECK_QSTATUS();
			status = pOfflineJobManager_->add(pJob.get());
			CHECK_QSTATUS();
			pJob.release();
		}
	}
	else {
		std::auto_ptr<MultipleRange> pRange;
		status = Util::createRange(listUpdate, &pRange);
		CHECK_QSTATUS();
		
		Lock<CriticalSection> lock(cs_);
		
		status = prepareSessionCache(pSubAccount);
		CHECK_QSTATUS();
		
		Imap4* pImap4 = 0;
		SessionCacher cacher(pSessionCache_, pFolderFrom, &pImap4, &status);
		CHECK_QSTATUS();
		
		string_ptr<WSTRING> wstrFolderName;
		status = Util::getFolderName(pFolderTo, &wstrFolderName);
		CHECK_QSTATUS();
		
		status = pImap4->copy(*pRange, wstrFolderName.get());
		CHECK_QSTATUS();
		
		cacher.release();
		
		if (bMove) {
			status = setFlags(pImap4, *pRange, pFolderFrom, listUpdate,
				MessageHolder::FLAG_DELETED, MessageHolder::FLAG_DELETED);
			CHECK_QSTATUS();
		}
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Imap4Driver::clearDeletedMessages(
	SubAccount* pSubAccount, NormalFolder* pFolder)
{
	assert(pSubAccount);
	assert(pFolder);
	
	DECLARE_QSTATUS();
	
	if (bOffline_) {
		// TODO
	}
	else {
		Lock<CriticalSection> lock(cs_);
		
		status = prepareSessionCache(pSubAccount);
		CHECK_QSTATUS();
		
		Imap4* pImap4 = 0;
		SessionCacher cacher(pSessionCache_, pFolder, &pImap4, &status);
		CHECK_QSTATUS();
		
		struct ExpungeProcessHook : public ProcessHook
		{
			virtual QSTATUS processExpungeResponse(ResponseExpunge* pExpunge)
			{
				DECLARE_QSTATUS();
				
				unsigned int n = pExpunge->getExpunge() - 1;
				unsigned int nMessage = n;
				UidList::iterator it = listUid_.begin();
				while (it != listUid_.end()) {
					if ((*it).first <= n)
						++nMessage;
					++it;
				}
				status = STLWrapper<UidList>(listUid_).push_back(
					UidList::value_type(n, nMessage));
				CHECK_QSTATUS();
				
				return QSTATUS_SUCCESS;
			}
			
			typedef std::vector<std::pair<unsigned int, unsigned int> > UidList;
			UidList listUid_;
		} hook;
		
		Hook h(pCallback_, &hook);
		status = pImap4->expunge();
		CHECK_QSTATUS();
		
		cacher.release();
		
		Lock<Folder> lock2(*pFolder);
		
		Folder::MessageHolderList l;
		status = STLWrapper<Folder::MessageHolderList>(
			l).resize(hook.listUid_.size());
		CHECK_QSTATUS();
		ExpungeProcessHook::UidList::size_type n = 0;
		while (n < hook.listUid_.size()) {
			l[n] = pFolder->getMessage(hook.listUid_[n].second);
			++n;
		}
		status = pFolder->deleteMessages(l);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

OfflineJobManager* qmimap4::Imap4Driver::getOfflineJobManager() const
{
	return pOfflineJobManager_;
}

QSTATUS qmimap4::Imap4Driver::prepareSessionCache(SubAccount* pSubAccount)
{
	DECLARE_QSTATUS();
	
	if (!pSessionCache_ || pSessionCache_->getSubAccount() != pSubAccount) {
		delete pSessionCache_;
		pSessionCache_ = 0;
		
		delete pCallback_;
		pCallback_ = 0;
		
		status = newQsObject(pSubAccount, &pCallback_);
		CHECK_QSTATUS();
		
		int nMaxSession = 0;
		status = pAccount_->getProperty(L"Imap4", L"MaxSession", 5, &nMaxSession);
		CHECK_QSTATUS();
		
		status = newQsObject(pAccount_, pSubAccount,
			pCallback_, nMaxSession, &pSessionCache_);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Imap4Driver::setFlags(Imap4* pImap4, const Range& range,
	NormalFolder* pFolder, const Folder::MessageHolderList& l,
	unsigned int nFlags, unsigned int nMask)
{
	DECLARE_QSTATUS();
	
	Flags flags(Util::getImap4FlagsFromMessageFlags(nFlags), &status);
	CHECK_QSTATUS();
	Flags mask(Util::getImap4FlagsFromMessageFlags(nMask), &status);
	CHECK_QSTATUS();
	FlagProcessHook hook(pFolder);
	Hook h(pCallback_, &hook);
	status = pImap4->setFlags(range, flags, mask);
	CHECK_QSTATUS();
	
	// Some server doesn't contain UID in a response to this STORE command
	// If so, FlagProcessHook cannot set new flags to MessageHolder.
	// So I'll update flags here to ensure flags updated.
	Folder::MessageHolderList::const_iterator it = l.begin();
	while (it != l.end())
		(*it++)->setFlags(nFlags, nMask);
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * Imap4Driver::ProcessHook
 *
 */

qmimap4::Imap4Driver::ProcessHook::~ProcessHook()
{
}

QSTATUS qmimap4::Imap4Driver::ProcessHook::processFetchResponse(
	ResponseFetch* pFetch)
{
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Imap4Driver::ProcessHook::processListResponse(
	ResponseList* pList)
{
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Imap4Driver::ProcessHook::processExpungeResponse(
	ResponseExpunge* pExpunge)
{
	return QSTATUS_SUCCESS;
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

QSTATUS qmimap4::Imap4Driver::FlagProcessHook::processFetchResponse(
	ResponseFetch* pFetch)
{
	DECLARE_QSTATUS();
	
	unsigned int nUid = 0;
	unsigned int nFlags = 0;
	
	int nCount = 0;
	
	const ResponseFetch::FetchDataList& l =
		pFetch->getFetchDataList();
	ResponseFetch::FetchDataList::const_iterator it = l.begin();
	while (it != l.end()) {
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
		++it;
	}
	
	if (nCount == 2) {
		MessageHolder* pmh = 0;
		status = pFolder_->getMessageById(nUid, &pmh);
		CHECK_QSTATUS();
		if (pmh)
			pmh->setFlags(nFlags, nFlags);
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * Imap4Driver::CallbackImpl
 *
 */

qmimap4::Imap4Driver::CallbackImpl::CallbackImpl(
	SubAccount* pSubAccount, QSTATUS* pstatus) :
	AbstractCallback(pSubAccount, pstatus),
	pProcessHook_(0)
{
}

qmimap4::Imap4Driver::CallbackImpl::~CallbackImpl()
{
}

void qmimap4::Imap4Driver::CallbackImpl::setProcessHook(
	ProcessHook* pProcessHook)
{
	pProcessHook_ = pProcessHook;
}

bool qmimap4::Imap4Driver::CallbackImpl::isCanceled(bool bForce) const
{
	return false;
}

QSTATUS qmimap4::Imap4Driver::CallbackImpl::initialize()
{
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Imap4Driver::CallbackImpl::lookup()
{
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Imap4Driver::CallbackImpl::connecting()
{
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Imap4Driver::CallbackImpl::connected()
{
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Imap4Driver::CallbackImpl::authenticating()
{
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Imap4Driver::CallbackImpl::setRange(
	unsigned int nMin, unsigned int nMax)
{
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Imap4Driver::CallbackImpl::setPos(unsigned int nPos)
{
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Imap4Driver::CallbackImpl::response(Response* pResponse)
{
	DECLARE_QSTATUS();
	
#define BEGIN_PROCESS_RESPONSE() \
	switch (pResponse->getType()) { \

#define END_PROCESS_RESPONSE() \
	} \

#define PROCESS_RESPONSE(type, name) \
	case Response::TYPE_##type: \
		if (pProcessHook_) { \
			status = pProcessHook_->process##name##Response( \
				static_cast<Response##name*>(pResponse)); \
			CHECK_QSTATUS(); \
		} \
		break; \
	
	BEGIN_PROCESS_RESPONSE()
		PROCESS_RESPONSE(EXPUNGE, Expunge)
		PROCESS_RESPONSE(FETCH, Fetch)
		PROCESS_RESPONSE(LIST, List)
	END_PROCESS_RESPONSE()
	
	return QSTATUS_SUCCESS;
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
	regist(L"imap4", this);
}

qmimap4::Imap4Factory::~Imap4Factory()
{
	unregist(L"imap4");
}

QSTATUS qmimap4::Imap4Factory::createDriver(
	Account* pAccount, ProtocolDriver** ppProtocolDriver)
{
	assert(pAccount);
	assert(ppProtocolDriver);
	
	DECLARE_QSTATUS();
	
	std::auto_ptr<Imap4Driver> pDriver;
	status = newQsObject(pAccount, &pDriver);
	CHECK_QSTATUS();
	
	*ppProtocolDriver = pDriver.release();
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * FolderUtil
 *
 */

qmimap4::FolderUtil::FolderUtil(Account* pAccount, QSTATUS* pstatus) :
	wstrRootFolder_(0)
{
	DECLARE_QSTATUS();
	
	for (int n = 0; n < countof(wstrSpecialFolders_); ++n)
		wstrSpecialFolders_[n] = 0;
	
	status = pAccount->getProperty(L"Imap4", L"RootFolder", 0, &wstrRootFolder_);
	CHECK_QSTATUS_SET(pstatus);
	
	struct {
		const WCHAR* pwszKey_;
		const WCHAR* pwszDefault_;
	} folders[] = {
		{ L"OutboxFolder",	L"Outbox"	},
		{ L"SentboxFolder",	L"Sentbox"	},
		{ L"TrashFolder",	L"Trash"	}
	};
	for (n = 0; n < countof(folders); ++n) {
		status = pAccount->getProperty(L"Imap4", folders[n].pwszKey_,
			folders[n].pwszDefault_, &wstrSpecialFolders_[n]);
		CHECK_QSTATUS_SET(pstatus);
	}
}

qmimap4::FolderUtil::~FolderUtil()
{
	freeWString(wstrRootFolder_);
	for (int n = 0; n < countof(wstrSpecialFolders_); ++n)
		freeWString(wstrSpecialFolders_[n]);
}

bool qmimap4::FolderUtil::isRootFolderSpecified() const
{
	return *wstrRootFolder_ != L'\0';
}

const WCHAR* qmimap4::FolderUtil::getRootFolder() const
{
	return wstrRootFolder_;
}

QSTATUS qmimap4::FolderUtil::getFolderData(const WCHAR* pwszName,
	WCHAR cSeparator, unsigned int nAttributes,
	WSTRING* pwstrName, unsigned int* pnFlags) const
{
	assert(pwszName);
	assert(pwstrName);
	assert(pnFlags);
	
	DECLARE_QSTATUS();
	
	*pwstrName = 0;
	*pnFlags = 0;
	
	bool bChildOfRootFolder = false;
	size_t nRootFolderLen = wcslen(wstrRootFolder_);
	if (nRootFolderLen != 0) {
		bChildOfRootFolder = wcsncmp(pwszName, wstrRootFolder_, nRootFolderLen) == 0 &&
			*(pwszName + nRootFolderLen) == cSeparator;
		if (bChildOfRootFolder)
			pwszName += nRootFolderLen + 1;
	}
	if (!*pwszName)
		return QSTATUS_SUCCESS;
	
	string_ptr<WSTRING> wstr(allocWString(pwszName));
	if (!wstr.get())
		return QSTATUS_OUTOFMEMORY;
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
		{ L"Inbox",					Folder::FLAG_INBOX							},
		{ wstrSpecialFolders_[0],	Folder::FLAG_OUTBOX | Folder::FLAG_DRAFTBOX	},
		{ wstrSpecialFolders_[1],	Folder::FLAG_SENTBOX						},
		{ wstrSpecialFolders_[2],	Folder::FLAG_TRASHBOX						}
	};
	for (int n = 0; n < countof(flags); ++n) {
		if (Util::isEqualFolderName(wstr.get(), flags[n].pwszName_, cSeparator))
			nFlags |= flags[n].nFlags_;
	}
	
	*pwstrName = wstr.release();
	*pnFlags = nFlags;
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * FolderListGetter
 *
 */

qmimap4::FolderListGetter::FolderListGetter(Account* pAccount,
	SubAccount* pSubAccount, QSTATUS* pstatus) :
	pAccount_(pAccount),
	pSubAccount_(pSubAccount),
	pFolderUtil_(0),
	pImap4_(0),
	pCallback_(0),
	pLogger_(0)
{
	DECLARE_QSTATUS();
	
	status = newQsObject(pAccount_, &pFolderUtil_);
	CHECK_QSTATUS_SET(pstatus);
	status = connect();
	CHECK_QSTATUS_SET(pstatus);
	status = listNamespaces();
	CHECK_QSTATUS_SET(pstatus);
	status = listFolders();
	CHECK_QSTATUS_SET(pstatus);
}

qmimap4::FolderListGetter::~FolderListGetter()
{
	if (pImap4_) {
		pImap4_->disconnect();
		delete pImap4_;
	}
	
	std::for_each(listNamespace_.begin(), listNamespace_.end(),
		unary_compose_f_gx(
			string_free<WSTRING>(),
			std::select1st<NamespaceList::value_type>()));
	FolderDataList::iterator it = listFolderData_.begin();
	while (it != listFolderData_.end())
		freeWString((*it++).wstrMailbox_);
	
	std::for_each(listFolder_.begin(), listFolder_.end(),
		unary_compose_f_gx(
			deleter<Folder>(),
			std::select1st<FolderList::value_type>()));
	
	delete pFolderUtil_;
	delete pCallback_;
	delete pLogger_;
}

QSTATUS qmimap4::FolderListGetter::getFolders(
	std::pair<Folder*, bool>** ppFolder, size_t* pnCount)
{
	assert(ppFolder);
	assert(pnCount);
	
	malloc_ptr<std::pair<Folder*, bool> > pFolder(
		static_cast<std::pair<Folder*, bool>*>(
			malloc(listFolder_.size()*sizeof(std::pair<Folder*, bool>))));
	if (!pFolder.get())
		return QSTATUS_OUTOFMEMORY;
	
	std::copy(listFolder_.begin(), listFolder_.end(), pFolder.get());
	
	*ppFolder = pFolder.release();
	*pnCount = listFolder_.size();
	
	listFolder_.clear();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::FolderListGetter::connect()
{
	DECLARE_QSTATUS();
	
	status = newQsObject(this, &pCallback_);
	CHECK_QSTATUS();
	
	if (pSubAccount_->isLog(Account::HOST_RECEIVE)) {
		status = pAccount_->openLogger(Account::HOST_RECEIVE, &pLogger_);
		CHECK_QSTATUS();
	}
	
	Imap4::Option option = {
		pSubAccount_->getTimeout(),
		pCallback_,
		pCallback_,
		pLogger_
	};
	status = newQsObject(option, &pImap4_);
	CHECK_QSTATUS();
	
	status = pImap4_->connect(pSubAccount_->getHost(Account::HOST_RECEIVE),
		pSubAccount_->getPort(Account::HOST_RECEIVE),
		pSubAccount_->isSsl(Account::HOST_RECEIVE));
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::FolderListGetter::listNamespaces()
{
	DECLARE_QSTATUS();
	
	int nUseNamespace = 0;
	status = pSubAccount_->getProperty(
		L"Imap4", L"UseNamespace", 1, &nUseNamespace);
	CHECK_QSTATUS();
	
	pCallback_->setNamespaceList(&listNamespace_);
	if (nUseNamespace && pImap4_->getCapability() & Imap4::CAPABILITY_NAMESPACE) {
		status = pImap4_->namespaceList();
		CHECK_QSTATUS();
	}
	else {
		status = pImap4_->list(false, L"", L"");
		CHECK_QSTATUS();
	}
	
	if (pFolderUtil_->isRootFolderSpecified() && !listNamespace_.empty()) {
		string_ptr<WSTRING> wstrInbox(allocWString(L"Inbox"));
		if (!wstrInbox.get())
			return QSTATUS_OUTOFMEMORY;
		status = STLWrapper<NamespaceList>(listNamespace_).push_back(
			std::make_pair(wstrInbox.get(), listNamespace_.front().second));
		CHECK_QSTATUS();
		wstrInbox.release();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::FolderListGetter::listFolders()
{
	DECLARE_QSTATUS();
	
	pCallback_->setFolderDataList(&listFolderData_);
	
	NamespaceList::iterator itNS = listNamespace_.begin();
	while (itNS != listNamespace_.end()) {
		status = pImap4_->list(false, L"", (*itNS).first);
		CHECK_QSTATUS();
		status = pImap4_->list(false, (*itNS).first, L"*");
		CHECK_QSTATUS();
		++itNS;
	}
	
	std::sort(listFolderData_.begin(), listFolderData_.end(), FolderDataLess());
	
	unsigned int nId = pAccount_->generateFolderId();
	FolderDataList::iterator itFD = listFolderData_.begin();
	while (itFD != listFolderData_.end()) {
		const FolderData& data = *itFD;
		
		Folder* pFolder = 0;
		status = getFolder(data.wstrMailbox_, data.cSeparator_,
			data.nFlags_, &nId, &pFolder);
		CHECK_QSTATUS();
		
		++itFD;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::FolderListGetter::getFolder(const WCHAR* pwszName,
	WCHAR cSeparator, unsigned int nFlags, unsigned int* pnId, Folder** ppFolder)
{
	assert(pwszName);
	assert(ppFolder);
	
	DECLARE_QSTATUS();
	
	*ppFolder = 0;
	
	Folder* pFolder = 0;
	
	FolderList::const_iterator itF = listFolder_.begin();
	while (itF != listFolder_.end()) {
		string_ptr<WSTRING> wstrName;
		status = (*itF).first->getFullName(&wstrName);
		CHECK_QSTATUS();
		if (wcscmp(wstrName.get(), pwszName) == 0) {
			pFolder = (*itF).first;
			break;
		}
		++itF;
	}
	
	if (!pFolder) {
		Folder* pParent = 0;
		const WCHAR* pName = 0;
		if (cSeparator != '\0')
			pName = wcsrchr(pwszName, cSeparator);
		if (pName) {
			string_ptr<WSTRING> wstrParentName(
				allocWString(pwszName, pName - pwszName));
			if (!wstrParentName.get())
				return QSTATUS_OUTOFMEMORY;
			status = getFolder(wstrParentName.get(), cSeparator,
				Folder::FLAG_NOSELECT, pnId, &pParent);
			CHECK_QSTATUS();
			
			++pName;
		}
		else {
			pName = pwszName;
		}
		
		bool bNew = false;
		status = pAccount_->getFolder(pwszName, &pFolder);
		CHECK_QSTATUS();
		if (pFolder) {
			// TODO
			// What happen if this folder is local folder or
			// this folder is query folder
			pFolder->setFlags(nFlags, ~Folder::FLAG_USER_MASK);
		}
		else {
			NormalFolder* pNormalFolder = 0;
			NormalFolder::Init init;
			init.nId_ = (*pnId)++;
			init.pwszName_ = pName;
			init.cSeparator_ = cSeparator;
			init.nFlags_ = nFlags;
			init.nCount_ = 0;
			init.nUnseenCount_ = 0;
			init.pParentFolder_ = pParent;
			init.pAccount_ = pAccount_;
			init.nValidity_ = 0;
			status = newQsObject(init, &pNormalFolder);
			CHECK_QSTATUS();
			
			pFolder = pNormalFolder;
			bNew = true;
		}
		status = STLWrapper<FolderList>(listFolder_).push_back(
			std::make_pair(pFolder, bNew));
		CHECK_QSTATUS();
	}
	
	*ppFolder = pFolder;
	
	return QSTATUS_SUCCESS;
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

qmimap4::FolderListGetter::CallbackImpl::CallbackImpl(
	FolderListGetter* pGetter, QSTATUS* pstatus) :
	AbstractCallback(pGetter->pSubAccount_, pstatus),
	pGetter_(pGetter),
	pListNamespace_(0),
	pListFolderData_(0)
{
}

qmimap4::FolderListGetter::CallbackImpl::~CallbackImpl()
{
}

void qmimap4::FolderListGetter::CallbackImpl::setNamespaceList(
	NamespaceList* pListNamespace)
{
	pListNamespace_ = pListNamespace;
	pListFolderData_ = 0;
}

void qmimap4::FolderListGetter::CallbackImpl::setFolderDataList(
	FolderDataList* pListFolderData)
{
	pListFolderData_ = pListFolderData;
	pListNamespace_ = 0;
}

bool qmimap4::FolderListGetter::CallbackImpl::isCanceled(bool bForce) const
{
	// TODO
	return false;
}

QSTATUS qmimap4::FolderListGetter::CallbackImpl::initialize()
{
	// TODO
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::FolderListGetter::CallbackImpl::lookup()
{
	// TODO
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::FolderListGetter::CallbackImpl::connecting()
{
	// TODO
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::FolderListGetter::CallbackImpl::connected()
{
	// TODO
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::FolderListGetter::CallbackImpl::authenticating()
{
	// TODO
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::FolderListGetter::CallbackImpl::setRange(
	unsigned int nMin, unsigned int nMax)
{
	// TODO
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::FolderListGetter::CallbackImpl::setPos(unsigned int nPos)
{
	// TODO
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::FolderListGetter::CallbackImpl::response(Response* pResponse)
{
	switch (pResponse->getType()) {
	case Response::TYPE_NAMESPACE:
		return processNamespace(
			static_cast<ResponseNamespace*>(pResponse));
	case Response::TYPE_LIST:
		return processList(
			static_cast<ResponseList*>(pResponse));
	default:
		break;
	}
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::FolderListGetter::CallbackImpl::processNamespace(
	ResponseNamespace* pNamespace)
{
	DECLARE_QSTATUS();
	
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
			NSList::const_iterator it = l.begin();
			while (it != l.end()) {
				string_ptr<WSTRING> wstr;
				const WCHAR* pwsz = (*it).first;
				if (!*pwsz && *pwszRootFolder) {
					WCHAR wsz[] = { (*it).second, L'\0' };
					wstr.reset(concat(pwszRootFolder, wsz));
				}
				else {
					wstr.reset(allocWString(pwsz));
				}
				if (!wstr.get())
					return QSTATUS_OUTOFMEMORY;
				status = STLWrapper<NamespaceList>(*pListNamespace_).push_back(
					std::make_pair(wstr.get(), (*it).second));
				CHECK_QSTATUS();
				wstr.release();
				++it;
			}
		}
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::FolderListGetter::CallbackImpl::processList(
	ResponseList* pList)
{
	DECLARE_QSTATUS();
	
	if (pListNamespace_) {
		const WCHAR* pwszRootFolder = pGetter_->pFolderUtil_->getRootFolder();
		WCHAR wszSeparator[] = { pList->getSeparator(), L'\0' };
		string_ptr<WSTRING> wstr(concat(
			pwszRootFolder, wszSeparator, pList->getMailbox()));
		if (!wstr.get())
			return QSTATUS_OUTOFMEMORY;
		status = STLWrapper<NamespaceList>(*pListNamespace_).push_back(
			std::make_pair(wstr.get(), pList->getSeparator()));
		CHECK_QSTATUS();
		wstr.release();
	}
	else {
		string_ptr<WSTRING> wstrName;
		unsigned int nFlags = 0;
		status = pGetter_->pFolderUtil_->getFolderData(pList->getMailbox(),
			pList->getSeparator(), pList->getAttributes(), &wstrName, &nFlags);
		CHECK_QSTATUS();
		if (wstrName.get()) {
			FolderData data = {
				wstrName.get(),
				pList->getSeparator(),
				nFlags
			};
			status = STLWrapper<FolderDataList>(
				*pListFolderData_).push_back(data);
			CHECK_QSTATUS();
			wstrName.release();
		}
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * SessionCache
 *
 */

qmimap4::SessionCache::SessionCache(Account* pAccount, SubAccount* pSubAccount,
	AbstractCallback* pCallback, size_t nMaxSession, QSTATUS* pstatus) :
	pAccount_(pAccount),
	pSubAccount_(pSubAccount),
	pCallback_(pCallback),
	nMaxSession_(nMaxSession)
{
	assert(pstatus);
	
	DECLARE_QSTATUS();
	
	*pstatus = QSTATUS_SUCCESS;
	
	status = STLWrapper<SessionList>(listSession_).reserve(nMaxSession);
	CHECK_QSTATUS_SET(pstatus);
}

qmimap4::SessionCache::~SessionCache()
{
	DECLARE_QSTATUS();
	
	SessionList::iterator it = listSession_.begin();
	while (it != listSession_.end()) {
		status = (*it).pImap4_->checkConnection();
		if (status == QSTATUS_SUCCESS)
			(*it).pImap4_->disconnect();
		delete (*it).pImap4_;
		delete (*it).pLogger_;
		++it;
	}
}

SubAccount* qmimap4::SessionCache::getSubAccount() const
{
	return pSubAccount_;
}

QSTATUS qmimap4::SessionCache::getSession(
	NormalFolder* pFolder, Imap4** ppImap4, Logger** ppLogger)
{
	assert(ppImap4);
	assert(ppLogger);
	
	DECLARE_QSTATUS();
	
	std::auto_ptr<Logger> pLogger;
	std::auto_ptr<Imap4> pImap4;
	bool bSelect = true;
	SessionList::iterator it = listSession_.end();
	if (pFolder)
		it = std::find_if(listSession_.begin(), listSession_.end(),
			std::bind2nd(
				binary_compose_f_gx_hy(
					std::equal_to<NormalFolder*>(),
					mem_data_ref(&Session::pFolder_),
					std::identity<NormalFolder*>()),
				pFolder));
	if (it != listSession_.end()) {
		pImap4.reset((*it).pImap4_);
		pLogger.reset((*it).pLogger_);
		listSession_.erase(it);
		bSelect = false;
	}
	else {
		if (listSession_.size() >= nMaxSession_) {
			it = listSession_.begin();
			pImap4.reset((*it).pImap4_);
			pLogger.reset((*it).pLogger_);
			listSession_.erase(it);
		}
	}
	
	if (pImap4.get()) {
		status = pImap4->checkConnection();
		if (status == QSTATUS_FAIL)
			pImap4.reset(0);
	}
	
	if (!pImap4.get()) {
		if (pSubAccount_->isLog(Account::HOST_RECEIVE)) {
			Logger* p = 0;
			status = pAccount_->openLogger(Account::HOST_RECEIVE, &p);
			CHECK_QSTATUS();
			pLogger.reset(p);
		}
		
		Imap4::Option option = {
			pSubAccount_->getTimeout(),
			pCallback_,
			pCallback_,
			pLogger.get()
		};
		status = newQsObject(option, &pImap4);
		CHECK_QSTATUS();
		status = pImap4->connect(
			pSubAccount_->getHost(Account::HOST_RECEIVE),
			pSubAccount_->getPort(Account::HOST_RECEIVE),
			pSubAccount_->isSsl(Account::HOST_RECEIVE));
		CHECK_QSTATUS();
		
		bSelect = true;
	}
	
	if (bSelect && pFolder) {
		string_ptr<WSTRING> wstrName;
		status = Util::getFolderName(pFolder, &wstrName);
		CHECK_QSTATUS();
		status = pImap4->select(wstrName.get());
		CHECK_QSTATUS();
	}
	
	*ppImap4 = pImap4.release();
	*ppLogger = pLogger.release();
	
	return QSTATUS_SUCCESS;
}

void qmimap4::SessionCache::releaseSession(
	NormalFolder* pFolder, Imap4* pImap4, Logger* pLogger)
{
	assert(listSession_.size() < nMaxSession_);
	Session s = { pFolder, pImap4, pLogger };
	listSession_.push_back(s);
}


/****************************************************************************
 *
 * SessionCacher
 *
 */

qmimap4::SessionCacher::SessionCacher(SessionCache* pCache,
	NormalFolder* pFolder, Imap4** ppImap4, QSTATUS* pstatus) :
	pCache_(pCache),
	pFolder_(pFolder),
	pImap4_(0),
	pLogger_(0)
{
	assert(pstatus);
	
	DECLARE_QSTATUS();
	
	status = pCache->getSession(pFolder, ppImap4, &pLogger_);
	CHECK_QSTATUS_SET(pstatus);
	
	pImap4_ = *ppImap4;
}

qmimap4::SessionCacher::~SessionCacher()
{
	delete pImap4_;
	delete pLogger_;
}

void qmimap4::SessionCacher::release()
{
	if (pImap4_) {
		pCache_->releaseSession(pFolder_, pImap4_, pLogger_);
		pImap4_ = 0;
		pLogger_ = 0;
	}
}
