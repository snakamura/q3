/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsconv.h>
#include <qsencoder.h>
#include <qsmd5.h>

#include <stdio.h>

#include "smtp.h"

using namespace qs;


/****************************************************************************
 *
 * Smtp
 *
 */

#define SMTP_ERROR(e) \
	do { \
		nError_ = e; \
		return false; \
	} while (false)

#define SMTP_ERROR_SOCKET(e) \
	do { \
		nError_ = e | pSocket_->getLastError(); \
		return false; \
	} while (false)

#define SMTP_ERROR_OR(e) \
	do { \
		nError_ |= e; \
		return false; \
	} while (false)

qmsmtp::Smtp::Smtp(long nTimeout,
				   qs::SocketCallback* pSocketCallback,
				   qs::SSLSocketCallback* pSSLSocketCallback,
				   SmtpCallback* pSmtpCallback,
				   qs::Logger* pLogger) :
	nTimeout_(nTimeout),
	pSocketCallback_(pSocketCallback),
	pSSLSocketCallback_(pSSLSocketCallback),
	pSmtpCallback_(pSmtpCallback),
	pLogger_(pLogger),
	nError_(SMTP_ERROR_SUCCESS)
{
}

qmsmtp::Smtp::~Smtp()
{
}

bool qmsmtp::Smtp::connect(const WCHAR* pwszHost,
						   short nPort,
						   Secure secure)
{
	assert(pwszHost);
	
	std::auto_ptr<Socket> pSocket(new Socket(nTimeout_, pSocketCallback_, pLogger_));
	
	if (!pSocket->connect(pwszHost, nPort))
		SMTP_ERROR(SMTP_ERROR_CONNECT | pSocket->getLastError());
	
	if (secure == SECURE_SSL) {
		SSLSocketFactory* pFactory = SSLSocketFactory::getFactory();
		if (!pFactory)
			SMTP_ERROR(SMTP_ERROR_SSL);
		
		std::auto_ptr<SSLSocket> pSSLSocket(pFactory->createSSLSocket(
			pSocket.get(), true, pSSLSocketCallback_, pLogger_));
		if (!pSSLSocket.get())
			SMTP_ERROR(SMTP_ERROR_SSL);
		
		pSocket.release();
		pSocket_ = pSSLSocket;
	}
	else {
		pSocket_ = pSocket;
	}
	
	unsigned int nCode = 0;
	if (!receive(&nCode))
		SMTP_ERROR_OR(SMTP_ERROR_GREETING);
	else if (nCode != 2)
		SMTP_ERROR(SMTP_ERROR_RESPONSE | SMTP_ERROR_GREETING);
	
	unsigned int nAuth = 0;
	bool bStartTls = false;
	if (!helo(&nAuth, &bStartTls))
		return false;
	
	if (secure == SECURE_STARTTLS) {
		if (!bStartTls)
			SMTP_ERROR(SMTP_ERROR_STARTTLS);
		
		if (!sendCommand("STARTTLS\r\n", &nCode))
			SMTP_ERROR_OR(SMTP_ERROR_STARTTLS);
		else if (nCode != 2)
			SMTP_ERROR(SMTP_ERROR_STARTTLS | SMTP_ERROR_RESPONSE);
		
		SSLSocketFactory* pFactory = SSLSocketFactory::getFactory();
		if (!pFactory)
			SMTP_ERROR(SMTP_ERROR_SSL);
		
		std::auto_ptr<SSLSocket> pSSLSocket(pFactory->createSSLSocket(
			static_cast<Socket*>(pSocket_.get()), true, pSSLSocketCallback_, pLogger_));
		if (!pSSLSocket.get())
			SMTP_ERROR(SMTP_ERROR_SSL);
		pSocket_.release();
		pSocket_ = pSSLSocket;
		
		if (!helo(&nAuth, &bStartTls))
			return false;
	}
	
	wstring_ptr wstrUserName;
	wstring_ptr wstrPassword;
	if (!pSmtpCallback_->getUserInfo(&wstrUserName, &wstrPassword))
		SMTP_ERROR(SMTP_ERROR_AUTH | SMTP_ERROR_OTHER);
	if (wstrUserName.get() && *wstrUserName.get()) {
		pSmtpCallback_->authenticating();
		
		string_ptr strUserName(wcs2mbs(wstrUserName.get()));
		string_ptr strPassword(wcs2mbs(wstrPassword.get()));
		
		unsigned int nAllowedMethods = getAuthMethods();
		nAuth &= nAllowedMethods;
		if (nAuth == 0)
			SMTP_ERROR(SMTP_ERROR_AUTH);
		
		Base64Encoder encoder(false);
		if (nAuth & AUTH_CRAMMD5) {
			string_ptr strMessage;
			if (!sendCommand("AUTH CRAM-MD5\r\n", &nCode, &strMessage))
				SMTP_ERROR_OR(SMTP_ERROR_AUTH);
			if (nCode == 3) {
				malloc_size_ptr<unsigned char> pCharange(encoder.decode(
					reinterpret_cast<unsigned char*>(strMessage.get() + 4),
					strlen(strMessage.get()) - 6));
				if (!pCharange.get())
					SMTP_ERROR(SMTP_ERROR_AUTH);
				CHAR szDigest[33];
				MD5::hmacToString(pCharange.get(), pCharange.size(),
					reinterpret_cast<unsigned char*>(strPassword.get()),
					strlen(strPassword.get()), szDigest);
				
				string_ptr strKey(concat(strUserName.get(), " ", szDigest));
				malloc_size_ptr<unsigned char> pResponse(encoder.encode(
					reinterpret_cast<unsigned char*>(strKey.get()),
					strlen(strKey.get())));
				if (!pResponse.get())
					SMTP_ERROR(SMTP_ERROR_AUTH);
				string_ptr strCommand(concat(reinterpret_cast<char*>(
					pResponse.get()), pResponse.size(), "\r\n", -1));
				if (!sendCommand(strCommand.get(), &nCode))
					SMTP_ERROR_OR(SMTP_ERROR_AUTH);
				if (nCode == 2)
					nAuth = 0;
			}
		}
		if (nAuth & AUTH_PLAIN) {
			size_t nUserNameLen = strlen(strUserName.get());
			size_t nPasswordLen = strlen(strPassword.get());
			string_ptr strKey(allocString(nUserNameLen*2 + nPasswordLen + 10));
			CHAR* pKey = strKey.get();
			strcpy(pKey, strUserName.get());
			pKey += nUserNameLen;
			*pKey++ = '\0';
			strcpy(pKey, strUserName.get());
			pKey += nUserNameLen;
			*pKey++ = '\0';
			strcpy(pKey, strPassword.get());
			pKey += nPasswordLen;
			*pKey = '\0';
			
			malloc_size_ptr<unsigned char> p(encoder.encode(
				reinterpret_cast<unsigned char*>(strKey.get()),
				strlen(strKey.get())));
			if (!p.get())
				SMTP_ERROR(SMTP_ERROR_AUTH);
			string_ptr strCommand(concat("AUTH PLAIN ", -1,
				reinterpret_cast<char*>(p.get()), p.size(), "\r\n", -1));
			if (!sendCommand(strCommand.get(), &nCode))
				SMTP_ERROR_OR(SMTP_ERROR_AUTH);
			if (nCode == 2)
				nAuth = 0;
		}
		if (nAuth & AUTH_LOGIN) {
			if (!sendCommand("AUTH LOGIN\r\n", &nCode))
				SMTP_ERROR_OR(SMTP_ERROR_AUTH);
			if (nCode == 3) {
				malloc_size_ptr<unsigned char> pUserName(encoder.encode(
					reinterpret_cast<unsigned char*>(strUserName.get()),
					strlen(strUserName.get())));
				if (!pUserName.get())
					SMTP_ERROR(SMTP_ERROR_AUTH);
				string_ptr strCommand(concat(reinterpret_cast<char*>(
					pUserName.get()), pUserName.size(), "\r\n", -1));
				if (!sendCommand(strCommand.get(), &nCode))
					SMTP_ERROR_OR(SMTP_ERROR_AUTH);
				if (nCode == 3) {
					malloc_size_ptr<unsigned char> pPassword(encoder.encode(
						reinterpret_cast<unsigned char*>(strPassword.get()),
						strlen(strPassword.get())));
					if (!pPassword.get())
						SMTP_ERROR(SMTP_ERROR_AUTH);
					strCommand = concat(reinterpret_cast<char*>(
						pPassword.get()), pPassword.size(), "\r\n", -1);
					if (!sendCommand(strCommand.get(), &nCode))
						SMTP_ERROR_OR(SMTP_ERROR_AUTH);
					if (nCode == 2)
						nAuth = 0;
				}
			}
		}
		if (nAuth != 0)
			SMTP_ERROR(SMTP_ERROR_AUTH);
	}
	
	return true;
}

