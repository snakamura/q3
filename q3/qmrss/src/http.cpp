/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#pragma warning(disable:4786)

#include <qsconv.h>
#include <qsencoder.h>
#include <qsosutil.h>

#include <wininet.h>

#include "http.h"
#include "resource.h"

using namespace qmrss;
using namespace qs;


/****************************************************************************
 *
 * Http
 *
 */

qmrss::Http::Http(SocketCallback* pSocketCallback,
				  SSLSocketCallback* pSSLSocketCallback,
				  HttpCallback* pHttpCallback,
				  Logger* pLogger) :
	nTimeout_(60),
	nProxyPort_(8080),
	pSocketCallback_(pSocketCallback),
	pSSLSocketCallback_(pSSLSocketCallback),
	pHttpCallback_(pHttpCallback),
	pLogger_(pLogger)
{
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
	unsigned int nRetryCount = pMethod->getRetryCount() + 1;
	if (bProxied) {
		bool bConnect = false;
		for (unsigned int n = 0; n < nRetryCount && !bConnect; ++n)
			bConnect = pSocket->connect(wstrProxyHost_.get(), nProxyPort_);
		if (!bConnect)
			return -1;
		
		if (pMethod->isSecure()) {
			StringBuffer<WSTRING> request;
			request.append(L"CONNECT ");
			request.append(pMethod->getHost());
			request.append(L":");
			WCHAR wszPort[32];
			_snwprintf(wszPort, countof(wszPort), L"%u", pMethod->getPort());
			request.append(wszPort);
			request.append(L" HTTP/1.0\r\n\r\n");
			if (!HttpUtil::write(pSocket.get(), request.getCharArray(), request.getLength()))
				return -1;
			
			xstring_ptr strResponse(HttpUtil::readLine(pSocket.get()));
			if (!strResponse.get())
				return -1;
			unsigned int nStatus = HttpUtil::parseResponse(strResponse.get());
			if (nStatus == -1 || nStatus/100 != 2)
				return -1;
			
			while (true) {
				xstring_ptr str(HttpUtil::readLine(pSocket.get()));
				if (!str.get())
					return -1;
				else if (!*str.get())
					break;
			}
		}
	}
	else {
		bool bConnect = false;
		for (unsigned int n = 0; n < nRetryCount && !bConnect; ++n)
			bConnect = pSocket->connect(pMethod->getHost(), pMethod->getPort());
		if (!bConnect)
			return -1;
	}
	
	if (pMethod->isSecure()) {
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
	
	std::auto_ptr<HttpConnection> pConnection(new HttpConnection(pSocketBase, bProxied));
	if (bProxied && wstrProxyUserName_.get() && wstrProxyPassword_.get())
		pMethod->setProxyCredential(wstrProxyUserName_.get(), wstrProxyPassword_.get());
	return pMethod->invoke(pConnection);
}

unsigned int qmrss::Http::getTimeout() const
{
	return nTimeout_;
}

void qmrss::Http::setTimeout(unsigned int nTimeout)
{
	nTimeout_ = nTimeout;
}

const WCHAR* qmrss::Http::getProxyHost() const
{
	return wstrProxyHost_.get();
}

void qmrss::Http::setProxyHost(const WCHAR* pwszProxyHost)
{
	if (pwszProxyHost)
		wstrProxyHost_ = allocWString(pwszProxyHost);
	else
		wstrProxyHost_.reset(0);
}

unsigned short qmrss::Http::getProxyPort() const
{
	return nProxyPort_;
}

void qmrss::Http::setProxyPort(unsigned short nProxyPort)
{
	nProxyPort_ = nProxyPort;
}

const WCHAR* qmrss::Http::getProxyUserName() const
{
	return wstrProxyUserName_.get();
}

void qmrss::Http::setProxyUserName(const WCHAR* pwszUserName)
{
	if (pwszUserName)
		wstrProxyUserName_ = allocWString(pwszUserName);
	else
		wstrProxyUserName_.reset(0);
}

const WCHAR* qmrss::Http::getProxyPassword() const
{
	return wstrProxyPassword_.get();
}

void qmrss::Http::setProxyPassword(const WCHAR* pwszPassword)
{
	if (pwszPassword)
		wstrProxyPassword_ = allocWString(pwszPassword);
	else
		wstrProxyPassword_.reset(0);
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

bool qmrss::HttpConnection::write(const WCHAR* p,
								  size_t nLen)
{
	return HttpUtil::write(pSocket_.get(), p, nLen);
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
	return HttpUtil::readLine(pInputStream_.get());
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

void qmrss::AbstractHttpMethod::setCredential(const WCHAR* pwszUserName,
											  const WCHAR* pwszPassword)
{
	assert(pwszUserName);
	assert(pwszPassword);
	
	wstrUserName_ = allocWString(pwszUserName);
	wstrPassword_ = allocWString(pwszPassword);
}

void qmrss::AbstractHttpMethod::setProxyCredential(const WCHAR* pwszUserName,
												   const WCHAR* pwszPassword)
{
	assert(pwszUserName);
	assert(pwszPassword);
	
	wstrProxyUserName_ = allocWString(pwszUserName);
	wstrProxyPassword_ = allocWString(pwszPassword);
}

const CHAR* qmrss::AbstractHttpMethod::getResponseLine() const
{
	return strResponseLine_.get();
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
	return nPort != static_cast<unsigned short>(-1) ? nPort : isSecure() ? 443 : 80;
}

bool qmrss::AbstractHttpMethod::isSecure() const
{
	return wcscmp(pURL_->getScheme(), L"https") == 0;
}

bool qmrss::AbstractHttpMethod::isReady() const
{
	return pURL_.get() != 0;
}

unsigned int qmrss::AbstractHttpMethod::getRetryCount() const
{
	return 2;
}

unsigned int qmrss::AbstractHttpMethod::invoke(std::auto_ptr<HttpConnection> pConnection)
{
	assert(pConnection.get());
	assert(pURL_.get());
	
	StringBuffer<WSTRING> requestHeader;
	
	const WCHAR* pwszName = getName();
	requestHeader.append(pwszName);
	requestHeader.append(L" ");
	
	if (pConnection->isProxied() && !isSecure()) {
		requestHeader.append(pURL_->getScheme());
		requestHeader.append(L"://");
		requestHeader.append(pURL_->getAuthority().get());
	}
	requestHeader.append(pURL_->getPath());
	if (pURL_->getQuery()) {
		requestHeader.append(L'?');
		requestHeader.append(pURL_->getQuery());
	}
	requestHeader.append(L" HTTP/1.0\r\n");
	
	requestHeader.append(L"Host: ");
	requestHeader.append(pURL_->getAuthority().get());
	requestHeader.append(L"\r\n");
	
	requestHeader.append(L"Connection: close\r\n");
	
	for (HeaderList::const_iterator it = listRequestHeader_.begin(); it != listRequestHeader_.end(); ++it) {
		requestHeader.append((*it).first);
		requestHeader.append(L": ");
		requestHeader.append((*it).second);
		requestHeader.append(L"\r\n");
	}
	
	std::pair<const WCHAR*, const WCHAR*> credential(getCredential());
	if (credential.first && credential.second) {
		wstring_ptr wstrCredential(HttpUtil::getBasicCredential(
			credential.first, credential.second));
		if (!wstrCredential.get())
			return -1;
		requestHeader.append(L"Authorization: Basic ");
		requestHeader.append(wstrCredential.get());
		requestHeader.append(L"\r\n");
	}
	
	if (pConnection->isProxied() && wstrProxyUserName_.get() && wstrProxyPassword_.get()) {
		wstring_ptr wstrCredential(HttpUtil::getBasicCredential(
			wstrProxyUserName_.get(), wstrProxyPassword_.get()));
		if (!wstrCredential.get())
			return -1;
		requestHeader.append(L"Proxy-Authorization: Basic ");
		requestHeader.append(wstrCredential.get());
		requestHeader.append(L"\r\n");
	}
	
	if (!getRequestHeaders(&requestHeader))
		return -1;
	
	size_t nBodyLen = getRequestBodyLength();
	if (nBodyLen != 0) {
		WCHAR wszLen[128];
		_snwprintf(wszLen, countof(wszLen), L"Content-Length: %u\r\n", nBodyLen);
		requestHeader.append(wszLen);
	}
	
	requestHeader.append(L"\r\n");
	
	if (!pConnection->write(requestHeader.getCharArray(), requestHeader.getLength()))
		return -1;
	
	if (nBodyLen != 0) {
		if (!writeRequestBody(pConnection.get()))
			return -1;
	}
	
	strResponseLine_ = pConnection->readLine();
	if (!strResponseLine_.get())
		return -1;
	unsigned int nStatus = HttpUtil::parseResponse(strResponseLine_.get());
	if (nStatus == -1)
		return -1;
	
	XStringBuffer<XSTRING> responseHeader;
	while (true) {
		xstring_ptr str(pConnection->readLine());
		if (!str.get())
			return -1;
		if (!*str.get())
			break;
		if (!responseHeader.append(str.get()) ||
			!responseHeader.append("\r\n"))
			return -1;
	}
	strResponseHeader_ = responseHeader.getXString();
	
	pConnection_ = pConnection;
	
	return nStatus;
}

bool qmrss::AbstractHttpMethod::getRequestHeaders(StringBuffer<WSTRING>* pBuf) const
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

std::pair<const WCHAR*, const WCHAR*> qmrss::AbstractHttpMethod::getCredential() const
{
	const WCHAR* pwszUserName = wstrUserName_.get();
	const WCHAR* pwszPassword = wstrPassword_.get();
	if (!pwszUserName && !pwszPassword) {
		pwszUserName = pURL_->getUser();
		pwszPassword = pURL_->getPassword();
	}
	return std::make_pair(pwszUserName, pwszPassword);
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
	if (nPort_ != static_cast<unsigned short>(-1)) {
		WCHAR wszPort[32];
		_snwprintf(wszPort, countof(wszPort), L":%d", static_cast<int>(nPort_));
		buf.append(wszPort);
	}
	buf.append(wstrPath_.get());
	if (wstrQuery_.get()) {
		buf.append(L'?');
		buf.append(wstrQuery_.get());
	}
	return buf.getString();
}

wstring_ptr qmrss::HttpURL::getAuthority() const
{
	StringBuffer<WSTRING> buf;
	buf.append(wstrHost_.get());
	if (nPort_ != static_cast<unsigned short>(-1)) {
		WCHAR wszPort[32];
		_snwprintf(wszPort, countof(wszPort), L":%d", static_cast<int>(nPort_));
		buf.append(wszPort);
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


/****************************************************************************
 *
 * HttpUtil
 *
 */

wstring_ptr qmrss::HttpUtil::getBasicCredential(const WCHAR* pwszUserName,
												const WCHAR* pwszPassword)
{
	wstring_ptr wstrCredential(concat(pwszUserName, L":", pwszPassword));
	size_t nLen = wcslen(wstrCredential.get());
	xstring_size_ptr strCredential(UTF8Converter().encode(wstrCredential.get(), &nLen));
	if (!strCredential.get())
		return 0;
	malloc_size_ptr<unsigned char> pCredential(Base64Encoder(false).encode(
		reinterpret_cast<const unsigned char*>(strCredential.get()), strCredential.size()));
	return mbs2wcs(reinterpret_cast<const CHAR*>(pCredential.get()), pCredential.size());
}

unsigned int qmrss::HttpUtil::parseResponse(const char* p)
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

xstring_ptr qmrss::HttpUtil::readLine(InputStream* pInputStream)
{
	bool bCr = false;
	XStringBuffer<XSTRING> buf;
	while (true) {
		unsigned char c = 0;
		size_t n = pInputStream->read(&c, 1);
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

xstring_ptr qmrss::HttpUtil::readLine(qs::SocketBase* pSocket)
{
	bool bCr = false;
	XStringBuffer<XSTRING> buf;
	while (true) {
		unsigned char c = 0;
		if (!readByte(pSocket, &c))
			return 0;
		
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

bool qmrss::HttpUtil::readByte(qs::SocketBase* pSocket,
							   unsigned char* p)
{
	int nSelect = pSocket->select(SocketBase::SELECT_READ);
	if (nSelect == -1)
		return false;
	else if (nSelect == 0)
		return false;
	return pSocket->recv(reinterpret_cast<char*>(p), 1, 0) == 1;
}

bool qmrss::HttpUtil::write(SocketBase* pSocket,
							const unsigned char* p,
							size_t nLen)
{
	while (nLen != 0) {
		int nSelect = pSocket->select(SocketBase::SELECT_WRITE | SocketBase::SELECT_READ);
		if (nSelect == -1)
			return false;
		else if (nSelect == 0)
			return false;
		int n = pSocket->send(reinterpret_cast<const char*>(p), static_cast<int>(nLen), 0);
		if (n == -1)
			return false;
		nLen -= n;
		p += n;
	}
	return true;
}

bool qmrss::HttpUtil::write(SocketBase* pSocket,
							const WCHAR* p,
							size_t nLen)
{
	string_ptr str(wcs2mbs(p, nLen));
	return write(pSocket, reinterpret_cast<const unsigned char*>(str.get()), strlen(str.get()));
}

wstring_ptr qmrss::HttpUtil::getRedirectLocation(const WCHAR* pwszURL,
												 const qs::Part& header,
												 UINT* pnErrorId)
{
	UnstructuredParser location;
	if (header.getField(L"Location", &location) != Part::FIELD_EXIST) {
		if (pnErrorId)
			*pnErrorId = IDS_ERROR_PARSEREDIRECTLOCATION;
		return 0;
	}
	const WCHAR* pwszLocation = location.getValue();
	wstring_ptr wstrLocation;
	if (!HttpURL::create(pwszLocation).get()) {
		// Because some server set a relative URL in Location:, I'll handle them here.
		// This is not allowed in HTTP/1.1.
		bool bRecover = false;
		if (*pwszLocation == L'/') {
			std::auto_ptr<HttpURL> pURL(HttpURL::create(pwszURL));
			if (pURL.get()) {
				StringBuffer<WSTRING> buf;
				buf.append(pURL->getScheme());
				buf.append(L"://");
				buf.append(pURL->getAuthority().get());
				buf.append(pwszLocation);
				
				std::auto_ptr<HttpURL> pLocation(HttpURL::create(buf.getCharArray()));
				if (pLocation.get()) {
					wstrLocation = pLocation->getURL();
					pwszLocation = wstrLocation.get();
					bRecover = true;
				}
			}
		}
		if (!bRecover) {
			if (pnErrorId)
				*pnErrorId = IDS_ERROR_INVALIDREDIRECTLOCATION;
			return 0;
		}
	}
	return allocWString(pwszLocation);
}

wstring_ptr qmrss::HttpUtil::resolveRelativeURL(const WCHAR* pwszURL,
												const WCHAR* pwszBaseURL)
{
	assert(pwszURL);
	assert(pwszBaseURL);
	
	if (HttpURL::create(pwszURL).get())
		return allocWString(pwszURL);
	
	std::auto_ptr<HttpURL> pBaseURL(HttpURL::create(pwszBaseURL));
	if (!pBaseURL.get())
		return allocWString(pwszURL);
	
	StringBuffer<WSTRING> buf;
	buf.append(pBaseURL->getScheme());
	buf.append(L"://");
	buf.append(pBaseURL->getAuthority().get());
	if (*pwszURL != L'/') {
		const WCHAR* pwszPath = pBaseURL->getPath();
		const WCHAR* p = wcsrchr(pwszPath, L'/');
		if (p)
			buf.append(pwszPath, p - pwszPath + 1);
	}
	buf.append(pwszURL);
	return buf.getString();
}

bool qmrss::HttpUtil::getInternetProxySetting(wstring_ptr* pwstrProxyHost,
											  unsigned short* pnProxyPort)
{
	Registry reg(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings");
	if (!reg)
		return false;
	
	DWORD dwProxyEnable = 0;
	if (!reg.getValue(L"ProxyEnable", &dwProxyEnable) || dwProxyEnable == 0)
		return false;
	
	wstring_ptr wstrProxy;
	if (!reg.getValue(L"ProxyServer", &wstrProxy))
		return false;
	
	if (wcschr(wstrProxy.get(), L';')) {
		const WCHAR* p = wcstok(wstrProxy.get(), L";");
		while (p) {
			if (wcsncmp(p, L"http=", 5) == 0) {
				wstrProxy = allocWString(p + 5);
				break;
			}
			p = wcstok(0, L";");
		}
	}
	
	const WCHAR* pPort = wcsrchr(wstrProxy.get(), L':');
	if (!pPort)
		return false;
	
	*pwstrProxyHost = allocWString(wstrProxy.get(), pPort - wstrProxy.get());
	
	WCHAR* pEnd = 0;
	*pnProxyPort = static_cast<unsigned short>(wcstol(pPort + 1, &pEnd, 0));
	if (*pEnd)
		return false;
	
	return true;
}

wstring_ptr qmrss::HttpUtil::getInternetCookie(const WCHAR* pwszURL)
{
#if !defined _WIN32_WCE || _WIN32_WCE >= 300
	W2T(pwszURL, ptszURL);
	DWORD dwSize = 0;
	if (!::InternetGetCookie(ptszURL, 0, 0, &dwSize))
		return 0;
	
	tstring_ptr tstrCookie(allocTString(dwSize));
	if (!::InternetGetCookie(ptszURL, 0, tstrCookie.get(), &dwSize))
		return 0;
	
	return tcs2wcs(tstrCookie.get());
#else
	return 0;
#endif
}

bool qmrss::HttpUtil::setInternetCookie(const WCHAR* pwszURL,
										const WCHAR* pwszCookie)
{
#if !defined _WIN32_WCE || _WIN32_WCE >= 300
	const WCHAR* p = wcschr(pwszCookie, L'=');
	if (!p)
		return false;
	
	tstring_ptr tstrName(wcs2tcs(pwszCookie, p - pwszCookie));
	
	W2T(pwszURL, ptszURL);
	W2T(p + 1, ptszData);
	return ::InternetSetCookie(ptszURL, tstrName.get(), ptszData) != 0;
#else
	return true;
#endif
}

void qmrss::HttpUtil::updateInternetCookies(const WCHAR* pwszURL,
											const Part& header)
{
	MultipleUnstructuredParser cookie;
	if (header.getField(L"Set-Cookie", &cookie) == Part::FIELD_EXIST) {
		const MultipleUnstructuredParser::ValueList& l = cookie.getValues();
		for (MultipleUnstructuredParser::ValueList::const_iterator it = l.begin(); it != l.end(); ++it)
			setInternetCookie(pwszURL, *it);
	}
}
