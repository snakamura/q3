/*
 * $Id: clipboard.cpp,v 1.1.1.1 2003/04/29 08:07:37 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsosutil.h>
#include <qserror.h>
#include <qswindow.h>
#include <qsconv.h>

#include <algorithm>

#include <windows.h>
#include <tchar.h>

using namespace qs;


/****************************************************************************
 *
 * Clipboard
 *
 */

qs::Clipboard::Clipboard(HWND hwnd, QSTATUS* pstatus) :
	bOpen_(false)
{
	assert(pstatus);
	
	bOpen_ = ::OpenClipboard(hwnd) != 0;
	*pstatus = bOpen_ ? QSTATUS_SUCCESS : QSTATUS_FAIL;
}

qs::Clipboard::~Clipboard()
{
	close();
}

QSTATUS qs::Clipboard::close()
{
	if (bOpen_)
		return ::CloseClipboard() ? QSTATUS_SUCCESS : QSTATUS_FAIL;
	else
		return QSTATUS_SUCCESS;
}

QSTATUS qs::Clipboard::getData(UINT nFormat, HANDLE* phMem) const
{
	assert(phMem);
	*phMem = ::GetClipboardData(nFormat);
	return *phMem ? QSTATUS_SUCCESS : QSTATUS_FAIL;
}

QSTATUS qs::Clipboard::setData(UINT nFormat, HANDLE hMem)
{
	return setData(nFormat, hMem, 0);
}

QSTATUS qs::Clipboard::setData(UINT nFormat, HANDLE hMem, HANDLE* phMem)
{
	HANDLE hMemPrevious = ::SetClipboardData(nFormat, hMem);
	if (phMem)
		*phMem = hMemPrevious;
	return hMemPrevious ? QSTATUS_SUCCESS : QSTATUS_FAIL;
}

QSTATUS qs::Clipboard::empty() const
{
	return ::EmptyClipboard() ? QSTATUS_SUCCESS : QSTATUS_FAIL;
}

QSTATUS qs::Clipboard::isFormatAvailable(UINT nFormat, bool* pbAvailable)
{
	assert(pbAvailable);
	*pbAvailable = ::IsClipboardFormatAvailable(nFormat) != 0;
	return QSTATUS_SUCCESS;
}

QSTATUS qs::Clipboard::setText(const WCHAR* pwszText)
{
	return Clipboard::setText(0, pwszText);
}

QSTATUS qs::Clipboard::setText(HWND hwnd, const WCHAR* pwszText)
{
	DECLARE_QSTATUS();
	
	if (!hwnd) {
		Window* pMainWindow = getMainWindow();
		if (!pMainWindow)
			return QSTATUS_FAIL;
		hwnd = pMainWindow->getHandle();
	}
	
	Clipboard clipboard(hwnd, &status);
	CHECK_QSTATUS();
	
	status = clipboard.empty();
	CHECK_QSTATUS();
	
	int nTextLen = wcslen(pwszText);
	size_t nCount = std::count(pwszText, pwszText + nTextLen, L'\n');
	
#ifdef UNICODE
#ifdef _WIN32_WCE
	HANDLE hMem = ::LocalAlloc(LMEM_FIXED, (nTextLen + nCount + 1)*sizeof(WCHAR));
	if (!hMem)
		return QSTATUS_OUTOFMEMORY;
	WCHAR* pwszMem = reinterpret_cast<WCHAR*>(hMem);
#else // _WIN32_WCE
	HANDLE hMem = ::GlobalAlloc(LMEM_FIXED, (nTextLen + nCount + 1)*sizeof(WCHAR));
	if (!hMem)
		return QSTATUS_OUTOFMEMORY;
	WCHAR* pwszMem = static_cast<WCHAR*>(::GlobalLock(hMem));
#endif
	const WCHAR* pSrc = pwszText;
	WCHAR* pDst = pwszMem;
#else // UNICODE
	string_ptr<TSTRING> tstrText(wcs2tcs(pwszText));
	if (!tstrText.get())
		return QSTATUS_OUTOFMEMORY;
	HANDLE hMem = ::GlobalAlloc(LMEM_FIXED, (strlen(tstrText.get()) + nCount + 1)*sizeof(CHAR));
	if (!hMem)
		return QSTATUS_OUTOFMEMORY;
	TCHAR* ptszMem = static_cast<TCHAR*>(::GlobalLock(hMem));
	const TCHAR* pSrc = tstrText.get();
	TCHAR* pDst = ptszMem;
#endif
	while (*pSrc) {
		if (*pSrc == _T('\n'))
			*pDst++ = _T('\r');
		*pDst++ = *pSrc++;
	}
	
#ifndef _WIN32_WCE
	::GlobalUnlock(hMem);
#endif
	
	status = clipboard.setData(CF_QSTEXT, hMem);
	if (status != QSTATUS_SUCCESS) {
#ifdef _WIN32_WCE
		::LocalFree(hMem);
#else
		::GlobalFree(hMem);
#endif
		return status;
	}
	return QSTATUS_SUCCESS;
}

QSTATUS qs::Clipboard::getText(WSTRING* pwstrText)
{
	return Clipboard::getText(0, pwstrText);
}

QSTATUS qs::Clipboard::getText(HWND hwnd, WSTRING* pwstrText)
{
	assert(pwstrText);
	
	*pwstrText = 0;
	
	DECLARE_QSTATUS();
	
	if (!hwnd) {
		Window* pMainWindow = getMainWindow();
		if (!pMainWindow)
			return QSTATUS_FAIL;
		hwnd = pMainWindow->getHandle();
	}
	
	Clipboard clipboard(hwnd, &status);
	CHECK_QSTATUS();
	
	HANDLE hMem = 0;
	status = clipboard.getData(CF_QSTEXT, &hMem);
	CHECK_QSTATUS();
	assert(hMem);
	
	const TCHAR* psz = 0;
	
#ifdef _WIN32_WCE
	psz = reinterpret_cast<const TCHAR*>(hMem);
#else // _WIN32_WCE
	psz = static_cast<const TCHAR*>(::GlobalLock(hMem));
#endif
	
#ifdef UNICODE
	string_ptr<WSTRING> wstrText(allocWString(wcslen(psz) + 1));
	const WCHAR* pSrc = psz;
	WCHAR* pDst = wstrText.get();
#else
	string_ptr<WSTRING> wstrText(tcs2wcs(psz));
	const WCHAR* pSrc = wstrText.get();
	WCHAR* pDst = wstrText.get();
#endif
	while (*pSrc) {
		if (*pSrc != L'\r')
			*pDst++ = *pSrc;
		++pSrc;
	}
	*pDst = L'\0';
	
#ifndef _WIN32_WCE
	::GlobalUnlock(hMem);
#endif // _WIN32_WCE
	
	*pwstrText = wstrText.release();
	
	return QSTATUS_SUCCESS;
}
