/*
 * $Id: parser.cpp,v 1.1.1.1 2003/04/29 08:07:34 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsnew.h>

#include "imap4.h"
#include "parser.h"

#pragma warning(disable:4786)

using namespace qmimap4;
using namespace qs;


/****************************************************************************
 *
 * Parser
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

qmimap4::Parser::Parser(Buffer* pBuffer,
	Imap4Callback* pCallback, qs::QSTATUS* pstatus) :
	pBuffer_(pBuffer),
	nIndex_(0),
	pCallback_(pCallback)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qmimap4::Parser::~Parser()
{
}

QSTATUS qmimap4::Parser::parse(const CHAR* pszTag,
	bool bAcceptContinue, ParserCallback* pCallback)
{
	assert(pszTag);
	
	DECLARE_QSTATUS();
	
	while (true) {
		Token token;
		string_ptr<STRING> strToken;
		status = getNextToken(&token, &strToken);
		CHECK_QSTATUS();
		if (token != TOKEN_ATOM)
			return QSTATUS_FAIL;
		
		if (strcmp(strToken.get(), "*") == 0) {
			Response* pResponse = 0;
			status = parseResponse(&pResponse);
			CHECK_QSTATUS();
			status = pCallback->response(pResponse);
			CHECK_QSTATUS();
			if (!*pszTag)
				break;
		}
		else if (strcmp(strToken.get(), "+") == 0) {
			ResponseContinue* pContinue = 0;
			status = parseContinueResponse(&pContinue);
			CHECK_QSTATUS();
			status = pCallback->response(pContinue);
			CHECK_QSTATUS();
			if (bAcceptContinue)
				break;
		}
		else {
			bool bEnd = strcmp(strToken.get(), pszTag) == 0;
			Response* pResponse = 0;
			status = parseResponse(&pResponse);
			CHECK_QSTATUS();
			status = pCallback->response(pResponse);
			CHECK_QSTATUS();
			if (bEnd)
				break;
		}
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Parser::getProcessedString(STRING* pstr) const
{
	return pBuffer_->substr(0, nIndex_, pstr);
}

QSTATUS qmimap4::Parser::getUnprocessedString(STRING* pstr) const
{
	return pBuffer_->substr(nIndex_, static_cast<size_t>(-1), pstr);
}

QSTATUS qmimap4::Parser::getNextToken(Buffer* pBuffer, size_t* pnIndex,
	const CHAR* pszSep, Imap4Callback* pCallback,
	Token* pToken, STRING* pstrToken)
{
	assert(pBuffer);
	assert(pnIndex);
	assert(pszSep);
	assert(pToken);
	assert(pstrToken);
	
	DECLARE_QSTATUS();
	
	*pToken = TOKEN_ERROR;
	*pstrToken = 0;
	
	size_t& nIndex = *pnIndex;
	
	StringBuffer<STRING> token(&status);
	CHECK_QSTATUS();
	
	CHAR c = pBuffer->get(nIndex);
	if (c == '\0') {
		return QSTATUS_FAIL;
	}
	else if (c == '\"') {
		for (++nIndex; ; ++nIndex) {
			c = pBuffer->get(nIndex);
			if (c == '\0') {
				return QSTATUS_FAIL;
			}
			else if (c == '\\') {
				CHAR cNext = pBuffer->get(nIndex + 1);
				if (cNext == '\0') {
					return QSTATUS_FAIL;
				}
				else if (cNext == '\\' || cNext == '\"') {
					status = token.append(cNext);
					CHECK_QSTATUS();
					++nIndex;
				}
				else {
					status = token.append(c);
					CHECK_QSTATUS();
				}
			}
			else if (c == '\"') {
				// Because of BUG of iMAIL
				// I skip '"' followed by any character but the separaters.
				CHAR cNext = pBuffer->get(nIndex + 1);
				if (cNext == '\0')
					return QSTATUS_FAIL;
				else if (strchr(pszSep, cNext) || cNext == ' ' || cNext == '\r')
					break;
			}
			else {
				status = token.append(c);
				CHECK_QSTATUS();
			}
		}
		++nIndex;
		*pToken = TOKEN_QUOTED;
	}
	else if (c == '{') {
		size_t nIndexEnd = pBuffer->find("}\r\n", nIndex + 1);
		if (nIndexEnd == static_cast<size_t>(-1))
			return QSTATUS_FAIL;
		
		CHAR* pEnd = 0;
		long nLen = strtol(pBuffer->str() + nIndex + 1, &pEnd, 10);
		if (*pEnd != '}')
			return QSTATUS_FAIL;
		nIndex = nIndexEnd + 3;
		
		if (pCallback && nLen >= 1024) {
			status = pCallback->setRange(0, nLen);
			CHECK_QSTATUS();
		}
		
		if (pBuffer->get(nIndex + nLen, pCallback, nIndex) == '\0')
			return QSTATUS_FAIL;
		
		if (pCallback && nLen >= 1024) {
			status = pCallback->setRange(0, 0);
			CHECK_QSTATUS();
			status = pCallback->setPos(0);
			CHECK_QSTATUS();
		}
		
		status = token.append(pBuffer->str() + nIndex, nLen);
		CHECK_QSTATUS();
		nIndex += nLen;
		*pToken = TOKEN_LITERAL;
	}
	else {
		size_t nIndexBegin = nIndex;
		for (; ; ++nIndex) {
			c = pBuffer->get(nIndex);
			if (c == '\0')
				return QSTATUS_FAIL;
			if (c == '[') {
				nIndex = pBuffer->find(']', nIndex + 1);
				if (nIndex == static_cast<size_t>(-1))
					return QSTATUS_FAIL;
			}
			else if (strchr(pszSep, c)) {
				break;
			}
		}
		
		if (nIndex - nIndexBegin == 3 &&
			strncmp(pBuffer->str() + nIndexBegin, "NIL", 3) == 0) {
			*pToken = TOKEN_NIL;
		}
		else {
			status = token.append(pBuffer->str() + nIndexBegin, nIndex - nIndexBegin);
			CHECK_QSTATUS();
			*pToken = TOKEN_ATOM;
		}
	}
	
	if (pBuffer->get(nIndex) == ' ')
		++nIndex;
	if (pBuffer->getError() != Imap4::IMAP4_ERROR_SUCCESS)
		return QSTATUS_FAIL;
	
	if (*pToken != TOKEN_NIL)
		*pstrToken = token.getString();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Parser::parseList(Buffer* pBuffer,
	size_t* pnIndex, Imap4Callback* pCallback, List** ppList)
{
	assert(pBuffer);
	assert(pnIndex);
	assert(ppList);
	assert(pBuffer->get(*pnIndex) == '(');
	
	DECLARE_QSTATUS();
	
	std::auto_ptr<List> pList;
	status = newQsObject(&pList);
	CHECK_QSTATUS();
	
	size_t& nIndex = *pnIndex;
	++nIndex;
	CHAR c = '\0';
	while ((c = pBuffer->get(nIndex)) != ')') {
		if (c == '\0') {
			return QSTATUS_FAIL;
		}
		else if (c == '(') {
			List* p = 0;
			status = parseList(pBuffer, pnIndex, pCallback, &p);
			CHECK_QSTATUS();
			std::auto_ptr<List> pChildList(p);
			status = pList->add(pChildList.get());
			CHECK_QSTATUS();
			pChildList.release();
		}
		else {
			// Because of BUG of iMAIL,
			// I add '(' to the separator.
			Token token;
			string_ptr<STRING> strToken;
			status = getNextToken(pBuffer, pnIndex,
				" ()", pCallback, &token, &strToken);
			CHECK_QSTATUS();
			
			std::auto_ptr<ListItem> pItem;
			if (token == TOKEN_NIL) {
				std::auto_ptr<ListItemNil> pNil;
				status = newQsObject(&pNil);
				CHECK_QSTATUS();
				pItem.reset(pNil.release());
			}
			else {
				std::auto_ptr<ListItemText> pText;
				status = newQsObject(strToken.get(), &pText);
				CHECK_QSTATUS();
				strToken.release();
				pItem.reset(pText.release());
			}
			status = pList->add(pItem.get());
			CHECK_QSTATUS();
			pItem.release();
			
			// Because of BUG of iMAIL
			// I skip space.
			while (pBuffer->get(nIndex) == ' ')
				++nIndex;
		}
	}
	
	++nIndex;
	while (pBuffer->get(nIndex) == ' ')
		++nIndex;
	if (pBuffer->getError() != Imap4::IMAP4_ERROR_SUCCESS)
		return QSTATUS_FAIL;
	
	*ppList = pList.release();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Parser::parseResponse(Response** ppResponse)
{
	assert(ppResponse);
	
	DECLARE_QSTATUS();
	
	std::auto_ptr<Response> pResponse;
	
	string_ptr<STRING> strToken;
	Token token;
	status = getNextToken(&token, &strToken, " \r");
	CHECK_QSTATUS();
	if (token != TOKEN_ATOM)
		return QSTATUS_FAIL;
	
	ResponseState::Flag flag = ResponseState::FLAG_UNKNOWN;
	if (strcmp(strToken.get(), "OK") == 0) {
		flag = ResponseState::FLAG_OK;
	}
	else if (strcmp(strToken.get(), "NO") == 0) {
		flag = ResponseState::FLAG_NO;
	}
	else if (strcmp(strToken.get(), "BAD") == 0) {
		flag = ResponseState::FLAG_BAD;
	}
	else if (strcmp(strToken.get(), "PREAUTH") == 0) {
		flag = ResponseState::FLAG_PREAUTH;
	}
	else if (strcmp(strToken.get(), "BYE") == 0) {
		flag = ResponseState::FLAG_BYE;
	}
	else if (strcmp(strToken.get(), "CAPABILITY") == 0) {
		ResponseCapability* pCapability = 0;
		status = parseCapabilityResponse(&pCapability);
		CHECK_QSTATUS();
		pResponse.reset(pCapability);
	}
	else if (strcmp(strToken.get(), "LIST") == 0) {
		ResponseList* pList = 0;
		status = parseListResponse(true, &pList);
		CHECK_QSTATUS();
		pResponse.reset(pList);
	}
	else if (strcmp(strToken.get(), "LSUB") == 0) {
		ResponseList* pList = 0;
		status = parseListResponse(false, &pList);
		CHECK_QSTATUS();
		pResponse.reset(pList);
	}
	else if (strcmp(strToken.get(), "STATUS") == 0) {
		ResponseStatus* pStatus = 0;
		status = parseStatusResponse(&pStatus);
		CHECK_QSTATUS();
		pResponse.reset(pStatus);
	}
	else if (strcmp(strToken.get(), "SEARCH") == 0) {
		ResponseSearch* pSearch = 0;
		status = parseSearchResponse(&pSearch);
		CHECK_QSTATUS();
		pResponse.reset(pSearch);
	}
	else if (strcmp(strToken.get(), "FLAGS") == 0) {
		ResponseFlags* pFlags = 0;
		status = parseFlagsResponse(&pFlags);
		CHECK_QSTATUS();
		pResponse.reset(pFlags);
	}
	else if (strcmp(strToken.get(), "NAMESPACE") == 0) {
		ResponseNamespace* pNamespace = 0;
		status = parseNamespaceResponse(&pNamespace);
		CHECK_QSTATUS();
		pResponse.reset(pNamespace);
	}
	else {
		CHAR* pEnd = 0;
		long nNumber = strtol(strToken.get(), &pEnd, 10);
		if (*pEnd)
			return QSTATUS_FAIL;
		
		strToken.reset(0);
		status = getNextToken(&token, &strToken, " \r");
		CHECK_QSTATUS();
		if (token != TOKEN_ATOM)
			return QSTATUS_FAIL;
		
		CHAR c = pBuffer_->get(nIndex_);
		if (c == '\0') {
			return QSTATUS_FAIL;
		}
		else if (c == '\r') {
			if (pBuffer_->get(nIndex_ + 1) != '\n')
				return QSTATUS_FAIL;
			else
				nIndex_ += 2;
		}
		
		if (strcmp(strToken.get(), "EXISTS") == 0) {
			ResponseExists* pExists = 0;
			status = newQsObject(nNumber, &pExists);
			CHECK_QSTATUS();
			pResponse.reset(pExists);
		}
		else if (strcmp(strToken.get(), "RECENT") == 0) {
			ResponseRecent* pRecent = 0;
			status = newQsObject(nNumber, &pRecent);
			CHECK_QSTATUS();
			pResponse.reset(pRecent);
		}
		else if (strcmp(strToken.get(), "EXPUNGE") == 0) {
			ResponseExpunge* pExpunge = 0;
			status = newQsObject(nNumber, &pExpunge);
			CHECK_QSTATUS();
			pResponse.reset(pExpunge);
		}
		else if (strcmp(strToken.get(), "FETCH") == 0) {
			ResponseFetch* pFetch = 0;
			status = parseFetchResponse(nNumber, &pFetch);
			CHECK_QSTATUS();
			pResponse.reset(pFetch);
		}
		else {
			return QSTATUS_FAIL;
		}
	}
	
	if (flag != ResponseState::FLAG_UNKNOWN) {
		std::auto_ptr<ResponseState> pResponseState;
		status = newQsObject(flag, &pResponseState);
		CHECK_QSTATUS();
		State* pState = 0;
		status = parseStatus(&pState);
		CHECK_QSTATUS();
		pResponseState->setState(pState);
		pResponse.reset(pResponseState.release());
	}
	
	*ppResponse = pResponse.release();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Parser::parseCapabilityResponse(ResponseCapability** ppCapability)
{
	assert(ppCapability);
	
	DECLARE_QSTATUS();
	
	*ppCapability = 0;
	
	std::auto_ptr<ResponseCapability> pCapability;
	status = newQsObject(&pCapability);
	CHECK_QSTATUS();
	
	while (true) {
		Token token;
		string_ptr<STRING> strToken;
		status = getNextToken(&token, &strToken, " \r");
		CHECK_QSTATUS();
		if (token != TOKEN_ATOM)
			return QSTATUS_FAIL;
		
		status = pCapability->add(strToken.get());
		CHECK_QSTATUS();
		
		CHAR c1 = pBuffer_->get(nIndex_);
		if (c1 == '\0')
			return QSTATUS_FAIL;
		CHAR c2 = pBuffer_->get(nIndex_ + 1);
		if (c2 == '\0')
			return QSTATUS_FAIL;
		if (c1 == '\r' && c2 == '\n') {
			nIndex_ += 2;
			break;
		}
	}
	
	*ppCapability = pCapability.release();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Parser::parseContinueResponse(ResponseContinue** ppContinue)
{
	assert(ppContinue);
	
	DECLARE_QSTATUS();
	
	*ppContinue = 0;
	
	std::auto_ptr<State> pState;
	
	CHAR c = pBuffer_->get(nIndex_);
	if (c == '\0') {
		return QSTATUS_FAIL;
	}
	else if (pBuffer_->get(nIndex_ - 1) == ' ') {
		State* p = 0;
		status = parseStatus(&p);
		CHECK_QSTATUS();
		pState.reset(p);
	}
	else {
		nIndex_ = pBuffer_->find("\r\n", nIndex_);
		if (nIndex_ == static_cast<size_t>(-1))
			return QSTATUS_FAIL;
		nIndex_ += 2;
	}
	
	status = newQsObject(pState.get(), ppContinue);
	CHECK_QSTATUS();
	pState.release();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Parser::parseFetchResponse(
	unsigned long nNumber, ResponseFetch** ppFetch)
{
	assert(ppFetch);
	
	DECLARE_QSTATUS();
	
	*ppFetch = 0;
	
	List* p = 0;
	status = parseList(&p);
	CHECK_QSTATUS();
	std::auto_ptr<List> pList(p);
	
	std::auto_ptr<ResponseFetch> pFetch;
	status = newQsObject(nNumber, pList.get(), &pFetch);
	CHECK_QSTATUS();
	
	nIndex_ = pBuffer_->find("\r\n", nIndex_);
	if (nIndex_ == static_cast<size_t>(-1))
		return QSTATUS_FAIL;
	nIndex_ += 2;
	
	*ppFetch = pFetch.release();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Parser::parseFlagsResponse(ResponseFlags** ppFlags)
{
	assert(ppFlags);
	
	DECLARE_QSTATUS();
	
	*ppFlags = 0;
	
	List* p = 0;
	status = parseList(&p);
	CHECK_QSTATUS();
	std::auto_ptr<List> pList(p);
	
	std::auto_ptr<ResponseFlags> pFlags;
	status = newQsObject(pList.get(), &pFlags);
	CHECK_QSTATUS();
	
	nIndex_ = pBuffer_->find("\r\n", nIndex_);
	if (nIndex_ == static_cast<size_t>(-1))
		return QSTATUS_FAIL;
	nIndex_ += 2;
	
	*ppFlags = pFlags.release();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Parser::parseListResponse(bool bList, ResponseList** ppList)
{
	assert(ppList);
	
	DECLARE_QSTATUS();
	
	*ppList = 0;
	
	List* p = 0;
	status = parseList(&p);
	CHECK_QSTATUS();
	std::auto_ptr<List> pList(p);
	
	Token token;
	string_ptr<STRING> strToken;
	status = getNextToken(&token, &strToken);
	CHECK_QSTATUS();
	
	CHAR cSeparator = '\0';
	if (token != TOKEN_NIL) {
		if (strlen(strToken.get()) != 1)
			return QSTATUS_FAIL;
		cSeparator = *strToken.get();
	}
	
	string_ptr<STRING> strMailbox;
	status = getNextToken(&token, &strMailbox, "\r");
	CHECK_QSTATUS();
	if (token == TOKEN_NIL)
		return QSTATUS_FAIL;
	if (pBuffer_->get(nIndex_) != '\r' || pBuffer_->get(nIndex_ + 1) != '\n')
		return QSTATUS_FAIL;
	nIndex_ += 2;
	
	status = newQsObject(bList, pList.get(),
		cSeparator, strMailbox.get(), ppList);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Parser::parseNamespaceResponse(ResponseNamespace** ppNamespace)
{
	assert(ppNamespace);
	
	DECLARE_QSTATUS();
	
	*ppNamespace = 0;
	
	std::auto_ptr<List> pLists[3];
	for (int n = 0; n < countof(pLists); ++n) {
		if (pBuffer_->get(nIndex_) == '(') {
			List* p = 0;
			status = parseList(&p);
			CHECK_QSTATUS();
			pLists[n].reset(p);
		}
		else {
			Token token;
			string_ptr<STRING> strToken;
			status = getNextToken(&token, &strToken);
			CHECK_QSTATUS();
			if (token != TOKEN_NIL)
				return QSTATUS_FAIL;
//			++nIndex_;
		}
	}
	
	status = newQsObject(pLists[0].get(),
		pLists[1].get(), pLists[2].get(), ppNamespace);
	CHECK_QSTATUS();
	
	nIndex_ = pBuffer_->find("\r\n", nIndex_);
	if (nIndex_ == static_cast<size_t>(-1))
		return QSTATUS_FAIL;
	nIndex_ += 2;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Parser::parseSearchResponse(ResponseSearch** ppSearch)
{
	assert(ppSearch);
	
	DECLARE_QSTATUS();
	
	*ppSearch = 0;
	
	std::auto_ptr<ResponseSearch> pSearch;
	status = newQsObject(&pSearch);
	CHECK_QSTATUS();
	
	while (true) {
		Token token;
		string_ptr<STRING> strToken;
		status = getNextToken(&token, &strToken, " \r");
		CHECK_QSTATUS();
		if (token != TOKEN_ATOM)
			return QSTATUS_FAIL;
		if (!*strToken.get())
			break;
		
		CHAR* pEnd = 0;
		long n = strtol(strToken.get(), &pEnd, 10);
		if (*pEnd)
			return QSTATUS_FAIL;
		status = pSearch->add(n);
		CHECK_QSTATUS();
	}
	
	nIndex_ += 2;
	
	*ppSearch = pSearch.release();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Parser::parseStatusResponse(ResponseStatus** ppStatus)
{
	assert(ppStatus);
	
	DECLARE_QSTATUS();
	
	*ppStatus = 0;
	
	Token token;
	string_ptr<STRING> strMailbox;
	status = getNextToken(&token, &strMailbox);
	CHECK_QSTATUS();
	if (token == TOKEN_NIL)
		return QSTATUS_FAIL;
	
	List* p = 0;
	status = parseList(&p);
	CHECK_QSTATUS();
	std::auto_ptr<List> pList(p);
	
	status = newQsObject(strMailbox.get(), pList.get(), ppStatus);
	CHECK_QSTATUS();
	
	nIndex_ = pBuffer_->find("\r\n", nIndex_);
	if (nIndex_ == static_cast<size_t>(-1))
		return QSTATUS_FAIL;
	nIndex_ += 2;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Parser::parseStatus(State** ppState)
{
	assert(ppState);
	
	DECLARE_QSTATUS();
	
	CHAR c = pBuffer_->get(nIndex_);
	if (c == '\0')
		return QSTATUS_FAIL;
	
	std::auto_ptr<State> pState;
	if (c == '[') {
		++nIndex_;
		
		Token token;
		string_ptr<STRING> strToken;
		status = getNextToken(&token, &strToken, " ]");
		CHECK_QSTATUS();
		if (token != TOKEN_ATOM)
			return QSTATUS_FAIL;
		
		struct S {
			enum Arg {
				ARG_NONE,
				ARG_LIST,
				ARG_NUMBER
			};
			const CHAR* pszCode_;
			State::Code code_;
			Arg arg_;
		} states[] = {
			{ "ALERT",			State::CODE_ALERT,			S::ARG_NONE		},
			{ "NEWNAME",		State::CODE_NEWNAME,		S::ARG_NONE		},
			{ "PARSE",			State::CODE_PARSE,			S::ARG_NONE		},
			{ "PERMANENTFLAGS",	State::CODE_PERMANENTFLAGS,	S::ARG_LIST		},
			{ "READ-ONLY",		State::CODE_READONLY,		S::ARG_NONE		},
			{ "READ-WRITE",		State::CODE_READWRITE,		S::ARG_NONE		},
			{ "TRYCREATE",		State::CODE_TRYCREATE,		S::ARG_NONE		},
			{ "UIDVALIDITY",	State::CODE_UIDVALIDITY,	S::ARG_NUMBER	},
			{ "UNSEEN",			State::CODE_UNSEEN,			S::ARG_NUMBER	},
			{ "UIDNEXT",		State::CODE_UIDNEXT,		S::ARG_NUMBER	}
		};
		
		State::Code code = State::CODE_NONE;
		S::Arg arg = S::ARG_NONE;
		for (int n = 0; n < countof(states); ++n) {
			if (strcmp(strToken.get(), states[n].pszCode_) == 0) {
				code = states[n].code_;
				arg = states[n].arg_;
				break;
			}
		}
		
		status = newQsObject(code, &pState);
		CHECK_QSTATUS();
		
		if (arg == S::ARG_LIST) {
			List* pList = 0;
			status = parseList(&pList);
			CHECK_QSTATUS();
			pState->setArg(pList);
			
			if (pBuffer_->get(nIndex_) != ']')
				return QSTATUS_FAIL;
		}
		else if (arg == S::ARG_NUMBER) {
			string_ptr<STRING> strArg;
			status = getNextToken(&token, &strArg, "]");
			CHECK_QSTATUS();
			if (token != TOKEN_ATOM)
				return QSTATUS_FAIL;
			
			CHAR* pEnd = 0;
			long nArg = strtol(strArg.get(), &pEnd, 10);
			if (*pEnd)
				return QSTATUS_FAIL;
			pState->setArg(nArg);
		}
		else {
			nIndex_ = pBuffer_->find(']', nIndex_);
			if (nIndex_ == static_cast<size_t>(-1))
				return QSTATUS_FAIL;
		}
		
		// Because of BUG of Exchange, I allow CRLF.
		CHAR cNext = pBuffer_->get(nIndex_ + 1);
		if (cNext == ' ')
			nIndex_ += 2;
		else if (cNext == '\r')
			nIndex_ += 1;
		else
			return QSTATUS_FAIL;
	}
	else {
		status = newQsObject(State::CODE_NONE, &pState);
		CHECK_QSTATUS();
	}
	
	size_t nIndex = pBuffer_->find("\r\n", nIndex_);
	if (nIndex == static_cast<size_t>(-1))
		return QSTATUS_FAIL;
	
	string_ptr<STRING> strMessage;
	status = pBuffer_->substr(nIndex_, nIndex - nIndex_, &strMessage);
	CHECK_QSTATUS();
	pState->setMessage(strMessage.release());
	
	nIndex_ = nIndex + 2;
	
	*ppState = pState.release();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Parser::parseList(List** ppList)
{
	return Parser::parseList(pBuffer_, &nIndex_, pCallback_, ppList);
}

QSTATUS qmimap4::Parser::getNextToken(Token* pToken, STRING* pstrToken)
{
	return getNextToken(pToken, pstrToken, " ");
}

QSTATUS qmimap4::Parser::getNextToken(Token* pToken,
	STRING* pstrToken, const CHAR* pszSep)
{
	return getNextToken(pBuffer_, &nIndex_,
		pszSep, pCallback_, pToken, pstrToken);
}


/****************************************************************************
 *
 * ParserCallback
 *
 */

