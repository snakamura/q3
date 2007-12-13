/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
 * All rights reserved.
 *
 */

#if defined BASEPLATFORM_WIN
#	include "versionwin.h"
#elif defined BASEPLATFORM_HPC
#	include "versionhpc.h"
#elif defined BASEPLATFORM_PPC
#	include "versionppc.h"
#else
#	error "Unsupported platform"
#endif
