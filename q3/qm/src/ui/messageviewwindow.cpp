/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmaccount.h>
#include <qmapplication.h>
#include <qmfolder.h>
#include <qmmessage.h>
#include <qmmessageholder.h>
#include <qmmessagewindow.h>
#include <qmtemplate.h>

#include <qsconv.h>
#include <qsstream.h>
#include <qsuiutil.h>

#include <tchar.h>
#ifdef QMHTMLVIEW
#	include <exdispid.h>
#	include <mshtmcid.h>
#	include <mshtmdid.h>
#endif
#ifndef _WIN32_WCE
#	include <tmschema.h>
#endif

#include "messageviewwindow.h"
#include "uiutil.h"
#include "../model/uri.h"
#include "../uimodel/messagemodel.h"

#pragma warning(disable:4786)

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * MessageViewWindow
 *
 */

qm::MessageViewWindow::~MessageViewWindow()
{
}


/****************************************************************************
 *
 * MessageViewWindowCallback
 *
 */

qm::MessageViewWindowCallback::~MessageViewWindowCallback()
{
}


/****************************************************************************
 *
 * MessageViewWindowFactory
 *
 */

qm::MessageViewWindowFactory::MessageViewWindowFactory(Document* pDocument,
													   Profile* pProfile,
													   const WCHAR* pwszSection,
													   MessageModel* pMessageModel,
													   MenuManager* pMenuManager,
													   MessageViewWindowCallback* pCallback,
													   bool bTextOnly) :
	pDocument_(pDocument),
	pProfile_(pProfile),
	pwszSection_(pwszSection),
	pMessageModel_(pMessageModel),
	pMenuManager_(pMenuManager),
	pCallback_(pCallback),
	bTextOnly_(bTextOnly),
#ifdef QMHTMLVIEW
	pText_(0),
	pHtml_(0)
#else
	pText_(0)
#endif
{
	pText_ = new TextMessageViewWindow(pDocument_, pProfile_,
		pwszSection_, pMessageModel_, pMenuManager_);
}

qm::MessageViewWindowFactory::~MessageViewWindowFactory()
{
}

bool qm::MessageViewWindowFactory::create(HWND hwnd)
{
	assert(hwnd);
	
#if defined _WIN32_WCE && _WIN32_WCE >= 300 && defined _WIN32_WCE_PSPC
	DWORD dwExStyle = 0;
#else
	DWORD dwExStyle = WS_EX_CLIENTEDGE;
#endif
	RECT rect;
	Window(hwnd).getClientRect(&rect);
	return pText_->create(L"QmTextMessageViewWindow", 0, WS_CHILD,
		0, 0, rect.right, rect.bottom, hwnd, dwExStyle, 0, 1002, 0);
}

MessageViewWindow* qm::MessageViewWindowFactory::getMessageViewWindow(const ContentTypeParser* pContentType)
{
#ifdef QMHTMLVIEW
	bool bHtml = !bTextOnly_ &&
		pContentType &&
		_wcsicmp(pContentType->getMediaType(), L"text") == 0 &&
		_wcsicmp(pContentType->getSubType(), L"html") == 0;
	
	if (bHtml && !pHtml_ && Application::getApplication().getAtlHandle()) {
		if (!createHtmlView())
			bHtml = false;
	}
	else if (bHtml) {
		bHtml = pHtml_ != 0;
	}
	
	return bHtml ? static_cast<MessageViewWindow*>(pHtml_) :
		static_cast<MessageViewWindow*>(pText_);
#else
	return pText_;
#endif
}

TextMessageViewWindow* qm::MessageViewWindowFactory::getTextMessageViewWindow()
{
	return pText_;
}

MessageViewWindow* qm::MessageViewWindowFactory::getLinkMessageViewWindow()
{
#ifdef QMHTMLVIEW
	if (!pHtml_ && Application::getApplication().getAtlHandle()) {
		if (!createHtmlView())
			return pText_;
	}
	return pHtml_;
#else
	return pText_;
#endif
}

bool qm::MessageViewWindowFactory::isSupported(const ContentTypeParser* pContentType) const
{
	return !pContentType || _wcsicmp(pContentType->getMediaType(), L"text") == 0;
}

void qm::MessageViewWindowFactory::reloadProfiles()
{
	pText_->reloadProfiles(pProfile_, pwszSection_);
}

#ifdef QMHTMLVIEW
bool qm::MessageViewWindowFactory::createHtmlView()
{
	std::auto_ptr<HtmlMessageViewWindow> pHtml(new HtmlMessageViewWindow(
		pProfile_, pwszSection_, pMessageModel_, pMenuManager_, pCallback_));
	HWND hwnd = pText_->getParent();
#ifdef _WIN32_WCE
	const WCHAR* pwszId = L"{8856F961-340A-11D0-A96B-00C04FD705A2}";
#else
	const WCHAR* pwszId = L"Shell.Explorer";
#endif
	RECT rect;
	Window(hwnd).getClientRect(&rect);
	if (!pHtml->create(L"QmHtmlMessageViewWindow", pwszId, WS_CHILD,
		0, 0, rect.right, rect.bottom, hwnd, WS_EX_CLIENTEDGE, 0, 1003, 0))
		return false;
	pHtml_ = pHtml.release();
	
	return true;
}
#endif


/****************************************************************************
 *
 * TextMessageViewWindow
 *
 */

qm::TextMessageViewWindow::TextMessageViewWindow(Document* pDocument,
												 Profile* pProfile,
												 const WCHAR* pwszSection,
												 MessageModel* pMessageModel,
												 MenuManager* pMenuManager) :
	TextWindow(0, pProfile, pwszSection, true),
	pDocument_(pDocument),
	pProfile_(pProfile),
	pMessageModel_(pMessageModel),
	pMenuManager_(pMenuManager)
{
	pTextModel_.reset(new ReadOnlyTextModel());
	setTextModel(pTextModel_.get());
	setLinkHandler(this);
}

qm::TextMessageViewWindow::~TextMessageViewWindow()
{
}

LRESULT qm::TextMessageViewWindow::windowProc(UINT uMsg,
											  WPARAM wParam,
											  LPARAM lParam)
{
	BEGIN_MESSAGE_HANDLER()
		HANDLE_CONTEXTMENU()
		HANDLE_LBUTTONDOWN()
	END_MESSAGE_HANDLER()
	return TextWindow::windowProc(uMsg, wParam, lParam);
}

LRESULT qm::TextMessageViewWindow::onContextMenu(HWND hwnd,
												 const POINT& pt)
{
	setActive();
	
	HMENU hmenu = pMenuManager_->getMenu(L"message", false, false);
	if (hmenu) {
		UINT nFlags = TPM_LEFTALIGN | TPM_TOPALIGN;
#ifndef _WIN32_WCE
		nFlags |= TPM_LEFTBUTTON | TPM_RIGHTBUTTON;
#endif
		::TrackPopupMenu(hmenu, nFlags, pt.x, pt.y, 0, getParentFrame(), 0);
	}
	
	return 0;
}

LRESULT qm::TextMessageViewWindow::onLButtonDown(UINT nFlags,
												 const POINT& pt)
{
#if defined _WIN32_WCE && _WIN32_WCE >= 300 && defined _WIN32_WCE_PSPC
	if (tapAndHold(pt))
		return 0;
#endif
	return TextWindow::onLButtonDown(nFlags, pt);
}

bool qm::TextMessageViewWindow::openLink(const WCHAR* pwszURL)
{
	return UIUtil::openURL(getParentFrame(), pwszURL);
}

Window& qm::TextMessageViewWindow::getWindow()
{
	return *this;
}

bool qm::TextMessageViewWindow::isActive()
{
	return hasFocus();
}

void qm::TextMessageViewWindow::setActive()
{
	setFocus();
}

bool qm::TextMessageViewWindow::setMessage(MessageHolder* pmh,
										   Message* pMessage,
										   const Template* pTemplate,
										   const WCHAR* pwszEncoding,
										   unsigned int nFlags,
										   unsigned int nSecurityMode)
{
	assert((pmh && pMessage) || (!pmh && !pMessage));
	
	if (pmh) {
		PartUtil util(*pMessage);
		wxstring_size_ptr wstrText;
		if (nFlags & FLAG_RAWMODE) {
			wstrText = util.getAllText(0, pwszEncoding, false);
		}
		else if (pTemplate) {
			// TODO
			// Performance up.
			// TODO
			// Get selected messages
			TemplateContext context(pmh, pMessage, MessageHolderList(),
				pmh->getFolder()->getAccount(), pDocument_, getHandle(),
				pwszEncoding, nSecurityMode, pProfile_, 0, TemplateContext::ArgumentList());
			wstring_ptr wstr;
			if (pTemplate->getValue(context, &wstr) != Template::RESULT_SUCCESS)
				return false;
			size_t nLen = wcslen(wstr.get());
			wstrText.reset(allocWXString(wstr.get(), nLen), nLen);
		}
		else if (nFlags & FLAG_INCLUDEHEADER) {
			wstrText = util.getFormattedText(false, pwszEncoding, true);
		}
		else {
			const Part* pPart = 0;
			if (PartUtil::isContentType(pMessage->getContentType(), L"multipart", L"alternative"))
				pPart = util.getAlternativePart(L"text", L"plain");
			
			if (pPart)
				wstrText = PartUtil(*pPart).getBodyText(0, pwszEncoding, true);
			else
				wstrText = util.getBodyText(0, pwszEncoding, true);
		}
		
		if (!wstrText.get())
			return false;
		
		std::auto_ptr<StringReader> pReader(new StringReader(wstrText));
		return pTextModel_->loadText(pReader, true);
	}
	else {
		return pTextModel_->setText(L"", 0);
	}
}

bool qm::TextMessageViewWindow::scrollPage(bool bPrev)
{
	SCROLLINFO info = {
		sizeof(info),
		SIF_ALL
	};
	::GetScrollInfo(getHandle(), SB_VERT, &info);
	if ((bPrev && info.nPos != 0) ||
		(!bPrev && info.nPos < static_cast<int>(info.nMax - info.nPage + 1))) {
		scroll(bPrev ? SCROLL_PAGEUP : SCROLL_PAGEDOWN, 0, false);
		return true;
	}
	else {
		return false;
	}
}

