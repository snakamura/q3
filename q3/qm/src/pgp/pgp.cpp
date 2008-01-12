/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#include <qmpgp.h>

#include <qsassert.h>

#include "pgp.h"

using namespace qm;
using namespace qs;

namespace qm {
struct PGPFactoryImpl;
}


/****************************************************************************
 *
 * PGPUtility
 *
 */

qm::PGPUtility::~PGPUtility()
{
}

std::auto_ptr<PGPUtility> qm::PGPUtility::getInstance(Profile* pProfile)
{
	return PGPFactory::getFactory()->createPGPUtility(pProfile);
}


/****************************************************************************
 *
 * PGPFactoryImpl
 *
 */

struct qm::PGPFactoryImpl
{
	static PGPFactory* pFactory__;
};

PGPFactory* qm::PGPFactoryImpl::pFactory__ = 0;


/****************************************************************************
 *
 * PGPFactory
 *
 */

qm::PGPFactory::PGPFactory()
{
}

qm::PGPFactory::~PGPFactory()
{
}

PGPFactory* qm::PGPFactory::getFactory()
{
	return PGPFactoryImpl::pFactory__;
}

void qm::PGPFactory::registerFactory(PGPFactory* pFactory)
{
	assert(!PGPFactoryImpl::pFactory__);
	PGPFactoryImpl::pFactory__ = pFactory;
}

void qm::PGPFactory::unregisterFactory(PGPFactory* pFactory)
{
	assert(PGPFactoryImpl::pFactory__ == pFactory);
	PGPFactoryImpl::pFactory__ = 0;
}


/****************************************************************************
 *
 * PGPPassphraseCallback
 *
 */

qm::PGPPassphraseCallback::~PGPPassphraseCallback()
{
}




/****************************************************************************
 *
 * PGPPassphraseCallbackImpl
 *
 */

qm::PGPPassphraseCallbackImpl::PGPPassphraseCallbackImpl(PasswordManager* pPasswordManager,
														 const WCHAR* pwszDefaultUserId) :
	pPasswordManager_(pPasswordManager),
	pwszDefaultUserId_(pwszDefaultUserId),
	state_(PASSWORDSTATE_ONETIME)
{
}

qm::PGPPassphraseCallbackImpl::~PGPPassphraseCallbackImpl()
{
}

void qm::PGPPassphraseCallbackImpl::save()
{
	if (wstrPassword_.get() &&
		(state_ == PASSWORDSTATE_SESSION || state_ == PASSWORDSTATE_SAVE)) {
		PGPPasswordCondition condition(wstrUserId_.get());
		pPasswordManager_->setPassword(condition,
			wstrPassword_.get(), state_ == PASSWORDSTATE_SAVE);
	}
}

wstring_ptr qm::PGPPassphraseCallbackImpl::getPassphrase(const WCHAR* pwszUserId)
{
	if (!pwszUserId)
		pwszUserId = pwszDefaultUserId_;
	if (!pwszUserId)
		return 0;
	
	wstring_ptr wstrPassword = pPasswordManager_->getPassword(
		PGPPasswordCondition(pwszUserId), false, &state_);
	if (!wstrPassword.get())
		return 0;
	
	wstrUserId_ = allocWString(pwszUserId);
	wstrPassword_ = allocWString(wstrPassword.get());
	
	return wstrPassword;
}

void qm::PGPPassphraseCallbackImpl::clear()
{
	wstrUserId_.reset(0);
	wstrPassword_.reset(0);
	state_ = PASSWORDSTATE_ONETIME;
}
