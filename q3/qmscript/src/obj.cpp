/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmapplication.h>

#include <qserror.h>
#include <qsnew.h>
#include <qsosutil.h>
#include <qsthread.h>

#include "macro.h"
#include "main.h"
#include "obj.h"

#ifndef DEPENDCHECK
#	include "qmobj_i.c"
#endif

#pragma warning(disable:4786)


using namespace qmscript;
using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * ObjectBase
 *
 */

qmscript::ObjectBase::ObjectBase()
{
}

qmscript::ObjectBase::~ObjectBase()
{
}


/****************************************************************************
 *
 * ApplicationImpl
 *
 */

qmscript::ApplicationImpl::ApplicationImpl(QSTATUS* pstatus) :
	pApplication_(0)
{
}

qmscript::ApplicationImpl::~ApplicationImpl()
{
}

QSTATUS qmscript::ApplicationImpl::init(Application* pApplication)
{
	assert(!pApplication_);
	pApplication_ = pApplication;
	return QSTATUS_SUCCESS;
}

HRESULT qmscript::ApplicationImpl::internalQueryInterface(REFIID riid, void** ppv)
{
	BEGIN_INTERFACE_MAP()
		INTERFACE_ENTRY(IID_IApplication, IApplication)
	END_INTERFACE_MAP()
}

STDMETHODIMP qmscript::ApplicationImpl::get_version(BSTR* pbstrVersion)
{
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrVersion;
	status = pApplication_->getVersion(false, &wstrVersion);
	CHECK_QSTATUS_HRESULT();
	
	*pbstrVersion = ::SysAllocString(wstrVersion.get());
	if (!*pbstrVersion)
		return E_OUTOFMEMORY;
	
	return S_OK;
}

STDMETHODIMP qmscript::ApplicationImpl::get_nothing(IDispatch** ppNothing)
{
	*ppNothing = 0;
	return S_OK;
}


/****************************************************************************
 *
 * DocumentImpl
 *
 */

qmscript::DocumentImpl::DocumentImpl(QSTATUS* pstatus) :
	pDocument_(0)
{
}

qmscript::DocumentImpl::~DocumentImpl()
{
}

QSTATUS qmscript::DocumentImpl::init(Document* pDocument)
{
	assert(!pDocument_);
	pDocument_ = pDocument;
	return QSTATUS_SUCCESS;
}

Document* qmscript::DocumentImpl::getDocument() const
{
	return pDocument_;
}

HRESULT qmscript::DocumentImpl::internalQueryInterface(REFIID riid, void** ppv)
{
	BEGIN_INTERFACE_MAP()
		INTERFACE_ENTRY(IID_IDocument, IDocument)
	END_INTERFACE_MAP()
}

STDMETHODIMP qmscript::DocumentImpl::get_accounts(IAccountList** ppAccountList)
{
	DECLARE_QSTATUS();
	
	std::auto_ptr<AccountListObj> pAccountList;
	status = newQsObject(&pAccountList);
	CHECK_QSTATUS_HRESULT();
	status = pAccountList->init(pDocument_);
	CHECK_QSTATUS_HRESULT();
	
	*ppAccountList = pAccountList.release();
	(*ppAccountList)->AddRef();
	
	return S_OK;
}

STDMETHODIMP qmscript::DocumentImpl::get_offline(VARIANT_BOOL* pbOffline)
{
	*pbOffline = pDocument_->isOffline() ? VARIANT_TRUE : VARIANT_FALSE;
	return S_OK;
}

STDMETHODIMP qmscript::DocumentImpl::put_offline(VARIANT_BOOL bOffline)
{
	DECLARE_QSTATUS();
	
	status = pDocument_->setOffline(bOffline == VARIANT_TRUE);
	CHECK_QSTATUS_HRESULT();
	
	return S_OK;
}


/****************************************************************************
 *
 * AccountImpl
 *
 */

qmscript::AccountImpl::AccountImpl(QSTATUS* pstatus) :
	pAccount_(0)
{
}

qmscript::AccountImpl::~AccountImpl()
{
}

QSTATUS qmscript::AccountImpl::init(Account* pAccount)
{
	assert(!pAccount_);
	pAccount_ = pAccount;
	return QSTATUS_SUCCESS;
}

Account* qmscript::AccountImpl::getAccount() const
{
	return pAccount_;
}

HRESULT qmscript::AccountImpl::internalQueryInterface(REFIID riid, void** ppv)
{
	BEGIN_INTERFACE_MAP()
		INTERFACE_ENTRY(IID_IAccount, IAccount)
	END_INTERFACE_MAP()
}

STDMETHODIMP qmscript::AccountImpl::get_name(BSTR* pbstrName)
{
	*pbstrName = ::SysAllocString(pAccount_->getName());
	return *pbstrName ? S_OK : E_OUTOFMEMORY;
}

STDMETHODIMP qmscript::AccountImpl::get_folders(IFolderList** ppFolderList)
{
	DECLARE_QSTATUS();
	
	std::auto_ptr<FolderListObj> pFolderList;
	status = newQsObject(&pFolderList);
	CHECK_QSTATUS_HRESULT();
	status = pFolderList->init(pAccount_);
	CHECK_QSTATUS_HRESULT();
	
	*ppFolderList = pFolderList.release();
	(*ppFolderList)->AddRef();
	
	return S_OK;
}


/****************************************************************************
 *
 * AccountListImpl
 *
 */

