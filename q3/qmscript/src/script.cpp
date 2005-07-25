/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsosutil.h>

#include "macro.h"
#include "obj.h"
#include "script.h"

using namespace qmscript;
using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * ScriptImpl
 *
 */

qmscript::ScriptImpl::ScriptImpl(const ScriptFactory::Init& init) :
	pActiveScript_(0)
{
	load(init);
}

qmscript::ScriptImpl::~ScriptImpl()
{
	if (pActiveScript_) {
		pActiveScript_->Close();
		pActiveScript_->Release();
	}
}

bool qmscript::ScriptImpl::operator!() const
{
	return !pActiveScript_;
}

bool qmscript::ScriptImpl::run(VARIANT* pvarArgs,
							   size_t nArgCount,
							   VARIANT* pvarResult)
{
	ComPtr<IActiveScriptSite> pSite;
	HRESULT hr = pActiveScript_->GetScriptSite(
		IID_IActiveScriptSite, reinterpret_cast<void**>(&pSite));
	if (FAILED(hr))
		return false;
	
	if (pvarArgs) {
		if (!static_cast<ActiveScriptSite*>(
			pSite.get())->setArguments(pvarArgs, nArgCount))
			return false;
	}
	
	hr = pActiveScript_->SetScriptState(SCRIPTSTATE_CONNECTED);
	if (FAILED(hr))
		return false;
	
	if (pvarResult) {
		VARIANT* pvar = static_cast<ActiveScriptSite*>(
			pSite.get())->getResult();
		hr = ::VariantCopy(pvarResult, pvar);
		if (FAILED(hr))
			return false;
	}
	
	return true;
}

bool qmscript::ScriptImpl::load(const ScriptFactory::Init& init)
{
	CLSID clsid;
	HRESULT hr = ::CLSIDFromProgID(init.pwszLanguage_, &clsid);
	if (FAILED(hr))
		return false;
	
	ComPtr<IActiveScript> pActiveScript;
	hr = ::CoCreateInstance(clsid, 0, CLSCTX_INPROC_SERVER,
		IID_IActiveScript, reinterpret_cast<void**>(&pActiveScript));
	if (FAILED(hr))
		return false;
	
	ComPtr<IActiveScriptParse> pParse;
	
	ComPtr<IPersistStreamInit> pPersistStreamInit;
	hr = pActiveScript->QueryInterface(IID_IPersistStreamInit,
		reinterpret_cast<void**>(&pPersistStreamInit));
	if (hr == E_NOINTERFACE) {
		hr = pActiveScript->QueryInterface(IID_IActiveScriptParse,
			reinterpret_cast<void**>(&pParse));
		if (FAILED(hr))
			return false;
		
		hr = pParse->InitNew();
		if (FAILED(hr))
			return false;
	}
	else if (SUCCEEDED(hr)) {
		ReaderIStream stream(init.pReader_);
		hr = pPersistStreamInit->Load(&stream);
		if (FAILED(hr))
			return false;
	}
	else {
		return false;
	}
	
	std::auto_ptr<ActiveScriptSite> pSite(new ActiveScriptSite(init));
	hr = pActiveScript->SetScriptSite(pSite.get());
	if (FAILED(hr))
		return false;
	pSite.release();
	
	if (pParse.get()) {
		StringBuffer<WSTRING> buf;
		WCHAR wsz[1024];
		while (true) {
			size_t nRead = init.pReader_->read(wsz, countof(wsz));
			if (nRead == -1)
				return false;
			else if (nRead == 0)
				break;
			buf.append(wsz, nRead);
		}
		EXCEPINFO ei = { 0 };
		hr = pParse->ParseScriptText(buf.getCharArray(), 0, 0, 0, 0, 0,
			SCRIPTTEXT_ISPERSISTENT | SCRIPTTEXT_ISVISIBLE, 0, &ei);
		if (FAILED(hr))
			return false;
	}
	
	struct {
		const WCHAR* pwszName_;
		bool b_;
	} names[] = {
		{ L"application",			true									},
		{ L"document",				true									},
		{ L"mainWindow",			init.window_.pMainWindow_ != 0			},
		{ L"editFrameWindow",		init.window_.pEditFrameWindow_ != 0		},
		{ L"messageFrameWindow",	init.window_.pMessageFrameWindow_ != 0	},
		{ L"macroParser",			true									},
		{ L"arguments",				true									},
		{ L"result",				true									}
	};
	for (int n = 0; n < countof(names); ++n) {
		if (names[n].b_) {
			hr = pActiveScript->AddNamedItem(names[n].pwszName_,
				SCRIPTITEM_ISSOURCE | SCRIPTITEM_ISVISIBLE);
			if (FAILED(hr))
				return false;
		}
	}
	
	pActiveScript_ = pActiveScript.release();
	
	return true;
}


