/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QS_H__
#define __QS_H__

#include <qsconfig.h>
#include <qswce.h>

#include <windows.h>

#include <memory>

typedef SSIZE_T ssize_t;

namespace qs {

/****************************************************************************
 *
 * Exceptions
 *
 */

#ifdef QS_EXCEPTION
//#	define QNOTHROW() throw()
#	define QNOTHROW()
#	define QTRY try
#	define QCATCH_ALL() catch(...)
#	define QTHROW_BADALLOC() throw(std::bad_alloc())
#else
#	define QNOTHROW()
#	define QTRY if (true)
#	define QCATCH_ALL() else
#	define QTHROW_BADALLOC() ::TerminateProcess(::GetCurrentProcess(), -1)
#endif


/****************************************************************************
 *
 * Instance
 *
 */

/**
 * Get instance handle of exe.
 *
 * @return Instance handle of exe.
 */
QSEXPORTPROC HINSTANCE getInstanceHandle();

/**
 * Get instance handle of dll.
 *
 * @return Instance handle of dll.
 */
QSEXPORTPROC HINSTANCE getDllInstanceHandle();

/**
 * Get instance handle of resource dll.
 *
 * @return Instance handle of resource dll.
 */
QSEXPORTPROC HINSTANCE getResourceDllInstanceHandle();

/**
 * Load resource dll associated with the specified instance.
 *
 * @Param hInst Instance handle.
 * @return Instance handle of resource dll if loaded, hInst otherwise.
 */
QSEXPORTPROC HINSTANCE loadResourceDll(HINSTANCE hInst);


/****************************************************************************
 *
 * Window
 *
 */

class Window;
class ModalHandler;

/**
 * Get main window.
 *
 * @return Main window.
 */
QSEXPORTPROC Window* getMainWindow();

/**
 * Set main window.
 */
QSEXPORTPROC void setMainWindow(Window* pWindow);

/**
 * Get title.
 *
 * @return Title.
 */
QSEXPORTPROC const WCHAR* getTitle();


/****************************************************************************
 *
 * misc
 *
 */

#ifdef _WIN32_WCE
#	define WCE_T(x) L##x
#else
#	define WCE_T(x) x
#endif

#if _MSC_VER < 1300
#	define for if (false); else for
#endif


/**
 * Get system encoding.
 *
 * @return System encoding.
 */
QSEXPORTPROC const WCHAR* getSystemEncoding();

#define countof(x) (sizeof(x)/sizeof(x[0]))
#define endof(x) ((x) + countof(x))

#if _STLPORT_VERSION >= 0x450 && (!defined _WIN32_WCE || _STLPORT_VERSION < 0x460)
#	define QSMIN min
#	define QSMAX max
#else
#	define QSMIN std::min
#	define QSMAX std::max
#endif

}

#endif // __QS_H__
