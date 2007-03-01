/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmapplication.h>
#include <qmdocument.h>
#include <qmmessage.h>
#include <qmmessageholder.h>

#include <qsthread.h>

#include "feed.h"
#include "main.h"
#include "resourceinc.h"
#include "rss.h"
#include "rssreceivesession.h"
#include "ui.h"

using namespace qmrss;
using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * RssReceiveSession
 *
 */

qmrss::RssReceiveSession::RssReceiveSession(FeedManager* pFeedManager) :
	pDocument_(0),
	pAccount_(0),
	pSubAccount_(0),
	pFolder_(0),
	pProfile_(0),
	pLogger_(0),
	pSessionCallback_(0),
	pFeedManager_(pFeedManager),
	pFeedList_(0)
{
}

qmrss::RssReceiveSession::~RssReceiveSession()
{
}

bool qmrss::RssReceiveSession::init(Document* pDocument,
									Account* pAccount,
									SubAccount* pSubAccount,
									Profile* pProfile,
									Logger* pLogger,
									ReceiveSessionCallback* pCallback)
{
	assert(pDocument);
	assert(pAccount);
	assert(pSubAccount);
	assert(pProfile);
	assert(pCallback);
	
	pDocument_ = pDocument;
	pAccount_ = pAccount;
	pSubAccount_ = pSubAccount;
	pProfile_ = pProfile;
	pLogger_ = pLogger;
	pSessionCallback_ = pCallback;
	
	return true;
}

void qmrss::RssReceiveSession::term()
{
	clearFeeds();
	if (pFeedList_)
		pFeedList_->save();
}

bool qmrss::RssReceiveSession::connect()
{
	pFeedList_ = pFeedManager_->get(pAccount_);
	return true;
}

void qmrss::RssReceiveSession::disconnect()
{
}

bool qmrss::RssReceiveSession::isConnected()
{
	return true;
}

bool qmrss::RssReceiveSession::selectFolder(NormalFolder* pFolder,
											unsigned int nFlags)
{
	assert(pFolder);
	assert(nFlags == 0);
	
	pFolder_ = pFolder;
	
	return true;
}

bool qmrss::RssReceiveSession::closeFolder()
{
	pFolder_ = 0;
	return true;
}

bool qmrss::RssReceiveSession::updateMessages()
{
	return true;
}

