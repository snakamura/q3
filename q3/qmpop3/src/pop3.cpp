/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsconv.h>
#include <qsmd5.h>
#include <qsmime.h>

#include <algorithm>
#include <stdio.h>

#include "pop3.h"

using namespace qmpop3;
using namespace qs;


/****************************************************************************
 *
 * Pop3
 *
 */

#define POP3_ERROR(e) \
	do { \
		nError_ = e; \
		return false; \
	} while (false)

#define POP3_ERROR_SOCKET(e) \
	do { \
		nError_ = e | pSocket_->getLastError(); \
		return false; \
	} while (false)

#define POP3_ERROR_OR(e) \
	do { \
		nError_ |= e; \
		return false; \
	} while (false)

const CHAR* qmpop3::Pop3::pszOk__ = "+OK";
const CHAR* qmpop3::Pop3::pszErr__ = "-ERR";

qmpop3::Pop3::Pop3(long nTimeout,
				   qs::SocketCallback* pSocketCallback,
				   qs::SSLSocketCallback* pSSLSocketCallback,
				   Pop3Callback* pPop3Callback,
				   qs::Logger* pLogger) :
	nTimeout_(nTimeout),
	pSocketCallback_(pSocketCallback),
	pSSLSocketCallback_(pSSLSocketCallback),
	pPop3Callback_(pPop3Callback),
	pLogger_(pLogger),
	nCount_(0),
	nError_(POP3_ERROR_SUCCESS)
{
}

qmpop3::Pop3::~Pop3()
{
}

bool qmpop3::Pop3::connect(const WCHAR* pwszHost,
						   short nPort,
						   bool bApop,
						   Secure secure)
{
	assert(pwszHost);
	
	std::auto_ptr<Socket> pSocket(new Socket(
		nTimeout_, pSocketCallback_, pLogger_));
	
	if (!pSocket->connect(pwszHost, nPort))
		POP3_ERROR(POP3_ERROR_CONNECT | pSocket->getLastError());
	
	if (secure == SECURE_SSL) {
		SSLSocketFactory* pFactory = SSLSocketFactory::getFactory();
		if (!pFactory)
			POP3_ERROR(POP3_ERROR_SSL);
		
		std::auto_ptr<SSLSocket> pSSLSocket = pFactory->createSSLSocket(
			pSocket.get(), true, pSSLSocketCallback_, pLogger_);
		if (!pSSLSocket.get())
			POP3_ERROR(POP3_ERROR_SSL);
		
		pSocket.release();
		pSocket_ = pSSLSocket;
	}
	else {
		pSocket_ = pSocket;
	}
	
	string_ptr strGreeting;
	if (!receive(&strGreeting))
		POP3_ERROR_OR(POP3_ERROR_GREETING);
	
	if (secure == SECURE_STARTTLS) {
		if (!sendCommand("STLS\r\n"))
			POP3_ERROR_OR(POP3_ERROR_STLS);
		
		SSLSocketFactory* pFactory = SSLSocketFactory::getFactory();
		if (!pFactory)
			POP3_ERROR(POP3_ERROR_SSL);
		
		std::auto_ptr<SSLSocket> pSSLSocket(pFactory->createSSLSocket(
			static_cast<Socket*>(pSocket_.get()), true, pSSLSocketCallback_, pLogger_));
		if (!pSSLSocket.get())
			POP3_ERROR(POP3_ERROR_SSL);
		pSocket_.release();
		pSocket_ = pSSLSocket;
	}
	
	wstring_ptr wstrUserName;
	wstring_ptr wstrPassword;
	if (!pPop3Callback_->getUserInfo(&wstrUserName, &wstrPassword))
		POP3_ERROR(POP3_ERROR_USER | POP3_ERROR_OTHER);
	string_ptr strUserName(wcs2mbs(wstrUserName.get()));
	string_ptr strPassword(wcs2mbs(wstrPassword.get()));
	
	pPop3Callback_->authenticating();
	
	if (bApop) {
		CHAR szDigestString[128] = "";
		const CHAR* pBegin = strchr(strGreeting.get(), '<');
		if (pBegin) {
			const CHAR* pEnd = strchr(pBegin + 1, '>');
			if (pEnd) {
				string_ptr strApop(allocString(pEnd - pBegin + strlen(strPassword.get()) + 2));
				CHAR* p = strApop.get();
				strncpy(p, pBegin, pEnd - pBegin + 1);
				p += pEnd - pBegin + 1;
				strcpy(p, strPassword.get());
				
				MD5::md5ToString(reinterpret_cast<const unsigned char*>(strApop.get()),
					strlen(strApop.get()), szDigestString);
			}
		}
		if (szDigestString[0] == '\0')
			POP3_ERROR(POP3_ERROR_GENERATEDIGEST | POP3_ERROR_APOP);
		
		string_ptr strApop(allocString(strlen(strUserName.get()) + 128 + 10));
		sprintf(strApop.get(), "APOP %s %s\r\n", strUserName.get(), szDigestString);
		if (!sendCommand(strApop.get()))
			POP3_ERROR_OR(POP3_ERROR_APOP);
	}
	else {
		string_ptr strUser(allocString(strlen(strUserName.get()) + 10));
		sprintf(strUser.get(), "USER %s\r\n", strUserName.get());
		if (!sendCommand(strUser.get()))
			POP3_ERROR_OR(POP3_ERROR_USER);
		
		string_ptr strPass(allocString(strlen(strPassword.get()) + 10));
		sprintf(strPass.get(), "PASS %s\r\n", strPassword.get());
		if (!sendCommand(strPass.get()))
			POP3_ERROR_OR(POP3_ERROR_PASS);
	}
	pPop3Callback_->setPassword(wstrPassword.get());
	
	string_ptr strStat;
	if (!sendCommand("STAT\r\n", &strStat))
		POP3_ERROR_OR(POP3_ERROR_STAT);
	
	const CHAR* pStat = strStat.get();
	int n = 0;
	for (n = 0; n < 3; ++n) {
		CHAR* pEnd = strchr(pStat, ' ');
		if (n != 2 && !pEnd)
			break;
		if (n == 1) {
			*pEnd = '\0';
			CHAR* p = 0;
			nCount_ = strtol(pStat, &p, 10);
			if (*p)
				POP3_ERROR(POP3_ERROR_PARSE | POP3_ERROR_STAT);
		}
		pStat= pEnd + 1;
	}
	if (n != 3)
		POP3_ERROR(POP3_ERROR_PARSE | POP3_ERROR_STAT);
	
	nError_ = POP3_ERROR_SUCCESS;
	
	return true;
}

