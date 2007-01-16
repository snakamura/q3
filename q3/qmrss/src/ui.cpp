/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmapplication.h>
#include <qmdocument.h>

#include <qshttp.h>
#include <qsregex.h>

#include "main.h"
#include "resourceinc.h"
#include "ui.h"
#include "util.h"


using namespace qmrss;
using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * ReceivePage
 *
 */

qmrss::ReceivePage::ReceivePage(SubAccount* pSubAccount) :
	DefaultPropertyPage(getResourceHandle(), IDD_RECEIVE),
	pSubAccount_(pSubAccount)
{
}

qmrss::ReceivePage::~ReceivePage()
{
}

LRESULT qmrss::ReceivePage::onCommand(WORD nCode,
									  WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_AUTHENTICATE, onAuthenticate)
		HANDLE_COMMAND_ID_RANGE(IDC_NOPROXY, IDC_CUSTOM, onProxy)
	END_COMMAND_HANDLER()
	return DefaultPropertyPage::onCommand(nCode, nId);
}

LRESULT qmrss::ReceivePage::onInitDialog(HWND hwndFocus,
										 LPARAM lParam)
{
	if (pSubAccount_->getPropertyInt(L"Http", L"UseInternetSetting"))
		sendDlgItemMessage(IDC_INTERNETSETTING, BM_SETCHECK, BST_CHECKED);
	else if (pSubAccount_->getPropertyInt(L"Http", L"UseProxy"))
		sendDlgItemMessage(IDC_CUSTOM, BM_SETCHECK, BST_CHECKED);
	else
		sendDlgItemMessage(IDC_NOPROXY, BM_SETCHECK, BST_CHECKED);
	
	wstring_ptr wstrHost(pSubAccount_->getPropertyString(L"Http", L"ProxyHost"));
	setDlgItemText(IDC_HOST, wstrHost.get());
	setDlgItemInt(IDC_PORT, pSubAccount_->getPropertyInt(L"Http", L"ProxyPort"));
	
	wstring_ptr wstrUserName(pSubAccount_->getPropertyString(L"Http", L"ProxyUserName"));
	wstring_ptr wstrPassword(pSubAccount_->getPropertyString(L"Http", L"ProxyPassword"));
	sendDlgItemMessage(IDC_AUTHENTICATE, BM_SETCHECK,
		*wstrUserName.get() && *wstrPassword.get() ? BST_CHECKED : BST_UNCHECKED);
	setDlgItemText(IDC_USERNAME, wstrUserName.get());
	setDlgItemText(IDC_PASSWORD, wstrPassword.get());
	
	updateState();
	
	return TRUE;
}

LRESULT qmrss::ReceivePage::onOk()
{
	bool bUseInternetSetting = false;
	bool bUseProxy = false;
	if (sendDlgItemMessage(IDC_INTERNETSETTING, BM_GETCHECK) == BST_CHECKED)
		bUseInternetSetting = true;
	else if (sendDlgItemMessage(IDC_CUSTOM, BM_GETCHECK) == BST_CHECKED)
		bUseProxy = true;
	pSubAccount_->setPropertyInt(L"Http", L"UseInternetSetting", bUseInternetSetting);
	pSubAccount_->setPropertyInt(L"Http", L"UseProxy", bUseProxy);
	
	wstring_ptr wstrHost(getDlgItemText(IDC_HOST));
	if (wstrHost.get())
		pSubAccount_->setPropertyString(L"Http", L"ProxyHost", wstrHost.get());
	pSubAccount_->setPropertyInt(L"Http", L"ProxyPort", getDlgItemInt(IDC_PORT));
	
	wstring_ptr wstrUserName;
	wstring_ptr wstrPassword;
	if (sendDlgItemMessage(IDC_AUTHENTICATE, BM_GETCHECK) == BST_CHECKED) {
		wstrUserName = getDlgItemText(IDC_USERNAME);
		wstrPassword = getDlgItemText(IDC_PASSWORD);
	}
	pSubAccount_->setPropertyString(L"Http", L"ProxyUserName",
		wstrUserName.get() ? wstrUserName.get() : L"");
	pSubAccount_->setPropertyString(L"Http", L"ProxyPassword",
		wstrPassword.get() ? wstrPassword.get() : L"");
	
	return DefaultPropertyPage::onOk();
}

LRESULT qmrss::ReceivePage::onAuthenticate()
{
	updateState();
	return 0;
}

LRESULT qmrss::ReceivePage::onProxy(UINT nId)
{
	updateState();
	return 0;
}

