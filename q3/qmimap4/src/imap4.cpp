/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsconv.h>
#include <qsencoder.h>
#include <qserror.h>
#include <qsmd5.h>
#include <qsnew.h>
#include <qsstl.h>

#include <algorithm>
#include <cstdio>

#include "imap4.h"
#include "parser.h"

#pragma warning(disable:4786)

using namespace qmimap4;
using namespace qs;


namespace qmimap4 {
class ListParserCallback;
class Imap4ParserCallback;
}


/****************************************************************************
 *
 * ListParserCallback
 *
 */

class qmimap4::ListParserCallback : public ParserCallback
{
public:
	typedef std::vector<Response*> ResponseList;

public:
	ListParserCallback();
	virtual ~ListParserCallback();

public:
	const ResponseList& getResponseList() const;
	ResponseState::Flag getResponse() const;
	void clear();

public:
	virtual QSTATUS response(Response* pResponse);

private:
	ListParserCallback(const ListParserCallback&);
	ListParserCallback& operator=(const ListParserCallback&);

private:
	ResponseList listResponse_;
};

qmimap4::ListParserCallback::ListParserCallback()
{
}

qmimap4::ListParserCallback::~ListParserCallback()
{
	clear();
}

const ListParserCallback::ResponseList& qmimap4::ListParserCallback::getResponseList() const
{
	return listResponse_;
}

ResponseState::Flag qmimap4::ListParserCallback::getResponse() const
{
	if (listResponse_.empty() ||
		listResponse_.back()->getType() != Response::TYPE_STATE)
		return ResponseState::FLAG_UNKNOWN;
	else
		return static_cast<ResponseState*>(listResponse_.back())->getFlag();
}

QSTATUS qmimap4::ListParserCallback::response(Response* pResponse)
{
	return STLWrapper<ResponseList>(listResponse_).push_back(pResponse);
}

void qmimap4::ListParserCallback::clear()
{
	std::for_each(listResponse_.begin(), listResponse_.end(), deleter<Response>());
	listResponse_.clear();
}


/****************************************************************************
 *
 * Imap4ParserCallback
 *
 */

class qmimap4::Imap4ParserCallback : public ParserCallback
{
public:
	Imap4ParserCallback(Imap4Callback* pCallback);
	virtual ~Imap4ParserCallback();

public:
	ResponseState::Flag getResponse() const;

public:
	virtual QSTATUS response(Response* pResponse);

private:
	Imap4ParserCallback(const Imap4ParserCallback&);
	Imap4ParserCallback& operator=(const Imap4ParserCallback&);

private:
	Imap4Callback* pCallback_;
	Response* pLastResponse_;
};

qmimap4::Imap4ParserCallback::Imap4ParserCallback(Imap4Callback* pCallback) :
	pCallback_(pCallback),
	pLastResponse_(0)
{
}

qmimap4::Imap4ParserCallback::~Imap4ParserCallback()
{
	delete pLastResponse_;
}

ResponseState::Flag qmimap4::Imap4ParserCallback::getResponse() const
{
	if (!pLastResponse_ || pLastResponse_->getType() != Response::TYPE_STATE)
		return ResponseState::FLAG_UNKNOWN;
	else
		return static_cast<ResponseState*>(pLastResponse_)->getFlag();
}

QSTATUS qmimap4::Imap4ParserCallback::response(Response* pResponse)
{
	assert(pResponse);
	
	DECLARE_QSTATUS();
	
	delete pLastResponse_;
	pLastResponse_ = pResponse;
	
	status = pCallback_->response(pResponse);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * Imap4
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

qmimap4::Imap4::Imap4(const Option& option, qs::QSTATUS* pstatus) :
	nTimeout_(option.nTimeout_),
	pSocketCallback_(option.pSocketCallback_),
	pSSLSocketCallback_(option.pSSLSocketCallback_),
	pImap4Callback_(option.pImap4Callback_),
	pLogger_(option.pLogger_),
	pSocket_(0),
	strOverBuf_(0),
	nCapability_(0),
	nAuth_(AUTH_LOGIN),
	bDisconnected_(false),
	nTag_(0),
	nError_(IMAP4_ERROR_SUCCESS)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
	
	DECLARE_QSTATUS();
	
	status = newQsObject(true, &pUTF7Converter_);
	CHECK_QSTATUS_SET(pstatus);
}

qmimap4::Imap4::~Imap4()
{
	delete pSocket_;
	freeString(strOverBuf_);
	delete pUTF7Converter_;
}

