/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmdocument.h>
#include <qmmessage.h>

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

qmrss::RssReceiveSession::RssReceiveSession() :
	pDocument_(0),
	pAccount_(0),
	pSubAccount_(0),
	pFolder_(0),
	hwnd_(0),
	pProfile_(0),
	pLogger_(0),
	pSessionCallback_(0)
{
}

qmrss::RssReceiveSession::~RssReceiveSession()
{
}

bool qmrss::RssReceiveSession::init(Document* pDocument,
									Account* pAccount,
									SubAccount* pSubAccount,
									HWND hwnd,
									Profile* pProfile,
									Logger* pLogger,
									ReceiveSessionCallback* pCallback)
{
	assert(pDocument);
	assert(pAccount);
	assert(pSubAccount);
	assert(hwnd);
	assert(pProfile);
	assert(pCallback);
	
	pDocument_ = pDocument;
	pAccount_ = pAccount;
	pSubAccount_ = pSubAccount;
	hwnd_ = hwnd;
	pProfile_ = pProfile;
	pLogger_ = pLogger;
	pSessionCallback_ = pCallback;
	
	return true;
}

bool qmrss::RssReceiveSession::connect()
{
	wstring_ptr wstrPath(concat(pAccount_->getPath(), L"\\feed.xml"));
	pFeedList_.reset(new FeedList(wstrPath.get()));
	return true;
}

void qmrss::RssReceiveSession::disconnect()
{
	// TODO
	// Remove unreferenced feeds from feed list.
	if (pFeedList_->isModified())
		pFeedList_->save();
}

bool qmrss::RssReceiveSession::isConnected()
{
	return true;
}

bool qmrss::RssReceiveSession::selectFolder(NormalFolder* pFolder)
{
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
	if (!pwszURL)
		return false;
	
	// TODO
	// Check if minimum duration has been exceeded since last update.
	
	log.debugf(L"Connecting to the site: %s", pwszURL);
	
	wstring_ptr wstrProxyHost(pSubAccount_->getProperty(L"Http", L"ProxyHost", L""));
	unsigned short nProxyPort(pSubAccount_->getProperty(L"Http", L"ProxyPort", 8080));
	bool bProxy = *wstrProxyHost.get() != L'\0';
	if (bProxy && log.isDebugEnabled())
		log.debugf(L"Using proxy: %s:%u", wstrProxyHost.get(), nProxyPort);
	
	CallbackImpl callback(pSubAccount_, pDocument_->getSecurity(), pSessionCallback_);
	callback.setMessage(IDS_REQUESTRSS);
	pSessionCallback_->setRange(0, 0);
	
	Http http(pSubAccount_->getTimeout(), bProxy ? wstrProxyHost.get() : 0,
		bProxy ? nProxyPort : 0, &callback, &callback, &callback, pLogger_);
	
	const Feed* pFeed = pFeedList_->getFeed(pwszURL);
	
	HttpMethodGet method(pwszURL);
	if (pFeed) {
		wstring_ptr wstrIfModifiedSince(pFeed->getLastModified().format(
			L"%W, %D %M1 %Y4 %h:%m:%s", Time::FORMAT_UTC));
		wstrIfModifiedSince = concat(wstrIfModifiedSince.get(), L" GMT");
		method.setRequestHeader(L"If-Modified-Since", wstrIfModifiedSince.get());
	}
	unsigned int nCode = http.invoke(&method);
	if (nCode == 304)
		return true;
	else if (nCode != 200)
		return false;
	
	callback.setMessage(IDS_PARSERSS);
	
	std::auto_ptr<Channel> pChannel(RssParser().parse(method.getResponseBodyAsStream()));
	if (!pChannel.get())
		return false;
	
	Part header;
	if (!header.create(0, method.getResponseHeader(), -1))
		return false;
	
	Time timePubDate(pChannel->getPubDate());
	if (timePubDate.wYear == 0)
		timePubDate = Time::getCurrentTime();
	
	callback.setMessage(IDS_PROCESSRSS);
	
	Time timeLastModified;
	DateParser lastModified;
	if (header.getField(L"Last-Modified", &lastModified) == Part::FIELD_EXIST)
		timeLastModified = lastModified.getTime();
	else
		timeLastModified = Time::getCurrentTime();
	std::auto_ptr<Feed> pFeedNew(new Feed(pwszURL, timeLastModified));
	
	const Channel::ItemList& listItem = pChannel->getItems();
	pSessionCallback_->setRange(0, listItem.size());
	pSessionCallback_->setPos(0);
	unsigned int n = 0;
	for (Channel::ItemList::const_reverse_iterator it = listItem.rbegin(); it != listItem.rend(); ++it, ++n) {
		const Item* pItem = *it;
		
		pSessionCallback_->setPos(n + 1);
		
		const WCHAR* pwszLink = pItem->getLink();
		if (pwszLink) {
			if (!pFeed || !pFeed->getItem(pwszLink)) {
				unsigned int nFlags = 0;
				
				Part header;
				malloc_size_ptr<unsigned char> pBody;
				
				// TODO
				// Sync filter.
				if (false) {
					HttpMethodGet method(pwszLink);
					unsigned int nCode = http.invoke(&method);
					if (nCode != 200)
						return false;
					
					if (!header.create(0, method.getResponseHeader(), -1))
						return false;
					pBody = method.getResponseBody();
				}
				else {
					nFlags = MessageHolder::FLAG_HEADERONLY;
				}
				
				Message msg;
				if (!createItemMessage(pItem, timePubDate,
					pBody.get() ? &header : 0, pBody.get(), pBody.size(), &msg))
					return false;
				
				Lock<Account> lock(*pAccount_);
				
				xstring_ptr strContent(msg.getContent());
				MessageHolder* pmh = pAccount_->storeMessage(pFolder_,
					strContent.get(), &msg, -1, nFlags, -1, false);
				if (!pmh)
					return false;
				
				pSessionCallback_->notifyNewMessage(pmh);
			}
			
			std::auto_ptr<FeedItem> pItem(new FeedItem(pwszLink));
			pFeedNew->addItem(pItem);
		}
	}
	
	pFeedList_->setFeed(pFeedNew);
	
	// TODO
	// Cache link.
	
	// TODO
	// Check trackbacks.
	
	return true;
}

