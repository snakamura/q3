/*
 * $Id: qscrypto.inl,v 1.1.1.1 2003/04/29 08:07:34 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QSCRYPTO_INL__
#define __QSCRYPTO_INL__


/****************************************************************************
 *
 * CryptoUtil
 *
 */

template<class T>
qs::QSTATUS qs::CryptoUtil<T>::getInstance(std::auto_ptr<T>* pap)
{
	assert(pap);
	
	DECLARE_QSTATUS();
	
	T* p = 0;
	status = T::getInstance(&p);
	CHECK_QSTATUS();
	pap->reset(p);
	
	return QSTATUS_SUCCESS;
}

template<class T>
qs::QSTATUS qs::CryptoUtil<T>::getInstance(
	const WCHAR* pwszName, std::auto_ptr<T>* pap)
{
	assert(pap);
	
	DECLARE_QSTATUS();
	
	T* p = 0;
	status = T::getInstance(pwszName, &p);
	CHECK_QSTATUS();
	pap->reset(p);
	
	return QSTATUS_SUCCESS;
}

#endif // __QSCRYPTO_INL__
