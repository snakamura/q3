/*
 * $Id: resourceinc.h,v 1.1.1.1 2003/04/29 08:07:32 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
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