bool qmrss::RssReceiveSession::applyOfflineJobs()
{
	// TODO
	// Download reserved messages.
	return true;
}

bool qmrss::RssReceiveSession::createItemMessage(const Item* pItem,
												 const Time& timePubDate,
												 const Part* pHeader,
												 const unsigned char* pBody,
												 size_t nBodyLen,
												 Message* pMessage)
{
	assert(pItem);
	assert(pMessage);
	assert((pHeader && pBody) || (!pHeader && !pBody));
	
	const WCHAR* pwszLink = pItem->getLink();
	assert(pwszLink);
	
	UnstructuredParser link(pwszLink, L"utf-8");
	if (!pMessage->setField(L"X-QMAIL-Link", link))
		return false;
	
	if (pBody) {
		ContentTypeParser contentType(L"multipart", L"alternative");
		WCHAR wszBoundary[128];
		Time time(Time::getCurrentTime());
		swprintf(wszBoundary, L"__boundary-%04d%02d%02d%02d%02d%02d%03d%04d__",
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
		{ L"X-RSS-Title",		pItem->getTitle()		},
		{ L"X-RSS-Link",		pItem->getLink()		},
		{ L"X-RSS-Category",	pItem->getCategory()	},
		{ L"X-RSS-Subject",		pItem->getSubject()		},
		{ L"X-RSS-Creator",		pItem->getCreator()		}
	};
	for (int n = 0; n < countof(properties); ++n) {
		if (properties[n].pwszValue_) {
			UnstructuredParser field(properties[n].pwszValue_, L"utf-8");
			if (!pMessage->setField(properties[n].pwszName_, field))
				return false;
		}
	}
	
	UTF8Converter converter;
	
	Part* pTextPart = 0;
	if (pBody) {
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
	
	if (!pMessage->sortHeader())
		return false;
	
	return true;
}


/****************************************************************************
 *
 * RssReceiveSession::CallbackImpl
 *
 */

qmrss::RssReceiveSession::CallbackImpl::CallbackImpl(SubAccount* pSubAccount,
													 const Security* pSecurity,
													 ReceiveSessionCallback* pSessionCallback) :
	DefaultSSLSocketCallback(pSubAccount, Account::HOST_RECEIVE, pSecurity),
	pSubAccount_(pSubAccount),
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

short qmrss::RssReceiveSessionUI::getDefaultPort()
{
	return 0;
}

std::auto_ptr<PropertyPage> qmrss::RssReceiveSessionUI::createPropertyPage(SubAccount* pSubAccount)
{
	return std::auto_ptr<PropertyPage>(new ReceivePage(pSubAccount));
}


/****************************************************************************
 *
 * RssReceiveSessionFactory
 *
 */

RssReceiveSessionFactory qmrss::RssReceiveSessionFactory::factory__;

qmrss::RssReceiveSessionFactory::RssReceiveSessionFactory()
{
	registerFactory(L"rss", this);
}

qmrss::RssReceiveSessionFactory::~RssReceiveSessionFactory()
{
	unregisterFactory(L"rss");
}

std::auto_ptr<ReceiveSession> qmrss::RssReceiveSessionFactory::createSession()
{
	return std::auto_ptr<ReceiveSession>(new RssReceiveSession());
}

std::auto_ptr<ReceiveSessionUI> qmrss::RssReceiveSessionFactory::createUI()
{
	return std::auto_ptr<ReceiveSessionUI>(new RssReceiveSessionUI());
}
