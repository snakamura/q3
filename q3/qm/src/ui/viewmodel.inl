/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __VIEWMODEL_INL__
#define __VIEWMODEL_INL__

#include <qmmessageholder.h>


/****************************************************************************
 *
 * ViewModelItem
 *
 */

inline qm::ViewModelItem::ViewModelItem(MessageHolder* pmh) :
	pmh_(pmh),
	pParentItem_(0),
	nFlags_(0),
	cr_(0xffffffff),
	nMessageFlags_(pmh->getFlags())
{
}

inline qm::ViewModelItem::ViewModelItem(unsigned int nMessageIdHash) :
	pmh_(0),
	pParentItem_(0),
	nFlags_(0),
	cr_(0xffffffff),
	nMessageFlags_(nMessageIdHash)
{
}

inline qm::ViewModelItem::~ViewModelItem()
{
}

inline qm::MessageHolder* qm::ViewModelItem::getMessageHolder() const
{
	assert(pmh_);
	return pmh_;
}

inline qm::ViewModelItem* qm::ViewModelItem::getParentItem() const
{
	assert(pmh_);
	return pParentItem_;
}

inline void qm::ViewModelItem::setParentItem(ViewModelItem* pParentItem)
{
	assert(pmh_);
	pParentItem_ = pParentItem;
}

inline bool qm::ViewModelItem::isFlag(Flag flag) const
{
	assert(pmh_);
	return (nFlags_ & flag) != 0;
}

inline unsigned int qm::ViewModelItem::getFlags() const
{
	assert(pmh_);
	return nFlags_;
}

inline void qm::ViewModelItem::setFlags(unsigned int nFlags,
										unsigned int nMask)
{
	assert(pmh_);
	nFlags_ &= ~nMask;
	nFlags_ |= nFlags & nMask;
}

inline COLORREF qm::ViewModelItem::getColor() const
{
	assert(pmh_);
	return cr_;
}

inline void qm::ViewModelItem::setColor(COLORREF cr)
{
	assert(pmh_);
	cr_ = cr;
}

inline unsigned int qm::ViewModelItem::getMessageFlags() const
{
	assert(pmh_);
	return nMessageFlags_;
}

inline void qm::ViewModelItem::setMessageFlags(unsigned int nFlags)
{
	assert(pmh_);
	nMessageFlags_ = nFlags;
}

inline unsigned int qm::ViewModelItem::getLevel() const
{
	assert(pmh_);
	
	unsigned int nLevel = 0;
	
	const ViewModelItem* pItem = getParentItem();
	while (pItem) {
		++nLevel;
		pItem = pItem->getParentItem();
	}
	
	return nLevel;
}

inline unsigned int qm::ViewModelItem::getMessageIdHash() const
{
	return pmh_ ? pmh_->getMessageIdHash() : nMessageFlags_;
}

inline const qm::MacroValue* qm::ViewModelItem::getCache(unsigned int n) const
{
	return *(reinterpret_cast<const MacroValue**>(reinterpret_cast<char*>(
		const_cast<ViewModelItem*>(this)) + sizeof(ViewModelItem)) + n);
}

inline void qm::ViewModelItem::setCache(unsigned int n,
										MacroValue* pValue) const
{
	*(reinterpret_cast<MacroValue**>(reinterpret_cast<char*>(
		const_cast<ViewModelItem*>(this)) + sizeof(ViewModelItem)) + n) = pValue;
}

inline qm::ViewModelItem* qm::ViewModelItem::newItem(MessageHolder* pmh,
													 unsigned int nCacheSize)
{
	size_t n = sizeof(ViewModelItem) + nCacheSize*sizeof(MacroValue*);
#ifdef NDEBUG
	void* p = std::__sgi_alloc::allocate(n);
#else
	void* p = malloc(n);
#endif
	memset(static_cast<char*>(p) + sizeof(ViewModelItem), 0, nCacheSize*sizeof(MacroValue*));
	
	return new (p) ViewModelItem(pmh);
}

inline void qm::ViewModelItem::deleteItem(ViewModelItem* pItem,
										  unsigned int nCacheSize)
{
	MacroValue** p = reinterpret_cast<MacroValue**>(reinterpret_cast<char*>(pItem) + sizeof(ViewModelItem));
	for (unsigned int n = 0; n < nCacheSize; ++n, ++p)
		MacroValuePtr pValue(*p);
	
	pItem->~ViewModelItem();
	
	size_t n = sizeof(ViewModelItem) + nCacheSize*sizeof(MacroValue*);
#ifdef NDEBUG
	std::__sgi_alloc::deallocate(pItem, n);
#else
	free(pItem);
#endif
}


/****************************************************************************
 *
 * ViewModelItemPtr
 *
 */

inline qm::ViewModelItemPtr::ViewModelItemPtr(MessageHolder* pmh,
											  unsigned int nCacheCount) :
	pItem_(ViewModelItem::newItem(pmh, nCacheCount)),
	nCacheCount_(nCacheCount)
{
}

inline qm::ViewModelItemPtr::~ViewModelItemPtr()
{
	if (pItem_)
		ViewModelItem::deleteItem(pItem_, nCacheCount_);
}

inline qm::ViewModelItem* qm::ViewModelItemPtr::operator->() const
{
	return pItem_;
}

inline qm::ViewModelItem* qm::ViewModelItemPtr::get() const
{
	return pItem_;
}

inline qm::ViewModelItem* qm::ViewModelItemPtr::release()
{
	ViewModelItem* pItem = pItem_;
	pItem_ = 0;
	return pItem;
}


/****************************************************************************
 *
 * ViewModelFolderComp
 *
 */

inline qm::ViewModelFolderComp::ViewModelFolderComp(Folder* pFolder) :
	pFolder_(pFolder)
{
}

inline qm::ViewModelFolderComp::~ViewModelFolderComp()
{
}

inline bool qm::ViewModelFolderComp::operator()(ViewModel* pViewModel) const
{
	return pViewModel->getFolder() == pFolder_;
}


/****************************************************************************
 *
 * ViewModelItemEqual
 *
 */

inline bool qm::ViewModelItemEqual::operator()(const ViewModelItem* pLhs,
											   const ViewModelItem* pRhs) const
{
	return pLhs->getMessageHolder() == pRhs->getMessageHolder();
}


/****************************************************************************
 *
 * ViewModelParentItemComp
 *
 */

inline qm::ViewModelParentItemComp::ViewModelParentItemComp(unsigned int nReferenceHash,
															const WCHAR* pwszReference) :
	nReferenceHash_(nReferenceHash),
	pwszReference_(pwszReference)
{
}

inline bool qm::ViewModelParentItemComp::operator()(const ViewModelItem* pItem) const
{
	MessageHolder* pmh = pItem->getMessageHolder();
	if (pmh->getMessageIdHash() != nReferenceHash_)
		return false;
	
	qs::wstring_ptr wstrMessageId(pmh->getMessageId());
	return wcscmp(wstrMessageId.get(), pwszReference_) == 0;
}

#endif // __VIEWMODEL_INL__
