/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmaccount.h>
#include <qmapplication.h>
#include <qmfilenames.h>
#include <qmfolder.h>
#include <qmmacro.h>
#include <qmmessage.h>
#include <qmmessageholder.h>

#include <qsconv.h>
#include <qsfile.h>
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
		swprintf(wsz, L"%u", pmh->getId());
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
		swprintf(wsz, L"%u", pmh->getFlags());
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
	case ViewColumn::TYPE_FROM:
	case ViewColumn::TYPE_TO:
	case ViewColumn::TYPE_FROMTO:
	case ViewColumn::TYPE_SUBJECT:
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
	
	bool bCurrent = false;
	switch (type_) {
	case ViewColumn::TYPE_NONE:
		assert(false);
		break;
	case ViewColumn::TYPE_ID:
		bCurrent = true;
		break;
	case ViewColumn::TYPE_DATE:
		pmh->getDate(pTime);
		break;
	case ViewColumn::TYPE_FROM:
	case ViewColumn::TYPE_TO:
	case ViewColumn::TYPE_FROMTO:
	case ViewColumn::TYPE_SUBJECT:
	case ViewColumn::TYPE_SIZE:
	case ViewColumn::TYPE_FLAGS:
		bCurrent = true;
		break;
	case ViewColumn::TYPE_OTHER:
		{
			MacroValuePtr pValue(pViewModel->getValue(pMacro_.get(), pmh));
			if (pValue.get() && pValue->getType() == MacroValue::TYPE_TIME)
				*pTime = static_cast<MacroValueTime*>(pValue.get())->getTime();
			else
				bCurrent = true;
		}
		break;
	default:
		assert(false);
		break;
	}
	
	if (bCurrent)
		*pTime = Time::getCurrentTime();
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
						 ViewDataItem* pDataItem,
						 Profile* pProfile,
						 Document* pDocument,
						 HWND hwnd,
						 SecurityModel* pSecurityModel,
						 const ColorManager* pColorManager) :
	pViewModelManager_(pViewModelManager),
	pFolder_(pFolder),
	pDataItem_(pDataItem),
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
	assert(pDataItem);
	assert(pProfile);
	
#ifndef NDEBUG
	nLock_ = 0;
#endif
	
	pColorSet_ = pColorManager->getColorSet(pFolder_);
	
	nSort_ = pDataItem_->getSort();
	nFocused_ = pDataItem_->getFocus();
	if ((nSort_ & SORT_INDEX_MASK) >= getColumnCount())
		nSort_ = SORT_ASCENDING | SORT_NOTHREAD | 1;
	
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
}

Folder* qm::ViewModel::getFolder() const
{
	return pFolder_;
}

const ViewColumnList& qm::ViewModel::getColumns() const
{
	return pDataItem_->getColumns();
}

unsigned int qm::ViewModel::getColumnCount() const
{
	return pDataItem_->getColumns().size();
}

const ViewColumn& qm::ViewModel::getColumn(unsigned int n) const
{
	assert(n < pDataItem_->getColumns().size());
	return *pDataItem_->getColumns()[n];
}

