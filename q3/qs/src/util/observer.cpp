/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsstl.h>
#include <qsutil.h>

#include <vector>
#include <algorithm>

using namespace qs;


/****************************************************************************
 *
 * Observer
 *
 */

qs::Observer::~Observer()
{
}


/****************************************************************************
 *
 * ObservableImpl
 *
 */

struct qs::ObservableImpl
{
	typedef std::vector<Observer*> ObserverList;
	
	ObserverList listObserver_;
};


/****************************************************************************
 *
 * Observable
 *
 */

qs::Observable::Observable() :
	pImpl_(0)
{
	pImpl_ = new ObservableImpl();
}

qs::Observable::~Observable()
{
	delete pImpl_;
	pImpl_ = 0;
}

void qs::Observable::addObserver(Observer* pObserver)
{
	assert(std::find(pImpl_->listObserver_.begin(),
		pImpl_->listObserver_.end(), pObserver) ==
		pImpl_->listObserver_.end());
	pImpl_->listObserver_.push_back(pObserver);
}

void qs::Observable::removeObserver(Observer* pObserver)
{
	ObservableImpl::ObserverList::iterator it = std::remove(
		pImpl_->listObserver_.begin(), pImpl_->listObserver_.end(), pObserver);
	assert(it != pImpl_->listObserver_.end());
	pImpl_->listObserver_.erase(it , pImpl_->listObserver_.end());
}

void qs::Observable::removeObservers()
{
	pImpl_->listObserver_.clear();
}

void qs::Observable::notifyObservers(void* pParam)
{
	notifyObservers(pParam, 0);
}

void qs::Observable::notifyObservers(void* pParam,
									 const Observer* pObserver)
{
	ObservableImpl::ObserverList::iterator it = pImpl_->listObserver_.begin();
	while (it != pImpl_->listObserver_.end()) {
		if (*it != pObserver)
			(*it)->onUpdate(this, pParam);
		++it;
	}
}