void qmrss::ReceivePage::updateState()
{
	bool bCustom = sendDlgItemMessage(IDC_CUSTOM, BM_GETCHECK) == BST_CHECKED;
	bool bAuth = sendDlgItemMessage(IDC_AUTHENTICATE, BM_GETCHECK) == BST_CHECKED;
	Window(getDlgItem(IDC_HOST)).enableWindow(bCustom);
	Window(getDlgItem(IDC_PORT)).enableWindow(bCustom);
	Window(getDlgItem(IDC_AUTHENTICATE)).enableWindow(bCustom);
	Window(getDlgItem(IDC_USERNAME)).enableWindow(bCustom && bAuth);
	Window(getDlgItem(IDC_PASSWORD)).enableWindow(bCustom && bAuth);
}


/****************************************************************************
 *
 * SendPage
 *
 */

qmrss::SendPage::SendPage(SubAccount* pSubAccount) :
	DefaultPropertyPage(getResourceHandle(), IDD_SEND),
	pSubAccount_(pSubAccount)
{
}

qmrss::SendPage::~SendPage()
{
}

LRESULT qmrss::SendPage::onInitDialog(HWND hwndFocus,
									  LPARAM lParam)
{
	return TRUE;
}

LRESULT qmrss::SendPage::onOk()
{
	return DefaultPropertyPage::onOk();
}


/****************************************************************************
 *
 * SubscribeData
 *
 */

qmrss::SubscribeData::SubscribeData(const WCHAR* pwszURL) :
	bMakeMultipart_(false),
	bUseDescriptionAsContent_(false),
	bUpdateIfModified_(false)
{
	if (pwszURL)
		wstrURL_ = allocWString(pwszURL);
}

qmrss::SubscribeData::~SubscribeData()
{
}


/****************************************************************************
 *
 * SubscribeURLPage
 *
 */

qmrss::SubscribeURLPage::SubscribeURLPage(Document* pDocument,
										  Account* pAccount,
										  SubscribeData* pData) :
#if !defined _WIN32_WCE || _WIN32_WCE < 0x300 || !defined _WIN32_WCE_PSPC
	DefaultPropertyPage(getResourceHandle(), IDD_SUBSCRIBEURL),
#else
	DefaultDialog(getResourceHandle(), IDD_SUBSCRIBEURL),
#endif
	pDocument_(pDocument),
	pAccount_(pAccount),
	pData_(pData)
{
}

qmrss::SubscribeURLPage::~SubscribeURLPage()
{
}

#if defined _WIN32_WCE && _WIN32_WCE >= 0x300 && defined _WIN32_WCE_PSPC
LRESULT qmrss::SubscribeURLPage::onInitDialog(HWND hwndFocus,
											  LPARAM lParam)
{
	initialize();
	init(false);
	return TRUE;
}

LRESULT qmrss::SubscribeURLPage::onOk()
{
	if (!next())
		return 0;
	return DefaultDialog::onOk();
}
#endif

LRESULT qmrss::SubscribeURLPage::onCommand(WORD nCode,
										   WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID_CODE(IDC_URL, EN_CHANGE, onURLChange)
	END_COMMAND_HANDLER()
#if !defined _WIN32_WCE || _WIN32_WCE < 0x300 || !defined _WIN32_WCE_PSPC
	return DefaultPropertyPage::onCommand(nCode, nId);
#else
	return DefaultDialog::onCommand(nCode, nId);
#endif
}

LRESULT qmrss::SubscribeURLPage::onURLChange()
{
	updateState();
	return 0;
}

#if !defined _WIN32_WCE || _WIN32_WCE < 0x300 || !defined _WIN32_WCE_PSPC
LRESULT qmrss::SubscribeURLPage::onNotify(NMHDR* pnmhdr,
										  bool* pbHandled)
{
	BEGIN_NOTIFY_HANDLER()
		HANDLE_NOTIFY_CODE(PSN_SETACTIVE, onSetActive)
		HANDLE_NOTIFY_CODE(PSN_WIZNEXT, onWizNext)
	END_NOTIFY_HANDLER()
	return DefaultPropertyPage::onNotify(pnmhdr, pbHandled);
}

LRESULT qmrss::SubscribeURLPage::onSetActive(NMHDR* pnmhdr,
											 bool* pbHandled)
{
	initialize();
	*pbHandled = true;
	return 0;
}

LRESULT qmrss::SubscribeURLPage::onWizNext(NMHDR* pnmhdr,
										   bool* pbHandled)
{
	if (!next())
		setWindowLong(DWLP_MSGRESULT, -1);
	*pbHandled = true;
	return TRUE;
}
#endif

