/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#include <qmaccount.h>
#include <qmapplication.h>
#include <qmmessage.h>
#include <qmmessageholder.h>
#include <qmmessageoperation.h>
#include <qsosutil.h>
#include <qmsecurity.h>

#include <qsassert.h>
#include <qsconv.h>
#include <qsfile.h>
#include <qsstl.h>
#include <qstextutil.h>
#include <qsthread.h>

#include <cstdio>

#include <shlobj.h>
#include <tchar.h>

#include "dataobject.h"
#include "messagecontext.h"
#include "tempfilecleaner.h"
#include "undo.h"
#include "uri.h"
#include "../util/util.h"

using namespace qm;
using namespace qs;


#ifndef _WIN32_WCE

/****************************************************************************
 *
 * DataObjectUtil
 *
 */

wstring_ptr DataObjectUtil::getName(const WCHAR* pwszName,
									NameList* pListName)
{
	assert(pwszName);
	assert(pListName);
	
	wstring_ptr wstrName(allocWString(pwszName));
	
	int n = 2;
	while (std::find_if(pListName->begin(), pListName->end(),
		boost::bind(string_equal_i<WCHAR>(), _1, wstrName.get())) != pListName->end()) {
		WCHAR wsz[16];
		wsprintf(wsz, L" (%d)", n);
		const WCHAR* p = wcsrchr(pwszName, L'.');
		if (p)
			wstrName = concat(pwszName, p - pwszName, wsz, -1, p, -1);
		else
			wstrName = concat(pwszName, wsz);
		++n;
	}
	pListName->push_back(allocWString(wstrName.get()).release());
	
	return wstrName;
}

#endif


/****************************************************************************
 *
 * MessageDataObject
 *
 */

UINT qm::MessageDataObject::nFormats__[] = {
	::RegisterClipboardFormat(_T("QmMessageDataFolder")),
	::RegisterClipboardFormat(_T("QmMessageDataMessageHolderList")),
	::RegisterClipboardFormat(_T("QmMessageDataFlag")),
#ifndef _WIN32_WCE
	CF_HDROP,
	::RegisterClipboardFormat(CFSTR_FILEDESCRIPTOR),
	::RegisterClipboardFormat(CFSTR_FILECONTENTS)
#endif
};