void qmsmtp::Smtp::disconnect()
{
	if (pSocket_.get()) {
		unsigned int nCode = 0;
		sendCommand("QUIT\r\n", &nCode);
		pSocket_.reset(0);
	}
}

bool qmsmtp::Smtp::sendMessage(const SendMessageData& data)
{
	assert(data.pszEnvelopeFrom_);
	assert(data.ppszAddresses_);
	assert(data.nAddressSize_ != 0);
	assert(data.pszMessage_);
	
	string_ptr strCommand(concat("MAIL FROM: <", data.pszEnvelopeFrom_, ">\r\n"));
	
	unsigned int nCode = 0;
	if (!sendCommand(strCommand.get(), &nCode))
		SMTP_ERROR_OR(SMTP_ERROR_MAIL);
	else if (nCode != 2)
		SMTP_ERROR(SMTP_ERROR_MAIL | SMTP_ERROR_RESPONSE);
	
	for (size_t n = 0; n < data.nAddressSize_; ++n) {
		strCommand = concat("RCPT TO: <", data.ppszAddresses_[n], ">\r\n");
		if (!sendCommand(strCommand.get(), &nCode))
			SMTP_ERROR_OR(SMTP_ERROR_RCPT);
		else if (nCode != 2)
			SMTP_ERROR(SMTP_ERROR_RCPT | SMTP_ERROR_RESPONSE);
	}
	
	typedef std::vector<SendData> SendDataList;
	SendDataList listSendData;
	const CHAR* p = data.pszMessage_;
	SendData sd = { p, 0 };
	for (size_t m = 0; m < data.nLength_; ++m, ++p) {
		if (*p == '.' && m > 1 && *(p - 1) == '\n' && *(p - 2) == '\r') {
			sd.nLength_ = p - sd.psz_ + 1;
			listSendData.push_back(sd);
			sd.psz_ = p;
		}
	}
	sd.nLength_ = p - sd.psz_;
	listSendData.push_back(sd);
	
	if (data.nLength_ > 2 &&
		data.pszMessage_[data.nLength_ - 2] == '\r' &&
		data.pszMessage_[data.nLength_ - 1] == '\n') {
		sd.psz_ = ".\r\n";
		sd.nLength_ = 3;
	}
	else {
		sd.psz_ = "\r\n.\r\n";
		sd.nLength_ = 5;
	}
	listSendData.push_back(sd);
	
	if (!sendCommand("DATA\r\n", &nCode))
		SMTP_ERROR_OR(SMTP_ERROR_DATA);
	else if (nCode != 3)
		SMTP_ERROR(SMTP_ERROR_DATA | SMTP_ERROR_RESPONSE);
	
	if (!send(&listSendData[0], listSendData.size(), true, &nCode, 0))
		SMTP_ERROR_OR(SMTP_ERROR_DATA);
	
	return true;
}

