/*
 * $Id: parser.h,v 1.1.1.1 2003/04/29 08:07:34 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
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
	Parser(Buffer* pBuffer, Imap4Callback* pCallback, qs::QSTATUS* pstatus);
	~Parser();

public:
	qs::QSTATUS parse(const CHAR* pszTag,
		bool bAcceptContinue, ParserCallback* pCallback);
	qs::QSTATUS getProcessedString(qs::STRING* pstr) const;
	qs::QSTATUS getUnprocessedString(qs::STRING* pstr) const;

public:
	static qs::QSTATUS getNextToken(Buffer* pBuffer, size_t* pnIndex,
		const CHAR* pszSep, Imap4Callback* pCallback,
		Token* pToken, qs::STRING* pstrToken);
	static qs::QSTATUS parseList(Buffer* pBuffer,
		size_t* pnIndex, Imap4Callback* pCallback, List** ppList);

private:
	qs::QSTATUS parseResponse(Response** ppResponse);
	qs::QSTATUS parseCapabilityResponse(ResponseCapability** ppCapability);
	qs::QSTATUS parseContinueResponse(ResponseContinue** ppContinue);
	qs::QSTATUS parseFetchResponse(unsigned long nNumber, ResponseFetch** ppFetch);
	qs::QSTATUS parseFlagsResponse(ResponseFlags** ppFlags);
	qs::QSTATUS parseListResponse(bool bList, ResponseList** ppList);
	qs::QSTATUS parseNamespaceResponse(ResponseNamespace** ppNamespace);
	qs::QSTATUS parseSearchResponse(ResponseSearch** ppSearch);
	qs::QSTATUS parseStatusResponse(ResponseStatus** ppStatus);
	qs::QSTATUS parseStatus(State** ppState);
	qs::QSTATUS parseList(List** ppList);
	
	qs::QSTATUS getNextToken(Token* pToken, qs::STRING* pstrToken);
	qs::QSTATUS getNextToken(Token* pToken,
		qs::STRING* pstrToken, const CHAR* pszSep);

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
	virtual qs::QSTATUS response(Response* pResponse) = 0;
};


/****************************************************************************
 *
 * Buffer
 *
 */

class Buffer
{
public:
	Buffer(const CHAR* psz, qs::QSTATUS* pstatus);
	Buffer(const CHAR* psz, qs::Socket* pSocket, qs::QSTATUS* pstatus);
	~Buffer();

public:
	CHAR get(size_t n);
	CHAR get(size_t n, Imap4Callback* pCallback, size_t nStart);
	size_t find(CHAR c, size_t n);
	size_t find(const CHAR* psz, size_t n);
	qs::QSTATUS substr(size_t nPos, size_t nLen, qs::STRING* pstr) const;
	const CHAR* str() const;
	
	unsigned int getError() const;

private:
	qs::QSTATUS receive(size_t n, Imap4Callback* pCallback, size_t nStart);

private:
	Buffer(const Buffer&);
	Buffer& operator=(const Buffer&);

private:
	qs::StringBuffer<qs::STRING> str_;
	qs::Socket* pSocket_;
	unsigned int nError_;
};

}

#endif // __PARSER_H__
