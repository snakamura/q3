/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifdef QMTABWINDOW

#include <qmaccount.h>
#include <qmdocument.h>

#include <qsassert.h>

#include <algorithm>

#include "tabmodel.h"
#include "uiutil.h"

#pragma warning(disable:4786)

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * TabItem
 *
 */

qm::TabItem::TabItem(Account* pAccount,
					 bool bLocked) :
	pAccount_(pAccount),
	pFolder_(0),
	bLocked_(bLocked)
{
}

qm::TabItem::TabItem(Folder* pFolder,
					 bool bLocked) :
	pAccount_(0),
	pFolder_(pFolder),
	bLocked_(bLocked)
{
}

qm::TabItem::~TabItem()
{
}

std::pair<Account*, Folder*> qm::TabItem::get() const
{
	return std::make_pair(pAccount_, pFolder_);
}

bool qm::TabItem::isLocked() const
{
	return bLocked_;
}

void qm::TabItem::setLocked(bool bLocked)
{
	bLocked_ = bLocked;
}


/****************************************************************************
 *
 * TabModel
 *
 */

qm::TabModel::~TabModel()
{
}


/****************************************************************************
 *
 * DefaultTabModel
 *
 */

qm::DefaultTabModel::DefaultTabModel(Document* pDocument,
									 Profile* pProfile) :
	pDocument_(pDocument),
	pProfile_(pProfile),
	nCurrent_(-1),
	nTemporary_(-1)
{
	pDocument_->addDocumentHandler(this);
}

qm::DefaultTabModel::~DefaultTabModel()
{
	std::for_each(listItem_.begin(), listItem_.end(), qs::deleter<TabItem>());
	
	pDocument_->removeDocumentHandler(this);
}

bool qm::DefaultTabModel::save() const
{
	Profile::StringList listValue;
	StringListFree<Profile::StringList> free(listValue);
	listValue.reserve(listItem_.size());
	for (ItemList::const_iterator it = listItem_.begin(); it != listItem_.end(); ++it) {
		TabItem* pItem = *it;
		std::pair<Account*, Folder*> p(pItem->get());
		wstring_ptr wstr(p.first ? UIUtil::formatAccount(p.first) : UIUtil::formatFolder(p.second));
		
		if (pItem->isLocked())
			wstr = concat(L"+", wstr.get());
		
		listValue.push_back(wstr.release());
	}
	
	pProfile_->setStringList(L"TabWindow", L"Tabs", listValue);
	pProfile_->setInt(L"TabWindow", L"CurrentTab", nCurrent_);
	
	return true;
}

int qm::DefaultTabModel::getCount()
{
	return listItem_.size();
}

int qm::DefaultTabModel::getCurrent()
{
	return nCurrent_;
}

void qm::DefaultTabModel::setCurrent(int nItem)
{
	setCurrent(nItem, false);
}

int qm::DefaultTabModel::getTemporary()
{
	return nTemporary_;
}

void qm::DefaultTabModel::setTemporary(int nItem)
{
	nTemporary_ = nItem;
}

TabItem* qm::DefaultTabModel::getItem(int nItem)
{
	assert(0 <= nItem && nItem < static_cast<int>(listItem_.size()));
	return listItem_[nItem];
}

bool qm::DefaultTabModel::isLocked(int nItem)
{
	return listItem_[nItem]->isLocked();
}

void qm::DefaultTabModel::setLocked(int nItem,
									bool bLocked)
{
	listItem_[nItem]->setLocked(bLocked);
}

void qm::DefaultTabModel::open(Account* pAccount)
{
	int nItem = addAccount(pAccount, false);
	setCurrent(nItem);
}

void qm::DefaultTabModel::open(Folder* pFolder)
{
	int nItem = addFolder(pFolder, false);
	setCurrent(nItem);
}

void qm::DefaultTabModel::close(int nItem)
{
	removeItem(nItem);
}

void qm::DefaultTabModel::moveItem(int nItem,
								   int nAmount)
{
	assert(0 <= nItem + nAmount && nItem + nAmount < static_cast<int>(listItem_.size()));
	
	if (nAmount == 0)
		return;
	
	TabItem* pItem = listItem_[nItem];
	ItemList::iterator it = listItem_.begin() + nItem;
	if (nAmount > 0) {
		it = std::copy(it + 1, it + nAmount + 1, it);
		*it = pItem;
	}
	else {
		it = std::copy_backward(it + nAmount, it, it + 1);
		*(it - 1) = pItem;
	}
	
	if (nItem == nCurrent_) {
		nCurrent_ += nAmount;
	}
	else if (nAmount > 0) {
		if (nItem <= nCurrent_ && nCurrent_ < nItem + nAmount)
			--nCurrent_;
	}
	else {
		if (nItem + nAmount <= nCurrent_ && nCurrent_ < nItem)
			++nCurrent_;
	}
	assert(listItem_[nCurrent_] == listItemOrder_.front());
	
	fireItemMoved(nItem, nAmount);
}

