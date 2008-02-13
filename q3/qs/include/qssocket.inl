/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */


/****************************************************************************
 *
 * FilterSocketCallback
 *
 */

template<class T>
qs::FilterSocketCallback<T>::FilterSocketCallback(SocketCallback* pCallback) :
	pCallback_(pCallback)
{
}

template<class T>
qs::FilterSocketCallback<T>::~FilterSocketCallback()
{
}

template<class T>
bool qs::FilterSocketCallback<T>::isCanceled(bool bForce) const
{
	return pCallback_->isCanceled(bForce);
}

template<class T>
void qs::FilterSocketCallback<T>::initialize()
{
	pCallback_->initialize();
}

template<class T>
void qs::FilterSocketCallback<T>::lookup()
{
	pCallback_->lookup();
}

template<class T>
void qs::FilterSocketCallback<T>::connecting()
{
	pCallback_->connecting();
}

template<class T>
void qs::FilterSocketCallback<T>::connected()
{
	pCallback_->connected();
}

template<class T>
void qs::FilterSocketCallback<T>::error(SocketBase::Error error,
										const WCHAR* pwszMessage)
{
	pCallback_->error(error, pwszMessage);
}
