/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmaccount.h>
#include <qmextensions.h>
#include <qmfolder.h>
#include <qmmacro.h>
#include <qmmessage.h>
#include <qmmessageholder.h>

#include <qsstl.h>
#include <qserror.h>
#include <qsnew.h>

#include <algorithm>

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

qm::ViewColumn::ViewColumn(const Init& init, QSTATUS* pstatus) :
	wstrTitle_(0),
	type_(init.type_),
	pMacro_(0),
	nFlags_(init.nFlags_),
	nWidth_(init.nWidth_)
{
	assert(pstatus);
	
	*pstatus = QSTATUS_SUCCESS;
	
	wstrTitle_ = allocWString(init.pwszTitle_);
	if (!wstrTitle_) {
		*pstatus = QSTATUS_OUTOFMEMORY;
		return;
	}
	pMacro_ = init.pMacro_;
}

qm::ViewColumn::~ViewColumn()
{
	freeWString(wstrTitle_);
	delete pMacro_;
}

const WCHAR* qm::ViewColumn::getTitle() const
{
	return wstrTitle_;
}

ViewColumn::Type qm::ViewColumn::getType() const
{
	return type_;
}

const Macro* qm::ViewColumn::getMacro() const
{
	return pMacro_;
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

QSTATUS qm::ViewColumn::getText(MessageHolder* pmh, WSTRING* pwstrValue) const
{
	assert(pmh);
	assert(pwstrValue);
	
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrText;
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
			status = pmh->getDate(&t);
			CHECK_QSTATUS();
			status = t.format(L"%Y2/%M0/%D %h:%m",
				Time::FORMAT_LOCAL, &wstrText);
			CHECK_QSTATUS();
		}
		break;
	case ViewColumn::TYPE_FROM:
		status = pmh->getFrom(&wstrText);
		CHECK_QSTATUS();
		break;
	case ViewColumn::TYPE_TO:
		status = pmh->getTo(&wstrText);
		CHECK_QSTATUS();
		break;
	case ViewColumn::TYPE_FROMTO:
		status = pmh->getFromTo(&wstrText);
		CHECK_QSTATUS();
		break;
	case ViewColumn::TYPE_SUBJECT:
		status = pmh->getSubject(&wstrText);
		CHECK_QSTATUS();
		break;
	case ViewColumn::TYPE_SIZE:
		swprintf(wsz, L"%dKB", pmh->getSize()/1024 + 1);
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
	if (!wstrText.get()) {
		wstrText.reset(allocWString(wsz));
		if (!wstrText.get())
			return QSTATUS_OUTOFMEMORY;
	}
	*pwstrValue = wstrText.release();
	
	return QSTATUS_SUCCESS;
}

unsigned int qm::ViewColumn::getNumber(MessageHolder* pmh) const
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
		// TODO
		break;
	default:
		assert(false);
		break;
	}
	return nValue;
}

