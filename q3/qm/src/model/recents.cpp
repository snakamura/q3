/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmaccount.h>
#include <qmmessageholder.h>
#include <qmrecents.h>

#include <qsthread.h>

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
	typedef std::vector<qs::WSTRING> URIList;
	typedef std::vector<RecentsHandler*> HandlerList;
	
	void fireRecentsChanged();
	
	Recents* pThis_;
	AccountManager* pAccountManager_;
	unsigned int nMax_;
	bool bAddAutoOnly_;
	std::auto_ptr<qs::RegexPattern> pFilter_;
	URIList list_;
	CriticalSection cs_;
#ifndef NDEBUG
	unsigned int nLock_;
#endif
	HandlerList listHandler_;
};

void qm::RecentsImpl::fireRecentsChanged()
{
	RecentsEvent event(pThis_);
	for (HandlerList::const_iterator it = listHandler_.begin(); it != listHandler_.end(); ++it)
		(*it)->recentsChanged(event);
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
	wstring_ptr wstrPattern(pProfile->getString(L"Recents", L"Filter", L""));
	if (*wstrPattern.get())
		pFilter = RegexCompiler().compile(wstrPattern.get());
	
	pImpl_ = new RecentsImpl();
	pImpl_->pThis_ = this;
	pImpl_->pAccountManager_ = pAccountManager;
	pImpl_->nMax_ = pProfile->getInt(L"Recents", L"Max", 20);
	pImpl_->bAddAutoOnly_ = pProfile->getInt(L"Recents", L"AddAutoOnly", 1) != 0;
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

unsigned int qm::Recents::getCount() const
{
	assert(isLocked());
	return pImpl_->list_.size();
}

const WCHAR* qm::Recents::get(unsigned int n) const
{
	assert(isLocked());
	assert(n < pImpl_->list_.size());
	return pImpl_->list_[n];
}

void qm::Recents::add(const WCHAR* pwszURI,
					  bool bAuto)
{
	if (!bAuto && pImpl_->bAddAutoOnly_)
		return;
	
	if (pImpl_->pFilter_.get()) {
		const WCHAR* pStart = 0;
		const WCHAR* pEnd = 0;
		pImpl_->pFilter_->search(pwszURI, -1, pwszURI, false, &pStart, &pEnd, 0);
		if (!pStart)
			return;
	}
	
	Lock<Recents> lock(*this);
	
	RecentsImpl::URIList& l = pImpl_->list_;
	
	wstring_ptr wstrURI(allocWString(pwszURI));
	l.push_back(wstrURI.get());
	wstrURI.release();
	
	while (l.size() > pImpl_->nMax_) {
		freeWString(l.front());
		l.erase(l.begin());
	}
	
	pImpl_->fireRecentsChanged();
}

void qm::Recents::remove(const WCHAR* pwszURI)
{
	Lock<Recents> lock(*this);
	
	RecentsImpl::URIList::iterator it = std::find_if(
		pImpl_->list_.begin(), pImpl_->list_.end(),
		std::bind2nd(string_equal<WCHAR>(), pwszURI));
	if (it != pImpl_->list_.end()) {
		freeWString(*it);
		pImpl_->list_.erase(it);
	}
	
	pImpl_->fireRecentsChanged();
}

void qm::Recents::clear()
{
	Lock<Recents> lock(*this);
	
	if (pImpl_->list_.empty())
		return;
	
	std::for_each(pImpl_->list_.begin(), pImpl_->list_.end(), string_free<WSTRING>());
	pImpl_->list_.clear();
	
	pImpl_->fireRecentsChanged();
}

void qm::Recents::removeSeens()
{
	Lock<Recents> lock(*this);
	
	bool bChanged = false;
	
	for (RecentsImpl::URIList::iterator it = pImpl_->list_.begin(); it != pImpl_->list_.end(); ) {
		bool bRemove = true;
		
		std::auto_ptr<URI> pURI(URI::parse(*it));
		if (pURI.get()) {
			MessagePtrLock mpl(pImpl_->pAccountManager_->getMessage(*pURI.get()));
			if (mpl)
				bRemove = mpl->isSeen();
		}
		
		if (bRemove) {
			freeWString(*it);
			it = pImpl_->list_.erase(it);
			bChanged = true;
		}
		else {
			++it;
		}
	}
	
	if (bChanged)
		pImpl_->fireRecentsChanged();
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

qm::RecentsEvent::RecentsEvent(Recents* pRecents) :
	pRecents_(pRecents)
{
}

qm::RecentsEvent::~RecentsEvent()
{
}

Recents* qm::RecentsEvent::getRecents() const
{
	return pRecents_;
}
