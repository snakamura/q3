/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmfolder.h>
#include <qmaccount.h>
#include <qmextensions.h>
#include <qmmessage.h>
#include <qmmessageholder.h>
#include <qmmessageoperation.h>

#include <qsassert.h>
#include <qsconv.h>
#include <qserror.h>
#include <qsfile.h>
#include <qsnew.h>
#include <qsstl.h>
#include <qsstream.h>
#include <qsthread.h>

#include <memory>
#include <algorithm>

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * FolderImpl
 *
 */

struct qm::FolderImpl
{
	typedef std::vector<FolderHandler*> FolderHandlerList;
	
	QSTATUS getMessages(const MessagePtrList& l,
		MessageHolderList* pList) const;
	
	QSTATUS fireMessageAdded(MessageHolder* pmh);
	QSTATUS fireMessageRemoved(MessageHolder* pmh);
	QSTATUS fireFolderDestroyed();
	
	Folder* pThis_;
	unsigned int nId_;
	WSTRING wstrName_;
	WCHAR cSeparator_;
	unsigned int nFlags_;
	Folder* pParentFolder_;
	Account* pAccount_;
	
	FolderHandlerList listFolderHandler_;
	
	bool bDestroyed_;
};

QSTATUS qm::FolderImpl::getMessages(const MessagePtrList& l,
	MessageHolderList* pList) const
{
	assert(pList);
	assert(pAccount_->isLocked());
	
	DECLARE_QSTATUS();
	
	status = STLWrapper<MessageHolderList>(*pList).reserve(l.size());
	CHECK_QSTATUS();
	
	MessagePtrList::const_iterator it = l.begin();
	while (it != l.end()) {
		MessagePtrLock mpl(*it);
		if (mpl)
			pList->push_back(mpl);
		++it;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::FolderImpl::fireMessageAdded(MessageHolder* pmh)
{
	assert(pmh);
	assert(pAccount_->isLocked());
	
	DECLARE_QSTATUS();
	
	FolderEvent event(pThis_, pmh);
	FolderHandlerList::const_iterator it = listFolderHandler_.begin();
	while (it != listFolderHandler_.end()) {
		status = (*it)->messageAdded(event);
		CHECK_QSTATUS();
		++it;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::FolderImpl::fireMessageRemoved(MessageHolder* pmh)
{
	assert(pmh);
	assert(pAccount_->isLocked());
	
	DECLARE_QSTATUS();
	
	if (bDestroyed_)
		return QSTATUS_SUCCESS;
	
	FolderEvent event(pThis_, pmh);
	FolderHandlerList::const_iterator it = listFolderHandler_.begin();
	while (it != listFolderHandler_.end()) {
		status = (*it)->messageRemoved(event);
		CHECK_QSTATUS();
		++it;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::FolderImpl::fireFolderDestroyed()
{
	DECLARE_QSTATUS();
	
	FolderEvent event(pThis_, 0);
	
	FolderHandlerList l;
	status = STLWrapper<FolderHandlerList>(l).resize(listFolderHandler_.size());
	CHECK_QSTATUS();
	std::copy(listFolderHandler_.begin(), listFolderHandler_.end(), l.begin());
	
	FolderHandlerList::const_iterator it = l.begin();
	while (it != l.end()) {
		status = (*it)->folderDestroyed(event);
		CHECK_QSTATUS();
		++it;
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * Folder
 *
 */

qm::Folder::Folder(const Init& init, QSTATUS* pstatus)
{
	assert(pstatus);
	
	*pstatus = QSTATUS_SUCCESS;
	
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrName(allocWString(init.pwszName_));
	if (!wstrName.get()) {
		*pstatus = QSTATUS_OUTOFMEMORY;
		return;
	}
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	pImpl_->pThis_ = this;
	pImpl_->nId_ = init.nId_;
	pImpl_->wstrName_ = wstrName.release();
	pImpl_->cSeparator_ = init.cSeparator_;
	pImpl_->nFlags_ = init.nFlags_;
	pImpl_->pParentFolder_ = init.pParentFolder_;
	pImpl_->pAccount_ = init.pAccount_;
//#ifndef NDEBUG
//	pImpl_->nLock_ = 0;
//#endif
	pImpl_->bDestroyed_ = false;
}

qm::Folder::~Folder()
{
	if (pImpl_) {
		freeWString(pImpl_->wstrName_);
		delete pImpl_;
		pImpl_ = 0;
	}
}

unsigned int qm::Folder::getId() const
{
	return pImpl_->nId_;
}

const WCHAR* qm::Folder::getName() const
{
	return pImpl_->wstrName_;
}

QSTATUS qm::Folder::getDisplayName(WSTRING* pwstrName) const
{
	assert(pwstrName);
	
	*pwstrName = allocWString(pImpl_->wstrName_);
	if (!*pwstrName)
		return QSTATUS_OUTOFMEMORY;
	return QSTATUS_SUCCESS;
}

QSTATUS qm::Folder::getFullName(WSTRING* pwstrName) const
{
	assert(pwstrName);
	
	DECLARE_QSTATUS();
	
	StringBuffer<WSTRING> fullName(pImpl_->wstrName_, &status);
	CHECK_QSTATUS();
	Folder* pParentFolder = pImpl_->pParentFolder_;
	while (pParentFolder) {
		status = fullName.insert(0, pParentFolder->pImpl_->cSeparator_);
		CHECK_QSTATUS();
		status = fullName.insert(0, pParentFolder->pImpl_->wstrName_);
		CHECK_QSTATUS();
		pParentFolder = pParentFolder->pImpl_->pParentFolder_;
	}
	*pwstrName = fullName.getString();
	
	return QSTATUS_SUCCESS;
}

WCHAR qm::Folder::getSeparator() const
{
	return pImpl_->cSeparator_;
}

unsigned int qm::Folder::getFlags() const
{
	return pImpl_->nFlags_;
}

bool qm::Folder::isFlag(Flag flag) const
{
	return (pImpl_->nFlags_ & flag) != 0;
}

void qm::Folder::setFlags(unsigned int nFlags, unsigned int nMask)
{
	pImpl_->nFlags_ &= ~nMask;
	pImpl_->nFlags_ |= nFlags & nMask;
}

Folder* qm::Folder::getParentFolder() const
{
	return pImpl_->pParentFolder_;
}

bool qm::Folder::isAncestorOf(const Folder* pFolder) const
{
	const Folder* pParent = pFolder->getParentFolder();
	while (pParent) {
		if (pParent == this)
			return true;
		pParent = pParent->getParentFolder();
	}
	return false;
}

bool qm::Folder::isHidden() const
{
	const Folder* p = this;
	while (p) {
		if (p->isFlag(Folder::FLAG_HIDE))
			return true;
		p = p->getParentFolder();
	}
	return false;
}

unsigned int qm::Folder::getLevel() const
{
	unsigned int nLevel = 0;
	
	const Folder* pFolder = getParentFolder();
	while (pFolder) {
		pFolder = pFolder->getParentFolder();
		++nLevel;
	}
	
	return nLevel;
}

Account* qm::Folder::getAccount() const
{
	return pImpl_->pAccount_;
}

QSTATUS qm::Folder::addFolderHandler(FolderHandler* pHandler)
{
	assert(std::find(pImpl_->listFolderHandler_.begin(),
		pImpl_->listFolderHandler_.end(), pHandler) ==
		pImpl_->listFolderHandler_.end());
	return STLWrapper<FolderImpl::FolderHandlerList>(
		pImpl_->listFolderHandler_).push_back(pHandler);
}

QSTATUS qm::Folder::removeFolderHandler(FolderHandler* pHandler)
{
	FolderImpl::FolderHandlerList& l = pImpl_->listFolderHandler_;
	FolderImpl::FolderHandlerList::iterator it = std::remove(
		l.begin(), l.end(), pHandler);
	assert(it != l.end());
	l.erase(it, l.end());
	return QSTATUS_SUCCESS;
}

QSTATUS qm::Folder::setName(const WCHAR* pwszName)
{
	string_ptr<WSTRING> wstrName(allocWString(pwszName));
	if (!wstrName.get())
		return QSTATUS_OUTOFMEMORY;
	freeWString(pImpl_->wstrName_);
	pImpl_->wstrName_ = wstrName.release();
	return QSTATUS_SUCCESS;
}

FolderImpl* qm::Folder::getImpl() const
{
	return pImpl_;
}


/****************************************************************************
 *
 * NormalFolderImpl
 *
 */

struct qm::NormalFolderImpl
{
public:
	QSTATUS getPath(WSTRING* pwstrPath) const;
	QSTATUS unstoreMessages(const MessageHolderList& l);
	QSTATUS unstoreAllMessages();

public:
	NormalFolder* pThis_;
	unsigned int nValidity_;
	unsigned int nCount_;
	unsigned int nUnseenCount_;
	unsigned int nDownloadCount_;
	unsigned int nDeletedCount_;
	unsigned int nLastSyncTime_;
	MessageHolderList listMessageHolder_;
	bool bLoad_;
};

QSTATUS qm::NormalFolderImpl::getPath(WSTRING* pwstrPath) const
{
	DECLARE_QSTATUS();
	
	WCHAR wsz[32];
	swprintf(wsz, L"\\%03x%s", pThis_->getId(), Extensions::MSGLIST);
	*pwstrPath = concat(pThis_->getAccount()->getPath(), wsz);
	
	return *pwstrPath ? QSTATUS_SUCCESS : QSTATUS_OUTOFMEMORY;
}

QSTATUS qm::NormalFolderImpl::unstoreMessages(const MessageHolderList& l)
{
	DECLARE_QSTATUS();
	
	MessageHolderList::const_iterator it = l.begin();
	while (it != l.end()) {
		status = pThis_->getAccount()->unstoreMessage(*it);
		CHECK_QSTATUS();
		++it;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::NormalFolderImpl::unstoreAllMessages()
{
	DECLARE_QSTATUS();
	
	MessageHolderList l;
	status = STLWrapper<MessageHolderList>(l).resize(listMessageHolder_.size());
	CHECK_QSTATUS();
	std::copy(listMessageHolder_.begin(), listMessageHolder_.end(), l.begin());
	
	status = unstoreMessages(l);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * NormalFolder
 *
 */

qm::NormalFolder::NormalFolder(const Init& init, QSTATUS* pstatus) :
	Folder(init, pstatus),
	pImpl_(0)
{
	if (*pstatus != QSTATUS_SUCCESS)
		return;
	
	DECLARE_QSTATUS();
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	pImpl_->pThis_ = this;
	pImpl_->nValidity_ = init.nValidity_;
	pImpl_->nCount_ = init.nCount_;
	pImpl_->nUnseenCount_ = init.nUnseenCount_;
	pImpl_->nDownloadCount_ = init.nDownloadCount_;
	pImpl_->nDeletedCount_ = init.nDeletedCount_;
	pImpl_->nLastSyncTime_ = 0;
	pImpl_->bLoad_ = false;
}

qm::NormalFolder::~NormalFolder()
{
	if (pImpl_) {
		std::for_each(pImpl_->listMessageHolder_.begin(),
			pImpl_->listMessageHolder_.end(), deleter<MessageHolder>());
		delete pImpl_;
		pImpl_ = 0;
	}
}

unsigned int qm::NormalFolder::getValidity() const
{
	Lock<Account> lock(*getAccount());
	return pImpl_->nValidity_;
}

QSTATUS qm::NormalFolder::setValidity(unsigned int nValidity)
{
	DECLARE_QSTATUS();
	
	Lock<Account> lock(*getAccount());
	
	status = pImpl_->unstoreAllMessages();
	CHECK_QSTATUS();
	
	pImpl_->nValidity_ = nValidity;
	
	return QSTATUS_SUCCESS;
}

unsigned int qm::NormalFolder::getDownloadCount() const
{
	Lock<Account> lock(*getAccount());
	return pImpl_->nDownloadCount_;
}

unsigned int qm::NormalFolder::getDeletedCount() const
{
	Lock<Account> lock(*getAccount());
	return pImpl_->nDeletedCount_;
}

unsigned int qm::NormalFolder::getLastSyncTime() const
{
	Lock<Account> lock(*getAccount());
	return pImpl_->nLastSyncTime_;
}

void qm::NormalFolder::setLastSyncTime(unsigned int nTime)
{
	Lock<Account> lock(*getAccount());
	pImpl_->nLastSyncTime_ = nTime;
}

QSTATUS qm::NormalFolder::getMessageById(unsigned int nId, MessagePtr* pptr)
{
	assert(pptr);
	
	DECLARE_QSTATUS();
	
	Lock<Account> lock(*getAccount());
	
	status = loadMessageHolders();
	CHECK_QSTATUS();
	
	*pptr = MessagePtr(getMessageById(nId));
	
	return QSTATUS_SUCCESS;
}

MessageHolder* qm::NormalFolder::getMessageById(unsigned int nId) const
{
	assert(pImpl_->bLoad_);
	assert(getAccount()->isLocked());
	
	DECLARE_QSTATUS();
	
	MessageHolder::Init init = { nId };
	MessageHolder mh(0, init, &status);
	CHECK_QSTATUS_VALUE(0);
	MessageHolderList::const_iterator it = std::lower_bound(
		pImpl_->listMessageHolder_.begin(),
		pImpl_->listMessageHolder_.end(),
		&mh,
		binary_compose_f_gx_hy(
			std::less<unsigned int>(),
			std::mem_fun(&MessageHolder::getId),
			std::mem_fun(&MessageHolder::getId)));
	return it != pImpl_->listMessageHolder_.end() && (*it)->getId() == nId ? *it : 0;
}

QSTATUS qm::NormalFolder::updateMessageFlags(
	const FlagList& listFlag, bool* pbClear)
{
	assert(pbClear);
	
	DECLARE_QSTATUS();
	
	*pbClear = false;
	
	Lock<Account> lock(*getAccount());
	
	status = loadMessageHolders();
	CHECK_QSTATUS();
	
	unsigned int nMask = MessageHolder::FLAG_SEEN |
		MessageHolder::FLAG_REPLIED | MessageHolder::FLAG_DRAFT |
		MessageHolder::FLAG_DELETED | MessageHolder::FLAG_MARKED;
	
	MessageHolderList listRemove;
	STLWrapper<MessageHolderList> wrapper(listRemove);
	
	MessageHolderList& l = pImpl_->listMessageHolder_;
	MessageHolderList::iterator itM = l.begin();
	FlagList::const_iterator itF = listFlag.begin();
	while (itM != l.end() && itF != listFlag.end()) {
		if ((*itM)->getId() == (*itF).first) {
			(*itM)->setFlags((*itF).second, nMask);
			++itF;
		}
		else {
			status = wrapper.push_back(*itM);
			CHECK_QSTATUS();
		}
		++itM;
	}
	if (itM != l.end()) {
		status = wrapper.reserve(listRemove.size() + (l.end() - itM));
		CHECK_QSTATUS();
		std::copy(itM, l.end(), std::back_inserter(listRemove));
	}
	
	if (itF != listFlag.end()) {
		// Server may have a bug
		// Clear all
		status = pImpl_->unstoreAllMessages();
		CHECK_QSTATUS();
		*pbClear = true;
	}
	else {
		status = pImpl_->unstoreMessages(listRemove);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::NormalFolder::removeAllMessages(MessageOperationCallback* pCallback)
{
	DECLARE_QSTATUS();
	
	Lock<Account> lock(*getAccount());
	
	status = loadMessageHolders();
	CHECK_QSTATUS();
	
	if (pImpl_->listMessageHolder_.empty())
		return QSTATUS_SUCCESS;
	
	MessageHolderList l;
	status = STLWrapper<MessageHolderList>(l).resize(
		pImpl_->listMessageHolder_.size());
	CHECK_QSTATUS();
	std::copy(pImpl_->listMessageHolder_.begin(),
		pImpl_->listMessageHolder_.end(), l.begin());
	
	status = getAccount()->removeMessages(l, false, pCallback);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

Folder::Type qm::NormalFolder::getType() const
{
	return TYPE_NORMAL;
}

unsigned int qm::NormalFolder::getCount() const
{
	Lock<Account> lock(*getAccount());
	if (pImpl_->bLoad_)
		return pImpl_->listMessageHolder_.size();
	else
		return pImpl_->nCount_;
}

unsigned int qm::NormalFolder::getUnseenCount() const
{
	Lock<Account> lock(*getAccount());
	return pImpl_->nUnseenCount_;
}

QSTATUS qm::NormalFolder::getSize(unsigned int* pnSize)
{
	assert(pnSize);
	
	DECLARE_QSTATUS();
	
	*pnSize = 0;
	
	Lock<Account> lock(*getAccount());
	
	status = loadMessageHolders();
	CHECK_QSTATUS();
	
	MessageHolderList::const_iterator it = pImpl_->listMessageHolder_.begin();
	while (it != pImpl_->listMessageHolder_.end()) {
		*pnSize += (*it)->getSize();
		++it;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::NormalFolder::getBoxSize(unsigned int* pnSize)
{
	assert(pnSize);
	
	DECLARE_QSTATUS();
	
	*pnSize = 0;
	
	Lock<Account> lock(*getAccount());
	
	status = loadMessageHolders();
	CHECK_QSTATUS();
	
	MessageHolderList::const_iterator it = pImpl_->listMessageHolder_.begin();
	while (it != pImpl_->listMessageHolder_.end()) {
		*pnSize += (*it)->getMessageBoxKey().nLength_;
		++it;
	}
	
	return QSTATUS_SUCCESS;
}

MessageHolder* qm::NormalFolder::getMessage(unsigned int n) const
{
	assert(pImpl_->bLoad_);
	assert(getAccount()->isLocked());
	assert(n < pImpl_->listMessageHolder_.size());
	return pImpl_->listMessageHolder_[n];
}

QSTATUS qm::NormalFolder::loadMessageHolders()
{
	DECLARE_QSTATUS();
	
	Lock<Account> lock(*getAccount());
	
	if (pImpl_->bLoad_)
		return QSTATUS_SUCCESS;
	
	pImpl_->nUnseenCount_ = 0;
	pImpl_->nDownloadCount_ = 0;
	pImpl_->nDeletedCount_ = 0;
	
	string_ptr<WSTRING> wstrPath;
	status = pImpl_->getPath(&wstrPath);
	CHECK_QSTATUS();
	
	W2T(wstrPath.get(), ptszPath);
	if (::GetFileAttributes(ptszPath) != 0xffffffff) {
		FileInputStream fileStream(wstrPath.get(), &status);
		CHECK_QSTATUS();
		BufferedInputStream stream(&fileStream, false, &status);
		CHECK_QSTATUS();
		
		STLWrapper<MessageHolderList> wrapper(pImpl_->listMessageHolder_);
		MessageHolder::Init init;
		size_t nRead = 0;
		while (true) {
			status = stream.read(reinterpret_cast<unsigned char*>(&init),
				sizeof(init), &nRead);
			if (nRead == static_cast<size_t>(-1))
				break;
			if (nRead != sizeof(init))
				return QSTATUS_FAIL;
			
			MessageHolder* pmh = 0;
			status = newQsObject(this, init, &pmh);
			CHECK_QSTATUS();
			status = wrapper.push_back(pmh);
			CHECK_QSTATUS();
			
			if (!pmh->isFlag(MessageHolder::FLAG_SEEN))
				++pImpl_->nUnseenCount_;
			if (pmh->isFlag(MessageHolder::FLAG_DOWNLOAD) ||
				pmh->isFlag(MessageHolder::FLAG_DOWNLOADTEXT))
				++pImpl_->nDownloadCount_;
			if (pmh->isFlag(MessageHolder::FLAG_DELETED))
				++pImpl_->nDeletedCount_;
		}
	}
	
	pImpl_->bLoad_ = true;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::NormalFolder::saveMessageHolders()
{
	DECLARE_QSTATUS();
	
	Lock<Account> lock(*getAccount());
	
	if (!pImpl_->bLoad_)
		return QSTATUS_SUCCESS;
	
	string_ptr<WSTRING> wstrPath;
	status = pImpl_->getPath(&wstrPath);
	CHECK_QSTATUS();
	
	TemporaryFileRenamer renamer(wstrPath.get(), &status);
	CHECK_QSTATUS();
	
	FileOutputStream fileStream(renamer.getPath(), &status);
	CHECK_QSTATUS();
	BufferedOutputStream stream(&fileStream, false, &status);
	CHECK_QSTATUS();
	
	MessageHolder::Init init;
	const MessageHolderList& l = pImpl_->listMessageHolder_;
	MessageHolderList::const_iterator it = l.begin();
	while (it != l.end()) {
		(*it)->getInit(&init);
		status = stream.write(reinterpret_cast<unsigned char*>(&init), sizeof(init));
		CHECK_QSTATUS();
		++it;
	}
	status = stream.close();
	CHECK_QSTATUS();
	
	status = renamer.rename();
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::NormalFolder::deletePermanent()
{
	DECLARE_QSTATUS();
	
	getImpl()->bDestroyed_ = true;
	
	status = pImpl_->unstoreAllMessages();
	CHECK_QSTATUS();
	
	string_ptr<WSTRING> wstrPath;
	status = pImpl_->getPath(&wstrPath);
	CHECK_QSTATUS();
	W2T(wstrPath.get(), ptszPath);
	::DeleteFile(ptszPath);
	
	status = getImpl()->fireFolderDestroyed();
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::NormalFolder::generateId(unsigned int* pnId)
{
	assert(pnId);
	
	DECLARE_QSTATUS();
	
	Lock<Account> lock(*getAccount());
	status = loadMessageHolders();
	CHECK_QSTATUS();
	
	const MessageHolderList& l = pImpl_->listMessageHolder_;
	
#ifndef NDEBUG
	unsigned int nMaxId = 0;
	MessageHolderList::const_iterator it = l.begin();
	while (it != l.end()) {
		assert(nMaxId < (*it)->getId());
		nMaxId = (*it)->getId();
		++it;
	}
#endif
	
	if (l.empty())
		*pnId = 1;
	else
		*pnId = l.back()->getId() + 1;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::NormalFolder::appendMessage(MessageHolder* pmh)
{
	assert(pmh);
	assert(pmh->getFolder() == this);
	assert(getAccount()->isLocked());
	
	DECLARE_QSTATUS();
	
	status = loadMessageHolders();
	CHECK_QSTATUS();
	
	assert(pImpl_->listMessageHolder_.empty() ||
		pImpl_->listMessageHolder_.back()->getId() < pmh->getId());
	
	status = STLWrapper<MessageHolderList>(
		pImpl_->listMessageHolder_).push_back(pmh);
	CHECK_QSTATUS();
	
	if (!pmh->isFlag(MessageHolder::FLAG_SEEN))
		++pImpl_->nUnseenCount_;
	if (pmh->isFlag(MessageHolder::FLAG_DOWNLOAD) ||
		pmh->isFlag(MessageHolder::FLAG_DOWNLOADTEXT))
		++pImpl_->nDownloadCount_;
	if (pmh->isFlag(MessageHolder::FLAG_DELETED))
		++pImpl_->nDeletedCount_;
	
	status = getImpl()->fireMessageAdded(pmh);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::NormalFolder::removeMessage(MessageHolder* pmh)
{
	assert(pmh);
	assert(pmh->getFolder() == this);
	assert(pImpl_->bLoad_);
	assert(getAccount()->isLocked());
	
	DECLARE_QSTATUS();
	
	MessageHolderList::iterator it = std::lower_bound(
		pImpl_->listMessageHolder_.begin(), pImpl_->listMessageHolder_.end(), pmh,
		binary_compose_f_gx_hy(
			std::less<unsigned int>(),
			std::mem_fun(&MessageHolder::getId),
			std::mem_fun(&MessageHolder::getId)));
	assert(it != pImpl_->listMessageHolder_.end() && *it == pmh);
	pImpl_->listMessageHolder_.erase(it);
	
	if (!pmh->isFlag(MessageHolder::FLAG_SEEN))
		--pImpl_->nUnseenCount_;
	if (pmh->isFlag(MessageHolder::FLAG_DOWNLOAD) ||
		pmh->isFlag(MessageHolder::FLAG_DOWNLOADTEXT))
		--pImpl_->nDownloadCount_;
	if (pmh->isFlag(MessageHolder::FLAG_DELETED))
		--pImpl_->nDeletedCount_;
	
	std::auto_ptr<MessageHolder> p(pmh);
	status = getImpl()->fireMessageRemoved(pmh);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::NormalFolder::moveMessages(
	const MessageHolderList& l, NormalFolder* pFolder)
{
	assert(!l.empty());
	assert(pFolder);
	assert(pFolder != this);
	assert(pImpl_->bLoad_);
	assert(getAccount()->isLocked());
	
	DECLARE_QSTATUS();
	
	MessageHolderList& listFrom = pImpl_->listMessageHolder_;
	MessageHolderList& listTo = pFolder->pImpl_->listMessageHolder_;
	status = STLWrapper<MessageHolderList>(
		listTo).reserve(listTo.size() + l.size());
	CHECK_QSTATUS();
	
	MessageHolderList::const_iterator it = l.begin();
	while (it != l.end()) {
		MessageHolder* pmh = *it;
		assert(pmh->getFolder() == this);
		
		unsigned int nId = 0;
		status = pFolder->generateId(&nId);
		
		MessageHolderList::iterator itM = std::lower_bound(
			listFrom.begin(), listFrom.end(), pmh,
			binary_compose_f_gx_hy(
				std::less<unsigned int>(),
				std::mem_fun(&MessageHolder::getId),
				std::mem_fun(&MessageHolder::getId)));
		assert(itM != listFrom.end() && *itM == pmh);
		listFrom.erase(itM);
		pmh->setFolder(pFolder);
		pmh->setId(nId);
		listTo.push_back(pmh);
		
		if (!pmh->isFlag(MessageHolder::FLAG_SEEN)) {
			--pImpl_->nUnseenCount_;
			++pFolder->pImpl_->nUnseenCount_;
		}
		if (pmh->isFlag(MessageHolder::FLAG_DOWNLOAD) ||
			pmh->isFlag(MessageHolder::FLAG_DOWNLOADTEXT)) {
			--pImpl_->nDownloadCount_;
			++pFolder->pImpl_->nDownloadCount_;
		}
		if (pmh->isFlag(MessageHolder::FLAG_DELETED)) {
			--pImpl_->nDeletedCount_;
			++pFolder->pImpl_->nDeletedCount_;
		}
		
		status = getImpl()->fireMessageRemoved(pmh);
		CHECK_QSTATUS();
		status = pFolder->getImpl()->fireMessageAdded(pmh);
		CHECK_QSTATUS();
		
		++it;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::NormalFolder::fireMessageFlagChanged(MessageHolder* pmh,
	unsigned int nOldFlags, unsigned int nNewFlags)
{
	assert(pmh);
	assert(nOldFlags != nNewFlags);
	
	DECLARE_QSTATUS();
	
	if (!(nOldFlags & MessageHolder::FLAG_SEEN) &&
		(nNewFlags & MessageHolder::FLAG_SEEN))
		--pImpl_->nUnseenCount_;
	else if ((nOldFlags & MessageHolder::FLAG_SEEN) &&
		!(nNewFlags & MessageHolder::FLAG_SEEN))
		++pImpl_->nUnseenCount_;
	if ((!(nOldFlags & MessageHolder::FLAG_DOWNLOAD) &&
		!(nOldFlags & MessageHolder::FLAG_DOWNLOADTEXT)) &&
		((nNewFlags & MessageHolder::FLAG_DOWNLOAD) ||
		(nNewFlags & MessageHolder::FLAG_DOWNLOADTEXT)))
		++pImpl_->nDownloadCount_;
	else if (((nOldFlags & MessageHolder::FLAG_DOWNLOAD) ||
		(nOldFlags & MessageHolder::FLAG_DOWNLOADTEXT)) &&
		(!(nNewFlags & MessageHolder::FLAG_DOWNLOAD) &&
		!(nNewFlags & MessageHolder::FLAG_DOWNLOADTEXT)))
		--pImpl_->nDownloadCount_;
	if (!(nOldFlags & MessageHolder::FLAG_DELETED) &&
		(nNewFlags & MessageHolder::FLAG_DELETED))
		++pImpl_->nDeletedCount_;
	else if ((nOldFlags & MessageHolder::FLAG_DELETED) &&
		!(nNewFlags & MessageHolder::FLAG_DELETED))
		--pImpl_->nDeletedCount_;
	
	MessageEvent event(this, pmh, nOldFlags, nNewFlags);
	
	FolderImpl::FolderHandlerList& l = getImpl()->listFolderHandler_;
	FolderImpl::FolderHandlerList::iterator it = l.begin();
	while (it != l.end()) {
		status = (*it)->messageChanged(event);
		CHECK_QSTATUS();
		++it;
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * QueryFolderImpl
 *
 */

struct qm::QueryFolderImpl
{
	WSTRING wstrCondition_;
};


/****************************************************************************
 *
 * QueryFolder
 *
 */

qm::QueryFolder::QueryFolder(const Init& init, QSTATUS* pstatus) :
	Folder(init, pstatus),
	pImpl_(0)
{
	DECLARE_QSTATUS();
	
	if (*pstatus != QSTATUS_SUCCESS)
		return;
	
	string_ptr<WSTRING> wstrCondition(allocWString(init.pwszCondition_));
	if (!wstrCondition.get()) {
		*pstatus = QSTATUS_OUTOFMEMORY;
		return;
	}
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	pImpl_->wstrCondition_ = wstrCondition.release();
}

qm::QueryFolder::~QueryFolder()
{
	if (pImpl_) {
		freeWString(pImpl_->wstrCondition_);
		delete pImpl_;
	}
}

const WCHAR* qm::QueryFolder::getCondition() const
{
	return pImpl_->wstrCondition_;
}

Folder::Type qm::QueryFolder::getType() const
{
	return TYPE_QUERY;
}

unsigned int qm::QueryFolder::getCount() const
{
	// TODO
	return 0;
}

unsigned int qm::QueryFolder::getUnseenCount() const
{
	// TODO
	return 0;
}

QSTATUS qm::QueryFolder::getSize(unsigned int* pnSize)
{
	*pnSize = 0;
	// TODO
	return QSTATUS_SUCCESS;
}

QSTATUS qm::QueryFolder::getBoxSize(unsigned int* pnSize)
{
	*pnSize = 0;
	// TODO
	return QSTATUS_SUCCESS;
}

MessageHolder* qm::QueryFolder::getMessage(unsigned int n) const
{
	// TODO
	return 0;
}

QSTATUS qm::QueryFolder::loadMessageHolders()
{
	// TODO
	return QSTATUS_SUCCESS;
}

QSTATUS qm::QueryFolder::saveMessageHolders()
{
	// TODO
	return QSTATUS_SUCCESS;
}

QSTATUS qm::QueryFolder::deletePermanent()
{
	// TODO
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * FolderLess
 *
 */

bool qm::FolderLess::operator()(
	const Folder* pFolderLhs, const Folder* pFolderRhs) const
{
	return compare(pFolderLhs, pFolderRhs) < 0;
}

int qm::FolderLess::compare(const Folder* pFolderLhs, const Folder* pFolderRhs)
{
	DECLARE_QSTATUS();
	
	FolderPath pathLhs;
	status = getFolderPath(pFolderLhs, &pathLhs);
	CHECK_QSTATUS_VALUE(false);
	FolderPath pathRhs;
	status = getFolderPath(pFolderRhs, &pathRhs);
	CHECK_QSTATUS_VALUE(false);
	
	FolderPath::size_type nSize = QSMIN(pathLhs.size(), pathRhs.size());
	for (FolderPath::size_type n = 0; n < nSize; ++n) {
		int nComp = compareSingle(pathLhs[n], pathRhs[n]);
		if (nComp != 0)
			return nComp;
	}
	return pathLhs.size() == pathRhs.size() ? 0 :
		pathLhs.size() < pathRhs.size() ? -1 : 1;
}

int qm::FolderLess::compareSingle(
	const Folder* pFolderLhs, const Folder* pFolderRhs)
{
	assert(pFolderLhs);
	assert(pFolderRhs);
	assert(pFolderLhs->getLevel() == pFolderRhs->getLevel());
	
	if (pFolderLhs->isFlag(Folder::FLAG_INBOX)) {
		if (!pFolderRhs->isFlag(Folder::FLAG_INBOX))
			return -1;
	}
	else if (pFolderRhs->isFlag(Folder::FLAG_INBOX)) {
		return 1;
	}
	else if (pFolderLhs->isFlag(Folder::FLAG_OUTBOX)) {
		if (!pFolderRhs->isFlag(Folder::FLAG_OUTBOX))
			return -1;
	}
	else if (pFolderRhs->isFlag(Folder::FLAG_OUTBOX)) {
		return 1;
	}
	else if (pFolderLhs->isFlag(Folder::FLAG_DRAFTBOX)) {
		if (!pFolderRhs->isFlag(Folder::FLAG_DRAFTBOX))
			return -1;
	}
	else if (pFolderRhs->isFlag(Folder::FLAG_DRAFTBOX)) {
		return 1;
	}
	else if (pFolderLhs->isFlag(Folder::FLAG_SENTBOX)) {
		if (!pFolderRhs->isFlag(Folder::FLAG_SENTBOX))
			return -1;
	}
	else if (pFolderRhs->isFlag(Folder::FLAG_SENTBOX)) {
		return 1;
	}
	else if (pFolderLhs->isFlag(Folder::FLAG_TRASHBOX)) {
		if (!pFolderRhs->isFlag(Folder::FLAG_TRASHBOX))
			return 1;
	}
	else if (pFolderRhs->isFlag(Folder::FLAG_TRASHBOX)) {
		return -1;
	}
	
	return _wcsicmp(pFolderLhs->getName(), pFolderRhs->getName());
}

QSTATUS qm::FolderLess::getFolderPath(const Folder* pFolder, FolderPath* pPath)
{
	assert(pFolder);
	assert(pPath);
	
	DECLARE_QSTATUS();
	
	while (pFolder) {
		status = STLWrapper<FolderPath>(*pPath).push_back(pFolder);
		CHECK_QSTATUS();
		pFolder = pFolder->getParentFolder();
	}
	std::reverse(pPath->begin(), pPath->end());
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * FolderHandler
 *
 */

qm::FolderHandler::~FolderHandler()
{
}


/****************************************************************************
 *
 * FolderEvent
 *
 */

qm::FolderEvent::FolderEvent(Folder* pFolder, MessageHolder* pmh) :
	pFolder_(pFolder),
	pmh_(pmh)
{
}

qm::FolderEvent::~FolderEvent()
{
}

Folder* qm::FolderEvent::getFolder() const
{
	return pFolder_;
}

MessageHolder* qm::FolderEvent::getMessageHolder() const
{
	return pmh_;
}


/****************************************************************************
 *
 * MessageEvent
 *
 */

qm::MessageEvent::MessageEvent(NormalFolder* pFolder, MessageHolder* pmh,
	unsigned int nOldFlags, unsigned int nNewFlags) :
	pFolder_(pFolder),
	pmh_(pmh),
	nOldFlags_(nOldFlags),
	nNewFlags_(nNewFlags)
{
}

qm::MessageEvent::~MessageEvent()
{
}

NormalFolder* qm::MessageEvent::getFolder() const
{
	return pFolder_;
}

MessageHolder* qm::MessageEvent::getMessageHolder() const
{
	return pmh_;
}

unsigned int qm::MessageEvent::getOldFlags() const
{
	return nOldFlags_;
}

unsigned int qm::MessageEvent::getNewFlags() const
{
	return nNewFlags_;
}
