/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
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
class SyncFilterWriter;


/****************************************************************************
 *
 * SyncFilterContentHandler
 *
 */

class SyncFilterContentHandler : public qs::DefaultHandler
{
public:
	explicit SyncFilterContentHandler(SyncFilterManager* pManager);
	virtual ~SyncFilterContentHandler();

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
	qs::wstring_ptr wstrCurrentParamName_;
	qs::StringBuffer<qs::WSTRING> buffer_;
	std::auto_ptr<MacroParser> pParser_;
};


/****************************************************************************
 *
 * SyncFilterWriter
 *
 */

class SyncFilterWriter
{
public:
	explicit SyncFilterWriter(qs::Writer* pWriter);
	~SyncFilterWriter();

public:
	bool write(const SyncFilterManager* pManager);

private:
	bool write(const SyncFilterSet* pFilterSet);
	bool write(const SyncFilter* pFilter);
	bool write(const SyncFilterAction* pAction);

private:
	SyncFilterWriter(const SyncFilterWriter&);
	SyncFilterWriter& operator=(const SyncFilterWriter&);

private:
	qs::OutputHandler handler_;
};

}

#endif // __SYNCFILTER_H__
