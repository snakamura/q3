/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmaccount.h>
#include <qmfilenames.h>
#include <qmfolder.h>
#include <qmmacro.h>
#include <qmmessage.h>
#include <qmmessageholder.h>

#include <qsstl.h>

#include <algorithm>

#include "securitymodel.h"
#include "viewmodel.h"
#include "../model/color.h"
#include "../model/filter.h"

#pragma warning(disable:4786)

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * ViewColumn
 *
 */

qm::ViewColumn::ViewColumn(const WCHAR* pwszTitle,
						   Type type,
						   std::auto_ptr<Macro> pMacro,
						   unsigned int nFlags,
						   unsigned int nWidth) :
	type_(type),
	pMacro_(pMacro),
	nFlags_(nFlags),
	nWidth_(nWidth)
{
	wstrTitle_ = allocWString(pwszTitle);
}

qm::ViewColumn::~ViewColumn()
{
}

const WCHAR* qm::ViewColumn::getTitle() const
{
	return wstrTitle_.get();
}

ViewColumn::Type qm::ViewColumn::getType() const
{
	return type_;
}

const Macro* qm::ViewColumn::getMacro() const
{
	return pMacro_.get();
}

unsigned int qm::ViewColumn::getFlags() const
{
	return nFlags_;
}

bool qm::ViewColumn::isFlag(Flag flag) const
{
	return (nFlags_ & flag) != 0;
}

void qm::ViewColumn::setFlags(unsigned int nFlags)
{
	nFlags_ = nFlags;
}

unsigned int qm::ViewColumn::getWidth() const
{
	return nWidth_;
}

void qm::ViewColumn::setWidth(unsigned int nWidth)
{
	nWidth_ = nWidth;
}

wstring_ptr qm::ViewColumn::getText(const ViewModel* pViewModel,
									MessageHolder* pmh) const
{
	assert(pmh);
	
	wstring_ptr wstrText;
	WCHAR wsz[32] = L"";
	switch (type_) {
	case ViewColumn::TYPE_NONE:
		assert(false);
		break;
	case ViewColumn::TYPE_ID:
		swprintf(wsz, L"%d", pmh->getId());
		break;
	case ViewColumn::TYPE_DATE:
		{
			Time t;
			pmh->getDate(&t);
			wstrText = t.format(L"%Y2/%M0/%D %h:%m", Time::FORMAT_LOCAL);
		}
		break;
	case ViewColumn::TYPE_FROM:
		wstrText = pmh->getFrom();
		break;
	case ViewColumn::TYPE_TO:
		wstrText = pmh->getTo();
		break;
	case ViewColumn::TYPE_FROMTO:
		wstrText = pmh->getFromTo();
		break;
	case ViewColumn::TYPE_SUBJECT:
		wstrText = pmh->getSubject();
		break;
	case ViewColumn::TYPE_SIZE:
		swprintf(wsz, L"%dKB", pmh->getSize()/1024 + 1);
		break;
	case ViewColumn::TYPE_FLAGS:
		// TODO
		break;
	case ViewColumn::TYPE_OTHER:
		{
			MacroValuePtr pValue(pViewModel->getValue(pMacro_.get(), pmh));
			if (pValue.get())
				wstrText = pValue->string();
		}
		break;
	default:
		assert(false);
		break;
	}
	if (!wstrText.get())
		wstrText = allocWString(wsz);
	
	return wstrText;
}

unsigned int qm::ViewColumn::getNumber(const ViewModel* pViewModel,
									   MessageHolder* pmh) const
{
	assert(pmh);
	
	unsigned int nValue = 0;
	switch (type_) {
	case ViewColumn::TYPE_NONE:
		assert(false);
		break;
	case ViewColumn::TYPE_ID:
		nValue = pmh->getId();
		break;
	case ViewColumn::TYPE_DATE:
		// TODO
		break;
	case ViewColumn::TYPE_FROM:
		// TODO
		break;
	case ViewColumn::TYPE_TO:
		// TODO
		break;
	case ViewColumn::TYPE_FROMTO:
		// TODO
		break;
	case ViewColumn::TYPE_SUBJECT:
		// TODO
		break;
	case ViewColumn::TYPE_SIZE:
		nValue = pmh->getSize();
		break;
	case ViewColumn::TYPE_FLAGS:
		nValue = pmh->getFlags();
		break;
	case ViewColumn::TYPE_OTHER:
		{
			MacroValuePtr pValue(pViewModel->getValue(pMacro_.get(), pmh));
			if (pValue.get())
				nValue = pValue->number();
		}
		break;
	default:
		assert(false);
		break;
	}
	return nValue;
}

void qm::ViewColumn::getTime(const ViewModel* pViewModel,
							 MessageHolder* pmh,
							 Time* pTime) const
{
	assert(pmh);
	assert(pTime);
	
	switch (type_) {
	case ViewColumn::TYPE_NONE:
		assert(false);
		break;
	case ViewColumn::TYPE_ID:
		// TODO
		break;
	case ViewColumn::TYPE_DATE:
		pmh->getDate(pTime);
		break;
	case ViewColumn::TYPE_FROM:
		// TODO
		break;
	case ViewColumn::TYPE_TO:
		// TODO
		break;
	case ViewColumn::TYPE_FROMTO:
		// TODO
		break;
	case ViewColumn::TYPE_SUBJECT:
		// TODO
		break;
	case ViewColumn::TYPE_SIZE:
		// TODO
		break;
	case ViewColumn::TYPE_FLAGS:
		// TODO
		break;
	case ViewColumn::TYPE_OTHER:
		// TODO
		break;
	default:
		assert(false);
		break;
	}
	
	// TODO
	// Default behavior
	// Convert to string and then conver to time
	// as it is a RFC2822 Date format
}


/****************************************************************************
 *
 * ViewModelHandler
 *
 */

qm::ViewModelHandler::~ViewModelHandler()
{
}


/****************************************************************************
 *
 * ViewModel::SelectionRestorer
 *
 */

qm::ViewModel::SelectionRestorer::SelectionRestorer(ViewModel* pViewModel,
													bool bRefresh,
													bool bIgnore) :
	pViewModel_(pViewModel),
	bRefresh_(bRefresh),
	pmhFocused_(0),
	pmhLastSelection_(0)
{
	assert(pViewModel);
	assert(pViewModel->isLocked());
	
	if (!bIgnore) {
		ViewModel::ItemList& l = pViewModel_->listItem_;
		if (!l.empty()) {
			ViewModelItem* pItemFocused = l[pViewModel_->nFocused_];
			assert(pItemFocused->getFlags() & ViewModelItem::FLAG_FOCUSED);
			pmhFocused_ = pItemFocused->getMessageHolder();
			
			ViewModelItem* pItemLastSelection = l[pViewModel_->nLastSelection_];
			pmhLastSelection_ = pItemLastSelection->getMessageHolder();
		}
	}
}

qm::ViewModel::SelectionRestorer::~SelectionRestorer()
{
}

