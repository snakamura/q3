/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QMCONFIG_H__
#define __QMCONFIG_H__

#ifndef STRICT
#	define STRICT
#endif

#ifdef _MT
#	undef QM_MULTITHREAD
#	define QM_MULTITHREAD
#endif

#ifdef QMEXPORT
#	define QMEXPORTPROC __declspec(dllexport)
#	define QMEXPORTCLASS __declspec(dllexport)
#else
#	define QMEXPORTPROC __declspec(dllimport)
#	define QMEXPORTCLASS __declspec(dllimport)
#endif

#endif // __QMCONFIG_H__
