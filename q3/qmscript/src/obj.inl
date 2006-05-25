/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __OBJ_INL__
#define __OBJ_INL__

#include "macro.h"
#include "main.h"


/****************************************************************************
 *
 * Object
 *
 */

template<class T>
qmscript::Object<T>::Object() :
	nRef_(0)
{
}

template<class T>
qmscript::Object<T>::~Object()
{
}

template<class T>
STDMETHODIMP qmscript::Object<T>::QueryInterface(REFIID riid,
												 void** ppv)
{
	if (riid == IID_IUnknown)
		*ppv = static_cast<IUnknown*>(this);
	else
		return internalQueryInterface(riid, ppv);
	
	AddRef();
	
	return S_OK;
}

template<class T>
STDMETHODIMP_(ULONG) qmscript::Object<T>::AddRef()
{
	return ::InterlockedIncrement(reinterpret_cast<LONG*>(&nRef_));
}

template<class T>
STDMETHODIMP_(ULONG) qmscript::Object<T>::Release()
{
	ULONG nRef = ::InterlockedDecrement(reinterpret_cast<LONG*>(&nRef_));
	if (nRef == 0)
		delete this;
	return nRef;
}


/****************************************************************************
 *
 * DispObject
 *
 */

template<class T, class I, const IID* piid>
qmscript::DispObject<T, I, piid>::DispObject() :
	pTypeInfo_(0)
{
	HRESULT hr = getTypeLib()->GetTypeInfoOfGuid(*piid, &pTypeInfo_);
	if (FAILED(hr)) {
		// TODO
	}
}

template<class T, class I, const IID* piid>
qmscript::DispObject<T, I, piid>::~DispObject()
{
	if (pTypeInfo_)
		pTypeInfo_->Release();
}

template<class T, class I, const IID* piid>
STDMETHODIMP qmscript::DispObject<T, I, piid>::QueryInterface(REFIID riid,
															  void** ppv)
{
	if (riid == IID_IDispatch)
		*ppv = static_cast<IDispatch*>(this);
	else
		return Object<T>::QueryInterface(riid, ppv);
	
	AddRef();
	
	return S_OK;
}

template<class T, class I, const IID* piid>
STDMETHODIMP qmscript::DispObject<T, I, piid>::GetIDsOfNames(REFIID riid,
															 LPOLESTR* ppwszNames,
															 unsigned int nNames,
															 LCID lcid,
															 DISPID* pDispId)
{
	return ::DispGetIDsOfNames(pTypeInfo_, ppwszNames, nNames, pDispId);
}

template<class T, class I, const IID* piid>
STDMETHODIMP qmscript::DispObject<T, I, piid>::GetTypeInfo(unsigned int nTypeInfo,
														   LCID lcid,
														   ITypeInfo** ppTypeInfo)
{
	if (nTypeInfo != 0)
		return DISP_E_BADINDEX;
	
	*ppTypeInfo = pTypeInfo_;
	(*ppTypeInfo)->AddRef();
	
	return S_OK;
}

template<class T, class I, const IID* piid>
STDMETHODIMP qmscript::DispObject<T, I, piid>::GetTypeInfoCount(unsigned int* pnCount)
{
	*pnCount = 1;
	return S_OK;
}

template<class T, class I, const IID* piid>
STDMETHODIMP qmscript::DispObject<T, I, piid>::Invoke(DISPID dispId,
													  REFIID riid,
													  LCID lcid,
													  WORD wFlags,
													  DISPPARAMS* pDispParams,
													  VARIANT* pvarResult,
													  EXCEPINFO* pExcepInfo,
													  unsigned int* pnArgErr)
{
	return ::DispInvoke(static_cast<I*>(this), pTypeInfo_, dispId,
		wFlags, pDispParams, pvarResult, pExcepInfo, pnArgErr);
}


/****************************************************************************
 *
 * ClassFactoryImpl
 *
 */

template<class T>
qmscript::ClassFactoryImpl<T>::ClassFactoryImpl()
{
}

template<class T>
qmscript::ClassFactoryImpl<T>::~ClassFactoryImpl()
{
}

template<class T>
HRESULT qmscript::ClassFactoryImpl<T>::internalQueryInterface(REFIID riid,
															  void** ppv)
{
	BEGIN_INTERFACE_MAP()
		INTERFACE_ENTRY(IID_IClassFactory, IClassFactory)
	END_INTERFACE_MAP()
}