void qm::ViewModel::SelectionRestorer::restore()
{
	ViewModel::ItemList& l = pViewModel_->listItem_;
	
	if (pmhFocused_) {
		ViewModelItem item(pmhFocused_);
		ViewModel::ItemList::iterator it = std::find_if(
			l.begin(), l.end(), std::bind2nd(ViewModelItemEqual(), &item));
		if (it != l.end())
			pViewModel_->nFocused_ = it - l.begin();
		else
			pViewModel_->nFocused_ = 0;
	}
	else {
		pViewModel_->nFocused_ = 0;
	}
	
	if (pmhLastSelection_) {
		ViewModelItem item(pmhLastSelection_);
		ItemList::iterator it = std::find_if(
			l.begin(), l.end(), std::bind2nd(ViewModelItemEqual(), &item));
		if (it != l.end())
			pViewModel_->nLastSelection_ = it - l.begin();
		else
			pViewModel_->nLastSelection_ = 0;
	}
	else {
		pViewModel_->nLastSelection_ = 0;
	}
	
	if (bRefresh_ && !l.empty()) {
		assert(pViewModel_->nFocused_ < l.size());
		l[pViewModel_->nFocused_]->setFlags(
			ViewModelItem::FLAG_FOCUSED, ViewModelItem::FLAG_FOCUSED);
		assert(pViewModel_->nLastSelection_ < l.size());
		l[pViewModel_->nLastSelection_]->setFlags(
			ViewModelItem::FLAG_SELECTED, ViewModelItem::FLAG_SELECTED);
	}
	
	assert(l.empty() || l[pViewModel_->nFocused_]->getFlags() & ViewModelItem::FLAG_FOCUSED);
}


/****************************************************************************
 *
 * ViewModel
 *
 */

qm::ViewModel::ViewModel(ViewModelManager* pViewModelManager,
						 Folder* pFolder,
						 Profile* pProfile,
						 Document* pDocument,
						 HWND hwnd,
						 SecurityModel* pSecurityModel,
						 const ColorManager* pColorManager) :
	pViewModelManager_(pViewModelManager),
	pFolder_(pFolder),
	pProfile_(pProfile),
	pDocument_(pDocument),
	hwnd_(hwnd),
	pSecurityModel_(pSecurityModel),
	pColorSet_(0),
	nUnseenCount_(0),
	nSort_(SORT_ASCENDING | SORT_NOTHREAD),
	pFilter_(0),
	nLastSelection_(0),
	nFocused_(0)
{
	assert(pFolder);
	assert(pProfile);
	
#ifndef NDEBUG
	nLock_ = 0;
#endif
	
	pColorSet_ = pColorManager->getColorSet(pFolder_);
	
	WCHAR wszSection[32];
	swprintf(wszSection, L"Folder%d", pFolder_->getId());
	nSort_ = pProfile_->getInt(wszSection, L"Sort", SORT_ASCENDING | SORT_NOTHREAD | 1);
	nFocused_ = pProfile_->getInt(wszSection, L"Focus", 0);
	
	loadColumns();
	if ((nSort_ & SORT_INDEX_MASK) >= getColumnCount())
		nSort_ = 0;
	
	Lock<ViewModel> lock(*this);
	
	update(false);
	
	if (nFocused_ >= listItem_.size())
		nFocused_ = listItem_.empty() ? 0 : listItem_.size() - 1;
	if (!listItem_.empty()) {
		nLastSelection_ = nFocused_;
		listItem_[nFocused_]->setFlags(
			ViewModelItem::FLAG_SELECTED | ViewModelItem::FLAG_FOCUSED,
			ViewModelItem::FLAG_SELECTED | ViewModelItem::FLAG_FOCUSED);
	}
	
	pFolder_->addFolderHandler(this);
	pFolder->getAccount()->addMessageHolderHandler(this);
}

qm::ViewModel::~ViewModel()
{
	pFolder_->getAccount()->removeMessageHolderHandler(this);
	pFolder_->removeFolderHandler(this);
	std::for_each(listItem_.begin(), listItem_.end(), deleter<ViewModelItem>());
	std::for_each(listColumn_.begin(), listColumn_.end(), deleter<ViewColumn>());
}

Folder* qm::ViewModel::getFolder() const
{
	return pFolder_;
}

const ViewModel::ColumnList& qm::ViewModel::getColumns() const
{
	return listColumn_;
}

unsigned int qm::ViewModel::getColumnCount() const
{
	return listColumn_.size();
}

const ViewColumn& qm::ViewModel::getColumn(unsigned int n) const
{
	assert(n < listColumn_.size());
	return *listColumn_[n];
}

ViewColumn& qm::ViewModel::getColumn(unsigned int n)
{
	assert(n < listColumn_.size());
	return *listColumn_[n];
}

unsigned int qm::ViewModel::getCount() const
{
	assert(isLocked());
	return listItem_.size();
}

unsigned int qm::ViewModel::getUnseenCount() const
{
	assert(isLocked());
	return nUnseenCount_;
}

const ViewModelItem* qm::ViewModel::getItem(unsigned int n)
{
	assert(isLocked());
	assert(n < getCount());
	
	ViewModelItem* pItem = listItem_[n];
	MessageHolder* pmh = pItem->getMessageHolder();
	if (pItem->getColor() == 0xffffffff ||
		pItem->getMessageFlags() != pmh->getFlags()) {
		COLORREF cr = 0xff000000;
		pItem->setMessageFlags(pmh->getFlags());
		if (pColorSet_) {
			Message msg;
			MacroContext context(pmh, &msg, MessageHolderList(),
				pFolder_->getAccount(), pDocument_, hwnd_, pProfile_, true,
				/*pSecurityModel_->isDecryptVerify()*/false, 0, 0);
			cr = pColorSet_->getColor(&context);
		}
		pItem->setColor(cr);
	}
	
	return pItem;
}

MessageHolder* qm::ViewModel::getMessageHolder(unsigned int n) const
{
	assert(isLocked());
	assert(n < getCount());
	return listItem_[n]->getMessageHolder();
}

unsigned int qm::ViewModel::getIndex(MessageHolder* pmh) const
{
	assert(isLocked());
	assert(pmh);
	
	ViewModelItem item(pmh);
	ItemList::const_iterator it = std::lower_bound(
		listItem_.begin(), listItem_.end(), &item,
		ViewModelItemComp(this, getColumn(nSort_ & SORT_INDEX_MASK),
			(nSort_ & SORT_DIRECTION_MASK) == SORT_ASCENDING,
			(nSort_ & SORT_THREAD_MASK) == SORT_THREAD));
	while (it != listItem_.end() && (*it)->getMessageHolder() != pmh)
		++it;
	if (it == listItem_.end()) {
		it = listItem_.begin();
		while (it != listItem_.end() && (*it)->getMessageHolder() != pmh)
			++it;
	}
	return it == listItem_.end() ?
		static_cast<unsigned int>(-1) : it - listItem_.begin();
}