qmscript::AccountListImpl::AccountListImpl(QSTATUS* pstatus) :
	pDocument_(0)
{
}

qmscript::AccountListImpl::~AccountListImpl()
{
}

QSTATUS qmscript::AccountListImpl::init(Document* pDocument)
{
	assert(!pDocument_);
	pDocument_ = pDocument;
	return QSTATUS_SUCCESS;
}

HRESULT qmscript::AccountListImpl::internalQueryInterface(REFIID riid, void** ppv)
{
	BEGIN_INTERFACE_MAP()
		INTERFACE_ENTRY(IID_IAccountList, IAccountList)
	END_INTERFACE_MAP()
}

STDMETHODIMP qmscript::AccountListImpl::get_item(
	VARIANT varIndexOrName, IAccount** ppAccount)
{
	DECLARE_QSTATUS();
	
	Account* pAccount = 0;
	if (varIndexOrName.vt == VT_BSTR) {
		pAccount = pDocument_->getAccount(varIndexOrName.bstrVal);
	}
	else {
		Variant v;
		HRESULT hr = ::VariantChangeType(&v, &varIndexOrName, 0, VT_INT);
		if (FAILED(hr))
			return hr;
		
		const Document::AccountList& l = pDocument_->getAccounts();
		if (v.intVal < 0 || static_cast<long>(l.size()) <= v.intVal)
			return E_INVALIDARG;
		pAccount = l[v.intVal];
	}
	
	if (pAccount) {
		status = Factory::getFactory().createAccount(pAccount, ppAccount);
		CHECK_QSTATUS_HRESULT();
	}
	else {
		*ppAccount = 0;
	}
	
	return S_OK;
}

STDMETHODIMP qmscript::AccountListImpl::get_length(unsigned int* pnLength)
{
	*pnLength = pDocument_->getAccounts().size();
	return S_OK;
}

STDMETHODIMP qmscript::AccountListImpl::_newEnum(IUnknown** ppUnk)
{
	DECLARE_QSTATUS();
	
	typedef std::vector<VARIANT> List;
	List l;
	struct Deleter
	{
		typedef std::vector<VARIANT> List;
		Deleter(List& l) : l_(l) {}
		~Deleter()
		{
			List::iterator it = l_.begin();
			while (it != l_.end()) {
				::VariantClear(&(*it));
				++it;
			}
		}
		List& l_;
	} deleter(l);
	
	const Document::AccountList& listAccount = pDocument_->getAccounts();
	
	status = STLWrapper<List>(l).resize(listAccount.size());
	CHECK_QSTATUS_HRESULT();
	Variant v;
	std::fill(l.begin(), l.end(), v);
	
	for (Document::AccountList::size_type n = 0; n < listAccount.size(); ++n) {
		IAccount* pAccount = 0;
		status = Factory::getFactory().createAccount(listAccount[n], &pAccount);
		CHECK_QSTATUS_HRESULT();
		l[n].vt = VT_DISPATCH;
		l[n].pdispVal = pAccount;
	}
	
	typedef Object<EnumBase<IEnumVARIANT, &IID_IEnumVARIANT, VARIANT> > EnumObj;
	std::auto_ptr<EnumObj> pEnum;
	status = newQsObject(&pEnum);
	CHECK_QSTATUS_HRESULT();
	status = pEnum->init(&l[0], l.size());
	CHECK_QSTATUS_HRESULT();
	
	*ppUnk = pEnum.release();
	(*ppUnk)->AddRef();
	
	return S_OK;
}


/****************************************************************************
 *
 * FolderBase
 *
 */

template<class T>
qmscript::FolderBase<T>::FolderBase(QSTATUS* pstatus) :
	T(pstatus),
	pFolder_(0)
{
}

template<class T>
qmscript::FolderBase<T>::~FolderBase()
{
}

template<class T>
QSTATUS qmscript::FolderBase<T>::init(Folder* pFolder)
{
	assert(!pFolder_);
	pFolder_ = pFolder;
	return T::init(pFolder);
}

template<class T>
STDMETHODIMP qmscript::FolderBase<T>::get_id(unsigned int* pnId)
{
	*pnId = pFolder_->getId();
	return S_OK;
}

template<class T>
STDMETHODIMP qmscript::FolderBase<T>::get_type(BSTR* pbstrType)
{
	switch (pFolder_->getType()) {
	case Folder::TYPE_NORMAL:
		*pbstrType = ::SysAllocString(L"normal");
		break;
	case Folder::TYPE_QUERY:
		*pbstrType = ::SysAllocString(L"query");
		break;
	default:
		assert(false);
		break;
	}
	return *pbstrType ? S_OK : E_OUTOFMEMORY;
}

template<class T>
STDMETHODIMP qmscript::FolderBase<T>::get_name(BSTR* pbstrName)
{
	*pbstrName = ::SysAllocString(pFolder_->getName());
	return *pbstrName ? S_OK : E_OUTOFMEMORY;
}

template<class T>
STDMETHODIMP qmscript::FolderBase<T>::get_fullName(BSTR* pbstrFullName)
{
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrFullName;
	status = pFolder_->getFullName(&wstrFullName);
	CHECK_QSTATUS_HRESULT();
	
	*pbstrFullName = ::SysAllocString(wstrFullName.get());
	return *pbstrFullName ? S_OK : E_OUTOFMEMORY;
}

