/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#pragma warning(disable:4786)

#include <qsconv.h>
#include <qsfile.h>
#include <qsmime.h>
#include <qsstl.h>
#include <qsstream.h>

#include "feed.h"

using namespace qmrss;
using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * FeedList
 *
 */

qmrss::FeedList::FeedList(const WCHAR* pwszPath) :
	bModified_(false)
{
	wstrPath_ = allocWString(pwszPath);
	
	if (!load()) {
		// TODO
	}
}

qmrss::FeedList::~FeedList()
{
	std::for_each(list_.begin(), list_.end(), qs::deleter<Feed>());
}

const FeedList::List& qmrss::FeedList::getFeeds() const
{
	assert(isLocked());
	return list_;
}

const Feed* qmrss::FeedList::getFeed(const WCHAR* pwszURL) const
{
	Lock<FeedList> lock(*this);
	
	List::const_iterator it = std::find_if(list_.begin(), list_.end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				string_equal<WCHAR>(),
				std::mem_fun(&Feed::getURL),
				std::identity<const WCHAR*>()),
			pwszURL));
	return it != list_.end() ? *it : 0;
}

void qmrss::FeedList::setFeed(std::auto_ptr<Feed> pFeed)
{
	Lock<FeedList> lock(*this);
	
	List::iterator it = std::find_if(list_.begin(), list_.end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				string_equal<WCHAR>(),
				std::mem_fun(&Feed::getURL),
				std::identity<const WCHAR*>()),
			pFeed->getURL()));
	if (it != list_.end()) {
		delete *it;
		*it = pFeed.release();
	}
	else {
		list_.push_back(pFeed.get());
		pFeed.release();
	}
	
	bModified_ = true;
}

void qmrss::FeedList::removeFeed(const Feed* pFeed)
{
	assert(isLocked());
	
	List::iterator it = std::find(list_.begin(), list_.end(), pFeed);
	if (it != list_.end()) {
		delete *it;
		list_.erase(it);
		bModified_ = true;
	}
}

bool qmrss::FeedList::save()
{
	Lock<FeedList> lock(*this);
	
	if (!bModified_)
		return true;
	
	TemporaryFileRenamer renamer(wstrPath_.get());
	
	FileOutputStream stream(renamer.getPath());
	if (!stream)
		return false;
	BufferedOutputStream bufferedStream(&stream, false);
	OutputStreamWriter writer(&bufferedStream, false, L"utf-8");
	if (!writer)
		return false;
	
	FeedWriter w(&writer, L"utf-8");
	if (!w.write(*this))
		return false;
	
	if (!writer.close())
		return false;
	
	if (!renamer.rename())
		return false;
	
	bModified_ = false;
	
	return true;
}

void qmrss::FeedList::lock() const
{
	cs_.lock();
#ifndef NDEBUG
	++nLock_;
#endif
}

void qmrss::FeedList::unlock() const
{
#ifndef NDEBUG
	--nLock_;
#endif
	cs_.unlock();
}

#ifndef NDEBUG
bool qmrss::FeedList::isLocked() const
{
	return nLock_ != 0;
}
#endif

bool qmrss::FeedList::load()
{
	W2T(wstrPath_.get(), ptszPath);
	if (::GetFileAttributes(ptszPath) != 0xffffffff) {
		XMLReader reader;
		FeedContentHandler handler(this);
		reader.setContentHandler(&handler);
		if (!reader.parse(wstrPath_.get()))
			return false;
	}
	
	bModified_ = false;
	
	return true;
}


/****************************************************************************
 *
 * Feed
 *
 */

qmrss::Feed::Feed(const WCHAR* pwszURL,
				  const Time& timeLastModified) :
	timeLastModified_(timeLastModified)
{
	wstrURL_ = allocWString(pwszURL);
}

qmrss::Feed::~Feed()
{
	std::for_each(listItem_.begin(), listItem_.end(), qs::deleter<FeedItem>());
}

const WCHAR* qmrss::Feed::getURL() const
{
	return wstrURL_.get();
}

const Time& qmrss::Feed::getLastModified() const
{
	return timeLastModified_;
}

const Feed::ItemList& qmrss::Feed::getItems() const
{
	return listItem_;
}

const FeedItem* qmrss::Feed::getItem(const WCHAR* pwszKey) const
{
	ItemList::const_iterator it = std::find_if(
		listItem_.begin(), listItem_.end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				string_equal<WCHAR>(),
				std::mem_fun(&FeedItem::getKey),
				std::identity<const WCHAR*>()),
			pwszKey));
	return it != listItem_.end() ? *it : 0;
}

void qmrss::Feed::addItem(std::auto_ptr<FeedItem> pItem)
{
	listItem_.push_back(pItem.get());
	pItem.release();
}


/****************************************************************************
 *
 * FeedItem
 *
 */

qmrss::FeedItem::FeedItem(const WCHAR* pwszKey)
{
	wstrKey_ = allocWString(pwszKey);
}

qmrss::FeedItem::~FeedItem()
{
}

const WCHAR* qmrss::FeedItem::getKey() const
{
	return wstrKey_.get();
}


/****************************************************************************
 *
 * FeedManager
 *
 */

qmrss::FeedManager::FeedManager()
{
}

qmrss::FeedManager::~FeedManager()
{
	std::for_each(map_.begin(), map_.end(),
		unary_compose_f_gx(
			qs::deleter<FeedList>(),
			std::select2nd<Map::value_type>()));
}

