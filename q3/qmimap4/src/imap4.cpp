/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsconv.h>
#include <qsencoder.h>
#include <qsmd5.h>
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
	virtual bool response(std::auto_ptr<Response> pResponse);

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

bool qmimap4::ListParserCallback::response(std::auto_ptr<Response> pResponse)
{
	listResponse_.push_back(pResponse.get());
	pResponse.release();
	return true;
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
	virtual bool response(std::auto_ptr<Response> pResponse);

private:
	Imap4ParserCallback(const Imap4ParserCallback&);
	Imap4ParserCallback& operator=(const Imap4ParserCallback&);

private:
	Imap4Callback* pCallback_;
	std::auto_ptr<Response> pLastResponse_;
};

qmimap4::Imap4ParserCallback::Imap4ParserCallback(Imap4Callback* pCallback) :
	pCallback_(pCallback)
{
}

qmimap4::Imap4ParserCallback::~Imap4ParserCallback()
{
}

ResponseState::Flag qmimap4::Imap4ParserCallback::getResponse() const
{
	if (!pLastResponse_.get() || pLastResponse_->getType() != Response::TYPE_STATE)
		return ResponseState::FLAG_UNKNOWN;
	else
		return static_cast<ResponseState*>(pLastResponse_.get())->getFlag();
}

bool qmimap4::Imap4ParserCallback::response(std::auto_ptr<Response> pResponse)
{
	assert(pResponse.get());
	pLastResponse_ = pResponse;
	return pCallback_->response(pLastResponse_.get());
}