ViewColumn& qm::ViewModel::getColumn(unsigned int n)
{
	assert(n < pDataItem_->getColumns().size());
	return *pDataItem_->getColumns()[n];
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
	return it == listItem_.end() ? -1 : it - listItem_.begin();
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
	pDataItem_->setSort(nSort_);
	pDataItem_->setFocus(nFocused_);
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
	if (nIndex != -1) {
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
	nItem_(-1)
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
	std::for_each(mapViewData_.begin(), mapViewData_.end(),
		unary_compose_f_gx(
			deleter<ViewData>(),
			std::select2nd<ViewDataMap::value_type>()));
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
	
	ViewDataItem* pViewDataItem = getViewDataItem(pFolder);
	std::auto_ptr<ViewModel> pViewModel(new ViewModel(this, pFolder, pViewDataItem,
		pProfile_, pDocument_, hwnd_, pSecurityModel_, pColorManager_.get()));
	listViewModel_.push_back(pViewModel.get());
	return pViewModel.release();
}

bool qm::ViewModelManager::save() const
{
	for (ViewModelList::const_iterator it = listViewModel_.begin(); it != listViewModel_.end(); ++it) {
		if (!(*it)->save())
			return false;
	}
	
	for (ViewDataMap::const_iterator it = mapViewData_.begin(); it != mapViewData_.end(); ++it) {
		if (!(*it).second->save())
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
	Folder* pFolder = pViewModel->getFolder();
	ViewDataMap::iterator itD = std::find_if(
		mapViewData_.begin(), mapViewData_.end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				std::equal_to<Account*>(),
				std::select1st<ViewDataMap::value_type>(),
				std::identity<Account*>()),
			pFolder->getAccount()));
	if (itD != mapViewData_.end())
		(*itD).second->removeItem(pFolder->getId());
	
	ViewModelList::iterator itM = std::remove(listViewModel_.begin(),
		listViewModel_.end(), pViewModel);
	listViewModel_.erase(itM, listViewModel_.end());
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
	
	ViewDataMap::iterator itD = std::find_if(
		mapViewData_.begin(), mapViewData_.end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				std::equal_to<Account*>(),
				std::select1st<ViewDataMap::value_type>(),
				std::identity<Account*>()),
			pAccount));
	if (itD != mapViewData_.end()) {
		delete (*itD).second;
		mapViewData_.erase(itD);
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

ViewDataItem* qm::ViewModelManager::getViewDataItem(Folder* pFolder)
{
	assert(pFolder);
	
	ViewData* pViewData = 0;
	Account* pAccount = pFolder->getAccount();
	ViewDataMap::iterator it = std::find_if(
		mapViewData_.begin(), mapViewData_.end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				std::equal_to<Account*>(),
				std::select1st<ViewDataMap::value_type>(),
				std::identity<Account*>()),
			pAccount));
	if (it != mapViewData_.end()) {
		pViewData = (*it).second;
	}
	else {
		wstring_ptr wstrPath(getViewsPath(pAccount));
		std::auto_ptr<ViewData> pNewViewData(new ViewData(wstrPath.get()));
		mapViewData_.push_back(std::make_pair(pAccount, pNewViewData.get()));
		pAccount->addAccountHandler(this);
		pViewData = pNewViewData.release();
		
		// TODO
		// Just for compatibility.
		// This should be removed after 2.9.6 is released.
		W2T(wstrPath.get(), ptszPath);
		if (::GetFileAttributes(ptszPath) == 0xffffffff) {
			wstring_ptr wstrProfilePath(concat(pAccount->getPath(), L"\\", FileNames::VIEW_XML));
			std::auto_ptr<XMLProfile> pProfile(new XMLProfile(wstrProfilePath.get()));
			if (pProfile->load()) {
				MacroParser parser(MacroParser::TYPE_COLUMN);
				const Account::FolderList& l = pAccount->getFolders();
				for (Account::FolderList::const_iterator it = l.begin(); it != l.end(); ++it) {
					unsigned int nId = (*it)->getId();
					
					std::auto_ptr<ViewDataItem> pItem(new ViewDataItem(nId));
					
					WCHAR wszSection[32];
					swprintf(wszSection, L"Folder%d", nId);
					
					pItem->setSort(pProfile->getInt(wszSection, L"Sort",
						ViewModel::SORT_ASCENDING | ViewModel::SORT_NOTHREAD | 1));
					pItem->setFocus(pProfile->getInt(wszSection, L"Focus", 0));
					
					int nColumn = 0;
					while (true) {
						wstring_ptr wstrTitle;
						ViewColumn::Type type = ViewColumn::TYPE_NONE;
						std::auto_ptr<Macro> pMacro;
						int nFlags = 0;
						int nWidth = 0;
						
						WCHAR wszKey[32];
						
						swprintf(wszKey, L"ColumnTitle%d", nColumn);
						wstrTitle = pProfile->getString(wszSection, wszKey, 0);
						
						swprintf(wszKey, L"ColumnMacro%d", nColumn);
						wstring_ptr wstrMacro(pProfile->getString(wszSection, wszKey, 0));
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
							if (!pMacro.get())
								return false;
							type = ViewColumn::TYPE_OTHER;
						}
						
						swprintf(wszKey, L"ColumnFlags%d", nColumn);
						nFlags = pProfile->getInt(wszSection, wszKey, 0);
						
						swprintf(wszKey, L"ColumnWidth%d", nColumn);
						nWidth = pProfile->getInt(wszSection, wszKey, 0);
						
						std::auto_ptr<ViewColumn> pColumn(new ViewColumn(
							wstrTitle.get(), type, pMacro, nFlags, nWidth));
						pItem->addColumn(pColumn);
						
						++nColumn;
					}
					if (nColumn != 0)
						pViewData->addItem(pItem);
				}
			}
		}
	}
	
	return pViewData->getItem(pFolder->getId());
}

