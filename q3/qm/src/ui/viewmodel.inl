/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
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

inline qm::ViewModelItem::ViewModelItem(MessageHolder* pmh, qs::QSTATUS* pstatus) :
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

inline void* qm::ViewModelItem::operator new(size_t n)
{
	assert(n == sizeof(ViewModelItem));
	return std::__sgi_alloc::allocate(n);
}

inline void qm::ViewModelItem::operator delete(void* p)
{
	std::__sgi_alloc::deallocate(p, sizeof(ViewModelItem));
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

inline unsigned int qm::ViewModelItem::getFlags() const
{
	assert(pmh_);
	return nFlags_;
}

inline void qm::ViewModelItem::setFlags(unsigned int nFlags, unsigned int nMask)
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

inline bool qm::ViewModelItemEqual::operator()(
	const ViewModelItem* pLhs, const ViewModelItem* pRhs) const
{
	return pLhs->getMessageHolder() == pRhs->getMessageHolder();
}


/****************************************************************************
 *
 * ViewModelParentItemComp
 *
 */

inline qm::ViewModelParentItemComp::ViewModelParentItemComp(
	unsigned int nReferenceHash, const WCHAR* pwszReference) :
	nReferenceHash_(nReferenceHash),
	pwszReference_(pwszReference)
{
}

inline bool qm::ViewModelParentItemComp::operator()(const ViewModelItem* pItem) const
{
	DECLARE_QSTATUS();
	
	MessageHolder* pmh = pItem->getMessageHolder();
	if (pmh->getMessageIdHash() != nReferenceHash_)
		return false;
	
	qs::string_ptr<qs::WSTRING> wstrMessageId;
	status = pmh->getMessageId(&wstrMessageId);
	CHECK_QSTATUS_VALUE(false);
	
	return wcscmp(wstrMessageId.get(), pwszReference_) == 0;
}

#endif // __VIEWMODEL_INL__
