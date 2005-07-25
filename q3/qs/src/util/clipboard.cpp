/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsosutil.h>
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

qs::Clipboard::Clipboard(HWND hwnd) :
	bOpen_(false)
{
	bOpen_ = ::OpenClipboard(hwnd) != 0;
}

qs::Clipboard::~Clipboard()
{
	close();
}

bool qs::Clipboard::operator!() const
{
	return !bOpen_;
}

bool qs::Clipboard::close()
{
	if (bOpen_)
		return ::CloseClipboard() != 0;
	else
		return true;
}

HANDLE qs::Clipboard::getData(UINT nFormat) const
{
	return ::GetClipboardData(nFormat);
}

HANDLE qs::Clipboard::setData(UINT nFormat,
							  HANDLE hMem)
{
	return ::SetClipboardData(nFormat, hMem);
}

bool qs::Clipboard::empty() const
{
	return ::EmptyClipboard() != 0;
}

bool qs::Clipboard::isFormatAvailable(UINT nFormat)
{
	return ::IsClipboardFormatAvailable(nFormat) != 0;
}

bool qs::Clipboard::setText(const WCHAR* pwszText)
{
	return Clipboard::setText(0, pwszText);
}

bool qs::Clipboard::setText(HWND hwnd,
							const WCHAR* pwszText)
{
	if (!hwnd) {
		Window* pMainWindow = getMainWindow();
		if (!pMainWindow)
			return false;
		hwnd = pMainWindow->getHandle();
	}
	
	Clipboard clipboard(hwnd);
	if (!clipboard)
		return false;
	
	if (!clipboard.empty())
		return false;
	
	size_t nTextLen = wcslen(pwszText);
	size_t nCount = std::count(pwszText, pwszText + nTextLen, L'\n');
	
#ifdef UNICODE
	HANDLE hMem = GlobalAlloc(GMEM_FIXED, (nTextLen + nCount + 1)*sizeof(WCHAR));
	if (!hMem)
		return false;
	LockGlobal lock(hMem);
	WCHAR* pwszMem = static_cast<WCHAR*>(lock.get());
	const WCHAR* pSrc = pwszText;
	WCHAR* pDst = pwszMem;
#else // UNICODE
	tstring_ptr tstrText(wcs2tcs(pwszText));
	HANDLE hMem = GlobalAlloc(GMEM_FIXED, (strlen(tstrText.get()) + nCount + 1)*sizeof(CHAR));
	if (!hMem)
		return false;
	LockGlobal lock(hMem);
	TCHAR* ptszMem = static_cast<TCHAR*>(lock.get());
	const TCHAR* pSrc = tstrText.get();
	TCHAR* pDst = ptszMem;
#endif
	while (*pSrc) {
		if (*pSrc == _T('\n'))
			*pDst++ = _T('\r');
		*pDst++ = *pSrc++;
	}
	*pDst = _T('\0');
	
	if (!clipboard.setData(CF_QSTEXT, hMem)) {
#ifdef _WIN32_WCE
		::LocalFree(hMem);
#else
		::GlobalFree(hMem);
#endif
		return false;
	}
	
	return true;
}

wstring_ptr qs::Clipboard::getText()
{
	return Clipboard::getText(0);
}

wstring_ptr qs::Clipboard::getText(HWND hwnd)
{
	if (!hwnd) {
		Window* pMainWindow = getMainWindow();
		if (!pMainWindow)
			return 0;
		hwnd = pMainWindow->getHandle();
	}
	
	Clipboard clipboard(hwnd);
	if (!clipboard)
		return 0;
	
	HANDLE hMem = clipboard.getData(CF_QSTEXT);
	if (!hMem)
		return 0;
	
	LockGlobal lock(hMem);
	const TCHAR* psz = static_cast<const TCHAR*>(lock.get());
	
#ifdef UNICODE
	wstring_ptr wstrText(allocWString(wcslen(psz) + 1));
	const WCHAR* pSrc = psz;
	WCHAR* pDst = wstrText.get();
#else
	wstring_ptr wstrText(tcs2wcs(psz));
	const WCHAR* pSrc = wstrText.get();
	WCHAR* pDst = wstrText.get();
#endif
	while (*pSrc) {
		if (*pSrc != L'\r')
			*pDst++ = *pSrc;
		++pSrc;
	}
	*pDst = L'\0';
	
	return wstrText;
}
