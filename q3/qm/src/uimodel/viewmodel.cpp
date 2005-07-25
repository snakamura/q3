/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#pragma warning(disable:4786)

#include <qmaccount.h>
#include <qmapplication.h>
#include <qmfilenames.h>
#include <qmfolder.h>
#include <qmmacro.h>
#include <qmmessage.h>
#include <qmmessageholder.h>
#include <qmsecurity.h>

#include <qsconv.h>
#include <qsfile.h>
#include <qsstl.h>

#include <algorithm>

#include "securitymodel.h"
#include "viewmodel.h"
#include "../model/color.h"
#include "../model/filter.h"
#include "../util/confighelper.h"

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
	nWidth_(nWidth),
	nCacheIndex_(-1)
{
	wstrTitle_ = allocWString(pwszTitle);
	
	if (type_ != TYPE_OTHER && nFlags_ & FLAG_CACHE)
		nFlags_ &= ~FLAG_CACHE;
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

unsigned int qm::ViewColumn::getWidth() const
{
	return nWidth_;
}

void qm::ViewColumn::setWidth(unsigned int nWidth)
{
	nWidth_ = nWidth;
}

void qm::ViewColumn::set(const WCHAR* pwszTitle,
						 Type type,
						 std::auto_ptr<Macro> pMacro,
						 unsigned int nFlags,
						 unsigned int nWidth)
{
	wstrTitle_ = allocWString(pwszTitle);
	type_ = type;
	pMacro_ = pMacro;
	nFlags_ = nFlags;
	nWidth_ = nWidth;
}

std::auto_ptr<ViewColumn> qm::ViewColumn::clone() const
{
	std::auto_ptr<Macro> pMacro;
	if (pMacro_.get()) {
		wstring_ptr wstrMacro(pMacro_->getString());
		pMacro = MacroParser().parse(wstrMacro.get());
		assert(pMacro.get());
	}
	
	return std::auto_ptr<ViewColumn>(new ViewColumn(
		wstrTitle_.get(), type_, pMacro, nFlags_, nWidth_));
}

void qm::ViewColumn::setCacheIndex(unsigned int n)
{
	nCacheIndex_ = n;
}

wstring_ptr qm::ViewColumn::getText(const ViewModel* pViewModel,
									const ViewModelItem* pItem) const
{
	assert(pItem);
	
	MessageHolder* pmh = pItem->getMessageHolder();
	
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
		if (nFlags_ & FLAG_CACHE) {
			assert(nCacheIndex_ != -1);
			const MacroValue* pCache = pItem->getCache(nCacheIndex_);
			if (pCache)
				wstrText = pCache->string();
		}
		if (!wstrText.get()) {
			MacroValuePtr pValue(pViewModel->getValue(pMacro_.get(), pmh));
			if (pValue.get()) {
				wstrText = pValue->string();
				if (nFlags_ & FLAG_CACHE) {
					if (pValue->getType() != MacroValue::TYPE_STRING)
						pValue = MacroValueFactory::getFactory().newString(wstrText.get());
					pItem->setCache(nCacheIndex_, pValue.release());
				}
			}
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
									   const ViewModelItem* pItem) const
{
	assert(pItem);
	
	MessageHolder* pmh = pItem->getMessageHolder();
	
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
			bool bCached = false;
			if (nFlags_ & FLAG_CACHE) {
				assert(nCacheIndex_ != -1);
				const MacroValue* pCache = pItem->getCache(nCacheIndex_);
				if (pCache) {
					nValue = pCache->number();
					bCached = true;
				}
			}
			if (!bCached) {
				MacroValuePtr pValue(pViewModel->getValue(pMacro_.get(), pmh));
				if (pValue.get()) {
					nValue = pValue->number();
					if (nFlags_ & FLAG_CACHE) {
						if (pValue->getType() != MacroValue::TYPE_NUMBER)
							pValue = MacroValueFactory::getFactory().newNumber(nValue);
						pItem->setCache(nCacheIndex_, pValue.release());
					}
				}
			}
		}
		break;
	default:
		assert(false);
		break;
	}
	return nValue;
}

void qm::ViewColumn::getTime(const ViewModel* pViewModel,
							 const ViewModelItem* pItem,
							 Time* pTime) const
{
	assert(pItem);
	assert(pTime);
	
	MessageHolder* pmh = pItem->getMessageHolder();
	
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
			bool bCache = false;
			if (nFlags_ & FLAG_CACHE) {
				assert(nCacheIndex_ != -1);
				const MacroValue* pCache = pItem->getCache(nCacheIndex_);
				if (pCache) {
					*pTime = static_cast<const MacroValueTime*>(pCache)->getTime();
					bCache = true;
				}
			}
			if (!bCache) {
				MacroValuePtr pValue(pViewModel->getValue(pMacro_.get(), pmh));
				if (pValue.get() && pValue->getType() == MacroValue::TYPE_TIME) {
					*pTime = static_cast<MacroValueTime*>(pValue.get())->getTime();
					pItem->setCache(nCacheIndex_, pValue.release());
				}
				else {
					bCurrent = true;
				}
			}
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
		unsigned int n = pViewModel_->getIndex(pmhFocused_);
		pViewModel_->nFocused_ = n != -1 ? n : 0;
	}
	else {
		pViewModel_->nFocused_ = 0;
	}
	
	if (pmhLastSelection_) {
		unsigned int n = pViewModel_->getIndex(pmhLastSelection_);
		pViewModel_->nLastSelection_ = n != -1 ? n : 0;
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
						 const Filter* pFilter,
						 Profile* pProfile,
						 Document* pDocument,
						 SecurityModel* pSecurityModel,
						 const ColorManager* pColorManager) :
	pViewModelManager_(pViewModelManager),
	pFolder_(pFolder),
	pDataItem_(pDataItem),
	pProfile_(pProfile),
	pDocument_(pDocument),
	pSecurityModel_(pSecurityModel),
	nUnseenCount_(0),
	nSort_(SORT_ASCENDING | SORT_NOTHREAD),
	nLastSelection_(0),
	nFocused_(0),
	nScroll_(-1),
	nMode_(0),
	nCacheCount_(0)
{
	assert(pFolder);
	assert(pDataItem);
	assert(pProfile);
	
#ifndef NDEBUG
	nLock_ = 0;
#endif
	
	pColorList_ = pColorManager->getColorList(pFolder_);
	
	nSort_ = pDataItem_->getSort();
	nFocused_ = pDataItem_->getFocus();
	if ((nSort_ & SORT_INDEX_MASK) >= getColumnCount())
		nSort_ = SORT_ASCENDING | SORT_NOTHREAD | 1;
	
	if (pFilter)
		pFilter_.reset(new Filter(*pFilter));
	
	nMode_ = pDataItem_->getMode();
	updateCacheCount();
	
	Lock<ViewModel> lock(*this);
	
	update(false, -1);
	
	if (nFocused_ >= listItem_.size())
		nFocused_ = listItem_.empty() ? 0 : static_cast<unsigned int>(listItem_.size() - 1);
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
	for (ItemList::iterator it = listItem_.begin(); it != listItem_.end(); ++it)
		ViewModelItem::deleteItem(*it, nCacheCount_);
}