/****************************************************************************
 *
 * Imap4
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

qmimap4::Imap4::Imap4(long nTimeout,
					  qs::SocketCallback* pSocketCallback,
					  qs::SSLSocketCallback* pSSLSocketCallback,
					  Imap4Callback* pImap4Callback,
					  qs::Logger* pLogger) :
	nTimeout_(nTimeout),
	pSocketCallback_(pSocketCallback),
	pSSLSocketCallback_(pSSLSocketCallback),
	pImap4Callback_(pImap4Callback),
	pLogger_(pLogger),
	nCapability_(0),
	nAuth_(AUTH_LOGIN),
	bDisconnected_(false),
	nTag_(0),
	nError_(IMAP4_ERROR_SUCCESS),
	utf7Converter_(true)
{
}

qmimap4::Imap4::~Imap4()
{
}

bool qmimap4::Imap4::connect(const WCHAR* pwszHost,
							 short nPort,
							 Ssl ssl)
{
	assert(pwszHost);
	
	std::auto_ptr<Socket> pSocket(new Socket(
		nTimeout_, pSocketCallback_, pLogger_));
	
	if (!pSocket->connect(pwszHost, nPort))
		IMAP4_ERROR(IMAP4_ERROR_CONNECT | pSocket->getLastError());
	
	if (ssl == SSL_SSL) {
		SSLSocketFactory* pFactory = SSLSocketFactory::getFactory();
		if (!pFactory)
			IMAP4_ERROR(IMAP4_ERROR_SSL);
		
		std::auto_ptr<SSLSocket> pSSLSocket = pFactory->createSSLSocket(
			pSocket.get(), true, pSSLSocketCallback_, pLogger_);
		if (!pSSLSocket.get())
			IMAP4_ERROR(IMAP4_ERROR_SSL);
		
		pSocket.release();
		pSocket_ = pSSLSocket;
	}
	else {
		pSocket_ = pSocket;
	}
	
	if (!processGreeting())
		return false;
	
	if (!processCapability())
		return false;
	
	if (ssl == SSL_STARTTLS) {
		if ((nCapability_ & CAPABILITY_STARTTLS) == 0)
			IMAP4_ERROR(IMAP4_ERROR_STARTTLS);
		
		ListParserCallback callback;
		if (!sendCommand("STARTTLS\r\n", &callback))
			IMAP4_ERROR_OR(IMAP4_ERROR_STARTTLS);
		const ListParserCallback::ResponseList& l = callback.getResponseList();
		if (l.size() != 1)
			IMAP4_ERROR(IMAP4_ERROR_STARTTLS | IMAP4_ERROR_PARSE);
		ResponseState::Flag flag = callback.getResponse();
		if (flag != ResponseState::FLAG_OK)
			IMAP4_ERROR(IMAP4_ERROR_STARTTLS | IMAP4_ERROR_PARSE);
		
		SSLSocketFactory* pFactory = SSLSocketFactory::getFactory();
		if (!pFactory)
			IMAP4_ERROR(IMAP4_ERROR_SSL);
		
		std::auto_ptr<SSLSocket> pSSLSocket(pFactory->createSSLSocket(
			static_cast<Socket*>(pSocket_.get()), true, pSSLSocketCallback_, pLogger_));
		if (!pSSLSocket.get())
			IMAP4_ERROR(IMAP4_ERROR_SSL);
		pSocket_.release();
		pSocket_ = pSSLSocket;
		
		if (!processCapability())
			return false;
	}
	
	if (!processLogin())
		return false;
	
	return true;
}

void qmimap4::Imap4::disconnect()
{
	if (pSocket_.get() && !bDisconnected_) {
		const CommandToken tokens[] = {
			{ "LOGOUT\r\n",	0,	0,	false,	true	}
		};
		
		pSocket_->setTimeout(1);
		
		if (!sendCommandTokens(tokens, countof(tokens)))
			nError_ |= IMAP4_ERROR_LOGOUT;
	}
	pSocket_.reset(0);
}

bool qmimap4::Imap4::checkConnection()
{
	if (!pSocket_.get())
		return false;
	
	if (bDisconnected_)
		return false;
	
	int nSelect = pSocket_->select(Socket::SELECT_READ, 0);
	if (nSelect == -1)
		IMAP4_ERROR_SOCKET(IMAP4_ERROR_SELECTSOCKET);
	else if (nSelect == 0)
		return true;
	
	char c = 0;
	if (pSocket_->recv(&c, 1, MSG_PEEK) != 1)
		return false;
	
	Imap4ParserCallback callback(pImap4Callback_);
	if (!receive("", false, &callback))
		return false;
	
	return !bDisconnected_;
}

bool qmimap4::Imap4::select(const WCHAR* pwszFolderName)
{
	assert(pwszFolderName);
	
	const CommandToken tokens[] = {
		{ "SELECT ",	0,				0,					false,	true	},
		{ 0,			pwszFolderName,	&utf7Converter_,	true,	true	},
		{ "\r\n",		0,				0,					false,	true	}
	};
	
	if (!sendCommandTokens(tokens, countof(tokens)))
		IMAP4_ERROR_OR(IMAP4_ERROR_SELECT);
	
	return true;
}

bool qmimap4::Imap4::close()
{
	const CommandToken tokens[] = {
		{ "CLOSE\r\n",	0,	0,	false,	true	}
	};
	
	if (!sendCommandTokens(tokens, countof(tokens)))
		IMAP4_ERROR_OR(IMAP4_ERROR_CLOSE);
	
	return true;
}

bool qmimap4::Imap4::noop()
{
	const CommandToken tokens[] = {
		{ "NOOP\r\n",	0,	0,	false,	true	}
	};
	
	if (!sendCommandTokens(tokens, countof(tokens)))
		IMAP4_ERROR_OR(IMAP4_ERROR_NOOP);
	
	return true;
}

bool qmimap4::Imap4::fetch(const Range& range,
						   const CHAR* pszFetch)
{
	assert(pszFetch);
	
	const CommandToken tokens[] = {
		{ "UID FETCH ",		0,	0,	false,	range.isUid()	},
		{ "FETCH ",			0,	0,	false,	!range.isUid()	},
		{ range.getRange(),	0,	0,	false,	true			},
		{ " ",				0,	0,	false,	true			},
		{ pszFetch,			0,	0,	false,	true			},
		{ "\r\n",			0,	0,	false,	true			}
	};
	
	if (!sendCommandTokens(tokens, countof(tokens)))
		IMAP4_ERROR_OR(IMAP4_ERROR_FETCH);
	
	return true;
}

bool qmimap4::Imap4::store(const Range& range,
						   const CHAR* pszStore)
{
	assert(pszStore);
	
	CommandToken tokens[] = {
		{ "UID STORE ",		0,	0,	false,	range.isUid()	},
		{ "STORE ",			0,	0,	false,	!range.isUid()	},
		{ range.getRange(),	0,	0,	false,	true			},
		{ " ",				0,	0,	false,	true			},
		{ pszStore,			0,	0,	false,	true			},
		{ "\r\n",			0,	0,	false,	true			}
	};
	
	if (!sendCommandTokens(tokens, countof(tokens)))
		IMAP4_ERROR_OR(IMAP4_ERROR_STORE);
	
	return true;
}

bool qmimap4::Imap4::copy(const Range& range,
						  const WCHAR* pwszFolderName)
{
	assert(pwszFolderName);
	
	CommandToken tokens[] = {
		{ "UID COPY ",		0,				0,					false,	range.isUid()	},
		{ "COPY ",			0,				0,					false,	!range.isUid()	},
		{ range.getRange(),	0,				0,					false,	true			},
		{ " ",				0,				0,					false,	true			},
		{ 0,				pwszFolderName, &utf7Converter_,	true,	true			},
		{ "\r\n",			0,				0,					false,	true			}
	};
	
	if (!sendCommandTokens(tokens, countof(tokens)))
		IMAP4_ERROR_OR(IMAP4_ERROR_COPY);
	
	return true;
}

bool qmimap4::Imap4::search(const WCHAR* pwszSearch,
							const WCHAR* pwszCharset,
							bool bUseCharset,
							bool bUid)
{
	assert(pwszSearch);
	assert(pwszCharset);
	
	string_ptr strSearch(encodeSearchString(pwszSearch, pwszCharset));
	if (!strSearch.get())
		return false;
	
	CommandToken tokens[] = {
		{ "UID SEARCH ",	0,				0,	false,	bUid		},
		{ "SEARCH ",		0,				0,	false,	!bUid		},
		{ "CHARSET ",		0,				0,	false,	bUseCharset	},
		{ 0,				pwszCharset,	0,	false,	bUseCharset	},
		{ " ",				0,				0,	false,	bUseCharset	},
		{ strSearch.get(),	0,				0,	false,	true		},
		{ "\r\n",			0,				0,	false,	true		}
	};
	
	if (!sendCommandTokens(tokens, countof(tokens)))
		IMAP4_ERROR_OR(IMAP4_ERROR_SEARCH);
	
	return true;
}

bool qmimap4::Imap4::expunge()
{
	const CommandToken tokens[] = {
		{ "EXPUNGE\r\n",	0,	0,	false,	true	}
	};
	
	if (!sendCommandTokens(tokens, countof(tokens)))
		IMAP4_ERROR_OR(IMAP4_ERROR_EXPUNGE);
	
	return true;
}

bool qmimap4::Imap4::append(const WCHAR* pwszFolderName,
							const CHAR* pszMessage,
							const Flags& flags)
{
	assert(pwszFolderName);
	assert(pszMessage);
	
	size_t nLen = wcslen(pwszFolderName);
	xstring_size_ptr strFolderName(utf7Converter_.encode(pwszFolderName, &nLen));
	if (!strFolderName.get())
		IMAP4_ERROR(IMAP4_ERROR_OTHER | IMAP4_ERROR_APPEND);
	string_ptr strQuotedFolderName(getQuotedString(strFolderName.get()));
	
	string_ptr strFlags(flags.getString());
	
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
	string_ptr strCommand(concat(c, countof(c)));
	
	ListParserCallback listCallback;
	string_ptr strTag;
	if (!sendCommand(strCommand.get(), &strTag, &listCallback))
		IMAP4_ERROR_OR(IMAP4_ERROR_APPEND);
	const ListParserCallback::ResponseList& l = listCallback.getResponseList();
	if (l.empty() || l.back()->getType() != Response::TYPE_CONTINUE)
		IMAP4_ERROR(IMAP4_ERROR_RESPONSE | IMAP4_ERROR_APPEND);
	for (ListParserCallback::ResponseList::size_type n = 0; n < l.size() - 1; ++n) {
		if (!pImap4Callback_->response(l[n]))
			return false;
	}
	
	const CHAR* pszContents[] = {
		pszMessage,
		"\r\n"
	};
	
	Imap4ParserCallback callback(pImap4Callback_);
	if (!send(pszContents, countof(pszContents), strTag.get(), false, &callback))
		IMAP4_ERROR_OR(IMAP4_ERROR_APPEND);
	if (callback.getResponse() != ResponseState::FLAG_OK)
		IMAP4_ERROR(IMAP4_ERROR_RESPONSE | IMAP4_ERROR_APPEND);
	
	return true;
}

bool qmimap4::Imap4::list(bool bSubscribeOnly,
						  const WCHAR* pwszRef,
						  const WCHAR* pwszMailbox)
{
	assert(pwszRef);
	assert(pwszMailbox);
	
	unsigned int nErrorHighLevel = bSubscribeOnly ?
		IMAP4_ERROR_LSUB : IMAP4_ERROR_LIST;
	
	CommandToken tokens[] = {
		{ "LIST ",	0,				0,					false,	!bSubscribeOnly	},
		{ "LSUB ",	0,				0,					false,	bSubscribeOnly	},
		{ 0,		pwszRef,		&utf7Converter_,	true,	true			},
		{ " ",		0,				0,					false,	true			},
		{ 0,		pwszMailbox,	&utf7Converter_,	true,	true			},
		{ "\r\n",	0,				0,					false,	true			}
	};
	
	if (!sendCommandTokens(tokens, countof(tokens)))
		IMAP4_ERROR_OR(nErrorHighLevel);
	
	return true;
}

bool qmimap4::Imap4::create(const WCHAR* pwszFolderName)
{
	assert(pwszFolderName);
	
	CommandToken tokens[] = {
		{ "CREATE ",	0,				0,					false,	true	},
		{ 0,			pwszFolderName,	&utf7Converter_,	true,	true	},
		{ "\r\n",		0,				0,					false,	true	}
	};
	
	if (!sendCommandTokens(tokens, countof(tokens)))
		IMAP4_ERROR_OR(IMAP4_ERROR_CREATE);
	
	return true;
}

bool qmimap4::Imap4::remove(const WCHAR* pwszFolderName)
{
	assert(pwszFolderName);
	
	CommandToken tokens[] = {
		{ "DELETE ",	0,				0,					false,	true	},
		{ 0,			pwszFolderName,	&utf7Converter_,	true,	true	},
		{ "\r\n",		0,				0,					false,	true	}
	};
	
	if (!sendCommandTokens(tokens, countof(tokens)))
		IMAP4_ERROR_OR(IMAP4_ERROR_DELETE);
	
	return true;
}

bool qmimap4::Imap4::rename(const WCHAR* pwszOldFolderName,
							const WCHAR* pwszNewFolderName)
{
	assert(pwszOldFolderName);
	assert(pwszNewFolderName);
	
	CommandToken tokens[] = {
		{ "RENAME ",	0,				0,						false,	true	},
		{ 0,			pwszOldFolderName,	&utf7Converter_,	true,	true	},
		{ " ",			0,				0,						false,	true	},
		{ 0,			pwszNewFolderName,	&utf7Converter_,	true,	true	},
		{ "\r\n",		0,				0,						false,	true	}
	};
	
	if (!sendCommandTokens(tokens, countof(tokens)))
		IMAP4_ERROR_OR(IMAP4_ERROR_RENAME);
	
	return true;
}

bool qmimap4::Imap4::subscribe(const WCHAR* pwszFolderName)
{
	assert(pwszFolderName);
	
	CommandToken tokens[] = {
		{ "SUBSCRIBE ",	0,				0,					false,	true	},
		{ 0,			pwszFolderName,	&utf7Converter_,	true,	true	},
		{ "\r\n",		0,				0,					false,	true	}
	};
	
	if (!sendCommandTokens(tokens, countof(tokens)))
		IMAP4_ERROR_OR(IMAP4_ERROR_SUBSCRIBE);
	
	return true;
}

bool qmimap4::Imap4::unsubscribe(const WCHAR* pwszFolderName)
{
	assert(pwszFolderName);
	
	CommandToken tokens[] = {
		{ "UNSUBSCRIBE ",	0,				0,					false,	true	},
		{ 0,				pwszFolderName,	&utf7Converter_,	true,	true	},
		{ "\r\n",			0,				0,					false,	true	}
	};
	
	if (!sendCommandTokens(tokens, countof(tokens)))
		IMAP4_ERROR_OR(IMAP4_ERROR_UNSUBSCRIBE);
	
	return true;
}

bool qmimap4::Imap4::namespaceList()
{
	const CommandToken tokens[] = {
		{ "NAMESPACE\r\n",	0,	0,	false,	true	}
	};
	
	if (!sendCommandTokens(tokens, countof(tokens)))
		IMAP4_ERROR_OR(IMAP4_ERROR_NAMESPACE);
	
	return true;
}

bool qmimap4::Imap4::getFlags(const Range& range)
{
	return fetch(range, "(FLAGS)");
}

bool qmimap4::Imap4::setFlags(const Range& range,
							  const Flags& flags,
							  const Flags& mask)
{
	string_ptr strAdded(flags.getAdded(mask));
	string_ptr strRemoved(flags.getRemoved(mask));
	
	if (strRemoved.get()) {
		string_ptr strStore(concat("-FLAGS ", strRemoved.get()));
		if (!store(range, strStore.get()))
			return false;
	}
	
	if (strAdded.get()) {
		string_ptr strStore(concat("+FLAGS ", strAdded.get()));
		if (!store(range, strStore.get()))
			return false;
	}
	
	return true;
}

bool qmimap4::Imap4::getMessageData(const Range& range,
									bool bClientParse,
									bool bBody,
									const CHAR* pszFields)
{
	StringBuffer<STRING> buf("(");
	
	if (!bClientParse)
		buf.append("ENVELOPE ");
	if (bBody)
		buf.append("BODYSTRUCTURE ");
	buf.append("BODY.PEEK[HEADER.FIELDS (");
	if (bClientParse)
		buf.append("From To Cc Bcc Sender Message-Id In-Reply-To Subject Date ");
	if (!bBody)
		buf.append("Content-Type Content-Disposition ");
	buf.append("References X-ML-Name");
	if (pszFields && *pszFields) {
		buf.append(" ");
		buf.append(pszFields);
	}
	buf.append(")] FLAGS UID RFC822.SIZE)");
	
	return fetch(range, buf.getCharArray());
}

bool qmimap4::Imap4::getMessage(const Range& range,
								bool bPeek)
{
	if (bPeek)
		return fetch(range, "BODY.PEEK[]");
	else
		return fetch(range, "BODY[]");
}

bool qmimap4::Imap4::getHeader(const Range& range,
							   bool bPeek)
{
	if (bPeek)
		return fetch(range, "BODY.PEEK[HEADER]");
	else
		return fetch(range, "BODY[HEADER]");
}

bool qmimap4::Imap4::getBodyStructure(const Range& range)
{
	return fetch(range, "BODYSTRUCTURE");
}

bool qmimap4::Imap4::getPart(const Range& range,
							 const PartPath& path)
{
	const CHAR* pszPath = path.getPath();
	const Concat c[] = {
		{ "(BODY.PEEK[",	-1 },
		{ pszPath,			-1 },
		{ "] BODY.PEEK[",	-1 },
		{ pszPath,			-1 },
		{ ".MIME])"			-1 }
	};
	string_ptr strFetch(concat(c, countof(c)));
	return fetch(range, strFetch.get());
}

bool qmimap4::Imap4::getPartMime(const Range& range,
								 const PartPath& path)
{
	const CHAR* pszPath = path.getPath();
	const Concat c[] = {
		{ "BODY.PEEK[",	-1 },
		{ pszPath,		-1 },
		{ ".MIME]"		-1 }
	};
	string_ptr strFetch(concat(c, countof(c)));
	return fetch(range, strFetch.get());
}

bool qmimap4::Imap4::getPartBody(const Range& range,
								 const PartPath& path)
{
	const CHAR* pszPath = path.getPath();
	const Concat c[] = {
		{ "BODY.PEEK[",	-1 },
		{ pszPath,		-1 },
		{ "]",			-1 }
	};
	string_ptr strFetch(concat(c, countof(c)));
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

bool qmimap4::Imap4::processGreeting()
{
	ListParserCallback callback;
	if (!receive("", false, &callback))
		IMAP4_ERROR_OR(IMAP4_ERROR_GREETING);
	
	const ListParserCallback::ResponseList& l = callback.getResponseList();
	if (l.size() != 1)
		IMAP4_ERROR(IMAP4_ERROR_GREETING | IMAP4_ERROR_PARSE);
	ResponseState::Flag flag = callback.getResponse();
	if (flag != ResponseState::FLAG_OK &&
		flag != ResponseState::FLAG_PREAUTH)
		IMAP4_ERROR(IMAP4_ERROR_GREETING | IMAP4_ERROR_PARSE);
	
	return true;
}

bool qmimap4::Imap4::processCapability()
{
	nCapability_ = 0;
	
	ListParserCallback callback;
	if (!sendCommand("CAPABILITY\r\n", &callback))
		IMAP4_ERROR_OR(IMAP4_ERROR_CAPABILITY);
	if (callback.getResponse() != ResponseState::FLAG_OK)
		IMAP4_ERROR(IMAP4_ERROR_CAPABILITY | IMAP4_ERROR_PARSE);
	
	const ListParserCallback::ResponseList& l = callback.getResponseList();
	ListParserCallback::ResponseList::const_iterator it = std::find_if(
		l.begin(), l.end(),
		std::bind2nd(binary_compose_f_gx_hy(std::equal_to<Response::Type>(),
			std::mem_fun(&Response::getType), std::identity<Response::Type>()),
			Response::TYPE_CAPABILITY));
	if (it == l.end())
		IMAP4_ERROR(IMAP4_ERROR_CAPABILITY | IMAP4_ERROR_PARSE);
	
	ResponseCapability* pCapability = static_cast<ResponseCapability*>(*it);
	if (!pCapability->isSupport("IMAP4REV1"))
		IMAP4_ERROR(IMAP4_ERROR_CAPABILITY | IMAP4_ERROR_PARSE);
	if (pCapability->isSupport("NAMESPACE"))
		nCapability_ |= CAPABILITY_NAMESPACE;
	if (pCapability->isSupport("STARTTLS"))
		nCapability_ |= CAPABILITY_STARTTLS;
	if (pCapability->isSupportAuth("CRAM-MD5"))
		nAuth_ |= AUTH_CRAMMD5;
	
	return true;
}

bool qmimap4::Imap4::processLogin()
{
	wstring_ptr wstrUserName;
	wstring_ptr wstrPassword;
	if (!pImap4Callback_->getUserInfo(&wstrUserName, &wstrPassword))
		IMAP4_ERROR(IMAP4_ERROR_LOGIN | IMAP4_ERROR_OTHER);
	string_ptr strUserName(wcs2mbs(wstrUserName.get()));
	string_ptr strPassword(wcs2mbs(wstrPassword.get()));
	
	pImap4Callback_->authenticating();
	
	unsigned int nAllowedMethods = getAuthMethods();
	unsigned int nAuth = nAuth_ & nAllowedMethods;
	if (nAuth == 0)
		IMAP4_ERROR(IMAP4_ERROR_AUTHENTICATE);
	
	bool bLogin = true;
	if (nAuth & AUTH_CRAMMD5) {
		ListParserCallback callback;
		string_ptr strTag;
		if (!sendCommand("AUTHENTICATE CRAM-MD5\r\n", &strTag, &callback))
			IMAP4_ERROR_OR(IMAP4_ERROR_AUTHENTICATE);
		
		const ListParserCallback::ResponseList& l = callback.getResponseList();
		if (l.size() == 1 && l.back()->getType() == Response::TYPE_CONTINUE) {
			const State* pState = static_cast<const ResponseContinue*>(l.back())->getState();
			if (pState) {
				const CHAR* pszText = pState->getMessage();
				if (pszText) {
					Base64Encoder encoder(false);
					malloc_size_ptr<unsigned char> pChallenge(encoder.decode(
						reinterpret_cast<const unsigned char*>(pszText), strlen(pszText)));
					if (!pChallenge.get())
						IMAP4_ERROR(IMAP4_ERROR_AUTHENTICATE | IMAP4_ERROR_OTHER);
					
					CHAR szDigestString[128] = "";
					MD5::hmacToString(pChallenge.get(), pChallenge.size(),
						reinterpret_cast<unsigned char*>(strPassword.get()),
						strlen(strPassword.get()), szDigestString);
					
					string_ptr strKey(concat(strUserName.get(), " ", szDigestString));
					malloc_size_ptr<unsigned char> pKey(encoder.encode(
						reinterpret_cast<unsigned char*>(strKey.get()), strlen(strKey.get())));
					if (!pKey.get())
						IMAP4_ERROR(IMAP4_ERROR_AUTHENTICATE | IMAP4_ERROR_OTHER);
					
					StringBuffer<STRING> bufSend(
						reinterpret_cast<CHAR*>(pKey.get()), pKey.size());
					bufSend.append("\r\n");
					callback.clear();
					if (!send(bufSend.getCharArray(), strTag.get(), false, &callback))
						IMAP4_ERROR_OR(IMAP4_ERROR_AUTHENTICATE);
					bLogin = callback.getResponse() != ResponseState::FLAG_OK;
				}
			}
		}
	}
	
	if (bLogin) {
		string_ptr strQuotedUserName(getQuotedString(strUserName.get()));
		string_ptr strQuotedPassword(getQuotedString(strPassword.get()));
		const Concat c[] = {
			{ "LOGIN ",					-1 },
			{ strQuotedUserName.get(),	-1 },
			{ " ",						-1 },
			{ strQuotedPassword.get(),	-1 },
			{ "\r\n",					-1 }
		};
		string_ptr strSend(concat(c, countof(c)));
		
		ListParserCallback callback;
		if (!sendCommand(strSend.get(), &callback))
			IMAP4_ERROR_OR(IMAP4_ERROR_LOGIN);
		if (callback.getResponse() != ResponseState::FLAG_OK)
			IMAP4_ERROR(IMAP4_ERROR_RESPONSE | IMAP4_ERROR_LOGIN);
	}
	pImap4Callback_->setPassword(wstrPassword.get());
	
	nError_ = IMAP4_ERROR_SUCCESS;
	
	return true;
}

bool qmimap4::Imap4::receive(const CHAR* pszTag,
							 bool bAcceptContinue,
							 ParserCallback* pCallback)
{
	assert(pCallback);
	
	if (!pSocket_.get())
		IMAP4_ERROR(IMAP4_ERROR_INVALIDSOCKET);
	
	Buffer buf(strOverBuf_.get(), pSocket_.get());
	strOverBuf_.reset(0);
	
	struct ParserCallbackImpl : public ParserCallback
	{
		ParserCallbackImpl(ParserCallback* pCallback) :
			pCallback_(pCallback),
			bDisconnected_(false)
		{
		}
		
		virtual bool response(std::auto_ptr<Response> pResponse)
		{
			if (pResponse->getType() == Response::TYPE_STATE &&
				static_cast<ResponseState*>(pResponse.get())->getFlag() == ResponseState::FLAG_BYE)
				bDisconnected_ = true;
			return pCallback_->response(pResponse);
		}
		
		ParserCallback* pCallback_;
		bool bDisconnected_;
	} callback(pCallback);
	
	Parser parser(&buf, pImap4Callback_);
	if (!parser.parse(pszTag, bAcceptContinue, &callback)) {
		bDisconnected_ = true;
		// TODO
		// Log
		
		nError_ = buf.getError();
		if (nError_ == IMAP4_ERROR_SUCCESS)
			nError_ = IMAP4_ERROR_PARSE;
		
		return false;
	}
	bDisconnected_ = callback.bDisconnected_;
	// TODO
	// Log
	strOverBuf_ = parser.getUnprocessedString();
	nError_ = IMAP4_ERROR_SUCCESS;
	
	return true;
}

bool qmimap4::Imap4::sendCommand(const CHAR* pszCommand,
								 ParserCallback* pCallback)
{
	return sendCommand(pszCommand, 0, pCallback);
}

bool qmimap4::Imap4::sendCommand(const CHAR* pszCommand,
								 string_ptr* pstrTag,
								 ParserCallback* pCallback)
{
	assert(pszCommand);
	assert(pCallback);
	
	if (!pSocket_.get())
		IMAP4_ERROR(IMAP4_ERROR_INVALIDSOCKET);
	
	string_ptr strTag(getTag());
	string_ptr strSendTag(allocString(strTag.get()));
	
	const CHAR* pszContents[] = {
		strSendTag.get(),
		" ",
		pszCommand
	};
	
	const CHAR* pszTag = strTag.get();
	bool bAcceptContinue = false;
	if (pstrTag) {
		*pstrTag = strTag;
		bAcceptContinue = true;
	}
	
	return send(pszContents, countof(pszContents), pszTag, bAcceptContinue, pCallback);
}

bool qmimap4::Imap4::send(const CHAR* pszContent,
						  const CHAR* pszTag,
						  bool bAcceptContinue,
						  ParserCallback* pCallback)
{
	return send(&pszContent, 1, pszTag, bAcceptContinue, pCallback);
}

bool qmimap4::Imap4::send(const CHAR** pszContents,
						  size_t nCount,
						  const CHAR* pszTag,
						  bool bAcceptContinue,
						  ParserCallback* pCallback)
{
	assert(pszContents);
	assert(pCallback);
	
	if (!pSocket_.get())
		IMAP4_ERROR(IMAP4_ERROR_INVALIDSOCKET);
	
	for (size_t n = 0; n < nCount; ++n) {
		const CHAR* pszContent = *(pszContents + n);
		size_t nLen = strlen(pszContent);
		
		size_t nTotalSend = 0;
		while (nTotalSend < nLen) {
			int nSelect = pSocket_->select(Socket::SELECT_READ | Socket::SELECT_WRITE);
			if (nSelect == -1)
				IMAP4_ERROR_SOCKET(IMAP4_ERROR_SELECTSOCKET);
			else if (nSelect == 0)
				IMAP4_ERROR(IMAP4_ERROR_TIMEOUT);
			
			size_t nSend = pSocket_->send(pszContent + nTotalSend, nLen - nTotalSend, 0);
			if (nSend == -1)
				IMAP4_ERROR_SOCKET(IMAP4_ERROR_SEND);
			nTotalSend += nSend;
		}
	}
	
	// TODO
	// Log
	
	return receive(pszTag, bAcceptContinue, pCallback);
}

bool qmimap4::Imap4::sendCommandTokens(const CommandToken* pTokens,
									   size_t nCount)
{
	assert(pTokens);
	
	StringBuffer<STRING> buf;
	
	for (size_t n = 0; n < nCount; ++n) {
		const CommandToken* pToken = pTokens + n;
		if (pToken->b_) {
			const CHAR* psz = 0;
			string_ptr str;
			
			if (pToken->psz_) {
				psz = pToken->psz_;
			}
			else {
				if (pToken->pConverter_) {
					size_t nLen = wcslen(pToken->pwsz_);
					xstring_size_ptr strEncoded(pToken->pConverter_->encode(
						pToken->pwsz_, &nLen));
					if (!strEncoded.get())
						IMAP4_ERROR(IMAP4_ERROR_OTHER);
					str = allocString(strEncoded.get());
				}
				else {
					str = wcs2mbs(pToken->pwsz_);
				}
				psz = str.get();
			}
			assert(psz);
			
			string_ptr strQuoted;
			if (pToken->bQuote_) {
				strQuoted = getQuotedString(psz);
				psz = strQuoted.get();
			}
			
			buf.append(psz);
		}
	}
	
	Imap4ParserCallback callback(pImap4Callback_);
	if (!sendCommand(buf.getCharArray(), &callback))
		return false;
	if (callback.getResponse() != ResponseState::FLAG_OK)
		IMAP4_ERROR(IMAP4_ERROR_RESPONSE);
	
	return true;
}

string_ptr qmimap4::Imap4::getTag()
{
	string_ptr strTag(allocString(32));
	sprintf(strTag.get(), "q%04lu", nTag_++);
	return strTag;
}

unsigned int qmimap4::Imap4::getAuthMethods()
{
	wstring_ptr wstrAuthMethods(pImap4Callback_->getAuthMethods());
	
	struct {
		const WCHAR* pwsz_;
		Auth auth_;
	} methods[] = {
		{ L"LOGIN",		AUTH_LOGIN		},
		{ L"CRAM-MD5",	AUTH_CRAMMD5	}
	};
	
	unsigned int nAuth = 0;
	
	const WCHAR* p = wcstok(wstrAuthMethods.get(), L" ");
	while (p) {
		for (int n = 0; n < countof(methods); ++n) {
			if (wcscmp(p, methods[n].pwsz_) == 0) {
				nAuth |= methods[n].auth_;
				break;
			}
		}
		p = wcstok(0, L" ");
	}
	
	return nAuth != 0 ? nAuth : 0xffffffff;
}

string_ptr qmimap4::Imap4::getQuotedString(const CHAR* psz)
{
	assert(psz);
	
	StringBuffer<STRING> buf(strlen(psz) + 1);
	
	buf.append('\"');
	
	while (*psz) {
		if (*psz == '\"' || *psz == '\\')
			buf.append('\\');
		buf.append(*psz);
		++psz;
	}
	
	buf.append('\"');
	
	return buf.getString();
}

string_ptr qmimap4::Imap4::encodeSearchString(const WCHAR* pwsz,
											  const WCHAR* pwszCharset)
{
	assert(pwsz);
	assert(pwszCharset);
	
	std::auto_ptr<Converter> pConverter(ConverterFactory::getInstance(pwszCharset));
	if (!pConverter.get())
		pConverter = ConverterFactory::getInstance(L"utf-8");
	assert(pConverter.get());
	
	StringBuffer<STRING> buf;
	
	const WCHAR* pBegin = pwsz;
	for (const WCHAR* p = pwsz; ; ++p) {
		if (*p == L'\"' || *p == L'\\' || *p == L'\0') {
			if (pBegin != p) {
				size_t nLen = p - pBegin;
				xstring_size_ptr str(pConverter->encode(pBegin, &nLen));
				if (!str.get())
					return 0;
				
				for (CHAR* p = str.get(); *p; ++p) {
					if (*p == '\"' || *p == '\\')
						buf.append('\\');
					buf.append(*p);
				}
			}
			if (!*p)
				break;
			buf.append(static_cast<CHAR>(*p));
			pBegin = p + 1;
		}
	}
	
	return buf.getString();
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
}

bool qmimap4::DefaultRange::isUid() const
{
	return bUid_;
}

const CHAR* qmimap4::DefaultRange::getRange() const
{
	assert(strRange_.get());
	return strRange_.get();
}

void qmimap4::DefaultRange::setRange(const CHAR* pszRange)
{
	assert(!strRange_.get());
	assert(pszRange);
	
	strRange_ = allocString(pszRange);
}


/****************************************************************************
 *
 * SingleRange
 *
 */

