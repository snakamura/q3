/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmaccount.h>
#include <qmdocument.h>
#include <qmmessage.h>
#include <qmmessageholder.h>
#include <qmmessageoperation.h>
#include <qsosutil.h>

#include <qsassert.h>
#include <qsconv.h>
#include <qsstl.h>
#include <qsthread.h>

#include <cstdio>

#include <shlobj.h>
#include <tchar.h>

#include "dataobject.h"
#include "uri.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * MessageDataObject
 *
 */

UINT qm::MessageDataObject::nFormats__[] = {
	::RegisterClipboardFormat(_T("QmMessageDataAccount")),
	::RegisterClipboardFormat(_T("QmMessageDataMessageHolderList")),
	::RegisterClipboardFormat(_T("QmMessageDataFlag")),
#ifndef _WIN32_WCE
	::RegisterClipboardFormat(CFSTR_FILEDESCRIPTOR),
	::RegisterClipboardFormat(CFSTR_FILECONTENTS)
#endif
};

FORMATETC qm::MessageDataObject::formats__[] = {
	{
		MessageDataObject::nFormats__[FORMAT_ACCOUNT],
		0,
		DVASPECT_CONTENT,
		-1,
		TYMED_HGLOBAL
	},
	{
		MessageDataObject::nFormats__[FORMAT_MESSAGEHOLDERLIST],
		0,
		DVASPECT_CONTENT,
		-1,
		TYMED_HGLOBAL
	},
	{
		MessageDataObject::nFormats__[FORMAT_FLAG],
		0,
		DVASPECT_CONTENT,
		-1,
		TYMED_HGLOBAL
	},
#ifndef _WIN32_WCE
	{
		MessageDataObject::nFormats__[FORMAT_FILEDESCRIPTOR],
		0,
		DVASPECT_CONTENT,
		-1,
		TYMED_HGLOBAL
	},
	{
		MessageDataObject::nFormats__[FORMAT_FILECONTENTS],
		0,
		DVASPECT_CONTENT,
		-1,
		TYMED_HGLOBAL
	}
#endif
};

qm::MessageDataObject::MessageDataObject(Document* pDocument) :
	nRef_(0),
	pDocument_(pDocument),
	pAccount_(0),
	flag_(FLAG_NONE)
{
	assert(pDocument);
}

qm::MessageDataObject::MessageDataObject(Document* pDocument,
										 Account* pAccount,
										 const MessageHolderList& l,
										 Flag flag) :
	nRef_(0),
	pDocument_(pDocument),
	pAccount_(pAccount),
	flag_(flag)
{
	assert(pDocument);
	assert(pAccount);
	assert(pAccount->isLocked());
	assert(!l.empty());
	
	listMessagePtr_.assign(l.begin(), l.end());
}

qm::MessageDataObject::~MessageDataObject()
{
}

STDMETHODIMP_(ULONG) qm::MessageDataObject::AddRef()
{
	return ::InterlockedIncrement(reinterpret_cast<LONG*>(&nRef_));
}

STDMETHODIMP_(ULONG) qm::MessageDataObject::Release()
{
	ULONG nRef = ::InterlockedDecrement(reinterpret_cast<LONG*>(&nRef_));
	if (nRef == 0)
		delete this;
	return nRef;
}

STDMETHODIMP qm::MessageDataObject::QueryInterface(REFIID riid,
												   void** ppv)
{
	*ppv = 0;
	
	if (riid == IID_IUnknown || riid == IID_IDataObject) {
		AddRef();
		*ppv = static_cast<IDataObject*>(this);
	}
	
	return *ppv ? S_OK : E_NOINTERFACE;
}

