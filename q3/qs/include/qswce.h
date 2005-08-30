/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QSWCE_H__
#define __QSWCE_H__

#ifdef _WIN32_WCE

#if _WIN32_WCE < 300

typedef int INT_PTR, *PINT_PTR;
typedef unsigned int UINT_PTR, *PUINT_PTR;
typedef long LONG_PTR, *PLONG_PTR;
typedef unsigned long ULONG_PTR, *PULONG_PTR;
typedef unsigned short UHALF_PTR, *PUHALF_PTR;
typedef short HALF_PTR, *PHALF_PTR;
typedef ULONG_PTR DWORD_PTR, *PDWORD_PTR;
typedef ULONG_PTR SIZE_T, *PSIZE_T;
typedef LONG_PTR SSIZE_T, *PSSIZE_T;

#	define GMEM_FIXED          LMEM_FIXED
#	define GMEM_MOVEABLE       LMEM_MOVEABLE
#	define GPTR                LPTR
#	define GHND                LHND
#	define GMEM_DDESHARE       LMEM_DDESHARE
#	define GMEM_DISCARDABLE    LMEM_DISCARDABLE
#	define GMEM_LOWER          LMEM_LOWER
#	define GMEM_NOCOMPACT      LMEM_NOCOMPACT
#	define GMEM_NODISCARD      LMEM_NODISCARD
#	define GMEM_NOT_BANKED     LMEM_NOT_BANKED
#	define GMEM_NOTIFY         LMEM_NOTIFY
#	define GMEM_SHARE          LMEM_SHARE
#	define GMEM_ZEROINIT       LMEM_ZEROINIT

#	define GlobalAlloc(flags, cb)				LocalAlloc(flags, cb)
#	define GlobalFree(handle)					LocalFree(handle)
#	define GlobalReAlloc(handle, cb, flags)		LocalReAlloc(handle, cb, LMEM_MOVEABLE)
#	define GlobalLock(lp)						LocalLock(lp)
#	define GlobalHandle(lp)						LocalHandle(lp)
#	define GlobalUnlock(hMem)					LocalUnlock(hMem)
#	define GlobalSize(hMem)						LocalSize(hMem)
#	define GlobalFlags(X)						LocalFlags(X)
#	define LocalPtrHandle(lp)					((HLOCAL)LocalHandle(lp))
#	define LocalLockPtr(lp)						((BOOL)LocalLock(LocalPtrHandle(lp)))
#	define LocalUnlockPtr(lp)					LocalUnlock(LocalPtrHandle(lp))
#	define LocalFreePtr(lp)						(LocalUnlockPtr(lp), (BOOL)LocalFree(LocalPtrHandle(lp)))
#	define LocalLock(X)							((LPVOID)(X))
#	define LocalUnlock(X)						(0)
#	define LocalHandle(X)						((HLOCAL)(X))
#	define LocalFlags(X)						(0)

extern "C" QSEXPORTPROC int _stricmp(const char* lhs,
									 const char* rhs);
extern "C" QSEXPORTPROC int _strnicmp(const char* lhs,
									  const char* rhs,
									  size_t n);
extern "C" QSEXPORTPROC long strtol(const char* p,
									char** ppEnd,
									int nBase);
#if _STLPORT_VERSION < 0x460
extern "C" QSEXPORTPROC int isdigit(int c);
#endif

#endif // _WIN32_WCE < 300

#define GetWindowLongPtr GetWindowLong
#define SetWindowLongPtr SetWindowLong

#define GWLP_ID   GWL_ID
#define GWLP_WNDPROC   GWL_WNDPROC
#define DWLP_MSGRESULT DWL_MSGRESULT

extern "C" QSEXPORTPROC int GetMenuItemCount(HMENU hmenu);

#endif // _WIN32_WCE

#endif // __QSWCE_H__