void qm::TextMessageViewWindow::setSelectMode(bool bSelectMode)
{
	setShowNewLine(bSelectMode);
	setShowTab(bSelectMode);
	setShowCaret(bSelectMode);
	deselectAll();
}

void qm::TextMessageViewWindow::setQuoteMode(bool bQuoteMode)
{
	setLineQuote(bQuoteMode);
}

bool qm::TextMessageViewWindow::find(const WCHAR* pwszFind,
									 unsigned int nFlags)
{
	unsigned int nFindFlags =
		(nFlags & MessageWindow::FIND_MATCHCASE ? FIND_MATCHCASE : 0) |
		(nFlags & MessageWindow::FIND_REGEX ? FIND_REGEX : 0) |
		(nFlags & MessageWindow::FIND_PREVIOUS ? FIND_PREVIOUS : 0);
	return TextWindow::find(pwszFind, nFindFlags);
}

unsigned int qm::TextMessageViewWindow::getSupportedFindFlags() const
{
	return MessageWindow::FIND_MATCHCASE |
		MessageWindow::FIND_REGEX |
		MessageWindow::FIND_PREVIOUS |
		MessageWindow::FIND_INCREMENTAL;
}

std::auto_ptr<MessageWindow::Mark> qm::TextMessageViewWindow::mark() const
{
	unsigned int nLine = -1;
	unsigned int nChar = -1;
	getFindPosition(false, &nLine, &nChar);
	return std::auto_ptr<MessageWindow::Mark>(new MarkImpl(
		nLine != -1 ? nLine : 0, nChar != -1 ? nChar : 0));
}

void qm::TextMessageViewWindow::reset(const MessageWindow::Mark& mark)
{
	const MarkImpl& impl = static_cast<const MarkImpl&>(mark);
	moveCaret(MOVECARET_POS, impl.nLine_, impl.nChar_, false, SELECT_CLEAR, false);
}

bool qm::TextMessageViewWindow::openLink()
{
	return TextWindow::openLink();
}

void qm::TextMessageViewWindow::copy()
{
	TextWindow::copy();
	
	MessagePtrLock mpl(pMessageModel_->getCurrentMessage());
	if (mpl)
		UIUtil::addMessageToClipboard(getHandle(), mpl);
}

bool qm::TextMessageViewWindow::canCopy()
{
	return TextWindow::canCopy();
}

void qm::TextMessageViewWindow::selectAll()
{
	TextWindow::selectAll();
}

bool qm::TextMessageViewWindow::canSelectAll()
{
	return TextWindow::canSelectAll();
}


/****************************************************************************
 *
 * TextMessageViewWindow::MarkImpl
 *
 */

qm::TextMessageViewWindow::MarkImpl::MarkImpl(unsigned int nLine,
											  unsigned int nChar) :
	nLine_(nLine),
	nChar_(nChar)
{
}

qm::TextMessageViewWindow::MarkImpl::~MarkImpl()
{
}


#ifdef QMHTMLVIEW

/****************************************************************************
 *
 * HtmlMessageViewWindow
 *
 */

qm::HtmlMessageViewWindow::HtmlMessageViewWindow(Profile* pProfile,
												 const WCHAR* pwszSection,
												 MessageModel* pMessageModel,
												 MenuManager* pMenuManager,
												 MessageViewWindowCallback* pCallback) :
	WindowBase(true),
	pProfile_(pProfile),
	pwszSection_(pwszSection),
	pMessageModel_(pMessageModel),
	pMenuManager_(pMenuManager),
	pCallback_(pCallback),
	pWebBrowser_(0),
	pServiceProvider_(0),
	pWebBrowserEvents_(0),
	dwConnectionPointCookie_(0),
	bAllowExternal_(false),
	bActivate_(false),
	bOnlineMode_(false)
{
	setWindowHandler(this, false);
}

qm::HtmlMessageViewWindow::~HtmlMessageViewWindow()
{
	ContentManager& manager = ContentManager::getInstance();
	manager.clearRelatedContent(this);
}

wstring_ptr qm::HtmlMessageViewWindow::getSuperClass()
{
	return allocWString(L"AtlAxWin");
}

LRESULT qm::HtmlMessageViewWindow::windowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	BEGIN_MESSAGE_HANDLER()
		HANDLE_CREATE()
		HANDLE_DESTROY()
#ifndef _WIN32_WCE
		HANDLE_NCPAINT()
		HANDLE_THEMECHANGED()
#endif
	END_MESSAGE_HANDLER()
	return DefaultWindowHandler::windowProc(uMsg, wParam, lParam);
}

LRESULT qm::HtmlMessageViewWindow::onCreate(CREATESTRUCT* pCreateStruct)
{
	if (DefaultWindowHandler::onCreate(pCreateStruct) == -1)
		return -1;
	
#ifndef _WIN32_WCE
	pTheme_.reset(new Theme(getHandle(), L"Edit"));
#endif
	
	HINSTANCE hInstAtl = Application::getApplication().getAtlHandle();
	assert(hInstAtl);
	typedef HRESULT (__stdcall *PFN_ATLAXGETCONTROL)(HWND, IUnknown**);
	PFN_ATLAXGETCONTROL pfnAtlAxGetControl = reinterpret_cast<PFN_ATLAXGETCONTROL>(
		::GetProcAddress(hInstAtl, WCE_T("AtlAxGetControl")));
	if (!pfnAtlAxGetControl)
		return -1;
	typedef HRESULT (__stdcall *PFN_ATLAXGETHOST)(HWND, IUnknown**);
	PFN_ATLAXGETHOST pfnAtlAxGetHost = reinterpret_cast<PFN_ATLAXGETHOST>(
		::GetProcAddress(hInstAtl, WCE_T("AtlAxGetHost")));
	if (!pfnAtlAxGetHost)
		return -1;
	
	HRESULT hr = S_OK;
	
	ComPtr<IUnknown> pUnkHost;
	hr = (*pfnAtlAxGetHost)(getHandle(), &pUnkHost);
	if (FAILED(hr))
		return -1;
	
	IAxWinAmbientDispatch* pAmbientDispatch;
	hr = pUnkHost->QueryInterface(IID_IAxWinAmbientDispatch,
		reinterpret_cast<void**>(&pAmbientDispatch));
	if (FAILED(hr))
		return -1;
	pAmbientDispatch->Release();
	std::auto_ptr<AmbientDispatchHook> pHook(
		new AmbientDispatchHook(this, pAmbientDispatch));
	pHook.release();
	
	ComPtr<IUnknown> pUnkControl;
	hr = (*pfnAtlAxGetControl)(getHandle(), &pUnkControl);
	if (FAILED(hr))
		return -1;
	hr = pUnkControl->QueryInterface(IID_IWebBrowser2,
		reinterpret_cast<void**>(&pWebBrowser_));
	if (FAILED(hr))
		return -1;
	
	ComPtr<IObjectWithSite> pSite;
	hr = pUnkHost->QueryInterface(IID_IObjectWithSite,
		reinterpret_cast<void**>(&pSite));
	if (FAILED(hr))
		return -1;
	
	pSecurityManager_ = new IInternetSecurityManagerImpl();
	pSecurityManager_->AddRef();
	
	pServiceProvider_ = new IServiceProviderImpl(this);
	pServiceProvider_->AddRef();
	hr = pSite->SetSite(static_cast<IServiceProvider*>(pServiceProvider_));
	if (FAILED(hr))
		return -1;
	
	ComPtr<IAxWinHostWindow> pAxWinHostWindow;
	hr = pUnkHost->QueryInterface(IID_IAxWinHostWindow,
		reinterpret_cast<void**>(&pAxWinHostWindow));
	if (FAILED(hr))
		return -1;
	
	IDocHostUIHandlerDispatchImpl* pDocHostUIHandlerDispatch =
		new IDocHostUIHandlerDispatchImpl(this);
	pDocHostUIHandlerDispatch->AddRef();
	hr = pAxWinHostWindow->SetExternalUIHandler(pDocHostUIHandlerDispatch);
	if (FAILED(hr))
		return -1;
	pDocHostUIHandlerDispatch->Release();
	
	BSTRPtr bstrURL(::SysAllocString(L"about:blank"));
	int n = 0;
	Variant v[4];
	v[0].vt = VT_I4;
	v[0].lVal = navNoHistory | navNoReadFromCache | navNoWriteToCache;
	hr = pWebBrowser_->Navigate(bstrURL.get(), &v[0], &v[1], &v[2], &v[3]);
	if (FAILED(hr))
		return -1;
	
	VARIANT_BOOL bBusy = VARIANT_TRUE;
	while (bBusy == VARIANT_TRUE) {
		::Sleep(10);
		hr = pWebBrowser_->get_Busy(&bBusy);
		if (FAILED(hr))
			return -1;
	}
	
	pWebBrowserEvents_ = new DWebBrowserEvents2Impl(this, pWebBrowser_);
	pWebBrowserEvents_->AddRef();
	ComPtr<IConnectionPointContainer> pConnectionPointContainer;
	hr = pWebBrowser_->QueryInterface(IID_IConnectionPointContainer,
		reinterpret_cast<void**>(&pConnectionPointContainer));
	if (FAILED(hr))
		return -1;
	ComPtr<IConnectionPoint> pConnectionPoint;
	hr = pConnectionPointContainer->FindConnectionPoint(
		DIID_DWebBrowserEvents2, &pConnectionPoint);
	if (FAILED(hr))
		return -1;
	hr = pConnectionPoint->Advise(pWebBrowserEvents_, &dwConnectionPointCookie_);
	if (FAILED(hr))
		return -1;
	
	return 0;
}

