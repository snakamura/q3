/*
 * $Id: actioninvoker.inl,v 1.1 2003/05/14 08:52:16 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
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
qmscript::ActionInvokeHelper<T>::ActionInvokeHelper(qs::QSTATUS* pstatus) :
	T(pstatus)
{
}

template<class T>
qmscript::ActionInvokeHelper<T>::~ActionInvokeHelper()
{
}

template<class T>
STDMETHODIMP qmscript::ActionInvokeHelper<T>::invokeAction(
	BSTR bstrAction, VARIANT arg1, VARIANT arg2, VARIANT arg3)
{
	DECLARE_QSTATUS();
	
	VARIANT* pvarArgs[3] = { &arg1, &arg2, &arg3 };
	size_t nArg = 0;
	for (nArg = 0; nArg < countof(pvarArgs); ++nArg) {
		if (pvarArgs[nArg]->vt == VT_ERROR)
			break;
	}
	
	const qm::ActionInvoker* pInvoker = getActionInvoker();
	status = pInvoker->invoke(bstrAction, pvarArgs, nArg);
	CHECK_QSTATUS_HRESULT();
	
	return S_OK;
}

#endif // __ACTIONINVOKER_INL__
