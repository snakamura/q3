/*
 * $Id: macro.h,v 1.1.1.1 2003/04/29 08:07:34 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __MACRO_H__
#define __MACRO_H__

#define CHECK_HRESULT() \
	if (FAILED(hr)) \
		return qs::QSTATUS_FAIL \

#define CHECK_QSTATUS_HRESULT() \
	if (status == qs::QSTATUS_OUTOFMEMORY) \
		return E_OUTOFMEMORY; \
	else if (status != qs::QSTATUS_SUCCESS) \
		return E_FAIL \

#define BEGIN_INTERFACE_MAP() \
	if (false) \
		; \

#define END_INTERFACE_MAP() \
	else \
		return E_NOINTERFACE; \
	AddRef(); \
	return S_OK;

#define INTERFACE_ENTRY(iid, i) \
	else if (riid == iid) \
		*ppv = static_cast<i*>(this); \

#define BEGIN_COCLASS_MAP() \
	DECLARE_QSTATUS(); \
	if (false) { \
	} \

#define END_COCLASS_MAP() \
	else { \
		return CLASS_E_CLASSNOTAVAILABLE; \
	} \
	return S_OK; \

#define COCLASS_ENTRY(clsid, classname) \
	if (rclsid == clsid) { \
		std::auto_ptr<Object<ClassFactoryImpl<classname> > > pClassFactory; \
		status = newQsObject(&pClassFactory); \
		CHECK_QSTATUS_HRESULT(); \
		HRESULT hr = pClassFactory->QueryInterface(riid, ppv); \
		if (FAILED(hr)) \
			return hr; \
		pClassFactory.release(); \
	} \

#endif // __MACRO_H__