FeedList* qmrss::FeedManager::get(qm::Account* pAccount)
{
	Lock<CriticalSection> lock(cs_);
	
	Map::iterator it = std::find_if(map_.begin(), map_.end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				std::equal_to<Account*>(),
				std::select1st<Map::value_type>(),
				std::identity<Account*>()),
			pAccount));
	if (it != map_.end())
		return (*it).second;
	
	wstring_ptr wstrPath(concat(pAccount->getPath(), L"\\feed.xml"));
	std::auto_ptr<FeedList> pFeedList(new FeedList(wstrPath.get()));
	map_.push_back(std::make_pair(pAccount, pFeedList.get()));
	return pFeedList.release();
}


/****************************************************************************
 *
 * FeedContentHandler
 *
 */

qmrss::FeedContentHandler::FeedContentHandler(FeedList* pList) :
	pList_(pList),
	state_(STATE_ROOT),
	pCurrentFeed_(0)
{
}

qmrss::FeedContentHandler::~FeedContentHandler()
{
}

bool qmrss::FeedContentHandler::startElement(const WCHAR* pwszNamespaceURI,
											 const WCHAR* pwszLocalName,
											 const WCHAR* pwszQName,
											 const Attributes& attributes)
{
	if (wcscmp(pwszLocalName, L"feedList") == 0) {
		if (state_ != STATE_ROOT)
			return false;
		state_ = STATE_FEEDLIST;
	}
	else if (wcscmp(pwszLocalName, L"feed") == 0) {
		if (state_ != STATE_FEEDLIST)
			return false;
		
		const WCHAR* pwszURL = 0;
		const WCHAR* pwszLastModified = 0;
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrName, L"url") == 0)
				pwszURL = attributes.getValue(n);
			else if (wcscmp(pwszAttrName, L"lastModified") == 0)
				pwszLastModified = attributes.getValue(n);
			else
				return false;
		}
		if (!pwszURL || !pwszLastModified)
			return false;
		
		string_ptr strLastModified(wcs2mbs(pwszLastModified));
		Time timeLastModified;
		if (!DateParser::parse(strLastModified.get(),
			-1, DateParser::FLAG_NONE, &timeLastModified))
			return false;
		
		assert(!pCurrentFeed_);
		std::auto_ptr<Feed> pFeed(new Feed(pwszURL, timeLastModified));
		pCurrentFeed_ = pFeed.get();
		pList_->setFeed(pFeed);
		
		state_ = STATE_FEED;
	}
	else if (wcscmp(pwszLocalName, L"item") == 0) {
		if (state_ != STATE_FEED)
			return false;
		
		state_ = STATE_ITEM;
	}
	else {
		return false;
	}
	
	return true;
}

bool qmrss::FeedContentHandler::endElement(const WCHAR* pwszNamespaceURI,
										   const WCHAR* pwszLocalName,
										   const WCHAR* pwszQName)
{
	if (wcscmp(pwszLocalName, L"feedList") == 0) {
		assert(state_ == STATE_FEEDLIST);
		state_ = STATE_ROOT;
	}
	else if (wcscmp(pwszLocalName, L"feed") == 0) {
		assert(state_ == STATE_FEED);
		assert(pCurrentFeed_);
		pCurrentFeed_ = 0;
		state_ = STATE_FEEDLIST;
	}
	else if (wcscmp(pwszLocalName, L"item") == 0) {
		assert(state_ == STATE_ITEM);
		
		if (buffer_.getLength() == 0)
			return false;
		
		std::auto_ptr<FeedItem> pItem(new FeedItem(buffer_.getCharArray()));
		pCurrentFeed_->addItem(pItem);
		
		buffer_.remove();
		
		state_ = STATE_FEED;
	}
	else {
		return false;
	}
	
	return true;
}

bool qmrss::FeedContentHandler::characters(const WCHAR* pwsz,
										   size_t nStart,
										   size_t nLength)
{
	if (state_ == STATE_ITEM) {
		buffer_.append(pwsz + nStart, nLength);
	}
	else {
		const WCHAR* p = pwsz + nStart;
		for (size_t n = 0; n < nLength; ++n, ++p) {
			if (*p != L' ' && *p != L'\t' && *p != '\n')
				return false;
		}
	}
	
	return true;
}


/****************************************************************************
 *
 * FeedWriter
 *
 */

qmrss::FeedWriter::FeedWriter(Writer* pWriter,
							  const WCHAR* pwszEncoding) :
	handler_(pWriter, pwszEncoding)
{
}

qmrss::FeedWriter::~FeedWriter()
{
}

bool qmrss::FeedWriter::write(const FeedList& l)
{
	if (!handler_.startDocument() ||
		!handler_.startElement(0, 0, L"feedList", DefaultAttributes()))
		return false;
	
	const FeedList::List& list = l.getFeeds();
	for (FeedList::List::const_iterator itF = list.begin(); itF != list.end(); ++itF) {
		const Feed* pFeed = *itF;
		
		wstring_ptr wstrLastModified(DateParser::unparse(pFeed->getLastModified()));
		SimpleAttributes::Item items[] = {
			{ L"url",			pFeed->getURL()			},
			{ L"lastModified",	wstrLastModified.get()	}
		};
		SimpleAttributes attrs(items, countof(items));
		if (!handler_.startElement(0, 0, L"feed", attrs))
			return false;
		
		const Feed::ItemList& listItem = pFeed->getItems();
		for (Feed::ItemList::const_iterator itI = listItem.begin(); itI != listItem.end(); ++itI) {
			const FeedItem* pItem = *itI;
			
			if (!handler_.startElement(0, 0, L"item", DefaultAttributes()) ||
				!handler_.characters(pItem->getKey(), 0, wcslen(pItem->getKey())) ||
				!handler_.endElement(0, 0, L"item"))
				return false;
		}
		
		if (!handler_.endElement(0, 0, L"feed"))
			return false;
	}
	
	if (!handler_.endElement(0, 0, L"feedList") ||
		!handler_.endDocument())
		return false;
	
	return true;
}