template<class T>
STDMETHODIMP qmscript::FolderBase<T>::get_separator(BSTR* pbstrSeparator)
{
	WCHAR cSeparator = pFolder_->getSeparator();
	*pbstrSeparator = ::SysAllocStringLen(&cSeparator, 1);
	return *pbstrSeparator ? S_OK : E_OUTOFMEMORY;
}

template<class T>
STDMETHODIMP qmscript::FolderBase<T>::get_flags(unsigned int* pnFlags)
{
	*pnFlags = pFolder_->getFlags();
	return S_OK;
}

template<class T>
STDMETHODIMP qmscript::FolderBase<T>::get_parent(IFolder** ppParent)
{
	DECLARE_QSTATUS();
	
	Folder* pParent = pFolder_->getParentFolder();
	if (pParent) {
		status = Factory::getFactory().createFolder(pParent, ppParent);
		CHECK_QSTATUS_HRESULT();
	}
	else {
		*ppParent = 0;
	}
	
	return S_OK;
}

template<class T>
STDMETHODIMP qmscript::FolderBase<T>::get_account(IAccount** ppAccount)
{
	DECLARE_QSTATUS();
	
	status = Factory::getFactory().createAccount(pFolder_->getAccount(), ppAccount);
	CHECK_QSTATUS_HRESULT();
	
	return S_OK;
}

template<class T>
STDMETHODIMP qmscript::FolderBase<T>::get_messages(
	IMessageHolderList** ppMessageHolderList)
{
	DECLARE_QSTATUS();
	
	std::auto_ptr<MessageHolderListObj> pMessageHolderList;
	status = newQsObject(&pMessageHolderList);
	CHECK_QSTATUS_HRESULT();
	status = pMessageHolderList->init(pFolder_);
	CHECK_QSTATUS_HRESULT();
	
	*ppMessageHolderList = pMessageHolderList.release();
	(*ppMessageHolderList)->AddRef();
	
	return S_OK;
}


/****************************************************************************
 *
 * NormalFolderImpl
 *
 */

qmscript::NormalFolderImpl::NormalFolderImpl(QSTATUS* pstatus)
{
}

qmscript::NormalFolderImpl::~NormalFolderImpl()
{
}

QSTATUS qmscript::NormalFolderImpl::init(Folder* pFolder)
{
	return QSTATUS_SUCCESS;
}

HRESULT qmscript::NormalFolderImpl::internalQueryInterface(REFIID riid, void** ppv)
{
	BEGIN_INTERFACE_MAP()
		INTERFACE_ENTRY(IID_IFolder, IFolder)
		INTERFACE_ENTRY(IID_INormalFolder, INormalFolder)
	END_INTERFACE_MAP()
}


/****************************************************************************
 *
 * QueryFolderImpl
 *
 */

qmscript::QueryFolderImpl::QueryFolderImpl(QSTATUS* pstatus)
{
}

qmscript::QueryFolderImpl::~QueryFolderImpl()
{
}

QSTATUS qmscript::QueryFolderImpl::init(Folder* pFolder)
{
	return QSTATUS_SUCCESS;
}

HRESULT qmscript::QueryFolderImpl::internalQueryInterface(REFIID riid, void** ppv)
{
	BEGIN_INTERFACE_MAP()
		INTERFACE_ENTRY(IID_IFolder, IFolder)
		INTERFACE_ENTRY(IID_IQueryFolder, IQueryFolder)
	END_INTERFACE_MAP()
}


/****************************************************************************
 *
 * FolderListImpl
 *
 */

qmscript::FolderListImpl::FolderListImpl(QSTATUS* pstatus) :
	pAccount_(0)
{
}

qmscript::FolderListImpl::~FolderListImpl()
{
}

QSTATUS qmscript::FolderListImpl::init(Account* pAccount)
{
	assert(!pAccount_);
	pAccount_ = pAccount;
	return QSTATUS_SUCCESS;
}

HRESULT qmscript::FolderListImpl::internalQueryInterface(REFIID riid, void** ppv)
{
	BEGIN_INTERFACE_MAP()
		INTERFACE_ENTRY(IID_IFolderList, IFolderList)
	END_INTERFACE_MAP()
}

STDMETHODIMP qmscript::FolderListImpl::get_item(
	VARIANT varIndexOrName, IFolder** ppFolder)
{
	DECLARE_QSTATUS();
	
	Folder* pFolder = 0;
	if (varIndexOrName.vt == VT_BSTR) {
		status = pAccount_->getFolder(varIndexOrName.bstrVal, &pFolder);
		CHECK_QSTATUS_HRESULT();
	}
	else {
		Variant v;
		HRESULT hr = ::VariantChangeType(&v, &varIndexOrName, 0, VT_INT);
		if (FAILED(hr))
			return hr;
		
		const Account::FolderList& l = pAccount_->getFolders();
		if (v.intVal < 0 || static_cast<long>(l.size()) <= v.intVal)
			return E_INVALIDARG;
		pFolder = l[v.intVal];
	}
	
	if (pFolder) {
		status = Factory::getFactory().createFolder(pFolder, ppFolder);
		CHECK_QSTATUS_HRESULT();
	}
	else {
		*ppFolder = 0;
	}
	
	return S_OK;
}

STDMETHODIMP qmscript::FolderListImpl::get_length(unsigned int* pnLength)
{
	*pnLength = pAccount_->getFolders().size();
	return S_OK;
}

