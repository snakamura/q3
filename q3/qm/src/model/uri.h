/*
 * $Id: uri.h,v 1.1.1.1 2003/04/29 08:07:31 snakamura Exp $
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


/****************************************************************************
 *
 * URI
 *
 */

class URI
{
public:
	static qs::QSTATUS getMessageHolder(const WCHAR* pwszURI,
		Document* pDocument, MessageHolder** ppmh);
	static qs::QSTATUS getURI(MessageHolder* pmh, qs::WSTRING* pwstrURI);
};

}

#endif // __URI_H__