/****************************************************************************
 *
 * ScriptFactoryImpl
 *
 */

ScriptFactoryImpl qmscript::ScriptFactoryImpl::factory__;

qmscript::ScriptFactoryImpl::ScriptFactoryImpl()
{
	registerFactory(this);
}

qmscript::ScriptFactoryImpl::~ScriptFactoryImpl()
{
	unregisterFactory(this);
}

std::auto_ptr<Script> qmscript::ScriptFactoryImpl::createScript(const Init& init)
{
	assert(init.pwszLanguage_);
	assert(init.pReader_);
	assert(init.pDocument_);
	assert(init.pProfile_);
	assert(init.pModalHandler_);
	
	std::auto_ptr<ScriptImpl> pScript(new ScriptImpl(init));
	if (!*pScript.get())
		return std::auto_ptr<Script>();
	return pScript;
}


/****************************************************************************
 *
 * ActiveScriptSite
 *
 */

qmscript::ActiveScriptSite::ActiveScriptSite(const ScriptFactory::Init& init) :
	nRef_(0),
	hwnd_(init.hwnd_),
	pModalHandler_(init.pModalHandler_),
	pApplication_(0),
	pDocument_(0),
	pMacroParser_(0),
	pArgumentList_(0),
	pResult_(0),
	pMainWindow_(0),
	pEditFrameWindow_(0),
	pMessageFrameWindow_(0)
{
	ComPtr<IApplication> pApplicationObj(
		Factory::getFactory().createApplication(&Application::getApplication()));
	ComPtr<IDocument> pDocumentObj(Factory::getFactory().createDocument(init.pDocument_));
	
	std::auto_ptr<MacroParserObj> pMacroParserObj(new MacroParserObj());
	pMacroParserObj->init(init.pDocument_, init.pProfile_, init.hwnd_);
	
	std::auto_ptr<ArgumentListObj> pArgumentListObj(new ArgumentListObj());
	
	std::auto_ptr<ResultObj> pResultObj(new ResultObj());
	
	std::auto_ptr<MainWindowObj> pMainWindowObj;
	std::auto_ptr<EditFrameWindowObj> pEditFrameWindowObj;
	std::auto_ptr<MessageFrameWindowObj> pMessageFrameWindowObj;
	switch (init.type_) {
	case ScriptFactory::TYPE_NONE:
		break;
	case ScriptFactory::TYPE_MAIN:
		pMainWindowObj.reset(new MainWindowObj());
		pMainWindowObj->init(init.window_.pMainWindow_);
		break;
	case ScriptFactory::TYPE_EDIT:
		pEditFrameWindowObj.reset(new EditFrameWindowObj());
		pEditFrameWindowObj->init(init.window_.pEditFrameWindow_);
		break;
	case ScriptFactory::TYPE_MESSAGE:
		pMessageFrameWindowObj.reset(new MessageFrameWindowObj());
		pMessageFrameWindowObj->init(init.window_.pMessageFrameWindow_);
		break;
	default:
		assert(false);
		break;
	}
	
	pApplication_ = pApplicationObj.release();
	pDocument_ = pDocumentObj.release();
	pMacroParser_ = pMacroParserObj.release();
	pMacroParser_->AddRef();
	pArgumentList_ = pArgumentListObj.release();
	pArgumentList_->AddRef();
	pResult_ = pResultObj.release();
	pResult_->AddRef();
	if (pMainWindowObj.get()) {
		pMainWindow_ = pMainWindowObj.release();
		pMainWindow_->AddRef();
	}
	if (pEditFrameWindowObj.get()) {
		pEditFrameWindow_ = pEditFrameWindowObj.release();
		pEditFrameWindow_->AddRef();
	}
	if (pMessageFrameWindowObj.get()) {
		pMessageFrameWindow_ = pMessageFrameWindowObj.release();
		pMessageFrameWindow_->AddRef();
	}
}

qmscript::ActiveScriptSite::~ActiveScriptSite()
{
	if (pApplication_)
		pApplication_->Release();
	if (pDocument_)
		pDocument_->Release();
	if (pMainWindow_)
		pMainWindow_->Release();
	if (pMacroParser_)
		pMacroParser_->Release();
	if (pArgumentList_)
		pArgumentList_->Release();
	if (pResult_)
		pResult_->Release();
}

