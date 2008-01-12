/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#pragma warning(disable:4786)

#include <qmapplication.h>
#include <qmsecurity.h>

#include <qsosutil.h>
#include <qsthread.h>

#include <boost/bind.hpp>

#include "macro.h"
#include "main.h"
#include "obj.h"

#ifndef DEPENDCHECK
#	include "qmobj_i.c"
#endif


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

qmscript::ApplicationImpl::ApplicationImpl() :
	pApplication_(0)
{
}

qmscript::ApplicationImpl::~ApplicationImpl()
{
}

void qmscript::ApplicationImpl::init(Application* pApplication)
{
	assert(!pApplication_);
	pApplication_ = pApplication;
}

HRESULT qmscript::ApplicationImpl::internalQueryInterface(REFIID riid,
														  void** ppv)
{
	BEGIN_INTERFACE_MAP()
		INTERFACE_ENTRY(IID_IApplication, IApplication)
	END_INTERFACE_MAP()
}

STDMETHODIMP qmscript::ApplicationImpl::get_version(BSTR* pbstrVersion)
{
	wstring_ptr wstrVersion(pApplication_->getVersion(L' ', false));
	
	*pbstrVersion = ::SysAllocString(wstrVersion.get());
	return *pbstrVersion ? S_OK : E_OUTOFMEMORY;
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

qmscript::DocumentImpl::DocumentImpl() :
	pDocument_(0)
{
}

qmscript::DocumentImpl::~DocumentImpl()
{
}

void qmscript::DocumentImpl::init(Document* pDocument)
{
	assert(!pDocument_);
	pDocument_ = pDocument;
}

Document* qmscript::DocumentImpl::getDocument() const
{
	return pDocument_;
}

HRESULT qmscript::DocumentImpl::internalQueryInterface(REFIID riid,
													   void** ppv)
{
	BEGIN_INTERFACE_MAP()
		INTERFACE_ENTRY(IID_IDocument, IDocument)
	END_INTERFACE_MAP()
}

STDMETHODIMP qmscript::DocumentImpl::get_accounts(IAccountList** ppAccountList)
{
	std::auto_ptr<AccountListObj> pAccountList(new AccountListObj());
	pAccountList->init(pDocument_);
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
	pDocument_->setOffline(bOffline == VARIANT_TRUE);
	return S_OK;
}


/****************************************************************************
 *
 * AccountImpl
 *
 */

qmscript::AccountImpl::AccountImpl() :
	pAccount_(0)
{
}

qmscript::AccountImpl::~AccountImpl()
{
}

void qmscript::AccountImpl::init(Account* pAccount)
{
	assert(!pAccount_);
	pAccount_ = pAccount;
}

Account* qmscript::AccountImpl::getAccount() const
{
	return pAccount_;
}

HRESULT qmscript::AccountImpl::internalQueryInterface(REFIID riid,
													  void** ppv)
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
	std::auto_ptr<FolderListObj> pFolderList(new FolderListObj());
	pFolderList->init(pAccount_);
	*ppFolderList = pFolderList.release();
	(*ppFolderList)->AddRef();
	return S_OK;
}


/****************************************************************************
 *
 * AccountListImpl
 *
 */

qmscript::AccountListImpl::AccountListImpl() :
	pDocument_(0)
{
}

qmscript::AccountListImpl::~AccountListImpl()
{
}

void qmscript::AccountListImpl::init(Document* pDocument)
{
	assert(!pDocument_);
	pDocument_ = pDocument;
}

HRESULT qmscript::AccountListImpl::internalQueryInterface(REFIID riid,
														  void** ppv)
{
	BEGIN_INTERFACE_MAP()
		INTERFACE_ENTRY(IID_IAccountList, IAccountList)
	END_INTERFACE_MAP()
}

STDMETHODIMP qmscript::AccountListImpl::get_item(VARIANT varIndexOrName,
												 IAccount** ppAccount)
{
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
	
	if (pAccount)
		*ppAccount = Factory::getFactory().createAccount(pAccount);
	else
		*ppAccount = 0;
	
	return S_OK;
}