void qmpop3::Pop3::disconnect()
{
	if (pSocket_.get()) {
		bool bQuit = true;
		switch (nError_ & POP3_ERROR_MASK_LOWLEVEL) {
		case POP3_ERROR_CONNECT:
		case POP3_ERROR_INVALIDSOCKET:
		case POP3_ERROR_TIMEOUT:
		case POP3_ERROR_SELECT:
		case POP3_ERROR_DISCONNECT:
		case POP3_ERROR_RECEIVE:
		case POP3_ERROR_SEND:
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
		if (bQuit)
			sendCommand("QUIT\r\n");
		pSocket_.reset(0);
	}
}

unsigned int qmpop3::Pop3::getMessageCount() const
{
	return nCount_;
}

bool qmpop3::Pop3::getMessage(unsigned int nMsg,
							  unsigned int nMaxLine,
							  xstring_size_ptr* pstrMessage,
							  unsigned int nEstimatedSize)
{
	assert(pstrMessage);
	
	if (nMsg >= getMessageCount())
		POP3_ERROR(POP3_ERROR_RETR | POP3_ERROR_RESPONSE);
	
	CHAR szRetr[128];
	if (nMaxLine == 0xffffffff)
		sprintf(szRetr, "RETR %d\r\n", nMsg + 1);
	else
		sprintf(szRetr, "TOP %d %d\r\n", nMsg + 1, nMaxLine);
	
	if (nEstimatedSize != -1)
		pPop3Callback_->setRange(0, nEstimatedSize);
	
	string_ptr strResponse;
	xstring_size_ptr strContent;
	if (!sendCommand(szRetr, &strResponse, &strContent,
		nEstimatedSize != -1 ? nEstimatedSize : 0))
		POP3_ERROR_OR(nMaxLine == 0xffffffff ? POP3_ERROR_RETR : POP3_ERROR_TOP);
	
	*pstrMessage = strContent;
	
	nError_ = POP3_ERROR_SUCCESS;
	
	return true;
}

bool qmpop3::Pop3::getMessageSize(unsigned int nMsg,
								  unsigned int* pnSize)
{
	assert(pnSize);
	
	*pnSize = -1;
	
	CHAR szList[128];
	sprintf(szList, "LIST %d\r\n", nMsg + 1);
	
	string_ptr strResponse;
	if (!sendCommand(szList, &strResponse))
		POP3_ERROR_OR(POP3_ERROR_LIST);
	
	const CHAR* p = strchr(strResponse.get(), ' ');
	if (!p)
		POP3_ERROR(POP3_ERROR_LIST | POP3_ERROR_PARSE);
	p = strchr(p + 1, ' ');
	if (!p)
		POP3_ERROR(POP3_ERROR_LIST | POP3_ERROR_PARSE);
	++p;
	CHAR* pEnd = strstr(p, "\r\n");
	if (!pEnd)
		POP3_ERROR(POP3_ERROR_LIST | POP3_ERROR_PARSE);
	// Skip whitespaces for compatibility with broken servers.
	while (pEnd - 1 > p && *(pEnd - 1) == ' ')
		--pEnd;
	*pEnd = '\0';
	CHAR* pTemp = 0;
	*pnSize = strtol(p, &pTemp, 10);
	if (*pTemp != '\0')
		POP3_ERROR(POP3_ERROR_LIST | POP3_ERROR_PARSE);
	
	nError_ = POP3_ERROR_SUCCESS;
	
	return true;
}

bool qmpop3::Pop3::getMessageSizes(MessageSizeList* pList)
{
	assert(pList);
	assert(pList->empty());
	
	pList->reserve(nCount_);
	
	string_ptr strResponse;
	xstring_size_ptr strContent;
	
	if (!sendCommand("LIST\r\n", &strResponse, &strContent, 0))
		POP3_ERROR_OR(POP3_ERROR_LIST);
	
	const CHAR* p = strContent.get();
	while (*p) {
		p = strchr(p, ' ');
		if (!p)
			POP3_ERROR(POP3_ERROR_LIST | POP3_ERROR_PARSE);
		++p;
		CHAR* pEnd = strstr(p, "\r\n");
		if (!pEnd)
			POP3_ERROR(POP3_ERROR_LIST | POP3_ERROR_PARSE);
		// Skip whitespaces for compatibility with broken servers.
		while (pEnd - 1 > p && *(pEnd - 1) == ' ')
			--pEnd;
		*pEnd = '\0';
		
		CHAR* pTemp = 0;
		unsigned int n = strtol(p, &pTemp, 10);
		if (*pTemp != '\0')
			POP3_ERROR(POP3_ERROR_LIST | POP3_ERROR_PARSE);
		
		pList->push_back(n);
		
		p = pEnd + 2;
	}
	
	nError_ = POP3_ERROR_SUCCESS;
	
	return true;
}

bool qmpop3::Pop3::deleteMessage(unsigned int nMsg)
{
	if (nMsg >= getMessageCount())
		POP3_ERROR(POP3_ERROR_DELE | POP3_ERROR_RESPONSE);
	
	CHAR szDele[128];
	sprintf(szDele, "DELE %d\r\n", nMsg + 1);
	
	if (!sendCommand(szDele))
		POP3_ERROR_OR(POP3_ERROR_DELE);
	
	nError_ = POP3_ERROR_SUCCESS;
	
	return true;
}

bool qmpop3::Pop3::getUid(unsigned int nMsg,
						  wstring_ptr* pwstrUid)
{
	assert(pwstrUid);
	
	if (nMsg >= getMessageCount())
		POP3_ERROR(POP3_ERROR_UIDL | POP3_ERROR_RESPONSE);
	
	CHAR szUidl[128];
	sprintf(szUidl, "UIDL %d\r\n", nMsg + 1);
	
	string_ptr strResponse;
	if (!sendCommand(szUidl, &strResponse))
		POP3_ERROR_OR(POP3_ERROR_UIDL);
	
	const CHAR* p = strchr(strResponse.get() + 4, ' ');
	if (!p)
		POP3_ERROR(POP3_ERROR_UIDL | POP3_ERROR_PARSE);
	const CHAR* pEnd = strstr(p, "\r\n");
	if (!pEnd)
		POP3_ERROR(POP3_ERROR_UIDL | POP3_ERROR_PARSE);
	*pwstrUid = mbs2wcs(p + 1, pEnd - p - 1);
	
	nError_ = POP3_ERROR_SUCCESS;
	
	return true;
}

bool qmpop3::Pop3::getUids(UidList* pList)
{
	assert(pList);
	assert(pList->empty());
	
	pList->reserve(nCount_);
	
	string_ptr strResponse;
	xstring_size_ptr strContent;
	if (!sendCommand("UIDL\r\n", &strResponse, &strContent, 0))
		POP3_ERROR_OR(POP3_ERROR_UIDL);
	
	const CHAR* p = strContent.get();
	while (*p) {
		p = strchr(p, ' ');
		if (!p)
			POP3_ERROR(POP3_ERROR_UIDL | POP3_ERROR_PARSE);
		++p;
		const CHAR* pEnd = strstr(p, "\r\n");
		if (!pEnd)
			POP3_ERROR(POP3_ERROR_UIDL | POP3_ERROR_PARSE);
		wstring_ptr strUid(mbs2wcs(p, pEnd - p));
		pList->push_back(strUid.release());
		p = pEnd + 2;
	}
	
	nError_ = POP3_ERROR_SUCCESS;
	
	return true;
}

bool qmpop3::Pop3::noop()
{
	if (!sendCommand("NOOP\r\n"))
		POP3_ERROR_OR(POP3_ERROR_NOOP);
	
	nError_ = POP3_ERROR_SUCCESS;
	
	return true;
}

bool qmpop3::Pop3::sendMessage(const CHAR* pszMessage,
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
	
	if (!sendCommand("XTND XMIT\r\n"))
		POP3_ERROR_OR(POP3_ERROR_XTNDXMIT);
	
	if (!send(&listSendData[0], listSendData.size(), true))
		POP3_ERROR_OR(POP3_ERROR_XTNDXMIT);
	
	nError_ = POP3_ERROR_SUCCESS;
	
	return true;
}

unsigned int qmpop3::Pop3::getLastError() const
{
	return nError_;
}

const WCHAR* qmpop3::Pop3::getLastErrorResponse() const
{
	return wstrErrorResponse_.get();
}

bool qmpop3::Pop3::receive(string_ptr* pstrResponse)
{
	return receive(pstrResponse, 0, 0);
}

bool qmpop3::Pop3::receive(string_ptr* pstrResponse,
						   xstring_size_ptr* pstrContent,
						   size_t nContentSizeHint)
{
	assert(!pstrContent || pstrResponse);
	
	if (!pSocket_.get())
		POP3_ERROR(POP3_ERROR_INVALIDSOCKET);
	
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
			POP3_ERROR_SOCKET(POP3_ERROR_SELECT);
		else if (nSelect == 0)
			POP3_ERROR(POP3_ERROR_TIMEOUT);
		
		if (bContent) {
			XStringBufferLock<XSTRING> lock(&bufContent, sizeof(buf));
			CHAR* pLock = lock.get();
			if (!pLock)
				POP3_ERROR(POP3_ERROR_OTHER);
			
			size_t nLen = pSocket_->recv(pLock, sizeof(buf), 0);
			if (nLen == -1)
				POP3_ERROR_SOCKET(POP3_ERROR_RECEIVE);
			else if (nLen == 0)
				POP3_ERROR(POP3_ERROR_DISCONNECT);
			
			if (!checkContent(pLock, &nLen, &state))
				POP3_ERROR(POP3_ERROR_PARSE);
			
			lock.unlock(nLen);
		}
		else {
			size_t nLen = pSocket_->recv(buf, sizeof(buf), 0);
			if (nLen == -1)
				POP3_ERROR_SOCKET(POP3_ERROR_RECEIVE);
			else if (nLen == 0)
				POP3_ERROR(POP3_ERROR_DISCONNECT);
			
			if (bMultiLine) {
				assert(!bContent);
				bufResponse.append(buf, nLen);
				
				const CHAR* pRes = bufResponse.getCharArray();
				const CHAR* p = strstr(pRes, "\r\n");
				if (p) {
					bContent = true;
					
					if (nContentSizeHint != 0) {
						if (!bufContent.reserve(nContentSizeHint + sizeof(buf)))
							POP3_ERROR(POP3_ERROR_OTHER);
					}
					
					size_t nContentLen = bufResponse.getLength() - (p - pRes) - 2;
					char* pContent = buf + nLen - nContentLen;
					if (!checkContent(pContent, &nContentLen, &state))
						POP3_ERROR(POP3_ERROR_PARSE);
					if (!bufContent.append(pContent, nContentLen))
						POP3_ERROR(POP3_ERROR_OTHER);
					bufResponse.remove(p + 2 - pRes, bufResponse.getLength());
				}
				if (bufResponse.getLength() > 4 && strncmp(pRes, pszErr__, 4) == 0)
					bMultiLine = false;
			}
			else {
				bufResponse.append(buf, nLen);
			}
		}
			
		if (bMultiLine)
			pPop3Callback_->setPos(bufContent.getLength());
		
		if (bMultiLine)
			bEnd = state == STATE_LF2;
		else
			bEnd = bufResponse.getLength() >= 2 &&
				strncmp(bufResponse.getCharArray() + bufResponse.getLength() - 2, "\r\n", 2) == 0;
	} while (!bEnd);
	
	if (strncmp(bufResponse.getCharArray(), pszOk__, 3) != 0) {
		setErrorResponse(bufResponse.getCharArray());
		POP3_ERROR(POP3_ERROR_RESPONSE);
	}
	
	if (pstrResponse)
		*pstrResponse = bufResponse.getString();
	if (pstrContent) {
		bufContent.remove(bufContent.getLength() - 2, bufContent.getLength());
		*pstrContent = bufContent.getXStringSize();
		if (!pstrContent->get())
			return false;
	}
	
	nError_ = POP3_ERROR_SUCCESS;
	
	return true;
}

