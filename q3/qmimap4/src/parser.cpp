/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#pragma warning(disable:4786)

#include "error.h"
#include "imap4.h"
#include "parser.h"

using namespace qmimap4;
using namespace qs;


/****************************************************************************
 *
 * Parser
 *
 */

qmimap4::Parser::Parser(Buffer* pBuffer,
						Imap4Callback* pCallback) :
	pBuffer_(pBuffer),
	nIndex_(0),
	pCallback_(pCallback)
{
}

qmimap4::Parser::~Parser()
{
}

bool qmimap4::Parser::parse(const CHAR* pszTag,
							bool bAcceptContinue,
							ParserCallback* pCallback)
{
	assert(pszTag);
	
	while (true) {
		struct X
		{
			X(Buffer* pBuffer,
			  size_t& nIndex) :
				pBuffer_(pBuffer),
				nIndex_(nIndex)
			{
			}
			
			~X()
			{
				pBuffer_->clearTokens();
				nIndex_ = pBuffer_->free(nIndex_);
			}
			
			Buffer* pBuffer_;
			size_t& nIndex_;
		} x(pBuffer_, nIndex_);
		
		std::pair<const CHAR*, size_t> tag;
		Token token = getNextToken(&tag);
		if (token != TOKEN_ATOM)
			return false;
		
		if (TokenUtil::isEqual(tag, "*")) {
			std::auto_ptr<Response> pResponse(parseResponse());
			if (!pResponse.get())
				return false;
			if (!pCallback->response(pResponse))
				return false;
			if (!*pszTag)
				break;
		}
		else if (TokenUtil::isEqual(tag, "+")) {
			std::auto_ptr<ResponseContinue> pContinue(parseContinueResponse());
			if (!pContinue.get())
				return false;
			if (!pCallback->response(std::auto_ptr<Response>(pContinue)))
				return false;
			if (bAcceptContinue)
				break;
		}
		else {
			bool bEnd = TokenUtil::isEqual(tag, pszTag);
			std::auto_ptr<Response> pResponse(parseResponse());
			if (!pResponse.get())
				return false;
			if (!pCallback->response(pResponse))
				return false;
			if (bEnd)
				break;
		}
	}
	
	return true;
}

string_ptr qmimap4::Parser::getProcessedString() const
{
	return pBuffer_->substr(0, nIndex_);
}

string_ptr qmimap4::Parser::getUnprocessedString() const
{
	return pBuffer_->substr(nIndex_, -1);
}

