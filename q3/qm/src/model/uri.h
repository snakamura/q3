/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __URI_H__
#define __URI_H__

#include <qm.h>

#include <qs.h>
#include <qsmime.h>
#include <qsstring.h>


namespace qm {

class URI;
class URIFragment;

class Document;
class Message;
class MessageHolder;
class MessagePtr;


/****************************************************************************
 *
 * URIFragment
 *
 */

class URIFragment
{
public:
	enum Type {
		TYPE_NONE,
		TYPE_MIME,
		TYPE_BODY,
		TYPE_HEADER,
		TYPE_TEXT
	};

public:
	typedef std::vector<unsigned int> Section;

public:
	URIFragment();
	URIFragment(const Section& section,
				Type type,
				const WCHAR* pwszName);
	URIFragment(MessageHolder* pmh);
	URIFragment(Message* pMessage,
				const qs::Part* pPart,
				Type type);
	URIFragment(const URIFragment& fragment);
	~URIFragment();

public:
	const Section& getSection() const;
	Type getType() const;
	const WCHAR* getName() const;
	qs::wstring_ptr toString() const;
	const qs::Part* getPart(const Message* pMessage) const;

private:
	URIFragment& operator=(const URIFragment&);

private:
	Section section_;
	Type type_;
	qs::wstring_ptr wstrName_;
};

bool operator==(const URIFragment& lhs,
				const URIFragment& rhs);
bool operator!=(const URIFragment& lhs,
				const URIFragment& rhs);
bool operator<(const URIFragment& lhs,
			   const URIFragment& rhs);


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
		unsigned int nId,
		const URIFragment::Section& section,
		URIFragment::Type type,
		const WCHAR* pwszName);
	URI(MessageHolder* pmh);
	URI(MessageHolder* pmh,
		Message* pMessage,
		const qs::Part* pPart,
		URIFragment::Type type);
	URI(const URI& uri);
	~URI();

public:
	const WCHAR* getAccount() const;
	const WCHAR* getFolder() const;
	unsigned int getValidity() const;
	unsigned int getId() const;
	const URIFragment& getFragment() const;
	qs::wstring_ptr toString() const;

public:
	static const WCHAR* getScheme();
	static std::auto_ptr<URI> parse(const WCHAR* pwszURI);

private:
	URI& operator=(const URI&);

private:
	qs::wstring_ptr wstrAccount_;
	qs::wstring_ptr wstrFolder_;
	unsigned int nValidity_;
	unsigned int nId_;
	URIFragment fragment_;
};

bool operator==(const URI& lhs,
				const URI& rhs);
bool operator!=(const URI& lhs,
				const URI& rhs);
bool operator<(const URI& lhs,
			   const URI& rhs);

}

#endif // __URI_H__
