/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __POP3_H__
#define __POP3_H__

#include <qs.h>
#include <qslog.h>
#include <qssocket.h>
#include <qsssl.h>
#include <qsstring.h>

#include <vector>

namespace qmpop3 {

class Pop3;
class Pop3Callback;


/****************************************************************************
 *
 * Pop3
 *
 */

class Pop3
{
public:
	enum Error {
		POP3_ERROR_SUCCESS			= 0x00000000,
		
		POP3_ERROR_INITIALIZE		= 0x00010000,
		POP3_ERROR_CONNECT			= 0x00020000,
		POP3_ERROR_GENERATEDIGEST	= 0x00030000,
		POP3_ERROR_PARSE			= 0x00040000,
		POP3_ERROR_TIMEOUT			= 0x00050000,
		POP3_ERROR_SELECT			= 0x00060000,
		POP3_ERROR_DISCONNECT		= 0x00070000,
		POP3_ERROR_RECEIVE			= 0x00080000,
		POP3_ERROR_SEND				= 0x00090000,
		POP3_ERROR_INVALIDSOCKET	= 0x000a0000,
		POP3_ERROR_RESPONSE			= 0x000b0000,
		POP3_ERROR_SSL				= 0x000c0000,
		POP3_ERROR_OTHER			= 0x000d0000,
		POP3_ERROR_MASK_LOWLEVEL	= 0x00ff0000,
		
		POP3_ERROR_GREETING			= 0x00000100,
		POP3_ERROR_APOP				= 0x00000200,
		POP3_ERROR_USER				= 0x00000300,
		POP3_ERROR_PASS				= 0x00000400,
		POP3_ERROR_STAT				= 0x00000500,
		POP3_ERROR_LIST				= 0x00000600,
		POP3_ERROR_UIDL				= 0x00000700,
		POP3_ERROR_RETR				= 0x00000800,
		POP3_ERROR_TOP				= 0x00000900,
		POP3_ERROR_DELE				= 0x00000a00,
		POP3_ERROR_NOOP				= 0x00000b00,
		POP3_ERROR_XTNDXMIT			= 0x00000c00,
		POP3_ERROR_STLS				= 0x00000d00,
		POP3_ERROR_MASK_HIGHLEVEL	= 0x0000ff00
	};
	
	enum Ssl {
		SSL_NONE		= 0x00,
		SSL_SSL			= 0x01,
		SSL_STARTTLS	= 0x02
	};
	
private:
	enum State {
		STATE_NONE,
		STATE_PERIOD,
		STATE_CR,
		STATE_LF
	};

public:
	struct Option
	{
		long nTimeout_;
		qs::SocketCallback* pSocketCallback_;
		qs::SSLSocketCallback* pSSLSocketCallback_;
		Pop3Callback* pPop3Callback_;
		qs::Logger* pLogger_;
	};

private:
	struct SendData
	{
		const CHAR* psz_;
		size_t nLength_;
	};

public:
	typedef std::vector<unsigned int> MessageSizeList;
	typedef std::vector<qs::WSTRING> UidList;

public:
	Pop3(const Option& option, qs::QSTATUS* pstatus);
	~Pop3();

public:
	qs::QSTATUS connect(const WCHAR* pwszHost,
		short nPort, bool bApop, Ssl ssl);
	qs::QSTATUS disconnect();
	qs::QSTATUS getMessageCount() const;
	qs::QSTATUS getMessage(unsigned int nMsg, unsigned int nMaxLine,
		qs::STRING* pstrMessage, unsigned int* pnSize);
	qs::QSTATUS getMessageSize(unsigned int nMsg, unsigned int* pnSize);
	qs::QSTATUS getMessageSizes(MessageSizeList* pList);
	qs::QSTATUS deleteMessage(unsigned int nMsg);
	qs::QSTATUS getUid(unsigned int nMsg, qs::WSTRING* pwstrUid);
	qs::QSTATUS getUids(UidList* pList);
	qs::QSTATUS noop();
	qs::QSTATUS sendMessage(const CHAR* pszMessage, size_t nLen);
	
	unsigned int getLastError() const;
	const WCHAR* getLastErrorResponse() const;

private:
	qs::QSTATUS receive(qs::STRING* pstrResponse);
	qs::QSTATUS receive(qs::STRING* pstrResponse, qs::STRING* pstrContent);
	qs::QSTATUS sendCommand(const CHAR* pszCommand);
	qs::QSTATUS sendCommand(const CHAR* pszCommand, qs::STRING* pstrResponse);
	qs::QSTATUS sendCommand(const CHAR* pszCommand,
		qs::STRING* pstrResponse, qs::STRING* pstrContent);
	qs::QSTATUS send(const SendData* pSendData, size_t nDataLen, bool bProgress);
	qs::QSTATUS send(const SendData* pSendData, size_t nDataLen,
		bool bProgress, qs::STRING* pstrResponse, qs::STRING* pstrContent);
	qs::QSTATUS setErrorResponse(const CHAR* pszErrorResponse);

private:
	static qs::QSTATUS addContent(qs::StringBuffer<qs::STRING>* pBuf,
		const CHAR* psz, size_t nLen, State* pState);

private:
	Pop3(const Pop3&);
	Pop3& operator=(const Pop3&);

private:
	long nTimeout_;
	qs::SocketCallback* pSocketCallback_;
	qs::SSLSocketCallback* pSSLSocketCallback_;
	Pop3Callback* pPop3Callback_;
	qs::Logger* pLogger_;
	qs::SocketBase* pSocket_;
	unsigned int nCount_;
	unsigned int nError_;
	qs::WSTRING wstrErrorResponse_;

private:
	static const CHAR* pszOk__;
	static const CHAR* pszErr__;
};


/****************************************************************************
 *
 * Pop3Callback
 *
 */

class Pop3Callback
{
public:
	virtual ~Pop3Callback();

public:
	virtual qs::QSTATUS getUserInfo(qs::WSTRING* pwstrUserName,
		qs::WSTRING* pwstrPassword) = 0;
	virtual qs::QSTATUS setPassword(const WCHAR* pwszPassword) = 0;
	
	virtual qs::QSTATUS authenticating() = 0;
	virtual qs::QSTATUS setRange(unsigned int nMin, unsigned int nMax) = 0;
	virtual qs::QSTATUS setPos(unsigned int nPos) = 0;
};

}

#endif // __POP3_H__
