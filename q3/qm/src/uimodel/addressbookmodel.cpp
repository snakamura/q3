/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmapplication.h>
#include <qmfilenames.h>

#include "addressbookmodel.h"
#include "../model/addressbook.h"

#pragma warning(disable:4786)

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * AddressBookModel
 *
 */

qm::AddressBookModel::AddressBookModel() :
	nSort_(0),
	bModified_(false)
{
	wstring_ptr wstrPath(Application::getApplication().getProfilePath(FileNames::ADDRESSBOOK_XML));
	pAddressBook_.reset(new AddressBook(wstrPath.get(), 0, false));
	
	initEntries();
	
	setSort(SORT_NAME | SORT_ASCENDING, SORT_COLUMN_MASK | SORT_DIRECTION_MASK);
}

qm::AddressBookModel::~AddressBookModel()
{
}

AddressBook* qm::AddressBookModel::getAddressBook() const
{
	return pAddressBook_.get();
}

unsigned int qm::AddressBookModel::getCount() const
{
	return listEntry_.size();
}

const AddressBookEntry* qm::AddressBookModel::getEntry(unsigned int nItem) const
{
	return listEntry_[nItem];
}

void qm::AddressBookModel::add(std::auto_ptr<AddressBookEntry> pEntry)
{
	AddressBookEntry* p = pEntry.get();
	pAddressBook_->addEntry(pEntry);
	
	EntryList::iterator it = std::lower_bound(listEntry_.begin(),
		listEntry_.end(), p, EntryLess(nSort_));
	unsigned int nItem = it - listEntry_.begin();
	listEntry_.insert(it, p);
	bModified_ = true;
	
	fireItemAdded(nItem);
}

void qm::AddressBookModel::remove(unsigned int nItem)
{
	const AddressBookEntry* pEntry = listEntry_[nItem];
	pAddressBook_->removeEntry(pEntry);
	
	listEntry_.erase(listEntry_.begin() + nItem);
	bModified_ = true;
	
	fireItemRemoved(nItem);
}

void qm::AddressBookModel::edit(unsigned int nItem,
								std::auto_ptr<AddressBookEntry> pEntry)
{
	*listEntry_[nItem] = *pEntry;
	bModified_ = true;
	
	fireItemEdited(nItem);
}

unsigned int qm::AddressBookModel::getSort() const
{
	return nSort_;
}

void qm::AddressBookModel::setSort(unsigned int nSort,
								   unsigned int nMask)
{
	nSort = (nSort_ & ~nMask) | (nSort & nMask);
	
	assert((nSort & SORT_COLUMN_MASK) == SORT_NAME ||
		(nSort & SORT_COLUMN_MASK) == SORT_ADDRESS ||
		(nSort & SORT_COLUMN_MASK) == SORT_COMMENT);
	assert((nSort & SORT_DIRECTION_MASK) == SORT_ASCENDING ||
		(nSort & SORT_DIRECTION_MASK) == SORT_DESCENDING);
	
	if (nSort != nSort_) {
		fireSorting();
		nSort_ = nSort;
		std::stable_sort(listEntry_.begin(), listEntry_.end(), EntryLess(nSort_));
		fireSorted();
	}
}

void qm::AddressBookModel::refresh()
{
	pAddressBook_->reload();
	initEntries();
	std::stable_sort(listEntry_.begin(), listEntry_.end(), EntryLess(nSort_));
	bModified_ = false;
	
	fireRefreshed();
}

bool qm::AddressBookModel::save() const
{
	if (!pAddressBook_->save())
		return false;
	
	bModified_ = false;
	
	fireSaved();
	
	return true;
}

bool qm::AddressBookModel::isModified() const
{
	return bModified_;
}

void qm::AddressBookModel::addAddressBookModelHandler(AddressBookModelHandler* pHandler)
{
	listHandler_.push_back(pHandler);
}

void qm::AddressBookModel::removeAddressBookModelHandler(AddressBookModelHandler* pHandler)
{
	HandlerList::iterator it = std::remove(
		listHandler_.begin(), listHandler_.end(), pHandler);
	listHandler_.erase(it, listHandler_.end());
}

void qm::AddressBookModel::initEntries()
{
	listEntry_.clear();
	
	const AddressBook::EntryList& l = pAddressBook_->getEntries();
	listEntry_.reserve(l.size());
	for (AddressBook::EntryList::const_iterator it = l.begin(); it != l.end(); ++it) {
		AddressBookEntry* pEntry = *it;
		assert(!pEntry->isExternal());
		listEntry_.push_back(pEntry);
	}
}

void qm::AddressBookModel::fireItemAdded(unsigned int nItem) const
{
	AddressBookModelEvent event(this, nItem);
	fireEvent(event, &AddressBookModelHandler::itemAdded);
}

