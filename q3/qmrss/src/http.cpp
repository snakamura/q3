/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsconv.h>

#include "http.h"

using namespace qmrss;
using namespace qs;


/****************************************************************************
 *
 * Http
 *
 */

qmrss::Http::Http(unsigned int nTimeout,
				  const WCHAR* pwszProxyHost,
				  unsigned short nProxyPort,
				  SocketCallback* pSocketCallback,
				  SSLSocketCallback* pSSLSocketCallback,
				  HttpCallback* pHttpCallback,
				  Logger* pLogger) :
	nTimeout_(nTimeout),
	nProxyPort_(nProxyPort),
	pSocketCallback_(pSocketCallback),
	pSSLSocketCallback_(pSSLSocketCallback),
	pHttpCallback_(pHttpCallback),
	pLogger_(pLogger)
{
	if (pwszProxyHost)
		wstrProxyHost_ = allocWString(pwszProxyHost);
}

qmrss::Http::~Http()
{
}

unsigned int qmrss::Http::invoke(HttpMethod* pMethod)
{
	assert(pMethod);
	
	if (!pMethod->isReady())
		return -1;
	
	std::auto_ptr<Socket> pSocket(new Socket(
		nTimeout_, pSocketCallback_, pLogger_));
	std::auto_ptr<SocketBase> pSocketBase;
	
	bool bProxied = wstrProxyHost_.get() != 0;
	if (bProxied) {
		if (!pSocket->connect(wstrProxyHost_.get(), nProxyPort_))
			return -1;
		pSocketBase = pSocket;
	}
	else {
		if (!pSocket->connect(pMethod->getHost(), pMethod->getPort()))
			return -1;
		
		if (pMethod->isSsl()) {
			SSLSocketFactory* pFactory = SSLSocketFactory::getFactory();
			if (!pFactory)
				return -1;
			
			std::auto_ptr<SSLSocket> pSSLSocket = pFactory->createSSLSocket(
				pSocket.get(), true, pSSLSocketCallback_, pLogger_);
			if (!pSSLSocket.get())
				return -1;
			
			pSocket.release();
			pSocketBase = pSSLSocket;
		}
		else {
			pSocketBase = pSocket;
		}
	}
	
	std::auto_ptr<HttpConnection> pConnection(
		new HttpConnection(pSocketBase, bProxied));
	return pMethod->invoke(pConnection);
}


/****************************************************************************
 *
 * HttpCallback
 *
 */

qmrss::HttpCallback::~HttpCallback()
{
}


/****************************************************************************
 *
 * HttpConnection
 *
 */

qmrss::HttpConnection::HttpConnection(std::auto_ptr<SocketBase> pSocket,
									  bool bProxied) :
	pSocket_(pSocket),
	bProxied_(bProxied)
{
}

qmrss::HttpConnection::~HttpConnection()
{
}

bool qmrss::HttpConnection::isProxied() const
{
	return bProxied_;
}

bool qmrss::HttpConnection::write(const unsigned char* p,
								  size_t nLen)
{
	while (nLen != 0) {
		int nSelect = pSocket_->select(SocketBase::SELECT_WRITE | SocketBase::SELECT_READ);
		if (nSelect == -1)
			return false;
		else if (nSelect == 0)
			return false;
		int n = pSocket_->send(reinterpret_cast<const char*>(p), nLen, 0);
		if (n == -1)
			return false;
		nLen -= n;
		p += n;
	}
	return true;
}

bool qmrss::HttpConnection::write(const WCHAR* p)
{
	return write(p, wcslen(p));
}

bool qmrss::HttpConnection::write(const WCHAR* p,
								  size_t nLen)
{
	string_ptr str(wcs2mbs(p, nLen));
	return write(reinterpret_cast<const unsigned char*>(str.get()), strlen(str.get()));
}

bool qmrss::HttpConnection::writeLine()
{
	return write(L"\r\n", 2);
}

bool qmrss::HttpConnection::writeLine(const WCHAR* p)
{
	return write(p, wcslen(p)) && writeLine();
}

bool qmrss::HttpConnection::writeLine(const WCHAR* p,
									  size_t nLen)
{
	return write(p, nLen) && writeLine();
}