void qmrss::SubscribeURLPage::initialize()
{
	if (pData_->wstrURL_.get())
		setDlgItemText(IDC_URL, pData_->wstrURL_.get());
	updateState();
}

bool qmrss::SubscribeURLPage::next()
{
	wstring_ptr wstrURL(getDlgItemText(IDC_URL));
	
	if (_wcsnicmp(wstrURL.get(), L"http://", 7) != 0 &&
		_wcsnicmp(wstrURL.get(), L"https://", 8) != 0)
		wstrURL = concat(L"http://", wstrURL.get());
	
	std::auto_ptr<Channel> pChannel(getChannel(wstrURL.get(), true));
	if (pChannel.get()) {
		pData_->wstrURL_ = allocWString(pChannel->getURL());
		if (pChannel->getTitle())
			pData_->wstrName_ = allocWString(pChannel->getTitle());
	}
	else {
		if (messageBox(getResourceHandle(), IDS_CONTINUE,
			MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION, getHandle()) != IDYES)
			return false;
		pData_->wstrURL_ = wstrURL;
	}
	pData_->bMakeMultipart_ = true;
	
	return true;
}

void qmrss::SubscribeURLPage::updateState()
{
	wstring_ptr wstrURL(getDlgItemText(IDC_URL));
	bool bEnable = *wstrURL.get() != L'\0';
#if !defined _WIN32_WCE || _WIN32_WCE < 0x300 || !defined _WIN32_WCE_PSPC
	PropSheet_SetWizButtons(getSheet()->getHandle(), bEnable ? PSWIZB_NEXT : 0);
#else
	Window(getDlgItem(IDOK)).enableWindow(bEnable);
#endif
}

std::auto_ptr<Channel> qmrss::SubscribeURLPage::getChannel(const WCHAR* pwszURL,
														   bool bAutoDiscovery) const
{
	std::auto_ptr<HttpURL> pURL(HttpURL::create(pwszURL));
	if (!pURL.get())
		return std::auto_ptr<Channel>();
	
	SubAccount* pSubAccount = pAccount_->getCurrentSubAccount();
	
	std::auto_ptr<Logger> pLogger;
	if (pSubAccount->isLog(Account::HOST_RECEIVE))
		pLogger = pAccount_->openLogger(Account::HOST_RECEIVE);
	
	DefaultCallback callback(pURL->getHost(),
		pSubAccount->getSslOption(), pDocument_->getSecurity());
	std::auto_ptr<Http> pHttp(Util::createHttp(pSubAccount,
		&callback, &callback, &callback, pLogger.get()));
	
	wstring_ptr wstrURL(allocWString(pwszURL));
	std::auto_ptr<HttpMethodGet> pMethod;
	for (int nRedirect = 0; nRedirect < MAX_REDIRECT; ++nRedirect) {
		pMethod.reset(new HttpMethodGet(wstrURL.get()));
		
		wstring_ptr wstrUserAgent(Application::getApplication().getVersion(L'/', false));
		pMethod->setRequestHeader(L"User-Agent", wstrUserAgent.get());
		
		wstring_ptr wstrCookie(HttpUtility::getInternetCookie(wstrURL.get()));
		if (wstrCookie.get() && *wstrCookie.get())
			pMethod->setRequestHeader(L"Cookie", wstrCookie.get());
		
		unsigned int nCode = pHttp->invoke(pMethod.get());
		switch (nCode) {
		case 200:
			break;
		case 301:
		case 302:
		case 303:
		case 307:
			if (nRedirect == MAX_REDIRECT - 1) {
				return std::auto_ptr<Channel>();
			}
			else {
				Part header;
				if (!header.create(0, pMethod->getResponseHeader(), -1))
					return std::auto_ptr<Channel>();
				HttpUtility::updateInternetCookies(wstrURL.get(), header);
				
				wstrURL = HttpUtility::getRedirectLocation(wstrURL.get(), header, 0);
				if (!wstrURL.get())
					return std::auto_ptr<Channel>();
				continue;
			}
		default:
			return std::auto_ptr<Channel>();
		}
		break;
	}
	
	Part header;
	if (!header.create(0, pMethod->getResponseHeader(), -1))
		return std::auto_ptr<Channel>();
	HttpUtility::updateInternetCookies(wstrURL.get(), header);
	
	const ContentTypeParser* pContentType = header.getContentType();
	if (bAutoDiscovery &&
		pContentType &&
		_wcsicmp(pContentType->getMediaType(), L"text") == 0 &&
		_wcsicmp(pContentType->getSubType(), L"html") == 0) {
		
		wstring_ptr wstrCharset(pContentType->getParameter(L"charset"));
		if (!wstrCharset.get())
			wstrCharset = allocWString(L"iso-8859-1");
		
		std::auto_ptr<Converter> pConverter(ConverterFactory::getInstance(wstrCharset.get()));
		if (!pConverter.get())
			pConverter = ConverterFactory::getInstance(L"utf-8");
		
		malloc_size_ptr<unsigned char> pBody(pMethod->getResponseBody());
		size_t nLen = pBody.size();
		wxstring_size_ptr wstrBody(pConverter->decode(reinterpret_cast<char*>(pBody.get()), &nLen));
		if (!wstrBody.get())
			return std::auto_ptr<Channel>();
		
		std::auto_ptr<RegexPattern> pPatternType(RegexCompiler().compile(
			L"<link [^<>]*type=\"?application/(rss|atom|rdf)\\+xml(\\s*;[^\"<>])*\"?[^<>]*/?>",
			RegexCompiler::MODE_MULTILINE | RegexCompiler::MODE_DOTALL | RegexCompiler::MODE_CASEINSENSITIVE));
		const WCHAR* pStart = 0;
		const WCHAR* pEnd = 0;
		pPatternType->search(wstrBody.get(), wstrBody.size(), wstrBody.get(), false, &pStart, &pEnd, 0);
		if (!pStart)
			return std::auto_ptr<Channel>();
		
		std::auto_ptr<RegexPattern> pPatternHref(RegexCompiler().compile(
			L"href=\"?([^ \"]+)\"?",
			RegexCompiler::MODE_MULTILINE | RegexCompiler::MODE_DOTALL | RegexCompiler::MODE_CASEINSENSITIVE));
		RegexRangeList listRange;
		pPatternHref->search(pStart, pEnd - pStart, pStart, false, &pStart, &pEnd, &listRange);
		if (!pStart)
			return std::auto_ptr<Channel>();
		
		const RegexRange& range = listRange.list_[1];
		wstring_ptr wstrFeedURL(allocWString(range.pStart_, range.pEnd_ - range.pStart_));
		wstrFeedURL = HttpUtility::resolveRelativeURL(wstrFeedURL.get(), wstrURL.get());
		return getChannel(wstrFeedURL.get(), false);
	}
	else {
		std::auto_ptr<Channel> pChannel(RssParser().parse(
			pwszURL, pMethod->getResponseBodyAsStream()));
		if (!pChannel.get())
			return std::auto_ptr<Channel>();
		return pChannel;
	}
}