unsigned int qmsmtp::Smtp::getLastError() const
{
	return nError_;
}

const WCHAR* qmsmtp::Smtp::getLastErrorResponse() const
{
	return wstrErrorResponse_.get();
}

bool qmsmtp::Smtp::helo(unsigned int* pnAuth,
						bool* pbStartTls)
{
	assert(pnAuth);
	assert(pbStartTls);
	
	*pnAuth = 0;
	*pbStartTls = false;
	
	wstring_ptr wstrLocalHost(pSmtpCallback_->getLocalHost());
	string_ptr strLocalHost;
	if (wstrLocalHost.get() && *wstrLocalHost.get()) {
		strLocalHost = wcs2mbs(wstrLocalHost.get());
	}
	else {
		strLocalHost = allocString(256);
		::gethostname(strLocalHost.get(), 256);
	}
	
	bool bEHLO = true;
	string_ptr strHelo(allocString(strlen(strLocalHost.get()) + 10));
	sprintf(strHelo.get(), "EHLO %s\r\n", strLocalHost.get());
	unsigned int nCode = 0;
	string_ptr strResult;
	if (!sendCommand(strHelo.get(), &nCode, &strResult))
		SMTP_ERROR_OR(SMTP_ERROR_HELO);
	if (nCode != 2) {
		strResult.reset(0);
		sprintf(strHelo.get(), "HELO %s\r\n", strLocalHost.get());
		if (!sendCommand(strHelo.get(), &nCode, &strResult))
			SMTP_ERROR_OR(SMTP_ERROR_HELO);
		bEHLO = false;
	}
	if (nCode != 2)
		SMTP_ERROR(SMTP_ERROR_HELO | SMTP_ERROR_RESPONSE);
	
	if (bEHLO) {
		struct
		{
			const CHAR* pszKey_;
			Auth auth_;
		} keys[] = {
			{ "LOGIN",		AUTH_LOGIN		},
			{ "PLAIN",		AUTH_PLAIN		},
			{ "CRAM-MD5",	AUTH_CRAMMD5	}
		};
		const CHAR* p = strstr(strResult.get(), "\r\n");
		assert(p);
		p += 2;
		while (true) {
			const CHAR* pEnd = strstr(p, "\r\n");
			if (!pEnd)
				break;
			if (*pnAuth == 0 &&
				strncmp(p + 4, "AUTH", 4) == 0 &&
				(*(p + 8) == ' ' || *(p + 8) == '=')) {
				p += 9;
				pEnd = p;
				while (true) {
					while (*pEnd != ' ' && *pEnd != '\r')
						++pEnd;
					for (int n = 0; n < countof(keys); ++n) {
						if (strlen(keys[n].pszKey_) == static_cast<size_t>(pEnd - p) &&
							strncmp(keys[n].pszKey_, p, pEnd - p) == 0) {
							*pnAuth |= keys[n].auth_;
							break;
						}
					}
					if (*pEnd == '\r')
						break;
					p = pEnd + 1;
					pEnd = p;
				}
			}
			else if (strncmp(p + 4, "STARTTLS", 8) == 0 && *(p + 12) == '\r') {
				*pbStartTls = true;
			}
			p = pEnd + 2;
		}
	}
	
	return true;
}