size_t qmrss::HttpConnection::read(unsigned char* p,
								   size_t nLen)
{
	if (!prepareInputStream())
		return -1;
	
	return pInputStream_->read(p, nLen);
}

xstring_ptr qmrss::HttpConnection::readLine()
{
	if (!prepareInputStream())
		return 0;
	
	bool bCr = false;
	XStringBuffer<XSTRING> buf;
	while (true) {
		unsigned char c = 0;
		size_t n = pInputStream_->read(&c, 1);
		if (n == -1)
			return 0;
		else if (n == 0)
			break;
		
		if (!buf.append(c))
			return 0;
		
		if (bCr) {
			if (c == '\n') {
				buf.remove(buf.getLength() - 2, buf.getLength());
				break;
			}
			else {
				bCr = false;
			}
		}
		else {
			bCr = c == '\r';
		}
	}
	return buf.getXString();
}

InputStream* qmrss::HttpConnection::getInputStream()
{
	if (!prepareInputStream())
		return 0;
	return pInputStream_.get();
}

bool qmrss::HttpConnection::prepareInputStream()
{
	if (pInputStream_.get())
		return true;
	
	pInputStream_.reset(new BufferedInputStream(pSocket_->getInputStream(), false));
	
	return true;
}


/****************************************************************************
 *
 * HttpMethod
 *
 */

qmrss::HttpMethod::~HttpMethod()
{
}


/****************************************************************************
 *
 * AbstractHttpMethod
 *
 */

qmrss::AbstractHttpMethod::AbstractHttpMethod(const WCHAR* pwszURL)
{
	pURL_ = HttpURL::create(pwszURL);
}

qmrss::AbstractHttpMethod::~AbstractHttpMethod()
{
	std::for_each(listRequestHeader_.begin(), listRequestHeader_.end(),
		unary_compose_fx_gx(
			string_free<WSTRING>(),
			string_free<WSTRING>()));
}

void qmrss::AbstractHttpMethod::setRequestHeader(const WCHAR* pwszName,
												 const WCHAR* pwszValue)
{
	wstring_ptr wstrName(allocWString(pwszName));
	wstring_ptr wstrValue(allocWString(pwszValue));
	listRequestHeader_.push_back(std::make_pair(wstrName.get(), wstrValue.get()));
	wstrName.release();
	wstrValue.release();
}

const CHAR* qmrss::AbstractHttpMethod::getResponseHeader() const
{
	return strResponseHeader_.get();
}

malloc_size_ptr<unsigned char> qmrss::AbstractHttpMethod::getResponseBody() const
{
	InputStream* pStream = getResponseBodyAsStream();
	
	malloc_ptr<unsigned char> p;
	size_t nLen = 0;
	while (true) {
		p.reset(static_cast<unsigned char*>(realloc(p.release(), nLen + 4096)));
		if (!p.get())
			return malloc_size_ptr<unsigned char>();
		
		size_t nRead = pStream->read(p.get() + nLen, 4096);
		if (nRead == -1)
			return malloc_size_ptr<unsigned char>();
		else if (nRead == 0)
			break;
		
		nLen += nRead;
	}
	
	return malloc_size_ptr<unsigned char>(p, nLen);
}

InputStream* qmrss::AbstractHttpMethod::getResponseBodyAsStream() const
{
	return pConnection_->getInputStream();
}

const WCHAR* qmrss::AbstractHttpMethod::getHost() const
{
	return pURL_->getHost();
}

unsigned short qmrss::AbstractHttpMethod::getPort() const
{
	unsigned short nPort = pURL_->getPort();
	return nPort != static_cast<unsigned short>(-1) ? nPort : isSsl() ? 443 : 80;
}

bool qmrss::AbstractHttpMethod::isSsl() const
{
	return wcscmp(pURL_->getScheme(), L"https") == 0;
}

bool qmrss::AbstractHttpMethod::isReady() const
{
	return pURL_.get() != 0;
}

