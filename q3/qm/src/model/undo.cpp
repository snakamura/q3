/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmdocument.h>

#include <qsstl.h>

#include <algorithm>
#include <functional>

#include "undo.h"
#include "uri.h"
#include "../util/util.h"

#pragma warning(disable:4786)

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * UndoManager
 *
 */

qm::UndoManager::UndoManager()
{
}

qm::UndoManager::~UndoManager()
{
}

void qm::UndoManager::pushUndoItem(std::auto_ptr<UndoItem> pUndoItem)
{
	if (!pUndoItem.get())
		clear();
	else
		pUndoItem_ = pUndoItem;
}

std::auto_ptr<UndoItem> qm::UndoManager::popUndoItem()
{
	return pUndoItem_;
}

bool qm::UndoManager::hasUndoItem() const
{
	return pUndoItem_.get() != 0;
}

void qm::UndoManager::clear()
{
	pUndoItem_.reset(0);
}


/****************************************************************************
 *
 * UndoItemList
 *
 */

qm::UndoItemList::UndoItemList()
{
}

qm::UndoItemList::~UndoItemList()
{
	std::for_each(listItem_.begin(), listItem_.end(), qs::deleter<UndoItem>());
}

void qm::UndoItemList::add(std::auto_ptr<UndoItem> pItem)
{
	listItem_.push_back(pItem.get());
	pItem.release();
}

std::auto_ptr<UndoItem> qm::UndoItemList::getUndoItem()
{
	std::auto_ptr<UndoItem> pItem;
	if (std::find(listItem_.begin(), listItem_.end(), static_cast<UndoItem*>(0)) == listItem_.end()) {
		if (listItem_.size() == 1) {
			pItem.reset(listItem_.front());
			listItem_.clear();
		}
		else if (!listItem_.empty()) {
			pItem.reset(new GroupUndoItem(listItem_));
		}
		assert(listItem_.empty());
	}
	return pItem;
}


/****************************************************************************
 *
 * UndoItem
 *
 */

qm::UndoItem::~UndoItem()
{
}


/****************************************************************************
 *
 * UndoExecutor
 *
 */

qm::UndoExecutor::~UndoExecutor()
{
}


/****************************************************************************
 *
 * EmptyUndoItem
 *
 */

qm::EmptyUndoItem::EmptyUndoItem()
{
}

qm::EmptyUndoItem::~EmptyUndoItem()
{
}

std::auto_ptr<UndoExecutor> qm::EmptyUndoItem::getExecutor(Document* pDocument)
{
	return std::auto_ptr<UndoExecutor>(new EmptyUndoExecutor());
}


/****************************************************************************
 *
 * EmptyUndoExecutor
 *
 */

qm::EmptyUndoExecutor::EmptyUndoExecutor()
{
}

qm::EmptyUndoExecutor::~EmptyUndoExecutor()
{
}

bool qm::EmptyUndoExecutor::execute()
{
	return true;
}


/****************************************************************************
 *
 * AbstractUndoExecutor
 *
 */

qm::AbstractUndoExecutor::AbstractUndoExecutor(Account* pAccount) :
	pAccount_(pAccount)
{
	assert(pAccount_->isLocked());
	pAccount_->lock();
}

qm::AbstractUndoExecutor::~AbstractUndoExecutor()
{
	pAccount_->unlock();
}

bool qm::AbstractUndoExecutor::execute()
{
	return execute(pAccount_);
}


/****************************************************************************
 *
 * SetFlagsUndoItem
 *
 */

qm::SetFlagsUndoItem::SetFlagsUndoItem()
{
#ifndef NDEBUG
	pFolder_ = 0;
#endif
}

qm::SetFlagsUndoItem::~SetFlagsUndoItem()
{
	std::for_each(listItem_.begin(), listItem_.end(),
		unary_compose_f_gx(
			qs::deleter<URI>(),
			mem_data_ref(&Item::pURI_)));
}

void qm::SetFlagsUndoItem::add(MessageHolder* pmh,
							   unsigned int nFlags,
							   unsigned int nMask)
{
#ifndef NDEBUG
	if (!pFolder_)
		pFolder_ = pmh->getFolder();
	assert(pmh->getFolder() == pFolder_);
#endif
	std::auto_ptr<URI> pURI(new URI(pmh));
	Item item = {
		pURI.get(),
		nFlags,
		nMask
	};
	listItem_.push_back(item);
	pURI.release();
	
	if (!wstrAccount_.get())
		wstrAccount_ = Util::formatAccount(pmh->getAccount());
}

