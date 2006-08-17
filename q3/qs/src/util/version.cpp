/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsosutil.h>

using namespace qs;


/****************************************************************************
 *
 * Version
 *
 */

#if _WIN32_WINNT >= 0x500
bool qs::Version::isWindowsXPOrLater()
{
	OSVERSIONINFOEX info = {
		sizeof(info),
		5,
		1,
	};
	
	DWORDLONG dwlConditionMask = 0;
	VER_SET_CONDITION(dwlConditionMask, VER_MAJORVERSION, VER_GREATER_EQUAL);
	VER_SET_CONDITION(dwlConditionMask, VER_MINORVERSION, VER_GREATER_EQUAL);
	VER_SET_CONDITION(dwlConditionMask, VER_SERVICEPACKMAJOR, VER_GREATER_EQUAL);
	VER_SET_CONDITION(dwlConditionMask, VER_SERVICEPACKMINOR, VER_GREATER_EQUAL);
	
	return ::VerifyVersionInfo(&info, VER_MAJORVERSION | VER_MINORVERSION, dwlConditionMask) != 0;
}
#endif
