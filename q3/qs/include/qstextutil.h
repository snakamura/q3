/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
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
	static wxstring_ptr fold(const WCHAR* pwszText,
							 size_t nLen,
							 size_t nLineWidth,
							 const WCHAR* pwszQuote,
							 size_t nQuoteLen,
							 size_t nTabWidth);
	static bool isHalfWidth(WCHAR c);
	static bool isBreakChar(WCHAR c);
	static bool isBreakSelf(WCHAR c);
	static bool isBreakBefore(WCHAR c);
	static bool isBreakAfter(WCHAR c);
	
	static std::pair<size_t, size_t> findURL(const WCHAR* pwszText,
											 size_t nLen,
											 const WCHAR* const* ppwszSchemas,
											 size_t nSchemaCount);
	static bool isURLChar(WCHAR c);
	
	static wstring_ptr encodePassword(const WCHAR* pwsz);
	static wstring_ptr decodePassword(const WCHAR* pwsz);
};

}

#include <qstextutil.inl>

#endif // __QSTEXTUTIL_H__