wstring_ptr qm::ViewModelManager::getViewsPath(Account* pAccount)
{
	const WCHAR* pwszProfileName = Application::getApplication().getProfileName();
	if (*pwszProfileName) {
		ConcatW c[] = {
			{ pAccount->getPath(),		-1	},
			{ L"\\",					1	},
			{ FileNames::VIEWS,			-1	},
			{ L"_",						1	},
			{ pwszProfileName,			-1	},
			{ FileNames::XML_EXT,		-1	}
		};
		return concat(c, countof(c));
	}
	else {
		return concat(pAccount->getPath(), L"\\", FileNames::VIEWS_XML);
	}
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


/****************************************************************************
 *
 * ViewData
 *
 */

qm::ViewData::ViewData(const WCHAR* pwszPath)
{
	wstrPath_ = allocWString(pwszPath);
	
	XMLReader reader;
	ViewDataContentHandler contentHandler(this);
	reader.setContentHandler(&contentHandler);
	if (!reader.parse(wstrPath_.get())) {
		// TODO
	}
}

qm::ViewData::~ViewData()
{
	std::for_each(listItem_.begin(), listItem_.end(), deleter<ViewDataItem>());
}

const ViewData::ItemList& qm::ViewData::getItems() const
{
	return listItem_;
}

ViewDataItem* qm::ViewData::getItem(unsigned int nFolderId)
{
	ViewDataItem item(nFolderId);
	ItemList::iterator it = std::lower_bound(
		listItem_.begin(), listItem_.end(),
		&item,
		binary_compose_f_gx_hy(
			std::less<unsigned int>(),
			std::mem_fun(&ViewDataItem::getFolderId),
			std::mem_fun(&ViewDataItem::getFolderId)));
	if (it != listItem_.end() && (*it)->getFolderId() == nFolderId) {
		return *it;
	}
	else {
		std::auto_ptr<ViewDataItem> pItem(createDefaultItem(nFolderId));
		listItem_.insert(it, pItem.get());
		return pItem.release();
	}
}

bool qm::ViewData::save() const
{
	TemporaryFileRenamer renamer(wstrPath_.get());
	
	FileOutputStream os(renamer.getPath());
	if (!os)
		return false;
	OutputStreamWriter writer(&os, false, L"utf-8");
	if (!writer)
		return false;
	BufferedWriter bufferedWriter(&writer, false);
	
	ViewDataWriter viewDataWriter(&bufferedWriter);
	if (!viewDataWriter.write(this))
		return false;
	
	if (!bufferedWriter.close())
		return false;
	
	if (!renamer.rename())
		return false;
	
	return true;
}

void qm::ViewData::addItem(std::auto_ptr<ViewDataItem> pItem)
{
	ItemList::iterator it = std::lower_bound(
		listItem_.begin(), listItem_.end(),
		pItem.get(),
		binary_compose_f_gx_hy(
			std::less<unsigned int>(),
			std::mem_fun(&ViewDataItem::getFolderId),
			std::mem_fun(&ViewDataItem::getFolderId)));
	if (it == listItem_.end() || (*it)->getFolderId() != pItem->getFolderId()) {
		listItem_.insert(it, pItem.get());
		pItem.release();
	}
}

void qm::ViewData::removeItem(unsigned int nFolderId)
{
	ViewDataItem item(nFolderId);
	ItemList::iterator it = std::lower_bound(
		listItem_.begin(), listItem_.end(),
		&item,
		binary_compose_f_gx_hy(
			std::less<unsigned int>(),
			std::mem_fun(&ViewDataItem::getFolderId),
			std::mem_fun(&ViewDataItem::getFolderId)));
	if (it != listItem_.end() && (*it)->getFolderId() == nFolderId) {
		delete *it;
		listItem_.erase(it);
	}
}

std::auto_ptr<ViewDataItem> qm::ViewData::createDefaultItem(unsigned int nFolderId)
{
	std::auto_ptr<ViewDataItem> pItem(new ViewDataItem(nFolderId));
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
		pItem->addColumn(pColumn);
	}
	return pItem;
}