void qm::ViewModel::setSort(unsigned int nSort)
{
	assert(nSort_ & SORT_DIRECTION_MASK);
	assert(nSort_ & SORT_THREAD_MASK);
	
	Lock<ViewModel> lock(*this);
	
	if ((nSort & SORT_DIRECTION_MASK) == 0) {
		if ((nSort & SORT_INDEX_MASK) == (nSort_ & SORT_INDEX_MASK))
			nSort |= (nSort_ & SORT_DIRECTION_MASK) == SORT_ASCENDING ?
				SORT_DESCENDING : SORT_ASCENDING;
		else
			nSort |= SORT_ASCENDING;
	}
	
	if ((nSort & SORT_THREAD_MASK) == 0)
		nSort |= nSort_ & SORT_THREAD_MASK;
	
	assert(nSort & SORT_DIRECTION_MASK);
	assert(nSort & SORT_THREAD_MASK);
	assert((nSort & SORT_INDEX_MASK) < getColumnCount());
	
	sort(nSort, true, true);
	
	nSort_ = nSort;
	
	fireSorted();
}

unsigned int qm::ViewModel::getSort() const
{
	return nSort_;
}

void qm::ViewModel::setFilter(const Filter* pFilter)
{
	if (pFilter != pFilter_) {
		pFilter_ = pFilter;
		update(true);
	}
}

const Filter* qm::ViewModel::getFilter() const
{
	return pFilter_;
}

void qm::ViewModel::addSelection(unsigned int n)
{
	assert(isLocked());
	assert(n < getCount());
	
	ViewModelItem* pItem = listItem_[n];
	if (!(pItem->getFlags() & ViewModelItem::FLAG_SELECTED)) {
		pItem->setFlags(ViewModelItem::FLAG_SELECTED, ViewModelItem::FLAG_SELECTED);
		fireItemStateChanged(n);
	}
}

void qm::ViewModel::addSelection(unsigned int nStart,
								 unsigned int nEnd)
{
	assert(isLocked());
	assert(nStart < getCount());
	assert(nEnd < getCount());
	
	if (nStart > nEnd)
		std::swap(nStart, nEnd);
	
	while (nStart <= nEnd)
		addSelection(nStart);
}

void qm::ViewModel::removeSelection(unsigned int n)
{
	assert(isLocked());
	assert(n < getCount());
	
	ViewModelItem* pItem = listItem_[n];
	if (pItem->getFlags() & ViewModelItem::FLAG_SELECTED) {
		pItem->setFlags(0, ViewModelItem::FLAG_SELECTED);
		fireItemStateChanged(n);
	}
}

void qm::ViewModel::setSelection(unsigned int n)
{
	assert(isLocked());
	
	clearSelection();
	addSelection(n);
}

void qm::ViewModel::setSelection(unsigned int nStart,
								 unsigned int nEnd)
{
	assert(isLocked());
	assert(nStart < getCount());
	assert(nEnd < getCount());
	
	if (nStart > nEnd)
		std::swap(nStart, nEnd);
	
	unsigned int n = 0;
	while (n < nStart) {
		if (isSelected(n))
			removeSelection(n);
		++n;
	}
	while (n <= nEnd) {
		if (!isSelected(n))
			addSelection(n);
		++n;
	}
	while (n < listItem_.size()) {
		if (isSelected(n))
			removeSelection(n);
		++n;
	}
}

void qm::ViewModel::clearSelection()
{
	assert(isLocked());
	
	for (ItemList::size_type n = 0; n < listItem_.size(); ++n)
		removeSelection(n);
}

void qm::ViewModel::getSelection(MessageHolderList* pList) const
{
	assert(pList);
	assert(isLocked());
	
	for (ItemList::const_iterator it = listItem_.begin(); it != listItem_.end(); ++it) {
		ViewModelItem* pItem = *it;
		if (pItem->getFlags() & ViewModelItem::FLAG_SELECTED)
			pList->push_back(pItem->getMessageHolder());
	}
}

bool qm::ViewModel::hasSelection() const
{
	assert(isLocked());
	
	ItemList::const_iterator it = listItem_.begin();
	while (it != listItem_.end() && !((*it)->getFlags() & ViewModelItem::FLAG_SELECTED))
		++it;
	
	return it != listItem_.end();
}

unsigned int qm::ViewModel::getSelectedCount() const
{
	assert(isLocked());
	
	return std::count_if(listItem_.begin(), listItem_.end(),
		std::bind2nd(
			std::mem_fun(&ViewModelItem::isFlag),
			ViewModelItem::FLAG_SELECTED));
}

bool qm::ViewModel::isSelected(unsigned int n) const
{
	assert(isLocked());
	assert(n < getCount());
	
	return (listItem_[n]->getFlags() & ViewModelItem::FLAG_SELECTED) != 0;
}

unsigned int qm::ViewModel::getLastSelection() const
{
	assert(isLocked());
	return nLastSelection_;
}

void qm::ViewModel::setLastSelection(unsigned int n)
{
	assert(isLocked());
	assert(n < getCount());
	nLastSelection_ = n;
}

void qm::ViewModel::setFocused(unsigned int n)
{
	Lock<ViewModel> lock(*this);
	
	assert(n < getCount());
	
	if (nFocused_ != n) {
		unsigned int nOld = nFocused_;
		nFocused_ = n;
		
		assert(listItem_[nOld]->getFlags() & ViewModelItem::FLAG_FOCUSED);
		listItem_[nOld]->setFlags(0, ViewModelItem::FLAG_FOCUSED);
		listItem_[nFocused_]->setFlags(ViewModelItem::FLAG_FOCUSED,
			ViewModelItem::FLAG_FOCUSED);
		
		fireItemStateChanged(nOld);
		fireItemStateChanged(n);
	}
}

unsigned int qm::ViewModel::getFocused() const
{
	Lock<ViewModel> lock(*this);
	assert(listItem_.empty() ||
		listItem_[nFocused_]->getFlags() & ViewModelItem::FLAG_FOCUSED);
	return nFocused_;
}

bool qm::ViewModel::isFocused(unsigned int n) const
{
	Lock<ViewModel> lock(*this);
	assert(n < getCount());
	assert(listItem_.empty() ||
		listItem_[nFocused_]->getFlags() & ViewModelItem::FLAG_FOCUSED);
	return n == nFocused_;
}

void qm::ViewModel::payAttention(unsigned int n)
{
	fireItemAttentionPaid(n);
}

bool qm::ViewModel::save() const
{
	WCHAR wszSection[32];
	swprintf(wszSection, L"Folder%d", pFolder_->getId());
	pProfile_->setInt(wszSection, L"Sort", nSort_);
	pProfile_->setInt(wszSection, L"Focus", nFocused_);
	
	saveColumns();
	
	return true;
}

void qm::ViewModel::addViewModelHandler(ViewModelHandler* pHandler)
{
	listHandler_.push_back(pHandler);
}