bool qmrss::RssReceiveSession::downloadMessages(const SyncFilterSet* pSyncFilterSet)
{
	Log log(pLogger_, L"qmrss::RssReceiveSession");
	
	const WCHAR* pwszURL = pFolder_->getParam(L"URL");
	if (!pwszURL || !*pwszURL) {
#if 1
		// TODO
		// Not to treat this as an error because there is no way to create
		// a folder which a user doesn't want to sync.
		// See <BTS:872> for details.
		return true;
#else
		reportError(IDS_ERROR_URL, pwszURL, 0);
		return false;
#endif
	}
	
	Time timeCurrent(Time::getCurrentTime());
	FeedItem::Date dateCurrent(FeedItem::convertTimeToDate(timeCurrent));
	
	const Feed* pFeed = pFeedList_->getFeed(pwszURL);
	
	std::auto_ptr<Channel> pChannel;
	Time timeLastModified;
	const WCHAR* pwszCommand = pFolder_->getParam(L"Command");
	if (pwszCommand && *pwszCommand) {
#ifndef _WIN32_WCE
		pChannel = getExecChannel(pwszURL, pwszCommand);
		timeLastModified = timeCurrent;
#else
		return true;
#endif
	}
	else {
		bool bNoChange = false;
		pChannel = getHttpChannel(pwszURL, pFeed, &timeLastModified, &bNoChange);
		if (bNoChange) {
			std::auto_ptr<Feed> pFeedNew(new Feed(pFeed, timeCurrent));
			pFeedList_->setFeed(pFeedNew, -1);
			return true;
		}
	}
	if (!pChannel.get())
		return false;
	
	Time timePubDate(pChannel->getPubDate());
	if (timePubDate.wYear == 0)
		timePubDate = timeCurrent;
	
	setMessage(IDS_PROCESSRSS);
	
	std::auto_ptr<Feed> pFeedNew(new Feed(pwszURL, timeLastModified));
	
	Content content = CONTENT_NONE;
	const WCHAR* pwszMakeMultipart = pFolder_->getParam(L"MakeMultipart");
	if (!pwszMakeMultipart || wcscmp(pwszMakeMultipart, L"false") != 0) {
		const WCHAR* pwsz = pFolder_->getParam(L"UseDescriptionAsContent");
		if (pwsz && wcscmp(pwsz, L"true") == 0)
			content = CONTENT_DESCRIPTION;
		else
			content = CONTENT_CONTENTENCODED;
	}
	
	const WCHAR* pwszUpdateIfModified = pFolder_->getParam(L"UpdateIfModified");
	bool bUpdateIfModified = pwszUpdateIfModified && wcscmp(pwszUpdateIfModified, L"true") == 0;
	
	MessagePtrList listDownloaded;
	
	const Channel::ItemList& listItem = pChannel->getItems();
	pSessionCallback_->setRange(0, listItem.size());
	pSessionCallback_->setPos(0);
	unsigned int nPos = 0;
	for (Channel::ItemList::const_reverse_iterator it = listItem.rbegin(); it != listItem.rend(); ++it, ++nPos) {
		const Item* pItem = *it;
		
		pSessionCallback_->setPos(nPos + 1);
		
		std::pair<const WCHAR*, bool> link(getLink(pChannel.get(), pItem));
		
		const WCHAR* pwszKey = 0;
		if (!bUpdateIfModified) {
			const WCHAR* pwszId = pItem->getId();
			if (pwszId)
				pwszKey = pwszId;
			else if (link.second)
				pwszKey = link.first;
		}
		wstring_ptr wstrHash;
		if (!pwszKey) {
			wstrHash = pItem->getHash();
			pwszKey = wstrHash.get();
		}
		
		if (!pFeed || !pFeed->getItem(pwszKey)) {
			Part header;
			malloc_size_ptr<unsigned char> pBody;
			
			// TODO
			// Sync filter.
#if 0
			if (link.second) {
				HttpMethodGet method(link.first);
				unsigned int nCode = pHttp->invoke(&method);
				if (nCode != 200)
					return false;
				
				if (!header.create(0, method.getResponseHeader(), -1))
					return false;
				pBody = method.getResponseBody();
			}
#endif
			
			Message msg;
			if (!createItemMessage(pChannel.get(), pItem, timePubDate,
				pBody.get() ? &header : 0, pBody.get(), pBody.size(), content, &msg))
				return false;
			
			Lock<Account> lock(*pAccount_);
			
			xstring_size_ptr strContent(msg.getContent());
			unsigned int nFlags = msg.isMultipart() ? 0 : MessageHolder::FLAG_TEXTONLY;
			MessageHolder* pmh = pAccount_->storeMessage(pFolder_, strContent.get(),
				strContent.size(), &msg, -1, nFlags, 0, -1, Account::OPFLAG_BACKGROUND, 0);
			if (!pmh)
				return false;
			
			listDownloaded.push_back(MessagePtr(pmh));
		}
		
		std::auto_ptr<FeedItem> pFeedItem(new FeedItem(pwszKey, dateCurrent));
		pFeedNew->addItem(pFeedItem);
	}
	
	pFeedList_->setFeed(pFeedNew, pSubAccount_->getPropertyInt(L"Rss", L"KeepDay"));
	
	// TODO
	// Cache link.
	
	// TODO
	// Check trackbacks.
	
	bool bApplyRules = (pSubAccount_->getAutoApplyRules() & SubAccount::AUTOAPPLYRULES_NEW) != 0;
	if (bApplyRules) {
		if (!applyRules(&listDownloaded))
			reportError(IDS_ERROR_APPLYRULES, 0, 0);
	}
	for (MessagePtrList::const_iterator it = listDownloaded.begin(); it != listDownloaded.end(); ++it) {
		bool bNotify = false;
		{
			MessagePtrLock mpl(*it);
			bNotify = mpl && !pAccount_->isSeen(mpl);
		}
		if (bNotify)
			pSessionCallback_->notifyNewMessage(*it);
	}
	
	return true;
}

