/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

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

#define IMAP4_ERROR(e) \
	do { \
		nError_ = e; \
		return false; \
	} while (false) \

#define IMAP4_ERROR_SOCKET(e) \
	do { \
		nError_ = e | pSocket_->getLastError(); \
		return false; \
	} while (false)

#define IMAP4_ERROR_OR(e) \
	do { \
		nError_ |= e; \
		return false; \
	} while (false) \

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
		string_ptr strToken;
		Token token = getNextToken(&strToken);
		if (token != TOKEN_ATOM)
			return false;
		
		if (strcmp(strToken.get(), "*") == 0) {
			std::auto_ptr<Response> pResponse(parseResponse());
			if (!pResponse.get())
				return false;
			if (!pCallback->response(pResponse))
				return false;
			if (!*pszTag)
				break;
		}
		else if (strcmp(strToken.get(), "+") == 0) {
			std::auto_ptr<ResponseContinue> pContinue(parseContinueResponse());
			if (!pContinue.get())
				return false;
			if (!pCallback->response(pContinue))
				return false;
			if (bAcceptContinue)
				break;
		}
		else {
			bool bEnd = strcmp(strToken.get(), pszTag) == 0;
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
											string_ptr* pstrToken)
{
	assert(pBuffer);
	assert(pnIndex);
	assert(pszSep);
	assert(pstrToken);
	
	pstrToken->reset(0);
	size_t& nIndex = *pnIndex;
	Token token = TOKEN_ERROR;
	
	string_ptr strToken;
	
	CHAR c = pBuffer->get(nIndex);
	if (c == '\0') {
		return TOKEN_ERROR;
	}
	else if (c == '\"') {
		StringBuffer<STRING> buf;
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
					buf.append(cNext);
					++nIndex;
				}
				else {
					buf.append(c);
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
				buf.append(c);
			}
		}
		++nIndex;
		token = TOKEN_QUOTED;
		strToken = buf.getString();
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
		
		strToken = allocString(pBuffer->str() + nIndex, nLen);
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
			strToken = allocString(pBuffer->str() + nIndexBegin, nIndex - nIndexBegin);
			token = TOKEN_ATOM;
		}
	}
	
	if (pBuffer->get(nIndex) == ' ')
		++nIndex;
	if (pBuffer->getError() != Imap4::IMAP4_ERROR_SUCCESS)
		return TOKEN_ERROR;
	
	if (token != TOKEN_NIL)
		*pstrToken = strToken;
	
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
			pList->add(pChildList);
		}
		else {
			// Because of BUG of iMAIL,
			// I add '(' to the separator.
			string_ptr strToken;
			Token token = getNextToken(pBuffer, pnIndex, " ()", pCallback, &strToken);
			if (token == TOKEN_ERROR)
				return std::auto_ptr<List>(0);
			
			std::auto_ptr<ListItem> pItem;
			if (token == TOKEN_NIL)
				pItem.reset(new ListItemNil());
			else
				pItem.reset(new ListItemText(strToken));
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
	
	string_ptr strToken;
	Token token = getNextToken(" \r", &strToken);
	if (token != TOKEN_ATOM)
		return std::auto_ptr<Response>(0);
	
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
		pResponse = parseCapabilityResponse();
	}
	else if (strcmp(strToken.get(), "LIST") == 0) {
		pResponse = parseListResponse(true);
	}
	else if (strcmp(strToken.get(), "LSUB") == 0) {
		pResponse = parseListResponse(false);
	}
	else if (strcmp(strToken.get(), "STATUS") == 0) {
		pResponse = parseStatusResponse();
	}
	else if (strcmp(strToken.get(), "SEARCH") == 0) {
		pResponse = parseSearchResponse();
	}
	else if (strcmp(strToken.get(), "FLAGS") == 0) {
		pResponse = parseFlagsResponse();
	}
	else if (strcmp(strToken.get(), "NAMESPACE") == 0) {
		pResponse = parseNamespaceResponse();
	}
	else {
		CHAR* pEnd = 0;
		long nNumber = strtol(strToken.get(), &pEnd, 10);
		if (*pEnd)
			return std::auto_ptr<Response>(0);
		
		strToken.reset(0);
		token = getNextToken(" \r", &strToken);
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
		
		if (strcmp(strToken.get(), "EXISTS") == 0)
			pResponse.reset(new ResponseExists(nNumber));
		else if (strcmp(strToken.get(), "RECENT") == 0)
			pResponse.reset(new ResponseRecent(nNumber));
		else if (strcmp(strToken.get(), "EXPUNGE") == 0)
			pResponse.reset(new ResponseExpunge(nNumber));
		else if (strcmp(strToken.get(), "FETCH") == 0)
			pResponse = parseFetchResponse(nNumber);
		else
			return std::auto_ptr<Response>(0);
	}
	
	if (flag != ResponseState::FLAG_UNKNOWN) {
		std::auto_ptr<ResponseState> pResponseState(new ResponseState(flag));
		std::auto_ptr<State> pState(parseStatus());
		pResponseState->setState(pState);
		pResponse = pResponseState;
	}
	
	return pResponse;
}

