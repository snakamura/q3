/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsconv.h>

#include <cstdio>

#include "nntp.h"

using namespace qmnntp;
using namespace qs;


/****************************************************************************
 *
 * Nntp
 *
 */

#define NNTP_ERROR(e) \
	do { \
		nError_ = e; \
		return false; \
	} while (false)

#define NNTP_ERROR_SOCKET(e) \
	do { \
		nError_ = e | pSocket_->getLastError(); \
		return false; \
	} while (false)

#define NNTP_ERROR_OR(e) \
	do { \
		nError_ |= e; \
		return false; \
	} while (false)

qmnntp::Nntp::Nntp(long nTimeout,
				   qs::SocketCallback* pSocketCallback,
				   qs::SSLSocketCallback* pSSLSocketCallback,
				   NntpCallback* pNntpCallback,
				   qs::Logger* pLogger) :
	nTimeout_(nTimeout),
	pSocketCallback_(pSocketCallback),
	pSSLSocketCallback_(pSSLSocketCallback),
	pNntpCallback_(pNntpCallback),
	pLogger_(pLogger),
	nCount_(0),
	nFirst_(0),
	nLast_(0),
	nError_(NNTP_ERROR_SUCCESS)
{
}

qmnntp::Nntp::~Nntp()
{
}

bool qmnntp::Nntp::connect(const WCHAR* pwszHost,
						   short nPort,
						   bool bSsl)
{
	assert(pwszHost);
	
	std::auto_ptr<Socket> pSocket(new Socket(
		nTimeout_, pSocketCallback_, pLogger_));
	if (!pSocket->connect(pwszHost, nPort))
		NNTP_ERROR(NNTP_ERROR_CONNECT | pSocket->getLastError());
	
	if (bSsl) {
		SSLSocketFactory* pFactory = SSLSocketFactory::getFactory();
		if (!pFactory)
			NNTP_ERROR(NNTP_ERROR_SSL);
		
		std::auto_ptr<SSLSocket> pSSLSocket(pFactory->createSSLSocket(
			pSocket.get(), true, pSSLSocketCallback_, pLogger_));
		if (!pSSLSocket.get())
			NNTP_ERROR(NNTP_ERROR_SSL);
		
		pSocket.release();
		pSocket_ = pSSLSocket;
	}
	else {
		pSocket_ = pSocket;
	}
	
	unsigned int nCode = 0;
	if (!receive(&nCode))
		NNTP_ERROR_OR(NNTP_ERROR_GREETING);
	else if (nCode/100 != 2)
		NNTP_ERROR(NNTP_ERROR_GREETING | NNTP_ERROR_RESPONSE);
	
	if (!sendCommand("MODE READER\r\n", &nCode))
		NNTP_ERROR_OR(NNTP_ERROR_MODEREADER);
	
	wstring_ptr wstrUserName;
	wstring_ptr wstrPassword;
	if (!pNntpCallback_->getUserInfo(&wstrUserName, &wstrPassword))
		NNTP_ERROR(NNTP_ERROR_AUTHINFO);
	if (wstrUserName.get() && *wstrUserName.get()) {
		pNntpCallback_->authenticating();
		
		string_ptr strCommand;
		unsigned int nCode = 0;
		
		string_ptr strUserName(wcs2mbs(wstrUserName.get()));
		strCommand = concat("AUTHINFO USER ", strUserName.get(), "\r\n");
		if (!sendCommand(strCommand.get(), &nCode))
			NNTP_ERROR_OR(NNTP_ERROR_AUTHINFO);
		else if (nCode != 381)
			NNTP_ERROR(NNTP_ERROR_AUTHINFO | NNTP_ERROR_RESPONSE);
		
		string_ptr strPassword(wcs2mbs(wstrPassword.get()));
		strCommand = concat("AUTHINFO PASS ", strPassword.get(), "\r\n");
		if (!sendCommand(strCommand.get(), &nCode))
			NNTP_ERROR_OR(NNTP_ERROR_AUTHINFO);
		else if (nCode != 281)
			NNTP_ERROR(NNTP_ERROR_AUTHINFO | NNTP_ERROR_RESPONSE);
	}
	
	return true;
}

