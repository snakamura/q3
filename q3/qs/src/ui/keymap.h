/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __KEYMAP_H__
#define __KEYMAP_H__

#include <qskeymap.h>
#include <qssax.h>


namespace qs {

/****************************************************************************
 *
 * KeyMapContentHandler
 *
 */

class KeyMapContentHandler : public DefaultHandler
{
public:
	typedef std::vector<std::pair<std::pair<WSTRING, WSTRING>, ACCEL> > AccelMap;

public:
	KeyMapContentHandler(AccelMap* pMapAccel, QSTATUS* pstatus);
	virtual ~KeyMapContentHandler();

public:
	virtual qs::QSTATUS startElement(const WCHAR* pwszNamespaceURI,
		const WCHAR* pwszLocalName, const WCHAR* pwszQName,
		const qs::Attributes& attributes);
	virtual qs::QSTATUS endElement(const WCHAR* pwszNamespaceURI,
		const WCHAR* pwszLocalName, const WCHAR* pwszQName);
	virtual qs::QSTATUS characters(const WCHAR* pwsz,
		size_t nStart, size_t nLength);

private:
	KeyMapContentHandler(const KeyMapContentHandler&);
	KeyMapContentHandler& operator=(const KeyMapContentHandler&);

private:
	enum State {
		STATE_ROOT,
		STATE_KEYMAPS,
		STATE_KEYMAP,
		STATE_ACTION,
		STATE_KEY
	};

private:
	AccelMap* pMapAccel_;
	State state_;
	WSTRING wstrCurrentName_;
	WSTRING wstrCurrentAction_;
};

}

#endif // __KEYMAP_H__
