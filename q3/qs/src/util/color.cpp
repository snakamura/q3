/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
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

qs::Color::Color(const WCHAR* pwszColor) :
	cr_(RGB(0, 0, 0))
{
	if (!setString(pwszColor))
		cr_ = 0xffffffff;
}

COLORREF qs::Color::getColor() const
{
	return cr_;
}

void qs::Color::setColor(COLORREF cr)
{
	cr_ = cr;
}

wstring_ptr qs::Color::getString() const
{
	const size_t nLen = 7;
	wstring_ptr wstrColor(allocWString(nLen));
	_snwprintf(wstrColor.get(), nLen, L"%02x%02x%02x",
		static_cast<int>(GetRValue(cr_)),
		static_cast<int>(GetGValue(cr_)),
		static_cast<int>(GetBValue(cr_)));
	return wstrColor;
}

bool qs::Color::setString(const WCHAR* pwszColor)
{
	if (wcslen(pwszColor) != 6)
		return false;
	
	for (int n = 0; n < 6; ++n) {
		WCHAR c = *(pwszColor + n);
		if ((c < L'0' && L'9' < c) &&
			(c < L'A' && L'F' < c) &&
			(c < L'a' && L'f' < c))
			return false;
	}
	
	int nRed = 0;
	int nGreen = 0;
	int nBlue = 0;
	swscanf(pwszColor, L"%02x%02x%02x", &nRed, &nGreen, &nBlue);
	cr_ = RGB(nRed, nGreen, nBlue);
	
	return true;
}
