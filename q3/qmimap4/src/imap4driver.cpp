/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#pragma warning(disable:4786)

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

using namespace qmimap4;
using namespace qm;
using namespace qs;


#define RETRY(action) RETRY_RETURN(action, false)

#define RETRY_RETURN(action, value) RETRY_COND_RETURN(action, false, value)

#define RETRY_COND(action, condition) RETRY_COND_RETURN(action, condition, false)

#define RETRY_COND_RETURN(action, condition, value) \
	for (int n = 0; ; ++n) { \
		if (action) \
			break; \
		if (cacher.isNew() || \
			!(pImap4->getLastError() & Socket::SOCKET_ERROR_MASK_SOCKET) || \
			condition) \
			return value; \
		if (!cacher.retry()) \
			return value; \
		pImap4 = cacher.get(); \
	} \


/****************************************************************************
 *
 * Imap4Driver
 *
 */

const unsigned int qmimap4::Imap4Driver::nSupport__ =
	Account::SUPPORT_REMOTEFOLDER |
	Account::SUPPORT_DELETEDMESSAGE |
	Account::SUPPORT_JUNKFILTER;

qmimap4::Imap4Driver::Imap4Driver(Account* pAccount,
								  PasswordCallback* pPasswordCallback,
								  const Security* pSecurity) :
	pAccount_(pAccount),
	pPasswordCallback_(pPasswordCallback),
	pSecurity_(pSecurity),
	pSubAccount_(0),
	bOffline_(true)
{
	pOfflineJobManager_.reset(new OfflineJobManager(pAccount_->getPath()));
}

qmimap4::Imap4Driver::~Imap4Driver()
{
}

bool qmimap4::Imap4Driver::save(bool bForce)
{
	Lock<CriticalSection> lock(cs_);
	
	return pOfflineJobManager_->save() || bForce;
}

bool qmimap4::Imap4Driver::isSupport(Account::Support support)
{
	return (nSupport__ & support) != 0;
}

void qmimap4::Imap4Driver::setOffline(bool bOffline)
{
	Lock<CriticalSection> lock(cs_);
	
	if (!bOffline_ && bOffline)
		pSessionCache_.reset(0);
	
	bOffline_ = bOffline;
}

void qmimap4::Imap4Driver::setSubAccount(qm::SubAccount* pSubAccount)
{
	if (pSubAccount_ != pSubAccount) {
		pSubAccount_ = pSubAccount;
		pSessionCache_.reset(0);
	}
}

std::auto_ptr<NormalFolder> qmimap4::Imap4Driver::createFolder(const WCHAR* pwszName,
															   Folder* pParent)
{
	assert(pwszName);
	
	Lock<CriticalSection> lock(cs_);
	
	wstring_ptr wstrRootFolder(pAccount_->getProperty(L"Imap4", L"RootFolder", L""));
	wstring_ptr wstrRootFolderSeparator(pAccount_->getProperty(L"Imap4", L"RootFolderSeparator", L"/"));
	
	if (!prepareSessionCache(false))
		return std::auto_ptr<NormalFolder>(0);
	
	SessionCacher cacher(pSessionCache_.get(), 0);
	Imap4* pImap4 = cacher.get();
	if (!pImap4)
		return std::auto_ptr<NormalFolder>(0);
	
	WCHAR cSeparator = L'/';
	wstring_ptr wstrFullName;
	if (pParent) {
		wstring_ptr wstrParentName(pParent->getFullName());
		cSeparator = pParent->getSeparator();
		ConcatW c[] = {
			{ wstrRootFolder.get(),				-1	},
			{ wstrRootFolderSeparator.get(),	1	},
			{ wstrParentName.get(),				-1	},
			{ &cSeparator,						1	},
			{ pwszName,							-1	}
		};
		bool bChildOfRoot = pParent->isFlag(Folder::FLAG_CHILDOFROOT) &&
			*wstrRootFolder.get() != L'\0';
		wstrFullName = concat(c + (bChildOfRoot ? 0 : 2),
			countof(c) - (bChildOfRoot ? 0 : 2));
	}
	else {
		ConcatW c[] = {
			{ wstrRootFolder.get(),				-1	},
			{ wstrRootFolderSeparator.get(),	1	},
			{ pwszName,							-1	}
		};
		bool bChildOfRoot = *wstrRootFolder.get() != L'\0';
		wstrFullName = concat(c + (bChildOfRoot ? 0 : 2),
			countof(c) - (bChildOfRoot ? 0 : 2));
	}
	
	RETRY_RETURN(pImap4->create(wstrFullName.get()), std::auto_ptr<NormalFolder>(0));
	
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
		return std::auto_ptr<NormalFolder>(0);
	if (!hook.bFound_)
		return std::auto_ptr<NormalFolder>(0);
	
	cacher.release();
	
	return std::auto_ptr<NormalFolder>(new NormalFolder(
		pAccount_->generateFolderId(), pwszName, hook.cSeparator_,
		hook.nFlags_, 0, 0, 0, 0, 0, pParent, pAccount_));
}

