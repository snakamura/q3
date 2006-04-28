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
 * Util
 *
 */

std::auto_ptr<Http> qmrss::Util::createHttp(SubAccount* pSubAccount,
											SocketCallback* pSocketCallback,
											SSLSocketCallback* pSSLSocketCallback,
											HttpCallback* pHttpCallback,
											Logger* pLogger)
{
	Log log(pLogger, L"qmrss::Util");
	
	bool bUseProxy = false;
	wstring_ptr wstrProxyHost;
	unsigned short nProxyPort = 8080;
	wstring_ptr wstrProxyUserName;
	wstring_ptr wstrProxyPassword;
	if (pSubAccount->getProperty(L"Http", L"UseInternetSetting", 0)) {
		bUseProxy = HttpUtil::getInternetProxySetting(&wstrProxyHost, &nProxyPort);
	}
	else if (pSubAccount->getProperty(L"Http", L"UseProxy", 0)) {
		wstrProxyHost = pSubAccount->getProperty(L"Http", L"ProxyHost", L"");
		nProxyPort = pSubAccount->getProperty(L"Http", L"ProxyPort", 8080);
		wstrProxyUserName = pSubAccount->getProperty(L"Http", L"ProxyUserName", L"");
		wstrProxyPassword = pSubAccount->getProperty(L"Http", L"ProxyPassword", L"");
		bUseProxy = true;
	}
	if (bUseProxy && log.isDebugEnabled())
		log.debugf(L"Using proxy: %s:%u", wstrProxyHost.get(), nProxyPort);
	
	std::auto_ptr<Http> pHttp(new Http(pSocketCallback,
		pSSLSocketCallback, pHttpCallback, pLogger));
	pHttp->setTimeout(pSubAccount->getTimeout());
	if (bUseProxy) {
		pHttp->setProxyHost(wstrProxyHost.get());
		pHttp->setProxyPort(nProxyPort);
		pHttp->setProxyUserName(wstrProxyUserName.get());
		pHttp->setProxyPassword(wstrProxyPassword.get());
	}
	return pHttp;
}


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