FORMATETC qm::MessageDataObject::formats__[] = {
	{
		MessageDataObject::nFormats__[FORMAT_FOLDER],
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
		MessageDataObject::nFormats__[FORMAT_HDROP],
		0,
		DVASPECT_CONTENT,
		-1,
		TYMED_HGLOBAL
	},
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

qm::MessageDataObject::MessageDataObject(AccountManager* pAccountManager,
										 const URIResolver* pURIResolver,
										 TempFileCleaner* pTempFileCleaner) :
	nRef_(0),
	pAccountManager_(pAccountManager),
	pURIResolver_(pURIResolver),
	pTempFileCleaner_(pTempFileCleaner),
	pFolder_(0),
	flag_(FLAG_NONE)
{
	assert(pAccountManager);
	assert(pURIResolver);
	assert(pTempFileCleaner);
}

qm::MessageDataObject::MessageDataObject(AccountManager* pAccountManager,
										 const URIResolver* pURIResolver,
										 TempFileCleaner* pTempFileCleaner,
										 Folder* pFolder,
										 const MessageHolderList& l,
										 Flag flag) :
	nRef_(0),
	pAccountManager_(pAccountManager),
	pURIResolver_(pURIResolver),
	pTempFileCleaner_(pTempFileCleaner),
	pFolder_(pFolder),
	flag_(flag)
{
	assert(pAccountManager);
	assert(pURIResolver);
	assert(pTempFileCleaner);
	assert(pFolder);
	assert(pFolder->getAccount()->isLocked());
	assert(!l.empty());
	
	listMessagePtr_.assign(l.begin(), l.end());
}

#ifdef _WIN32_WCE
qm::MessageDataObject::MessageDataObject(AccountManager* pAccountManager,
										 const URIResolver* pURIResolver) :
	nRef_(0),
	pAccountManager_(pAccountManager),
	pURIResolver_(pURIResolver),
	pTempFileCleaner_(0),
	pFolder_(0),
	flag_(FLAG_NONE)
{
	assert(pAccountManager);
	assert(pURIResolver);
}
#endif

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
	if (pFormat->cfFormat == nFormats__[FORMAT_FOLDER]) {
		wstring_ptr wstrName(Util::formatFolder(pFolder_));
		hGlobal = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT,
			(wcslen(wstrName.get()) + 1)*sizeof(WCHAR));
		if (!hGlobal)
			return E_OUTOFMEMORY;
		
		LockGlobal lock(hGlobal);
		wcscpy(static_cast<WCHAR*>(lock.get()), wstrName.get());
	}
	else if (pFormat->cfFormat == nFormats__[FORMAT_MESSAGEHOLDERLIST]) {
		typedef std::vector<WCHAR> Buffer;
		Buffer buf;
		for (MessagePtrList::const_iterator it = listMessagePtr_.begin(); it != listMessagePtr_.end(); ++it) {
			MessagePtrLock mpl(*it);
			if (mpl) {
				wstring_ptr wstrURI(MessageHolderURI(mpl).toString());
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
		
		LockGlobal lock(hGlobal);
		memcpy(static_cast<WCHAR*>(lock.get()), &buf[0], buf.size()*sizeof(WCHAR));
	}
	else if (pFormat->cfFormat == nFormats__[FORMAT_FLAG]) {
		hGlobal = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, sizeof(Flag));
		if (!hGlobal)
			return E_OUTOFMEMORY;
		
		LockGlobal lock(hGlobal);
		*static_cast<Flag*>(lock.get()) = flag_;
	}
#ifndef _WIN32_WCE
	else if (pFormat->cfFormat == nFormats__[FORMAT_HDROP]) {
		if (!createTempFiles())
			return E_FAIL;
		
		hGlobal = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT,
			sizeof(DROPFILES) + listMessagePtr_.size()*MAX_PATH*sizeof(WCHAR) + sizeof(WCHAR));
		if (!hGlobal)
			return E_OUTOFMEMORY;
		
		MessagePtrList::size_type n = 0;
		{
			LockGlobal lock(hGlobal);
			DROPFILES* pdf = static_cast<DROPFILES*>(lock.get());
			pdf->pFiles = sizeof(DROPFILES);
			pdf->fWide = TRUE;
			
			WCHAR* p = reinterpret_cast<WCHAR*>(static_cast<CHAR*>(lock.get()) + sizeof(DROPFILES));
			DataObjectUtil::NameList listName;
			CONTAINER_DELETER(deleter, listName, &freeWString);
			while (n < listMessagePtr_.size()) {
				MessagePtrLock mpl(listMessagePtr_[n]);
				if (!mpl)
					break;
				
				wstring_ptr wstrName(getName(mpl, &listName));
				wstring_ptr wstrPath(concat(wstrTempDir_.get(), L"\\", wstrName.get()));
				wcscpy(p, wstrPath.get());
				p += wcslen(wstrPath.get()) + 1;
				
				++n;
			}
			*p = L'\0';
		}
		if (n != listMessagePtr_.size()) {
			GlobalFree(hGlobal);
			return E_FAIL;
		}
	}
	else if (pFormat->cfFormat == nFormats__[FORMAT_FILECONTENTS]) {
		if (pFormat->lindex < 0 ||
			static_cast<LONG>(listMessagePtr_.size()) <= pFormat->lindex)
			return E_FAIL;
		
		MessagePtr ptr = listMessagePtr_[pFormat->lindex];
		MessagePtrLock mpl(ptr);
		if (!mpl)
			return E_FAIL;
		
		Message msg;
		if (!mpl->getMessage(Account::GMF_ALL, 0, SECURITYMODE_NONE, &msg))
			return E_FAIL;
		xstring_size_ptr strContent(msg.getContent());
		if (!strContent.get())
			return E_FAIL;
		
		hGlobal = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, strContent.size());
		if (!hGlobal)
			return E_OUTOFMEMORY;
		
		LockGlobal lock(hGlobal);
		memcpy(static_cast<CHAR*>(lock.get()), strContent.get(), strContent.size());
	}
	else if (pFormat->cfFormat == nFormats__[FORMAT_FILEDESCRIPTOR]) {
		hGlobal = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT,
			sizeof(FILEGROUPDESCRIPTOR) +
			(listMessagePtr_.size() - 1)*sizeof(FILEDESCRIPTOR));
		if (!hGlobal)
			return E_OUTOFMEMORY;
		
		MessagePtrList::size_type n = 0;
		{
			LockGlobal lock(hGlobal);
			FILEGROUPDESCRIPTOR* pfgd = static_cast<FILEGROUPDESCRIPTOR*>(lock.get());
			pfgd->cItems = static_cast<UINT>(listMessagePtr_.size());
			DataObjectUtil::NameList listName;
			CONTAINER_DELETER(deleter, listName, &freeWString);
			while (n < listMessagePtr_.size()) {
				MessagePtrLock mpl(listMessagePtr_[n]);
				if (!mpl)
					break;
				pfgd->fgd[n].dwFlags = FD_WRITESTIME;
				
				Time time;
				mpl->getDate(&time);
				::SystemTimeToFileTime(&time, &pfgd->fgd[n].ftLastWriteTime);
				
				wstring_ptr wstrName(getName(mpl, &listName));
				W2T(wstrName.get(), ptszName);
				_tcsncpy(pfgd->fgd[n].cFileName, ptszName, MAX_PATH - 1);
				
				++n;
			}
		}
		if (n != listMessagePtr_.size()) {
			GlobalFree(hGlobal);
			return E_FAIL;
		}
	}
