/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsconv.h>
#include <qsmd5.h>
#include <qsmime.h>
#include <qsnew.h>

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

const CHAR* qmpop3::Pop3::pszOk__ = "+OK";
const CHAR* qmpop3::Pop3::pszErr__ = "-ERR";

qmpop3::Pop3::Pop3(const Option& option, QSTATUS* pstatus) :
	nTimeout_(option.nTimeout_),
	pSocketCallback_(option.pSocketCallback_),
	pSSLSocketCallback_(option.pSSLSocketCallback_),
	pPop3Callback_(option.pPop3Callback_),
	pLogger_(option.pLogger_),
	pSocket_(0),
	nCount_(0),
	nError_(POP3_ERROR_SUCCESS),
	wstrErrorResponse_(0)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qmpop3::Pop3::~Pop3()
{
	delete pSocket_;
	freeWString(wstrErrorResponse_);
}

QSTATUS qmpop3::Pop3::connect(const WCHAR* pwszHost,
	short nPort, bool bApop, bool bSsl)
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
	CHECK_QSTATUS_ERROR(POP3_ERROR_INITIALIZE);
	
	status = pSocket->connect(pwszHost, nPort);
	CHECK_QSTATUS_ERROR(POP3_ERROR_CONNECT | pSocket->getLastError());
	
	if (bSsl) {
		SSLSocketFactory* pFactory = SSLSocketFactory::getFactory();
		CHECK_ERROR(!pFactory, QSTATUS_FAIL, POP3_ERROR_SSL);
		SSLSocket* pSSLSocket = 0;
		status = pFactory->createSSLSocket(pSocket.get(),
			true, pSSLSocketCallback_, pLogger_, &pSSLSocket);
		CHECK_QSTATUS_ERROR(POP3_ERROR_SSL);
		pSocket.release();
		pSocket_ = pSSLSocket;
	}
	else {
		pSocket_ = pSocket.release();
	}
	
	string_ptr<STRING> strGreeting;
	status = receive(&strGreeting);
	CHECK_QSTATUS_ERROR_OR(POP3_ERROR_GREETING);
	
	string_ptr<WSTRING> wstrUserName;
	string_ptr<WSTRING> wstrPassword;
	status = pPop3Callback_->getUserInfo(&wstrUserName, &wstrPassword);
	CHECK_QSTATUS_ERROR_OR(POP3_ERROR_USER | POP3_ERROR_OTHER);
	string_ptr<STRING> strUserName(wcs2mbs(wstrUserName.get()));
	CHECK_ERROR(!strUserName.get(), QSTATUS_OUTOFMEMORY,
		POP3_ERROR_USER | POP3_ERROR_OTHER);
	string_ptr<STRING> strPassword(wcs2mbs(wstrPassword.get()));
	CHECK_ERROR(!strPassword.get(), QSTATUS_OUTOFMEMORY,
		POP3_ERROR_USER | POP3_ERROR_OTHER);
	
	status = pPop3Callback_->authenticating();
	CHECK_QSTATUS_ERROR(POP3_ERROR_OTHER);
	
	if (bApop) {
		CHAR szDigestString[128] = "";
		const CHAR* pBegin = strchr(strGreeting.get(), '<');
		if (pBegin) {
			const CHAR* pEnd = strchr(pBegin + 1, '>');
			if (pEnd) {
				string_ptr<STRING> strApop(allocString(
					pEnd - pBegin + strlen(strPassword.get()) + 2));
				CHECK_ERROR(!strApop.get(), QSTATUS_OUTOFMEMORY,
					POP3_ERROR_GENERATEDIGEST | POP3_ERROR_APOP);
				CHAR* p = strApop.get();
				strncpy(p, pBegin, pEnd - pBegin + 1);
				p += pEnd - pBegin + 1;
				strcpy(p, strPassword.get());
				
				MD5::md5ToString(reinterpret_cast<const unsigned char*>(strApop.get()),
					strlen(strApop.get()), szDigestString);
			}
		}
		CHECK_ERROR(szDigestString[0] == '\0', QSTATUS_FAIL,
			POP3_ERROR_GENERATEDIGEST | POP3_ERROR_APOP);
		
		string_ptr<STRING> strApop(allocString(strlen(strUserName.get()) + 128 + 10));
		CHECK_ERROR(!strApop.get(), QSTATUS_OUTOFMEMORY,
			POP3_ERROR_GENERATEDIGEST | POP3_ERROR_APOP);
		sprintf(strApop.get(), "APOP %s %s\r\n", strUserName.get(), szDigestString);
		status = sendCommand(strApop.get());
		CHECK_QSTATUS_ERROR_OR(POP3_ERROR_APOP);
	}
	else {
		string_ptr<STRING> strUser(allocString(strlen(strUserName.get()) + 10));
		CHECK_ERROR(!strUser.get(), QSTATUS_OUTOFMEMORY,
			POP3_ERROR_OTHER | POP3_ERROR_USER);
		sprintf(strUser.get(), "USER %s\r\n", strUserName.get());
		status = sendCommand(strUser.get());
		CHECK_QSTATUS_ERROR_OR(POP3_ERROR_USER);
		
		string_ptr<STRING> strPass(allocString(strlen(strPassword.get()) + 10));
		CHECK_ERROR(!strPass.get(), QSTATUS_OUTOFMEMORY,
			POP3_ERROR_OTHER | POP3_ERROR_PASS);
		sprintf(strPass.get(), "PASS %s\r\n", strPassword.get());
		status = sendCommand(strPass.get());
		CHECK_QSTATUS_ERROR_OR(POP3_ERROR_PASS);
	}
	status = pPop3Callback_->setPassword(wstrPassword.get());
	CHECK_QSTATUS_ERROR(POP3_ERROR_USER | POP3_ERROR_OTHER);
	
	string_ptr<STRING> strStat;
	status = sendCommand("STAT\r\n", &strStat);
	CHECK_QSTATUS_ERROR_OR(POP3_ERROR_STAT);
	
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
			CHECK_ERROR(*p, QSTATUS_FAIL, POP3_ERROR_PARSE | POP3_ERROR_STAT);
		}
		pStat= pEnd + 1;
	}
	CHECK_ERROR(n != 3, QSTATUS_FAIL, POP3_ERROR_PARSE | POP3_ERROR_STAT);
	
	nError_ = POP3_ERROR_SUCCESS;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmpop3::Pop3::disconnect()
{
	if (pSocket_) {
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
		delete pSocket_;
		pSocket_ = 0;
	}
	return QSTATUS_SUCCESS;
}