std::auto_ptr<UndoExecutor> qm::SetFlagsUndoItem::getExecutor(Document* pDocument)
{
	if (listItem_.empty())
		return std::auto_ptr<UndoExecutor>(new EmptyUndoExecutor());
	
	Account* pAccount = Util::getAccountOrFolder(pDocument, wstrAccount_.get()).first;
	if (!pAccount)
		return std::auto_ptr<UndoExecutor>();
	Lock<Account> lock(*pAccount);
	
	std::auto_ptr<SetFlagsUndoExecutor> pExecutor(new SetFlagsUndoExecutor(pAccount));
	for (ItemList::const_iterator it = listItem_.begin(); it != listItem_.end(); ++it) {
		const Item& item = *it;
		MessagePtrLock mpl(pDocument->getMessage(*item.pURI_));
		if (!mpl)
			return std::auto_ptr<UndoExecutor>();
		pExecutor->add(mpl, item.nFlags_, item.nMask_);
	}
	return pExecutor;
}


/****************************************************************************
 *
 * SetFlagsUndoExecutor
 *
 */

qm::SetFlagsUndoExecutor::SetFlagsUndoExecutor(Account* pAccount) :
	AbstractUndoExecutor(pAccount)
{
}

qm::SetFlagsUndoExecutor::~SetFlagsUndoExecutor()
{
}

void qm::SetFlagsUndoExecutor::add(MessageHolder* pmh,
								   unsigned int nFlags,
								   unsigned int nMask)
{
	Item item = {
		pmh,
		nFlags,
		nMask
	};
	listItem_.push_back(item);
}

bool qm::SetFlagsUndoExecutor::execute(Account* pAccount)
{
	for (ItemList::const_iterator it = listItem_.begin(); it != listItem_.end(); ++it) {
		const Item& item = *it;
		assert(item.pmh_->getAccount() == pAccount);
		if (!pAccount->setMessagesFlags(MessageHolderList(1, item.pmh_),
			item.nFlags_, item.nMask_, 0))
			return false;
	}
	return true;
}


/****************************************************************************
 *
 * MessageListUndoItem
 *
 */

qm::MessageListUndoItem::MessageListUndoItem(const MessageHolderList& l)
{
	listURI_.reserve(l.size());
	
	NormalFolder* pFolder = 0;
	for (MessageHolderList::const_iterator it = l.begin(); it != l.end(); ++it) {
		MessageHolder* pmh = *it;
		
		if (!pFolder)
			pFolder = pmh->getFolder();
		assert(pmh->getFolder() == pFolder);
		
		listURI_.push_back(new URI(pmh));
	}
	
	if (pFolder)
		wstrFolder_ = Util::formatFolder(pFolder);
}

qm::MessageListUndoItem::~MessageListUndoItem()
{
	std::for_each(listURI_.begin(), listURI_.end(), qs::deleter<URI>());
}

std::auto_ptr<UndoExecutor> qm::MessageListUndoItem::getExecutor(Document* pDocument)
{
	if (listURI_.empty())
		return std::auto_ptr<UndoExecutor>(new EmptyUndoExecutor());
	
	NormalFolder* pFolder = getFolder(pDocument, wstrFolder_.get());
	if (!pFolder)
		return std::auto_ptr<UndoExecutor>();
	
	Account* pAccount = pFolder->getAccount();
	Lock<Account> lock(*pAccount);
	MessageHolderList l;
	l.reserve(listURI_.size());
	for (URIList::const_iterator it = listURI_.begin(); it != listURI_.end(); ++it) {
		URI* pURI = *it;
		MessagePtrLock mpl(pDocument->getMessage(*pURI));
		if (!mpl)
			return std::auto_ptr<UndoExecutor>();
		l.push_back(mpl);
	}
	return getExecutor(pDocument, pAccount, pFolder, l);
}

NormalFolder* qm::MessageListUndoItem::getFolder(Document* pDocument,
												 const WCHAR* pwszFolder)
{
	assert(pDocument);
	assert(pwszFolder);
	
	std::pair<Account*, Folder*> p(Util::getAccountOrFolder(pDocument, pwszFolder));
	if (!p.second || p.second->getType() != Folder::TYPE_NORMAL)
		return 0;
	return static_cast<NormalFolder*>(p.second);
}


/****************************************************************************
 *
 * MessageListUndoExecutor
 *
 */

qm::MessageListUndoExecutor::MessageListUndoExecutor(Document* pDocument,
													 Account* pAccount,
													 NormalFolder* pFolder,
													 MessageHolderList& l) :
	AbstractUndoExecutor(pAccount),
	pDocument_(pDocument),
	pFolder_(pFolder)
{
	listMessageHolder_.swap(l);
}

qm::MessageListUndoExecutor::~MessageListUndoExecutor()
{
}

bool qm::MessageListUndoExecutor::execute(Account* pAccount)
{
	return execute(pDocument_, pAccount, pFolder_, listMessageHolder_);
}


/****************************************************************************
 *
 * MoveUndoItem
 *
 */

