/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsconv.h>
#include <qsdevicecontext.h>
#include <qsdialog.h>
#include <qsinit.h>
#include <qsosutil.h>
#include <qsprofile.h>
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

QSTATUS qs::UIUtil::createFontFromProfile(Profile* pProfile,
	const WCHAR* pwszSection, bool bDefaultFixedWidth, HFONT* phfont)
{
	assert(pProfile);
	assert(phfont);
	
	DECLARE_QSTATUS();
	
	const WCHAR* pwszDefaultFace = 0;
	if (bDefaultFixedWidth)
		pwszDefaultFace = Init::getInit().getDefaultFixedWidthFont();
	else
		pwszDefaultFace = Init::getInit().getDefaultProportionalFont();
	CHECK_QSTATUS();
	string_ptr<WSTRING> wstrFontFace;
	status = pProfile->getString(pwszSection,
		L"FontFace", pwszDefaultFace, &wstrFontFace);
	CHECK_QSTATUS();
	int nFontSize = 0;
	status = pProfile->getInt(pwszSection, L"FontSize", 9, &nFontSize);
	CHECK_QSTATUS();
	int nFontStyle = 0;
	status = pProfile->getInt(pwszSection, L"FontStyle", 0, &nFontStyle);
	CHECK_QSTATUS();
	int nFontCharset = 0;
	status = pProfile->getInt(pwszSection, L"FontCharset", 0, &nFontCharset);
	CHECK_QSTATUS();
	
	ClientDeviceContext dc(0, &status);
	CHECK_QSTATUS();
	LOGFONT lf;
	status = FontHelper::createLogFont(dc, wstrFontFace.get(),
		nFontSize, nFontStyle, nFontCharset, &lf);
	CHECK_QSTATUS();
	*phfont = ::CreateFontIndirect(&lf);
	
	return *phfont ? QSTATUS_SUCCESS : QSTATUS_FAIL;
}

#ifndef _WIN32_WCE
static int CALLBACK browseCallbackProc(HWND hwnd,
	UINT uMsg, LPARAM lParam, LPARAM lpData)
{
	switch (uMsg) {
	case BFFM_INITIALIZED:
		::SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
		break;
	}
	return 0;
}
#endif

QSTATUS qs::UIUtil::browseFolder(HWND hwnd, const WCHAR* pwszTitle,
	const WCHAR* pwszInitialPath, WSTRING* pwstrPath)
{
	assert(pwstrPath);
	
	DECLARE_QSTATUS();
	
	*pwstrPath = 0;
	
	string_ptr<WSTRING> wstrPath;
	
#ifdef _WIN32_WCE
	BrowseFolderDialog dialog(pwszTitle, pwszInitialPath, &status);
	CHECK_QSTATUS();
	int nRet = 0;
	status = dialog.doModal(hwnd, 0, &nRet);
	CHECK_QSTATUS();
	if (nRet == IDOK) {
		wstrPath.reset(allocWString(dialog.getPath()));
		if (!wstrPath.get())
			return QSTATUS_OUTOFMEMORY;
	}
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
		if (::SHGetPathFromIDList(pList, tszPath)) {
			wstrPath.reset(tcs2wcs(tszPath));
			if (!wstrPath.get())
				return QSTATUS_OUTOFMEMORY;
		}
		
		ComPtr<IMalloc> pMalloc;
		::SHGetMalloc(&pMalloc);
		pMalloc->Free(pList);
	}
#endif
	
	*pwstrPath = wstrPath.release();
	
	return QSTATUS_SUCCESS;
}