Parser::Token qmimap4::Parser::getNextToken(Buffer* pBuffer,
											size_t* pnIndex,
											const CHAR* pszSep,
											Imap4Callback* pCallback,
											TokenValue* pTokenValue,
											std::pair<const CHAR*, size_t>* pToken)
{
	assert(pBuffer);
	assert(pnIndex);
	assert(pszSep);
	assert((pTokenValue || pToken) && (!pTokenValue || !pToken));
	
	size_t& nIndex = *pnIndex;
	const CHAR* pszToken = 0;
	size_t nTokenLen = 0;
	
	Token token = TOKEN_ERROR;
	
	CHAR c = pBuffer->get(nIndex);
	if (c == '\0') {
		return TOKEN_ERROR;
	}
	else if (c == '\"') {
		size_t nIndexBegin = nIndex + 1;
		size_t nIndexEnd = nIndexBegin;
		for (++nIndex; ; ++nIndex) {
			c = pBuffer->get(nIndex);
			if (c == '\0') {
				return TOKEN_ERROR;
			}
			else if (c == '\\') {
				CHAR cNext = pBuffer->get(nIndex + 1);
				if (cNext == '\0') {
					return TOKEN_ERROR;
				}
				else if (cNext == '\\' || cNext == '\"') {
					pBuffer->set(nIndexEnd++, cNext);
					++nIndex;
				}
				else {
					pBuffer->set(nIndexEnd++, c);
				}
			}
			else if (c == '\"') {
				// Because of BUG of iMAIL
				// I skip '"' followed by any character but the separaters.
				CHAR cNext = pBuffer->get(nIndex + 1);
				if (cNext == '\0')
					return TOKEN_ERROR;
				else if (strchr(pszSep, cNext) || cNext == ' ' || cNext == '\r')
					break;
			}
			else {
				pBuffer->set(nIndexEnd++, c);
			}
		}
		++nIndex;
		token = TOKEN_QUOTED;
		pszToken = pBuffer->str() + nIndexBegin;
		nTokenLen = nIndexEnd - nIndexBegin;
	}
	else if (c == '{') {
		size_t nIndexEnd = pBuffer->find("}\r\n", nIndex + 1);
		if (nIndexEnd == -1)
			return TOKEN_ERROR;
		
		CHAR* pEnd = 0;
		long nLen = strtol(pBuffer->str() + nIndex + 1, &pEnd, 10);
		if (*pEnd != '}')
			return TOKEN_ERROR;
		nIndex = nIndexEnd + 3;
		
		if (pCallback && nLen >= 1024)
			pCallback->setRange(0, nLen);
		
		if (pBuffer->get(nIndex + nLen, pCallback, nIndex) == '\0')
			return TOKEN_ERROR;
		
		if (pCallback && nLen >= 1024) {
			pCallback->setRange(0, 0);
			pCallback->setPos(0);
		}
		
		pszToken = pBuffer->str() + nIndex;
		nTokenLen = nLen;
		nIndex += nLen;
		token = TOKEN_LITERAL;
	}
	else {
		size_t nIndexBegin = nIndex;
		for (; ; ++nIndex) {
			c = pBuffer->get(nIndex);
			if (c == '\0')
				return TOKEN_ERROR;
			if (c == '[') {
				nIndex = pBuffer->find(']', nIndex + 1);
				if (nIndex == -1)
					return TOKEN_ERROR;
			}
			else if (strchr(pszSep, c)) {
				break;
			}
		}
		
		if (nIndex - nIndexBegin == 3 &&
			strncmp(pBuffer->str() + nIndexBegin, "NIL", 3) == 0) {
			token = TOKEN_NIL;
		}
		else {
			pszToken = pBuffer->str() + nIndexBegin;
			nTokenLen = nIndex - nIndexBegin;
			token = TOKEN_ATOM;
		}
	}
	
	if (pBuffer->get(nIndex) == ' ')
		++nIndex;
	if (pBuffer->getError() != Imap4::IMAP4_ERROR_SUCCESS)
		return TOKEN_ERROR;
	
	if (pTokenValue) {
		pTokenValue->set(pBuffer, pszToken ? pBuffer->addToken(pszToken, nTokenLen) : -1);
	}
	else {
		pToken->first = pszToken;
		pToken->second = nTokenLen;
	}
	
	return token;
}

