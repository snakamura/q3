/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __LISTWINDOW_INL__
#define __LISTWINDOW_INL__

/****************************************************************************
 *
 * PaintInfo
 *
 */

inline qm::PaintInfo::PaintInfo(qs::DeviceContext* pdc,
								ViewModel* pViewModel,
								unsigned int nIndex,
								const RECT& rect) :
	pdc_(pdc),
	pViewModel_(pViewModel),
	nIndex_(nIndex),
	pItem_(pViewModel->getItem(nIndex)),
	rect_(rect)
{
}

inline qm::PaintInfo::~PaintInfo()
{
}

inline qs::DeviceContext* qm::PaintInfo::getDeviceContext() const
{
	return pdc_;
}

inline qm::ViewModel* qm::PaintInfo::getViewModel() const
{
	return pViewModel_;
}

inline unsigned int qm::PaintInfo::getIndex() const
{
	return nIndex_;
}

inline const qm::ViewModelItem* qm::PaintInfo::getItem() const
{
	return pItem_;
}

inline const RECT& qm::PaintInfo::getRect() const
{
	return rect_;
}

#endif // __LISTWINDOW_INL__