void qmnntp::Nntp::disconnect()
{
	if (pSocket_.get()) {
		bool bQuit = true;
		switch (nError_ & NNTP_ERROR_MASK_LOWLEVEL) {
		case NNTP_ERROR_CONNECT:
		case NNTP_ERROR_INVALIDSOCKET:
		case NNTP_ERROR_TIMEOUT:
		case NNTP_ERROR_SELECT:
		case NNTP_ERROR_DISCONNECT:
		case NNTP_ERROR_RECEIVE:
		case NNTP_ERROR_SEND:
			bQuit = false;
			break;
		}
		switch (nError_ & Socket::SOCKET_ERROR_MASK_SOCKET) {
		case Socket::SOCKET_ERROR_SOCKET:
		case Socket::SOCKET_ERROR_LOOKUPNAME:
		case Socket::SOCKET_ERROR_CONNECTTIMEOUT:
		case Socket::SOCKET_ERROR_CONNECT:
			bQuit = false;
			break;
		}
		if (bQuit) {
			pSocket_->setTimeout(1);
			
			unsigned int nCode = 0;
			sendCommand("QUIT\r\n", &nCode);
		}
		pSocket_.reset(0);
	}
}

const WCHAR* qmnntp::Nntp::getGroup() const
{
	return wstrGroup_.get();
}

unsigned int qmnntp::Nntp::getEstimatedCount() const
{
	return nCount_;
}

unsigned int qmnntp::Nntp::getFirst() const
{
	return nFirst_;
}

unsigned int qmnntp::Nntp::getLast() const
{
	return nLast_;
}

bool qmnntp::Nntp::group(const WCHAR* pwszGroup)
{
	assert(pwszGroup);
	
	wstring_ptr wstrGroup(allocWString(pwszGroup));
	string_ptr strGroup(wcs2mbs(pwszGroup));
	
	string_ptr strCommand(concat("GROUP ", strGroup.get(), "\r\n"));
	
	unsigned int nCode = 0;
	string_ptr strResponse;
	if (!sendCommand(strCommand.get(), &nCode, &strResponse))
		NNTP_ERROR_OR(NNTP_ERROR_GROUP);
	else if (nCode != 211)
		NNTP_ERROR(NNTP_ERROR_GROUP | NNTP_ERROR_RESPONSE);
	
	const CHAR* p = strResponse.get() + 4;
	CHAR* pEnd = 0;
	unsigned int nCount = strtol(p, &pEnd, 10);
	if (*pEnd != ' ')
		NNTP_ERROR(NNTP_ERROR_GROUP | NNTP_ERROR_RESPONSE);
	
	p = pEnd + 1;
	unsigned int nFirst = strtol(p, &pEnd, 10);
	if (*pEnd != ' ')
		NNTP_ERROR(NNTP_ERROR_GROUP | NNTP_ERROR_RESPONSE);
	
	p = pEnd + 1;
	unsigned int nLast = strtol(p, &pEnd, 10);
	if (*pEnd != ' ')
		NNTP_ERROR(NNTP_ERROR_GROUP | NNTP_ERROR_RESPONSE);
	
	wstrGroup_ = wstrGroup;
	nCount_ = nCount;
	nFirst_ = nFirst;
	nLast_ = nLast;
	
	return true;
}

bool qmnntp::Nntp::getMessage(unsigned int n,
							  GetMessageFlag flag,
							  xstring_ptr* pstrMessage)
{
	return getMessage(n, 0, flag, pstrMessage);
}

bool qmnntp::Nntp::getMessage(const WCHAR* pwszMessageId,
							  GetMessageFlag flag,
							  xstring_ptr* pstrMessage)
{
	assert(pwszMessageId);
	return getMessage(-1, pwszMessageId, flag, pstrMessage);
}