template<class T>
STDMETHODIMP qmscript::ClassFactoryImpl<T>::CreateInstance(IUnknown* pUnkOuter,
														   REFIID riid,
														   void** ppv)
{
	if (pUnkOuter)
		return CLASS_E_NOAGGREGATION;
	
	std::auto_ptr<T> pObj(new T());
	
	HRESULT hr = pObj->QueryInterface(riid, ppv);
	if (FAILED(hr))
		return hr;
	pObj.release();
	
	return S_OK;
}

template<class T>
STDMETHODIMP qmscript::ClassFactoryImpl<T>::LockServer(BOOL bLock)
{
	return S_OK;
}


/****************************************************************************
 *
 * EnumBase
 *
 */

template<class I, const IID* piid, class T, class Traits>
qmscript::EnumBase<I, piid, T, Traits>::EnumBase() :
	n_(0)
{
}

template<class I, const IID* piid, class T, class Traits>
qmscript::EnumBase<I, piid, T, Traits>::~EnumBase()
{
	for (List::iterator it = list_.begin(); it != list_.end(); ++it)
		traits_.destroy(&(*it));
}

template<class I, const IID* piid, class T, class Traits>
bool qmscript::EnumBase<I, piid, T, Traits>::init(T* p,
												  size_t nCount)
{
	return init(p, nCount, Traits());
}

template<class I, const IID* piid, class T, class Traits>
bool qmscript::EnumBase<I, piid, T, Traits>::init(T* p,
												  size_t nCount,
												  const Traits& traits)
{
	list_.resize(nCount);
	Variant v;
	std::fill(list_.begin(), list_.end(), v);
	
	for (size_t n = 0; n < nCount; ++n) {
		if (!traits_.copy(&list_[n], &p[n]))
			return false;
	}
	
	return true;
}

template<class I, const IID* piid, class T, class Traits>
HRESULT qmscript::EnumBase<I, piid, T, Traits>::internalQueryInterface(REFIID riid,
																	   void** ppv)
{
	BEGIN_INTERFACE_MAP()
		INTERFACE_ENTRY(*piid, I)
	END_INTERFACE_MAP()
}

template<class I, const IID* piid, class T, class Traits>
STDMETHODIMP qmscript::EnumBase<I, piid, T, Traits>::Next(ULONG nElem,
														  T* p,
														  ULONG* pnFetched)
{
	HRESULT hrReturn = S_OK;
	if (n_ + nElem > list_.size()) {
		nElem = static_cast<ULONG>(list_.size() - n_);
		hrReturn = S_FALSE;
	}
	
	for (ULONG n = 0; n < nElem; ++n, ++n_) {
		if (!traits_.copy(&p[n], &list_[n_]))
			return E_FAIL;
	}
	
	if (pnFetched)
		*pnFetched = nElem;
	
	return hrReturn;
}

template<class I, const IID* piid, class T, class Traits>
STDMETHODIMP qmscript::EnumBase<I, piid, T, Traits>::Skip(ULONG nElem)
{
	if (n_ + nElem > list_.size())
		return S_FALSE;
	n_ += nElem;
	
	return S_OK;
}

template<class I, const IID* piid, class T, class Traits>
STDMETHODIMP qmscript::EnumBase<I, piid, T, Traits>::Reset()
{
	n_ = 0;
	return S_OK;
}

template<class I, const IID* piid, class T, class Traits>
STDMETHODIMP qmscript::EnumBase<I, piid, T, Traits>::Clone(I** ppEnum)
{
	std::auto_ptr<Object<EnumBase<I, piid, T, Traits> > > pEnum(
		new Object<EnumBase<I, piid, T, Traits> >());
	if (!pEnum->init(&list_[0], list_.size()))
		return E_FAIL;
	
	*ppEnum = pEnum.release();
	(*ppEnum)->AddRef();
	
	return S_OK;
}


/****************************************************************************
 *
 * EnumTraits
 *
 */

inline bool qmscript::EnumTraits<VARIANT>::init(VARIANT* p) const
{
	::VariantInit(p);
	return true;
}

inline void qmscript::EnumTraits<VARIANT>::destroy(VARIANT* p) const
{
	::VariantClear(p);
}

inline bool qmscript::EnumTraits<VARIANT>::copy(VARIANT* pTo,
												VARIANT* pFrom) const
{
	HRESULT hr = ::VariantCopy(pTo, pFrom);
	return SUCCEEDED(hr);
}

#endif // __OBJ_INL__
