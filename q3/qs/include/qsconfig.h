/*
 * $Id: qsconfig.h,v 1.1.1.1 2003/04/29 08:07:34 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QSCONFIG_H__
#define __QSCONFIG_H__

#ifndef STRICT
#	define STRICT
#endif

#ifdef _MT
#	undef QS_MULTITHREAD
#	define QS_MULTITHREAD
#endif

//#ifndef JAPAN
//#	undef QS_KCONVERT
//#	define QS_KCONVERT
//#endif

#if defined QS_KCONVERT || defined QS_KDRAW
#	undef QS_KCRTL
#	define QS_KCTRL
#endif

#ifdef _WIN32_WCE
#	undef QS_KANJIIN
#	define QS_KANJIIN
#endif

#ifdef QSEXPORT
#	define QSEXPORTPROC __declspec(dllexport)
#	define QSEXPORTCLASS __declspec(dllexport)
#else
#	define QSEXPORTPROC __declspec(dllimport)
#	define QSEXPORTCLASS __declspec(dllimport)
#endif

#endif // __QSCONFIG_H__
