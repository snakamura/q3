/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
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
class LockGlobal;
template<class Interface> class ComPtr;
class BSTRPtr;
class Variant;
class SafeArrayPtr;
class StgMedium;
class Library;
class Registry;
class Clipboard;
class Process;

class InputStream;
class OutputStream;


/****************************************************************************
 *
 * Point
 *
 */

struct Point : public POINT
{
	Point();
	Point(LONG x,
		  LONG y);
};


/****************************************************************************
 *
 * Size
 *
 */

struct Size : public SIZE
{
	Size();
	Size(LONG cx,
		 LONG cy);
};


/****************************************************************************
 *
 * Rect
 *
 */

struct Rect : public RECT
{
	Rect();
	Rect(LONG left,
		 LONG top,
		 LONG right,
		 LONG bottom);
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
 * LockGlobal
 *
 */

class QSEXPORTCLASS LockGlobal
{
public:
	LockGlobal(HGLOBAL hGlobal);
	~LockGlobal();

public:
	void* get() const;

private:
	LockGlobal(const LockGlobal&);
	LockGlobal& operator=(const LockGlobal&);

private:
	HGLOBAL hGlobal_;
	void* p_;
};


/****************************************************************************
 *
 * ComPtr
 *
 */

template<class Interface>
class ComPtr
{
public:
	ComPtr();
	explicit ComPtr(Interface* p);
	ComPtr(const ComPtr& ptr);
	~ComPtr();

public:
	Interface** operator&();
	Interface* operator->();
	ComPtr& operator=(const ComPtr& ptr);

public:
	Interface* get() const;
	Interface* release();

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
	explicit Library(const WCHAR* pwszPath);
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
	/**
	 * Create instance with the specified key and the specified
	 * name of the sub key.
	 * Call operator! to check if success or not.
	 *
	 * @param hkey [in] Key.
	 * @param pwszSubKey [in] Sub key name.
	 */
	Registry(HKEY hkey,
			 const WCHAR* pwszSubKey);
	
	/**
	 * Create instance with the specified key and the specified
	 * name of the sub key.
	 * Call operator! to check if success or not.
	 *
	 * @param hkey [in] Key.
	 * @param pwszSubKey [in] Sub key name.
	 * @param bReadOnly [in] Readonly or not.
	 */
	Registry(HKEY hkey,
			 const WCHAR* pwszSubKey,
			 bool bReadOnly);
	
	/**
	 * Create instance with the specified key and the specified
	 * name of the sub key, the class.
	 * Call operator! to check if success or not.
	 *
	 * @param hkey [in] Key.
	 * @param pwszSubKey [in] Sub key name.
	 * @param pwszClass [in] Sub key class.
	 */
	Registry(HKEY hkey,
			 const WCHAR* pwszSubKey,
			 const WCHAR* pwszClass);
	
	~Registry();

public:
	/**
	 * Check if key is opened or not.
	 *
	 * @return true if key is not opened, false otherwise.
	 */
	bool operator!() const;
	
	/**
	 * Get the handle of the key.
	 *
	 * @return Handle.
	 */
	operator HKEY() const;

public:
	/**
	 * Get number value.
	 *
	 * @param pwszName [in] Value name, null if getting default value.
	 * @param pdwValue [out] Value.
	 * @return true if success, false otherwise.
	 * @exception std::bad_alloc Out of memory.
	 */
	bool getValue(const WCHAR* pwszName,
				  DWORD* pdwValue);
	
	/**
	 * Get string value.
	 *
	 * @param pwszName [in] Value name, null if getting default value.
	 * @param pwstrValue [out] Value.
	 * @return true if success, false otherwise.
	 * @exception std::bad_alloc Out of memory.
	 */
	bool getValue(const WCHAR* pwszName,
				  wstring_ptr* pwstrValue);
	
	/**
	 * Get binary value. Buffer must be allocated.
	 *
	 * @param pwszName [in] Value name, null if getting default value.
	 * @param pByte [in] Buffer.
	 * @param pdwSize [in] Buffer size.
	 *                [out] Size written.
	 * @return true if success, false otherwise.
	 * @exception std::bad_alloc Out of memory.
	 */
	bool getValue(const WCHAR* pwszName,
				  BYTE* pByte,
				  DWORD* pdwSize);
	
	/**
	 * Set number value.
	 *
	 * @param pwszName [in] Value name, null if getting default value.
	 * @param dwValue [in] Value.
	 * @return true if success, false otherwise.
	 * @exception std::bad_alloc Out of memory.
	 */
	bool setValue(const WCHAR* pwszName,
				  DWORD dwValue);
	
