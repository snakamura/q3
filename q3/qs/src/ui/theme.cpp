/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef _WIN32_WCE

#include <qstheme.h>

#include <tchar.h>
#include <uxtheme.h>

using namespace qs;


/****************************************************************************
 *
 * ThemeImpl
 *
 */

struct qs::ThemeImpl
{
	HINSTANCE hInstUxTheme_;
	HTHEME hTheme_;
};


/****************************************************************************
 *
 * Theme
 *
 */

qs::Theme::Theme(HWND hwnd,
				 const WCHAR* pwszClasses) :
	pImpl_(0)
{
	pImpl_ = new ThemeImpl();
	pImpl_->hInstUxTheme_ = 0;
	pImpl_->hTheme_ = 0;
	
	pImpl_->hInstUxTheme_ = ::LoadLibrary(_T("uxtheme.dll"));
	if (pImpl_->hInstUxTheme_) {
		typedef BOOL (WINAPI *PFN_ISTHEMEACTIVE)();
		PFN_ISTHEMEACTIVE pfnIsThemeActive = reinterpret_cast<PFN_ISTHEMEACTIVE>(
			::GetProcAddress(pImpl_->hInstUxTheme_, "IsThemeActive"));
		if ((*pfnIsThemeActive)()) {
			typedef HTHEME (WINAPI *PFN_OPENTHEMEDATA)(HWND, LPCWSTR);
			PFN_OPENTHEMEDATA pfnOpenThemeData = reinterpret_cast<PFN_OPENTHEMEDATA>(
				::GetProcAddress(pImpl_->hInstUxTheme_, "OpenThemeData"));
			pImpl_->hTheme_ = (*pfnOpenThemeData)(hwnd, pwszClasses);
		}
	}
}

qs::Theme::~Theme()
{
	if (pImpl_->hInstUxTheme_) {
		if (pImpl_->hTheme_) {
			typedef HRESULT (WINAPI *PFN_CLOSETHEMEDATA)(HTHEME);
			PFN_CLOSETHEMEDATA pfnCloseThemeData = reinterpret_cast<PFN_CLOSETHEMEDATA>(
				::GetProcAddress(pImpl_->hInstUxTheme_, "CloseThemeData"));
			(*pfnCloseThemeData)(pImpl_->hTheme_);
		}
		::FreeLibrary(pImpl_->hInstUxTheme_);
	}
	delete pImpl_;
}

bool qs::Theme::isActive() const
{
	return pImpl_->hTheme_ != 0;
}

bool qs::Theme::drawBackground(HDC hdc,
							   int nPartId,
							   int nStateId,
							   const RECT& rect,
							   const RECT* pRectClip)
{
	if (!pImpl_->hTheme_)
		return false;
	
	typedef HRESULT (WINAPI *PFN_DRAWTHEMEBACKGROUND)(HTHEME, HDC, int, int, const RECT*, const RECT*);
	PFN_DRAWTHEMEBACKGROUND pfnDrawThemeBackground = reinterpret_cast<PFN_DRAWTHEMEBACKGROUND>(
		::GetProcAddress(pImpl_->hInstUxTheme_, "DrawThemeBackground"));
	return (*pfnDrawThemeBackground)(pImpl_->hTheme_, hdc, nPartId, nStateId, &rect, pRectClip) == S_OK;
}

bool qs::Theme::drawEdge(HDC hdc,
						 int nPartId,
						 int nStateId,
						 const RECT& rect,
						 UINT nEdge,
						 UINT nFlags,
						 RECT* pRect)
{
	if (!pImpl_->hTheme_)
		return false;
	
	typedef HRESULT (WINAPI *PFN_DRAWTHEMEEDGE)(HTHEME, HDC, int, int, const RECT*, UINT, UINT, RECT*);
	PFN_DRAWTHEMEEDGE pfnDrawThemeEdge = reinterpret_cast<PFN_DRAWTHEMEEDGE>(
		::GetProcAddress(pImpl_->hInstUxTheme_, "DrawThemeEdge"));
	return (*pfnDrawThemeEdge)(pImpl_->hTheme_, hdc, nPartId, nStateId, &rect, nEdge, nFlags, pRect) == S_OK;
}

int qs::Theme::getSysSize(int nId)
{
	if (!pImpl_->hTheme_)
		return ::GetSystemMetrics(nId);
	
	typedef int (WINAPI *PFN_GETTHEMESYSSIZE)(HTHEME, int);
	PFN_GETTHEMESYSSIZE pfnGetThemeSysSize = reinterpret_cast<PFN_GETTHEMESYSSIZE>(
		::GetProcAddress(pImpl_->hInstUxTheme_, "GetThemeSysSize"));
	return (*pfnGetThemeSysSize)(pImpl_->hTheme_, nId) == S_OK;
}

#endif // _WIN32_WCE
