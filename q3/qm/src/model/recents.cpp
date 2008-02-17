/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#include <qmaccount.h>
#include <qmmacro.h>
#include <qmmessageholder.h>
#include <qmrecents.h>

#include <qsthread.h>

#include <boost/bind.hpp>

#include "messagecontext.h"
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
	typedef std::vector<std::pair<MessageHolderURI*, Time> > URIList;
	typedef std::vector<RecentsHandler*> HandlerList;
	
	void fireRecentsChanged(RecentsEvent::Type type);
	
	Recents* pThis_;
	const URIResolver* pURIResolver_;
	Profile* pProfile_;
	unsigned int nMax_;
	std::auto_ptr<Macro> pFilter_;
	std::auto_ptr<qs::RegexPattern> pURLFilter_;
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

qm::Recents::Recents(const URIResolver* pURIResolver,
					 Profile* pProfile) :
	pImpl_(0)
{
	std::auto_ptr<Macro> pFilter;
	wstring_ptr wstrFilter(pProfile->getString(L"Recents", L"MacroFilter"));
	if (*wstrFilter.get())
		pFilter = MacroParser().parse(wstrFilter.get());
	
	std::auto_ptr<RegexPattern> pURLFilter;
	wstring_ptr wstrPattern(pProfile->getString(L"Recents", L"Filter"));
	if (*wstrPattern.get())
		pURLFilter = RegexCompiler().compile(wstrPattern.get());
	
	pImpl_ = new RecentsImpl();
	pImpl_->pThis_ = this;
	pImpl_->pURIResolver_ = pURIResolver;
	pImpl_->pProfile_ = pProfile;
	pImpl_->nMax_ = pProfile->getInt(L"Recents", L"Max");
	pImpl_->pFilter_ = pFilter;
	pImpl_->pURLFilter_ = pURLFilter;
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
	return pImpl_->nMax_;
}

void qm::Recents::setMax(unsigned int nMax)
{
	pImpl_->nMax_ = nMax;
}

const Macro* qm::Recents::getFilter() const
{
	return pImpl_->pFilter_.get();
}

void qm::Recents::setFilter(std::auto_ptr<Macro> pFilter)
{
	pImpl_->pFilter_ = pFilter;
}

unsigned int qm::Recents::getCount() const
{
	assert(isLocked());
	return static_cast<unsigned int>(pImpl_->list_.size());
}

const std::pair<MessageHolderURI*, qs::Time>& qm::Recents::get(unsigned int n) const
{
	assert(isLocked());
	assert(n < pImpl_->list_.size());
	return pImpl_->list_[n];
}

void qm::Recents::add(std::auto_ptr<MessageHolderURI> pURI)
{
	assert(pURI.get());
	
	if (pImpl_->nMax_ == 0)
		return;
	
	if (pImpl_->pURLFilter_.get()) {
		wstring_ptr wstrURI(pURI->toString());
		const WCHAR* pStart = 0;
		const WCHAR* pEnd = 0;
		pImpl_->pURLFilter_->search(wstrURI.get(), -1, wstrURI.get(), false, &pStart, &pEnd, 0);
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

void qm::Recents::remove(const MessageHolderURI* pURI)
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
		boost::bind(boost::checked_deleter<MessageHolderURI>(),
			boost::bind(&RecentsImpl::URIList::value_type::first, _1)));
	pImpl_->list_.clear();
	
	pImpl_->fireRecentsChanged(RecentsEvent::TYPE_REMOVED);
}

void qm::Recents::removeSeens()
{
	Lock<Recents> lock(*this);
	
	bool bChanged = false;
	
	for (RecentsImpl::URIList::iterator it = pImpl_->list_.begin(); it != pImpl_->list_.end(); ) {
		const MessageHolderURI* pURI = (*it).first;
		MessagePtrLock mpl(pURI->resolveMessagePtr(pImpl_->pURIResolver_));
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
	
	wstring_ptr wstrFilter;
	if (pImpl_->pFilter_.get())
		wstrFilter = pImpl_->pFilter_->getString();
	else
		wstrFilter = allocWString(L"");
	pImpl_->pProfile_->setString(L"Recents", L"MacroFilter", wstrFilter.get());
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