LRESULT qm::HtmlMessageViewWindow::onDestroy()
{
	if (pWebBrowser_) {
		if (dwConnectionPointCookie_ != 0) {
			ComPtr<IConnectionPointContainer> pConnectionPointContainer;
			ComPtr<IConnectionPoint> pConnectionPoint;
			HRESULT hr = pWebBrowser_->QueryInterface(IID_IConnectionPointContainer,
				reinterpret_cast<void**>(&pConnectionPointContainer));
			if (SUCCEEDED(hr))
				hr = pConnectionPointContainer->FindConnectionPoint(
					DIID_DWebBrowserEvents2, &pConnectionPoint);
			if (SUCCEEDED(hr))
				hr = pConnectionPoint->Unadvise(dwConnectionPointCookie_);
		}
		pWebBrowser_->Release();
		pWebBrowser_ = 0;
	}
	if (pServiceProvider_) {
		pServiceProvider_->Release();
		pServiceProvider_ = 0;
	}
	if (pSecurityManager_) {
		pSecurityManager_->Release();
		pSecurityManager_ = 0;
	}
	if (pWebBrowserEvents_) {
		pWebBrowserEvents_->Release();
		pWebBrowserEvents_ = 0;
	}
	
#ifndef _WIN32_WCE
	pTheme_.reset(0);
#endif
	
	return DefaultWindowHandler::onDestroy();
}

#ifndef _WIN32_WCE
LRESULT qm::HtmlMessageViewWindow::onNcPaint(HRGN hrgn)
{
	DefaultWindowHandler::onNcPaint(hrgn);
	
	if (getWindowLong(GWL_EXSTYLE) & WS_EX_CLIENTEDGE && pTheme_->isActive())
		qs::UIUtil::drawThemeBorder(pTheme_.get(), getHandle(), EP_EDITTEXT, 0, ::GetSysColor(COLOR_WINDOW));
	
	return 0;
}

LRESULT qm::HtmlMessageViewWindow::onThemeChanged()
{
	pTheme_.reset(new Theme(getHandle(), L"Edit"));
	return 0;
}
#endif

Window& qm::HtmlMessageViewWindow::getWindow()
{
	return *this;
}

bool qm::HtmlMessageViewWindow::isActive()
{
	HWND hwnd = ::GetFocus();
	while (hwnd) {
		if (hwnd == getHandle())
			return true;
		hwnd = ::GetParent(hwnd);
	}
	return false;
}

void qm::HtmlMessageViewWindow::setActive()
{
	HRESULT hr = S_OK;
	
	ComPtr<IDispatch> pDisp;
	hr = pWebBrowser_->get_Document(&pDisp);
	if (FAILED(hr))
		return;
	ComPtr<IHTMLDocument2> pHTMLDocument;
	hr = pDisp->QueryInterface(IID_IHTMLDocument2,
		reinterpret_cast<void**>(&pHTMLDocument));
	if (FAILED(hr))
		return;
	
	bool bComplete = true;
	BSTRPtr bstrState;
	hr = pHTMLDocument->get_readyState(&bstrState);
	if (SUCCEEDED(hr))
		bComplete = wcscmp(bstrState.get(), L"complete") == 0 ||
			wcscmp(bstrState.get(), L"interactive") == 0;
	if (bComplete) {
		ComPtr<IHTMLWindow2> pHTMLWindow;
		hr = pHTMLDocument->get_parentWindow(&pHTMLWindow);
		if (FAILED(hr))
			return;
		
		hr = pHTMLWindow->focus();
		if (FAILED(hr))
			return;
		
		bActivate_ = false;
	}
	else {
		bActivate_ = true;
	}
}

bool qm::HtmlMessageViewWindow::setMessage(MessageHolder* pmh,
										   Message* pMessage,
										   const Template* pTemplate,
										   const WCHAR* pwszEncoding,
										   unsigned int nFlags,
										   unsigned int nSecurityMode)
{
	assert(pmh && pMessage);
	
	Account* pAccount = pmh->getFolder()->getAccount();
	
	HRESULT hr = S_OK;
	
	bool bOnlineMode = (nFlags & FLAG_ONLINEMODE) != 0;
	if (bOnlineMode != bOnlineMode_) {
		bOnlineMode_ = bOnlineMode;
		
		ComPtr<IOleControl> pControl;
		hr = pWebBrowser_->QueryInterface(IID_IOleControl,
			reinterpret_cast<void**>(&pControl));
		if (FAILED(hr))
			return false;
		pControl->OnAmbientPropertyChange(DISPID_AMBIENT_DLCONTROL);
	}
	
	pSecurityManager_->setInternetZone((nFlags & FLAG_INTERNETZONE) != 0);
	
	wstring_ptr wstrURL;
	bool bClear = true;
	
	UnstructuredParser link;
	if (pAccount->isSupport(Account::SUPPORT_EXTERNALLINK) &&
		!pMessage->isMultipart() &&
		pMessage->getField(L"X-QMAIL-Link", &link) == Part::FIELD_EXIST) {
		wstrURL = allocWString(link.getValue());
		bAllowExternal_ = true;
		
		BSTRPtr bstrPrevURL;
		if (pWebBrowser_->get_LocationURL(&bstrPrevURL) == S_OK) {
			const WCHAR* p1 = wcsrchr(wstrURL.get(), L'#');
			size_t nLen1 = p1 ? p1 - wstrURL.get() : wcslen(wstrURL.get());
			const WCHAR* p2 = wcsrchr(bstrPrevURL.get(), L'#');
			size_t nLen2 = p2 ? p2 - bstrPrevURL.get() : wcslen(bstrPrevURL.get());
			bClear = nLen1 != nLen2 || _wcsnicmp(wstrURL.get(), bstrPrevURL.get(), nLen1) != 0;
		}
	}
	else {
		PartUtil util(*pMessage);
		const Part* pPart = util.getAlternativePart(L"text", L"html");
		assert(pPart);
		
		ContentManager& manager = ContentManager::getInstance();
		wstring_ptr wstrId(manager.prepareRelatedContent(
			this, *pMessage, *pPart, pwszEncoding));
		wstrURL = concat(L"cid:", wstrId.get());
		bAllowExternal_ = false;
	}
	
	if (bClear) {
		ComPtr<IDispatch> pDisp;
		if (pWebBrowser_->get_Document(&pDisp) == S_OK && pDisp.get()) {
			ComPtr<IHTMLDocument2> pHTMLDocument;
			if (pDisp->QueryInterface(IID_IHTMLDocument2,
				reinterpret_cast<void**>(&pHTMLDocument)) == S_OK) {
				ComPtr<IHTMLElement> pBody;
				if (pHTMLDocument->get_body(&pBody) == S_OK && pBody.get()) {
					BSTRPtr bstrBody(::SysAllocString(L""));
					if (bstrBody.get())
						pBody->put_innerHTML(bstrBody.get());
					
					ComPtr<IHTMLStyle> pStyle;
					if (pBody->get_style(&pStyle) == S_OK && pStyle.get()) {
						BSTRPtr bstrStyle(::SysAllocString(L"window"));
						if (bstrStyle.get())
							pStyle->put_background(bstrStyle.get());
					}
				}
			}
		}
	}
	
	BSTRPtr bstrURL(::SysAllocString(wstrURL.get()));
	if (!bstrURL.get())
		return false;
	int n = 0;
	Variant v[4];
	v[0].vt = VT_I4;
	v[0].lVal = navNoHistory | navNoReadFromCache | navNoWriteToCache;
	hr = pWebBrowser_->Navigate(bstrURL.get(), &v[0], &v[1], &v[2], &v[3]);
	if (FAILED(hr))
		return false;
	
	return true;
}

bool qm::HtmlMessageViewWindow::scrollPage(bool bPrev)
{
	HRESULT hr = S_OK;
	
	ComPtr<IDispatch> pDisp;
	hr = pWebBrowser_->get_Document(&pDisp);
	if (FAILED(hr))
		return false;
	ComPtr<IHTMLDocument2> pHTMLDocument;
	hr = pDisp->QueryInterface(IID_IHTMLDocument2,
		reinterpret_cast<void**>(&pHTMLDocument));
	if (FAILED(hr))
		return false;
	ComPtr<IHTMLElement> pBodyElement;
	hr = pHTMLDocument->get_body(&pBodyElement);
	if (FAILED(hr))
		return false;
	ComPtr<IHTMLElement2> pBodyElement2;
	hr = pBodyElement->QueryInterface(IID_IHTMLElement2,
		reinterpret_cast<void**>(&pBodyElement2));
	if (FAILED(hr))
		return false;
	
	long nScrollHeight = 0;
	hr = pBodyElement2->get_scrollHeight(&nScrollHeight);
	if (FAILED(hr))
		return false;
	long nClientHeight = 0;
	hr = pBodyElement2->get_clientHeight(&nClientHeight);
	if (FAILED(hr))
		return false;
	long nScrollTop = 0;
	hr = pBodyElement2->get_scrollTop(&nScrollTop);
	if (FAILED(hr))
		return false;
	
	if ((bPrev && nScrollTop != 0) ||
		(!bPrev && nScrollTop + nClientHeight < nScrollHeight)) {
		BSTRPtr bstrScroll(::SysAllocString(bPrev ? L"pageUp" : L"pageDown"));
		VARIANT v;
		v.vt = VT_BSTR;
		v.bstrVal = bstrScroll.get();
		hr = pBodyElement2->doScroll(v);
		if (FAILED(hr))
			return false;
		return true;
	}
	
	return false;
}

void qm::HtmlMessageViewWindow::setSelectMode(bool bSelectMode)
{
}

void qm::HtmlMessageViewWindow::setQuoteMode(bool bQuoteMode)
{
}

