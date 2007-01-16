/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __MACRO_H__
#define __MACRO_H__

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
	if (false) { \
	} \

#define END_COCLASS_MAP() \
	else { \
		return CLASS_E_CLASSNOTAVAILABLE; \
	} \
	return S_OK; \

#define COCLASS_ENTRY(clsid, classname) \
	if (rclsid == clsid) { \
		std::auto_ptr<Object<ClassFactoryImpl<classname> > > pClassFactory( \
			new Object<ClassFactoryImpl<classname> >(); \
		HRESULT hr = pClassFactory->QueryInterface(riid, ppv); \
		if (FAILED(hr)) \
			return hr; \
		pClassFactory.release(); \
	} \

#endif // __MACRO_H__
