/*
 * $Id: qstextutil.h,v 1.1.1.1 2003/04/29 08:07:35 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QSTEXTUTIL_H__
#define __QSTEXTUTIL_H__

#include <qs.h>
#include <qsstring.h>


namespace qs {

class TextUtil;


/****************************************************************************
 *
 * TextUtil
 *
 */

class QSEXPORTCLASS TextUtil
{
public:
	static QSTATUS fold(const WCHAR* pwszText, size_t nLen,
		size_t nLineWidth, const WCHAR* pwszQuote, size_t nQuoteLen,
		size_t nTabWidth, WSTRING* pwstrText);
	static bool isHalfWidth(WCHAR c);
	static bool isBreakChar(WCHAR c);
	static bool isBreakSelf(WCHAR c);
	static bool isBreakBefore(WCHAR c);
	static bool isBreakAfter(WCHAR c);
	
	static std::pair<size_t, size_t> findURL(const WCHAR* pwszText,
		size_t nLen, const WCHAR* const* ppwszSchemas, size_t nSchemaCount);
	static bool isURLChar(WCHAR c);
};

}

#include <qstextutil.inl>

#endif // __QSTEXTUTIL_H__
