/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsconv.h>
#include <qsdevicecontext.h>
#include <qsstl.h>
#include <qsstring.h>

#include <vector>

#include <tchar.h>

using namespace qs;


/****************************************************************************
 *
 * DeviceContext
 *
 */

bool qs::DeviceContext::extTextOutEllipsis(int x,
										   int y,
										   UINT nOptions,
										   const RECT& rect,
										   const WCHAR* pwszString,
										   UINT nCount)
{
	int nWidth = rect.right - x;
	int nFit = 0;
	SIZE size;
	if (!getTextExtentEx(pwszString, nCount, nWidth, &nFit, 0, &size))
		return false;
	else if (size.cx <= nWidth)
		return extTextOut(x, y, nOptions, rect, pwszString, nCount, 0);
	
	wstring_ptr str(allocWString(nFit + 4));
	wcsncpy(str.get(), pwszString, nFit);
	while (nFit >= 0) {
		wcscpy(str.get() + nFit, L"...");
		if (!getTextExtentEx(pwszString, nFit + 3, nWidth, 0, 0, &size))
			return false;
		else if (size.cx <= nWidth)
			break;
		--nFit;
	}
	if (nFit < 0)
		return true;
	
	return extTextOut(x, y, nOptions, rect, str.get(), nFit + 3, 0);
}

#ifndef UNICODE
bool qs::DeviceContext::getTextExtentEx(const WCHAR* pwszString,
										int nCount,
										int nMaxExtent,
										int* pnFit,
										int* pnDx,
										SIZE* pSize) const
{
	assert(hdc_);
	
	string_ptr str(wcs2mbs(pwszString, nCount));
	
	int nLen = strlen(str.get());
	
	std::vector<int> dx;
	if (pnDx)
		dx.resize(nLen);
	
	if (!::GetTextExtentExPoint(hdc_, str.get(), nLen,
		nMaxExtent, pnFit, pnDx ? &dx[0] : 0, pSize))
		return false;
	
	if (pnDx) {
		int nDx = pnFit ? *pnFit : nLen;
		int* pn = pnDx;
#if 1
		const CHAR* p = str.get();
		for (int n = 0; n < nDx; ++n, ++p) {
			if (::IsDBCSLeadByte(*p)) {
				*pn++ = dx[n + 1];
				++p;
				++n;
			}
			else {
				*pn++ = dx[n];
			}
		}
#else
		// This works well on NT but not on 9x but it should.
		for (int n = 0; n < nDx; ++n) {
			if (n == 0 || dx[n] != dx[n - 1])
				*pn++ = dx[n];
		}
#endif
		if (pnFit)
			*pnFit = pn - pnDx;
	}
	else if (pnFit) {
		if (*pnFit == nLen)
			*pnFit = nCount;
		else
			*pnFit = ::MultiByteToWideChar(CP_ACP, 0, str.get(), *pnFit, 0, 0);
	}
	
	return true;
}
#endif

int qs::DeviceContext::enumFontFamilies(const WCHAR* pwszFamily,
										FONTENUMPROC pProc,
										LPARAM lParam) const
{
	assert(hdc_);
	tstring_ptr tstrFamily;
	if (pwszFamily)
		tstrFamily = wcs2tcs(pwszFamily);
	return ::EnumFontFamilies(hdc_, tstrFamily.get(), pProc, lParam);
}


/****************************************************************************
 *
 * ClientDeviceContext
 *
 */

qs::ClientDeviceContext::ClientDeviceContext(HWND hwnd) :
	DeviceContext(0),
	hwnd_(hwnd)
{
	setHandle(::GetDC(hwnd_));
}

qs::ClientDeviceContext::~ClientDeviceContext()
{
	if (getHandle())
		::ReleaseDC(hwnd_, getHandle());
}


/****************************************************************************
 *
 * WindowDeviceContext
 *
 */

qs::WindowDeviceContext::WindowDeviceContext(HWND hwnd) :
	DeviceContext(0),
	hwnd_(hwnd)
{
	setHandle(::GetWindowDC(hwnd_));
}

qs::WindowDeviceContext::~WindowDeviceContext()
{
	if (getHandle())
		::ReleaseDC(hwnd_, getHandle());
}


/****************************************************************************
 *
 * PaintDeviceContext
 *
 */

qs::PaintDeviceContext::PaintDeviceContext(HWND hwnd) :
	DeviceContext(0),
	hwnd_(hwnd)
{
	setHandle(::BeginPaint(hwnd_, &ps_));
}

qs::PaintDeviceContext::~PaintDeviceContext()
{
	if (getHandle())
		::EndPaint(hwnd_, &ps_);
}


/****************************************************************************
 *
 * CompatibleDeviceContext
 *
 */

qs::CompatibleDeviceContext::CompatibleDeviceContext(HDC hdc) :
	DeviceContext(0)
{
	setHandle(::CreateCompatibleDC(hdc));
}

CompatibleDeviceContext::~CompatibleDeviceContext()
{
	if (getHandle())
		::DeleteDC(getHandle());
}


/****************************************************************************
 *
 * FontHelper
 *
 */

static int CALLBACK enumFontFamProc(ENUMLOGFONT*,
									NEWTEXTMETRIC*,
									int,
									LPARAM);

void qs::FontHelper::createLogFont(HDC hdc,
								   const WCHAR* pwszFaceName,
								   double dPointSize,
								   unsigned int nStyle,
								   unsigned int nCharset,
								   LOGFONT* plf)
{
	memset(plf, 0, sizeof(LOGFONT));
	
	W2T(pwszFaceName, ptszFaceName);
	_tcsncpy(plf->lfFaceName, ptszFaceName, countof(plf->lfFaceName));
	plf->lfCharSet = DEFAULT_CHARSET;
	plf->lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
	
	DeviceContext dc(hdc);
	dc.enumFontFamilies(pwszFaceName,
		reinterpret_cast<FONTENUMPROC>(enumFontFamProc),
		reinterpret_cast<LPARAM>(plf));
	double dHeight = dPointSize*dc.getDeviceCaps(LOGPIXELSY)/72;
	long nHeight = static_cast<long>(dHeight);
	if (dHeight - nHeight > 0.5)
		++nHeight;
	plf->lfHeight = -nHeight;
	plf->lfWidth = 0;
	plf->lfWeight = nStyle & STYLE_BOLD ? FW_BOLD : FW_NORMAL;
	plf->lfItalic = (nStyle & STYLE_ITALIC) != 0;
	plf->lfUnderline = (nStyle & STYLE_UNDERLINE) != 0;
	plf->lfStrikeOut = (nStyle & STYLE_STRIKEOUT) != 0;
	if (nCharset)
		plf->lfCharSet = nCharset;
}

static int CALLBACK enumFontFamProc(ENUMLOGFONT* pelf,
									NEWTEXTMETRIC* pntm,
									int nFontType,
									LPARAM lParam)
{
	LOGFONT* plf = reinterpret_cast<LOGFONT*>(lParam);
	if (_tcscmp(plf->lfFaceName, pelf->elfLogFont.lfFaceName) == 0) {
		::memcpy(plf, &pelf->elfLogFont, sizeof(LOGFONT));
		return 0;
	}
	return 1;
}
