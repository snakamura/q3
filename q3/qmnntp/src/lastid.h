/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
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
	LastIdList(const WCHAR* pwszPath, qs::QSTATUS* pstatus);
	~LastIdList();

public:
	const IdList& getList() const;
	unsigned int getLastId(const WCHAR* pwszName) const;
	qs::QSTATUS setLastId(const WCHAR* pwszName, unsigned int nId);
	bool isModified() const;
	qs::QSTATUS save();

private:
	qs::QSTATUS load();

private:
	LastIdList(const LastIdList&);
	LastIdList& operator=(const LastIdList&);

private:
	qs::WSTRING wstrPath_;
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
	LastIdContentHandler(LastIdList* pList, qs::QSTATUS* pstatus);
	virtual ~LastIdContentHandler();

public:
	virtual qs::QSTATUS startElement(const WCHAR* pwszNamespaceURI,
		const WCHAR* pwszLocalName, const WCHAR* pwszQName,
		const qs::Attributes& attributes);
	virtual qs::QSTATUS endElement(const WCHAR* pwszNamespaceURI,
		const WCHAR* pwszLocalName, const WCHAR* pwszQName);
	virtual qs::QSTATUS characters(const WCHAR* pwsz,
		size_t nStart, size_t nLength);

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
	qs::WSTRING wstrName_;
	qs::StringBuffer<qs::WSTRING>* pBuffer_;
};


/****************************************************************************
 *
 * LastIdWriter
 *
 */

class LastIdWriter
{
public:
	LastIdWriter(qs::Writer* pWriter, qs::QSTATUS* pstatus);
	~LastIdWriter();

public:
	qs::QSTATUS write(const LastIdList& l);

private:
	LastIdWriter(const LastIdWriter&);
	LastIdWriter& operator=(const LastIdWriter&);

private:
	qs::OutputHandler handler_;
};

}

#endif __LASTID_H__