qmimap4::SingleRange::SingleRange(unsigned long n,
								  bool bUid) :
	DefaultRange(bUid)
{
	assert(n != 0);
	
	CHAR szRange[128];
	sprintf(szRange, "%lu", n);
	setRange(szRange);
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
										  unsigned long nEnd,
										  bool bUid) :
	DefaultRange(bUid)
{
	assert(nBegin != 0 && nBegin != static_cast<unsigned long>(-1));
	assert(nEnd == static_cast<unsigned long>(-1) || nBegin <= nEnd);
	
	CHAR szRange[128];
	if (nEnd == static_cast<unsigned long>(-1))
		sprintf(szRange, "%lu:*", nBegin);
	else if (nBegin == nEnd)
		sprintf(szRange, "%lu", nBegin);
	else
		sprintf(szRange, "%lu:%lu", nBegin, nEnd);
	setRange(szRange);
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
									  size_t nCount,
									  bool bUid) :
	DefaultRange(bUid)
{
	assert(pn);
	assert(nCount != 0);
	
	StringBuffer<STRING> buf;
	
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
			
			if (buf.getLength() != 0)
				buf.append(",");
			buf.append(sz);
			
			nBegin = nId;
			nEnd = nId;
		}
	}
	
	setRange(buf.getCharArray());
}