void qm::DefaultTabModel::setAccount(Account* pAccount)
{
	assert(pAccount);
	
	bool bOpen = false;
	if (listItem_.empty()) {
		bOpen = true;
	}
	else {
		TabItem* pItem = listItem_[nCurrent_];
		if (pItem->get().first == pAccount)
			return;
		bOpen = pItem->isLocked();
	}
	
	if (bOpen) {
		open(pAccount);
	}
	else {
		std::auto_ptr<TabItem> pOldItem(listItem_[nCurrent_]);
		std::auto_ptr<TabItem> pNewItem(new TabItem(pAccount, false));
		listItem_[nCurrent_] = pNewItem.get();
		
		assert(listItemOrder_.front() == pOldItem.get());
		listItemOrder_.front() = pNewItem.get();
		
		std::pair<Account*, Folder*> pOld(pOldItem->get());
		std::pair<Account*, Folder*> pNew(pNewItem->get());
		resetHandlers(pOld.first, pOld.second, pNew.first, pNew.second);
		fireItemChanged(nCurrent_, pOldItem.get(), pNewItem.release());
	}
}

void qm::DefaultTabModel::setFolder(Folder* pFolder)
{
	assert(pFolder);
	
	bool bOpen = false;
	if (listItem_.empty()) {
		bOpen = true;
	}
	else {
		TabItem* pItem = listItem_[nCurrent_];
		if (pItem->get().second == pFolder)
			return;
		bOpen = pItem->isLocked();
	}
	
	if (bOpen) {
		open(pFolder);
	}
	else {
		std::auto_ptr<TabItem> pOldItem(listItem_[nCurrent_]);
		std::auto_ptr<TabItem> pNewItem(new TabItem(pFolder, false));
		listItem_[nCurrent_] = pNewItem.get();
		
		assert(listItemOrder_.front() == pOldItem.get());
		listItemOrder_.front() = pNewItem.get();
		
		std::pair<Account*, Folder*> pOld(pOldItem->get());
		std::pair<Account*, Folder*> pNew(pNewItem->get());
		resetHandlers(pOld.first, pOld.second, pNew.first, pNew.second);
		fireItemChanged(nCurrent_, pOldItem.get(), pNewItem.release());
	}
}

void qm::DefaultTabModel::addTabModelHandler(TabModelHandler* pHandler)
{
	listHandler_.push_back(pHandler);
}

void qm::DefaultTabModel::removeTabModelHandler(TabModelHandler* pHandler)
{
	HandlerList::iterator it = std::remove(listHandler_.begin(),
		listHandler_.end(), pHandler);
	listHandler_.erase(it, listHandler_.end());
}

void qm::DefaultTabModel::accountListChanged(const AccountListChangedEvent& event)
{
	const Document::AccountList& listAccount = pDocument_->getAccounts();
	
	for (ItemList::reverse_iterator it = listItem_.rbegin(); it != listItem_.rend(); ++it) {
		TabItem* pItem = *it;
		std::pair<Account*, Folder*> p(pItem->get());
		
		Account* pAccount = p.first ? p.first : p.second ? p.second->getAccount() : 0;
		if (std::find(listAccount.begin(), listAccount.end(), pAccount) == listAccount.end())
			removeItem(it.base() - 1 - listItem_.begin());
	}
}

void qm::DefaultTabModel::documentInitialized(const DocumentEvent& event)
{
	Profile::StringList listFolders;
	StringListFree<Profile::StringList> free(listFolders);
	pProfile_->getStringList(L"TabWindow", L"Tabs", &listFolders);
	for (Profile::StringList::const_iterator it = listFolders.begin(); it != listFolders.end(); ++it) {
		const WCHAR* pwsz = *it;
		
		bool bLocked = false;
		if (*pwsz == L'+') {
			bLocked = true;
			++pwsz;
		}
		
		std::pair<Account*, Folder*> p(UIUtil::getAccountOrFolder(pDocument_, pwsz));
		if (p.first)
			addAccount(p.first, bLocked);
		else if (p.second)
			addFolder(p.second, bLocked);
	}
	
	if (!listItem_.empty()) {
		int nItem = pProfile_->getInt(L"TabWindow", L"CurrentTab", 0);
		if (0 <= nItem && nItem < getCount())
			setCurrent(nItem);
	}
}

