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
#include "../util/confighelper.h"

#pragma warning(disable:4786)

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * TabItem
 *
 */

qm::TabItem::TabItem(Account* pAccount,
					 bool bLocked,
					 const WCHAR* pwszTitle) :
	pAccount_(pAccount),
	pFolder_(0),
	bLocked_(bLocked)
{
	if (pwszTitle)
		wstrTitle_ = allocWString(pwszTitle);
}

qm::TabItem::TabItem(Folder* pFolder,
					 bool bLocked,
					 const WCHAR* pwszTitle) :
	pAccount_(0),
	pFolder_(pFolder),
	bLocked_(bLocked)
{
	if (pwszTitle)
		wstrTitle_ = allocWString(pwszTitle);
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

const WCHAR* qm::TabItem::getTitle() const
{
	return wstrTitle_.get();
}

void qm::TabItem::setTitle(const WCHAR* pwszTitle)
{
	if (pwszTitle)
		wstrTitle_ = allocWString(pwszTitle);
	else
		wstrTitle_.reset(0);
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
									 Profile* pProfile,
									 const WCHAR* pwszPath) :
	pDocument_(pDocument),
	pProfile_(pProfile),
	nCurrent_(-1),
	nTemporary_(-1),
	nReuse_(REUSE_NONE)
{
	wstrPath_ = allocWString(pwszPath);
	nReuse_ = pProfile_->getInt(L"TabWindow", L"Reuse", REUSE_NONE);
	
	pDocument_->addDocumentHandler(this);
}

qm::DefaultTabModel::~DefaultTabModel()
{
	std::for_each(listItem_.begin(), listItem_.end(), qs::deleter<TabItem>());
	
	pDocument_->removeDocumentHandler(this);
}

bool qm::DefaultTabModel::save() const
{
	if (!ConfigSaver<const DefaultTabModel*, TabModelWriter>::save(this, wstrPath_.get()))
		return false;
	
	pProfile_->setInt(L"TabWindow", L"CurrentTab", nCurrent_);
	
	return true;
}

const DefaultTabModel::ItemList& qm::DefaultTabModel::getItems() const
{
	return listItem_;
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

const TabItem* qm::DefaultTabModel::getItem(int nItem)
{
	assert(0 <= nItem && nItem < static_cast<int>(listItem_.size()));
	return listItem_[nItem];
}

void qm::DefaultTabModel::setLocked(int nItem,
									bool bLocked)
{
	TabItem* pItem = listItem_[nItem];
	pItem->setLocked(bLocked);
	fireItemChanged(nItem, pItem, pItem);
}

void qm::DefaultTabModel::setTitle(int nItem,
								   const WCHAR* pwszTitle)
{
	TabItem* pItem = listItem_[nItem];
	pItem->setTitle(pwszTitle);
	fireItemChanged(nItem, pItem, pItem);
}

void qm::DefaultTabModel::open(Account* pAccount)
{
	open(pAccount, true);
}

void qm::DefaultTabModel::open(Folder* pFolder)
{
	open(pFolder, true);
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
		open(pAccount, false);
	}
	else {
		int nItem = -1;
		if (nReuse_ & REUSE_CHANGE)
			nItem = getItem(pAccount);
		if (nItem != -1) {
			setCurrent(nItem);
		}
		else {
			assert(listItemOrder_.front() == listItem_[nCurrent_]);
			setAccount(nCurrent_, pAccount);
			assert(listItemOrder_.front() == listItem_[nCurrent_]);
		}
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
		open(pFolder, false);
	}
	else {
		int nItem = -1;
		if (nReuse_ & REUSE_CHANGE)
			nItem = getItem(pFolder);
		if (nItem != -1) {
			setCurrent(nItem);
		}
		else {
			assert(listItemOrder_.front() == listItem_[nCurrent_]);
			setFolder(nCurrent_, pFolder);
			assert(listItemOrder_.front() == listItem_[nCurrent_]);
		}
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
	XMLReader reader;
	TabModelContentHandler handler(this, pDocument_);
	reader.setContentHandler(&handler);
	if (!reader.parse(wstrPath_.get())) {
		// TODO
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

void qm::DefaultTabModel::open(Account* pAccount,
							   bool bForce)
{
	int nItem = -1;
	if (!bForce) {
		nItem = getItem(pAccount);
		if (nItem == -1 && nReuse_ & REUSE_OPEN) {
			nItem = getReusableItem();
			if (nItem != -1)
				setAccount(nItem, pAccount);
		}
	}
	if (nItem == -1)
		nItem = addAccount(pAccount, false, 0);
	setCurrent(nItem);
}

void qm::DefaultTabModel::open(Folder* pFolder,
							   bool bForce)
{
	int nItem = -1;
	if (!bForce) {
		nItem = getItem(pFolder);
		if (nItem == -1 && nReuse_ & REUSE_OPEN) {
			nItem = getReusableItem();
			if (nItem != -1)
				setFolder(nItem, pFolder);
		}
	}
	if (nItem == -1)
		nItem = addFolder(pFolder, false, 0);
	setCurrent(nItem);
}

int qm::DefaultTabModel::addAccount(Account* pAccount,
									bool bLocked,
									const WCHAR* pwszTitle)
{
	std::auto_ptr<TabItem> pItem(new TabItem(pAccount, bLocked, pwszTitle));
	listItem_.push_back(pItem.get());
	listItemOrder_.push_back(pItem.get());
	resetHandlers(0, 0, pAccount, 0);
	int nItem = listItem_.size() - 1;
	fireItemAdded(nItem, pItem.release());
	return nItem;
}

int qm::DefaultTabModel::addFolder(Folder* pFolder,
								   bool bLocked,
								   const WCHAR* pwszTitle)
{
	std::auto_ptr<TabItem> pItem(new TabItem(pFolder, bLocked, pwszTitle));
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

void qm::DefaultTabModel::setAccount(int nItem,
									 Account* pAccount)
{
	std::auto_ptr<TabItem> pItem(new TabItem(
		pAccount, false, listItem_[nItem]->getTitle()));
	setItem(nItem, pItem);
}

void qm::DefaultTabModel::setFolder(int nItem,
									Folder* pFolder)
{
	std::auto_ptr<TabItem> pItem(new TabItem(
		pFolder, false, listItem_[nItem]->getTitle()));
	setItem(nItem, pItem);
}

void qm::DefaultTabModel::setItem(int nItem,
								  std::auto_ptr<TabItem> pItem)
{
	std::auto_ptr<TabItem> pOldItem(listItem_[nItem]);
	listItem_[nItem] = pItem.get();
	
	ItemList::iterator it = std::find(listItemOrder_.begin(),
		listItemOrder_.end(), pOldItem.get());
	assert(it != listItemOrder_.end());
	*it = pItem.get();
	
	std::pair<Account*, Folder*> pOld(pOldItem->get());
	std::pair<Account*, Folder*> pNew(pItem->get());
	resetHandlers(pOld.first, pOld.second, pNew.first, pNew.second);
	fireItemChanged(nItem, pOldItem.get(), pItem.release());
}

int qm::DefaultTabModel::getItem(Account* pAccount) const
{
	ItemList::const_iterator it = std::find_if(
		listItem_.begin(), listItem_.end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				std::equal_to<Account*>(),
				unary_compose_f_gx(
					std::select1st<std::pair<Account*, Folder*> >(),
					std::mem_fun(&TabItem::get)),
				std::identity<Account*>()),
			pAccount));
	return it != listItem_.end() ? it - listItem_.begin() : -1;
}

int qm::DefaultTabModel::getItem(Folder* pFolder) const
{
	ItemList::const_iterator it = std::find_if(
		listItem_.begin(), listItem_.end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				std::equal_to<Folder*>(),
				unary_compose_f_gx(
					std::select2nd<std::pair<Account*, Folder*> >(),
					std::mem_fun(&TabItem::get)),
				std::identity<Folder*>()),
			pFolder));
	return it != listItem_.end() ? it - listItem_.begin() : -1;
}

int qm::DefaultTabModel::getReusableItem() const
{
	ItemList::const_reverse_iterator it = std::find_if(
		listItem_.rbegin(), listItem_.rend(),
		std::not1(std::mem_fun(&TabItem::isLocked)));
	if (it == listItem_.rend())
		return -1;
	return it.base() - listItem_.begin() - 1;
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
								 const TabItem* pOldItem,
								 const TabItem* pNewItem) :
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

const TabItem* qm::TabModelEvent::getOldItem() const
{
	return pOldItem_;
}

const TabItem* qm::TabModelEvent::getNewItem() const
{
	return pNewItem_;
}

int qm::TabModelEvent::getAmount() const
{
	return nAmount_;
}


/****************************************************************************
 *
 * TabModelContentHandler
 *
 */

qm::TabModelContentHandler::TabModelContentHandler(DefaultTabModel* pTabModel,
												   Document* pDocument) :
	pTabModel_(pTabModel),
	pDocument_(pDocument),
	state_(STATE_ROOT)
{
}

qm::TabModelContentHandler::~TabModelContentHandler()
{
}

bool qm::TabModelContentHandler::startElement(const WCHAR* pwszNamespaceURI,
											  const WCHAR* pwszLocalName,
											  const WCHAR* pwszQName,
											  const qs::Attributes& attributes)
{
	if (wcscmp(pwszLocalName, L"tabs") == 0) {
		if (state_ != STATE_ROOT)
			return false;
		if (attributes.getLength() != 0)
			return false;
		state_ = STATE_TABS;
	}
	else if (wcscmp(pwszLocalName, L"tab") == 0) {
		if (state_ != STATE_TABS)
			return false;
		
		const WCHAR* pwszAccount = 0;
		const WCHAR* pwszFolder = 0;
		bool bLocked = false;
		const WCHAR* pwszTitle = 0;
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrName, L"account") == 0)
				pwszAccount = attributes.getValue(n);
			else if (wcscmp(pwszAttrName, L"folder") == 0)
				pwszFolder = attributes.getValue(n);
			else if (wcscmp(pwszAttrName, L"locked") == 0)
				bLocked = wcscmp(attributes.getValue(n), L"true") == 0;
			else if (wcscmp(pwszAttrName, L"title") == 0)
				pwszTitle = attributes.getValue(n);
			else
				return false;
		}
		if (!pwszAccount)
			return false;
		
		Account* pAccount = pDocument_->getAccount(pwszAccount);
		if (pAccount) {
			Folder* pFolder = 0;
			if (pwszFolder)
				pFolder = pAccount->getFolder(pwszFolder);
			
			std::auto_ptr<TabItem> pItem;
			if (pFolder)
				pTabModel_->addFolder(pFolder, bLocked, pwszTitle);
			else
				pTabModel_->addAccount(pAccount, bLocked, pwszTitle);
		}
		
		state_ = STATE_TAB;
	}
	else {
		return false;
	}
	return true;
}