STDMETHODIMP qmscript::FolderListImpl::_newEnum(IUnknown** ppUnk)
{
	DECLARE_QSTATUS();
	
	typedef std::vector<VARIANT> List;
	List l;
	struct Deleter
	{
		typedef std::vector<VARIANT> List;
		Deleter(List& l) : l_(l) {}
		~Deleter()
		{
			List::iterator it = l_.begin();
			while (it != l_.end()) {
				::VariantClear(&(*it));
				++it;
			}
		}
		List& l_;
	} deleter(l);
	
	const Account::FolderList& listFolder = pAccount_->getFolders();
	
	status = STLWrapper<List>(l).resize(listFolder.size());
	CHECK_QSTATUS_HRESULT();
	Variant v;
	std::fill(l.begin(), l.end(), v);
	
	for (Account::FolderList::size_type n = 0; n < listFolder.size(); ++n) {
		IFolder* pFolder = 0;
		status = Factory::getFactory().createFolder(listFolder[n], &pFolder);
		CHECK_QSTATUS_HRESULT();
		l[n].vt = VT_DISPATCH;
		l[n].pdispVal = pFolder;
	}
	
	typedef Object<EnumBase<IEnumVARIANT, &IID_IEnumVARIANT, VARIANT> > EnumObj;
	std::auto_ptr<EnumObj> pEnum;
	status = newQsObject(&pEnum);
	CHECK_QSTATUS_HRESULT();
	status = pEnum->init(&l[0], l.size());
	CHECK_QSTATUS_HRESULT();
	
	*ppUnk = pEnum.release();
	(*ppUnk)->AddRef();
	
	return S_OK;
}


/****************************************************************************
 *
 * MessageHolderImpl
 *
 */

qmscript::MessageHolderImpl::MessageHolderImpl(QSTATUS* pstatus) :
	pmh_(0)
{
}

qmscript::MessageHolderImpl::~MessageHolderImpl()
{
}

QSTATUS qmscript::MessageHolderImpl::init(MessageHolder* pmh)
{
	assert(!pmh_);
	pmh_ = pmh;
	return S_OK;
}

MessageHolder* qmscript::MessageHolderImpl::getMessageHolder() const
{
	return pmh_;
}

HRESULT qmscript::MessageHolderImpl::internalQueryInterface(REFIID riid, void** ppv)
{
	BEGIN_INTERFACE_MAP()
		INTERFACE_ENTRY(IID_IMessageHolder, IMessageHolder)
	END_INTERFACE_MAP()
}

STDMETHODIMP qmscript::MessageHolderImpl::get_id(unsigned int* pnId)
{
	*pnId = pmh_->getId();
	return S_OK;
}

STDMETHODIMP qmscript::MessageHolderImpl::get_flags(unsigned int* pnFlags)
{
	*pnFlags = pmh_->getFlags();
	return S_OK;
}

STDMETHODIMP qmscript::MessageHolderImpl::get_from(BSTR* pbstrFrom)
{
	return getString(&MessageHolder::getFrom, pbstrFrom);
}

STDMETHODIMP qmscript::MessageHolderImpl::get_to(BSTR* pbstrTo)
{
	return getString(&MessageHolder::getTo, pbstrTo);
}

STDMETHODIMP qmscript::MessageHolderImpl::get_fromTo(BSTR* pbstrFromTo)
{
	return getString(&MessageHolder::getFromTo, pbstrFromTo);
}

STDMETHODIMP qmscript::MessageHolderImpl::get_subject(BSTR* pbstrSubject)
{
	return getString(&MessageHolder::getSubject, pbstrSubject);
}

STDMETHODIMP qmscript::MessageHolderImpl::get_date(DATE* pDate)
{
	// TODO
	return E_NOTIMPL;
}

STDMETHODIMP qmscript::MessageHolderImpl::get_size(unsigned int* pnSize)
{
	*pnSize = pmh_->getSize();
	return S_OK;
}

STDMETHODIMP qmscript::MessageHolderImpl::get_folder(IFolder** ppFolder)
{
	DECLARE_QSTATUS();
	
	status = Factory::getFactory().createFolder(pmh_->getFolder(), ppFolder);
	CHECK_QSTATUS_HRESULT();
	
	return S_OK;
}

STDMETHODIMP qmscript::MessageHolderImpl::get_message(IMessage** ppMessage)
{
	DECLARE_QSTATUS();
	
	std::auto_ptr<MessageObj> pMessage;
	status = newQsObject(&pMessage);
	CHECK_QSTATUS_HRESULT();
	
	status = pmh_->getMessage(Account::GETMESSAGEFLAG_ALL,
		0, pMessage->getMessage());
	CHECK_QSTATUS_HRESULT();
	
	*ppMessage = pMessage.release();
	(*ppMessage)->AddRef();
	
	return S_OK;
}

HRESULT qmscript::MessageHolderImpl::getString(
	QSTATUS (MessageHolder::*pfn)(WSTRING*) const, BSTR* pbstr)
{
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstr;
	status = (pmh_->*pfn)(&wstr);
	CHECK_QSTATUS_HRESULT();
	
	*pbstr = ::SysAllocString(wstr.get());
	
	return *pbstr ? S_OK : E_OUTOFMEMORY;
}


/****************************************************************************
 *
 * MessageHolderListImpl
 *
 */

qmscript::MessageHolderListImpl::MessageHolderListImpl(QSTATUS* pstatus) :
	pFolder_(0)
{
}

qmscript::MessageHolderListImpl::~MessageHolderListImpl()
{
}

