/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QSHTTP_H__
#define __QSHTTP_H__

#include <qslog.h>
#include <qsmime.h>
#include <qssocket.h>
#include <qsssl.h>


namespace qs {

class Http;
class HttpCallback;
class HttpConnection;
class HttpMethod;
	class HttpMethodGet;
class HttpURL;
class HttpUtility;


/****************************************************************************
 *
 * Http
 *
 */

#pragma warning(push)
#pragma warning(disable:4251)

class QSEXPORTCLASS Http
{
public:
	Http(SocketCallback* pSocketCallback,
		 SSLSocketCallback* pSSLSocketCallback,
		 HttpCallback* pHttpCallback,
		 Logger* pLogger);
	~Http();

public:
	unsigned int invoke(HttpMethod* pMethod);
	
	unsigned int getTimeout() const;
	void setTimeout(unsigned int nTimeout);
	
	const WCHAR* getProxyHost() const;
	void setProxyHost(const WCHAR* pwszProxyHost);
	unsigned short getProxyPort() const;
	void setProxyPort(unsigned short nProxyPort);
	const WCHAR* getProxyUserName() const;
	void setProxyUserName(const WCHAR* pwszUserName);
	const WCHAR* getProxyPassword() const;
	void setProxyPassword(const WCHAR* pwszPassword);

private:
	Http(const Http&);
	Http& operator=(const Http&);

private:
	unsigned int nTimeout_;
	wstring_ptr wstrProxyHost_;
	unsigned short nProxyPort_;
	wstring_ptr wstrProxyUserName_;
	wstring_ptr wstrProxyPassword_;
	SocketCallback* pSocketCallback_;
	SSLSocketCallback* pSSLSocketCallback_;
	HttpCallback* pHttpCallback_;
	Logger* pLogger_;
};

#pragma warning(pop)


/****************************************************************************
 *
 * HttpCallback
 *
 */

class QSEXPORTCLASS HttpCallback
{
public:
	virtual ~HttpCallback();
};


/****************************************************************************
 *
 * HttpConnection
 *
 */

#pragma warning(push)
#pragma warning(disable:4251)

class QSEXPORTCLASS HttpConnection
{
public:
	HttpConnection(std::auto_ptr<SocketBase> pSocket,
				   bool bProxied);
	~HttpConnection();

public:
	bool isProxied() const;
	bool write(const WCHAR* p,
			   size_t nLen);
	size_t read(unsigned char* p,
				size_t nLen);
	xstring_ptr readLine();
	InputStream* getInputStream();

private:
	bool prepareInputStream();

private:
	HttpConnection(const HttpConnection&);
	HttpConnection& operator=(const HttpConnection&);

private:
	std::auto_ptr<SocketBase> pSocket_;
	bool bProxied_;
	std::auto_ptr<BufferedInputStream> pInputStream_;
};

#pragma warning(pop)


/****************************************************************************
 *
 * HttpMethod
 *
 */

class QSEXPORTCLASS HttpMethod
{
public:
	virtual ~HttpMethod();

public:
	virtual void setRequestHeader(const WCHAR* pwszName,
								  const WCHAR* pwszValue) = 0;
	virtual void setCredential(const WCHAR* pwszUserName,
							   const WCHAR* pwszPassword) = 0;
	virtual void setProxyCredential(const WCHAR* pwszUserName,
									const WCHAR* pwszPassword) = 0;

public:
	virtual const CHAR* getResponseLine() const = 0;
	virtual const CHAR* getResponseHeader() const = 0;
	virtual malloc_size_ptr<unsigned char> getResponseBody() const = 0;
	virtual InputStream* getResponseBodyAsStream() const = 0;

public:
	virtual const WCHAR* getHost() const = 0;
	virtual unsigned short getPort() const = 0;
	virtual bool isSecure() const = 0;
	virtual bool isReady() const = 0;
	virtual unsigned int getRetryCount() const = 0;
	virtual unsigned int invoke(std::auto_ptr<HttpConnection> pConnection) = 0;
};


/****************************************************************************
 *
 * AbstractHttpMethod
 *
 */

#pragma warning(push)
#pragma warning(disable:4251)

class QSEXPORTCLASS AbstractHttpMethod : public HttpMethod
{
public:
	AbstractHttpMethod(const WCHAR* pwszURL);
	virtual ~AbstractHttpMethod();

public:
	virtual void setRequestHeader(const WCHAR* pwszName,
								  const WCHAR* pwszValue);
	virtual void setCredential(const WCHAR* pwszUserName,
							   const WCHAR* pwszPassword);
	virtual void setProxyCredential(const WCHAR* pwszUserName,
									const WCHAR* pwszPassword);

public:
	virtual const CHAR* getResponseLine() const;
	virtual const CHAR* getResponseHeader() const;
	virtual malloc_size_ptr<unsigned char> getResponseBody() const;
	virtual InputStream* getResponseBodyAsStream() const;

public:
	virtual const WCHAR* getHost() const;
	virtual unsigned short getPort() const;
	virtual bool isSecure() const;
	virtual bool isReady() const;
	virtual unsigned int getRetryCount() const;
	virtual unsigned int invoke(std::auto_ptr<HttpConnection> pConnection);

protected:
	virtual const WCHAR* getName() const = 0;
	virtual bool getRequestHeaders(StringBuffer<WSTRING>* pBuf) const;
	virtual size_t getRequestBodyLength() const;
	virtual bool writeRequestBody(HttpConnection* pConnection) const;

private:
	std::pair<const WCHAR*, const WCHAR*> getCredential() const;

private:
	AbstractHttpMethod(const AbstractHttpMethod&);
	AbstractHttpMethod& operator=(const AbstractHttpMethod&);

private:
	typedef std::vector<std::pair<WSTRING, WSTRING> > HeaderList;

private:
	std::auto_ptr<HttpURL> pURL_;
	HeaderList listRequestHeader_;
	wstring_ptr wstrUserName_;
	wstring_ptr wstrPassword_;
	wstring_ptr wstrProxyUserName_;
	wstring_ptr wstrProxyPassword_;
	xstring_ptr strResponseLine_;
	xstring_ptr strResponseHeader_;
	std::auto_ptr<HttpConnection> pConnection_;
};

#pragma warning(pop)


/****************************************************************************
 *
 * HttpMethodGet
 *
 */

class QSEXPORTCLASS HttpMethodGet : public AbstractHttpMethod
{
public:
	HttpMethodGet(const WCHAR* pwszURL);
	virtual ~HttpMethodGet();

protected:
	virtual const WCHAR* getName() const;

private:
	HttpMethodGet(const HttpMethodGet&);
	HttpMethodGet& operator=(const HttpMethodGet&);
};


/****************************************************************************
 *
 * HttpURL
 *
 */

#pragma warning(push)
#pragma warning(disable:4251)

class QSEXPORTCLASS HttpURL
{
public:
	HttpURL(const WCHAR* pwszScheme,
			const WCHAR* pwszHost,
			unsigned short nPort,
			const WCHAR* pwszUser,
			const WCHAR* pwszPassword,
			const WCHAR* pwszPath,
			const WCHAR* pwszQuery);
	~HttpURL();

public:
	const WCHAR* getScheme() const;
	const WCHAR* getHost() const;
	unsigned short getPort() const;
	const WCHAR* getUser() const;
	const WCHAR* getPassword() const;
	const WCHAR* getPath() const;
	const WCHAR* getQuery() const;
	wstring_ptr getURL() const;
	wstring_ptr getAuthority() const;

public:
	static std::auto_ptr<HttpURL> create(const WCHAR* pwszURL);

private:
	HttpURL(const HttpURL&);
	HttpURL& operator=(const HttpURL&);

private:
	wstring_ptr wstrScheme_;
	wstring_ptr wstrHost_;
	unsigned short nPort_;
	wstring_ptr wstrUser_;
	wstring_ptr wstrPassword_;
	wstring_ptr wstrPath_;
	wstring_ptr wstrQuery_;
};

#pragma warning(pop)


/****************************************************************************
 *
 * HttpUtility
 *
 */

class QSEXPORTCLASS HttpUtility
{
public:
	enum RedirectError {
		REDIRECTERROR_SUCCESS,
		REDIRECTERROR_PARSELOCATION,
		REDIRECTERROR_INVALIDLOCATION
	};

public:
	static wstring_ptr getRedirectLocation(const WCHAR* pwszURL,
										   const Part& header,
										   RedirectError* pError);
	static wstring_ptr resolveRelativeURL(const WCHAR* pwszURL,
										  const WCHAR* pwszBaseURL);
	
	static bool getInternetProxySetting(wstring_ptr* pwstrProxyHost,
										unsigned short* pnProxyPort);
	static wstring_ptr getInternetCookie(const WCHAR* pwszURL);
	static bool setInternetCookie(const WCHAR* pwszURL,
								  const WCHAR* pwszCookie);
	static void updateInternetCookies(const WCHAR* pwszURL,
									  const Part& header);
};

}

#endif // __QSHTTP_H__