#endif
	else {
		return DV_E_FORMATETC;
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
	
	LockGlobal lock(pMedium->hGlobal);
	void* pData = lock.get();
	if (pFormat->cfFormat == nFormats__[FORMAT_FOLDER]) {
		pFolder_ = pAccountManager_->getFolder(0, static_cast<WCHAR*>(pData));
	}
	else if (pFormat->cfFormat == nFormats__[FORMAT_MESSAGEHOLDERLIST]) {
		const WCHAR* p = static_cast<const WCHAR*>(pData);
		while (*p) {
			std::auto_ptr<MessageHolderURI> pURI(URIFactory::parseMessageHolderURI(p));
			if (!pURI.get())
				return E_FAIL;
			listMessagePtr_.push_back(pURI->resolveMessagePtr(pURIResolver_));
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
	
	IEnumFORMATETCImpl* pEnum = new IEnumFORMATETCImpl(formats__, countof(formats__));
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

ComPtr<IDataObject> qm::MessageDataObject::getClipboard(AccountManager* pAccountManager,
														const URIResolver* pURIResolver)
{
	assert(pAccountManager);
	
#ifdef _WIN32_WCE
	std::auto_ptr<MessageDataObject> pDataObject(
		new MessageDataObject(pAccountManager, pURIResolver));
	
	Clipboard clipboard(0);
	if (!clipboard)
		return ComPtr<IDataObject>();
	
	for (int n = 0; n < countof(formats__); ++n) {
		FORMATETC etc = formats__[n];
		STGMEDIUM medium;
		medium.tymed = TYMED_HGLOBAL;
		medium.hGlobal = clipboard.getData(etc.cfFormat);
		if (!medium.hGlobal)
			return ComPtr<IDataObject>();
		HRESULT hr = pDataObject->SetData(&etc, &medium, FALSE);
		if (hr != S_OK)
			return ComPtr<IDataObject>();
	}
	
	pDataObject->AddRef();
	return ComPtr<IDataObject>(pDataObject.release());
#else
	ComPtr<IDataObject> pDataObject;
	HRESULT hr = ::OleGetClipboard(&pDataObject);
	if (hr != S_OK)
		return ComPtr<IDataObject>();
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
										  AccountManager* pAccountManager,
										  const URIResolver* pURIResolver,
										  NormalFolder* pFolderTo,
										  Flag flag,
										  MessageOperationCallback* pCallback,
										  UndoManager* pUndoManager)
{
	assert(pDataObject);
	assert(pAccountManager);
	assert(pURIResolver);
	assert(pFolderTo);
	
	if (flag == FLAG_NONE)
		flag = getPasteFlag(pDataObject, pAccountManager, pFolderTo);
	
	Folder* pFolderFrom = getFolder(pDataObject, pAccountManager);
	
	FORMATETC fe = formats__[FORMAT_MESSAGEHOLDERLIST];
	StgMedium stm;
	if (pDataObject->GetData(&fe, &stm) != S_OK)
		return true;
	
	LockGlobal lock(stm.hGlobal);
	const WCHAR* p = static_cast<const WCHAR*>(lock.get());
	MessagePtrList listMessagePtr;
	while (*p) {
		std::auto_ptr<MessageHolderURI> pURI(URIFactory::parseMessageHolderURI(p));
		if (!pURI.get())
			return false;
		listMessagePtr.push_back(pURI->resolveMessagePtr(pURIResolver));
		p += wcslen(p) + 1;
	}
	
	if (listMessagePtr.empty())
		return true;
	
	if (pCallback)
		pCallback->setCount(static_cast<unsigned int>(listMessagePtr.size()));
	
	UndoItemList undo;
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
		
		unsigned int nCopyFlags = Account::OPFLAG_ACTIVE |
			Account::COPYFLAG_MANAGEJUNK;
		if (flag == FLAG_MOVE)
			nCopyFlags |= Account::COPYFLAG_MOVE;
		if (!pAccount->copyMessages(l, pFolderFrom,
			pFolderTo, nCopyFlags, pCallback, &undo, 0))
			return false;
		
		if (pCallback && pCallback->isCanceled())
			break;
	}
	pUndoManager->pushUndoItem(undo.getUndoItem());
	
	return true;
}

bool qm::MessageDataObject::canPasteMessage(IDataObject* pDataObject)
{
	assert(pDataObject);
	
	FORMATETC fe = formats__[FORMAT_MESSAGEHOLDERLIST];
	return pDataObject->QueryGetData(&fe) == S_OK;
}

MessageDataObject::Flag qm::MessageDataObject::getPasteFlag(IDataObject* pDataObject,
															AccountManager* pAccountManager,
															NormalFolder* pFolder)
{
	assert(pDataObject);
	
	Flag flag = FLAG_NONE;
	
	FORMATETC fe = formats__[FORMAT_FLAG];
	StgMedium stm;
	HRESULT hr = pDataObject->GetData(&fe, &stm);
	if (hr == S_OK) {
		LockGlobal lock(stm.hGlobal);
		flag = *static_cast<Flag*>(lock.get());
	}
	
	if (flag == FLAG_NONE) {
#if 1
		flag = FLAG_MOVE;
#else
		Account* pAccount = 0;
		FORMATETC fe = formats__[FORMAT_FOLDER];
		StgMedium stm;
		hr = pDataObject->GetData(&fe, &stm);
		if (hr == S_OK) {
			LockGlobal lock(stm.hGlobal);
			const WCHAR* pwszName = static_cast<const WCHAR*>(lock.get());
			Folder* pFolderFrom = pAccountManager->getFolder(0, pwszName);
			if (pFolderFrom)
				pAccount = pFolderFrom->getAccount();
		}
		flag = pFolder->getAccount() != pAccount ? FLAG_COPY : FLAG_MOVE;
#endif
	}
	
	return flag;
}

qm::Folder* qm::MessageDataObject::getFolder(IDataObject* pDataObject,
											 AccountManager* pAccountManager)
{
	assert(pDataObject);
	
	FORMATETC fe = formats__[FORMAT_FOLDER];
	StgMedium stm;
	HRESULT hr = pDataObject->GetData(&fe, &stm);
	if (hr != S_OK)
		return 0;
	
	LockGlobal lock(stm.hGlobal);
	Folder* pFolder = pAccountManager->getFolder(0, static_cast<WCHAR*>(lock.get()));
	
	return pFolder;
}

bool qm::MessageDataObject::getURIs(IDataObject* pDataObject,
									URIList* pList)
{
	FORMATETC fe = formats__[FORMAT_MESSAGEHOLDERLIST];
	StgMedium stm;
	HRESULT hr = pDataObject->GetData(&fe, &stm);
	if (hr != S_OK)
		return false;
	
	LockGlobal lock(stm.hGlobal);
	const WCHAR* p = reinterpret_cast<const WCHAR*>(lock.get());
	while (*p) {
		std::auto_ptr<URI> pURI(URIFactory::parseURI(p));
		pList->push_back(pURI.get());
		pURI.release();
		p += wcslen(p) + 1;
	}
	
	return true;
}

#ifndef _WIN32_WCE
bool qm::MessageDataObject::createTempFiles()
{
	if (wstrTempDir_.get())
		return true;
	
	Time time(Time::getCurrentTime());
	WCHAR wszName[128];
	_snwprintf(wszName, countof(wszName), L"\\message-%04d%02d%02d%02d%02d%02d%03d-%u",
		time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute,
		time.wSecond, time.wMilliseconds, ::GetCurrentThreadId());
	const WCHAR* pwszTempDir = Application::getApplication().getTemporaryFolder();
	wstring_ptr wstrDir(concat(pwszTempDir, wszName));
	if (!File::createDirectory(wstrDir.get()))
		return false;
	pTempFileCleaner_->addDirectory(wstrDir.get());
	
	DataObjectUtil::NameList listName;
	CONTAINER_DELETER(deleter, listName, &freeWString);
	for (MessagePtrList::const_iterator it = listMessagePtr_.begin(); it != listMessagePtr_.end(); ++it) {
		MessagePtrLock mpl(*it);
		if (!mpl)
			return false;
		
		Message msg;
		if (!mpl->getMessage(Account::GMF_ALL, 0, SECURITYMODE_NONE, &msg))
			return false;
		xstring_size_ptr strContent(msg.getContent());
		if (!strContent.get())
			return false;
		
		wstring_ptr wstrName(getName(mpl, &listName));
		wstring_ptr wstrPath(concat(wstrDir.get(), L"\\", wstrName.get()));
		FileOutputStream stream(wstrPath.get());
		if (!stream ||
			stream.write(reinterpret_cast<const unsigned char*>(strContent.get()), strContent.size()) != strContent.size() ||
			!stream.close())
			return false;
		pTempFileCleaner_->addFile(wstrPath.get());
	}
	
	wstrTempDir_ = wstrDir;
	
	return true;
}

wstring_ptr qm::MessageDataObject::getName(MessageHolder* pmh,
										   DataObjectUtil::NameList* pListName)
{
	assert(pmh);
	assert(pListName);
	
	wstring_ptr wstrSubject(pmh->getSubject());
	wstring_ptr wstrName(getFileName(*wstrSubject.get() ? wstrSubject.get() : L"Untitled"));
	return DataObjectUtil::getName(wstrName.get(), pListName);
}

wstring_ptr qm::MessageDataObject::getFileName(const WCHAR* pwszName)
{
	assert(pwszName);
	
	size_t nLen = wcslen(pwszName);
	if (nLen + 5 >= MAX_PATH)
		nLen = MAX_PATH - 6;
	
	wstring_ptr wstrName(concat(pwszName, nLen, L".eml", 4));
	
	if (File::isDeviceName(wstrName.get()))
		wstrName = concat(L"_", wstrName.get());
	
	const WCHAR* pwszEscape = L"\\/:*?\"<>|";
	for (WCHAR* p = wstrName.get(); *p; ++p) {
		if (!TextUtil::isFileNameChar(*p))
			*p = L'_';
	}
	
	return wstrName;
}
#endif


/****************************************************************************
 *
 * FolderDataObject
 *
 */

UINT qm::FolderDataObject::nFormats__[] = {
	::RegisterClipboardFormat(_T("QmFolderData"))
};

FORMATETC qm::FolderDataObject::formats__[] = {
	{
		FolderDataObject::nFormats__[FORMAT_FOLDER],
		0,
		DVASPECT_CONTENT,
		-1,
		TYMED_HGLOBAL
	}
};

qm::FolderDataObject::FolderDataObject(Account* pAccount) :
	nRef_(0),
	pAccount_(pAccount),
	pFolder_(0)
{
}

qm::FolderDataObject::FolderDataObject(Folder* pFolder) :
	nRef_(0),
	pAccount_(0),
	pFolder_(pFolder)
{
}

qm::FolderDataObject::~FolderDataObject()
{
}

STDMETHODIMP_(ULONG) qm::FolderDataObject::AddRef()
{
	return ::InterlockedIncrement(reinterpret_cast<LONG*>(&nRef_));
}

STDMETHODIMP_(ULONG) qm::FolderDataObject::Release()
{
	ULONG nRef = ::InterlockedDecrement(reinterpret_cast<LONG*>(&nRef_));
	if (nRef == 0)
		delete this;
	return nRef;
}

STDMETHODIMP qm::FolderDataObject::QueryInterface(REFIID riid,
												  void** ppv)
{
	*ppv = 0;
	
	if (riid == IID_IUnknown || riid == IID_IDataObject) {
		AddRef();
		*ppv = static_cast<IDataObject*>(this);
	}
	
	return *ppv ? S_OK : E_NOINTERFACE;
}

STDMETHODIMP qm::FolderDataObject::GetData(FORMATETC* pFormat,
										   STGMEDIUM* pMedium)
{
	HRESULT hr = QueryGetData(pFormat);
	if (hr != S_OK)
		return hr;
	
	HGLOBAL hGlobal = 0;
	if (pFormat->cfFormat == nFormats__[FORMAT_FOLDER]) {
		wstring_ptr wstrName;
		if (pAccount_)
			wstrName = Util::formatAccount(pAccount_);
		else
			wstrName = Util::formatFolder(pFolder_);
		hGlobal = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT,
			(wcslen(wstrName.get()) + 1)*sizeof(WCHAR));
		if (!hGlobal)
			return E_OUTOFMEMORY;
		
		LockGlobal lock(hGlobal);
		wcscpy(static_cast<WCHAR*>(lock.get()), wstrName.get());
	}
	else {
		return DV_E_FORMATETC;
	}
	
	pMedium->tymed = TYMED_HGLOBAL;
	pMedium->hGlobal = hGlobal;
	pMedium->pUnkForRelease = 0;
	
	return S_OK;
}

STDMETHODIMP qm::FolderDataObject::GetDataHere(FORMATETC* pFormat,
											   STGMEDIUM* pMedium)
{
	return E_NOTIMPL;
}

STDMETHODIMP qm::FolderDataObject::QueryGetData(FORMATETC* pFormat)
{
	int n = 0;
	while (n < countof(nFormats__)) {
		if (pFormat->cfFormat == nFormats__[n])
			break;
		++n;
	}
	if (n == countof(nFormats__) || pFormat->ptd)
		return DV_E_FORMATETC;
	else if (pFormat->lindex != -1)
		return DV_E_LINDEX;
	else if (!(pFormat->tymed & TYMED_HGLOBAL))
		return DV_E_TYMED;
	else if (pFormat->dwAspect != DVASPECT_CONTENT)
		return DV_E_DVASPECT;
	else
		return S_OK;
}

STDMETHODIMP qm::FolderDataObject::GetCanonicalFormatEtc(FORMATETC* pFormatIn,
														 FORMATETC* pFormatOut)
{
	return E_NOTIMPL;
}

STDMETHODIMP qm::FolderDataObject::SetData(FORMATETC* pFormat,
										   STGMEDIUM* pMedium,
										   BOOL bRelease)
{
	return E_NOTIMPL;
}

STDMETHODIMP qm::FolderDataObject::EnumFormatEtc(DWORD dwDirection,
												 IEnumFORMATETC** ppEnum)
{
	if (dwDirection != DATADIR_GET)
		return E_NOTIMPL;
	
	IEnumFORMATETCImpl* pEnum = new IEnumFORMATETCImpl(formats__, countof(formats__));
	pEnum->AddRef();
	
	*ppEnum = pEnum;
	
	return S_OK;
}

STDMETHODIMP qm::FolderDataObject::DAdvise(FORMATETC* pFormat,
										   DWORD advf,
										   IAdviseSink* pSink,
										   DWORD* pdwConnection)
{
	return E_NOTIMPL;
}

STDMETHODIMP qm::FolderDataObject::DUnadvise(DWORD dwConnection)
{
	return E_NOTIMPL;
}

STDMETHODIMP qm::FolderDataObject::EnumDAdvise(IEnumSTATDATA** ppEnum)
{
	return E_NOTIMPL;
}

bool qm::FolderDataObject::canPasteFolder(IDataObject* pDataObject)
{
	assert(pDataObject);
	
	FORMATETC fe = formats__[FORMAT_FOLDER];
	return pDataObject->QueryGetData(&fe) == S_OK;
}

std::pair<Account*, qm::Folder*> qm::FolderDataObject::get(IDataObject* pDataObject,
														   AccountManager* pAccountManager)
{
	assert(pDataObject);
	assert(pAccountManager);
	
	FORMATETC fe = formats__[FORMAT_FOLDER];
	StgMedium stm;
	HRESULT hr = pDataObject->GetData(&fe, &stm);
	if (hr != S_OK)
		return std::pair<Account*, Folder*>(0, 0);
	
	LockGlobal lock(stm.hGlobal);
	
	return Util::getAccountOrFolder(pAccountManager, static_cast<const WCHAR*>(lock.get()));
}


#ifndef _WIN32_WCE

/****************************************************************************
 *
 * URIDataObject
 *
 */

UINT qm::URIDataObject::nFormats__[] = {
	CF_HDROP,
	::RegisterClipboardFormat(CFSTR_FILEDESCRIPTOR),
	::RegisterClipboardFormat(CFSTR_FILECONTENTS),
};

FORMATETC qm::URIDataObject::formats__[] = {
	{
		URIDataObject::nFormats__[FORMAT_HDROP],
		0,
		DVASPECT_CONTENT,
		-1,
		TYMED_HGLOBAL
	},
	{
		URIDataObject::nFormats__[FORMAT_FILEDESCRIPTOR],
		0,
		DVASPECT_CONTENT,
		-1,
		TYMED_HGLOBAL
	},
	{
		URIDataObject::nFormats__[FORMAT_FILECONTENTS],
		0,
		DVASPECT_CONTENT,
		-1,
		TYMED_HGLOBAL
	}
};

qm::URIDataObject::URIDataObject(const URIResolver* pURIResolver,
								 TempFileCleaner* pTempFileCleaner,
								 unsigned int nSecurityMode,
								 URIList& listURI) :
	nRef_(0),
	pURIResolver_(pURIResolver),
	pTempFileCleaner_(pTempFileCleaner),
	nSecurityMode_(nSecurityMode)
{
	listURI_.swap(listURI);
}

qm::URIDataObject::~URIDataObject()
{
	std::for_each(listURI_.begin(), listURI_.end(),
		boost::checked_deleter<URI>());
}

STDMETHODIMP_(ULONG) qm::URIDataObject::AddRef()
{
	return ::InterlockedIncrement(reinterpret_cast<LONG*>(&nRef_));
}

STDMETHODIMP_(ULONG) qm::URIDataObject::Release()
{
	ULONG nRef = ::InterlockedDecrement(reinterpret_cast<LONG*>(&nRef_));
	if (nRef == 0)
		delete this;
	return nRef;
}

STDMETHODIMP qm::URIDataObject::QueryInterface(REFIID riid,
											   void** ppv)
{
	*ppv = 0;
	
	if (riid == IID_IUnknown || riid == IID_IDataObject) {
		AddRef();
		*ppv = static_cast<IDataObject*>(this);
	}
	
	return *ppv ? S_OK : E_NOINTERFACE;
}

STDMETHODIMP qm::URIDataObject::GetData(FORMATETC* pFormat,
										STGMEDIUM* pMedium)
{
	HRESULT hr = QueryGetData(pFormat);
	if (hr != S_OK)
		return hr;
	
	HGLOBAL hGlobal = 0;
	if (pFormat->cfFormat == nFormats__[FORMAT_HDROP]) {
		if (!createTempFiles())
			return E_FAIL;
		
		hGlobal = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT,
			sizeof(DROPFILES) + listURI_.size()*MAX_PATH*sizeof(WCHAR) + sizeof(WCHAR));
		if (!hGlobal)
			return E_OUTOFMEMORY;
		
		LockGlobal lock(hGlobal);
		DROPFILES* pdf = static_cast<DROPFILES*>(lock.get());
		pdf->pFiles = sizeof(DROPFILES);
		pdf->fWide = TRUE;
		
		WCHAR* p = reinterpret_cast<WCHAR*>(static_cast<CHAR*>(lock.get()) + sizeof(DROPFILES));
		DataObjectUtil::NameList listName;
		CONTAINER_DELETER(deleter, listName, &freeWString);
		for (URIList::const_iterator it = listURI_.begin(); it != listURI_.end(); ++it) {
			const URI* pURI = *it;
			
			wstring_ptr wstrName(getName(pURI, &listName));
			wstring_ptr wstrPath(concat(wstrTempDir_.get(), L"\\", wstrName.get()));
			wcscpy(p, wstrPath.get());
			p += wcslen(wstrPath.get()) + 1;
		}
		
		*p = L'\0';
	}
	else if (pFormat->cfFormat == nFormats__[FORMAT_FILECONTENTS]) {
		if (pFormat->lindex < 0 ||
			static_cast<LONG>(listURI_.size()) <= pFormat->lindex)
			return E_FAIL;
		
		const URI* pURI = listURI_[pFormat->lindex];
		std::auto_ptr<MessageContext> pContext;
		const Part* pPart = getPart(pURI, &pContext);
		if (!pPart)
			return E_FAIL;
		
		BodyData bodyData(PartUtil(*pPart).getBodyData());
		if (!bodyData.get())
			return E_FAIL;
		
		hGlobal = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, bodyData.size());
		if (!hGlobal)
			return E_OUTOFMEMORY;
		
		LockGlobal lock(hGlobal);
		memcpy(static_cast<CHAR*>(lock.get()), bodyData.get(), bodyData.size());
	}
	else if (pFormat->cfFormat == nFormats__[FORMAT_FILEDESCRIPTOR]) {
		hGlobal = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT,
			sizeof(FILEGROUPDESCRIPTOR) +
			(listURI_.size() - 1)*sizeof(FILEDESCRIPTOR));
		if (!hGlobal)
			return E_OUTOFMEMORY;
		
		LockGlobal lock(hGlobal);
		FILEGROUPDESCRIPTOR* pfgd = static_cast<FILEGROUPDESCRIPTOR*>(lock.get());
		pfgd->cItems = static_cast<UINT>(listURI_.size());
		DataObjectUtil::NameList listName;
		CONTAINER_DELETER(deleter, listName, &freeWString);
		for (URIList::size_type n = 0; n < listURI_.size(); ++n) {
			const URI* pURI = listURI_[n];
			
			pfgd->fgd[n].dwFlags = 0;
			
			wstring_ptr wstrName(getName(pURI, &listName));
			W2T(wstrName.get(), ptszName);
			_tcsncpy(pfgd->fgd[n].cFileName, ptszName, MAX_PATH - 1);
		}
	}
	else {
		return DV_E_FORMATETC;
	}
	
	pMedium->tymed = TYMED_HGLOBAL;
	pMedium->hGlobal = hGlobal;
	pMedium->pUnkForRelease = 0;
	
	return S_OK;
}