qm::MoveUndoItem::MoveUndoItem(const MessageHolderList& l,
							   NormalFolder* pFolderTo) :
	MessageListUndoItem(l)
{
	wstrFolderTo_ = Util::formatFolder(pFolderTo);
}

qm::MoveUndoItem::~MoveUndoItem()
{
}

std::auto_ptr<UndoExecutor> qm::MoveUndoItem::getExecutor(Document* pDocument,
														  Account* pAccount,
														  NormalFolder* pFolder,
														  MessageHolderList& l)
{
	NormalFolder* pFolderTo = getFolder(pDocument, wstrFolderTo_.get());
	if (!pFolderTo)
		return std::auto_ptr<UndoExecutor>();
	return std::auto_ptr<UndoExecutor>(new MoveUndoExecutor(
		pDocument, pAccount, pFolder, pFolderTo, l));
}


/****************************************************************************
 *
 * MoveUndoExecutor
 *
 */

qm::MoveUndoExecutor::MoveUndoExecutor(Document* pDocument,
									   Account* pAccount,
									   NormalFolder* pFolderFrom,
									   NormalFolder* pFolderTo,
									   MessageHolderList& l) :
	MessageListUndoExecutor(pDocument, pAccount, pFolderFrom, l),
	pFolderTo_(pFolderTo)
{
}

qm::MoveUndoExecutor::~MoveUndoExecutor()
{
}

bool qm::MoveUndoExecutor::execute(Document* pDocument,
								   Account* pAccount,
								   NormalFolder* pFolder,
								   const MessageHolderList& l)
{
	return pAccount->copyMessages(l, pFolder, pFolderTo_, true, 0, 0);
}


/****************************************************************************
 *
 * DeleteUndoItem
 *
 */

qm::DeleteUndoItem::DeleteUndoItem(const MessageHolderList& l) :
	MessageListUndoItem(l)
{
}

qm::DeleteUndoItem::~DeleteUndoItem()
{
}

std::auto_ptr<UndoExecutor> qm::DeleteUndoItem::getExecutor(Document* pDocument,
															Account* pAccount,
															NormalFolder* pFolder,
															MessageHolderList& l)
{
	return std::auto_ptr<UndoExecutor>(
		new DeleteUndoExecutor(pDocument, pAccount, pFolder, l));
}


/****************************************************************************
 *
 * DeleteUndoExecutor
 *
 */

qm::DeleteUndoExecutor::DeleteUndoExecutor(Document* pDocument,
										   Account* pAccount,
										   NormalFolder* pFolder,
										   MessageHolderList& l) :
	MessageListUndoExecutor(pDocument, pAccount, pFolder, l)
{
}

qm::DeleteUndoExecutor::~DeleteUndoExecutor()
{
}

bool qm::DeleteUndoExecutor::execute(Document* pDocument,
									 Account* pAccount,
									 NormalFolder* pFolder,
									 const MessageHolderList& l)
{
	return pAccount->removeMessages(l, pFolder, true, 0, 0);
}


/****************************************************************************
 *
 * GroupUndoItem
 *
 */

qm::GroupUndoItem::GroupUndoItem(ItemList& listItem)
{
	assert(std::find(listItem.begin(), listItem.end(), static_cast<UndoItem*>(0)) == listItem.end());
	listItem_.swap(listItem);
}

qm::GroupUndoItem::~GroupUndoItem()
{
	std::for_each(listItem_.begin(), listItem_.end(), qs::deleter<UndoItem>());
}

std::auto_ptr<UndoExecutor> qm::GroupUndoItem::getExecutor(Document* pDocument)
{
	std::auto_ptr<GroupUndoExecutor> pExecutor(new GroupUndoExecutor());
	for (ItemList::const_iterator it = listItem_.begin(); it != listItem_.end(); ++it) {
		std::auto_ptr<UndoExecutor> p((*it)->getExecutor(pDocument));
		if (!p.get())
			return std::auto_ptr<UndoExecutor>();
		pExecutor->add(p);
	}
	return pExecutor;
}


/****************************************************************************
 *
 * GroupUndoExecutor
 *
 */

qm::GroupUndoExecutor::GroupUndoExecutor()
{
}

qm::GroupUndoExecutor::~GroupUndoExecutor()
{
	std::for_each(listExecutor_.begin(), listExecutor_.end(), qs::deleter<UndoExecutor>());
}

void qm::GroupUndoExecutor::add(std::auto_ptr<UndoExecutor> pExecutor)
{
	listExecutor_.push_back(pExecutor.get());
	pExecutor.release();
}

bool qm::GroupUndoExecutor::execute()
{
	return std::find_if(listExecutor_.begin(), listExecutor_.end(),
		std::not1(std::mem_fun(&UndoExecutor::execute))) == listExecutor_.end();
}
