/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#include <qs.h>

#include <cstdlib>


#ifdef _WIN32_WCE

#if _WIN32_WCE < 0x300

extern "C" QSEXPORTPROC int _stricmp(const char* lhs,
									 const char* rhs)
{
	while (*lhs) {
		if (*lhs != *rhs) {
			char cLhs = tolower(*lhs);
			char cRhs = tolower(*rhs);
			if (cLhs < cRhs)
				return -1;
			else if (cLhs > cRhs)
				return 1;
		}
		++lhs;
		++rhs;
	}
	return *rhs ? -1 : 0;
}

extern "C" QSEXPORTPROC int _strnicmp(const char* lhs,
									  const char* rhs,
									  size_t n)
{
	for (size_t m = 0; m < n; ++m) {
		if (!*lhs || !*rhs) {
			if (*lhs < *rhs)
				return -1;
			else if (*lhs > *rhs)
				return 1;
			else
				return 0;
		}
		else if (*lhs != *rhs) {
			char cLhs = tolower(*lhs);
			char cRhs = tolower(*rhs);
			if (cLhs < cRhs)
				return -1;
			else if (cLhs > cRhs)
				return 1;
		}
		++lhs;
		++rhs;
	}
	return 0;
}

bool isValid(char c,
			 int nBase)
{
	if (nBase <= 10)
		return '0' <= c && c < '0' + nBase;
	else
		return ('0' <= c && c <= '9') ||
			('a' <= c && c < 'a' + (nBase - 10)) ||
			('A' <= c && c < 'A' + (nBase - 10));
}

extern "C" QSEXPORTPROC long strtol(const char* p,
									char** ppEnd,
									int nBase)
{
	const char* pOrg = p;
	WCHAR wsz[32];
	WCHAR* pw = wsz;
	while (p - pOrg < countof(wsz) && isValid(*p, nBase)) {
		*pw++ = static_cast<WCHAR>(*p);
		++p;
	}
	if (p - pOrg == countof(wsz))
		return 0;
	*pw = L'\0';
	*ppEnd = const_cast<char*>(p);
	
	WCHAR* pEnd = 0;
	return wcstol(wsz, &pEnd, nBase);
}

#if _STLPORT_VERSION < 0x460
extern "C" QSEXPORTPROC int isdigit(int c)
{
	return '0' <= c && c <= '9';
}
#endif

#endif // _WIN32_WCE < 0x300

extern "C" QSEXPORTPROC int GetMenuItemCount(HMENU hmenu)
{
	int n = 0;
	while (true) {
		MENUITEMINFO mii = { sizeof(mii) };
		if (!::GetMenuItemInfo(hmenu, n, TRUE, &mii))
			break;
		++n;
	}
	return n;
}

#endif // _WIN32_WCE
