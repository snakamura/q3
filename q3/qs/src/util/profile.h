/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __PROFILE_H__
#define __PROFILE_H__

#include <qs.h>
#include <qsprofile.h>
#include <qssax.h>
#include <qsstring.h>


namespace qs {

/****************************************************************************
 *
 * XMLProfileContentHandler
 *
 */

class XMLProfileContentHandler : public DefaultHandler
{
public:
	XMLProfileContentHandler(AbstractProfile::Map* pMap);
	virtual ~XMLProfileContentHandler();

public:
	virtual bool startElement(const WCHAR* pwszNamespaceURI,
							  const WCHAR* pwszLocalName,
							  const WCHAR* pwszQName,
							  const Attributes& attributes);
	virtual bool endElement(const WCHAR* pwszNamespaceURI,
							const WCHAR* pwszLocalName,
							const WCHAR* pwszQName);
	virtual bool characters(const WCHAR* pwsz,
							size_t nStart,
							size_t nLength);

private:
	XMLProfileContentHandler(const XMLProfileContentHandler&);
	XMLProfileContentHandler& operator=(const XMLProfileContentHandler&);

private:
	enum State {
		STATE_ROOT,
		STATE_PROFILE,
		STATE_SECTION,
		STATE_KEY
	};

private:
	AbstractProfile::Map* pMap_;
	State state_;
	wstring_ptr wstrSection_;
	wstring_ptr wstrEntry_;
	StringBuffer<WSTRING> buffer_;
};

}

#endif // __PROFILE_H__