bool qm::HtmlMessageViewWindow::find(const WCHAR* pwszFind,
									 unsigned int nFlags)
{
	assert(pwszFind);
	
	bool bPrev = (nFlags & MessageWindow::FIND_PREVIOUS) != 0;
	
	HRESULT hr = S_OK;
	
	ComPtr<IDispatch> pDispDocument;
	hr = pWebBrowser_->get_Document(&pDispDocument);
	if (FAILED(hr))
		return false;
	ComPtr<IHTMLDocument2> pHTMLDocument;
	hr = pDispDocument->QueryInterface(IID_IHTMLDocument2,
		reinterpret_cast<void**>(&pHTMLDocument));
	if (FAILED(hr))
		return false;
	
	ComPtr<IHTMLSelectionObject> pSelection;
	hr = pHTMLDocument->get_selection(&pSelection);
	if (FAILED(hr))
		return false;
	
	ComPtr<IDispatch> pDispRange;
	hr = pSelection->createRange(&pDispRange);
	if (FAILED(hr))
		return false;
	ComPtr<IHTMLTxtRange> pRange;
	hr = pDispRange->QueryInterface(IID_IHTMLTxtRange,
		reinterpret_cast<void**>(&pRange));
	if (FAILED(hr))
		return false;
	
	BSTRPtr bstrText;
	hr = pRange->get_text(&bstrText);
	if (FAILED(hr))
		return false;
	if (bstrText.get() && *bstrText.get()) {
		hr = pRange->collapse(bPrev ? VARIANT_TRUE : VARIANT_FALSE);
		if (FAILED(hr))
			return false;
	}
	else {
		ComPtr<IHTMLElement> pElement;
		hr = pHTMLDocument->get_body(&pElement);
		if (FAILED(hr))
			return false;
		ComPtr<IHTMLBodyElement> pBody;
		hr = pElement->QueryInterface(IID_IHTMLBodyElement,
			reinterpret_cast<void**>(&pBody));
		if (FAILED(hr))
			return false;
		
		pRange.release()->Release();
		hr = pBody->createTextRange(&pRange);
		if (FAILED(hr))
			return false;
	}
	
	BSTRPtr bstrFind(::SysAllocString(pwszFind));
	if (!bstrFind.get())
		return false;
	VARIANT_BOOL bFound = VARIANT_FALSE;
	hr = pRange->findText(bstrFind.get(), bPrev ? -1 : 1,
		nFlags & MessageWindow::FIND_MATCHCASE ? 4 : 0, &bFound);
	if (FAILED(hr))
		return false;
	if (bFound == VARIANT_TRUE) {
		hr = pRange->select();
		if (FAILED(hr))
			return false;
	}
	return bFound == VARIANT_TRUE;
}

unsigned int qm::HtmlMessageViewWindow::getSupportedFindFlags() const
{
	return MessageWindow::FIND_MATCHCASE | MessageWindow::FIND_PREVIOUS;
}

std::auto_ptr<MessageWindow::Mark> qm::HtmlMessageViewWindow::mark() const
{
	return std::auto_ptr<MessageWindow::Mark>();
}

void qm::HtmlMessageViewWindow::reset(const MessageWindow::Mark& mark)
{
	assert(false);
}

bool qm::HtmlMessageViewWindow::openLink()
{
	return true;
}
void qm::HtmlMessageViewWindow::copy()
{
	pWebBrowser_->ExecWB(OLECMDID_COPY, OLECMDEXECOPT_DONTPROMPTUSER, 0, 0);
	
	MessagePtrLock mpl(pMessageModel_->getCurrentMessage());
	if (mpl)
		UIUtil::addMessageToClipboard(getHandle(), mpl);
}

bool qm::HtmlMessageViewWindow::canCopy()
{
	OLECMDF f;
	HRESULT hr = pWebBrowser_->QueryStatusWB(OLECMDID_COPY, &f);
	return (f & OLECMDF_ENABLED) != 0;
}

void qm::HtmlMessageViewWindow::selectAll()
{
	pWebBrowser_->ExecWB(OLECMDID_SELECTALL, OLECMDEXECOPT_DONTPROMPTUSER, 0, 0);
}

bool qm::HtmlMessageViewWindow::canSelectAll()
{
	return true;
}


/****************************************************************************
 *
 * HtmlMessageViewWindow::Content
 *
 */

void HtmlMessageViewWindow::Content::destroy()
{
	freeWString(wstrContentId_);
	wstrContentId_ = 0;
	freeWString(wstrMimeType_);
	wstrMimeType_ = 0;
	free(pData_);
	pData_ = 0;
}


/****************************************************************************
 *
 * HtmlMessageViewWindow::ContentManager
 *
 */

HtmlMessageViewWindow::ContentManager qm::HtmlMessageViewWindow::ContentManager::instance__;

qm::HtmlMessageViewWindow::ContentManager::ContentManager()
{
	if (::CoInternetGetSession(0, &pInternetSession_, 0) == S_OK) {
		ComPtr<IClassFactory> pClassFactory(new InternetProtocolFactory());
		pClassFactory->AddRef();
		
		CLSID clsid = { 0x3646a74a, 0x7908, 0x4bed, { 0xb7, 0x6a, 0xab, 0x1a, 0x6f, 0x6b, 0xcf, 0x10 } };
		if (pInternetSession_->RegisterNameSpace(pClassFactory.get(), clsid, L"cid", 0, 0, 0) == S_OK)
			pClassFactory_ = pClassFactory;
	}
}

qm::HtmlMessageViewWindow::ContentManager::~ContentManager()
{
	if (pInternetSession_.get() && pClassFactory_.get())
		pInternetSession_->UnregisterNameSpace(pClassFactory_.get(), L"cid");
}

HtmlMessageViewWindow::Content qm::HtmlMessageViewWindow::ContentManager::getContent(const WCHAR* pwszContentId) const
{
	for (ContentList::const_iterator it = listContent_.begin(); it != listContent_.end(); ++it) {
		if ((*it).wstrContentId_ && wcscmp((*it).wstrContentId_, pwszContentId) == 0)
			return *it;
	}
	
	Content content = { 0, 0, 0, 0, 0 };
	return content;
}

wstring_ptr qm::HtmlMessageViewWindow::ContentManager::prepareRelatedContent(HtmlMessageViewWindow* pHtmlMessageViewWindow,
																			 const Message& msg,
																			 const qs::Part& partHtml,
																			 const WCHAR* pwszEncoding)
{
	clearRelatedContent(pHtmlMessageViewWindow);
	
	Time time(Time::getCurrentTime());
	WCHAR wszId[256];
	swprintf(wszId, L"%u%04d%02d%02d%02d%02d%02d%03d@local",
		::GetCurrentProcessId(), time.wYear, time.wMonth, time.wDay,
		time.wHour, time.wMinute, time.wSecond, time.wMilliseconds);
	
	prepareRelatedContent(pHtmlMessageViewWindow, partHtml, wszId, pwszEncoding);
	
	wstring_ptr wstrId(allocWString(wszId));
	
	prepareRelatedContent(pHtmlMessageViewWindow, msg, 0, 0);
	
	return wstrId;
}

void qm::HtmlMessageViewWindow::ContentManager::clearRelatedContent(HtmlMessageViewWindow* pHtmlMessageViewWindow)
{
	for (ContentList::iterator it = listContent_.begin(); it != listContent_.end(); ) {
		if ((*it).pHtmlMessageViewWindow_ == pHtmlMessageViewWindow) {
			(*it).destroy();
			it = listContent_.erase(it);
		}
		else {
			++it;
		}
	}
}

void qm::HtmlMessageViewWindow::ContentManager::prepareRelatedContent(HtmlMessageViewWindow* pHtmlMessageViewWindow,
																	  const qs::Part& part,
																	  const WCHAR* pwszId,
																	  const WCHAR* pwszEncoding)
{
	if (part.isMultipart()) {
		const Part::PartList& l = part.getPartList();
		for (Part::PartList::const_iterator it = l.begin(); it != l.end(); ++it)
			prepareRelatedContent(pHtmlMessageViewWindow, **it, 0, 0);
	}
	else {
		wstring_ptr wstrId;
		if (pwszId) {
			wstrId = allocWString(pwszId);
		}
		else {
			MessageIdParser contentId;
			if (part.getField(L"Content-Id", &contentId) == Part::FIELD_EXIST)
				wstrId = allocWString(contentId.getMessageId());
		}
		
		if (wstrId.get()) {
			wstring_ptr wstrMimeType;
			const WCHAR* pwszMediaType = L"text";
			const WCHAR* pwszSubType = L"plain";
			wstring_ptr wstrCharset;
			const ContentTypeParser* pContentType = part.getContentType();
			if (pContentType) {
				pwszMediaType = pContentType->getMediaType();
				pwszSubType = pContentType->getSubType();
				wstrCharset = pContentType->getParameter(L"charset");
			}
			if (pwszEncoding)
				wstrCharset = allocWString(pwszEncoding);
			
			size_t nMimeTypeLen = wcslen(pwszMediaType) + wcslen(pwszSubType) +
				(wstrCharset.get() ? wcslen(wstrCharset.get()) + 12 : 0) + 2;
			wstrMimeType = allocWString(nMimeTypeLen);
			wcscpy(wstrMimeType.get(), pwszMediaType);
			wcscat(wstrMimeType.get(), L"/");
			wcscat(wstrMimeType.get(), pwszSubType);
			if (wstrCharset.get()) {
				wcscat(wstrMimeType.get(), L"; charset=\"");
				wcscat(wstrMimeType.get(), wstrCharset.get());
				wcscat(wstrMimeType.get(), L"\"");
			}
			assert(wcslen(wstrMimeType.get()) < nMimeTypeLen);
			
			malloc_size_ptr<unsigned char> pData(part.getBodyData());
			if (!pData.get())
				return;
			Content content = {
				pHtmlMessageViewWindow,
				wstrId.get(),
				wstrMimeType.get(),
				pData.get(),
				pData.size()
			};
			listContent_.push_back(content);
			wstrId.release();
			wstrMimeType.release();
			pData.release();
		}
	}
}

HtmlMessageViewWindow::ContentManager& qm::HtmlMessageViewWindow::ContentManager::getInstance()
{
	return instance__;
}


/****************************************************************************
 *
 * HtmlMessageViewWindow::IInternetSecurityManagerImpl
 *
 */

qm::HtmlMessageViewWindow::IInternetSecurityManagerImpl::IInternetSecurityManagerImpl() :
	nRef_(0),
	bInternetZone_(false)
{
}

qm::HtmlMessageViewWindow::IInternetSecurityManagerImpl::~IInternetSecurityManagerImpl()
{
}

bool qm::HtmlMessageViewWindow::IInternetSecurityManagerImpl::isInternetZone() const
{
	return bInternetZone_;
}

void qm::HtmlMessageViewWindow::IInternetSecurityManagerImpl::setInternetZone(bool bInternetZone)
{
	bInternetZone_ = bInternetZone;
}