qmimap4::ParserCallback::~ParserCallback()
{
}


/****************************************************************************
 *
 * Buffer
 *
 */

qmimap4::Buffer::Buffer(const CHAR* psz, QSTATUS* pstatus) :
	str_(pstatus),
	pSocket_(0),
	nError_(Imap4::IMAP4_ERROR_SUCCESS)
{
	assert(pstatus);
	
	if (*pstatus != QSTATUS_SUCCESS)
		return;
	
	DECLARE_QSTATUS();
	
	if (psz) {
		status = str_.append(psz);
		CHECK_QSTATUS_SET(pstatus);
	}
}

qmimap4::Buffer::Buffer(const CHAR* psz, Socket* pSocket, QSTATUS* pstatus) :
	str_(pstatus),
	pSocket_(pSocket),
	nError_(Imap4::IMAP4_ERROR_SUCCESS)
{
	assert(pstatus);
	
	if (*pstatus != QSTATUS_SUCCESS)
		return;
	
	DECLARE_QSTATUS();
	
	if (psz) {
		status = str_.append(psz);
		CHECK_QSTATUS_SET(pstatus);
	}
}

qmimap4::Buffer::~Buffer()
{
}

CHAR qmimap4::Buffer::get(size_t n)
{
	return get(n, 0, 0);
}

