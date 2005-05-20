/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsconv.h>
#include <qsmd5.h>
#include <qsmime.h>

#include "rss.h"

using namespace qmrss;
using namespace qs;


/****************************************************************************
 *
 * Channel
 *
 */

qmrss::Channel::Channel(const WCHAR* pwszURL)
{
	wstrURL_ = allocWString(pwszURL);
}

qmrss::Channel::~Channel()
{
	std::for_each(listItem_.begin(), listItem_.end(), qs::deleter<Item>());
}

const WCHAR* qmrss::Channel::getURL() const
{
	return wstrURL_.get();
}

const WCHAR* qmrss::Channel::getTitle() const
{
	return wstrTitle_.get();
}

const WCHAR* qmrss::Channel::getLink() const
{
	return wstrLink_.get();
}

const Time& qmrss::Channel::getPubDate() const
{
	return timePubDate_;
}

const Channel::ItemList& qmrss::Channel::getItems() const
{
	return listItem_;
}

void qmrss::Channel::setTitle(wstring_ptr wstrTitle)
{
	wstrTitle_ = wstrTitle;
}

void qmrss::Channel::setLink(wstring_ptr wstrLink)
{
	wstrLink_ = wstrLink;
}

void qmrss::Channel::setPubDate(const qs::Time& time)
{
	timePubDate_ = time;
}

void qmrss::Channel::addItem(std::auto_ptr<Item> pItem)
{
	listItem_.push_back(pItem.get());
	pItem.release();
}


/****************************************************************************
 *
 * Item
 *
 */

qmrss::Item::Item()
{
}

qmrss::Item::~Item()
{
}

const WCHAR* qmrss::Item::getTitle() const
{
	return wstrTitle_.get();
}

const WCHAR* qmrss::Item::getLink() const
{
	return wstrLink_.get();
}

const WCHAR* qmrss::Item::getDescription() const
{
	return wstrDescription_.get();
}

const WCHAR* qmrss::Item::getCategory() const
{
	return wstrCategory_.get();
}

const WCHAR* qmrss::Item::getSubject() const
{
	return wstrSubject_.get();
}

const WCHAR* qmrss::Item::getCreator() const
{
	return wstrCreator_.get();
}

const Time& qmrss::Item::getPubDate() const
{
	return timePubDate_;
}

const WCHAR* qmrss::Item::getContentEncoded() const
{
	return wstrContentEncoded_.get();
}

const WCHAR* qmrss::Item::getId() const
{
	return wstrId_.get();
}

wstring_ptr qmrss::Item::getHash() const
{
	StringBuffer<WSTRING> buf;
	const WCHAR* pwsz[] = {
		wstrTitle_.get(),
		wstrLink_.get(),
		wstrDescription_.get(),
		wstrCategory_.get(),
		wstrSubject_.get(),
		wstrCreator_.get(),
		wstrContentEncoded_.get(),
		wstrId_.get()
	};
	for (int n = 0; n < countof(pwsz); ++n) {
		const WCHAR* p = pwsz[n];
		if (p)
			buf.append(p);
	}
	
	wstring_ptr wstrTime(timePubDate_.format(L"%Y4%M0%D%h%m%s%z", Time::FORMAT_ORIGINAL));
	buf.append(wstrTime.get());
	
	size_t nLen = buf.getLength();
	xstring_size_ptr str(UTF8Converter().encode(buf.getCharArray(), &nLen));
	
	CHAR szMD5[128/8*2 + 1];
	MD5::md5ToString(reinterpret_cast<unsigned char*>(str.get()), str.size(), szMD5);
	
	return mbs2wcs(szMD5, 128/8*2);
}

void qmrss::Item::setTitle(wstring_ptr wstrTitle)
{
	wstrTitle_ = wstrTitle;
}

void qmrss::Item::setLink(wstring_ptr wstrLink)
{
	wstrLink_ = wstrLink;
}

void qmrss::Item::setDescription(wstring_ptr wstrDescription)
{
	wstrDescription_ = wstrDescription;
}

void qmrss::Item::addCategory(wstring_ptr wstrCategory)
{
	if (wstrCategory_.get())
		wstrCategory_ = concat(wstrCategory_.get(), L", ", wstrCategory.get());
	else
		wstrCategory_ = wstrCategory;
}

void qmrss::Item::addSubject(wstring_ptr wstrSubject)
{
	if (wstrSubject_.get())
		wstrSubject_ = concat(wstrSubject_.get(), L", ", wstrSubject.get());
	else
		wstrSubject_ = wstrSubject;
}

void qmrss::Item::addCreator(wstring_ptr wstrCreator)
{
	if (wstrCreator_.get())
		wstrCreator_ = concat(wstrCreator_.get(), L", ", wstrCreator.get());
	else
		wstrCreator_ = wstrCreator;
}