QSTATUS qmimap4::Imap4::connect(const WCHAR* pwszHost, short nPort, Ssl ssl)
{
	assert(pwszHost);
	
	DECLARE_QSTATUS();
	
	Socket::Option option = {
		nTimeout_,
		pSocketCallback_,
		pLogger_
	};
	std::auto_ptr<Socket> pSocket;
	status = newQsObject(option, &pSocket);
	CHECK_QSTATUS_ERROR(IMAP4_ERROR_INITIALIZE);
	
	status = pSocket->connect(pwszHost, nPort);
	CHECK_QSTATUS_ERROR(IMAP4_ERROR_CONNECT | pSocket->getLastError());
	
	if (ssl == SSL_SSL) {
		SSLSocketFactory* pFactory = SSLSocketFactory::getFactory();
		CHECK_ERROR(!pFactory, QSTATUS_FAIL, IMAP4_ERROR_SSL);
		SSLSocket* pSSLSocket = 0;
		status = pFactory->createSSLSocket(pSocket.get(),
			true, pSSLSocketCallback_, pLogger_, &pSSLSocket);
		CHECK_QSTATUS_ERROR(IMAP4_ERROR_SSL);
		pSocket.release();
		pSocket_ = pSSLSocket;
	}
	else {
		pSocket_ = pSocket.release();
	}
	
	status = processGreeting();
	CHECK_QSTATUS();
	
	status = processCapability();
	CHECK_QSTATUS();
	
	if (ssl == SSL_STARTTLS) {
		CHECK_ERROR(!(nCapability_ & CAPABILITY_STARTTLS),
			QSTATUS_FAIL, IMAP4_ERROR_STARTTLS);
		ListParserCallback callback;
		status = sendCommand("STARTTLS\r\n", &callback);
		CHECK_QSTATUS_ERROR_OR(IMAP4_ERROR_STARTTLS);
		const ListParserCallback::ResponseList& l = callback.getResponseList();
		CHECK_ERROR(l.size() != 1, QSTATUS_FAIL,
			IMAP4_ERROR_STARTTLS | IMAP4_ERROR_PARSE);
		ResponseState::Flag flag = callback.getResponse();
		CHECK_ERROR(flag != ResponseState::FLAG_OK,
			QSTATUS_FAIL, IMAP4_ERROR_STARTTLS | IMAP4_ERROR_PARSE);
		
		SSLSocketFactory* pFactory = SSLSocketFactory::getFactory();
		CHECK_ERROR(!pFactory, QSTATUS_FAIL, IMAP4_ERROR_SSL);
		SSLSocket* pSSLSocket = 0;
		status = pFactory->createSSLSocket(static_cast<Socket*>(pSocket_),
			true, pSSLSocketCallback_, pLogger_, &pSSLSocket);
		CHECK_QSTATUS_ERROR(IMAP4_ERROR_SSL);
		pSocket_ = pSSLSocket;
		
		status = processCapability();
		CHECK_QSTATUS();
	}
	
	status = processLogin();
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Imap4::disconnect()
{
	DECLARE_QSTATUS();
	
	if (pSocket_) {
		const CommandToken tokens[] = {
			{ "LOGOUT\r\n",	0,	0,	false,	true	}
		};
		
		status = sendCommandTokens(tokens, countof(tokens));
		if (status != QSTATUS_SUCCESS)
			nError_ |= IMAP4_ERROR_LOGOUT;
		
		delete pSocket_;
		pSocket_ = 0;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Imap4::checkConnection()
{
	DECLARE_QSTATUS();
	
	if (!pSocket_)
		return QSTATUS_FAIL;
	
	int nSelect = Socket::SELECT_READ;
	status = pSocket_->select(&nSelect, 0);
	CHECK_QSTATUS_ERROR(IMAP4_ERROR_SELECTSOCKET | pSocket_->getLastError());
	if (nSelect == 0)
		return QSTATUS_SUCCESS;
	
	char c = 0;
	int nLen = 1;
	status = pSocket_->recv(&c, &nLen, MSG_PEEK);
	CHECK_QSTATUS();
	if (nLen == 0)
		return QSTATUS_FAIL;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Imap4::select(const WCHAR* pwszFolderName)
{
	assert(pwszFolderName);
	
	DECLARE_QSTATUS();
	
	const CommandToken tokens[] = {
		{ "SELECT ",	0,				0,					false,	true	},
		{ 0,			pwszFolderName,	pUTF7Converter_,	true,	true	},
		{ "\r\n",		0,				0,					false,	true	}
	};
	
	status = sendCommandTokens(tokens, countof(tokens));
	CHECK_QSTATUS_ERROR_OR(IMAP4_ERROR_SELECT);
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Imap4::close()
{
	DECLARE_QSTATUS();
	
	const CommandToken tokens[] = {
		{ "CLOSE\r\n",	0,	0,	false,	true	}
	};
	
	status = sendCommandTokens(tokens, countof(tokens));
	CHECK_QSTATUS_ERROR_OR(IMAP4_ERROR_CLOSE);
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Imap4::noop()
{
	DECLARE_QSTATUS();
	
	const CommandToken tokens[] = {
		{ "NOOP\r\n",	0,	0,	false,	true	}
	};
	
	status = sendCommandTokens(tokens, countof(tokens));
	CHECK_QSTATUS_ERROR_OR(IMAP4_ERROR_NOOP);
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Imap4::fetch(const Range& range, const CHAR* pszFetch)
{
	assert(pszFetch);
	
	DECLARE_QSTATUS();
	
	const CommandToken tokens[] = {
		{ "UID FETCH ",		0,	0,	false,	range.isUid()	},
		{ "FETCH ",			0,	0,	false,	!range.isUid()	},
		{ range.getRange(),	0,	0,	false,	true			},
		{ " ",				0,	0,	false,	true			},
		{ pszFetch,			0,	0,	false,	true			},
		{ "\r\n",			0,	0,	false,	true			}
	};
	
	status = sendCommandTokens(tokens, countof(tokens));
	CHECK_QSTATUS_ERROR_OR(IMAP4_ERROR_FETCH);
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Imap4::store(const Range& range, const CHAR* pszStore)
{
	assert(pszStore);
	
	DECLARE_QSTATUS();
	
	CommandToken tokens[] = {
		{ "UID STORE ",		0,	0,	false,	range.isUid()	},
		{ "STORE ",			0,	0,	false,	!range.isUid()	},
		{ range.getRange(),	0,	0,	false,	true			},
		{ " ",				0,	0,	false,	true			},
		{ pszStore,			0,	0,	false,	true			},
		{ "\r\n",			0,	0,	false,	true			}
	};
	
	status = sendCommandTokens(tokens, countof(tokens));
	CHECK_QSTATUS_ERROR_OR(IMAP4_ERROR_STORE);
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Imap4::copy(const Range& range, const WCHAR* pwszFolderName)
{
	assert(pwszFolderName);
	
	DECLARE_QSTATUS();
	
	CommandToken tokens[] = {
		{ "UID COPY ",		0,				0,					false,	range.isUid()	},
		{ "COPY ",			0,				0,					false,	!range.isUid()	},
		{ range.getRange(),	0,				0,					false,	true			},
		{ " ",				0,				0,					false,	true			},
		{ 0,				pwszFolderName, pUTF7Converter_,	true,	true			},
		{ "\r\n",			0,				0,					false,	true			}
	};
	
	status = sendCommandTokens(tokens, countof(tokens));
	CHECK_QSTATUS_ERROR_OR(IMAP4_ERROR_COPY);
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Imap4::search(const WCHAR* pwszSearch,
	const WCHAR* pwszCharset, bool bUseCharset, bool bUid)
{
	assert(pwszSearch);
	assert(pwszCharset);
	
	DECLARE_QSTATUS();
	
	string_ptr<STRING> strSearch;
	status = encodeSearchString(pwszSearch, pwszCharset, &strSearch);
	CHECK_QSTATUS_ERROR(IMAP4_ERROR_OTHER | IMAP4_ERROR_SEARCH);
	
	CommandToken tokens[] = {
		{ "UID SEARCH ",	0,				0,	false,	bUid		},
		{ "SEARCH ",		0,				0,	false,	!bUid		},
		{ "CHARSET ",		0,				0,	false,	bUseCharset	},
		{ 0,				pwszCharset,	0,	false,	bUseCharset	},
		{ " ",				0,				0,	false,	bUseCharset	},
		{ strSearch.get(),	0,				0,	false,	true		},
		{ "\r\n",			0,				0,	false,	true		}
	};
	
	status = sendCommandTokens(tokens, countof(tokens));
	CHECK_QSTATUS_ERROR_OR(IMAP4_ERROR_SEARCH);
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Imap4::expunge()
{
	DECLARE_QSTATUS();
	
	const CommandToken tokens[] = {
		{ "EXPUNGE\r\n",	0,	0,	false,	true	}
	};
	
	status = sendCommandTokens(tokens, countof(tokens));
	CHECK_QSTATUS_ERROR_OR(IMAP4_ERROR_EXPUNGE);
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Imap4::append(const WCHAR* pwszFolderName,
	const CHAR* pszMessage, const Flags& flags)
{
	assert(pwszFolderName);
	assert(pszMessage);
	
	DECLARE_QSTATUS();
	
	string_ptr<STRING> strFolderName;
	size_t nLen = wcslen(pwszFolderName);
	status = pUTF7Converter_->encode(pwszFolderName,
		&nLen, &strFolderName, 0);
	CHECK_QSTATUS_ERROR(IMAP4_ERROR_OTHER | IMAP4_ERROR_APPEND);
	string_ptr<STRING> strQuotedFolderName;
	status = getQuotedString(strFolderName.get(), &strQuotedFolderName);
	CHECK_QSTATUS_ERROR(IMAP4_ERROR_OTHER | IMAP4_ERROR_APPEND);
	
	string_ptr<STRING> strFlags;
	status = flags.getString(&strFlags);
	CHECK_QSTATUS_ERROR(IMAP4_ERROR_OTHER | IMAP4_ERROR_APPEND);
	
	const CHAR* pszSeparator = "";
	if (strlen(strFlags.get()) != 0)
		pszSeparator = " ";
	
	CHAR szLen[32];
	sprintf(szLen, "%d", strlen(pszMessage));
	
	const Concat c[] = {
		{ "APPEND ",					-1 },
		{ strQuotedFolderName.get(),	-1 },
		{ " ",							-1 },
		{ strFlags.get(),				-1 },
		{ pszSeparator,					-1 },
		{ "{",							-1 },
		{ szLen,						-1 },
		{ "}\r\n",						-1 }
	};
	string_ptr<STRING> strCommand(concat(c, countof(c)));
	CHECK_ERROR(!strCommand.get(), QSTATUS_OUTOFMEMORY,
		IMAP4_ERROR_OTHER | IMAP4_ERROR_APPEND);
	
	ListParserCallback listCallback;
	string_ptr<STRING> strTag;
	status = sendCommand(strCommand.get(), &strTag, &listCallback);
	CHECK_QSTATUS_ERROR_OR(IMAP4_ERROR_APPEND);
	const ListParserCallback::ResponseList& l = listCallback.getResponseList();
	CHECK_ERROR(l.empty() || l.back()->getType() != Response::TYPE_CONTINUE,
		QSTATUS_FAIL, IMAP4_ERROR_RESPONSE | IMAP4_ERROR_APPEND);
	ListParserCallback::ResponseList::size_type n = 0;
	while (n < l.size() - 1) {
		status = pImap4Callback_->response(l[n]);
		CHECK_QSTATUS();
		++n;
	}
	
	const CHAR* pszContents[] = {
		pszMessage,
		"\r\n"
	};
	
	Imap4ParserCallback callback(pImap4Callback_);
	status = send(pszContents, countof(pszContents), strTag.get(), false, &callback);
	CHECK_QSTATUS_ERROR_OR(IMAP4_ERROR_APPEND);
	CHECK_ERROR(callback.getResponse() != ResponseState::FLAG_OK,
		QSTATUS_FAIL, IMAP4_ERROR_RESPONSE | IMAP4_ERROR_APPEND);
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Imap4::list(bool bSubscribeOnly,
	const WCHAR* pwszRef, const WCHAR* pwszMailbox)
{
	assert(pwszRef);
	assert(pwszMailbox);
	
	DECLARE_QSTATUS();
	
	unsigned int nErrorHighLevel = bSubscribeOnly ?
		IMAP4_ERROR_LSUB : IMAP4_ERROR_LIST;
	
	CommandToken tokens[] = {
		{ "LIST ",	0,				0,					false,	!bSubscribeOnly	},
		{ "LSUB ",	0,				0,					false,	bSubscribeOnly	},
		{ 0,		pwszRef,		pUTF7Converter_,	true,	true			},
		{ " ",		0,				0,					false,	true			},
		{ 0,		pwszMailbox,	pUTF7Converter_,	true,	true			},
		{ "\r\n",	0,				0,					false,	true			}
	};
	
	status = sendCommandTokens(tokens, countof(tokens));
	CHECK_QSTATUS_ERROR_OR(nErrorHighLevel);
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Imap4::create(const WCHAR* pwszFolderName)
{
	assert(pwszFolderName);
	
	DECLARE_QSTATUS();
	
	CommandToken tokens[] = {
		{ "CREATE ",	0,				0,					false,	true	},
		{ 0,			pwszFolderName,	pUTF7Converter_,	true,	true	},
		{ "\r\n",		0,				0,					false,	true	}
	};
	
	status = sendCommandTokens(tokens, countof(tokens));
	CHECK_QSTATUS_ERROR_OR(IMAP4_ERROR_CREATE);
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Imap4::remove(const WCHAR* pwszFolderName)
{
	assert(pwszFolderName);
	
	DECLARE_QSTATUS();
	
	CommandToken tokens[] = {
		{ "DELETE ",	0,				0,					false,	true	},
		{ 0,			pwszFolderName,	pUTF7Converter_,	true,	true	},
		{ "\r\n",		0,				0,					false,	true	}
	};
	
	status = sendCommandTokens(tokens, countof(tokens));
	CHECK_QSTATUS_ERROR_OR(IMAP4_ERROR_DELETE);
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Imap4::rename(const WCHAR* pwszOldFolderName,
	const WCHAR* pwszNewFolderName)
{
	assert(pwszOldFolderName);
	assert(pwszNewFolderName);
	
	DECLARE_QSTATUS();
	
	CommandToken tokens[] = {
		{ "RENAME ",	0,				0,						false,	true	},
		{ 0,			pwszOldFolderName,	pUTF7Converter_,	true,	true	},
		{ " ",			0,				0,						false,	true	},
		{ 0,			pwszNewFolderName,	pUTF7Converter_,	true,	true	},
		{ "\r\n",		0,				0,						false,	true	}
	};
	
	status = sendCommandTokens(tokens, countof(tokens));
	CHECK_QSTATUS_ERROR_OR(IMAP4_ERROR_RENAME);
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Imap4::subscribe(const WCHAR* pwszFolderName)
{
	assert(pwszFolderName);
	
	DECLARE_QSTATUS();
	
	CommandToken tokens[] = {
		{ "SUBSCRIBE ",	0,				0,					false,	true	},
		{ 0,			pwszFolderName,	pUTF7Converter_,	true,	true	},
		{ "\r\n",		0,				0,					false,	true	}
	};
	
	status = sendCommandTokens(tokens, countof(tokens));
	CHECK_QSTATUS_ERROR_OR(IMAP4_ERROR_SUBSCRIBE);
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Imap4::unsubscribe(const WCHAR* pwszFolderName)
{
	assert(pwszFolderName);
	
	DECLARE_QSTATUS();
	
	CommandToken tokens[] = {
		{ "UNSUBSCRIBE ",	0,				0,					false,	true	},
		{ 0,				pwszFolderName,	pUTF7Converter_,	true,	true	},
		{ "\r\n",			0,				0,					false,	true	}
	};
	
	status = sendCommandTokens(tokens, countof(tokens));
	CHECK_QSTATUS_ERROR_OR(IMAP4_ERROR_UNSUBSCRIBE);
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Imap4::namespaceList()
{
	DECLARE_QSTATUS();
	
	const CommandToken tokens[] = {
		{ "NAMESPACE\r\n",	0,	0,	false,	true	}
	};
	
	status = sendCommandTokens(tokens, countof(tokens));
	CHECK_QSTATUS_ERROR_OR(IMAP4_ERROR_NAMESPACE);
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Imap4::getFlags(const Range& range)
{
	return fetch(range, "(FLAGS)");
}

QSTATUS qmimap4::Imap4::setFlags(const Range& range,
	const Flags& flags, const Flags& mask)
{
	DECLARE_QSTATUS();
	
	string_ptr<STRING> strAdded;
	status = flags.getAdded(mask, &strAdded);
	CHECK_QSTATUS_ERROR(IMAP4_ERROR_OTHER | IMAP4_ERROR_STORE);
	string_ptr<STRING> strRemoved;
	status = flags.getRemoved(mask, &strRemoved);
	CHECK_QSTATUS_ERROR(IMAP4_ERROR_OTHER | IMAP4_ERROR_STORE);
	
	if (strRemoved.get()) {
		string_ptr<STRING> strStore(concat("-FLAGS ", strRemoved.get()));
		CHECK_ERROR(!strStore.get(), QSTATUS_OUTOFMEMORY,
			IMAP4_ERROR_OTHER | IMAP4_ERROR_STORE);
		status = store(range, strStore.get());
		CHECK_QSTATUS();
	}
	
	if (strAdded.get()) {
		string_ptr<STRING> strStore(concat("+FLAGS ", strAdded.get()));
		CHECK_ERROR(!strStore.get(), QSTATUS_OUTOFMEMORY,
			IMAP4_ERROR_OTHER | IMAP4_ERROR_STORE);
		status = store(range, strStore.get());
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Imap4::getMessageData(const Range& range,
	bool bClientParse, bool bBody, const CHAR* pszFields)
{
	DECLARE_QSTATUS();
	
	StringBuffer<STRING> buf("(", &status);
	CHECK_QSTATUS_ERROR(IMAP4_ERROR_OTHER | IMAP4_ERROR_FETCH);
	
	if (!bClientParse) {
		status = buf.append("ENVELOPE ");
		CHECK_QSTATUS_ERROR(IMAP4_ERROR_OTHER | IMAP4_ERROR_FETCH);
	}
	if (bBody) {
		status = buf.append("BODYSTRUCTURE ");
		CHECK_QSTATUS_ERROR(IMAP4_ERROR_OTHER | IMAP4_ERROR_FETCH);
	}
	status = buf.append("BODY.PEEK[HEADER.FIELDS (");
	CHECK_QSTATUS_ERROR(IMAP4_ERROR_OTHER | IMAP4_ERROR_FETCH);
	if (bClientParse) {
		status = buf.append("From To Cc Bcc Sender Message-Id In-Reply-To Subject Date ");
		CHECK_QSTATUS_ERROR(IMAP4_ERROR_OTHER | IMAP4_ERROR_FETCH);
	}
	if (!bBody) {
		status = buf.append("Content-Type Content-Disposition ");
		CHECK_QSTATUS_ERROR(IMAP4_ERROR_OTHER | IMAP4_ERROR_FETCH);
	}
	status = buf.append("References X-ML-Name");
	CHECK_QSTATUS_ERROR(IMAP4_ERROR_OTHER | IMAP4_ERROR_FETCH);
	if (pszFields && *pszFields) {
		status = buf.append(" ");
		CHECK_QSTATUS_ERROR(IMAP4_ERROR_OTHER | IMAP4_ERROR_FETCH);
		status = buf.append(pszFields);
		CHECK_QSTATUS_ERROR(IMAP4_ERROR_OTHER | IMAP4_ERROR_FETCH);
	}
	status = buf.append(")] FLAGS UID RFC822.SIZE)");
	CHECK_QSTATUS_ERROR(IMAP4_ERROR_OTHER | IMAP4_ERROR_FETCH);
	
	return fetch(range, buf.getCharArray());
}

QSTATUS qmimap4::Imap4::getMessage(const Range& range, bool bPeek)
{
	if (bPeek)
		return fetch(range, "BODY.PEEK[]");
	else
		return fetch(range, "BODY[]");
}

QSTATUS qmimap4::Imap4::getHeader(const Range& range, bool bPeek)
{
	if (bPeek)
		return fetch(range, "BODY.PEEK[HEADER]");
	else
		return fetch(range, "BODY[HEADER]");
}

QSTATUS qmimap4::Imap4::getBodyStructure(const Range& range)
{
	return fetch(range, "BODYSTRUCTURE");
}

QSTATUS qmimap4::Imap4::getPart(const Range& range, const PartPath& path)
{
	const CHAR* pszPath = path.getPath();
	const Concat c[] = {
		{ "(BODY.PEEK[",	-1 },
		{ pszPath,			-1 },
		{ "] BODY.PEEK[",	-1 },
		{ pszPath,			-1 },
		{ ".MIME])"			-1 }
	};
	string_ptr<STRING> strFetch(concat(c, countof(c)));
	CHECK_ERROR(!strFetch.get(), QSTATUS_OUTOFMEMORY,
		IMAP4_ERROR_OTHER | IMAP4_ERROR_FETCH);
	
	return fetch(range, strFetch.get());
}

QSTATUS qmimap4::Imap4::getPartMime(const Range& range, const PartPath& path)
{
	const CHAR* pszPath = path.getPath();
	const Concat c[] = {
		{ "BODY.PEEK[",	-1 },
		{ pszPath,		-1 },
		{ ".MIME]"		-1 }
	};
	string_ptr<STRING> strFetch(concat(c, countof(c)));
	CHECK_ERROR(!strFetch.get(), QSTATUS_OUTOFMEMORY,
		IMAP4_ERROR_OTHER | IMAP4_ERROR_FETCH);
	
	return fetch(range, strFetch.get());
}

QSTATUS qmimap4::Imap4::getPartBody(const Range& range, const PartPath& path)
{
	const CHAR* pszPath = path.getPath();
	const Concat c[] = {
		{ "BODY.PEEK[",	-1 },
		{ pszPath,		-1 },
		{ "]",			-1 }
	};
	string_ptr<STRING> strFetch(concat(c, countof(c)));
	CHECK_ERROR(!strFetch.get(), QSTATUS_OUTOFMEMORY,
		IMAP4_ERROR_OTHER | IMAP4_ERROR_FETCH);
	
	return fetch(range, strFetch.get());
}

unsigned int qmimap4::Imap4::getCapability() const
{
	return nCapability_;
}

unsigned int qmimap4::Imap4::getLastError() const
{
	return nError_;
}

const WCHAR* qmimap4::Imap4::getLastErrorResponse() const
{
	// TODO
	return L"";
}

QSTATUS qmimap4::Imap4::processGreeting()
{
	DECLARE_QSTATUS();
	
	ListParserCallback callback;
	status = receive("", false, &callback);
	CHECK_QSTATUS_ERROR_OR(IMAP4_ERROR_GREETING);
	
	const ListParserCallback::ResponseList& l = callback.getResponseList();
	CHECK_ERROR(l.size() != 1, QSTATUS_FAIL,
		IMAP4_ERROR_GREETING | IMAP4_ERROR_PARSE);
	ResponseState::Flag flag = callback.getResponse();
	CHECK_ERROR(flag != ResponseState::FLAG_OK &&
		flag != ResponseState::FLAG_PREAUTH,
		QSTATUS_FAIL, IMAP4_ERROR_GREETING | IMAP4_ERROR_PARSE);
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Imap4::processCapability()
{
	DECLARE_QSTATUS();
	
	nCapability_ = 0;
	
	ListParserCallback callback;
	status = sendCommand("CAPABILITY\r\n", &callback);
	CHECK_QSTATUS_ERROR_OR(IMAP4_ERROR_CAPABILITY);
	CHECK_ERROR(callback.getResponse() != ResponseState::FLAG_OK,
		QSTATUS_FAIL, IMAP4_ERROR_CAPABILITY | IMAP4_ERROR_PARSE);
	
	const ListParserCallback::ResponseList& l = callback.getResponseList();
	ListParserCallback::ResponseList::const_iterator it = std::find_if(
		l.begin(), l.end(),
		std::bind2nd(binary_compose_f_gx_hy(std::equal_to<Response::Type>(),
			std::mem_fun(&Response::getType), std::identity<Response::Type>()),
			Response::TYPE_CAPABILITY));
	CHECK_ERROR(it == l.end(), QSTATUS_FAIL,
		IMAP4_ERROR_CAPABILITY | IMAP4_ERROR_PARSE);
	
	ResponseCapability* pCapability = static_cast<ResponseCapability*>(*it);
	CHECK_ERROR(!pCapability->isSupport("IMAP4REV1"), QSTATUS_FAIL,
		IMAP4_ERROR_CAPABILITY | IMAP4_ERROR_PARSE);
	if (pCapability->isSupport("NAMESPACE"))
		nCapability_ |= CAPABILITY_NAMESPACE;
	if (pCapability->isSupport("STARTTLS"))
		nCapability_ |= CAPABILITY_STARTTLS;
	if (pCapability->isSupportAuth("CRAM-MD5"))
		nAuth_ |= AUTH_CRAMMD5;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Imap4::processLogin()
{
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrUserName;
	string_ptr<WSTRING> wstrPassword;
	status = pImap4Callback_->getUserInfo(&wstrUserName, &wstrPassword);
	CHECK_QSTATUS_ERROR_OR(IMAP4_ERROR_LOGIN | IMAP4_ERROR_OTHER);
	string_ptr<STRING> strUserName(wcs2mbs(wstrUserName.get()));
	CHECK_ERROR(!strUserName.get(), QSTATUS_OUTOFMEMORY,
		IMAP4_ERROR_LOGIN | IMAP4_ERROR_OTHER);
	string_ptr<STRING> strPassword(wcs2mbs(wstrPassword.get()));
	CHECK_ERROR(!strPassword.get(), QSTATUS_OUTOFMEMORY,
		IMAP4_ERROR_LOGIN | IMAP4_ERROR_OTHER);
	
	status = pImap4Callback_->authenticating();
	CHECK_QSTATUS();
	
	unsigned int nAllowedMethods = 0;
	status = getAuthMethods(&nAllowedMethods);
	CHECK_QSTATUS();
	unsigned int nAuth = nAuth_ & nAllowedMethods;
	CHECK_ERROR(nAuth == 0, QSTATUS_FAIL, IMAP4_ERROR_AUTHENTICATE);
	
	bool bLogin = true;
	if (nAuth & AUTH_CRAMMD5) {
		ListParserCallback callback;
		string_ptr<STRING> strTag;
		status = sendCommand("AUTHENTICATE CRAM-MD5\r\n", &strTag, &callback);
		CHECK_QSTATUS_ERROR_OR(IMAP4_ERROR_AUTHENTICATE);
		
		const ListParserCallback::ResponseList& l = callback.getResponseList();
		if (l.size() == 1 && l.back()->getType() == Response::TYPE_CONTINUE) {
			const State* pState = static_cast<const ResponseContinue*>(
				l.back())->getState();
			if (pState) {
				const CHAR* pszText = pState->getMessage();
				if (pszText) {
					Base64Encoder encoder(false, &status);
					CHECK_QSTATUS_ERROR(IMAP4_ERROR_AUTHENTICATE |
						IMAP4_ERROR_OTHER);
					unsigned char* p = 0;
					size_t nLen = 0;
					status = encoder.decode(
						reinterpret_cast<const unsigned char*>(pszText),
						strlen(pszText), &p, &nLen);
					CHECK_QSTATUS_ERROR(IMAP4_ERROR_AUTHENTICATE |
						IMAP4_ERROR_OTHER);
					malloc_ptr<unsigned char> pChallenge(p);
					
					CHAR szDigestString[128] = "";
					MD5::hmacToString(pChallenge.get(), nLen,
						reinterpret_cast<unsigned char*>(strPassword.get()),
						strlen(strPassword.get()), szDigestString);
					
					string_ptr<STRING> strKey(concat(
						strUserName.get(), " ", szDigestString));
					CHECK_ERROR(!strKey.get(), QSTATUS_OUTOFMEMORY,
						IMAP4_ERROR_AUTHENTICATE | IMAP4_ERROR_OTHER);
					status = encoder.encode(
						reinterpret_cast<unsigned char*>(strKey.get()),
						strlen(strKey.get()), &p, &nLen);
					CHECK_QSTATUS_ERROR(IMAP4_ERROR_AUTHENTICATE |
						IMAP4_ERROR_OTHER);
					malloc_ptr<unsigned char> pKey(p);
					
					StringBuffer<STRING> bufSend(
						reinterpret_cast<CHAR*>(pKey.get()), nLen, &status);
					CHECK_QSTATUS_ERROR(IMAP4_ERROR_AUTHENTICATE |
						IMAP4_ERROR_OTHER);
					status = bufSend.append("\r\n");
					CHECK_QSTATUS_ERROR(IMAP4_ERROR_AUTHENTICATE |
						IMAP4_ERROR_OTHER);
					callback.clear();
					status = send(bufSend.getCharArray(), strTag.get(), false, &callback);
					CHECK_QSTATUS_ERROR_OR(IMAP4_ERROR_AUTHENTICATE);
					bLogin = callback.getResponse() != ResponseState::FLAG_OK;
				}
			}
		}
	}
	
	if (bLogin) {
		string_ptr<STRING> strQuotedUserName;
		status = getQuotedString(strUserName.get(), &strQuotedUserName);
		CHECK_QSTATUS_ERROR(IMAP4_ERROR_LOGIN | IMAP4_ERROR_OTHER);
		string_ptr<STRING> strQuotedPassword;
		status = getQuotedString(strPassword.get(), &strQuotedPassword);
		CHECK_QSTATUS_ERROR(IMAP4_ERROR_LOGIN | IMAP4_ERROR_OTHER);
		const Concat c[] = {
			{ "LOGIN ",					-1 },
			{ strQuotedUserName.get(),	-1 },
			{ " ",						-1 },
			{ strQuotedPassword.get(),	-1 },
			{ "\r\n",					-1 }
		};
		string_ptr<STRING> strSend(concat(c, countof(c)));
		CHECK_QSTATUS_ERROR(IMAP4_ERROR_LOGIN | IMAP4_ERROR_OTHER);
		
		ListParserCallback callback;
		status = sendCommand(strSend.get(), &callback);
		CHECK_QSTATUS_ERROR_OR(IMAP4_ERROR_LOGIN);
		CHECK_ERROR(callback.getResponse() != ResponseState::FLAG_OK,
			QSTATUS_FAIL, IMAP4_ERROR_RESPONSE | IMAP4_ERROR_LOGIN);
	}
	status = pImap4Callback_->setPassword(wstrPassword.get());
	CHECK_QSTATUS_ERROR(IMAP4_ERROR_LOGIN | IMAP4_ERROR_OTHER);
	
	nError_ = IMAP4_ERROR_SUCCESS;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Imap4::receive(const CHAR* pszTag,
	bool bAcceptContinue, ParserCallback* pCallback)
{
	assert(pCallback);
	
	DECLARE_QSTATUS();
	
	CHECK_ERROR(!pSocket_, QSTATUS_FAIL, IMAP4_ERROR_INVALIDSOCKET);
	
	Buffer buf(strOverBuf_, pSocket_, &status);
	CHECK_QSTATUS_ERROR(IMAP4_ERROR_OTHER);
	freeString(strOverBuf_);
	strOverBuf_ = 0;
	
	Parser parser(&buf, pImap4Callback_, &status);
	CHECK_QSTATUS_ERROR(IMAP4_ERROR_OTHER);
	status = parser.parse(pszTag, bAcceptContinue, pCallback);
	if (status == QSTATUS_SUCCESS) {
		// TODO
		// Log
		status = parser.getUnprocessedString(&strOverBuf_);
		CHECK_QSTATUS();
		
		nError_ = IMAP4_ERROR_SUCCESS;
	}
	else {
		bDisconnected_ = true;
		// TODO
		// Log
		
		nError_ = buf.getError();
		if (nError_ == IMAP4_ERROR_SUCCESS)
			nError_ = IMAP4_ERROR_PARSE;
	}
	
	return status;
}

QSTATUS qmimap4::Imap4::sendCommand(const CHAR* pszCommand, ParserCallback* pCallback)
{
	return sendCommand(pszCommand, 0, pCallback);
}

QSTATUS qmimap4::Imap4::sendCommand(const CHAR* pszCommand,
	STRING* pstrTag, ParserCallback* pCallback)
{
	assert(pszCommand);
	assert(pCallback);
	
	DECLARE_QSTATUS();
	
	CHECK_ERROR(!pSocket_, QSTATUS_FAIL, IMAP4_ERROR_INVALIDSOCKET);
	
	string_ptr<STRING> strTag;
	status = getTag(&strTag);
	CHECK_QSTATUS_ERROR(IMAP4_ERROR_OTHER);
	
	string_ptr<STRING> strSendTag(allocString(strTag.get()));
	CHECK_QSTATUS_ERROR(IMAP4_ERROR_OTHER);
	
	const CHAR* pszContents[] = {
		strSendTag.get(),
		" ",
		pszCommand
	};
	
	const CHAR* pszTag = strTag.get();
	bool bAcceptContinue = false;
	if (pstrTag) {
		*pstrTag = strTag.release();
		bAcceptContinue = true;
	}
	
	return send(pszContents, countof(pszContents),
		pszTag, bAcceptContinue, pCallback);
}

QSTATUS qmimap4::Imap4::send(const CHAR* pszContent,
	const CHAR* pszTag, bool bAcceptContinue, ParserCallback* pCallback)
{
	return send(&pszContent, 1, pszTag, bAcceptContinue, pCallback);
}

QSTATUS qmimap4::Imap4::send(const CHAR** pszContents, size_t nCount,
	const CHAR* pszTag, bool bAcceptContinue, ParserCallback* pCallback)
{
	assert(pszContents);
	assert(pCallback);
	
	DECLARE_QSTATUS();
	
	CHECK_ERROR(!pSocket_, QSTATUS_FAIL, IMAP4_ERROR_INVALIDSOCKET);
	
	for (size_t n = 0; n < nCount; ++n) {
		const CHAR* pszContent = *(pszContents + n);
		size_t nLen = strlen(pszContent);
		
		size_t nTotalSend = 0;
		while (nTotalSend < nLen) {
			int nSelect = Socket::SELECT_READ | Socket::SELECT_WRITE;
			status = pSocket_->select(&nSelect);
			CHECK_QSTATUS_ERROR(IMAP4_ERROR_SELECTSOCKET | pSocket_->getLastError());
			CHECK_ERROR(nSelect == 0, QSTATUS_FAIL, IMAP4_ERROR_TIMEOUT);
			
			int nSend = nLen - nTotalSend;
			status = pSocket_->send(pszContent + nTotalSend, &nSend, 0);
			CHECK_QSTATUS_ERROR(IMAP4_ERROR_SEND | pSocket_->getLastError());
			nTotalSend += nSend;
		}
	}
	
	// TODO
	// Log
	
	return receive(pszTag, bAcceptContinue, pCallback);
}

QSTATUS qmimap4::Imap4::sendCommandTokens(
	const CommandToken* pTokens, size_t nCount)
{
	assert(pTokens);
	
	DECLARE_QSTATUS();
	
	StringBuffer<STRING> buf(&status);
	CHECK_QSTATUS();
	
	for (size_t n = 0; n < nCount; ++n) {
		const CommandToken* pToken = pTokens + n;
		if (pToken->b_) {
			const CHAR* psz = 0;
			string_ptr<STRING> str;
			
			if (pToken->psz_) {
				psz = pToken->psz_;
			}
			else {
				if (pToken->pConverter_) {
					size_t nLen = wcslen(pToken->pwsz_);
					status = pToken->pConverter_->encode(
						pToken->pwsz_, &nLen, &str, 0);
					CHECK_QSTATUS_ERROR(IMAP4_ERROR_OTHER);
				}
				else {
					str.reset(wcs2mbs(pToken->pwsz_));
					CHECK_ERROR(!str.get(), QSTATUS_OUTOFMEMORY,
						IMAP4_ERROR_OTHER);
				}
				psz = str.get();
			}
			assert(psz);
			
			string_ptr<STRING> strQuoted;
			if (pToken->bQuote_) {
				status = getQuotedString(psz, &strQuoted);
				CHECK_QSTATUS_ERROR(IMAP4_ERROR_OTHER);
				psz = strQuoted.get();
			}
			
			status = buf.append(psz);
			CHECK_QSTATUS_ERROR(IMAP4_ERROR_OTHER);
		}
	}
	
	Imap4ParserCallback callback(pImap4Callback_);
	status = sendCommand(buf.getCharArray(), &callback);
	CHECK_QSTATUS();
	CHECK_ERROR(callback.getResponse() != ResponseState::FLAG_OK,
		QSTATUS_FAIL, IMAP4_ERROR_RESPONSE);
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Imap4::getTag(STRING* pstrTag)
{
	assert(pstrTag);
	
	*pstrTag = allocString(32);
	if (!*pstrTag)
		return QSTATUS_FAIL;
	sprintf(*pstrTag, "q%04lu", nTag_++);
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Imap4::getAuthMethods(unsigned int* pnAuth)
{
	assert(pnAuth);
	
	DECLARE_QSTATUS();
	
	*pnAuth = 0;
	
	string_ptr<WSTRING> wstrAuthMethods;
	status = pImap4Callback_->getAuthMethods(&wstrAuthMethods);
	CHECK_QSTATUS();
	
	struct {
		const WCHAR* pwsz_;
		Auth auth_;
	} methods[] = {
		{ L"LOGIN",		AUTH_LOGIN		},
		{ L"CRAM-MD5",	AUTH_CRAMMD5	}
	};
	
	const WCHAR* p = wcstok(wstrAuthMethods.get(), L" ");
	while (p) {
		for (int n = 0; n < countof(methods); ++n) {
			if (wcscmp(p, methods[n].pwsz_) == 0) {
				*pnAuth |= methods[n].auth_;
				break;
			}
		}
		p = wcstok(0, L" ");
	}
	
	if (*pnAuth == 0)
		*pnAuth = 0xffffffff;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Imap4::getQuotedString(const CHAR* psz, STRING* pstrQuoted)
{
	assert(psz);
	assert(pstrQuoted);
	
	DECLARE_QSTATUS();
	
	StringBuffer<STRING> buf(strlen(psz) + 1, &status);
	CHECK_QSTATUS();
	
	status = buf.append('\"');
	CHECK_QSTATUS();
	
	while (*psz) {
		if (*psz == '\"' || *psz == '\\') {
			status = buf.append('\\');
			CHECK_QSTATUS();
		}
		status = buf.append(*psz);
		CHECK_QSTATUS();
		++psz;
	}
	
	status = buf.append('\"');
	CHECK_QSTATUS();
	
	*pstrQuoted = buf.getString();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Imap4::encodeSearchString(
	const WCHAR* pwsz, const WCHAR* pwszCharset, STRING* pstr)
{
	assert(pwsz);
	assert(pwszCharset);
	assert(pstr);
	
	DECLARE_QSTATUS();
	
	std::auto_ptr<Converter> pConverter;
	status = ConverterFactory::getInstance(pwszCharset, &pConverter);
	CHECK_QSTATUS();
	if (!pConverter.get()) {
		status = ConverterFactory::getInstance(L"utf-8", &pConverter);
		CHECK_QSTATUS();
	}
	assert(pConverter.get());
	
	StringBuffer<STRING> buf(&status);
	CHECK_QSTATUS();
	
	const WCHAR* pBegin = pwsz;
	for (const WCHAR* p = pwsz; ; ++p) {
		if (*p == L'\"' || *p == L'\\' || *p == L'\0') {
			if (pBegin != p) {
				string_ptr<STRING> str;
				size_t nLen = p - pBegin;
				status = pConverter->encode(pBegin, &nLen, &str, 0);
				CHECK_QSTATUS();
				
				for (CHAR* p = str.get(); *p; ++p) {
					if (*p == '\"' || *p == '\\') {
						status = buf.append('\\');
						CHECK_QSTATUS();
					}
					status = buf.append(*p);
					CHECK_QSTATUS();
				}
			}
			if (!*p)
				break;
			status = buf.append(static_cast<CHAR>(*p));
			CHECK_QSTATUS();
			pBegin = p + 1;
		}
	}
	
	*pstr = buf.getString();
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * Imap4Callback
 *
 */

qmimap4::Imap4Callback::~Imap4Callback()
{
}


/****************************************************************************
 *
 * Range
 *
 */

qmimap4::Range::~Range()
{
}


/****************************************************************************
 *
 * DefaultRange
 *
 */

qmimap4::DefaultRange::DefaultRange(bool bUid) :
	bUid_(bUid),
	strRange_(0)
{
}

qmimap4::DefaultRange::~DefaultRange()
{
	freeString(strRange_);
}

bool qmimap4::DefaultRange::isUid() const
{
	return bUid_;
}

const CHAR* qmimap4::DefaultRange::getRange() const
{
	assert(strRange_);
	return strRange_;
}

QSTATUS qmimap4::DefaultRange::setRange(const CHAR* pszRange)
{
	assert(!strRange_);
	assert(pszRange);
	
	strRange_ = allocString(pszRange);
	
	return strRange_ ? QSTATUS_SUCCESS : QSTATUS_OUTOFMEMORY;
}


/****************************************************************************
 *
 * SingleRange
 *
 */

qmimap4::SingleRange::SingleRange(unsigned long n, bool bUid, QSTATUS* pstatus) :
	DefaultRange(bUid)
{
	assert(pstatus);
	assert(n != 0);
	
	CHAR szRange[128];
	sprintf(szRange, "%lu", n);
	*pstatus = setRange(szRange);
}

qmimap4::SingleRange::~SingleRange()
{
}


/****************************************************************************
 *
 * ContinuousRange
 *
 */

qmimap4::ContinuousRange::ContinuousRange(unsigned long nBegin,
	unsigned long nEnd, bool bUid, QSTATUS* pstatus) :
	DefaultRange(bUid)
{
	assert(pstatus);
	assert(nBegin != 0 && nBegin != static_cast<unsigned long>(-1));
	assert(nEnd == static_cast<unsigned long>(-1) || nBegin <= nEnd);
	
	CHAR szRange[128];
	if (nEnd == static_cast<unsigned long>(-1))
		sprintf(szRange, "%lu:*", nBegin);
	else if (nBegin == nEnd)
		sprintf(szRange, "%lu", nBegin);
	else
		sprintf(szRange, "%lu:%lu", nBegin, nEnd);
	*pstatus = setRange(szRange);
}

qmimap4::ContinuousRange::~ContinuousRange()
{
}


/****************************************************************************
 *
 * MultipleRange
 *
 */

qmimap4::MultipleRange::MultipleRange(const unsigned long* pn,
	size_t nCount, bool bUid, QSTATUS* pstatus) :
	DefaultRange(bUid)
{
	assert(pstatus);
	assert(pn);
	assert(nCount != 0);
	
	DECLARE_QSTATUS();
	
	StringBuffer<STRING> buf(&status);
	CHECK_QSTATUS_SET(pstatus);
	
	CHAR sz[128];
	unsigned long nBegin = 0;
	unsigned long nEnd = 0;
	for (size_t n = 0; n <= nCount; ++n) {
		unsigned long nId = n != nCount ? *(pn + n) : 0;
		
		assert(n == nCount || (nId != 0 && nId != static_cast<unsigned long>(-1)));
		
		if (nBegin == 0) {
			nBegin = nId;
			nEnd = nId;
		}
		else if (nEnd == nId - 1) {
			nEnd = nId;
		}
		else {
			assert(nBegin <= nEnd);
			if (nBegin == nEnd)
				sprintf(sz, "%lu", nBegin);
			else
				sprintf(sz, "%lu:%lu", nBegin, nEnd);
			
			if (buf.getLength() != 0) {
				status = buf.append(",");
				CHECK_QSTATUS_SET(pstatus);
			}
			status = buf.append(sz);
			CHECK_QSTATUS_SET(pstatus);
			
			nBegin = nId;
			nEnd = nId;
		}
	}
	
	*pstatus = setRange(buf.getCharArray());
}

qmimap4::MultipleRange::~MultipleRange()
{
}


/****************************************************************************
 *
 * TextRange
 *
 */

qmimap4::TextRange::TextRange(const CHAR* pszRange, bool bUid, QSTATUS* pstatus) :
	DefaultRange(bUid)
{
	*pstatus = setRange(pszRange);
}

qmimap4::TextRange::~TextRange()
{
}


/****************************************************************************
 *
 * Flags
 *
 */

qmimap4::Flags::Flags(unsigned int nSystemFlags, QSTATUS* pstatus) :
	nSystemFlags_(0)
{
	assert(pstatus);
	*pstatus = init(nSystemFlags, 0, 0);
}

qmimap4::Flags::Flags(unsigned int nSystemFlags,
	const CHAR** pszUserFlags, size_t nCount, QSTATUS* pstatus) :
	nSystemFlags_(0)
{
	assert(pstatus);
	*pstatus = init(nSystemFlags, pszUserFlags, nCount);
}

qmimap4::Flags::~Flags()
{
	std::for_each(listFlag_.begin(), listFlag_.end(), string_free<STRING>());
}

QSTATUS qmimap4::Flags::getString(STRING* pstr) const
{
	assert(pstr);
	
	DECLARE_QSTATUS();
	
	StringBuffer<STRING> buf(&status);
	CHECK_QSTATUS();
	
	const Imap4::Flag flags[] = {
		Imap4::FLAG_ANSWERED,
		Imap4::FLAG_FLAGGED,
		Imap4::FLAG_DELETED,
		Imap4::FLAG_SEEN,
		Imap4::FLAG_DRAFT
	};
	
	for (int n = 0; n < countof(flags); ++n) {
		if (nSystemFlags_ & flags[n]) {
			if (buf.getLength() == 0)
				status = buf.append("(");
			else
				status = buf.append(" ");
			CHECK_QSTATUS();
			status = buf.append(getFlagString(flags[n]));
			CHECK_QSTATUS();
		}
	}
	
	FlagList::const_iterator it = listFlag_.begin();
	while (it != listFlag_.end()) {
		if (buf.getLength() == 0)
			status = buf.append("(");
		else
			status = buf.append(" ");
		CHECK_QSTATUS();
		status = buf.append(*it);
		CHECK_QSTATUS();
		++it;
	}
	
	if (buf.getLength() != 0) {
		status = buf.append(")");
		CHECK_QSTATUS();
	}
	*pstr = buf.getString();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Flags::getAdded(const Flags& mask, STRING* pstr) const
{
	return getString(mask, true, pstr);
}

QSTATUS qmimap4::Flags::getRemoved(const Flags& mask, STRING* pstr) const
{
	return getString(mask, false, pstr);
}

bool qmimap4::Flags::contains(Imap4::Flag flag) const
{
	return (nSystemFlags_ & flag) != 0;
}

bool qmimap4::Flags::contains(const CHAR* pszFlag) const
{
	return std::find_if(listFlag_.begin(), listFlag_.end(),
		std::bind2nd(string_equal<CHAR>(), pszFlag)) != listFlag_.end();
}

QSTATUS qmimap4::Flags::init(unsigned int nSystemFlags,
	const CHAR** pszUserFlags, size_t nCount)
{
	DECLARE_QSTATUS();
	
	nSystemFlags_ = nSystemFlags;
	
	status = STLWrapper<FlagList>(listFlag_).reserve(nCount);
	CHECK_QSTATUS();
	
	for (size_t n = 0; n < nCount; ++n) {
		STRING strFlag = allocString(*(pszUserFlags + n));
		if (!strFlag)
			return QSTATUS_OUTOFMEMORY;
		listFlag_.push_back(strFlag);
	}
	
	return QSTATUS_SUCCESS;
}

const CHAR* qmimap4::Flags::getFlagString(Imap4::Flag flag)
{
	struct {
		Imap4::Flag flag_;
		const CHAR* psz_;
	} flags[] = {
		{ Imap4::FLAG_ANSWERED,	"\\ANSWERED"	},
		{ Imap4::FLAG_FLAGGED,	"\\FLAGGED"		},
		{ Imap4::FLAG_DELETED,	"\\DELETED"		},
		{ Imap4::FLAG_SEEN,		"\\SEEN"		},
		{ Imap4::FLAG_DRAFT,	"\\DRAFT"		}
	};
	for (int n = 0; n < countof(flags); ++n) {
		if (flag == flags[n].flag_)
			return flags[n].psz_;
	}
	assert(false);
	return 0;
}

QSTATUS qmimap4::Flags::getString(const Flags& mask, bool bAdd, STRING* pstr) const
{
	assert(pstr);
	
	DECLARE_QSTATUS();
	
	*pstr = 0;
	
	const Imap4::Flag flags[] = {
		Imap4::FLAG_ANSWERED,
		Imap4::FLAG_FLAGGED,
		Imap4::FLAG_DELETED,
		Imap4::FLAG_SEEN,
		Imap4::FLAG_DRAFT
	};
	
	StringBuffer<STRING> buf(&status);
	CHECK_QSTATUS();
	
	for (int n = 0; n < countof(flags); ++n) {
		if ((mask.nSystemFlags_ & flags[n])) {
			bool b = contains(flags[n]);
			if ((bAdd && b) || (!bAdd && !b)) {
				if (buf.getLength() == 0)
					status = buf.append("(");
				else
					status = buf.append(" ");
				CHECK_QSTATUS();
				status = buf.append(getFlagString(flags[n]));
				CHECK_QSTATUS();
			}
		}
	}
	
	FlagList::const_iterator it = mask.listFlag_.begin();
	while (it != mask.listFlag_.end()) {
		bool b = contains(*it);
		if ((bAdd && b) || (!bAdd && !b)) {
			if (buf.getLength() == 0)
				status = buf.append("(");
			else
				status = buf.append(" ");
			CHECK_QSTATUS();
			status = buf.append(*it);
			CHECK_QSTATUS();
		}
	}
	
	if (buf.getLength() != 0) {
		status = buf.append(")");
		CHECK_QSTATUS();
	}
	
	if (buf.getLength() != 0)
		*pstr = buf.getString();
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * PartPath
 *
 */

qmimap4::PartPath::PartPath(const unsigned int* pnPart,
	size_t nCount, QSTATUS* pstatus) :
	strPath_(0)
{
	assert(pnPart);
	assert(nCount != 0);
	
	DECLARE_QSTATUS();
	
	*pstatus = QSTATUS_SUCCESS;
	
	StringBuffer<STRING> buf(&status);
	CHECK_QSTATUS_SET(pstatus);
	
	CHAR sz[32];
	for (size_t n = 0; n < nCount; ++n) {
		sprintf(sz, "%u", *(pnPart + n));
		if (buf.getLength() != 0) {
			status = buf.append(".");
			CHECK_QSTATUS_SET(pstatus);
		}
		status = buf.append(sz);
		CHECK_QSTATUS_SET(pstatus);
	}
	
	strPath_ = buf.getString();
}

qmimap4::PartPath::~PartPath()
{
	freeString(strPath_);
}

const CHAR* qmimap4::PartPath::getPath() const
{
	return strPath_;
}


/****************************************************************************
 *
 * Response
 *
 */

qmimap4::Response::Response(Type type) :
	type_(type)
{
}

qmimap4::Response::~Response()
{
}

Response::Type qmimap4::Response::getType() const
{
	return type_;
}


/****************************************************************************
 *
 * ResponseCapability
 *
 */

qmimap4::ResponseCapability::ResponseCapability(QSTATUS* pstatus) :
	Response(TYPE_CAPABILITY)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qmimap4::ResponseCapability::~ResponseCapability()
{
	std::for_each(listCapability_.begin(),
		listCapability_.end(), string_free<STRING>());
	std::for_each(listAuth_.begin(), listAuth_.end(), string_free<STRING>());
}

bool qmimap4::ResponseCapability::isSupport(const CHAR* pszCapability) const
{
	return std::find_if(listCapability_.begin(), listCapability_.end(),
		std::bind2nd(string_equal_i<CHAR>(), pszCapability)) != listCapability_.end();
}

bool qmimap4::ResponseCapability::isSupportAuth(const CHAR* pszAuth) const
{
	return std::find_if(listAuth_.begin(), listAuth_.end(),
		std::bind2nd(string_equal_i<CHAR>(), pszAuth)) != listAuth_.end();
}

QSTATUS qmimap4::ResponseCapability::add(const CHAR* psz)
{
	assert(psz);
	
	DECLARE_QSTATUS();
	
	CapabilityList* p = 0;
	if (_strnicmp(psz, "AUTH=", 5) == 0)
		p = &listAuth_;
	else
		p = &listCapability_;
	
	string_ptr<STRING> str;
	if (p == &listAuth_)
		str.reset(allocString(psz + 5));
	else
		str.reset(allocString(psz));
	if (!str.get())
		return QSTATUS_OUTOFMEMORY;
	
	status = STLWrapper<CapabilityList>(*p).push_back(str.get());
	CHECK_QSTATUS();
	str.release();
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * ResponseContinue
 *
 */

qmimap4::ResponseContinue::ResponseContinue(
	State* pState, QSTATUS* pstatus) :
	Response(TYPE_CONTINUE),
	pState_(pState)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qmimap4::ResponseContinue::~ResponseContinue()
{
	delete pState_;
}

const State* qmimap4::ResponseContinue::getState() const
{
	return pState_;
}


/****************************************************************************
 *
 * ResponseExists
 *
 */

qmimap4::ResponseExists::ResponseExists(
	unsigned long nExists, QSTATUS* pstatus) :
	Response(TYPE_EXISTS),
	nExists_(nExists)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qmimap4::ResponseExists::~ResponseExists()
{
}

unsigned long qmimap4::ResponseExists::getExists() const
{
	return nExists_;
}


/****************************************************************************
 *
 * ResponseExpunge
 *
 */

qmimap4::ResponseExpunge::ResponseExpunge(
	unsigned long nExpunge, QSTATUS* pstatus) :
	Response(TYPE_EXPUNGE),
	nExpunge_(nExpunge)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qmimap4::ResponseExpunge::~ResponseExpunge()
{
}

unsigned long qmimap4::ResponseExpunge::getExpunge() const
{
	return nExpunge_;
}


/****************************************************************************
 *
 * ResponseFetch
 *
 */

qmimap4::ResponseFetch::ResponseFetch(
	unsigned long nNumber, List* pList, QSTATUS* pstatus) :
	Response(TYPE_FETCH),
	nNumber_(nNumber)
{
	assert(pstatus);
	
	DECLARE_QSTATUS();
	
	*pstatus = QSTATUS_FAIL;
	
	const List::ItemList& l = pList->getList();
	if (l.size() % 2 != 0)
		return;
	status = STLWrapper<FetchDataList>(listData_).reserve(l.size()/2);
	CHECK_QSTATUS_SET(pstatus);
	
	for (List::ItemList::size_type n = 0; n < l.size(); n += 2) {
		if (l[n]->getType() != ListItem::TYPE_TEXT)
			return;
		
		string_ptr<STRING> strName(
			static_cast<ListItemText*>(l[n])->releaseText());
		if (_stricmp(strName.get(), "ENVELOPE") == 0) {
			// ENVELOPE
			if (l[n + 1]->getType() != ListItem::TYPE_LIST)
				return;
			
			FetchDataEnvelope* pEnvelope = 0;
			status = newQsObject(static_cast<List*>(l[n + 1]), &pEnvelope);
			CHECK_QSTATUS_SET(pstatus);
			listData_.push_back(pEnvelope);
		}
		else if (_stricmp(strName.get(), "FLAGS") == 0) {
			// FLAGS
			if (l[n + 1]->getType() != ListItem::TYPE_LIST)
				return;
			
			FetchDataFlags* pFlags = 0;
			status = newQsObject(static_cast<List*>(l[n + 1]), &pFlags);
			CHECK_QSTATUS_SET(pstatus);
			listData_.push_back(pFlags);
		}
		else if (_stricmp(strName.get(), "INTERNALDATE") == 0) {
			// INTERNALDATE
			if (l[n + 1]->getType() != ListItem::TYPE_TEXT)
				return;
			
			FetchDataInternalDate* pDate = 0;
			status = newQsObject(
				static_cast<ListItemText*>(l[n + 1])->getText(), &pDate);
			CHECK_QSTATUS_SET(pstatus);
			listData_.push_back(pDate);
		}
		else if (_stricmp(strName.get(), "RFC822") == 0 ||
			_stricmp(strName.get(), "RFC822.HEADER") == 0 ||
			_stricmp(strName.get(), "RFC822.TEXT") == 0) {
			// RFC822
			if (l[n + 1]->getType() != ListItem::TYPE_TEXT)
				return;
			
			const CHAR* pszSection = strName.get()[6] == '\0' ? 0 : strName.get() + 7;
			FetchDataBody* pBody = 0;
			status = newQsObject(pszSection,
				static_cast<ListItemText*>(l[n + 1])->releaseText(), &pBody);
			CHECK_QSTATUS_SET(pstatus);
			listData_.push_back(pBody);
		}
		else if (_stricmp(strName.get(), "RFC822.SIZE") == 0) {
			// SIZE
			if (l[n + 1]->getType() != ListItem::TYPE_TEXT)
				return;
			
			const CHAR* pszSize = static_cast<ListItemText*>(l[n + 1])->getText();
			CHAR* pEnd = 0;
			long nSize = strtol(pszSize, &pEnd, 10);
			if (*pEnd)
				return;
			
			FetchDataSize* pSize = 0;
			status = newQsObject(nSize, &pSize);
			CHECK_QSTATUS_SET(pstatus);
			listData_.push_back(pSize);
		}
		else if (_stricmp(strName.get(), "BODY") == 0 ||
			_stricmp(strName.get(), "BODYSTRUCTURE") == 0) {
			// BODY, BODYSTRUCTURE
			if (l[n + 1]->getType() != ListItem::TYPE_LIST)
				return;
			
			bool bExtended = strName.get()[4] != '\0';
			FetchDataBodyStructure* pStructure = 0;
			status = newQsObject(static_cast<List*>(l[n + 1]),
				bExtended, &pStructure);
			CHECK_QSTATUS_SET(pstatus);
			listData_.push_back(pStructure);
		}
		else if (_stricmp(strName.get(), "UID") == 0) {
			// UID
			if (l[n + 1]->getType() != ListItem::TYPE_TEXT)
				return;
			
			const CHAR* pszUid = static_cast<ListItemText*>(l[n + 1])->getText();
			CHAR* pEnd = 0;
			long nUid = strtol(pszUid, &pEnd, 10);
			if (*pEnd)
				return;
			
			FetchDataUid* pUid = 0;
			status = newQsObject(nUid, &pUid);
			CHECK_QSTATUS_SET(pstatus);
			listData_.push_back(pUid);
		}
		else if (_strnicmp(strName.get(), "BODY[", 5) == 0) {
			// BODY[SECTION]
			if (l[n + 1]->getType() != ListItem::TYPE_TEXT)
				return;
			
			const CHAR* p = strName.get() + 5;
			CHAR* pEnd = strchr(p, ']');
			if (!pEnd)
				return;
			*pEnd = '\0';
			
			FetchDataBody* pBody = 0;
			status = newQsObject(p,
				static_cast<ListItemText*>(l[n + 1])->releaseText(), &pBody);
			CHECK_QSTATUS_SET(pstatus);
			listData_.push_back(pBody);
		}
		else {
			return;
		}
	}
	
	*pstatus = QSTATUS_SUCCESS;
}

qmimap4::ResponseFetch::~ResponseFetch()
{
	std::for_each(listData_.begin(), listData_.end(), deleter<FetchData>());
}

unsigned long qmimap4::ResponseFetch::getNumber() const
{
	return nNumber_;
}

bool qmimap4::ResponseFetch::isUid(unsigned long nUid) const
{
	FetchDataList::const_iterator it = listData_.begin();
	while (it != listData_.end()) {
		assert(*it);
		if ((*it)->getType() == FetchData::TYPE_UID &&
			static_cast<FetchDataUid*>(*it)->getUid() == nUid)
			return true;
	}
	return false;
}

const ResponseFetch::FetchDataList& qmimap4::ResponseFetch::getFetchDataList() const
{
	return listData_;
}

FetchData* qmimap4::ResponseFetch::detach(FetchData* pFetchData)
{
	assert(pFetchData);
	
	FetchDataList::iterator it = std::find(
		listData_.begin(), listData_.end(), pFetchData);
	assert(it != listData_.end());
	*it = 0;
	
	return pFetchData;
}


/****************************************************************************
 *
 * ResponseFlags
 *
 */

qmimap4::ResponseFlags::ResponseFlags(List* pList, QSTATUS* pstatus) :
	Response(TYPE_FLAGS),
	nSystemFlags_(0)
{
	assert(pstatus);
	
	DECLARE_QSTATUS();
	
	*pstatus = QSTATUS_FAIL;
	
	const List::ItemList& l = pList->getList();
	for (List::ItemList::const_iterator it = l.begin(); it != l.end(); ++it) {
		if ((*it)->getType() != ListItem::TYPE_TEXT)
			return;
		
		struct {
			const CHAR* pszName_;
			Imap4::Flag flag_;
		} flags[] = {
			{ "\\ANSWERED",	Imap4::FLAG_ANSWERED	},
			{ "\\FLAGGED",	Imap4::FLAG_FLAGGED		},
			{ "\\DELETED",	Imap4::FLAG_DELETED		},
			{ "\\SEEN",		Imap4::FLAG_SEEN		},
			{ "\\DRAFT",	Imap4::FLAG_DRAFT		},
		};
		const CHAR* pszFlag = static_cast<ListItemText*>(*it)->getText();
		for (int n = 0; n < countof(flags); ++n) {
			if (_stricmp(pszFlag, flags[n].pszName_) == 0) {
				nSystemFlags_ |= flags[n].flag_;
				break;
			}
		}
		if (n == countof(flags)) {
			string_ptr<STRING> str(static_cast<ListItemText*>(*it)->releaseText());
			status = STLWrapper<FlagList>(listCustomFlag_).push_back(str.get());
			CHECK_QSTATUS_SET(pstatus);
			str.release();
		}
	}
	
	*pstatus = QSTATUS_SUCCESS;
}

qmimap4::ResponseFlags::~ResponseFlags()
{
	std::for_each(listCustomFlag_.begin(), listCustomFlag_.end(),
		string_free<STRING>());
}

unsigned int qmimap4::ResponseFlags::getSystemFlags() const
{
	return nSystemFlags_;
}

const ResponseFlags::FlagList& qmimap4::ResponseFlags::getCustomFlags() const
{
	return listCustomFlag_;
}


/****************************************************************************
 *
 * ResponseList
 *
 */

qmimap4::ResponseList::ResponseList(bool bList, List* pListAttribute,
	CHAR cSeparator, const CHAR* pszMailbox, qs::QSTATUS* pstatus) :
	Response(TYPE_LIST),
	bList_(bList),
	nAttributes_(0),
	cSeparator_('\0'),
	wstrMailbox_(0)
{
	assert(pstatus);
	
	*pstatus = QSTATUS_FAIL;
	
	DECLARE_QSTATUS();
	
	UTF7Converter converter(true, &status);
	CHECK_QSTATUS_SET(pstatus);
	size_t nLen = strlen(pszMailbox);
	status = converter.decode(pszMailbox, &nLen, &wstrMailbox_, 0);
	CHECK_QSTATUS_SET(pstatus);
	
	cSeparator_ = cSeparator;
	
	const List::ItemList& l = pListAttribute->getList();
	for (List::ItemList::const_iterator it = l.begin(); it != l.end(); ++it) {
		if ((*it)->getType() != ListItem::TYPE_TEXT)
			return;
		
		struct {
			const CHAR* pszName_;
			Attribute attribute_;
		} attributes[] = {
			{ "\\NOINFERIORS",	ATTRIBUTE_NOINFERIORS	},
			{ "\\NOSELECT",		ATTRIBUTE_NOSELECT		},
			{ "\\MARKED",		ATTRIBUTE_MARKED		},
			{ "\\UNMARKED",		ATTRIBUTE_UNMARKED		}
		};
		const CHAR* pszAttribute = static_cast<ListItemText*>(*it)->getText();
		for (int n = 0; n < countof(attributes); ++n) {
			if (_stricmp(pszAttribute, attributes[n].pszName_) == 0) {
				nAttributes_ |= attributes[n].attribute_;
				break;
			}
		}
	}
	
	*pstatus = QSTATUS_SUCCESS;
}

qmimap4::ResponseList::~ResponseList()
{
	freeWString(wstrMailbox_);
}

bool qmimap4::ResponseList::isList() const
{
	return bList_;
}

unsigned int qmimap4::ResponseList::getAttributes() const
{
	return nAttributes_;
}

WCHAR qmimap4::ResponseList::getSeparator() const
{
	return cSeparator_;
}

const WCHAR* qmimap4::ResponseList::getMailbox() const
{
	return wstrMailbox_;
}


/****************************************************************************
 *
 * ResponseNamespace
 *
 */

qmimap4::ResponseNamespace::ResponseNamespace(List* pListPersonal,
	List* pListOthers, List* pListShared, qs::QSTATUS* pstatus) :
	Response(TYPE_NAMESPACE)
{
	assert(pstatus);
	
	*pstatus = QSTATUS_FAIL;
	
	DECLARE_QSTATUS();
	
	UTF7Converter converter(true, &status);
	CHECK_QSTATUS_SET(pstatus);
	
	struct {
		List* pList_;
		NamespaceList* pnsl_;
	} namespaces[] = {
		{ pListPersonal,	&listPersonal_	},
		{ pListOthers,		&listOthers_	},
		{ pListShared,		&listShared_	}
	};
	for (int n = 0; n < countof(namespaces); ++n) {
		if (!namespaces[n].pList_)
			continue;
		
		STLWrapper<NamespaceList> wrapper(*namespaces[n].pnsl_);
		const List::ItemList& l = namespaces[n].pList_->getList();
		for (List::ItemList::const_iterator it = l.begin(); it != l.end(); ++it) {
			if ((*it)->getType() != ListItem::TYPE_LIST)
				return;
			
			const List::ItemList& nss = static_cast<List*>(*it)->getList();
			if (nss.size() != 2 ||
				nss.front()->getType() != ListItem::TYPE_TEXT ||
				(nss.back()->getType() != ListItem::TYPE_TEXT &&
					nss.back()->getType() != ListItem::TYPE_NIL))
				return;
			
			const CHAR* pszName = static_cast<ListItemText*>(nss.front())->getText();
			string_ptr<WSTRING> wstrName;
			size_t nLen = strlen(pszName);
			status = converter.decode(pszName, &nLen, &wstrName, 0);
			CHECK_QSTATUS_SET(pstatus);
			
			const CHAR* pszSeparator = "";
			if (nss.back()->getType() == ListItem::TYPE_TEXT)
				pszSeparator = static_cast<ListItemText*>(nss.back())->getText();
			WCHAR cSep = *pszSeparator;
			
			status = wrapper.push_back(std::make_pair(wstrName.get(), cSep));
			CHECK_QSTATUS_SET(pstatus);
			wstrName.release();
		}
	}
	
	*pstatus = QSTATUS_SUCCESS;
}

qmimap4::ResponseNamespace::~ResponseNamespace()
{
	NamespaceList* p[] = {
		&listPersonal_,
		&listOthers_,
		&listShared_
	};
	for (int n = 0; n < countof(p); ++n)
		std::for_each(p[n]->begin(), p[n]->end(),
			unary_compose_f_gx(
				string_free<WSTRING>(),
				std::select1st<NamespaceList::value_type>()));
}

const ResponseNamespace::NamespaceList& qmimap4::ResponseNamespace::getPersonal() const
{
	return listPersonal_;
}

const ResponseNamespace::NamespaceList& qmimap4::ResponseNamespace::getOthers() const
{
	return listOthers_;
}

const ResponseNamespace::NamespaceList& qmimap4::ResponseNamespace::getShared() const
{
	return listShared_;
}


/****************************************************************************
 *
 * ResponseRecent
 *
 */

qmimap4::ResponseRecent::ResponseRecent(
	unsigned long nRecent, qs::QSTATUS* pstatus) :
	Response(TYPE_RECENT),
	nRecent_(nRecent)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qmimap4::ResponseRecent::~ResponseRecent()
{
}

unsigned long qmimap4::ResponseRecent::getRecent() const
{
	return nRecent_;
}


/****************************************************************************
 *
 * ResponseSearch
 *
 */

qmimap4::ResponseSearch::ResponseSearch(QSTATUS* pstatus) :
	Response(TYPE_SEARCH)
{
}

qmimap4::ResponseSearch::~ResponseSearch()
{
}

const ResponseSearch::ResultList& qmimap4::ResponseSearch::getResult() const
{
	return listResult_;
}

QSTATUS qmimap4::ResponseSearch::add(unsigned long n)
{
	return STLWrapper<ResultList>(listResult_).push_back(n);
}


/****************************************************************************
 *
 * ResponseState
 *
 */

qmimap4::ResponseState::ResponseState(Flag flag, QSTATUS* pstatus) :
	Response(TYPE_STATE),
	flag_(flag),
	pState_(0)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qmimap4::ResponseState::~ResponseState()
{
	delete pState_;
}

ResponseState::Flag qmimap4::ResponseState::getFlag() const
{
	return flag_;
}

State* qmimap4::ResponseState::getState() const
{
	return pState_;
}

void qmimap4::ResponseState::setState(State* pState)
{
	pState_ = pState;
}


/****************************************************************************
 *
 * ResponseStatus
 *
 */

qmimap4::ResponseStatus::ResponseStatus(const CHAR* pszMailbox,
	List* pList, QSTATUS* pstatus) :
	Response(TYPE_STATUS),
	wstrMailbox_(0)
{
	assert(pstatus);
	
	*pstatus = QSTATUS_FAIL;
	
	DECLARE_QSTATUS();
	
	UTF7Converter converter(true, &status);
	CHECK_QSTATUS_SET(pstatus);
	
	size_t nLen = static_cast<size_t>(-1);
	status = converter.decode(pszMailbox, &nLen, &wstrMailbox_, 0);
	CHECK_QSTATUS_SET(pstatus);
	
	const List::ItemList& l = pList->getList();
	if (l.size() % 2 != 0)
		return;
	for (List::ItemList::size_type n = 0; n < l.size(); n += 2) {
		if (l[n]->getType() != ListItem::TYPE_TEXT ||
			l[n + 1]->getType() != ListItem::TYPE_TEXT)
			return;
		
		struct {
			const CHAR* pszName_;
			Status status_;
		} statuses[] = {
			{ "MESSAGES",		STATUS_MESSAGES		},
			{ "RECENT",			STATUS_RECENT		},
			{ "UIDNEXT",		STATUS_UIDNEXT		},
			{ "UIDVALIDITY",	STATUS_UIDVALIDITY	},
			{ "UNSEEN",			STATUS_UNSEEN		}
		};
		Status s = STATUS_UNKNOWN;
		const CHAR* pszStatus = static_cast<ListItemText*>(l[n])->getText();
		for (int m = 0; m < countof(statuses); ++m) {
			if (_stricmp(pszStatus, statuses[m].pszName_) == 0) {
				s = statuses[m].status_;
				break;
			}
		}
		if (s != STATUS_UNKNOWN) {
			const CHAR* psz = static_cast<ListItemText*>(l[n + 1])->getText();
			CHAR* pEnd = 0;
			unsigned long n = strtol(psz, &pEnd, 10);
			if (*pEnd)
				return;
			status = STLWrapper<StatusList>(listStatus_).push_back(std::make_pair(s, n));
			CHECK_QSTATUS_SET(pstatus);
		}
	}
	
	*pstatus = QSTATUS_SUCCESS;
}

qmimap4::ResponseStatus::~ResponseStatus()
{
	if (wstrMailbox_)
		free(wstrMailbox_);
}

const WCHAR* qmimap4::ResponseStatus::getMailbox() const
{
	return wstrMailbox_;
}

const ResponseStatus::StatusList& qmimap4::ResponseStatus::getStatusList() const
{
	return listStatus_;
}


/****************************************************************************
 *
 * FetchData
 *
 */

qmimap4::FetchData::FetchData(Type type) :
	type_(type)
{
}

qmimap4::FetchData::~FetchData()
{
}

FetchData::Type qmimap4::FetchData::getType() const
{
	return type_;
}


/****************************************************************************
 *
 * FetchDataBody
 *
 */

qmimap4::FetchDataBody::FetchDataBody(const CHAR* pszSection,
	qs::STRING strContent, qs::QSTATUS* pstatus) :
	FetchData(TYPE_BODY),
	section_(SECTION_NONE),
	strContent_(0)
{
	assert(pszSection);
	assert(strContent);
	assert(pstatus);
	
	DECLARE_QSTATUS();
	
	*pstatus = QSTATUS_FAIL;
	
	const CHAR* p = pszSection;
	while (*p && isdigit(*p)) {
		CHAR* pEnd = 0;
		long n = strtol(p, &pEnd, 10);
		if (*pEnd && *pEnd != '.')
			return;
		else if (n == 0)
			return;
		status = STLWrapper<PartPath>(partPath_).push_back(n);
		CHECK_QSTATUS_SET(pstatus);
		p = !*pEnd ? pEnd : pEnd + 1;
	}
	
	if (*p) {
		const CHAR* pEnd = strchr(p, ' ');
		if (!pEnd) {
			struct {
				const CHAR* psz_;
				Section section_;
			} sections[] = {
				{ "HEADER",	SECTION_HEADER	},
				{ "TEXT",	SECTION_TEXT	},
				{ "MIME",	SECTION_MIME	}
			};
			for (int n = 0; n < countof(sections); ++n) {
				if (_stricmp(p, sections[n].psz_) == 0) {
					section_ = sections[n].section_;
					break;
				}
			}
			if (n == countof(sections))
				return;
		}
		else {
			if (_strnicmp(p, "HEADER.FIELDS", pEnd - p) == 0)
				section_ = SECTION_HEADER_FIELDS;
			else if (_strnicmp(p, "HEADER.FIELDS.NOT", pEnd - p) == 0)
				section_ = SECTION_HEADER_FIELDS_NOT;
			else
				return;
			
			p = pEnd + 1;
			if (*p != '(')
				return;
			
			Buffer buffer(p, &status);
			CHECK_QSTATUS_SET(pstatus);
			size_t n = 0;
			List* pList = 0;
			status = Parser::parseList(&buffer, &n, 0, &pList);
			CHECK_QSTATUS_SET(pstatus);
			assert(pList);
			std::auto_ptr<List> p(pList);
			const List::ItemList& l = pList->getList();
			List::ItemList::const_iterator it = l.begin();
			while (it != l.end()) {
				if ((*it)->getType() != ListItem::TYPE_TEXT)
					return;
				string_ptr<STRING> str(static_cast<ListItemText*>(*it)->releaseText());
				status = STLWrapper<FieldList>(listField_).push_back(str.get());
				CHECK_QSTATUS_SET(pstatus);
				str.release();
				++it;
			}
			
			if (listField_.empty())
				return;
		}
	}
	if (section_ == SECTION_MIME && partPath_.empty())
		return;
	
	strContent_ = strContent;
	
	*pstatus = QSTATUS_SUCCESS;
}

qmimap4::FetchDataBody::~FetchDataBody()
{
	std::for_each(listField_.begin(), listField_.end(), string_free<STRING>());
	freeString(strContent_);
}

FetchDataBody::Section qmimap4::FetchDataBody::getSection() const
{
	return section_;
}

const FetchDataBody::PartPath& qmimap4::FetchDataBody::getPartPath() const
{
	return partPath_;
}

const FetchDataBody::FieldList& qmimap4::FetchDataBody::getFieldList() const
{
	return listField_;
}

const CHAR* qmimap4::FetchDataBody::getContent() const
{
	return strContent_;
}

STRING qmimap4::FetchDataBody::releaseContent()
{
	STRING strContent = strContent_;
	strContent_ = 0;
	return strContent;
}


/****************************************************************************
 *
 * FetchDataBodyStructure
 *
 */

qmimap4::FetchDataBodyStructure::FetchDataBodyStructure(
	List* pList, bool bExtended, QSTATUS* pstatus) :
	FetchData(TYPE_BODYSTRUCTURE),
	strContentType_(0),
	strContentSubType_(0),
	strId_(0),
	strDescription_(0),
	strEncoding_(0),
	nSize_(0),
	nLine_(0),
	strMd5_(0),
	strDisposition_(0),
	pEnvelope_(0)
{
	assert(pstatus);
	
	DECLARE_QSTATUS();
	
	*pstatus = QSTATUS_FAIL;
	
	const List::ItemList& l = pList->getList();
	bool bMultipart = !l.empty() && l.front()->getType() == ListItem::TYPE_LIST;
	if (bMultipart) {
		strContentType_ = allocString("MULTIPART");
		if (!strContentType_) {
			*pstatus = QSTATUS_OUTOFMEMORY;
			return;
		}
	}
	
	STRING* pstr[] = {
		&strContentType_,
		&strContentSubType_,
		0,
		&strId_,
		&strDescription_,
		&strEncoding_
	};
	unsigned long* pn[] = {
		&nSize_,
		0,
		0,
		&nLine_
	};
	
	int nCount = 0;
	bool bComplete = false;
	for (List::ItemList::const_iterator it = l.begin(); it != l.end(); ++it) {
		if (bMultipart) {
			if (!bExtended && nCount > 0)
				return;
			
			switch (nCount) {
			case 0:
				// Part or SubType
				switch ((*it)->getType()) {
				case ListItem::TYPE_NIL:
					return;
				case ListItem::TYPE_LIST:
					status = parseChild(*it, bExtended);
					CHECK_QSTATUS_SET(pstatus);
					break;
				case ListItem::TYPE_TEXT:
					strContentSubType_ = static_cast<ListItemText*>(*it)->releaseText();
					nCount = 1;
					bComplete = true;
					break;
				default:
					assert(false);
					return;
				}
				break;
			case 2:
				// Parameter
				status = parseParam(*it, &listContentTypeParam_);
				CHECK_QSTATUS_SET(pstatus);
				break;
			case 3:
				// Disposition
				status = parseDisposition(*it);
				CHECK_QSTATUS_SET(pstatus);
				break;
			case 4:
				// Language
				status = parseLanguage(*it);
				CHECK_QSTATUS_SET(pstatus);
				break;
			default:
				break;
			}
			if (nCount > 0)
				++nCount;
		}
		else {
			if (!bExtended && nCount > 9)
				return;
			
			switch (nCount) {
			case 0:
				// Type
			case 1:
				// SubType
			case 3:
				// ID
			case 4:
				// Description
			case 5:
				// Encoding
				switch ((*it)->getType()) {
				case ListItem::TYPE_TEXT:
					*pstr[nCount] = static_cast<ListItemText*>(*it)->releaseText();
					if (*pstr[nCount]) {
						for (CHAR* p = *pstr[nCount]; *p; ++p)
							*p = toupper(*p);
					}
					break;
				case ListItem::TYPE_LIST:
					return;
				case ListItem::TYPE_NIL:
					break;
				default:
					assert(false);
					return;
				}
				break;
			case 2:
				// Parameter
				status = parseParam(*it, &listContentTypeParam_);
				CHECK_QSTATUS_SET(pstatus);
				break;
			case 6:
				// Octet
			case 9:
				// Line
				if ((*it)->getType() != ListItem::TYPE_TEXT) {
					return;
				}
				else {
					CHAR* pEnd = 0;
					*pn[nCount - 6] = strtol(static_cast<ListItemText*>(*it)->getText(),
						&pEnd, 10);
					if (*pEnd)
						return;
				}
				if (nCount == 6) {
					if (strcmp(strContentType_, "MESSAGE") == 0 &&
						strcmp(strContentSubType_, "RFC822") == 0) {
					}
					else if (strcmp(strContentType_, "TEXT") == 0) {
						nCount = 8;
					}
					else {
						nCount = 9;
						bComplete = true;
					}
				}
				else if (nCount == 9) {
					bComplete = true;
				}
				break;
			case 7:
				// Envelope
				if ((*it)->getType() != ListItem::TYPE_LIST)
					return;
				status = newQsObject(static_cast<List*>(*it), &pEnvelope_);
				CHECK_QSTATUS_SET(pstatus);
				break;
			case 8:
				// Body
				status = parseChild(*it, bExtended);
				CHECK_QSTATUS_SET(pstatus);
				break;
			case 10:
				// MD5
				switch ((*it)->getType()) {
				case ListItem::TYPE_TEXT:
					strMd5_ = static_cast<ListItemText*>(*it)->releaseText();
					break;
				case ListItem::TYPE_LIST:
					return;
				case ListItem::TYPE_NIL:
					break;
				default:
					assert(false);
					return;
				}
				break;
			case 11:
				// Disposition
				status = parseDisposition(*it);
				CHECK_QSTATUS_SET(pstatus);
				break;
			case 12:
				// Language
				status = parseLanguage(*it);
				CHECK_QSTATUS_SET(pstatus);
				break;
			default:
				// Unknown extension
				break;
			}
			++nCount;
		}
	}
	
	if (!bComplete)
		return;
	
	*pstatus = QSTATUS_SUCCESS;
}

qmimap4::FetchDataBodyStructure::~FetchDataBodyStructure()
{
	freeString(strContentType_);
	freeString(strContentSubType_);
	std::for_each(listContentTypeParam_.begin(), listContentTypeParam_.end(),
		unary_compose_fx_gx(string_free<STRING>(), string_free<STRING>()));
	freeString(strId_);
	freeString(strDescription_);
	freeString(strEncoding_);
	freeString(strMd5_);
	freeString(strDisposition_);
	std::for_each(listDispositionParam_.begin(), listDispositionParam_.end(),
		unary_compose_fx_gx(string_free<STRING>(), string_free<STRING>()));
	std::for_each(listLanguage_.begin(),
		listLanguage_.end(), string_free<STRING>());
	delete pEnvelope_;
	std::for_each(listChild_.begin(), listChild_.end(),
		deleter<FetchDataBodyStructure>());
}

const CHAR* qmimap4::FetchDataBodyStructure::getContentType() const
{
	return strContentType_;
}

const CHAR* qmimap4::FetchDataBodyStructure::getContentSubType() const
{
	return strContentSubType_;
}

const FetchDataBodyStructure::ParamList& qmimap4::FetchDataBodyStructure::getContentParams() const
{
	return listContentTypeParam_;
}

const CHAR* qmimap4::FetchDataBodyStructure::getId() const
{
	return strId_;
}

const CHAR* qmimap4::FetchDataBodyStructure::getDescription() const
{
	return strDescription_;
}

const CHAR* qmimap4::FetchDataBodyStructure::getEncoding() const
{
	return strEncoding_;
}

unsigned long qmimap4::FetchDataBodyStructure::getSize() const
{
	return nSize_;
}

unsigned long qmimap4::FetchDataBodyStructure::getLine() const
{
	return nLine_;
}

const CHAR* qmimap4::FetchDataBodyStructure::getMd5() const
{
	return strMd5_;
}

const CHAR* qmimap4::FetchDataBodyStructure::getDisposition() const
{
	return strDisposition_;
}

const FetchDataBodyStructure::ParamList& qmimap4::FetchDataBodyStructure::getDispositionParams() const
{
	return listDispositionParam_;
}

const FetchDataBodyStructure::LanguageList& qmimap4::FetchDataBodyStructure::getLanguages() const
{
	return listLanguage_;
}

const FetchDataEnvelope* qmimap4::FetchDataBodyStructure::getEnvelope() const
{
	return pEnvelope_;
}

const FetchDataBodyStructure::ChildList& qmimap4::FetchDataBodyStructure::getChildList() const
{
	return listChild_;
}

QSTATUS qmimap4::FetchDataBodyStructure::parseChild(ListItem* pListItem, bool bExtended)
{
	assert(pListItem);
	
	DECLARE_QSTATUS();
	
	if (pListItem->getType() != ListItem::TYPE_LIST)
		return QSTATUS_FAIL;
	
	std::auto_ptr<FetchDataBodyStructure> pStructure;
	status = newQsObject(static_cast<List*>(pListItem), bExtended, &pStructure);
	CHECK_QSTATUS();
	status = STLWrapper<ChildList>(listChild_).push_back(pStructure.get());
	CHECK_QSTATUS();
	pStructure.release();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::FetchDataBodyStructure::parseParam(
	ListItem* pListItem, ParamList* pListParam)
{
	assert(pListItem);
	assert(pListParam);
	
	DECLARE_QSTATUS();
	
	switch (pListItem->getType()) {
	case ListItem::TYPE_NIL:
		break;
	case ListItem::TYPE_TEXT:
		return QSTATUS_FAIL;
	case ListItem::TYPE_LIST:
		{
			const List::ItemList& l = static_cast<List*>(pListItem)->getList();
			if (l.size() % 2 != 0)
				return QSTATUS_FAIL;
			for (List::ItemList::size_type n = 0; n < l.size(); n += 2) {
				if (l[n]->getType() != ListItem::TYPE_TEXT ||
					l[n + 1]->getType() != ListItem::TYPE_TEXT)
					return QSTATUS_FAIL;
				
				string_ptr<STRING> strName(
					static_cast<ListItemText*>(l[n])->releaseText());
				if (!strName.get())
					return QSTATUS_FAIL;
				string_ptr<STRING> strValue(
					static_cast<ListItemText*>(l[n + 1])->releaseText());
				if (!strValue.get())
					return QSTATUS_FAIL;
				
				status = STLWrapper<ParamList>(*pListParam).push_back(
					std::make_pair(strName.get(), strValue.get()));
				CHECK_QSTATUS();
				strName.release();
				strValue.release();
			}
		}
		break;
	default:
		assert(false);
		return QSTATUS_FAIL;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::FetchDataBodyStructure::parseDisposition(ListItem* pListItem)
{
	assert(pListItem);
	
	DECLARE_QSTATUS();
	
	switch (pListItem->getType()) {
	case ListItem::TYPE_NIL:
		break;
	case ListItem::TYPE_TEXT:
		return QSTATUS_FAIL;
	case ListItem::TYPE_LIST:
		{
			const List::ItemList& l = static_cast<List*>(pListItem)->getList();
			if (l.size() != 2)
				return QSTATUS_FAIL;
			
			if (l.front()->getType() != ListItem::TYPE_TEXT)
				return QSTATUS_FAIL;
			strDisposition_ = static_cast<ListItemText*>(l.front())->releaseText();
			
			status = parseParam(l.back(), &listDispositionParam_);
			CHECK_QSTATUS();
		}
		break;
	default:
		assert(false);
		return QSTATUS_FAIL;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::FetchDataBodyStructure::parseLanguage(ListItem* pListItem)
{
	assert(pListItem);
	
	DECLARE_QSTATUS();
	
	switch (pListItem->getType()) {
	case ListItem::TYPE_NIL:
		break;
	case ListItem::TYPE_TEXT:
		{
			string_ptr<STRING> str(static_cast<ListItemText*>(pListItem)->releaseText());
			if (str.get()) {
				status = STLWrapper<LanguageList>(listLanguage_).push_back(str.get());
				CHECK_QSTATUS();
				str.release();
			}
		}
		break;
	case ListItem::TYPE_LIST:
		{
			const List::ItemList& l = static_cast<List*>(pListItem)->getList();
			for (List::ItemList::const_iterator it = l.begin(); it != l.end(); ++it) {
				if ((*it)->getType() != ListItem::TYPE_TEXT)
					return QSTATUS_FAIL;
				string_ptr<STRING> str(static_cast<ListItemText*>(*it)->releaseText());
				status = STLWrapper<LanguageList>(listLanguage_).push_back(str.get());
				CHECK_QSTATUS();
				str.release();
			}
		}
		break;
	default:
		assert(false);
		return QSTATUS_FAIL;
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * FetchDataEnvelope
 *
 */

qmimap4::FetchDataEnvelope::FetchDataEnvelope(List* pList, QSTATUS* pstatus) :
	FetchData(TYPE_ENVELOPE),
	strDate_(0),
	strSubject_(0),
	strInReplyTo_(0),
	strMessageId_(0)
{
	assert(pstatus);
	
	DECLARE_QSTATUS();
	
	*pstatus = QSTATUS_FAIL;
	
	STRING* pstr[] = {
		&strDate_,
		&strSubject_,
		&strInReplyTo_,
		&strMessageId_
	};
	
	const List::ItemList& l = pList->getList();
	int nItem = 0;
	int nText = 0;
	int nList = 0;
	for (List::ItemList::const_iterator it = l.begin(); it != l.end(); ++it) {
		switch (nItem) {
		case 0:
			// Date
		case 1:
			// Subject
		case 8:
			// In-Reply-To
		case 9:
			// Message-Id
			switch ((*it)->getType()) {
			case ListItem::TYPE_NIL:
				break;
			case ListItem::TYPE_TEXT:
				*pstr[nText] = static_cast<ListItemText*>(*it)->releaseText();
				break;
			case ListItem::TYPE_LIST:
				return;
			default:
				assert(false);
				return;
			}
			++nText;
			break;
		case 2:
			// From
		case 3:
			// Sender
		case 4:
			// Reply-To
		case 5:
			// To
		case 6:
			// Cc
		case 7:
			// Bcc
			switch ((*it)->getType()) {
			case ListItem::TYPE_NIL:
				break;
			case ListItem::TYPE_TEXT:
				return;
			case ListItem::TYPE_LIST:
				{
					const List::ItemList& l = static_cast<List*>(*it)->getList();
					List::ItemList::const_iterator itAddr = l.begin();
					while (itAddr != l.end()) {
						switch ((*itAddr)->getType()) {
						case ListItem::TYPE_NIL:
							break;
						case ListItem::TYPE_TEXT:
							// Because of BUG of iMAIL
//							return;
							break;
						case ListItem::TYPE_LIST:
							{
								std::auto_ptr<EnvelopeAddress> pAddress;
								status = newQsObject(
									static_cast<List*>(*itAddr), &pAddress);
								CHECK_QSTATUS_SET(pstatus);
								status = STLWrapper<AddressList>(
									listAddress_[nList]).push_back(pAddress.get());
								CHECK_QSTATUS_SET(pstatus);
								pAddress.release();
							}
							break;
						default:
							assert(false);
							return;
						}
						++itAddr;
					}
				}
				break;
			default:
				assert(false);
				return;
			}
			++nList;
			break;
		}
		++nItem;
	}
	
	*pstatus = QSTATUS_SUCCESS;
}

qmimap4::FetchDataEnvelope::~FetchDataEnvelope()
{
	for (int n = 0; n < countof(listAddress_); ++n)
		std::for_each(listAddress_[n].begin(), listAddress_[n].end(),
			deleter<EnvelopeAddress>());
	freeString(strDate_);
	freeString(strSubject_);
	freeString(strMessageId_);
	freeString(strInReplyTo_);
}

const FetchDataEnvelope::AddressList& qmimap4::FetchDataEnvelope::getAddresses(
	Address address) const
{
	return listAddress_[address];
}

const CHAR* qmimap4::FetchDataEnvelope::getDate() const
{
	return strDate_;
}

const CHAR* qmimap4::FetchDataEnvelope::getSubject() const
{
	return strSubject_;
}

const CHAR* qmimap4::FetchDataEnvelope::getMessageId() const
{
	return strMessageId_;
}

const CHAR* qmimap4::FetchDataEnvelope::getInReplyTo() const
{
	return strInReplyTo_;
}


/****************************************************************************
 *
 * FetchDataFlags
 *
 */

qmimap4::FetchDataFlags::FetchDataFlags(List* pList, qs::QSTATUS* pstatus) :
	FetchData(TYPE_FLAGS),
	nSystemFlags_(0)
{
	assert(pstatus);
	
	*pstatus = QSTATUS_SUCCESS;
	
	DECLARE_QSTATUS();
	
	const List::ItemList& l = pList->getList();
	for (List::ItemList::const_iterator it = l.begin(); it != l.end(); ++it) {
		if ((*it)->getType() != ListItem::TYPE_TEXT) {
			*pstatus = QSTATUS_FAIL;
			return;
		}
		struct {
			const CHAR* pszName_;
			Imap4::Flag flag_;
		} systemFlags[] = {
			{ "\\ANSWERED",	Imap4::FLAG_ANSWERED	},
			{ "\\FLAGGED",	Imap4::FLAG_FLAGGED		},
			{ "\\DELETED",	Imap4::FLAG_DELETED		},
			{ "\\SEEN",		Imap4::FLAG_SEEN		},
			{ "\\DRAFT",	Imap4::FLAG_DRAFT		},
			{ "\\RECENT",	Imap4::FLAG_RECENT		}
		};
		const CHAR* pszFlag = static_cast<ListItemText*>(*it)->getText();
		for (int n = 0; n < countof(systemFlags); ++n) {
			if (_stricmp(pszFlag, systemFlags[n].pszName_) == 0) {
				nSystemFlags_ |= systemFlags[n].flag_;
				break;
			}
		}
		if (n == countof(systemFlags)) {
			string_ptr<STRING> str(static_cast<ListItemText*>(*it)->releaseText());
			status = STLWrapper<FlagList>(listCustomFlag_).push_back(str.get());
			CHECK_QSTATUS_SET(pstatus);
			str.release();
		}
	}
}

qmimap4::FetchDataFlags::~FetchDataFlags()
{
	std::for_each(listCustomFlag_.begin(), listCustomFlag_.end(),
		string_free<STRING>());
}

unsigned int qmimap4::FetchDataFlags::getSystemFlags() const
{
	return nSystemFlags_;
}

const FetchDataFlags::FlagList& qmimap4::FetchDataFlags::getCustomFlags() const
{
	return listCustomFlag_;
}


/****************************************************************************
 *
 * FetchDataInternalDate
 *
 */

qmimap4::FetchDataInternalDate::FetchDataInternalDate(
	const CHAR* pszDate, qs::QSTATUS* pstatus) :
	FetchData(TYPE_INTERNALDATE)
{
	assert(pstatus);
	
	*pstatus = QSTATUS_FAIL;
	
	if (strlen(pszDate) != 26 ||
		pszDate[2] != '-' ||
		pszDate[6] != '-' ||
		pszDate[11] != ' ' ||
		pszDate[14] != ':' ||
		pszDate[17] != ':' ||
		pszDate[20] != ' ')
		return;
	
	CHAR* pTemp = 0;
	
	if (!isdigit(pszDate[1]))
		return;
	time_.wDay = pszDate[1] - '0';
	if (isdigit(pszDate[0]))
		time_.wDay += (pszDate[0] - '0')*10;
	else if (pszDate[0] != ' ')
		return;
	
	const CHAR* pMonth = pszDate + 3;
	const CHAR* szMonthes[] = {
		"Jan", "Feb", "Mar", "Apr", "May", "Jun",
		"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
	};
	int nMonth = 0;
	for (nMonth = 0; nMonth < countof(szMonthes); ++nMonth) {
		if (strncmp(pMonth, szMonthes[nMonth], 3) == 0)
			break;
	}
	if (nMonth == countof(szMonthes))
		return;
	time_.wMonth = nMonth;
	
	const CHAR* pYear = pszDate + 7;
	time_.wYear = static_cast<WORD>(strtol(pYear, &pTemp, 10));
	if (pTemp != pYear + 4)
		return;
	
	const CHAR* pTime = pszDate + 12;
	WORD* pwTimes[] = { &time_.wHour, &time_.wMinute, &time_.wSecond };
	for (int n = 0; n < countof(pwTimes); ++n) {
		if (!isdigit(pTime[0]) || !isdigit(pTime[1]))
			return;
		*pwTimes[n] = (pTime[0] - '0')*10 + (pTime[1] - '0');
		pTime += 3;
	}
	
	const CHAR* pZone = pszDate + 21;
	if (pZone[0] != '+' && pZone[0] != '-')
		return;
	long nZone = strtol(pZone, &pTemp, 10);
	if (pTemp != pZone + 4)
		return;
	if (pZone[0] == '-')
		nZone = -nZone;
	time_.setTimeZone(nZone);
	
	*pstatus = QSTATUS_SUCCESS;
}

qmimap4::FetchDataInternalDate::~FetchDataInternalDate()
{
}

const Time& qmimap4::FetchDataInternalDate::getTime() const
{
	return time_;
}


/****************************************************************************
 *
 * FetchDataSize
 *
 */

qmimap4::FetchDataSize::FetchDataSize(unsigned long nSize, QSTATUS* pstatus) :
	FetchData(TYPE_SIZE),
	nSize_(nSize)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qmimap4::FetchDataSize::~FetchDataSize()
{
}

unsigned long qmimap4::FetchDataSize::getSize() const
{
	return nSize_;
}


/****************************************************************************
 *
 * FetchDataUid
 *
 */

qmimap4::FetchDataUid::FetchDataUid(unsigned long nUid, QSTATUS* pstatus) :
	FetchData(TYPE_UID),
	nUid_(nUid)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qmimap4::FetchDataUid::~FetchDataUid()
{
}

unsigned long qmimap4::FetchDataUid::getUid() const
{
	return nUid_;
}


/****************************************************************************
 *
 * EnvelopeAddress
 *
 */

qmimap4::EnvelopeAddress::EnvelopeAddress(List* pList, QSTATUS* pstatus) :
	strName_(0),
	strDomain_(0),
	strMailbox_(0),
	strHost_(0)
{
	*pstatus = QSTATUS_FAIL;
	
	STRING* pstr[] = { &strName_, &strDomain_, &strMailbox_, &strHost_ };
	
	const List::ItemList& l = pList->getList();
	if (l.size() != 4)
		return;
	for (List::ItemList::size_type n = 0; n < l.size(); ++n) {
		if (l[n]->getType() == ListItem::TYPE_NIL)
			continue;
		else if (l[n]->getType() != ListItem::TYPE_TEXT)
			return;
		*pstr[n] = static_cast<ListItemText*>(l[n])->releaseText();
	}
	
	*pstatus = QSTATUS_SUCCESS;
}

qmimap4::EnvelopeAddress::~EnvelopeAddress()
{
	freeString(strName_);
	freeString(strDomain_);
	freeString(strMailbox_);
	freeString(strHost_);
}

const CHAR* qmimap4::EnvelopeAddress::getName() const
{
	return strName_;
}

const CHAR* qmimap4::EnvelopeAddress::getDomain() const
{
	return strDomain_;
}

const CHAR* qmimap4::EnvelopeAddress::getMailbox() const
{
	return strMailbox_;
}

const CHAR* qmimap4::EnvelopeAddress::getHost() const
{
	return strHost_;
}


/****************************************************************************
 *
 * ListItem
 *
 */

qmimap4::ListItem::ListItem(Type type) :
	type_(type)
{
}

qmimap4::ListItem::~ListItem()
{
}

ListItem::Type qmimap4::ListItem::getType() const
{
	return type_;
}


/****************************************************************************
 *
 * ListItemNil
 *
 */

qmimap4::ListItemNil::ListItemNil(QSTATUS* pstatus) :
	ListItem(TYPE_NIL)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qmimap4::ListItemNil::~ListItemNil()
{
}


/****************************************************************************
 *
 * ListItemText
 *
 */

qmimap4::ListItemText::ListItemText(qs::STRING str, QSTATUS* pstatus) :
	ListItem(TYPE_TEXT),
	str_(str)
{
	assert(str_);
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qmimap4::ListItemText::~ListItemText()
{
	freeString(str_);
}

const CHAR* qmimap4::ListItemText::getText() const
{
	assert(str_);
	return str_;
}

STRING qmimap4::ListItemText::releaseText()
{
	assert(str_);
	
	STRING str = str_;
	str_ = 0;
	return str;
}


/****************************************************************************
 *
 * List
 *
 */

qmimap4::List::List(QSTATUS* pstatus) :
	ListItem(TYPE_LIST)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qmimap4::List::~List()
{
	std::for_each(list_.begin(), list_.end(), deleter<ListItem>());
}

const List::ItemList& qmimap4::List::getList() const
{
	return list_;
}

QSTATUS qmimap4::List::add(ListItem* pItem)
{
	return STLWrapper<ItemList>(list_).push_back(pItem);
}


/****************************************************************************
 *
 * State
 *
 */

qmimap4::State::State(Code code, QSTATUS* pstatus) :
	code_(code),
	strMessage_(0),
	n_(0),
	pList_(0)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qmimap4::State::~State()
{
	freeString(strMessage_);
	delete pList_;
}

State::Code qmimap4::State::getCode() const
{
	return code_;
}

const CHAR* qmimap4::State::getMessage() const
{
	return strMessage_;
}

unsigned long qmimap4::State::getArgNumber() const
{
	return n_;
}

const List* qmimap4::State::getArgList() const
{
	return pList_;
}

void qmimap4::State::setMessage(qs::STRING str)
{
	strMessage_ = str;
}

void qmimap4::State::setArg(unsigned long n)
{
	n_ = n;
}

void qmimap4::State::setArg(List* pList)
{
	pList_ = pList;
}