void qm::ViewModel::removeViewModelHandler(ViewModelHandler* pHandler)
{
	ViewModelHandlerList& l = listHandler_;
	ViewModelHandlerList::iterator it = std::remove(
		l.begin(), l.end(), pHandler);
	l.erase(it, l.end());
}

void qm::ViewModel::lock() const
{
	pFolder_->getAccount()->lock();
#ifndef NDEBUG
	++nLock_;
#endif
}

void qm::ViewModel::unlock() const
{
#ifndef NDEBUG
	--nLock_;
#endif
	pFolder_->getAccount()->unlock();
}

#ifndef NDEBUG
bool qm::ViewModel::isLocked() const
{
	return nLock_ != 0;
}
#endif

MacroValuePtr qm::ViewModel::getValue(const Macro* pMacro,
									  MessageHolder* pmh) const
{
	Message msg;
	MacroContext context(pmh, &msg, MessageHolderList(),
		pFolder_->getAccount(), pDocument_, hwnd_, pProfile_, true,
		/*pSecurityModel_->isDecryptVerify()*/false, 0, 0);
	return pMacro->value(&context);
}

void qm::ViewModel::messageAdded(const FolderEvent& event)
{
	assert(event.getFolder() == pFolder_);
	
	Lock<ViewModel> lock(*this);
	
	// TODO
	// Resort if this view model is sorted by flags
	
	MessageHolder* pmh = event.getMessageHolder();
	
	bool bAdd = true;
	if (pFilter_) {
		Message msg;
		MacroContext context(pmh, &msg, MessageHolderList(),
			pFolder_->getAccount(), pDocument_, hwnd_, pProfile_, false,
			/*pSecurityModel_->isDecryptVerify()*/false, 0, 0);
		bAdd = pFilter_->match(&context);
	}
	
	if (bAdd) {
		std::auto_ptr<ViewModelItem> pItem(new ViewModelItem(pmh));
		
		if ((getSort() & SORT_THREAD_MASK) == SORT_THREAD) {
			unsigned int nReferenceHash = pmh->getReferenceHash();
			if (nReferenceHash != 0) {
				wstring_ptr wstrReference(pmh->getReference());
				
				for (ItemList::const_iterator it = listItem_.begin(); it != listItem_.end(); ++it) {
					MessageHolder* pmhParent = (*it)->getMessageHolder();
					if (pmhParent->getMessageIdHash() == nReferenceHash) {
						wstring_ptr wstrMessageId(pmhParent->getMessageId());
						if (wcscmp(wstrReference.get(), wstrMessageId.get()) == 0) {
							pItem->setParentItem(*it);
							break;
						}
					}
				}
			}
		}
		
		ItemList::iterator it = std::lower_bound(
			listItem_.begin(), listItem_.end(), pItem.get(),
			ViewModelItemComp(this, getColumn(nSort_ & SORT_INDEX_MASK),
				(nSort_ & SORT_DIRECTION_MASK) == SORT_ASCENDING,
				(nSort_ & SORT_THREAD_MASK) == SORT_THREAD));
		
		ItemList::iterator itInsert = listItem_.insert(it, pItem.get());
		pItem.release();
		
		unsigned int nPos = itInsert - listItem_.begin();
		if (nLastSelection_ >= nPos && nLastSelection_ < listItem_.size() - 1)
			++nLastSelection_;
		if (nFocused_ >= nPos && nFocused_ < listItem_.size() - 1)
			++nFocused_;
		if (listItem_.size() == 1) {
			assert(nFocused_ == 0);
			assert(nLastSelection_ == 0);
			listItem_[0]->setFlags(
				ViewModelItem::FLAG_FOCUSED | ViewModelItem::FLAG_SELECTED,
				ViewModelItem::FLAG_FOCUSED | ViewModelItem::FLAG_SELECTED);
		}
		assert(listItem_.empty() ||
			(listItem_[nFocused_]->getFlags() & ViewModelItem::FLAG_FOCUSED));
		
		if (!pmh->isFlag(MessageHolder::FLAG_SEEN))
			++nUnseenCount_;
		
		fireItemAdded(nPos);
	}
}

void qm::ViewModel::messageRemoved(const FolderEvent& event)
{
	assert(event.getFolder() == pFolder_);
	
	Lock<ViewModel> lock(*this);
	
	MessageHolder* pmh = event.getMessageHolder();
	
	unsigned int nIndex = getIndex(pmh);
	if (nIndex != static_cast<unsigned int>(-1)) {
		ItemList::iterator it = listItem_.begin() + nIndex;
		
		bool bHasFocus = ((*it)->getFlags() & ViewModelItem::FLAG_FOCUSED) != 0;
		bool bSelected = ((*it)->getFlags() & ViewModelItem::FLAG_SELECTED) != 0;
		
		bool bSort = false;
		if ((getSort() & SORT_THREAD_MASK) == SORT_THREAD) {
			ViewModelItem* pItem = *it;
			unsigned int nLevel = pItem->getLevel();
			
			ItemList::const_iterator itEnd = it + 1;
			while (itEnd != listItem_.end() && (*itEnd)->getLevel() > nLevel)
				++itEnd;
			
			ItemList::const_iterator itC = it + 1;
			while (itC != itEnd) {
				if ((*itC)->getParentItem() == pItem) {
					(*itC)->setParentItem(0);
					bSort = true;
				}
				++itC;
			}
			assert(std::find_if(listItem_.begin(), listItem_.end(),
				std::bind2nd(
					binary_compose_f_gx_hy(
						std::equal_to<ViewModelItem*>(),
						std::mem_fun(&ViewModelItem::getParentItem),
						std::identity<ViewModelItem*>()),
					pItem)) == listItem_.end());
		}
		
		fireItemRemoved(nIndex);
		
		delete *it;
		it = listItem_.erase(it);
		
		if (bHasFocus) {
			assert(nFocused_ == nIndex);
			if (it == listItem_.end()) {
				if (listItem_.empty())
					nFocused_ = 0;
				else
					--nFocused_;
			}
			if (!listItem_.empty())
				listItem_[nFocused_]->setFlags(ViewModelItem::FLAG_FOCUSED,
					ViewModelItem::FLAG_FOCUSED);
		}
		else if (nFocused_ > nIndex) {
			--nFocused_;
			assert(!listItem_.empty());
			assert(listItem_[nFocused_]->getFlags() & ViewModelItem::FLAG_FOCUSED);
		}
		
		if (bSelected) {
			if (it == listItem_.end()) {
				if (!listItem_.empty())
					listItem_.back()->setFlags(ViewModelItem::FLAG_SELECTED,
						ViewModelItem::FLAG_SELECTED);
			}
			else {
				(*it)->setFlags(ViewModelItem::FLAG_SELECTED,
					ViewModelItem::FLAG_SELECTED);
			}
		}
		if (nLastSelection_ == nIndex) {
			if (it == listItem_.end()) {
				if (listItem_.empty()) {
					nLastSelection_ = 0;
				}
				else {
					nLastSelection_ = listItem_.size() - 1;
					listItem_[nLastSelection_]->setFlags(
						ViewModelItem::FLAG_SELECTED,
						ViewModelItem::FLAG_SELECTED);
				}
			}
		}
		else if (nLastSelection_ > nIndex) {
			--nLastSelection_;
			assert(!listItem_.empty());
		}
		
		if (bSort)
			sort(nSort_, true, false);
		
		if (!pmh->isFlag(MessageHolder::FLAG_SEEN))
			--nUnseenCount_;
	}
}

