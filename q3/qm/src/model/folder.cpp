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
#include <qmmessage.h>
#include <qmmessageholder.h>
#include <qmmessageoperation.h>
#include <qmsearch.h>

#include <qsassert.h>
#include <qsconv.h>
#include <qsfile.h>
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
public:
	typedef std::vector<FolderHandler*> FolderHandlerList;

public:
	void getMessages(const MessagePtrList& l,
					MessageHolderList* pList) const;

public:
	void fireMessageAdded(const MessageHolderList& l);
	void fireMessageRemoved(const MessageHolderList& l);
	void fireMessageRefreshed();
	void fireUnseenCountChanged();
	void fireFolderRenamed();
	void fireFolderDestroyed();

public:
	static unsigned int getSize(Folder* pFolder);
	static unsigned int getBoxSize(Folder* pFolder);

public:
	Folder* pThis_;
	unsigned int nId_;
	wstring_ptr wstrName_;
	WCHAR cSeparator_;
	unsigned int nFlags_;
	Folder::ParamList listParam_;
	Folder* pParentFolder_;
	Account* pAccount_;
	
	FolderHandlerList listFolderHandler_;
	
	bool bDestroyed_;
};

void qm::FolderImpl::getMessages(const MessagePtrList& l,
								 MessageHolderList* pList) const
{
	assert(pList);
	assert(pAccount_->isLocked());
	
	pList->reserve(l.size());
	
	for (MessagePtrList::const_iterator it = l.begin(); it != l.end(); ++it) {
		MessagePtrLock mpl(*it);
		if (mpl)
			pList->push_back(mpl);
	}
}

void qm::FolderImpl::fireMessageAdded(const MessageHolderList& l)
{
	assert(pAccount_->isLocked());
	
	FolderMessageEvent event(pThis_, l);
	for (FolderHandlerList::const_iterator it = listFolderHandler_.begin(); it != listFolderHandler_.end(); ++it)
		(*it)->messageAdded(event);
}

void qm::FolderImpl::fireMessageRemoved(const MessageHolderList& l)
{
	assert(pAccount_->isLocked());
	
	if (bDestroyed_)
		return;
	
	FolderMessageEvent event(pThis_, l);
	for (FolderHandlerList::const_iterator it = listFolderHandler_.begin(); it != listFolderHandler_.end(); ++it)
		(*it)->messageRemoved(event);
}

void qm::FolderImpl::fireMessageRefreshed()
{
	assert(pAccount_->isLocked());
	
	FolderEvent event(pThis_);
	for (FolderHandlerList::const_iterator it = listFolderHandler_.begin(); it != listFolderHandler_.end(); ++it)
		(*it)->messageRefreshed(event);
}

void qm::FolderImpl::fireUnseenCountChanged()
{
	assert(pAccount_->isLocked());
	
	FolderEvent event(pThis_);
	for (FolderHandlerList::const_iterator it = listFolderHandler_.begin(); it != listFolderHandler_.end(); ++it)
		(*it)->unseenCountChanged(event);
}

void qm::FolderImpl::fireFolderRenamed()
{
	FolderEvent event(pThis_);
	
	for (FolderHandlerList::const_iterator it = listFolderHandler_.begin(); it != listFolderHandler_.end(); ++it)
		(*it)->folderRenamed(event);
}

void qm::FolderImpl::fireFolderDestroyed()
{
	FolderEvent event(pThis_);
	
	FolderHandlerList l(listFolderHandler_);
	for (FolderHandlerList::const_iterator it = l.begin(); it != l.end(); ++it)
		(*it)->folderDestroyed(event);
}

unsigned int qm::FolderImpl::getSize(Folder* pFolder)
{
	Lock<Account> lock(*pFolder->getAccount());
	
	unsigned int nSize = 0;
	
	const MessageHolderList& l = pFolder->getMessages();
	for (MessageHolderList::const_iterator it = l.begin(); it != l.end(); ++it)
		nSize += (*it)->getSize();
	
	return nSize;
}

unsigned int qm::FolderImpl::getBoxSize(Folder* pFolder)
{
	Lock<Account> lock(*pFolder->getAccount());
	
	unsigned int nSize = 0;
	
	const MessageHolderList& l = pFolder->getMessages();
	for (MessageHolderList::const_iterator it = l.begin(); it != l.end(); ++it)
		nSize += (*it)->getMessageBoxKey().nLength_;
	
	return nSize;
}


/****************************************************************************
 *
 * Folder
 *
 */

