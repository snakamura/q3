/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __RESOURCEINC_H__
#define __RESOURCEINC_H__

#ifdef _WIN32_WCE_PSPC
#	include "resourceppc.h"
#elif defined _WIN32_WCE
#	include "resourcehpc.h"
#else
#	include "resource.h"
#endif

#endif // __RESOURCEINC_H__