bool qmpop3::Pop3::sendCommand(const CHAR* pszCommand)
{
	return sendCommand(pszCommand, 0, 0, 0);
}

bool qmpop3::Pop3::sendCommand(const CHAR* pszCommand,
							   string_ptr* pstrResponse)
{
	return sendCommand(pszCommand, pstrResponse, 0, 0);
}

bool qmpop3::Pop3::sendCommand(const CHAR* pszCommand,
							   string_ptr* pstrResponse,
							   xstring_size_ptr* pstrContent,
							   size_t nContentSizeHint)
{
	SendData sd = {
		pszCommand,
		strlen(pszCommand)
	};
	return send(&sd, 1, false, pstrResponse, pstrContent, nContentSizeHint);
}

bool qmpop3::Pop3::send(const SendData* pSendData,
						size_t nDataLen,
						bool bProgress)
{
	return send(pSendData, nDataLen, bProgress, 0, 0, 0);
}

bool qmpop3::Pop3::send(const SendData* pSendData,
						size_t nDataLen,
						bool bProgress,
						string_ptr* pstrResponse,
						xstring_size_ptr* pstrContent,
						size_t nContentSizeHint)
{
	assert(pSendData);
	assert(nDataLen > 0);
	
	if (!pSocket_.get())
		POP3_ERROR(POP3_ERROR_INVALIDSOCKET);
	
	if (bProgress) {
		size_t nLen = 0;
		for (size_t n = 0; n < nDataLen; ++n)
			nLen += (pSendData + n)->nLength_;
		pPop3Callback_->setRange(0, nLen);
	}
	
	for (size_t n = 0; n < nDataLen; ++n) {
		const SendData& data = *(pSendData + n);
		size_t nTotal = 0;
		while (nTotal < data.nLength_) {
			int nSelect = pSocket_->select(Socket::SELECT_READ | Socket::SELECT_WRITE);
			if (nSelect == -1)
				POP3_ERROR_SOCKET(POP3_ERROR_SELECT);
			else if (nSelect == 0)
				POP3_ERROR(POP3_ERROR_TIMEOUT);
			
			size_t nSend = pSocket_->send(data.psz_ + nTotal,
				QSMIN(size_t(SEND_BLOCK_SIZE), data.nLength_ - nTotal), 0);
			if (nSend == -1)
				POP3_ERROR_SOCKET(POP3_ERROR_SEND);
			nTotal += nSend;
			
			if (bProgress)
				pPop3Callback_->setPos(nTotal);
		}
	}
	if (bProgress) {
		pPop3Callback_->setRange(0, 0);
		pPop3Callback_->setPos(0);
	}
	
	return receive(pstrResponse, pstrContent, nContentSizeHint);
}

void qmpop3::Pop3::setErrorResponse(const CHAR* pszErrorResponse)
{
	wstrErrorResponse_ = mbs2wcs(pszErrorResponse);
}

bool qmpop3::Pop3::checkContent(CHAR* psz,
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
			else if (c == '\r')
				*pState = STATE_CR1;
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
			else if (c == '\r')
				*pState = STATE_CR1;
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
 * Pop3Callback
 *
 */

qmpop3::Pop3Callback::~Pop3Callback()
{
}
