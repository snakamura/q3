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
	static const WCHAR* getBreak(const WCHAR* pBegin,
								 const WCHAR* pEnd,
								 const WCHAR* p);
	static bool isHalfWidth(WCHAR c);
	static bool isBreakChar(WCHAR c);
	static bool isBreakSelf(WCHAR c);
	static bool isBreakBefore(WCHAR c);
	static bool isBreakAfter(WCHAR c);
	static bool isLineStartProhibited(WCHAR c);
	static bool isLineEndProhibited(WCHAR c);
	static bool isDangling(WCHAR c);
	
	static std::pair<size_t, size_t> findURL(const WCHAR* pwszText,
											 size_t nLen,
											 const WCHAR* const* ppwszSchemas,
											 size_t nSchemaCount);
	static bool isURLChar(WCHAR c);
	static bool isFileNameChar(CHAR c);
	static bool isFileNameChar(WCHAR c);
	static bool isPathChar(CHAR c);
	static bool isPathChar(WCHAR c);
	static bool isDriveLetterChar(WCHAR c);
	
	static wstring_ptr replace(const WCHAR* pwsz,
							   const WCHAR* pwszFind,
							   const WCHAR* pwszReplace);
	
	static wstring_ptr encodePassword(const WCHAR* pwsz);
	static wstring_ptr decodePassword(const WCHAR* pwsz);

private:
	static const WCHAR wszLineStartProhibited__[];
	static const WCHAR wszLineEndProhibited__[];
	static const WCHAR wszDangling__[];
};

}

#include <qstextutil.inl>

#endif // __QSTEXTUTIL_H__
