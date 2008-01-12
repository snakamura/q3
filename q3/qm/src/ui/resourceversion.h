/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
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
