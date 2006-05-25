/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __FEED_H__
#define __FEED_H__

#include <qssax.h>
#include <qsstring.h>
#include <qsthread.h>
#include <qsutil.h>

#include <qmaccount.h>

#include <vector>


namespace qmrss {

class FeedList;
class Feed;
class FeedItem;
class FeedManager;
class FeedContentHandler;
class FeedWriter;


/****************************************************************************
 *
 * FeedList
 *
 */

class FeedList
{
public:
	typedef std::vector<Feed*> List;

public:
	explicit FeedList(const WCHAR* pwszPath);
	~FeedList();

public:
	const List& getFeeds() const;
	const Feed* getFeed(const WCHAR* pwszURL) const;
	void setFeed(std::auto_ptr<Feed> pFeed);
	void removeFeed(const Feed* pFeed);
	bool save();
	
	void lock() const;
	void unlock() const;
#ifndef NDEBUG
	bool isLocked() const;
#endif

private:
	bool load();

private:
	FeedList(const FeedList&);
	FeedList& operator=(const FeedList&);

private:
	qs::wstring_ptr wstrPath_;
	List list_;
	bool bModified_;
	qs::CriticalSection cs_;
#ifndef NDEBUG
	mutable unsigned int nLock_;
#endif
};


/****************************************************************************
 *
 * Feed
 *
 */

class Feed
{
public:
	typedef std::vector<FeedItem*> ItemList;

public:
	Feed(const WCHAR* pwszURL,
		 const qs::Time& timeLastModified);
	~Feed();

public:
	const WCHAR* getURL() const;
	const qs::Time& getLastModified() const;
	const ItemList& getItems() const;
	const FeedItem* getItem(const WCHAR* pwszKey) const;
	void addItem(std::auto_ptr<FeedItem> pItem);

private:
	Feed(const Feed&);
	Feed& operator=(const Feed&);

private:
	qs::wstring_ptr wstrURL_;
	qs::Time timeLastModified_;
	ItemList listItem_;
};


/****************************************************************************
 *
 * FeedItem
 *
 */

class FeedItem
{
public:
	FeedItem(const WCHAR* pwszKey);
	~FeedItem();

public:
	const WCHAR* getKey() const;

private:
	FeedItem(const FeedItem&);
	FeedItem& operator=(const FeedItem&);

private:
	qs::wstring_ptr wstrKey_;
};


/****************************************************************************
 *
 * FeedManager
 *
 */

class FeedManager
{
public:
	FeedManager();
	~FeedManager();

public:
	FeedList* get(qm::Account* pAccount);

private:
	FeedManager(const FeedManager&);
	FeedManager& operator=(const FeedManager&);

private:
	typedef std::vector<std::pair<qm::Account*, FeedList*> > Map;

private:
	Map map_;
	qs::CriticalSection cs_;
};


/****************************************************************************
 *
 * FeedContentHandler
 *
 */

class FeedContentHandler : public qs::DefaultHandler
{
public:
	explicit FeedContentHandler(FeedList* pList);
	virtual ~FeedContentHandler();

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
	FeedContentHandler(const FeedContentHandler&);
	FeedContentHandler& operator=(const FeedContentHandler&);

private:
	enum State {
		STATE_ROOT,
		STATE_FEEDLIST,
		STATE_FEED,
		STATE_ITEM
	};

private:
	FeedList* pList_;
	State state_;
	Feed* pCurrentFeed_;
	qs::StringBuffer<qs::WSTRING> buffer_;
};


/****************************************************************************
 *
 * FeedWriter
 *
 */

class FeedWriter
{
public:
	explicit FeedWriter(qs::Writer* pWriter);
	~FeedWriter();

public:
	bool write(const FeedList& l);

private:
	FeedWriter(const FeedWriter&);
	FeedWriter& operator=(const FeedWriter&);

private:
	qs::OutputHandler handler_;
};

}

#endif // __FEED_H__