void qmrss::Item::setPubDate(const Time& time)
{
	timePubDate_ = time;
}

void qmrss::Item::setContentEncoded(qs::wstring_ptr wstrContentEncoded)
{
	wstrContentEncoded_ = wstrContentEncoded;
}

void qmrss::Item::setId(qs::wstring_ptr wstrId)
{
	wstrId_ = wstrId;
}


/****************************************************************************
 *
 * RssParser
 *
 */

qmrss::RssParser::RssParser()
{
}

qmrss::RssParser::~RssParser()
{
}

std::auto_ptr<Channel> qmrss::RssParser::parse(const WCHAR* pwszURL,
											   InputStream* pInputStream)
{
	std::auto_ptr<Channel> pChannel(new Channel(pwszURL));
	
	XMLReader reader;
	RssContentHandler handler(pChannel.get());
	reader.setContentHandler(&handler);
	InputSource source(pInputStream);
	if (!reader.parse(&source))
		return std::auto_ptr<Channel>();
	
	return pChannel;
}


/****************************************************************************
 *
 * RssContentHandler
 *
 */

qmrss::RssContentHandler::RssContentHandler(Channel* pChannel) :
	pChannel_(pChannel)
{
}

qmrss::RssContentHandler::~RssContentHandler()
{
}

bool qmrss::RssContentHandler::startElement(const WCHAR* pwszNamespaceURI,
											const WCHAR* pwszLocalName,
											const WCHAR* pwszQName,
											const Attributes& attributes)
{
	if (!pHandler_.get()) {
		if (pwszNamespaceURI) {
			if (wcscmp(pwszNamespaceURI, L"http://www.w3.org/1999/02/22-rdf-syntax-ns#") == 0 &&
				wcscmp(pwszLocalName, L"RDF") == 0)
				pHandler_.reset(new Rss10Handler(pChannel_));
			else if (wcscmp(pwszNamespaceURI, L"http://purl.org/atom/ns#") == 0 &&
				wcscmp(pwszLocalName, L"feed") == 0)
				pHandler_.reset(new AtomHandler(pChannel_));
			else
				return false;
		}
		else {
			if (wcscmp(pwszLocalName, L"rss") != 0)
				return false;
			
			const WCHAR* pwszVersion = attributes.getValue(0, L"version");
			if (!pwszVersion)
				return false;
			else if (wcscmp(pwszVersion, L"0.91") == 0 ||
				wcscmp(pwszVersion, L"2.0") == 0)
				pHandler_.reset(new Rss20Handler(pChannel_));
			else
				return false;
		}
	}
	
	return pHandler_->startElement(pwszNamespaceURI, pwszLocalName, pwszQName, attributes);
}

bool qmrss::RssContentHandler::endElement(const WCHAR* pwszNamespaceURI,
										  const WCHAR* pwszLocalName,
										  const WCHAR* pwszQName)
{
	return pHandler_->endElement(pwszNamespaceURI, pwszLocalName, pwszQName);
}

bool qmrss::RssContentHandler::characters(const WCHAR* pwsz,
										  size_t nStart,
										  size_t nLength)
{
	return pHandler_->characters(pwsz, nStart, nLength);
}


/****************************************************************************
 *
 * RssHandler
 *
 */

qmrss::RssHandler::~RssHandler()
{
}


/****************************************************************************
 *
 * Rss10Handler
 *
 */

qmrss::Rss10Handler::Rss10Handler(Channel* pChannel) :
	pChannel_(pChannel),
	pCurrentItem_(0)
{
	stackState_.push_back(STATE_ROOT);
}

qmrss::Rss10Handler::~Rss10Handler()
{
}

