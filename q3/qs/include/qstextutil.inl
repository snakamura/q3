/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#ifdef QS_KCONVERT
#	include <kctrl.h>
#endif

/****************************************************************************
 *
 * TextUtil
 *
 */

inline bool qs::TextUtil::isHalfWidth(WCHAR c)
{
#ifdef QS_KCONVERT
	return ::is_hankaku(c) != 0;
#else
	return ::WideCharToMultiByte(CP_ACP, 0, &c, 1, 0, 0, 0, 0) == 1;
#endif
}

inline bool qs::TextUtil::isBreakChar(WCHAR c)
{
	return isBreakSelf(c) || isBreakBefore(c) || isBreakAfter(c);
}

inline bool qs::TextUtil::isBreakSelf(WCHAR c)
{
	return c == L' ';
}

inline bool qs::TextUtil::isBreakBefore(WCHAR c)
{
	return c == L'(' ||
		c == L'[' ||
		c == L'{' ||
		c == L'\t' ||
		c == L'$' ||
		c >= 0x500;
}

inline bool qs::TextUtil::isBreakAfter(WCHAR c)
{
	return c == L')' ||
		c == L']' ||
		c == L'}' ||
		c == L'%' ||
		c == L'-' ||
		c >= 0x500;
}

inline bool qs::TextUtil::isLineStartProhibited(WCHAR c)
{
	return c != L'\0' && wcschr(wszLineStartProhibited__, c) != 0;
}

inline bool qs::TextUtil::isLineEndProhibited(WCHAR c)
{
	return c != L'\0' && wcschr(wszLineEndProhibited__, c) != 0;
}

inline bool qs::TextUtil::isDangling(WCHAR c)
{
	return c != L'\0' && wcschr(wszDangling__, c) != 0;
}