STDMETHODIMP qmscript::AccountListImpl::get_length(unsigned int* pnLength)
{
	*pnLength = static_cast<int>(pDocument_->getAccounts().size());
	return S_OK;
}

STDMETHODIMP qmscript::AccountListImpl::_newEnum(IUnknown** ppUnk)
{
	typedef std::vector<VARIANT> List;
	List l;
	struct Deleter
	{
		typedef std::vector<VARIANT> List;
		
		Deleter(List& l) :
			l_(l)
		{
		}
		
		~Deleter()
		{
			for (List::iterator it = l_.begin(); it != l_.end(); ++it)
				::VariantClear(&(*it));
		}
		
		List& l_;
	} deleter(l);
	
	const Document::AccountList& listAccount = pDocument_->getAccounts();
	
	l.resize(listAccount.size());
	Variant v;
	std::fill(l.begin(), l.end(), v);
	
	for (Document::AccountList::size_type n = 0; n < listAccount.size(); ++n) {
		IAccount* pAccount = Factory::getFactory().createAccount(listAccount[n]);
		l[n].vt = VT_DISPATCH;
		l[n].pdispVal = pAccount;
	}
	
	typedef Object<EnumBase<IEnumVARIANT, &IID_IEnumVARIANT, VARIANT> > EnumObj;
	std::auto_ptr<EnumObj> pEnum(new EnumObj());
	if (!pEnum->init(&l[0], l.size()))
		return E_FAIL;
	
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
qmscript::FolderBase<T>::FolderBase() :
	pFolder_(0)
{
}

template<class T>
qmscript::FolderBase<T>::~FolderBase()
{
}

template<class T>
void qmscript::FolderBase<T>::init(Folder* pFolder)
{
	assert(!pFolder_);
	pFolder_ = pFolder;
	T::init(pFolder);
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
	wstring_ptr wstrFullName(pFolder_->getFullName());
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
	Folder* pParent = pFolder_->getParentFolder();
	if (pParent)
		*ppParent = Factory::getFactory().createFolder(pParent);
	else
		*ppParent = 0;
	return S_OK;
}

template<class T>
STDMETHODIMP qmscript::FolderBase<T>::get_account(IAccount** ppAccount)
{
	*ppAccount = Factory::getFactory().createAccount(pFolder_->getAccount());
	return S_OK;
}

template<class T>
STDMETHODIMP qmscript::FolderBase<T>::get_messages(IMessageHolderList** ppMessageHolderList)
{
	std::auto_ptr<MessageHolderListObj> pMessageHolderList(new MessageHolderListObj());
	pMessageHolderList->init(pFolder_);
	*ppMessageHolderList = pMessageHolderList.release();
	(*ppMessageHolderList)->AddRef();
	return S_OK;
}


/****************************************************************************
 *
 * NormalFolderImpl
 *
 */

qmscript::NormalFolderImpl::NormalFolderImpl()
{
}

qmscript::NormalFolderImpl::~NormalFolderImpl()
{
}

void qmscript::NormalFolderImpl::init(Folder* pFolder)
{
}

HRESULT qmscript::NormalFolderImpl::internalQueryInterface(REFIID riid,
														   void** ppv)
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

qmscript::QueryFolderImpl::QueryFolderImpl()
{
}

qmscript::QueryFolderImpl::~QueryFolderImpl()
{
}

void qmscript::QueryFolderImpl::init(Folder* pFolder)
{
}

HRESULT qmscript::QueryFolderImpl::internalQueryInterface(REFIID riid,
														  void** ppv)
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

qmscript::FolderListImpl::FolderListImpl() :
	pAccount_(0)
{
}

qmscript::FolderListImpl::~FolderListImpl()
{
}

void qmscript::FolderListImpl::init(Account* pAccount)
{
	assert(!pAccount_);
	pAccount_ = pAccount;
}

HRESULT qmscript::FolderListImpl::internalQueryInterface(REFIID riid,
														 void** ppv)
{
	BEGIN_INTERFACE_MAP()
		INTERFACE_ENTRY(IID_IFolderList, IFolderList)
	END_INTERFACE_MAP()
}

STDMETHODIMP qmscript::FolderListImpl::get_item(VARIANT varIndexOrName,
												IFolder** ppFolder)
{
	Folder* pFolder = 0;
	if (varIndexOrName.vt == VT_BSTR) {
		pFolder = pAccount_->getFolder(varIndexOrName.bstrVal);
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
	
	if (pFolder)
		*ppFolder = Factory::getFactory().createFolder(pFolder);
	else
		*ppFolder = 0;
	
	return S_OK;
}

STDMETHODIMP qmscript::FolderListImpl::get_length(unsigned int* pnLength)
{
	*pnLength = static_cast<unsigned int>(pAccount_->getFolders().size());
	return S_OK;
}

STDMETHODIMP qmscript::FolderListImpl::_newEnum(IUnknown** ppUnk)
{
	typedef std::vector<VARIANT> List;
	List l;
	struct Deleter
	{
		typedef std::vector<VARIANT> List;
		
		Deleter(List& l) :
			l_(l)
		{
		}
		
		~Deleter()
		{
			for (List::iterator it = l_.begin(); it != l_.end(); ++it)
				::VariantClear(&(*it));
		}
		
		List& l_;
	} deleter(l);
	
	const Account::FolderList& listFolder = pAccount_->getFolders();
	
	l.resize(listFolder.size());
	Variant v;
	std::fill(l.begin(), l.end(), v);
	
	for (Account::FolderList::size_type n = 0; n < listFolder.size(); ++n) {
		IFolder* pFolder = Factory::getFactory().createFolder(listFolder[n]);
		l[n].vt = VT_DISPATCH;
		l[n].pdispVal = pFolder;
	}
	
	typedef Object<EnumBase<IEnumVARIANT, &IID_IEnumVARIANT, VARIANT> > EnumObj;
	std::auto_ptr<EnumObj> pEnum(new EnumObj());
	if (!pEnum->init(&l[0], l.size()))
		return E_FAIL;
	
	*ppUnk = pEnum.release();
	(*ppUnk)->AddRef();
	
	return S_OK;
}


/****************************************************************************
 *
 * MessageHolderImpl
 *
 */

qmscript::MessageHolderImpl::MessageHolderImpl() :
	pmh_(0)
{
}

qmscript::MessageHolderImpl::~MessageHolderImpl()
{
}

void qmscript::MessageHolderImpl::init(MessageHolder* pmh)
{
	assert(!pmh_);
	pmh_ = pmh;
}

MessageHolder* qmscript::MessageHolderImpl::getMessageHolder() const
{
	return pmh_;
}

HRESULT qmscript::MessageHolderImpl::internalQueryInterface(REFIID riid,
															void** ppv)
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
	*ppFolder = Factory::getFactory().createFolder(pmh_->getFolder());
	return S_OK;
}

STDMETHODIMP qmscript::MessageHolderImpl::get_message(IMessage** ppMessage)
{
	std::auto_ptr<MessageObj> pMessage(new MessageObj());
	if (!pmh_->getMessage(Account::GETMESSAGEFLAG_ALL,
		0, SECURITYMODE_NONE, pMessage->getMessage()))
		return E_FAIL;
	
	*ppMessage = pMessage.release();
	(*ppMessage)->AddRef();
	
	return S_OK;
}

HRESULT qmscript::MessageHolderImpl::getString(wstring_ptr (MessageHolder::*pfn)() const,
											   BSTR* pbstr)
{
	wstring_ptr wstr((pmh_->*pfn)());
	*pbstr = ::SysAllocString(wstr.get());
	return *pbstr ? S_OK : E_OUTOFMEMORY;
}


/****************************************************************************
 *
 * MessageHolderListImpl
 *
 */

qmscript::MessageHolderListImpl::MessageHolderListImpl() :
	pFolder_(0)
{
}

qmscript::MessageHolderListImpl::~MessageHolderListImpl()
{
}

void qmscript::MessageHolderListImpl::init(Folder* pFolder)
{
	assert(!pFolder_);
	pFolder_ = pFolder;
}

HRESULT qmscript::MessageHolderListImpl::internalQueryInterface(REFIID riid,
																void** ppv)
{
	BEGIN_INTERFACE_MAP()
		INTERFACE_ENTRY(IID_IMessageHolderList, IMessageHolderList)
	END_INTERFACE_MAP()
}

STDMETHODIMP qmscript::MessageHolderListImpl::get_item(unsigned int nIndex,
													   IMessageHolder** ppMessageHolder)
{
	Lock<Account> lock(*pFolder_->getAccount());
	
	if (!pFolder_->loadMessageHolders())
		return E_FAIL;
	
	if (nIndex < pFolder_->getCount()) {
		MessageHolder* pmh = pFolder_->getMessage(nIndex);
		*ppMessageHolder = Factory::getFactory().createMessageHolder(pmh);
	}
	else {
		*ppMessageHolder = 0;
	}
	
	return S_OK;
}

STDMETHODIMP qmscript::MessageHolderListImpl::get_length(unsigned int* pnLength)
{
	*pnLength = pFolder_->getCount();
	return S_OK;
}


/****************************************************************************
 *
 * MessageImpl
 *
 */

qmscript::MessageImpl::MessageImpl()
{
}

qmscript::MessageImpl::~MessageImpl()
{
}

Message* qmscript::MessageImpl::getMessage()
{
	return &msg_;
}

HRESULT qmscript::MessageImpl::internalQueryInterface(REFIID riid,
													  void** ppv)
{
	BEGIN_INTERFACE_MAP()
		INTERFACE_ENTRY(IID_IMessage, IMessage)
	END_INTERFACE_MAP()
}

STDMETHODIMP qmscript::MessageImpl::get_content(VARIANT* pvarContent)
{
	xstring_size_ptr strContent(msg_.getContent());
	if (!strContent.get())
		return E_FAIL;
	size_t nLen = strContent.size();
	
	SAFEARRAY* psa = ::SafeArrayCreateVector(VT_UI1, 0, static_cast<ULONG>(nLen));
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

STDMETHODIMP qmscript::MessageImpl::get_bodyText(BSTR bstrQuote,
												 BSTR bstrCharset,
												 VARIANT_BOOL bForceRfc822Inline,
												 BSTR* pbstrBody)
{
	const WCHAR* pwszQuote = 0;
	if (bstrQuote && *bstrQuote)
		pwszQuote = bstrQuote;
	const WCHAR* pwszCharset = 0;
	if (bstrCharset && *bstrCharset)
		pwszCharset = bstrCharset;
	
	wxstring_size_ptr wstrBody(PartUtil(msg_).getBodyText(pwszQuote, pwszCharset,
		bForceRfc822Inline == VARIANT_TRUE ? PartUtil::RFC822_INLINE : PartUtil::RFC822_AUTO));
	if (!wstrBody.get())
		return E_FAIL;
	
	*pbstrBody = ::SysAllocStringLen(wstrBody.get(), static_cast<UINT>(wstrBody.size()));
	
	return *pbstrBody ? S_OK : E_OUTOFMEMORY;
}


/****************************************************************************
 *
 * MainWindowImpl
 *
 */

qmscript::MainWindowImpl::MainWindowImpl() :
	pMainWindow_(0)
{
}

qmscript::MainWindowImpl::~MainWindowImpl()
{
}

void qmscript::MainWindowImpl::init(MainWindow* pMainWindow)
{
	assert(!pMainWindow_);
	pMainWindow_ = pMainWindow;
}

HRESULT qmscript::MainWindowImpl::internalQueryInterface(REFIID riid,
														 void** ppv)
{
	BEGIN_INTERFACE_MAP()
		INTERFACE_ENTRY(IID_IActionTarget, IActionTarget)
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

qmscript::EditFrameWindowImpl::EditFrameWindowImpl() :
	pEditFrameWindow_(0)
{
}

qmscript::EditFrameWindowImpl::~EditFrameWindowImpl()
{
}

void qmscript::EditFrameWindowImpl::init(EditFrameWindow* pEditFrameWindow)
{
	assert(!pEditFrameWindow_);
	pEditFrameWindow_ = pEditFrameWindow;
}

HRESULT qmscript::EditFrameWindowImpl::internalQueryInterface(REFIID riid,
															  void** ppv)
{
	BEGIN_INTERFACE_MAP()
		INTERFACE_ENTRY(IID_IActionTarget, IActionTarget)
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

qmscript::MessageFrameWindowImpl::MessageFrameWindowImpl() :
	pMessageFrameWindow_(0)
{
}

qmscript::MessageFrameWindowImpl::~MessageFrameWindowImpl()
{
}

void qmscript::MessageFrameWindowImpl::init(MessageFrameWindow* pMessageFrameWindow)
{
	assert(!pMessageFrameWindow_);
	pMessageFrameWindow_ = pMessageFrameWindow;
}

HRESULT qmscript::MessageFrameWindowImpl::internalQueryInterface(REFIID riid,
																 void** ppv)
{
	BEGIN_INTERFACE_MAP()
		INTERFACE_ENTRY(IID_IActionTarget, IActionTarget)
		INTERFACE_ENTRY(IID_IMessageFrameWindow, IMessageFrameWindow)
	END_INTERFACE_MAP()
}

const ActionInvoker* qmscript::MessageFrameWindowImpl::getActionInvoker() const
{
	return pMessageFrameWindow_->getActionInvoker();
}


/****************************************************************************
 *
 * ActionTargetImpl
 *
 */

qmscript::ActionTargetImpl::ActionTargetImpl() :
	pActionInvoker_(0)
{
}

qmscript::ActionTargetImpl::~ActionTargetImpl()
{
}

void qmscript::ActionTargetImpl::init(const ActionInvoker* pActionInvoker)
{
	pActionInvoker_ = pActionInvoker;
}

HRESULT qmscript::ActionTargetImpl::internalQueryInterface(REFIID riid,
													 void** ppv)
{
	BEGIN_INTERFACE_MAP()
		INTERFACE_ENTRY(IID_IActionTarget, IActionTarget)
	END_INTERFACE_MAP()
}

const ActionInvoker* qmscript::ActionTargetImpl::getActionInvoker() const
{
	return pActionInvoker_;
}


/****************************************************************************
 *
 * MacroImpl
 *
 */

qmscript::MacroImpl::MacroImpl() :
	pMacro_(0),
	pDocument_(0),
	pProfile_(0),
	hwnd_(0)
{
}

qmscript::MacroImpl::~MacroImpl()
{
	for (VariableList::iterator it = listVariable_.begin(); it != listVariable_.end(); ++it) {
		freeWString((*it).first);
		::VariantClear(&(*it).second);
	}
}

void qmscript::MacroImpl::init(std::auto_ptr<Macro> pMacro,
							   Document* pDocument,
							   Profile* pProfile,
							   HWND hwnd)
{
	assert(!pMacro_.get());
	pMacro_ = pMacro;
	pDocument_ = pDocument;
	pProfile_ = pProfile;
	hwnd_ = hwnd;
}

HRESULT qmscript::MacroImpl::internalQueryInterface(REFIID riid,
													void** ppv)
{
	BEGIN_INTERFACE_MAP()
		INTERFACE_ENTRY(IID_IMacro, IMacro)
	END_INTERFACE_MAP()
}

STDMETHODIMP qmscript::MacroImpl::evaluate(IMessageHolder* pMessageHolder,
										   IAccount* pAccount,
										   VARIANT* pvarResult)
{
	MessageHolder* pmh = 0;
	if (pMessageHolder)
		pmh = static_cast<MessageHolderObj*>(pMessageHolder)->getMessageHolder();
	Message msg;
	
	MacroVariableHolder variable;
	for (VariableList::iterator it = listVariable_.begin(); it != listVariable_.end(); ++it) {
		VARIANT v;
		::VariantInit(&v);
		HRESULT hr = ::VariantChangeType(&v, &(*it).second, 0, VT_BSTR);
		if (FAILED(hr))
			return hr;
		
		MacroValuePtr pValue(MacroValueFactory::getFactory().newString(v.bstrVal));
		variable.setVariable((*it).first, pValue);
	}
	
	Account* pAccountObj = 0;
	if (pAccount)
		pAccountObj = static_cast<AccountObj*>(pAccount)->getAccount();
	
	// TODO
	// Get selected?
	// Get current folder?
	MacroContext context(pmh, pmh ? &msg : 0, pAccountObj,
		MessageHolderList(), 0, pDocument_, 0, hwnd_, pProfile_, 0,
		MacroContext::FLAG_UI, SECURITYMODE_NONE, 0, &variable);
	MacroValuePtr pValue(pMacro_->value(&context));
	if (!pValue.get())
		return E_FAIL;
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
			MacroValue::String wstrValue(pValue->string());
			pvarResult->vt = VT_BSTR;
			pvarResult->bstrVal = ::SysAllocString(wstrValue.get());
			if (!pvarResult->bstrVal)
				return E_OUTOFMEMORY;
		}
		break;
	}
	
	return S_OK;
}

STDMETHODIMP qmscript::MacroImpl::setVariable(BSTR bstrName,
											  VARIANT var)
{
	VariableList::iterator it = std::find_if(
		listVariable_.begin(), listVariable_.end(),
		boost::bind(string_equal<WCHAR>(),
			boost::bind(&VariableList::value_type::first, _1), bstrName));
	if (it != listVariable_.end()) {
		::VariantClear(&(*it).second);
		HRESULT hr = ::VariantCopy(&(*it).second, &var);
		if (FAILED(hr))
			return hr;
	}
	else {
		wstring_ptr wstrName(allocWString(bstrName));
		
		VARIANT v;
		::VariantInit(&v);
		HRESULT hr = ::VariantCopy(&v, &var);
		if (FAILED(hr))
			return hr;
		
		struct Deleter
		{
			Deleter(VARIANT& v) :
				p_(&v)
			{
			}
			
			~Deleter()
			{
				if (p_)
					::VariantClear(p_);
			}
			
			void release()
			{
				p_ = 0;
			}
			
			VARIANT* p_;
		} deleter(v);
		
		listVariable_.push_back(std::make_pair(wstrName.get(), v));
		wstrName.release();
		deleter.release();
	}
	
	return S_OK;
}

STDMETHODIMP qmscript::MacroImpl::getVariable(BSTR bstrName,
											  VARIANT* pVar)
{
	::VariantInit(pVar);
	
	VariableList::iterator it = std::find_if(
		listVariable_.begin(), listVariable_.end(),
		boost::bind(string_equal<WCHAR>(),
			boost::bind(&VariableList::value_type::first, _1), bstrName));
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
		boost::bind(string_equal<WCHAR>(),
			boost::bind(&VariableList::value_type::first, _1), bstrName));
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

qmscript::MacroParserImpl::MacroParserImpl() :
	pDocument_(0),
	pProfile_(0),
	hwnd_(0)
{
}

qmscript::MacroParserImpl::~MacroParserImpl()
{
}

void qmscript::MacroParserImpl::init(Document* pDocument,
									 Profile* pProfile,
									 HWND hwnd)
{
	assert(!pDocument_);
	pDocument_ = pDocument;
	pProfile_ = pProfile;
	hwnd_ = hwnd;
}

HRESULT qmscript::MacroParserImpl::internalQueryInterface(REFIID riid,
														  void** ppv)
{
	BEGIN_INTERFACE_MAP()
		INTERFACE_ENTRY(IID_IMacroParser, IMacroParser)
	END_INTERFACE_MAP()
}

STDMETHODIMP qmscript::MacroParserImpl::parse(BSTR bstrMacro,
											  IMacro** ppMacro)
{
	std::auto_ptr<Macro> pMacro(MacroParser().parse(bstrMacro));
	
	std::auto_ptr<MacroObj> pMacroObj(new MacroObj());
	pMacroObj->init(pMacro, pDocument_, pProfile_, hwnd_);
	
	*ppMacro = pMacroObj.release();
	(*ppMacro)->AddRef();
	
	return S_OK;
}


/****************************************************************************
 *
 * ArgumentListImpl
 *
 */

qmscript::ArgumentListImpl::ArgumentListImpl()
{
}

qmscript::ArgumentListImpl::~ArgumentListImpl()
{
	for (ArgumentList::iterator it = listArgument_.begin(); it != listArgument_.end(); ++it)
		::VariantClear(&(*it));
}

bool qmscript::ArgumentListImpl::init(VARIANT* pvar,
									  size_t nCount)
{
	assert(listArgument_.empty());
	
	listArgument_.resize(nCount);
	Variant v;
	std::fill(listArgument_.begin(), listArgument_.end(), v);
	
	for (size_t n = 0; n < nCount; ++n) {
		HRESULT hr = ::VariantCopy(&listArgument_[n], pvar + n);
		if (FAILED(hr))
			return false;
	}
	
	return true;
}

HRESULT qmscript::ArgumentListImpl::internalQueryInterface(REFIID riid,
														   void** ppv)
{
	BEGIN_INTERFACE_MAP()
		INTERFACE_ENTRY(IID_IArgumentList, IArgumentList)
	END_INTERFACE_MAP()
}

STDMETHODIMP qmscript::ArgumentListImpl::get_item(unsigned int nIndex,
												  VARIANT* pvarArg)
{
	if (nIndex >= listArgument_.size())
		return E_INVALIDARG;
	return ::VariantCopy(pvarArg, &listArgument_[nIndex]);
}

STDMETHODIMP qmscript::ArgumentListImpl::get_length(unsigned int* pnLength)
{
	*pnLength = static_cast<unsigned int>(listArgument_.size());
	return S_OK;
}

STDMETHODIMP qmscript::ArgumentListImpl::_newEnum(IUnknown** ppUnk)
{
	typedef Object<EnumBase<IEnumVARIANT, &IID_IEnumVARIANT, VARIANT> > EnumObj;
	std::auto_ptr<EnumObj> pEnum(new EnumObj());
	if (!pEnum->init(&listArgument_[0], listArgument_.size()))
		return E_FAIL;
	
	*ppUnk = pEnum.release();
	(*ppUnk)->AddRef();
	
	return S_OK;
}


/****************************************************************************
 *
 * ResultImpl
 *
 */

qmscript::ResultImpl::ResultImpl()
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

HRESULT qmscript::ResultImpl::internalQueryInterface(REFIID riid,
													 void** ppv)
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

IApplication* qmscript::Factory::createApplication(Application* pApplication)
{
	std::auto_ptr<ApplicationObj> pApplicationObj(new ApplicationObj());
	pApplicationObj->init(pApplication);
	
	IApplication* p = pApplicationObj.release();
	p->AddRef();
	
	return p;
}

IDocument* qmscript::Factory::createDocument(Document* pDocument)
{
	std::auto_ptr<DocumentObj> pDocumentObj(new DocumentObj());
	pDocumentObj->init(pDocument);
	
	IDocument* p = pDocumentObj.release();
	p->AddRef();
	
	return p;
}

IAccount* qmscript::Factory::createAccount(Account* pAccount)
{
	assert(pAccount);
	
	std::auto_ptr<AccountObj> pAccountObj(new AccountObj());
	pAccountObj->init(pAccount);
	
	IAccount* p = pAccountObj.release();
	p->AddRef();
	
	return p;
}

IFolder* qmscript::Factory::createFolder(Folder* pFolder)
{
	assert(pFolder);
	
	IFolder* p = 0;
	
	switch (pFolder->getType()) {
	case Folder::TYPE_NORMAL:
		{
			std::auto_ptr<NormalFolderObj> pFolderObj(new NormalFolderObj());
			pFolderObj->init(static_cast<NormalFolder*>(pFolder));
			p = pFolderObj.release();
			p->AddRef();
		}
		break;
	case Folder::TYPE_QUERY:
		{
			std::auto_ptr<QueryFolderObj> pFolderObj(new QueryFolderObj());
			pFolderObj->init(static_cast<QueryFolder*>(pFolder));
			p = pFolderObj.release();
			p->AddRef();
		}
		break;
	default:
		assert(false);
		break;
	}
	
	return p;
}

IMessageHolder* qmscript::Factory::createMessageHolder(MessageHolder* pmh)
{
	assert(pmh);
	
	std::auto_ptr<MessageHolderObj> pMessageHolderObj(new MessageHolderObj());
	pMessageHolderObj->init(pmh);
	
	IMessageHolder* p = pMessageHolderObj.release();
	p->AddRef();
	
	return p;
}

Factory& qmscript::Factory::getFactory()
{
	return factory__;
}