bool qmrss::Rss10Handler::startElement(const WCHAR* pwszNamespaceURI,
									   const WCHAR* pwszLocalName,
									   const WCHAR* pwszQName,
									   const Attributes& attributes)
{
	assert(!stackState_.empty());
	
	State state = stackState_.back();
	switch (state) {
	case STATE_ROOT:
		if (!pwszNamespaceURI ||
			wcscmp(pwszNamespaceURI, L"http://www.w3.org/1999/02/22-rdf-syntax-ns#") != 0 ||
			wcscmp(pwszLocalName, L"RDF") != 0)
			return false;
		stackState_.push_back(STATE_RDF);
		break;
	case STATE_RDF:
		if (pwszNamespaceURI && wcscmp(pwszNamespaceURI, L"http://purl.org/rss/1.0/") == 0) {
			if (wcscmp(pwszLocalName, L"channel") == 0) {
				stackState_.push_back(STATE_CHANNEL);
			}
			else if (wcscmp(pwszLocalName, L"item") == 0) {
				std::auto_ptr<Item> pItem(new Item());
				pCurrentItem_ = pItem.get();
				pChannel_->addItem(pItem);
				stackState_.push_back(STATE_ITEM);
			}
			else {
				stackState_.push_back(STATE_UNKNOWN);
			}
		}
		else {
			stackState_.push_back(STATE_UNKNOWN);
		}
		break;
	case STATE_CHANNEL:
		if (pwszNamespaceURI && wcscmp(pwszNamespaceURI, L"http://purl.org/rss/1.0/") == 0) {
			if (wcscmp(pwszLocalName, L"title") == 0)
				stackState_.push_back(STATE_TITLE);
			else if (wcscmp(pwszLocalName, L"link") == 0)
				stackState_.push_back(STATE_LINK);
			else
				stackState_.push_back(STATE_UNKNOWN);
		}
		else if (pwszNamespaceURI && wcscmp(pwszNamespaceURI, L"http://purl.org/dc/elements/1.1/") == 0) {
			if (wcscmp(pwszLocalName, L"date") == 0)
				stackState_.push_back(STATE_DATE);
			else
				stackState_.push_back(STATE_UNKNOWN);
		}
		else {
			stackState_.push_back(STATE_UNKNOWN);
		}
		break;
	case STATE_TITLE:
	case STATE_LINK:
	case STATE_DATE:
		stackState_.push_back(STATE_UNKNOWN);
		break;
	case STATE_ITEM:
		if (pwszNamespaceURI && wcscmp(pwszNamespaceURI, L"http://purl.org/rss/1.0/") == 0) {
			if (wcscmp(pwszLocalName, L"title") == 0 ||
				wcscmp(pwszLocalName, L"link") == 0 ||
				wcscmp(pwszLocalName, L"description") == 0 ||
				wcscmp(pwszLocalName, L"category") == 0)
				stackState_.push_back(STATE_PROPERTY);
			else
				stackState_.push_back(STATE_UNKNOWN);
		}
		else if (pwszNamespaceURI && wcscmp(pwszNamespaceURI, L"http://purl.org/dc/elements/1.1/") == 0) {
			if (wcscmp(pwszLocalName, L"subject") == 0 ||
				wcscmp(pwszLocalName, L"creator") == 0 ||
				wcscmp(pwszLocalName, L"date") == 0)
				stackState_.push_back(STATE_PROPERTY);
			else
				stackState_.push_back(STATE_UNKNOWN);
		}
		else if (pwszNamespaceURI && wcscmp(pwszNamespaceURI, L"http://purl.org/rss/1.0/modules/content/") == 0) {
			if (wcscmp(pwszLocalName, L"encoded") == 0)
				stackState_.push_back(STATE_PROPERTY);
			else
				stackState_.push_back(STATE_UNKNOWN);
		}
		else {
			stackState_.push_back(STATE_UNKNOWN);
		}
		break;
	case STATE_PROPERTY:
		stackState_.push_back(STATE_UNKNOWN);
		break;
	case STATE_UNKNOWN:
		stackState_.push_back(STATE_UNKNOWN);
		break;
	default:
		assert(false);
		return false;
	}
	return true;
}

