/*
 * $Id: color.cpp,v 1.1.1.1 2003/04/29 08:07:37 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsutil.h>

#include <stdio.h>

using namespace qs;


/****************************************************************************
 *
 * Color
 *
 */

qs::Color::Color(COLORREF cr) :
	cr_(cr)
{
}

qs::Color::Color(const WCHAR* pwszColor, QSTATUS* pstatus) :
	cr_(RGB(0, 0, 0))
{
	*pstatus = setString(pwszColor);
}

COLORREF qs::Color::getColor() const
{
	return cr_;
}

void qs::Color::setColor(COLORREF cr)
{
	cr_ = cr;
}

QSTATUS qs::Color::getString(WSTRING* pwstrColor) const
{
	string_ptr<WSTRING> wstrColor(allocWString(7));
	if (!wstrColor.get())
		return QSTATUS_OUTOFMEMORY;
	
	swprintf(wstrColor.get(), L"%02x%02x%02x",
		static_cast<int>(GetRValue(cr_)),
		static_cast<int>(GetGValue(cr_)),
		static_cast<int>(GetBValue(cr_)));
	
	*pwstrColor = wstrColor.release();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::Color::setString(const WCHAR* pwszColor)
{
	if (wcslen(pwszColor) != 6)
		return QSTATUS_FAIL;
	
	for (int n = 0; n < 6; ++n) {
		WCHAR c = *(pwszColor + n);
		if ((c < L'0' && L'9' < c) &&
			(c < L'A' && L'F' < c) &&
			(c < L'a' && L'f' < c))
			return QSTATUS_FAIL;
	}
	
	int nRed = 0;
	int nGreen = 0;
	int nBlue = 0;
	swscanf(pwszColor, L"%02x%02x%02x", &nRed, &nGreen, &nBlue);
	cr_ = RGB(nRed, nGreen, nBlue);
	
	return QSTATUS_SUCCESS;
}
