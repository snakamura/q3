/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmaccount.h>
#include <qmmessageholder.h>
#include <qmrecents.h>

#include <qsthread.h>

#include <boost/bind.hpp>

#include "uri.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * RecentsImpl
 *
 */

struct qm::RecentsImpl
{
	typedef std::vector<std::pair<URI*, Time> > URIList;
	typedef std::vector<RecentsHandler*> HandlerList;
	
	void fireRecentsChanged(RecentsEvent::Type type);
	
	Recents* pThis_;
	AccountManager* pAccountManager_;
	Profile* pProfile_;
	unsigned int nMax_;
	std::auto_ptr<qs::RegexPattern> pFilter_;
	URIList list_;
	CriticalSection cs_;
#ifndef NDEBUG
	unsigned int nLock_;
#endif
	HandlerList listHandler_;
};

void qm::RecentsImpl::fireRecentsChanged(RecentsEvent::Type type)
{
	RecentsEvent event(pThis_, type);
	std::for_each(listHandler_.begin(), listHandler_.end(),
		boost::bind(&RecentsHandler::recentsChanged, _1, boost::cref(event)));
}


/****************************************************************************
 *
 * Recents
 *
 */

qm::Recents::Recents(AccountManager* pAccountManager,
					 Profile* pProfile) :
	pImpl_(0)
{
	std::auto_ptr<RegexPattern> pFilter;
	wstring_ptr wstrPattern(pProfile->getString(L"Recents", L"Filter"));
	if (*wstrPattern.get())
		pFilter = RegexCompiler().compile(wstrPattern.get());
	
	pImpl_ = new RecentsImpl();
	pImpl_->pThis_ = this;
	pImpl_->pAccountManager_ = pAccountManager;
	pImpl_->pProfile_ = pProfile;
	pImpl_->nMax_ = pProfile->getInt(L"Recents", L"Max");
	pImpl_->pFilter_ = pFilter;
#ifndef NDEBUG
	pImpl_->nLock_ = 0;
#endif
}

qm::Recents::~Recents()
{
	if (pImpl_) {
		clear();
		delete pImpl_;
	}
}

unsigned int qm::Recents::getMax() const
{
	Lock<Recents> lock(*this);
	return pImpl_->nMax_;
}

void qm::Recents::setMax(unsigned int nMax)
{
	Lock<Recents> lock(*this);
	pImpl_->nMax_ = nMax;
}

unsigned int qm::Recents::getCount() const
{
	assert(isLocked());
	return static_cast<unsigned int>(pImpl_->list_.size());
}

const std::pair<URI*, qs::Time>& qm::Recents::get(unsigned int n) const
{
	assert(isLocked());
	assert(n < pImpl_->list_.size());
	return pImpl_->list_[n];
}

void qm::Recents::add(std::auto_ptr<URI> pURI)
{
	assert(pURI.get());
	
	if (pImpl_->nMax_ == 0)
		return;
	
	if (pImpl_->pFilter_.get()) {
		wstring_ptr wstrURI(pURI->toString());
		const WCHAR* pStart = 0;
		const WCHAR* pEnd = 0;
		pImpl_->pFilter_->search(wstrURI.get(), -1, wstrURI.get(), false, &pStart, &pEnd, 0);
		if (!pStart)
			return;
	}
	
	Lock<Recents> lock(*this);
	
	RecentsImpl::URIList& l = pImpl_->list_;
	
	l.push_back(std::make_pair(pURI.get(), Time::getCurrentTime()));
	pURI.release();
	
	while (l.size() > pImpl_->nMax_) {
		delete l.front().first;
		l.erase(l.begin());
	}
	
	pImpl_->fireRecentsChanged(RecentsEvent::TYPE_ADDED);
}

void qm::Recents::remove(const URI* pURI)
{
	assert(pURI);
	
	Lock<Recents> lock(*this);
	
	RecentsImpl::URIList::iterator it = pImpl_->list_.begin();
	while (it != pImpl_->list_.end() && *(*it).first != *pURI)
		++it;
	if (it != pImpl_->list_.end()) {
		delete (*it).first;
		pImpl_->list_.erase(it);
	}
	
	pImpl_->fireRecentsChanged(RecentsEvent::TYPE_REMOVED);
}

void qm::Recents::clear()
{
	Lock<Recents> lock(*this);
	
	if (pImpl_->list_.empty())
		return;
	
	std::for_each(pImpl_->list_.begin(), pImpl_->list_.end(),
		unary_compose_f_gx(
			qs::deleter<URI>(),
			std::select1st<RecentsImpl::URIList::value_type>()));
	pImpl_->list_.clear();
	
	pImpl_->fireRecentsChanged(RecentsEvent::TYPE_REMOVED);
}

void qm::Recents::removeSeens()
{
	Lock<Recents> lock(*this);
	
	bool bChanged = false;
	
	for (RecentsImpl::URIList::iterator it = pImpl_->list_.begin(); it != pImpl_->list_.end(); ) {
		MessagePtrLock mpl(pImpl_->pAccountManager_->getMessage(*(*it).first));
		if (!mpl || mpl->isSeen()) {
			delete (*it).first;
			it = pImpl_->list_.erase(it);
			bChanged = true;
		}
		else {
			++it;
		}
	}
	
	if (bChanged)
		pImpl_->fireRecentsChanged(RecentsEvent::TYPE_REMOVED);
}

void qm::Recents::save() const
{
	pImpl_->pProfile_->setInt(L"Recents", L"Max", pImpl_->nMax_);
}

void qm::Recents::lock() const
{
	pImpl_->cs_.lock();
#ifndef NDEBUG
	++pImpl_->nLock_;
#endif
}

void qm::Recents::unlock() const
{
#ifndef NDEBUG
	--pImpl_->nLock_;
#endif
	pImpl_->cs_.unlock();
}

#ifndef NDEBUG
bool qm::Recents::isLocked() const
{
	return pImpl_->nLock_ != 0;
}
#endif

void qm::Recents::addRecentsHandler(RecentsHandler* pHandler)
{
	pImpl_->listHandler_.push_back(pHandler);
}

void qm::Recents::removeRecentsHandler(RecentsHandler* pHandler)
{
	RecentsImpl::HandlerList::iterator it = std::remove(
		pImpl_->listHandler_.begin(), pImpl_->listHandler_.end(), pHandler);
	pImpl_->listHandler_.erase(it, pImpl_->listHandler_.end());
}


/****************************************************************************
 *
 * RecentsHandler
 *
 */

qm::RecentsHandler::~RecentsHandler()
{
}


/****************************************************************************
 *
 * RecentsEvent
 *
 */

qm::RecentsEvent::RecentsEvent(Recents* pRecents,
							   Type type) :
	pRecents_(pRecents),
	type_(type)
{
}

qm::RecentsEvent::~RecentsEvent()
{
}

Recents* qm::RecentsEvent::getRecents() const
{
	return pRecents_;
}

RecentsEvent::Type qm::RecentsEvent::getType() const
{
	return type_;
}
