/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __RESOURCEVERSION_H__
#define __RESOURCEVERSION_H__

#if defined BASEPLATFORM_WIN
#	include "resourceversionwin.h"
#elif defined BASEPLATFORM_HPC
#	include "resourceversionhpc.h"
#elif defined BASEPLATFORM_PPC
#	include "resourceversionppc.h"
#else
#	error "Unsupported platform"
#endif

#endif // __RESOURCEVERSION_H__
