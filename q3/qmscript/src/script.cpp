/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qserror.h>
#include <qsnew.h>
#include <qsosutil.h>

#include "macro.h"
#include "obj.h"
#include "script.h"

using namespace qmscript;
using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * Script
 *
 */

qmscript::Script::~Script()
{
}


/****************************************************************************
 *
 * ScriptFactory
 *
 */

qmscript::ScriptFactory::~ScriptFactory()
{
}


/****************************************************************************
 *
 * Global functions
 *
 */

extern "C" ScriptFactory* newScriptFactory()
{
	DECLARE_QSTATUS();
	
	std::auto_ptr<ScriptFactoryImpl> pFactory;
	status = newQsObject(&pFactory);
	CHECK_QSTATUS_VALUE(0);
	
	return pFactory.release();
}

extern "C" void deleteScriptFactory(ScriptFactory* pFactory)
{
	delete pFactory;
}


/****************************************************************************
 *
 * ScriptImpl
 *
 */

qmscript::ScriptImpl::ScriptImpl(
	const ScriptFactory::Init& init, QSTATUS* pstatus) :
	pActiveScript_(0)
{
	DECLARE_QSTATUS();
	
	status = load(init);
	CHECK_QSTATUS_SET(pstatus);
}

qmscript::ScriptImpl::~ScriptImpl()
{
	if (pActiveScript_) {
		pActiveScript_->Close();
		pActiveScript_->Release();
	}
}

