/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsdevicecontext.h>
#include <qsconv.h>
#include <qsstring.h>

#include <tchar.h>

using namespace qs;


/****************************************************************************
 *
 * DeviceContext
 *
 */

int qs::DeviceContext::enumFontFamilies(const WCHAR* pwszFamily,
	FONTENUMPROC pProc, LPARAM lParam) const
{
	assert(hdc_);
	string_ptr<TSTRING> tstrFamily(wcs2tcs(pwszFamily));
	if (!tstrFamily.get())
		return -1;
	return ::EnumFontFamilies(hdc_, tstrFamily.get(), pProc, lParam);
}


/****************************************************************************
 *
 * ClientDeviceContext
 *
 */

qs::ClientDeviceContext::ClientDeviceContext(HWND hwnd, QSTATUS* pstatus) :
	DeviceContext(0),
	hwnd_(hwnd)
{
	assert(pstatus);
	
	HDC hdc = ::GetDC(hwnd_);
	if (!hdc) {
		*pstatus = QSTATUS_FAIL;
		return;
	}
	setHandle(hdc);
}

qs::ClientDeviceContext::~ClientDeviceContext()
{
	::ReleaseDC(hwnd_, getHandle());
}


/****************************************************************************
 *
 * PaintDeviceContext
 *
 */

qs::PaintDeviceContext::PaintDeviceContext(HWND hwnd, QSTATUS* pstatus) :
	DeviceContext(0),
	hwnd_(hwnd)
{
	assert(pstatus);
	
	HDC hdc = ::BeginPaint(hwnd_, &ps_);
	if (!hdc) {
		*pstatus = QSTATUS_FAIL;
		return;
	}
	setHandle(hdc);
}

qs::PaintDeviceContext::~PaintDeviceContext()
{
	::EndPaint(hwnd_, &ps_);
}


/****************************************************************************
 *
 * CompatibleDeviceContext
 *
 */

qs::CompatibleDeviceContext::CompatibleDeviceContext(
	HDC hdc, QSTATUS* pstatus) :
	DeviceContext(0)
{
	assert(pstatus);
	
	*pstatus = QSTATUS_SUCCESS;
	
	HDC hdcMem = ::CreateCompatibleDC(hdc);
	if (!hdcMem) {
		*pstatus = QSTATUS_FAIL;
		return;
	}
	setHandle(hdcMem);
}

CompatibleDeviceContext::~CompatibleDeviceContext()
{
	::DeleteDC(getHandle());
}


/****************************************************************************
 *
 * FontHelper
 *
 */

int CALLBACK enumFontFamProc(ENUMLOGFONT*, NEWTEXTMETRIC*, int, LPARAM);

QSTATUS qs::FontHelper::createLogFont(HDC hdc, const WCHAR* pwszFaceName,
	int nPointSize, unsigned int nStyle, LOGFONT* plf)
{
	DECLARE_QSTATUS();
	
	memset(plf, 0, sizeof(LOGFONT));
	
	W2T(pwszFaceName, ptszFaceName);
	_tcsncpy(plf->lfFaceName, ptszFaceName, countof(plf->lfFaceName));
	
	DeviceContext dc(hdc);
	dc.enumFontFamilies(pwszFaceName,
		reinterpret_cast<FONTENUMPROC>(enumFontFamProc),
		reinterpret_cast<LPARAM>(plf));
	plf->lfHeight = -(nPointSize*dc.getDeviceCaps(LOGPIXELSY)/72);
	plf->lfWidth = 0;
	plf->lfWeight = nStyle & STYLE_BOLD ? FW_BOLD : FW_NORMAL;
	plf->lfItalic = (nStyle & STYLE_ITALIC) != 0;
	plf->lfUnderline = (nStyle & STYLE_UNDERLINE) != 0;
	plf->lfStrikeOut = (nStyle & STYLE_STRIKEOUT) != 0;
	plf->lfCharSet = DEFAULT_CHARSET;
	
	return QSTATUS_SUCCESS;
}

int CALLBACK enumFontFamProc(ENUMLOGFONT* pelf,
	NEWTEXTMETRIC* pntm, int nFontType, LPARAM lParam)
{
	LOGFONT* plf = reinterpret_cast<LOGFONT*>(lParam);
	if (_tcscmp(plf->lfFaceName, pelf->elfLogFont.lfFaceName) == 0) {
		::memcpy(plf, &pelf->elfLogFont, sizeof(LOGFONT));
		return 0;
	}
	return 1;
}
