/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsconv.h>
#include <qsencoder.h>
#include <qserror.h>
#include <qsmd5.h>
#include <qsnew.h>

#include <stdio.h>

#include "smtp.h"

using namespace qs;


/****************************************************************************
 *
 * Smtp
 *
 */

#define CHECK_QSTATUS_ERROR(e) \
	if (status != QSTATUS_SUCCESS) { \
		nError_ = e; \
		return status; \
	} \

#define CHECK_QSTATUS_ERROR_OR(e) \
	if (status != QSTATUS_SUCCESS) { \
		nError_ |= e; \
		return status; \
	} \

#define CHECK_ERROR(c, q, e) \
	if (c) { \
		nError_ = e; \
		return q; \
	} \

qmsmtp::Smtp::Smtp(const Option& option, QSTATUS* pstatus) :
	nTimeout_(option.nTimeout_),
	pSocketCallback_(option.pSocketCallback_),
	pSSLSocketCallback_(option.pSSLSocketCallback_),
	pSmtpCallback_(option.pSmtpCallback_),
	pLogger_(option.pLogger_),
	pSocket_(0),
	nError_(SMTP_ERROR_SUCCESS),
	wstrErrorResponse_(0)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qmsmtp::Smtp::~Smtp()
{
	delete pSocket_;
	freeWString(wstrErrorResponse_);
}