bool qmimap4::Imap4Driver::removeFolder(NormalFolder* pFolder)
{
	assert(pFolder);
	
	Lock<CriticalSection> lock(cs_);
	
	if (!prepareSessionCache(true))
		return false;
	
	SessionCacher cacher(pSessionCache_.get(), 0);
	Imap4* pImap4 = cacher.get();
	if (!pImap4)
		return 0;
	
	wstring_ptr wstrName(Util::getFolderName(pFolder));
	
	RETRY(pImap4->remove(wstrName.get()));
	
	cacher.release();
	
	return true;
}

bool qmimap4::Imap4Driver::renameFolder(NormalFolder* pFolder,
										const WCHAR* pwszName)
{
	assert(pFolder);
	assert(pwszName);
	
	Lock<CriticalSection> lock(cs_);
	
	if (!prepareSessionCache(true))
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
	
	RETRY(pImap4->rename(wstrOldName.get(), wstrNewName.get()));
	
	cacher.release();
	
	return true;
}

bool qmimap4::Imap4Driver::moveFolder(NormalFolder* pFolder,
									  NormalFolder* pParent,
									  const WCHAR* pwszName)
{
	assert(pFolder);
	
	Lock<CriticalSection> lock(cs_);
	
	if (!pwszName)
		pwszName = pFolder->getName();
	
	if (!prepareSessionCache(true))
		return false;
	
	SessionCacher cacher(pSessionCache_.get(), 0);
	Imap4* pImap4 = cacher.get();
	if (!pImap4)
		return 0;
	
	wstring_ptr wstrOldName(Util::getFolderName(pFolder));
	
	wstring_ptr wstrNewName;
	if (pParent) {
		wstring_ptr wstrParentName(Util::getFolderName(pParent));
		WCHAR wsz[] = { pParent->getSeparator(), L'\0' };
		wstrNewName = concat(wstrParentName.get(), wsz, pwszName);
	}
	else {
		wstring_ptr wstrRootFolder(pAccount_->getProperty(L"Imap4", L"RootFolder", L""));
		if (*wstrRootFolder.get()) {
			WCHAR wsz[] = { pFolder->getSeparator(), L'\0' };
			wstrNewName = concat(wstrRootFolder.get(), wsz, pwszName);
		}
		else {
			wstrNewName = allocWString(pwszName);
		}
	}
	
	RETRY(pImap4->rename(wstrOldName.get(), wstrNewName.get()));
	
	cacher.release();
	
	return true;
}

bool qmimap4::Imap4Driver::getRemoteFolders(RemoteFolderList* pList)
{
	assert(pList);
	
	Lock<CriticalSection> lock(cs_);
	
	FolderListGetter getter(pAccount_, pSubAccount_, pPasswordCallback_, pSecurity_);
	if (!getter.update())
		return false;
	getter.getFolders(pList);
	
	return true;
}

