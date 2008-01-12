/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#ifndef __ACTIONINVOKER_INL__
#define __ACTIONINVOKER_INL__

#include <qmaction.h>


/****************************************************************************
 *
 * ActionInvokeHelper
 *
 */

template<class T>
qmscript::ActionInvokeHelper<T>::ActionInvokeHelper()
{
}

template<class T>
qmscript::ActionInvokeHelper<T>::~ActionInvokeHelper()
{
}

template<class T>
STDMETHODIMP qmscript::ActionInvokeHelper<T>::invokeAction(BSTR bstrAction,
														   VARIANT arg1,
														   VARIANT arg2,
														   VARIANT arg3)
{
	VARIANT* pvarArgs[] = { &arg1, &arg2, &arg3 };
	const WCHAR* pwszParam[countof(pvarArgs)] = { 0 };
	Variant params[countof(pvarArgs)];
	size_t nArg = 0;
	for (nArg = 0; nArg < countof(pvarArgs); ++nArg) {
		if (pvarArgs[nArg]->vt == VT_ERROR)
			break;
		
		HRESULT hr = ::VariantChangeType(&params[nArg],
			pvarArgs[nArg], VARIANT_ALPHABOOL, VT_BSTR);
		if (hr != S_OK)
			return hr;
		
		pwszParam[nArg] = params[nArg].bstrVal;
	}
	
	const qm::ActionInvoker* pInvoker = getActionInvoker();
	pInvoker->invoke(bstrAction, pwszParam, nArg);
	
	return S_OK;
}

#endif // __ACTIONINVOKER_INL__