QSTATUS qmscript::MessageHolderListImpl::init(Folder* pFolder)
{
	assert(!pFolder_);
	pFolder_ = pFolder;
	return QSTATUS_SUCCESS;
}

HRESULT qmscript::MessageHolderListImpl::internalQueryInterface(REFIID riid, void** ppv)
{
	BEGIN_INTERFACE_MAP()
		INTERFACE_ENTRY(IID_IMessageHolderList, IMessageHolderList)
	END_INTERFACE_MAP()
}

STDMETHODIMP qmscript::MessageHolderListImpl::get_item(
	unsigned int nIndex, IMessageHolder** ppMessageHolder)
{
	DECLARE_QSTATUS();
	
	Lock<Account> lock(*pFolder_->getAccount());
	
	status = pFolder_->loadMessageHolders();
	CHECK_QSTATUS_HRESULT();
	
	if (nIndex < pFolder_->getCount()) {
		MessageHolder* pmh = pFolder_->getMessage(nIndex);
		status = Factory::getFactory().createMessageHolder(
			pmh, ppMessageHolder);
		CHECK_QSTATUS_HRESULT();
	}
	else {
		*ppMessageHolder = 0;
	}
	
	return S_OK;
}

STDMETHODIMP qmscript::MessageHolderListImpl::get_length(
	unsigned int* pnLength)
{
	*pnLength = pFolder_->getCount();
	return S_OK;
}


/****************************************************************************
 *
 * MessageImpl
 *
 */

qmscript::MessageImpl::MessageImpl(QSTATUS* pstatus) :
	msg_(pstatus)
{
}

qmscript::MessageImpl::~MessageImpl()
{
}

Message* qmscript::MessageImpl::getMessage()
{
	return &msg_;
}

HRESULT qmscript::MessageImpl::internalQueryInterface(REFIID riid, void** ppv)
{
	BEGIN_INTERFACE_MAP()
		INTERFACE_ENTRY(IID_IMessage, IMessage)
	END_INTERFACE_MAP()
}

STDMETHODIMP qmscript::MessageImpl::get_content(VARIANT* pvarContent)
{
	DECLARE_QSTATUS();
	
	string_ptr<STRING> strContent;
	status = msg_.getContent(&strContent);
	CHECK_QSTATUS_HRESULT();
	size_t nLen = strlen(strContent.get());
	
	SAFEARRAY* psa = ::SafeArrayCreateVector(VT_UI1, 0, nLen);
	if (!psa)
		return E_OUTOFMEMORY;
	void* pData = 0;
	HRESULT hr = ::SafeArrayAccessData(psa, &pData);
	if (SUCCEEDED(hr)) {
		memcpy(pData, strContent.get(), nLen);
		::SafeArrayUnaccessData(psa);
	}
	if (FAILED(hr)) {
		::SafeArrayDestroy(psa);
		return hr;
	}
	
	pvarContent->vt = VT_UI1 | VT_ARRAY;
	pvarContent->parray = psa;
	
	return S_OK;
}

STDMETHODIMP qmscript::MessageImpl::get_bodyText(
	BSTR bstrQuote, BSTR bstrCharset, BSTR* pbstrBody)
{
	DECLARE_QSTATUS();
	
	const WCHAR* pwszQuote = 0;
	if (bstrQuote && *bstrQuote)
		pwszQuote = bstrQuote;
	const WCHAR* pwszCharset = 0;
	if (bstrCharset && *bstrCharset)
		pwszCharset = bstrCharset;
	
	string_ptr<WSTRING> wstrBody;
	status = PartUtil(msg_).getBodyText(pwszQuote, pwszCharset, &wstrBody);
	CHECK_QSTATUS_HRESULT();
	
	*pbstrBody = ::SysAllocString(wstrBody.get());
	
	return *pbstrBody ? S_OK : E_OUTOFMEMORY;
}


/****************************************************************************
 *
 * MainWindowImpl
 *
 */

qmscript::MainWindowImpl::MainWindowImpl(QSTATUS* pstatus) :
	pMainWindow_(0)
{
}

qmscript::MainWindowImpl::~MainWindowImpl()
{
}

QSTATUS qmscript::MainWindowImpl::init(MainWindow* pMainWindow)
{
	assert(!pMainWindow_);
	pMainWindow_ = pMainWindow;
	return QSTATUS_SUCCESS;
}

HRESULT qmscript::MainWindowImpl::internalQueryInterface(REFIID riid, void** ppv)
{
	BEGIN_INTERFACE_MAP()
		INTERFACE_ENTRY(IID_IMainWindow, IMainWindow)
	END_INTERFACE_MAP()
}

const ActionInvoker* qmscript::MainWindowImpl::getActionInvoker() const
{
	return pMainWindow_->getActionInvoker();
}


/****************************************************************************
 *
 * EditFrameWindowImpl
 *
 */

qmscript::EditFrameWindowImpl::EditFrameWindowImpl(QSTATUS* pstatus) :
	pEditFrameWindow_(0)
{
}

qmscript::EditFrameWindowImpl::~EditFrameWindowImpl()
{
}

QSTATUS qmscript::EditFrameWindowImpl::init(EditFrameWindow* pEditFrameWindow)
{
	assert(!pEditFrameWindow_);
	pEditFrameWindow_ = pEditFrameWindow;
	return QSTATUS_SUCCESS;
}