/****************************************************************************
 *
 * ViewDataItem
 *
 */

qm::ViewDataItem::ViewDataItem(unsigned int nFolderId) :
	nFolderId_(nFolderId),
	nFocus_(0),
	nSort_(ViewModel::SORT_ASCENDING | ViewModel::SORT_NOTHREAD | 1)
{
}

qm::ViewDataItem::~ViewDataItem()
{
	std::for_each(listColumn_.begin(), listColumn_.end(), deleter<ViewColumn>());
}

unsigned int qm::ViewDataItem::getFolderId() const
{
	return nFolderId_;
}

const ViewColumnList& qm::ViewDataItem::getColumns() const
{
	return listColumn_;
}

void qm::ViewDataItem::addColumn(std::auto_ptr<ViewColumn> pColumn)
{
	listColumn_.push_back(pColumn.get());
	pColumn.release();
}

unsigned int qm::ViewDataItem::getFocus() const
{
	return nFocus_;
}

void qm::ViewDataItem::setFocus(unsigned int nFocus)
{
	nFocus_ = nFocus;
}

unsigned int qm::ViewDataItem::getSort() const
{
	return nSort_;
}

void qm::ViewDataItem::setSort(unsigned int nSort)
{
	nSort_ = nSort;
}


/****************************************************************************
*
* ViewDataContentHandler
*
*/

qm::ViewDataContentHandler::ViewDataContentHandler(ViewData* pData) :
	pData_(pData),
	state_(STATE_ROOT),
	type_(ViewColumn::TYPE_NONE),
	nFlags_(-1),
	nWidth_(-1),
	nSort_(-1),
	parser_(MacroParser::TYPE_COLUMN)
{
}

qm::ViewDataContentHandler::~ViewDataContentHandler()
{
}