CHAR qmimap4::Buffer::get(size_t n, Imap4Callback* pCallback, size_t nStart)
{
	DECLARE_QSTATUS();
	
	if (n >= str_.getLength()) {
		if (!pSocket_)
			return '\0';
		status = receive(n, pCallback, nStart);
		CHECK_QSTATUS_VALUE('\0');
	}
	assert(str_.getLength() >= n);
	
	return str_.get(n);
}

size_t qmimap4::Buffer::find(CHAR c, size_t n)
{
	DECLARE_QSTATUS();
	
	const CHAR* p = 0;
	while (!p) {
		p = strchr(str_.getCharArray() + n, c);
		if (!p) {
			status = receive(str_.getLength(), 0, 0);
			CHECK_QSTATUS_VALUE(static_cast<size_t>(-1));
		}
	}
	return p - str_.getCharArray();
}

size_t qmimap4::Buffer::find(const CHAR* psz, size_t n)
{
	DECLARE_QSTATUS();
	
	const CHAR* p = 0;
	while (!p) {
		p = strstr(str_.getCharArray() + n, psz);
		if (!p) {
			status = receive(str_.getLength(), 0, 0);
			CHECK_QSTATUS_VALUE(static_cast<size_t>(-1));
		}
	}
	return p - str_.getCharArray();
}

