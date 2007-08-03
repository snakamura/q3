/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmaccount.h>
#include <qmjunk.h>
#include <qmmessage.h>
#include <qmmessageholder.h>
#include <qmsecurity.h>

#include <qsassert.h>
#include <qsinit.h>
#include <qslog.h>

#include "junk.h"

using namespace qm;
using namespace qs;

namespace qm {
struct JunkFilterFactoryImpl;
}


/****************************************************************************
 *
 * JunkFilter
 *
 */

qm::JunkFilter::~JunkFilter()
{
}

std::auto_ptr<JunkFilter> qm::JunkFilter::getInstance(const WCHAR* pwszPath,
													  Profile* pProfile)
{
	JunkFilterFactory* pFactory = JunkFilterFactory::getFactory();
	if (!pFactory)
		return std::auto_ptr<JunkFilter>();
	return pFactory->createJunkFilter(pwszPath, pProfile);
}


/****************************************************************************
 *
 * JunkFilterFactoryImpl
 *
 */

struct qm::JunkFilterFactoryImpl
{
	static JunkFilterFactory* pFactory__;
};

JunkFilterFactory* qm::JunkFilterFactoryImpl::pFactory__ = 0;


/****************************************************************************
 *
 * JunkFilterFactory
 *
 */

qm::JunkFilterFactory::JunkFilterFactory()
{
}

qm::JunkFilterFactory::~JunkFilterFactory()
{
}

JunkFilterFactory* qm::JunkFilterFactory::getFactory()
{
	return JunkFilterFactoryImpl::pFactory__;
}

void qm::JunkFilterFactory::registerFactory(JunkFilterFactory* pFactory)
{
	assert(!JunkFilterFactoryImpl::pFactory__);
	JunkFilterFactoryImpl::pFactory__ = pFactory;
}

void qm::JunkFilterFactory::unregisterFactory(JunkFilterFactory* pFactory)
{
	assert(JunkFilterFactoryImpl::pFactory__ == pFactory);
	JunkFilterFactoryImpl::pFactory__ = 0;
}


/****************************************************************************
 *
 * JunkFilterUtil
 *
 */

bool qm::JunkFilterUtil::manage(JunkFilter* pJunkFilter,
								MessageHolder* pmh,
								unsigned int nOperation)
{
	Log log(InitThread::getInitThread().getLogger(), L"qm::JunkFilterUtil");
	
	wstring_ptr wstrId(pmh->getMessageId());
	bool b = true;
	JunkFilter::Status status = pJunkFilter->getStatus(wstrId.get());
	switch (status) {
	case JunkFilter::STATUS_NONE:
		break;
	case JunkFilter::STATUS_CLEAN:
		b = nOperation & JunkFilter::OPERATION_REMOVECLEAN ||
			nOperation & JunkFilter::OPERATION_ADDJUNK;
		break;
	case JunkFilter::STATUS_JUNK:
		b = nOperation & JunkFilter::OPERATION_ADDCLEAN ||
			nOperation & JunkFilter::OPERATION_REMOVEJUNK;
		break;
	default:
		assert(false);
		break;
	}
	if (!b) {
		log.debugf(L"Ignoring an message already learned as %s.",
			status == JunkFilter::STATUS_CLEAN ? L"clean" : L"junk");
		return true;
	}
	
	Message msg;
	unsigned int nFlags = pJunkFilter->isScanAttachment() ?
		Account::GETMESSAGEFLAG_ALL : Account::GETMESSAGEFLAG_TEXT;
	if (!pmh->getMessage(nFlags, 0, SECURITYMODE_NONE, &msg)) {
		log.error(L"Could not get a message while managing a junk filter.");
		return false;
	}
	
	return pJunkFilter->manage(msg, nOperation);
}
