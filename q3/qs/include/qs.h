/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QS_H__
#define __QS_H__

#include <qsconfig.h>

#include <windows.h>

#include <memory>

#if defined _WIN32_WCE && _WIN32_WCE <= 211
	typedef int INT_PTR;
#endif

namespace qs {

/****************************************************************************
 *
 * Error Status
 *
 */

typedef unsigned int QSTATUS;


/****************************************************************************
 *
 * Instance
 *
 */

QSEXPORTPROC HINSTANCE getInstanceHandle();
QSEXPORTPROC HINSTANCE getDllInstanceHandle();


/****************************************************************************
 *
 * Window
 *
 */

class Window;
class ModalHandler;

QSEXPORTPROC Window* getMainWindow();
QSEXPORTPROC void setMainWindow(Window* pWindow);

QSEXPORTPROC const WCHAR* getTitle();

QSEXPORTPROC ModalHandler* getModalHandler();
QSEXPORTPROC void setModalHandler(ModalHandler* pModalHandler);


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

QSEXPORTPROC const WCHAR* getSystemEncoding();

#define countof(x) sizeof(x)/sizeof(x[0])

#if _STLPORT_VERSION >= 0x450
#	define QSMIN min
#	define QSMAX max
#else
#	define QSMIN std::min
#	define QSMAX std::max
#endif


}

#endif // __QS_H__
