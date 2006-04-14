/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __NNTP_H__
#define __NNTP_H__

#include <qs.h>
#include <qslog.h>
#include <qssocket.h>
#include <qsssl.h>
#include <qsstring.h>

#include <vector>


namespace qmnntp {

class Nntp;
class NntpCallback;
class MessagesData;
class GroupsData;


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
		NNTP_ERROR_SSL				= 0x000b0000,
		NNTP_ERROR_PARSE			= 0x000c0000,
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
		NNTP_ERROR_LIST				= 0x00000a00,
		NNTP_ERROR_NEWGROUPS		= 0x00000b00,
		NNTP_ERROR_MASK_HIGHLEVEL	= 0x0000ff00
	};
	
	enum GetMessageFlag {
		GETMESSAGEFLAG_ARTICLE	= 0,
		GETMESSAGEFLAG_HEAD		= 1,
		GETMESSAGEFLAG_BODY		= 2
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
	Nntp(long nTimeout,
		 qs::SocketCallback* pSocketCallback,
		 qs::SSLSocketCallback* pSSLSocketCallback,
		 NntpCallback* pNntpCallback,
		 qs::Logger* pLogger);
	~Nntp();

public:
	bool connect(const WCHAR* pwszHost,
				 short nPort,
				 bool bSsl);
	void disconnect();
	const WCHAR* getGroup() const;
	unsigned int getEstimatedCount() const;
	unsigned int getFirst() const;
	unsigned int getLast() const;
	bool group(const WCHAR* pwszGroup);
	bool getMessage(unsigned int n,
					GetMessageFlag flag,
					qs::xstring_size_ptr* pstrMessage,
					unsigned int nEstimatedSize);
	bool getMessage(const WCHAR* pwszMessageId,
					GetMessageFlag flag,
					qs::xstring_size_ptr* pstrMessage,
					unsigned int nEstimatedSize);
	bool getMessagesData(unsigned int nStart,
						 unsigned int nEnd,
						 std::auto_ptr<MessagesData>* ppMessagesData);
	bool postMessage(const CHAR* pszMessage,
					 size_t nLen);
	bool list(std::auto_ptr<GroupsData>* ppGroupsData);
	bool newGroups(const WCHAR* pwszDate,
				   const WCHAR* pwszTime,
				   bool bGMT,
				   std::auto_ptr<GroupsData>* ppGroupsData);
	
	unsigned int getLastError() const;
	const WCHAR* getLastErrorResponse() const;

private:
	bool getMessage(unsigned int n,
					const WCHAR* pwszMessageId,
					GetMessageFlag flag,
					qs::xstring_size_ptr* pstrMessage,
					unsigned int nEstimatedSize);
	bool receive(unsigned int* pnCode);
	bool receive(unsigned int* pnCode,
				 qs::string_ptr* pstrResponse);
	bool receive(const CHAR* pszMultilineCodes[],
				 size_t nCodeCount,
				 unsigned int* pnCode,
				 qs::string_ptr* pstrResponse,
				 qs::xstring_size_ptr* pstrContent);
	bool sendCommand(const CHAR* pszCommand,
					 unsigned int* pnCode);
	bool sendCommand(const CHAR* pszCommand,
					 unsigned int* pnCode,
					 qs::string_ptr* pstrResponse);
	bool sendCommand(const CHAR* pszCommand,
					 const CHAR* pszMultilineCodes[],
					 size_t nCodeCount,
					 unsigned int* pnCode,
					 qs::string_ptr* pstrResponse,
					 qs::xstring_size_ptr* pstrContent);
	bool send(const SendData* pSendData,
			  size_t nDataLen,
			  const CHAR* pszMultilineCodes[],
			  size_t nCodeCount,
			  unsigned int* pnCode,
			  qs::string_ptr* pstrResponse,
			  qs::xstring_size_ptr* pstrContent);
	void setErrorResponse(const CHAR* pszErrorResponse);

private:
	static bool checkContent(CHAR* psz,
							 size_t* pnLen,
							 State* pState);

private:
	Nntp(const Nntp&);
	Nntp& operator=(const Nntp&);

private:
	long nTimeout_;
	qs::SocketCallback* pSocketCallback_;
	qs::SSLSocketCallback* pSSLSocketCallback_;
	NntpCallback* pNntpCallback_;
	qs::Logger* pLogger_;
	std::auto_ptr<qs::SocketBase> pSocket_;
	qs::wstring_ptr wstrGroup_;
	unsigned int nCount_;
	unsigned int nFirst_;
	unsigned int nLast_;
	unsigned int nError_;
	qs::wstring_ptr wstrErrorResponse_;
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
	virtual bool getUserInfo(qs::wstring_ptr* pwstrUserName,
							 qs::wstring_ptr* pwstrPassword) = 0;
	virtual void setPassword(const WCHAR* pwszPassword) = 0;
	
	virtual void authenticating() = 0;
	virtual void setRange(size_t nMin,
						  size_t nMax) = 0;
	virtual void setPos(size_t nPos) = 0;
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
	MessagesData();
	~MessagesData();

public:
	size_t getCount() const;
	const Item& getItem(size_t n) const;

public:
	bool setData(qs::xstring_ptr strData,
				 size_t nEstimatedSize);

private:
	MessagesData(const MessagesData&);
	MessagesData& operator=(const MessagesData&);

private:
	typedef std::vector<Item> ItemList;

private:
	qs::xstring_ptr strData_;
	ItemList listItem_;
};


/****************************************************************************
 *
 * GroupsData
 *
 */

class GroupsData
{
public:
	struct Item
	{
		const CHAR* pszGroup_;
		unsigned int nLast_;
		unsigned int nFirst_;
		const CHAR* pszPost_;
	};

public:
	GroupsData();
	~GroupsData();

public:
	size_t getCount() const;
	const Item& getItem(size_t n) const;

public:
	bool setData(qs::xstring_ptr strData,
				 size_t nEstimatedSize);

private:
	GroupsData(const GroupsData&);
	GroupsData& operator=(const GroupsData&);

private:
	typedef std::vector<Item> ItemList;

private:
	qs::xstring_ptr strData_;
	ItemList listItem_;
};

}

#endif // __NNTP_H__