bool qmnntp::Nntp::getMessagesData(unsigned int nStart,
								   unsigned int nEnd,
								   std::auto_ptr<MessagesData>* ppMessageData)
{
	assert(nStart <= nEnd);
	assert(ppMessageData);
	
	CHAR szCommand[128];
	sprintf(szCommand, "XOVER %u-%u\r\n", nStart, nEnd);
	
	const CHAR* pszCodes[] = {
		"224"
	};
	
	unsigned int nCode = 0;
	string_ptr strResponse;
	xstring_ptr strContent;
	if (!sendCommand(szCommand, pszCodes, countof(pszCodes),
		&nCode, &strResponse, &strContent))
		NNTP_ERROR_OR(NNTP_ERROR_XOVER);
	else if (nCode != 224 && nCode != 420)
		NNTP_ERROR(NNTP_ERROR_XOVER | NNTP_ERROR_RESPONSE);
	
	std::auto_ptr<MessagesData> pData(new MessagesData());
	
	if (nCode == 224) {
		if (!pData->setData(strContent))
			NNTP_ERROR(NNTP_ERROR_XOVER | NNTP_ERROR_PARSE);
	}
	
	*ppMessageData = pData;
	
	return true;
}

bool qmnntp::Nntp::postMessage(const CHAR* pszMessage,
							   size_t nLen)
{
	assert(pszMessage);
	
	if (nLen == -1)
		nLen = strlen(pszMessage);
	
	typedef std::vector<SendData> SendDataList;
	SendDataList listSendData;
	
	const CHAR* p = pszMessage;
	SendData sd = { p, 0 };
	for (size_t m = 0; m < nLen; ++m, ++p) {
		if (*p == '.' && m > 1 && *(p - 1) == '\n' && *(p - 2) == '\r') {
			sd.nLength_ = p - sd.psz_ + 1;
			listSendData.push_back(sd);
			sd.psz_ = p;
		}
	}
	sd.nLength_ = p - sd.psz_;
	listSendData.push_back(sd);
	
	if (nLen > 2 && pszMessage[nLen - 2] == '\r' && pszMessage[nLen - 1] == '\n') {
		sd.psz_ = ".\r\n";
		sd.nLength_ = 3;
	}
	else {
		sd.psz_ = "\r\n.\r\n";
		sd.nLength_ = 5;
	}
	listSendData.push_back(sd);
	
	unsigned int nCode = 0;
	if (!sendCommand("POST\r\n", &nCode))
		NNTP_ERROR_OR(NNTP_ERROR_POST);
	else if (nCode != 340)
		NNTP_ERROR(NNTP_ERROR_POST | NNTP_ERROR_RESPONSE);
	
	if (!send(&listSendData[0], listSendData.size(), 0, 0, &nCode, 0, 0))
		NNTP_ERROR_OR(NNTP_ERROR_POST);
	else if (nCode != 240)
		NNTP_ERROR(NNTP_ERROR_POST | NNTP_ERROR_RESPONSE);
	
	return true;
}

unsigned int qmnntp::Nntp::getLastError() const
{
	return nError_;
}

const WCHAR* qmnntp::Nntp::getLastErrorResponse() const
{
	return wstrErrorResponse_.get();
}

bool qmnntp::Nntp::getMessage(unsigned int n,
							  const WCHAR* pwszMessageId,
							  GetMessageFlag flag,
							  xstring_ptr* pstrMessage)
{
	assert((n != -1 && !pwszMessageId) || (n == -1 && pwszMessageId));
	assert(pstrMessage);
	
	struct {
		const CHAR* pszCommand_;
		unsigned int nCode_;
		Error error_;
	} types[] = {
		{ "ARTICLE ",	220,	NNTP_ERROR_ARTICLE	},
		{ "HEAD ",		221,	NNTP_ERROR_HEAD		},
		{ "BODY ",		222,	NNTP_ERROR_BODY		}
	};
	
	Error error = types[flag].error_;
	unsigned int nCodeNotFound = 0;
	
	StringBuffer<STRING> buf;
	buf.append(types[flag].pszCommand_);
	
	if (pwszMessageId) {
		string_ptr strMessageId(wcs2mbs(pwszMessageId));
		buf.append("<");
		buf.append(strMessageId.get());
		buf.append(">");
		
		nCodeNotFound = 430;
	}
	else {
		CHAR sz[32];
		sprintf(sz, "%u", n);
		buf.append(sz);
		
		nCodeNotFound = 423;
	}
	
	buf.append("\r\n");
	
	const CHAR* pszCodes[] = {
		"220",
		"221",
		"222"
	};
	
	unsigned int nCode = 0;
	string_ptr strResponse;
	xstring_ptr strContent;
	if (!sendCommand(buf.getCharArray(), pszCodes,
		countof(pszCodes), &nCode, &strResponse, &strContent))
		NNTP_ERROR_OR(error);
	else if (nCode != types[flag].nCode_ && nCode != nCodeNotFound)
		NNTP_ERROR(error | NNTP_ERROR_RESPONSE);
	
	*pstrMessage = strContent;
	
	return true;
}

