/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __RESOURCEINC_H__
#define __RESOURCEINC_H__

#include "resource.h"
#ifdef _WIN32_WCE_PSPC
#	include "resourceppc.h"
#elif defined _WIN32_WCE
#	include "resourcehpc.h"
#else
#	include "resourcewin.h"
#endif

#endif // __RESOURCEINC_H__
