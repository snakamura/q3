/*
 * $Id: qsassert.h,v 1.1.1.1 2003/04/29 08:07:34 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QSASSERT_H__
#define __QSASSERT_H__

#ifdef _WIN32_WCE
#	undef assert
#	define assert(x)
#	define _ASSERT_DEFINED
#else
#	include <cassert>
#endif

#endif // __QSASSERT_H__