bool qmimap4::Imap4Driver::getMessage(MessageHolder* pmh,
									  unsigned int nFlags,
									  GetMessageCallback* pCallback)
{
	assert(pmh);
	assert(pCallback);
	assert(!pmh->getFolder()->isFlag(Folder::FLAG_LOCAL));
	assert(!pmh->isFlag(MessageHolder::FLAG_LOCAL));
	
	if (bOffline_)
		return true;
	
	Lock<CriticalSection> lock(cs_);
	
	unsigned int nOption = pSubAccount_->getProperty(L"Imap4", L"Option", 0xff);
	
	if (!prepareSessionCache(false))
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
						GetMessageCallback* pCallback) :
			nUid_(nUid),
			bHeaderOnly_(bHeaderOnly),
			pmh_(pmh),
			pCallback_(pCallback),
			bProcessed_(false)
		{
		}
		
		virtual bool processFetchResponse(ResponseFetch* pFetch)
		{
			unsigned int nUid = pFetch->getUid();
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
				case FetchData::TYPE_FLAGS:
					pmh_->setFlags(Util::getMessageFlagsFromImap4Flags(
						static_cast<FetchDataFlags*>(*it)->getSystemFlags(),
						static_cast<FetchDataFlags*>(*it)->getCustomFlags()), nMask);
					break;
				}
			}
			
			if (nUid == nUid_ && pBody) {
				bProcessed_ = true;
				
				FetchDataBody::Section s = pBody->getSection();
				if (((s == FetchDataBody::SECTION_NONE && !bHeaderOnly_) ||
					(s == FetchDataBody::SECTION_HEADER && bHeaderOnly_)) &&
					pBody->getPartPath().empty()) {
					std::pair<const CHAR*, size_t> content(pBody->getContent().get());
					if (!pCallback_->message(content.first, content.second,
						bHeaderOnly_ ? Message::FLAG_HEADERONLY : Message::FLAG_NONE, true))
						return false;
				}
			}
			
			return true;
		}
		
		unsigned int nUid_;
		bool bHeaderOnly_;
		MessageHolder* pmh_;
		GetMessageCallback* pCallback_;
		bool bProcessed_;
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
			unsigned int nUid = pFetch->getUid();
			FetchDataBodyStructure* pBodyStructure = 0;
			
			const ResponseFetch::FetchDataList& l = pFetch->getFetchDataList();
			for (ResponseFetch::FetchDataList::const_iterator it = l.begin(); it != l.end(); ++it) {
				switch ((*it)->getType()) {
				case FetchData::TYPE_BODYSTRUCTURE:
					pBodyStructure = static_cast<FetchDataBodyStructure*>(*it);
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
							bool bHtml,
							MessageHolder* pmh,
							unsigned int nOption,
							GetMessageCallback* pCallback) :
			nUid_(nUid),
			pBodyStructure_(pBodyStructure),
			listPart_(listPart),
			nPartCount_(nPartCount),
			bHtml_(bHtml),
			pmh_(pmh),
			nOption_(nOption),
			pCallback_(pCallback)
		{
		}
		
		virtual bool processFetchResponse(ResponseFetch* pFetch)
		{
			unsigned int nUid = pFetch->getUid();
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
				case FetchData::TYPE_FLAGS:
					pmh_->setFlags(Util::getMessageFlagsFromImap4Flags(
						static_cast<FetchDataFlags*>(*it)->getSystemFlags(),
						static_cast<FetchDataFlags*>(*it)->getCustomFlags()), nMask);
					break;
				}
			}
			
			if (listBody.size() == nPartCount_ && nUid == nUid_) {
				xstring_size_ptr strContent(Util::getContentFromBodyStructureAndBodies(
					listPart_, listBody, (nOption_ & OPTION_TRUSTBODYSTRUCTURE) != 0));
				if (!strContent.get())
					return false;
				if (!pCallback_->message(strContent.get(), strContent.size(),
					bHtml_ ? Message::FLAG_HTMLONLY : Message::FLAG_TEXTONLY, true))
					return false;
			}
			
			return true;
		}
		
		unsigned int nUid_;
		FetchDataBodyStructure* pBodyStructure_;
		const PartList& listPart_;
		unsigned int nPartCount_;
		bool bHtml_;
		MessageHolder* pmh_;
		unsigned int nOption_;
		GetMessageCallback* pCallback_;
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
		RETRY_COND(pImap4->getBodyStructure(range), hook.getBodyStructure());
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
					(nFlags & Account::GETMESSAGEFLAG_MAKESEEN) == 0,
					(nOption & OPTION_TRUSTBODYSTRUCTURE) == 0,
					&strArg, &nPartCount, &bAll);
				
				BodyListProcessHook hook(pmh->getId(), pBodyStructure,
					listPart, nPartCount, bHtml, pmh, nOption, pCallback);
				Hook h(pCallback_.get(), &hook);
				
				if (!pImap4->fetch(range, strArg.get()))
					return false;
			}
		}
	}
	else {
		if (bHeaderOnly) {
			BodyProcessHook hook(pmh->getId(), true, pmh, pCallback);
			Hook h(pCallback_.get(), &hook);
			RETRY_COND(pImap4->getHeader(range, (nFlags & Account::GETMESSAGEFLAG_MAKESEEN) == 0), hook.bProcessed_);
		}
		else {
			BodyProcessHook hook(pmh->getId(), false, pmh, pCallback);
			Hook h(pCallback_.get(), &hook);
			RETRY_COND(pImap4->getMessage(range, (nFlags & Account::GETMESSAGEFLAG_MAKESEEN) == 0), hook.bProcessed_);
		}
	}
	
	cacher.release();
	
	return true;
}

bool qmimap4::Imap4Driver::setMessagesFlags(NormalFolder* pFolder,
											const MessageHolderList& l,
											unsigned int nFlags,
											unsigned int nMask)
{
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
		
		if (!prepareSessionCache(false))
			return false;
		
		SessionCacher cacher(pSessionCache_.get(), pFolder);
		Imap4* pImap4 = cacher.get();
		if (!pImap4)
			return false;
		
		RETRY(setFlags(pImap4, *pRange, pFolder, listUpdate, nFlags, nMask));
		
		cacher.release();
	}
	
	return true;
}

