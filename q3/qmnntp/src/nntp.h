/*
 * $Id: nntp.h,v 1.1.1.1 2003/04/29 08:07:34 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __NNTP_H__
#define __NNTP_H__

#include <qs.h>
#include <qslog.h>
#include <qssocket.h>
#include <qsstring.h>

#include <vector>


namespace qmnntp {

class Nntp;
class NntpCallback;
class MessagesData;


/****************************************************************************
 *
 * Nntp
 *
 */

class Nntp
{
public:
	enum Error {
		NNTP_ERROR_SUCCESS			= 0x00000000,
		
		NNTP_ERROR_INITIALIZE		= 0x00010000,
		NNTP_ERROR_CONNECT			= 0x00020000,
		NNTP_ERROR_RESPONSE			= 0x00030000,
		NNTP_ERROR_INVALIDSOCKET	= 0x00040000,
		NNTP_ERROR_OTHER			= 0x00050000,
		NNTP_ERROR_SELECT			= 0x00060000,
		NNTP_ERROR_TIMEOUT			= 0x00070000,
		NNTP_ERROR_RECEIVE			= 0x00080000,
		NNTP_ERROR_DISCONNECT		= 0x00090000,
		NNTP_ERROR_SEND				= 0x000a0000,
		NNTP_ERROR_MASK_LOWLEVEL	= 0x00ff0000,
		
		NNTP_ERROR_GREETING			= 0x00000100,
		NNTP_ERROR_GROUP			= 0x00000200,
		NNTP_ERROR_AUTHINFO			= 0x00000300,
		NNTP_ERROR_ARTICLE			= 0x00000400,
		NNTP_ERROR_HEAD				= 0x00000500,
		NNTP_ERROR_BODY				= 0x00000600,
		NNTP_ERROR_XOVER			= 0x00000700,
		NNTP_ERROR_MODEREADER		= 0x00000800,
		NNTP_ERROR_POST				= 0x00000900,
		NNTP_ERROR_MASK_HIGHLEVEL	= 0x0000ff00
	};
	
	enum GetMessageFlag {
		GETMESSAGEFLAG_ARTICLE	= 0,
		GETMESSAGEFLAG_HEAD		= 1,
		GETMESSAGEFLAG_BODY		= 2
	};

public:
	struct Option
	{
		long nTimeout_;
		qs::SocketCallback* pSocketCallback_;
		NntpCallback* pNntpCallback_;
		qs::Logger* pLogger_;
	};

private:
	enum State {
		STATE_NONE,
		STATE_PERIOD,
		STATE_CR,
		STATE_LF
	};

private:
	struct SendData
	{
		const CHAR* psz_;
		size_t nLength_;
	};

public:
	Nntp(const Option& option, qs::QSTATUS* pstatus);
	~Nntp();

public:
	qs::QSTATUS connect(const WCHAR* pwszHost, short nPort, bool bSsl);
	qs::QSTATUS disconnect();
	const WCHAR* getGroup() const;
	unsigned int getEstimatedCount() const;
	unsigned int getFirst() const;
	unsigned int getLast() const;
	qs::QSTATUS group(const WCHAR* pwszGroup);
	qs::QSTATUS getMessage(unsigned int n,
		GetMessageFlag flag, qs::STRING* pstrMessage);
	qs::QSTATUS getMessage(const WCHAR* pwszMessageId,
		GetMessageFlag flag, qs::STRING* pstrMessage);
	qs::QSTATUS getMessagesData(unsigned int nStart,
		unsigned int nEnd, MessagesData** ppMessageData);
	qs::QSTATUS postMessage(const CHAR* pszMessage, size_t nLen);
	
	unsigned int getLastError() const;
	const WCHAR* getLastErrorResponse() const;

private:
	qs::QSTATUS getMessage(unsigned int n, const WCHAR* pwszMessageId,
		GetMessageFlag flag, qs::STRING* pstrMessage);
	qs::QSTATUS receive(unsigned int* pnCode);
	qs::QSTATUS receive(unsigned int* pnCode, qs::STRING* pstrResponse);
	qs::QSTATUS receive(const CHAR* pszMultilineCodes[], size_t nCodeCount,
		unsigned int* pnCode, qs::STRING* pstrResponse, qs::STRING* pstrContent);
	qs::QSTATUS sendCommand(const CHAR* pszCommand, unsigned int* pnCode);
	qs::QSTATUS sendCommand(const CHAR* pszCommand,
		unsigned int* pnCode, qs::STRING* pstrResponse);
	qs::QSTATUS sendCommand(const CHAR* pszCommand,
		const CHAR* pszMultilineCodes[], size_t nCodeCount,
		unsigned int* pnCode, qs::STRING* pstrResponse, qs::STRING* pstrContent);
	qs::QSTATUS send(const SendData* pSendData, size_t nDataLen,
		const CHAR* pszMultilineCodes[], size_t nCodeCount,
		unsigned int* pnCode, qs::STRING* pstrResponse, qs::STRING* pstrContent);
	qs::QSTATUS setErrorResponse(const CHAR* pszErrorResponse);

private:
	static qs::QSTATUS addContent(qs::StringBuffer<qs::STRING>* pBuf,
		const CHAR* psz, size_t nLen, State* pState);

private:
	Nntp(const Nntp&);
	Nntp& operator=(const Nntp&);

private:
	long nTimeout_;
	qs::SocketCallback* pSocketCallback_;
	NntpCallback* pNntpCallback_;
	qs::Logger* pLogger_;
	qs::Socket* pSocket_;
	qs::WSTRING wstrGroup_;
	unsigned int nCount_;
	unsigned int nFirst_;
	unsigned int nLast_;
	unsigned int nError_;
	qs::WSTRING wstrErrorResponse_;
};


/****************************************************************************
 *
 * NntpCallback
 *
 */

class NntpCallback
{
public:
	virtual ~NntpCallback();

public:
	virtual qs::QSTATUS getUserInfo(qs::WSTRING* pwstrUserName,
		qs::WSTRING* pwstrPassword) = 0;
	virtual qs::QSTATUS setPassword(const WCHAR* pwszPassword) = 0;
	
	virtual qs::QSTATUS authenticating() = 0;
	virtual qs::QSTATUS setRange(unsigned int nMin, unsigned int nMax) = 0;
	virtual qs::QSTATUS setPos(unsigned int nPos) = 0;
};


/****************************************************************************
 *
 * MessagesData
 *
 */

class MessagesData
{
public:
	struct Item
	{
		unsigned int nId_;
		const CHAR* pszSubject_;
		const CHAR* pszFrom_;
		const CHAR* pszDate_;
		const CHAR* pszMessageId_;
		const CHAR* pszReferences_;
		unsigned int nBytes_;
		unsigned int nLine_;
	};

public:
	MessagesData(qs::QSTATUS* pstatus);
	~MessagesData();

public:
	size_t getCount() const;
	const Item& getItem(size_t n) const;

public:
	qs::QSTATUS setData(qs::STRING strData);

private:
	MessagesData(const MessagesData&);
	MessagesData& operator=(const MessagesData&);

private:
	typedef std::vector<Item> ItemList;

private:
	qs::STRING strData_;
	ItemList listItem_;
};

}

#endif // __NNTP_H__
