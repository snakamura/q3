/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
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

#include <commdlg.h>
#ifndef _WIN32_WCE
#	include <shlobj.h>
#endif
#ifdef _WIN32_WCE_PSPC
#	include <aygshell.h>
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
	LOGFONT lf;
	getLogFontFromProfile(pProfile, pwszSection, bDefaultFixedWidth, &lf);
	return ::CreateFontIndirect(&lf);
}

void qs::UIUtil::getLogFontFromProfile(Profile* pProfile,
									   const WCHAR* pwszSection,
									   bool bDefaultFixedWidth,
									   LOGFONT* pLogFont)
{
	assert(pProfile);
	assert(pwszSection);
	assert(pLogFont);
	
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
	FontHelper::createLogFont(dc, wstrFontFace.get(),
		dFontSize, nFontStyle, nFontCharset, pLogFont);
}

void qs::UIUtil::setLogFontToProfile(Profile* pProfile,
									 const WCHAR* pwszSection,
									 const LOGFONT& lf)
{
	assert(pProfile);
	assert(pwszSection);
	
	T2W(lf.lfFaceName, pwszFaceName);
	pProfile->setString(pwszSection, L"FontFace", pwszFaceName);
	
	ClientDeviceContext dc(0);
	double dPointSize = -lf.lfHeight*72.0/dc.getDeviceCaps(LOGPIXELSY);
	WCHAR wszSize[64];
	swprintf(wszSize, L"%.1lf", dPointSize);
	size_t nLen = wcslen(wszSize);
	if (nLen > 2 && wcscmp(wszSize + nLen - 2, L".0") == 0)
		wszSize[nLen - 2] = L'\0';
	pProfile->setString(pwszSection, L"FontSize", wszSize);
	
	int nFontStyle = 0;
	if (lf.lfWeight >= FW_BOLD)
		nFontStyle |= FontHelper::STYLE_BOLD;
	if (lf.lfItalic)
		nFontStyle |= FontHelper::STYLE_ITALIC;
	if (lf.lfUnderline)
		nFontStyle |= FontHelper::STYLE_UNDERLINE;
	if (lf.lfStrikeOut)
		nFontStyle |= FontHelper::STYLE_STRIKEOUT;
	pProfile->setInt(pwszSection, L"FontStyle", nFontStyle);
	
	if (lf.lfCharSet != 0)
		pProfile->setInt(pwszSection, L"FontCharset", lf.lfCharSet);
}

bool qs::UIUtil::isImeEnabled(HWND hwnd)
{
	bool bEnabled = false;
	
	HIMC hImc = ::ImmGetContext(hwnd);
	if (hImc) {
		bEnabled = ::ImmGetOpenStatus(hImc) != 0;
		::ImmReleaseContext(hwnd, hImc);
	}
	
	return bEnabled;
}

void qs::UIUtil::setImeEnabled(HWND hwnd,
							   bool bEnabled)
{
	HIMC hImc = ::ImmGetContext(hwnd);
	if (hImc) {
		::ImmSetOpenStatus(hImc, bEnabled);
		::ImmReleaseContext(hwnd, hImc);
	}
}

bool qs::UIUtil::browseFont(HWND hwnd,
							LOGFONT* pLogFont)
{
#if 0//!defined _WIN32_WCE || (_WIN32_WCE >= 400 && !defined _WIN32_WCE_PSPC)
	CHOOSEFONT cf = {
		sizeof(cf),
		hwnd,
		0,
		pLogFont,
		0,
		CF_INITTOLOGFONTSTRUCT | CF_SCREENFONTS,
	};
	return ::ChooseFont(&cf) != 0;
#else
	FontDialog dialog(*pLogFont);
	if (dialog.doModal(hwnd) != IDOK)
		return false;
	
	*pLogFont = dialog.getLogFont();
	
	return true;
#endif
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

int qs::UIUtil::getLogPixel()
{
	ClientDeviceContext dc(0);
	return dc.getDeviceCaps(LOGPIXELSY);
}

#if defined _WIN32_WCE && _WIN32_WCE >= 300 && defined _WIN32_WCE_PSPC
void qs::UIUtil::getWorkArea(RECT* pRect)
{
	assert(pRect);
	
	const int nDefaultMenuHeight = 26;
	ClientDeviceContext dc(0);
	int nMenuHeight = static_cast<int>(nDefaultMenuHeight*(getLogPixel()/96.0));
	
	SIPINFO si = { sizeof(si) };
	::SHSipInfo(SPI_GETSIPINFO, 0, &si, 0);
	
	*pRect = si.rcVisibleDesktop;
	if ((si.fdwFlags & SIPF_ON) == 0)
		pRect->bottom -= nMenuHeight;
}
#endif