QSTATUS qmpop3::Pop3::getMessageCount() const
{
	return nCount_;
}

QSTATUS qmpop3::Pop3::getMessage(unsigned int nMsg,
	unsigned int nMaxLine, STRING* pstrMessage, unsigned int* pnSize)
{
	assert(pstrMessage);
	
	DECLARE_QSTATUS();
	
	CHECK_ERROR(nMsg >= getMessageCount(), QSTATUS_FAIL,
		POP3_ERROR_RETR | POP3_ERROR_RESPONSE);
	
	CHAR szRetr[128];
	if (nMaxLine == 0xffffffff)
		sprintf(szRetr, "RETR %d\r\n", nMsg + 1);
	else
		sprintf(szRetr, "TOP %d %d\r\n", nMsg + 1, nMaxLine);
	
	if (pnSize) {
		status = pPop3Callback_->setRange(0, *pnSize);
		CHECK_QSTATUS_ERROR(
			(nMaxLine == 0xffffffff ? POP3_ERROR_RETR : POP3_ERROR_TOP) |
			POP3_ERROR_OTHER);
	}
	string_ptr<STRING> strResponse;
	string_ptr<STRING> strContent;
	status = sendCommand(szRetr, &strResponse, &strContent);
	CHECK_QSTATUS_ERROR(
		(nMaxLine == 0xffffffff ? POP3_ERROR_RETR : POP3_ERROR_TOP));
	
	size_t nLen = strlen(strContent.get());
	if (pnSize)
		*pnSize = nLen;
	if (nLen >= 2 && strContent[nLen - 1] == '\n' && strContent[nLen - 2] == '\r')
		strContent[nLen - 2] = '\0';
	
	*pstrMessage = strContent.release();
	nError_ = POP3_ERROR_SUCCESS;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmpop3::Pop3::getMessageSize(unsigned int nMsg, unsigned int* pnSize)
{
	assert(pnSize);
	
	DECLARE_QSTATUS();
	
	*pnSize = static_cast<unsigned int>(-1);
	
	CHAR szList[128];
	sprintf(szList, "LIST %d\r\n", nMsg + 1);
	
	string_ptr<STRING> strResponse;
	status = sendCommand(szList, &strResponse);
	CHECK_QSTATUS_ERROR_OR(POP3_ERROR_LIST);
	
	const CHAR* p = strchr(strResponse.get(), ' ');
	CHECK_ERROR(!p, QSTATUS_FAIL, POP3_ERROR_LIST | POP3_ERROR_PARSE);
	p = strchr(p + 1, ' ');
	CHECK_ERROR(!p, QSTATUS_FAIL, POP3_ERROR_LIST | POP3_ERROR_PARSE);
	++p;
	CHAR* pEnd = strchr(p, '\r');
	CHECK_ERROR(!pEnd, QSTATUS_FAIL, POP3_ERROR_LIST | POP3_ERROR_PARSE);
	*pEnd = '\0';
	CHAR* pTemp = 0;
	*pnSize = strtol(p, &pTemp, 10);
	CHECK_ERROR(*pTemp != '\0', QSTATUS_FAIL, POP3_ERROR_LIST | POP3_ERROR_PARSE);
	
	nError_ = POP3_ERROR_SUCCESS;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmpop3::Pop3::getMessageSizes(MessageSizeList* pList)
{
	assert(pList);
	assert(pList->empty());
	
	DECLARE_QSTATUS();
	
	status = STLWrapper<MessageSizeList>(*pList).reserve(nCount_);
	CHECK_QSTATUS_ERROR(POP3_ERROR_LIST | POP3_ERROR_OTHER);
	
	string_ptr<STRING> strResponse;
	string_ptr<STRING> strContent;
	
	status = sendCommand("LIST\r\n", &strResponse, &strContent);
	CHECK_QSTATUS_ERROR_OR(POP3_ERROR_LIST);
	
	const CHAR* p = strContent.get();
	while (*p) {
		p = strchr(p, ' ');
		CHECK_ERROR(!p, QSTATUS_FAIL, POP3_ERROR_LIST | POP3_ERROR_PARSE);
		++p;
		CHAR* pEnd = strstr(p, "\r\n");
		CHECK_ERROR(!pEnd, QSTATUS_FAIL, POP3_ERROR_LIST | POP3_ERROR_PARSE);
		*pEnd = '\0';
		
		CHAR* pTemp = 0;
		unsigned int n = strtol(p, &pTemp, 10);
		CHECK_ERROR(*pTemp != '\0', QSTATUS_FAIL, POP3_ERROR_LIST | POP3_ERROR_PARSE);
		
		pList->push_back(n);
		
		p = pEnd + 2;
	}
	
	nError_ = POP3_ERROR_SUCCESS;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmpop3::Pop3::deleteMessage(unsigned int nMsg)
{
	DECLARE_QSTATUS();
	
	CHECK_ERROR(nMsg >= getMessageCount(), QSTATUS_FAIL,
		POP3_ERROR_DELE | POP3_ERROR_RESPONSE);
	
	CHAR szDele[128];
	sprintf(szDele, "DELE %d\r\n", nMsg + 1);
	
	status = sendCommand(szDele);
	CHECK_QSTATUS_ERROR_OR(POP3_ERROR_DELE);
	
	nError_ = POP3_ERROR_SUCCESS;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmpop3::Pop3::getUid(unsigned int nMsg, WSTRING* pwstrUid)
{
	assert(pwstrUid);
	
	DECLARE_QSTATUS();
	
	CHECK_ERROR(nMsg >= getMessageCount(), QSTATUS_FAIL,
		POP3_ERROR_UIDL | POP3_ERROR_RESPONSE);
	
	CHAR szUidl[128];
	sprintf(szUidl, "UIDL %d\r\n", nMsg + 1);
	
	string_ptr<STRING> strResponse;
	status = sendCommand(szUidl, &strResponse);
	CHECK_QSTATUS_ERROR(POP3_ERROR_UIDL);
	
	const CHAR* p = strchr(strResponse.get() + 4, ' ');
	CHECK_ERROR(!p, QSTATUS_FAIL, POP3_ERROR_UIDL | POP3_ERROR_PARSE);
	const CHAR* pEnd = strstr(p, "\r\n");
	CHECK_ERROR(!pEnd, QSTATUS_FAIL, POP3_ERROR_UIDL | POP3_ERROR_PARSE);
	*pwstrUid = mbs2wcs(p + 1, pEnd - p - 1);
	CHECK_ERROR(!*pwstrUid, QSTATUS_OUTOFMEMORY, POP3_ERROR_UIDL | POP3_ERROR_OTHER);
	
	nError_ = POP3_ERROR_SUCCESS;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmpop3::Pop3::getUids(UidList* pList)
{
	assert(pList);
	assert(pList->empty());
	
	DECLARE_QSTATUS();
	
	status = STLWrapper<UidList>(*pList).reserve(nCount_);
	CHECK_QSTATUS_ERROR(POP3_ERROR_UIDL | POP3_ERROR_OTHER);
	
	string_ptr<STRING> strResponse;
	string_ptr<STRING> strContent;
	status = sendCommand("UIDL\r\n", &strResponse, &strContent);
	CHECK_QSTATUS_ERROR(POP3_ERROR_UIDL);
	
	const CHAR* p = strContent.get();
	while (*p) {
		p = strchr(p, ' ');
		CHECK_ERROR(!p, QSTATUS_FAIL, POP3_ERROR_UIDL | POP3_ERROR_PARSE);
		++p;
		const CHAR* pEnd = strstr(p, "\r\n");
		CHECK_ERROR(!pEnd, QSTATUS_FAIL, POP3_ERROR_UIDL | POP3_ERROR_PARSE);
		string_ptr<WSTRING> strUid(mbs2wcs(p, pEnd - p));
		CHECK_ERROR(!strUid.get(), QSTATUS_OUTOFMEMORY,
			POP3_ERROR_UIDL | POP3_ERROR_OTHER);
		pList->push_back(strUid.release());
		p = pEnd + 2;
	}
	
	nError_ = POP3_ERROR_SUCCESS;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmpop3::Pop3::noop()
{
	DECLARE_QSTATUS();
	
	status = sendCommand("NOOP\r\n");
	CHECK_QSTATUS_ERROR_OR(POP3_ERROR_NOOP);
	
	nError_ = POP3_ERROR_SUCCESS;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmpop3::Pop3::sendMessage(const CHAR* pszMessage, size_t nLen)
{
	assert(pszMessage);
	
	DECLARE_QSTATUS();
	
	if (nLen == -1)
		nLen = strlen(pszMessage);
	
	typedef std::vector<SendData> SendDataList;
	SendDataList listSendData;
	
	const CHAR* p = pszMessage;
	SendData sd = { p, 0 };
	for (size_t m = 0; m < nLen; ++m, ++p) {
		if (*p == '.' && m > 1 && *(p - 1) == '\n' && *(p - 2) == '\r') {
			sd.nLength_ = p - sd.psz_ + 1;
			status = STLWrapper<SendDataList>(listSendData).push_back(sd);
			CHECK_QSTATUS_ERROR(POP3_ERROR_XTNDXMIT);
			sd.psz_ = p;
		}
	}
	sd.nLength_ = p - sd.psz_;
	status = STLWrapper<SendDataList>(listSendData).push_back(sd);
	CHECK_QSTATUS_ERROR(POP3_ERROR_XTNDXMIT);
	
	if (nLen > 2 && pszMessage[nLen - 2] == '\r' && pszMessage[nLen - 1] == '\n') {
		sd.psz_ = ".\r\n";
		sd.nLength_ = 3;
	}
	else {
		sd.psz_ = "\r\n.\r\n";
		sd.nLength_ = 5;
	}
	status = STLWrapper<SendDataList>(listSendData).push_back(sd);
	CHECK_QSTATUS_ERROR(POP3_ERROR_XTNDXMIT);
	
	status = sendCommand("XTND XMIT\r\n");
	CHECK_QSTATUS_ERROR_OR(POP3_ERROR_XTNDXMIT);
	
	status = send(&listSendData[0], listSendData.size(), true);
	CHECK_QSTATUS_ERROR_OR(POP3_ERROR_XTNDXMIT);
	
	nError_ = POP3_ERROR_SUCCESS;
	
	return QSTATUS_SUCCESS;
}

unsigned int qmpop3::Pop3::getLastError() const
{
	return nError_;
}

const WCHAR* qmpop3::Pop3::getLastErrorResponse() const
{
	return wstrErrorResponse_;
}

QSTATUS qmpop3::Pop3::receive(STRING* pstrResponse)
{
	return receive(pstrResponse, 0);
}

QSTATUS qmpop3::Pop3::receive(STRING* pstrResponse, STRING* pstrContent)
{
	assert(!pstrContent || pstrResponse);
	
	DECLARE_QSTATUS();
	
	CHECK_ERROR(!pSocket_, QSTATUS_FAIL, POP3_ERROR_INVALIDSOCKET);
	
	StringBuffer<STRING> bufResponse(&status);
	CHECK_QSTATUS_ERROR(POP3_ERROR_OTHER);
	StringBuffer<STRING> bufContent(&status);
	CHECK_QSTATUS_ERROR(POP3_ERROR_OTHER);
	
	bool bContent = false;
	bool bMultiLine = pstrContent != 0;
	char buf[1024];
	State state = STATE_NONE;
	bool bEnd = false;
	do {
		int nSelect = Socket::SELECT_READ;
		status = pSocket_->select(&nSelect);
		CHECK_QSTATUS_ERROR(POP3_ERROR_SELECT | pSocket_->getLastError());
		CHECK_ERROR(nSelect == 0, QSTATUS_FAIL, POP3_ERROR_TIMEOUT);
		
		int nLen = sizeof(buf);
		status = pSocket_->recv(buf, &nLen, 0);
		CHECK_QSTATUS_ERROR(POP3_ERROR_RECEIVE | pSocket_->getLastError());
		CHECK_ERROR(nLen == 0, QSTATUS_FAIL, POP3_ERROR_DISCONNECT);
		
		if (bMultiLine) {
			if (!bContent) {
				status = bufResponse.append(buf, nLen);
				CHECK_QSTATUS_ERROR(POP3_ERROR_OTHER);
				
				const CHAR* pRes = bufResponse.getCharArray();
				const CHAR* p = strstr(pRes, "\r\n");
				if (p) {
					bContent = true;
					status = addContent(&bufContent, p + 2,
						bufResponse.getLength() - (p - pRes) - 2, &state);
					CHECK_QSTATUS_ERROR(POP3_ERROR_OTHER);
					bufResponse.remove(p + 2 - pRes, bufResponse.getLength());
				}
				if (bufResponse.getLength() > 4 &&
					strncmp(pRes, pszErr__, 4) == 0)
					bMultiLine = false;
			}
			else {
				status = addContent(&bufContent, buf, nLen, &state);
				CHECK_QSTATUS_ERROR(POP3_ERROR_OTHER);
			}
		}
		else {
			status = bufResponse.append(buf, nLen);
			CHECK_QSTATUS_ERROR(POP3_ERROR_OTHER);
		}
		
		if (bMultiLine) {
			status = pPop3Callback_->setPos(bufContent.getLength());
			CHECK_QSTATUS_ERROR(POP3_ERROR_OTHER);
		}
		
		if (bMultiLine)
			bEnd = state == STATE_LF;
		else
			bEnd = bufResponse.getLength() >= 2 &&
				strncmp(bufResponse.getCharArray() + bufResponse.getLength() - 2, "\r\n", 2) == 0;
	} while (!bEnd);
	
	if (strncmp(bufResponse.getCharArray(), pszOk__, 3) != 0) {
		status = setErrorResponse(bufResponse.getCharArray());
		CHECK_QSTATUS_ERROR(POP3_ERROR_OTHER);
		CHECK_ERROR(true, QSTATUS_FAIL, POP3_ERROR_RESPONSE);
	}
	
	if (pstrResponse)
		*pstrResponse = bufResponse.getString();
	if (pstrContent) {
		bufContent.remove(bufContent.getLength() - 2, bufContent.getLength());
		*pstrContent = bufContent.getString();
	}
	
	nError_ = POP3_ERROR_SUCCESS;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmpop3::Pop3::sendCommand(const CHAR* pszCommand)
{
	return sendCommand(pszCommand, 0, 0);
}

QSTATUS qmpop3::Pop3::sendCommand(const CHAR* pszCommand, STRING* pstrResponse)
{
	return sendCommand(pszCommand, pstrResponse, 0);
}

QSTATUS qmpop3::Pop3::sendCommand(const CHAR* pszCommand,
	STRING* pstrResponse, STRING* pstrContent)
{
	SendData sd = { pszCommand, strlen(pszCommand) };
	return send(&sd, 1, false, pstrResponse, pstrContent);
}

QSTATUS qmpop3::Pop3::send(const SendData* pSendData, size_t nDataLen, bool bProgress)
{
	return send(pSendData, nDataLen, bProgress, 0, 0);
}

QSTATUS qmpop3::Pop3::send(const SendData* pSendData, size_t nDataLen,
	bool bProgress, STRING* pstrResponse, STRING* pstrContent)
{
	assert(pSendData);
	assert(nDataLen > 0);
	
	DECLARE_QSTATUS();
	
	CHECK_ERROR(!pSocket_, QSTATUS_FAIL, POP3_ERROR_INVALIDSOCKET);
	
	if (bProgress) {
		size_t nLen = 0;
		for (size_t n = 0; n < nDataLen; ++n)
			nLen += (pSendData + n)->nLength_;
		status = pPop3Callback_->setRange(0, nLen);
		CHECK_QSTATUS_ERROR(POP3_ERROR_SEND);
	}
	
	for (size_t n = 0; n < nDataLen; ++n) {
		const SendData& data = *(pSendData + n);
		size_t nTotal = 0;
		while (nTotal < data.nLength_) {
			int nSelect = Socket::SELECT_READ | Socket::SELECT_WRITE;
			status = pSocket_->select(&nSelect);
			CHECK_QSTATUS_ERROR(POP3_ERROR_SELECT | pSocket_->getLastError());
			CHECK_ERROR(nSelect == 0, QSTATUS_FAIL, POP3_ERROR_TIMEOUT);
			
			int nSend = QSMIN(size_t(2048), data.nLength_ - nTotal);
			status = pSocket_->send(data.psz_ + nTotal, &nSend, 0);
			CHECK_QSTATUS_ERROR(POP3_ERROR_SEND | pSocket_->getLastError());
			nTotal += nSend;
			if (bProgress) {
				status = pPop3Callback_->setPos(nTotal);
				CHECK_QSTATUS_ERROR(POP3_ERROR_SEND);
			}
		}
	}
	if (bProgress) {
		status = pPop3Callback_->setRange(0, 0);
		CHECK_QSTATUS_ERROR(POP3_ERROR_SEND);
		status = pPop3Callback_->setPos(0);
		CHECK_QSTATUS_ERROR(POP3_ERROR_SEND);
	}
	
	return receive(pstrResponse, pstrContent);
}

QSTATUS qmpop3::Pop3::setErrorResponse(const CHAR* pszErrorResponse)
{
	freeWString(wstrErrorResponse_);
	wstrErrorResponse_ = mbs2wcs(pszErrorResponse);
	if (!wstrErrorResponse_)
		return QSTATUS_OUTOFMEMORY;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmpop3::Pop3::addContent(StringBuffer<STRING>* pBuf,
	const CHAR* psz, size_t nLen, State* pState)
{
	DECLARE_QSTATUS();
	
	while (nLen != 0) {
		CHAR c = *psz;
		switch (c) {
		case '.':
			if (*pState != STATE_PERIOD &&
				(pBuf->getLength() == 0 ||
					(pBuf->getLength() >= 2 &&
					pBuf->get(pBuf->getLength() - 1) == '\n' &&
					pBuf->get(pBuf->getLength() - 2) == '\r'))) {
				*pState = STATE_PERIOD;
				c = '\0';
			}
			else {
				*pState = STATE_NONE;
			}
			break;
		case '\r':
			*pState = *pState == STATE_PERIOD ? STATE_CR : STATE_NONE;
			break;
		case '\n':
			*pState = *pState == STATE_CR ? STATE_LF : STATE_NONE;
			break;
		default:
			*pState = STATE_NONE;
			break;
		}
		if (c != '\0') {
			status = pBuf->append(c);
			CHECK_QSTATUS();
		}
		
		++psz;
		--nLen;
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * Pop3Callback
 *
 */

qmpop3::Pop3Callback::~Pop3Callback()
{
}
