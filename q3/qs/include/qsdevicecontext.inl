/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QSDEVICECONTEXT_INL__
#define __QSDEVICECONTEXT_INL__

#include <qsassert.h>


/****************************************************************************
 *
 * DeviceContext
 *
 */

inline qs::DeviceContext::DeviceContext(HDC hdc) :
	hdc_(hdc)
{
}

inline qs::DeviceContext::DeviceContext(const DeviceContext& dc) :
	hdc_(dc.getHandle())
{
}

inline qs::DeviceContext::~DeviceContext()
{
}

inline qs::DeviceContext::operator HDC() const
{
	return hdc_;
}

inline qs::DeviceContext& qs::DeviceContext::operator=(HDC hdc)
{
	hdc_ = hdc;
	return *this;
}

inline qs::DeviceContext& qs::DeviceContext::operator=(const DeviceContext& dc)
{
	hdc_ = dc.getHandle();
	return *this;
}

inline bool qs::DeviceContext::operator!() const
{
	return !hdc_;
}

inline HDC qs::DeviceContext::getHandle() const
{
	return hdc_;
}

inline COLORREF qs::DeviceContext::getTextColor()
{
	assert(hdc_);
	return ::GetTextColor(hdc_);
}

inline COLORREF qs::DeviceContext::setTextColor(COLORREF cr)
{
	assert(hdc_);
	return ::SetTextColor(hdc_, cr);
}

inline COLORREF qs::DeviceContext::getBkColor()
{
	assert(hdc_);
	return ::GetBkColor(hdc_);
}

inline COLORREF qs::DeviceContext::setBkColor(COLORREF cr)
{
	assert(hdc_);
	return ::SetBkColor(hdc_, cr);
}

inline int qs::DeviceContext::setBkMode(int nBkMode)
{
	assert(hdc_);
	return ::SetBkMode(hdc_, nBkMode);
}

inline int qs::DeviceContext::getClipBox(RECT* pRect)
{
	assert(hdc_);
	return ::GetClipBox(hdc_, pRect);
}

inline int qs::DeviceContext::excludeClipRect(int nLeft,
											  int nTop,
											  int nRight,
											  int nBottom)
{
	assert(hdc_);
	return ::ExcludeClipRect(hdc_, nLeft, nTop, nRight, nBottom);
}

inline HFONT qs::DeviceContext::selectObject(HFONT hfont)
{
	assert(hdc_);
	return static_cast<HFONT>(::SelectObject(hdc_, hfont));
}

inline HPEN qs::DeviceContext::selectObject(HPEN hpen)
{
	assert(hdc_);
	return static_cast<HPEN>(::SelectObject(hdc_, hpen));
}

inline HBRUSH qs::DeviceContext::selectObject(HBRUSH hbrush)
{
	assert(hdc_);
	return static_cast<HBRUSH>(::SelectObject(hdc_, hbrush));
}

inline HBITMAP qs::DeviceContext::selectObject(HBITMAP hbm)
{
	assert(hdc_);
	return static_cast<HBITMAP>(::SelectObject(hdc_, hbm));
}

inline bool qs::DeviceContext::polyline(const POINT* ppt,
										int nPoints)
{
	assert(hdc_);
	return ::Polyline(hdc_, ppt, nPoints) != 0;
}

inline bool qs::DeviceContext::bitBlt(int x,
									  int y,
									  int nWidth,
									  int nHeight,
									  HDC hdc,
									  int nSrcX,
									  int nSrcY,
									  DWORD dwRop)
{
	assert(hdc_);
	return ::BitBlt(hdc_, x, y, nWidth, nHeight,
		hdc, nSrcX, nSrcY, dwRop) != 0;
}

inline bool qs::DeviceContext::patBlt(int x,
									  int y,
									  int nWidth,
									  int nHeight,
									  DWORD dwRop)
{
	assert(hdc_);
	return ::PatBlt(hdc_, x, y, nWidth, nHeight, dwRop) != 0;
}