STDMETHODIMP_(ULONG) qm::HtmlMessageViewWindow::IInternetSecurityManagerImpl::AddRef()
{
	return ::InterlockedIncrement(reinterpret_cast<LONG*>(&nRef_));
}

STDMETHODIMP_(ULONG) qm::HtmlMessageViewWindow::IInternetSecurityManagerImpl::Release()
{
	ULONG nRef = ::InterlockedDecrement(reinterpret_cast<LONG*>(&nRef_));
	if (nRef == 0)
		delete this;
	return nRef;
}

STDMETHODIMP qm::HtmlMessageViewWindow::IInternetSecurityManagerImpl::QueryInterface(REFIID riid,
																					 void** ppv)
{
	*ppv = 0;
	
	if (riid == IID_IUnknown || riid == IID_IInternetSecurityManager) {
		AddRef();
		*ppv = static_cast<IInternetSecurityManager*>(this);
	}
	
	return *ppv ? S_OK : E_NOINTERFACE;
}

STDMETHODIMP qm::HtmlMessageViewWindow::IInternetSecurityManagerImpl::SetSecuritySite(IInternetSecurityMgrSite* pSite)
{
	return INET_E_DEFAULT_ACTION;
}

STDMETHODIMP qm::HtmlMessageViewWindow::IInternetSecurityManagerImpl::GetSecuritySite(IInternetSecurityMgrSite** ppSite)
{
	return INET_E_DEFAULT_ACTION;
}

STDMETHODIMP qm::HtmlMessageViewWindow::IInternetSecurityManagerImpl::MapUrlToZone(LPCWSTR pwszUrl,
																				   DWORD* pdwZone,
																				   DWORD dwFlags)
{
	if (!pdwZone)
		return E_INVALIDARG;
	
	*pdwZone = bInternetZone_ ? URLZONE_INTERNET : URLZONE_UNTRUSTED;
	
	return S_OK;
}

STDMETHODIMP qm::HtmlMessageViewWindow::IInternetSecurityManagerImpl::GetSecurityId(LPCWSTR pwszUrl,
																					BYTE* pbSecurityId,
																					DWORD* pcbSecurityId,
																					DWORD_PTR dwReserved)
{
	return INET_E_DEFAULT_ACTION;
}

STDMETHODIMP qm::HtmlMessageViewWindow::IInternetSecurityManagerImpl::ProcessUrlAction(LPCWSTR pwszUrl,
																					   DWORD dwAction,
																					   BYTE* pPolicy,
																					   DWORD cbPolicy,
																					   BYTE* pContext,
																					   DWORD cbContext,
																					   DWORD dwFlags,
																					   DWORD dwReserved)
{
	if (!bInternetZone_) {
		if (cbPolicy < sizeof(DWORD))
			return S_FALSE;
		
		*reinterpret_cast<DWORD*>(pPolicy) = URLPOLICY_DISALLOW;
		
		return S_OK;
	}
	else {
		return INET_E_DEFAULT_ACTION;
	}
}

STDMETHODIMP qm::HtmlMessageViewWindow::IInternetSecurityManagerImpl::QueryCustomPolicy(LPCWSTR pwszUrl,
																						REFGUID guidKey,
																						BYTE** ppPolicy,
																						DWORD* pcbPolicy,
																						BYTE* pContent,
																						DWORD cbContent,
																						DWORD dwReserved)
{
	return INET_E_DEFAULT_ACTION;
}

STDMETHODIMP qm::HtmlMessageViewWindow::IInternetSecurityManagerImpl::SetZoneMapping(DWORD dwZone,
																					 LPCWSTR pwszPattern,
																					 DWORD dwFlags)
{
	return INET_E_DEFAULT_ACTION;
}

STDMETHODIMP qm::HtmlMessageViewWindow::IInternetSecurityManagerImpl::GetZoneMappings(DWORD dwZone,
																					  IEnumString** ppenumString,
																					  DWORD dwFlags)
{
	return INET_E_DEFAULT_ACTION;
}


/****************************************************************************
 *
 * InternetProtocol
 *
 */

qm::HtmlMessageViewWindow::InternetProtocol::InternetProtocol() :
	nRef_(0),
	pCurrent_(0),
	pSink_(0)
{
}

qm::HtmlMessageViewWindow::InternetProtocol::~InternetProtocol()
{
}

STDMETHODIMP_(ULONG) qm::HtmlMessageViewWindow::InternetProtocol::AddRef()
{
	return ::InterlockedIncrement(reinterpret_cast<LONG*>(&nRef_));
}

STDMETHODIMP_(ULONG) qm::HtmlMessageViewWindow::InternetProtocol::Release()
{
	ULONG nRef = ::InterlockedDecrement(reinterpret_cast<LONG*>(&nRef_));
	if (nRef == 0)
		delete this;
	return nRef;
}

STDMETHODIMP qm::HtmlMessageViewWindow::InternetProtocol::QueryInterface(REFIID riid,
																		 void** ppv)
{
	*ppv = 0;
	
	if (riid == IID_IUnknown ||
		riid == IID_IInternetProtocol ||
		riid == IID_IInternetProtocolRoot) {
		AddRef();
		*ppv = static_cast<IInternetProtocol*>(this);
	}
	else if (riid == IID_IInternetProtocolInfo) {
		AddRef();
		*ppv = static_cast<IInternetProtocolInfo*>(this);
	}
	
	return *ppv ? S_OK : E_NOINTERFACE;
}

STDMETHODIMP qm::HtmlMessageViewWindow::InternetProtocol::Abort(HRESULT hrReason,
																DWORD dwOptions)
{
	return S_OK;
}

STDMETHODIMP qm::HtmlMessageViewWindow::InternetProtocol::Continue(PROTOCOLDATA* pProtocolData)
{
	return E_NOTIMPL;
}

STDMETHODIMP qm::HtmlMessageViewWindow::InternetProtocol::Resume()
{
	return E_NOTIMPL;
}

STDMETHODIMP qm::HtmlMessageViewWindow::InternetProtocol::Start(LPCWSTR pwszUrl,
																IInternetProtocolSink* pSink,
																IInternetBindInfo* pBindInfo,
																DWORD dwFlags,
																HANDLE_PTR dwReserved)
{
	if (wcsncmp(pwszUrl, L"cid:", 4) != 0)
		return E_FAIL;
	
	ContentManager& manager = ContentManager::getInstance();
	content_ = manager.getContent(pwszUrl + 4);
	if (!content_.wstrContentId_)
		return E_FAIL;
	
	pCurrent_ = content_.pData_;
	
	pSink_ = pSink;
	
	HRESULT hr = S_OK;
	
	hr = pSink_->ReportProgress(BINDSTATUS_MIMETYPEAVAILABLE, content_.wstrMimeType_);
	if (FAILED(hr))
		return hr;
	
	hr = pSink->ReportData(BSCF_LASTDATANOTIFICATION, 0, 0);
	if (FAILED(hr))
		return hr;
	
	hr = pSink->ReportResult(S_OK, 0, 0);
	if (FAILED(hr))
		return hr;
	
	return S_OK;
}

STDMETHODIMP qm::HtmlMessageViewWindow::InternetProtocol::Suspend()
{
	return E_NOTIMPL;
}

STDMETHODIMP qm::HtmlMessageViewWindow::InternetProtocol::Terminate(DWORD dwOptions)
{
	return S_OK;
}

STDMETHODIMP qm::HtmlMessageViewWindow::InternetProtocol::LockRequest(DWORD dwOptions)
{
	return S_OK;
}

STDMETHODIMP qm::HtmlMessageViewWindow::InternetProtocol::Read(void* pv,
															   ULONG cb,
															   ULONG* pcbRead)
{
	assert(pv);
	assert(pcbRead);
	
	*pcbRead = 0;
	
	if (cb > content_.nDataLen_ - (pCurrent_ - content_.pData_))
		cb = content_.nDataLen_ - (pCurrent_ - content_.pData_);
	
	if (cb == 0)
		return S_FALSE;
	
	memcpy(pv, pCurrent_, cb);
	*pcbRead = cb;
	pCurrent_ += cb;
	
	return S_OK;
}

STDMETHODIMP qm::HtmlMessageViewWindow::InternetProtocol::Seek(LARGE_INTEGER move,
															   DWORD dwOrigin,
															   ULARGE_INTEGER* pNewPos)
{
	return E_NOTIMPL;
}

STDMETHODIMP qm::HtmlMessageViewWindow::InternetProtocol::UnlockRequest()
{
	return S_OK;
}

STDMETHODIMP qm::HtmlMessageViewWindow::InternetProtocol::CombineUrl(LPCWSTR pwszBaseUrl,
																	 LPCWSTR pwszRelativeUrl,
																	 DWORD dwCombineFlags,
																	 LPWSTR pwszResult,
																	 DWORD cchResult,
																	 DWORD* pcchResult,
																	 DWORD dwReserved)
{
	return E_NOTIMPL;
}

STDMETHODIMP qm::HtmlMessageViewWindow::InternetProtocol::CompareUrl(LPCWSTR pwszUrl1,
																	 LPCWSTR pwszUrl2,
																	 DWORD dwCompareFlags)
{
	return E_NOTIMPL;
}

STDMETHODIMP qm::HtmlMessageViewWindow::InternetProtocol::ParseUrl(LPCWSTR pwszUrl,
																   PARSEACTION action,
																   DWORD dwParseFlags,
																   LPWSTR pwszResult,
																   DWORD cchResult,
																   DWORD* pcchResult,
																   DWORD dwReserved)
{
	return E_NOTIMPL;
}

STDMETHODIMP qm::HtmlMessageViewWindow::InternetProtocol::QueryInfo(LPCWSTR pwszUrl,
																	QUERYOPTION option,
																	DWORD dwQueryFlags,
																	LPVOID pBuffer,
																	DWORD cbBuffer,
																	DWORD* pcbBuffer,
																	DWORD dwReserved)
{
	return E_NOTIMPL;
}


/****************************************************************************
 *
 * HtmlMessageViewWindow::InternetProtocolFactory
 *
 */

qm::HtmlMessageViewWindow::InternetProtocolFactory::InternetProtocolFactory() :
	nRef_(0)
{
}

qm::HtmlMessageViewWindow::InternetProtocolFactory::~InternetProtocolFactory()
{
}

