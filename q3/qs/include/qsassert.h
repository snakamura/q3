/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
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