void qm::AddressBookModel::fireItemRemoved(unsigned int nItem) const
{
	AddressBookModelEvent event(this, nItem);
	fireEvent(event, &AddressBookModelHandler::itemRemoved);
}

void qm::AddressBookModel::fireItemEdited(unsigned int nItem) const
{
	AddressBookModelEvent event(this, nItem);
	fireEvent(event, &AddressBookModelHandler::itemEdited);
}

void qm::AddressBookModel::fireRefreshed() const
{
	AddressBookModelEvent event(this);
	fireEvent(event, &AddressBookModelHandler::refreshed);
}

void qm::AddressBookModel::fireSorting() const
{
	AddressBookModelEvent event(this);
	fireEvent(event, &AddressBookModelHandler::sorting);
}

void qm::AddressBookModel::fireSorted() const
{
	AddressBookModelEvent event(this);
	fireEvent(event, &AddressBookModelHandler::sorted);
}

void qm::AddressBookModel::fireSaved() const
{
	AddressBookModelEvent event(this);
	fireEvent(event, &AddressBookModelHandler::saved);
}

void qm::AddressBookModel::fireEvent(const AddressBookModelEvent& event,
									 void (AddressBookModelHandler::*pfn)(const AddressBookModelEvent& event)) const
{
	for (HandlerList::const_iterator it = listHandler_.begin(); it != listHandler_.end(); ++it)
		((*it)->*pfn)(event);
}


/****************************************************************************
 *
 * AddressBookModel::EntryLess
 *
 */

qm::AddressBookModel::EntryLess::EntryLess(unsigned int nSort) :
	nSort_(nSort)
{
}

bool qm::AddressBookModel::EntryLess::operator()(const AddressBookEntry* pLhs,
												 const AddressBookEntry* pRhs) const
{
	int nComp = 0;
	const WCHAR* pwszLhs = getSortKey(pLhs);
	const WCHAR* pwszRhs = getSortKey(pRhs);
	if (!pwszLhs)
		nComp = pwszRhs ? 1 : 0;
	else
		nComp = pwszRhs ? _wcsicmp(pwszLhs, pwszRhs) : -1;
	
	if ((nSort_ & SORT_DIRECTION_MASK) == SORT_DESCENDING)
		nComp = -nComp;
	
	return nComp < 0;
}

const WCHAR* qm::AddressBookModel::EntryLess::getSortKey(const AddressBookEntry* pEntry) const
{
	if ((nSort_ & SORT_COLUMN_MASK) == SORT_NAME) {
		return pEntry->getActualSortKey();
	}
	else {
		const AddressBookEntry::AddressList& l = pEntry->getAddresses();
		if (!l.empty()) {
			const AddressBookAddress* pAddress = l.front();
			switch (nSort_ & SORT_COLUMN_MASK) {
			case SORT_ADDRESS:
				return pAddress->getAddress();
			case SORT_COMMENT:
				return pAddress->getComment();
			}
		}
	}
	return 0;
}


/****************************************************************************
 *
 * AddressBookModelHandler
 *
 */

qm::AddressBookModelHandler::~AddressBookModelHandler()
{
}


/****************************************************************************
 *
 * DefaultAddressBookModelHandler
 *
 */

qm::DefaultAddressBookModelHandler::~DefaultAddressBookModelHandler()
{
}

void qm::DefaultAddressBookModelHandler::itemAdded(const AddressBookModelEvent& event)
{
}

void qm::DefaultAddressBookModelHandler::itemRemoved(const AddressBookModelEvent& event)
{
}

void qm::DefaultAddressBookModelHandler::itemEdited(const AddressBookModelEvent& event)
{
}

void qm::DefaultAddressBookModelHandler::refreshed(const AddressBookModelEvent& event)
{
}

void qm::DefaultAddressBookModelHandler::sorting(const AddressBookModelEvent& event)
{
}

void qm::DefaultAddressBookModelHandler::sorted(const AddressBookModelEvent& event)
{
}

void qm::DefaultAddressBookModelHandler::saved(const AddressBookModelEvent& event)
{
}


/****************************************************************************
 *
 * AddressBookModelEvent
 *
 */

qm::AddressBookModelEvent::AddressBookModelEvent(const AddressBookModel* pModel) :
	pModel_(pModel),
	nItem_(-1)
{
}

qm::AddressBookModelEvent::AddressBookModelEvent(const AddressBookModel* pModel,
												 unsigned int nItem) :
	pModel_(pModel),
	nItem_(nItem)
{
}

qm::AddressBookModelEvent::~AddressBookModelEvent()
{
}

const AddressBookModel* qm::AddressBookModelEvent::getModel() const
{
	return pModel_;
}

unsigned int qm::AddressBookModelEvent::getItem() const
{
	return nItem_;
}