qmimap4::MultipleRange::~MultipleRange()
{
}


/****************************************************************************
 *
 * TextRange
 *
 */

qmimap4::TextRange::TextRange(const CHAR* pszRange,
							  bool bUid) :
	DefaultRange(bUid)
{
	setRange(pszRange);
}

qmimap4::TextRange::~TextRange()
{
}


/****************************************************************************
 *
 * Flags
 *
 */

qmimap4::Flags::Flags(unsigned int nSystemFlags) :
	nSystemFlags_(0)
{
	init(nSystemFlags, 0, 0);
}

qmimap4::Flags::Flags(unsigned int nSystemFlags,
					  const CHAR** pszUserFlags,
					  size_t nCount) :
	nSystemFlags_(0)
{
	init(nSystemFlags, pszUserFlags, nCount);
}

qmimap4::Flags::~Flags()
{
	std::for_each(listFlag_.begin(), listFlag_.end(), string_free<STRING>());
}

string_ptr qmimap4::Flags::getString() const
{
	StringBuffer<STRING> buf;
	
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
				buf.append("(");
			else
				buf.append(" ");
			buf.append(getFlagString(flags[n]));
		}
	}
	
	for (FlagList::const_iterator it = listFlag_.begin(); it != listFlag_.end(); ++it) {
		if (buf.getLength() == 0)
			buf.append("(");
		else
			buf.append(" ");
		buf.append(*it);
	}
	
	if (buf.getLength() != 0)
		buf.append(")");
	
	return buf.getString();
}

string_ptr qmimap4::Flags::getAdded(const Flags& mask) const
{
	return getString(mask, true);
}