bool qmnntp::Nntp::receive(unsigned int* pnCode)
{
	return receive(0, 0, pnCode, 0, 0);
}

bool qmnntp::Nntp::receive(unsigned int* pnCode,
						   string_ptr* pstrResponse)
{
	return receive(0, 0, pnCode, pstrResponse, 0);
}

bool qmnntp::Nntp::receive(const CHAR* pszMultilineCodes[],
						   size_t nCodeCount,
						   unsigned int* pnCode,
						   string_ptr* pstrResponse,
						   xstring_ptr* pstrContent)
{
	assert((!pszMultilineCodes && !pstrContent && nCodeCount == 0) ||
		(pszMultilineCodes && pstrContent && nCodeCount != 0));
	assert(pnCode);
	
	if (!pSocket_.get())
		NNTP_ERROR(NNTP_ERROR_INVALIDSOCKET);
	
	StringBuffer<STRING> bufResponse;
	XStringBuffer<XSTRING> bufContent;
	
	bool bContent = false;
	bool bMultiLine = pstrContent != 0;
	char buf[RECEIVE_BLOCK_SIZE];
	State state = STATE_LF1;
	bool bEnd = false;
	do {
		int nSelect = pSocket_->select(Socket::SELECT_READ);
		if (nSelect == -1)
			NNTP_ERROR_SOCKET(NNTP_ERROR_SELECT);
		else if (nSelect == 0)
			NNTP_ERROR(NNTP_ERROR_TIMEOUT);
		
		if (bContent) {
			XStringBufferLock<XSTRING> lock(&bufContent, sizeof(buf));
			CHAR* pLock = lock.get();
			if (!pLock)
				NNTP_ERROR(NNTP_ERROR_OTHER);
			
			size_t nLen = pSocket_->recv(pLock, sizeof(buf), 0);
			if (nLen == -1)
				NNTP_ERROR_SOCKET(NNTP_ERROR_RECEIVE);
			else if (nLen == 0)
				NNTP_ERROR(NNTP_ERROR_DISCONNECT);
			
			if (!checkContent(pLock, &nLen, &state))
				NNTP_ERROR(NNTP_ERROR_PARSE);
			
			lock.unlock(nLen);
		}
		else {
			size_t nLen = pSocket_->recv(buf, sizeof(buf), 0);
			if (nLen == -1)
				NNTP_ERROR_SOCKET(NNTP_ERROR_RECEIVE);
			else if (nLen == 0)
				NNTP_ERROR(NNTP_ERROR_DISCONNECT);
			
			if (bMultiLine) {
				assert(!bContent);
				bufResponse.append(buf, nLen);
				
				const CHAR* pRes = bufResponse.getCharArray();
				const CHAR* p = strstr(pRes, "\r\n");
				if (p) {
					bContent = true;
					
					size_t nContentLen = bufResponse.getLength() - (p - pRes) - 2;
					char* pContent = buf + nLen - nContentLen;
					if (!checkContent(pContent, &nContentLen, &state))
						NNTP_ERROR(NNTP_ERROR_PARSE);
					if (!bufContent.append(pContent, nContentLen))
						NNTP_ERROR(NNTP_ERROR_OTHER);
					bufResponse.remove(p + 2 - pRes, bufResponse.getLength());
				}
				if (bufResponse.getLength() >= 3) {
					size_t n = 0;
					while (n < nCodeCount) {
						if (strncmp(pRes, pszMultilineCodes[n], 3) == 0)
							break;
						++n;
					}
					if (n == nCodeCount)
						bMultiLine = false;
				}
			}
			else {
				bufResponse.append(buf, nLen);
			}
		}
		
		if (bMultiLine)
			pNntpCallback_->setPos(bufContent.getLength());
		
		if (bMultiLine)
			bEnd = state == STATE_LF2;
		else
			bEnd = bufResponse.getLength() >= 2 &&
				strncmp(bufResponse.getCharArray() + bufResponse.getLength() - 2, "\r\n", 2) == 0;
	} while (!bEnd);
	
	const CHAR* p = bufResponse.getCharArray();
	if (bufResponse.getLength() < 3 || !isdigit(*p) ||
		!isdigit(*(p + 1)) || !isdigit(*(p + 2)))
		NNTP_ERROR(NNTP_ERROR_RESPONSE);
	*pnCode = (*p - '0')*100 + (*(p + 1) - '0')*10 + (*(p + 2) - '0');
	
	if (*p != '1' && *p != '2' && *p != '3')
		setErrorResponse(bufResponse.getCharArray());
	
	if (pstrResponse)
		*pstrResponse = bufResponse.getString();
	if (pstrContent && bMultiLine) {
		bufContent.remove(bufContent.getLength() - 2, bufContent.getLength());
		*pstrContent = bufContent.getXString();
		if (!pstrContent->get())
			return false;
	}
	
	nError_ = NNTP_ERROR_SUCCESS;
	
	return true;
}

