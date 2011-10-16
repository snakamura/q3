/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#include <qmsecurity.h>

#include "messagecontext.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * MessageContext
 *
 */

qm::MessageContext::~MessageContext()
{
}


/****************************************************************************
 *
 * MessagePtrMessageContext
 *
 */

qm::MessagePtrMessageContext::MessagePtrMessageContext(MessageHolder* pmh) :
	ptr_(pmh),
	nSecurityMode_(SECURITYMODE_NONE)
{
	assert(pmh);
}

qm::MessagePtrMessageContext::MessagePtrMessageContext(MessagePtr ptr) :
	ptr_(ptr),
	nSecurityMode_(SECURITYMODE_NONE)
{
	assert(!!ptr);
}

qm::MessagePtrMessageContext::~MessagePtrMessageContext()
{
}

Account* qm::MessagePtrMessageContext::getAccount() const
{
	return ptr_.getFolder()->getAccount();
}

MessagePtr qm::MessagePtrMessageContext::getMessagePtr() const
{
	return ptr_;
}

Message* qm::MessagePtrMessageContext::getMessage(unsigned int nFlags,
												  const WCHAR* pwszField,
												  unsigned int nSecurityMode)
{
	MessagePtrLock mpl(ptr_);
	if (!mpl)
		return 0;
	
	if (nSecurityMode != nSecurityMode_)
		msg_.clear();
	nSecurityMode_ = nSecurityMode;
	
	if (!mpl->getMessage(nFlags, pwszField, nSecurityMode, &msg_))
		return 0;
	
	return &msg_;
}

MessagePtr qm::MessagePtrMessageContext::getOriginMessagePtr() const
{
	return ptr_;
}

std::auto_ptr<URI> qm::MessagePtrMessageContext::getURI() const
{
	MessagePtrLock mpl(ptr_);
	if (!mpl)
		return std::auto_ptr<URI>();
	return std::auto_ptr<URI>(new MessageHolderURI(mpl));
}

std::auto_ptr<URI> qm::MessagePtrMessageContext::getURI(const qs::Part* pPart,
														URIFragment::Type type) const
{
	assert(pPart->getRootPart() == &msg_);
	
	MessagePtrLock mpl(ptr_);
	if (!mpl)
		return std::auto_ptr<URI>();
	return std::auto_ptr<URI>(new MessageHolderURI(
		mpl, &msg_, pPart, URIFragment::TYPE_BODY));
}

MessageContextPtr qm::MessagePtrMessageContext::safeCopy() const
{
	// If this message has not been fully loaded,
	// and the associated folder has FLAG_CACHEWHENREAD flag,
	// MessageContext hold by MessageModel would be changed
	// while calling getMessage, which will delete the MessageContext.
	// To avoid being deleted in getMessage, we have to call this
	// method to copy MessageContext before calling getMessage.
	// Actually, it's not a good idea to check these conditions here
	// because MessageContext doesn't need to know about MessageModel.
	// But for performance reasons, we check them here.
	if (msg_.getFlag() == Message::FLAG_NONE) {
		return MessageContextPtr(this);
	}
	else {
		MessagePtrLock mpl(ptr_);
		if (!mpl || !mpl->getFolder()->isFlag(Folder::FLAG_CACHEWHENREAD))
			return MessageContextPtr(this);
		else
			return MessageContextPtr(new MessagePtrMessageContext(ptr_), true);
	}
}


/****************************************************************************
 *
 * MessageMessageContext
 *
 */

qm::MessageMessageContext::MessageMessageContext(const CHAR* pszMessage,
												 size_t nLen,
												 const MessagePtr& originPtr) :
	originPtr_(originPtr),
	bSuccess_(true),
	pURIResolver_(0),
	nURIId_(-1)
{
	bSuccess_ = msg_.create(pszMessage, nLen, Message::FLAG_NONE);
}

qm::MessageMessageContext::MessageMessageContext(const MessageMessageContext& context) :
	originPtr_(context.originPtr_),
	bSuccess_(true),
	pURIResolver_(context.pURIResolver_),
	nURIId_(-1)
{
	xstring_size_ptr strContent(context.msg_.getContent());
	bSuccess_ = msg_.create(strContent.get(), strContent.size(), Message::FLAG_NONE);
	if (pURIResolver_)
		nURIId_ = pURIResolver_->registerMessageContext(this);
}

qm::MessageMessageContext::~MessageMessageContext()
{
	if (pURIResolver_)
		pURIResolver_->unregisterMessageContext(nURIId_);
}

bool qm::MessageMessageContext::operator!() const
{
	return !bSuccess_;
}

void qm::MessageMessageContext::registerURI(URIResolver* pURIResolver)
{
	pURIResolver_ = pURIResolver;
	nURIId_ = pURIResolver_->registerMessageContext(this);
}

Account* qm::MessageMessageContext::getAccount() const
{
	return 0;
}

MessagePtr qm::MessageMessageContext::getMessagePtr() const
{
	return MessagePtr();
}

Message* qm::MessageMessageContext::getMessage(unsigned int nFlags,
											   const WCHAR* pwszField,
											   unsigned int nSecurityMode)
{
	return &msg_;
}

MessagePtr qm::MessageMessageContext::getOriginMessagePtr() const
{
	return originPtr_;
}

std::auto_ptr<URI> qm::MessageMessageContext::getURI() const
{
	if (nURIId_ == -1)
		return std::auto_ptr<URI>();
	return std::auto_ptr<URI>(new TemporaryURI(nURIId_, &msg_));
}

std::auto_ptr<URI> qm::MessageMessageContext::getURI(const qs::Part* pPart,
													 URIFragment::Type type) const
{
	assert(pPart->getRootPart() == &msg_);
	
	if (nURIId_ == -1)
		return std::auto_ptr<URI>();
	return std::auto_ptr<URI>(new TemporaryURI(nURIId_, &msg_, pPart, type));
}

MessageContextPtr qm::MessageMessageContext::safeCopy() const
{
	return MessageContextPtr(this);
}


/*****************************************************************************
 *
 * MessageContextPtr
 *
 */

qm::MessageContextPtr::MessageContextPtr(MessageContext *pContext,
                                         bool bDelete)
{
	pContext_ = pContext;
	bDelete_ = bDelete;
}

qm::MessageContextPtr::MessageContextPtr(const MessageContext *pContext)
{
	pContext_ = const_cast<MessageContext*>(pContext);
	bDelete_ = false;
}

qm::MessageContextPtr::MessageContextPtr(MessageContextPtr& ptr)
{
	pContext_ = ptr.pContext_;
	bDelete_ = ptr.bDelete_;
	
	ptr.pContext_ = 0;
	ptr.bDelete_ = false;
}

qm::MessageContextPtr::~MessageContextPtr()
{
	if (bDelete_)
		delete pContext_;
}

bool qm::MessageContextPtr::operator!() const
{
	return pContext_ == 0;
}

MessageContext* qm::MessageContextPtr::operator->() const
{
	return pContext_;
}

MessageContext* qm::MessageContextPtr::get() const
{
	return pContext_;
}