bool qm::ViewDataContentHandler::startElement(const WCHAR* pwszNamespaceURI,
											  const WCHAR* pwszLocalName,
											  const WCHAR* pwszQName,
											  const qs::Attributes& attributes)
{
	if (wcscmp(pwszLocalName, L"views") == 0) {
		if (state_ != STATE_ROOT)
			return false;
		if (attributes.getLength() != 0)
			return false;
		state_ = STATE_VIEWS;
	}
	else if (wcscmp(pwszLocalName, L"view") == 0) {
		if (state_ != STATE_VIEWS)
			return false;
		
		const WCHAR* pwszFolder = 0;
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrLocalName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrLocalName, L"folder") == 0)
				pwszFolder = attributes.getValue(n);
			else
				return false;
		}
		if (!pwszFolder)
			return false;
		
		WCHAR* pEnd = 0;
		unsigned int nFolderId = wcstol(pwszFolder, &pEnd, 10);
		if (*pEnd)
			return false;
		
		assert(!pItem_.get());
		pItem_.reset(new ViewDataItem(nFolderId));
		
		state_ = STATE_VIEW;
	}
	else if (wcscmp(pwszLocalName, L"column") == 0) {
		if (state_ != STATE_COLUMNS)
			return false;
		
		unsigned int nFlags = 0;
		const WCHAR* pwszSort = 0;
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrLocalName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrLocalName, L"indent") == 0)
				nFlags |= wcscmp(attributes.getValue(n), L"true") == 0 ?
					ViewColumn::FLAG_INDENT : 0;
			else if (wcscmp(pwszAttrLocalName, L"line") == 0)
				nFlags |= wcscmp(attributes.getValue(n), L"true") == 0 ?
					ViewColumn::FLAG_LINE : 0;
			else if (wcscmp(pwszAttrLocalName, L"align") == 0)
				nFlags |= wcscmp(attributes.getValue(n), L"right") == 0 ?
					ViewColumn::FLAG_RIGHTALIGN : 0;
			else if (wcscmp(pwszAttrLocalName, L"icon") == 0)
				nFlags |= wcscmp(attributes.getValue(n), L"true") == 0 ?
					ViewColumn::FLAG_ICON : 0;
			else if (wcscmp(pwszAttrLocalName, L"sort") == 0)
				pwszSort = attributes.getValue(n);
			else
				return false;
		}
		if (pwszSort) {
			if (wcscmp(pwszSort, L"number") == 0)
				nFlags |= ViewColumn::FLAG_SORT_NUMBER;
			else if (wcscmp(pwszSort, L"date") == 0)
				nFlags |= ViewColumn::FLAG_SORT_DATE;
			else
				nFlags |= ViewColumn::FLAG_SORT_TEXT;
		}
		else {
			nFlags |= ViewColumn::FLAG_SORT_TEXT;
		}
		nFlags_ = nFlags;
		
		state_ = STATE_COLUMN;
	}
	else if (wcscmp(pwszLocalName, L"sort") == 0) {
		if (state_ != STATE_VIEW)
			return false;
		
		unsigned int nSort = 0;
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrLocalName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrLocalName, L"direction") == 0)
				nSort |= wcscmp(attributes.getValue(n), L"ascending") == 0 ?
					ViewModel::SORT_ASCENDING : ViewModel::SORT_DESCENDING;
			else if (wcscmp(pwszAttrLocalName, L"thread") == 0)
				nSort |= wcscmp(attributes.getValue(n), L"true") == 0 ?
					ViewModel::SORT_THREAD : ViewModel::SORT_NOTHREAD;
			else
				return false;
		}
		nSort_ = nSort;
		
		state_ = STATE_SORT;
	}
	else {
		struct {
			const WCHAR* pwszLocalName_;
			State stateBefore_;
			State stateAfter_;
		} items[] = {
			{ L"columns",	STATE_VIEW,		STATE_COLUMNS	},
			{ L"title",		STATE_COLUMN,	STATE_TITLE		},
			{ L"macro",		STATE_COLUMN,	STATE_MACRO		},
			{ L"width",		STATE_COLUMN,	STATE_WIDTH		},
			{ L"focus",		STATE_VIEW,		STATE_FOCUS		},
		};
		int n = 0;
		while (n < countof(items) && wcscmp(pwszLocalName, items[n].pwszLocalName_) != 0)
			++n;
		if (n == countof(items))
			return false;
		
		if (state_ != items[n].stateBefore_)
			return false;
		if (attributes.getLength() != 0)
			return false;
		state_ = items[n].stateAfter_;
	}
	return true;
}

