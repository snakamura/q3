/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#ifndef __MESSAGECONTEXT_H__
#define __MESSAGECONTEXT_H__

#include <qm.h>
#include <qmmessage.h>
#include <qmmessageholder.h>

#include "uri.h"


namespace qm {

class MessageContext;
	class MessagePtrMessageContext;
	class MessageMessageContext;

class Account;


/****************************************************************************
 *
 * MessageContext
 *
 */

class MessageContext
{
public:
	virtual ~MessageContext();

public:
	virtual Account* getAccount() const = 0;
	virtual MessagePtr getMessagePtr() const = 0;
	virtual Message* getMessage(unsigned int nFlags,
								const WCHAR* pwszField,
								unsigned int nSecurityMode) = 0;
	virtual MessagePtr getOriginMessagePtr() const = 0;
	virtual std::auto_ptr<URI> getURI() const = 0;
	virtual std::auto_ptr<URI> getURI(const qs::Part* pPart,
									  URIFragment::Type type) const = 0;
};


/****************************************************************************
 *
 * MessagePtrMessageContext
 *
 */

class MessagePtrMessageContext : public MessageContext
{
public:
	explicit MessagePtrMessageContext(MessageHolder* pmh);
	explicit MessagePtrMessageContext(MessagePtr ptr);
	~MessagePtrMessageContext();

public:
	virtual Account* getAccount() const;
	virtual MessagePtr getMessagePtr() const;
	virtual Message* getMessage(unsigned int nFlags,
								const WCHAR* pwszField,
								unsigned int nSecurityMode);
	virtual MessagePtr getOriginMessagePtr() const;
	virtual std::auto_ptr<URI> getURI() const;
	virtual std::auto_ptr<URI> getURI(const qs::Part* pPart,
									  URIFragment::Type type) const;

private:
	MessagePtrMessageContext(const MessagePtrMessageContext&);
	MessagePtrMessageContext& operator=(const MessagePtrMessageContext&);

private:
	MessagePtr ptr_;
	Message msg_;
	unsigned int nSecurityMode_;
};


/****************************************************************************
 *
 * MessageMessageContext
 *
 */

class MessageMessageContext : public MessageContext
{
public:
	MessageMessageContext(const CHAR* pszMessage,
						  size_t nLen,
						  const MessagePtr& originPtr);
	MessageMessageContext(const MessageMessageContext& context);
	virtual ~MessageMessageContext();

public:
	bool operator!() const;
	void registerURI(URIResolver* pURIResolver);

public:
	virtual Account* getAccount() const;
	virtual MessagePtr getMessagePtr() const;
	virtual Message* getMessage(unsigned int nFlags,
								const WCHAR* pwszField,
								unsigned int nSecurityMode);
	virtual MessagePtr getOriginMessagePtr() const;
	virtual std::auto_ptr<URI> getURI() const;
	virtual std::auto_ptr<URI> getURI(const qs::Part* pPart,
									  URIFragment::Type type) const;

private:
	MessageMessageContext& operator=(const MessageMessageContext&);

private:
	Message msg_;
	MessagePtr originPtr_;
	bool bSuccess_;
	URIResolver* pURIResolver_;
	unsigned int nURIId_;
};

}

#endif // __MESSAGECONTEXT_H__
