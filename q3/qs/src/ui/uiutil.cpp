/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsconv.h>
#include <qsdevicecontext.h>
#include <qsdialog.h>
#include <qsinit.h>
#include <qsosutil.h>
#include <qsprofile.h>
#include <qstheme.h>
#include <qsuiutil.h>

#ifndef _WIN32_WCE
#	include <shlobj.h>
#endif

using namespace qs;


/****************************************************************************
 *
 * UIUtil
 *
 */

HFONT qs::UIUtil::createFontFromProfile(Profile* pProfile,
										const WCHAR* pwszSection,
										bool bDefaultFixedWidth)
{
	assert(pProfile);
	
	const WCHAR* pwszDefaultFace = 0;
	if (bDefaultFixedWidth)
		pwszDefaultFace = Init::getInit().getDefaultFixedWidthFont();
	else
		pwszDefaultFace = Init::getInit().getDefaultProportionalFont();
	
	wstring_ptr wstrFontFace(pProfile->getString(
		pwszSection, L"FontFace", pwszDefaultFace));
	wstring_ptr wstrFontSize(pProfile->getString(
		pwszSection, L"FontSize", L"9"));
	WCHAR* pEnd = 0;
	double dFontSize = wcstod(wstrFontSize.get(), &pEnd);
	if (*pEnd)
		dFontSize = 9;
	int nFontStyle = pProfile->getInt(pwszSection, L"FontStyle", 0);
	int nFontCharset = pProfile->getInt(pwszSection, L"FontCharset", 0);
	
	ClientDeviceContext dc(0);
	LOGFONT lf;
	FontHelper::createLogFont(dc, wstrFontFace.get(),
		dFontSize, nFontStyle, nFontCharset, &lf);
	return ::CreateFontIndirect(&lf);
}

#ifndef _WIN32_WCE
static int CALLBACK browseCallbackProc(HWND hwnd,
									   UINT uMsg,
									   LPARAM lParam,
									   LPARAM lpData)
{
	switch (uMsg) {
	case BFFM_INITIALIZED:
		::SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
		break;
	}
	return 0;
}
#endif

wstring_ptr qs::UIUtil::browseFolder(HWND hwnd,
									 const WCHAR* pwszTitle,
									 const WCHAR* pwszInitialPath)
{
	wstring_ptr wstrPath;
	
#ifdef _WIN32_WCE
	BrowseFolderDialog dialog(pwszTitle, pwszInitialPath);
	int nRet = dialog.doModal(hwnd);
	if (nRet == IDOK)
		wstrPath = allocWString(dialog.getPath());
#else
	W2T(pwszTitle, ptszTitle);
	W2T(pwszInitialPath, ptszInitialPath);
	TCHAR tszDisplayName[MAX_PATH];
	BROWSEINFO info = {
		hwnd,
		0,
		tszDisplayName,
		ptszTitle,
		BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE,
		&browseCallbackProc,
		reinterpret_cast<LPARAM>(ptszInitialPath),
		0
	};
	LPITEMIDLIST pList = ::SHBrowseForFolder(&info);
	if (pList) {
		TCHAR tszPath[MAX_PATH];
		if (::SHGetPathFromIDList(pList, tszPath))
			wstrPath = tcs2wcs(tszPath);
		
		ComPtr<IMalloc> pMalloc;
		::SHGetMalloc(&pMalloc);
		pMalloc->Free(pList);
	}
#endif
	
	return wstrPath;
}

#ifndef _WIN32_WCE
bool qs::UIUtil::drawThemeBorder(Theme* pTheme,
								 HWND hwnd,
								 int nPartId,
								 int nStateId,
								 COLORREF crBackground)
{
	RECT rect;
	::GetWindowRect(hwnd, &rect);
	rect.right -= rect.left;
	rect.left = 0;
	rect.bottom -= rect.top;
	rect.top = 0;
	
	WindowDeviceContext dc(hwnd);
	int nEdgeWidth = ::GetSystemMetrics(SM_CXEDGE);
	int nEdgeHeight = ::GetSystemMetrics(SM_CYEDGE);
	dc.excludeClipRect(nEdgeWidth, nEdgeHeight,
		rect.right - nEdgeWidth, rect.bottom - nEdgeHeight);
	dc.fillSolidRect(rect, crBackground);
	
	int nBorderWidth = pTheme->getSysSize(SM_CXBORDER);
	int nBorderHeight = pTheme->getSysSize(SM_CYBORDER);
	dc.excludeClipRect(nBorderWidth, nBorderHeight,
		rect.right - nBorderWidth, rect.bottom - nBorderHeight);
	return pTheme->drawBackground(dc.getHandle(), nPartId, nStateId, rect, 0);
}
#endif