unsigned int qmrss::AbstractHttpMethod::invoke(std::auto_ptr<HttpConnection> pConnection)
{
	assert(pConnection.get());
	assert(pURL_.get());
	
	const WCHAR* pwszName = getName();
	
	StringBuffer<WSTRING> buf;
	if (pConnection->isProxied()) {
		buf.append(pURL_->getScheme());
		buf.append(L"://");
		buf.append(pURL_->getHost());
	}
	buf.append(pURL_->getPath());
	if (pURL_->getQuery()) {
		buf.append(L'?');
		buf.append(pURL_->getQuery());
	}
	
	if (!pConnection->write(pwszName) ||
		!pConnection->write(L" ") ||
		!pConnection->write(buf.getCharArray(), buf.getLength()) ||
		!pConnection->writeLine(L" HTTP/1.0"))
		return -1;
	
	if (!pConnection->write(L"Host: ") ||
		!pConnection->writeLine(pURL_->getHost()))
		return -1;
	
	if (!pConnection->writeLine(L"Connection: close"))
		return -1;
	
	// TODO
	// Send authenticate header.
	
	for (HeaderList::const_iterator it = listRequestHeader_.begin(); it != listRequestHeader_.end(); ++it) {
		if (!pConnection->write((*it).first) ||
			!pConnection->write(L": ") ||
			!pConnection->writeLine((*it).second))
			return -1;
	}
	
	if (!writeRequestHeaders(pConnection.get()))
		return -1;
	
	size_t nBodyLen = getRequestBodyLength();
	if (nBodyLen != 0) {
		WCHAR wszLen[32];
		swprintf(wszLen, L"%u", nBodyLen);
		if (!pConnection->write(L"Content-Length: ") ||
			!pConnection->writeLine(wszLen))
			return -1;
	}
	
	if (!pConnection->writeLine())
		return -1;
	
	if (nBodyLen != 0) {
		if (!writeRequestBody(pConnection.get()))
			return -1;
	}
	
	xstring_ptr strResponse(pConnection->readLine());
	if (!strResponse.get())
		return -1;
	unsigned int nStatus = parseResponse(strResponse.get());
	if (nStatus == -1)
		return -1;
	
	XStringBuffer<XSTRING> header;
	while (true) {
		xstring_ptr str(pConnection->readLine());
		if (!str.get())
			return -1;
		if (!*str.get())
			break;
		if (!header.append(str.get()) ||
			!header.append("\r\n"))
			return -1;
	}
	strResponseHeader_ = header.getXString();
	
	pConnection_ = pConnection;
	
	return nStatus;
}

bool qmrss::AbstractHttpMethod::writeRequestHeaders(HttpConnection* pConnection) const
{
	return true;
}

size_t qmrss::AbstractHttpMethod::getRequestBodyLength() const
{
	return 0;
}

bool qmrss::AbstractHttpMethod::writeRequestBody(HttpConnection* pConnection) const
{
	return true;
}

unsigned int qmrss::AbstractHttpMethod::parseResponse(const char* p) const
{
	if (strncmp(p, "HTTP/1.0 ", 9) != 0 &&
		strncmp(p, "HTTP/1.1 ", 9) != 0)
		return -1;
	
	char szStatus[4];
	strncpy(szStatus, p + 9, 3);
	szStatus[3] = '\0';
	
	char* pEnd = 0;
	long nStatus = strtol(szStatus, &pEnd, 10);
	if (*pEnd || nStatus < 0 || 600 < nStatus)
		return -1;
	
	return static_cast<unsigned int>(nStatus);
}


/****************************************************************************
 *
 * HttpMethodGet
 *
 */

qmrss::HttpMethodGet::HttpMethodGet(const WCHAR* pwszURL) :
	AbstractHttpMethod(pwszURL)
{
}

qmrss::HttpMethodGet::~HttpMethodGet()
{
}

const WCHAR* qmrss::HttpMethodGet::getName() const
{
	return L"GET";
}


/****************************************************************************
 *
 * HttpURL
 *
 */

