/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsconv.h>
#include <qsosutil.h>

#include <tchar.h>

using namespace qs;


/****************************************************************************
 *
 * Registry
 *
 */

qs::Registry::Registry(HKEY hkey,
					   const WCHAR* pwszSubKey) :
	hkey_(0)
{
	init(hkey, pwszSubKey, 0);
}

qs::Registry::Registry(HKEY hkey,
					   const WCHAR* pwszSubKey,
					   const WCHAR* pwszClass) :
	hkey_(0)
{
	init(hkey, pwszSubKey, pwszClass);
}

qs::Registry::~Registry()
{
}

bool qs::Registry::operator!() const
{
	return hkey_ == 0;
}

qs::Registry::operator HKEY() const
{
	return hkey_;
}

bool qs::Registry::getValue(const WCHAR* pwszName,
							DWORD* pdwValue)
{
	assert(pdwValue);
	
	if (!hkey_)
		return false;
	
	W2T(pwszName, ptszName);
	
	DWORD dwSize = sizeof(DWORD);
	DWORD dwType = REG_DWORD;
	return ::RegQueryValueEx(hkey_, ptszName, 0, &dwType,
		reinterpret_cast<LPBYTE>(pdwValue), &dwSize) == ERROR_SUCCESS;
}

bool qs::Registry::getValue(const WCHAR* pwszName,
							wstring_ptr* pwstrValue)
{
	assert(pwstrValue);
	
	if (!hkey_)
		return false;
	
	W2T(pwszName, ptszName);
	
	DWORD dwType = REG_SZ;
	DWORD dwSize = 0;
	if (::RegQueryValueEx(hkey_, ptszName, 0, &dwType, 0, &dwSize) != ERROR_SUCCESS)
		return false;
	tstring_ptr tstrValue(allocTString(dwSize + 1));
	if (::RegQueryValueEx(hkey_, ptszName, 0, &dwType,
		reinterpret_cast<LPBYTE>(tstrValue.get()), &dwSize) != ERROR_SUCCESS)
		return false;
	*pwstrValue = tcs2wcs(tstrValue.get());
	
	return true;
}

bool qs::Registry::getValue(const WCHAR* pwszName,
							BYTE* pByte,
							size_t* pnSize)
{
	assert(pByte);
	assert(pnSize);
	
	if (!hkey_)
		return false;
	
	W2T(pwszName, ptszName);
	
	DWORD dwSize = *pnSize;
	DWORD dwType = REG_BINARY;
	if (!::RegQueryValueEx(hkey_, ptszName, 0, &dwType, pByte, &dwSize) != ERROR_SUCCESS)
		return false;
	*pnSize = dwSize;
	
	return true;
}

bool qs::Registry::setValue(const WCHAR* pwszName,
							DWORD dwValue)
{
	if (!hkey_)
		return false;
	
	W2T(pwszName, ptszName);
	return ::RegSetValueEx(hkey_, ptszName, 0, REG_DWORD,
		reinterpret_cast<const BYTE*>(&dwValue), sizeof(DWORD)) == ERROR_SUCCESS;
}

bool qs::Registry::setValue(const WCHAR* pwszName,
							const WCHAR* pwszValue)
{
	if (!hkey_)
		return false;
	
	W2T(pwszName, ptszName);
	W2T(pwszValue, ptszValue);
	return ::RegSetValueEx(hkey_, ptszName, 0, REG_SZ,
		reinterpret_cast<const BYTE*>(ptszValue),
		(_tcslen(ptszValue) + 1)*sizeof(TCHAR)) == ERROR_SUCCESS;
}

bool qs::Registry::setValue(const WCHAR* pwszName,
							const BYTE* pByte,
							size_t nSize)
{
	if (!hkey_)
		return false;
	
	W2T(pwszName, ptszName);
	return ::RegSetValueEx(hkey_, ptszName, 0, REG_BINARY, pByte, nSize) == ERROR_SUCCESS;
}

bool qs::Registry::deleteKey(HKEY hkey,
							 const WCHAR* pwszSubKey)
{
	assert(hkey);
	
	W2T(pwszSubKey, ptszSubKey);
	return ::RegDeleteKey(hkey, ptszSubKey) == ERROR_SUCCESS;
}

void qs::Registry::init(HKEY hkey,
						const WCHAR* pwszSubKey,
						const WCHAR* pwszClass)
{
	W2T(pwszSubKey, ptszSubKey);
	
	tstring_ptr tstrClass;
	if (pwszClass)
		tstrClass = wcs2tcs(pwszClass);
	
	DWORD dw = 0;
#ifdef _WIN32_WCE
	LONG nRet = ::RegCreateKeyEx(hkey, ptszSubKey,
		0, tstrClass.get(), 0, 0, 0, &hkey_, &dw);
#else // _WIN32_WCE
	LONG nRet = ::RegCreateKeyEx(hkey, ptszSubKey, 0, tstrClass.get(),
		REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, 0, &hkey_, &dw);
#endif // _WIN32_WCE
	if (nRet != ERROR_SUCCESS)
		hkey_ = 0;
}
