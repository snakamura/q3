/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
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
	explicit KeyMapContentHandler(AccelMap* pMapAccel);
	virtual ~KeyMapContentHandler();

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
	wstring_ptr wstrCurrentName_;
	wstring_ptr wstrCurrentAction_;
};

}

#endif // __KEYMAP_H__