STDMETHODIMP qm::MessageDataObject::GetData(FORMATETC* pFormat,
											STGMEDIUM* pMedium)
{
	HRESULT hr = QueryGetData(pFormat);
	if (hr != S_OK)
		return hr;
	
	HGLOBAL hGlobal = 0;
	if (pFormat->cfFormat == nFormats__[FORMAT_ACCOUNT]) {
		const WCHAR* pwszName = pAccount_->getName();
		hGlobal = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT,
			(wcslen(pwszName) + 1)*sizeof(WCHAR));
		if (!hGlobal)
			return E_OUTOFMEMORY;
		
		WCHAR* p = reinterpret_cast<WCHAR*>(GlobalLock(hGlobal));
		wcscpy(p, pwszName);
		GlobalUnlock(hGlobal);
	}
	else if (pFormat->cfFormat == nFormats__[FORMAT_MESSAGEHOLDERLIST]) {
		typedef std::vector<WCHAR> Buffer;
		Buffer buf;
		for (MessagePtrList::const_iterator it = listMessagePtr_.begin(); it != listMessagePtr_.end(); ++it) {
			MessagePtrLock mpl(*it);
			if (mpl) {
				wstring_ptr wstrURI(URI::getURI(mpl));
				size_t nLen = wcslen(wstrURI.get());
				Buffer::size_type n = buf.size();
				buf.resize(buf.size() + nLen + 1);
				std::copy(wstrURI.get(), wstrURI.get() + nLen + 1, buf.begin() + n);
			}
		}
		buf.push_back(L'\0');
		
		hGlobal = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT,
			buf.size()*sizeof(WCHAR));
		if (!hGlobal)
			return E_OUTOFMEMORY;
		WCHAR* p = reinterpret_cast<WCHAR*>(GlobalLock(hGlobal));
		memcpy(p, &buf[0], buf.size()*sizeof(WCHAR));
		GlobalUnlock(hGlobal);
	}
	else if (pFormat->cfFormat == nFormats__[FORMAT_FLAG]) {
		hGlobal = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, sizeof(Flag));
		if (!hGlobal)
			return E_OUTOFMEMORY;
		
		Flag* pFlag = reinterpret_cast<Flag*>(GlobalLock(hGlobal));
		*pFlag = flag_;
		GlobalUnlock(hGlobal);
	}
#ifndef _WIN32_WCE
	else if (pFormat->cfFormat == nFormats__[FORMAT_FILECONTENTS]) {
		if (pFormat->lindex < 0 ||
			static_cast<LONG>(listMessagePtr_.size()) <= pFormat->lindex)
			return E_FAIL;
		
		MessagePtr ptr = listMessagePtr_[pFormat->lindex];
		MessagePtrLock mpl(ptr);
		if (!mpl)
			return E_FAIL;
		
		Message msg;
		unsigned int nFlags = Account::GETMESSAGEFLAG_ALL |
			Account::GETMESSAGEFLAG_NOSECURITY;
		if (!mpl->getMessage(nFlags, 0, &msg))
			return E_FAIL;
		xstring_ptr strContent(msg.getContent());
		if (!strContent.get())
			return E_FAIL;
		
		size_t nLen = strlen(strContent.get());
		hGlobal = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, nLen);
		if (!hGlobal)
			return E_OUTOFMEMORY;
		CHAR* p = reinterpret_cast<CHAR*>(GlobalLock(hGlobal));
		memcpy(p, strContent.get(), nLen);
		GlobalUnlock(hGlobal);
	}
	else if (pFormat->cfFormat == nFormats__[FORMAT_FILEDESCRIPTOR]) {
		hGlobal = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT,
			sizeof(FILEGROUPDESCRIPTOR) +
			(listMessagePtr_.size() - 1)*sizeof(FILEDESCRIPTOR));
		if (!hGlobal)
			return E_OUTOFMEMORY;
		FILEGROUPDESCRIPTOR* pfgd =
			reinterpret_cast<FILEGROUPDESCRIPTOR*>(GlobalLock(hGlobal));
		pfgd->cItems = listMessagePtr_.size();
		MessagePtrList::size_type n = 0;
		while (n < listMessagePtr_.size()) {
			MessagePtrLock mpl(listMessagePtr_[n]);
			if (!mpl)
				break;
			pfgd->fgd[n].dwFlags = FD_WRITESTIME;
			
			Time time;
			mpl->getDate(&time);
			::SystemTimeToFileTime(&time, &pfgd->fgd[n].ftLastWriteTime);
			
			wstring_ptr wstrSubject(mpl->getSubject());
			wstring_ptr wstrName(getFileName(wstrSubject.get()));
			W2T(wstrName.get(), ptszName);
			_tcsncpy(pfgd->fgd[n].cFileName, ptszName, MAX_PATH - 1);
			
			++n;
		}
		GlobalUnlock(hGlobal);
		
		if (n != listMessagePtr_.size()) {
			GlobalFree(hGlobal);
			return E_FAIL;
		}
	}
#endif
	else {
		assert(false);
	}
	
	pMedium->tymed = TYMED_HGLOBAL;
	pMedium->hGlobal = hGlobal;
	pMedium->pUnkForRelease = 0;
	
	return S_OK;
}