QSTATUS qmscript::ScriptImpl::run(VARIANT* pvarArgs,
	size_t nArgCount, VARIANT* pvarResult)
{
	DECLARE_QSTATUS();
	
	ComPtr<IActiveScriptSite> pSite;
	HRESULT hr = pActiveScript_->GetScriptSite(IID_IActiveScriptSite,
		reinterpret_cast<void**>(&pSite));
	CHECK_HRESULT();
	
	if (pvarArgs) {
		status = static_cast<ActiveScriptSite*>(
			pSite.get())->setArguments(pvarArgs, nArgCount);
		CHECK_QSTATUS();
	}
	
	hr = pActiveScript_->SetScriptState(SCRIPTSTATE_CONNECTED);
	CHECK_HRESULT();
	
	if (pvarResult) {
		VARIANT* pvar = static_cast<ActiveScriptSite*>(
			pSite.get())->getResult();
		hr = ::VariantCopy(pvarResult, pvar);
		CHECK_HRESULT();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmscript::ScriptImpl::load(const ScriptFactory::Init& init)
{
	DECLARE_QSTATUS();
	
	CLSID clsid;
	HRESULT hr = ::CLSIDFromProgID(init.pwszLanguage_, &clsid);
	CHECK_HRESULT();
	
	ComPtr<IActiveScript> pActiveScript;
	hr = ::CoCreateInstance(clsid, 0, CLSCTX_INPROC_SERVER,
		IID_IActiveScript, reinterpret_cast<void**>(&pActiveScript));
	CHECK_HRESULT();
	
	ComPtr<IActiveScriptParse> pParse;
	
	ComPtr<IPersistStreamInit> pPersistStreamInit;
	hr = pActiveScript->QueryInterface(IID_IPersistStreamInit,
		reinterpret_cast<void**>(&pPersistStreamInit));
	if (hr == E_NOINTERFACE) {
		hr = pActiveScript->QueryInterface(IID_IActiveScriptParse,
			reinterpret_cast<void**>(&pParse));
		CHECK_HRESULT();
		
		hr = pParse->InitNew();
		CHECK_HRESULT();
	}
	else {
		CHECK_HRESULT();
		
		ReaderIStream stream(init.pReader_, &status);
		CHECK_QSTATUS();
		hr = pPersistStreamInit->Load(&stream);
		CHECK_HRESULT();
	}
	
	std::auto_ptr<ActiveScriptSite> pSite;
	status = newQsObject(init, &pSite);
	CHECK_QSTATUS();
	hr = pActiveScript->SetScriptSite(pSite.get());
	CHECK_HRESULT();
	pSite.release();
	
	if (pParse.get()) {
		StringBuffer<WSTRING> buf(&status);
		CHECK_QSTATUS();
		WCHAR wsz[1024];
		while (true) {
			size_t nRead = 0;
			status = init.pReader_->read(wsz, countof(wsz), &nRead);
			CHECK_QSTATUS();
			if (nRead == -1)
				break;
			status = buf.append(wsz, nRead);
			CHECK_QSTATUS();
		}
		EXCEPINFO ei = { 0 };
		hr = pParse->ParseScriptText(buf.getCharArray(), 0, 0, 0, 0, 0,
			SCRIPTTEXT_ISPERSISTENT | SCRIPTTEXT_ISVISIBLE, 0, &ei);
		CHECK_HRESULT();
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
			CHECK_HRESULT();
		}
	}
	
	pActiveScript_ = pActiveScript.release();
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * ScriptFactoryImpl
 *
 */

qmscript::ScriptFactoryImpl::ScriptFactoryImpl(QSTATUS* pstatus)
{
}

qmscript::ScriptFactoryImpl::~ScriptFactoryImpl()
{
}

QSTATUS qmscript::ScriptFactoryImpl::newScript(
	const Init& init, Script** ppScript)
{
	assert(init.pwszLanguage_);
	assert(init.pReader_);
	assert(init.pDocument_);
	assert(init.pProfile_);
	assert(init.hwnd_);
	assert(init.pModalHandler_);
	assert(ppScript);
	
	DECLARE_QSTATUS();
	
	*ppScript = 0;
	
	std::auto_ptr<ScriptImpl> pScript;
	status = newQsObject(init, &pScript);
	CHECK_QSTATUS();
	
	*ppScript = pScript.release();
	
	return QSTATUS_SUCCESS;
}

void qmscript::ScriptFactoryImpl::deleteScript(Script* pScript)
{
	delete pScript;
}


/****************************************************************************
 *
 * ActiveScriptSite
 *
 */

qmscript::ActiveScriptSite::ActiveScriptSite(
	const ScriptFactory::Init& init, QSTATUS* pstatus) :
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
	DECLARE_QSTATUS();
	
	ComPtr<IApplication> pApplicationObj;
	status = Factory::getFactory().createApplication(
		&Application::getApplication(), &pApplicationObj);
	CHECK_QSTATUS_SET(pstatus);
	
	ComPtr<IDocument> pDocumentObj;
	status = Factory::getFactory().createDocument(
		init.pDocument_, &pDocumentObj);
	CHECK_QSTATUS_SET(pstatus);
	
	std::auto_ptr<MacroParserObj> pMacroParserObj;
	status = newQsObject(&pMacroParserObj);
	CHECK_QSTATUS_SET(pstatus);
	status = pMacroParserObj->init(
		init.pDocument_, init.pProfile_, init.hwnd_);
	CHECK_QSTATUS_SET(pstatus);
	
	std::auto_ptr<ArgumentListObj> pArgumentListObj;
	status = newQsObject(&pArgumentListObj);
	CHECK_QSTATUS_SET(pstatus);
	
	std::auto_ptr<ResultObj> pResultObj;
	status = newQsObject(&pResultObj);
	CHECK_QSTATUS_SET(pstatus);
	
	std::auto_ptr<MainWindowObj> pMainWindowObj;
	std::auto_ptr<EditFrameWindowObj> pEditFrameWindowObj;
	std::auto_ptr<MessageFrameWindowObj> pMessageFrameWindowObj;
	switch (init.type_) {
	case ScriptFactory::TYPE_NONE:
		break;
	case ScriptFactory::TYPE_MAIN:
		status = newQsObject(&pMainWindowObj);
		CHECK_QSTATUS_SET(pstatus);
		status = pMainWindowObj->init(init.window_.pMainWindow_);
		CHECK_QSTATUS_SET(pstatus);
		break;
	case ScriptFactory::TYPE_EDIT:
		status = newQsObject(&pEditFrameWindowObj);
		CHECK_QSTATUS_SET(pstatus);
		status = pEditFrameWindowObj->init(init.window_.pEditFrameWindow_);
		CHECK_QSTATUS_SET(pstatus);
		break;
	case ScriptFactory::TYPE_MESSAGE:
		status = newQsObject(&pMessageFrameWindowObj);
		CHECK_QSTATUS_SET(pstatus);
		status = pMessageFrameWindowObj->init(init.window_.pMessageFrameWindow_);
		CHECK_QSTATUS_SET(pstatus);
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

QSTATUS qmscript::ActiveScriptSite::setArguments(VARIANT* pvarArgs, size_t nCount)
{
	return static_cast<ArgumentListObj*>(pArgumentList_)->init(pvarArgs, nCount);
}

VARIANT* qmscript::ActiveScriptSite::getResult()
{
	return static_cast<ResultObj*>(pResult_)->getValue();
}

STDMETHODIMP qmscript::ActiveScriptSite::QueryInterface(
	REFIID riid, void** ppv)
{
	*ppv = 0;
	
	if (riid == IID_IUnknown || riid == IID_IActiveScriptSite)
		*ppv = static_cast<IActiveScriptSite*>(this);
	else if (riid == IID_IActiveScriptSiteWindow)
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

STDMETHODIMP qmscript::ActiveScriptSite::GetItemInfo(
	LPCOLESTR pwszName, DWORD dwReturnMask,
	IUnknown** ppUnkItem, ITypeInfo** ppTypeInfo)
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

STDMETHODIMP qmscript::ActiveScriptSite::GetDocVersionString(
	BSTR* pbstrVersionString)
{
	return E_NOTIMPL;
}

STDMETHODIMP qmscript::ActiveScriptSite::OnScriptTerminate(
	const VARIANT* pvarResult, const EXCEPINFO* pExecpInfo)
{
	return S_OK;
}

STDMETHODIMP qmscript::ActiveScriptSite::OnStateChange(SCRIPTSTATE state)
{
	return S_OK;
}

STDMETHODIMP qmscript::ActiveScriptSite::OnScriptError(
	IActiveScriptError* pError)
{
	DECLARE_QSTATUS();
	
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
	
	StringBuffer<WSTRING> buf(&status);
	CHECK_QSTATUS_HRESULT();
	if (bstrSource.get()) {
		status = buf.append(bstrSource.get());
		CHECK_QSTATUS_HRESULT();
		status = buf.append(L"\n");
		CHECK_QSTATUS_HRESULT();
	}
	if (bstrDescription.get()) {
		status = buf.append(bstrDescription.get());
		CHECK_QSTATUS_HRESULT();
		status = buf.append(L"\n");
		CHECK_QSTATUS_HRESULT();
	}
	WCHAR wsz[32];
	swprintf(wsz, L"%d:%d ", nLine, nChar);
	status = buf.append(wsz);
	CHECK_QSTATUS_HRESULT();
	if (bstrLine.get()) {
		status = buf.append(bstrLine.get());
		CHECK_QSTATUS_HRESULT();
	}
	
	int nRet = 0;
	messageBox(buf.getCharArray(), MB_OK | MB_ICONERROR, hwnd_, 0, 0, &nRet);
	
	return E_NOTIMPL;
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

qmscript::ReaderIStream::ReaderIStream(Reader* pReader, QSTATUS* pstatus) :
	pReader_(pReader)
{
}

qmscript::ReaderIStream::~ReaderIStream()
{
}

STDMETHODIMP qmscript::ReaderIStream::QueryInterface(REFIID riid, void** ppv)
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

STDMETHODIMP qmscript::ReaderIStream::Read(void* pv, ULONG cb, ULONG* pcbRead)
{
	DECLARE_QSTATUS();
	
	size_t nRead = 0;
	status = pReader_->read(static_cast<WCHAR*>(pv),
		cb/sizeof(WCHAR), &nRead);
	if (status == QSTATUS_OUTOFMEMORY)
		return E_OUTOFMEMORY;
	else if (status != QSTATUS_SUCCESS)
		return E_FAIL;
	
	*pcbRead = nRead*sizeof(WCHAR);
	
	return S_OK;
}

STDMETHODIMP qmscript::ReaderIStream::Write(
	const void* pv, ULONG cb, ULONG* pcbWritten)
{
	return E_NOTIMPL;
}

STDMETHODIMP qmscript::ReaderIStream::Seek(LARGE_INTEGER nMove,
	DWORD dwOrigin, ULARGE_INTEGER* pnPosition)
{
	return E_NOTIMPL;
}

STDMETHODIMP qmscript::ReaderIStream::SetSize(ULARGE_INTEGER nSize)
{
	return E_NOTIMPL;
}

STDMETHODIMP qmscript::ReaderIStream::CopyTo(IStream* pStream, ULARGE_INTEGER cb,
	ULARGE_INTEGER* pcbRead, ULARGE_INTEGER* pcbWritten)
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

STDMETHODIMP qmscript::ReaderIStream::LockRegion(
	ULARGE_INTEGER nOffset, ULARGE_INTEGER cb, DWORD dwType)
{
	return E_NOTIMPL;
}

STDMETHODIMP qmscript::ReaderIStream::UnlockRegion(
	ULARGE_INTEGER nOffset, ULARGE_INTEGER cb, DWORD dwType)
{
	return E_NOTIMPL;
}

STDMETHODIMP qmscript::ReaderIStream::Stat(STATSTG* pStatStg, DWORD dwFlags)
{
	return E_NOTIMPL;
}

STDMETHODIMP qmscript::ReaderIStream::Clone(IStream** ppStream)
{
	return E_NOTIMPL;
}