bool qm::TabModelContentHandler::endElement(const WCHAR* pwszNamespaceURI,
											const WCHAR* pwszLocalName,
											const WCHAR* pwszQName)
{
	if (wcscmp(pwszLocalName, L"tabs") == 0) {
		assert(state_ == STATE_TABS);
		state_ = STATE_ROOT;
	}
	else if (wcscmp(pwszLocalName, L"tab") == 0) {
		assert(state_ == STATE_TAB);
		state_ = STATE_TABS;
	}
	else {
		return false;
	}
	return true;
}

bool qm::TabModelContentHandler::characters(const WCHAR* pwsz,
											size_t nStart,
											size_t nLength)
{
	const WCHAR* p = pwsz + nStart;
	for (size_t n = 0; n < nLength; ++n, ++p) {
		if (*p != L' ' && *p != L'\t' && *p != '\n')
			return false;
	}
	return true;
}


/****************************************************************************
 *
 * TabModelWriter
 *
 */

qm::TabModelWriter::TabModelWriter(Writer* pWriter) :
	handler_(pWriter)
{
}

qm::TabModelWriter::~TabModelWriter()
{
}

bool qm::TabModelWriter::write(const DefaultTabModel* pTabModel)
{
	if (!handler_.startDocument())
		return false;
	if (!handler_.startElement(0, 0, L"tabs", DefaultAttributes()))
		return false;
	
	const DefaultTabModel::ItemList& listItem = pTabModel->getItems();
	for (DefaultTabModel::ItemList::const_iterator it = listItem.begin(); it != listItem.end(); ++it) {
		const TabItem* pItem = *it;
		if (!write(pItem))
			return false;
	}
	
	if (!handler_.endElement(0, 0, L"tabs"))
		return false;
	if (!handler_.endDocument())
		return false;
	
	return true;
}

bool qm::TabModelWriter::write(const TabItem* pItem)
{
	std::pair<Account*, Folder*> p(pItem->get());
	
	Account* pAccount = p.first ? p.first : p.second->getAccount();
	const WCHAR* pwszAccount = pAccount->getName();
	wstring_ptr wstrFolder;
	if (p.second)
		wstrFolder = p.second->getFullName();
	bool bLocked = pItem->isLocked();
	const WCHAR* pwszTitle = pItem->getTitle();
	
	const SimpleAttributes::Item items[] = {
		{ L"account",	pwszAccount																},
		{ L"folder",	wstrFolder.get(),				!wstrFolder.get()						},
		{ L"locked",	bLocked ? L"true" : L"false",	!bLocked								},
		{ L"title",		pwszTitle,						!pwszTitle								}
	};
	SimpleAttributes attrs(items, countof(items));
	return handler_.startElement(0, 0, L"tab", attrs) &&
		handler_.endElement(0, 0, L"tab");
}

#endif // QMTABWINDOW
