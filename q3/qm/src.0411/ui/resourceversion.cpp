/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#include <qmresourceversion.h>

#if defined BASEPLATFORM_WIN
#	include "resourceversionwin.h"
#elif defined BASEPLATFORM_HPC
#	include "resourceversionhpc.h"
#elif defined BASEPLATFORM_PPC
#	include "resourceversionppc.h"
#else
#	error "Unsupported platform"
#endif

using namespace qm;

extern "C" QMEXPORTPROC const ResourceVersion* getResourceVersions() {
	return resourceVersions;
}