STDMETHODIMP_(ULONG) qm::HtmlMessageViewWindow::InternetProtocolFactory::AddRef()
{
	return ::InterlockedIncrement(reinterpret_cast<LONG*>(&nRef_));
}

STDMETHODIMP_(ULONG) qm::HtmlMessageViewWindow::InternetProtocolFactory::Release()
{
	ULONG nRef = ::InterlockedDecrement(reinterpret_cast<LONG*>(&nRef_));
	if (nRef == 0)
		delete this;
	return nRef;
}

STDMETHODIMP qm::HtmlMessageViewWindow::InternetProtocolFactory::QueryInterface(REFIID riid,
																				void** ppv)
{
	*ppv = 0;
	
	if (riid == IID_IUnknown || riid == IID_IClassFactory) {
		AddRef();
		*ppv = static_cast<IClassFactory*>(this);
	}
	
	return *ppv ? S_OK : E_NOINTERFACE;
}

STDMETHODIMP qm::HtmlMessageViewWindow::InternetProtocolFactory::CreateInstance(IUnknown* pUnkOuter,
																				REFIID riid,
																				void** ppvObj)
{
	if (pUnkOuter)
		return CLASS_E_NOAGGREGATION;
	
	if (riid != IID_IInternetProtocol)
		return E_NOINTERFACE;
	
	InternetProtocol* pInternetProtocol = new InternetProtocol();
	pInternetProtocol->AddRef();
	*ppvObj = static_cast<IInternetProtocol*>(pInternetProtocol);
	
	return S_OK;
}

STDMETHODIMP qm::HtmlMessageViewWindow::InternetProtocolFactory::LockServer(BOOL bLock)
{
	return E_NOTIMPL;
}


/****************************************************************************
 *
 * HtmlMessageViewWindow::IServiceProviderImpl
 *
 */

qm::HtmlMessageViewWindow::IServiceProviderImpl::IServiceProviderImpl(HtmlMessageViewWindow* pHtmlMessageViewWindow) :
	nRef_(0),
	pHtmlMessageViewWindow_(pHtmlMessageViewWindow)
{
}

qm::HtmlMessageViewWindow::IServiceProviderImpl::~IServiceProviderImpl()
{
}

STDMETHODIMP_(ULONG) qm::HtmlMessageViewWindow::IServiceProviderImpl::AddRef()
{
	return ::InterlockedIncrement(reinterpret_cast<LONG*>(&nRef_));
}

STDMETHODIMP_(ULONG) qm::HtmlMessageViewWindow::IServiceProviderImpl::Release()
{
	ULONG nRef = ::InterlockedDecrement(reinterpret_cast<LONG*>(&nRef_));
	if (nRef == 0)
		delete this;
	return nRef;
}

STDMETHODIMP qm::HtmlMessageViewWindow::IServiceProviderImpl::QueryInterface(REFIID riid,
																			 void** ppv)
{
	*ppv = 0;
	
	if (riid == IID_IUnknown || riid == IID_IServiceProvider) {
		AddRef();
		*ppv = static_cast<IServiceProvider*>(this);
	}
	
	return *ppv ? S_OK : E_NOINTERFACE;
}

STDMETHODIMP qm::HtmlMessageViewWindow::IServiceProviderImpl::QueryService(REFGUID guidService,
																		   REFIID riid,
																		   void** ppv)
{
	*ppv = 0;
	
	if (guidService == SID_SInternetSecurityManager &&
		riid == IID_IInternetSecurityManager) {
		IInternetSecurityManagerImpl* pSecurityManager = pHtmlMessageViewWindow_->pSecurityManager_;
		pSecurityManager->AddRef();
		*ppv = static_cast<IInternetSecurityManager*>(pSecurityManager);
	}
	
	return *ppv ? S_OK : E_NOINTERFACE;
}


/****************************************************************************
 *
 * HtmlMessageViewWindow::IDocHostUIHandlerDispatchImpl
 *
 */

qm::HtmlMessageViewWindow::IDocHostUIHandlerDispatchImpl::IDocHostUIHandlerDispatchImpl(HtmlMessageViewWindow* pWindow) :
	nRef_(0),
	pWindow_(pWindow)
{
}

qm::HtmlMessageViewWindow::IDocHostUIHandlerDispatchImpl::~IDocHostUIHandlerDispatchImpl()
{
}

STDMETHODIMP_(ULONG) qm::HtmlMessageViewWindow::IDocHostUIHandlerDispatchImpl::AddRef()
{
	return ::InterlockedIncrement(reinterpret_cast<LONG*>(&nRef_));
}

STDMETHODIMP_(ULONG) qm::HtmlMessageViewWindow::IDocHostUIHandlerDispatchImpl::Release()
{
	ULONG nRef = ::InterlockedDecrement(reinterpret_cast<LONG*>(&nRef_));
	if (nRef == 0)
		delete this;
	return nRef;
}

STDMETHODIMP qm::HtmlMessageViewWindow::IDocHostUIHandlerDispatchImpl::QueryInterface(REFIID riid,
																					  void** ppv)
{
	*ppv = 0;
	
	if (riid == IID_IUnknown ||
		riid == IID_IDispatch ||
		riid == IID_IDocHostUIHandlerDispatch) {
		AddRef();
		*ppv = static_cast<IDocHostUIHandlerDispatch*>(this);
	}
	
	return *ppv ? S_OK : E_NOINTERFACE;
}

STDMETHODIMP HtmlMessageViewWindow::IDocHostUIHandlerDispatchImpl::GetIDsOfNames(REFIID riid,
																				 OLECHAR** rgszNames,
																				 unsigned int cNames,
																				 LCID lcid,
																				 DISPID* pDispId)
{
	return E_NOTIMPL;
}

STDMETHODIMP HtmlMessageViewWindow::IDocHostUIHandlerDispatchImpl::GetTypeInfo(unsigned int nTypeInfo,
																			   LCID lcid,
																			   ITypeInfo** ppTypeInfo)
{
	return E_NOTIMPL;
}

STDMETHODIMP HtmlMessageViewWindow::IDocHostUIHandlerDispatchImpl::GetTypeInfoCount(unsigned int* pcTypeInfo)
{
	return E_NOTIMPL;
}

STDMETHODIMP HtmlMessageViewWindow::IDocHostUIHandlerDispatchImpl::Invoke(DISPID dispId,
																		  REFIID riid,
																		  LCID lcid,
																		  WORD wFlags,
																		  DISPPARAMS* pDispParams,
																		  VARIANT* pVarResult,
																		  EXCEPINFO* pExcepInfo,
																		  unsigned int* pnArgErr)
{
	return E_NOTIMPL;
}

STDMETHODIMP qm::HtmlMessageViewWindow::IDocHostUIHandlerDispatchImpl::ShowContextMenu(DWORD dwId,
																					   DWORD x,
																					   DWORD y,
																					   IUnknown* pUnk,
																					   IDispatch* pDisp,
																					   HRESULT* phrResult)
{
	pWindow_->setActive();
	
	HMENU hmenu = pWindow_->pMenuManager_->getMenu(L"message", false, false);
	if (hmenu) {
		UINT nFlags = TPM_LEFTALIGN | TPM_TOPALIGN;
#ifndef _WIN32_WCE
		nFlags |= TPM_LEFTBUTTON | TPM_RIGHTBUTTON;
#endif
		::TrackPopupMenu(hmenu, nFlags, x, y, 0, pWindow_->getParentFrame(), 0);
	}
	
	*phrResult = S_OK;
	
	return S_OK;
}

STDMETHODIMP qm::HtmlMessageViewWindow::IDocHostUIHandlerDispatchImpl::GetHostInfo(DWORD* pdwFlags,
																				   DWORD* pdwDoubleClick)
{
#ifdef _WIN32_WCE
	*pdwFlags = DOCHOSTUIFLAG_OPENNEWWIN;
#else
	*pdwFlags = DOCHOSTUIFLAG_OPENNEWWIN | DOCHOSTUIFLAG_NO3DBORDER | DOCHOSTUIFLAG_THEME;
#endif
	*pdwDoubleClick = DOCHOSTUIDBLCLK_DEFAULT;
	return S_OK;
}

STDMETHODIMP qm::HtmlMessageViewWindow::IDocHostUIHandlerDispatchImpl::ShowUI(DWORD dwId,
																			  IUnknown* pActiveObject,
																			  IUnknown* pCommandTarget,
																			  IUnknown* pFrame,
																			  IUnknown* pUIWindow,
																			  HRESULT* phrResult)
{
	return S_OK;
}

STDMETHODIMP qm::HtmlMessageViewWindow::IDocHostUIHandlerDispatchImpl::HideUI()
{
	return S_OK;
}

STDMETHODIMP qm::HtmlMessageViewWindow::IDocHostUIHandlerDispatchImpl::UpdateUI()
{
	return S_OK;
}

STDMETHODIMP qm::HtmlMessageViewWindow::IDocHostUIHandlerDispatchImpl::EnableModeless(VARIANT_BOOL bEnable)
{
	return S_OK;
}

STDMETHODIMP qm::HtmlMessageViewWindow::IDocHostUIHandlerDispatchImpl::OnDocWindowActivate(VARIANT_BOOL bActivate)
{
	return S_OK;
}

STDMETHODIMP qm::HtmlMessageViewWindow::IDocHostUIHandlerDispatchImpl::OnFrameWindowActivate(VARIANT_BOOL bActivate)
{
	return S_OK;
}

STDMETHODIMP qm::HtmlMessageViewWindow::IDocHostUIHandlerDispatchImpl::ResizeBorder(long left,
																					long top,
																					long right,
																					long buttom,
																					IUnknown* pUIWindow,
																					VARIANT_BOOL bFrameWindow)
{
	return S_OK;
}

STDMETHODIMP qm::HtmlMessageViewWindow::IDocHostUIHandlerDispatchImpl::TranslateAccelerator(DWORD hwnd,
																							DWORD nMessage,
																							DWORD wParam,
																							DWORD lParam,
																							BSTR bstrGuidCmdGroup,
																							DWORD nCmdId,
																							HRESULT* phrResult)
{
	*phrResult = S_FALSE;
	return S_OK;
}