bool qmnntp::Nntp::sendCommand(const CHAR* pszCommand,
							   unsigned int* pnCode)
{
	return sendCommand(pszCommand, 0, 0, pnCode, 0, 0);
}

bool qmnntp::Nntp::sendCommand(const CHAR* pszCommand,
							   unsigned int* pnCode,
							   string_ptr* pstrResponse)
{
	return sendCommand(pszCommand, 0, 0, pnCode, pstrResponse, 0);
}

bool qmnntp::Nntp::sendCommand(const CHAR* pszCommand,
							   const CHAR* pszMultilineCodes[],
							   size_t nCodeCount,
							   unsigned int* pnCode,
							   string_ptr* pstrResponse,
							   xstring_ptr* pstrContent)
{
	SendData sd = {
		pszCommand,
		strlen(pszCommand)
	};
	return send(&sd, 1, pszMultilineCodes,
		nCodeCount, pnCode, pstrResponse, pstrContent);
}

bool qmnntp::Nntp::send(const SendData* pSendData,
						size_t nDataLen,
						const CHAR* pszMultilineCodes[],
						size_t nCodeCount,
						unsigned int* pnCode,
						string_ptr* pstrResponse,
						xstring_ptr* pstrContent)
{
	assert(pSendData);
	assert(nDataLen > 0);
	
	if (!pSocket_.get())
		NNTP_ERROR(NNTP_ERROR_INVALIDSOCKET);
	
//	if (bProgress) {
//		size_t nLen = 0;
//		for (size_t n = 0; n < nDataLen; ++n)
//			nLen += (pSendData + n)->nLength_;
//		pNntpCallback_->setRange(0, nLen);
//	}
	
	for (size_t n = 0; n < nDataLen; ++n) {
		const SendData& data = *(pSendData + n);
		size_t nTotal = 0;
		while (nTotal < data.nLength_) {
			int nSelect = pSocket_->select(Socket::SELECT_READ | Socket::SELECT_WRITE);
			if (nSelect == -1)
				NNTP_ERROR_SOCKET(NNTP_ERROR_SELECT);
			else if (nSelect == 0)
				NNTP_ERROR(NNTP_ERROR_TIMEOUT);
			
			size_t nSend = pSocket_->send(data.psz_ + nTotal,
				QSMIN(size_t(SEND_BLOCK_SIZE), data.nLength_ - nTotal), 0);
			if (nSend == -1)
				NNTP_ERROR_SOCKET(NNTP_ERROR_SEND);
			nTotal += nSend;
//			if (bProgress)
//				pNntpCallback_->setPos(nTotal);
		}
	}
//	if (bProgress) {
//		pNntpCallback_->setRange(0, 0);
//		pNntpCallback_->setPos(0);
//	}
	
	return receive(pszMultilineCodes, nCodeCount, pnCode, pstrResponse, pstrContent);
}