string_ptr qmimap4::Flags::getRemoved(const Flags& mask) const
{
	return getString(mask, false);
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

void qmimap4::Flags::init(unsigned int nSystemFlags,
						  const CHAR** pszUserFlags,
						  size_t nCount)
{
	nSystemFlags_ = nSystemFlags;
	
	listFlag_.reserve(nCount);
	
	for (size_t n = 0; n < nCount; ++n) {
		string_ptr strFlag = allocString(*(pszUserFlags + n));
		listFlag_.push_back(strFlag.release());
	}
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

string_ptr qmimap4::Flags::getString(const Flags& mask,
									 bool bAdd) const
{
	const Imap4::Flag flags[] = {
		Imap4::FLAG_ANSWERED,
		Imap4::FLAG_FLAGGED,
		Imap4::FLAG_DELETED,
		Imap4::FLAG_SEEN,
		Imap4::FLAG_DRAFT
	};
	
	StringBuffer<STRING> buf;
	
	for (int n = 0; n < countof(flags); ++n) {
		if ((mask.nSystemFlags_ & flags[n])) {
			bool b = contains(flags[n]);
			if ((bAdd && b) || (!bAdd && !b)) {
				if (buf.getLength() == 0)
					buf.append("(");
				else
					buf.append(" ");
				buf.append(getFlagString(flags[n]));
			}
		}
	}
	
	for (FlagList::const_iterator it = mask.listFlag_.begin(); it != mask.listFlag_.end(); ++it) {
		bool b = contains(*it);
		if ((bAdd && b) || (!bAdd && !b)) {
			if (buf.getLength() == 0)
				buf.append("(");
			else
				buf.append(" ");
			buf.append(*it);
		}
	}
	
	if (buf.getLength() == 0)
		return 0;
	
	buf.append(")");
	return buf.getString();
}


/****************************************************************************
 *
 * PartPath
 *
 */

qmimap4::PartPath::PartPath(const unsigned int* pnPart,
							size_t nCount)
{
	assert(pnPart);
	assert(nCount != 0);
	
	StringBuffer<STRING> buf;
	
	CHAR sz[32];
	for (size_t n = 0; n < nCount; ++n) {
		sprintf(sz, "%u", *(pnPart + n));
		if (buf.getLength() != 0)
			buf.append(".");
		buf.append(sz);
	}
	
	strPath_ = buf.getString();
}

qmimap4::PartPath::~PartPath()
{
}

const CHAR* qmimap4::PartPath::getPath() const
{
	return strPath_.get();
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

qmimap4::ResponseCapability::ResponseCapability() :
	Response(TYPE_CAPABILITY)
{
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

void qmimap4::ResponseCapability::add(const CHAR* psz)
{
	assert(psz);
	
	if (_strnicmp(psz, "AUTH=", 5) == 0) {
		string_ptr strAuth(allocString(psz + 5));
		listAuth_.push_back(strAuth.get());
		strAuth.release();
	}
	else {
		string_ptr str(allocString(psz));
		listCapability_.push_back(str.get());
		str.release();
	}
}


/****************************************************************************
 *
 * ResponseContinue
 *
 */

qmimap4::ResponseContinue::ResponseContinue(std::auto_ptr<State> pState) :
	Response(TYPE_CONTINUE),
	pState_(pState)
{
}

qmimap4::ResponseContinue::~ResponseContinue()
{
}

const State* qmimap4::ResponseContinue::getState() const
{
	return pState_.get();
}


/****************************************************************************
 *
 * ResponseExists
 *
 */

qmimap4::ResponseExists::ResponseExists(unsigned long nExists) :
	Response(TYPE_EXISTS),
	nExists_(nExists)
{
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

qmimap4::ResponseExpunge::ResponseExpunge(unsigned long nExpunge) :
	Response(TYPE_EXPUNGE),
	nExpunge_(nExpunge)
{
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

qmimap4::ResponseFetch::ResponseFetch(unsigned long nNumber,
									  FetchDataList& listData) :
	Response(TYPE_FETCH),
	nNumber_(nNumber)
{
	listData_.swap(listData);
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

std::auto_ptr<ResponseFetch> qmimap4::ResponseFetch::create(unsigned long nNumber,
															List* pList)
{
	FetchDataList listData;
	struct Deleter
	{
		Deleter(FetchDataList& listData) :
			listData_(listData)
		{
		}
		
		~Deleter()
		{
			std::for_each(listData_.begin(), listData_.end(), deleter<FetchData>());
		}
		
		FetchDataList& listData_;
	} deleter(listData);
	
	const List::ItemList& l = pList->getList();
	if (l.size() % 2 != 0)
		return 0;
	listData.reserve(l.size()/2);
	
	for (List::ItemList::size_type n = 0; n < l.size(); n += 2) {
		if (l[n]->getType() != ListItem::TYPE_TEXT)
			return 0;
		
		string_ptr strName(static_cast<ListItemText*>(l[n])->releaseText());
		if (_stricmp(strName.get(), "ENVELOPE") == 0) {
			// ENVELOPE
			if (l[n + 1]->getType() != ListItem::TYPE_LIST)
				return 0;
			
			std::auto_ptr<FetchDataEnvelope> pEnvelope(
				FetchDataEnvelope::create(static_cast<List*>(l[n + 1])));
			if (!pEnvelope.get())
				return 0;
			listData.push_back(pEnvelope.release());
		}
		else if (_stricmp(strName.get(), "FLAGS") == 0) {
			// FLAGS
			if (l[n + 1]->getType() != ListItem::TYPE_LIST)
				return 0;
			
			std::auto_ptr<FetchDataFlags> pFlags(
				FetchDataFlags::create(static_cast<List*>(l[n + 1])));
			if (!pFlags.get())
				return 0;
			listData.push_back(pFlags.release());
		}
		else if (_stricmp(strName.get(), "INTERNALDATE") == 0) {
			// INTERNALDATE
			if (l[n + 1]->getType() != ListItem::TYPE_TEXT)
				return 0;
			
			std::auto_ptr<FetchDataInternalDate> pDate(
				FetchDataInternalDate::create(static_cast<ListItemText*>(l[n + 1])->getText()));
			if (!pDate.get())
				return 0;
			listData.push_back(pDate.release());
		}
		else if (_stricmp(strName.get(), "RFC822") == 0 ||
			_stricmp(strName.get(), "RFC822.HEADER") == 0 ||
			_stricmp(strName.get(), "RFC822.TEXT") == 0) {
			// RFC822
			if (l[n + 1]->getType() != ListItem::TYPE_TEXT)
				return 0;
			
			const CHAR* pszSection = strName.get()[6] == '\0' ? 0 : strName.get() + 7;
			std::auto_ptr<FetchDataBody> pBody(FetchDataBody::create(
				pszSection, static_cast<ListItemText*>(l[n + 1])->releaseText()));
			if (!pBody.get())
				return 0;
			listData.push_back(pBody.release());
		}
		else if (_stricmp(strName.get(), "RFC822.SIZE") == 0) {
			// SIZE
			if (l[n + 1]->getType() != ListItem::TYPE_TEXT)
				return 0;
			
			const CHAR* pszSize = static_cast<ListItemText*>(l[n + 1])->getText();
			CHAR* pEnd = 0;
			long nSize = strtol(pszSize, &pEnd, 10);
			if (*pEnd)
				return 0;
			
			std::auto_ptr<FetchDataSize> pSize(new FetchDataSize(nSize));
			listData.push_back(pSize.release());
		}
		else if (_stricmp(strName.get(), "BODY") == 0 ||
			_stricmp(strName.get(), "BODYSTRUCTURE") == 0) {
			// BODY, BODYSTRUCTURE
			if (l[n + 1]->getType() != ListItem::TYPE_LIST)
				return 0;
			
			bool bExtended = strName.get()[4] != '\0';
			std::auto_ptr<FetchDataBodyStructure> pStructure(
				FetchDataBodyStructure::create(static_cast<List*>(l[n + 1]), bExtended));
			if (!pStructure.get())
				return 0;
			listData.push_back(pStructure.release());
		}
		else if (_stricmp(strName.get(), "UID") == 0) {
			// UID
			if (l[n + 1]->getType() != ListItem::TYPE_TEXT)
				return 0;
			
			const CHAR* pszUid = static_cast<ListItemText*>(l[n + 1])->getText();
			CHAR* pEnd = 0;
			long nUid = strtol(pszUid, &pEnd, 10);
			if (*pEnd)
				return 0;
			
			std::auto_ptr<FetchDataUid> pUid(new FetchDataUid(nUid));
			listData.push_back(pUid.release());
		}
		else if (_strnicmp(strName.get(), "BODY[", 5) == 0) {
			// BODY[SECTION]
			if (l[n + 1]->getType() != ListItem::TYPE_TEXT)
				return 0;
			
			const CHAR* p = strName.get() + 5;
			CHAR* pEnd = strchr(p, ']');
			if (!pEnd)
				return 0;
			*pEnd = '\0';
			
			std::auto_ptr<FetchDataBody> pBody(FetchDataBody::create(
				p, static_cast<ListItemText*>(l[n + 1])->releaseText()));
			if (!pBody.get())
				return 0;
			listData.push_back(pBody.release());
		}
		else {
			return 0;
		}
	}
	
	return new ResponseFetch(nNumber, listData);
}


/****************************************************************************
 *
 * ResponseFlags
 *
 */

qmimap4::ResponseFlags::ResponseFlags(unsigned int nSystemFlags,
									  FlagList& listCustomFlag) :
	Response(TYPE_FLAGS),
	nSystemFlags_(nSystemFlags)
{
	listCustomFlag_.swap(listCustomFlag);
}

qmimap4::ResponseFlags::~ResponseFlags()
{
	std::for_each(listCustomFlag_.begin(),
		listCustomFlag_.end(), string_free<STRING>());
}

unsigned int qmimap4::ResponseFlags::getSystemFlags() const
{
	return nSystemFlags_;
}

const ResponseFlags::FlagList& qmimap4::ResponseFlags::getCustomFlags() const
{
	return listCustomFlag_;
}

std::auto_ptr<ResponseFlags> qmimap4::ResponseFlags::create(List* pList)
{
	unsigned int nSystemFlags = 0;
	FlagList listCustomFlag;
	
	struct Deleter
	{
		Deleter(FlagList& listCustomFlag) :
			listCustomFlag_(listCustomFlag)
		{
		}
		
		~Deleter()
		{
			std::for_each(listCustomFlag_.begin(),
				listCustomFlag_.end(), string_free<STRING>());
		}
		
		FlagList& listCustomFlag_;
	} deleter(listCustomFlag);
	
	const List::ItemList& l = pList->getList();
	for (List::ItemList::const_iterator it = l.begin(); it != l.end(); ++it) {
		if ((*it)->getType() != ListItem::TYPE_TEXT)
			return 0;
		
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
		int n = 0;
		while (n < countof(flags)) {
			if (_stricmp(pszFlag, flags[n].pszName_) == 0) {
				nSystemFlags |= flags[n].flag_;
				break;
			}
			++n;
		}
		if (n == countof(flags)) {
			string_ptr str(static_cast<ListItemText*>(*it)->releaseText());
			listCustomFlag.push_back(str.get());
			str.release();
		}
	}
	
	return new ResponseFlags(nSystemFlags, listCustomFlag);
}


/****************************************************************************
 *
 * ResponseList
 *
 */

qmimap4::ResponseList::ResponseList(bool bList,
									unsigned int nAttributes,
									WCHAR cSeparator,
									const WCHAR* pwszMailbox) :
	Response(TYPE_LIST),
	bList_(bList),
	nAttributes_(nAttributes),
	cSeparator_(cSeparator)
{
	wstrMailbox_ = allocWString(pwszMailbox);
}

qmimap4::ResponseList::~ResponseList()
{
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
	return wstrMailbox_.get();
}

std::auto_ptr<ResponseList> qmimap4::ResponseList::create(bool bList,
														  List* pListAttribute,
														  CHAR cSeparator,
														  const CHAR* pszMailbox)
{
	UTF7Converter converter(true);
	size_t nLen = strlen(pszMailbox);
	wxstring_size_ptr wstrMailbox(converter.decode(pszMailbox, &nLen));
	if (!wstrMailbox.get())
		return 0;
	
	unsigned int nAttributes = 0;
	const List::ItemList& l = pListAttribute->getList();
	for (List::ItemList::const_iterator it = l.begin(); it != l.end(); ++it) {
		if ((*it)->getType() != ListItem::TYPE_TEXT)
			return 0;
		
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
				nAttributes |= attributes[n].attribute_;
				break;
			}
		}
	}
	
	return new ResponseList(bList, nAttributes, cSeparator, wstrMailbox.get());
}


/****************************************************************************
 *
 * ResponseNamespace
 *
 */

qmimap4::ResponseNamespace::ResponseNamespace(NamespaceList& listPersonal,
											  NamespaceList& listOthers,
											  NamespaceList& listShared) :
	Response(TYPE_NAMESPACE)
{
	listPersonal_.swap(listPersonal);
	listOthers_.swap(listOthers);
	listShared_.swap(listShared);
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

std::auto_ptr<ResponseNamespace> qmimap4::ResponseNamespace::create(List* pListPersonal,
																	List* pListOthers,
																	List* pListShared)
{
	UTF7Converter converter(true);
	
	NamespaceList listPersonal;
	NamespaceList listOthers;
	NamespaceList listShared;
	
	struct Deleter
	{
		Deleter(NamespaceList& listPersonal,
				NamespaceList& listOthers,
				NamespaceList& listShared)
		{
			p_[0] = &listPersonal;
			p_[1] = &listOthers;
			p_[2] = &listShared;
		}
		
		~Deleter()
		{
			for (int n = 0; n < countof(p_); ++n)
				std::for_each(p_[n]->begin(), p_[n]->end(),
					unary_compose_f_gx(
						string_free<WSTRING>(),
						std::select1st<ResponseNamespace::NamespaceList::value_type>()));
		}
		
		NamespaceList* p_[3];
	} deleter(listPersonal, listOthers, listShared);
	
	struct {
		List* pList_;
		NamespaceList* pnsl_;
	} namespaces[] = {
		{ pListPersonal,	&listPersonal	},
		{ pListOthers,		&listOthers		},
		{ pListShared,		&listShared		}
	};
	for (int n = 0; n < countof(namespaces); ++n) {
		if (!namespaces[n].pList_)
			continue;
		
		NamespaceList& listNamespace = *namespaces[n].pnsl_;
		const List::ItemList& l = namespaces[n].pList_->getList();
		for (List::ItemList::const_iterator it = l.begin(); it != l.end(); ++it) {
			if ((*it)->getType() != ListItem::TYPE_LIST)
				return 0;
			
			const List::ItemList& nss = static_cast<List*>(*it)->getList();
			if (nss.size() != 2 ||
				nss.front()->getType() != ListItem::TYPE_TEXT ||
				(nss.back()->getType() != ListItem::TYPE_TEXT &&
					nss.back()->getType() != ListItem::TYPE_NIL))
				return 0;
			
			const CHAR* pszName = static_cast<ListItemText*>(nss.front())->getText();
			size_t nLen = strlen(pszName);
			wxstring_size_ptr wstrName(converter.decode(pszName, &nLen));
			if (!wstrName.get())
				return 0;
			
			const CHAR* pszSeparator = "";
			if (nss.back()->getType() == ListItem::TYPE_TEXT)
				pszSeparator = static_cast<ListItemText*>(nss.back())->getText();
			WCHAR cSep = *pszSeparator;
			
			wstring_ptr wstr(allocWString(wstrName.get()));
			listNamespace.push_back(std::make_pair(wstr.get(), cSep));
			wstr.release();
		}
	}
	
	return new ResponseNamespace(listPersonal, listOthers, listShared);
}


/****************************************************************************
 *
 * ResponseRecent
 *
 */

qmimap4::ResponseRecent::ResponseRecent(unsigned long nRecent) :
	Response(TYPE_RECENT),
	nRecent_(nRecent)
{
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

qmimap4::ResponseSearch::ResponseSearch() :
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

void qmimap4::ResponseSearch::add(unsigned long n)
{
	listResult_.push_back(n);
}


/****************************************************************************
 *
 * ResponseState
 *
 */

qmimap4::ResponseState::ResponseState(Flag flag) :
	Response(TYPE_STATE),
	flag_(flag)
{
}

qmimap4::ResponseState::~ResponseState()
{
}

ResponseState::Flag qmimap4::ResponseState::getFlag() const
{
	return flag_;
}

State* qmimap4::ResponseState::getState() const
{
	return pState_.get();
}

void qmimap4::ResponseState::setState(std::auto_ptr<State> pState)
{
	pState_ = pState;
}


/****************************************************************************
 *
 * ResponseStatus
 *
 */

qmimap4::ResponseStatus::ResponseStatus(const WCHAR* pwszMailbox,
										StatusList& listStatus) :
	Response(TYPE_STATUS)
{
	wstrMailbox_ = allocWString(pwszMailbox);
	listStatus_.swap(listStatus);
}

qmimap4::ResponseStatus::~ResponseStatus()
{
}

const WCHAR* qmimap4::ResponseStatus::getMailbox() const
{
	return wstrMailbox_.get();
}

const ResponseStatus::StatusList& qmimap4::ResponseStatus::getStatusList() const
{
	return listStatus_;
}

std::auto_ptr<ResponseStatus> qmimap4::ResponseStatus::create(const CHAR* pszMailbox,
															  List* pList)
{
	size_t nLen = strlen(pszMailbox);
	wxstring_size_ptr wstrMailbox(UTF7Converter(true).decode(pszMailbox, &nLen));
	if (!wstrMailbox.get())
		return 0;
	
	StatusList listStatus;
	const List::ItemList& l = pList->getList();
	if (l.size() % 2 != 0)
		return 0;
	for (List::ItemList::size_type n = 0; n < l.size(); n += 2) {
		if (l[n]->getType() != ListItem::TYPE_TEXT ||
			l[n + 1]->getType() != ListItem::TYPE_TEXT)
			return 0;
		
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
				return 0;
			listStatus.push_back(std::make_pair(s, n));
		}
	}
	
	return new ResponseStatus(wstrMailbox.get(), listStatus);
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

qmimap4::FetchDataBody::FetchDataBody(Section section,
									  PartPath& partPath,
									  FieldList& listField,
									  qs::string_ptr strContent) :
	FetchData(TYPE_BODY),
	section_(section),
	strContent_(strContent)
{
	partPath_.swap(partPath);
	listField_.swap(listField);
}

qmimap4::FetchDataBody::~FetchDataBody()
{
	std::for_each(listField_.begin(), listField_.end(), string_free<STRING>());
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
	return strContent_.get();
}

string_ptr qmimap4::FetchDataBody::releaseContent()
{
	string_ptr strContent = strContent_;
	strContent_.reset(0);
	return strContent;
}

std::auto_ptr<FetchDataBody> qmimap4::FetchDataBody::create(const CHAR* pszSection,
															qs::string_ptr strContent)
{
	assert(pszSection);
	assert(strContent.get());
	
	Section section = SECTION_NONE;
	PartPath partPath;
	FieldList listField;
	
	struct Deleter
	{
		Deleter(FieldList& listField) :
			listField_(listField)
		{
		}
		
		~Deleter()
		{
			std::for_each(listField_.begin(), listField_.end(), string_free<STRING>());
		}
		
		FieldList& listField_;
	} deleter(listField);
	
	const CHAR* p = pszSection;
	while (*p && isdigit(*p)) {
		CHAR* pEnd = 0;
		long n = strtol(p, &pEnd, 10);
		if (*pEnd && *pEnd != '.')
			return 0;
		else if (n == 0)
			return 0;
		partPath.push_back(n);
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
			int n = 0;
			while (n < countof(sections)) {
				if (_stricmp(p, sections[n].psz_) == 0) {
					section = sections[n].section_;
					break;
				}
				++n;
			}
			if (n == countof(sections))
				return 0;
		}
		else {
			if (_strnicmp(p, "HEADER.FIELDS", pEnd - p) == 0)
				section = SECTION_HEADER_FIELDS;
			else if (_strnicmp(p, "HEADER.FIELDS.NOT", pEnd - p) == 0)
				section = SECTION_HEADER_FIELDS_NOT;
			else
				return 0;
			
			p = pEnd + 1;
			if (*p != '(')
				return 0;
			
			Buffer buffer(p);
			size_t n = 0;
			std::auto_ptr<List> pList(Parser::parseList(&buffer, &n, 0));
			if (!pList.get())
				return 0;
			const List::ItemList& l = pList->getList();
			for (List::ItemList::const_iterator it = l.begin(); it != l.end(); ++it) {
				if ((*it)->getType() != ListItem::TYPE_TEXT)
					return 0;
				string_ptr str(static_cast<ListItemText*>(*it)->releaseText());
				listField.push_back(str.get());
				str.release();
			}
			
			if (listField.empty())
				return 0;
		}
	}
	if (section == SECTION_MIME && partPath.empty())
		return 0;
	
	return new FetchDataBody(section, partPath, listField, strContent);
}


/****************************************************************************
 *
 * FetchDataBodyStructure
 *
 */

qmimap4::FetchDataBodyStructure::FetchDataBodyStructure(string_ptr strContentType,
														string_ptr strContentSubType,
														ParamList& listContentTypeParam,
														string_ptr strId,
														string_ptr strDescription,
														string_ptr strEncoding,
														unsigned long nSize,
														unsigned long nLine,
														string_ptr strMd5,
														string_ptr strDisposition,
														ParamList& listDispositionParam,
														LanguageList& listLanguage,
														std::auto_ptr<FetchDataEnvelope> pEnvelope,
														ChildList& listChild) :
	FetchData(TYPE_BODYSTRUCTURE),
	strContentType_(strContentType),
	strContentSubType_(strContentSubType),
	strId_(strId),
	strDescription_(strDescription),
	strEncoding_(strEncoding),
	nSize_(nSize),
	nLine_(nLine),
	strMd5_(strMd5),
	strDisposition_(strDisposition),
	pEnvelope_(pEnvelope)
{
	listContentTypeParam_.swap(listContentTypeParam);
	listDispositionParam_.swap(listDispositionParam);
	listLanguage_.swap(listLanguage);
	listChild_.swap(listChild);
}

qmimap4::FetchDataBodyStructure::~FetchDataBodyStructure()
{
	std::for_each(listContentTypeParam_.begin(), listContentTypeParam_.end(),
		unary_compose_fx_gx(string_free<STRING>(), string_free<STRING>()));
	std::for_each(listDispositionParam_.begin(), listDispositionParam_.end(),
		unary_compose_fx_gx(string_free<STRING>(), string_free<STRING>()));
	std::for_each(listLanguage_.begin(),
		listLanguage_.end(), string_free<STRING>());
	std::for_each(listChild_.begin(), listChild_.end(),
		deleter<FetchDataBodyStructure>());
}

const CHAR* qmimap4::FetchDataBodyStructure::getContentType() const
{
	return strContentType_.get();
}

const CHAR* qmimap4::FetchDataBodyStructure::getContentSubType() const
{
	return strContentSubType_.get();
}

const FetchDataBodyStructure::ParamList& qmimap4::FetchDataBodyStructure::getContentParams() const
{
	return listContentTypeParam_;
}

const CHAR* qmimap4::FetchDataBodyStructure::getId() const
{
	return strId_.get();
}

const CHAR* qmimap4::FetchDataBodyStructure::getDescription() const
{
	return strDescription_.get();
}

const CHAR* qmimap4::FetchDataBodyStructure::getEncoding() const
{
	return strEncoding_.get();
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
	return strMd5_.get();
}

const CHAR* qmimap4::FetchDataBodyStructure::getDisposition() const
{
	return strDisposition_.get();
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
	return pEnvelope_.get();
}

const FetchDataBodyStructure::ChildList& qmimap4::FetchDataBodyStructure::getChildList() const
{
	return listChild_;
}

std::auto_ptr<FetchDataBodyStructure> qmimap4::FetchDataBodyStructure::create(List* pList,
																			  bool bExtended)
{
	const List::ItemList& l = pList->getList();
	
	string_ptr strContentType;
	string_ptr strContentSubType;
	string_ptr strId;
	string_ptr strDescription;
	string_ptr strEncoding;
	string_ptr strMd5;
	string_ptr strDisposition;
	string_ptr* pstr[] = {
		&strContentType,
		&strContentSubType,
		0,
		&strId,
		&strDescription,
		&strEncoding
	};
	unsigned long nSize = 0;
	unsigned long nLine = 0;
	unsigned long* pn[] = {
		&nSize,
		0,
		0,
		&nLine
	};
	ParamList listContentTypeParam;
	ParamList listDispositionParam;
	LanguageList listLanguage;
	std::auto_ptr<FetchDataEnvelope> pEnvelope;
	ChildList listChild;
	
	struct Deleter
	{
		Deleter(ParamList& listContentTypeParam,
				ParamList& listDispositionParam,
				LanguageList& listLanguage,
				ChildList& listChild) :
			listContentTypeParam_(listContentTypeParam),
			listDispositionParam_(listDispositionParam),
			listLanguage_(listLanguage),
			listChild_(listChild)
		{
		}
		
		~Deleter()
		{
			std::for_each(listContentTypeParam_.begin(), listContentTypeParam_.end(),
				unary_compose_fx_gx(string_free<STRING>(), string_free<STRING>()));
			std::for_each(listDispositionParam_.begin(), listDispositionParam_.end(),
				unary_compose_fx_gx(string_free<STRING>(), string_free<STRING>()));
			std::for_each(listLanguage_.begin(),
				listLanguage_.end(), string_free<STRING>());
			std::for_each(listChild_.begin(), listChild_.end(),
				deleter<FetchDataBodyStructure>());
		}
		
		ParamList& listContentTypeParam_;
		ParamList& listDispositionParam_;
		LanguageList& listLanguage_;
		ChildList& listChild_;
	} deleter(listContentTypeParam, listDispositionParam, listLanguage, listChild);
		
	bool bMultipart = !l.empty() && l.front()->getType() == ListItem::TYPE_LIST;
	if (bMultipart)
		strContentType = allocString("MULTIPART");
	
	int nCount = 0;
	bool bComplete = false;
	for (List::ItemList::const_iterator it = l.begin(); it != l.end(); ++it) {
		if (bMultipart) {
			if (!bExtended && nCount > 0)
				return 0;
			
			switch (nCount) {
			case 0:
				// Part or SubType
				switch ((*it)->getType()) {
				case ListItem::TYPE_NIL:
					return 0;
				case ListItem::TYPE_LIST:
					{
						std::auto_ptr<FetchDataBodyStructure> pChild(parseChild(*it, bExtended));
						if (!pChild.get())
							return 0;
						listChild.push_back(pChild.get());
						pChild.release();
					}
					break;
				case ListItem::TYPE_TEXT:
					strContentSubType = static_cast<ListItemText*>(*it)->releaseText();
					nCount = 1;
					bComplete = true;
					break;
				default:
					assert(false);
					return 0;
				}
				break;
			case 2:
				// Parameter
				if (!parseParam(*it, &listContentTypeParam))
					return 0;
				break;
			case 3:
				// Disposition
				if (!parseDisposition(*it, &strDisposition, &listDispositionParam))
					return 0;
				break;
			case 4:
				// Language
				if (!parseLanguage(*it, &listLanguage))
					return 0;
				break;
			default:
				break;
			}
			if (nCount > 0)
				++nCount;
		}
		else {
			if (!bExtended && nCount > 9)
				return 0;
			
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
					if (pstr[nCount]->get()) {
						for (CHAR* p = pstr[nCount]->get(); *p; ++p)
							*p = toupper(*p);
					}
					break;
				case ListItem::TYPE_LIST:
					return 0;
				case ListItem::TYPE_NIL:
					break;
				default:
					assert(false);
					return 0;
				}
				break;
			case 2:
				// Parameter
				if (!parseParam(*it, &listContentTypeParam))
					return 0;
				break;
			case 6:
				// Octet
			case 9:
				// Line
				if ((*it)->getType() != ListItem::TYPE_TEXT) {
					return 0;
				}
				else {
					CHAR* pEnd = 0;
					*pn[nCount - 6] = strtol(static_cast<ListItemText*>(*it)->getText(), &pEnd, 10);
					if (*pEnd)
						return 0;
				}
				if (nCount == 6) {
					if (strcmp(strContentType.get(), "MESSAGE") == 0 &&
						strcmp(strContentSubType.get(), "RFC822") == 0) {
					}
					else if (strcmp(strContentType.get(), "TEXT") == 0) {
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
					return 0;
				pEnvelope = FetchDataEnvelope::create(static_cast<List*>(*it));
				if (!pEnvelope.get())
					return 0;
				break;
			case 8:
				// Body
				{
					std::auto_ptr<FetchDataBodyStructure> pChild(parseChild(*it, bExtended));
					if (!pChild.get())
						return 0;
					listChild.push_back(pChild.get());
					pChild.release();
				}
				break;
			case 10:
				// MD5
				switch ((*it)->getType()) {
				case ListItem::TYPE_TEXT:
					strMd5 = static_cast<ListItemText*>(*it)->releaseText();
					break;
				case ListItem::TYPE_LIST:
					return 0;
				case ListItem::TYPE_NIL:
					break;
				default:
					assert(false);
					return 0;
				}
				break;
			case 11:
				// Disposition
				if (!parseDisposition(*it, &strDisposition, &listDispositionParam))
					return 0;
				break;
			case 12:
				// Language
				if (!parseLanguage(*it, &listLanguage))
					return 0;
				break;
			default:
				// Unknown extension
				break;
			}
			++nCount;
		}
	}
	
	if (!bComplete)
		return 0;
	
	return new FetchDataBodyStructure(strContentType, strContentSubType,
		listContentTypeParam, strId, strDescription, strEncoding,
		nSize, nLine, strMd5, strDisposition, listDispositionParam,
		listLanguage, pEnvelope, listChild);
}

std::auto_ptr<FetchDataBodyStructure> qmimap4::FetchDataBodyStructure::parseChild(ListItem* pListItem,
																				  bool bExtended)
{
	assert(pListItem);
	
	if (pListItem->getType() != ListItem::TYPE_LIST)
		return false;
	
	return create(static_cast<List*>(pListItem), bExtended);
}

bool qmimap4::FetchDataBodyStructure::parseParam(ListItem* pListItem,
												 ParamList* pListParam)
{
	assert(pListItem);
	assert(pListParam);
	
	switch (pListItem->getType()) {
	case ListItem::TYPE_NIL:
		break;
	case ListItem::TYPE_TEXT:
		return false;
	case ListItem::TYPE_LIST:
		{
			const List::ItemList& l = static_cast<List*>(pListItem)->getList();
			if (l.size() % 2 != 0)
				return false;
			for (List::ItemList::size_type n = 0; n < l.size(); n += 2) {
				if (l[n]->getType() != ListItem::TYPE_TEXT ||
					l[n + 1]->getType() != ListItem::TYPE_TEXT)
					return false;
				
				string_ptr strName(static_cast<ListItemText*>(l[n])->releaseText());
				if (!strName.get())
					return false;
				string_ptr strValue(static_cast<ListItemText*>(l[n + 1])->releaseText());
				if (!strValue.get())
					return false;
				pListParam->push_back(std::make_pair(strName.get(), strValue.get()));
				strName.release();
				strValue.release();
			}
		}
		break;
	default:
		assert(false);
		return false;
	}
	
	return true;
}

bool qmimap4::FetchDataBodyStructure::parseDisposition(ListItem* pListItem,
													   string_ptr* pstrDisposition,
													   ParamList* pListParam)
{
	assert(pListItem);
	assert(pstrDisposition);
	assert(pListParam);
	
	switch (pListItem->getType()) {
	case ListItem::TYPE_NIL:
		break;
	case ListItem::TYPE_TEXT:
		return false;
	case ListItem::TYPE_LIST:
		{
			const List::ItemList& l = static_cast<List*>(pListItem)->getList();
			if (l.size() != 2)
				return false;
			
			if (l.front()->getType() != ListItem::TYPE_TEXT)
				return false;
			*pstrDisposition = static_cast<ListItemText*>(l.front())->releaseText();
			
			if (!parseParam(l.back(), pListParam))
				return false;
		}
		break;
	default:
		assert(false);
		return false;
	}
	
	return true;
}

bool qmimap4::FetchDataBodyStructure::parseLanguage(ListItem* pListItem,
													LanguageList* pListLanguage)
{
	assert(pListItem);
	assert(pListLanguage);
	
	switch (pListItem->getType()) {
	case ListItem::TYPE_NIL:
		break;
	case ListItem::TYPE_TEXT:
		{
			string_ptr str(static_cast<ListItemText*>(pListItem)->releaseText());
			if (str.get()) {
				pListLanguage->push_back(str.get());
				str.release();
			}
		}
		break;
	case ListItem::TYPE_LIST:
		{
			const List::ItemList& l = static_cast<List*>(pListItem)->getList();
			for (List::ItemList::const_iterator it = l.begin(); it != l.end(); ++it) {
				if ((*it)->getType() != ListItem::TYPE_TEXT)
					return false;
				string_ptr str(static_cast<ListItemText*>(*it)->releaseText());
				pListLanguage->push_back(str.get());
				str.release();
			}
		}
		break;
	default:
		assert(false);
		return false;
	}
	
	return true;
}


/****************************************************************************
 *
 * FetchDataEnvelope
 *
 */

qmimap4::FetchDataEnvelope::FetchDataEnvelope(AddressList* pListAddress,
											  string_ptr strDate,
											  string_ptr strSubject,
											  string_ptr strInReplyTo,
											  string_ptr strMessageId) :
	FetchData(TYPE_ENVELOPE),
	strDate_(strDate),
	strSubject_(strSubject),
	strInReplyTo_(strInReplyTo),
	strMessageId_(strMessageId)
{
	for (int n = 0; n < countof(listAddress_); ++n)
		listAddress_[n].swap(pListAddress[n]);
}

qmimap4::FetchDataEnvelope::~FetchDataEnvelope()
{
	for (int n = 0; n < countof(listAddress_); ++n)
		std::for_each(listAddress_[n].begin(), listAddress_[n].end(),
			deleter<EnvelopeAddress>());
}

const FetchDataEnvelope::AddressList& qmimap4::FetchDataEnvelope::getAddresses(
	Address address) const
{
	return listAddress_[address];
}

const CHAR* qmimap4::FetchDataEnvelope::getDate() const
{
	return strDate_.get();
}

const CHAR* qmimap4::FetchDataEnvelope::getSubject() const
{
	return strSubject_.get();
}

const CHAR* qmimap4::FetchDataEnvelope::getMessageId() const
{
	return strMessageId_.get();
}

const CHAR* qmimap4::FetchDataEnvelope::getInReplyTo() const
{
	return strInReplyTo_.get();
}

std::auto_ptr<FetchDataEnvelope> qmimap4::FetchDataEnvelope::create(List* pList)
{
	AddressList listAddress[6];
	string_ptr str[4];
	
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
				str[nText] = static_cast<ListItemText*>(*it)->releaseText();
				break;
			case ListItem::TYPE_LIST:
				return 0;
			default:
				assert(false);
				return 0;
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
				return 0;
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
								std::auto_ptr<EnvelopeAddress> pAddress(
									EnvelopeAddress::create(static_cast<List*>(*itAddr)));
								if (!pAddress.get())
									return 0;
								listAddress[nList].push_back(pAddress.get());
								pAddress.release();
							}
							break;
						default:
							assert(false);
							return 0;
						}
						++itAddr;
					}
				}
				break;
			default:
				assert(false);
				return 0;
			}
			++nList;
			break;
		}
		++nItem;
	}
	
	return new FetchDataEnvelope(listAddress, str[0], str[1], str[2], str[3]);
}