STDMETHODIMP qm::MessageDataObject::GetDataHere(FORMATETC* pFormat,
												STGMEDIUM* pMedium)
{
	return E_NOTIMPL;
}

STDMETHODIMP qm::MessageDataObject::QueryGetData(FORMATETC* pFormat)
{
	int n = 0;
	while (n < countof(nFormats__)) {
		if (pFormat->cfFormat == nFormats__[n])
			break;
		++n;
	}
	if (n == countof(nFormats__) || pFormat->ptd)
		return DV_E_FORMATETC;
#ifdef _WIN32_WCE
	else if (pFormat->lindex != -1)
		return DV_E_LINDEX;
#else
	else if (pFormat->cfFormat != nFormats__[FORMAT_FILECONTENTS] && pFormat->lindex != -1)
		return DV_E_LINDEX;
	else if (pFormat->cfFormat == nFormats__[FORMAT_FILECONTENTS] &&
		(pFormat->lindex < 0 || static_cast<LONG>(listMessagePtr_.size()) <= pFormat->lindex))
		return DV_E_LINDEX;
#endif
	else if (!(pFormat->tymed & TYMED_HGLOBAL))
		return DV_E_TYMED;
	else if (pFormat->dwAspect != DVASPECT_CONTENT)
		return DV_E_DVASPECT;
	else
		return S_OK;
}

STDMETHODIMP qm::MessageDataObject::GetCanonicalFormatEtc(FORMATETC* pFormatIn,
														  FORMATETC* pFormatOut)
{
	return E_NOTIMPL;
}

STDMETHODIMP qm::MessageDataObject::SetData(FORMATETC* pFormat,
											STGMEDIUM* pMedium,
											BOOL bRelease)
{
	if (pFormat->tymed != TYMED_HGLOBAL)
		return DV_E_TYMED;
	
	void* pData = GlobalLock(pMedium->hGlobal);
	struct Deleter
	{
		Deleter(HGLOBAL hGlobal) : hGlobal_(hGlobal) {}
		~Deleter() { GlobalUnlock(hGlobal_); }
		HGLOBAL hGlobal_;
	} deleter(pMedium->hGlobal);
	
	if (pFormat->cfFormat == nFormats__[FORMAT_ACCOUNT]) {
		pAccount_ = pDocument_->getAccount(static_cast<WCHAR*>(pData));
	}
	else if (pFormat->cfFormat == nFormats__[FORMAT_MESSAGEHOLDERLIST]) {
		const WCHAR* p = static_cast<const WCHAR*>(pData);
		while (*p) {
			MessagePtr ptr;
			if (!URI::getMessageHolder(p, pDocument_, &ptr))
				return E_FAIL;
			listMessagePtr_.push_back(ptr);
			p += wcslen(p) + 1;
		}
	}
	else if (pFormat->cfFormat == nFormats__[FORMAT_FLAG]) {
		flag_ = *static_cast<Flag*>(pData);
	}
	else {
		return DV_E_FORMATETC;
	}
	
	return S_OK;
}

STDMETHODIMP qm::MessageDataObject::EnumFormatEtc(DWORD dwDirection,
												  IEnumFORMATETC** ppEnum)
{
	if (dwDirection != DATADIR_GET)
		return E_NOTIMPL;
	
	IEnumFORMATETCImpl* pEnum = new IEnumFORMATETCImpl();
	pEnum->AddRef();
	
	*ppEnum = pEnum;
	
	return S_OK;
}

STDMETHODIMP qm::MessageDataObject::DAdvise(FORMATETC* pFormat,
											DWORD advf,
											IAdviseSink* pSink,
											DWORD* pdwConnection)
{
	return E_NOTIMPL;
}

STDMETHODIMP qm::MessageDataObject::DUnadvise(DWORD dwConnection)
{
	return E_NOTIMPL;
}

STDMETHODIMP qm::MessageDataObject::EnumDAdvise(IEnumSTATDATA** ppEnum)
{
	return E_NOTIMPL;
}

bool qm::MessageDataObject::setClipboard(IDataObject* pDataObject)
{
	assert(pDataObject);
	
#ifdef _WIN32_WCE
	Clipboard clipboard(0);
	if (!clipboard)
		return false;
	if (!clipboard.empty())
		return false;
	
	for (int n = 0; n < countof(formats__); ++n) {
		FORMATETC etc = formats__[n];
		StgMedium medium;
		HRESULT hr = pDataObject->GetData(&etc, &medium);
		if (hr != S_OK)
			return false;
		if (!clipboard.setData(etc.cfFormat, medium.hGlobal))
			return false;
	}
#else
	HRESULT hr = ::OleSetClipboard(pDataObject);
	if (hr != S_OK)
		return false;
#endif
	
	return true;
}