void qm::DefaultTabModel::folderListChanged(const FolderListChangedEvent& event)
{
	Account* pAccount = event.getAccount();
	const Account::FolderList& listFolder = pAccount->getFolders();
	
	for (ItemList::reverse_iterator it = listItem_.rbegin(); it != listItem_.rend(); ++it) {
		TabItem* pItem = *it;
		std::pair<Account*, Folder*> p(pItem->get());
		
		if (p.second && p.second->getAccount() == pAccount) {
			if (std::find(listFolder.begin(), listFolder.end(), p.second) == listFolder.end())
				removeItem(it.base() - 1 - listItem_.begin());
		}
	}
}

void qm::DefaultTabModel::fireItemAdded(int nItem,
										TabItem* pItem)
{
	TabModelEvent event(this, nItem, 0, pItem);
	for (HandlerList::const_iterator it = listHandler_.begin(); it != listHandler_.end(); ++it)
		(*it)->itemAdded(event);
}

void qm::DefaultTabModel::fireItemRemoved(int nItem,
										  TabItem* pItem)
{
	TabModelEvent event(this, nItem, pItem, 0);
	for (HandlerList::const_iterator it = listHandler_.begin(); it != listHandler_.end(); ++it)
		(*it)->itemRemoved(event);
}

void qm::DefaultTabModel::fireItemChanged(int nItem,
										  TabItem* pOldItem,
										  TabItem* pNewItem)
{
	TabModelEvent event(this, nItem, pOldItem, pNewItem);
	for (HandlerList::const_iterator it = listHandler_.begin(); it != listHandler_.end(); ++it)
		(*it)->itemChanged(event);
}

void qm::DefaultTabModel::fireItemMoved(int nItem,
										int nAmount)
{
	TabModelEvent event(this, nItem, nAmount);
	for (HandlerList::const_iterator it = listHandler_.begin(); it != listHandler_.end(); ++it)
		(*it)->itemMoved(event);
}

void qm::DefaultTabModel::fireCurrentChanged()
{
	TabModelEvent event(this, nCurrent_);
	for (HandlerList::const_iterator it = listHandler_.begin(); it != listHandler_.end(); ++it)
		(*it)->currentChanged(event);
}

void qm::DefaultTabModel::setCurrent(int nItem,
									 bool bForce)
{
	assert((0 <= nItem && nItem < static_cast<int>(listItem_.size())) || nItem == -1);
	
	if (nItem != nCurrent_ || bForce) {
		nCurrent_ = nItem;
		
		if (nItem != -1) {
			TabItem* pItem = listItem_[nItem];
			ItemList::iterator it = std::find(listItemOrder_.begin(),
				listItemOrder_.end(), pItem);
			assert(it != listItemOrder_.end());
			if (it != listItemOrder_.begin()) {
				std::copy_backward(listItemOrder_.begin(), it, it + 1);
				listItemOrder_.front() = pItem;
			}
		}
		
		fireCurrentChanged();
	}
}

int qm::DefaultTabModel::addAccount(Account* pAccount,
									bool bLocked)
{
	std::auto_ptr<TabItem> pItem(new TabItem(pAccount, bLocked));
	listItem_.push_back(pItem.get());
	listItemOrder_.push_back(pItem.get());
	resetHandlers(0, 0, pAccount, 0);
	int nItem = listItem_.size() - 1;
	fireItemAdded(nItem, pItem.release());
	return nItem;
}

int qm::DefaultTabModel::addFolder(Folder* pFolder,
								   bool bLocked)
{
	std::auto_ptr<TabItem> pItem(new TabItem(pFolder, bLocked));
	listItem_.push_back(pItem.get());
	listItemOrder_.push_back(pItem.get());
	resetHandlers(0, 0, 0, pFolder);
	int nItem = listItem_.size() - 1;
	fireItemAdded(nItem, pItem.release());
	return nItem;
}