STDMETHODIMP qm::URIDataObject::GetDataHere(FORMATETC* pFormat,
											STGMEDIUM* pMedium)
{
	return E_NOTIMPL;
}

STDMETHODIMP qm::URIDataObject::QueryGetData(FORMATETC* pFormat)
{
	int n = 0;
	while (n < countof(nFormats__)) {
		if (pFormat->cfFormat == nFormats__[n])
			break;
		++n;
	}
	if (n == countof(nFormats__) || pFormat->ptd)
		return DV_E_FORMATETC;
	else if (pFormat->cfFormat != nFormats__[FORMAT_FILECONTENTS] && pFormat->lindex != -1)
		return DV_E_LINDEX;
	else if (pFormat->cfFormat == nFormats__[FORMAT_FILECONTENTS] &&
		(pFormat->lindex < 0 || static_cast<LONG>(listURI_.size()) <= pFormat->lindex))
		return DV_E_LINDEX;
	else if (!(pFormat->tymed & TYMED_HGLOBAL))
		return DV_E_TYMED;
	else if (pFormat->dwAspect != DVASPECT_CONTENT)
		return DV_E_DVASPECT;
	else
		return S_OK;
}

STDMETHODIMP qm::URIDataObject::GetCanonicalFormatEtc(FORMATETC* pFormatIn,
													  FORMATETC* pFormatOut)
{
	return E_NOTIMPL;
}