bool qm::ViewDataContentHandler::endElement(const WCHAR* pwszNamespaceURI,
											const WCHAR* pwszLocalName,
											const WCHAR* pwszQName)
{
	if (wcscmp(pwszLocalName, L"views") == 0) {
		assert(state_ == STATE_VIEWS);
		state_ = STATE_ROOT;
	}
	else if (wcscmp(pwszLocalName, L"view") == 0) {
		assert(state_ == STATE_VIEW);
		assert(pItem_.get());
		pData_->addItem(pItem_);
		state_ = STATE_VIEWS;
	}
	else if (wcscmp(pwszLocalName, L"columns") == 0) {
		assert(state_ == STATE_COLUMNS);
		state_ = STATE_VIEW;
	}
	else if (wcscmp(pwszLocalName, L"column") == 0) {
		assert(state_ == STATE_COLUMN);
		
		if (!wstrTitle_.get() ||
			type_ == ViewColumn::TYPE_NONE ||
			nFlags_ == -1 ||
			nWidth_ == -1)
			return false;
		
		std::auto_ptr<ViewColumn> pColumn(new ViewColumn(
			wstrTitle_.get(), type_, pMacro_, nFlags_, nWidth_));
		assert(pItem_.get());
		pItem_->addColumn(pColumn);
		
		wstrTitle_.reset(0);
		type_ = ViewColumn::TYPE_NONE;
		nFlags_ = -1;
		nWidth_ = -1;
		
		state_ = STATE_COLUMNS;
	}
	else if (wcscmp(pwszLocalName, L"title") == 0) {
		assert(state_ == STATE_TITLE);
		assert(!wstrTitle_.get());
		wstrTitle_ = buffer_.getString();
		state_ = STATE_COLUMN;
	}
	else if (wcscmp(pwszLocalName, L"macro") == 0) {
		assert(state_ == STATE_MACRO);
		
		type_ = ViewColumn::TYPE_NONE;
		const WCHAR* pwszMacro = buffer_.getCharArray();
		if (*pwszMacro == L'%') {
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
			for (int n = 0; n < countof(defaults) && type_ == ViewColumn::TYPE_NONE; ++n) {
				if (_wcsicmp(pwszMacro, defaults[n].pwszMacro_) == 0)
					type_ = defaults[n].type_;
			}
		}
		if (type_ == ViewColumn::TYPE_NONE) {
			pMacro_ = parser_.parse(pwszMacro);
			if (!pMacro_.get())
				return false;
			type_ = ViewColumn::TYPE_OTHER;
		}
		
		buffer_.remove();
		state_ = STATE_COLUMN;
	}
	else if (wcscmp(pwszLocalName, L"width") == 0) {
		assert(state_ == STATE_WIDTH);
		
		WCHAR* pEnd = 0;
		nWidth_ = wcstol(buffer_.getCharArray(), &pEnd, 10);
		if (*pEnd)
			return false;
		
		buffer_.remove();
		state_ = STATE_COLUMN;
	}
	else if (wcscmp(pwszLocalName, L"focus") == 0) {
		assert(state_ == STATE_FOCUS);
		
		WCHAR* pEnd = 0;
		unsigned int nFocus = wcstol(buffer_.getCharArray(), &pEnd, 10);
		if (*pEnd)
			return false;
		assert(pItem_.get());
		pItem_->setFocus(nFocus);
		
		buffer_.remove();
		state_ = STATE_VIEW;
	}
	else if (wcscmp(pwszLocalName, L"sort") == 0) {
		assert(state_ == STATE_SORT);
		
		WCHAR* pEnd = 0;
		unsigned int nSort = wcstol(buffer_.getCharArray(), &pEnd, 10);
		if (*pEnd)
			return false;
		assert(pItem_.get());
		pItem_->setSort(nSort | nSort_);
		
		nSort_ = -1;
		
		buffer_.remove();
		state_ = STATE_VIEW;
	}
	else {
		return false;
	}
	return true;
}