bool qmscript::ActiveScriptSite::setArguments(VARIANT* pvarArgs,
											  size_t nCount)
{
	return static_cast<ArgumentListObj*>(pArgumentList_)->init(pvarArgs, nCount);
}

VARIANT* qmscript::ActiveScriptSite::getResult()
{
	return static_cast<ResultObj*>(pResult_)->getValue();
}

STDMETHODIMP qmscript::ActiveScriptSite::QueryInterface(REFIID riid,
														void** ppv)
{
	*ppv = 0;
	
	if (riid == IID_IUnknown || riid == IID_IActiveScriptSite)
		*ppv = static_cast<IActiveScriptSite*>(this);
	else if (riid == IID_IActiveScriptSiteWindow && hwnd_)
		*ppv = static_cast<IActiveScriptSiteWindow*>(this);
	else
		return E_NOINTERFACE;
	AddRef();
	
	return S_OK;
}

STDMETHODIMP_(ULONG) qmscript::ActiveScriptSite::AddRef()
{
	return ::InterlockedIncrement(reinterpret_cast<LONG*>(&nRef_));
}

STDMETHODIMP_(ULONG) qmscript::ActiveScriptSite::Release()
{
	ULONG nRef = ::InterlockedDecrement(reinterpret_cast<LONG*>(&nRef_));
	if (nRef == 0)
		delete this;
	return nRef;
}

STDMETHODIMP qmscript::ActiveScriptSite::GetLCID(LCID* plcid)
{
	return E_NOTIMPL;
}

STDMETHODIMP qmscript::ActiveScriptSite::GetItemInfo(LPCOLESTR pwszName,
													 DWORD dwReturnMask,
													 IUnknown** ppUnkItem,
													 ITypeInfo** ppTypeInfo)
{
	if (dwReturnMask & SCRIPTINFO_IUNKNOWN)
		*ppUnkItem = 0;
	if (dwReturnMask & SCRIPTINFO_ITYPEINFO)
		*ppTypeInfo = 0;
	
	HRESULT hr = S_OK;
	
	struct {
		const WCHAR* pwszName_;
		IDispatch* pDisp_;
	} items[] = {
		{ L"application",			pApplication_			},
		{ L"document",				pDocument_				},
		{ L"macroParser",			pMacroParser_			},
		{ L"arguments",				pArgumentList_			},
		{ L"result",				pResult_				},
		{ L"mainWindow",			pMainWindow_			},
		{ L"editFrameWindow",		pEditFrameWindow_		},
		{ L"messageFrameWindow",	pMessageFrameWindow_	},
	};
	for (int n = 0; n < countof(items); ++n) {
		if (::wcscmp(pwszName, items[n].pwszName_) == 0 && items[n].pDisp_) {
			if (dwReturnMask & SCRIPTINFO_ITYPEINFO) {
				hr = items[n].pDisp_->GetTypeInfo(0, 9, ppTypeInfo);
				if (FAILED(hr))
					return hr;
			}
			
			if (dwReturnMask & SCRIPTINFO_IUNKNOWN) {
				*ppUnkItem = items[n].pDisp_;
				(*ppUnkItem)->AddRef();
			}
			
			return S_OK;
		}
	}
	
	return TYPE_E_ELEMENTNOTFOUND;
}

STDMETHODIMP qmscript::ActiveScriptSite::GetDocVersionString(BSTR* pbstrVersionString)
{
	return E_NOTIMPL;
}

STDMETHODIMP qmscript::ActiveScriptSite::OnScriptTerminate(const VARIANT* pvarResult,
														   const EXCEPINFO* pExecpInfo)
{
	return S_OK;
}

STDMETHODIMP qmscript::ActiveScriptSite::OnStateChange(SCRIPTSTATE state)
{
	return S_OK;
}