Folder* qm::ViewModel::getFolder() const
{
	return pFolder_;
}

const ViewColumnList& qm::ViewModel::getColumns() const
{
	return pDataItem_->getColumns();
}

void qm::ViewModel::setColumns(const ViewColumnList& listColumn)
{
	pDataItem_->setColumns(listColumn);
	
	if ((nSort_ & SORT_INDEX_MASK) >= listColumn.size())
		nSort_ = SORT_ASCENDING | SORT_NOTHREAD | 1;
	
	unsigned int nCacheCount = nCacheCount_;
	updateCacheCount();
	update(true, nCacheCount);
	
	fireColumnChanged();
}

unsigned int qm::ViewModel::getColumnCount() const
{
	return static_cast<unsigned int>(pDataItem_->getColumns().size());
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
	return static_cast<unsigned int>(listItem_.size());
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
		if (pColorList_.get()) {
			Message msg;
			MacroContext context(pmh, &msg, MessageHolderList(),
				pFolder_->getAccount(), pDocument_, 0, pProfile_, 0,
				MacroContext::FLAG_UITHREAD | MacroContext::FLAG_GETMESSAGEASPOSSIBLE,
				/*pSecurityModel_->getSecurityMode()*/SECURITYMODE_NONE, 0, 0);
			cr = pColorList_->getColor(&context);
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
	
	ViewModelItemPtr pItem(pmh, nCacheCount_);
	ItemList::const_iterator it = std::lower_bound(
		listItem_.begin(), listItem_.end(), pItem.get(),
		getComparator(nSort_));
	while (it != listItem_.end() && (*it)->getMessageHolder() != pmh)
		++it;
	if (it == listItem_.end()) {
		it = listItem_.begin();
		while (it != listItem_.end() && (*it)->getMessageHolder() != pmh)
			++it;
	}
	return it == listItem_.end() ? -1 : static_cast<unsigned int>(it - listItem_.begin());
}

void qm::ViewModel::setSort(unsigned int nSort,
							unsigned int nMask)
{
	assert(nSort_ & SORT_DIRECTION_MASK);
	assert(nSort_ & SORT_THREAD_MASK);
	
	Lock<ViewModel> lock(*this);
	
	nSort = (nSort & nMask) | (nSort_ & ~nMask);
	
	assert(nSort & SORT_DIRECTION_MASK);
	assert(nSort & SORT_THREAD_MASK);
	assert((nSort & SORT_INDEX_MASK) < getColumnCount());
	
	bool bUpdateParentLink = (nSort & (SORT_THREAD_MASK | SORT_FLOATTHREAD)) !=
		(nSort_ & (SORT_THREAD_MASK | SORT_FLOATTHREAD));
	sort(nSort, true, bUpdateParentLink);
	
	nSort_ = nSort;
	
	fireSorted();
}

unsigned int qm::ViewModel::getSort() const
{
	return nSort_;
}

void qm::ViewModel::setFilter(const Filter* pFilter)
{
	if (pFilter)
		pFilter_.reset(new Filter(*pFilter));
	else
		pFilter_.reset(0);
	update(true, -1);
}

const Filter* qm::ViewModel::getFilter() const
{
	return pFilter_.get();
}

void qm::ViewModel::setMode(unsigned int nMode,
							unsigned int nMask)
{
	nMode_ = (nMode & nMask) | (nMode_ & ~nMask);
}

unsigned int qm::ViewModel::getMode() const
{
	return nMode_;
}

void qm::ViewModel::addSelection(unsigned int n)
{
	assert(isLocked());
	assert(n < getCount());
	
	ViewModelItem* pItem = listItem_[n];
	if (!(pItem->getFlags() & ViewModelItem::FLAG_SELECTED)) {
		pItem->setFlags(ViewModelItem::FLAG_SELECTED, ViewModelItem::FLAG_SELECTED);
		fireItemStateChanged(n, true);
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
		fireItemStateChanged(n, true);
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
		removeSelection(static_cast<unsigned int>(n));
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
	
	return static_cast<unsigned int>(std::count_if(
		listItem_.begin(), listItem_.end(),
		std::bind2nd(
			std::mem_fun(&ViewModelItem::isFlag),
			ViewModelItem::FLAG_SELECTED)));
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

void qm::ViewModel::setFocused(unsigned int n,
							   bool bDelay)
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
		
		fireItemStateChanged(nOld, bDelay);
		fireItemStateChanged(n, bDelay);
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

unsigned int qm::ViewModel::getScroll() const
{
	return nScroll_;
}

void qm::ViewModel::setScroll(unsigned int nScroll)
{
	nScroll_ = nScroll;
}

void qm::ViewModel::payAttention(unsigned int n)
{
	fireItemAttentionPaid(n);
}

void qm::ViewModel::invalidateColors(const ColorManager* pColorManager)
{
	Lock<ViewModel> lock(*this);
	
	pColorList_ = pColorManager->getColorList(pFolder_);
	
	std::for_each(listItem_.begin(), listItem_.end(),
		std::mem_fun(&ViewModelItem::invalidateColor));
	
	fireColorChanged();
}

void qm::ViewModel::save() const
{
	pDataItem_->setSort(nSort_);
	pDataItem_->setFocus(nFocused_);
	pDataItem_->setFilter(pFilter_.get() ? pFilter_->getName() : 0);
	pDataItem_->setMode(nMode_);
}

void qm::ViewModel::destroy()
{
	fireDestroyed();
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
		pFolder_->getAccount(), pDocument_, 0, pProfile_, 0,
		MacroContext::FLAG_UITHREAD | MacroContext::FLAG_GETMESSAGEASPOSSIBLE,
		/*pSecurityModel_->getSecurityMode()*/SECURITYMODE_NONE, 0, 0);
	return pMacro->value(&context);
}

bool qm::ViewModel::isMode(Mode mode) const
{
	return (nMode_ & mode) != 0;
}

void qm::ViewModel::setMode(Mode mode,
							bool b)
{
	unsigned int nMode = nMode_;
	if (b)
		nMode_ |= mode;
	else
		nMode_ &= ~mode;
	
	if (nMode_ != nMode)
		fireMessageViewModeChanged(mode, b);
}

void qm::ViewModel::messageAdded(const FolderMessageEvent& event)
{
	assert(event.getFolder() == pFolder_);
	
	Lock<ViewModel> lock(*this);
	
	// TODO
	// Resort if this view model is sorted by flags
	
	bool bAdded = false;
	const MessageHolderList& l = event.getMessageHolders();
	for (MessageHolderList::const_iterator itM = l.begin(); itM != l.end(); ++itM) {
		MessageHolder* pmh = *itM;
		
		bool bAdd = true;
		if (pFilter_.get()) {
			Message msg;
			MacroContext context(pmh, &msg, MessageHolderList(),
				pFolder_->getAccount(), pDocument_, 0, pProfile_, 0,
				MacroContext::FLAG_UITHREAD | MacroContext::FLAG_GETMESSAGEASPOSSIBLE,
				/*pSecurityModel_->getSecurityMode()*/SECURITYMODE_NONE, 0, 0);
			bAdd = pFilter_->match(&context);
		}
		
		if (bAdd) {
			ViewModelItemPtr pItem(pmh, nCacheCount_);
			
			if ((getSort() & SORT_THREAD_MASK) == SORT_THREAD) {
				ItemList::iterator itParent = listItem_.end();
				
				unsigned int nReferenceHash = pmh->getReferenceHash();
				if (nReferenceHash != 0) {
					wstring_ptr wstrReference(pmh->getReference());
					
					for (itParent = listItem_.begin(); itParent != listItem_.end(); ++itParent) {
						MessageHolder* pmhParent = (*itParent)->getMessageHolder();
						if (pmhParent->getMessageIdHash() == nReferenceHash) {
							wstring_ptr wstrMessageId(pmhParent->getMessageId());
							if (wcscmp(wstrReference.get(), wstrMessageId.get()) == 0)
								break;
						}
					}
				}
				
				if (itParent != listItem_.end()) {
					ViewModelItem* pParentItem = *itParent;
					pItem->setParentItem(pParentItem);
					if (isFloatThread(nSort_)) {
						ViewModelItemComp comp(getComparator(nSort_));
						if (pParentItem->updateLatestItem(pItem.get(), comp)) {
							ItemList::iterator itThreadBegin = itParent;
							while ((*itThreadBegin)->getParentItem())
								--itThreadBegin;
							ItemList::iterator itThreadEnd = itParent + 1;
							while (itThreadEnd != listItem_.end() && (*itThreadEnd)->getParentItem())
								++itThreadEnd;
							
							if ((nSort_ & SORT_DIRECTION_MASK) == SORT_ASCENDING) {
								ItemList::iterator itInsert = itThreadEnd;
								while (itInsert != listItem_.end()) {
									if (!(*itInsert)->getParentItem() &&
										comp(*itThreadBegin, *itInsert))
										break;
									++itInsert;
								}
								
								if (itInsert != itThreadEnd) {
									SelectionRestorer restorer(this, false, false);
									ItemList l(itThreadBegin, itThreadEnd);
									std::copy(l.begin(), l.end(),
										std::copy(itThreadEnd, itInsert, itThreadBegin));
									restorer.restore();
								}
							}
							else {
								ItemList::iterator itInsert = itThreadBegin;
								for (ItemList::iterator it = itThreadBegin; it != listItem_.begin(); ) {
									--it;
									if (!(*it)->getParentItem()) {
										if (comp(*it, *itThreadBegin))
											break;
										itInsert = it;
									}
								}
								
								if (itInsert != itThreadBegin) {
									SelectionRestorer restorer(this, false, false);
									ItemList l(itThreadBegin, itThreadEnd);
									std::copy_backward(l.begin(), l.end(),
										std::copy_backward(itInsert, itThreadBegin, itThreadEnd));
									restorer.restore();
								}
							}
						}
					}
				}
			}
			
			ItemList::iterator it = std::lower_bound(
				listItem_.begin(), listItem_.end(), pItem.get(),
				getComparator(nSort_));
			
			ItemList::iterator itInsert = listItem_.insert(it, pItem.get());
			pItem.release();
			
			unsigned int nPos = static_cast<unsigned int>(itInsert - listItem_.begin());
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
			
			if (!pmh->isSeen())
				++nUnseenCount_;
			
			bAdded = true;
		}
	}
	
	if (bAdded)
		fireItemAdded();
}

void qm::ViewModel::messageRemoved(const FolderMessageEvent& event)
{
	assert(event.getFolder() == pFolder_);
	
	Lock<ViewModel> lock(*this);
	
	bool bSort = false;
	const MessageHolderList& l = event.getMessageHolders();
	
	typedef std::vector<unsigned int> IndexList;
	IndexList listIndex;
	listIndex.reserve(l.size());
	for (MessageHolderList::const_iterator it = l.begin(); it != l.end(); ++it) {
		unsigned int nIndex = getIndex(*it);
		if (nIndex != -1)
			listIndex.push_back(nIndex);
	}
	if (listIndex.empty())
		return;
	std::sort(listIndex.begin(), listIndex.end(), std::greater<unsigned int>());
	
	typedef std::vector<int> ThreadList;
	ThreadList listThread;
	if (isFloatThread(nSort_)) {
		listThread.resize(listItem_.size());
		for (IndexList::size_type n = 0; n < listIndex.size(); ++n) {
			unsigned int nIndex = listIndex[n];
			
			unsigned int nBegin = nIndex;
			while (nBegin != 0 && listItem_[nBegin]->getParentItem())
				--nBegin;
			
			unsigned int nEnd = nIndex + 1;
			while (nEnd < listItem_.size() && listItem_[nEnd]->getParentItem())
				++nEnd;
			
			while (nBegin < nEnd)
				listThread[nBegin++] = 1;
		}
		
		for (IndexList::size_type n = 0; n < listIndex.size(); ++n)
			listThread.erase(listThread.begin() + listIndex[n]);
	}
	
	for (IndexList::const_iterator itI = listIndex.begin(); itI != listIndex.end(); ++itI) {
		unsigned int nIndex = *itI;
		ItemList::iterator it = listItem_.begin() + nIndex;
		
		MessageHolder* pmh = (*it)->getMessageHolder();
		bool bHasFocus = ((*it)->getFlags() & ViewModelItem::FLAG_FOCUSED) != 0;
		bool bSelected = ((*it)->getFlags() & ViewModelItem::FLAG_SELECTED) != 0;
		
		if ((getSort() & SORT_THREAD_MASK) == SORT_THREAD) {
			ViewModelItem* pItem = *it;
			unsigned int nLevel = pItem->getLevel();
			
			ItemList::const_iterator itEnd = it + 1;
			while (itEnd != listItem_.end() && (*itEnd)->getLevel() > nLevel)
				++itEnd;
			
			for (ItemList::const_iterator itC = it + 1; itC != itEnd; ++itC) {
				if ((*itC)->getParentItem() == pItem) {
					(*itC)->setParentItem(0);
					bSort = true;
				}
			}
			assert(std::find_if(listItem_.begin(), listItem_.end(),
				std::bind2nd(
					binary_compose_f_gx_hy(
						std::equal_to<ViewModelItem*>(),
						std::mem_fun(&ViewModelItem::getParentItem),
						std::identity<ViewModelItem*>()),
					pItem)) == listItem_.end());
		}
		
		ViewModelItem::deleteItem(*it, nCacheCount_);
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
					nLastSelection_ = static_cast<unsigned int>(listItem_.size() - 1);
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
		
		if (!pmh->isSeen())
			--nUnseenCount_;
	}
	
	if (isFloatThread(nSort_)) {
		assert(listThread.size() == listItem_.size());
		
		ViewModelItemComp comp(getComparator(nSort_));
		for (ThreadList::size_type n = 0; n < listThread.size(); ++n) {
			if (listThread[n]) {
				ViewModelItem* pItem = listItem_[n];
				pItem->clearLatestItem();
				ViewModelItem* pParentItem = pItem->getParentItem();
				if (pParentItem)
					pParentItem->updateLatestItem(pItem, comp);
				
				bSort = true;
			}
		}
	}
	
	if (bSort)
		sort(nSort_, true, false);
	
	fireItemRemoved();
}

void qm::ViewModel::messageRefreshed(const FolderEvent& event)
{
	update(true, -1);
}

void qm::ViewModel::folderDestroyed(const FolderEvent& event)
{
	pViewModelManager_->removeViewModel(this);
}

void qm::ViewModel::messageHolderChanged(const MessageHolderEvent& event)
{
	Lock<ViewModel> lock(*this);
	
	MessageHolder* pmh = event.getMessageHolder();
	unsigned int n = getIndex(pmh);
	if (n != -1) {
		Account* pAccount = pmh->getAccount();
		assert(pAccount == pFolder_->getAccount());
		unsigned int nOldFlags = event.getOldFlags();
		unsigned int nNewFlags = event.getNewFlags();
		if (pAccount->isSeen(nOldFlags) && !pAccount->isSeen(nNewFlags))
			++nUnseenCount_;
		else if (!pAccount->isSeen(nOldFlags) && pAccount->isSeen(nNewFlags))
			--nUnseenCount_;
		
		fireItemChanged(n);
	}
}

void qm::ViewModel::messageHolderDestroyed(const MessageHolderEvent& event)
{
}

void qm::ViewModel::update(bool bRestoreSelection,
						   unsigned int nOldCacheCount)
{
	if (nOldCacheCount == -1)
		nOldCacheCount = nCacheCount_;
	
	Lock<ViewModel> lock(*this);
	
	// TODO
	// Check error
	if (!pFolder_->loadMessageHolders())
		return;
	
	SelectionRestorer restorer(this, true, !bRestoreSelection);
	
	for (ItemList::iterator it = listItem_.begin(); it != listItem_.end(); ++it)
		ViewModelItem::deleteItem(*it, nOldCacheCount);
	listItem_.clear();
	
	unsigned int nCount = pFolder_->getCount();
	listItem_.reserve(nCount);
	
	nUnseenCount_ = 0;
	
	MacroVariableHolder globalVariable;
	for (unsigned int n = 0; n < nCount; ++n) {
		MessageHolder* pmh = pFolder_->getMessage(n);
		bool bAdd = true;
		if (pFilter_.get()) {
			Message msg;
			MacroContext context(pmh, &msg, MessageHolderList(),
				pFolder_->getAccount(), pDocument_, 0, pProfile_, 0,
				MacroContext::FLAG_UITHREAD | MacroContext::FLAG_GETMESSAGEASPOSSIBLE,
				/*pSecurityModel_->getSecurityMode()*/SECURITYMODE_NONE, 0, &globalVariable);
			bAdd = pFilter_->match(&context);
		}
		if (bAdd) {
			ViewModelItemPtr pItem(pmh, nCacheCount_);
			listItem_.push_back(pItem.release());
			
			if (!pmh->isSeen())
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
		makeParentLink(isFloatThread(nSort));
	
	std::stable_sort(listItem_.begin(), listItem_.end(), getComparator(nSort));
	
	if (bRestoreSelection)
		restorer.restore();
}

ViewModelItemComp qm::ViewModel::getComparator(unsigned int nSort) const
{
	return ViewModelItemComp(this, getColumn(nSort & SORT_INDEX_MASK),
		(nSort & SORT_DIRECTION_MASK) == SORT_ASCENDING,
		(nSort & SORT_THREAD_MASK) == SORT_THREAD, isFloatThread(nSort));
}

bool qm::ViewModel::isFloatThread(unsigned int nSort) const
{
	return (nSort & SORT_THREAD_MASK) == SORT_THREAD && nSort & SORT_FLOATTHREAD;
}

void qm::ViewModel::makeParentLink(bool bUpdateLatest)
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
	
	for (ItemList::iterator it = listItem_.begin(); it != listItem_.end(); ++it) {
		ViewModelItem* pItem = *it;
		makeParentLink(listItemSortedByMessageIdHash, listItemSortedByPointer, pItem);
		if (bUpdateLatest)
			pItem->clearLatestItem();
	}
	if (bUpdateLatest) {
		ViewModelItemComp comp(getComparator(nSort_));
		for (ItemList::iterator it = listItem_.begin(); it != listItem_.end(); ++it) {
			ViewModelItem* pItem = *it;
			ViewModelItem* pParentItem = pItem->getParentItem();
			if (pParentItem)
				pParentItem->updateLatestItem(pItem->getLatestItem(), comp);
		}
	}
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

void qm::ViewModel::updateCacheCount()
{
	nCacheCount_ = 0;
	const ViewColumnList& listColumn = getColumns();
	for (ViewColumnList::const_iterator it = listColumn.begin(); it != listColumn.end(); ++it) {
		ViewColumn* pColumn = *it;
		if (pColumn->isFlag(ViewColumn::FLAG_CACHE)) {
			pColumn->setCacheIndex(nCacheCount_);
			++nCacheCount_;
		}
	}
}

void qm::ViewModel::fireItemAdded() const
{
	fireEvent(ViewModelEvent(this), &ViewModelHandler::itemAdded);
}

void qm::ViewModel::fireItemRemoved() const
{
	fireEvent(ViewModelEvent(this), &ViewModelHandler::itemRemoved);
}

void qm::ViewModel::fireItemChanged(unsigned int nItem) const
{
	fireEvent(ViewModelEvent(this, nItem), &ViewModelHandler::itemChanged);
}

void qm::ViewModel::fireItemStateChanged(unsigned int nItem,
										 bool bDelay) const
{
	fireEvent(ViewModelEvent(this, nItem, bDelay), &ViewModelHandler::itemStateChanged);
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

void qm::ViewModel::fireColorChanged() const
{
	fireEvent(ViewModelEvent(this), &ViewModelHandler::colorChanged);
}

void qm::ViewModel::fireColumnChanged() const
{
	fireEvent(ViewModelEvent(this), &ViewModelHandler::columnChanged);
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

void qm::DefaultViewModelHandler::colorChanged(const ViewModelEvent& event)
{
}

void qm::DefaultViewModelHandler::columnChanged(const ViewModelEvent& event)
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
	nItem_(-1),
	bDelay_(true)
{
}

qm::ViewModelEvent::ViewModelEvent(const ViewModel* pViewModel,
								   unsigned int nItem) :
	pViewModel_(pViewModel),
	nItem_(nItem),
	bDelay_(true)
{
}

qm::ViewModelEvent::ViewModelEvent(const ViewModel* pViewModel,
								   unsigned int nItem,
								   bool bDelay) :
	pViewModel_(pViewModel),
	nItem_(nItem),
	bDelay_(bDelay)
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

bool qm::ViewModelEvent::isDelay() const
{
	return bDelay_;
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

qm::ViewModelManager::ViewModelManager(Document* pDocument,
									   Profile* pProfile,
									   SecurityModel* pSecurityModel) :
	pDocument_(pDocument),
	pProfile_(pProfile),
	pSecurityModel_(pSecurityModel),
	pCurrentAccount_(0),
	pCurrentViewModel_(0)
{
	const Application& app = Application::getApplication();
	pDefaultViewData_.reset(new DefaultViewData(app.getProfilePath(FileNames::VIEWS_XML).get()));
	pFilterManager_.reset(new FilterManager(app.getProfilePath(FileNames::FILTERS_XML).get()));
	pColorManager_.reset(new ColorManager(app.getProfilePath(FileNames::COLORS_XML).get()));
	pColorManager_->addColorManagerHandler(this);
}

qm::ViewModelManager::~ViewModelManager()
{
	pColorManager_->removeColorManagerHandler(this);
	
	std::for_each(listViewModel_.begin(),
		listViewModel_.end(), deleter<ViewModel>());
	std::for_each(mapViewData_.begin(), mapViewData_.end(),
		unary_compose_f_gx(
			deleter<ViewData>(),
			std::select2nd<ViewDataMap::value_type>()));
}

DefaultViewData* qm::ViewModelManager::getDefaultViewData() const
{
	return pDefaultViewData_.get();
}

ColorManager* qm::ViewModelManager::getColorManager() const
{
	return pColorManager_.get();
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
	
	const Filter* pFilter = 0;
	if (pViewDataItem->getFilter())
		pFilter = pFilterManager_->getFilter(pViewDataItem->getFilter());
	
	std::auto_ptr<ViewModel> pViewModel(new ViewModel(
		this, pFolder, pViewDataItem, pFilter, pProfile_,
		pDocument_, pSecurityModel_, pColorManager_.get()));
	listViewModel_.push_back(pViewModel.get());
	
	return pViewModel.release();
}

bool qm::ViewModelManager::save(bool bForce) const
{
	for (ViewModelList::const_iterator it = listViewModel_.begin(); it != listViewModel_.end(); ++it)
		(*it)->save();
	
	for (ViewDataMap::const_iterator it = mapViewData_.begin(); it != mapViewData_.end(); ++it) {
		if (!(*it).second->save() && !bForce)
			return false;
	}
	
	if (!pDefaultViewData_->save() && !bForce)
		return false;
	
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
	pViewModel->destroy();
	delete pViewModel;
}

void qm::ViewModelManager::accountDestroyed(const AccountEvent& event)
{
	Account* pAccount = event.getAccount();
	
	if (pAccount == pCurrentAccount_)
		setCurrentFolder(0, 0);
	
	ViewModelList::iterator itV = listViewModel_.begin();
	while (itV != listViewModel_.end()) {
		ViewModel* pViewModel = *itV;
		if (pViewModel->getFolder()->getAccount() == pAccount) {
			itV = listViewModel_.erase(itV);
			pViewModel->destroy();
			delete pViewModel;
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

void qm::ViewModelManager::colorSetsChanged(const ColorManagerEvent& event)
{
	invalidateColors();
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
		std::auto_ptr<ViewData> pNewViewData(new ViewData(pDefaultViewData_.get(), wstrPath.get()));
		mapViewData_.push_back(std::make_pair(pAccount, pNewViewData.get()));
		pAccount->addAccountHandler(this);
		pViewData = pNewViewData.release();
	}
	
	return pViewData->getItem(pFolder);
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

void qm::ViewModelManager::invalidateColors()
{
//	std::for_each(listViewModel_.begin(), listViewModel_.end(),
//		std::bind2nd(
//			std::mem_fun(&ViewModel::invalidateColors),
//			pColorManager_.get()));
	for (ViewModelList::const_iterator it = listViewModel_.begin(); it != listViewModel_.end(); ++it)
		(*it)->invalidateColors(pColorManager_.get());
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
										 bool bThread,
										 bool bFloat) :
	pViewModel_(pViewModel),
	column_(column),
	bAscending_(bAscending),
	bThread_(bThread),
	bFloat_(bFloat)
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
	unsigned int nLevel = -1;
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
			if (bFloat_)
				nLevel = pLhs->getLevel();
		}
	}
	
	if (!bFixed) {
		bool bLatest = bThread_ && bFloat_ && nLevel == 0;
		int nComp = compare(bLatest ? pLhs->getLatestItem() : pLhs,
			bLatest ? pRhs->getLatestItem() : pRhs);
		if (bThread_ && nComp == 0) {
			MessageHolder* pmhLhs = pLhs->getMessageHolder();
			MessageHolder* pmhRhs = pRhs->getMessageHolder();
			nComp = pmhLhs->getId() < pmhRhs->getId() ? -1 :
				pmhLhs->getId() > pmhRhs->getId() ? 1 : 0;
		}
		if (!bAscending_)
			nComp = -nComp;
		bLess = nComp < 0;
	}
	
	return bLess;
}

int qm::ViewModelItemComp::compare(const ViewModelItem* pLhs,
								   const ViewModelItem* pRhs) const
{
	unsigned int nFlags = column_.getFlags();
	if ((nFlags & ViewColumn::FLAG_SORT_MASK) == ViewColumn::FLAG_SORT_NUMBER) {
		unsigned int nLhs = column_.getNumber(pViewModel_, pLhs);
		unsigned int nRhs = column_.getNumber(pViewModel_, pRhs);
		return nLhs < nRhs ? -1 : nLhs > nRhs ? 1 : 0;
	}
	else if ((nFlags & ViewColumn::FLAG_SORT_MASK) == ViewColumn::FLAG_SORT_DATE) {
		Time timeLhs;
		column_.getTime(pViewModel_, pLhs, &timeLhs);
		Time timeRhs;
		column_.getTime(pViewModel_, pRhs, &timeRhs);
		return timeLhs < timeRhs ? -1 : timeLhs > timeRhs ? 1 : 0;
	}
	else {
		wstring_ptr wstrLhs(column_.getText(pViewModel_, pLhs));
		wstring_ptr wstrRhs(column_.getText(pViewModel_, pRhs));
		return _wcsicmp(wstrLhs.get(), wstrRhs.get());
	}
}


/****************************************************************************
 *
 * ViewData
 *
 */

qm::ViewData::ViewData(DefaultViewData* pDefaultViewData,
					   const WCHAR* pwszPath) :
	pDefaultViewData_(pDefaultViewData)
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

ViewDataItem* qm::ViewData::getItem(const Folder* pFolder)
{
	assert(pFolder);
	
	unsigned int nId = pFolder->getId();
	
	ViewDataItem item(nId);
	ItemList::iterator it = std::lower_bound(
		listItem_.begin(), listItem_.end(),
		&item,
		binary_compose_f_gx_hy(
			std::less<unsigned int>(),
			std::mem_fun(&ViewDataItem::getFolderId),
			std::mem_fun(&ViewDataItem::getFolderId)));
	if (it != listItem_.end() && (*it)->getFolderId() == nId) {
		return *it;
	}
	else {
		const WCHAR* pwszClass = pFolder->getAccount()->getClass();
		ViewDataItem* pDefaultItem = pDefaultViewData_->getItem(pwszClass);
		std::auto_ptr<ViewDataItem> pItem(pDefaultItem->clone(nId));
		listItem_.insert(it, pItem.get());
		return pItem.release();
	}
}

bool qm::ViewData::save() const
{
	TemporaryFileRenamer renamer(wstrPath_.get());
	
	FileOutputStream stream(renamer.getPath());
	if (!stream)
		return false;
	BufferedOutputStream bufferedStream(&stream, false);
	OutputStreamWriter writer(&bufferedStream, false, L"utf-8");
	if (!writer)
		return false;
	
	ViewDataWriter viewDataWriter(&writer);
	if (!viewDataWriter.write(this))
		return false;
	
	if (!writer.close())
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


/****************************************************************************
 *
 * DefaultViewData
 *
 */

qm::DefaultViewData::DefaultViewData(const WCHAR* pwszPath)
{
	wstrPath_ = allocWString(pwszPath);
	
	XMLReader reader;
	ViewDataContentHandler contentHandler(this);
	reader.setContentHandler(&contentHandler);
	if (!reader.parse(wstrPath_.get())) {
		// TODO
	}
}

qm::DefaultViewData::~DefaultViewData()
{
	std::for_each(listItem_.begin(), listItem_.end(),
		unary_compose_fx_gx(
			string_free<WSTRING>(),
			qs::deleter<ViewDataItem>()));
}

const DefaultViewData::ItemList& qm::DefaultViewData::getItems() const
{
	return listItem_;
}

ViewDataItem* qm::DefaultViewData::getItem(const WCHAR* pwszClass)
{
	ItemList::const_iterator it = std::find_if(
		listItem_.begin(), listItem_.end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				string_equal<WCHAR>(),
				std::select1st<ItemList::value_type>(),
				std::identity<const WCHAR*>()),
			pwszClass));
	if (it == listItem_.end()) {
		wstring_ptr wstrClass(allocWString(pwszClass));
		std::auto_ptr<ViewDataItem> pItem(createDefaultItem());
		listItem_.push_back(ItemList::value_type(wstrClass.get(), pItem.get()));
		wstrClass.release();
		return pItem.release();
	}
	else {
		return (*it).second;
	}
}

void qm::DefaultViewData::setItem(const WCHAR* pwszClass,
								  std::auto_ptr<ViewDataItem> pItem)
{
	wstring_ptr wstrClass(allocWString(pwszClass));
	
	ItemList::iterator it = std::find_if(
		listItem_.begin(), listItem_.end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				string_equal<WCHAR>(),
				std::select1st<ItemList::value_type>(),
				std::identity<const WCHAR*>()),
			pwszClass));
	if (it != listItem_.end()) {
		freeWString((*it).first);
		delete (*it).second;
		(*it).first = wstrClass.release();
		(*it).second = pItem.release();
	}
	else {
		listItem_.push_back(ItemList::value_type(wstrClass.get(), pItem.get()));
		wstrClass.release();
		pItem.release();
	}
}

bool qm::DefaultViewData::save() const
{
	return ConfigSaver<const DefaultViewData*, ViewDataWriter>::save(this, wstrPath_.get());
}

std::auto_ptr<ViewDataItem> qm::DefaultViewData::createDefaultItem()
{
	std::auto_ptr<ViewDataItem> pItem(new ViewDataItem(0));
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
	nSort_(ViewModel::SORT_ASCENDING | ViewModel::SORT_NOTHREAD | 1),
	nMode_(MessageViewMode::MODE_QUOTE)
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

void qm::ViewDataItem::setColumns(const ViewColumnList& listColumn)
{
	std::for_each(listColumn_.begin(), listColumn_.end(), deleter<ViewColumn>());
	listColumn_ = listColumn;
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

const WCHAR* qm::ViewDataItem::getFilter() const
{
	return wstrFilter_.get();
}

void qm::ViewDataItem::setFilter(const WCHAR* pwszFilter)
{
	if (pwszFilter)
		wstrFilter_ = allocWString(pwszFilter);
	else
		wstrFilter_.reset(0);
}

unsigned int qm::ViewDataItem::getMode() const
{
	return nMode_;
}

void qm::ViewDataItem::setMode(unsigned int nMode)
{
	nMode_ = nMode;
}

std::auto_ptr<ViewDataItem> qm::ViewDataItem::clone(unsigned int nFolderId) const
{
	std::auto_ptr<ViewDataItem> pItem(new ViewDataItem(nFolderId));
	
	for (ViewColumnList::const_iterator it = listColumn_.begin(); it != listColumn_.end(); ++it)
		pItem->addColumn((*it)->clone());
	pItem->setSort(nSort_);
	pItem->setFilter(wstrFilter_.get());
	pItem->setMode(nMode_);
	
	return pItem;
}


/****************************************************************************
*
* ViewDataContentHandler
*
*/

qm::ViewDataContentHandler::ViewDataContentHandler(ViewData* pData) :
	pData_(pData),
	pDefaultData_(0),
	state_(STATE_ROOT),
	type_(ViewColumn::TYPE_NONE),
	nFlags_(-1),
	nWidth_(-1),
	nSort_(-1)
{
}

qm::ViewDataContentHandler::ViewDataContentHandler(DefaultViewData* pDefaultData) :
	pData_(0),
	pDefaultData_(pDefaultData),
	state_(STATE_ROOT),
	type_(ViewColumn::TYPE_NONE),
	nFlags_(-1),
	nWidth_(-1),
	nSort_(-1)
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
		const WCHAR* pwszClass = 0;
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrLocalName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrLocalName, L"folder") == 0)
				pwszFolder = attributes.getValue(n);
			else if (wcscmp(pwszAttrLocalName, L"class") == 0)
				pwszClass = attributes.getValue(n);
			else
				return false;
		}
		
		unsigned int nFolderId = 0;
		if (pDefaultData_) {
			// TODO
			// Ignore error for compatibility.
			// Should be removed in the future.
			if (pwszClass)
				wstrClass_ = allocWString(pwszClass);
		}
		else {
			if (!pwszFolder)
				return false;
			
			WCHAR* pEnd = 0;
			nFolderId = wcstol(pwszFolder, &pEnd, 10);
			if (*pEnd)
				return false;
		}
		
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
			else if (wcscmp(pwszAttrLocalName, L"cache") == 0)
				nFlags |= wcscmp(attributes.getValue(n), L"true") == 0 ?
					ViewColumn::FLAG_CACHE : 0;
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
			else if (wcscmp(pwszAttrLocalName, L"floatthread") == 0)
				nSort |= wcscmp(attributes.getValue(n), L"true") == 0 ?
					ViewModel::SORT_FLOATTHREAD : 0;
			else
				return false;
		}
		nSort_ = nSort;
		
		state_ = STATE_SORT;
	}
	else if (wcscmp(pwszLocalName, L"mode") == 0) {
		if (state_ != STATE_VIEW)
			return false;
		
		struct Item {
			const WCHAR* pwszName_;
			MessageViewMode::Mode mode_;
		} items[] = {
			{ L"raw",			MessageViewMode::MODE_RAW			},
			{ L"html",			MessageViewMode::MODE_HTML			},
			{ L"htmlonline",	MessageViewMode::MODE_HTMLONLINE	},
			{ L"select",		MessageViewMode::MODE_SELECT		},
			{ L"quote",			MessageViewMode::MODE_QUOTE			},
			{ L"internetzone",	MessageViewMode::MODE_INTERNETZONE	}
		};
		
		unsigned int nMode = 0;
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrLocalName = attributes.getLocalName(n);
			int m = 0;
			for (m = 0; m < countof(items); ++m) {
				if (wcscmp(pwszAttrLocalName, items[m].pwszName_) == 0) {
					if (wcscmp(attributes.getValue(n), L"true") == 0)
						nMode |= items[m].mode_;
					break;
				}
			}
			if (m == countof(items))
				return false;
		}
		assert(pItem_.get());
		pItem_->setMode(nMode);
		
		state_ = STATE_MODE;
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
			{ L"filter",	STATE_VIEW,		STATE_FILTER	},
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
		
		if (pDefaultData_) {
			// TODO
			// Ignore error for compatiblity.
			// Should be removed in the future.
			if (wstrClass_.get())
				pDefaultData_->setItem(wstrClass_.get(), pItem_);
			wstrClass_.reset(0);
		}
		else {
			pData_->addItem(pItem_);
		}
		
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
			pMacro_ = MacroParser().parse(pwszMacro);
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
	else if (wcscmp(pwszLocalName, L"filter") == 0) {
		assert(state_ == STATE_FILTER);
		
		if (buffer_.getLength() != 0) {
			pItem_->setFilter(buffer_.getCharArray());
			buffer_.remove();
		}
		
		state_ = STATE_VIEW;
	}
	else if (wcscmp(pwszLocalName, L"mode") == 0) {
		assert(state_ == STATE_MODE);
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
		state_ == STATE_SORT ||
		state_ == STATE_FILTER) {
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
	for (ViewData::ItemList::const_iterator it = listItem.begin(); it != listItem.end(); ++it) {
		if (!write(*it, 0))
			return false;
	}
	
	if (!handler_.endElement(0, 0, L"views"))
		return false;
	if (!handler_.endDocument())
		return false;
	
	return true;
}

bool qm::ViewDataWriter::write(const DefaultViewData* pData)
{
	if (!handler_.startDocument())
		return false;
	if (!handler_.startElement(0, 0, L"views", DefaultAttributes()))
		return false;
	
	const DefaultViewData::ItemList& listItem = pData->getItems();
	for (DefaultViewData::ItemList::const_iterator it = listItem.begin(); it != listItem.end(); ++it) {
		if (!write((*it).second, (*it).first))
			return false;
	}
	
	if (!handler_.endElement(0, 0, L"views"))
		return false;
	if (!handler_.endDocument())
		return false;
	
	return true;
}

bool qm::ViewDataWriter::write(const ViewDataItem* pItem,
							   const WCHAR* pwszClass)
{
	if (pwszClass) {
		SimpleAttributes viewAttrs(L"class", pwszClass);
		if (!handler_.startElement(0, 0, L"view", viewAttrs))
			return false;
	}
	else {
		WCHAR wszFolder[32];
		swprintf(wszFolder, L"%u", pItem->getFolderId());
		SimpleAttributes viewAttrs(L"folder", wszFolder);
		if (!handler_.startElement(0, 0, L"view", viewAttrs))
			return false;
	}
	
	if (!handler_.startElement(0, 0, L"columns", DefaultAttributes()))
		return false;
	
	const ViewColumnList& listColumn = pItem->getColumns();
	for (ViewColumnList::const_iterator itC = listColumn.begin(); itC != listColumn.end(); ++itC) {
		ViewColumn* pColumn = *itC;
		
		unsigned int nFlags = pColumn->getFlags();
		const WCHAR* pwszSort = 0;
		switch (nFlags & ViewColumn::FLAG_SORT_MASK) {
		case ViewColumn::FLAG_SORT_TEXT:
			pwszSort = L"text";
			break;
		case ViewColumn::FLAG_SORT_NUMBER:
			pwszSort = L"number";
			break;
		case ViewColumn::FLAG_SORT_DATE:
			pwszSort = L"date";
			break;
		default:
			assert(false);
			break;
		}
		const SimpleAttributes::Item items[] = {
			{ L"indent",	nFlags & ViewColumn::FLAG_INDENT ? L"true" : L"false",		(nFlags & ViewColumn::FLAG_INDENT) == 0		},
			{ L"line",		nFlags & ViewColumn::FLAG_LINE ? L"true" : L"false",		(nFlags & ViewColumn::FLAG_LINE) == 0		},
			{ L"icon",		nFlags & ViewColumn::FLAG_ICON ? L"true" : L"false",		(nFlags & ViewColumn::FLAG_ICON) == 0		},
			{ L"cache",		nFlags & ViewColumn::FLAG_CACHE ? L"true" : L"false",		(nFlags & ViewColumn::FLAG_CACHE) == 0		},
			{ L"align",		nFlags & ViewColumn::FLAG_RIGHTALIGN ? L"right" : L"left",	(nFlags & ViewColumn::FLAG_RIGHTALIGN) == 0	},
			{ L"sort",		pwszSort																								}
		};
		SimpleAttributes columnAttrs(items, countof(items));
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
	
	unsigned int nSort = pItem->getSort();
	const SimpleAttributes::Item sortItems[] = {
		{ L"direction",		(nSort & ViewModel::SORT_DIRECTION_MASK) == ViewModel::SORT_ASCENDING ? L"ascending" : L"descending"	},
		{ L"thread",		(nSort & ViewModel::SORT_THREAD_MASK) == ViewModel::SORT_THREAD ? L"true" : L"false"					},
		{ L"floatthread",	nSort & ViewModel::SORT_FLOATTHREAD ? L"true" : L"false"												}
	};
	SimpleAttributes sortAttrs(sortItems, countof(sortItems));
	if (!handler_.startElement(0, 0, L"sort", sortAttrs))
		return false;
	WCHAR wsz[32];
	swprintf(wsz, L"%u", pItem->getSort() & ViewModel::SORT_INDEX_MASK);
	if (!handler_.characters(wsz, 0, wcslen(wsz)))
		return false;
	if (!handler_.endElement(0, 0, L"sort"))
		return false;
	
	if (pItem->getFilter()) {
		if (!HandlerHelper::textElement(&handler_, L"filter", pItem->getFilter(), -1))
			return false;
	}
	
	unsigned int nMode = pItem->getMode();
	const SimpleAttributes::Item modeItems[] = {
		{ L"raw",			nMode & MessageViewMode::MODE_RAW ? L"true" : L"false",				(nMode & MessageViewMode::MODE_RAW) == 0			},
		{ L"html",			nMode & MessageViewMode::MODE_HTML ? L"true" : L"false",			(nMode & MessageViewMode::MODE_HTML) == 0			},
		{ L"htmlonline",	nMode & MessageViewMode::MODE_HTMLONLINE ? L"true" : L"false",		(nMode & MessageViewMode::MODE_HTMLONLINE) == 0		},
		{ L"select",		nMode & MessageViewMode::MODE_SELECT ? L"true" : L"false",			(nMode & MessageViewMode::MODE_SELECT) == 0			},
		{ L"quote",			nMode & MessageViewMode::MODE_QUOTE ? L"true" : L"false",			(nMode & MessageViewMode::MODE_QUOTE) == 0			},
		{ L"internetzone",	nMode & MessageViewMode::MODE_INTERNETZONE ? L"true" : L"false",	(nMode & MessageViewMode::MODE_INTERNETZONE) == 0	},
	};
	SimpleAttributes modeAttrs(modeItems, countof(modeItems));
	if (!handler_.startElement(0, 0, L"mode", modeAttrs) ||
		!handler_.endElement(0, 0, L"mode"))
		return false;
	
	if (!handler_.endElement(0, 0, L"view"))
		return false;
	
	return true;
}
