/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#pragma warning(disable:4786)

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
#	ifdef QMHTMLVIEWWEBBROWSER
#		include <exdispid.h>
#		include <mshtmcid.h>
#		include <mshtmdid.h>
#	elif defined QMHTMLVIEWHTMLCTRL
#		include <htmlctrl.h>
#		include <wvdispid.h>
#	else
#		error "Unknown HTMLVIEW"
#	endif
#endif
#ifndef _WIN32_WCE
#	include <tmschema.h>
#endif

#include "messageviewwindow.h"
#include "uiutil.h"
#include "../model/uri.h"
#include "../uimodel/messagemodel.h"

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

qm::MessageViewWindowFactory::MessageViewWindowFactory(MessageWindow* pMessageWindow,
													   Document* pDocument,
													   Profile* pProfile,
													   const WCHAR* pwszSection,
													   MessageModel* pMessageModel,
													   MenuManager* pMenuManager,
													   MessageViewWindowCallback* pCallback,
													   bool bTextOnly) :
	pMessageWindow_(pMessageWindow),
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

bool qm::MessageViewWindowFactory::create()
{
#if defined _WIN32_WCE && _WIN32_WCE >= 300 && defined _WIN32_WCE_PSPC
	DWORD dwExStyle = 0;
#else
	DWORD dwExStyle = WS_EX_CLIENTEDGE;
#endif
	RECT rect;
	pMessageWindow_->getClientRect(&rect);
	return pText_->create(L"QmTextMessageViewWindow", 0, WS_CHILD,
		0, 0, rect.right, rect.bottom, pMessageWindow_->getHandle(),
		dwExStyle, 0, ID_TEXTMESSAGEVIEWWINDOW, 0);
}

