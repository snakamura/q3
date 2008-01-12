/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
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
	class MessageHolderURI;
	class TemporaryURI;
class URIFragment;
class URIFactory;
class URIResolver;

class Document;
class Message;
class MessageContext;
class MessageHolder;
class MessageMessageContext;
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
	URIFragment(const Message* pMessage);
	URIFragment(const Message* pMessage,
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
	virtual ~URI();

public:
	virtual const URIFragment& getFragment() const = 0;
	virtual std::auto_ptr<MessageContext> resolve(const URIResolver* pResolver) const = 0;
	virtual MessagePtr resolveMessagePtr(const URIResolver* pResolver) const = 0;
	virtual qs::wstring_ptr toString() const = 0;
	virtual std::auto_ptr<URI> clone() const = 0;
};


/****************************************************************************
 *
 * MessageHolderURI
 *
 */

class MessageHolderURI : public URI
{
public:
	MessageHolderURI(const WCHAR* pwszAccount,
					 const WCHAR* pwszFolder,
					 unsigned int nValidity,
					 unsigned int nId,
					 const URIFragment& fragment);
	explicit MessageHolderURI(MessageHolder* pmh);
	MessageHolderURI(MessageHolder* pmh,
					 const Message* pMessage,
					 const qs::Part* pPart,
					 URIFragment::Type type);
	MessageHolderURI(const MessageHolderURI& uri);
	virtual ~MessageHolderURI();

public:
	const WCHAR* getAccount() const;
	const WCHAR* getFolder() const;
	unsigned int getValidity() const;
	unsigned int getId() const;

public:
	virtual const URIFragment& getFragment() const;
	virtual std::auto_ptr<MessageContext> resolve(const URIResolver* pResolver) const;
	virtual MessagePtr resolveMessagePtr(const URIResolver* pResolver) const;
	virtual qs::wstring_ptr toString() const;
	virtual std::auto_ptr<URI> clone() const;

public:
	static const WCHAR* getScheme();

private:
	MessageHolderURI& operator=(const MessageHolderURI&);

private:
	qs::wstring_ptr wstrAccount_;
	qs::wstring_ptr wstrFolder_;
	unsigned int nValidity_;
	unsigned int nId_;
	URIFragment fragment_;
};

bool operator==(const MessageHolderURI& lhs,
				const MessageHolderURI& rhs);
bool operator!=(const MessageHolderURI& lhs,
				const MessageHolderURI& rhs);
bool operator<(const MessageHolderURI& lhs,
			   const MessageHolderURI& rhs);


/****************************************************************************
 *
 * TemporaryURI
 *
 */

class TemporaryURI : public URI
{
public:
	TemporaryURI(unsigned int nId,
				 const URIFragment& fragment);
	TemporaryURI(unsigned int nId,
				 const Message* pMessage);
	TemporaryURI(unsigned int nId,
				 const Message* pMessage,
				 const qs::Part* pPart,
				 URIFragment::Type type);
	TemporaryURI(const TemporaryURI& uri);
	virtual ~TemporaryURI();

public:
	virtual const URIFragment& getFragment() const;
	virtual std::auto_ptr<MessageContext> resolve(const URIResolver* pResolver) const;
	virtual MessagePtr resolveMessagePtr(const URIResolver* pResolver) const;
	virtual qs::wstring_ptr toString() const;
	virtual std::auto_ptr<URI> clone() const;

public:
	static const WCHAR* getScheme();

private:
	TemporaryURI& operator=(const TemporaryURI&);

private:
	unsigned int nId_;
	URIFragment fragment_;
};


/****************************************************************************
 *
 * URIFactory
 *
 */

class URIFactory
{
public:
	static std::auto_ptr<URI> parseURI(const WCHAR* pwszURI);
	static std::auto_ptr<MessageHolderURI> parseMessageHolderURI(const WCHAR* pwszURI);
	static std::auto_ptr<TemporaryURI> parseTemporaryURI(const WCHAR* pwszURI);
	static bool isURI(const WCHAR* pwszURI);
	static bool isMessageHolderURI(const WCHAR* pwszURI);
	static bool isTemporaryURI(const WCHAR* pwszURI);

private:
	static URIFragment parseFragment(WCHAR* pwszFragment);
};


/****************************************************************************
 *
 * URIResolver
 *
 */

class URIResolver
{
public:
	explicit URIResolver(AccountManager* pAccountManager);
	~URIResolver();

public:
	Account* getAccount(const WCHAR* pwszName) const;
	std::auto_ptr<MessageContext> getMessageContext(unsigned int nId) const;
	std::auto_ptr<URI> getTemporaryURI(const Message* pMessage,
									   unsigned int nSecurityMode) const;
	std::auto_ptr<URI> getTemporaryURI(const qs::Part* pPart,
									   URIFragment::Type type,
									   unsigned int nSecurityMode) const;
	unsigned int registerMessageContext(MessageMessageContext* pContext);
	void unregisterMessageContext(unsigned int nId);

private:
	URIResolver(const URIResolver&);
	URIResolver& operator=(const URIResolver&);

private:
	typedef std::vector<std::pair<unsigned int, MessageMessageContext*> > MessageContextMap;

private:
	AccountManager* pAccountManager_;
	MessageContextMap mapMessageContext_;
	unsigned int nMaxId_;
};

}

#endif // __URI_H__
