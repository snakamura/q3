/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __MESSAGEENUMERATOR_H__
#define __MESSAGEENUMERATOR_H__

#include <qm.h>

#include "uri.h"


namespace qm {

class MessageEnumerator;
	class MessageHolderListMessageEnumerator;
	class MessageContextMessageEnumerator;


/****************************************************************************
 *
 * MessageEnumerator
 *
 */

class MessageEnumerator
{
public:
	virtual ~MessageEnumerator();

public:
	virtual bool next() = 0;
	virtual void reset() = 0;
	virtual size_t size() const = 0;
	virtual Account* getAccount() const = 0;
	virtual Folder* getFolder() const = 0;
	virtual MessageHolder* getMessageHolder() const = 0;
	virtual Message* getMessage(unsigned int nFlags,
								const WCHAR* pwszField,
								unsigned int nSecurityMode,
								Message* pMessage) = 0;
	virtual MessagePtr getOriginMessagePtr() const = 0;
	virtual std::auto_ptr<URI> getURI(const Message* pMessage,
									  const qs::Part* pPart,
									  URIFragment::Type type) const = 0;
};


/****************************************************************************
 *
 * MessageHolderListMessageEnumerator
 *
 */

class MessageHolderListMessageEnumerator : public MessageEnumerator
{
public:
	MessageHolderListMessageEnumerator(Account* pAccount,
									   Folder* pFolder,
									   MessageHolderList& l);
	virtual ~MessageHolderListMessageEnumerator();

public:
	virtual bool next();
	virtual void reset();
	virtual size_t size() const;
	virtual Account* getAccount() const;
	virtual Folder* getFolder() const;
	virtual MessageHolder* getMessageHolder() const;
	virtual Message* getMessage(unsigned int nFlags,
								const WCHAR* pwszField,
								unsigned int nSecurityMode,
								Message* pMessage);
	virtual MessagePtr getOriginMessagePtr() const;
	virtual std::auto_ptr<URI> getURI(const Message* pMessage,
									  const qs::Part* pPart,
									  URIFragment::Type type) const;

private:
	MessageHolderListMessageEnumerator(const MessageHolderListMessageEnumerator&);
	MessageHolderListMessageEnumerator& operator=(const MessageHolderListMessageEnumerator&);

private:
	Account* pAccount_;
	Folder* pFolder_;
	MessageHolderList l_;
	MessageHolderList::const_iterator it_;
};


/****************************************************************************
 *
 * MessageContextMessageEnumerator
 *
 */

class MessageContextMessageEnumerator : public MessageEnumerator
{
public:
	explicit MessageContextMessageEnumerator(MessageContext* pContext);
	virtual ~MessageContextMessageEnumerator();

public:
	virtual bool next();
	virtual void reset();
	virtual size_t size() const;
	virtual Account* getAccount() const;
	virtual Folder* getFolder() const;
	virtual MessageHolder* getMessageHolder() const;
	virtual Message* getMessage(unsigned int nFlags,
								const WCHAR* pwszField,
								unsigned int nSecurityMode,
								Message* pMessage);
	virtual MessagePtr getOriginMessagePtr() const;
	virtual std::auto_ptr<URI> getURI(const Message* pMessage,
									  const qs::Part* pPart,
									  URIFragment::Type type) const;

private:
	MessageContextMessageEnumerator(const MessageContextMessageEnumerator&);
	MessageContextMessageEnumerator& operator=(const MessageContextMessageEnumerator&);

private:
	MessageContext* pContext_;
	Account* pAccount_;
	Folder* pFolder_;
	MessageHolder* pmh_;
	bool bFinish_;
};

}

#endif // __MESSAGEENUMERATOR_H__