MessageViewWindow* qm::MessageViewWindowFactory::getMessageViewWindow(const ContentTypeParser* pContentType)
{
#ifdef QMHTMLVIEW
	bool bHtml = !bTextOnly_ &&
		pContentType &&
		_wcsicmp(pContentType->getMediaType(), L"text") == 0 &&
		_wcsicmp(pContentType->getSubType(), L"html") == 0;
	
	if (bHtml && !pHtml_ && isHtmlViewSupported()) {
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
	if (!pHtml_ && isHtmlViewSupported()) {
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
bool qm::MessageViewWindowFactory::isHtmlViewSupported() const
{
#ifdef QMHTMLVIEWWEBBROWSER
	return Application::getApplication().getAtlHandle() != 0;
#else
	return InitHTMLControl(getInstanceHandle()) != 0;
#endif
}

bool qm::MessageViewWindowFactory::createHtmlView()
{
	std::auto_ptr<HtmlMessageViewWindow> pHtml(new HtmlMessageViewWindow(pProfile_,
		pwszSection_, pMessageWindow_, pMessageModel_, pMenuManager_, pCallback_));
#ifdef QMHTMLVIEWWEBBROWSER
#ifdef _WIN32_WCE
	const WCHAR* pwszId = L"{8856F961-340A-11D0-A96B-00C04FD705A2}";
#else
	const WCHAR* pwszId = L"Shell.Explorer";
#endif
	DWORD dwStyle = WS_CHILD;
	DWORD dwExStyle = WS_EX_CLIENTEDGE;
#else
	const WCHAR* pwszId = 0;
	DWORD dwStyle = WS_CHILD | HS_CONTEXTMENU |
		HS_CLEARTYPE | HS_NOSCRIPTING | HS_NOACTIVEX;
	DWORD dwExStyle = 0;
#endif
	RECT rect;
	pMessageWindow_->getClientRect(&rect);
	if (!pHtml->create(L"QmHtmlMessageViewWindow", pwszId, dwStyle,
		0, 0, rect.right, rect.bottom, pMessageWindow_->getHandle(),
		dwExStyle, 0, ID_HTMLMESSAGEVIEWWINDOW, 0))
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
	pMenuManager_(pMenuManager),
	nScrollPos_(0)
{
	pTextModel_.reset(new ReadOnlyTextModel());
	pTextModel_->addReadOnlyTextModelHandler(this);
	setTextModel(pTextModel_.get());
	setLinkHandler(this);
}

qm::TextMessageViewWindow::~TextMessageViewWindow()
{
	pTextModel_->removeReadOnlyTextModelHandler(this);
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

void qm::TextMessageViewWindow::textLoaded(const qs::ReadOnlyTextModelEvent& event)
{
	if (nScrollPos_ != 0 && getScrollPos() == 0)
		scroll(SCROLL_VERTICALPOS, nScrollPos_, false);
	nScrollPos_ = 0;
}

bool qm::TextMessageViewWindow::openLink(const WCHAR* pwszURL)
{
	return UIUtil::openURL(pwszURL, pProfile_, getParentFrame());
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
	
	nScrollPos_ = 0;
	
	if (pmh) {
		PartUtil util(*pMessage);
		wxstring_size_ptr wstrText;
		if (nFlags & FLAG_RAWMODE) {
			wstrText = util.getAllText(0, pwszEncoding, false);
		}
		else if (nFlags & FLAG_SOURCEMODE) {
			xstring_size_ptr strContent(pMessage->getContent());
			if (strContent.get()) {
				XStringBuffer<WXSTRING> buf;
				if (PartUtil::a2w(strContent.get(), strContent.size(), &buf))
					wstrText = buf.getXStringSize();
			}
		}
		else if (pTemplate) {
			// TODO
			// Performance up.
			// TODO
			// Get selected messages
			TemplateContext context(pmh, pMessage, MessageHolderList(),
				pmh->getFolder()->getAccount(), pDocument_, getHandle(), pwszEncoding,
				MacroContext::FLAG_UITHREAD | MacroContext::FLAG_UI,
				nSecurityMode, pProfile_, 0, TemplateContext::ArgumentList());
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
	getScrollInfo(SB_VERT, &info);
	if ((bPrev && info.nPos != 0) ||
		(!bPrev && info.nPos < static_cast<int>(info.nMax - info.nPage + 1))) {
		scroll(bPrev ? SCROLL_PAGEUP : SCROLL_PAGEDOWN, 0, false);
		return true;
	}
	else {
		return false;
	}
}

int qm::TextMessageViewWindow::getScrollPos() const
{
	return Window::getScrollPos(SB_VERT);
}

void qm::TextMessageViewWindow::setScrollPos(int nPos)
{
	if (pTextModel_->isLoading()) {
		nScrollPos_ = nPos;
	}
	else {
		nScrollPos_ = 0;
		scroll(SCROLL_VERTICALPOS, nPos, false);
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

void qm::TextMessageViewWindow::setZoom(unsigned int nZoom)
{
}

void qm::TextMessageViewWindow::setFit(MessageViewMode::Fit fit)
{
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
	size_t nLine = -1;
	size_t nChar = -1;
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

qm::TextMessageViewWindow::MarkImpl::MarkImpl(size_t nLine,
											  size_t nChar) :
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
 * HtmlContent
 *
 */

qm::HtmlContent::HtmlContent(const WCHAR* pwszContentId,
							 const WCHAR* pwszMimeType,
							 qs::malloc_size_ptr<unsigned char> pData,
							 const void* pCookie) :
	pData_(pData),
	pCookie_(pCookie)
{
	assert(pwszContentId);
	assert(pwszMimeType);
	
	wstrContentId_ = allocWString(pwszContentId);
	wstrMimeType_ = allocWString(pwszMimeType);
}

qm::HtmlContent::~HtmlContent()
{
}

const WCHAR* qm::HtmlContent::getContentId() const
{
	return wstrContentId_.get();
}

const WCHAR* qm::HtmlContent::getMimeType() const
{
	return wstrMimeType_.get();
}

const unsigned char* qm::HtmlContent::getData() const
{
	return pData_.get();
}

size_t qm::HtmlContent::getDataSize() const
{
	return pData_.size();
}

const void* qm::HtmlContent::getCookie() const
{
	return pCookie_;
}


/****************************************************************************
 *
 * HtmlContentManager
 *
 */

qm::HtmlContentManager::HtmlContentManager()
{
}

qm::HtmlContentManager::~HtmlContentManager()
{
	std::for_each(listContent_.begin(), listContent_.end(), qs::deleter<HtmlContent>());
	listContent_.clear();
}

const HtmlContent* qm::HtmlContentManager::get(const WCHAR* pwszContentId) const
{
	ContentList::const_iterator it = std::find_if(
		listContent_.begin(), listContent_.end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				string_equal<WCHAR>(),
				std::mem_fun(&HtmlContent::getContentId),
				std::identity<const WCHAR*>()),
			pwszContentId));
	return it != listContent_.end() ? *it : 0;
}

wstring_ptr qm::HtmlContentManager::prepare(const Message& msg,
											const Part& partHtml,
											const WCHAR* pwszEncoding,
											const void* pCookie)
{
	clear(pCookie);
	
	Time time(Time::getCurrentTime());
	WCHAR wszId[256];
	_snwprintf(wszId, countof(wszId), L"%u%04d%02d%02d%02d%02d%02d%03d@local",
		::GetCurrentProcessId(), time.wYear, time.wMonth, time.wDay,
		time.wHour, time.wMinute, time.wSecond, time.wMilliseconds);
	
	prepare(partHtml, wszId, pwszEncoding, pCookie);
	
	wstring_ptr wstrId(allocWString(wszId));
	
	prepare(msg, 0, 0, pCookie);
	
	return wstrId;
}

void qm::HtmlContentManager::clear(const void* pCookie)
{
	for (ContentList::iterator it = listContent_.begin(); it != listContent_.end(); ) {
		HtmlContent* pContent = *it;
		if (pContent->getCookie() == pCookie) {
			delete pContent;
			it = listContent_.erase(it);
		}
		else {
			++it;
		}
	}
}

void qm::HtmlContentManager::prepare(const qs::Part& part,
									 const WCHAR* pwszId,
									 const WCHAR* pwszEncoding,
									 const void* pCookie)
{
	if (part.isMultipart()) {
		const Part::PartList& l = part.getPartList();
		for (Part::PartList::const_iterator it = l.begin(); it != l.end(); ++it)
			prepare(**it, 0, 0, pCookie);
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
			
			bool bText = _wcsicmp(pwszMediaType, L"text") == 0;
			if (bText && pwszEncoding)
				wstrCharset = allocWString(L"utf-16");
			
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
			
			malloc_size_ptr<unsigned char> pData;
			if (bText && pwszEncoding) {
				wxstring_size_ptr wstrBody(part.getBodyText(pwszEncoding));
				if (!wstrBody.get())
					return;
				malloc_ptr<WCHAR> p(static_cast<WCHAR*>(
					malloc((wstrBody.size() + 2)*sizeof(WCHAR))));
				if (!p.get())
					return;
				*p = 0xfeff;
				wcscpy(p.get() + 1, wstrBody.get());
				pData.reset(reinterpret_cast<unsigned char*>(p.release()),
					(wstrBody.size() + 1)*sizeof(WCHAR));
			}
			else {
				pData = part.getBodyData();
				if (!pData.get())
					return;
			}
			std::auto_ptr<HtmlContent> pContent(new HtmlContent(
				wstrId.get(), wstrMimeType.get(), pData, pCookie));
			listContent_.push_back(pContent.get());
			pContent.release();
		}
	}
}


/****************************************************************************
 *
 * InternetProtocol
 *
 */

qm::InternetProtocol::InternetProtocol(const HtmlContentManager& contentManager) :
	nRef_(0),
	contentManager_(contentManager),
	pContent_(0),
	pCurrent_(0),
	pSink_(0)
{
}

qm::InternetProtocol::~InternetProtocol()
{
}

STDMETHODIMP_(ULONG) qm::InternetProtocol::AddRef()
{
	return ::InterlockedIncrement(reinterpret_cast<LONG*>(&nRef_));
}

STDMETHODIMP_(ULONG) qm::InternetProtocol::Release()
{
	ULONG nRef = ::InterlockedDecrement(reinterpret_cast<LONG*>(&nRef_));
	if (nRef == 0)
		delete this;
	return nRef;
}

STDMETHODIMP qm::InternetProtocol::QueryInterface(REFIID riid,
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

STDMETHODIMP qm::InternetProtocol::Abort(HRESULT hrReason,
										 DWORD dwOptions)
{
	return S_OK;
}

STDMETHODIMP qm::InternetProtocol::Continue(PROTOCOLDATA* pProtocolData)
{
	return E_NOTIMPL;
}

STDMETHODIMP qm::InternetProtocol::Resume()
{
	return E_NOTIMPL;
}

STDMETHODIMP qm::InternetProtocol::Start(LPCWSTR pwszUrl,
										 IInternetProtocolSink* pSink,
										 IInternetBindInfo* pBindInfo,
										 DWORD dwFlags,
										 HANDLE_PTR dwReserved)
{
	if (wcsncmp(pwszUrl, L"cid:", 4) != 0)
		return E_FAIL;
	
	pContent_ = contentManager_.get(pwszUrl + 4);
	if (!pContent_)
		return E_FAIL;
	
	pCurrent_ = pContent_->getData();
	
	pSink_ = pSink;
	
	HRESULT hr = S_OK;
	
	hr = pSink_->ReportProgress(BINDSTATUS_MIMETYPEAVAILABLE, pContent_->getMimeType());
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

STDMETHODIMP qm::InternetProtocol::Suspend()
{
	return E_NOTIMPL;
}

STDMETHODIMP qm::InternetProtocol::Terminate(DWORD dwOptions)
{
	return S_OK;
}

STDMETHODIMP qm::InternetProtocol::LockRequest(DWORD dwOptions)
{
	return S_OK;
}

STDMETHODIMP qm::InternetProtocol::Read(void* pv,
										ULONG cb,
										ULONG* pcbRead)
{
	assert(pv);
	assert(pcbRead);
	
	*pcbRead = 0;
	
	if (cb > pContent_->getDataSize() - (pCurrent_ - pContent_->getData()))
		cb = static_cast<ULONG>(pContent_->getDataSize() - (pCurrent_ - pContent_->getData()));
	
	if (cb == 0)
		return S_FALSE;
	
	memcpy(pv, pCurrent_, cb);
	*pcbRead = cb;
	pCurrent_ += cb;
	
	return S_OK;
}

STDMETHODIMP qm::InternetProtocol::Seek(LARGE_INTEGER move,
										DWORD dwOrigin,
										ULARGE_INTEGER* pNewPos)
{
	return E_NOTIMPL;
}

STDMETHODIMP qm::InternetProtocol::UnlockRequest()
{
	return S_OK;
}

STDMETHODIMP qm::InternetProtocol::CombineUrl(LPCWSTR pwszBaseUrl,
											  LPCWSTR pwszRelativeUrl,
											  DWORD dwCombineFlags,
											  LPWSTR pwszResult,
											  DWORD cchResult,
											  DWORD* pcchResult,
											  DWORD dwReserved)
{
	return E_NOTIMPL;
}

STDMETHODIMP qm::InternetProtocol::CompareUrl(LPCWSTR pwszUrl1,
											  LPCWSTR pwszUrl2,
											  DWORD dwCompareFlags)
{
	return E_NOTIMPL;
}

STDMETHODIMP qm::InternetProtocol::ParseUrl(LPCWSTR pwszUrl,
											PARSEACTION action,
											DWORD dwParseFlags,
											LPWSTR pwszResult,
											DWORD cchResult,
											DWORD* pcchResult,
											DWORD dwReserved)
{
	return E_NOTIMPL;
}

STDMETHODIMP qm::InternetProtocol::QueryInfo(LPCWSTR pwszUrl,
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
 * InternetProtocolFactory
 *
 */

qm::InternetProtocolFactory::InternetProtocolFactory(const HtmlContentManager& contentManager) :
	nRef_(0),
	contentManager_(contentManager)
{
}

qm::InternetProtocolFactory::~InternetProtocolFactory()
{
}

STDMETHODIMP_(ULONG) qm::InternetProtocolFactory::AddRef()
{
	return ::InterlockedIncrement(reinterpret_cast<LONG*>(&nRef_));
}

STDMETHODIMP_(ULONG) qm::InternetProtocolFactory::Release()
{
	ULONG nRef = ::InterlockedDecrement(reinterpret_cast<LONG*>(&nRef_));
	if (nRef == 0)
		delete this;
	return nRef;
}

STDMETHODIMP qm::InternetProtocolFactory::QueryInterface(REFIID riid,
														 void** ppv)
{
	*ppv = 0;
	
	if (riid == IID_IUnknown || riid == IID_IClassFactory) {
		AddRef();
		*ppv = static_cast<IClassFactory*>(this);
	}
	
	return *ppv ? S_OK : E_NOINTERFACE;
}

STDMETHODIMP qm::InternetProtocolFactory::CreateInstance(IUnknown* pUnkOuter,
														 REFIID riid,
														 void** ppvObj)
{
	if (pUnkOuter)
		return CLASS_E_NOAGGREGATION;
	
	if (riid != IID_IInternetProtocol)
		return E_NOINTERFACE;
	
	InternetProtocol* pInternetProtocol = new InternetProtocol(contentManager_);
	pInternetProtocol->AddRef();
	*ppvObj = static_cast<IInternetProtocol*>(pInternetProtocol);
	
	return S_OK;
}

STDMETHODIMP qm::InternetProtocolFactory::LockServer(BOOL bLock)
{
	return E_NOTIMPL;
}


/****************************************************************************
 *
 * InternetSessionInit
 *
 */

InternetSessionInit qm::InternetSessionInit::instance__;

qm::InternetSessionInit::InternetSessionInit()
{
	if (::CoInternetGetSession(0, &pInternetSession_, 0) == S_OK) {
		ComPtr<IClassFactory> pClassFactory(new InternetProtocolFactory(contentManager_));
		pClassFactory->AddRef();
		
		CLSID clsid = { 0x3646a74a, 0x7908, 0x4bed, { 0xb7, 0x6a, 0xab, 0x1a, 0x6f, 0x6b, 0xcf, 0x10 } };
		if (pInternetSession_->RegisterNameSpace(pClassFactory.get(), clsid, L"cid", 0, 0, 0) == S_OK)
			pClassFactory_ = pClassFactory;
	}
}

qm::InternetSessionInit::~InternetSessionInit()
{
	if (pInternetSession_.get() && pClassFactory_.get())
		pInternetSession_->UnregisterNameSpace(pClassFactory_.get(), L"cid");
}

HtmlContentManager& qm::InternetSessionInit::getContentManager()
{
	return instance__.contentManager_;
}


/****************************************************************************
 *
 * HtmlMessageViewWindow
 *
 */

#ifdef QMHTMLVIEWWEBBROWSER

qm::HtmlMessageViewWindow::HtmlMessageViewWindow(Profile* pProfile,
												 const WCHAR* pwszSection,
												 MessageWindow* pMessageWindow,
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
	nScrollPos_(0),
	bOnlineMode_(false)
{
	setWindowHandler(this, false);
}

qm::HtmlMessageViewWindow::~HtmlMessageViewWindow()
{
	InternetSessionInit::getContentManager().clear(this);
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
	
	ComPtr<IHTMLDocument2> pHTMLDocument(getHTMLDocument());
	if (!pHTMLDocument.get())
		return;
	
	READYSTATE rs;
	hr = pWebBrowser_->get_ReadyState(&rs);
	if (SUCCEEDED(hr) && (rs == READYSTATE_COMPLETE || rs == READYSTATE_INTERACTIVE)) {
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
	
	nScrollPos_ = 0;
	
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
		
		wstring_ptr wstrId(InternetSessionInit::getContentManager().prepare(
			*pMessage, *pPart, pwszEncoding, this));
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
	
	ComPtr<IHTMLElement2> pBodyElement(getHTMLBodyElement());
	if (!pBodyElement.get())
		return false;
	
	long nScrollHeight = 0;
	hr = pBodyElement->get_scrollHeight(&nScrollHeight);
	if (FAILED(hr))
		return false;
	long nClientHeight = 0;
	hr = pBodyElement->get_clientHeight(&nClientHeight);
	if (FAILED(hr))
		return false;
	long nScrollTop = 0;
	hr = pBodyElement->get_scrollTop(&nScrollTop);
	if (FAILED(hr))
		return false;
	
	if ((bPrev && nScrollTop != 0) ||
		(!bPrev && nScrollTop + nClientHeight < nScrollHeight)) {
		BSTRPtr bstrScroll(::SysAllocString(bPrev ? L"pageUp" : L"pageDown"));
		VARIANT v;
		v.vt = VT_BSTR;
		v.bstrVal = bstrScroll.get();
		hr = pBodyElement->doScroll(v);
		if (FAILED(hr))
			return false;
		return true;
	}
	
	return false;
}

int qm::HtmlMessageViewWindow::getScrollPos() const
{
	ComPtr<IHTMLElement2> pBodyElement(getHTMLBodyElement());
	if (!pBodyElement.get())
		return 0;
	
	long nScrollTop = 0;
	HRESULT hr = pBodyElement->get_scrollTop(&nScrollTop);
	if (FAILED(hr))
		return 0;
	
	return static_cast<int>(nScrollTop);
}

void qm::HtmlMessageViewWindow::setScrollPos(int nPos)
{
	HRESULT hr = S_OK;
	
	READYSTATE rs;
	hr = pWebBrowser_->get_ReadyState(&rs);
	if (SUCCEEDED(hr) && rs == READYSTATE_COMPLETE) {
		ComPtr<IHTMLElement2> pBodyElement(getHTMLBodyElement());
		if (pBodyElement.get())
			pBodyElement->put_scrollTop(nPos);
		nScrollPos_ = 0;
	}
	else {
		nScrollPos_ = nPos;
	}
}

void qm::HtmlMessageViewWindow::setSelectMode(bool bSelectMode)
{
}

void qm::HtmlMessageViewWindow::setQuoteMode(bool bQuoteMode)
{
}

void qm::HtmlMessageViewWindow::setZoom(unsigned int nZoom)
{
	if (nZoom != MessageViewMode::ZOOM_NONE) {
		VARIANT varZoom;
		varZoom.vt = VT_I4;
		varZoom.lVal = nZoom;
		pWebBrowser_->ExecWB(OLECMDID_ZOOM, OLECMDEXECOPT_DONTPROMPTUSER, &varZoom, 0);
	}
}

void qm::HtmlMessageViewWindow::setFit(MessageViewMode::Fit fit)
{
}

bool qm::HtmlMessageViewWindow::find(const WCHAR* pwszFind,
									 unsigned int nFlags)
{
	assert(pwszFind);
	
	bool bPrev = (nFlags & MessageWindow::FIND_PREVIOUS) != 0;
	
	HRESULT hr = S_OK;
	
	ComPtr<IHTMLDocument2> pHTMLDocument(getHTMLDocument());
	if (!pHTMLDocument.get())
		return false;
	
	ComPtr<IHTMLSelectionObject> pSelection;
	hr = pHTMLDocument->get_selection(&pSelection);
	if (FAILED(hr) || !pSelection.get())
		return false;
	
	ComPtr<IDispatch> pDispRange;
	hr = pSelection->createRange(&pDispRange);
	if (FAILED(hr) || !pDispRange.get())
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
		if (FAILED(hr) || !pElement.get())
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

ComPtr<IHTMLDocument2> qm::HtmlMessageViewWindow::getHTMLDocument() const
{
	HRESULT hr = S_OK;
	
	ComPtr<IDispatch> pDisp;
	hr = pWebBrowser_->get_Document(&pDisp);
	if (FAILED(hr) || !pDisp.get())
		return ComPtr<IHTMLDocument2>();
	
	ComPtr<IHTMLDocument2> pHTMLDocument;
	hr = pDisp->QueryInterface(IID_IHTMLDocument2,
		reinterpret_cast<void**>(&pHTMLDocument));
	if (FAILED(hr))
		return ComPtr<IHTMLDocument2>();
	
	return pHTMLDocument;
}

ComPtr<IHTMLElement2> qm::HtmlMessageViewWindow::getHTMLBodyElement() const
{
	HRESULT hr = S_OK;
	
	ComPtr<IHTMLDocument2> pHTMLDocument(getHTMLDocument());
	if (!pHTMLDocument.get())
		return ComPtr<IHTMLElement2>();
	
	ComPtr<IHTMLElement> pBodyElement;
	hr = pHTMLDocument->get_body(&pBodyElement);
	if (FAILED(hr) || !pBodyElement.get())
		return ComPtr<IHTMLElement2>();
	
	ComPtr<IHTMLElement2> pBodyElement2;
	hr = pBodyElement->QueryInterface(IID_IHTMLElement2,
		reinterpret_cast<void**>(&pBodyElement2));
	if (FAILED(hr))
		return ComPtr<IHTMLElement2>();
	
	return pBodyElement2;
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
#ifndef DISPID_NEWWINDOW3
#	define DISPID_NEWWINDOW3 273
#endif
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
		if (pVarURL->vt != (VT_VARIANT | VT_BYREF))
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
			wcsncmp(bstrURL, L"ftp:", 4) == 0 ||
			wcsncmp(bstrURL, L"mailto:", 7) == 0))
			UIUtil::openURL(bstrURL, pHtmlMessageViewWindow_->pProfile_,
				pHtmlMessageViewWindow_->getParentFrame());
	}
	else if (dispId == DISPID_NEWWINDOW3) {
		if (!pDispParams || pDispParams->cArgs != 5)
			return E_INVALIDARG;
		
		VARIANT* pVarURL = pDispParams->rgvarg;
		if (pVarURL->vt != VT_BSTR)
			return E_INVALIDARG;
		BSTR bstrURL = pVarURL->bstrVal;
		
		VARIANT* pVarCancel = pDispParams->rgvarg + 3;
		if (pVarCancel->vt != (VT_BOOL | VT_BYREF))
			return E_INVALIDARG;
		*pVarCancel->pboolVal = VARIANT_TRUE;
		
		UIUtil::openURL(bstrURL, pHtmlMessageViewWindow_->pProfile_,
			pHtmlMessageViewWindow_->getParentFrame());
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
		if (pHtmlMessageViewWindow_->nScrollPos_ != 0) {
			if (pHtmlMessageViewWindow_->getScrollPos() == 0)
				pHtmlMessageViewWindow_->setScrollPos(pHtmlMessageViewWindow_->nScrollPos_);
			pHtmlMessageViewWindow_->nScrollPos_ = 0;
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

#elif defined QMHTMLVIEWHTMLCTRL

qm::HtmlMessageViewWindow::HtmlMessageViewWindow(qs::Profile* pProfile,
												 const WCHAR* pwszSection,
												 MessageWindow* pMessageWindow,
												 MessageModel* pMessageModel,
												 qs::MenuManager* pMenuManager,
												 MessageViewWindowCallback* pCallback) :
	WindowBase(true),
	pProfile_(pProfile),
	pwszSection_(pwszSection),
	pMessageWindow_(pMessageWindow),
	pMessageModel_(pMessageModel),
	pMenuManager_(pMenuManager),
	pCallback_(pCallback),
	nId_(0),
#if _WIN32_WCE >= 420
	pWebBrowser_(0),
#endif
#if 0
	pWebBrowserEvents_(0),
	dwConnectionPointCookie_(0),
#endif
	bAllowExternal_(false),
	nScrollPos_(0),
	bOnlineMode_(false)
{
	setWindowHandler(this, false);
}

qm::HtmlMessageViewWindow::~HtmlMessageViewWindow()
{
}

wstring_ptr qm::HtmlMessageViewWindow::getSuperClass()
{
	return WC_HTML;
}

LRESULT qm::HtmlMessageViewWindow::windowProc(UINT uMsg,
											  WPARAM wParam,
											  LPARAM lParam)
{
	BEGIN_MESSAGE_HANDLER()
		HANDLE_CREATE()
		HANDLE_DESTROY()
	END_MESSAGE_HANDLER()
	return DefaultWindowHandler::windowProc(uMsg, wParam, lParam);
}

LRESULT qm::HtmlMessageViewWindow::onCreate(CREATESTRUCT* pCreateStruct)
{
	if (DefaultWindowHandler::onCreate(pCreateStruct) == -1)
		return -1;
	
	nId_ = getWindowLong(GWL_ID);
	
	pMessageWindow_->addNotifyHandler(this);
	
#if _WIN32_WCE >= 420
	ComPtr<IDispatch> pDispBrowser;
	sendMessage(DTM_BROWSERDISPATCH, 0, reinterpret_cast<LPARAM>(&pDispBrowser));
	if (!pDispBrowser.get())
		return -1;
	HRESULT hr = pDispBrowser->QueryInterface(IID_IBrowser2,
		reinterpret_cast<void**>(&pWebBrowser_));
	if (FAILED(hr))
		return -1;
#endif
	
#if 0
	pWebBrowserEvents_ = new DWebBrowserEvents2Impl(this, pWebBrowser_);
	pWebBrowserEvents_->AddRef();
	ComPtr<IConnectionPointContainer> pConnectionPointContainer;
	hr = pWebBrowser_->QueryInterface(IID_IConnectionPointContainer,
		reinterpret_cast<void**>(&pConnectionPointContainer));
	if (FAILED(hr))
		return -1;
	
	ComPtr<IEnumConnectionPoints> pEnum;
	hr = pConnectionPointContainer->EnumConnectionPoints(&pEnum);
	if (FAILED(hr))
		return -1;
	
	ComPtr<IConnectionPoint> pConnectionPoint;
	hr = pConnectionPointContainer->FindConnectionPoint(
		IID_IDispatch/*DIID__DPIEWebBrowserEvents2*/, &pConnectionPoint);
	if (FAILED(hr))
		return -1;
	hr = pConnectionPoint->Advise(pWebBrowserEvents_, &dwConnectionPointCookie_);
	if (FAILED(hr))
		return -1;
#endif
	
	return 0;
}

LRESULT qm::HtmlMessageViewWindow::onDestroy()
{
	pMessageWindow_->removeNotifyHandler(this);
	
#if _WIN32_WCE >= 420
	if (pWebBrowser_) {
#if 0
		if (dwConnectionPointCookie_ != 0) {
			ComPtr<IConnectionPointContainer> pConnectionPointContainer;
			ComPtr<IConnectionPoint> pConnectionPoint;
			HRESULT hr = pWebBrowser_->QueryInterface(IID_IConnectionPointContainer,
				reinterpret_cast<void**>(&pConnectionPointContainer));
			if (SUCCEEDED(hr))
				hr = pConnectionPointContainer->FindConnectionPoint(
					IID_IDispatch/*DIID__DPIEWebBrowserEvents2*/, &pConnectionPoint);
			if (SUCCEEDED(hr))
				hr = pConnectionPoint->Unadvise(dwConnectionPointCookie_);
		}
#endif
		pWebBrowser_->Release();
		pWebBrowser_ = 0;
	}
#endif
#if 0
	if (pWebBrowserEvents_) {
		pWebBrowserEvents_->Release();
		pWebBrowserEvents_ = 0;
	}
#endif
	
	return DefaultWindowHandler::onDestroy();
}

LRESULT qm::HtmlMessageViewWindow::onNotify(NMHDR* pnmhdr,
											bool* pbHandled)
{
	BEGIN_NOTIFY_HANDLER()
		HANDLE_NOTIFY(NM_CONTEXTMENU, 0xffffffff, onContextMenu)
		HANDLE_NOTIFY(NM_DOCUMENTCOMPLETE, nId_, onDocumentComplete)
		HANDLE_NOTIFY(NM_HOTSPOT, nId_, onHotSpot)
		HANDLE_NOTIFY(NM_INLINE_IMAGE, nId_, onInlineImage)
		HANDLE_NOTIFY(NM_INLINE_SOUND, nId_, onInline)
		HANDLE_NOTIFY(NM_INLINE_XML, nId_, onInline)
		HANDLE_NOTIFY(NM_META, nId_, onMeta)
	END_NOTIFY_HANDLER()
	return NotifyHandler::onNotify(pnmhdr, pbHandled);
}

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
	setFocus();
}

bool qm::HtmlMessageViewWindow::setMessage(MessageHolder* pmh,
										   Message* pMessage,
										   const Template* pTemplate,
										   const WCHAR* pwszEncoding,
										   unsigned int nFlags,
										   unsigned int nSecurityMode)
{
	assert(pmh && pMessage);
	
	nScrollPos_ = 0;
	
	Account* pAccount = pmh->getFolder()->getAccount();
	
	HRESULT hr = S_OK;
	
	bOnlineMode_ = (nFlags & FLAG_ONLINEMODE) != 0;
	
	sendMessage(DTM_ENABLESCRIPTING, 0, (nFlags & FLAG_INTERNETZONE) != 0);
	
	wstring_ptr wstrURL;
	bool bClear = true;
	
	UnstructuredParser link;
	if (pAccount->isSupport(Account::SUPPORT_EXTERNALLINK) &&
		!pMessage->isMultipart() &&
		pMessage->getField(L"X-QMAIL-Link", &link) == Part::FIELD_EXIST) {
		wstrURL = allocWString(link.getValue());
		bAllowExternal_ = true;
		
#if _WIN32_WCE >= 420
		BSTRPtr bstrPrevURL;
		if (pWebBrowser_->get_LocationURL(&bstrPrevURL) == S_OK) {
			const WCHAR* p1 = wcsrchr(wstrURL.get(), L'#');
			size_t nLen1 = p1 ? p1 - wstrURL.get() : wcslen(wstrURL.get());
			const WCHAR* p2 = wcsrchr(bstrPrevURL.get(), L'#');
			size_t nLen2 = p2 ? p2 - bstrPrevURL.get() : wcslen(bstrPrevURL.get());
			bClear = nLen1 != nLen2 || _wcsnicmp(wstrURL.get(), bstrPrevURL.get(), nLen1) != 0;
		}
#endif
	}
	else {
		PartUtil util(*pMessage);
		const Part* pPart = util.getAlternativePart(L"text", L"html");
		assert(pPart);
		
		wstring_ptr wstrId(InternetSessionInit::getContentManager().prepare(
			*pMessage, *pPart, pwszEncoding, this));
		wstrURL = concat(L"cid:", wstrId.get());
		bAllowExternal_ = false;
	}
	
	if (bClear)
		sendMessage(DTM_CLEAR);
	
	sendMessage(DTM_NAVIGATE, 0, reinterpret_cast<LPARAM>(wstrURL.get()));
	
	return true;
}

bool qm::HtmlMessageViewWindow::scrollPage(bool bPrev)
{
	Window wnd(Window::getWindow(GW_CHILD));
	SCROLLINFO info = {
		sizeof(info),
		SIF_ALL
	};
	wnd.getScrollInfo(SB_VERT, &info);
	if ((bPrev && info.nPos != 0) ||
		(!bPrev && info.nPos < static_cast<int>(info.nMax - info.nPage + 1))) {
		wnd.sendMessage(WM_VSCROLL, bPrev ? SB_PAGEUP : SB_PAGEDOWN);
		return true;
	}
	else {
		return false;
	}
}

int qm::HtmlMessageViewWindow::getScrollPos() const
{
	return Window(Window::getWindow(GW_CHILD)).getScrollPos(SB_VERT);
}

void qm::HtmlMessageViewWindow::setScrollPos(int nPos)
{
	Window(Window::getWindow(GW_CHILD)).setScrollPos(SB_VERT, nPos);
}

void qm::HtmlMessageViewWindow::setSelectMode(bool bSelectMode)
{
}

void qm::HtmlMessageViewWindow::setQuoteMode(bool bQuoteMode)
{
}

void qm::HtmlMessageViewWindow::setZoom(unsigned int nZoom)
{
	if (nZoom != MessageViewMode::ZOOM_NONE)
		sendMessage(DTM_ZOOMLEVEL, 0, nZoom);
}

void qm::HtmlMessageViewWindow::setFit(MessageViewMode::Fit fit)
{
#if _WIN32_WCE >= 420
	IBrowser3* pBrowser3 = 0;
	HRESULT hr = pWebBrowser_->QueryInterface(IID_IBrowser3,
		reinterpret_cast<void**>(&pBrowser3));
	if (hr != S_OK)
		pBrowser3 = 0;
	
	switch (fit) {
	case MessageViewMode::FIT_NONE:
		pWebBrowser_->put_FitToWindow(VARIANT_FALSE);
		if (pBrowser3)
//			pBrowser3->put_SuperFitToWindow(VARIANT_FALSE);
		break;
	case MessageViewMode::FIT_NORMAL:
		pWebBrowser_->put_FitToWindow(VARIANT_TRUE);
		if (pBrowser3)
//			pBrowser3->put_SuperFitToWindow(VARIANT_FALSE);
		break;
	case MessageViewMode::FIT_SUPER:
		pWebBrowser_->put_FitToWindow(VARIANT_FALSE);
		if (pBrowser3)
//			pBrowser3->put_SuperFitToWindow(VARIANT_TRUE);
		break;
	default:
		assert(false);
		break;
	}
#endif
}

bool qm::HtmlMessageViewWindow::find(const WCHAR* pwszFind,
									 unsigned int nFlags)
{
	return false;
}

unsigned int qm::HtmlMessageViewWindow::getSupportedFindFlags() const
{
	return -1;
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
	HWND hwnd = Window::getWindow(GW_CHILD);
	if (hwnd)
		Window(hwnd).sendMessage(WM_COMMAND, 0x000156BA);
	else
		sendMessage(WM_COPY);
}

bool qm::HtmlMessageViewWindow::canCopy()
{
	return sendMessage(DTM_ISSELECTION) != 0;
}

void qm::HtmlMessageViewWindow::selectAll()
{
	sendMessage(DTM_SELECTALL);
}

bool qm::HtmlMessageViewWindow::canSelectAll()
{
	return true;
}

LRESULT qm::HtmlMessageViewWindow::onContextMenu(NMHDR* pnmhdr,
												 bool* pbHandled)
{
	*pbHandled = true;
	
	NM_HTMLCONTEXT* pContext = reinterpret_cast<NM_HTMLCONTEXT*>(pnmhdr);
	
	setActive();
	
	HMENU hmenu = pMenuManager_->getMenu(L"message", false, false);
	if (hmenu) {
		UINT nFlags = TPM_LEFTALIGN | TPM_TOPALIGN;
#ifndef _WIN32_WCE
		nFlags |= TPM_LEFTBUTTON | TPM_RIGHTBUTTON;
#endif
		::TrackPopupMenu(hmenu, nFlags, pContext->pt.x, pContext->pt.y, 0, getParentFrame(), 0);
	}
	
	return 1;
}

LRESULT qm::HtmlMessageViewWindow::onDocumentComplete(NMHDR* pnmhdr,
													  bool* pbHandled)
{
	*pbHandled = true;
	
	if (nScrollPos_ != 0) {
		if (getScrollPos() == 0)
			Window(Window::getWindow(GW_CHILD)).setScrollPos(SB_VERT, nScrollPos_);
		nScrollPos_ = 0;
	}
	return 1;
}

LRESULT qm::HtmlMessageViewWindow::onHotSpot(NMHDR* pnmhdr,
											 bool* pbHandled)
{
	*pbHandled = true;
	
	wstring_ptr wstrURL(getTarget(pnmhdr));
	if (wcsncmp(wstrURL.get(), L"http:", 5) == 0 ||
		wcsncmp(wstrURL.get(), L"https:", 6) == 0 ||
		wcsncmp(wstrURL.get(), L"ftp:", 4) == 0 ||
		wcsncmp(wstrURL.get(), L"mailto:", 7) == 0)
		UIUtil::openURL(wstrURL.get(), pProfile_, getParentFrame());
	return 1;
}

LRESULT qm::HtmlMessageViewWindow::onInlineImage(NMHDR* pnmhdr,
												 bool* pbHandled)
{
	*pbHandled = true;
	
	wstring_ptr wstrURL(getTarget(pnmhdr));
	if (bOnlineMode_ || wcsncmp(wstrURL.get(), L"cid:", 4) == 0)
		return 0;
	
	sendMessage(DTM_IMAGEFAIL, 0, reinterpret_cast<NM_HTMLVIEW*>(pnmhdr)->dwCookie);
	
	return 1;
}

LRESULT qm::HtmlMessageViewWindow::onInline(NMHDR* pnmhdr,
											bool* pbHandled)
{
	*pbHandled = true;
	
	wstring_ptr wstrURL(getTarget(pnmhdr));
	if (bOnlineMode_ || wcsncmp(wstrURL.get(), L"cid:", 4) == 0)
		return 0;
	else
		return 1;
}

LRESULT qm::HtmlMessageViewWindow::onMeta(NMHDR* pnmhdr,
										  bool* pbHandled)
{
	*pbHandled = true;
	
	wstring_ptr wstrHttpEquiv(getTarget(pnmhdr));
	if (_wcsicmp(wstrHttpEquiv.get(), L"content-type") == 0 ||
		_wcsicmp(wstrHttpEquiv.get(), L"refresh") == 0)
		return 1;
	else
		return 0;
}

wstring_ptr qm::HtmlMessageViewWindow::getTarget(NMHDR* pnmhdr) const
{
	// TODO
	bool bUnicode = false;
	if (bUnicode) {
		NM_HTMLVIEWW* pHtmlView = reinterpret_cast<NM_HTMLVIEWW*>(pnmhdr);
		return allocWString(pHtmlView->szTarget);
	}
	else {
		NM_HTMLVIEWA* pHtmlView = reinterpret_cast<NM_HTMLVIEWA*>(pnmhdr);
		return mbs2wcs(pHtmlView->szTarget);
	}
}


#if 0
/****************************************************************************
 *
 * HtmlMessageViewWindow::DWebBrowserEvents2Impl
 *
 */

qm::HtmlMessageViewWindow::DWebBrowserEvents2Impl::DWebBrowserEvents2Impl(HtmlMessageViewWindow* pHtmlMessageViewWindow,
																		  IBrowser2* pWebBrowser) :
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
		riid == DIID__DPIEWebBrowserEvents2) {
		AddRef();
		*ppv = static_cast<_DPIEWebBrowserEvents2*>(this);
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
	if (dispId == DISPID_BEFORENAVIGATE) {
		if (!pDispParams || pDispParams->cArgs != 6)
			return E_INVALIDARG;
		
		VARIANT* pVarURL = pDispParams->rgvarg + 5;
		if (pVarURL->vt != VT_BSTR)
			return E_INVALIDARG;
		BSTR bstrURL = pVarURL->bstrVal;
		bool bAllow = pHtmlMessageViewWindow_->bAllowExternal_ ||
			wcsncmp(bstrURL, L"cid:", 4) == 0;
		pHtmlMessageViewWindow_->bAllowExternal_ = false;
		
		if (!bAllow)
			pWebBrowser_->Stop();
		
		if (!bAllow &&
			(wcsncmp(bstrURL, L"http:", 5) == 0 ||
			wcsncmp(bstrURL, L"https:", 6) == 0 ||
			wcsncmp(bstrURL, L"ftp:", 4) == 0) ||
			wcsncmp(bstrURL, L"mailto:", 7) == 0)
			UIUtil::openURL(bstrURL, pHtmlMessageViewWindow_->pProfile_,
				pHtmlMessageViewWindow_->getParentFrame());
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
#endif

#endif // QMHTMLVIEWHTMLCTRL

#endif // QMHTMLVIEW