void qm::ViewModel::messageRefreshed(const FolderEvent& event)
{
	update(true);
}

void qm::ViewModel::unseenCountChanged(const FolderEvent& event)
{
}

void qm::ViewModel::folderDestroyed(const FolderEvent& event)
{
	fireDestroyed();
	pViewModelManager_->removeViewModel(this);
}

void qm::ViewModel::messageHolderChanged(const MessageHolderEvent& event)
{
	Lock<ViewModel> lock(*this);
	
	ViewModelItem item(event.getMessageHolder());
	ItemList::iterator it = std::find_if(
		listItem_.begin(), listItem_.end(),
		std::bind2nd(ViewModelItemEqual(), &item));
	if (it != listItem_.end()) {
		unsigned int nOldFlags = event.getOldFlags();
		unsigned int nNewFlags = event.getNewFlags();
		if (nOldFlags & MessageHolder::FLAG_SEEN &&
			!(nNewFlags & MessageHolder::FLAG_SEEN))
			++nUnseenCount_;
		else if (!(nOldFlags & MessageHolder::FLAG_SEEN) &&
			nNewFlags & MessageHolder::FLAG_SEEN)
			--nUnseenCount_;
		
		fireItemChanged(it - listItem_.begin());
	}
}

void qm::ViewModel::messageHolderDestroyed(const MessageHolderEvent& event)
{
}

void qm::ViewModel::loadColumns()
{
	MacroParser parser(MacroParser::TYPE_COLUMN);
	
	WCHAR wszSection[32];
	swprintf(wszSection, L"Folder%d", pFolder_->getId());
	
	int nColumn = 0;
	while (true) {
		wstring_ptr wstrTitle;
		ViewColumn::Type type = ViewColumn::TYPE_NONE;
		std::auto_ptr<Macro> pMacro;
		int nFlags = 0;
		int nWidth = 0;
		
		WCHAR wszKey[32];
		
		swprintf(wszKey, L"ColumnTitle%d", nColumn);
		wstrTitle = pProfile_->getString(wszSection, wszKey, 0);
		
		swprintf(wszKey, L"ColumnMacro%d", nColumn);
		wstring_ptr wstrMacro(pProfile_->getString(wszSection, wszKey, 0));
		if (!*wstrMacro.get())
			break;
		if (*wstrMacro.get() == L'%') {
			struct {
				const WCHAR* pwszMacro_;
				ViewColumn::Type type_;
			} defaults[] = {
				{ L"%id",		ViewColumn::TYPE_ID			},
				{ L"%date",		ViewColumn::TYPE_DATE		},
				{ L"%from",		ViewColumn::TYPE_FROM		},
				{ L"%to",		ViewColumn::TYPE_TO			},
				{ L"%fromto",	ViewColumn::TYPE_FROMTO		},
				{ L"%subject",	ViewColumn::TYPE_SUBJECT	},
				{ L"%size",		ViewColumn::TYPE_SIZE		},
				{ L"%flags",	ViewColumn::TYPE_FLAGS		}
			};
			for (int n = 0; n < countof(defaults) && type == ViewColumn::TYPE_NONE; ++n) {
				if (_wcsicmp(wstrMacro.get(), defaults[n].pwszMacro_) == 0)
					type = defaults[n].type_;
			}
		}
		if (type == ViewColumn::TYPE_NONE) {
			pMacro = parser.parse(wstrMacro.get());
			type = ViewColumn::TYPE_OTHER;
		}
		
		swprintf(wszKey, L"ColumnFlags%d", nColumn);
		nFlags = pProfile_->getInt(wszSection, wszKey, 0);
		
		swprintf(wszKey, L"ColumnWidth%d", nColumn);
		nWidth = pProfile_->getInt(wszSection, wszKey, 0);
		
		std::auto_ptr<ViewColumn> pColumn(new ViewColumn(
			wstrTitle.get(), type, pMacro, nFlags, nWidth));
		listColumn_.push_back(pColumn.get());
		pColumn.release();
		
		++nColumn;
	}
	
	if (nColumn == 0) {
		struct {
			const WCHAR* pwszTitle_;
			ViewColumn::Type type_;
			unsigned int nFlags_;
			unsigned int nWidth_;
		} columns[] = {
			{
				L"",
				ViewColumn::TYPE_FLAGS,
				ViewColumn::FLAG_ICON | ViewColumn::FLAG_SORT_NUMBER,
				28
			},
			{
				L"Date",
				ViewColumn::TYPE_DATE,
				ViewColumn::FLAG_SORT_DATE,
				80
			},
			{
				L"From / To",
				ViewColumn::TYPE_FROMTO,
				ViewColumn::FLAG_SORT_TEXT,
				120
			},
			{
				L"Subject",
				ViewColumn::TYPE_SUBJECT,
				ViewColumn::FLAG_INDENT | ViewColumn::FLAG_LINE | ViewColumn::FLAG_SORT_TEXT,
				250
			},
			{
				L"Size",
				ViewColumn::TYPE_SIZE,
				ViewColumn::FLAG_RIGHTALIGN | ViewColumn::FLAG_SORT_NUMBER,
				40
			}
		};
		for (int n = 0; n < countof(columns); ++n) {
			std::auto_ptr<Macro> pMacro;
			std::auto_ptr<ViewColumn> pColumn(new ViewColumn(
				columns[n].pwszTitle_, columns[n].type_, pMacro,
				columns[n].nFlags_, columns[n].nWidth_));
			listColumn_.push_back(pColumn.get());
			pColumn.release();
		}
	}

}

void qm::ViewModel::saveColumns() const
{
	WCHAR wszSection[32];
	swprintf(wszSection, L"Folder%d", pFolder_->getId());
	
	for (ColumnList::size_type n = 0; n < listColumn_.size(); ++n) {
		ViewColumn* pColumn = listColumn_[n];
		
		WCHAR wszKey[32];
		
		swprintf(wszKey, L"ColumnTitle%d", n);
		pProfile_->setString(wszSection, wszKey, pColumn->getTitle());
		
		struct {
			ViewColumn::Type type_;
			const WCHAR* pwszMacro_;
		} macros[] = {
			{ ViewColumn::TYPE_ID,		L"%Id"				},
			{ ViewColumn::TYPE_DATE,	L"%Date"			},
			{ ViewColumn::TYPE_FROM,	L"%From"			},
			{ ViewColumn::TYPE_TO,		L"%To"				},
			{ ViewColumn::TYPE_FROMTO,	L"%FromTo"			},
			{ ViewColumn::TYPE_SUBJECT,	L"%Subject"			},
			{ ViewColumn::TYPE_SIZE,	L"%Size"			},
			{ ViewColumn::TYPE_FLAGS,	L"%Flags"			}
		};
		const WCHAR* pwszMacro = 0;
		for (int m = 0; m < countof(macros) && !pwszMacro; ++m) {
			if (macros[m].type_ == pColumn->getType())
				pwszMacro = macros[m].pwszMacro_;
		}
		wstring_ptr wstrMacro;
		if (!pwszMacro) {
			wstrMacro = pColumn->getMacro()->getString();
			pwszMacro = wstrMacro.get();
		}
		swprintf(wszKey, L"ColumnMacro%d", n);
		pProfile_->setString(wszSection, wszKey, pwszMacro);
		
		swprintf(wszKey, L"ColumnFlags%d", n);
		pProfile_->setInt(wszSection, wszKey, pColumn->getFlags());
		
		swprintf(wszKey, L"ColumnWidth%d", n);
		pProfile_->setInt(wszSection, wszKey, pColumn->getWidth());
	}
}

