/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include "messagecontext.h"
#include "messageenumerator.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * MessageEnumerator
 *
 */

qm::MessageEnumerator::~MessageEnumerator()
{
}


/****************************************************************************
 *
 * MessageHolderListMessageEnumerator
 *
 */

qm::MessageHolderListMessageEnumerator::MessageHolderListMessageEnumerator(Account* pAccount,
																		   Folder* pFolder,
																		   MessageHolderList& l) :
	pAccount_(pAccount),
	pFolder_(pFolder)
{
	assert(!pAccount || pAccount->isLocked());
	assert((pAccount && pFolder) || l.empty());
	
	if (pAccount_)
		pAccount_->lock();
	
	l_.swap(l);
	it_ = l_.end();
}

qm::MessageHolderListMessageEnumerator::~MessageHolderListMessageEnumerator()
{
	if (pAccount_)
		pAccount_->unlock();
}

bool qm::MessageHolderListMessageEnumerator::next()
{
	if (it_ == l_.end())
		it_ = l_.begin();
	else
		++it_;
	return it_ != l_.end();
}

void qm::MessageHolderListMessageEnumerator::reset()
{
	it_ = l_.end();
}

size_t qm::MessageHolderListMessageEnumerator::size() const
{
	return l_.size();
}

Account* qm::MessageHolderListMessageEnumerator::getAccount() const
{
	return pAccount_;
}

Folder* qm::MessageHolderListMessageEnumerator::getFolder() const
{
	return pFolder_;
}

MessageHolder* qm::MessageHolderListMessageEnumerator::getMessageHolder() const
{
	assert(it_ != l_.end());
	return *it_;
}

Message* qm::MessageHolderListMessageEnumerator::getMessage(unsigned int nFlags,
															const WCHAR* pwszField,
															unsigned int nSecurityMode,
															Message* pMessage)
{
	assert(it_ != l_.end());
	
	if (!(*it_)->getMessage(nFlags, pwszField, nSecurityMode, pMessage))
		return 0;
	return pMessage;
}

MessagePtr qm::MessageHolderListMessageEnumerator::getOriginMessagePtr() const
{
	return MessagePtr();
}

std::auto_ptr<URI> qm::MessageHolderListMessageEnumerator::getURI(const Message* pMessage,
																  const qs::Part* pPart,
																  URIFragment::Type type) const
{
	assert(it_ != l_.end());
	return std::auto_ptr<URI>(new MessageHolderURI(
		*it_, pMessage, pPart, URIFragment::TYPE_BODY));
}


/****************************************************************************
 *
 * MessageContextMessageEnumerator
 *
 */

qm::MessageContextMessageEnumerator::MessageContextMessageEnumerator(MessageContext* pContext) :
	pContext_(pContext),
	pAccount_(0),
	pFolder_(0),
	pmh_(0),
	bFinish_(!pContext)
{
	if (pContext_) {
		MessagePtr ptr(pContext_->getMessagePtr());
		if (!!ptr) {
			MessagePtrLock mpl(ptr);
			if (mpl) {
				pAccount_ = mpl->getAccount();
				pFolder_ = mpl->getFolder();
				pmh_ = mpl;
				pAccount_->lock();
			}
			else {
				pContext_ = 0;
				bFinish_ = true;
			}
		}
	}
}

qm::MessageContextMessageEnumerator::~MessageContextMessageEnumerator()
{
	if (pAccount_)
		pAccount_->unlock();
}

bool qm::MessageContextMessageEnumerator::next()
{
	if (bFinish_)
		return false;
	bFinish_ = true;
	return true;
}

void qm::MessageContextMessageEnumerator::reset()
{
	bFinish_ = false;
}

size_t qm::MessageContextMessageEnumerator::size() const
{
	return pContext_ ? 1 : 0;
}

Account* qm::MessageContextMessageEnumerator::getAccount() const
{
	return pAccount_;
}

Folder* qm::MessageContextMessageEnumerator::getFolder() const
{
	return pFolder_;
}

MessageHolder* qm::MessageContextMessageEnumerator::getMessageHolder() const
{
	return pmh_;
}

Message* qm::MessageContextMessageEnumerator::getMessage(unsigned int nFlags,
														 const WCHAR* pwszField,
														 unsigned int nSecurityMode,
														 Message* pMessage)
{
	assert(pContext_);
	return pContext_->getMessage(nFlags, pwszField, nSecurityMode);
}

MessagePtr qm::MessageContextMessageEnumerator::getOriginMessagePtr() const
{
	assert(pContext_);
	return pContext_->getOriginMessagePtr();
}

std::auto_ptr<URI> qm::MessageContextMessageEnumerator::getURI(const Message* pMessage,
															   const qs::Part* pPart,
															   URIFragment::Type type) const
{
	assert(pContext_);
	return pContext_->getURI(pPart, type);
}