QSTATUS qmimap4::Buffer::substr(size_t nPos, size_t nLen, STRING* pstr) const
{
	assert(pstr);
	assert((nLen == static_cast<size_t>(-1) && str_.getLength() >= nPos) ||
		str_.getLength() >= nPos + nLen);
	
	const CHAR* p = str_.getCharArray() + nPos;
	if (nLen == static_cast<size_t>(-1))
		nLen = strlen(p);
	*pstr = allocString(p, nLen);
	
	return *pstr ? QSTATUS_SUCCESS : QSTATUS_OUTOFMEMORY;
}

const CHAR* qmimap4::Buffer::str() const
{
	return str_.getCharArray();
}

unsigned int qmimap4::Buffer::getError() const
{
	return nError_;
}

QSTATUS qmimap4::Buffer::receive(size_t n, Imap4Callback* pCallback, size_t nStart)
{
	DECLARE_QSTATUS();
	
	CHECK_ERROR(!pSocket_, QSTATUS_FAIL, Imap4::IMAP4_ERROR_PARSE);
	
	if (pCallback) {
		status = pCallback->setPos(0);
		CHECK_QSTATUS();
	}
	
	char buf[1024];
	do {
		int nSelect = Socket::SELECT_READ;
		status = pSocket_->select(&nSelect);
		CHECK_QSTATUS_ERROR(Imap4::IMAP4_ERROR_SELECTSOCKET | pSocket_->getLastError());
		CHECK_ERROR(nSelect == 0, QSTATUS_FAIL, Imap4::IMAP4_ERROR_TIMEOUT);
		
		int nLen = sizeof(buf);
		status = pSocket_->recv(buf, &nLen, 0);
		CHECK_QSTATUS_ERROR(Imap4::IMAP4_ERROR_RECEIVE | pSocket_->getLastError());
		CHECK_ERROR(nLen == 0, QSTATUS_FAIL, Imap4::IMAP4_ERROR_DISCONNECT);
		status = str_.append(buf, nLen);
		CHECK_QSTATUS_ERROR(Imap4::IMAP4_ERROR_OTHER);
		
		if (pCallback) {
			status = pCallback->setPos(str_.getLength() - nStart);
			CHECK_QSTATUS();
		}
	} while (str_.getLength() <= n);
	
	return QSTATUS_SUCCESS;
}