	/**
	 * Set string value.
	 *
	 * @param pwszName [in] Value name, null if getting default value.
	 * @param pwszValue [in] Value.
	 * @return true if success, false otherwise.
	 * @exception std::bad_alloc Out of memory.
	 */
	bool setValue(const WCHAR* pwszName,
				  const WCHAR* pwszValue);
	
	/**
	 * Set binary value.
	 *
	 * @param pwszName [in] Value name, null if getting default value.
	 * @param pByte [in] Buffer.
	 * @param dwSize [in] Buffer size.
	 * @return true if success, false otherwise.
	 * @exception std::bad_alloc Out of memory.
	 */
	bool setValue(const WCHAR* pwszName,
				  const BYTE* pByte,
				  DWORD dwSize);

public:
	/**
	 * Delete the specified key.
	 *
	 * @param hkey [in] Key.
	 * @param pwszSubKey [in] Sub key name.
	 */
	static bool deleteKey(HKEY hkey,
						  const WCHAR* pwszSubKey);

private:
	void init(HKEY hkey,
			  const WCHAR* pwszSubKey,
			  const WCHAR* pwszClass,
			  bool bReadOnly);

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
	/**
	 * Create instance.
	 * Call operator! to check if success or not.
	 *
	 * @param hwnd [in] Window handle. Can be null.
	 */
	explicit Clipboard(HWND hwnd);
	
	~Clipboard();

public:
	/**
	 * Check if clipboard is opened or not.
	 *
	 * @return true if clipboard is not opened, false otherwise.
	 */
	bool operator!() const;

public:
	/**
	 * Close clipboard.
	 *
	 * @return true if success, false otherwise.
	 */
	bool close();
	
	/**
	 * Get data from clipboard with the specified format.
	 *
	 * @param nFormat [in] Clipboard format.
	 * @return HANDLE Data.
	 */
	HANDLE getData(UINT nFormat) const;
	
	/**
	 * Set data to clipboard.
	 *
	 * @param nFormat [in] Clipboard format.
	 * @param hMem [in] Data.
	 * @return Privous data.
	 */
	HANDLE setData(UINT nFormat,
				   HANDLE hMem);
	
	/**
	 * Empty clipboard.
	 *
	 * @return true if success, false otherwise.
	 */
	bool empty() const;

public:
	/**
	 * Check if the specified format is available or not.
	 *
	 * @param nFormat [in] Clipboard format.
	 * @return true if available, false otherwise.
	 */
	static bool isFormatAvailable(UINT nFormat);
	
	/**
	 * Set text to clipboard.
	 *
	 * @param pwszText [in] String.
	 * @return true if success, false otherwise.
	 * @exception std::bad_alloc Out of memory.
	 */
	static bool setText(const WCHAR* pwszText);
	
	/**
	 * Set text to clipboard.
	 *
	 * @param hwnd [in] Window handle.
	 * @param pwszText [in] String.
	 * @return true if success, false otherwise.
	 * @exception std::bad_alloc Out of memory.
	 */
	static bool setText(HWND hwnd,
						const WCHAR* pwszText);
	
	/**
	 * Get text from clipboard.
	 *
	 * @return String.
	 * @exception std::bad_alloc Out of memory.
	 */
	static wstring_ptr getText();
	
	/**
	 * Get text from clipboard.
	 *
	 * @param hwnd [in] Window handle.
	 * @return String.
	 * @exception std::bad_alloc Out of memory.
	 */
	static wstring_ptr getText(HWND hwnd);

private:
	Clipboard(const Clipboard&);
	Clipboard& operator=(const Clipboard&);

private:
	bool bOpen_;
};


/****************************************************************************
 *
 * Process
 *
 */

class QSEXPORTCLASS Process
{
public:
#ifndef _WIN32_WCE
	/**
	 * Execute the specified command.
	 *
	 * @param pwszCommand [in] Command.
	 * @param pwszInput [in] String witch is written to stdin. Can be null.
	 * @return String witch is written to stdout.
	 * @exception std::bad_alloc Out of memory.
	 */
	static wstring_ptr exec(const WCHAR* pwszCommand,
							const WCHAR* pwszInput);
	
	static int exec(const WCHAR* pwszCommand,
					InputStream* pStdInput,
					OutputStream* pStdOutput,
					OutputStream* pStdError);
#endif
};

}

#include <qsosutil.inl>

#endif // __QSOSUTIL_H__
