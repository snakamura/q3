/*
 * $Id: qsosutil.inl,v 1.1.1.1 2003/04/29 08:07:34 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QSOSUTIL_INL__
#define __QSOSUTIL_INL__


/****************************************************************************
 *
 * Point
 *
 */

inline qs::Point::Point()
{
	x = 0;
	y = 0;
}

inline qs::Point::Point(LONG x, LONG y)
{
	this->x = x;
	this->y = y;
}


/****************************************************************************
 *
 * Size
 *
 */

inline qs::Size::Size()
{
	cx = 0;
	cy = 0;
}

inline qs::Size::Size(LONG cx, LONG cy)
{
	this->cx = cx;
	this->cy = cy;
}


/****************************************************************************
 *
 * Rect
 *
 */

inline qs::Rect::Rect()
{
	left = 0;
	top = 0;
	right = 0;
	bottom = 0;
}

inline qs::Rect::Rect(LONG left, LONG top, LONG right, LONG bottom)
{
	this->left = left;
	this->top = top;
	this->right = right;
	this->bottom = bottom;
}


/****************************************************************************
 *
 * AutoHandle
 *
 */

inline qs::AutoHandle::AutoHandle(HANDLE handle) :
	handle_(handle == INVALID_HANDLE_VALUE ? 0 : handle)
{
}

inline qs::AutoHandle::~AutoHandle()
{
	close();
}

inline HANDLE qs::AutoHandle::get() const
{
	return handle_;
}

inline void qs::AutoHandle::close()
{
	if (handle_) {
		::CloseHandle(handle_);
		handle_ = 0;
	}
}


/****************************************************************************
 *
 * AutoFindHandle
 *
 */

inline qs::AutoFindHandle::AutoFindHandle(HANDLE handle) :
	handle_(handle == INVALID_HANDLE_VALUE ? 0 : handle)
{
}

inline qs::AutoFindHandle::~AutoFindHandle()
{
	close();
}

inline HANDLE qs::AutoFindHandle::get() const
{
	return handle_;
}

inline void qs::AutoFindHandle::close()
{
	if (handle_) {
		::FindClose(handle_);
		handle_ = 0;
	}
}


/****************************************************************************
 *
 * AutoMenuHandle
 *
 */

inline qs::AutoMenuHandle::AutoMenuHandle() :
	hmenu_(0)
{
}

inline qs::AutoMenuHandle::AutoMenuHandle(HMENU hmenu) :
	hmenu_(hmenu)
{
}

inline qs::AutoMenuHandle::~AutoMenuHandle()
{
	if (hmenu_)
		::DestroyMenu(hmenu_);
}

inline HMENU qs::AutoMenuHandle::get() const
{
	return hmenu_;
}

inline HMENU qs::AutoMenuHandle::release()
{
	HMENU hmenu = hmenu_;
	hmenu_ = 0;
	return hmenu;
}

inline HMENU* qs::AutoMenuHandle::operator&()
{
	assert(!hmenu_);
	return &hmenu_;
}


/****************************************************************************
 *
 * ComPtr
 *
 */

template<class Interface>
qs::ComPtr<Interface>::ComPtr() :
	p_(0)
{
}

template<class Interface>
qs::ComPtr<Interface>::ComPtr(Interface* p) :
	p_(p)
{
}

template<class Interface>
qs::ComPtr<Interface>::~ComPtr()
{
	if (p_)
		p_->Release();
}

template<class Interface>
Interface** qs::ComPtr<Interface>::operator&()
{
	assert(!p_);
	return &p_;
}

template<class Interface>
Interface* qs::ComPtr<Interface>::operator->()
{
	return p_;
}

template<class Interface>
Interface* qs::ComPtr<Interface>::get() const
{
	return p_;
}

template<class Interface>
Interface* qs::ComPtr<Interface>::release()
{
	Interface* p = p_;
	p_ = 0;
	return p;
}


/****************************************************************************
 *
 * BSTRPtr
 *
 */

inline qs::BSTRPtr::BSTRPtr() :
	bstr_(0)
{
}

inline qs::BSTRPtr::BSTRPtr(BSTR bstr) :
	bstr_(bstr)
{
}

inline qs::BSTRPtr::~BSTRPtr()
{
	if (bstr_)
		::SysFreeString(bstr_);
}

inline BSTR* qs::BSTRPtr::operator&()
{
	assert(!bstr_);
	return &bstr_;
}

inline BSTR qs::BSTRPtr::get() const
{
	return bstr_;
}

inline BSTR qs::BSTRPtr::release()
{
	BSTR bstr = bstr_;
	bstr_ = 0;
	return bstr;
}


/****************************************************************************
 *
 * Variant
 *
 */

inline qs::Variant::Variant()
{
	::VariantInit(this);
}

inline qs::Variant::Variant(BSTR bstr)
{
	::VariantInit(this);
	vt = VT_BSTR;
	bstrVal = bstr;
}

inline qs::Variant::~Variant()
{
	::VariantClear(this);
}


/****************************************************************************
 *
 * SafeArrayPtr
 *
 */

inline qs::SafeArrayPtr::SafeArrayPtr(SAFEARRAY* pArray) :
	pArray_(pArray)
{
}

inline qs::SafeArrayPtr::~SafeArrayPtr()
{
	if (pArray_)
		::SafeArrayDestroy(pArray_);
}

inline SAFEARRAY* qs::SafeArrayPtr::get() const
{
	return pArray_;
}

inline SAFEARRAY* qs::SafeArrayPtr::release()
{
	SAFEARRAY* pArray = pArray_;
	pArray_ = 0;
	return pArray;
}


/****************************************************************************
 *
 * StgMedium
 *
 */

inline qs::StgMedium::StgMedium()
{
	tymed = TYMED_NULL;
}

inline qs::StgMedium::~StgMedium()
{
	if (tymed != TYMED_NULL)
		::ReleaseStgMedium(this);
}

#endif // __QSOSUTIL_INL__