std::auto_ptr<ResponseCapability> qmimap4::Parser::parseCapabilityResponse()
{
	std::auto_ptr<ResponseCapability> pCapability(new ResponseCapability());
	
	while (true) {
		string_ptr strToken;
		Token token = getNextToken(" \r", &strToken);
		if (token != TOKEN_ATOM)
			return std::auto_ptr<ResponseCapability>(0);
		
		pCapability->add(strToken.get());
		
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
	
	nIndex_ = pBuffer_->find("\r\n", nIndex_);
	if (nIndex_ == -1)
		return std::auto_ptr<ResponseFetch>(0);
	nIndex_ += 2;
	
	return ResponseFetch::create(nNumber, pList.get());
}

std::auto_ptr<ResponseFlags> qmimap4::Parser::parseFlagsResponse()
{
	std::auto_ptr<List> pList(parseList());
	
	nIndex_ = pBuffer_->find("\r\n", nIndex_);
	if (nIndex_ == -1)
		return std::auto_ptr<ResponseFlags>(0);
	nIndex_ += 2;
	
	return ResponseFlags::create(pList.get());
}

std::auto_ptr<ResponseList> qmimap4::Parser::parseListResponse(bool bList)
{
	std::auto_ptr<List> pList(parseList());
	
	string_ptr strToken;
	Token token = getNextToken(&strToken);
	if (token == TOKEN_ERROR)
		return std::auto_ptr<ResponseList>(0);
	
	CHAR cSeparator = '\0';
	if (token != TOKEN_NIL) {
		if (strlen(strToken.get()) != 1)
			return std::auto_ptr<ResponseList>(0);
		cSeparator = *strToken.get();
	}
	
	string_ptr strMailbox;
	token = getNextToken("\r", &strMailbox);
	if (token == TOKEN_ERROR || token == TOKEN_NIL)
		return std::auto_ptr<ResponseList>(0);
	if (pBuffer_->get(nIndex_) != '\r' || pBuffer_->get(nIndex_ + 1) != '\n')
		return std::auto_ptr<ResponseList>(0);
	nIndex_ += 2;
	
	return ResponseList::create(bList, pList.get(), cSeparator, strMailbox.get());
}

std::auto_ptr<ResponseNamespace> qmimap4::Parser::parseNamespaceResponse()
{
	std::auto_ptr<List> pLists[3];
	for (int n = 0; n < countof(pLists); ++n) {
		if (pBuffer_->get(nIndex_) == '(') {
			pLists[n] = parseList();
		}
		else {
			string_ptr strToken;
			Token token = getNextToken(&strToken);
			if (token != TOKEN_NIL)
				return std::auto_ptr<ResponseNamespace>(0);
//			++nIndex_;
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
		string_ptr strToken;
		Token token = getNextToken(" \r", &strToken);
		if (token != TOKEN_ATOM)
			return std::auto_ptr<ResponseSearch>(0);
		if (!*strToken.get())
			break;
		
		CHAR* pEnd = 0;
		long n = strtol(strToken.get(), &pEnd, 10);
		if (*pEnd)
			return std::auto_ptr<ResponseSearch>(0);
		pSearch->add(n);
	}
	
	nIndex_ += 2;
	
	return pSearch;
}

std::auto_ptr<ResponseStatus> qmimap4::Parser::parseStatusResponse()
{
	string_ptr strMailbox;
	Token token = getNextToken(&strMailbox);
	if (token == TOKEN_ERROR || token == TOKEN_NIL)
		return std::auto_ptr<ResponseStatus>(0);
	
	std::auto_ptr<List> pList(parseList());
	
	nIndex_ = pBuffer_->find("\r\n", nIndex_);
	if (nIndex_ == -1)
		return std::auto_ptr<ResponseStatus>(0);
	nIndex_ += 2;
	
	return ResponseStatus::create(strMailbox.get(), pList.get());
}

std::auto_ptr<State> qmimap4::Parser::parseStatus()
{
	CHAR c = pBuffer_->get(nIndex_);
	if (c == '\0')
		return std::auto_ptr<State>(0);
	
	std::auto_ptr<State> pState;
	if (c == '[') {
		++nIndex_;
		
		string_ptr strToken;
		Token token = getNextToken(" ]", &strToken);
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
			if (strcmp(strToken.get(), states[n].pszCode_) == 0) {
				code = states[n].code_;
				arg = states[n].arg_;
				break;
			}
		}
		
		pState.reset(new State(code));
		
		if (arg == S::ARG_LIST) {
			std::auto_ptr<List> pList(parseList());
			pState->setArg(pList);
			
			if (pBuffer_->get(nIndex_) != ']')
				return std::auto_ptr<State>(0);
		}
		else if (arg == S::ARG_NUMBER) {
			string_ptr strArg;
			token = getNextToken("]", &strArg);
			if (token != TOKEN_ATOM)
				return std::auto_ptr<State>(0);
			
			CHAR* pEnd = 0;
			long nArg = strtol(strArg.get(), &pEnd, 10);
			if (*pEnd)
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

Parser::Token qmimap4::Parser::getNextToken(string_ptr* pstrToken)
{
	return getNextToken(" ", pstrToken);
}

Parser::Token qmimap4::Parser::getNextToken(const CHAR* pszSep,
											string_ptr* pstrToken)
{
	return getNextToken(pBuffer_, &nIndex_, pszSep, pCallback_, pstrToken);
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

qmimap4::Buffer::Buffer(const CHAR* psz) :
	pSocket_(0),
	nError_(Imap4::IMAP4_ERROR_SUCCESS)
{
	if (psz) {
		if (!buf_.append(psz)) {
			// TODO
		}
	}
}

qmimap4::Buffer::Buffer(const CHAR* psz,
						SocketBase* pSocket) :
	pSocket_(pSocket),
	nError_(Imap4::IMAP4_ERROR_SUCCESS)
{
	if (psz) {
		if (!buf_.append(psz)) {
			// TODO
		}
	}
}

qmimap4::Buffer::~Buffer()
{
}

CHAR qmimap4::Buffer::get(size_t n)
{
	return get(n, 0, 0);
}

CHAR qmimap4::Buffer::get(size_t n,
						  Imap4Callback* pCallback,
						  size_t nStart)
{
	if (n >= buf_.getLength()) {
		if (!pSocket_)
			return '\0';
		if (!receive(n, pCallback, nStart))
			return '\0';
	}
	assert(buf_.getLength() >= n);
	
	return buf_.get(n);
}

size_t qmimap4::Buffer::find(CHAR c,
							 size_t n)
{
	const CHAR* p = 0;
	while (!p) {
		p = strchr(buf_.getCharArray() + n, c);
		if (!p) {
			if (!receive(buf_.getLength(), 0, 0))
				return -1;
		}
	}
	return p - buf_.getCharArray();
}

size_t qmimap4::Buffer::find(const CHAR* psz,
							 size_t n)
{
	const CHAR* p = 0;
	while (!p) {
		p = strstr(buf_.getCharArray() + n, psz);
		if (!p) {
			if (!receive(buf_.getLength(), 0, 0))
				return -1;
		}
	}
	return p - buf_.getCharArray();
}

string_ptr qmimap4::Buffer::substr(size_t nPos,
								   size_t nLen) const
{
	assert((nLen == -1 && buf_.getLength() >= nPos) ||
		buf_.getLength() >= nPos + nLen);
	
	const CHAR* p = buf_.getCharArray() + nPos;
	if (nLen == -1)
		nLen = strlen(p);
	return allocString(p, nLen);
}

const CHAR* qmimap4::Buffer::str() const
{
	return buf_.getCharArray();
}

unsigned int qmimap4::Buffer::getError() const
{
	return nError_;
}

bool qmimap4::Buffer::receive(size_t n,
							  Imap4Callback* pCallback,
							  size_t nStart)
{
	if (!pSocket_)
		IMAP4_ERROR(Imap4::IMAP4_ERROR_PARSE);
	
	if (pCallback)
		pCallback->setPos(0);
	
	size_t nAllocSize = n - buf_.getLength() + Imap4::RECEIVE_BLOCK_SIZE;
	XStringBufferLock<XSTRING> lock(&buf_, nAllocSize);
	CHAR* pLock = lock.get();
	if (!pLock)
		return false;
	
	CHAR* p = pLock;
	do {
		int nSelect = pSocket_->select(Socket::SELECT_READ);
		if (nSelect == -1)
			IMAP4_ERROR_SOCKET(Imap4::IMAP4_ERROR_SELECTSOCKET);
		else if (nSelect == 0)
			IMAP4_ERROR(Imap4::IMAP4_ERROR_TIMEOUT);
		
		size_t nLen = pSocket_->recv(p, Imap4::RECEIVE_BLOCK_SIZE, 0);
		if (nLen == -1)
			IMAP4_ERROR_SOCKET(Imap4::IMAP4_ERROR_RECEIVE);
		else if (nLen == 0)
			IMAP4_ERROR(Imap4::IMAP4_ERROR_DISCONNECT);
		
		p += nLen;
		
		if (pCallback)
			pCallback->setPos((buf_.getLength() - nStart) + (p - pLock));
	} while (buf_.getLength() + (p - pLock) <= n);
	
	lock.unlock(p - pLock);
	
	return true;
}