bool qmimap4::Imap4Driver::setMessagesLabel(NormalFolder* pFolder,
											const MessageHolderList& l,
											const WCHAR* pwszLabel)
{
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
		wstring_ptr wstrLabel(pmh->getLabel());
		if ((!pwszLabel && !*wstrLabel.get()) ||
			(pwszLabel && wcscmp(pwszLabel, wstrLabel.get()) != 0)) {
			if (pmh->isFlag(MessageHolder::FLAG_LOCAL))
				pmh->setLabel(pwszLabel);
			else
				listUpdate.push_back(pmh);
		}
	}
	if (listUpdate.empty())
		return true;
	
	typedef std::vector<WSTRING> LabelList;
	LabelList listLabel;
	StringListFree<LabelList> free(listLabel);
	for (MessageHolderList::const_iterator it = listUpdate.begin(); it != listUpdate.end(); ++it) {
		MessageHolder* pmh = *it;
		wstring_ptr wstrLabel(pmh->getLabel());
		if (wstrLabel.get() && *wstrLabel.get()) {
			if (std::find_if(listLabel.begin(), listLabel.end(),
				std::bind2nd(string_equal<WCHAR>(), wstrLabel.get())) == listLabel.end()) {
				listLabel.push_back(wstrLabel.get());
				wstrLabel.release();
			}
		}
	}
	if (pwszLabel && *pwszLabel) {
		if (std::find_if(listLabel.begin(), listLabel.end(),
			std::bind2nd(string_equal<WCHAR>(), pwszLabel)) == listLabel.end()) {
			wstring_ptr wstrLabel(allocWString(pwszLabel));
			listLabel.push_back(wstrLabel.get());
			wstrLabel.release();
		}
	}
	
	if (bOffline_) {
		Util::UidList listUid;
		Util::createUidList(l, &listUid);
		
		if (!listUid.empty()) {
			wstring_ptr wstrFolder(pFolder->getFullName());
			std::auto_ptr<SetLabelOfflineJob> pJob(new SetLabelOfflineJob(
				wstrFolder.get(), listUid, pwszLabel,
				const_cast<const WCHAR**>(&listLabel[0]), listLabel.size()));
			pOfflineJobManager_->add(pJob);
		}
	}
	else {
		Lock<CriticalSection> lock(cs_);
		
		if (!prepareSessionCache(false))
			return false;
		
		SessionCacher cacher(pSessionCache_.get(), pFolder);
		Imap4* pImap4 = cacher.get();
		if (!pImap4)
			return false;
		
		FlagProcessHook hook(pFolder);
		Hook h(pCallback_.get(), &hook);
		
		std::auto_ptr<MultipleRange> pRange(Util::createRange(listUpdate));
		std::auto_ptr<Flags> pFlags(Util::getImap4FlagsFromLabels(
			0, &pwszLabel, pwszLabel && *pwszLabel ? 1 : 0));
		if (!pFlags.get())
			return false;
		std::auto_ptr<Flags> pMask(Util::getImap4FlagsFromLabels(0,
			const_cast<const WCHAR**>(&listLabel[0]), listLabel.size()));
		if (!pMask.get())
			return false;
		RETRY(pImap4->setFlags(*pRange, *pFlags, *pMask));
		
		cacher.release();
	}
	for (MessageHolderList::const_iterator it = listUpdate.begin(); it != listUpdate.end(); ++it)
		(*it)->setLabel(pwszLabel);
	
	return true;
}

bool qmimap4::Imap4Driver::appendMessage(NormalFolder* pFolder,
										 const CHAR* pszMessage,
										 size_t nLen,
										 unsigned int nFlags,
										 const WCHAR* pwszLabel)
{
	assert(pFolder);
	assert(pszMessage);
	
	Lock<Account> lock(*pAccount_);
	
	if (bOffline_) {
		MessageHolder* pmh = pAccount_->storeMessage(pFolder, pszMessage,
			nLen, 0, -1, nFlags | MessageHolder::FLAG_LOCAL, pwszLabel, -1, false);
		if (!pmh)
			return false;
		
		wstring_ptr wstrFolder(pFolder->getFullName());
		std::auto_ptr<AppendOfflineJob> pJob(
			new AppendOfflineJob(wstrFolder.get(), pmh->getId()));
		pOfflineJobManager_->add(pJob);
	}
	else {
		Lock<CriticalSection> lock(cs_);
		
		if (!prepareSessionCache(false))
			return false;
		
		SessionCacher cacher(pSessionCache_.get(), 0);
		Imap4* pImap4 = cacher.get();
		if (!pImap4)
			return false;
		
		wstring_ptr wstrFolderName(Util::getFolderName(pFolder));
		
		std::auto_ptr<Flags> pFlags(Util::getImap4FlagsFromLabels(nFlags,
			&pwszLabel, pwszLabel && *pwszLabel ? 1 : 0));
		if (!pFlags.get())
			return false;
		RETRY(pImap4->append(wstrFolderName.get(), pszMessage, nLen, *pFlags));
		
		cacher.release();
	}
	
	return true;
}