bool qm::ViewDataContentHandler::characters(const WCHAR* pwsz,
											size_t nStart,
											size_t nLength)
{
	if (state_ == STATE_TITLE ||
		state_ == STATE_MACRO ||
		state_ == STATE_WIDTH ||
		state_ == STATE_FOCUS ||
		state_ == STATE_SORT) {
		buffer_.append(pwsz + nStart, nLength);
	}
	else {
		const WCHAR* p = pwsz + nStart;
		for (size_t n = 0; n < nLength; ++n, ++p) {
			if (*p != L' ' && *p != L'\t' && *p != L'\n')
				return false;
		}
	}
	return true;
}


/****************************************************************************
 *
 * ViewDataWriter
 *
 */

qm::ViewDataWriter::ViewDataWriter(qs::Writer* pWriter) :
	handler_(pWriter)
{
}

qm::ViewDataWriter::~ViewDataWriter()
{
}

bool qm::ViewDataWriter::write(const ViewData* pData)
{
	DefaultAttributes attrs;
	
	if (!handler_.startDocument())
		return false;
	if (!handler_.startElement(0, 0, L"views", attrs))
		return false;
	
	const ViewData::ItemList& listItem = pData->getItems();
	for (ViewData::ItemList::const_iterator itI = listItem.begin(); itI != listItem.end(); ++itI) {
		ViewDataItem* pItem = *itI;
		
		class ViewAttrs : public DefaultAttributes
		{
		public:
			ViewAttrs(unsigned int nFolder)
			{
				swprintf(wszFolder_, L"%u", nFolder);
			}
		
		public:
			virtual int getLength() const
			{
				return 1;
			}
			
			virtual const WCHAR* getQName(int nIndex) const
			{
				assert(nIndex == 0);
				return L"folder";
			}
			
			virtual const WCHAR* getValue(int nIndex) const
			{
				assert(nIndex == 0);
				return wszFolder_;
			}
		
		private:
			WCHAR wszFolder_[32];
		} viewAttrs(pItem->getFolderId());
		
		if (!handler_.startElement(0, 0, L"view", viewAttrs))
			return false;
		
		if (!handler_.startElement(0, 0, L"columns", attrs))
			return false;
		
		const ViewColumnList& listColumn = pItem->getColumns();
		for (ViewColumnList::const_iterator itC = listColumn.begin(); itC != listColumn.end(); ++itC) {
			ViewColumn* pColumn = *itC;
			
			class ColumnAttrs : public DefaultAttributes
			{
			public:
				ColumnAttrs(unsigned int nFlags) :
					nFlags_(nFlags)
				{
				}
				
			public:
				virtual int getLength() const
				{
					return 5;
				}
				
				virtual const WCHAR* getQName(int nIndex) const
				{
					switch (nIndex) {
					case 0:
						return L"indent";
					case 1:
						return L"line";
					case 2:
						return L"icon";
					case 3:
						return L"align";
					case 4:
						return L"sort";
					default:
						assert(false);
						return 0;
					}
				}
				
				virtual const WCHAR* getValue(int nIndex) const
				{
					switch (nIndex) {
					case 0:
						return nFlags_ & ViewColumn::FLAG_INDENT ? L"true" : L"false";
					case 1:
						return nFlags_ & ViewColumn::FLAG_LINE ? L"true" : L"false";
					case 2:
						return nFlags_ & ViewColumn::FLAG_ICON ? L"true" : L"false";
					case 3:
						return nFlags_ & ViewColumn::FLAG_RIGHTALIGN ? L"right" : L"left";
					case 4:
						switch (nFlags_ & ViewColumn::FLAG_SORT_MASK) {
						case ViewColumn::FLAG_SORT_TEXT:
							return L"text";
						case ViewColumn::FLAG_SORT_NUMBER:
							return L"number";
						case ViewColumn::FLAG_SORT_DATE:
							return L"date";
						default:
							assert(false);
							return 0;
						}
					default:
						assert(false);
						return 0;
					}
				}
			
			private:
				unsigned int nFlags_;
			} columnAttrs(pColumn->getFlags());
			
			if (!handler_.startElement(0, 0, L"column", columnAttrs))
				return false;
			
			if (!HandlerHelper::textElement(&handler_, L"title", pColumn->getTitle(), -1))
				return false;
			
			const WCHAR* pwszMacro = 0;
			wstring_ptr wstrMacro;
			switch (pColumn->getType()) {
			case ViewColumn::TYPE_NONE:
				assert(false);
				return false;
			case ViewColumn::TYPE_ID:
				pwszMacro = L"%Id";
				break;
			case ViewColumn::TYPE_DATE:
				pwszMacro = L"%Date";
				break;
			case ViewColumn::TYPE_FROM:
				pwszMacro = L"%From";
				break;
			case ViewColumn::TYPE_TO:
				pwszMacro = L"%To";
				break;
			case ViewColumn::TYPE_FROMTO:
				pwszMacro = L"%FromTo";
				break;
			case ViewColumn::TYPE_SUBJECT:
				pwszMacro = L"%Subject";
				break;
			case ViewColumn::TYPE_SIZE:
				pwszMacro = L"%Size";
				break;
			case ViewColumn::TYPE_FLAGS:
				pwszMacro = L"%Flags";
				break;
			case ViewColumn::TYPE_OTHER:
				wstrMacro = pColumn->getMacro()->getString();
				pwszMacro = wstrMacro.get();
				break;
			default:
				assert(false);
				return false;
			}
			if (!HandlerHelper::textElement(&handler_, L"macro", pwszMacro, -1))
				return false;
			
			if (!HandlerHelper::numberElement(&handler_, L"width", pColumn->getWidth()))
				return false;
			
			if (!handler_.endElement(0, 0, L"column"))
				return false;
		}
		
		if (!handler_.endElement(0, 0, L"columns"))
			return false;
		
		if (!HandlerHelper::numberElement(&handler_, L"focus", pItem->getFocus()))
			return false;
		
		class SortAttrs : public DefaultAttributes
		{
		public:
			SortAttrs(unsigned int nSort) :
				nSort_(nSort)
			{
			}
			
		public:
			virtual int getLength() const
			{
				return 2;
			}
			
			virtual const WCHAR* getQName(int nIndex) const
			{
				switch (nIndex) {
				case 0:
					return L"direction";
				case 1:
					return L"thread";
				default:
					assert(false);
					return 0;
				}
			}
			
			virtual const WCHAR* getValue(int nIndex) const
			{
				switch (nIndex) {
				case 0:
					return (nSort_ & ViewModel::SORT_DIRECTION_MASK) == ViewModel::SORT_ASCENDING ?
						L"ascending" : L"descending";
				case 1:
					return (nSort_ & ViewModel::SORT_THREAD_MASK) == ViewModel::SORT_THREAD ?
						L"true" : L"false";
				default:
					assert(false);
					return 0;
				}
			}
		
		private:
			unsigned int nSort_;
		} sortAttrs(pItem->getSort());
		
		if (!handler_.startElement(0, 0, L"sort", sortAttrs))
			return false;
		WCHAR wsz[32];
		swprintf(wsz, L"%u", pItem->getSort() & ViewModel::SORT_INDEX_MASK);
		if (!handler_.characters(wsz, 0, wcslen(wsz)))
			return false;
		if (!handler_.endElement(0, 0, L"sort"))
			return false;
		
		if (!handler_.endElement(0, 0, L"view"))
			return false;
	}
	
	if (!handler_.endElement(0, 0, L"views"))
		return false;
	if (!handler_.endDocument())
		return false;
	
	return true;
}
