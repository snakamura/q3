/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __URI_H__
#define __URI_H__

#include <qm.h>

#include <qs.h>
#include <qsstring.h>


namespace qm {

class URI;

class Document;
class MessageHolder;
class MessagePtr;


/****************************************************************************
 *
 * URI
 *
 */

class URI
{
public:
	static qs::QSTATUS getMessageHolder(const WCHAR* pwszURI,
		Document* pDocument, MessagePtr* pptr);
	static qs::QSTATUS getURI(MessageHolder* pmh, qs::WSTRING* pwstrURI);
};

}

#endif // __URI_H__