bool qmrss::RssReceiveSession::applyOfflineJobs()
{
	// TODO
	// Download reserved messages.
	return true;
}

std::auto_ptr<Channel> qmrss::RssReceiveSession::getHttpChannel(const WCHAR* pwszURL,
																const Feed* pFeed,
																Time* pTimeLastModified,
																bool* pbNoChange)
{
	assert(pwszURL && *pwszURL);
	assert(pTimeLastModified);
	assert(pbNoChange);
	
	Log log(pLogger_, L"qmrss::RssReceiveSession");
	
	std::auto_ptr<HttpURL> pURL(HttpURL::create(pwszURL));
	if (!pURL.get()) {
		reportError(IDS_ERROR_URL, pwszURL, 0);
		return std::auto_ptr<Channel>();
	}
	
	// TODO
	// Check if minimum duration has been exceeded since last update.
	
	log.debugf(L"Connecting to the site: %s", pwszURL);
	
	CallbackImpl callback(pSubAccount_, pURL->getHost(),
		pDocument_->getSecurity(), pSessionCallback_);
	setMessage(IDS_REQUESTRSS);
	pSessionCallback_->setRange(0, 0);
	
	std::auto_ptr<Http> pHttp(Util::createHttp(pSubAccount_,
		&callback, &callback, &callback, pLogger_));
	
	wstring_ptr wstrURL(allocWString(pwszURL));
	std::auto_ptr<HttpMethodGet> pMethod;
	for (int nRedirect = 0; nRedirect < MAX_REDIRECT; ++nRedirect) {
		pMethod.reset(new HttpMethodGet(wstrURL.get()));
		if (pFeed) {
			wstring_ptr wstrIfModifiedSince(pFeed->getLastModified().format(
				L"%W, %D %M1 %Y4 %h:%m:%s", Time::FORMAT_UTC));
			wstrIfModifiedSince = concat(wstrIfModifiedSince.get(), L" GMT");
			pMethod->setRequestHeader(L"If-Modified-Since", wstrIfModifiedSince.get());
		}
		
		const WCHAR* pwszUserName = pFolder_->getParam(L"UserName");
		const WCHAR* pwszPassword = pFolder_->getParam(L"Password");
		if (pwszUserName && *pwszUserName && pwszPassword && *pwszPassword)
			pMethod->setCredential(pwszUserName, pwszPassword);
		
		wstring_ptr wstrUserAgent(Application::getApplication().getVersion(L'/', false));
		pMethod->setRequestHeader(L"User-Agent", wstrUserAgent.get());
		
		const WCHAR* pwszCookie = pFolder_->getParam(L"Cookie");
		wstring_ptr wstrCookie;
		if (!pwszCookie || !*pwszCookie) {
			wstrCookie = HttpUtility::getInternetCookie(wstrURL.get());
			pwszCookie = wstrCookie.get();
		}
		if (pwszCookie && *pwszCookie)
			pMethod->setRequestHeader(L"Cookie", pwszCookie);
		
		unsigned int nCode = pHttp->invoke(pMethod.get());
		switch (nCode) {
		case 200:
			break;
		case 301:
		case 302:
		case 303:
		case 307:
			if (nRedirect == MAX_REDIRECT - 1) {
				reportError(IDS_ERROR_EXCEEDMAXREDIRECT, wstrURL.get(), pMethod.get());
				return std::auto_ptr<Channel>();
			}
			else {
				Part header;
				if (!header.create(0, pMethod->getResponseHeader(), -1)) {
					reportError(IDS_ERROR_PARSERESPONSEHEADER, wstrURL.get(), pMethod.get());
					return std::auto_ptr<Channel>();
				}
				HttpUtility::updateInternetCookies(wstrURL.get(), header);
				
				HttpUtility::RedirectError error = HttpUtility::REDIRECTERROR_SUCCESS;
				wstrURL = HttpUtility::getRedirectLocation(wstrURL.get(), header, &error);
				if (!wstrURL.get()) {
					UINT nIds[] = {
						0,
						IDS_ERROR_PARSEREDIRECTLOCATION,
						IDS_ERROR_INVALIDREDIRECTLOCATION
					};
					assert(error - HttpUtility::REDIRECTERROR_SUCCESS < countof(nIds));
					reportError(nIds[error - HttpUtility::REDIRECTERROR_SUCCESS], wstrURL.get(), pMethod.get());
					return std::auto_ptr<Channel>();
				}
				continue;
			}
		case 304:
			*pbNoChange = true;
			return std::auto_ptr<Channel>();
		default:
			reportError(IDS_ERROR_GET, wstrURL.get(), pMethod.get());
			return std::auto_ptr<Channel>();
		}
		break;
	}
	
	setMessage(IDS_PARSERSS);
	
	std::auto_ptr<Channel> pChannel(RssParser().parse(
		pwszURL, pMethod->getResponseBodyAsStream()));
	if (!pChannel.get()) {
		reportError(IDS_ERROR_PARSE, wstrURL.get(), 0);
		return std::auto_ptr<Channel>();
	}
	
	Part header;
	if (!header.create(0, pMethod->getResponseHeader(), -1)) {
		reportError(IDS_ERROR_PARSERESPONSEHEADER, wstrURL.get(), 0);
		return std::auto_ptr<Channel>();
	}
	HttpUtility::updateInternetCookies(wstrURL.get(), header);
	
	DateParser lastModified;
	if (header.getField(L"Last-Modified", &lastModified) == Part::FIELD_EXIST)
		*pTimeLastModified = lastModified.getTime();
	else
		*pTimeLastModified = Time::getCurrentTime();
	
	return pChannel;
}

