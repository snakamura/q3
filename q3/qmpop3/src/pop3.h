/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
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
	
	enum Secure {
		SECURE_NONE,
		SECURE_SSL,
		SECURE_STARTTLS
	};
	
private:
	enum State {
		STATE_NONE,
		STATE_CR1,
		STATE_LF1,
		STATE_PERIOD,
		STATE_CR2,
		STATE_LF2
	};
	
	enum {
		SEND_BLOCK_SIZE		= 8192,
		RECEIVE_BLOCK_SIZE	= 8192
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
	Pop3(long nTimeout,
		 qs::SocketCallback* pSocketCallback,
		 qs::SSLSocketCallback* pSSLSocketCallback,
		 Pop3Callback* pPop3Callback,
		 qs::Logger* pLogger);
	~Pop3();

public:
	bool connect(const WCHAR* pwszHost,
				 short nPort,
				 bool bApop,
				 Secure secure);
	void disconnect();
	unsigned int getMessageCount() const;
	bool getMessage(unsigned int nMsg,
					unsigned int nMaxLine,
					qs::xstring_size_ptr* pstrMessage,
					unsigned int nEstimatedSize);
	bool getMessageSize(unsigned int nMsg,
						unsigned int* pnSize);
	bool getMessageSizes(MessageSizeList* pList);
	bool deleteMessage(unsigned int nMsg);
	bool getUid(unsigned int nMsg,
				qs::wstring_ptr* pwstrUid);
	bool getUids(UidList* pList);
	bool noop();
	bool sendMessage(const CHAR* pszMessage,
					 size_t nLen);
	
	unsigned int getLastError() const;
	const WCHAR* getLastErrorResponse() const;

private:
	bool receive(qs::string_ptr* pstrResponse);
	bool receive(qs::string_ptr* pstrResponse,
				 qs::xstring_size_ptr* pstrContent,
				 size_t nContentSizeHint);
	bool sendCommand(const CHAR* pszCommand);
	bool sendCommand(const CHAR* pszCommand,
					 qs::string_ptr* pstrResponse);
	bool sendCommand(const CHAR* pszCommand,
					 qs::string_ptr* pstrResponse,
					 qs::xstring_size_ptr* pstrContent,
					 size_t nContentSizeHint);
	bool send(const SendData* pSendData,
			  size_t nDataLen,
			  bool bProgress);
	bool send(const SendData* pSendData,
			  size_t nDataLen,
			  bool bProgress,
			  qs::string_ptr* pstrResponse,
			  qs::xstring_size_ptr* pstrContent,
			  size_t nContentSizeHint);
	void setErrorResponse(const CHAR* pszErrorResponse);

private:
	static bool checkContent(CHAR* psz,
							 size_t* pnLen,
							 State* pState);

private:
	Pop3(const Pop3&);
	Pop3& operator=(const Pop3&);

private:
	long nTimeout_;
	qs::SocketCallback* pSocketCallback_;
	qs::SSLSocketCallback* pSSLSocketCallback_;
	Pop3Callback* pPop3Callback_;
	qs::Logger* pLogger_;
	std::auto_ptr<qs::SocketBase> pSocket_;
	unsigned int nCount_;
	unsigned int nError_;
	qs::wstring_ptr wstrErrorResponse_;

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
	virtual bool getUserInfo(qs::wstring_ptr* pwstrUserName,
							 qs::wstring_ptr* pwstrPassword) = 0;
	virtual void setPassword(const WCHAR* pwszPassword) = 0;
	
	virtual void authenticating() = 0;
	virtual void setRange(size_t nMin,
						  size_t nMax) = 0;
	virtual void setPos(size_t nPos) = 0;
};

}

#endif // __POP3_H__