IDataObject* qm::MessageDataObject::getClipboard(Document* pDocument)
{
	assert(pDocument);
	
#ifdef _WIN32_WCE
	std::auto_ptr<MessageDataObject> pDataObject(new MessageDataObject(pDocument));
	
	Clipboard clipboard(0);
	if (!clipboard)
		return 0;
	
	for (int n = 0; n < countof(formats__); ++n) {
		FORMATETC etc = formats__[n];
		STGMEDIUM medium;
		medium.tymed = TYMED_HGLOBAL;
		medium.hGlobal = clipboard.getData(etc.cfFormat);
		if (!medium.hGlobal)
			return 0;
		HRESULT hr = pDataObject->SetData(&etc, &medium, FALSE);
		if (hr != S_OK)
			return 0;
	}
	
	return pDataObject.release();
#else
	IDataObject* pDataObject = 0;
	HRESULT hr = ::OleGetClipboard(&pDataObject);
	if (hr != S_OK)
		return 0;
	return pDataObject;
#endif
}

bool qm::MessageDataObject::queryClipboard()
{
#ifdef _WIN32_WCE
	return Clipboard::isFormatAvailable(nFormats__[FORMAT_MESSAGEHOLDERLIST]);
#else
	ComPtr<IDataObject> pDataObject;
	HRESULT hr = ::OleGetClipboard(&pDataObject);
	if (hr != S_OK)
		return false;
	return canPasteMessage(pDataObject.get());
#endif
}

bool qm::MessageDataObject::pasteMessages(IDataObject* pDataObject,
										  Document* pDocument,
										  NormalFolder* pFolderTo,
										  Flag flag,
										  MessageOperationCallback* pCallback)
{
	assert(pDataObject);
	assert(pDocument);
	assert(pFolderTo);
	
	HRESULT hr = S_OK;
	
	if (flag == FLAG_NONE)
		flag = getPasteFlag(pDataObject, pDocument, pFolderTo);
	
	FORMATETC fe = formats__[FORMAT_MESSAGEHOLDERLIST];
	STGMEDIUM stm;
	hr = pDataObject->GetData(&fe, &stm);
	if (hr == S_OK) {
		struct Deleter
		{
			Deleter(STGMEDIUM& stm) : stm_(stm) {}
			~Deleter()
			{
				GlobalUnlock(stm_.hGlobal);
				::ReleaseStgMedium(&stm_);
			}
			STGMEDIUM& stm_;
		} deleter(stm);
		
		const WCHAR* p = reinterpret_cast<const WCHAR*>(GlobalLock(stm.hGlobal));
		MessagePtrList listMessagePtr;
		while (*p) {
			MessagePtr ptr;
			if (!URI::getMessageHolder(p, pDocument, &ptr))
				return false;
			listMessagePtr.push_back(ptr);
			p += wcslen(p) + 1;
		}
		
		if (!listMessagePtr.empty()) {
			if (pCallback)
				pCallback->setCount(listMessagePtr.size());
			
			while (true) {
				Account* pAccount = 0;
				AccountLock lock;
				MessageHolderList l;
				
				for (MessagePtrList::iterator it = listMessagePtr.begin(); it != listMessagePtr.end(); ++it) {
					MessagePtrLock mpl(*it);
					if (mpl) {
						if (!pAccount) {
							pAccount = mpl->getAccount();
							lock.set(pAccount);
						}
						if (mpl->getAccount() == pAccount) {
							l.push_back(mpl);
							*it = MessagePtr();
						}
					}
				}
				if (!pAccount)
					break;
				
				assert(!l.empty());
				
				if (!pAccount->copyMessages(l, pFolderTo, flag == FLAG_MOVE, pCallback))
					return false;
				
				if (pCallback && pCallback->isCanceled())
					break;
			}
		}
	}
	
	return true;
}

bool qm::MessageDataObject::canPasteMessage(IDataObject* pDataObject)
{
	assert(pDataObject);
	
	FORMATETC fe = formats__[FORMAT_MESSAGEHOLDERLIST];
	return pDataObject->QueryGetData(&fe) == S_OK;
}

