/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __HTTP_H__
#define __HTTP_H__

#include <qslog.h>
#include <qsmime.h>
#include <qssocket.h>
#include <qsssl.h>


namespace qmrss {

class Http;
class HttpCallback;
class HttpConnection;
class HttpMethod;
	class HttpMethodGet;
class HttpURL;
class HttpUtil;


/****************************************************************************
 *
 * Http
 *
 */

class Http
{
public:
	Http(qs::SocketCallback* pSocketCallback,
		 qs::SSLSocketCallback* pSSLSocketCallback,
		 HttpCallback* pHttpCallback,
		 qs::Logger* pLogger);
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
	qs::wstring_ptr wstrProxyHost_;
	unsigned short nProxyPort_;
	qs::wstring_ptr wstrProxyUserName_;
	qs::wstring_ptr wstrProxyPassword_;
	qs::SocketCallback* pSocketCallback_;
	qs::SSLSocketCallback* pSSLSocketCallback_;
	HttpCallback* pHttpCallback_;
	qs::Logger* pLogger_;
};


/****************************************************************************
 *
 * HttpCallback
 *
 */

class HttpCallback
{
public:
	virtual ~HttpCallback();
};


/****************************************************************************
 *
 * HttpConnection
 *
 */

class HttpConnection
{
public:
	HttpConnection(std::auto_ptr<qs::SocketBase> pSocket,
				   bool bProxied);
	~HttpConnection();

public:
	bool isProxied() const;
	bool write(const WCHAR* p,
			   size_t nLen);
	size_t read(unsigned char* p,
				size_t nLen);
	qs::xstring_ptr readLine();
	qs::InputStream* getInputStream();

private:
	bool prepareInputStream();

private:
	HttpConnection(const HttpConnection&);
	HttpConnection& operator=(const HttpConnection&);

private:
	std::auto_ptr<qs::SocketBase> pSocket_;
	bool bProxied_;
	std::auto_ptr<qs::BufferedInputStream> pInputStream_;
};


/****************************************************************************
 *
 * HttpMethod
 *
 */

class HttpMethod
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
	virtual qs::malloc_size_ptr<unsigned char> getResponseBody() const = 0;
	virtual qs::InputStream* getResponseBodyAsStream() const = 0;

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

class AbstractHttpMethod : public HttpMethod
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
	virtual qs::malloc_size_ptr<unsigned char> getResponseBody() const;
	virtual qs::InputStream* getResponseBodyAsStream() const;

public:
	virtual const WCHAR* getHost() const;
	virtual unsigned short getPort() const;
	virtual bool isSecure() const;
	virtual bool isReady() const;
	virtual unsigned int getRetryCount() const;
	virtual unsigned int invoke(std::auto_ptr<HttpConnection> pConnection);

protected:
	virtual const WCHAR* getName() const = 0;
	virtual bool getRequestHeaders(qs::StringBuffer<qs::WSTRING>* pBuf) const;
	virtual size_t getRequestBodyLength() const;
	virtual bool writeRequestBody(HttpConnection* pConnection) const;

private:
	std::pair<const WCHAR*, const WCHAR*> getCredential() const;

private:
	AbstractHttpMethod(const AbstractHttpMethod&);
	AbstractHttpMethod& operator=(const AbstractHttpMethod&);

private:
	typedef std::vector<std::pair<qs::WSTRING, qs::WSTRING> > HeaderList;

private:
	std::auto_ptr<HttpURL> pURL_;
	HeaderList listRequestHeader_;
	qs::wstring_ptr wstrUserName_;
	qs::wstring_ptr wstrPassword_;
	qs::wstring_ptr wstrProxyUserName_;
	qs::wstring_ptr wstrProxyPassword_;
	qs::xstring_ptr strResponseLine_;
	qs::xstring_ptr strResponseHeader_;
	std::auto_ptr<HttpConnection> pConnection_;
};


/****************************************************************************
 *
 * HttpMethodGet
 *
 */

class HttpMethodGet : public AbstractHttpMethod
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

class HttpURL
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
	qs::wstring_ptr getURL() const;
	qs::wstring_ptr getAuthority() const;

public:
	static std::auto_ptr<HttpURL> create(const WCHAR* pwszURL);

private:
	HttpURL(const HttpURL&);
	HttpURL& operator=(const HttpURL&);

private:
	qs::wstring_ptr wstrScheme_;
	qs::wstring_ptr wstrHost_;
	unsigned short nPort_;
	qs::wstring_ptr wstrUser_;
	qs::wstring_ptr wstrPassword_;
	qs::wstring_ptr wstrPath_;
	qs::wstring_ptr wstrQuery_;
};


/****************************************************************************
 *
 * HttpUtil
 *
 */

class HttpUtil
{
public:
	static qs::wstring_ptr getBasicCredential(const WCHAR* pwszUserName,
											  const WCHAR* pwszPassword);
	static unsigned int parseResponse(const char* p);
	static qs::xstring_ptr readLine(qs::InputStream* pInputStream);
	static qs::xstring_ptr readLine(qs::SocketBase* pSocket);
	static bool readByte(qs::SocketBase* pSocket,
						 unsigned char* p);
	static bool write(qs::SocketBase* pSocket,
					  const unsigned char* p,
					  size_t nLen);
	static bool write(qs::SocketBase* pSocket,
					  const WCHAR* p,
					  size_t nLen);
	
	static qs::wstring_ptr getRedirectLocation(const WCHAR* pwszURL,
											   const qs::Part& header,
											   UINT* pnErrorId);
	static qs::wstring_ptr resolveRelativeURL(const WCHAR* pwszURL,
											  const WCHAR* pwszBaseURL);
	
	static bool getInternetProxySetting(qs::wstring_ptr* pwstrProxyHost,
										unsigned short* pnProxyPort);
	static qs::wstring_ptr getInternetCookie(const WCHAR* pwszURL);
	static bool setInternetCookie(const WCHAR* pwszURL,
								  const WCHAR* pwszCookie);
	static void updateInternetCookies(const WCHAR* pwszURL,
									  const qs::Part& header);
};

}

#endif // __HTTP_H__
