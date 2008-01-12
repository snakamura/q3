/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#ifndef __LASTID_H__
#define __LASTID_H__

#include <qm.h>
#include <qmaccount.h>

#include <qs.h>
#include <qssax.h>
#include <qsstring.h>
#include <qsthread.h>

#include <vector>


namespace qmnntp {

class LastIdList;
class LastIdManager;
class LastIdContentHandler;
class LastIdWriter;


/****************************************************************************
 *
 * LastIdList
 *
 */

class LastIdList
{
public:
	typedef std::vector<std::pair<qs::WSTRING, unsigned int> > IdList;

public:
	explicit LastIdList(const WCHAR* pwszPath);
	~LastIdList();

public:
	const IdList& getList() const;
	unsigned int getLastId(const WCHAR* pwszName) const;
	void setLastId(const WCHAR* pwszName,
				   unsigned int nId);
	void removeLastId(const WCHAR* pwszName);
	bool save();
	
	void lock() const;
	void unlock() const;
#ifndef NDEBUG
	bool isLocked() const;
#endif

private:
	bool load();

private:
	LastIdList(const LastIdList&);
	LastIdList& operator=(const LastIdList&);

private:
	qs::wstring_ptr wstrPath_;
	IdList listId_;
	bool bModified_;
	qs::CriticalSection cs_;
#ifndef NDEBUG
	mutable unsigned int nLock_;
#endif
};


/****************************************************************************
 *
 * LastIdManager
 *
 */

class LastIdManager
{
public:
	LastIdManager();
	~LastIdManager();

public:
	LastIdList* get(qm::Account* pAccount);

private:
	LastIdManager(const LastIdManager&);
	LastIdManager& operator=(const LastIdManager&);

private:
	typedef std::vector<std::pair<qm::Account*, LastIdList*> > Map;

private:
	Map map_;
	qs::CriticalSection cs_;
};


/****************************************************************************
 *
 * LastIdContentHandler
 *
 */

class LastIdContentHandler : public qs::DefaultHandler
{
public:
	explicit LastIdContentHandler(LastIdList* pList);
	virtual ~LastIdContentHandler();

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
	LastIdContentHandler(const LastIdContentHandler&);
	LastIdContentHandler& operator=(const LastIdContentHandler&);

private:
	enum State {
		STATE_ROOT,
		STATE_LASTIDLIST,
		STATE_LASTID
	};

private:
	LastIdList* pList_;
	State state_;
	qs::wstring_ptr wstrName_;
	qs::StringBuffer<qs::WSTRING> buffer_;
};


/****************************************************************************
 *
 * LastIdWriter
 *
 */

class LastIdWriter
{
public:
	LastIdWriter(qs::Writer* pWriter,
				 const WCHAR* pwszEncoding);
	~LastIdWriter();

public:
	bool write(const LastIdList& l);

private:
	LastIdWriter(const LastIdWriter&);
	LastIdWriter& operator=(const LastIdWriter&);

private:
	qs::OutputHandler handler_;
};

}

#endif __LASTID_H__
