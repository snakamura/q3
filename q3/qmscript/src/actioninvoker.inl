/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
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
	size_t nArg = 0;
	for (nArg = 0; nArg < countof(pvarArgs); ++nArg) {
		if (pvarArgs[nArg]->vt == VT_ERROR)
			break;
	}
	
	const qm::ActionInvoker* pInvoker = getActionInvoker();
	pInvoker->invoke(bstrAction, pvarArgs, nArg);
	
	return S_OK;
}

#endif // __ACTIONINVOKER_INL__