#ifndef _WIN32_WCE
std::auto_ptr<Channel> qmrss::RssReceiveSession::getExecChannel(const WCHAR* pwszURL,
																const WCHAR* pwszCommandLine)
{
	assert(pwszURL && *pwszURL);
	assert(pwszCommandLine && *pwszCommandLine);
	
	Log log(pLogger_, L"qmrss::RssReceiveSession");
	
	setMessage(IDS_REQUESTRSS);
	
	ByteOutputStream os;
	if (Process::exec(pwszCommandLine, 0, &os, 0) != 0) {
		reportError(IDS_ERROR_GET, pwszURL, 0);
		return std::auto_ptr<Channel>();
	}
	
	setMessage(IDS_PARSERSS);
	
	ByteInputStream is(os.getBuffer(), os.getLength(), false);
	std::auto_ptr<Channel> pChannel(RssParser().parse(pwszURL, &is));
	if (!pChannel.get()) {
		reportError(IDS_ERROR_PARSE, pwszURL, 0);
		return std::auto_ptr<Channel>();
	}
	
	return pChannel;
}
#endif

void qmrss::RssReceiveSession::clearFeeds()
{
	if (!pFeedList_)
		return;
	
	Lock<FeedList> lock(*pFeedList_);
	
	FeedList::List listRemove;
	const FeedList::List& listFeed = pFeedList_->getFeeds();
	for (FeedList::List::const_iterator it = listFeed.begin(); it != listFeed.end(); ++it) {
		Feed* pFeed = *it;
		if (!pAccount_->getFolderByParam(L"URL", pFeed->getURL()))
			listRemove.push_back(pFeed);
	}
	for (FeedList::List::const_iterator it = listRemove.begin(); it != listRemove.end(); ++it)
		pFeedList_->removeFeed(*it);
}