HRESULT qmscript::EditFrameWindowImpl::internalQueryInterface(REFIID riid, void** ppv)
{
	BEGIN_INTERFACE_MAP()
		INTERFACE_ENTRY(IID_IEditFrameWindow, IEditFrameWindow)
	END_INTERFACE_MAP()
}

const ActionInvoker* qmscript::EditFrameWindowImpl::getActionInvoker() const
{
	return pEditFrameWindow_->getActionInvoker();
}


/****************************************************************************
 *
 * MessageFrameWindowImpl
 *
 */

qmscript::MessageFrameWindowImpl::MessageFrameWindowImpl(QSTATUS* pstatus) :
	pMessageFrameWindow_(0)
{
}

qmscript::MessageFrameWindowImpl::~MessageFrameWindowImpl()
{
}

QSTATUS qmscript::MessageFrameWindowImpl::init(MessageFrameWindow* pMessageFrameWindow)
{
	assert(!pMessageFrameWindow_);
	pMessageFrameWindow_ = pMessageFrameWindow;
	return QSTATUS_SUCCESS;
}

HRESULT qmscript::MessageFrameWindowImpl::internalQueryInterface(REFIID riid, void** ppv)
{
	BEGIN_INTERFACE_MAP()
		INTERFACE_ENTRY(IID_IMessageFrameWindow, IMessageFrameWindow)
	END_INTERFACE_MAP()
}

const ActionInvoker* qmscript::MessageFrameWindowImpl::getActionInvoker() const
{
	return pMessageFrameWindow_->getActionInvoker();
}


/****************************************************************************
 *
 * MacroImpl
 *
 */

qmscript::MacroImpl::MacroImpl(QSTATUS* pstatus) :
	pMacro_(0),
	pDocument_(0),
	pProfile_(0),
	hwnd_(0)
{
}

qmscript::MacroImpl::~MacroImpl()
{
	delete pMacro_;
	
	VariableList::iterator it = listVariable_.begin();
	while (it != listVariable_.end()) {
		freeWString((*it).first);
		::VariantClear(&(*it).second);
		++it;
	}
}

QSTATUS qmscript::MacroImpl::init(Macro* pMacro,
	Document* pDocument, Profile* pProfile, HWND hwnd)
{
	assert(!pMacro_);
	pMacro_ = pMacro;
	pDocument_ = pDocument;
	pProfile_ = pProfile;
	hwnd_ = hwnd;
	return QSTATUS_SUCCESS;
}

HRESULT qmscript::MacroImpl::internalQueryInterface(REFIID riid, void** ppv)
{
	BEGIN_INTERFACE_MAP()
		INTERFACE_ENTRY(IID_IMacro, IMacro)
	END_INTERFACE_MAP()
}

STDMETHODIMP qmscript::MacroImpl::evaluate(IMessageHolder* pMessageHolder,
	IAccount* pAccount, VARIANT* pvarResult)
{
	DECLARE_QSTATUS();
	
	MessageHolder* pmh = 0;
	if (pMessageHolder)
		pmh = static_cast<MessageHolderObj*>(pMessageHolder)->getMessageHolder();
	Message msg(&status);
	CHECK_QSTATUS_HRESULT();
	
	MacroVariableHolder variable(&status);
	VariableList::iterator it = listVariable_.begin();
	while (it != listVariable_.end()) {
		VARIANT v;
		::VariantInit(&v);
		HRESULT hr = ::VariantChangeType(&v, &(*it).second, 0, VT_BSTR);
		if (FAILED(hr))
			return hr;
		
		MacroValuePtr pValue;
		status = MacroValueFactory::getFactory().newString(v.bstrVal,
			reinterpret_cast<MacroValueString**>(&pValue));
		CHECK_QSTATUS_HRESULT();
		status = variable.setVariable((*it).first, pValue.get());
		CHECK_QSTATUS_HRESULT();
		
		++it;
	}
	
	MacroContext::Init init = {
		pmh,
		pmh ? &msg : 0,
		static_cast<AccountObj*>(pAccount)->getAccount(),
		pDocument_,
		hwnd_,
		pProfile_,
		false,
		0,
		&variable
	};
	MacroContext context(init, &status);
	CHECK_QSTATUS_HRESULT();
	MacroValuePtr pValue;
	status = pMacro_->value(&context, &pValue);
	CHECK_QSTATUS_HRESULT();
	switch (pValue->getType()) {
	case MacroValue::TYPE_BOOLEAN:
		pvarResult->vt = VT_BOOL;
		pvarResult->boolVal = pValue->boolean() ? VARIANT_TRUE : VARIANT_FALSE;
		break;
	case MacroValue::TYPE_NUMBER:
		pvarResult->vt = VT_I4;
		pvarResult->lVal = pValue->number();
		break;
	default:
		{
			string_ptr<WSTRING> wstrValue;
			status = pValue->string(&wstrValue);
			CHECK_QSTATUS_HRESULT();
			pvarResult->vt = VT_BSTR;
			pvarResult->bstrVal = ::SysAllocString(wstrValue.get());
			if (!pvarResult->bstrVal)
				return E_OUTOFMEMORY;
		}
		break;
	}
	
	return S_OK;
}