void qm::ViewModel::update(bool bRestoreSelection)
{
	Lock<ViewModel> lock(*this);
	
	// TODO
	// Check error
	if (!pFolder_->loadMessageHolders())
		return;
	
	SelectionRestorer restorer(this, true, !bRestoreSelection);
	
	std::for_each(listItem_.begin(), listItem_.end(), deleter<ViewModelItem>());
	listItem_.clear();
	
	unsigned int nCount = pFolder_->getCount();
	listItem_.reserve(nCount);
	
	nUnseenCount_ = 0;
	
	MacroVariableHolder globalVariable;
	for (unsigned int n = 0; n < nCount; ++n) {
		MessageHolder* pmh = pFolder_->getMessage(n);
		bool bAdd = true;
		if (pFilter_) {
			Message msg;
			MacroContext context(pmh, &msg, MessageHolderList(),
				pFolder_->getAccount(), pDocument_, hwnd_, pProfile_, false,
				/*pSecurityModel_->isDecryptVerify()*/false, 0, &globalVariable);
			bAdd = pFilter_->match(&context);
		}
		if (bAdd) {
			std::auto_ptr<ViewModelItem> pItem(new ViewModelItem(pmh));
			listItem_.push_back(pItem.release());
			
			if (!pmh->isFlag(MessageHolder::FLAG_SEEN))
				++nUnseenCount_;
		}
	}
	
	sort(nSort_, false, true);
	
	if (bRestoreSelection)
		restorer.restore();
	
	fireUpdated();
}

void qm::ViewModel::sort(unsigned int nSort,
						 bool bRestoreSelection,
						 bool bUpdateParentLink)
{
	assert(nSort & SORT_DIRECTION_MASK);
	assert(nSort & SORT_THREAD_MASK);
	
	Lock<ViewModel> lock(*this);
	
	SelectionRestorer restorer(this, false, !bRestoreSelection);
	
	if (bUpdateParentLink && (nSort & SORT_THREAD_MASK) == SORT_THREAD)
		makeParentLink();
	
	std::stable_sort(listItem_.begin(), listItem_.end(),
		ViewModelItemComp(this, getColumn(nSort & SORT_INDEX_MASK),
			(nSort & SORT_DIRECTION_MASK) == SORT_ASCENDING,
			(nSort & SORT_THREAD_MASK) == SORT_THREAD));
	
	if (bRestoreSelection)
		restorer.restore();
}

void qm::ViewModel::makeParentLink()
{
	Lock<ViewModel> lock(*this);
	
	ItemList listItemSortedByMessageIdHash(listItem_);
	std::sort(listItemSortedByMessageIdHash.begin(),
		listItemSortedByMessageIdHash.end(),
		binary_compose_f_gx_hy(
			std::less<unsigned int>(),
			unary_compose_f_gx(
				std::mem_fun(&MessageHolder::getMessageIdHash),
				std::mem_fun(&ViewModelItem::getMessageHolder)),
			unary_compose_f_gx(
				std::mem_fun(&MessageHolder::getMessageIdHash),
				std::mem_fun(&ViewModelItem::getMessageHolder))));
	
	ItemList listItemSortedByPointer(listItem_);
	std::sort(listItemSortedByPointer.begin(),
		listItemSortedByPointer.end());
	
	for (ItemList::iterator it = listItem_.begin(); it != listItem_.end(); ++it)
		makeParentLink(listItemSortedByMessageIdHash, listItemSortedByPointer, *it);
}

void qm::ViewModel::makeParentLink(const ItemList& listItemSortedByMessageIdHash,
								   const ItemList& listItemSortedByPointer,
								   ViewModelItem* pItem)
{
	assert(pItem);
	
	MessageHolder* pmh = pItem->getMessageHolder();
	ViewModelItem* pParentItem = pItem->getParentItem();
	if (!pParentItem) {
		unsigned int nReferenceHash = pmh->getReferenceHash();
		if (nReferenceHash != 0) {
			ViewModelItem item(nReferenceHash);
			ItemList::const_iterator it = std::lower_bound(
				listItemSortedByMessageIdHash.begin(),
				listItemSortedByMessageIdHash.end(), &item,
				binary_compose_f_gx_hy(
					std::less<unsigned int>(),
					std::mem_fun(&ViewModelItem::getMessageIdHash),
					std::mem_fun(&ViewModelItem::getMessageIdHash)));
			if  (it != listItemSortedByMessageIdHash.end() &&
				(*it)->getMessageHolder()->getMessageIdHash() == nReferenceHash) {
				bool bFound = false;
				wstring_ptr wstrReference(pmh->getReference());
				assert(*wstrReference.get());
				while  (it != listItemSortedByMessageIdHash.end() &&
					(*it)->getMessageHolder()->getMessageIdHash() == nReferenceHash &&
					!bFound) {
					wstring_ptr wstrMessageId((*it)->getMessageHolder()->getMessageId());
					if (wcscmp(wstrReference.get(), wstrMessageId.get()) == 0) {
						bFound = true;
						break;
					}
					++it;
				}
				if (bFound)
					pItem->setParentItem(*it);
			}
		}
	}
	else {
		ItemList::const_iterator it = std::lower_bound(
			listItemSortedByPointer.begin(),
			listItemSortedByPointer.end(), pParentItem);
		if (it == listItemSortedByPointer.end() || *it != pParentItem)
			pItem->setParentItem(0);
	}
}

void qm::ViewModel::fireItemAdded(unsigned int nItem) const
{
	fireEvent(ViewModelEvent(this, nItem), &ViewModelHandler::itemAdded);
}

void qm::ViewModel::fireItemRemoved(unsigned int nItem) const
{
	fireEvent(ViewModelEvent(this, nItem), &ViewModelHandler::itemRemoved);
}

void qm::ViewModel::fireItemChanged(unsigned int nItem) const
{
	fireEvent(ViewModelEvent(this, nItem), &ViewModelHandler::itemChanged);
}

