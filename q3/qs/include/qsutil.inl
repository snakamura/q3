/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QSUTIL_INL__
#define __QSUTIL_INL__


/****************************************************************************
 *
 * TemplateObserver
 *
 */

template<class ObservableImpl, class Param>
qs::TemplateObserver<ObservableImpl, Param>::~TemplateObserver()
{
}

template<class ObservableImpl, class Param>
void qs::TemplateObserver<ObservableImpl, Param>::onUpdate(Observable* pObservable,
														   void* pParam)
{
	onUpdate(static_cast<ObservableImpl*>(pObservable),
		static_cast<Param*>(pParam));
}


#endif // __QSUTIL_INL__
