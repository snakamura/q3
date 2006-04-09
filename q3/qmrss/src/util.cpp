/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include "util.h"

using namespace qs;
using namespace qm;
using namespace qmrss;


/****************************************************************************
 *
 * DefaultCallback
 *
 */

qmrss::DefaultCallback::DefaultCallback(const WCHAR* pwszHost,
										unsigned int nSslOption,
										const Security* pSecurity) :
	AbstractSSLSocketCallback(pSecurity),
	nSslOption_(nSslOption)
{
	wstrHost_ = allocWString(pwszHost);
}

qmrss::DefaultCallback::~DefaultCallback()
{
}

bool qmrss::DefaultCallback::isCanceled(bool bForce) const
{
	return false;
}

void qmrss::DefaultCallback::initialize()
{
}

void qmrss::DefaultCallback::lookup()
{
}

void qmrss::DefaultCallback::connecting()
{
}

void qmrss::DefaultCallback::connected()
{
}

unsigned int qmrss::DefaultCallback::getOption()
{
	return nSslOption_;
}

const WCHAR* qmrss::DefaultCallback::getHost()
{
	return wstrHost_.get();
}
