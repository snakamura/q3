/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __LASTID_H__
#define __LASTID_H__

#include <qm.h>

#include <qs.h>
#include <qssax.h>
#include <qsstring.h>

#include <vector>


namespace qmnntp {

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
	bool isModified() const;
	bool save();

private:
	bool load();

private:
	LastIdList(const LastIdList&);
	LastIdList& operator=(const LastIdList&);

private:
	qs::wstring_ptr wstrPath_;
	IdList listId_;
	bool bModified_;
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
	explicit LastIdWriter(qs::Writer* pWriter);
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