void qm::DefaultTabModel::removeItem(int nItem)
{
	assert(0 <= nItem && nItem < static_cast<int>(listItem_.size()));
	
	std::auto_ptr<TabItem> pItem(listItem_[nItem]);
	std::pair<Account*, Folder*> p(pItem->get());
	listItem_.erase(listItem_.begin() + nItem);
	listItemOrder_.erase(std::remove(listItemOrder_.begin(), listItemOrder_.end(), pItem.get()), listItemOrder_.end());
	resetHandlers(p.first, p.second, 0, 0);
	fireItemRemoved(nItem, pItem.get());
	
	int nCurrent = nCurrent_;
	if (!listItem_.empty()) {
		ItemList::const_iterator it = std::find(listItem_.begin(),
			listItem_.end(), listItemOrder_.front());
		assert(it != listItem_.end());
		nCurrent = it - listItem_.begin();
	}
	else {
		nCurrent = -1;
	}
	setCurrent(nCurrent, true);
}

void qm::DefaultTabModel::resetHandlers(Account* pOldAccount,
										Folder* pOldFolder,
										Account* pNewAccount,
										Folder* pNewFolder)
{
	if (!pOldAccount && pOldFolder)
		pOldAccount = pOldFolder->getAccount();
	
	if (pOldAccount) {
		AccountList::iterator it = std::find_if(
			listHandledAccount_.begin(), listHandledAccount_.end(),
			std::bind2nd(
				binary_compose_f_gx_hy(
					std::equal_to<Account*>(),
					std::select1st<AccountList::value_type>(),
					std::identity<Account*>()),
				pOldAccount));
		assert(it != listHandledAccount_.end());
		if (--(*it).second == 0) {
			pOldAccount->removeAccountHandler(this);
			listHandledAccount_.erase(it);
		}
	}
	
	if (!pNewAccount && pNewFolder)
		pNewAccount = pNewFolder->getAccount();
	
	if (pNewAccount) {
		AccountList::iterator it = std::find_if(
			listHandledAccount_.begin(), listHandledAccount_.end(),
			std::bind2nd(
				binary_compose_f_gx_hy(
					std::equal_to<Account*>(),
					std::select1st<AccountList::value_type>(),
					std::identity<Account*>()),
				pNewAccount));
		if (it != listHandledAccount_.end()) {
			++(*it).second;
		}
		else {
			pNewAccount->addAccountHandler(this);
			listHandledAccount_.push_back(std::make_pair(pNewAccount, 1));
		}
	}
}


/****************************************************************************
 *
 * TabModelHandler
 *
 */

qm::TabModelHandler::~TabModelHandler()
{
}


/****************************************************************************
 *
 * DefaultTabModelHandler
 *
 */

qm::DefaultTabModelHandler::DefaultTabModelHandler()
{
}

qm::DefaultTabModelHandler::~DefaultTabModelHandler()
{
}

void qm::DefaultTabModelHandler::itemAdded(const TabModelEvent& event)
{
}

void qm::DefaultTabModelHandler::itemRemoved(const TabModelEvent& event)
{
}

void qm::DefaultTabModelHandler::itemChanged(const TabModelEvent& event)
{
}

void qm::DefaultTabModelHandler::itemMoved(const TabModelEvent& event)
{
}

void qm::DefaultTabModelHandler::currentChanged(const TabModelEvent& event)
{
}


/****************************************************************************
 *
 * TabModelEvent
 *
 */

qm::TabModelEvent::TabModelEvent(TabModel* pTabModel,
								 int nItem) :
	pTabModel_(pTabModel),
	nItem_(nItem),
	pOldItem_(0),
	pNewItem_(0),
	nAmount_(0)
{
}

qm::TabModelEvent::TabModelEvent(TabModel* pTabModel,
								 int nItem,
								 TabItem* pOldItem,
								 TabItem* pNewItem) :
	pTabModel_(pTabModel),
	nItem_(nItem),
	pOldItem_(pOldItem),
	pNewItem_(pNewItem),
	nAmount_(0)
{
}

qm::TabModelEvent::TabModelEvent(TabModel* pTabModel,
								 int nItem,
								 int nAmount) :
	pTabModel_(pTabModel),
	nItem_(nItem),
	pOldItem_(0),
	pNewItem_(0),
	nAmount_(nAmount)
{
}

qm::TabModelEvent::~TabModelEvent()
{
}

TabModel* qm::TabModelEvent::getTabModel() const
{
	return pTabModel_;
}

int qm::TabModelEvent::getItem() const
{
	return nItem_;
}

TabItem* qm::TabModelEvent::getOldItem() const
{
	return pOldItem_;
}

TabItem* qm::TabModelEvent::getNewItem() const
{
	return pNewItem_;
}

int qm::TabModelEvent::getAmount() const
{
	return nAmount_;
}

#endif // QMTABWINDOW
