/*
 * $Id: profile.h,v 1.1.1.1 2003/04/29 08:07:37 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
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
	XMLProfileContentHandler(AbstractProfile::Map* pMap, QSTATUS* pstatus);
	virtual ~XMLProfileContentHandler();

public:
	virtual QSTATUS startElement(const WCHAR* pwszNamespaceURI,
		const WCHAR* pwszLocalName, const WCHAR* pwszQName,
		const Attributes& attributes);
	virtual QSTATUS endElement(const WCHAR* pwszNamespaceURI,
		const WCHAR* pwszLocalName, const WCHAR* pwszQName);
	virtual QSTATUS characters(const WCHAR* pwsz,
		size_t nStart, size_t nLength);

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
	WSTRING wstrSection_;
	WSTRING wstrEntry_;
	StringBuffer<WSTRING>* pBuffer_;
};


/****************************************************************************
 *
 * XMLProfileAttributes
 *
 */

class XMLProfileAttributes : public DefaultAttributes
{
public:
	XMLProfileAttributes(const WCHAR* pwszName);
	virtual ~XMLProfileAttributes();

public:
	virtual int getLength() const;
	virtual const WCHAR* getQName(int nIndex) const;
	virtual const WCHAR* getValue(int nIndex) const;

private:
	XMLProfileAttributes(const XMLProfileAttributes&);
	XMLProfileAttributes& operator=(const XMLProfileAttributes&);

private:
	const WCHAR* pwszName_;
};




}

#endif // __PROFILE_H__