STDMETHODIMP qmscript::ActiveScriptSite::OnScriptError(IActiveScriptError* pError)
{
	DWORD dwContext = 0;
	ULONG nLine = 0;
	LONG nChar = 0;
	HRESULT hr = pError->GetSourcePosition(&dwContext, &nLine, &nChar);
	if (FAILED(hr))
		return hr;
	
	BSTRPtr bstrLine;
	pError->GetSourceLineText(&bstrLine);
	
	EXCEPINFO info = { 0 };
	pError->GetExceptionInfo(&info);
	
	BSTRPtr bstrSource(info.bstrSource);
	BSTRPtr bstrDescription(info.bstrDescription);
	BSTRPtr bstrHelpFile(info.bstrHelpFile);
	
	StringBuffer<WSTRING> buf;
	if (bstrSource.get()) {
		buf.append(bstrSource.get());
		buf.append(L"\n");
	}
	if (bstrDescription.get()) {
		buf.append(bstrDescription.get());
		buf.append(L"\n");
	}
	WCHAR wsz[32];
	swprintf(wsz, L"%d:%d ", nLine, nChar);
	buf.append(wsz);
	if (bstrLine.get())
		buf.append(bstrLine.get());
	
	messageBox(buf.getCharArray(), MB_OK | MB_ICONERROR, hwnd_, 0, 0);
	
	return S_OK;
}

STDMETHODIMP qmscript::ActiveScriptSite::OnEnterScript()
{
	return S_OK;
}

STDMETHODIMP qmscript::ActiveScriptSite::OnLeaveScript()
{
	return S_OK;
}

STDMETHODIMP qmscript::ActiveScriptSite::GetWindow(HWND* phwnd)
{
	*phwnd = hwnd_;
	return S_OK;
}

STDMETHODIMP qmscript::ActiveScriptSite::EnableModeless(BOOL bEnable)
{
	if (bEnable)
		pModalHandler_->postModalDialog(0);
	else
		pModalHandler_->preModalDialog(0);
	
	return S_OK;
}


/****************************************************************************
 *
 * ReaderIStream
 *
 */

qmscript::ReaderIStream::ReaderIStream(Reader* pReader) :
	pReader_(pReader)
{
}

qmscript::ReaderIStream::~ReaderIStream()
{
}

STDMETHODIMP qmscript::ReaderIStream::QueryInterface(REFIID riid,
													 void** ppv)
{
	return E_NOTIMPL;
}

STDMETHODIMP_(ULONG) qmscript::ReaderIStream::AddRef()
{
	return E_NOTIMPL;
}

STDMETHODIMP_(ULONG) qmscript::ReaderIStream::Release()
{
	return E_NOTIMPL;
}

STDMETHODIMP qmscript::ReaderIStream::Read(void* pv,
										   ULONG cb,
										   ULONG* pcbRead)
{
	size_t nRead = pReader_->read(static_cast<WCHAR*>(pv), cb/sizeof(WCHAR));
	if (nRead == -1)
		return E_FAIL;
	
	*pcbRead = static_cast<ULONG>(nRead*sizeof(WCHAR));
	
	return S_OK;
}

STDMETHODIMP qmscript::ReaderIStream::Write(const void* pv,
											ULONG cb,
											ULONG* pcbWritten)
{
	return E_NOTIMPL;
}

STDMETHODIMP qmscript::ReaderIStream::Seek(LARGE_INTEGER nMove,
										   DWORD dwOrigin,
										   ULARGE_INTEGER* pnPosition)
{
	return E_NOTIMPL;
}

STDMETHODIMP qmscript::ReaderIStream::SetSize(ULARGE_INTEGER nSize)
{
	return E_NOTIMPL;
}

STDMETHODIMP qmscript::ReaderIStream::CopyTo(IStream* pStream,
											 ULARGE_INTEGER cb,
											 ULARGE_INTEGER* pcbRead,
											 ULARGE_INTEGER* pcbWritten)
{
	return E_NOTIMPL;
}

STDMETHODIMP qmscript::ReaderIStream::Commit(DWORD dwFlags)
{
	return E_NOTIMPL;
}

STDMETHODIMP qmscript::ReaderIStream::Revert()
{
	return E_NOTIMPL;
}

STDMETHODIMP qmscript::ReaderIStream::LockRegion(ULARGE_INTEGER nOffset,
												 ULARGE_INTEGER cb,
												 DWORD dwType)
{
	return E_NOTIMPL;
}

STDMETHODIMP qmscript::ReaderIStream::UnlockRegion(ULARGE_INTEGER nOffset,
												   ULARGE_INTEGER cb,
												   DWORD dwType)
{
	return E_NOTIMPL;
}

STDMETHODIMP qmscript::ReaderIStream::Stat(STATSTG* pStatStg,
										   DWORD dwFlags)
{
	return E_NOTIMPL;
}

STDMETHODIMP qmscript::ReaderIStream::Clone(IStream** ppStream)
{
	return E_NOTIMPL;
}