MessageDataObject::Flag qm::MessageDataObject::getPasteFlag(IDataObject* pDataObject,
															Document* pDocument,
															NormalFolder* pFolder)
{
	assert(pDataObject);
	
	Flag flag = FLAG_NONE;
	
	FORMATETC fe = formats__[FORMAT_FLAG];
	STGMEDIUM stm;
	HRESULT hr = pDataObject->GetData(&fe, &stm);
	if (hr == S_OK) {
		flag = *reinterpret_cast<Flag*>(GlobalLock(stm.hGlobal));
		GlobalUnlock(stm.hGlobal);
		::ReleaseStgMedium(&stm);
	}
	
	if (flag == FLAG_NONE) {
		Account* pAccount = 0;
		FORMATETC fe = formats__[FORMAT_ACCOUNT];
		STGMEDIUM stm;
		hr = pDataObject->GetData(&fe, &stm);
		if (hr == S_OK) {
			const WCHAR* pwszName = reinterpret_cast<const WCHAR*>(
				GlobalLock(stm.hGlobal));
			pAccount = pDocument->getAccount(pwszName);
			GlobalUnlock(stm.hGlobal);
			::ReleaseStgMedium(&stm);
		}
		flag = pFolder->getAccount() != pAccount ? FLAG_COPY : FLAG_MOVE;
	}
	
	return flag;
}

wstring_ptr qm::MessageDataObject::getFileName(const WCHAR* pwszName)
{
	assert(pwszName);
	
	size_t nLen = wcslen(pwszName);
	if (nLen + 4 >= MAX_PATH)
		nLen = MAX_PATH - 5;
	
	wstring_ptr wstrName(concat(pwszName, nLen, L".eml", 4));
	
	const WCHAR* pwszEscape = L"\\/:*?\"<>|";
	for (WCHAR* p = wstrName.get(); *p; ++p) {
		if (wcschr(pwszEscape, *p))
			*p = L'_';
	}
	
	return wstrName;
}


/****************************************************************************
 *
 * MessageDataObject::IEnumFORMATETCImpl
 *
 */

qm::MessageDataObject::IEnumFORMATETCImpl::IEnumFORMATETCImpl() :
	nRef_(0),
	nCurrent_(0)
{
}

qm::MessageDataObject::IEnumFORMATETCImpl::~IEnumFORMATETCImpl()
{
}

STDMETHODIMP_(ULONG) qm::MessageDataObject::IEnumFORMATETCImpl::AddRef()
{
	return ::InterlockedIncrement(reinterpret_cast<LONG*>(&nRef_));
}

STDMETHODIMP_(ULONG) qm::MessageDataObject::IEnumFORMATETCImpl::Release()
{
	ULONG nRef = ::InterlockedDecrement(reinterpret_cast<LONG*>(&nRef_));
	if (nRef == 0)
		delete this;
	return nRef;
}

STDMETHODIMP qm::MessageDataObject::IEnumFORMATETCImpl::QueryInterface(REFIID riid,
																	   void** ppv)
{
	*ppv = 0;
	
	if (riid == IID_IUnknown || riid == IID_IEnumFORMATETC) {
		AddRef();
		*ppv = static_cast<IEnumFORMATETC*>(this);
	}
	
	return *ppv ? S_OK : E_NOINTERFACE;
}

STDMETHODIMP qm::MessageDataObject::IEnumFORMATETCImpl::Next(ULONG celt,
															 FORMATETC* rgelt,
															 ULONG* pceltFetched)
{
	int nCount = 0;
	while (celt > 0 && nCurrent_ < countof(MessageDataObject::formats__)) {
		rgelt[nCount++] = MessageDataObject::formats__[nCurrent_++];
		--celt;
	}
	if (pceltFetched)
		*pceltFetched = nCount;
	
	return nCount != 0 ? S_OK : S_FALSE;
}

STDMETHODIMP qm::MessageDataObject::IEnumFORMATETCImpl::Skip(ULONG celt)
{
	nCurrent_ += celt;
	return nCurrent_ > countof(MessageDataObject::formats__) ? S_FALSE : S_OK;
}

STDMETHODIMP qm::MessageDataObject::IEnumFORMATETCImpl::Reset()
{
	nCurrent_ = 0;
	return S_OK;
}

STDMETHODIMP qm::MessageDataObject::IEnumFORMATETCImpl::Clone(IEnumFORMATETC** ppEnum)
{
	IEnumFORMATETCImpl* pEnum = new IEnumFORMATETCImpl();
	pEnum->nCurrent_ = nCurrent_;
	pEnum->AddRef();
	
	*ppEnum = pEnum;
	
	return S_OK;
}