STDMETHODIMP qm::HtmlMessageViewWindow::IDocHostUIHandlerDispatchImpl::GetOptionKeyPath(BSTR* pbstrKey,
																						DWORD dw)
{
	return S_FALSE;
}

STDMETHODIMP qm::HtmlMessageViewWindow::IDocHostUIHandlerDispatchImpl::GetDropTarget(IUnknown* pDropTarget,
																					 IUnknown** ppDropTarget)
{
	return S_FALSE;
}

STDMETHODIMP qm::HtmlMessageViewWindow::IDocHostUIHandlerDispatchImpl::GetExternal(IDispatch** ppDispatch)
{
	return E_NOINTERFACE;
}

STDMETHODIMP qm::HtmlMessageViewWindow::IDocHostUIHandlerDispatchImpl::TranslateUrl(DWORD dwTranslate,
																					BSTR bstrURLIn,
																					BSTR* bstrURLOut)
{
	return S_FALSE;
}

STDMETHODIMP qm::HtmlMessageViewWindow::IDocHostUIHandlerDispatchImpl::FilterDataObject(IUnknown* pInObject,
																						IUnknown** ppOutObject)
{
	return S_FALSE;
}


/****************************************************************************
 *
 * HtmlMessageViewWindow::DWebBrowserEvents2Impl
 *
 */

qm::HtmlMessageViewWindow::DWebBrowserEvents2Impl::DWebBrowserEvents2Impl(HtmlMessageViewWindow* pHtmlMessageViewWindow,
																		  IWebBrowser2* pWebBrowser) :
	nRef_(0),
	pHtmlMessageViewWindow_(pHtmlMessageViewWindow),
	pWebBrowser_(pWebBrowser)
{
}

qm::HtmlMessageViewWindow::DWebBrowserEvents2Impl::~DWebBrowserEvents2Impl()
{
}

STDMETHODIMP_(ULONG) qm::HtmlMessageViewWindow::DWebBrowserEvents2Impl::AddRef()
{
	return ::InterlockedIncrement(reinterpret_cast<LONG*>(&nRef_));
}

STDMETHODIMP_(ULONG) qm::HtmlMessageViewWindow::DWebBrowserEvents2Impl::Release()
{
	ULONG nRef = ::InterlockedDecrement(reinterpret_cast<LONG*>(&nRef_));
	if (nRef == 0)
		delete this;
	return nRef;
}

STDMETHODIMP qm::HtmlMessageViewWindow::DWebBrowserEvents2Impl::QueryInterface(REFIID riid,
																			   void** ppv)
{
	*ppv = 0;
	
	if (riid == IID_IUnknown ||
		riid == IID_IDispatch ||
		riid == DIID_DWebBrowserEvents2) {
		AddRef();
		*ppv = static_cast<DWebBrowserEvents2*>(this);
	}
	
	return *ppv ? S_OK : E_NOINTERFACE;
}

STDMETHODIMP HtmlMessageViewWindow::DWebBrowserEvents2Impl::GetIDsOfNames(REFIID riid,
																		  OLECHAR** rgszNames,
																		  unsigned int cNames,
																		  LCID lcid,
																		  DISPID* pDispId)
{
	return E_NOTIMPL;
}

STDMETHODIMP HtmlMessageViewWindow::DWebBrowserEvents2Impl::GetTypeInfo(unsigned int nTypeInfo,
																		LCID lcid,
																		ITypeInfo** ppTypeInfo)
{
	return E_NOTIMPL;
}

STDMETHODIMP HtmlMessageViewWindow::DWebBrowserEvents2Impl::GetTypeInfoCount(unsigned int* pcTypeInfo)
{
	return E_NOTIMPL;
}

STDMETHODIMP HtmlMessageViewWindow::DWebBrowserEvents2Impl::Invoke(DISPID dispId,
																   REFIID riid,
																   LCID lcid,
																   WORD wFlags,
																   DISPPARAMS* pDispParams,
																   VARIANT* pVarResult,
																   EXCEPINFO* pExcepInfo,
																   unsigned int* pnArgErr)
{
	if (dispId == DISPID_BEFORENAVIGATE2) {
		if (!pDispParams || pDispParams->cArgs != 7)
			return E_INVALIDARG;
		
		VARIANT* pVarWebBrowser = pDispParams->rgvarg + 6;
		if (pVarWebBrowser->vt != VT_DISPATCH)
			return E_INVALIDARG;
		ComPtr<IWebBrowser2> pWebBrowser;
		HRESULT hr = pVarWebBrowser->pdispVal->QueryInterface(IID_IWebBrowser2,
			reinterpret_cast<void**>(&pWebBrowser));
		if (FAILED(hr))
			return hr;
		
		VARIANT* pVarURL = pDispParams->rgvarg + 5;
		if (pVarURL->vt != (VT_VARIANT | VT_BYREF) ||
			pVarURL->pvarVal->vt != VT_BSTR)
			return E_INVALIDARG;
		BSTR bstrURL = pVarURL->pvarVal->bstrVal;
		bool bAllow = pWebBrowser.get() != pWebBrowser_ ||
			pHtmlMessageViewWindow_->bAllowExternal_ ||
			wcsncmp(bstrURL, L"cid:", 4) == 0;
		pHtmlMessageViewWindow_->bAllowExternal_ = false;
		
		VARIANT* pVarCancel = pDispParams->rgvarg;
		if (pVarCancel->vt != (VT_BOOL | VT_BYREF))
			return E_INVALIDARG;
		*pVarCancel->pboolVal = bAllow ? VARIANT_FALSE : VARIANT_TRUE;
		if (!bAllow)
			pWebBrowser->Stop();
		
		if (!bAllow &&
			pWebBrowser.get() == pWebBrowser_ &&
			(wcsncmp(bstrURL, L"http:", 5) == 0 ||
			wcsncmp(bstrURL, L"https:", 6) == 0 ||
			wcsncmp(bstrURL, L"ftp:", 4) == 0)) {
			tstring_ptr tstrURL(wcs2tcs(bstrURL));
			SHELLEXECUTEINFO sei = {
				sizeof(sei),
				0,
				pHtmlMessageViewWindow_->getParentFrame(),
#ifdef _WIN32_WCE
				_T("open"),
#else
				0,
#endif
				tstrURL.get(),
				0,
				0,
				SW_SHOW,
			};
			::ShellExecuteEx(&sei);
		}
	}
	else if (dispId == DISPID_DOCUMENTCOMPLETE) {
		if (!pDispParams || pDispParams->cArgs != 2)
			return E_INVALIDARG;
		
		if (pHtmlMessageViewWindow_->bActivate_) {
			VARIANT* pVarWebBrowser = pDispParams->rgvarg + 1;
			if (pVarWebBrowser->vt != VT_DISPATCH)
				return E_INVALIDARG;
			ComPtr<IWebBrowser2> pWebBrowser;
			HRESULT hr = pVarWebBrowser->pdispVal->QueryInterface(IID_IWebBrowser2,
				reinterpret_cast<void**>(&pWebBrowser));
			if (FAILED(hr))
				return hr;
			if (pWebBrowser.get() == pWebBrowser_) {
				VARIANT* pVarURL = pDispParams->rgvarg;
				if (pVarURL->vt != (VT_VARIANT | VT_BYREF) ||
					pVarURL->pvarVal->vt != VT_BSTR)
					return E_INVALIDARG;
				BSTR bstrURL = pVarURL->pvarVal->bstrVal;
				if (wcscmp(bstrURL, L"about:blank") != 0)
					pHtmlMessageViewWindow_->setActive();
			}
			pHtmlMessageViewWindow_->bActivate_ = false;
		}
	}
	else if (dispId == DISPID_STATUSTEXTCHANGE) {
		if (!pDispParams || pDispParams->cArgs != 1)
			return E_INVALIDARG;
		
		if (pHtmlMessageViewWindow_->pCallback_) {
			VARIANT* pVarText = pDispParams->rgvarg;
			if (pVarText->vt != VT_BSTR)
				return E_INVALIDARG;
			pHtmlMessageViewWindow_->pCallback_->statusTextChanged(pVarText->bstrVal);
		}
	}
	else {
		return E_NOTIMPL;
	}
	
	return S_OK;
}


/****************************************************************************
 *
 * HtmlMessageViewWindow::AmbientDispatchHook
 *
 */

HtmlMessageViewWindow::AmbientDispatchHook::Map qm::HtmlMessageViewWindow::AmbientDispatchHook::map__;

qm::HtmlMessageViewWindow::AmbientDispatchHook::AmbientDispatchHook(HtmlMessageViewWindow* pHtmlMessageViewWindow,
																	IDispatch* pDispatch) :
	nRef_(1),
	pHtmlMessageViewWindow_(pHtmlMessageViewWindow),
	pDispatch_(pDispatch),
	pDispatchVtbl_(0)
{
	IDispatchType* pDispatchType = reinterpret_cast<IDispatchType*>(pDispatch);
	pDispatchVtbl_ = pDispatchType->pVtbl;
	pDispatchType->pVtbl = reinterpret_cast<IDispatchType*>(this)->pVtbl;
	
	map__.push_back(Map::value_type(pDispatchType, this));
}

qm::HtmlMessageViewWindow::AmbientDispatchHook::~AmbientDispatchHook()
{
	Map::iterator it = map__.begin();
	while (it != map__.end() && (*it).second != this)
		++it;
	assert(it != map__.end());
	map__.erase(it);
}

STDMETHODIMP_(ULONG) qm::HtmlMessageViewWindow::AmbientDispatchHook::AddRef()
{
	return (*getVtbl()->AddRef)(getDispatchType());
}

STDMETHODIMP_(ULONG) qm::HtmlMessageViewWindow::AmbientDispatchHook::Release()
{
	AmbientDispatchHook* pThis = getThis();
	ULONG nRef = (*getVtbl()->Release)(getDispatchType());
	if (--pThis->nRef_ == 0) {
		IDispatchType* pDispatchType = reinterpret_cast<IDispatchType*>(pThis->pDispatch_);
		pDispatchType->pVtbl = pThis->pDispatchVtbl_;
		delete pThis;
	}
	return nRef;
}