/****************************************************************************
 *
 * FetchDataFlags
 *
 */

qmimap4::FetchDataFlags::FetchDataFlags(unsigned int nSystemFlags,
										FlagList& listCustomFlag) :
	FetchData(TYPE_FLAGS),
	nSystemFlags_(nSystemFlags)
{
	listCustomFlag_.swap(listCustomFlag);
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

std::auto_ptr<FetchDataFlags> qmimap4::FetchDataFlags::create(List* pList)
{
	unsigned int nSystemFlags = 0;
	FlagList listCustomFlag;
	
	const List::ItemList& l = pList->getList();
	for (List::ItemList::const_iterator it = l.begin(); it != l.end(); ++it) {
		if ((*it)->getType() != ListItem::TYPE_TEXT)
			return 0;
		
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
		int n = 0;
		while (n < countof(systemFlags)) {
			if (_stricmp(pszFlag, systemFlags[n].pszName_) == 0) {
				nSystemFlags |= systemFlags[n].flag_;
				break;
			}
			++n;
		}
		if (n == countof(systemFlags)) {
			string_ptr str(static_cast<ListItemText*>(*it)->releaseText());
			listCustomFlag.push_back(str.get());
			str.release();
		}
	}
	
	return new FetchDataFlags(nSystemFlags, listCustomFlag);
}


/****************************************************************************
 *
 * FetchDataInternalDate
 *
 */

qmimap4::FetchDataInternalDate::FetchDataInternalDate(const Time& time) :
	FetchData(TYPE_INTERNALDATE),
	time_(time)
{
}

qmimap4::FetchDataInternalDate::~FetchDataInternalDate()
{
}

const Time& qmimap4::FetchDataInternalDate::getTime() const
{
	return time_;
}

std::auto_ptr<FetchDataInternalDate> qmimap4::FetchDataInternalDate::create(const CHAR* pszDate)
{
	if (strlen(pszDate) != 26 ||
		pszDate[2] != '-' ||
		pszDate[6] != '-' ||
		pszDate[11] != ' ' ||
		pszDate[14] != ':' ||
		pszDate[17] != ':' ||
		pszDate[20] != ' ')
		return 0;
	
	Time time;
	
	CHAR* pTemp = 0;
	
	if (!isdigit(pszDate[1]))
		return 0;
	time.wDay = pszDate[1] - '0';
	if (isdigit(pszDate[0]))
		time.wDay += (pszDate[0] - '0')*10;
	else if (pszDate[0] != ' ')
		return 0;
	
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
		return 0;
	time.wMonth = nMonth;
	
	const CHAR* pYear = pszDate + 7;
	time.wYear = static_cast<WORD>(strtol(pYear, &pTemp, 10));
	if (pTemp != pYear + 4)
		return 0;
	
	const CHAR* pTime = pszDate + 12;
	WORD* pwTimes[] = {
		&time.wHour,
		&time.wMinute,
		&time.wSecond
	};
	for (int n = 0; n < countof(pwTimes); ++n) {
		if (!isdigit(pTime[0]) || !isdigit(pTime[1]))
			return 0;
		*pwTimes[n] = (pTime[0] - '0')*10 + (pTime[1] - '0');
		pTime += 3;
	}
	
	const CHAR* pZone = pszDate + 21;
	if (pZone[0] != '+' && pZone[0] != '-')
		return 0;
	long nZone = strtol(pZone, &pTemp, 10);
	if (pTemp != pZone + 4)
		return 0;
	if (pZone[0] == '-')
		nZone = -nZone;
	time.setTimeZone(nZone);
	
	return new FetchDataInternalDate(time);
}


/****************************************************************************
 *
 * FetchDataSize
 *
 */

qmimap4::FetchDataSize::FetchDataSize(unsigned long nSize) :
	FetchData(TYPE_SIZE),
	nSize_(nSize)
{
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

qmimap4::FetchDataUid::FetchDataUid(unsigned long nUid) :
	FetchData(TYPE_UID),
	nUid_(nUid)
{
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

qmimap4::EnvelopeAddress::EnvelopeAddress(string_ptr strName,
										  string_ptr strDomain,
										  string_ptr strMailbox,
										  string_ptr strHost) :
	strName_(strName),
	strDomain_(strDomain),
	strMailbox_(strMailbox),
	strHost_(strHost)
{
}

qmimap4::EnvelopeAddress::~EnvelopeAddress()
{
}

const CHAR* qmimap4::EnvelopeAddress::getName() const
{
	return strName_.get();
}

const CHAR* qmimap4::EnvelopeAddress::getDomain() const
{
	return strDomain_.get();
}

const CHAR* qmimap4::EnvelopeAddress::getMailbox() const
{
	return strMailbox_.get();
}

const CHAR* qmimap4::EnvelopeAddress::getHost() const
{
	return strHost_.get();
}

std::auto_ptr<EnvelopeAddress> qmimap4::EnvelopeAddress::create(List* pList)
{
	string_ptr str[4];
	
	const List::ItemList& l = pList->getList();
	if (l.size() != 4)
		return 0;
	
	for (List::ItemList::size_type n = 0; n < l.size(); ++n) {
		if (l[n]->getType() == ListItem::TYPE_NIL)
			continue;
		else if (l[n]->getType() != ListItem::TYPE_TEXT)
			return 0;
		str[n] = static_cast<ListItemText*>(l[n])->releaseText();
	}
	
	return new EnvelopeAddress(str[0], str[1], str[2], str[3]);
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

qmimap4::ListItemNil::ListItemNil() :
	ListItem(TYPE_NIL)
{
}

qmimap4::ListItemNil::~ListItemNil()
{
}


/****************************************************************************
 *
 * ListItemText
 *
 */

qmimap4::ListItemText::ListItemText(qs::string_ptr str) :
	ListItem(TYPE_TEXT),
	str_(str)
{
	assert(str_.get());
}

qmimap4::ListItemText::~ListItemText()
{
}

const CHAR* qmimap4::ListItemText::getText() const
{
	assert(str_.get());
	return str_.get();
}

string_ptr qmimap4::ListItemText::releaseText()
{
	assert(str_.get());
	
	string_ptr str = str_;
	str_.reset(0);
	return str;
}


/****************************************************************************
 *
 * List
 *
 */

qmimap4::List::List() :
	ListItem(TYPE_LIST)
{
}

qmimap4::List::~List()
{
	std::for_each(list_.begin(), list_.end(), deleter<ListItem>());
}

const List::ItemList& qmimap4::List::getList() const
{
	return list_;
}

void qmimap4::List::add(std::auto_ptr<ListItem> pItem)
{
	list_.push_back(pItem.get());
	pItem.release();
}


/****************************************************************************
 *
 * State
 *
 */

qmimap4::State::State(Code code) :
	code_(code),
	n_(0)
{
}

qmimap4::State::~State()
{
}

State::Code qmimap4::State::getCode() const
{
	return code_;
}

const CHAR* qmimap4::State::getMessage() const
{
	return strMessage_.get();
}

unsigned long qmimap4::State::getArgNumber() const
{
	return n_;
}

const List* qmimap4::State::getArgList() const
{
	return pList_.get();
}

void qmimap4::State::setMessage(string_ptr str)
{
	strMessage_ = str;
}

void qmimap4::State::setArg(unsigned long n)
{
	n_ = n;
}

void qmimap4::State::setArg(std::auto_ptr<List> pList)
{
	pList_ = pList;
}