qmrss::HttpURL::HttpURL(const WCHAR* pwszScheme,
						const WCHAR* pwszHost,
						unsigned short nPort,
						const WCHAR* pwszUser,
						const WCHAR* pwszPassword,
						const WCHAR* pwszPath,
						const WCHAR* pwszQuery) :
	nPort_(nPort)
{
	assert(pwszScheme);
	assert(pwszHost);
	assert(pwszPath);
	
	wstrScheme_ = allocWString(pwszScheme);
	wstrHost_ = allocWString(pwszHost);
	if (pwszUser)
		wstrUser_ = allocWString(pwszUser);
	if (pwszPassword)
		wstrPassword_ = allocWString(pwszPassword);
	wstrPath_ = allocWString(pwszPath);
	if (pwszQuery)
		wstrQuery_ = allocWString(pwszQuery);
}

qmrss::HttpURL::~HttpURL()
{
}

const WCHAR* qmrss::HttpURL::getScheme() const
{
	return wstrScheme_.get();
}

const WCHAR* qmrss::HttpURL::getHost() const
{
	return wstrHost_.get();
}

unsigned short qmrss::HttpURL::getPort() const
{
	return nPort_;
}

const WCHAR* qmrss::HttpURL::getUser() const
{
	return wstrUser_.get();
}

const WCHAR* qmrss::HttpURL::getPassword() const
{
	return wstrPassword_.get();
}

const WCHAR* qmrss::HttpURL::getPath() const
{
	return wstrPath_.get();
}

const WCHAR* qmrss::HttpURL::getQuery() const
{
	return wstrQuery_.get();
}

wstring_ptr qmrss::HttpURL::getURL() const
{
	StringBuffer<WSTRING> buf;
	buf.append(wstrScheme_.get());
	buf.append(L"://");
	if (wstrUser_.get()) {
		buf.append(wstrUser_.get());
		if (wstrPassword_.get()) {
			buf.append(L':');
			buf.append(wstrPassword_.get());
		}
		buf.append(L'@');
	}
	buf.append(wstrHost_.get());
	buf.append(L'/');
	buf.append(wstrPath_.get());
	if (wstrQuery_.get()) {
		buf.append(L'?');
		buf.append(wstrQuery_.get());
	}
	return buf.getString();
}

std::auto_ptr<HttpURL> qmrss::HttpURL::create(const WCHAR* pwszURL)
{
	const WCHAR* p = wcsstr(pwszURL, L"://");
	if (!p)
		return std::auto_ptr<HttpURL>();
	
	wstring_ptr wstrScheme(allocWString(pwszURL, p - pwszURL));
	if (wcscmp(wstrScheme.get(), L"http") != 0 &&
		wcscmp(wstrScheme.get(), L"https") != 0)
		return std::auto_ptr<HttpURL>();
	
	p += 3;
	
	const WCHAR* p2 = wcschr(p, L'/');
	wstring_ptr wstrHost(allocWString(p, p2 ? p2 - p : -1));
	
	wstring_ptr wstrUser;
	wstring_ptr wstrPassword;
	WCHAR* pAt = wcschr(wstrHost.get(), L'@');
	if (pAt) {
		*pAt = L'\0';
		
		WCHAR* pPassword = wcschr(wstrHost.get(), L':');
		if (pPassword) {
			wstrPassword = allocWString(pPassword + 1);
			*pPassword = L'\0';
		}
		wstrUser = allocWString(wstrHost.get());
		
		wstrHost = allocWString(pAt + 1);
	}
	
	unsigned short nPort = -1;
	WCHAR* pPort = wcsrchr(wstrHost.get(), L':');
	if (pPort) {
		WCHAR* pEnd = 0;
		long n = wcstol(pPort + 1, &pEnd, 10);
		if (*pEnd || n < 0 || USHRT_MAX < n)
			return std::auto_ptr<HttpURL>();
		nPort = static_cast<unsigned short>(n);
		*pPort = L'\0';
	}
	
	wstring_ptr wstrPath;
	wstring_ptr wstrQuery;
	if (p2) {
		const WCHAR* p3 = wcschr(p2, L'?');
		wstrPath = allocWString(p2, p3 ? p3 - p2 : -1);
		if (p3)
			wstrQuery = allocWString(p3 + 1);
	}
	else {
		wstrPath = allocWString(L"/");
	}
	
	return std::auto_ptr<HttpURL>(new HttpURL(wstrScheme.get(),
		wstrHost.get(), nPort, wstrUser.get(), wstrPassword.get(),
		wstrPath.get(), wstrQuery.get()));
}