STDMETHODIMP qm::HtmlMessageViewWindow::AmbientDispatchHook::QueryInterface(REFIID riid,
																			void** ppv)
{
	return (*getVtbl()->QueryInterface)(getDispatchType(), riid, ppv);
}

STDMETHODIMP qm::HtmlMessageViewWindow::AmbientDispatchHook::GetIDsOfNames(REFIID riid,
																		   OLECHAR** rgszNames,
																		   unsigned int cNames,
																		   LCID lcid,
																		   DISPID* pDispId)
{
	return (*getVtbl()->GetIDsOfNames)(getDispatchType(),
		riid, rgszNames, cNames, lcid, pDispId);
}

STDMETHODIMP qm::HtmlMessageViewWindow::AmbientDispatchHook::GetTypeInfo(unsigned int nTypeInfo,
																		 LCID lcid,
																		 ITypeInfo** ppTypeInfo)
{
	return (*getVtbl()->GetTypeInfo)(getDispatchType(), nTypeInfo, lcid, ppTypeInfo);
}

STDMETHODIMP qm::HtmlMessageViewWindow::AmbientDispatchHook::GetTypeInfoCount(unsigned int* pcTypeInfo)
{
	return (*getVtbl()->GetTypeInfoCount)(getDispatchType(), pcTypeInfo);
}

STDMETHODIMP qm::HtmlMessageViewWindow::AmbientDispatchHook::Invoke(DISPID dispId,
																	REFIID riid,
																	LCID lcid,
																	WORD wFlags,
																	DISPPARAMS* pDispParams,
																	VARIANT* pVarResult,
																	EXCEPINFO* pExcepInfo,
																	unsigned int* pnArgErr)
{
	if (dispId == DISPID_AMBIENT_DLCONTROL) {
		::VariantInit(pVarResult);
		pVarResult->vt = VT_I4;
		pVarResult->lVal = DLCTL_DLIMAGES | DLCTL_VIDEOS | DLCTL_BGSOUNDS | DLCTL_NO_CLIENTPULL;
		if (!getThis()->pHtmlMessageViewWindow_->bOnlineMode_)
			pVarResult->lVal |= DLCTL_FORCEOFFLINE;
		return S_OK;
	}
	
	return (*getVtbl()->Invoke)(getDispatchType(), dispId, riid,
		lcid, wFlags, pDispParams, pVarResult, pExcepInfo, pnArgErr);
}

STDMETHODIMP qm::HtmlMessageViewWindow::AmbientDispatchHook::put_AllowWindowlessActivation(VARIANT_BOOL b)
{
	return (*getVtbl()->put_AllowWindowlessActivation)(getDispatchType(), b);
}

STDMETHODIMP qm::HtmlMessageViewWindow::AmbientDispatchHook::get_AllowWindowlessActivation(VARIANT_BOOL* pb)
{
	return (*getVtbl()->get_AllowWindowlessActivation)(getDispatchType(), pb);
}

STDMETHODIMP qm::HtmlMessageViewWindow::AmbientDispatchHook::put_BackColor(OLE_COLOR cr)
{
	return (*getVtbl()->put_BackColor)(getDispatchType(), cr);
}

STDMETHODIMP qm::HtmlMessageViewWindow::AmbientDispatchHook::get_BackColor(OLE_COLOR* pcr)
{
	return (*getVtbl()->get_BackColor)(getDispatchType(), pcr);
}

STDMETHODIMP qm::HtmlMessageViewWindow::AmbientDispatchHook::put_ForeColor(OLE_COLOR cr)
{
	return (*getVtbl()->put_ForeColor)(getDispatchType(), cr);
}

STDMETHODIMP qm::HtmlMessageViewWindow::AmbientDispatchHook::get_ForeColor(OLE_COLOR* pcr)
{
	return (*getVtbl()->get_ForeColor)(getDispatchType(), pcr);
}

STDMETHODIMP qm::HtmlMessageViewWindow::AmbientDispatchHook::put_LocaleID(LCID lcid)
{
	return (*getVtbl()->put_LocaleID)(getDispatchType(), lcid);
}

STDMETHODIMP qm::HtmlMessageViewWindow::AmbientDispatchHook::get_LocaleID(LCID* plcid)
{
	return (*getVtbl()->get_LocaleID)(getDispatchType(), plcid);
}

STDMETHODIMP qm::HtmlMessageViewWindow::AmbientDispatchHook::put_UserMode(VARIANT_BOOL b)
{
	return (*getVtbl()->put_UserMode)(getDispatchType(), b);
}

STDMETHODIMP qm::HtmlMessageViewWindow::AmbientDispatchHook::get_UserMode(VARIANT_BOOL* pb)
{
	return (*getVtbl()->get_UserMode)(getDispatchType(), pb);
}

STDMETHODIMP qm::HtmlMessageViewWindow::AmbientDispatchHook::put_DisplayAsDefault(VARIANT_BOOL b)
{
	return (*getVtbl()->put_DisplayAsDefault)(getDispatchType(), b);
}

STDMETHODIMP qm::HtmlMessageViewWindow::AmbientDispatchHook::get_DisplayAsDefault(VARIANT_BOOL* pb)
{
	return (*getVtbl()->get_DisplayAsDefault)(getDispatchType(), pb);
}

STDMETHODIMP qm::HtmlMessageViewWindow::AmbientDispatchHook::put_Font(IFontDisp* pFont)
{
	return (*getVtbl()->put_Font)(getDispatchType(), pFont);
}

STDMETHODIMP qm::HtmlMessageViewWindow::AmbientDispatchHook::get_Font(IFontDisp** ppFont)
{
	return (*getVtbl()->get_Font)(getDispatchType(), ppFont);
}

STDMETHODIMP qm::HtmlMessageViewWindow::AmbientDispatchHook::put_MessageReflect(VARIANT_BOOL b)
{
	return (*getVtbl()->put_MessageReflect)(getDispatchType(), b);
}

STDMETHODIMP qm::HtmlMessageViewWindow::AmbientDispatchHook::get_MessageReflect(VARIANT_BOOL* pb)
{
	return (*getVtbl()->get_MessageReflect)(getDispatchType(), pb);
}

STDMETHODIMP qm::HtmlMessageViewWindow::AmbientDispatchHook::get_ShowGrabHandles(VARIANT_BOOL* pb)
{
	return (*getVtbl()->get_ShowGrabHandles)(getDispatchType(), pb);
}

STDMETHODIMP qm::HtmlMessageViewWindow::AmbientDispatchHook::get_ShowHatching(VARIANT_BOOL* pb)
{
	return (*getVtbl()->get_ShowHatching)(getDispatchType(), pb);
}

STDMETHODIMP qm::HtmlMessageViewWindow::AmbientDispatchHook::put_DocHostFlags(DWORD dw)
{
	return (*getVtbl()->put_DocHostFlags)(getDispatchType(), dw);
}

STDMETHODIMP qm::HtmlMessageViewWindow::AmbientDispatchHook::get_DocHostFlags(DWORD* pdw)
{
	return (*getVtbl()->get_DocHostFlags)(getDispatchType(), pdw);
}

STDMETHODIMP qm::HtmlMessageViewWindow::AmbientDispatchHook::put_DocHostDoubleClickFlags(DWORD dw)
{
	return (*getVtbl()->put_DocHostDoubleClickFlags)(getDispatchType(), dw);
}

STDMETHODIMP qm::HtmlMessageViewWindow::AmbientDispatchHook::get_DocHostDoubleClickFlags(DWORD* pdw)
{
	return (*getVtbl()->get_DocHostDoubleClickFlags)(getDispatchType(), pdw);
}

STDMETHODIMP qm::HtmlMessageViewWindow::AmbientDispatchHook::put_AllowContextMenu(VARIANT_BOOL b)
{
	return (*getVtbl()->put_AllowContextMenu)(getDispatchType(), b);
}

STDMETHODIMP qm::HtmlMessageViewWindow::AmbientDispatchHook::get_AllowContextMenu(VARIANT_BOOL* pb)
{
	return (*getVtbl()->get_AllowContextMenu)(getDispatchType(), pb);
}

STDMETHODIMP qm::HtmlMessageViewWindow::AmbientDispatchHook::put_AllowShowUI(VARIANT_BOOL b)
{
	return (*getVtbl()->put_AllowShowUI)(getDispatchType(), b);
}

STDMETHODIMP qm::HtmlMessageViewWindow::AmbientDispatchHook::get_AllowShowUI(VARIANT_BOOL* pb)
{
	return (*getVtbl()->get_AllowShowUI)(getDispatchType(), pb);
}

STDMETHODIMP qm::HtmlMessageViewWindow::AmbientDispatchHook::put_OptionKeyPath(BSTR bstr)
{
	return (*getVtbl()->put_OptionKeyPath)(getDispatchType(), bstr);
}

STDMETHODIMP qm::HtmlMessageViewWindow::AmbientDispatchHook::get_OptionKeyPath(BSTR* pbstr)
{
	return (*getVtbl()->get_OptionKeyPath)(getDispatchType(), pbstr);
}

HtmlMessageViewWindow::AmbientDispatchHook* qm::HtmlMessageViewWindow::AmbientDispatchHook::getThis()
{
	return get(reinterpret_cast<IDispatchType*>(this));
}

HtmlMessageViewWindow::AmbientDispatchHook::IDispatchType* qm::HtmlMessageViewWindow::AmbientDispatchHook::getDispatchType()
{
	return reinterpret_cast<IDispatchType*>(reinterpret_cast<char*>(this));
}

HtmlMessageViewWindow::AmbientDispatchHook::IDispatchVtbl* qm::HtmlMessageViewWindow::AmbientDispatchHook::getVtbl()
{
	return getThis()->pDispatchVtbl_;
}

HtmlMessageViewWindow::AmbientDispatchHook* qm::HtmlMessageViewWindow::AmbientDispatchHook::get(IDispatchType* pDispatchType)
{
	Map::const_iterator it = map__.begin();
	while (it != map__.end() && (*it).first != pDispatchType)
		++it;
	assert(it != map__.end());
	return (*it).second;
}

#endif // QMHTMLVIEW