void qm::ViewModel::fireItemStateChanged(unsigned int nItem) const
{
	fireEvent(ViewModelEvent(this, nItem), &ViewModelHandler::itemStateChanged);
}

void qm::ViewModel::fireItemAttentionPaid(unsigned int nItem) const
{
	fireEvent(ViewModelEvent(this, nItem), &ViewModelHandler::itemAttentionPaid);
}

void qm::ViewModel::fireUpdated() const
{
	fireEvent(ViewModelEvent(this), &ViewModelHandler::updated);
}

void qm::ViewModel::fireSorted() const
{
	fireEvent(ViewModelEvent(this), &ViewModelHandler::sorted);
}

void qm::ViewModel::fireDestroyed() const
{
	ViewModelHandlerList l(listHandler_);
	ViewModelEvent event(this);
	for (ViewModelHandlerList::const_iterator it = l.begin(); it != l.end(); ++it)
		(*it)->destroyed(event);
}

void qm::ViewModel::fireEvent(const ViewModelEvent& event,
							  void (ViewModelHandler::*pfn)(const ViewModelEvent&)) const
{
	for (ViewModelHandlerList::const_iterator it = listHandler_.begin(); it != listHandler_.end(); ++it)
		((*it)->*pfn)(event);
}


/****************************************************************************
 *
 * DefaultViewModelHandler
 *
 */

qm::DefaultViewModelHandler::DefaultViewModelHandler()
{
}

qm::DefaultViewModelHandler::~DefaultViewModelHandler()
{
}

void qm::DefaultViewModelHandler::itemAdded(const ViewModelEvent& event)
{
}

void qm::DefaultViewModelHandler::itemRemoved(const ViewModelEvent& event)
{
}

void qm::DefaultViewModelHandler::itemChanged(const ViewModelEvent& event)
{
}

void qm::DefaultViewModelHandler::itemStateChanged(const ViewModelEvent& event)
{
}

void qm::DefaultViewModelHandler::itemAttentionPaid(const ViewModelEvent& event)
{
}

void qm::DefaultViewModelHandler::updated(const ViewModelEvent& event)
{
}

void qm::DefaultViewModelHandler::sorted(const ViewModelEvent& event)
{
}

void qm::DefaultViewModelHandler::destroyed(const ViewModelEvent& event)
{
}


/****************************************************************************
 *
 * ViewModelEvent
 *
 */

qm::ViewModelEvent::ViewModelEvent(const ViewModel* pViewModel) :
	pViewModel_(pViewModel),
	nItem_(static_cast<unsigned int>(-1))
{
}

qm::ViewModelEvent::ViewModelEvent(const ViewModel* pViewModel,
								   unsigned int nItem) :
	pViewModel_(pViewModel),
	nItem_(nItem)
{
}

qm::ViewModelEvent::~ViewModelEvent()
{
}

const ViewModel* qm::ViewModelEvent::getViewModel() const
{
	return pViewModel_;
}

unsigned int qm::ViewModelEvent::getItem() const
{
	return nItem_;
}


/****************************************************************************
 *
 * ViewModelHolder
 *
 */

qm::ViewModelHolder::~ViewModelHolder()
{
}


/****************************************************************************
 *
 * ViewModelManager
 *
 */

qm::ViewModelManager::ViewModelManager(Profile* pProfile,
									   Document* pDocument,
									   HWND hwnd,
									   SecurityModel* pSecurityModel) :
	pProfile_(pProfile),
	pDocument_(pDocument),
	hwnd_(hwnd),
	pSecurityModel_(pSecurityModel),
	pCurrentAccount_(0),
	pCurrentViewModel_(0)
{
	pFilterManager_.reset(new FilterManager());
	pColorManager_.reset(new ColorManager());
}

qm::ViewModelManager::~ViewModelManager()
{
	std::for_each(listViewModel_.begin(),
		listViewModel_.end(), deleter<ViewModel>());
	std::for_each(mapProfile_.begin(), mapProfile_.end(),
		unary_compose_f_gx(
			deleter<Profile>(), std::select2nd<ProfileMap::value_type>()));
}

FilterManager* qm::ViewModelManager::getFilterManager() const
{
	return pFilterManager_.get();
}

Account* qm::ViewModelManager::getCurrentAccount() const
{
	return pCurrentAccount_;
}

void qm::ViewModelManager::setCurrentAccount(Account* pAccount)
{
	setCurrentFolder(pAccount, 0);
}

void qm::ViewModelManager::setCurrentFolder(Folder* pFolder)
{
	setCurrentFolder(0, pFolder);
}

ViewModel* qm::ViewModelManager::getCurrentViewModel() const
{
	return pCurrentViewModel_;
}

ViewModel* qm::ViewModelManager::getViewModel(Folder* pFolder)
{
	assert(pFolder);
	
	ViewModelList::iterator it = std::find_if(listViewModel_.begin(),
		listViewModel_.end(), ViewModelFolderComp(pFolder));
	if (it != listViewModel_.end())
		return *it;
	
	Profile* pProfile = getProfile(pFolder);
	std::auto_ptr<ViewModel> pViewModel(new ViewModel(this, pFolder, pProfile,
		pDocument_, hwnd_, pSecurityModel_, pColorManager_.get()));
	listViewModel_.push_back(pViewModel.get());
	return pViewModel.release();
}

bool qm::ViewModelManager::save() const
{
	for (ViewModelList::const_iterator itM = listViewModel_.begin(); itM != listViewModel_.end(); ++itM) {
		if (!(*itM)->save())
			return false;
	}
	
	for (ProfileMap::const_iterator itP = mapProfile_.begin(); itP != mapProfile_.end(); ++itP) {
		if (!(*itP).second->save())
			return false;
	}
	
	return true;
}

void qm::ViewModelManager::addViewModelManagerHandler(ViewModelManagerHandler* pHandler)
{
	listHandler_.push_back(pHandler);
}

void qm::ViewModelManager::removeViewModelManagerHandler(ViewModelManagerHandler* pHandler)
{
	HandlerList& l = listHandler_;
	HandlerList::iterator it = std::remove(l.begin(), l.end(), pHandler);
	l.erase(it, l.end());
}

void qm::ViewModelManager::removeViewModel(ViewModel* pViewModel)
{
	ViewModelList::iterator it = std::remove(listViewModel_.begin(),
		listViewModel_.end(), pViewModel);
	listViewModel_.erase(it, listViewModel_.end());
	delete pViewModel;
}

void qm::ViewModelManager::accountDestroyed(const AccountEvent& event)
{
	Account* pAccount = event.getAccount();
	
	ViewModelList::iterator itV = listViewModel_.begin();
	while (itV != listViewModel_.end()) {
		ViewModel* pViewModel = *itV;
		if (pViewModel->getFolder()->getAccount() == pAccount) {
			delete pViewModel;
			itV = listViewModel_.erase(itV);
		}
		else {
			++itV;
		}
	}
	
	ProfileMap::iterator itP = std::find_if(
		mapProfile_.begin(), mapProfile_.end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				std::equal_to<Account*>(),
				std::select1st<ProfileMap::value_type>(),
				std::identity<Account*>()),
			pAccount));
	if (itP != mapProfile_.end()) {
		delete (*itP).second;
		mapProfile_.erase(itP);
	}
}