bool qmrss::RssReceiveSession::applyRules(MessagePtrList* pList)
{
	RuleManager* pRuleManager = pDocument_->getRuleManager();
	DefaultReceiveSessionRuleCallback callback(pSessionCallback_);
	return pRuleManager->applyAuto(pFolder_, pList,
		pDocument_, pProfile_, RuleManager::AUTOFLAG_NONE, &callback);
}

void qmrss::RssReceiveSession::setMessage(UINT nId)
{
	wstring_ptr wstrMessage(loadString(getResourceHandle(), nId));
	pSessionCallback_->setMessage(wstrMessage.get());
}

void qmrss::RssReceiveSession::reportError(UINT nId,
										   const WCHAR* pwszParam,
										   HttpMethod* pMethod)
{
	HINSTANCE hInst = getResourceHandle();
	
	wstring_ptr wstrMessage(loadString(hInst, IDS_ERROR_MESSAGE));
	
	wstring_ptr wstrDescription(loadString(hInst, nId));
	wstring_ptr wstrResponse;
	if (pMethod)
		wstrResponse = mbs2wcs(pMethod->getResponseLine());
	
	const WCHAR* pwszDescription[] = {
		wstrDescription.get(),
		pwszParam,
		wstrResponse.get()
	};
	
	SessionErrorInfo info(pAccount_, pSubAccount_, pFolder_,
		wstrMessage.get(), 0, pwszDescription, countof(pwszDescription));
	pSessionCallback_->addError(info);
}

