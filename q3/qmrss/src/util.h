/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __UTIL_H__
#define __UTIL_H__

#include <qmsecurity.h>
#include <qmsession.h>

#include <qshttp.h>


namespace qmrss {

/****************************************************************************
 *
 * Util
 *
 */

class Util
{
public:
	static std::auto_ptr<qs::Http> createHttp(qm::SubAccount* pSubAccount,
											  qs::SocketCallback* pSocketCallback,
											  qs::SSLSocketCallback* pSSLSocketCallback,
											  qs::HttpCallback* pHttpCallback,
											  qs::Logger* pLogger);
};


/****************************************************************************
 *
 * DefaultCallback
 *
 */

class DefaultCallback :
	public qs::SocketCallback,
	public qm::AbstractSSLSocketCallback,
	public qs::HttpCallback
{
public:
	DefaultCallback(const WCHAR* pwszHost,
					unsigned int nSslOption,
					const qm::Security* pSecurity);
	virtual ~DefaultCallback();

public:
	virtual bool isCanceled(bool bForce) const;
	virtual void initialize();
	virtual void lookup();
	virtual void connecting();
	virtual void connected();

protected:
	virtual unsigned int getOption();
	virtual const WCHAR* getHost();

private:
	DefaultCallback(const DefaultCallback&);
	DefaultCallback& operator=(const DefaultCallback&);

private:
	unsigned int nSslOption_;
	qs::wstring_ptr wstrHost_;
};

}

#endif // __UTIL_H__