bool qmsmtp::Smtp::receive(unsigned int* pnCode)
{
	return receive(pnCode, 0);
}

bool qmsmtp::Smtp::receive(unsigned int* pnCode,
						   string_ptr* pstrResponse)
{
	assert(pnCode);
	
	if (!pSocket_.get())
		SMTP_ERROR(SMTP_ERROR_INVALIDSOCKET);
	
	StringBuffer<STRING> bufResponse;
	
	char buf[RECEIVE_BLOCK_SIZE];
	bool bEnd = false;
	do {
		int nSelect = pSocket_->select(Socket::SELECT_READ);
		if (nSelect == -1)
			SMTP_ERROR_SOCKET(SMTP_ERROR_SELECT);
		else if (nSelect == 0)
			SMTP_ERROR(SMTP_ERROR_TIMEOUT);
		
		size_t nLen = pSocket_->recv(buf, sizeof(buf), 0);
		if (nLen == -1)
			SMTP_ERROR_SOCKET(SMTP_ERROR_RECEIVE);
		else if (nLen == 0)
			SMTP_ERROR(SMTP_ERROR_DISCONNECT);
		
		bufResponse.append(buf, nLen);
		
		const CHAR* pBuf = bufResponse.getCharArray();
		const CHAR* p = pBuf + bufResponse.getLength();
		while (p > pBuf && *p != '\n')
			--p;
		if (p > pBuf) {
			do {
				--p;
			} while (p > pBuf && *p != '\n');
			if (p > pBuf)
				++p;
			if (bufResponse.getLength() > static_cast<size_t>(p - pBuf + 3) &&
				*(p + 3) == ' ')
				bEnd = true;
		}
		
	} while (!bEnd);
	
	setErrorResponse(bufResponse.getCharArray());
	
	*pnCode = bufResponse.get(0) - '0';
	if (pstrResponse)
		*pstrResponse = bufResponse.getString();
	
	
	return true;
}