bool qmrss::Rss10Handler::endElement(const WCHAR* pwszNamespaceURI,
									 const WCHAR* pwszLocalName,
									 const WCHAR* pwszQName)
{
	assert(!stackState_.empty());
	
	State state = stackState_.back();
	switch (state) {
	case STATE_ROOT:
		assert(false);
		return false;
	case STATE_RDF:
		assert(wcscmp(pwszNamespaceURI, L"http://www.w3.org/1999/02/22-rdf-syntax-ns#") == 0 &&
			wcscmp(pwszLocalName, L"RDF") == 0);
		break;
	case STATE_CHANNEL:
		assert(wcscmp(pwszNamespaceURI, L"http://purl.org/rss/1.0/") == 0 &&
			wcscmp(pwszLocalName, L"channel") == 0);
		break;
	case STATE_TITLE:
		pChannel_->setTitle(buffer_.getString());
		break;
	case STATE_LINK:
		pChannel_->setLink(buffer_.getString());
		break;
	case STATE_DATE:
		{
			Time date;
			if (ParserUtil::parseDate(buffer_.getCharArray(), &date))
				pChannel_->setPubDate(date);
			buffer_.remove();
		}
		break;
	case STATE_ITEM:
		assert(wcscmp(pwszNamespaceURI, L"http://purl.org/rss/1.0/") == 0 &&
			wcscmp(pwszLocalName, L"item") == 0);
		assert(pCurrentItem_);
		pCurrentItem_ = 0;
		break;
	case STATE_PROPERTY:
		assert(pCurrentItem_);
		assert(pwszNamespaceURI);
		if (wcscmp(pwszNamespaceURI, L"http://purl.org/rss/1.0/") == 0) {
			if (wcscmp(pwszLocalName, L"title") == 0) {
				pCurrentItem_->setTitle(buffer_.getString());
			}
			else if (wcscmp(pwszLocalName, L"link") == 0) {
				pCurrentItem_->setLink(buffer_.getString());
			}
			else if (wcscmp(pwszLocalName, L"description") == 0) {
				pCurrentItem_->setDescription(buffer_.getString());
			}
			else if (wcscmp(pwszLocalName, L"category") == 0) {
				pCurrentItem_->addCategory(buffer_.getString());
			}
			else {
				assert(false);
			}
		}
		else if (wcscmp(pwszNamespaceURI, L"http://purl.org/dc/elements/1.1/") == 0) {
			if (wcscmp(pwszLocalName, L"subject") == 0) {
				pCurrentItem_->addSubject(buffer_.getString());
			}
			else if (wcscmp(pwszLocalName, L"creator") == 0) {
				pCurrentItem_->addCreator(buffer_.getString());
			}
			else if (wcscmp(pwszLocalName, L"date") == 0) {
				Time date;
				if (ParserUtil::parseDate(buffer_.getCharArray(), &date))
					pCurrentItem_->setPubDate(date);
				buffer_.remove();
			}
			else {
				assert(false);
			}
		}
		else if (wcscmp(pwszNamespaceURI, L"http://purl.org/rss/1.0/modules/content/") == 0) {
			if (wcscmp(pwszLocalName, L"encoded") == 0)
				pCurrentItem_->setContentEncoded(buffer_.getString());
			else
				assert(false);
		}
		else {
			assert(false);
		}
		break;
	case STATE_UNKNOWN:
		break;
	default:
		assert(false);
		return false;
	}
	
	stackState_.pop_back();
	
	return true;
}

bool qmrss::Rss10Handler::characters(const WCHAR* pwsz,
									 size_t nStart,
									 size_t nLength)
{
	assert(!stackState_.empty());
	
	State state = stackState_.back();
	if (state == STATE_TITLE ||
		state == STATE_LINK ||
		state == STATE_DATE ||
		state == STATE_PROPERTY)
		buffer_.append(pwsz + nStart, nLength);
	return true;
}


/****************************************************************************
 *
 * Rss20Handler
 *
 */

qmrss::Rss20Handler::Rss20Handler(Channel* pChannel) :
	pChannel_(pChannel),
	pCurrentItem_(0)
{
	stackState_.push_back(STATE_ROOT);
}

qmrss::Rss20Handler::~Rss20Handler()
{
}

bool qmrss::Rss20Handler::startElement(const WCHAR* pwszNamespaceURI,
									   const WCHAR* pwszLocalName,
									   const WCHAR* pwszQName,
									   const Attributes& attributes)
{
	assert(!stackState_.empty());
	
	State state = stackState_.back();
	switch (state) {
	case STATE_ROOT:
		if (pwszNamespaceURI || wcscmp(pwszLocalName, L"rss") != 0)
			return false;
		stackState_.push_back(STATE_RSS);
		break;
	case STATE_RSS:
		if (!pwszNamespaceURI && wcscmp(pwszLocalName, L"channel") == 0)
			stackState_.push_back(STATE_CHANNEL);
		else
			stackState_.push_back(STATE_UNKNOWN);
		break;
	case STATE_CHANNEL:
		if (!pwszNamespaceURI) {
			if (wcscmp(pwszLocalName, L"item") == 0) {
				std::auto_ptr<Item> pItem(new Item());
				pCurrentItem_ = pItem.get();
				pChannel_->addItem(pItem);
				stackState_.push_back(STATE_ITEM);
			}
			else if (wcscmp(pwszLocalName, L"title") == 0) {
				stackState_.push_back(STATE_TITLE);
			}
			else if (wcscmp(pwszLocalName, L"link") == 0) {
				stackState_.push_back(STATE_LINK);
			}
			else if (wcscmp(pwszLocalName, L"pubDate") == 0) {
				stackState_.push_back(STATE_PUBDATE);
			}
			else {
				stackState_.push_back(STATE_UNKNOWN);
			}
		}
		else {
			stackState_.push_back(STATE_UNKNOWN);
		}
		break;
	case STATE_TITLE:
	case STATE_LINK:
	case STATE_PUBDATE:
		stackState_.push_back(STATE_UNKNOWN);
		break;
	case STATE_ITEM:
		if (!pwszNamespaceURI) {
			if (wcscmp(pwszLocalName, L"title") == 0 ||
				wcscmp(pwszLocalName, L"link") == 0 ||
				wcscmp(pwszLocalName, L"description") == 0 ||
				wcscmp(pwszLocalName, L"category") == 0 ||
				wcscmp(pwszLocalName, L"pubDate") == 0)
				stackState_.push_back(STATE_PROPERTY);
			else
				stackState_.push_back(STATE_UNKNOWN);
		}
		else if (wcscmp(pwszNamespaceURI, L"http://purl.org/dc/elements/1.1/") == 0) {
			if (wcscmp(pwszLocalName, L"subject") == 0 ||
				wcscmp(pwszLocalName, L"creator") == 0 ||
				wcscmp(pwszLocalName, L"date") == 0)
				stackState_.push_back(STATE_PROPERTY);
			else
				stackState_.push_back(STATE_UNKNOWN);
		}
		else if (wcscmp(pwszNamespaceURI, L"http://purl.org/rss/1.0/modules/content/") == 0) {
			if (wcscmp(pwszLocalName, L"encoded") == 0)
				stackState_.push_back(STATE_PROPERTY);
			else
				stackState_.push_back(STATE_UNKNOWN);
		}
		else {
			stackState_.push_back(STATE_UNKNOWN);
		}
		break;
	case STATE_PROPERTY:
		stackState_.push_back(STATE_UNKNOWN);
		break;
	case STATE_UNKNOWN:
		stackState_.push_back(STATE_UNKNOWN);
		break;
	default:
		assert(false);
		return false;
	}
	return true;
}