bool qmimap4::Imap4Driver::removeMessages(NormalFolder* pFolder,
										  const MessageHolderList& l)
{
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
	
	return setMessagesFlags(pFolder, l,
		MessageHolder::FLAG_DELETED, MessageHolder::FLAG_DELETED);
}

bool qmimap4::Imap4Driver::copyMessages(const MessageHolderList& l,
										NormalFolder* pFolderFrom,
										NormalFolder* pFolderTo,
										bool bMove)
{
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
		
		if (!prepareSessionCache(false))
			return false;
		
		SessionCacher cacher(pSessionCache_.get(), pFolderFrom);
		Imap4* pImap4 = cacher.get();
		if (!pImap4)
			return false;
		
		wstring_ptr wstrFolderName(Util::getFolderName(pFolderTo));
		RETRY(pImap4->copy(*pRange, wstrFolderName.get()));
		
		cacher.release();
		
		if (bMove) {
			if (!setFlags(pImap4, *pRange, pFolderFrom, listUpdate,
				MessageHolder::FLAG_DELETED, MessageHolder::FLAG_DELETED))
				return false;
		}
	}
	
	return true;
}

OfflineJobManager* qmimap4::Imap4Driver::getOfflineJobManager() const
{
	return pOfflineJobManager_.get();
}

bool qmimap4::Imap4Driver::search(NormalFolder* pFolder,
								  const WCHAR* pwszCondition,
								  const WCHAR* pwszCharset,
								  bool bUseCharset,
								  MessageHolderList* pList)
{
	assert(pFolder);
	assert(pwszCondition);
	assert(pList);
	
	if (!bOffline_) {
		Lock<CriticalSection> lock(cs_);
		
		if (!prepareSessionCache(false))
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
				pList_(pList),
				bProcessed_(false)
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
				
				bProcessed_ = true;
				
				return true;
			}
			
			NormalFolder* pFolder_;
			MessageHolderList* pList_;
			bool bProcessed_;
		} hook(pFolder, pList);
		
		Hook h(pCallback_.get(), &hook);
		RETRY_COND(pImap4->search(pwszCondition, pwszCharset, bUseCharset, true), hook.bProcessed_);
		
		cacher.release();
	}
	
	return true;
}

bool qmimap4::Imap4Driver::prepareSessionCache(bool bClear)
{
	if (bClear)
		pSessionCache_.reset(0);
	
	if (!pSessionCache_.get()) {
		pCallback_.reset(new CallbackImpl(pSubAccount_, pPasswordCallback_, pSecurity_));
		
		int nMaxSession = pAccount_->getProperty(L"Imap4", L"MaxSession", 5);
		pSessionCache_.reset(new SessionCache(pAccount_,
			pSubAccount_, pCallback_.get(), nMaxSession));
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
	unsigned int nUid = pFetch->getUid();
	unsigned int nFlags = 0;
	wstring_ptr wstrLabel;
	
	int nCount = 0;
	
	const ResponseFetch::FetchDataList& l = pFetch->getFetchDataList();
	for (ResponseFetch::FetchDataList::const_iterator it = l.begin(); it != l.end(); ++it) {
		switch ((*it)->getType()) {
		case FetchData::TYPE_FLAGS:
			nFlags = Util::getMessageFlagsFromImap4Flags(
				static_cast<FetchDataFlags*>(*it)->getSystemFlags(),
				static_cast<FetchDataFlags*>(*it)->getCustomFlags());
			wstrLabel = Util::getLabelFromImap4Flags(
				static_cast<FetchDataFlags*>(*it)->getCustomFlags());
			++nCount;
			break;
		}
	}
	
	if (nUid != -1 && nCount == 1) {
		MessagePtr ptr(pFolder_->getMessageById(nUid));
		MessagePtrLock mpl(ptr);
		if (mpl) {
			mpl->setFlags(nFlags, nFlags);
			mpl->setLabel(wstrLabel.get());
		}
	}
	
	return true;
}


/****************************************************************************
 *
 * Imap4Driver::CallbackImpl
 *
 */

qmimap4::Imap4Driver::CallbackImpl::CallbackImpl(SubAccount* pSubAccount,
												 PasswordCallback* pPasswordCallback,
												 const Security* pSecurity) :
	AbstractCallback(pSubAccount, pPasswordCallback, pSecurity),
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

void qmimap4::Imap4Driver::CallbackImpl::setRange(size_t nMin,
												  size_t nMax)
{
}

void qmimap4::Imap4Driver::CallbackImpl::setPos(size_t nPos)
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
																  PasswordCallback* pPasswordCallback,
																  const qm::Security* pSecurity)
{
	assert(pAccount);
	assert(pPasswordCallback);
	assert(pSecurity);
	
	return std::auto_ptr<ProtocolDriver>(new Imap4Driver(
		pAccount, pPasswordCallback, pSecurity));
}