QSTATUS qmsmtp::Smtp::connect(const WCHAR* pwszHost, short nPort, Ssl ssl)
{
	assert(pwszHost);
	
	DECLARE_QSTATUS();
	
	Socket::Option option = {
		nTimeout_,
		pSocketCallback_,
		pLogger_
	};
	std::auto_ptr<Socket> pSocket;
	status = newQsObject(option, &pSocket);
	CHECK_QSTATUS_ERROR(SMTP_ERROR_INITIALIZE);
	
	status = pSocket->connect(pwszHost, nPort);
	CHECK_QSTATUS_ERROR(SMTP_ERROR_CONNECT | pSocket->getLastError());
	
	if (ssl == SSL_SSL) {
		SSLSocketFactory* pFactory = SSLSocketFactory::getFactory();
		CHECK_ERROR(!pFactory, QSTATUS_FAIL, SMTP_ERROR_SSL);
		SSLSocket* pSSLSocket = 0;
		status = pFactory->createSSLSocket(pSocket.get(),
			true, pSSLSocketCallback_, pLogger_, &pSSLSocket);
		CHECK_QSTATUS_ERROR(SMTP_ERROR_SSL);
		pSocket.release();
		pSocket_ = pSSLSocket;
	}
	else {
		pSocket_ = pSocket.release();
	}
	
	unsigned int nCode = 0;
	status = receive(&nCode);
	CHECK_QSTATUS_ERROR_OR(SMTP_ERROR_GREETING);
	CHECK_ERROR(nCode != 2, QSTATUS_FAIL,
		SMTP_ERROR_RESPONSE | SMTP_ERROR_GREETING);
	
	unsigned int nAuth = 0;
	bool bStartTls = false;
	status = helo(&nAuth, &bStartTls);
	CHECK_QSTATUS();
	
	if (ssl == SSL_STARTTLS) {
		CHECK_ERROR(!bStartTls, QSTATUS_FAIL, SMTP_ERROR_STARTTLS);
		status = sendCommand("STARTTLS\r\n", &nCode);
		CHECK_QSTATUS_ERROR_OR(SMTP_ERROR_STARTTLS);
		CHECK_ERROR(nCode != 2, QSTATUS_FAIL,
			SMTP_ERROR_STARTTLS | SMTP_ERROR_RESPONSE);
		
		SSLSocketFactory* pFactory = SSLSocketFactory::getFactory();
		CHECK_ERROR(!pFactory, QSTATUS_FAIL, SMTP_ERROR_SSL);
		SSLSocket* pSSLSocket = 0;
		status = pFactory->createSSLSocket(static_cast<Socket*>(pSocket_),
			true, pSSLSocketCallback_, pLogger_, &pSSLSocket);
		CHECK_QSTATUS_ERROR(SMTP_ERROR_SSL);
		pSocket_ = pSSLSocket;
		
		status = helo(&nAuth, &bStartTls);
		CHECK_QSTATUS();
	}
	
	string_ptr<WSTRING> wstrUserName;
	string_ptr<WSTRING> wstrPassword;
	status = pSmtpCallback_->getUserInfo(&wstrUserName, &wstrPassword);
	CHECK_QSTATUS_ERROR_OR(SMTP_ERROR_AUTH | SMTP_ERROR_OTHER);
	if (wstrUserName.get() && *wstrUserName.get()) {
		status = pSmtpCallback_->authenticating();
		CHECK_QSTATUS();
		
		string_ptr<STRING> strUserName(wcs2mbs(wstrUserName.get()));
		CHECK_ERROR(!strUserName.get(), QSTATUS_OUTOFMEMORY,
			SMTP_ERROR_AUTH | SMTP_ERROR_OTHER);
		string_ptr<STRING> strPassword(wcs2mbs(wstrPassword.get()));
		CHECK_ERROR(!strPassword.get(), QSTATUS_OUTOFMEMORY,
			SMTP_ERROR_AUTH | SMTP_ERROR_OTHER);
		
		unsigned int nAllowedMethods = 0;
		status = getAuthMethods(&nAllowedMethods);
		CHECK_QSTATUS();
		nAuth &= nAllowedMethods;
		CHECK_ERROR(nAuth == 0, QSTATUS_FAIL, SMTP_ERROR_AUTH);
		
		Base64Encoder encoder(false, &status);
		if (nAuth & AUTH_CRAMMD5) {
			string_ptr<STRING> strMessage;
			status = sendCommand("AUTH CRAM-MD5\r\n", &nCode, &strMessage);
			CHECK_QSTATUS_ERROR_OR(SMTP_ERROR_AUTH);
			if (nCode == 3) {
				CHECK_QSTATUS_ERROR(SMTP_ERROR_AUTH);
				unsigned char* p = 0;
				size_t nLen = 0;
				status = encoder.decode(
					reinterpret_cast<unsigned char*>(strMessage.get() + 4),
					strlen(strMessage.get()) - 6, &p, &nLen);
				CHECK_QSTATUS_ERROR(SMTP_ERROR_AUTH);
				CHAR szDigest[33];
				MD5::hmacToString(p, nLen,
					reinterpret_cast<unsigned char*>(strPassword.get()),
					strlen(strPassword.get()), szDigest);
				free(p);
				
				string_ptr<STRING> strKey(
					concat(strUserName.get(), " ", szDigest));
				CHECK_ERROR(!strKey.get(), QSTATUS_OUTOFMEMORY, SMTP_ERROR_AUTH);
				status = encoder.encode(
					reinterpret_cast<unsigned char*>(strKey.get()),
					strlen(strKey.get()), &p, &nLen);
				CHECK_QSTATUS_ERROR(SMTP_ERROR_AUTH);
				malloc_ptr<unsigned char> ap(p);
				string_ptr<STRING> strCommand(
					concat(reinterpret_cast<char*>(p), nLen, "\r\n", -1));
				CHECK_ERROR(!strCommand.get(), QSTATUS_OUTOFMEMORY, SMTP_ERROR_AUTH);
				status = sendCommand(strCommand.get(), &nCode);
				CHECK_QSTATUS_ERROR_OR(SMTP_ERROR_AUTH);
				if (nCode == 2)
					nAuth = 0;
			}
		}
		if (nAuth & AUTH_PLAIN) {
			size_t nUserNameLen = strlen(strUserName.get());
			size_t nPasswordLen = strlen(strPassword.get());
			string_ptr<STRING> strKey(allocString(nUserNameLen*2 + nPasswordLen + 10));
			CHECK_ERROR(!strKey.get(), QSTATUS_OUTOFMEMORY, SMTP_ERROR_AUTH);
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
			
			unsigned char* p = 0;
			size_t nLen = 0;
			status = encoder.encode(
				reinterpret_cast<unsigned char*>(strKey.get()),
				strlen(strKey.get()), &p, &nLen);
			CHECK_QSTATUS_ERROR(SMTP_ERROR_AUTH);
			malloc_ptr<unsigned char> ap(p);
			string_ptr<STRING> strCommand(concat("AUTH PLAIN ", -1,
				reinterpret_cast<char*>(p), nLen, "\r\n", -1));
			CHECK_ERROR(!strCommand.get(), QSTATUS_OUTOFMEMORY, SMTP_ERROR_AUTH);
			status = sendCommand(strCommand.get(), &nCode);
			CHECK_QSTATUS_ERROR_OR(SMTP_ERROR_AUTH);
			if (nCode == 2)
				nAuth = 0;
		}
		if (nAuth & AUTH_LOGIN) {
			status = sendCommand("AUTH LOGIN\r\n", &nCode);
			CHECK_QSTATUS_ERROR_OR(SMTP_ERROR_AUTH);
			if (nCode == 3) {
				unsigned char* p = 0;
				size_t nLen = 0;
				status = encoder.encode(
					reinterpret_cast<unsigned char*>(strUserName.get()),
					strlen(strUserName.get()), &p, &nLen);
				CHECK_QSTATUS_ERROR(SMTP_ERROR_AUTH);
				malloc_ptr<unsigned char> ap(p);
				string_ptr<STRING> strCommand(concat(
					reinterpret_cast<char*>(p), nLen, "\r\n", -1));
				status = sendCommand(strCommand.get(), &nCode);
				CHECK_QSTATUS_ERROR_OR(SMTP_ERROR_AUTH);
				if (nCode == 3) {
					status = encoder.encode(
						reinterpret_cast<unsigned char*>(strPassword.get()),
						strlen(strPassword.get()), &p, &nLen);
					CHECK_QSTATUS_ERROR(SMTP_ERROR_AUTH);
					ap.reset(p);
					strCommand.reset(concat(
						reinterpret_cast<char*>(p), nLen, "\r\n", -1));
					status = sendCommand(strCommand.get(), &nCode);
					CHECK_QSTATUS_ERROR_OR(SMTP_ERROR_AUTH);
					if (nCode == 2)
						nAuth = 0;
				}
			}
		}
		CHECK_ERROR(nAuth != 0, QSTATUS_FAIL, SMTP_ERROR_AUTH);
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmsmtp::Smtp::disconnect()
{
	if (pSocket_) {
		unsigned int nCode = 0;
		sendCommand("QUIT\r\n", &nCode);
		delete pSocket_;
		pSocket_ = 0;
	}
	return QSTATUS_SUCCESS;
}

QSTATUS qmsmtp::Smtp::sendMessage(const SendMessageData& data)
{
	assert(data.pszEnvelopeFrom_);
	assert(data.ppszAddresses_);
	assert(data.nAddressSize_ != 0);
	assert(data.pszMessage_);
	
	DECLARE_QSTATUS();
	
	string_ptr<STRING> strCommand(
		concat("MAIL FROM: <", data.pszEnvelopeFrom_, ">\r\n"));
	CHECK_ERROR(!strCommand.get(),
		QSTATUS_OUTOFMEMORY, SMTP_ERROR_MAIL | SMTP_ERROR_OTHER);
	
	unsigned int nCode = 0;
	status = sendCommand(strCommand.get(), &nCode);
	CHECK_QSTATUS_ERROR_OR(SMTP_ERROR_MAIL);
	CHECK_ERROR(nCode != 2, QSTATUS_FAIL,
		SMTP_ERROR_MAIL | SMTP_ERROR_RESPONSE);
	
	for (size_t n = 0; n < data.nAddressSize_; ++n) {
		strCommand.reset(concat(
			"RCPT TO: <", data.ppszAddresses_[n], ">\r\n"));
		CHECK_ERROR(!strCommand.get(),
			QSTATUS_OUTOFMEMORY, SMTP_ERROR_RCPT | SMTP_ERROR_OTHER);
		status = sendCommand(strCommand.get(), &nCode);
		CHECK_QSTATUS_ERROR_OR(SMTP_ERROR_RCPT);
		CHECK_ERROR(nCode != 2, QSTATUS_FAIL,
			SMTP_ERROR_RCPT | SMTP_ERROR_RESPONSE);
	}
	
	typedef std::vector<SendData> SendDataList;
	SendDataList listSendData;
	const CHAR* p = data.pszMessage_;
	SendData sd = { p, 0 };
	for (size_t m = 0; m < data.nLength_; ++m, ++p) {
		if (*p == '.' && m > 1 && *(p - 1) == '\n' && *(p - 2) == '\r') {
			sd.nLength_ = p - sd.psz_ + 1;
			status = STLWrapper<SendDataList>(listSendData).push_back(sd);
			CHECK_QSTATUS_ERROR(SMTP_ERROR_DATA);
			sd.psz_ = p;
		}
	}
	sd.nLength_ = p - sd.psz_;
	status = STLWrapper<SendDataList>(listSendData).push_back(sd);
	CHECK_QSTATUS_ERROR(SMTP_ERROR_DATA);
	
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
	status = STLWrapper<SendDataList>(listSendData).push_back(sd);
	CHECK_QSTATUS_ERROR(SMTP_ERROR_DATA);
	
	status = sendCommand("DATA\r\n", &nCode);
	CHECK_QSTATUS_ERROR_OR(SMTP_ERROR_DATA);
	CHECK_ERROR(nCode != 3, QSTATUS_FAIL, SMTP_ERROR_DATA | SMTP_ERROR_RESPONSE);
	
	status = send(&listSendData[0], listSendData.size(), true, &nCode, 0);
	CHECK_QSTATUS_ERROR_OR(SMTP_ERROR_DATA);
	
	return QSTATUS_SUCCESS;
}

unsigned int qmsmtp::Smtp::getLastError() const
{
	return nError_;
}

const WCHAR* qmsmtp::Smtp::getLastErrorResponse() const
{
	return wstrErrorResponse_;
}

QSTATUS qmsmtp::Smtp::helo(unsigned int* pnAuth, bool* pbStartTls)
{
	assert(pnAuth);
	assert(pbStartTls);
	
	DECLARE_QSTATUS();
	
	*pnAuth = 0;
	*pbStartTls = false;
	
	string_ptr<WSTRING> wstrLocalHost;
	status = pSmtpCallback_->getLocalHost(&wstrLocalHost);
	CHECK_QSTATUS_ERROR(SMTP_ERROR_OTHER);
	string_ptr<STRING> strLocalHost;
	if (wstrLocalHost.get() && *wstrLocalHost.get()) {
		strLocalHost.reset(wcs2mbs(wstrLocalHost.get()));
		CHECK_ERROR(!strLocalHost.get(),
			QSTATUS_OUTOFMEMORY, SMTP_ERROR_OTHER | SMTP_ERROR_HELO);
	}
	else {
		strLocalHost.reset(allocString(256));
		CHECK_ERROR(!strLocalHost.get(),
			QSTATUS_OUTOFMEMORY, SMTP_ERROR_OTHER | SMTP_ERROR_HELO);
		::gethostname(strLocalHost.get(), 256);
	}
	
	bool bEHLO = true;
	string_ptr<STRING> strHelo(allocString(strlen(strLocalHost.get()) + 10));
	CHECK_ERROR(!strHelo.get(), QSTATUS_OUTOFMEMORY,
		SMTP_ERROR_OTHER | SMTP_ERROR_HELO);
	sprintf(strHelo.get(), "EHLO %s\r\n", strLocalHost.get());
	unsigned int nCode = 0;
	string_ptr<STRING> strResult;
	status = sendCommand(strHelo.get(), &nCode, &strResult);
	CHECK_QSTATUS_ERROR_OR(SMTP_ERROR_HELO);
	if (nCode != 2) {
		strResult.reset(0);
		sprintf(strHelo.get(), "HELO %s\r\n", strLocalHost.get());
		status = sendCommand(strHelo.get(), &nCode, &strResult);
		CHECK_QSTATUS_ERROR_OR(SMTP_ERROR_HELO);
		bEHLO = false;
	}
	CHECK_ERROR(nCode != 2, QSTATUS_FAIL, SMTP_ERROR_HELO | SMTP_ERROR_RESPONSE);
	
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
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmsmtp::Smtp::receive(unsigned int* pnCode)
{
	return receive(pnCode, 0);
}

QSTATUS qmsmtp::Smtp::receive(unsigned int* pnCode, STRING* pstrResponse)
{
	assert(pnCode);
	
	DECLARE_QSTATUS();
	
	CHECK_ERROR(!pSocket_, QSTATUS_FAIL, SMTP_ERROR_INVALIDSOCKET);
	
	StringBuffer<STRING> bufResponse(&status);
	CHECK_QSTATUS_ERROR(SMTP_ERROR_OTHER);
	
	char buf[1024];
	bool bEnd = false;
	do {
		int nSelect = Socket::SELECT_READ;
		status = pSocket_->select(&nSelect);
		CHECK_QSTATUS_ERROR(SMTP_ERROR_SELECT | pSocket_->getLastError());
		CHECK_ERROR(nSelect == 0, QSTATUS_FAIL, SMTP_ERROR_TIMEOUT);
		
		int nLen = sizeof(nLen);
		status = pSocket_->recv(buf, &nLen, 0);
		CHECK_QSTATUS_ERROR(SMTP_ERROR_RECEIVE | pSocket_->getLastError());
		CHECK_ERROR(nLen == 0, QSTATUS_FAIL, SMTP_ERROR_DISCONNECT);
		
		status = bufResponse.append(buf, nLen);
		CHECK_QSTATUS_ERROR(SMTP_ERROR_OTHER);
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
	
	status = setErrorResponse(bufResponse.getCharArray());
	CHECK_QSTATUS_ERROR(SMTP_ERROR_OTHER);
	
	*pnCode = bufResponse.get(0) - '0';
	if (pstrResponse)
		*pstrResponse = bufResponse.getString();
	
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmsmtp::Smtp::send(const SendData* pSendData, size_t nDataLen,
	bool bProgress, unsigned int* pnCode, qs::STRING* pstrResponse)
{
	assert(pSendData);
	assert(nDataLen > 0);
	assert(pnCode);
	
	DECLARE_QSTATUS();
	
	CHECK_ERROR(!pSocket_, QSTATUS_FAIL, SMTP_ERROR_INVALIDSOCKET);
	
	if (bProgress) {
		size_t nLen = 0;
		for (size_t n = 0; n < nDataLen; ++n)
			nLen += (pSendData + n)->nLength_;
		status = pSmtpCallback_->setRange(0, nLen);
		CHECK_QSTATUS_ERROR(SMTP_ERROR_SEND);
	}
	
	for (size_t n = 0; n < nDataLen; ++n) {
		const SendData& data = *(pSendData + n);
		size_t nTotal = 0;
		while (nTotal < data.nLength_) {
			int nSelect = Socket::SELECT_READ | Socket::SELECT_WRITE;
			status = pSocket_->select(&nSelect);
			CHECK_QSTATUS_ERROR(SMTP_ERROR_SELECT | pSocket_->getLastError());
			CHECK_ERROR(nSelect == 0, QSTATUS_FAIL, SMTP_ERROR_TIMEOUT);
			
			int nSend = QSMIN(size_t(2048), data.nLength_ - nTotal);
			status = pSocket_->send(data.psz_ + nTotal, &nSend, 0);
			CHECK_QSTATUS_ERROR(SMTP_ERROR_SEND | pSocket_->getLastError());
			nTotal += nSend;
			if (bProgress) {
				status = pSmtpCallback_->setPos(nTotal);
				CHECK_QSTATUS_ERROR(SMTP_ERROR_SEND);
			}
		}
	}
	if (bProgress) {
		status = pSmtpCallback_->setRange(0, 0);
		CHECK_QSTATUS_ERROR(SMTP_ERROR_SEND);
		status = pSmtpCallback_->setPos(0);
		CHECK_QSTATUS_ERROR(SMTP_ERROR_SEND);
	}
	
	return receive(pnCode, pstrResponse);
}

QSTATUS qmsmtp::Smtp::sendCommand(const CHAR* psz, unsigned int* pnCode)
{
	return sendCommand(psz, pnCode, 0);
}

QSTATUS qmsmtp::Smtp::sendCommand(const CHAR* psz,
	unsigned int* pnCode, STRING* pstrResponse)
{
	SendData data = { psz, strlen(psz) };
	return send(&data, 1, false, pnCode, pstrResponse);
}

QSTATUS qmsmtp::Smtp::setErrorResponse(const CHAR* pszErrorResponse)
{
	freeWString(wstrErrorResponse_);
	wstrErrorResponse_ = mbs2wcs(pszErrorResponse);
	if (!wstrErrorResponse_)
		return QSTATUS_OUTOFMEMORY;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmsmtp::Smtp::getAuthMethods(unsigned int* pnAuth)
{
	assert(pnAuth);
	
	DECLARE_QSTATUS();
	
	*pnAuth = 0;
	
	string_ptr<WSTRING> wstrAuthMethods;
	status = pSmtpCallback_->getAuthMethods(&wstrAuthMethods);
	CHECK_QSTATUS();
	
	struct {
		const WCHAR* pwsz_;
		Auth auth_;
	} methods[] = {
		{ L"LOGIN",		AUTH_LOGIN		},
		{ L"PLAIN",		AUTH_PLAIN		},
		{ L"CRAM-MD5",	AUTH_CRAMMD5	}
	};
	
	const WCHAR* p = wcstok(wstrAuthMethods.get(), L" ");
	while (p) {
		for (int n = 0; n < countof(methods); ++n) {
			if (wcscmp(p, methods[n].pwsz_) == 0) {
				*pnAuth |= methods[n].auth_;
				break;
			}
		}
		p = wcstok(0, L" ");
	}
	
	if (*pnAuth == 0)
		*pnAuth = 0xffffffff;
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * SmtpCallback
 *
 */

qmsmtp::SmtpCallback::~SmtpCallback()
{
}
