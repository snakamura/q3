/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __PARSER_H__
#define __PARSER_H__

#include <qs.h>
#include <qssocket.h>
#include <qsstring.h>


namespace qmimap4 {

class Parser;
class ParserCallback;
class Buffer;

class Imap4Callback;


/****************************************************************************
 *
 * Parser
 *
 */

class Parser
{
public:
	enum Token {
		TOKEN_ERROR,
		TOKEN_ATOM,
		TOKEN_QUOTED,
		TOKEN_LITERAL,
		TOKEN_NIL
	};

public:
	Parser(Buffer* pBuffer,
		   Imap4Callback* pCallback);
	~Parser();

public:
	bool parse(const CHAR* pszTag,
			   bool bAcceptContinue,
			   ParserCallback* pCallback);
	qs::string_ptr getProcessedString() const;
	qs::string_ptr getUnprocessedString() const;

public:
	static Token getNextToken(Buffer* pBuffer,
							  size_t* pnIndex,
							  const CHAR* pszSep,
							  Imap4Callback* pCallback,
							  qs::string_ptr* pstrToken);
	static std::auto_ptr<List> parseList(Buffer* pBuffer,
										 size_t* pnIndex,
										 Imap4Callback* pCallback);

private:
	std::auto_ptr<Response> parseResponse();
	std::auto_ptr<ResponseCapability> parseCapabilityResponse();
	std::auto_ptr<ResponseContinue> parseContinueResponse();
	std::auto_ptr<ResponseFetch> parseFetchResponse(unsigned long nNumber);
	std::auto_ptr<ResponseFlags> parseFlagsResponse();
	std::auto_ptr<ResponseList> parseListResponse(bool bList);
	std::auto_ptr<ResponseNamespace> parseNamespaceResponse();
	std::auto_ptr<ResponseSearch> parseSearchResponse();
	std::auto_ptr<ResponseStatus> parseStatusResponse();
	std::auto_ptr<State> parseStatus();
	std::auto_ptr<List> parseList();
	
	Token getNextToken(qs::string_ptr* pstrToken);
	Token getNextToken(const CHAR* pszSep,
					   qs::string_ptr* pstrToken);

private:
	Parser(const Parser&);
	Parser& operator=(const Parser&);

private:
	Buffer* pBuffer_;
	size_t nIndex_;
	Imap4Callback* pCallback_;
};


/****************************************************************************
 *
 * ParserCallback
 *
 */

class ParserCallback
{
public:
	virtual ~ParserCallback();

public:
	virtual bool response(std::auto_ptr<Response> pResponse) = 0;
};


/****************************************************************************
 *
 * Buffer
 *
 */

class Buffer
{
public:
	Buffer(const CHAR* psz);
	Buffer(const CHAR* psz,
		   qs::SocketBase* pSocket);
	~Buffer();

public:
	CHAR get(size_t n);
	CHAR get(size_t n,
			 Imap4Callback* pCallback,
			 size_t nStart);
	size_t find(CHAR c,
				size_t n);
	size_t find(const CHAR* psz,
				size_t n);
	qs::string_ptr substr(size_t nPos,
						  size_t nLen) const;
	const CHAR* str() const;
	
	unsigned int getError() const;
	
	size_t free(size_t n);

private:
	bool receive(size_t n,
				 Imap4Callback* pCallback,
				 size_t nStart);

private:
	Buffer(const Buffer&);
	Buffer& operator=(const Buffer&);

private:
	qs::XStringBuffer<qs::XSTRING> buf_;
	qs::SocketBase* pSocket_;
	unsigned int nError_;
};

}

#endif // __PARSER_H__