void qmnntp::Nntp::setErrorResponse(const CHAR* pszErrorResponse)
{
	wstrErrorResponse_ = mbs2wcs(pszErrorResponse);
}

bool qmnntp::Nntp::checkContent(CHAR* psz,
								size_t* pnLen,
								State* pState)
{
	assert(psz);
	assert(pnLen);
	assert(pState);
	
	CHAR* p = psz;
	CHAR* pOrg = p;
	size_t nLen = *pnLen;
	
	while (nLen != 0) {
		CHAR c = *psz;
		
		switch (*pState) {
		case STATE_NONE:
			if (c == '\r')
				*pState = STATE_CR1;
			break;
		case STATE_CR1:
			if (c == '\n')
				*pState = STATE_LF1;
			else
				*pState = STATE_NONE;
			break;
		case STATE_LF1:
			if (c == '.') {
				c = '\0';
				*pState = STATE_PERIOD;
			}
			else if (c == '\r') {
				*pState = STATE_CR1;
			}
			else {
				*pState = STATE_NONE;
			}
			break;
		case STATE_PERIOD:
			if (c == '\r')
				*pState = STATE_CR2;
			else
				*pState = STATE_NONE;
			break;
		case STATE_CR2:
			if (c == '\n')
				*pState = STATE_LF2;
			else
				*pState = STATE_NONE;
			break;
		case STATE_LF2:
			return false;
		default:
			assert(false);
			return false;
		}
		if (c != '\0') {
			if (p != psz)
				*p = c;
			++p;
		}
		
		++psz;
		--nLen;
	}
	
	*pnLen = p - pOrg;
	
	return true;
}


/****************************************************************************
 *
 * NntpCallback
 *
 */

qmnntp::NntpCallback::~NntpCallback()
{
}


/****************************************************************************
 *
 * MessagesData
 *
 */

qmnntp::MessagesData::MessagesData()
{
}

qmnntp::MessagesData::~MessagesData()
{
}

size_t qmnntp::MessagesData::getCount() const
{
	return listItem_.size();
}

const MessagesData::Item& qmnntp::MessagesData::getItem(size_t n) const
{
	assert(n < listItem_.size());
	return listItem_[n];
}

bool qmnntp::MessagesData::setData(xstring_ptr strData)
{
	assert(strData.get());
	
	CHAR* p = strData.get();
	
	while (*p) {
		Item item = { 0 };
		int n = 0;
		CHAR* pEnd = 0;
		
		item.nId_ = strtol(p, &pEnd, 10);
		if (*pEnd != '\t')
			return false;
		p = pEnd + 1;
		
		const CHAR** pp[] = {
			&item.pszSubject_,
			&item.pszFrom_,
			&item.pszDate_,
			&item.pszMessageId_,
			&item.pszReferences_
		};
		for (n = 0; n < countof(pp); ++n) {
			*pp[n] = p;
			p = strchr(p, '\t');
			if (!p)
				return false;
			*p = '\0';
			++p;
		}
		
		unsigned int* pn[] = {
			&item.nBytes_,
			&item.nLine_
		};
		for (n = 0; n < countof(pn); ++n) {
			*pn[n] = strtol(p, &pEnd, 10);
			while (*pEnd == ' ')
				++pEnd;
			if (n == countof(pn) - 1) {
				if (*pEnd != '\t' && *pEnd != '\r' && *p != '\0')
					return false;
			}
			else {
				if (*pEnd != '\t')
					return false;
				p = pEnd + 1;
			}
		}
		
		listItem_.push_back(item);
		
		p = strstr(p, "\r\n");
		if (!p)
			break;
		p += 2;
	}
	
	strData_ = strData;
	
	return true;
}