/****************************************************************************
 *
 * SubscribePropertyPage
 *
 */

qmrss::SubscribePropertyPage::SubscribePropertyPage(SubscribeData* pData) :
#if !defined _WIN32_WCE || _WIN32_WCE < 0x300 || !defined _WIN32_WCE_PSPC
	DefaultPropertyPage(getResourceHandle(), IDD_SUBSCRIBEPROPERTY),
#else
	DefaultDialog(getResourceHandle(), IDD_SUBSCRIBEPROPERTY),
#endif
	pData_(pData)
{
}

qmrss::SubscribePropertyPage::~SubscribePropertyPage()
{
}

#if defined _WIN32_WCE && _WIN32_WCE >= 0x300 && defined _WIN32_WCE_PSPC
LRESULT qmrss::SubscribePropertyPage::onInitDialog(HWND hwndFocus,
												   LPARAM lParam)
{
	initialize();
	init(false);
	return TRUE;
}

LRESULT qmrss::SubscribePropertyPage::onOk()
{
	finish();
	return DefaultDialog::onOk();
}
#endif

LRESULT qmrss::SubscribePropertyPage::onCommand(WORD nCode,
												WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID_CODE(IDC_AUTHENTICATE, BN_CLICKED, onAuthenticateClicked)
		HANDLE_COMMAND_ID_CODE(IDC_MAKEMULTIPART, BN_CLICKED, onMakeMultipartClicked)
		HANDLE_COMMAND_ID_CODE(IDC_NAME, EN_CHANGE, onNameChange)
	END_COMMAND_HANDLER()
#if !defined _WIN32_WCE || _WIN32_WCE < 0x300 || !defined _WIN32_WCE_PSPC
	return DefaultPropertyPage::onCommand(nCode, nId);
#else
	return DefaultDialog::onCommand(nCode, nId);
#endif
}

LRESULT qmrss::SubscribePropertyPage::onAuthenticateClicked()
{
	updateState();
	return 0;
}

LRESULT qmrss::SubscribePropertyPage::onMakeMultipartClicked()
{
	updateState();
	return 0;
}