/****************************************************************************
 *
 * FolderUtil
 *
 */

qmimap4::FolderUtil::FolderUtil(Account* pAccount) :
	pAccount_(pAccount),
	cRootFolderSeparator_(L'/')
{
	wstrRootFolder_ = pAccount->getProperty(L"Imap4", L"RootFolder", L"");
	
	wstring_ptr wstrRootFolderSeparator(pAccount->getProperty(
		L"Imap4", L"RootFolderSeparator", L"/"));
	if (*wstrRootFolderSeparator.get())
		cRootFolderSeparator_ = *wstrRootFolderSeparator.get();
	
	struct {
		const WCHAR* pwszKey_;
		const WCHAR* pwszDefault_;
	} folders[] = {
		{ L"OutboxFolder",		L"Outbox"	},
		{ L"DraftboxFolder",	L"Outbox"	},
		{ L"SentboxFolder",		L"Sentbox"	},
		{ L"TrashFolder",		L"Trash"	},
		{ L"JunkFolder",		L"Junk"		}
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

WCHAR qmimap4::FolderUtil::getRootFolderSeparator() const
{
	return cRootFolderSeparator_;
}

void qmimap4::FolderUtil::setRootFolderSeparator(WCHAR c)
{
	cRootFolderSeparator_ = c;
}

void qmimap4::FolderUtil::getFolderData(const WCHAR* pwszName,
										WCHAR cSeparator,
										unsigned int nAttributes,
										wstring_ptr* pwstrName,
										unsigned int* pnFlags) const
{
	assert(pwszName);
	assert(*pwszName);
	assert(pwstrName);
	assert(pnFlags);
	
	pwstrName->reset(0);
	*pnFlags = 0;
	
	bool bChildOfRootFolder = false;
	size_t nRootFolderLen = wcslen(wstrRootFolder_.get());
	if (nRootFolderLen != 0) {
		if (wcsncmp(pwszName, wstrRootFolder_.get(), nRootFolderLen) == 0) {
			const WCHAR* p = pwszName + nRootFolderLen;
			if (*p == L'\0' || (*p == cSeparator && *(p + 1) == L'\0')) {
				return;
			}
			else if (*p == cSeparator) {
				bChildOfRootFolder = true;
				pwszName += nRootFolderLen + 1;
			}
		}
	}
	
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
		{ wstrSpecialFolders_[3].get(),	Folder::FLAG_TRASHBOX						},
		{ wstrSpecialFolders_[4].get(),	Folder::FLAG_JUNKBOX						}
	};
	for (int n = 0; n < countof(flags); ++n) {
		if (Util::isEqualFolderName(wstr.get(), flags[n].pwszName_, cSeparator))
			nFlags |= flags[n].nFlags_;
	}
	
	*pwstrName = wstr;
	*pnFlags = nFlags;
}

void qmimap4::FolderUtil::save() const
{
	WCHAR wszRootFolderSeparator[] = { cRootFolderSeparator_, L'\0' };
	pAccount_->setProperty(L"Imap4", L"RootFolderSeparator", wszRootFolderSeparator);
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
		{ Folder::FLAG_TRASHBOX,	L"TrashFolder"		},
		{ Folder::FLAG_JUNKBOX,		L"JunkFolder"		}
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
											PasswordCallback* pPasswordCallback,
											const Security* pSecurity) :
	pAccount_(pAccount),
	pSubAccount_(pSubAccount),
	pPasswordCallback_(pPasswordCallback),
	pSecurity_(pSecurity)
{
	FolderUtil::saveSpecialFolders(pAccount_);
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
	
	for (FolderInfoList::const_iterator it = listFolderInfo_.begin(); it != listFolderInfo_.end(); ++it) {
		if ((*it).bNew_)
			delete (*it).pFolder_;
		freeWString((*it).wstrFullName_);
	}
	
	pFolderUtil_->save();
}

bool qmimap4::FolderListGetter::update()
{
	return connect() && listNamespaces() && listFolders();
}

void qmimap4::FolderListGetter::getFolders(Imap4Driver::RemoteFolderList* pList)
{
	assert(pList);
	
	pList->resize(listFolderInfo_.size());
	for (FolderInfoList::size_type n = 0; n < listFolderInfo_.size(); ++n) {
		FolderInfo& info = listFolderInfo_[n];
		(*pList)[n] = std::make_pair(info.pFolder_, info.bNew_);
		freeWString(info.wstrFullName_);
	}
	listFolderInfo_.clear();
}

bool qmimap4::FolderListGetter::connect()
{
	pCallback_.reset(new CallbackImpl(this, pPasswordCallback_, pSecurity_));
	
	if (pSubAccount_->isLog(Account::HOST_RECEIVE))
		pLogger_ = pAccount_->openLogger(Account::HOST_RECEIVE);
	
	pImap4_.reset(new Imap4(pSubAccount_->getTimeout(), pCallback_.get(),
		pCallback_.get(), pCallback_.get(), pLogger_.get()));
	
	Imap4::Secure secure = Util::getSecure(pSubAccount_);
	return pImap4_->connect(pSubAccount_->getHost(Account::HOST_RECEIVE),
		pSubAccount_->getPort(Account::HOST_RECEIVE), secure);
}

bool qmimap4::FolderListGetter::listNamespaces()
{
	pCallback_->setNamespaceList(&listNamespace_);
	
	bool bUseNamespace = pSubAccount_->getProperty(L"Imap4", L"UseNamespace", 0) &&
		pImap4_->getCapability() & Imap4::CAPABILITY_NAMESPACE;
	if (bUseNamespace) {
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
		if (*(*itNS).first) {
			if (!pImap4_->list(false, L"", (*itNS).first))
				return false;
		}
		if (!pImap4_->list(false, (*itNS).first, L"*"))
			return false;
	}
	
	std::sort(listFolderData_.begin(), listFolderData_.end(), FolderDataLess());
	
	unsigned int nId = pAccount_->generateFolderId();
	for (FolderDataList::iterator itFD = listFolderData_.begin(); itFD != listFolderData_.end(); ++itFD) {
		const FolderData& data = *itFD;
		getFolder(data.wstrMailbox_, data.cSeparator_, data.nFlags_, &nId);
	}
	
	return true;
}

Folder* qmimap4::FolderListGetter::getFolder(const WCHAR* pwszName,
											 WCHAR cSeparator,
											 unsigned int nFlags,
											 unsigned int* pnId)
{
	assert(pwszName);
	
	FolderInfo info = {
		0,
		false,
		const_cast<WSTRING>(pwszName)
	};
	FolderInfoList::iterator it = std::lower_bound(
		listFolderInfo_.begin(), listFolderInfo_.end(),
		info, FolderInfoLess());
	if (it != listFolderInfo_.end() && wcscmp(pwszName, (*it).wstrFullName_) == 0)
		return (*it).pFolder_;
	
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
	Folder* pFolder = pAccount_->getFolder(pwszName);
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
	
	wstring_ptr wstrFullName(pFolder->getFullName());
	assert(wcscmp(wstrFullName.get(), pwszName) == 0);
	FolderInfo infoNew = {
		pFolder,
		bNew,
		wstrFullName.get()
	};
	listFolderInfo_.insert(it, infoNew);
	wstrFullName.release();
	
	return pFolder;
}


/****************************************************************************
 *
 * FolderListGetter::FolderDataLess
 *
 */

bool qmimap4::FolderListGetter::FolderDataLess::operator()(const FolderData& lhs,
														   const FolderData& rhs) const
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
 * FolderListGetter::FolderInfoLess
 *
 */

bool qmimap4::FolderListGetter::FolderInfoLess::operator()(const FolderInfo& lhs,
														   const FolderInfo& rhs) const
{
	return wcscmp(lhs.wstrFullName_, rhs.wstrFullName_) < 0;
}


/****************************************************************************
 *
 * FolderListGetter::CallbackImpl
 *
 */

qmimap4::FolderListGetter::CallbackImpl::CallbackImpl(FolderListGetter* pGetter,
													  PasswordCallback* pPasswordCallback,
													  const Security* pSecurity) :
	AbstractCallback(pGetter->pSubAccount_, pPasswordCallback, pSecurity),
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

void qmimap4::FolderListGetter::CallbackImpl::setRange(size_t nMin,
													   size_t nMax)
{
	// TODO
}

void qmimap4::FolderListGetter::CallbackImpl::setPos(size_t nPos)
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
		bool bUse[] = {
			pGetter_->pSubAccount_->getProperty(L"Imap4", L"UsePersonal", 1) != 0,
			pGetter_->pSubAccount_->getProperty(L"Imap4", L"UseOthers", 1) != 0,
			pGetter_->pSubAccount_->getProperty(L"Imap4", L"UseShared", 1) != 0
		};
		for (int n = 0; n < countof(pLists); ++n) {
			if (bUse[n]) {
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
	}
	
	return true;
}

bool qmimap4::FolderListGetter::CallbackImpl::processList(ResponseList* pList)
{
	if (pListNamespace_) {
		FolderUtil* pFolderUtil = pGetter_->pFolderUtil_.get();
		wstring_ptr wstr;
		if (pFolderUtil->isRootFolderSpecified()) {
			WCHAR wszSeparator[] = { pList->getSeparator(), L'\0' };
			wstr = concat(pFolderUtil->getRootFolder(), wszSeparator, pList->getMailbox());
			pFolderUtil->setRootFolderSeparator(pList->getSeparator());
		}
		else {
			wstr = allocWString(pList->getMailbox());
		}
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
		if (!isForceDisconnect((*it).nLastUsedTime_) && (*it).pImap4_->checkConnection())
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
									   Session* pSession,
									   bool* pbNew)
{
	assert(pSession);
	
	for (int n = 0; ; ++n) {
		std::auto_ptr<Logger> pLogger;
		std::auto_ptr<Imap4> pImap4;
		unsigned int nLastSelectedTime = 0;
		bool bNew = true;
		if (!getSessionWithoutSelect(pFolder, &pLogger, &pImap4, &nLastSelectedTime, &bNew))
			return false;
		
		if (pFolder && isNeedSelect(pFolder, nLastSelectedTime)) {
			wstring_ptr wstrName(Util::getFolderName(pFolder));
			if (pImap4->select(wstrName.get()))
				nLastSelectedTime = ::GetTickCount();
			else if (bNew || !(pImap4->getLastError() & Socket::SOCKET_ERROR_MASK_SOCKET))
				return false;
			else
				continue;
		}
		
		pSession->pFolder_ = pFolder;
		pSession->pImap4_ = pImap4.release();
		pSession->pLogger_ = pLogger.release();
		pSession->nLastUsedTime_ = 0;
		pSession->nLastSelectedTime_ = nLastSelectedTime;
		*pbNew = bNew;
		
		break;
	}
	
	return true;
}

void qmimap4::SessionCache::releaseSession(Session session)
{
	assert(listSession_.size() < nMaxSession_);
	session.nLastUsedTime_ = ::GetTickCount();
	listSession_.push_back(session);
}

bool qmimap4::SessionCache::getSessionWithoutSelect(NormalFolder* pFolder,
													std::auto_ptr<Logger>* ppLogger,
													std::auto_ptr<Imap4>* ppImap4,
													unsigned int* pnLastSelectedTime,
													bool* pbNew)
{
	assert(ppLogger);
	assert(ppImap4);
	assert(pnLastSelectedTime);
	assert(pbNew);
	
	std::auto_ptr<Logger> pLogger;
	std::auto_ptr<Imap4> pImap4;
	unsigned int nLastUsedTime = 0;
	unsigned int nLastSelectedTime = 0;
	bool bNew = false;
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
		if (isForceDisconnect(nLastUsedTime) || !pImap4->checkConnection())
			pImap4.reset(0);
	}
	
	if (!pImap4.get()) {
		if (pSubAccount_->isLog(Account::HOST_RECEIVE))
			pLogger = pAccount_->openLogger(Account::HOST_RECEIVE);
		
		pImap4.reset(new Imap4(pSubAccount_->getTimeout(),
			pCallback_, pCallback_, pCallback_, pLogger.get()));
		Imap4::Secure secure = Util::getSecure(pSubAccount_);
		if (!pImap4->connect(pSubAccount_->getHost(Account::HOST_RECEIVE),
			pSubAccount_->getPort(Account::HOST_RECEIVE), secure))
			return false;
		
		nLastSelectedTime = 0;
		bNew = true;
	}
	
	*ppLogger = pLogger;
	*ppImap4 = pImap4;
	*pnLastSelectedTime = nLastSelectedTime;
	*pbNew = bNew;
	
	return true;
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

bool qmimap4::SessionCache::isForceDisconnect(unsigned int nLastUsedTime) const
{
	return nForceDisconnect_ != 0 && nLastUsedTime + nForceDisconnect_*1000 < ::GetTickCount();
}


/****************************************************************************
 *
 * SessionCacher
 *
 */

qmimap4::SessionCacher::SessionCacher(SessionCache* pCache,
									  NormalFolder* pFolder) :
	pCache_(pCache),
	pFolder_(pFolder),
	bNew_(true)
{
	init();
	create();
}

qmimap4::SessionCacher::~SessionCacher()
{
	destroy();
}

Imap4* qmimap4::SessionCacher::get() const
{
	return session_.pImap4_;
}

bool qmimap4::SessionCacher::isNew() const
{
	return bNew_;
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

bool qmimap4::SessionCacher::retry()
{
	destroy();
	return create();
}

void qmimap4::SessionCacher::init()
{
	session_.pFolder_ = 0;
	session_.pImap4_ = 0;
	session_.pLogger_ = 0;
	session_.nLastSelectedTime_ = 0;
	bNew_ = true;
}

bool qmimap4::SessionCacher::create()
{
	if (!pCache_->getSession(pFolder_, &session_, &bNew_)) {
		assert(!session_.pFolder_);
		assert(!session_.pImap4_);
		assert(!session_.pLogger_);
		return false;
	}
	return true;
}

void qmimap4::SessionCacher::destroy()
{
	delete session_.pImap4_;
	delete session_.pLogger_;
	
	init();
}
