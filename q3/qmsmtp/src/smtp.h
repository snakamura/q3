/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __SMTP_H__
#define __SMTP_H__

#include <qs.h>
#include <qslog.h>
#include <qssocket.h>
#include <qsssl.h>


namespace qmsmtp {

class Smtp;
class SmtpCallback;


/****************************************************************************
 *
 * Smtp
 *
 */

class Smtp
{
public:
	enum {
		SMTP_ERROR_SUCCESS			= 0x00000000,
		
		SMTP_ERROR_INITIALIZE		= 0x00010000,
		SMTP_ERROR_CONNECT			= 0x00020000,
		SMTP_ERROR_RESPONSE			= 0x00030000,
		SMTP_ERROR_INVALIDSOCKET	= 0x00040000,
		SMTP_ERROR_TIMEOUT			= 0x00050000,
		SMTP_ERROR_SELECT			= 0x00060000,
		SMTP_ERROR_DISCONNECT		= 0x00070000,
		SMTP_ERROR_RECEIVE			= 0x00080000,
		SMTP_ERROR_SEND				= 0x00090000,
		SMTP_ERROR_OTHER			= 0x000a0000,
		SMTP_ERROR_SSL				= 0x000b0000,
		SMTP_ERROR_MASK_LOWLEVEL	= 0x00ff0000,
		
		SMTP_ERROR_GREETING			= 0x00000100,
		SMTP_ERROR_HELO				= 0x00000200,
		SMTP_ERROR_EHLO				= 0x00000300,
		SMTP_ERROR_AUTH				= 0x00000400,
		SMTP_ERROR_MAIL				= 0x00000500,
		SMTP_ERROR_RCPT				= 0x00000600,
		SMTP_ERROR_DATA				= 0x00000700,
		SMTP_ERROR_STARTTLS			= 0x00000800,
		SMTP_ERROR_MASK_HIGHLEVEL	= 0x0000ff00,
	};
	
	enum Auth {
		AUTH_LOGIN		= 0x01,
		AUTH_PLAIN		= 0x02,
		AUTH_CRAMMD5	= 0x04
	};
	
	enum Secure {
		SECURE_NONE,
		SECURE_SSL,
		SECURE_STARTTLS
	};

public:
	struct SendMessageData
	{
		const CHAR* pszEnvelopeFrom_;
		const CHAR** ppszAddresses_;
		size_t nAddressSize_;
		const CHAR* pszMessage_;
		size_t nLength_;
	};

private:
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
	Smtp(long nTimeout,
		 qs::SocketCallback* pSocketCallback,
		 qs::SSLSocketCallback* pSSLSocketCallback,
		 SmtpCallback* pSmtpCallback,
		 qs::Logger* pLogger);
	~Smtp();

public:
	bool connect(const WCHAR* pwszHost,
				 short nPort,
				 Secure secure);
	void disconnect();
	bool sendMessage(const SendMessageData& data);
	
	unsigned int getLastError() const;
	const WCHAR* getLastErrorResponse() const;

private:
	bool helo(unsigned int* pnAuth,
			  bool* pbStartTls);
	bool receive(unsigned int* pnCode);
	bool receive(unsigned int* pnCode,
				 qs::string_ptr* pstrResponse);
	bool send(const SendData* pSendData,
			  size_t nDataLen,
			  bool bProgress,
			  unsigned int* pnCode,
			  qs::string_ptr* pstrResponse);
	bool sendCommand(const CHAR* psz,
					 unsigned int* pnCode);
	bool sendCommand(const CHAR* psz,
					 unsigned int* pnCode,
					 qs::string_ptr* pstrResponse);
	void setErrorResponse(const CHAR* pszErrorResponse);
	unsigned int getAuthMethods();

private:
	Smtp(const Smtp&);
	Smtp& operator=(const Smtp&);

private:
	long nTimeout_;
	qs::SocketCallback* pSocketCallback_;
	qs::SSLSocketCallback* pSSLSocketCallback_;
	SmtpCallback* pSmtpCallback_;
	qs::Logger* pLogger_;
	std::auto_ptr<qs::SocketBase> pSocket_;
	unsigned int nError_;
	qs::wstring_ptr wstrErrorResponse_;
};


/****************************************************************************
 *
 * SmtpCallback
 *
 */

class SmtpCallback
{
public:
	virtual ~SmtpCallback();

public:
	virtual bool getUserInfo(qs::wstring_ptr* pwstrUserName,
							 qs::wstring_ptr* pwstrPassword) = 0;
	virtual void setPassword(const WCHAR* pwszPassword) = 0;
	virtual qs::wstring_ptr getLocalHost() = 0;
	virtual qs::wstring_ptr getAuthMethods() = 0;
	
	virtual void authenticating() = 0;
	virtual void setRange(unsigned int nMin,
						  unsigned int nMax) = 0;
	virtual void setPos(unsigned int nPos) = 0;
};

}

#endif // __SMTP_H__