LRESULT qmrss::SubscribePropertyPage::onNameChange()
{
	updateState();
	return 0;
}

#if !defined _WIN32_WCE || _WIN32_WCE < 0x300 || !defined _WIN32_WCE_PSPC
LRESULT qmrss::SubscribePropertyPage::onNotify(NMHDR* pnmhdr,
											   bool* pbHandled)
{
	BEGIN_NOTIFY_HANDLER()
		HANDLE_NOTIFY_CODE(PSN_SETACTIVE, onSetActive)
		HANDLE_NOTIFY_CODE(PSN_WIZFINISH, onWizFinish)
	END_NOTIFY_HANDLER()
	return DefaultPropertyPage::onNotify(pnmhdr, pbHandled);
}

LRESULT qmrss::SubscribePropertyPage::onSetActive(NMHDR* pnmhdr,
												  bool* pbHandled)
{
	initialize();
	*pbHandled = true;
	return 0;
}

LRESULT qmrss::SubscribePropertyPage::onWizFinish(NMHDR* pnmhdr,
												  bool* pbHandled)
{
	finish();
	*pbHandled = true;
	return FALSE;
}
#endif

void qmrss::SubscribePropertyPage::initialize()
{
	if (pData_->wstrName_.get())
		setDlgItemText(IDC_NAME, pData_->wstrName_.get());
	sendDlgItemMessage(IDC_MAKEMULTIPART, BM_SETCHECK,
		pData_->bMakeMultipart_ ? BST_CHECKED : BST_UNCHECKED);
	sendDlgItemMessage(IDC_USEDESCRIPTIONASCONTENT, BM_SETCHECK,
		pData_->bUseDescriptionAsContent_ ? BST_CHECKED : BST_UNCHECKED);
	sendDlgItemMessage(IDC_UPDATEIFMODIFIED, BM_SETCHECK,
		pData_->bUpdateIfModified_ ? BST_CHECKED : BST_UNCHECKED);
	sendDlgItemMessage(IDC_UPDATEIFMODIFIED, BM_SETCHECK,
		pData_->wstrUserName_.get() && pData_->wstrPassword_.get() ? BST_CHECKED : BST_UNCHECKED);
	if (pData_->wstrUserName_.get())
		setDlgItemText(IDC_USERNAME, pData_->wstrUserName_.get());
	if (pData_->wstrPassword_.get())
		setDlgItemText(IDC_PASSWORD, pData_->wstrPassword_.get());
	
	updateState();
}

void qmrss::SubscribePropertyPage::finish()
{
	pData_->wstrName_ = getDlgItemText(IDC_NAME);
	pData_->bMakeMultipart_ = sendDlgItemMessage(IDC_MAKEMULTIPART, BM_GETCHECK) == BST_CHECKED;
	pData_->bUseDescriptionAsContent_ = sendDlgItemMessage(IDC_USEDESCRIPTIONASCONTENT, BM_GETCHECK) == BST_CHECKED;
	pData_->bUpdateIfModified_ = sendDlgItemMessage(IDC_UPDATEIFMODIFIED, BM_GETCHECK) == BST_CHECKED;
	
	if (sendDlgItemMessage(IDC_AUTHENTICATE, BM_GETCHECK) == BST_CHECKED) {
		pData_->wstrUserName_ = getDlgItemText(IDC_USERNAME);
		pData_->wstrPassword_ = getDlgItemText(IDC_PASSWORD);
	}
}

void qmrss::SubscribePropertyPage::updateState()
{
	wstring_ptr wstrName(getDlgItemText(IDC_NAME));
	bool bEnable = *wstrName.get() != L'\0';
#if !defined _WIN32_WCE || _WIN32_WCE < 0x300 || !defined _WIN32_WCE_PSPC
	PropSheet_SetWizButtons(getSheet()->getHandle(),
		PSWIZB_BACK | (bEnable ? PSWIZB_FINISH : PSWIZB_DISABLEDFINISH));
#else
	Window(getDlgItem(IDOK)).enableWindow(bEnable);
#endif
	
	bool bMakeMultipart = sendDlgItemMessage(IDC_MAKEMULTIPART, BM_GETCHECK) == BST_CHECKED;
	Window(getDlgItem(IDC_USEDESCRIPTIONASCONTENT)).enableWindow(bMakeMultipart);
	
	bool bAuthenticate = sendDlgItemMessage(IDC_AUTHENTICATE, BM_GETCHECK) == BST_CHECKED;
	Window(getDlgItem(IDC_USERNAME)).enableWindow(bAuthenticate);
	Window(getDlgItem(IDC_PASSWORD)).enableWindow(bAuthenticate);
}
