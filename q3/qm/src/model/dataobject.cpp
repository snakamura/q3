/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
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
#include <qserror.h>
#include <qsnew.h>
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

qm::MessageDataObject::MessageDataObject(Document* pDocument, QSTATUS* pstatus) :
	nRef_(0),
	pDocument_(pDocument),
	pAccount_(0),
	flag_(FLAG_NONE)
{
	assert(pDocument);
}

qm::MessageDataObject::MessageDataObject(Document* pDocument,
	Account* pAccount, const MessageHolderList& l, Flag flag, QSTATUS* pstatus) :
	nRef_(0),
	pDocument_(pDocument),
	pAccount_(pAccount),
	flag_(flag)
{
	assert(pDocument);
	assert(pAccount);
	assert(!l.empty());
	assert(pstatus);
	
	DECLARE_QSTATUS();
	
	*pstatus = QSTATUS_SUCCESS;
	
	status = STLWrapper<MessagePtrList>(listMessagePtr_).resize(l.size());
	CHECK_QSTATUS_SET(pstatus);
	std::copy(l.begin(), l.end(), listMessagePtr_.begin());
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

STDMETHODIMP qm::MessageDataObject::QueryInterface(REFIID riid, void** ppv)
{
	*ppv = 0;
	
	if (riid == IID_IUnknown || riid == IID_IDataObject) {
		AddRef();
		*ppv = static_cast<IDataObject*>(this);
	}
	
	return *ppv ? S_OK : E_NOINTERFACE;
}

STDMETHODIMP qm::MessageDataObject::GetData(
	FORMATETC* pFormat, STGMEDIUM* pMedium)
{
	DECLARE_QSTATUS();
	
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
		MessagePtrList::const_iterator it = listMessagePtr_.begin();
		while (it != listMessagePtr_.end()) {
			MessagePtrLock mpl(*it);
			if (mpl) {
				string_ptr<WSTRING> wstrURI;
				status = URI::getURI(mpl, &wstrURI);
				CHECK_QSTATUS_VALUE(E_OUTOFMEMORY);
				size_t nLen = wcslen(wstrURI.get());
				status = STLWrapper<Buffer>(buf).reserve(buf.size() + nLen + 1);
				CHECK_QSTATUS_VALUE(E_OUTOFMEMORY);
				std::copy(wstrURI.get(), wstrURI.get() + nLen + 1,
					std::back_inserter(buf));
			}
			++it;
		}
		status = STLWrapper<Buffer>(buf).push_back(L'\0');
		CHECK_QSTATUS_VALUE(E_OUTOFMEMORY);
		
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
		
		Message msg(&status);
		CHECK_QSTATUS_VALUE(E_OUTOFMEMORY);
		status = mpl->getMessage(Account::GETMESSAGEFLAG_ALL, 0, &msg);
		CHECK_QSTATUS_VALUE(E_FAIL);
		string_ptr<STRING> strContent;
		status = msg.getContent(&strContent);
		CHECK_QSTATUS_VALUE(E_OUTOFMEMORY);
		
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
			if (mpl) {
				pfgd->fgd[n].dwFlags = FD_WRITESTIME;
				
				Time time;
				status = mpl->getDate(&time);
				if (status != QSTATUS_SUCCESS)
					break;
				::SystemTimeToFileTime(&time, &pfgd->fgd[n].ftLastWriteTime);
				
				string_ptr<WSTRING> wstrSubject;
				status = mpl->getSubject(&wstrSubject);
				if (status != QSTATUS_SUCCESS)
					break;
				string_ptr<WSTRING> wstrName;
				status = getFileName(wstrSubject.get(), &wstrName);
				if (status != QSTATUS_SUCCESS)
					break;
				string_ptr<TSTRING> tstrName(wcs2tcs(wstrName.get()));
				if (!tstrName.get())
					break;
				_tcsncpy(pfgd->fgd[n].cFileName, tstrName.get(), MAX_PATH - 1);
			}
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

STDMETHODIMP qm::MessageDataObject::GetDataHere(
	FORMATETC* pFormat, STGMEDIUM* pMedium)
{
	return E_NOTIMPL;
}

STDMETHODIMP qm::MessageDataObject::QueryGetData(FORMATETC* pFormat)
{
	for (int n = 0; n < countof(nFormats__); ++n) {
		if (pFormat->cfFormat == nFormats__[n])
			break;
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

STDMETHODIMP qm::MessageDataObject::GetCanonicalFormatEtc(
	FORMATETC* pFormatIn, FORMATETC* pFormatOut)
{
	return E_NOTIMPL;
}

STDMETHODIMP qm::MessageDataObject::SetData(
	FORMATETC* pFormat, STGMEDIUM* pMedium, BOOL bRelease)
{
	DECLARE_QSTATUS();
	
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
			status = URI::getMessageHolder(p, pDocument_, &ptr);
			CHECK_QSTATUS_VALUE(E_FAIL);
			status = STLWrapper<MessagePtrList>(listMessagePtr_).push_back(ptr);
			CHECK_QSTATUS_VALUE(E_OUTOFMEMORY);
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

STDMETHODIMP qm::MessageDataObject::EnumFormatEtc(
	DWORD dwDirection, IEnumFORMATETC** ppEnum)
{
	DECLARE_QSTATUS();
	
	if (dwDirection != DATADIR_GET)
		return E_NOTIMPL;
	
	IEnumFORMATETCImpl* pEnum = 0;
	status = newQsObject(&pEnum);
	if (status == QSTATUS_OUTOFMEMORY)
		return E_OUTOFMEMORY;
	pEnum->AddRef();
	
	*ppEnum = pEnum;
	
	return S_OK;
}

STDMETHODIMP qm::MessageDataObject::DAdvise(FORMATETC* pFormat,
	DWORD advf, IAdviseSink* pSink, DWORD* pdwConnection)
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

QSTATUS qm::MessageDataObject::setClipboard(IDataObject* pDataObject)
{
	assert(pDataObject);
	
	DECLARE_QSTATUS();
	
#ifdef _WIN32_WCE
	Clipboard clipboard(0, &status);
	CHECK_QSTATUS();
	status = clipboard.empty();
	CHECK_QSTATUS();
	
	for (int n = 0; n < countof(formats__); ++n) {
		FORMATETC etc = formats__[n];
		StgMedium medium;
		HRESULT hr = pDataObject->GetData(&etc, &medium);
		if (hr != S_OK)
			return QSTATUS_FAIL;
		status = clipboard.setData(etc.cfFormat, medium.hGlobal);
		CHECK_QSTATUS();
	}
#else
	HRESULT hr = ::OleSetClipboard(pDataObject);
	if (hr != S_OK)
		return QSTATUS_FAIL;
#endif
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MessageDataObject::getClipboard(
	Document* pDocument, IDataObject** ppDataObject)
{
	assert(pDocument);
	assert(ppDataObject);
	
	DECLARE_QSTATUS();
	
	*ppDataObject = 0;
	
#ifdef _WIN32_WCE
	std::auto_ptr<MessageDataObject> pDataObject;
	status = newQsObject(pDocument, &pDataObject);
	CHECK_QSTATUS();
	
	Clipboard clipboard(0, &status);
	CHECK_QSTATUS();
	
	for (int n = 0; n < countof(formats__); ++n) {
		FORMATETC etc = formats__[n];
		STGMEDIUM medium;
		status = clipboard.getData(etc.cfFormat, &medium.hGlobal);
		CHECK_QSTATUS();
		HRESULT hr = pDataObject->SetData(&etc, &medium, FALSE);
		if (hr != S_OK)
			return QSTATUS_FAIL;
	}
	
	*ppDataObject = pDataObject.release();
#else
	HRESULT hr = ::OleGetClipboard(ppDataObject);
	if (hr != S_OK)
		return QSTATUS_FAIL;
#endif
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MessageDataObject::queryClipboard(bool* pbData)
{
	assert(pbData);
	
	DECLARE_QSTATUS();
	
	*pbData = false;
	
#ifdef _WIN32_WCE
	status = Clipboard::isFormatAvailable(
		nFormats__[FORMAT_MESSAGEHOLDERLIST], pbData);
	CHECK_QSTATUS();
#else
	ComPtr<IDataObject> pDataObject;
	HRESULT hr = ::OleGetClipboard(&pDataObject);
	if (hr != S_OK)
		return QSTATUS_FAIL;
	*pbData = canPasteMessage(pDataObject.get());
#endif
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MessageDataObject::pasteMessages(IDataObject* pDataObject,
	Document* pDocument, NormalFolder* pFolderTo,
	Flag flag, MessageOperationCallback* pCallback)
{
	assert(pDataObject);
	assert(pDocument);
	assert(pFolderTo);
	
	DECLARE_QSTATUS();
	
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
		
		const WCHAR* p = reinterpret_cast<const WCHAR*>(
			GlobalLock(stm.hGlobal));
		MessagePtrList listMessagePtr;
		while (*p) {
			MessagePtr ptr;
			status = URI::getMessageHolder(p, pDocument, &ptr);
			CHECK_QSTATUS();
			status = STLWrapper<MessagePtrList>(listMessagePtr).push_back(ptr);
			CHECK_QSTATUS();
			
			p += wcslen(p) + 1;
		}
		
		if (!listMessagePtr.empty()) {
			if (pCallback) {
				status = pCallback->setCount(listMessagePtr.size());
				CHECK_QSTATUS();
			}
			
			while (true) {
				Account* pAccount = 0;
				AccountLock lock;
				MessageHolderList l;
				
				MessagePtrList::iterator it = listMessagePtr.begin();
				while (it != listMessagePtr.end()) {
					MessagePtrLock mpl(*it);
					if (mpl) {
						if (!pAccount) {
							pAccount = mpl->getAccount();
							lock.set(pAccount);
						}
						if (mpl->getAccount() == pAccount) {
							status = STLWrapper<MessageHolderList>(l).push_back(mpl);
							CHECK_QSTATUS();
							*it = MessagePtr();
						}
					}
					++it;
				}
				if (!pAccount)
					break;
				
				assert(!l.empty());
				
				status = pAccount->copyMessages(l, pFolderTo,
					flag == FLAG_MOVE, pCallback);
				CHECK_QSTATUS();
				
				if (pCallback && pCallback->isCanceled())
					break;
			}
		}
	}
	
	return QSTATUS_SUCCESS;
}

bool qm::MessageDataObject::canPasteMessage(IDataObject* pDataObject)
{
	assert(pDataObject);
	
	FORMATETC fe = formats__[FORMAT_MESSAGEHOLDERLIST];
	return pDataObject->QueryGetData(&fe) == S_OK;
}

MessageDataObject::Flag qm::MessageDataObject::getPasteFlag(
	IDataObject* pDataObject, Document* pDocument, NormalFolder* pFolder)
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

QSTATUS qm::MessageDataObject::getFileName(
	const WCHAR* pwszName, qs::WSTRING* pwstrName)
{
	assert(pwszName);
	assert(pwstrName);
	
	*pwstrName = 0;
	
	size_t nLen = wcslen(pwszName);
	if (nLen + 4 >= MAX_PATH)
		nLen = MAX_PATH - 5;
	
	string_ptr<WSTRING> wstrName(concat(pwszName, nLen, L".eml", 4));
	if (!wstrName.get())
		return QSTATUS_OUTOFMEMORY;
	
	const WCHAR* pwszEscape = L"\\/:*?\"<>|";
	for (WCHAR* p = wstrName.get(); *p; ++p) {
		if (wcschr(pwszEscape, *p))
			*p = L'_';
	}
	
	*pwstrName = wstrName.release();
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * MessageDataObject::IEnumFORMATETCImpl
 *
 */

qm::MessageDataObject::IEnumFORMATETCImpl::IEnumFORMATETCImpl(QSTATUS* pstatus) :
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

STDMETHODIMP qm::MessageDataObject::IEnumFORMATETCImpl::QueryInterface(
	REFIID riid, void** ppv)
{
	*ppv = 0;
	
	if (riid == IID_IUnknown || riid == IID_IEnumFORMATETC) {
		AddRef();
		*ppv = static_cast<IEnumFORMATETC*>(this);
	}
	
	return *ppv ? S_OK : E_NOINTERFACE;
}

STDMETHODIMP qm::MessageDataObject::IEnumFORMATETCImpl::Next(
	ULONG celt, FORMATETC* rgelt, ULONG* pceltFetched)
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

STDMETHODIMP qm::MessageDataObject::IEnumFORMATETCImpl::Clone(
	IEnumFORMATETC** ppEnum)
{
	DECLARE_QSTATUS();
	
	IEnumFORMATETCImpl* pEnum = 0;
	status = newQsObject(&pEnum);
	if (status == QSTATUS_OUTOFMEMORY)
		return E_OUTOFMEMORY;
	pEnum->nCurrent_ = nCurrent_;
	pEnum->AddRef();
	
	*ppEnum = pEnum;
	
	return S_OK;
}
