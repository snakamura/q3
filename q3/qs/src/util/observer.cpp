/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsutil.h>
#include <qsstl.h>
#include <qserror.h>
#include <qsnew.h>

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

qs::Observable::Observable(QSTATUS* pstatus) :
	pImpl_(0)
{
	assert(pstatus);
	
	*pstatus = newObject(&pImpl_);
}

qs::Observable::~Observable()
{
	delete pImpl_;
	pImpl_ = 0;
}

QSTATUS qs::Observable::addObserver(Observer* pObserver)
{
	assert(std::find(pImpl_->listObserver_.begin(),
		pImpl_->listObserver_.end(), pObserver) ==
		pImpl_->listObserver_.end());
	return STLWrapper<ObservableImpl::ObserverList>
		(pImpl_->listObserver_).push_back(pObserver);
}

QSTATUS qs::Observable::removeObserver(Observer* pObserver)
{
	ObservableImpl::ObserverList::iterator it = std::remove(
		pImpl_->listObserver_.begin(), pImpl_->listObserver_.end(), pObserver);
	assert(it != pImpl_->listObserver_.end());
	pImpl_->listObserver_.erase(it , pImpl_->listObserver_.end());
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::Observable::removeObservers()
{
	pImpl_->listObserver_.clear();
	return QSTATUS_SUCCESS;
}

QSTATUS qs::Observable::notifyObservers(void* pParam)
{
	return notifyObservers(pParam, 0);
}

QSTATUS qs::Observable::notifyObservers(void* pParam, const Observer* pObserver)
{
	DECLARE_QSTATUS();
	
	ObservableImpl::ObserverList::iterator it = pImpl_->listObserver_.begin();
	while (it != pImpl_->listObserver_.end() && status == QSTATUS_SUCCESS) {
		if (*it != pObserver)
			status = (*it)->onUpdate(this, pParam);
		++it;
	}
	return status;
}