STDMETHODIMP qmscript::MacroImpl::setVariable(BSTR bstrName, VARIANT var)
{
	DECLARE_QSTATUS();
	
	VariableList::iterator it = std::find_if(
		listVariable_.begin(), listVariable_.end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				string_equal<WCHAR>(),
				std::select1st<VariableList::value_type>(),
				std::identity<const WCHAR*>()),
			bstrName));
	if (it != listVariable_.end()) {
		::VariantClear(&(*it).second);
		HRESULT hr = ::VariantCopy(&(*it).second, &var);
		if (FAILED(hr))
			return hr;
	}
	else {
		string_ptr<WSTRING> wstrName(allocWString(bstrName));
		if (!wstrName.get())
			return E_OUTOFMEMORY;
		
		VARIANT v;
		::VariantInit(&v);
		HRESULT hr = ::VariantCopy(&v, &var);
		if (FAILED(hr))
			return hr;
		
		struct Deleter
		{
			Deleter(VARIANT& v) : p_(&v) {}
			~Deleter()
			{
				if (p_)
					::VariantClear(p_);
			}
			void release() { p_ = 0; }
			VARIANT* p_;
		} deleter(v);
		
		status = STLWrapper<VariableList>(listVariable_).push_back(
			std::make_pair(wstrName.get(), v));
		CHECK_QSTATUS_HRESULT();
		wstrName.release();
		deleter.release();
	}
	
	return S_OK;
}

STDMETHODIMP qmscript::MacroImpl::getVariable(BSTR bstrName, VARIANT* pVar)
{
	::VariantInit(pVar);
	
	VariableList::iterator it = std::find_if(
		listVariable_.begin(), listVariable_.end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				string_equal<WCHAR>(),
				std::select1st<VariableList::value_type>(),
				std::identity<const WCHAR*>()),
			bstrName));
	if (it != listVariable_.end()) {
		HRESULT hr = ::VariantCopy(pVar, &(*it).second);
		if (FAILED(hr))
			return hr;
	}
	
	return S_OK;
}

STDMETHODIMP qmscript::MacroImpl::removeVariable(BSTR bstrName)
{
	VariableList::iterator it = std::find_if(
		listVariable_.begin(), listVariable_.end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				string_equal<WCHAR>(),
				std::select1st<VariableList::value_type>(),
				std::identity<const WCHAR*>()),
			bstrName));
	if (it != listVariable_.end()) {
		freeWString((*it).first);
		::VariantClear(&(*it).second);
		listVariable_.erase(it);
	}
	
	return S_OK;
}


/****************************************************************************
 *
 * MacroParserImpl
 *
 */

qmscript::MacroParserImpl::MacroParserImpl(QSTATUS* pstatus) :
	pDocument_(0),
	pProfile_(0),
	hwnd_(0)
{
}

qmscript::MacroParserImpl::~MacroParserImpl()
{
}

QSTATUS qmscript::MacroParserImpl::init(Document* pDocument,
	Profile* pProfile, HWND hwnd)
{
	assert(!pDocument_);
	pDocument_ = pDocument;
	pProfile_ = pProfile;
	hwnd_ = hwnd;
	return QSTATUS_SUCCESS;
}

HRESULT qmscript::MacroParserImpl::internalQueryInterface(REFIID riid, void** ppv)
{
	BEGIN_INTERFACE_MAP()
		INTERFACE_ENTRY(IID_IMacroParser, IMacroParser)
	END_INTERFACE_MAP()
}

STDMETHODIMP qmscript::MacroParserImpl::parse(BSTR bstrMacro, IMacro** ppMacro)
{
	DECLARE_QSTATUS();
	
	// TODO
	// Type ?
	MacroParser parser(MacroParser::TYPE_TEMPLATE, &status);
	CHECK_QSTATUS_HRESULT();
	Macro* p = 0;
	status = parser.parse(bstrMacro, &p);
	CHECK_QSTATUS_HRESULT();
	std::auto_ptr<Macro> pMacro(p);
	
	std::auto_ptr<MacroObj> pMacroObj;
	status = newQsObject(&pMacroObj);
	CHECK_QSTATUS_HRESULT();
	status = pMacroObj->init(pMacro.get(), pDocument_, pProfile_, hwnd_);
	CHECK_QSTATUS_HRESULT();
	pMacro.release();
	
	*ppMacro = pMacroObj.release();
	(*ppMacro)->AddRef();
	
	return S_OK;
}


/****************************************************************************
 *
 * ArgumentListImpl
 *
 */

qmscript::ArgumentListImpl::ArgumentListImpl(QSTATUS* pstatus)
{
}

qmscript::ArgumentListImpl::~ArgumentListImpl()
{
	ArgumentList::iterator it = listArgument_.begin();
	while (it != listArgument_.end()) {
		::VariantClear(&(*it));
		++it;
	}
}

QSTATUS qmscript::ArgumentListImpl::init(VARIANT* pvar, size_t nCount)
{
	assert(listArgument_.empty());
	
	DECLARE_QSTATUS();
	
	status = STLWrapper<ArgumentList>(listArgument_).resize(nCount);
	CHECK_QSTATUS();
	Variant v;
	std::fill(listArgument_.begin(), listArgument_.end(), v);
	
	for (size_t n = 0; n < nCount; ++n) {
		HRESULT hr = ::VariantCopy(&listArgument_[n], pvar + n);
		CHECK_HRESULT();
	}
	
	return QSTATUS_SUCCESS;
}

HRESULT qmscript::ArgumentListImpl::internalQueryInterface(REFIID riid, void** ppv)
{
	BEGIN_INTERFACE_MAP()
		INTERFACE_ENTRY(IID_IArgumentList, IArgumentList)
	END_INTERFACE_MAP()
}