void qm::ViewModelManager::setCurrentFolder(Account* pAccount,
											Folder* pFolder)
{
	assert(!pAccount || !pFolder);
	
	if (pAccount)
		pCurrentAccount_ = pAccount;
	else if (pFolder)
		pCurrentAccount_ = pFolder->getAccount();
	else
		pCurrentAccount_ = 0;
	
	ViewModel* pViewModel = 0;
	if (pFolder)
		pViewModel = getViewModel(pFolder);
	setCurrentViewModel(pViewModel);
}

void qm::ViewModelManager::setCurrentViewModel(ViewModel* pViewModel)
{
	ViewModel* pOldViewModel = pCurrentViewModel_;
	pCurrentViewModel_ = pViewModel;
	
	fireViewModelSelected(pViewModel, pOldViewModel);
}

Profile* qm::ViewModelManager::getProfile(Folder* pFolder)
{
	assert(pFolder);
	
	Account* pAccount = pFolder->getAccount();
	ProfileMap::iterator it = std::find_if(
		mapProfile_.begin(), mapProfile_.end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				std::equal_to<Account*>(),
				std::select1st<ProfileMap::value_type>(),
				std::identity<Account*>()),
			pAccount));
	if (it != mapProfile_.end())
		return (*it).second;
	
	wstring_ptr wstrPath(concat(pAccount->getPath(), L"\\", FileNames::VIEW_XML));
	
	std::auto_ptr<XMLProfile> pProfile(new XMLProfile(wstrPath.get()));
	// TODO
	// Ignore load error?
	pProfile->load();
	mapProfile_.push_back(std::make_pair(pAccount, pProfile.get()));
	pAccount->addAccountHandler(this);
	return pProfile.release();
}

void qm::ViewModelManager::fireViewModelSelected(ViewModel* pNewViewModel,
												 ViewModel* pOldViewModel) const
{
	ViewModelManagerEvent event(this, pNewViewModel, pOldViewModel);
	for (HandlerList::const_iterator it = listHandler_.begin(); it != listHandler_.end(); ++it)
		(*it)->viewModelSelected(event);
}


/****************************************************************************
 *
 * ViewModelManagerHandler
 *
 */

qm::ViewModelManagerHandler::~ViewModelManagerHandler()
{
}


/****************************************************************************
 *
 * ViewModelManagerEvent
 *
 */

qm::ViewModelManagerEvent::ViewModelManagerEvent(const ViewModelManager* pViewModelManager,
												 ViewModel* pNewViewModel,
												 ViewModel* pOldViewModel) :
	pViewModelManager_(pViewModelManager),
	pNewViewModel_(pNewViewModel),
	pOldViewModel_(pOldViewModel)
{
}

qm::ViewModelManagerEvent::~ViewModelManagerEvent()
{
}

const ViewModelManager* qm::ViewModelManagerEvent::getViewModelManager() const
{
	return pViewModelManager_;
}

ViewModel* qm::ViewModelManagerEvent::getNewViewModel() const
{
	return pNewViewModel_;
}

ViewModel* qm::ViewModelManagerEvent::getOldViewModel() const
{
	return pOldViewModel_;
}


/****************************************************************************
 *
 * ViewModelItemComp
 *
 */

qm::ViewModelItemComp::ViewModelItemComp(const ViewModel* pViewModel,
										 const ViewColumn& column,
										 bool bAscending,
										 bool bThread) :
	pViewModel_(pViewModel),
	column_(column),
	bAscending_(bAscending),
	bThread_(bThread)
{
}

qm::ViewModelItemComp::~ViewModelItemComp()
{
}

bool qm::ViewModelItemComp::operator()(const ViewModelItem* pLhs,
									   const ViewModelItem* pRhs) const
{
	bool bLess = false;
	bool bFixed = false;
	if (bThread_) {
		unsigned int nLevelLhs = pLhs->getLevel();
		unsigned int nLevelRhs = pRhs->getLevel();
		if (nLevelLhs < nLevelRhs) {
			for (unsigned int n = 0; n < nLevelRhs - nLevelLhs; ++n)
				pRhs = pRhs->getParentItem();
		}
		else if (nLevelLhs > nLevelRhs) {
			for (unsigned int n = 0; n < nLevelLhs - nLevelRhs; ++n)
				pLhs = pLhs->getParentItem();
		}
		assert(pLhs && pRhs);
		
		if (pLhs == pRhs) {
			if (nLevelLhs < nLevelRhs) {
				bLess = true;
				bFixed = true;
			}
			else if (nLevelLhs > nLevelRhs) {
				bLess = false;
				bFixed = true;
			}
		}
		else {
			ViewModelItem* pParentLhs = pLhs->getParentItem();
			ViewModelItem* pParentRhs = pRhs->getParentItem();
			while (pParentLhs != pParentRhs) {
				pLhs = pParentLhs;
				pRhs = pParentRhs;
				pParentLhs = pParentLhs->getParentItem();
				pParentRhs = pParentRhs->getParentItem();
			}
		}
	}
	
	if (!bFixed) {
		MessageHolder* pmhLhs = pLhs->getMessageHolder();
		MessageHolder* pmhRhs = pRhs->getMessageHolder();
		
		int nComp = 0;
		unsigned int nFlags = column_.getFlags();
		if ((nFlags & ViewColumn::FLAG_SORT_MASK) == ViewColumn::FLAG_SORT_NUMBER) {
			unsigned int nLhs = column_.getNumber(pViewModel_, pmhLhs);
			unsigned int nRhs = column_.getNumber(pViewModel_, pmhRhs);
			nComp = nLhs < nRhs ? -1 : nLhs > nRhs ? 1 : 0;
		}
		else if ((nFlags & ViewColumn::FLAG_SORT_MASK) == ViewColumn::FLAG_SORT_DATE) {
			Time timeLhs;
			column_.getTime(pViewModel_, pmhLhs, &timeLhs);
			Time timeRhs;
			column_.getTime(pViewModel_, pmhRhs, &timeRhs);
			nComp = timeLhs < timeRhs ? -1 : timeLhs > timeRhs ? 1 : 0;
		}
		else {
			wstring_ptr wstrTextLhs(column_.getText(pViewModel_, pmhLhs));
			wstring_ptr wstrTextRhs(column_.getText(pViewModel_, pmhRhs));
			nComp = _wcsicmp(wstrTextLhs.get(), wstrTextRhs.get());
		}
		if (bThread_ && nComp == 0)
			nComp = pmhLhs->getId() < pmhRhs->getId() ? -1 :
				pmhLhs->getId() > pmhRhs->getId() ? 1 : 0;
		if (!bAscending_)
			nComp = -nComp;
		bLess = nComp < 0;
	}
	
	return bLess;
}