STDMETHODIMP qm::URIDataObject::SetData(FORMATETC* pFormat,
										STGMEDIUM* pMedium,
										BOOL bRelease)
{
	return E_NOTIMPL;
}

STDMETHODIMP qm::URIDataObject::EnumFormatEtc(DWORD dwDirection,
											  IEnumFORMATETC** ppEnum)
{
	if (dwDirection != DATADIR_GET)
		return E_NOTIMPL;
	
	IEnumFORMATETCImpl* pEnum = new IEnumFORMATETCImpl(formats__, countof(formats__));
	pEnum->AddRef();
	
	*ppEnum = pEnum;
	
	return S_OK;
}

STDMETHODIMP qm::URIDataObject::DAdvise(FORMATETC* pFormat,
										DWORD advf,
										IAdviseSink* pSink,
										DWORD* pdwConnection)
{
	return E_NOTIMPL;
}

STDMETHODIMP qm::URIDataObject::DUnadvise(DWORD dwConnection)
{
	return E_NOTIMPL;
}

STDMETHODIMP qm::URIDataObject::EnumDAdvise(IEnumSTATDATA** ppEnum)
{
	return E_NOTIMPL;
}

bool qm::URIDataObject::createTempFiles()
{
	if (wstrTempDir_.get())
		return true;
	
	Time time(Time::getCurrentTime());
	WCHAR wszName[128];
	_snwprintf(wszName, countof(wszName), L"\\uri-%04d%02d%02d%02d%02d%02d%03d-%u",
		time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute,
		time.wSecond, time.wMilliseconds, ::GetCurrentThreadId());
	const WCHAR* pwszTempDir = Application::getApplication().getTemporaryFolder();
	wstring_ptr wstrDir(concat(pwszTempDir, wszName));
	if (!File::createDirectory(wstrDir.get()))
		return false;
	pTempFileCleaner_->addDirectory(wstrDir.get());
	
	DataObjectUtil::NameList listName;
	CONTAINER_DELETER(deleter, listName, &freeWString);
	for (URIList::const_iterator it = listURI_.begin(); it != listURI_.end(); ++it) {
		const URI* pURI = *it;
		
		std::auto_ptr<MessageContext> pContext;
		const Part* pPart = getPart(pURI, &pContext);
		if (!pPart)
			return false;
		
		wstring_ptr wstrName(getName(pURI, &listName));
		wstring_ptr wstrPath(concat(wstrDir.get(), L"\\", wstrName.get()));
		FileOutputStream stream(wstrPath.get());
		if (!stream ||
			!AttachmentParser(*pPart).detach(&stream) ||
			!stream.close())
			return false;
		pTempFileCleaner_->addFile(wstrPath.get());
	}
	
	wstrTempDir_ = wstrDir;
	
	return true;
}