QSTATUS qm::ViewColumn::getTime(MessageHolder* pmh, Time* pTime) const
{
	assert(pmh);
	assert(pTime);
	
	DECLARE_QSTATUS();
	
	switch (type_) {
	case ViewColumn::TYPE_NONE:
		assert(false);
		break;
	case ViewColumn::TYPE_ID:
		// TODO
		break;
	case ViewColumn::TYPE_DATE:
		status = pmh->getDate(pTime);
		CHECK_QSTATUS();
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
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * ViewModel::SelectionRestorer
 *
 */

qm::ViewModel::SelectionRestorer::SelectionRestorer(ViewModel* pViewModel,
	bool bRefresh, bool bIgnore, QSTATUS* pstatus) :
	pViewModel_(pViewModel),
	bRefresh_(bRefresh),
	pmhFocused_(0),
	pmhLastSelection_(0)
{
	assert(pViewModel);
	assert(pstatus);
	assert(pViewModel->isLocked());
	
	*pstatus = QSTATUS_SUCCESS;
	
	if (!bIgnore) {
		ViewModel::ItemList& l = pViewModel_->listItem_;
		if (!l.empty()) {
			ViewModelItem* pItemFocused = l[pViewModel_->nFocused_];
			assert(pItemFocused->getFlags() & ViewModel::FLAG_FOCUSED);
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
			ViewModel::FLAG_FOCUSED, ViewModel::FLAG_FOCUSED);
		assert(pViewModel_->nLastSelection_ < l.size());
		l[pViewModel_->nLastSelection_]->setFlags(
			ViewModel::FLAG_SELECTED, ViewModel::FLAG_SELECTED);
	}
	
	assert(l.empty() || l[pViewModel_->nFocused_]->getFlags() & ViewModel::FLAG_FOCUSED);
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
 * ViewModel
 *
 */

qm::ViewModel::ViewModel(ViewModelManager* pViewModelManager,
	Folder* pFolder, Profile* pProfile, Document* pDocument,
	HWND hwnd, const ColorManager* pColorManager, QSTATUS* pstatus) :
	pViewModelManager_(pViewModelManager),
	pFolder_(pFolder),
	pProfile_(pProfile),
	pDocument_(pDocument),
	hwnd_(hwnd),
	pColorSet_(0),
	nUnseenCount_(0),
	nSort_(SORT_ASCENDING | SORT_NOTHREAD),
	pFilter_(0),
	nLastSelection_(0),
	nFocused_(0)
{
	assert(pFolder);
	assert(pProfile);
	assert(pstatus);
	
	DECLARE_QSTATUS();
	
	*pstatus = QSTATUS_SUCCESS;
	
#ifndef NDEBUG
	nLock_ = 0;
#endif
	
	status = pColorManager->getColorSet(pFolder_, &pColorSet_);
	CHECK_QSTATUS_SET(pstatus);
	
	WCHAR wszSection[32];
	swprintf(wszSection, L"Folder%d", pFolder_->getId());
	status = pProfile_->getInt(wszSection, L"Sort",
		SORT_ASCENDING | SORT_NOTHREAD | 1, reinterpret_cast<int*>(&nSort_));
	CHECK_QSTATUS_SET(pstatus);
	status = pProfile_->getInt(wszSection, L"Focus", 0,
		reinterpret_cast<int*>(&nFocused_));
	CHECK_QSTATUS_SET(pstatus);
	
	status = loadColumns();
	CHECK_QSTATUS_SET(pstatus);
	if ((nSort_ & SORT_INDEX_MASK) >= getColumnCount())
		nSort_ = 0;
	
	Lock<ViewModel> lock(*this);
	
	status = update(false);
	CHECK_QSTATUS_SET(pstatus);
	
	if (nFocused_ >= listItem_.size())
		nFocused_ = listItem_.empty() ? 0 : listItem_.size() - 1;
	if (!listItem_.empty()) {
		nLastSelection_ = nFocused_;
		listItem_[nFocused_]->setFlags(FLAG_SELECTED | FLAG_FOCUSED,
			FLAG_SELECTED | FLAG_FOCUSED);
	}
	
	status = pFolder_->addFolderHandler(this);
	CHECK_QSTATUS_SET(pstatus);
}

qm::ViewModel::~ViewModel()
{
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
	
	DECLARE_QSTATUS();
	
	ViewModelItem* pItem = listItem_[n];
	MessageHolder* pmh = pItem->getMessageHolder();
	if (pItem->getColor() == 0xffffffff ||
		pItem->getMessageFlags() != pmh->getFlags()) {
		COLORREF cr = 0xff000000;
		pItem->setMessageFlags(pmh->getFlags());
		if (pColorSet_) {
			Message msg(&status);
			if (status == QSTATUS_SUCCESS) {
				MacroContext::Init init = {
					pmh,
					&msg,
					pFolder_->getAccount(),
					pDocument_,
					hwnd_,
					pProfile_,
					true,
					0,
					0
				};
				MacroContext context(init, &status);
				if (status == QSTATUS_SUCCESS)
					pColorSet_->getColor(&context, &cr);
			}
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

QSTATUS qm::ViewModel::setSort(unsigned int nSort)
{
	assert(nSort_ & SORT_DIRECTION_MASK);
	assert(nSort_ & SORT_THREAD_MASK);
	
	DECLARE_QSTATUS();
	
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
	
	status = sort(nSort, true, true);
	CHECK_QSTATUS();
	
	nSort_ = nSort;
	
	fireSorted();
	
	return QSTATUS_SUCCESS;
}

unsigned int qm::ViewModel::getSort() const
{
	return nSort_;
}

QSTATUS qm::ViewModel::setFilter(const Filter* pFilter)
{
	DECLARE_QSTATUS();
	
	if (pFilter != pFilter_) {
		pFilter_ = pFilter;
		status = update(true);
		CHECK_QSTATUS();
	}
	return QSTATUS_SUCCESS;
}

const Filter* qm::ViewModel::getFilter() const
{
	return pFilter_;
}

QSTATUS qm::ViewModel::addSelection(unsigned int n)
{
	assert(isLocked());
	assert(n < getCount());
	
	DECLARE_QSTATUS();
	
	ViewModelItem* pItem = listItem_[n];
	if (!(pItem->getFlags() & FLAG_SELECTED)) {
		pItem->setFlags(FLAG_SELECTED, FLAG_SELECTED);
		status = fireItemStateChanged(n);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ViewModel::addSelection(unsigned int nStart, unsigned int nEnd)
{
	assert(isLocked());
	assert(nStart < getCount());
	assert(nEnd < getCount());
	
	DECLARE_QSTATUS();
	
	if (nStart > nEnd)
		std::swap(nStart, nEnd);
	
	while (nStart <= nEnd) {
		status = addSelection(nStart);
		++nStart;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ViewModel::removeSelection(unsigned int n)
{
	assert(isLocked());
	assert(n < getCount());
	
	DECLARE_QSTATUS();
	
	ViewModelItem* pItem = listItem_[n];
	if (pItem->getFlags() & FLAG_SELECTED) {
		pItem->setFlags(0, FLAG_SELECTED);
		status = fireItemStateChanged(n);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ViewModel::setSelection(unsigned int n)
{
	assert(isLocked());
	
	DECLARE_QSTATUS();
	
	status = clearSelection();
	CHECK_QSTATUS();
	
	status = addSelection(n);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ViewModel::setSelection(unsigned int nStart, unsigned int nEnd)
{
	assert(isLocked());
	assert(nStart < getCount());
	assert(nEnd < getCount());
	
	DECLARE_QSTATUS();
	
	if (nStart > nEnd)
		std::swap(nStart, nEnd);
	
	unsigned int n = 0;
	while (n < nStart) {
		if (isSelected(n)) {
			status = removeSelection(n);
			CHECK_QSTATUS();
		}
		++n;
	}
	while (n <= nEnd) {
		if (!isSelected(n)) {
			status = addSelection(n);
			CHECK_QSTATUS();
		}
		++n;
	}
	while (n < listItem_.size()) {
		if (isSelected(n)) {
			status = removeSelection(n);
			CHECK_QSTATUS();
		}
		++n;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ViewModel::clearSelection()
{
	assert(isLocked());
	
	DECLARE_QSTATUS();
	
	for (ItemList::size_type n = 0; n < listItem_.size(); ++n) {
		status = removeSelection(n);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ViewModel::getSelection(Folder::MessageHolderList* pList) const
{
	assert(pList);
	assert(isLocked());
	
	DECLARE_QSTATUS();
	
	ItemList::const_iterator it = listItem_.begin();
	while (it != listItem_.end()) {
		if ((*it)->getFlags() & FLAG_SELECTED) {
			status = STLWrapper<Folder::MessageHolderList>(
				*pList).push_back((*it)->getMessageHolder());
			CHECK_QSTATUS();
		}
		++it;
	}
	
	return QSTATUS_SUCCESS;
}

bool qm::ViewModel::hasSelection() const
{
	assert(isLocked());
	
	ItemList::const_iterator it = listItem_.begin();
	while (it != listItem_.end() && !((*it)->getFlags() & FLAG_SELECTED))
		++it;
	
	return it != listItem_.end();
}

unsigned int qm::ViewModel::getSelectedCount() const
{
	assert(isLocked());
	
	unsigned int nCount = 0;
	
	ItemList::const_iterator it = listItem_.begin();
	while (it != listItem_.end()) {
		if ((*it)->getFlags() & FLAG_SELECTED)
			++nCount;
		++it;
	}
	
	return nCount;
}

bool qm::ViewModel::isSelected(unsigned int n) const
{
	assert(isLocked());
	assert(n < getCount());
	
	return (listItem_[n]->getFlags() & FLAG_SELECTED) != 0;
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

QSTATUS qm::ViewModel::setFocused(unsigned int n)
{
	Lock<ViewModel> lock(*this);
	
	assert(n < getCount());
	
	DECLARE_QSTATUS();
	
	if (nFocused_ != n) {
		unsigned int nOld = nFocused_;
		nFocused_ = n;
		
		assert(listItem_[nOld]->getFlags() & FLAG_FOCUSED);
		listItem_[nOld]->setFlags(0, FLAG_FOCUSED);
		listItem_[nFocused_]->setFlags(FLAG_FOCUSED, FLAG_FOCUSED);
		
		status = fireItemStateChanged(nOld);
		CHECK_QSTATUS();
		status = fireItemStateChanged(n);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

unsigned int qm::ViewModel::getFocused() const
{
	Lock<ViewModel> lock(*this);
	assert(listItem_.empty() || listItem_[nFocused_]->getFlags() & FLAG_FOCUSED);
	return nFocused_;
}

bool qm::ViewModel::isFocused(unsigned int n) const
{
	Lock<ViewModel> lock(*this);
	assert(n < getCount());
	assert(listItem_.empty() || listItem_[nFocused_]->getFlags() & FLAG_FOCUSED);
	return n == nFocused_;
}

QSTATUS qm::ViewModel::payAttention(unsigned int n)
{
	return fireItemAttentionPaid(n);
}

QSTATUS qm::ViewModel::save() const
{
	DECLARE_QSTATUS();
	
	WCHAR wszSection[32];
	swprintf(wszSection, L"Folder%d", pFolder_->getId());
	status = pProfile_->setInt(wszSection, L"Sort", nSort_);
	CHECK_QSTATUS();
	status = pProfile_->setInt(wszSection, L"Focus", nFocused_);
	CHECK_QSTATUS();
	
	status = saveColumns();
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ViewModel::addViewModelHandler(ViewModelHandler* pHandler)
{
	return STLWrapper<ViewModelHandlerList>(listHandler_).push_back(pHandler);
}

QSTATUS qm::ViewModel::removeViewModelHandler(ViewModelHandler* pHandler)
{
	ViewModelHandlerList& l = listHandler_;
	ViewModelHandlerList::iterator it = std::remove(
		l.begin(), l.end(), pHandler);
	l.erase(it, l.end());
	return QSTATUS_SUCCESS;
}

void qm::ViewModel::lock() const
{
	pFolder_->lock();
#ifndef NDEBUG
	++nLock_;
#endif
}

void qm::ViewModel::unlock() const
{
#ifndef NDEBUG
	--nLock_;
#endif
	pFolder_->unlock();
}

#ifndef NDEBUG
bool qm::ViewModel::isLocked() const
{
	return nLock_ != 0;
}
#endif

QSTATUS qm::ViewModel::messageAdded(const FolderEvent& event)
{
	assert(event.getFolder() == pFolder_);
	
	DECLARE_QSTATUS();
	
	Lock<ViewModel> lock(*this);
	
	// TODO
	// Resort if this view model is sorted by flags
	
	MessageHolder* pmh = event.getMessageHolder();
	
	bool bAdd = true;
	if (pFilter_) {
		Message msg(&status);
		CHECK_QSTATUS();
		MacroContext::Init init = {
			pmh,
			&msg,
			pFolder_->getAccount(),
			pDocument_,
			hwnd_,
			pProfile_,
			false,
			0,
			0
		};
		MacroContext context(init, &status);
		CHECK_QSTATUS();
		status = pFilter_->match(&context, &bAdd);
		CHECK_QSTATUS();
	}
	
	if (bAdd) {
		std::auto_ptr<ViewModelItem> pItem;
		status = newQsObject(pmh, &pItem);
		CHECK_QSTATUS();
		
		if ((getSort() & SORT_THREAD_MASK) == SORT_THREAD) {
			unsigned int nReferenceHash = pmh->getReferenceHash();
			if (nReferenceHash != 0) {
				string_ptr<WSTRING> wstrReference;
				status = pmh->getReference(&wstrReference);
				CHECK_QSTATUS();
				
				ItemList::const_iterator it = listItem_.begin();
				while (it != listItem_.end()) {
					MessageHolder* pmhParent = (*it)->getMessageHolder();
					if (pmhParent->getMessageIdHash() == nReferenceHash) {
						string_ptr<WSTRING> wstrMessageId;
						status = pmhParent->getMessageId(&wstrMessageId);
						CHECK_QSTATUS();
						if (wcscmp(wstrReference.get(), wstrMessageId.get()) == 0) {
							pItem->setParentItem(*it);
							break;
						}
					}
					++it;
				}
			}
		}
		
		ItemList::iterator it = std::lower_bound(
			listItem_.begin(), listItem_.end(), pItem.get(),
			ViewModelItemComp(this, getColumn(nSort_ & SORT_INDEX_MASK),
				(nSort_ & SORT_DIRECTION_MASK) == SORT_ASCENDING,
				(nSort_ & SORT_THREAD_MASK) == SORT_THREAD));
		
		ItemList::iterator itInsert;
		status = STLWrapper<ItemList>(listItem_).insert(it, pItem.get(), &itInsert);
		CHECK_QSTATUS();
		pItem.release();
		
		unsigned int nPos = itInsert - listItem_.begin();
		if (nLastSelection_ >= nPos && nLastSelection_ < listItem_.size() - 1)
			++nLastSelection_;
		if (nFocused_ >= nPos && nFocused_ < listItem_.size() - 1)
			++nFocused_;
		if (listItem_.size() == 1) {
			assert(nFocused_ == 0);
			assert(nLastSelection_ == 0);
			listItem_[0]->setFlags(FLAG_FOCUSED | FLAG_SELECTED,
				FLAG_FOCUSED | FLAG_SELECTED);
		}
		assert(listItem_.empty() || (listItem_[nFocused_]->getFlags() & FLAG_FOCUSED));
		
		status = fireItemAdded(nPos);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ViewModel::messageRemoved(const FolderEvent& event)
{
	assert(event.getFolder() == pFolder_);
	
	DECLARE_QSTATUS();
	
	Lock<ViewModel> lock(*this);
	
	unsigned int nIndex = getIndex(event.getMessageHolder());
	if (nIndex != static_cast<unsigned int>(-1)) {
		ItemList::iterator it = listItem_.begin() + nIndex;
		
		bool bHasFocus = ((*it)->getFlags() & FLAG_FOCUSED) != 0;
		bool bSelected = ((*it)->getFlags() & FLAG_SELECTED) != 0;
		
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
				listItem_[nFocused_]->setFlags(FLAG_FOCUSED, FLAG_FOCUSED);
		}
		else if (nFocused_ > nIndex) {
			--nFocused_;
			assert(!listItem_.empty());
			assert(listItem_[nFocused_]->getFlags() & FLAG_FOCUSED);
		}
		
		if (bSelected) {
			if (it == listItem_.end()) {
				if (!listItem_.empty())
					listItem_.back()->setFlags(FLAG_SELECTED, FLAG_SELECTED);
			}
			else {
				(*it)->setFlags(FLAG_SELECTED, FLAG_SELECTED);
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
						FLAG_SELECTED, FLAG_SELECTED);
				}
			}
		}
		else if (nLastSelection_ > nIndex) {
			--nLastSelection_;
			assert(!listItem_.empty());
		}
		
		if (bSort) {
			status = sort(nSort_, true, false);
			CHECK_QSTATUS();
		}
		
		status = fireItemRemoved(nIndex);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ViewModel::messageChanged(const MessageEvent& event)
{
	DECLARE_QSTATUS();
	
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
		
		status = fireItemChanged(it - listItem_.begin());
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ViewModel::folderDestroyed(const FolderEvent& event)
{
	fireDestroyed();
	pViewModelManager_->removeViewModel(this);
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ViewModel::loadColumns()
{
	DECLARE_QSTATUS();
	
	STLWrapper<ColumnList> wrapper(listColumn_);
	
	MacroParser parser(MacroParser::TYPE_COLUMN, &status);
	CHECK_QSTATUS();
	
	WCHAR wszSection[32];
	swprintf(wszSection, L"Folder%d", pFolder_->getId());
	
	int nColumn = 0;
	while (true) {
		string_ptr<WSTRING> wstrTitle;
		ViewColumn::Type type = ViewColumn::TYPE_NONE;
		std::auto_ptr<Macro> pMacro;
		int nFlags = 0;
		int nWidth = 0;
		
		WCHAR wszKey[32];
		
		swprintf(wszKey, L"ColumnTitle%d", nColumn);
		status = pProfile_->getString(wszSection, wszKey, 0, &wstrTitle);
		CHECK_QSTATUS();
		
		swprintf(wszKey, L"ColumnMacro%d", nColumn);
		string_ptr<WSTRING> wstrMacro;
		status = pProfile_->getString(wszSection, wszKey, 0, &wstrMacro);
		CHECK_QSTATUS();
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
			int n = 0;
			while (n < countof(defaults) && type == ViewColumn::TYPE_NONE) {
				if (_wcsicmp(wstrMacro.get(), defaults[n].pwszMacro_) == 0)
					type = defaults[n].type_;
				++n;
			}
		}
		if (type == ViewColumn::TYPE_NONE) {
			Macro* p = 0;
			status = parser.parse(wstrMacro.get(), &p);
			CHECK_QSTATUS();
			pMacro.reset(p);
			type = ViewColumn::TYPE_OTHER;
		}
		
		swprintf(wszKey, L"ColumnFlags%d", nColumn);
		status = pProfile_->getInt(wszSection, wszKey, 0, &nFlags);
		CHECK_QSTATUS();
		
		swprintf(wszKey, L"ColumnWidth%d", nColumn);
		status = pProfile_->getInt(wszSection, wszKey, 0, &nWidth);
		CHECK_QSTATUS();
		
		ViewColumn::Init init = {
			wstrTitle.get(),
			type,
			pMacro.get(),
			nFlags,
			nWidth
		};
		std::auto_ptr<ViewColumn> pColumn;
		status = newQsObject(init, &pColumn);
		CHECK_QSTATUS();
		pMacro.release();
		
		status = wrapper.push_back(pColumn.get());
		CHECK_QSTATUS();
		pColumn.release();

		++nColumn;
	}
	
	if (nColumn == 0) {
		ViewColumn::Init inits[] = {
			{
				L"",
				ViewColumn::TYPE_FLAGS,
				0,
				ViewColumn::FLAG_ICON | ViewColumn::FLAG_SORT_NUMBER,
				28
			},
			{
				L"Date",
				ViewColumn::TYPE_DATE,
				0,
				ViewColumn::FLAG_SORT_DATE,
				80
			},
			{
				L"From / To",
				ViewColumn::TYPE_FROMTO,
				0,
				ViewColumn::FLAG_SORT_TEXT,
				120
			},
			{
				L"Subject",
				ViewColumn::TYPE_SUBJECT,
				0,
				ViewColumn::FLAG_INDENT | ViewColumn::FLAG_LINE | ViewColumn::FLAG_SORT_TEXT,
				250
			},
			{
				L"Size",
				ViewColumn::TYPE_SIZE,
				0,
				ViewColumn::FLAG_RIGHTALIGN | ViewColumn::FLAG_SORT_NUMBER,
				40
			}
		};
		for (int n = 0; n < countof(inits); ++n) {
			std::auto_ptr<ViewColumn> pColumn;
			status = newQsObject(inits[n], &pColumn);
			CHECK_QSTATUS();
			status = wrapper.push_back(pColumn.get());
			CHECK_QSTATUS();
			pColumn.release();
		}
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ViewModel::saveColumns() const
{
	DECLARE_QSTATUS();
	
	WCHAR wszSection[32];
	swprintf(wszSection, L"Folder%d", pFolder_->getId());
	
	for (ColumnList::size_type n = 0; n < listColumn_.size(); ++n) {
		ViewColumn* pColumn = listColumn_[n];
		
		WCHAR wszKey[32];
		
		swprintf(wszKey, L"ColumnTitle%d", n);
		status = pProfile_->setString(wszSection, wszKey, pColumn->getTitle());
		CHECK_QSTATUS();
		
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
		string_ptr<WSTRING> wstrMacro;
		if (!pwszMacro) {
			status = pColumn->getMacro()->getString(&wstrMacro);
			CHECK_QSTATUS();
			pwszMacro = wstrMacro.get();
		}
		swprintf(wszKey, L"ColumnMacro%d", n);
		status = pProfile_->setString(wszSection, wszKey, pwszMacro);
		CHECK_QSTATUS();
		
		swprintf(wszKey, L"ColumnFlags%d", n);
		status = pProfile_->setInt(wszSection, wszKey, pColumn->getFlags());
		CHECK_QSTATUS();
		
		swprintf(wszKey, L"ColumnWidth%d", n);
		status = pProfile_->setInt(wszSection, wszKey, pColumn->getWidth());
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ViewModel::update(bool bRestoreSelection)
{
	DECLARE_QSTATUS();
	
	Lock<ViewModel> lock(*this);
	
	status = pFolder_->loadMessageHolders();
	CHECK_QSTATUS();
	
	SelectionRestorer restorer(this, true, !bRestoreSelection, &status);
	CHECK_QSTATUS();
	
	std::for_each(listItem_.begin(), listItem_.end(), deleter<ViewModelItem>());
	listItem_.clear();
	
	unsigned int nCount = pFolder_->getCount();
	status = STLWrapper<ItemList>(listItem_).reserve(nCount);
	CHECK_QSTATUS();
	
	nUnseenCount_ = 0;
	
	MacroVariableHolder globalVariable(&status);
	CHECK_QSTATUS();
	for (unsigned int n = 0; n < nCount; ++n) {
		MessageHolder* pmh = pFolder_->getMessage(n);
		bool bAdd = true;
		if (pFilter_) {
			Message msg(&status);
			CHECK_QSTATUS();
			MacroContext::Init init = {
				pmh,
				&msg,
				pFolder_->getAccount(),
				pDocument_,
				hwnd_,
				pProfile_,
				false,
				0,
				&globalVariable
			};
			MacroContext context(init, &status);
			CHECK_QSTATUS();
			status = pFilter_->match(&context, &bAdd);
			CHECK_QSTATUS();
		}
		if (bAdd) {
			std::auto_ptr<ViewModelItem> pItem;
			status = newQsObject(pmh, &pItem);
			CHECK_QSTATUS();
			listItem_.push_back(pItem.release());
			
			if (!pmh->isFlag(MessageHolder::FLAG_SEEN))
				++nUnseenCount_;
		}
	}
	
	status = sort(nSort_, false, true);
	CHECK_QSTATUS();
	
	if (bRestoreSelection)
		restorer.restore();
	
	status = fireUpdated();
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ViewModel::sort(unsigned int nSort,
	bool bRestoreSelection, bool bUpdateParentLink)
{
	assert(nSort & SORT_DIRECTION_MASK);
	assert(nSort & SORT_THREAD_MASK);
	
	DECLARE_QSTATUS();
	
	Lock<ViewModel> lock(*this);
	
	SelectionRestorer restorer(this, false, !bRestoreSelection, &status);
	CHECK_QSTATUS();
	
	if (bUpdateParentLink && (nSort & SORT_THREAD_MASK) == SORT_THREAD) {
		status = makeParentLink();
		CHECK_QSTATUS();
	}
	
	std::stable_sort(listItem_.begin(), listItem_.end(),
		ViewModelItemComp(this, getColumn(nSort & SORT_INDEX_MASK),
			(nSort & SORT_DIRECTION_MASK) == SORT_ASCENDING,
			(nSort & SORT_THREAD_MASK) == SORT_THREAD));
	
	if (bRestoreSelection)
		restorer.restore();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ViewModel::makeParentLink()
{
	DECLARE_QSTATUS();
	
	Lock<ViewModel> lock(*this);
	
	ItemList listItemSortedByMessageIdHash;
	status = STLWrapper<ItemList>(
		listItemSortedByMessageIdHash).resize(listItem_.size());
	CHECK_QSTATUS();
	std::copy(listItem_.begin(), listItem_.end(),
		listItemSortedByMessageIdHash.begin());
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
	
	ItemList listItemSortedByPointer;
	status = STLWrapper<ItemList>(
		listItemSortedByPointer).resize(listItem_.size());
	CHECK_QSTATUS();
	std::copy(listItem_.begin(), listItem_.end(),
		listItemSortedByPointer.begin());
	std::sort(listItemSortedByPointer.begin(),
		listItemSortedByPointer.end());
	
	ItemList::iterator it = listItem_.begin();
	while (it != listItem_.end()) {
		status = makeParentLink(listItemSortedByMessageIdHash,
			listItemSortedByPointer, *it);
		CHECK_QSTATUS();
		++it;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ViewModel::makeParentLink(
	const ItemList& listItemSortedByMessageIdHash,
	const ItemList& listItemSortedByPointer, ViewModelItem* pItem)
{
	assert(pItem);
	
	DECLARE_QSTATUS();
	
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
				string_ptr<WSTRING> wstrReference;
				status = pmh->getReference(&wstrReference);
				CHECK_QSTATUS();
				assert(*wstrReference.get());
				while  (it != listItemSortedByMessageIdHash.end() &&
					(*it)->getMessageHolder()->getMessageIdHash() == nReferenceHash &&
					!bFound) {
					string_ptr<WSTRING> wstrMessageId;
					status = (*it)->getMessageHolder()->getMessageId(&wstrMessageId);
					CHECK_QSTATUS();
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
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ViewModel::fireItemAdded(unsigned int nItem) const
{
	ViewModelEvent event(this, nItem);
	return fireEvent(event, &ViewModelHandler::itemAdded);
}

QSTATUS qm::ViewModel::fireItemRemoved(unsigned int nItem) const
{
	ViewModelEvent event(this, nItem);
	return fireEvent(event, &ViewModelHandler::itemRemoved);
}

QSTATUS qm::ViewModel::fireItemChanged(unsigned int nItem) const
{
	ViewModelEvent event(this, nItem);
	return fireEvent(event, &ViewModelHandler::itemChanged);
}

QSTATUS qm::ViewModel::fireItemStateChanged(unsigned int nItem) const
{
	ViewModelEvent event(this, nItem);
	return fireEvent(event, &ViewModelHandler::itemStateChanged);
}

QSTATUS qm::ViewModel::fireItemAttentionPaid(unsigned int nItem) const
{
	ViewModelEvent event(this, nItem);
	return fireEvent(event, &ViewModelHandler::itemAttentionPaid);
}

QSTATUS qm::ViewModel::fireUpdated() const
{
	ViewModelEvent event(this);
	return fireEvent(event, &ViewModelHandler::updated);
}

QSTATUS qm::ViewModel::fireSorted() const
{
	ViewModelEvent event(this);
	return fireEvent(event, &ViewModelHandler::sorted);
}

QSTATUS qm::ViewModel::fireDestroyed() const
{
	DECLARE_QSTATUS();
	
	ViewModelHandlerList l;
	status = STLWrapper<ViewModelHandlerList>(l).resize(listHandler_.size());
	CHECK_QSTATUS();
	std::copy(listHandler_.begin(), listHandler_.end(), l.begin());
	
	ViewModelEvent event(this);
	
	ViewModelHandlerList::const_iterator it = l.begin();
	while (it != l.end()) {
		status = (*it)->destroyed(event);
		CHECK_QSTATUS();
		++it;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ViewModel::fireEvent(const ViewModelEvent& event,
	QSTATUS (ViewModelHandler::*pfn)(const ViewModelEvent&)) const
{
	DECLARE_QSTATUS();
	
	ViewModelHandlerList::const_iterator it = listHandler_.begin();
	while (it != listHandler_.end()) {
		status = ((*it)->*pfn)(event);
		CHECK_QSTATUS();
		++it;
	}
	
	return QSTATUS_SUCCESS;
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

QSTATUS qm::DefaultViewModelHandler::itemAdded(const ViewModelEvent& event)
{
	return QSTATUS_SUCCESS;
}

QSTATUS qm::DefaultViewModelHandler::itemRemoved(const ViewModelEvent& event)
{
	return QSTATUS_SUCCESS;
}

QSTATUS qm::DefaultViewModelHandler::itemChanged(const ViewModelEvent& event)
{
	return QSTATUS_SUCCESS;
}

QSTATUS qm::DefaultViewModelHandler::itemStateChanged(const ViewModelEvent& event)
{
	return QSTATUS_SUCCESS;
}

QSTATUS qm::DefaultViewModelHandler::itemAttentionPaid(const ViewModelEvent& event)
{
	return QSTATUS_SUCCESS;
}

QSTATUS qm::DefaultViewModelHandler::updated(const ViewModelEvent& event)
{
	return QSTATUS_SUCCESS;
}

QSTATUS qm::DefaultViewModelHandler::sorted(const ViewModelEvent& event)
{
	return QSTATUS_SUCCESS;
}

QSTATUS qm::DefaultViewModelHandler::destroyed(const ViewModelEvent& event)
{
	return QSTATUS_SUCCESS;
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

qm::ViewModelEvent::ViewModelEvent(
	const ViewModel* pViewModel, unsigned int nItem) :
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
 * ViewModelManager
 *
 */

qm::ViewModelManager::ViewModelManager(Profile* pProfile, Document* pDocument,
	HWND hwnd, FolderModel* pFolderModel, QSTATUS* pstatus) :
	pProfile_(pProfile),
	pDocument_(pDocument),
	hwnd_(hwnd),
	pCurrentAccount_(0),
	pCurrentViewModel_(0),
	pFilterManager_(0),
	pColorManager_(0),
	pDelayedFolderModelHandler_(0)
{
	assert(pstatus);
	
	DECLARE_QSTATUS();
	
	std::auto_ptr<FilterManager> pFilterManager;
	status = newQsObject(&pFilterManager);
	CHECK_QSTATUS_SET(pstatus);
	std::auto_ptr<ColorManager> pColorManager;
	status = newQsObject(&pColorManager);
	CHECK_QSTATUS_SET(pstatus);
	
	status = newQsObject(this, &pDelayedFolderModelHandler_);
	CHECK_QSTATUS_SET(pstatus);
	status = pFolderModel->addFolderModelHandler(pDelayedFolderModelHandler_);
	CHECK_QSTATUS_SET(pstatus);
	
	pFilterManager_ = pFilterManager.release();
	pColorManager_ = pColorManager.release();
}

qm::ViewModelManager::~ViewModelManager()
{
	std::for_each(listViewModel_.begin(),
		listViewModel_.end(), deleter<ViewModel>());
	std::for_each(mapProfile_.begin(), mapProfile_.end(),
		unary_compose_f_gx(
			deleter<Profile>(), std::select2nd<ProfileMap::value_type>()));
	delete pFilterManager_;
	delete pColorManager_;
	delete pDelayedFolderModelHandler_;
}

FilterManager* qm::ViewModelManager::getFilterManager() const
{
	return pFilterManager_;
}

Account* qm::ViewModelManager::getCurrentAccount() const
{
	return pCurrentAccount_;
}

ViewModel* qm::ViewModelManager::getCurrentViewModel() const
{
	return pCurrentViewModel_;
}

QSTATUS qm::ViewModelManager::getViewModel(
	Folder* pFolder, ViewModel** ppViewModel)
{
	assert(pFolder);
	assert(ppViewModel);
	
	DECLARE_QSTATUS();
	
	*ppViewModel = 0;
	
	ViewModelList::iterator it = std::find_if(listViewModel_.begin(),
		listViewModel_.end(), ViewModelFolderComp(pFolder));
	if (it == listViewModel_.end()) {
		Profile* pProfile = 0;
		status = getProfile(pFolder, &pProfile);
		CHECK_QSTATUS();
		std::auto_ptr<ViewModel> pViewModel;
		status = newQsObject(this, pFolder, pProfile,
			pDocument_, hwnd_, pColorManager_, &pViewModel);
		CHECK_QSTATUS();
		STLWrapper<ViewModelList>(listViewModel_).push_back(pViewModel.get());
		CHECK_QSTATUS();
		*ppViewModel = pViewModel.release();
	}
	else {
		*ppViewModel = *it;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ViewModelManager::save() const
{
	DECLARE_QSTATUS();
	
	ViewModelList::const_iterator itM = listViewModel_.begin();
	while (itM != listViewModel_.end()) {
		status = (*itM)->save();
		CHECK_QSTATUS();
		++itM;
	}
	
	ProfileMap::const_iterator itP = mapProfile_.begin();
	while (itP != mapProfile_.end()) {
		status = (*itP).second->save();
		CHECK_QSTATUS();
		++itP;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ViewModelManager::addViewModelManagerHandler(
	ViewModelManagerHandler* pHandler)
{
	return STLWrapper<HandlerList>(listHandler_).push_back(pHandler);
}

QSTATUS qm::ViewModelManager::removeViewModelManagerHandler(
	ViewModelManagerHandler* pHandler)
{
	HandlerList& l = listHandler_;
	HandlerList::iterator it = std::remove(l.begin(), l.end(), pHandler);
	l.erase(it, l.end());
	return QSTATUS_SUCCESS;
}

void qm::ViewModelManager::removeViewModel(ViewModel* pViewModel)
{
	ViewModelList::iterator it = std::remove(listViewModel_.begin(),
		listViewModel_.end(), pViewModel);
	listViewModel_.erase(it, listViewModel_.end());
	delete pViewModel;
}

QSTATUS qm::ViewModelManager::accountSelected(const FolderModelEvent& event)
{
	return setCurrentFolder(event.getAccount(), 0);
}

QSTATUS qm::ViewModelManager::folderSelected(const FolderModelEvent& event)
{
	return setCurrentFolder(0, event.getFolder());
}

QSTATUS qm::ViewModelManager::setCurrentFolder(Account* pAccount, Folder* pFolder)
{
	assert(pAccount || pFolder);
	assert(!pAccount || !pFolder);
	
	DECLARE_QSTATUS();
	
	pCurrentAccount_ = pAccount ? pAccount : pFolder->getAccount();
	
	ViewModel* pViewModel = 0;
	if (pFolder) {
		status = getViewModel(pFolder, &pViewModel);
		if (status != QSTATUS_SUCCESS) {
			// TODO
		}
	}
	status = setCurrentViewModel(pViewModel);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ViewModelManager::setCurrentViewModel(ViewModel* pViewModel)
{
	DECLARE_QSTATUS();
	
	ViewModel* pOldViewModel = pCurrentViewModel_;
	pCurrentViewModel_ = pViewModel;
	
	status = fireViewModelSelected(pViewModel, pOldViewModel);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ViewModelManager::getProfile(Folder* pFolder, Profile** ppProfile)
{
	assert(pFolder);
	assert(ppProfile);
	
	DECLARE_QSTATUS();
	
	Account* pAccount = pFolder->getAccount();
	ProfileMap::iterator it = std::find_if(
		mapProfile_.begin(), mapProfile_.end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				std::equal_to<Account*>(),
				std::select1st<ProfileMap::value_type>(),
				std::identity<Account*>()),
			pAccount));
	if (it == mapProfile_.end()) {
		string_ptr<WSTRING> wstrPath(concat(
			pAccount->getPath(), L"\\", Extensions::VIEW));
		if (!wstrPath.get())
			return QSTATUS_OUTOFMEMORY;
		std::auto_ptr<XMLProfile> pProfile;
		status = newQsObject(wstrPath.get(), &pProfile);
		CHECK_QSTATUS();
		status = pProfile->load();
		CHECK_QSTATUS();
		status = STLWrapper<ProfileMap>(mapProfile_).push_back(
			std::make_pair(pAccount, pProfile.get()));
		CHECK_QSTATUS();
		*ppProfile = pProfile.release();
	}
	else {
		*ppProfile = (*it).second;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ViewModelManager::fireViewModelSelected(
	ViewModel* pNewViewModel, ViewModel* pOldViewModel) const
{
	DECLARE_QSTATUS();
	
	ViewModelManagerEvent event(this, pNewViewModel, pOldViewModel);
	HandlerList::const_iterator it = listHandler_.begin();
	while (it != listHandler_.end()) {
		status = (*it)->viewModelSelected(event);
		CHECK_QSTATUS();
		++it;
	}
	return QSTATUS_SUCCESS;
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

qm::ViewModelManagerEvent::ViewModelManagerEvent(
	const ViewModelManager* pViewModelManager,
	ViewModel* pNewViewModel, ViewModel* pOldViewModel) :
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
	const ViewColumn& column, bool bAscending, bool bThread) :
	pViewModel_(pViewModel),
	column_(column),
	bAscending_(bAscending),
	bThread_(bThread)
{
}

qm::ViewModelItemComp::~ViewModelItemComp()
{
}

bool qm::ViewModelItemComp::operator()(
	const ViewModelItem* pLhs, const ViewModelItem* pRhs) const
{
	DECLARE_QSTATUS();
	
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
			unsigned int nLhs = column_.getNumber(pmhLhs);
			unsigned int nRhs = column_.getNumber(pmhRhs);
			nComp = nLhs < nRhs ? -1 : nLhs > nRhs ? 1 : 0;
		}
		else if ((nFlags & ViewColumn::FLAG_SORT_MASK) == ViewColumn::FLAG_SORT_DATE) {
			Time timeLhs;
			status = column_.getTime(pmhLhs, &timeLhs);
			CHECK_QSTATUS_VALUE(false);
			Time timeRhs;
			status = column_.getTime(pmhRhs, &timeRhs);
			CHECK_QSTATUS_VALUE(false);
			nComp = timeLhs < timeRhs ? -1 : timeLhs > timeRhs ? 1 : 0;
		}
		else {
			string_ptr<WSTRING> wstrTextLhs;
			status = column_.getText(pmhLhs, &wstrTextLhs);
			CHECK_QSTATUS_VALUE(false);
			string_ptr<WSTRING> wstrTextRhs;
			status = column_.getText(pmhRhs, &wstrTextRhs);
			CHECK_QSTATUS_VALUE(false);
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
