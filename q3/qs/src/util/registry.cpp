/*
 * $Id: registry.cpp,v 1.1.1.1 2003/04/29 08:07:37 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsosutil.h>
#include <qserror.h>
#include <qsconv.h>

#include <tchar.h>

using namespace qs;


/****************************************************************************
 *
 * Registry
 *
 */

qs::Registry::Registry(HKEY hkey, const WCHAR* pwszSubKey, QSTATUS* pstatus) :
	hkey_(0)
{
	assert(pstatus);
	*pstatus = init(hkey, pwszSubKey, 0);
}

qs::Registry::Registry(HKEY hkey, const WCHAR* pwszSubKey,
	const WCHAR* pwszClass, QSTATUS* pstatus) :
	hkey_(0)
{
	assert(pstatus);
	*pstatus = init(hkey, pwszSubKey, pwszClass);
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

QSTATUS qs::Registry::getValue(const WCHAR* pwszName, DWORD* pdwValue)
{
	LONG nRet = ERROR_SUCCESS;
	return getValue(pwszName, pdwValue, &nRet);
}

QSTATUS qs::Registry::getValue(const WCHAR* pwszName, DWORD* pdwValue, LONG* pnRet)
{
	assert(pdwValue);
	assert(pnRet);
	
	if (!hkey_)
		return QSTATUS_FAIL;
	
	W2T(pwszName, ptszName);
	
	DWORD dwSize = sizeof(DWORD);
	DWORD dwType = REG_DWORD;
	*pnRet = ::RegQueryValueEx(hkey_, ptszName, 0,
		&dwType, reinterpret_cast<LPBYTE>(pdwValue), &dwSize);
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::Registry::getValue(const WCHAR* pwszName, WSTRING* pwstrValue)
{
	LONG nRet = ERROR_SUCCESS;
	return getValue(pwszName, pwstrValue, &nRet);
}

QSTATUS qs::Registry::getValue(const WCHAR* pwszName,
	WSTRING* pwstrValue, LONG* pnRet)
{
	assert(pwstrValue);
	assert(pnRet);
	
	if (!hkey_)
		return QSTATUS_FAIL;
	
	W2T(pwszName, ptszName);
	
	DWORD dwType = REG_SZ;
	DWORD dwSize = 0;
	*pnRet = ::RegQueryValueEx(hkey_, ptszName, 0, &dwType, 0, &dwSize);
	if (*pnRet != ERROR_SUCCESS)
		return QSTATUS_SUCCESS;
	string_ptr<TSTRING> tstrValue(allocTString(dwSize + 1));
	*pnRet = ::RegQueryValueEx(hkey_, ptszName, 0,
		&dwType, reinterpret_cast<LPBYTE>(tstrValue.get()), &dwSize);
	if (*pnRet != ERROR_SUCCESS)
		return QSTATUS_SUCCESS;
	*pwstrValue = tcs2wcs(tstrValue.get());
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::Registry::getValue(const WCHAR* pwszName, BYTE* pByte, int* pnSize)
{
	LONG nRet = ERROR_SUCCESS;
	return getValue(pwszName, pByte, pnSize, &nRet);
}

QSTATUS qs::Registry::getValue(const WCHAR* pwszName,
	BYTE* pByte, int* pnSize, LONG* pnRet)
{
	assert(pByte);
	assert(pnSize);
	assert(pnRet);
	
	if (!hkey_)
		return QSTATUS_FAIL;
	
	W2T(pwszName, ptszName);
	
	DWORD dwSize = *pnSize;
	DWORD dwType = REG_BINARY;
	*pnRet = ::RegQueryValueEx(hkey_, ptszName, 0, &dwType, pByte, &dwSize);
	*pnSize = dwSize;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::Registry::setValue(const WCHAR* pwszName, DWORD dwValue)
{
	LONG nRet = ERROR_SUCCESS;
	return setValue(pwszName, dwValue, &nRet);
}

QSTATUS qs::Registry::setValue(const WCHAR* pwszName, DWORD dwValue, LONG* pnRet)
{
	assert(pnRet);
	
	if (!hkey_)
		return QSTATUS_FAIL;
	
	W2T(pwszName, ptszName);
	*pnRet = ::RegSetValueEx(hkey_, ptszName, 0, REG_DWORD,
		reinterpret_cast<const BYTE*>(&dwValue), sizeof(DWORD));
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::Registry::setValue(const WCHAR* pwszName, const WCHAR* pwszValue)
{
	LONG nRet = ERROR_SUCCESS;
	return setValue(pwszName, pwszValue, &nRet);
}

QSTATUS qs::Registry::setValue(const WCHAR* pwszName,
	const WCHAR* pwszValue, LONG* pnRet)
{
	assert(pnRet);
	
	if (!hkey_)
		return QSTATUS_FAIL;
	
	W2T(pwszName, ptszName);
	W2T(pwszValue, ptszValue);
	*pnRet = ::RegSetValueEx(hkey_, ptszName, 0, REG_SZ,
		reinterpret_cast<const BYTE*>(ptszValue),
		(_tcslen(ptszValue) + 1)*sizeof(TCHAR));
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::Registry::setValue(const WCHAR* pwszName, const BYTE* pByte, int nSize)
{
	LONG nRet = ERROR_SUCCESS;
	return setValue(pwszName, pByte, nSize, &nRet);
}

QSTATUS qs::Registry::setValue(const WCHAR* pwszName,
	const BYTE* pByte, int nSize, LONG* pnRet)
{
	assert(pnRet);
	
	if (!hkey_)
		return QSTATUS_FAIL;
	
	W2T(pwszName, ptszName);
	*pnRet = ::RegSetValueEx(hkey_, ptszName, 0, REG_BINARY, pByte, nSize);
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::Registry::deleteKey(HKEY hkey,
	const WCHAR* pwszSubKey, LONG* pnRet)
{
	assert(hkey);
	assert(pnRet);
	
	W2T(pwszSubKey, ptszSubKey);
	*pnRet = ::RegDeleteKey(hkey, ptszSubKey);
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::Registry::init(HKEY hkey, const WCHAR* pwszSubKey, const WCHAR* pwszClass)
{
	W2T(pwszSubKey, ptszSubKey);
	
	string_ptr<TSTRING> tstrClass;
	if (pwszClass) {
		tstrClass.reset(wcs2tcs(pwszClass));
		if (!tstrClass.get())
			return QSTATUS_OUTOFMEMORY;
	}
	
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
	
	return QSTATUS_SUCCESS;
}
