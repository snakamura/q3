/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __RSS_H__
#define __RSS_H__

#include <qssax.h>
#include <qsstream.h>
#include <qsutil.h>


namespace qmrss {

class Channel;
class Item;
class RssParser;
class RssContentHandler;
class RssHandler;
	class Rss10Handler;
	class Rss20Handler;


/****************************************************************************
 *
 * Channel
 *
 */

class Channel
{
public:
	typedef std::vector<Item*> ItemList;

public:
	explicit Channel(const WCHAR* pwszURL);
	~Channel();

public:
	const WCHAR* getURL() const;
	const qs::Time& getPubDate() const;
	const ItemList& getItems() const;

public:
	void setPubDate(const qs::Time& time);
	void addItem(std::auto_ptr<Item> pItem);

private:
	Channel(Channel&);
	Channel& operator=(Channel&);

private:
	qs::wstring_ptr wstrURL_;
	qs::Time timePubDate_;
	ItemList listItem_;
};


/****************************************************************************
 *
 * Item
 *
 */

class Item
{
public:
	Item();
	~Item();

public:
	const WCHAR* getTitle() const;
	const WCHAR* getLink() const;
	const WCHAR* getDescription() const;
	const WCHAR* getCategory() const;
	const WCHAR* getSubject() const;
	const WCHAR* getCreator() const;
	const qs::Time& getPubDate() const;
	const WCHAR* getContentEncoded() const;
	qs::wstring_ptr getHash() const;

public:
	void setTitle(qs::wstring_ptr wstrTitle);
	void setLink(qs::wstring_ptr wstrLink);
	void setDescription(qs::wstring_ptr wstrDescription);
	void addCategory(qs::wstring_ptr wstrCategory);
	void addSubject(qs::wstring_ptr wstrSubject);
	void addCreator(qs::wstring_ptr wstrCreator);
	void setPubDate(const qs::Time& time);
	void setContentEncoded(qs::wstring_ptr wstrContentEncoded);

private:
	Item(const Item&);
	Item& operator=(const Item&);

private:
	qs::wstring_ptr wstrTitle_;
	qs::wstring_ptr wstrLink_;
	qs::wstring_ptr wstrDescription_;
	qs::wstring_ptr wstrCategory_;
	qs::wstring_ptr wstrSubject_;
	qs::wstring_ptr wstrCreator_;
	qs::Time timePubDate_;
	qs::wstring_ptr wstrContentEncoded_;
};


/****************************************************************************
 *
 * RssParser
 *
 */

class RssParser
{
public:
	RssParser();
	~RssParser();

public:
	std::auto_ptr<Channel> parse(const WCHAR* pwszURL,
								 qs::InputStream* pInputStream);

private:
	RssParser(const RssParser&);
	RssParser& operator=(const RssParser&);
};


/****************************************************************************
 *
 * RssContentHandler
 *
 */

class RssContentHandler : public qs::DefaultHandler
{
public:
	explicit RssContentHandler(Channel* pChannel);
	virtual ~RssContentHandler();

public:
	virtual bool startElement(const WCHAR* pwszNamespaceURI,
							  const WCHAR* pwszLocalName,
							  const WCHAR* pwszQName,
							  const qs::Attributes& attributes);
	virtual bool endElement(const WCHAR* pwszNamespaceURI,
							const WCHAR* pwszLocalName,
							const WCHAR* pwszQName);
	virtual bool characters(const WCHAR* pwsz,
							size_t nStart,
							size_t nLength);

private:
	RssContentHandler(const RssContentHandler&);
	RssContentHandler& operator=(const RssContentHandler&);

private:
	Channel* pChannel_;
	std::auto_ptr<RssHandler> pHandler_;
};


/****************************************************************************
 *
 * RssHandler
 *
 */

class RssHandler
{
public:
	virtual ~RssHandler();

public:
	virtual bool startElement(const WCHAR* pwszNamespaceURI,
							  const WCHAR* pwszLocalName,
							  const WCHAR* pwszQName,
							  const qs::Attributes& attributes) = 0;
	virtual bool endElement(const WCHAR* pwszNamespaceURI,
							const WCHAR* pwszLocalName,
							const WCHAR* pwszQName) = 0;
	virtual bool characters(const WCHAR* pwsz,
							size_t nStart,
							size_t nLength) = 0;
};


/****************************************************************************
 *
 * Rss10Handler
 *
 */

class Rss10Handler : public RssHandler
{
public:
	explicit Rss10Handler(Channel* pChannel);
	virtual ~Rss10Handler();

public:
	virtual bool startElement(const WCHAR* pwszNamespaceURI,
							  const WCHAR* pwszLocalName,
							  const WCHAR* pwszQName,
							  const qs::Attributes& attributes);
	virtual bool endElement(const WCHAR* pwszNamespaceURI,
							const WCHAR* pwszLocalName,
							const WCHAR* pwszQName);
	virtual bool characters(const WCHAR* pwsz,
							size_t nStart,
							size_t nLength);

private:
	Rss10Handler(const Rss10Handler&);
	Rss10Handler& operator=(const Rss10Handler&);

private:
	enum State {
		STATE_ROOT,
		STATE_RDF,
		STATE_CHANNEL,
		STATE_DATE,
		STATE_ITEM,
		STATE_PROPERTY,
		STATE_UNKNOWN
	};

private:
	typedef std::vector<State> StateStack;

private:
	Channel* pChannel_;
	StateStack stackState_;
	qs::StringBuffer<qs::WSTRING> buffer_;
	Item* pCurrentItem_;
};


/****************************************************************************
 *
 * Rss20Handler
 *
 */

class Rss20Handler : public RssHandler
{
public:
	explicit Rss20Handler(Channel* pChannel);
	virtual ~Rss20Handler();

public:
	virtual bool startElement(const WCHAR* pwszNamespaceURI,
							  const WCHAR* pwszLocalName,
							  const WCHAR* pwszQName,
							  const qs::Attributes& attributes);
	virtual bool endElement(const WCHAR* pwszNamespaceURI,
							const WCHAR* pwszLocalName,
							const WCHAR* pwszQName);
	virtual bool characters(const WCHAR* pwsz,
							size_t nStart,
							size_t nLength);

private:
	Rss20Handler(const Rss20Handler&);
	Rss20Handler& operator=(const Rss20Handler&);

private:
	enum State {
		STATE_ROOT,
		STATE_RSS,
		STATE_CHANNEL,
		STATE_PUBDATE,
		STATE_ITEM,
		STATE_PROPERTY,
		STATE_UNKNOWN
	};

private:
	typedef std::vector<State> StateStack;

private:
	Channel* pChannel_;
	StateStack stackState_;
	qs::StringBuffer<qs::WSTRING> buffer_;
	Item* pCurrentItem_;
};


/****************************************************************************
 *
 * ParserUtil
 *
 */

struct ParserUtil
{
	static bool parseDate(const WCHAR* pwszDate,
						  qs::Time* pDate);
	static int parseNumber(const WCHAR* p,
						   int nDigit);
};

}

#endif // __RSS_H__