bool qmrss::Rss20Handler::endElement(const WCHAR* pwszNamespaceURI,
									 const WCHAR* pwszLocalName,
									 const WCHAR* pwszQName)
{
	assert(!stackState_.empty());
	
	State state = stackState_.back();
	switch (state) {
	case STATE_ROOT:
		assert(false);
		return false;
	case STATE_RSS:
		assert(!pwszNamespaceURI && wcscmp(pwszLocalName, L"rss") == 0);
		break;
	case STATE_CHANNEL:
		assert(!pwszNamespaceURI && wcscmp(pwszLocalName, L"channel") == 0);
		break;
	case STATE_TITLE:
		pChannel_->setTitle(buffer_.getString());
		break;
	case STATE_LINK:
		pChannel_->setLink(buffer_.getString());
		break;
	case STATE_PUBDATE:
		{
			Time date;
			string_ptr strDate(wcs2mbs(buffer_.getCharArray()));
			if (DateParser::parse(strDate.get(), -1, true, true, &date))
				pChannel_->setPubDate(date);
			buffer_.remove();
		}
		break;
	case STATE_ITEM:
		assert(!pwszNamespaceURI && wcscmp(pwszLocalName, L"item") == 0);
		assert(pCurrentItem_);
		pCurrentItem_ = 0;
		break;
	case STATE_PROPERTY:
		assert(pCurrentItem_);
		if (!pwszNamespaceURI) {
			if (wcscmp(pwszLocalName, L"title") == 0) {
				pCurrentItem_->setTitle(buffer_.getString());
			}
			else if (wcscmp(pwszLocalName, L"link") == 0) {
				pCurrentItem_->setLink(buffer_.getString());
			}
			else if (wcscmp(pwszLocalName, L"description") == 0) {
				pCurrentItem_->setDescription(buffer_.getString());
			}
			else if (wcscmp(pwszLocalName, L"category") == 0) {
				pCurrentItem_->addCategory(buffer_.getString());
			}
			else if (wcscmp(pwszLocalName, L"pubDate") == 0) {
				Time date;
				string_ptr strDate(wcs2mbs(buffer_.getCharArray()));
				if (DateParser::parse(strDate.get(), -1, true, true, &date))
					pCurrentItem_->setPubDate(date);
				buffer_.remove();
			}
			else {
				assert(false);
			}
		}
		else if (wcscmp(pwszNamespaceURI, L"http://purl.org/dc/elements/1.1/") == 0) {
			if (wcscmp(pwszLocalName, L"subject") == 0) {
				pCurrentItem_->addSubject(buffer_.getString());
			}
			else if (wcscmp(pwszLocalName, L"creator") == 0) {
				pCurrentItem_->addCreator(buffer_.getString());
			}
			else if (wcscmp(pwszLocalName, L"date") == 0) {
				Time date;
				if (ParserUtil::parseDate(buffer_.getCharArray(), &date))
					pCurrentItem_->setPubDate(date);
				buffer_.remove();
			}
			else {
				assert(false);
			}
		}
		else if (wcscmp(pwszNamespaceURI, L"http://purl.org/rss/1.0/modules/content/") == 0) {
			if (wcscmp(pwszLocalName, L"encoded") == 0)
				pCurrentItem_->setContentEncoded(buffer_.getString());
			else
				assert(false);
		}
		else {
			assert(false);
		}
		break;
	case STATE_UNKNOWN:
		break;
	default:
		assert(false);
		return false;
	}
	
	stackState_.pop_back();
	
	return true;
}

