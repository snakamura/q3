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
		Folder::MessageHolderList* pList) const;
	
	QSTATUS fireMessageAdded(MessageHolder* pmh);
	QSTATUS fireMessageRemoved(MessageHolder* pmh);
	
	Folder* pThis_;
	unsigned int nId_;
	WSTRING wstrName_;
	WCHAR cSeparator_;
	unsigned int nFlags_;
	Folder* pParentFolder_;
	Account* pAccount_;
	
	FolderHandlerList listFolderHandler_;
	
	CriticalSection csLock_;
#ifndef NDEBUG
	unsigned int nLock_;
#endif
};

QSTATUS qm::FolderImpl::getMessages(const MessagePtrList& l,
	Folder::MessageHolderList* pList) const
{
	assert(pList);
	assert(pThis_->isLocked());
	
	DECLARE_QSTATUS();
	
	status = STLWrapper<Folder::MessageHolderList>(
		*pList).reserve(l.size());
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
	
	DECLARE_QSTATUS();
	
	FolderEvent event(pThis_, pmh);
	
	Lock<Folder> lock(*pThis_);
	
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
	
	DECLARE_QSTATUS();
	
	FolderEvent event(pThis_, pmh);
	
	Lock<Folder> lock(*pThis_);
	
	FolderHandlerList::const_iterator it = listFolderHandler_.begin();
	while (it != listFolderHandler_.end()) {
		status = (*it)->messageRemoved(event);
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
#ifndef NDEBUG
	pImpl_->nLock_ = 0;
#endif
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
	Lock<Folder> lock(*this);
	return pImpl_->wstrName_;
}

QSTATUS qm::Folder::getDisplayName(WSTRING* pwstrName) const
{
	assert(pwstrName);
	
	Lock<Folder> lock(*this);
	*pwstrName = allocWString(pImpl_->wstrName_);
	if (!*pwstrName)
		return QSTATUS_OUTOFMEMORY;
	return QSTATUS_SUCCESS;
}

QSTATUS qm::Folder::getFullName(WSTRING* pwstrName) const
{
	assert(pwstrName);
	
	DECLARE_QSTATUS();
	
	Lock<Folder> lock(*this);
	
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
	Lock<Folder> lock(*this);
	return pImpl_->cSeparator_;
}

unsigned int qm::Folder::getFlags() const
{
	Lock<Folder> lock(*this);
	return pImpl_->nFlags_;
}

bool qm::Folder::isFlag(Flag flag) const
{
	Lock<Folder> lock(*this);
	return (pImpl_->nFlags_ & flag) != 0;
}

void qm::Folder::setFlags(unsigned int nFlags, unsigned int nMask)
{
	Lock<Folder> lock(*this);
	
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

QSTATUS qm::Folder::setMessagesFlags(const MessagePtrList& l,
	unsigned int nFlags, unsigned int nMask)
{
	DECLARE_QSTATUS();
	
	Lock<Folder> lock(*this);
	
	MessageHolderList listMessageHolder;
	status = pImpl_->getMessages(l, &listMessageHolder);
	CHECK_QSTATUS();
	
	return setMessagesFlags(listMessageHolder, nFlags, nMask);
}

QSTATUS qm::Folder::copyMessages(const MessagePtrList& l, NormalFolder* pFolder,
	bool bMove, MessageOperationCallback* pCallback)
{
	DECLARE_QSTATUS();
	
	Lock<Folder> lock(*this);
	
	MessageHolderList listMessageHolder;
	status = pImpl_->getMessages(l, &listMessageHolder);
	CHECK_QSTATUS();
	
	return copyMessages(listMessageHolder, pFolder, bMove, pCallback);
}

QSTATUS qm::Folder::removeMessages(const MessagePtrList& l,
	bool bDirect, MessageOperationCallback* pCallback)
{
	DECLARE_QSTATUS();
	
	Lock<Folder> lock(*this);
	
	MessageHolderList listMessageHolder;
	status = pImpl_->getMessages(l, &listMessageHolder);
	CHECK_QSTATUS();
	
	return removeMessages(listMessageHolder, bDirect, pCallback);
}

QSTATUS qm::Folder::addFolderHandler(FolderHandler* pHandler)
{
	Lock<Folder> lock(*this);
	
	assert(std::find(pImpl_->listFolderHandler_.begin(),
		pImpl_->listFolderHandler_.end(), pHandler) ==
		pImpl_->listFolderHandler_.end());
	return STLWrapper<FolderImpl::FolderHandlerList>(
		pImpl_->listFolderHandler_).push_back(pHandler);
}

QSTATUS qm::Folder::removeFolderHandler(FolderHandler* pHandler)
{
	Lock<Folder> lock(*this);
	
	FolderImpl::FolderHandlerList& l = pImpl_->listFolderHandler_;
	FolderImpl::FolderHandlerList::iterator it = std::remove(
		l.begin(), l.end(), pHandler);
	assert(it != l.end());
	l.erase(it, l.end());
	return QSTATUS_SUCCESS;
}

void qm::Folder::lock() const
{
	pImpl_->csLock_.lock();
#ifndef NDEBUG
	++pImpl_->nLock_;
#endif
}

void qm::Folder::unlock() const
{
#ifndef NDEBUG
	--pImpl_->nLock_;
#endif
	pImpl_->csLock_.unlock();
}

#ifndef NDEBUG
bool qm::Folder::isLocked() const
{
	return pImpl_->nLock_ != 0;
}
#endif

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
	typedef Folder::MessageHolderList MessageHolderList;

public:
	QSTATUS getPath(WSTRING* pwstrPath) const;

public:
	NormalFolder* pThis_;
	unsigned int nValidity_;
	unsigned int nCount_;
	unsigned int nUnseenCount_;
	unsigned int nDownloadCount_;
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
	Lock<Folder> lock(*this);
	return pImpl_->nValidity_;
}

QSTATUS qm::NormalFolder::setValidity(unsigned int nValidity)
{
	DECLARE_QSTATUS();
	
	Lock<NormalFolder> lock(*this);
	
	status = deleteAllMessages();
	CHECK_QSTATUS();
	
	pImpl_->nValidity_ = nValidity;
	
	return QSTATUS_SUCCESS;
}

unsigned int qm::NormalFolder::getDownloadCount() const
{
	Lock<Folder> lock(*this);
	return pImpl_->nDownloadCount_;
}

QSTATUS qm::NormalFolder::getMessageById(
	unsigned int nId, MessageHolder** ppmh)
{
	assert(ppmh);
	
	DECLARE_QSTATUS();
	
	*ppmh = 0;
	
	Lock<NormalFolder> lock(*this);
	
	status = loadMessageHolders();
	CHECK_QSTATUS();
	
	*ppmh = getMessageById(nId);
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::NormalFolder::updateMessageFlags(
	const FlagList& listFlag, bool* pbClear)
{
	assert(pbClear);
	
	DECLARE_QSTATUS();
	
	*pbClear = false;
	
	Lock<NormalFolder> lock(*this);
	
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
		status = deleteAllMessages();
		CHECK_QSTATUS();
		*pbClear = true;
	}
	else {
		status = deleteMessages(listRemove, 0);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::NormalFolder::appendMessage(const Message& msg, unsigned int nFlags)
{
	assert(msg.getFlag() != Message::FLAG_EMPTY);
	
	DECLARE_QSTATUS();
	
	string_ptr<STRING> strMessage;
	status = msg.getContent(&strMessage);
	CHECK_QSTATUS();
	
	Lock<Folder> lock(*this);
	
	return getAccount()->appendMessage(this,
		strMessage.get(), msg, nFlags, static_cast<unsigned int>(-1));
}

QSTATUS qm::NormalFolder::removeAllMessages(MessageOperationCallback* pCallback)
{
	DECLARE_QSTATUS();
	
	Lock<Folder> lock(*this);

	status = loadMessageHolders();
	CHECK_QSTATUS();
	
	MessageHolderList l;
	status = STLWrapper<MessageHolderList>(l).resize(
		pImpl_->listMessageHolder_.size());
	CHECK_QSTATUS();
	std::copy(pImpl_->listMessageHolder_.begin(),
		pImpl_->listMessageHolder_.end(), l.begin());
	
	status = getAccount()->removeMessages(this, l, false, pCallback);
	CHECK_QSTATUS();
	
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::NormalFolder::clearDeletedMessages()
{
	return getAccount()->clearDeletedMessages(this);
}

Folder::Type qm::NormalFolder::getType() const
{
	return TYPE_NORMAL;
}

unsigned int qm::NormalFolder::getCount() const
{
	Lock<Folder> lock(*this);
	if (pImpl_->bLoad_)
		return pImpl_->listMessageHolder_.size();
	else
		return pImpl_->nCount_;
}

unsigned int qm::NormalFolder::getUnseenCount() const
{
	Lock<Folder> lock(*this);
	return pImpl_->nUnseenCount_;
}

QSTATUS qm::NormalFolder::getSize(unsigned int* pnSize)
{
	assert(pnSize);
	
	DECLARE_QSTATUS();
	
	*pnSize = 0;
	
	Lock<Folder> lock(*this);
	
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
	
	Lock<Folder> lock(*this);
	
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
	assert(isLocked());
	assert(n < pImpl_->listMessageHolder_.size());
	return pImpl_->listMessageHolder_[n];
}

MessageHolder* qm::NormalFolder::getMessageById(unsigned int nId) const
{
	assert(pImpl_->bLoad_);
	assert(isLocked());
	
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

QSTATUS qm::NormalFolder::setMessagesFlags(const MessageHolderList& l,
	unsigned int nFlags, unsigned int nMask)
{
	assert(isLocked());
	return getAccount()->setMessagesFlags(this, l, nFlags, nMask);
}

QSTATUS qm::NormalFolder::copyMessages(const MessageHolderList& l,
	NormalFolder* pFolder, bool bMove, MessageOperationCallback* pCallback)
{
	assert(isLocked());
	return getAccount()->copyMessages(l, this, pFolder, bMove, pCallback);
}

QSTATUS qm::NormalFolder::removeMessages(const MessageHolderList& l,
	bool bDirect, MessageOperationCallback* pCallback)
{
	assert(isLocked());
	return getAccount()->removeMessages(this, l, bDirect, pCallback);
}

QSTATUS qm::NormalFolder::loadMessageHolders()
{
	DECLARE_QSTATUS();
	
	Lock<Folder> lock(*this);
	
	if (pImpl_->bLoad_)
		return QSTATUS_SUCCESS;
	
	pImpl_->nUnseenCount_ = 0;
	pImpl_->nDownloadCount_ = 0;
	
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
		}
	}
	
	pImpl_->bLoad_ = true;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::NormalFolder::saveMessageHolders()
{
	DECLARE_QSTATUS();
	
	Lock<Folder> lock(*this);
	
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
	// TODO
	return QSTATUS_SUCCESS;
}

QSTATUS qm::NormalFolder::generateId(unsigned int* pnId)
{
	assert(pnId);
	
	DECLARE_QSTATUS();
	
	Lock<Folder> lock(*this);
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
	
	DECLARE_QSTATUS();
	
	Lock<Folder> lock(*this);
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
	
	status = getImpl()->fireMessageAdded(pmh);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::NormalFolder::removeMessage(MessageHolder* pmh)
{
	assert(pmh);
	assert(pmh->getFolder() == this);
	assert(pImpl_->bLoad_);
	
	DECLARE_QSTATUS();
	
	Lock<Folder> lock(*this);
	
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
	
	std::auto_ptr<MessageHolder> p(pmh);
	status = getImpl()->fireMessageRemoved(pmh);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::NormalFolder::deleteMessages(const MessageHolderList& l,
	MessageOperationCallback* pCallback)
{
	assert(&l != &pImpl_->listMessageHolder_);
	
	DECLARE_QSTATUS();
	
	Lock<Folder> lock(*this);
	
	MessageHolderList::const_iterator it = l.begin();
	while (it != l.end()) {
		status = getAccount()->unstoreMessage(*it);
		CHECK_QSTATUS();
		
		if (pCallback) {
			status = pCallback->step(1);
			CHECK_QSTATUS();
		}
		
		++it;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::NormalFolder::deleteAllMessages()
{
	DECLARE_QSTATUS();
	
	Lock<Folder> lock(*this);
	
	MessageHolderList l;
	status = STLWrapper<MessageHolderList>(l).resize(
		pImpl_->listMessageHolder_.size());
	CHECK_QSTATUS();
	std::copy(pImpl_->listMessageHolder_.begin(),
		pImpl_->listMessageHolder_.end(), l.begin());
	
	status = deleteMessages(l, 0);
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
	assert(isLocked());
	
	DECLARE_QSTATUS();
	
	Lock<Folder> lock(*pFolder);
	
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
		
		status = getImpl()->fireMessageRemoved(pmh);
		CHECK_QSTATUS();
		status = pFolder->getImpl()->fireMessageAdded(pmh);
		CHECK_QSTATUS();
		
		++it;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::NormalFolder::getData(MessageCacheKey key,
	MessageCacheItem item, WSTRING* pwstrData) const
{
	return getAccount()->getData(key, item, pwstrData);
}

QSTATUS qm::NormalFolder::getMessage(MessageHolder* pmh,
	unsigned int nFlags, Message* pMessage) const
{
	return getAccount()->getMessage(pmh, nFlags, pMessage);
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
	
	MessageEvent event(this, pmh, nOldFlags, nNewFlags);
	
	Lock<Folder> lock(*this);
	
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
 * QueryFolder
 *
 */

qm::QueryFolder::QueryFolder(const Init& init, QSTATUS* pstatus) :
	Folder(init, pstatus),
	wstrMacro_(0)
{
	if (*pstatus != QSTATUS_SUCCESS)
		return;
	
	wstrMacro_ = allocWString(init.pwszMacro_);
	if (!wstrMacro_) {
		*pstatus = QSTATUS_OUTOFMEMORY;
		return;
	}
}

qm::QueryFolder::~QueryFolder()
{
	freeWString(wstrMacro_);
}

const WCHAR* qm::QueryFolder::getMacro() const
{
	return wstrMacro_;
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

MessageHolder* qm::QueryFolder::getMessageById(unsigned int nId) const
{
	// TODO
	return 0;
}

QSTATUS qm::QueryFolder::setMessagesFlags(const MessageHolderList& l,
	unsigned int nFlags, unsigned int nMask)
{
	// TODO
	return QSTATUS_SUCCESS;
}

QSTATUS qm::QueryFolder::copyMessages(const MessageHolderList& l,
	NormalFolder* pFolder, bool bMove, MessageOperationCallback* pCallback)
{
	// TODO
	return QSTATUS_SUCCESS;
}

QSTATUS qm::QueryFolder::removeMessages(const MessageHolderList& l,
	bool bDirect, MessageOperationCallback* pCallback)
{
	// TODO
	return QSTATUS_SUCCESS;
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