qm::Folder::Folder(unsigned int nId,
				   const WCHAR* pwszName,
				   WCHAR cSeparator,
				   unsigned int nFlags,
				   Folder* pParentFolder,
				   Account* pAccount)
{
	wstring_ptr wstrName(allocWString(pwszName));
	
	pImpl_ = new FolderImpl();
	pImpl_->pThis_ = this;
	pImpl_->nId_ = nId;
	pImpl_->wstrName_ = wstrName;
	pImpl_->cSeparator_ = cSeparator;
	pImpl_->nFlags_ = nFlags;
	pImpl_->pParentFolder_ = pParentFolder;
	pImpl_->pAccount_ = pAccount;
	pImpl_->bDestroyed_ = false;
}

qm::Folder::~Folder()
{
	if (pImpl_) {
		std::for_each(pImpl_->listParam_.begin(), pImpl_->listParam_.end(),
			unary_compose_fx_gx(string_free<WSTRING>(), string_free<WSTRING>()));
		
		delete pImpl_;
	}
}

unsigned int qm::Folder::getId() const
{
	return pImpl_->nId_;
}

const WCHAR* qm::Folder::getName() const
{
	return pImpl_->wstrName_.get();
}

wstring_ptr qm::Folder::getDisplayName() const
{
	return allocWString(pImpl_->wstrName_.get());
}

wstring_ptr qm::Folder::getFullName() const
{
	StringBuffer<WSTRING> fullName(pImpl_->wstrName_.get());
	Folder* pParentFolder = pImpl_->pParentFolder_;
	while (pParentFolder) {
		fullName.insert(0, pParentFolder->pImpl_->cSeparator_);
		fullName.insert(0, pParentFolder->pImpl_->wstrName_.get());
		pParentFolder = pParentFolder->pImpl_->pParentFolder_;
	}
	return fullName.getString();
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

const Folder::ParamList& qm::Folder::getParams() const
{
	return pImpl_->listParam_;
}

void qm::Folder::setParams(ParamList& listParam)
{
	assert(pImpl_->listParam_.empty());
	pImpl_->listParam_.swap(listParam);
}

const WCHAR* qm::Folder::getParam(const WCHAR* pwszName) const
{
	ParamList::const_iterator it = std::find_if(
		pImpl_->listParam_.begin(), pImpl_->listParam_.end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				string_equal<WCHAR>(),
				std::select1st<ParamList::value_type>(),
				std::identity<const WCHAR*>()),
			pwszName));
	return it != pImpl_->listParam_.end() ? (*it).second : 0;
}

void qm::Folder::setParam(const WCHAR* pwszName,
						  const WCHAR* pwszValue)
{
	ParamList::iterator it = std::find_if(
		pImpl_->listParam_.begin(), pImpl_->listParam_.end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				string_equal<WCHAR>(),
				std::select1st<ParamList::value_type>(),
				std::identity<const WCHAR*>()),
			pwszName));
	wstring_ptr wstrValue(allocWString(pwszValue));
	if (it != pImpl_->listParam_.end()) {
		freeWString((*it).second);
		(*it).second = wstrValue.release();
	}
	else {
		wstring_ptr wstrName(allocWString(pwszName));
		pImpl_->listParam_.push_back(ParamList::value_type(
			wstrName.get(), wstrValue.get()));
		wstrName.release();
		wstrValue.release();
	}
}

void qm::Folder::removeParam(const WCHAR* pwszName)
{
	ParamList::iterator it = std::find_if(
		pImpl_->listParam_.begin(), pImpl_->listParam_.end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				string_equal<WCHAR>(),
				std::select1st<ParamList::value_type>(),
				std::identity<const WCHAR*>()),
			pwszName));
	if (it != pImpl_->listParam_.end()) {
		freeWString((*it).first);
		freeWString((*it).second);
		pImpl_->listParam_.erase(it);
	}
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

void qm::Folder::addFolderHandler(FolderHandler* pHandler)
{
	Lock<Account> lock(*getAccount());
	
	assert(std::find(pImpl_->listFolderHandler_.begin(),
		pImpl_->listFolderHandler_.end(), pHandler) ==
		pImpl_->listFolderHandler_.end());
	pImpl_->listFolderHandler_.push_back(pHandler);
}

void qm::Folder::removeFolderHandler(FolderHandler* pHandler)
{
	Lock<Account> lock(*getAccount());
	
	FolderImpl::FolderHandlerList::iterator it = std::remove(
		pImpl_->listFolderHandler_.begin(),
		pImpl_->listFolderHandler_.end(), pHandler);
	assert(it != pImpl_->listFolderHandler_.end());
	pImpl_->listFolderHandler_.erase(it, pImpl_->listFolderHandler_.end());
}

void qm::Folder::setName(const WCHAR* pwszName)
{
	pImpl_->wstrName_ = allocWString(pwszName);
	pImpl_->fireFolderRenamed();
}

void qm::Folder::setFlags(unsigned int nFlags,
						  unsigned int nMask)
{
	pImpl_->nFlags_ &= ~nMask;
	pImpl_->nFlags_ |= nFlags & nMask;
}