bool qmrss::Rss20Handler::characters(const WCHAR* pwsz,
									 size_t nStart,
									 size_t nLength)
{
	assert(!stackState_.empty());
	
	State state = stackState_.back();
	if (state == STATE_TITLE ||
		state == STATE_LINK ||
		state == STATE_PUBDATE ||
		state == STATE_PROPERTY)
		buffer_.append(pwsz + nStart, nLength);
	return true;
}


/****************************************************************************
 *
 * AtomHandler
 *
 */

qmrss::AtomHandler::AtomHandler(Channel* pChannel) :
	pChannel_(pChannel),
	pCurrentItem_(0)
{
	stackState_.push_back(STATE_ROOT);
}

qmrss::AtomHandler::~AtomHandler()
{
}

bool qmrss::AtomHandler::startElement(const WCHAR* pwszNamespaceURI,
									  const WCHAR* pwszLocalName,
									  const WCHAR* pwszQName,
									  const Attributes& attributes)
{
	assert(!stackState_.empty());
	
	State state = stackState_.back();
	switch (state) {
	case STATE_ROOT:
		if (!pwszNamespaceURI ||
			wcscmp(pwszNamespaceURI, L"http://purl.org/atom/ns#") != 0 ||
			wcscmp(pwszLocalName, L"feed") != 0)
			return false;
		stackState_.push_back(STATE_FEED);
		break;
	case STATE_FEED:
		if (pwszNamespaceURI && wcscmp(pwszNamespaceURI, L"http://purl.org/atom/ns#") == 0) {
			if (wcscmp(pwszLocalName, L"entry") == 0) {
				std::auto_ptr<Item> pItem(new Item());
				pCurrentItem_ = pItem.get();
				pChannel_->addItem(pItem);
				stackState_.push_back(STATE_ENTRY);
			}
			else if (wcscmp(pwszLocalName, L"title") == 0) {
				stackState_.push_back(STATE_TITLE);
			}
			else if (wcscmp(pwszLocalName, L"link") == 0) {
				const WCHAR* pwszRel = attributes.getValue(L"rel");
				if (!pwszRel || wcscmp(pwszRel, L"alternative") == 0) {
					const WCHAR* pwszHref = attributes.getValue(L"href");
					if (pwszHref)
						pChannel_->setLink(allocWString(pwszHref));
				}
				stackState_.push_back(STATE_UNKNOWN);
			}
			else if (wcscmp(pwszLocalName, L"modified") == 0) {
				stackState_.push_back(STATE_MODIFIED);
			}
			else {
				stackState_.push_back(STATE_UNKNOWN);
			}
		}
		else {
			stackState_.push_back(STATE_UNKNOWN);
		}
		break;
	case STATE_TITLE:
	case STATE_MODIFIED:
		stackState_.push_back(STATE_UNKNOWN);
		break;
	case STATE_ENTRY:
		if (pwszNamespaceURI && wcscmp(pwszNamespaceURI, L"http://purl.org/atom/ns#") == 0) {
			if (wcscmp(pwszLocalName, L"title") == 0 ||
				wcscmp(pwszLocalName, L"modified") == 0 ||
				wcscmp(pwszLocalName, L"summary") == 0 ||
				wcscmp(pwszLocalName, L"id") == 0) {
				stackState_.push_back(STATE_PROPERTY);
			}
			else if (wcscmp(pwszLocalName, L"content") == 0) {
				const WCHAR* pwszMode = attributes.getValue(L"mode");
				if (pwszMode && wcscmp(pwszMode, L"escaped") == 0)
					stackState_.push_back(STATE_PROPERTY);
				else
					stackState_.push_back(STATE_CONTENT);
			}
			else if (wcscmp(pwszLocalName, L"link") == 0) {
				const WCHAR* pwszHref = attributes.getValue(L"href");
				if (pwszHref)
					pCurrentItem_->setLink(allocWString(pwszHref));
				stackState_.push_back(STATE_UNKNOWN);
			}
			else if (wcscmp(pwszLocalName, L"author") == 0) {
				stackState_.push_back(STATE_AUTHOR);
			}
			else {
				stackState_.push_back(STATE_UNKNOWN);
			}
		}
		else {
			stackState_.push_back(STATE_UNKNOWN);
		}
		break;
	case STATE_PROPERTY:
		stackState_.push_back(STATE_UNKNOWN);
		break;
	case STATE_AUTHOR:
		if (pwszNamespaceURI && wcscmp(pwszNamespaceURI, L"http://purl.org/atom/ns#") == 0) {
			if (wcscmp(pwszLocalName, L"name") == 0)
				stackState_.push_back(STATE_NAME);
			else if (wcscmp(pwszLocalName, L"email") == 0)
				stackState_.push_back(STATE_EMAIL);
			else
				stackState_.push_back(STATE_UNKNOWN);
		}
		else {
			stackState_.push_back(STATE_UNKNOWN);
		}
		break;
	case STATE_NAME:
		stackState_.push_back(STATE_UNKNOWN);
		break;
	case STATE_EMAIL:
		stackState_.push_back(STATE_UNKNOWN);
		break;
	case STATE_CONTENT:
	case STATE_CONTENTCHILD:
		buffer_.append(L"<");
		buffer_.append(pwszQName);
		for (int n = 0; n < attributes.getLength(); ++n) {
			buffer_.append(L" ");
			buffer_.append(attributes.getQName(n));
			buffer_.append(L"=\"");
			escape(attributes.getValue(n), -1, true, &buffer_);
			buffer_.append(L"\"");
		}
		buffer_.append(L">");
		stackState_.push_back(STATE_CONTENTCHILD);
		break;
	case STATE_UNKNOWN:
		stackState_.push_back(STATE_UNKNOWN);
		break;
	default:
		assert(false);
		break;
	}
	return true;
}

