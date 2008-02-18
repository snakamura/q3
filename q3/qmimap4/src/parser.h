/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#ifndef __PARSER_H__
#define __PARSER_H__

#include <qs.h>
#include <qsstring.h>


namespace qmimap4 {

class Parser;
class ParserCallback;

class Buffer;
class Imap4Callback;
class TokenValue;


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
							  TokenValue* pTokenValue,
							  std::pair<const CHAR*, size_t>* pToken);
	static std::auto_ptr<List> parseList(Buffer* pBuffer,
										 size_t* pnIndex,
										 Imap4Callback* pCallback);

private:
	std::auto_ptr<Response> parseResponse();
	std::auto_ptr<ResponseCapability> parseCapabilityResponse();
	std::auto_ptr<ResponseContinue> parseContinueResponse();
	std::auto_ptr<ResponseFetch> parseFetchResponse(unsigned int nNumber);
	std::auto_ptr<ResponseFlags> parseFlagsResponse();
	std::auto_ptr<ResponseList> parseListResponse(bool bList);
	std::auto_ptr<ResponseNamespace> parseNamespaceResponse();
	std::auto_ptr<ResponseSearch> parseSearchResponse();
	std::auto_ptr<ResponseStatus> parseStatusResponse();
	std::auto_ptr<State> parseStatus();
	std::auto_ptr<List> parseList();
	
	Token getNextToken(std::pair<const CHAR*, size_t>* pToken);
	Token getNextToken(const CHAR* pszSep,
					   std::pair<const CHAR*, size_t>* pToken);
	Token getNextToken(TokenValue* pTokenValue);
	Token getNextToken(const CHAR* pszSep,
					   TokenValue* pTokenValue);

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

}

#endif // __PARSER_H__