const Part* qm::URIDataObject::getPart(const URI* pURI,
									   std::auto_ptr<MessageContext>* ppContext)
{
	assert(pURI);
	assert(ppContext);
	
	std::auto_ptr<MessageContext> pContext(pURI->resolve(pURIResolver_));
	if (!pContext.get())
		return 0;
	Message* pMessage = pContext->getMessage(Account::GMF_ALL, 0, nSecurityMode_);
	if (!pMessage)
		return 0;
	const Part* pPart = pURI->getFragment().getPart(pMessage);
	if (!pPart)
		return 0;
	
	*ppContext = pContext;
	return pPart;
}

wstring_ptr qm::URIDataObject::getName(const URI* pURI,
									   DataObjectUtil::NameList* pListName)
{
	assert(pURI);
	assert(pListName);
	
	const WCHAR* pwszName = pURI->getFragment().getName();
	wstring_ptr wstrName(allocWString(pwszName ? pwszName : L"Untitled"));
	return DataObjectUtil::getName(wstrName.get(), pListName);
}

#endif


/****************************************************************************
 *
 * IEnumFORMATETCImpl
 *
 */

qm::IEnumFORMATETCImpl::IEnumFORMATETCImpl(FORMATETC* pFormatEtc,
										   size_t nCount) :
	nRef_(0),
	pFormatEtc_(pFormatEtc),
	nCount_(nCount),
	nCurrent_(0)
{
}

