/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QMSCRIPTCONFIG_H__
#define __QMSCRIPTCONFIG_H__

#ifndef STRICT
#	define STRICT
#endif

#ifdef QMSCRIPTEXPORT
#	define QMSCRIPTEXPORTPROC __declspec(dllexport)
#	define QMSCRIPTEXPORTCLASS __declspec(dllexport)
#else
#	define QMSCRIPTEXPORTPROC __declspec(dllimport)
#	define QMSCRIPTEXPORTCLASS __declspec(dllimport)
#endif

#endif // __QMSCRIPTCONFIG_H__