STDMETHODIMP qmscript::ArgumentListImpl::get_item(
	unsigned int nIndex, VARIANT* pvarArg)
{
	if (nIndex >= listArgument_.size())
		return E_INVALIDARG;
	return ::VariantCopy(pvarArg, &listArgument_[nIndex]);
}

STDMETHODIMP qmscript::ArgumentListImpl::get_length(unsigned int* pnLength)
{
	*pnLength = listArgument_.size();
	return S_OK;
}

STDMETHODIMP qmscript::ArgumentListImpl::_newEnum(IUnknown** ppUnk)
{
	DECLARE_QSTATUS();
	
	typedef Object<EnumBase<IEnumVARIANT, &IID_IEnumVARIANT, VARIANT> > EnumObj;
	std::auto_ptr<EnumObj> pEnum;
	status = newQsObject(&pEnum);
	CHECK_QSTATUS_HRESULT();
	status = pEnum->init(&listArgument_[0], listArgument_.size());
	CHECK_QSTATUS_HRESULT();
	
	*ppUnk = pEnum.release();
	(*ppUnk)->AddRef();
	
	return S_OK;
}


/****************************************************************************
 *
 * ResultImpl
 *
 */

qmscript::ResultImpl::ResultImpl(QSTATUS* pstatus)
{
	::VariantInit(&varValue_);
}

qmscript::ResultImpl::~ResultImpl()
{
	::VariantClear(&varValue_);
}

VARIANT* qmscript::ResultImpl::getValue()
{
	return &varValue_;
}

HRESULT qmscript::ResultImpl::internalQueryInterface(REFIID riid, void** ppv)
{
	BEGIN_INTERFACE_MAP()
		INTERFACE_ENTRY(IID_IResult, IResult)
	END_INTERFACE_MAP()
}

STDMETHODIMP qmscript::ResultImpl::put_value(VARIANT varValue)
{
	::VariantClear(&varValue_);
	return ::VariantCopy(&varValue_, &varValue);
}


/****************************************************************************
 *
 * Factory
 *
 */

Factory qmscript::Factory::factory__;

qmscript::Factory::Factory()
{
}

qmscript::Factory::~Factory()
{
}

QSTATUS qmscript::Factory::createApplication(
	Application* pApplication, IApplication** ppApplication)
{
	DECLARE_QSTATUS();
	
	std::auto_ptr<ApplicationObj> pApplicationObj;
	status = newQsObject(&pApplicationObj);
	CHECK_QSTATUS();
	status = pApplicationObj->init(pApplication);
	CHECK_QSTATUS();
	
	*ppApplication = pApplicationObj.release();
	(*ppApplication)->AddRef();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmscript::Factory::createDocument(
	Document* pDocument, IDocument** ppDocument)
{
	DECLARE_QSTATUS();
	
	std::auto_ptr<DocumentObj> pDocumentObj;
	status = newQsObject(&pDocumentObj);
	CHECK_QSTATUS();
	status = pDocumentObj->init(pDocument);
	CHECK_QSTATUS();
	
	*ppDocument = pDocumentObj.release();
	(*ppDocument)->AddRef();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmscript::Factory::createAccount(
	Account* pAccount, IAccount** ppAccount)
{
	assert(pAccount);
	assert(ppAccount);
	
	DECLARE_QSTATUS();
	
	std::auto_ptr<AccountObj> pAccountObj;
	status = newQsObject(&pAccountObj);
	CHECK_QSTATUS();
	status = pAccountObj->init(pAccount);
	CHECK_QSTATUS();
	*ppAccount = pAccountObj.release();
	(*ppAccount)->AddRef();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmscript::Factory::createFolder(
	Folder* pFolder, IFolder** ppFolder)
{
	assert(pFolder);
	assert(ppFolder);
	
	DECLARE_QSTATUS();
	
	switch (pFolder->getType()) {
	case Folder::TYPE_NORMAL:
		{
			std::auto_ptr<NormalFolderObj> pFolderObj;
			status = newQsObject(&pFolderObj);
			CHECK_QSTATUS();
			status = pFolderObj->init(static_cast<NormalFolder*>(pFolder));
			CHECK_QSTATUS();
			*ppFolder = pFolderObj.release();
			(*ppFolder)->AddRef();
		}
		break;
	case Folder::TYPE_QUERY:
		{
			std::auto_ptr<QueryFolderObj> pFolderObj;
			status = newQsObject(&pFolderObj);
			CHECK_QSTATUS();
			status = pFolderObj->init(static_cast<QueryFolder*>(pFolder));
			CHECK_QSTATUS();
			*ppFolder = pFolderObj.release();
			(*ppFolder)->AddRef();
		}
		break;
	default:
		assert(false);
		break;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmscript::Factory::createMessageHolder(
	MessageHolder* pmh, IMessageHolder** ppMessageHolder)
{
	assert(pmh);
	assert(ppMessageHolder);
	
	DECLARE_QSTATUS();
	
	std::auto_ptr<MessageHolderObj> pMessageHolderObj;
	status = newQsObject(&pMessageHolderObj);
	CHECK_QSTATUS();
	status = pMessageHolderObj->init(pmh);
	CHECK_QSTATUS();
	*ppMessageHolder = pMessageHolderObj.release();
	(*ppMessageHolder)->AddRef();
	
	return QSTATUS_SUCCESS;
}

Factory& qmscript::Factory::getFactory()
{
	return factory__;
}