qm::IEnumFORMATETCImpl::~IEnumFORMATETCImpl()
{
}

STDMETHODIMP_(ULONG) qm::IEnumFORMATETCImpl::AddRef()
{
	return ::InterlockedIncrement(reinterpret_cast<LONG*>(&nRef_));
}

STDMETHODIMP_(ULONG) qm::IEnumFORMATETCImpl::Release()
{
	ULONG nRef = ::InterlockedDecrement(reinterpret_cast<LONG*>(&nRef_));
	if (nRef == 0)
		delete this;
	return nRef;
}

STDMETHODIMP qm::IEnumFORMATETCImpl::QueryInterface(REFIID riid,
													void** ppv)
{
	*ppv = 0;
	
	if (riid == IID_IUnknown || riid == IID_IEnumFORMATETC) {
		AddRef();
		*ppv = static_cast<IEnumFORMATETC*>(this);
	}
	
	return *ppv ? S_OK : E_NOINTERFACE;
}

STDMETHODIMP qm::IEnumFORMATETCImpl::Next(ULONG celt,
										  FORMATETC* rgelt,
										  ULONG* pceltFetched)
{
	int nCount = 0;
	while (celt > 0 && nCurrent_ < nCount_) {
		rgelt[nCount++] = pFormatEtc_[nCurrent_++];
		--celt;
	}
	if (pceltFetched)
		*pceltFetched = nCount;
	
	return nCount != 0 ? S_OK : S_FALSE;
}

STDMETHODIMP qm::IEnumFORMATETCImpl::Skip(ULONG celt)
{
	nCurrent_ += celt;
	return nCurrent_ > nCount_ ? S_FALSE : S_OK;
}

STDMETHODIMP qm::IEnumFORMATETCImpl::Reset()
{
	nCurrent_ = 0;
	return S_OK;
}

STDMETHODIMP qm::IEnumFORMATETCImpl::Clone(IEnumFORMATETC** ppEnum)
{
	IEnumFORMATETCImpl* pEnum = new IEnumFORMATETCImpl(pFormatEtc_, nCount_);
	pEnum->nCurrent_ = nCurrent_;
	pEnum->AddRef();
	
	*ppEnum = pEnum;
	
	return S_OK;
}
