/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsconv.h>
#include <qsnew.h>

#include <cstdio>

#include "nntp.h"

using namespace qmnntp;
using namespace qs;


/****************************************************************************
 *
 * Nntp
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

qmnntp::Nntp::Nntp(const Option& option, QSTATUS* pstatus) :
	nTimeout_(option.nTimeout_),
	pSocketCallback_(option.pSocketCallback_),
	pSSLSocketCallback_(option.pSSLSocketCallback_),
	pNntpCallback_(option.pNntpCallback_),
	pLogger_(option.pLogger_),
	pSocket_(0),
	wstrGroup_(0),
	nCount_(0),
	nFirst_(0),
	nLast_(0),
	nError_(NNTP_ERROR_SUCCESS),
	wstrErrorResponse_(0)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qmnntp::Nntp::~Nntp()
{
	freeWString(wstrGroup_);
	delete pSocket_;
	freeWString(wstrErrorResponse_);
}

QSTATUS qmnntp::Nntp::connect(const WCHAR* pwszHost, short nPort, bool bSsl)
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
	CHECK_QSTATUS_ERROR(NNTP_ERROR_INITIALIZE);
	
	status = pSocket->connect(pwszHost, nPort);
	CHECK_QSTATUS_ERROR(NNTP_ERROR_CONNECT | pSocket->getLastError());
	
	if (bSsl) {
		SSLSocketFactory* pFactory = SSLSocketFactory::getFactory();
		CHECK_ERROR(!pFactory, QSTATUS_FAIL, NNTP_ERROR_SSL);
		SSLSocket* pSSLSocket = 0;
		status = pFactory->createSSLSocket(pSocket.get(),
			true, pSSLSocketCallback_, pLogger_, &pSSLSocket);
		CHECK_QSTATUS_ERROR(NNTP_ERROR_SSL);
		pSocket.release();
		pSocket_ = pSSLSocket;
	}
	else {
		pSocket_ = pSocket.release();
	}
	
	unsigned int nCode = 0;
	status = receive(&nCode);
	CHECK_QSTATUS_ERROR_OR(NNTP_ERROR_GREETING);
	CHECK_ERROR(nCode/100 != 2, QSTATUS_FAIL,
		NNTP_ERROR_GREETING | NNTP_ERROR_RESPONSE);
	
	status = sendCommand("MODE READER\r\n", &nCode);
	CHECK_QSTATUS_ERROR_OR(NNTP_ERROR_MODEREADER);
	
	string_ptr<WSTRING> wstrUserName;
	string_ptr<WSTRING> wstrPassword;
	status = pNntpCallback_->getUserInfo(&wstrUserName, &wstrPassword);
	CHECK_QSTATUS_ERROR_OR(NNTP_ERROR_AUTHINFO);
	if (wstrUserName.get() && *wstrUserName.get()) {
		status = pNntpCallback_->authenticating();
		CHECK_QSTATUS_ERROR(NNTP_ERROR_OTHER);
		
		string_ptr<STRING> strCommand;
		unsigned int nCode = 0;
		
		string_ptr<STRING> strUserName(wcs2mbs(wstrUserName.get()));
		CHECK_ERROR(!strUserName.get(), QSTATUS_OUTOFMEMORY,
			NNTP_ERROR_AUTHINFO | NNTP_ERROR_OTHER);
		strCommand.reset(concat("AUTHINFO USER ", strUserName.get(), "\r\n"));
		CHECK_ERROR(!strCommand.get(), QSTATUS_OUTOFMEMORY,
			NNTP_ERROR_AUTHINFO | NNTP_ERROR_OTHER);
		status = sendCommand(strCommand.get(), &nCode);
		CHECK_QSTATUS_ERROR_OR(NNTP_ERROR_AUTHINFO);
		CHECK_ERROR(nCode != 381, QSTATUS_FAIL,
			NNTP_ERROR_AUTHINFO | NNTP_ERROR_RESPONSE);
		
		string_ptr<STRING> strPassword(wcs2mbs(wstrPassword.get()));
		CHECK_ERROR(!strPassword.get(), QSTATUS_OUTOFMEMORY,
			NNTP_ERROR_AUTHINFO | NNTP_ERROR_OTHER);
		strCommand.reset(concat("AUTHINFO PASS ", strPassword.get(), "\r\n"));
		CHECK_ERROR(!strCommand.get(), QSTATUS_OUTOFMEMORY,
			NNTP_ERROR_AUTHINFO | NNTP_ERROR_OTHER);
		status = sendCommand(strCommand.get(), &nCode);
		CHECK_QSTATUS_ERROR_OR(NNTP_ERROR_AUTHINFO);
		CHECK_ERROR(nCode != 281, QSTATUS_FAIL,
			NNTP_ERROR_AUTHINFO | NNTP_ERROR_RESPONSE);
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmnntp::Nntp::disconnect()
{
	if (pSocket_) {
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
			unsigned int nCode = 0;
			sendCommand("QUIT\r\n", &nCode);
		}
		delete pSocket_;
		pSocket_ = 0;
	}
	return QSTATUS_SUCCESS;
}

const WCHAR* qmnntp::Nntp::getGroup() const
{
	return wstrGroup_;
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

QSTATUS qmnntp::Nntp::group(const WCHAR* pwszGroup)
{
	assert(pwszGroup);
	
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrGroup(allocWString(pwszGroup));
	CHECK_ERROR(!wstrGroup.get(), QSTATUS_OUTOFMEMORY,
		NNTP_ERROR_GROUP | NNTP_ERROR_OTHER);
	
	string_ptr<STRING> strGroup(wcs2mbs(pwszGroup));
	CHECK_ERROR(!strGroup.get(), QSTATUS_OUTOFMEMORY,
		NNTP_ERROR_GROUP | NNTP_ERROR_OTHER);
	
	string_ptr<STRING> strCommand(concat("GROUP ", strGroup.get(), "\r\n"));
	CHECK_ERROR(!strCommand.get(), QSTATUS_OUTOFMEMORY,
		NNTP_ERROR_GROUP | NNTP_ERROR_OTHER);
	
	unsigned int nCode = 0;
	string_ptr<STRING> strResponse;
	status = sendCommand(strCommand.get(), &nCode, &strResponse);
	CHECK_QSTATUS_ERROR_OR(NNTP_ERROR_GROUP);
	CHECK_ERROR(nCode != 211, QSTATUS_FAIL,
		NNTP_ERROR_GROUP | NNTP_ERROR_RESPONSE);
	
	const CHAR* p = strResponse.get() + 4;
	CHAR* pEnd = 0;
	unsigned int nCount = strtol(p, &pEnd, 10);
	CHECK_ERROR(*pEnd != ' ', QSTATUS_FAIL,
		NNTP_ERROR_GROUP | NNTP_ERROR_RESPONSE);
	
	p = pEnd + 1;
	unsigned int nFirst = strtol(p, &pEnd, 10);
	CHECK_ERROR(*pEnd != ' ', QSTATUS_FAIL,
		NNTP_ERROR_GROUP | NNTP_ERROR_RESPONSE);
	
	p = pEnd + 1;
	unsigned int nLast = strtol(p, &pEnd, 10);
	CHECK_ERROR(*pEnd != ' ', QSTATUS_FAIL,
		NNTP_ERROR_GROUP | NNTP_ERROR_RESPONSE);
	
	freeWString(wstrGroup_);
	wstrGroup_ = wstrGroup.release();
	nCount_ = nCount;
	nFirst_ = nFirst;
	nLast_ = nLast;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmnntp::Nntp::getMessage(unsigned int n,
	GetMessageFlag flag, STRING* pstrMessage)
{
	return getMessage(n, 0, flag, pstrMessage);
}

QSTATUS qmnntp::Nntp::getMessage(const WCHAR* pwszMessageId,
	GetMessageFlag flag, STRING* pstrMessage)
{
	assert(pwszMessageId);
	return getMessage(-1, pwszMessageId, flag, pstrMessage);
}

QSTATUS qmnntp::Nntp::getMessagesData(unsigned int nStart,
	unsigned int nEnd, MessagesData** ppMessageData)
{
	assert(nStart <= nEnd);
	assert(ppMessageData);
	
	DECLARE_QSTATUS();
	
	CHAR szCommand[128];
	sprintf(szCommand, "XOVER %u-%u\r\n", nStart, nEnd);
	
	const CHAR* pszCodes[] = {
		"224"
	};
	
	unsigned int nCode = 0;
	string_ptr<STRING> strResponse;
	string_ptr<STRING> strContent;
	status = sendCommand(szCommand, pszCodes,
		countof(pszCodes), &nCode, &strResponse, &strContent);
	CHECK_QSTATUS_ERROR_OR(NNTP_ERROR_XOVER);
	CHECK_ERROR(nCode != 224 && nCode != 420, QSTATUS_FAIL,
		NNTP_ERROR_XOVER | NNTP_ERROR_RESPONSE);
	
	std::auto_ptr<MessagesData> pData;
	status = newQsObject(&pData);
	CHECK_QSTATUS_ERROR(NNTP_ERROR_XOVER | NNTP_ERROR_OTHER);
	
	if (nCode == 224) {
		status = pData->setData(strContent.get());
		CHECK_QSTATUS_ERROR(NNTP_ERROR_XOVER | NNTP_ERROR_RESPONSE);
		strContent.release();
	}
	
	*ppMessageData = pData.release();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmnntp::Nntp::postMessage(const CHAR* pszMessage, size_t nLen)
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
			CHECK_QSTATUS_ERROR(NNTP_ERROR_POST);
			sd.psz_ = p;
		}
	}
	sd.nLength_ = p - sd.psz_;
	status = STLWrapper<SendDataList>(listSendData).push_back(sd);
	CHECK_QSTATUS_ERROR(NNTP_ERROR_POST);
	
	if (nLen > 2 && pszMessage[nLen - 2] == '\r' && pszMessage[nLen - 1] == '\n') {
		sd.psz_ = ".\r\n";
		sd.nLength_ = 3;
	}
	else {
		sd.psz_ = "\r\n.\r\n";
		sd.nLength_ = 5;
	}
	status = STLWrapper<SendDataList>(listSendData).push_back(sd);
	CHECK_QSTATUS_ERROR_OR(NNTP_ERROR_POST);
	
	unsigned int nCode = 0;
	status = sendCommand("POST\r\n", &nCode);
	CHECK_QSTATUS_ERROR_OR(NNTP_ERROR_POST);
	CHECK_ERROR(nCode != 340, QSTATUS_FAIL,
		NNTP_ERROR_POST | NNTP_ERROR_RESPONSE);
	
	status = send(&listSendData[0], listSendData.size(), 0, 0, &nCode, 0, 0);
	CHECK_QSTATUS_ERROR_OR(NNTP_ERROR_POST);
	CHECK_ERROR(nCode != 240, QSTATUS_FAIL,
		NNTP_ERROR_POST | NNTP_ERROR_RESPONSE);
	
	return QSTATUS_SUCCESS;
}

unsigned int qmnntp::Nntp::getLastError() const
{
	return nError_;
}

const WCHAR* qmnntp::Nntp::getLastErrorResponse() const
{
	return wstrErrorResponse_;
}

QSTATUS qmnntp::Nntp::getMessage(unsigned int n, const WCHAR* pwszMessageId,
	GetMessageFlag flag, STRING* pstrMessage)
{
	assert((n != -1 && !pwszMessageId) || (n == -1 && pwszMessageId));
	assert(pstrMessage);
	
	DECLARE_QSTATUS();
	
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
	
	StringBuffer<STRING> buf(&status);
	CHECK_QSTATUS();
	status = buf.append(types[flag].pszCommand_);
	CHECK_QSTATUS_ERROR(error | NNTP_ERROR_OTHER);
	
	if (pwszMessageId) {
		string_ptr<STRING> strMessageId(wcs2mbs(pwszMessageId));
		CHECK_ERROR(!strMessageId.get(), QSTATUS_OUTOFMEMORY,
			error | NNTP_ERROR_OTHER);
		status = buf.append("<");
		CHECK_QSTATUS_ERROR(error | NNTP_ERROR_OTHER);
		status = buf.append(strMessageId.get());
		CHECK_QSTATUS_ERROR(error | NNTP_ERROR_OTHER);
		status = buf.append(">");
		CHECK_QSTATUS_ERROR(error | NNTP_ERROR_OTHER);
		
		nCodeNotFound = 430;
	}
	else {
		CHAR sz[32];
		sprintf(sz, "%u", n);
		status = buf.append(sz);
		CHECK_QSTATUS_ERROR(error | NNTP_ERROR_OTHER);
		
		nCodeNotFound = 423;
	}
	
	status = buf.append("\r\n");
	CHECK_QSTATUS_ERROR(error | NNTP_ERROR_OTHER);
	
	const CHAR* pszCodes[] = {
		"220",
		"221",
		"222"
	};
	
	unsigned int nCode = 0;
	string_ptr<STRING> strResponse;
	string_ptr<STRING> strContent;
	status = sendCommand(buf.getCharArray(), pszCodes,
		countof(pszCodes), &nCode, &strResponse, &strContent);
	CHECK_QSTATUS_ERROR_OR(error);
	CHECK_ERROR(nCode != types[flag].nCode_ && nCode != nCodeNotFound,
		QSTATUS_FAIL, error | NNTP_ERROR_RESPONSE);
	
	*pstrMessage = strContent.release();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmnntp::Nntp::receive(unsigned int* pnCode)
{
	return receive(0, 0, pnCode, 0, 0);
}

QSTATUS qmnntp::Nntp::receive(unsigned int* pnCode, STRING* pstrResponse)
{
	return receive(0, 0, pnCode, pstrResponse, 0);
}

QSTATUS qmnntp::Nntp::receive(const CHAR* pszMultilineCodes[], size_t nCodeCount,
	unsigned int* pnCode, STRING* pstrResponse, STRING* pstrContent)
{
	assert((!pszMultilineCodes && !pstrContent && nCodeCount == 0) ||
		(pszMultilineCodes && pstrContent && nCodeCount != 0));
	assert(pnCode);
	
	DECLARE_QSTATUS();
	
	CHECK_ERROR(!pSocket_, QSTATUS_FAIL, NNTP_ERROR_INVALIDSOCKET);
	
	StringBuffer<STRING> bufResponse(&status);
	CHECK_QSTATUS_ERROR(NNTP_ERROR_OTHER);
	StringBuffer<STRING> bufContent(&status);
	CHECK_QSTATUS_ERROR(NNTP_ERROR_OTHER);
	
	bool bContent = false;
	bool bMultiLine = pstrContent != 0;
	char buf[1024];
	State state = STATE_NONE;
	bool bEnd = false;
	do {
		int nSelect = Socket::SELECT_READ;
		status = pSocket_->select(&nSelect);
		CHECK_QSTATUS_ERROR(NNTP_ERROR_SELECT | pSocket_->getLastError());
		CHECK_ERROR(nSelect == 0, QSTATUS_FAIL, NNTP_ERROR_TIMEOUT);
		
		int nLen = sizeof(buf);
		status = pSocket_->recv(buf, &nLen, 0);
		CHECK_QSTATUS_ERROR(NNTP_ERROR_RECEIVE | pSocket_->getLastError());
		CHECK_ERROR(nLen == 0, QSTATUS_FAIL, NNTP_ERROR_DISCONNECT);
		
		if (bMultiLine) {
			if (!bContent) {
				status = bufResponse.append(buf, nLen);
				CHECK_QSTATUS_ERROR(NNTP_ERROR_OTHER);
				
				const CHAR* pRes = bufResponse.getCharArray();
				const CHAR* p = strstr(pRes, "\r\n");
				if (p) {
					bContent = true;
					status = addContent(&bufContent, p + 2,
						bufResponse.getLength() - (p - pRes) - 2, &state);
					CHECK_QSTATUS_ERROR(NNTP_ERROR_OTHER);
					bufResponse.remove(p + 2 - pRes, bufResponse.getLength());
				}
				if (bufResponse.getLength() >= 3) {
					for (size_t n = 0; n < nCodeCount; ++n) {
						if (strncmp(pRes, pszMultilineCodes[n], 3) == 0)
							break;
					}
					if (n == nCodeCount)
						bMultiLine = false;
				}
			}
			else {
				status = addContent(&bufContent, buf, nLen, &state);
				CHECK_QSTATUS_ERROR(NNTP_ERROR_OTHER);
			}
		}
		else {
			status = bufResponse.append(buf, nLen);
			CHECK_QSTATUS_ERROR(NNTP_ERROR_OTHER);
		}
		
		if (bMultiLine) {
			status = pNntpCallback_->setPos(bufContent.getLength());
			CHECK_QSTATUS_ERROR(NNTP_ERROR_OTHER);
		}
		
		if (bMultiLine)
			bEnd = state == STATE_LF;
		else
			bEnd = bufResponse.getLength() >= 2 &&
				strncmp(bufResponse.getCharArray() + bufResponse.getLength() - 2, "\r\n", 2) == 0;
	} while (!bEnd);
	
	const CHAR* p = bufResponse.getCharArray();
	CHECK_ERROR(bufResponse.getLength() < 3 || !isdigit(*p) ||
		!isdigit(*(p + 1)) || !isdigit(*(p + 2)),
		QSTATUS_FAIL, NNTP_ERROR_RESPONSE);
	*pnCode = (*p - '0')*100 + (*(p + 1) - '0')*10 + (*(p + 2) - '0');
	
	if (*p != '1' && *p != '2' && *p != '3') {
		status = setErrorResponse(bufResponse.getCharArray());
		CHECK_QSTATUS_ERROR(NNTP_ERROR_OTHER);
//		CHECK_ERROR(true, QSTATUS_FAIL, NNTP_ERROR_RESPONSE);
	}
	
	if (pstrResponse)
		*pstrResponse = bufResponse.getString();
	if (pstrContent && bMultiLine) {
		bufContent.remove(bufContent.getLength() - 2, bufContent.getLength());
		*pstrContent = bufContent.getString();
	}
	
	nError_ = NNTP_ERROR_SUCCESS;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmnntp::Nntp::sendCommand(const CHAR* pszCommand, unsigned int* pnCode)
{
	return sendCommand(pszCommand, 0, 0, pnCode, 0, 0);
}

QSTATUS qmnntp::Nntp::sendCommand(const CHAR* pszCommand,
	unsigned int* pnCode, STRING* pstrResponse)
{
	return sendCommand(pszCommand, 0, 0, pnCode, pstrResponse, 0);
}

QSTATUS qmnntp::Nntp::sendCommand(const CHAR* pszCommand,
	const CHAR* pszMultilineCodes[], size_t nCodeCount,
	unsigned int* pnCode, STRING* pstrResponse, STRING* pstrContent)
{
	SendData sd = { pszCommand, strlen(pszCommand) };
	return send(&sd, 1, pszMultilineCodes,
		nCodeCount, pnCode, pstrResponse, pstrContent);
}

QSTATUS qmnntp::Nntp::send(const SendData* pSendData, size_t nDataLen,
	const CHAR* pszMultilineCodes[], size_t nCodeCount,
	unsigned int* pnCode, STRING* pstrResponse, STRING* pstrContent)
{
	assert(pSendData);
	assert(nDataLen > 0);
	
	DECLARE_QSTATUS();
	
	CHECK_ERROR(!pSocket_, QSTATUS_FAIL, NNTP_ERROR_INVALIDSOCKET);
	
//	if (bProgress) {
//		size_t nLen = 0;
//		for (size_t n = 0; n < nDataLen; ++n)
//			nLen += (pSendData + n)->nLength_;
//		status = pNntpCallback_->setRange(0, nLen);
//		CHECK_QSTATUS_ERROR(NNTP_ERROR_SEND);
//	}
	
	for (size_t n = 0; n < nDataLen; ++n) {
		const SendData& data = *(pSendData + n);
		size_t nTotal = 0;
		while (nTotal < data.nLength_) {
			int nSelect = Socket::SELECT_READ | Socket::SELECT_WRITE;
			status = pSocket_->select(&nSelect);
			CHECK_QSTATUS_ERROR(NNTP_ERROR_SELECT | pSocket_->getLastError());
			CHECK_ERROR(nSelect == 0, QSTATUS_FAIL, NNTP_ERROR_TIMEOUT);
			
			int nSend = QSMIN(size_t(2048), data.nLength_ - nTotal);
			status = pSocket_->send(data.psz_ + nTotal, &nSend, 0);
			CHECK_QSTATUS_ERROR(NNTP_ERROR_SEND | pSocket_->getLastError());
			nTotal += nSend;
//			if (bProgress) {
//				status = pNntpCallback_->setPos(nTotal);
//				CHECK_QSTATUS_ERROR(NNTP_ERROR_SEND);
//			}
		}
	}
//	if (bProgress) {
//		status = pNntpCallback_->setRange(0, 0);
//		CHECK_QSTATUS_ERROR(NNTP_ERROR_SEND);
//		status = pNntpCallback_->setPos(0);
//		CHECK_QSTATUS_ERROR(NNTP_ERROR_SEND);
//	}
	
	return receive(pszMultilineCodes, nCodeCount, pnCode, pstrResponse, pstrContent);
}

QSTATUS qmnntp::Nntp::setErrorResponse(const CHAR* pszErrorResponse)
{
	freeWString(wstrErrorResponse_);
	wstrErrorResponse_ = mbs2wcs(pszErrorResponse);
	if (!wstrErrorResponse_)
		return QSTATUS_OUTOFMEMORY;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmnntp::Nntp::addContent(StringBuffer<STRING>* pBuf,
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

qmnntp::MessagesData::MessagesData(QSTATUS* pstatus) :
	strData_(0)
{
}

qmnntp::MessagesData::~MessagesData()
{
	freeString(strData_);
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

QSTATUS qmnntp::MessagesData::setData(STRING strData)
{
	assert(strData);
	
	DECLARE_QSTATUS();
	
	CHAR* p = strData;
	
	while (*p) {
		Item item = { 0 };
		int n = 0;
		CHAR* pEnd = 0;
		
		item.nId_ = strtol(p, &pEnd, 10);
		if (*pEnd != '\t')
			return QSTATUS_FAIL;
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
				return QSTATUS_FAIL;
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
					return QSTATUS_FAIL;
			}
			else {
				if (*pEnd != '\t')
					return QSTATUS_FAIL;
				p = pEnd + 1;
			}
		}
		
		status = STLWrapper<ItemList>(listItem_).push_back(item);
		CHECK_QSTATUS();
		
		p = strstr(p, "\r\n");
		if (!p)
			break;
		p += 2;
	}
	
	strData_ = strData;
	
	return QSTATUS_SUCCESS;
}