std::auto_ptr<List> qmimap4::Parser::parseList(Buffer* pBuffer,
											   size_t* pnIndex,
											   Imap4Callback* pCallback)
{
	assert(pBuffer);
	assert(pnIndex);
	assert(pBuffer->get(*pnIndex) == '(');
	
	std::auto_ptr<List> pList(new List());
	
	size_t& nIndex = *pnIndex;
	++nIndex;
	CHAR c = '\0';
	while ((c = pBuffer->get(nIndex)) != ')') {
		if (c == '\0') {
			return std::auto_ptr<List>(0);
		}
		else if (c == '(') {
			std::auto_ptr<List> pChildList(parseList(pBuffer, pnIndex, pCallback));
			if (!pChildList.get())
				return std::auto_ptr<List>(0);
			pList->add(std::auto_ptr<ListItem>(pChildList));
		}
		else {
			// Because of BUG of iMAIL,
			// I add '(' to the separator.
			TokenValue tokenValue;
			Token token = getNextToken(pBuffer, pnIndex, " ()", pCallback, &tokenValue, 0);
			if (token == TOKEN_ERROR)
				return std::auto_ptr<List>(0);
			
			std::auto_ptr<ListItem> pItem;
			if (token == TOKEN_NIL)
				pItem.reset(new ListItemNil());
			else
				pItem.reset(new ListItemText(tokenValue));
			pList->add(pItem);
			
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
		return std::auto_ptr<List>(0);
	
	return pList;
}

std::auto_ptr<Response> qmimap4::Parser::parseResponse()
{
	std::auto_ptr<Response> pResponse;
	
	
	std::pair<const CHAR*, size_t> value;
	Token token = getNextToken(" \r", &value);
	if (token != TOKEN_ATOM)
		return std::auto_ptr<Response>(0);
	
	ResponseState::Flag flag = ResponseState::FLAG_UNKNOWN;
	switch (*value.first) {
	case 'B':
		if (TokenUtil::isEqual(value, "BAD"))
			flag = ResponseState::FLAG_BAD;
		else if (TokenUtil::isEqual(value, "BYE"))
			flag = ResponseState::FLAG_BYE;
		break;
	case 'C':
		if (TokenUtil::isEqual(value, "CAPABILITY"))
			pResponse = parseCapabilityResponse();
		break;
	case 'F':
		if (TokenUtil::isEqual(value, "FLAGS"))
			pResponse = parseFlagsResponse();
		break;
	case 'L':
		if (TokenUtil::isEqual(value, "LIST"))
			pResponse = parseListResponse(true);
		else if (TokenUtil::isEqual(value, "LSUB"))
			pResponse = parseListResponse(false);
		break;
	case 'N':
		if (TokenUtil::isEqual(value, "NO"))
			flag = ResponseState::FLAG_NO;
		else if (TokenUtil::isEqual(value, "NAMESPACE"))
			pResponse = parseNamespaceResponse();
		break;
	case 'O':
		if (TokenUtil::isEqual(value, "OK"))
			flag = ResponseState::FLAG_OK;
		break;
	case 'P':
		if (TokenUtil::isEqual(value, "PREAUTH"))
			flag = ResponseState::FLAG_PREAUTH;
		break;
	case 'S':
		if (TokenUtil::isEqual(value, "STATUS"))
			pResponse = parseStatusResponse();
		else if (TokenUtil::isEqual(value, "SEARCH"))
			pResponse = parseSearchResponse();
		break;
	}
	if (flag == ResponseState::FLAG_UNKNOWN && !pResponse.get()) {
		unsigned long nNumber = 0;
		if (!TokenUtil::string2number(value, &nNumber))
			return std::auto_ptr<Response>(0);
		
		TokenValue tokenValue;
		token = getNextToken(" \r", &tokenValue);
		if (token != TOKEN_ATOM)
			return std::auto_ptr<Response>(0);
		
		CHAR c = pBuffer_->get(nIndex_);
		if (c == '\0') {
			return std::auto_ptr<Response>(0);
		}
		else if (c == '\r') {
			if (pBuffer_->get(nIndex_ + 1) != '\n')
				return std::auto_ptr<Response>(0);
			else
				nIndex_ += 2;
		}
		
		value = tokenValue.get();
		switch (*value.first) {
		case 'E':
			if (TokenUtil::isEqual(value, "EXISTS"))
				pResponse.reset(new ResponseExists(nNumber));
			else if (TokenUtil::isEqual(value, "EXPUNGE"))
				pResponse.reset(new ResponseExpunge(nNumber));
			break;
		case L'F':
			if (TokenUtil::isEqual(value, "FETCH"))
				pResponse = parseFetchResponse(nNumber);
			break;
		case L'R':
			if (TokenUtil::isEqual(value, "RECENT"))
				pResponse.reset(new ResponseRecent(nNumber));
			break;
		}
		if (!pResponse.get())
			return std::auto_ptr<Response>(0);
	}
	
	if (flag != ResponseState::FLAG_UNKNOWN) {
		std::auto_ptr<ResponseState> pResponseState(new ResponseState(flag));
		std::auto_ptr<State> pState(parseStatus());
		if (!pState.get())
			return std::auto_ptr<Response>(0);
		pResponseState->setState(pState);
		pResponse = pResponseState;
	}
	
	return pResponse;
}

std::auto_ptr<ResponseCapability> qmimap4::Parser::parseCapabilityResponse()
{
	std::auto_ptr<ResponseCapability> pCapability(new ResponseCapability());
	
	while (true) {
		std::pair<const CHAR*, size_t> value;
		Token token = getNextToken(" \r", &value);
		if (token != TOKEN_ATOM)
			return std::auto_ptr<ResponseCapability>(0);
		
		pCapability->add(value.first, value.second);
		
		CHAR c1 = pBuffer_->get(nIndex_);
		if (c1 == '\0')
			return std::auto_ptr<ResponseCapability>(0);
		CHAR c2 = pBuffer_->get(nIndex_ + 1);
		if (c2 == '\0')
			return std::auto_ptr<ResponseCapability>(0);
		if (c1 == '\r' && c2 == '\n') {
			nIndex_ += 2;
			break;
		}
	}
	
	return pCapability;
}

std::auto_ptr<ResponseContinue> qmimap4::Parser::parseContinueResponse()
{
	std::auto_ptr<State> pState;
	
	CHAR c = pBuffer_->get(nIndex_);
	if (c == '\0') {
		return std::auto_ptr<ResponseContinue>(0);
	}
	else if (pBuffer_->get(nIndex_ - 1) == ' ') {
		pState = parseStatus();
		if (!pState.get())
			return std::auto_ptr<ResponseContinue>(0);
	}
	else {
		nIndex_ = pBuffer_->find("\r\n", nIndex_);
		if (nIndex_ == static_cast<size_t>(-1))
			return std::auto_ptr<ResponseContinue>(0);
		nIndex_ += 2;
	}
	
	return std::auto_ptr<ResponseContinue>(new ResponseContinue(pState));
}

std::auto_ptr<ResponseFetch> qmimap4::Parser::parseFetchResponse(unsigned long nNumber)
{
	std::auto_ptr<List> pList(parseList());
	if (!pList.get())
		return std::auto_ptr<ResponseFetch>(0);
	
	nIndex_ = pBuffer_->find("\r\n", nIndex_);
	if (nIndex_ == -1)
		return std::auto_ptr<ResponseFetch>(0);
	nIndex_ += 2;
	
	return ResponseFetch::create(nNumber, pList.get());
}

std::auto_ptr<ResponseFlags> qmimap4::Parser::parseFlagsResponse()
{
	std::auto_ptr<List> pList(parseList());
	if (!pList.get())
		return std::auto_ptr<ResponseFlags>(0);
	
	nIndex_ = pBuffer_->find("\r\n", nIndex_);
	if (nIndex_ == -1)
		return std::auto_ptr<ResponseFlags>(0);
	nIndex_ += 2;
	
	return ResponseFlags::create(pList.get());
}

std::auto_ptr<ResponseList> qmimap4::Parser::parseListResponse(bool bList)
{
	std::auto_ptr<List> pList(parseList());
	if (!pList.get())
		return std::auto_ptr<ResponseList>(0);
	
	std::pair<const CHAR*, size_t> separator;
	Token token = getNextToken(&separator);
	if (token == TOKEN_ERROR)
		return std::auto_ptr<ResponseList>(0);
	
	CHAR cSeparator = '\0';
	if (token != TOKEN_NIL) {
		if (separator.second != 1)
			return std::auto_ptr<ResponseList>(0);
		cSeparator = *separator.first;
	}
	
	TokenValue tokenValue;
	token = getNextToken("\r", &tokenValue);
	if (token == TOKEN_ERROR || token == TOKEN_NIL)
		return std::auto_ptr<ResponseList>(0);
	if (pBuffer_->get(nIndex_) != '\r' || pBuffer_->get(nIndex_ + 1) != '\n')
		return std::auto_ptr<ResponseList>(0);
	nIndex_ += 2;
	
	std::pair<const CHAR*, size_t> mailbox(tokenValue.get());
	return ResponseList::create(bList, pList.get(), cSeparator, mailbox.first, mailbox.second);
}

std::auto_ptr<ResponseNamespace> qmimap4::Parser::parseNamespaceResponse()
{
	std::auto_ptr<List> pLists[3];
	for (int n = 0; n < countof(pLists); ++n) {
		if (pBuffer_->get(nIndex_) == '(') {
			pLists[n] = parseList();
			if (!pLists[n].get())
				return std::auto_ptr<ResponseNamespace>(0);
		}
		else {
			const CHAR* pszSep = n != countof(pLists) - 1 ? " " : " \r";
			std::pair<const CHAR*, size_t> nil;
			Token token = getNextToken(pszSep, &nil);
			if (token != TOKEN_NIL)
				return std::auto_ptr<ResponseNamespace>(0);
		}
	}
	
	nIndex_ = pBuffer_->find("\r\n", nIndex_);
	if (nIndex_ == -1)
		return std::auto_ptr<ResponseNamespace>(0);
	nIndex_ += 2;
	
	return ResponseNamespace::create(pLists[0].get(), pLists[1].get(), pLists[2].get());
}

std::auto_ptr<ResponseSearch> qmimap4::Parser::parseSearchResponse()
{
	std::auto_ptr<ResponseSearch> pSearch(new ResponseSearch());
	
	while (true) {
		std::pair<const CHAR*, size_t> id;
		Token token = getNextToken(" \r", &id);
		if (token != TOKEN_ATOM)
			return std::auto_ptr<ResponseSearch>(0);
		if (id.second == 0)
			break;
		
		unsigned long nId = 0;
		if (!TokenUtil::string2number(id, &nId))
			return std::auto_ptr<ResponseSearch>(0);
		pSearch->add(nId);
	}
	
	nIndex_ += 2;
	
	return pSearch;
}

std::auto_ptr<ResponseStatus> qmimap4::Parser::parseStatusResponse()
{
	TokenValue tokenValue;
	Token token = getNextToken(&tokenValue);
	if (token == TOKEN_ERROR || token == TOKEN_NIL)
		return std::auto_ptr<ResponseStatus>(0);
	
	std::auto_ptr<List> pList(parseList());
	if (!pList.get())
		return std::auto_ptr<ResponseStatus>(0);
	
	nIndex_ = pBuffer_->find("\r\n", nIndex_);
	if (nIndex_ == -1)
		return std::auto_ptr<ResponseStatus>(0);
	nIndex_ += 2;
	
	std::pair<const CHAR*, size_t> mailbox(tokenValue.get());
	return ResponseStatus::create(mailbox.first, mailbox.second, pList.get());
}

std::auto_ptr<State> qmimap4::Parser::parseStatus()
{
	CHAR c = pBuffer_->get(nIndex_);
	if (c == '\0')
		return std::auto_ptr<State>(0);
	
	std::auto_ptr<State> pState;
	if (c == '[') {
		++nIndex_;
		
		std::pair<const CHAR*, size_t> codeName;
		Token token = getNextToken(" ]", &codeName);
		if (token != TOKEN_ATOM)
			return std::auto_ptr<State>(0);
		
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
		
		State::Code code = State::CODE_OTHER;
		S::Arg arg = S::ARG_NONE;
		for (int n = 0; n < countof(states); ++n) {
			if (TokenUtil::isEqual(codeName, states[n].pszCode_)) {
				code = states[n].code_;
				arg = states[n].arg_;
				break;
			}
		}
		
		pState.reset(new State(code));
		
		if (arg == S::ARG_LIST) {
			std::auto_ptr<List> pList(parseList());
			if (!pList.get())
				return std::auto_ptr<State>(0);
			pState->setArg(pList);
			
			if (pBuffer_->get(nIndex_) != ']')
				return std::auto_ptr<State>(0);
		}
		else if (arg == S::ARG_NUMBER) {
			std::pair<const CHAR*, size_t> arg;
			token = getNextToken("]", &arg);
			if (token != TOKEN_ATOM)
				return std::auto_ptr<State>(0);
			
			unsigned long nArg = 0;
			if (!TokenUtil::string2number(arg, &nArg))
				return std::auto_ptr<State>(0);
			pState->setArg(nArg);
		}
		else {
			nIndex_ = pBuffer_->find(']', nIndex_);
			if (nIndex_ == -1)
				return std::auto_ptr<State>(0);
		}
		
		// Because of BUG of Exchange, I allow CRLF.
		CHAR cNext = pBuffer_->get(nIndex_ + 1);
		if (cNext == ' ')
			nIndex_ += 2;
		else if (cNext == '\r')
			nIndex_ += 1;
		else
			return std::auto_ptr<State>(0);
	}
	else {
		pState.reset(new State(State::CODE_NONE));
	}
	
	size_t nIndex = pBuffer_->find("\r\n", nIndex_);
	if (nIndex == -1)
		return std::auto_ptr<State>(0);
	
	string_ptr strMessage(pBuffer_->substr(nIndex_, nIndex - nIndex_));
	pState->setMessage(strMessage);
	
	nIndex_ = nIndex + 2;
	
	return pState;
}

std::auto_ptr<List> qmimap4::Parser::parseList()
{
	return Parser::parseList(pBuffer_, &nIndex_, pCallback_);
}

Parser::Token qmimap4::Parser::getNextToken(std::pair<const CHAR*, size_t>* pToken)
{
	return getNextToken(" ", pToken);
}

Parser::Token qmimap4::Parser::getNextToken(const CHAR* pszSep,
											std::pair<const CHAR*, size_t>* pToken)
{
	return getNextToken(pBuffer_, &nIndex_, pszSep, pCallback_, 0, pToken);
}

Parser::Token qmimap4::Parser::getNextToken(TokenValue* pTokenValue)
{
	return getNextToken(" ", pTokenValue);
}

Parser::Token qmimap4::Parser::getNextToken(const CHAR* pszSep,
											TokenValue* pTokenValue)
{
	return getNextToken(pBuffer_, &nIndex_, pszSep, pCallback_, pTokenValue, 0);
}


/****************************************************************************
 *
 * ParserCallback
 *
 */

qmimap4::ParserCallback::~ParserCallback()
{
}