bool qmrss::AtomHandler::endElement(const WCHAR* pwszNamespaceURI,
									const WCHAR* pwszLocalName,
									const WCHAR* pwszQName)
{
	assert(!stackState_.empty());
	
	State state = stackState_.back();
	switch (state) {
	case STATE_ROOT:
		assert(false);
		return false;
	case STATE_FEED:
		assert(wcscmp(pwszNamespaceURI, L"http://purl.org/atom/ns#") == 0 &&
			wcscmp(pwszLocalName, L"feed") == 0);
		break;
	case STATE_TITLE:
		pChannel_->setTitle(buffer_.getString());
		break;
	case STATE_MODIFIED:
		{
			assert(wcscmp(pwszNamespaceURI, L"http://purl.org/atom/ns#") == 0 &&
				wcscmp(pwszLocalName, L"modified") == 0);
			Time date;
			if (ParserUtil::parseDate(buffer_.getCharArray(), &date))
				pChannel_->setPubDate(date);
			buffer_.remove();
		}
		break;
	case STATE_ENTRY:
		assert(wcscmp(pwszNamespaceURI, L"http://purl.org/atom/ns#") == 0 &&
			wcscmp(pwszLocalName, L"entry") == 0);
		assert(pCurrentItem_);
		pCurrentItem_ = 0;
		break;
	case STATE_PROPERTY:
		if (wcscmp(pwszNamespaceURI, L"http://purl.org/atom/ns#") == 0) {
			if (wcscmp(pwszLocalName, L"title") == 0) {
				pCurrentItem_->setTitle(buffer_.getString());
			}
			else if (wcscmp(pwszLocalName, L"modified") == 0) {
				Time date;
				if (ParserUtil::parseDate(buffer_.getCharArray(), &date))
					pCurrentItem_->setPubDate(date);
				buffer_.remove();
			}
			else if (wcscmp(pwszLocalName, L"summary") == 0) {
				pCurrentItem_->setDescription(buffer_.getString());
			}
			else if (wcscmp(pwszLocalName, L"content") == 0) {
				pCurrentItem_->setContentEncoded(buffer_.getString());
			}
			else if (wcscmp(pwszLocalName, L"id") == 0) {
				pCurrentItem_->setId(buffer_.getString());
			}
			else {
				assert(false);
			}
		}
		else {
			assert(false);
		}
		break;
	case STATE_AUTHOR:
		{
			StringBuffer<WSTRING> buf;
			if (wstrName_.get())
				buf.append(wstrName_.get());
			if (wstrEmail_.get()) {
				if (wstrName_.get())
					buf.append(L" <");
				buf.append(wstrEmail_.get());
				if (wstrName_.get())
					buf.append(L">");
			}
			pCurrentItem_->addCreator(buf.getString());
			wstrName_.reset(0);
			wstrEmail_.reset(0);
		}
		break;
	case STATE_NAME:
		wstrName_ = buffer_.getString();
		break;
	case STATE_EMAIL:
		wstrEmail_ = buffer_.getString();
		break;
	case STATE_CONTENT:
		assert(wcscmp(pwszNamespaceURI, L"http://purl.org/atom/ns#") == 0 &&
			wcscmp(pwszLocalName, L"content") == 0);
		pCurrentItem_->setContentEncoded(buffer_.getString());
		break;
	case STATE_CONTENTCHILD:
		buffer_.append(L"</");
		buffer_.append(pwszQName);
		buffer_.append(L">");
		break;
	case STATE_UNKNOWN:
		break;
	default:
		assert(false);
		break;
	}
	
	stackState_.pop_back();
	
	return true;
}