inline COLORREF qs::DeviceContext::getPixel(int x,
											int y)
{
	assert(hdc_);
	return ::GetPixel(hdc_, x, y);
}

inline COLORREF qs::DeviceContext::setPixel(int x,
											int y,
											COLORREF cr)
{
	assert(hdc_);
	return ::SetPixel(hdc_, x, y, cr);
}

inline bool qs::DeviceContext::extTextOut(int x,
										  int y,
										  UINT nOptions,
										  const RECT& rect,
										  const WCHAR* pwszString,
										  UINT nCount,
										  int* pnDx)
{
	assert(hdc_);
	return ::ExtTextOutW(hdc_, x, y, nOptions, &rect,
		pwszString, nCount, pnDx) != 0;
}

inline bool qs::DeviceContext::getTextMetrics(TEXTMETRIC* ptm) const
{
	assert(hdc_);
	return ::GetTextMetrics(hdc_, ptm) != 0;
}

inline bool qs::DeviceContext::getTextExtent(const WCHAR* pwszString,
											 int nCount,
											 SIZE* pSize) const
{
	assert(hdc_);
	return ::GetTextExtentPoint32W(hdc_, pwszString, nCount, pSize) != 0;
}

#ifdef UNICODE
inline bool qs::DeviceContext::getTextExtentEx(const WCHAR* pwszString,
											   int nCount,
											   int nMaxExtent,
											   int* pnFit,
											   int* pnDx,
											   SIZE* pSize) const
{
	assert(hdc_);
	return ::GetTextExtentExPointW(hdc_, pwszString,
		nCount, nMaxExtent, pnFit, pnDx, pSize) != 0;
}
#endif

inline bool qs::DeviceContext::fillSolidRect(const RECT& rect,
											 COLORREF cr)
{
	setBkColor(cr);
	return extTextOut(rect.left, rect.top,
		ETO_CLIPPED | ETO_OPAQUE, rect, L"", 0, 0);
}

inline bool qs::DeviceContext::drawFocusRect(const RECT& rect)
{
	assert(hdc_);
	return ::DrawFocusRect(hdc_, &rect) != 0;
}

inline bool qs::DeviceContext::drawFrameControl(RECT* pRect,
												UINT nType,
												UINT nState)
{
	assert(hdc_);
	return ::DrawFrameControl(hdc_, pRect, nType, nState) != 0;
}

inline int qs::DeviceContext::getDeviceCaps(int nIndex) const
{
	assert(hdc_);
	return ::GetDeviceCaps(hdc_, nIndex);
}

inline bool qs::DeviceContext::drawIcon(int x,
										int y,
										HICON hIcon)
{
	assert(hdc_);
	return ::DrawIcon(hdc_, x, y, hIcon) != 0;
}

inline void qs::DeviceContext::setHandle(HDC hdc)
{
	assert(!hdc_);
	hdc_ = hdc;
}


/****************************************************************************
 *
 * ObjectSelector
 *
 */

template<class Object>
qs::ObjectSelector<Object>::ObjectSelector(DeviceContext& dc,
										   Object o) :
	dc_(dc),
	o_(0)
{
	if (o)
		o_ = dc_.selectObject(o);
}

template<class Object>
qs::ObjectSelector<Object>::~ObjectSelector()
{
	if (o_)
		dc_.selectObject(o_);
}


/****************************************************************************
 *
 * GdiObject
 *
 */

template<class Object>
qs::GdiObject<Object>::GdiObject(Object o) :
	o_(o)
{
}

template<class Object>
qs::GdiObject<Object>::~GdiObject()
{
	if (o_)
		::DeleteObject(o_);
}

template<class Object>
Object qs::GdiObject<Object>::get() const
{
	return o_;
}

template<class Object>
Object qs::GdiObject<Object>::release()
{
	Object o = o_;
	o_ = 0;
	return o;
}


#endif // __QSDEVICECONTEXT_INL__
