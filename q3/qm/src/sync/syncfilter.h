/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __SYNCFILTER_H__
#define __SYNCFILTER_H__

#include <qmmacro.h>
#include <qmsyncfilter.h>

#include <qssax.h>


namespace qm {

class SyncFilterContentHandler;


/****************************************************************************
 *
 * SyncFilterContentHandler
 *
 */

class SyncFilterContentHandler : public qs::DefaultHandler
{
public:
	typedef std::vector<SyncFilterSet*> FilterSetList;

public:
	SyncFilterContentHandler(SyncFilterManager* pManager, qs::QSTATUS* pstatus);
	virtual ~SyncFilterContentHandler();

public:
	virtual qs::QSTATUS startElement(const WCHAR* pwszNamespaceURI,
		const WCHAR* pwszLocalName, const WCHAR* pwszQName,
		const qs::Attributes& attributes);
	virtual qs::QSTATUS endElement(const WCHAR* pwszNamespaceURI,
		const WCHAR* pwszLocalName, const WCHAR* pwszQName);
	virtual qs::QSTATUS characters(const WCHAR* pwsz,
		size_t nStart, size_t nLength);

private:
	SyncFilterContentHandler(const SyncFilterContentHandler&);
	SyncFilterContentHandler& operator=(const SyncFilterContentHandler&);

private:
	enum State {
		STATE_ROOT,
		STATE_FILTERS,
		STATE_FILTERSET,
		STATE_FILTER,
		STATE_ACTION,
		STATE_PARAM
	};

private:
	SyncFilterManager* pManager_;
	State state_;
	SyncFilterSet* pCurrentFilterSet_;
	SyncFilter* pCurrentFilter_;
	SyncFilterAction* pCurrentAction_;
	qs::WSTRING wstrCurrentParamName_;
	qs::StringBuffer<qs::WSTRING>* pBuffer_;
	MacroParser* pParser_;
};

}

#endif // __SYNCFILTER_H__