bool qmrss::AtomHandler::characters(const WCHAR* pwsz,
									size_t nStart,
									size_t nLength)
{
	assert(!stackState_.empty());
	
	State state = stackState_.back();
	if (state == STATE_TITLE ||
		state == STATE_MODIFIED ||
		state == STATE_PROPERTY ||
		state == STATE_NAME ||
		state == STATE_EMAIL)
		buffer_.append(pwsz + nStart, nLength);
	else if (state == STATE_CONTENT ||
		state == STATE_CONTENTCHILD)
		escape(pwsz + nStart, nLength, false, &buffer_);
	
	return true;
}

void qmrss::AtomHandler::escape(const WCHAR* pwsz,
								size_t nLen,
								bool bAttribute,
								qs::StringBuffer<qs::WSTRING>* pBuf)
{
	if (nLen == -1)
		nLen = wcslen(pwsz);
	
	for (const WCHAR* p = pwsz; p < pwsz + nLen; ++p) {
		if (*p == L'<')
			pBuf->append(L"&lt;");
		else if (*p == L'&')
			pBuf->append(L"&amp;");
		else if (*p == L'\"' && bAttribute)
			pBuf->append(L"&quot;");
		else
			pBuf->append(*p);
	}
}


/****************************************************************************
 *
 * ParserUtil
 *
 */

bool qmrss::ParserUtil::parseDate(const WCHAR* pwszDate,
								  Time* pDate)
{
	const WCHAR* p = pwszDate;
	
	int nYear = parseNumber(p, 4);
	if (nYear == -1)
		return false;
	p += 4;
	pDate->wYear = nYear;
	if (*p == L'\0')
		return true;
	else if (*p != L'-')
		return false;
	++p;
	
	int nMonth = parseNumber(p, 2);
	if (nMonth <= 0 || 12 < nMonth)
		return false;
	p += 2;
	pDate->wMonth = nMonth;
	if (*p == L'\0')
		return true;
	else if (*p != L'-')
		return false;
	++p;
	
	int nDay = parseNumber(p, 2);
	if (nDay <= 0 || 31 < nDay)
		return false;
	p += 2;
	pDate->wDay = nDay;
	if (*p == L'\0')
		return true;
	else if (*p != L'T')
		return false;
	++p;
	
	int nHour = parseNumber(p, 2);
	if (nHour < 0 || 23 < nHour)
		return false;
	p += 2;
	pDate->wHour = nHour;
	if (*p != L':')
		return false;
	++p;
	
	int nMinute = parseNumber(p, 2);
	if (nMinute < 0 || 59 < nMinute)
		return false;
	p += 2;
	pDate->wMinute = nMinute;
	if (*p == L':') {
		++p;
		
		int nSecond = parseNumber(p, 2);
		if (nSecond < 0 || 60 < nSecond)
			return false;
		p += 2;
		pDate->wSecond = nSecond;
		if (*p == L'.') {
			++p;
			while (L'0' <= *p && *p <= L'9')
				++p;
		}
	}
	
	int nTimeZone = 0;
	if (*p == L'Z') {
	}
	else if (*p == L'+' || *p == L'-') {
		bool bMinus = *p == L'-';
		++p;
		int nTimeZoneHour = parseNumber(p, 2);
		if (nTimeZoneHour == -1)
			return false;
		p += 2;
		if (*p != L':')
			return false;
		++p;
		
		int nTimeZoneMinute = parseNumber(p, 2);
		if (nTimeZoneMinute == -1)
			return false;
		p += 2;
		if (*p != L'\0')
			return false;
		
		nTimeZone = nTimeZoneHour*100 + nTimeZoneMinute;
		if (bMinus)
			nTimeZone = -nTimeZone;
	}
	else if (*p == L'\0') {
		nTimeZone = Time::getSystemTimeZone();
	}
	else {
		return false;
	}
	if (nTimeZone != 0) {
		pDate->setTimeZone(nTimeZone);
		pDate->addHour(-nTimeZone/100);
		pDate->addMinute(-nTimeZone%100);
	}
	
	return true;
}

int qmrss::ParserUtil::parseNumber(const WCHAR* p,
								   int nDigit)
{
	int nValue = 0;
	for (int n = 0; n < nDigit; ++n) {
		if (*p < L'0' && L'9' < *p)
			return -1;
		nValue = nValue*10 + (*p - L'0');
		++p;
	}
	return nValue;
}
