/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
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
	URI(const WCHAR* pwszAccount,
		const WCHAR* pwszFolder,
		unsigned int nValidity,
		unsigned int nId);
	URI(MessageHolder* pmh);
	~URI();

public:
	const WCHAR* getAccount() const;
	const WCHAR* getFolder() const;
	unsigned int getValidity() const;
	unsigned int getId() const;
	qs::wstring_ptr toString() const;

public:
	static const WCHAR* getScheme();
	static std::auto_ptr<URI> parse(const WCHAR* pwszURI);

private:
	URI(const URI&);
	URI& operator=(const URI&);

private:
	qs::wstring_ptr wstrAccount_;
	qs::wstring_ptr wstrFolder_;
	unsigned int nValidity_;
	unsigned int nId_;
};

}

#endif // __URI_H__