bool qmrss::RssReceiveSession::createItemMessage(const Channel* pChannel,
												 const Item* pItem,
												 const Time& timePubDate,
												 const Part* pHeader,
												 const unsigned char* pBody,
												 size_t nBodyLen,
												 Content content,
												 Message* pMessage)
{
	assert(pItem);
	assert(pMessage);
	assert((pHeader && pBody) || (!pHeader && !pBody));
	
	SimpleParser mimeVersion(L"1.0", 0);
	if (!pMessage->setField(L"MIME-Version", mimeVersion))
		return false;
	
	const WCHAR* pwszLink = getLink(pChannel, pItem).first;
	
	UnstructuredParser link(pwszLink, L"utf-8");
	if (!pMessage->setField(L"X-QMAIL-Link", link))
		return false;
	
	const WCHAR* pwszContent = 0;
	switch (content) {
	case CONTENT_CONTENTENCODED:
		pwszContent = pItem->getContentEncoded();
		break;
	case CONTENT_DESCRIPTION:
		pwszContent = pItem->getDescription();
		break;
	}
	
	bool bMultipart = pBody || pwszContent;
	if (bMultipart) {
		ContentTypeParser contentType(L"multipart", L"alternative");
		WCHAR wszBoundary[128];
		Time time(Time::getCurrentTime());
		_snwprintf(wszBoundary, countof(wszBoundary),
			L"__boundary-%04d%02d%02d%02d%02d%02d%03d%04d__",
			time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute,
			time.wSecond, time.wMilliseconds, ::GetCurrentThreadId());
		contentType.setParameter(L"boundary", wszBoundary);
		if (!pMessage->setField(L"Content-Type", contentType))
			return false;
	}
	
	const Time* pTime = &pItem->getPubDate();
	if (pTime->wYear == 0)
		pTime = &timePubDate;
	DateParser date(*pTime);
	if (!pMessage->setField(L"Date", date))
		return false;
	
	const WCHAR* pwszTitle = pItem->getTitle();
	if (pwszTitle) {
		UnstructuredParser title(pwszTitle, L"utf-8");
		if (!pMessage->setField(L"Subject", title))
			return false;
	}
	
	struct {
		const WCHAR* pwszName_;
		const WCHAR* pwszValue_;
	} properties[] = {
		{ L"X-RSS-ChannelTitle",	pChannel->getTitle()	},
		{ L"X-RSS-ChannelLink",		pChannel->getLink()		},
		{ L"X-RSS-Title",			pItem->getTitle()		},
		{ L"X-RSS-Link",			pItem->getLink()		},
		{ L"X-RSS-Category",		pItem->getCategory()	},
		{ L"X-RSS-Subject",			pItem->getSubject()		},
		{ L"X-RSS-Creator",			pItem->getCreator()		}
	};
	for (int n = 0; n < countof(properties); ++n) {
		if (properties[n].pwszValue_) {
			UnstructuredParser field(properties[n].pwszValue_, L"utf-8");
			if (!pMessage->setField(properties[n].pwszName_, field))
				return false;
		}
	}
	
	const Item::EnclosureList& listEnclosure = pItem->getEnclosures();
	for (Item::EnclosureList::const_iterator it = listEnclosure.begin(); it != listEnclosure.end(); ++it) {
		UnstructuredParser field((*it)->getURL(), L"utf-8");
		if (!pMessage->setField(L"X-RSS-Enclosure", field))
			return false;
	}
	
	UTF8Converter converter;
	
	Part* pTextPart = 0;
	if (bMultipart) {
		std::auto_ptr<Part> pPart(new Part());
		pTextPart = pPart.get();
		pMessage->addPart(pPart);
	}
	else {
		pTextPart = pMessage;
	}
	ContentTypeParser contentType(L"text", L"plain");
	contentType.setParameter(L"charset", L"utf-8");
	if (!pTextPart->setField(L"Content-Type", contentType))
		return false;
	
	XStringBuffer<XSTRING> body;
	
	if (!body.append("<") ||
		converter.encode(pwszLink, wcslen(pwszLink), &body) == -1 ||
		!body.append(">\r\n\r\n"))
		return false;
	
	if (!listEnclosure.empty()) {
		for (Item::EnclosureList::const_iterator it = listEnclosure.begin(); it != listEnclosure.end(); ++it) {
			const WCHAR* pwszURL = (*it)->getURL();
			if (!body.append("<") ||
				converter.encode(pwszURL, wcslen(pwszURL), &body) == -1 ||
				!body.append(">\r\n"))
				return false;
		}
		if (!body.append("\r\n"))
			return false;
	}
	
	const WCHAR* pwszDescription = pItem->getDescription();
	if (pwszDescription) {
		if (converter.encode(pwszDescription, wcslen(pwszDescription), &body) == -1 ||
			!body.append("\r\n"))
			return false;
	}
	
	pTextPart->setBody(body.getXString());
	
	if (pBody) {
		std::auto_ptr<Part> pHtmlPart(new Part());
		
		XStringBuffer<XSTRING> body;
		
		ContentTypeParser contentType;
		if (pHeader->getField(L"Content-Type", &contentType) == Part::FIELD_EXIST) {
			if (!pHtmlPart->setField(L"Content-Type", contentType))
				return false;
		}
		
		if (!body.append(reinterpret_cast<const CHAR*>(pBody), nBodyLen))
			return false;
		
		pHtmlPart->setBody(body.getXString());
		pMessage->addPart(pHtmlPart);
	}
	else if (pwszContent) {
		std::auto_ptr<Part> pHtmlPart(new Part());
		
		ContentTypeParser contentType(L"text", L"html");
		contentType.setParameter(L"charset", L"utf-8");
		if (!pHtmlPart->setField(L"Content-Type", contentType))
			return false;
		
		string_ptr strBaseURL(wcs2mbs(pwszLink));
		
		XStringBuffer<XSTRING> body;
		if (!body.append("<html>\r\n") ||
			!body.append("<head>\r\n") ||
			!body.append("<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">\r\n") ||
			!body.append("<base href=\"") ||
			!body.append(strBaseURL.get()) ||
			!body.append("\">\r\n") ||
			!body.append("</head>\r\n") ||
			!body.append("<body>\r\n") ||
			converter.encode(pwszContent, wcslen(pwszContent), &body) == -1 ||
			!body.append("</body>\r\n") ||
			!body.append("</html>\r\n"))
			return false;
		
		pHtmlPart->setBody(body.getXString());
		pMessage->addPart(pHtmlPart);
	}
	
	if (!pMessage->sortHeader())
		return false;
	
	return true;
}

std::pair<const WCHAR*, bool> qmrss::RssReceiveSession::getLink(const Channel* pChannel,
																const Item* pItem)
{
	const WCHAR* pwszLink = pItem->getLink();
	bool bItemLink = true;
	if (!pwszLink) {
		pwszLink = pChannel->getLink();
		if (!pwszLink)
			pwszLink = pChannel->getURL();
		bItemLink = false;
	}
	return std::make_pair(pwszLink, bItemLink);
}


/****************************************************************************
 *
 * RssReceiveSession::CallbackImpl
 *
 */

qmrss::RssReceiveSession::CallbackImpl::CallbackImpl(SubAccount* pSubAccount,
													 const WCHAR* pwszHost,
													 const Security* pSecurity,
													 ReceiveSessionCallback* pSessionCallback) :
	DefaultCallback(pwszHost, pSubAccount->getSslOption(), pSecurity),
	pSessionCallback_(pSessionCallback)
{
}

qmrss::RssReceiveSession::CallbackImpl::~CallbackImpl()
{
}

void qmrss::RssReceiveSession::CallbackImpl::setMessage(UINT nId)
{
	wstring_ptr wstrMessage(loadString(getResourceHandle(), nId));
	pSessionCallback_->setMessage(wstrMessage.get());
}

bool qmrss::RssReceiveSession::CallbackImpl::isCanceled(bool bForce) const
{
	return pSessionCallback_->isCanceled(bForce);
}

void qmrss::RssReceiveSession::CallbackImpl::initialize()
{
	setMessage(IDS_INITIALIZE);
}

void qmrss::RssReceiveSession::CallbackImpl::lookup()
{
	setMessage(IDS_LOOKUP);
}

void qmrss::RssReceiveSession::CallbackImpl::connecting()
{
	setMessage(IDS_CONNECTING);
}

void qmrss::RssReceiveSession::CallbackImpl::connected()
{
	setMessage(IDS_CONNECTED);
}


/****************************************************************************
 *
 * RssReceiveSessionUI
 *
 */

qmrss::RssReceiveSessionUI::RssReceiveSessionUI()
{
}

qmrss::RssReceiveSessionUI::~RssReceiveSessionUI()
{
}

const WCHAR* qmrss::RssReceiveSessionUI::getClass()
{
	return L"rss";
}

wstring_ptr qmrss::RssReceiveSessionUI::getDisplayName()
{
	return loadString(getResourceHandle(), IDS_RSS);
}

short qmrss::RssReceiveSessionUI::getDefaultPort(bool bSecure)
{
	return 0;
}

bool qmrss::RssReceiveSessionUI::isSupported(Support support)
{
	return false;
}

std::auto_ptr<PropertyPage> qmrss::RssReceiveSessionUI::createPropertyPage(SubAccount* pSubAccount)
{
	return std::auto_ptr<PropertyPage>(new ReceivePage(pSubAccount));
}

void qmrss::RssReceiveSessionUI::subscribe(Document* pDocument,
										   Account* pAccount,
										   Folder* pFolder,
										   PasswordCallback* pPasswordCallback,
										   HWND hwnd,
										   void* pParam)
{
	SubscribeData data(static_cast<WCHAR*>(pParam));
	
#if !defined _WIN32_WCE || _WIN32_WCE < 0x300 || !defined _WIN32_WCE_PSPC
	PropertySheetBase sheet(getResourceHandle(), L"Test", false);
	sheet.getHeader().dwFlags |= PSH_WIZARD;
	
	SubscribeURLPage pageURL(pDocument, pAccount, &data);
	sheet.add(&pageURL);
	SubscribePropertyPage pageProperty(&data);
	sheet.add(&pageProperty);
	if (sheet.doModal(hwnd) != IDOK)
		return;
#else
	SubscribeURLPage pageURL(pDocument, pAccount, &data);
	if (pageURL.doModal(hwnd) != IDOK)
		return;
	
	SubscribePropertyPage pageProperty(&data);
	if (pageProperty.doModal(hwnd) != IDOK)
		return;
#endif
	
	const Account::FolderList& listFolder = pAccount->getFolders();
	Account::FolderList::const_iterator it = listFolder.begin();
	while (it != listFolder.end()) {
		const Folder* pFolder = *it;
		if (pFolder->getType() == Folder::TYPE_NORMAL &&
			pFolder->isFlag(Folder::FLAG_SYNCABLE)) {
			const WCHAR* pwszURL = pFolder->getParam(L"URL");
			if (pwszURL && _wcsicmp(pwszURL, data.wstrURL_.get()) == 0)
				break;
		}
		++it;
	}
	if (it != listFolder.end()) {
		if (messageBox(getResourceHandle(), IDS_DUPLICATEDFEED, MB_YESNO | MB_ICONQUESTION, hwnd) != IDYES)
			return;
	}
	
	NormalFolder* pNewFolder = pAccount->createNormalFolder(
		data.wstrName_.get(), pFolder, false, true);
	if (!pNewFolder) {
		messageBox(getResourceHandle(), IDS_ERROR_SUBSCRIBE, MB_OK | MB_ICONERROR, hwnd);
		return;
	}
	
	pNewFolder->setParam(L"URL", data.wstrURL_.get());
	if (data.wstrUserName_.get())
		pNewFolder->setParam(L"UserName", data.wstrUserName_.get());
	if (data.wstrPassword_.get())
		pNewFolder->setParam(L"Password", data.wstrPassword_.get());
	pNewFolder->setParam(L"MakeMultipart", data.bMakeMultipart_ ? L"true" : L"false");
	pNewFolder->setParam(L"UseDescriptionAsContent", data.bUseDescriptionAsContent_ ? L"true" : L"false");
	pNewFolder->setParam(L"UpdateIfModified", data.bUpdateIfModified_ ? L"true" : L"false");
}

bool qmrss::RssReceiveSessionUI::canSubscribe(Account* pAccount,
											  Folder* pFolder)
{
	return true;
}

wstring_ptr qmrss::RssReceiveSessionUI::getSubscribeText()
{
	return loadString(getResourceHandle(), IDS_SUBSCRIBE);
}


/****************************************************************************
 *
 * RssReceiveSessionFactory
 *
 */

RssReceiveSessionFactory qmrss::RssReceiveSessionFactory::factory__;

qmrss::RssReceiveSessionFactory::RssReceiveSessionFactory()
{
	pFeedManager_.reset(new FeedManager());
	
	registerFactory(L"rss", this);
}

qmrss::RssReceiveSessionFactory::~RssReceiveSessionFactory()
{
	unregisterFactory(L"rss");
}

std::auto_ptr<ReceiveSession> qmrss::RssReceiveSessionFactory::createSession()
{
	return std::auto_ptr<ReceiveSession>(new RssReceiveSession(pFeedManager_.get()));
}

std::auto_ptr<ReceiveSessionUI> qmrss::RssReceiveSessionFactory::createUI()
{
	return std::auto_ptr<ReceiveSessionUI>(new RssReceiveSessionUI());
}
