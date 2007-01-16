/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
 * All rights reserved.
 *
 */

#pragma warning(disable:4786)

#include <qsconv.h>
#include <qsfile.h>
#include <qsinit.h>
#include <qslog.h>
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
		Log log(InitThread::getInitThread().getLogger(), L"qmrss::FeedList");
		log.errorf(L"Failed to load feed list: %s", pwszPath);
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
	
	Feed feed(pwszURL, Time::getCurrentTime());
	List::const_iterator it = std::lower_bound(
		list_.begin(), list_.end(), &feed,
		binary_compose_f_gx_hy(
			string_less<WCHAR>(),
			std::mem_fun(&Feed::getURL),
			std::mem_fun(&Feed::getURL)));
	return it != list_.end() && wcscmp((*it)->getURL(), pwszURL) == 0 ? *it : 0;
}

void qmrss::FeedList::setFeed(std::auto_ptr<Feed> pFeed,
							  int nKeepDay)
{
	Lock<FeedList> lock(*this);
	
	List::iterator it = std::lower_bound(
		list_.begin(), list_.end(), pFeed.get(),
		binary_compose_f_gx_hy(
			string_less<WCHAR>(),
			std::mem_fun(&Feed::getURL),
			std::mem_fun(&Feed::getURL)));
	if (it != list_.end() && wcscmp((*it)->getURL(), pFeed->getURL()) == 0) {
		if (nKeepDay != -1)
			pFeed->merge(*it, Time::getCurrentTime().addDay(-nKeepDay));
		delete *it;
		*it = pFeed.release();
	}
	else {
		list_.insert(it, pFeed.get());
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

void qmrss::FeedList::setFeeds(List& l)
{
	assert(list_.empty());
	
	sortFeeds(l);
	list_.swap(l);
}

bool qmrss::FeedList::load()
{
	if (File::isFileExisting(wstrPath_.get())) {
		XMLReader reader;
		FeedContentHandler handler(this);
		reader.setContentHandler(&handler);
		if (!reader.parse(wstrPath_.get()))
			return false;
	}
	
	bModified_ = false;
	
	return true;
}

void qmrss::FeedList::sortFeeds(List& l)
{
	std::sort(l.begin(), l.end(),
		binary_compose_f_gx_hy(
			string_less<WCHAR>(),
			std::mem_fun(&Feed::getURL),
			std::mem_fun(&Feed::getURL)));
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
	assert(pwszURL);
	
	wstrURL_ = allocWString(pwszURL);
}

qmrss::Feed::Feed(const Feed* pFeed,
				  const Time& time) :
	timeLastModified_(pFeed->getLastModified())
{
	assert(pFeed);
	
	wstrURL_ = allocWString(pFeed->getURL());
	
	FeedItem::Date date(FeedItem::convertTimeToDate(time));
	
	const ItemList& l = pFeed->listItem_;
	listItem_.reserve(l.size());
	for (ItemList::const_iterator it = l.begin(); it != l.end(); ++it) {
		const FeedItem* pItem = *it;
		listItem_.push_back(new FeedItem(pItem->getKey(), date));
	}
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
	FeedItem::Date date = { 0, 0, 0 };
	FeedItem item(pwszKey, date);
	ItemList::const_iterator it = std::lower_bound(
		listItem_.begin(), listItem_.end(), &item,
		binary_compose_f_gx_hy(
			string_less<WCHAR>(),
			std::mem_fun(&FeedItem::getKey),
			std::mem_fun(&FeedItem::getKey)));
	return it != listItem_.end() && wcscmp((*it)->getKey(), pwszKey) == 0 ? *it : 0;
}

void qmrss::Feed::addItem(std::auto_ptr<FeedItem> pItem)
{
	ItemList::iterator it = std::lower_bound(
		listItem_.begin(), listItem_.end(), pItem.get(),
		binary_compose_f_gx_hy(
			string_less<WCHAR>(),
			std::mem_fun(&FeedItem::getKey),
			std::mem_fun(&FeedItem::getKey)));
	if (it == listItem_.end() || wcscmp((*it)->getKey(), pItem->getKey()) != 0) {
		listItem_.insert(it, pItem.get());
		pItem.release();
	}
}

void qmrss::Feed::merge(Feed* pFeed,
						const Time& timeAfter)
{
	ItemList& l = pFeed->listItem_;
	bool bAdded = false;
	for (ItemList::iterator it = l.begin(); it != l.end(); ) {
		FeedItem* pItem = *it;
		
		if (!getItem(pItem->getKey())) {
			const FeedItem::Date& date = pItem->getDate();
			Time t(date.nYear_, date.nMonth_, 0, date.nDay_, 0, 0, 0, 0, 0);
			if (t > timeAfter) {
				listItem_.push_back(pItem);
				it = l.erase(it);
				bAdded = true;
			}
			else {
				++it;
			}
		}
		else {
			++it;
		}
	}
	
	if (bAdded)
		sortItems(listItem_);
}

void qmrss::Feed::setItems(ItemList& listItem)
{
	assert(listItem_.empty());
	
	sortItems(listItem);
	listItem_.swap(listItem);
}

void qmrss::Feed::sortItems(ItemList& listItem)
{
	std::sort(listItem.begin(), listItem.end(),
		binary_compose_f_gx_hy(
			string_less<WCHAR>(),
			std::mem_fun(&FeedItem::getKey),
			std::mem_fun(&FeedItem::getKey)));
}


/****************************************************************************
 *
 * FeedItem
 *
 */

qmrss::FeedItem::FeedItem(const WCHAR* pwszKey,
						  const Date& date) :
	date_(date)
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

const FeedItem::Date& qmrss::FeedItem::getDate() const
{
	return date_;
}

FeedItem::Date qmrss::FeedItem::convertTimeToDate(const qs::Time& time)
{
	Date date = {
		time.wYear,
		time.wMonth,
		time.wDay
	};
	return date;
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
	std::for_each(listItem_.begin(), listItem_.end(), qs::deleter<FeedItem>());
	std::for_each(list_.begin(), list_.end(), qs::deleter<Feed>());
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
		list_.push_back(pFeed.get());
		pFeed.release();
		
		state_ = STATE_FEED;
	}
	else if (wcscmp(pwszLocalName, L"item") == 0) {
		if (state_ != STATE_FEED)
			return false;
		
		const WCHAR* pwszDate = 0;
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrName, L"date") == 0)
				pwszDate = attributes.getValue(n);
			else
				return false;
		}
		
		// TODO
		// Allow that time is not specified for compatibility.
		// Make it error in the future.
		if (pwszDate) {
			int nYear = 0;
			int nMonth = 0;
			int nDay = 0;
			if (swscanf(pwszDate, L"%04d-%02d-%02d", &nYear, &nMonth, &nDay) != 3)
				return false;
			itemDate_.nYear_ = nYear;
			itemDate_.nMonth_ = nMonth;
			itemDate_.nDay_ = nDay;
		}
		else {
			itemDate_ = FeedItem::convertTimeToDate(Time::getCurrentTime());
		}
		
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
		
		pList_->setFeeds(list_);
		
		state_ = STATE_ROOT;
	}
	else if (wcscmp(pwszLocalName, L"feed") == 0) {
		assert(state_ == STATE_FEED);
		assert(pCurrentFeed_);
		
		pCurrentFeed_->setItems(listItem_);
		
		pCurrentFeed_ = 0;
		state_ = STATE_FEEDLIST;
	}
	else if (wcscmp(pwszLocalName, L"item") == 0) {
		assert(state_ == STATE_ITEM);
		
		if (buffer_.getLength() == 0)
			return false;
		
		std::auto_ptr<FeedItem> pItem(new FeedItem(
			buffer_.getCharArray(), itemDate_));
		listItem_.push_back(pItem.get());
		pItem.release();
		
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
			
			const FeedItem::Date& date = pItem->getDate();
			WCHAR wszDate[32];
			_snwprintf(wszDate, countof(wszDate), L"%04d-%02d-%02d",
				date.nYear_, date.nMonth_, date.nDay_);
			
			if (!handler_.startElement(0, 0, L"item", SimpleAttributes(L"date", wszDate)) ||
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
