/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QSOSUTIL_H__
#define __QSOSUTIL_H__

#include <qs.h>
#include <qsstring.h>

#include <windows.h>

namespace qs {

struct Point;
struct Size;
struct Rect;
class AutoHandle;
class AutoFindHandle;
class AutoMenuHandle;
template<class Interface> class ComPtr;
class BSTRPtr;
class Variant;
class SafeArrayPtr;
class StgMedium;
class Library;
class Registry;
class Clipboard;


/****************************************************************************
 *
 * Point
 *
 */

struct Point : public POINT
{
	Point();
	Point(LONG x, LONG y);
};


/****************************************************************************
 *
 * Size
 *
 */

struct Size : public SIZE
{
	Size();
	Size(LONG cx, LONG cy);
};


/****************************************************************************
 *
 * Rect
 *
 */

struct Rect : public RECT
{
	Rect();
	Rect(LONG left, LONG top, LONG right, LONG bottom);
};


/****************************************************************************
 *
 * AutoHandle
 *
 */

class QSEXPORTCLASS AutoHandle
{
public:
	AutoHandle();
	explicit AutoHandle(HANDLE handle);
	~AutoHandle();

public:
	HANDLE get() const;
	HANDLE release();
	void close();
	HANDLE* operator&();

private:
	AutoHandle(const AutoHandle&);
	AutoHandle& operator=(const AutoHandle&);

private:
	HANDLE handle_;
};


/****************************************************************************
 *
 * AutoFindHandle
 *
 */

class QSEXPORTCLASS AutoFindHandle
{
public:
	explicit AutoFindHandle(HANDLE handle);
	~AutoFindHandle();

public:
	HANDLE get() const;
	HANDLE release();
	void close();

private:
	AutoFindHandle(const AutoFindHandle&);
	AutoFindHandle& operator=(const AutoFindHandle&);

private:
	HANDLE handle_;
};


/****************************************************************************
 *
 * AutoMenuHandle
 *
 */

class QSEXPORTCLASS AutoMenuHandle
{
public:
	AutoMenuHandle();
	explicit AutoMenuHandle(HMENU hmenu);
	~AutoMenuHandle();

public:
	HMENU get() const;
	HMENU release();
	HMENU* operator&();

private:
	AutoMenuHandle(const AutoMenuHandle&);
	AutoMenuHandle& operator=(const AutoMenuHandle&);

private:
	HMENU hmenu_;
};


/****************************************************************************
 *
 * ComPtr
 *
 */

template<class Interface>
class QSEXPORTCLASS ComPtr
{
public:
	ComPtr();
	explicit ComPtr(Interface* p);
	~ComPtr();

public:
	Interface** operator&();
	Interface* operator->();

public:
	Interface* get() const;
	Interface* release();

private:
	ComPtr(const ComPtr&);
	ComPtr& operator=(const ComPtr&);

private:
	Interface* p_;
};


/****************************************************************************
 *
 * BSTRPtr
 *
 */

class QSEXPORTCLASS BSTRPtr
{
public:
	BSTRPtr();
	BSTRPtr(BSTR bstr);
	~BSTRPtr();

public:
	BSTR* operator&();

public:
	BSTR get() const;
	BSTR release();

private:
	BSTRPtr(const BSTRPtr&);
	BSTRPtr& operator=(const BSTRPtr&);

private:
	BSTR bstr_;
};


/****************************************************************************
 *
 * Variant
 *
 */

class QSEXPORTCLASS Variant : public VARIANT
{
public:
	Variant();
	Variant(BSTR bstr);
	~Variant();

private:
	Variant(const Variant&);
	Variant& operator=(const Variant&);
};


/****************************************************************************
 *
 * SafeArrayPtr
 *
 */

class QSEXPORTCLASS SafeArrayPtr
{
public:
	SafeArrayPtr(SAFEARRAY* pArray);
	~SafeArrayPtr();

public:
	SAFEARRAY* get() const;
	SAFEARRAY* release();

private:
	SafeArrayPtr(const SafeArrayPtr&);
	SafeArrayPtr& operator=(const SafeArrayPtr&);

private:
	SAFEARRAY* pArray_;
};


/****************************************************************************
 *
 * StgMedium
 *
 */

class QSEXPORTCLASS StgMedium : public STGMEDIUM
{
public:
	StgMedium();
	~StgMedium();

private:
	StgMedium(const StgMedium&);
	StgMedium& operator=(const StgMedium&);
};


/****************************************************************************
 *
 * Library
 *
 */

class QSEXPORTCLASS Library
{
public:
	Library(const WCHAR* pwszPath, QSTATUS* pstatus);
	~Library();

public:
	bool operator!() const;
	operator HINSTANCE() const;

private:
	Library(const Library&);
	Library& operator=(const Library&);

private:
	HINSTANCE hInst_;
};


/****************************************************************************
 *
 * Registry
 *
 */

class QSEXPORTCLASS Registry
{
public:
	Registry(HKEY hkey, const WCHAR* pwszSubKey, QSTATUS* pstatus);
	Registry(HKEY hkey, const WCHAR* pwszSubKey,
		const WCHAR* pwszClass, QSTATUS* pstatus);
	~Registry();

public:
	bool operator!() const;
	operator HKEY() const;

public:
	QSTATUS getValue(const WCHAR* pwszName, DWORD* pdwValue);
	QSTATUS getValue(const WCHAR* pwszName, DWORD* pdwValue, LONG* pnRet);
	QSTATUS getValue(const WCHAR* pwszName, WSTRING* pwstrValue);
	QSTATUS getValue(const WCHAR* pwszName, WSTRING* pwstrValue, LONG* pnRet);
	QSTATUS getValue(const WCHAR* pwszName, BYTE* pByte, int* pnSize);
	QSTATUS getValue(const WCHAR* pwszName, BYTE* pByte, int* pnSize, LONG* pnRet);
	QSTATUS setValue(const WCHAR* pwszName, DWORD dwValue);
	QSTATUS setValue(const WCHAR* pwszName, DWORD dwValue, LONG* pnRet);
	QSTATUS setValue(const WCHAR* pwszName, const WCHAR* pwszValue);
	QSTATUS setValue(const WCHAR* pwszName, const WCHAR* pwszValue, LONG* pnRet);
	QSTATUS setValue(const WCHAR* pwszName, const BYTE* pByte, int nSize);
	QSTATUS setValue(const WCHAR* pwszName, const BYTE* pByte, int nSize, LONG* pnRet);

public:
	static QSTATUS deleteKey(HKEY hkey, const WCHAR* pwszSubKey, LONG* pnRet);

private:
	QSTATUS init(HKEY hkey, const WCHAR* pwszSubKey, const WCHAR* pwszClass);

private:
	Registry(const Registry&);
	Registry operator=(const Registry&);

private:
	HKEY hkey_;
};


/****************************************************************************
 *
 * Clipboard
 *
 */

class QSEXPORTCLASS Clipboard
{
public:
	enum {
#ifdef UNICODE
		CF_QSTEXT	= CF_UNICODETEXT
#else
		CF_QSTEXT	= CF_TEXT
#endif
	};

public:
	Clipboard(HWND hwnd, QSTATUS* pstatus);
	~Clipboard();

public:
	QSTATUS close();
	QSTATUS getData(UINT nFormat, HANDLE* phMem) const;
	QSTATUS setData(UINT nFormat, HANDLE hMem);
	QSTATUS setData(UINT nFormat, HANDLE hMem, HANDLE* phMem);
	QSTATUS empty() const;

public:
	static QSTATUS isFormatAvailable(UINT nFormat, bool* pbAvailable);
	static QSTATUS setText(const WCHAR* pwszText);
	static QSTATUS setText(HWND hwnd, const WCHAR* pwszText);
	static QSTATUS getText(WSTRING* pwstrText);
	static QSTATUS getText(HWND hwnd, WSTRING* pwstrText);

private:
	Clipboard(const Clipboard&);
	Clipboard& operator=(const Clipboard&);

private:
	bool bOpen_;
};

}

#include <qsosutil.inl>

#endif // __QSOSUTIL_H__