bool qmsmtp::Smtp::send(const SendData* pSendData,
						size_t nDataLen,
						bool bProgress,
						unsigned int* pnCode,
						qs::string_ptr* pstrResponse)
{
	assert(pSendData);
	assert(nDataLen > 0);
	assert(pnCode);
	
	if (!pSocket_.get())
		SMTP_ERROR(SMTP_ERROR_INVALIDSOCKET);
	
	if (bProgress) {
		size_t nLen = 0;
		for (size_t n = 0; n < nDataLen; ++n)
			nLen += (pSendData + n)->nLength_;
		pSmtpCallback_->setRange(0, nLen);
	}
	
	for (size_t n = 0; n < nDataLen; ++n) {
		const SendData& data = *(pSendData + n);
		size_t nTotal = 0;
		while (nTotal < data.nLength_) {
			int nSelect = pSocket_->select(Socket::SELECT_READ | Socket::SELECT_WRITE);
			if (nSelect == -1)
				SMTP_ERROR_SOCKET(SMTP_ERROR_SELECT);
			else if (nSelect == 0)
				SMTP_ERROR(SMTP_ERROR_TIMEOUT);
			
			size_t nSend = pSocket_->send(data.psz_ + nTotal,
				QSMIN(size_t(SEND_BLOCK_SIZE), data.nLength_ - nTotal), 0);
			if (nSend == -1)
				SMTP_ERROR_SOCKET(SMTP_ERROR_SEND);
			nTotal += nSend;
			
			if (bProgress)
				pSmtpCallback_->setPos(nTotal);
		}
	}
	if (bProgress) {
		pSmtpCallback_->setRange(0, 0);
		pSmtpCallback_->setPos(0);
	}
	
	return receive(pnCode, pstrResponse);
}

bool qmsmtp::Smtp::sendCommand(const CHAR* psz,
							   unsigned int* pnCode)
{
	return sendCommand(psz, pnCode, 0);
}

bool qmsmtp::Smtp::sendCommand(const CHAR* psz,
							   unsigned int* pnCode,
							   string_ptr* pstrResponse)
{
	SendData data = { psz, strlen(psz) };
	return send(&data, 1, false, pnCode, pstrResponse);
}

void qmsmtp::Smtp::setErrorResponse(const CHAR* pszErrorResponse)
{
	wstrErrorResponse_ = mbs2wcs(pszErrorResponse);
}

unsigned int qmsmtp::Smtp::getAuthMethods()
{
	wstring_ptr wstrAuthMethods(pSmtpCallback_->getAuthMethods());
	
	struct {
		const WCHAR* pwsz_;
		Auth auth_;
	} methods[] = {
		{ L"LOGIN",		AUTH_LOGIN		},
		{ L"PLAIN",		AUTH_PLAIN		},
		{ L"CRAM-MD5",	AUTH_CRAMMD5	}
	};
	
	unsigned int nAuth = 0;
	
	const WCHAR* p = wcstok(wstrAuthMethods.get(), L" ");
	while (p) {
		for (int n = 0; n < countof(methods); ++n) {
			if (wcscmp(p, methods[n].pwsz_) == 0) {
				nAuth |= methods[n].auth_;
				break;
			}
		}
		p = wcstok(0, L" ");
	}
	
	if (nAuth == 0)
		nAuth = 0xffffffff;
	
	return nAuth;
}


/****************************************************************************
 *
 * SmtpCallback
 *
 */

qmsmtp::SmtpCallback::~SmtpCallback()
{
}