void qm::Folder::setParentFolder(Folder* pParentFolder)
{
	pImpl_->pParentFolder_ = pParentFolder;
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

struct qm::NormalFolderImpl : public MessageHolderHandler
{
public:
	wstring_ptr getPath() const;
	bool unstoreMessages(const MessageHolderList& l);
	bool unstoreAllMessages();

public:
	virtual void messageHolderChanged(const MessageHolderEvent& event);
	virtual void messageHolderDestroyed(const MessageHolderEvent& event);

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
	unsigned int nMaxId_;
	mutable bool bModified_;
};

wstring_ptr qm::NormalFolderImpl::getPath() const
{
	WCHAR wsz[32];
	_snwprintf(wsz, countof(wsz), L"\\%03d%s", pThis_->getId(), FileNames::INDEX_EXT);
	return concat(pThis_->getAccount()->getPath(), wsz);
}

bool qm::NormalFolderImpl::unstoreMessages(const MessageHolderList& l)
{
	return l.empty() ? true : pThis_->getAccount()->unstoreMessages(l, 0);
}

bool qm::NormalFolderImpl::unstoreAllMessages()
{
	MessageHolderList l(listMessageHolder_);
	return unstoreMessages(l);
}

void qm::NormalFolderImpl::messageHolderChanged(const MessageHolderEvent& event)
{
	MessageHolder* pmh = event.getMessageHolder();
	if (pmh->getFolder() == pThis_) {
		Account* pAccount = pmh->getAccount();
		assert(pAccount == pThis_->getAccount());
		unsigned int nOldFlags = event.getOldFlags();
		unsigned int nNewFlags = event.getNewFlags();
		
		if (nOldFlags != nNewFlags) {
			unsigned int nUnseenCount = nUnseenCount_;
			if (!pAccount->isSeen(nOldFlags) && pAccount->isSeen(nNewFlags))
				--nUnseenCount_;
			else if (pAccount->isSeen(nOldFlags) && !pAccount->isSeen(nNewFlags))
				++nUnseenCount_;
			if ((!(nOldFlags & MessageHolder::FLAG_DOWNLOAD) &&
				!(nOldFlags & MessageHolder::FLAG_DOWNLOADTEXT)) &&
				((nNewFlags & MessageHolder::FLAG_DOWNLOAD) ||
				(nNewFlags & MessageHolder::FLAG_DOWNLOADTEXT)))
				++nDownloadCount_;
			else if (((nOldFlags & MessageHolder::FLAG_DOWNLOAD) ||
				(nOldFlags & MessageHolder::FLAG_DOWNLOADTEXT)) &&
				(!(nNewFlags & MessageHolder::FLAG_DOWNLOAD) &&
				!(nNewFlags & MessageHolder::FLAG_DOWNLOADTEXT)))
				--nDownloadCount_;
			if (!(nOldFlags & MessageHolder::FLAG_DELETED) &&
				(nNewFlags & MessageHolder::FLAG_DELETED))
				++nDeletedCount_;
			else if ((nOldFlags & MessageHolder::FLAG_DELETED) &&
				!(nNewFlags & MessageHolder::FLAG_DELETED))
				--nDeletedCount_;
			
			if (nUnseenCount != nUnseenCount_)
				pThis_->getImpl()->fireUnseenCountChanged();
		}
		
		bModified_ = true;
	}
}

void qm::NormalFolderImpl::messageHolderDestroyed(const MessageHolderEvent& event)
{
}


/****************************************************************************
 *
 * NormalFolder
 *
 */

qm::NormalFolder::NormalFolder(unsigned int nId,
							   const WCHAR* pwszName,
							   WCHAR cSeparator,
							   unsigned int nFlags,
							   unsigned int nCount,
							   unsigned int nUnseenCount,
							   unsigned int nValidity,
							   unsigned int nDownloadCount,
							   unsigned int nDeletedCount,
							   Folder* pParentFolder,
							   Account* pAccount) :
	Folder(nId, pwszName, cSeparator, nFlags, pParentFolder, pAccount),
	pImpl_(0)
{
	pImpl_ = new NormalFolderImpl();
	pImpl_->pThis_ = this;
	pImpl_->nValidity_ = nValidity;
	pImpl_->nCount_ = nCount;
	pImpl_->nUnseenCount_ = nUnseenCount;
	pImpl_->nDownloadCount_ = nDownloadCount;
	pImpl_->nDeletedCount_ = nDeletedCount;
	pImpl_->nLastSyncTime_ = 0;
	pImpl_->bLoad_ = false;
	pImpl_->nMaxId_ = 0;
	pImpl_->bModified_ = false;
	
	getAccount()->addMessageHolderHandler(pImpl_);
}

qm::NormalFolder::~NormalFolder()
{
	if (pImpl_) {
		getAccount()->removeMessageHolderHandler(pImpl_);
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

bool qm::NormalFolder::setValidity(unsigned int nValidity)
{
	Lock<Account> lock(*getAccount());
	
	if (!pImpl_->unstoreAllMessages())
		return false;
	
	pImpl_->nValidity_ = nValidity;
	
	return true;
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

MessagePtr qm::NormalFolder::getMessageById(unsigned int nId)
{
	Lock<Account> lock(*getAccount());
	
	if (!loadMessageHolders()) {
		// TODO
	}
	
	return MessagePtr(getMessageHolderById(nId));
}

MessageHolder* qm::NormalFolder::getMessageHolderById(unsigned int nId) const
{
	assert(pImpl_->bLoad_);
	assert(getAccount()->isLocked());
	
	MessageHolder::Init init = { nId };
	MessageHolder mh(0, init);
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

bool qm::NormalFolder::updateMessageInfos(const MessageInfoList& listMessageInfo,
										  bool* pbClear)
{
	assert(pbClear);
	
	*pbClear = false;
	
	Lock<Account> lock(*getAccount());
	
	if (!loadMessageHolders())
		return false;
	
	const unsigned int nMask = MessageHolder::FLAG_SEEN |
		MessageHolder::FLAG_REPLIED | MessageHolder::FLAG_DRAFT |
		MessageHolder::FLAG_DELETED | MessageHolder::FLAG_MARKED;
	
	MessageHolderList listRemove;
	
	MessageHolderList& l = pImpl_->listMessageHolder_;
	MessageHolderList::iterator itM = l.begin();
	MessageInfoList::const_iterator itI = listMessageInfo.begin();
	while (itM != l.end() && itI != listMessageInfo.end()) {
		if ((*itM)->getId() == (*itI).nId_) {
			(*itM)->setFlags((*itI).nFlags_, nMask);
			if (!(*itM)->setLabel((*itI).wstrLabel_))
				return false;
			++itI;
		}
		else {
			listRemove.push_back(*itM);
		}
		++itM;
	}
	if (itM != l.end()) {
		listRemove.reserve(listRemove.size() + (l.end() - itM));
		std::copy(itM, l.end(), std::back_inserter(listRemove));
	}
	
	if (itI != listMessageInfo.end()) {
		// Server may have a bug
		// Clear all
		if (!pImpl_->unstoreAllMessages())
			return false;
		*pbClear = true;
	}
	else {
		if (!pImpl_->unstoreMessages(listRemove))
			return false;
	}
	
	return true;
}

Folder::Type qm::NormalFolder::getType() const
{
	return TYPE_NORMAL;
}

unsigned int qm::NormalFolder::getCount() const
{
	Lock<Account> lock(*getAccount());
	if (pImpl_->bLoad_)
		return static_cast<unsigned int>(pImpl_->listMessageHolder_.size());
	else
		return pImpl_->nCount_;
}

unsigned int qm::NormalFolder::getUnseenCount() const
{
	Lock<Account> lock(*getAccount());
	return pImpl_->nUnseenCount_;
}

unsigned int qm::NormalFolder::getSize()
{
	return FolderImpl::getSize(this);
}

unsigned int qm::NormalFolder::getBoxSize()
{
	return FolderImpl::getBoxSize(this);
}

MessageHolder* qm::NormalFolder::getMessage(unsigned int n) const
{
	assert(pImpl_->bLoad_);
	assert(getAccount()->isLocked());
	assert(n < pImpl_->listMessageHolder_.size());
	return pImpl_->listMessageHolder_[n];
}

const MessageHolderList& qm::NormalFolder::getMessages()
{
	assert(getAccount()->isLocked());
	
	if (!loadMessageHolders()) {
		// TODO
	}
	
	return pImpl_->listMessageHolder_;
}

bool qm::NormalFolder::loadMessageHolders()
{
	Lock<Account> lock(*getAccount());
	
	if (pImpl_->bLoad_)
		return true;
	
	pImpl_->nUnseenCount_ = 0;
	pImpl_->nDownloadCount_ = 0;
	pImpl_->nDeletedCount_ = 0;
	pImpl_->bModified_ = false;
	
	wstring_ptr wstrPath(pImpl_->getPath());
	
	MessageHolderList& l = pImpl_->listMessageHolder_;
	
	W2T(wstrPath.get(), ptszPath);
	if (::GetFileAttributes(ptszPath) != 0xffffffff) {
		FileInputStream fileStream(wstrPath.get());
		if (!fileStream)
			return false;
		BufferedInputStream stream(&fileStream, false);
		
		MessageHolder::Init init;
		size_t nRead = 0;
		while (true) {
			size_t nRead = stream.read(reinterpret_cast<unsigned char*>(&init), sizeof(init));
			if (nRead == 0)
				break;
			else if (nRead != sizeof(init))
				return false;
			
			std::auto_ptr<MessageHolder> pmh(new MessageHolder(this, init));
			l.push_back(pmh.get());
			MessageHolder* p = pmh.release();
			
			if (!p->isSeen())
				++pImpl_->nUnseenCount_;
			if (p->isFlag(MessageHolder::FLAG_DOWNLOAD) ||
				p->isFlag(MessageHolder::FLAG_DOWNLOADTEXT))
				++pImpl_->nDownloadCount_;
			if (p->isFlag(MessageHolder::FLAG_DELETED))
				++pImpl_->nDeletedCount_;
		}
	}
	
	pImpl_->nMaxId_ = l.empty() ? 0 : l.back()->getId();
	
	pImpl_->bLoad_ = true;
	
	return true;
}

bool qm::NormalFolder::saveMessageHolders()
{
	Lock<Account> lock(*getAccount());
	
	if (!pImpl_->bLoad_ || !pImpl_->bModified_)
		return true;
	
	wstring_ptr wstrPath(pImpl_->getPath());
	
	TemporaryFileRenamer renamer(wstrPath.get());
	
	FileOutputStream fileStream(renamer.getPath());
	if (!fileStream)
		return false;
	BufferedOutputStream stream(&fileStream, false);
	
	MessageHolder::Init init;
	const MessageHolderList& l = pImpl_->listMessageHolder_;
	for (MessageHolderList::const_iterator it = l.begin(); it != l.end(); ++it) {
		(*it)->getInit(&init);
		if (stream.write(reinterpret_cast<unsigned char*>(&init), sizeof(init)) == -1)
			return false;
	}
	if (!stream.close())
		return false;
	
	if (!renamer.rename())
		return false;
	
	pImpl_->bModified_ = false;
	
	return true;
}

bool qm::NormalFolder::deletePermanent()
{
	getImpl()->bDestroyed_ = true;
	
	if (!pImpl_->unstoreAllMessages())
		return false;
	
	wstring_ptr wstrPath(pImpl_->getPath());
	W2T(wstrPath.get(), ptszPath);
	::DeleteFile(ptszPath);
	
	getImpl()->fireFolderDestroyed();
	
	return true;
}

unsigned int qm::NormalFolder::generateId()
{
	Lock<Account> lock(*getAccount());
	if (!loadMessageHolders())
		return -1;
	
	const MessageHolderList& l = pImpl_->listMessageHolder_;
	
#ifndef NDEBUG
	unsigned int nMaxId = 0;
	for (MessageHolderList::const_iterator it = l.begin(); it != l.end(); ++it) {
		assert(nMaxId < (*it)->getId());
		nMaxId = (*it)->getId();
	}
	assert(pImpl_->nMaxId_ >= nMaxId);
#endif
	
	return pImpl_->nMaxId_ + 1;
}

bool qm::NormalFolder::appendMessage(std::auto_ptr<MessageHolder> pmh)
{
	assert(pmh.get());
	assert(pmh->getFolder() == this);
	assert(getAccount()->isLocked());
	
	if (!loadMessageHolders())
		return false;
	
	pImpl_->bModified_ = true;
	
	assert(pImpl_->listMessageHolder_.empty() ||
		pImpl_->listMessageHolder_.back()->getId() < pmh->getId());
	
	pImpl_->listMessageHolder_.push_back(pmh.get());
	MessageHolder* p = pmh.release();
	
	if (!p->isSeen())
		++pImpl_->nUnseenCount_;
	if (p->isFlag(MessageHolder::FLAG_DOWNLOAD) ||
		p->isFlag(MessageHolder::FLAG_DOWNLOADTEXT))
		++pImpl_->nDownloadCount_;
	if (p->isFlag(MessageHolder::FLAG_DELETED))
		++pImpl_->nDeletedCount_;
	
	pImpl_->nMaxId_ = p->getId();
	
	getImpl()->fireMessageAdded(MessageHolderList(1, p));
	
	return true;
}

void qm::NormalFolder::removeMessages(const MessageHolderList& l)
{
	assert(pImpl_->bLoad_);
	assert(getAccount()->isLocked());
	
	pImpl_->bModified_ = true;
	
	for (MessageHolderList::const_iterator it = l.begin(); it != l.end(); ++it) {
		MessageHolder* pmh = *it;
		assert(pmh);
		assert(pmh->getFolder() == this);
		
		MessageHolderList::iterator itD = std::lower_bound(
			pImpl_->listMessageHolder_.begin(), pImpl_->listMessageHolder_.end(), pmh,
			binary_compose_f_gx_hy(
				std::less<unsigned int>(),
				std::mem_fun(&MessageHolder::getId),
				std::mem_fun(&MessageHolder::getId)));
		assert(itD != pImpl_->listMessageHolder_.end() && *itD == pmh);
		pImpl_->listMessageHolder_.erase(itD);
		
		if (!pmh->isSeen())
			--pImpl_->nUnseenCount_;
		if (pmh->isFlag(MessageHolder::FLAG_DOWNLOAD) ||
			pmh->isFlag(MessageHolder::FLAG_DOWNLOADTEXT))
			--pImpl_->nDownloadCount_;
		if (pmh->isFlag(MessageHolder::FLAG_DELETED))
			--pImpl_->nDeletedCount_;
	}
	
	struct Deleter
	{
		Deleter(const MessageHolderList& l) : l_(l) {}
		~Deleter()
		{
			for (MessageHolderList::const_iterator it = l_.begin(); it != l_.end(); ++it) {
				std::auto_ptr<MessageHolder> pmh(*it);
				pmh->destroy();
			}
		}
		const MessageHolderList& l_;
	} deleter(l);
	getImpl()->fireMessageRemoved(l);
}

bool qm::NormalFolder::moveMessages(const MessageHolderList& l,
									NormalFolder* pFolder)
{
	assert(!l.empty());
	assert(pFolder);
	assert(pFolder != this);
	assert(pImpl_->bLoad_);
	assert(getAccount()->isLocked());
	
	if (!pFolder->loadMessageHolders())
		return false;
	
	pImpl_->bModified_ = true;
	pFolder->pImpl_->bModified_ = true;
	
	MessageHolderList& listFrom = pImpl_->listMessageHolder_;
	MessageHolderList& listTo = pFolder->pImpl_->listMessageHolder_;
	listTo.reserve(listTo.size() + l.size());
	
	for (MessageHolderList::const_iterator it = l.begin(); it != l.end(); ++it) {
		MessageHolder* pmh = *it;
		assert(pmh->getFolder() == this);
		
		unsigned int nId = pFolder->generateId();
		if (nId == -1)
			return false;
		
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
		
		if (!pmh->isSeen()) {
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
		
		pFolder->pImpl_->nMaxId_ = pmh->getId();
	}
	
	getImpl()->fireMessageRemoved(l);
	pFolder->getImpl()->fireMessageAdded(l);
	
	return true;
}


/****************************************************************************
 *
 * QueryFolderImpl
 *
 */

struct qm::QueryFolderImpl : public MessageHolderHandler
{
public:
	virtual void messageHolderChanged(const MessageHolderEvent& event);
	virtual void messageHolderDestroyed(const MessageHolderEvent& event);

public:
	QueryFolder* pThis_;
	wstring_ptr wstrDriver_;
	wstring_ptr wstrCondition_;
	wstring_ptr wstrTargetFolder_;
	bool bRecursive_;
	MessageHolderList listMessageHolder_;
	unsigned int nUnseenCount_;
};

void qm::QueryFolderImpl::messageHolderChanged(const MessageHolderEvent& event)
{
	MessageHolder* pmh = event.getMessageHolder();
	MessageHolderList::const_iterator it = std::lower_bound(
		listMessageHolder_.begin(), listMessageHolder_.end(), pmh);
	if (it != listMessageHolder_.end() && *it == pmh) {
		Account* pAccount = pmh->getAccount();
		assert(pAccount == pThis_->getAccount());
		unsigned int nOldFlags = event.getOldFlags();
		unsigned int nNewFlags = event.getNewFlags();
		
		unsigned int nUnseenCount = nUnseenCount_;
		if (!pAccount->isSeen(nOldFlags) && pAccount->isSeen(nNewFlags))
			--nUnseenCount_;
		else if (pAccount->isSeen(nOldFlags) && !pAccount->isSeen(nNewFlags))
			++nUnseenCount_;
		
		if (nUnseenCount != nUnseenCount_)
			pThis_->getImpl()->fireUnseenCountChanged();
	}
}

void qm::QueryFolderImpl::messageHolderDestroyed(const MessageHolderEvent& event)
{
	MessageHolder* pmh = event.getMessageHolder();
	MessageHolderList::iterator it = std::lower_bound(
		listMessageHolder_.begin(), listMessageHolder_.end(), pmh);
	if (it != listMessageHolder_.end() && *it == pmh) {
		if (!pmh->isSeen())
			--nUnseenCount_;
		listMessageHolder_.erase(it);
		pThis_->getImpl()->fireMessageRemoved(MessageHolderList(1, pmh));
	}
}


/****************************************************************************
 *
 * QueryFolder
 *
 */

qm::QueryFolder::QueryFolder(unsigned int nId,
							 const WCHAR* pwszName,
							 WCHAR cSeparator,
							 unsigned int nFlags,
							 unsigned int nCount,
							 unsigned int nUseenCount,
							 const WCHAR* pwszDriver,
							 const WCHAR* pwszCondition,
							 const WCHAR* pwszTargetFolder,
							 bool bRecursive,
							 Folder* pParentFolder,
							 Account* pAccount) :
	Folder(nId, pwszName, cSeparator, nFlags, pParentFolder, pAccount),
	pImpl_(0)
{
	pImpl_ = new QueryFolderImpl();
	pImpl_->pThis_ = this;
	pImpl_->bRecursive_ = false;
	pImpl_->nUnseenCount_ = 0;
	
	set(pwszDriver, pwszCondition, pwszTargetFolder, bRecursive);
	getAccount()->addMessageHolderHandler(pImpl_);
}

qm::QueryFolder::~QueryFolder()
{
	if (pImpl_) {
		getAccount()->removeMessageHolderHandler(pImpl_);
		delete pImpl_;
	}
}

const WCHAR* qm::QueryFolder::getDriver() const
{
	return pImpl_->wstrDriver_.get();
}

const WCHAR* qm::QueryFolder::getCondition() const
{
	return pImpl_->wstrCondition_.get();
}

const WCHAR* qm::QueryFolder::getTargetFolder() const
{
	return pImpl_->wstrTargetFolder_.get();
}

bool qm::QueryFolder::isRecursive() const
{
	return pImpl_->bRecursive_;
}

void qm::QueryFolder::set(const WCHAR* pwszDriver,
						  const WCHAR* pwszCondition,
						  const WCHAR* pwszTargetFolder,
						  bool bRecursive)
{
	assert(pwszDriver);
	assert(pwszCondition);
	assert(!pwszTargetFolder || *pwszTargetFolder);
	
	pImpl_->wstrDriver_ = allocWString(pwszDriver);
	pImpl_->wstrCondition_ = allocWString(pwszCondition);
	if (pwszTargetFolder)
		pImpl_->wstrTargetFolder_ = allocWString(pwszTargetFolder);
	else
		pImpl_->wstrTargetFolder_.reset(0);
	pImpl_->bRecursive_ = bRecursive;
}

bool qm::QueryFolder::search(Document* pDocument,
							 HWND hwnd,
							 Profile* pProfile,
							 unsigned int nSecurityMode)
{
	Lock<Account> lock(*getAccount());
	
	pImpl_->listMessageHolder_.clear();
	
	std::auto_ptr<SearchDriver> pDriver(SearchDriverFactory::getDriver(
		pImpl_->wstrDriver_.get(), pDocument, getAccount(), hwnd, pProfile));
	if (!pDriver.get())
		return true;
	
	SearchContext context(pImpl_->wstrCondition_.get(),
		pImpl_->wstrTargetFolder_.get(), pImpl_->bRecursive_, nSecurityMode);
	if (!pDriver->search(context, &pImpl_->listMessageHolder_))
		return false;
	std::sort(pImpl_->listMessageHolder_.begin(), pImpl_->listMessageHolder_.end());
	
	bool (MessageHolder::*pfn)() const = &MessageHolder::isSeen;
	pImpl_->nUnseenCount_ = static_cast<unsigned int>(
		std::count_if(
			pImpl_->listMessageHolder_.begin(),
			pImpl_->listMessageHolder_.end(),
			std::not1(std::mem_fun(pfn))));
	
	getImpl()->fireMessageRefreshed();
	
	return true;
}

Folder::Type qm::QueryFolder::getType() const
{
	return TYPE_QUERY;
}

unsigned int qm::QueryFolder::getCount() const
{
	Lock<Account> lock(*getAccount());
	return static_cast<unsigned int>(pImpl_->listMessageHolder_.size());
}

unsigned int qm::QueryFolder::getUnseenCount() const
{
	return pImpl_->nUnseenCount_;
}

unsigned int qm::QueryFolder::getSize()
{
	return FolderImpl::getSize(this);
}

unsigned int qm::QueryFolder::getBoxSize()
{
	return FolderImpl::getBoxSize(this);
}

MessageHolder* qm::QueryFolder::getMessage(unsigned int n) const
{
	assert(getAccount()->isLocked());
	assert(n < pImpl_->listMessageHolder_.size());
	return pImpl_->listMessageHolder_[n];
}

const MessageHolderList& qm::QueryFolder::getMessages()
{
	assert(getAccount()->isLocked());
	
	if (!loadMessageHolders()) {
		// TODO
	}
	
	return pImpl_->listMessageHolder_;
}

bool qm::QueryFolder::loadMessageHolders()
{
	return true;
}

bool qm::QueryFolder::saveMessageHolders()
{
	return true;
}

bool qm::QueryFolder::deletePermanent()
{
	getImpl()->fireFolderDestroyed();
	return true;
}

void qm::QueryFolder::removeMessages(const MessageHolderList& l)
{
	assert(getAccount()->isLocked());
	
	MessageHolderList listRemove(l);
	std::sort(listRemove.begin(), listRemove.end());
	
	MessageHolderList::iterator itS = pImpl_->listMessageHolder_.begin();
	for (MessageHolderList::iterator itR = listRemove.begin(); itR != listRemove.end(); ) {
		MessageHolder* pmh = *itR;
		itS = std::lower_bound(itS, pImpl_->listMessageHolder_.end(), pmh);
		if (itS != pImpl_->listMessageHolder_.end() && *itS == pmh) {
			itS = pImpl_->listMessageHolder_.erase(itS);
			if (!pmh->isSeen())
				--pImpl_->nUnseenCount_;
			++itR;
		}
		else {
			itR = listRemove.erase(itR);
		}
	}
	
	getImpl()->fireMessageRemoved(listRemove);
}


/****************************************************************************
 *
 * FolderLess
 *
 */

bool qm::FolderLess::operator()(const Folder* pFolderLhs,
								const Folder* pFolderRhs) const
{
	return compare(pFolderLhs, pFolderRhs) < 0;
}

int qm::FolderLess::compare(const Folder* pFolderLhs,
							const Folder* pFolderRhs)
{
	FolderPath pathLhs;
	getFolderPath(pFolderLhs, &pathLhs);
	FolderPath pathRhs;
	getFolderPath(pFolderRhs, &pathRhs);
	
	FolderPath::size_type nSize = QSMIN(pathLhs.size(), pathRhs.size());
	for (FolderPath::size_type n = 0; n < nSize; ++n) {
		int nComp = compareSingle(pathLhs[n], pathRhs[n]);
		if (nComp != 0)
			return nComp;
	}
	return pathLhs.size() == pathRhs.size() ? 0 :
		pathLhs.size() < pathRhs.size() ? -1 : 1;
}

int qm::FolderLess::compareSingle(const Folder* pFolderLhs,
								  const Folder* pFolderRhs)
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
	else if (pFolderLhs->isFlag(Folder::FLAG_SEARCHBOX)) {
		if (!pFolderRhs->isFlag(Folder::FLAG_SEARCHBOX))
			return 1;
	}
	else if (pFolderRhs->isFlag(Folder::FLAG_SEARCHBOX)) {
		return -1;
	}
	else if (pFolderLhs->isFlag(Folder::FLAG_JUNKBOX)) {
		if (!pFolderRhs->isFlag(Folder::FLAG_JUNKBOX))
			return 1;
	}
	else if (pFolderRhs->isFlag(Folder::FLAG_JUNKBOX)) {
		return -1;
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

void qm::FolderLess::getFolderPath(const Folder* pFolder,
								   FolderPath* pPath)
{
	assert(pFolder);
	assert(pPath);
	
	while (pFolder) {
		pPath->push_back(pFolder);
		pFolder = pFolder->getParentFolder();
	}
	std::reverse(pPath->begin(), pPath->end());
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
 * DefaultFolderHandler
 *
 */

qm::DefaultFolderHandler::DefaultFolderHandler()
{
}

qm::DefaultFolderHandler::~DefaultFolderHandler()
{
}

void qm::DefaultFolderHandler::messageAdded(const FolderMessageEvent& event)
{
}

void qm::DefaultFolderHandler::messageRemoved(const FolderMessageEvent& event)
{
}

void qm::DefaultFolderHandler::messageRefreshed(const FolderEvent& event)
{
}

void qm::DefaultFolderHandler::unseenCountChanged(const FolderEvent& event)
{
}

void qm::DefaultFolderHandler::folderRenamed(const FolderEvent& event)
{
}

void qm::DefaultFolderHandler::folderDestroyed(const FolderEvent& event)
{
}


/****************************************************************************
 *
 * FolderEvent
 *
 */

qm::FolderEvent::FolderEvent(Folder* pFolder) :
	pFolder_(pFolder)
{
}

qm::FolderEvent::~FolderEvent()
{
}

Folder* qm::FolderEvent::getFolder() const
{
	return pFolder_;
}


/****************************************************************************
 *
 * FolderMessageEvent
 *
 */

qm::FolderMessageEvent::FolderMessageEvent(Folder* pFolder,
										   const MessageHolderList& l) :
	FolderEvent(pFolder),
	l_(l)
{
}

qm::FolderMessageEvent::~FolderMessageEvent()
{
}

const MessageHolderList& qm::FolderMessageEvent::getMessageHolders() const
{
	return l_;
}
