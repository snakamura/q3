/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __IMAP4_H__
#define __IMAP4_H__

#include <qs.h>
#include <qsconv.h>
#include <qslog.h>
#include <qssocket.h>
#include <qsssl.h>
#include <qsutil.h>

#include <vector>


namespace qmimap4 {

class Imap4;
class Imap4Callback;
class Range;
	class DefaultRange;
		class SingleRange;
		class ContinuousRange;
		class MultipleRange;
		class TextRange;
class Flags;
class PartPath;
class Response;
	class ResponseCapability;
	class ResponseContinue;
	class ResponseExists;
	class ResponseExpunge;
	class ResponseFetch;
	class ResponseFlags;
	class ResponseList;
	class ResponseNamespace;
	class ResponseRecent;
	class ResponseSearch;
	class ResponseState;
	class ResponseStatus;
class FetchData;
	class FetchDataBody;
	class FetchDataBodyStructure;
	class FetchDataEnvelope;
	class FetchDataFlags;
	class FetchDataInternalDate;
	class FetchDataSize;
	class FetchDataUid;
class EnvelopeAddress;
class ListItem;
	class ListItemNil;
	class ListItemText;
	class List;
class State;

class ParserCallback;


/****************************************************************************
 *
 * Imap4
 *
 */

class Imap4
{
private:
	struct CommandToken;

public:
	enum {
		IMAP4_ERROR_SUCCESS			= 0x00000000,
		
		IMAP4_ERROR_INITIALIZE		= 0x00010000,
		IMAP4_ERROR_CONNECT			= 0x00020000,
		IMAP4_ERROR_SELECTSOCKET	= 0x00030000,
		IMAP4_ERROR_TIMEOUT			= 0x00040000,
		IMAP4_ERROR_DISCONNECT		= 0x00050000,
		IMAP4_ERROR_RECEIVE			= 0x00060000,
		IMAP4_ERROR_PARSE			= 0x00070000,
		IMAP4_ERROR_OTHER			= 0x00080000,
		IMAP4_ERROR_INVALIDSOCKET	= 0x00090000,
		IMAP4_ERROR_SEND			= 0x000a0000,
		IMAP4_ERROR_RESPONSE		= 0x000b0000,
		IMAP4_ERROR_SSL				= 0x000c0000,
		IMAP4_ERROR_MASK_LOWLEVEL	= 0x00ff0000,
		
		IMAP4_ERROR_GREETING		= 0x00000100,
		IMAP4_ERROR_LOGIN			= 0x00000200,
		IMAP4_ERROR_CAPABILITY		= 0x00000300,
		IMAP4_ERROR_FETCH			= 0x00000400,
		IMAP4_ERROR_STORE			= 0x00000500,
		IMAP4_ERROR_SELECT			= 0x00000600,
		IMAP4_ERROR_LSUB			= 0x00000700,
		IMAP4_ERROR_LIST			= 0x00000800,
		IMAP4_ERROR_COPY			= 0x00000900,
		IMAP4_ERROR_APPEND			= 0x00000a00,
		IMAP4_ERROR_NOOP			= 0x00000b00,
		IMAP4_ERROR_CREATE			= 0x00000c00,
		IMAP4_ERROR_DELETE			= 0x00000d00,
		IMAP4_ERROR_RENAME			= 0x00000e00,
		IMAP4_ERROR_SUBSCRIBE		= 0x00000f00,
		IMAP4_ERROR_UNSUBSCRIBE		= 0x00001000,
		IMAP4_ERROR_CLOSE			= 0x00001100,
		IMAP4_ERROR_EXPUNGE			= 0x00001200,
		IMAP4_ERROR_AUTHENTICATE	= 0x00001300,
		IMAP4_ERROR_SEARCH			= 0x00001400,
		IMAP4_ERROR_NAMESPACE		= 0x00001500,
		IMAP4_ERROR_LOGOUT			= 0x00001600,
		IMAP4_ERROR_MASK_HIGHLEVEL	= 0x0000ff00
	};
	
	enum Flag {
		FLAG_ANSWERED	= 0x01,
		FLAG_FLAGGED	= 0x02,
		FLAG_DELETED	= 0x04,
		FLAG_SEEN		= 0x08,
		FLAG_DRAFT		= 0x10,
		FLAG_RECENT		= 0x20,
	};
	
	enum Capability {
		CAPABILITY_NAMESPACE	= 0x0001,
	};
	
	enum Auth {
		AUTH_LOGIN		= 0x01,
		AUTH_CRAMMD5	= 0x02
	};

public:
	struct Option
	{
		long nTimeout_;
		qs::SocketCallback* pSocketCallback_;
		qs::SSLSocketCallback* pSSLSocketCallback_;
		Imap4Callback* pImap4Callback_;
		qs::Logger* pLogger_;
	};

public:
	Imap4(const Option& option, qs::QSTATUS* pstatus);
	~Imap4();

public:
	qs::QSTATUS connect(const WCHAR* pwszHost, short nPort, bool bSsl);
	qs::QSTATUS disconnect();
	qs::QSTATUS checkConnection();
	
	qs::QSTATUS select(const WCHAR* pwszFolderName);
	qs::QSTATUS close();
	qs::QSTATUS noop();
	
	qs::QSTATUS fetch(const Range& range, const CHAR* pszFetch);
	qs::QSTATUS store(const Range& range, const CHAR* pszStore);
	qs::QSTATUS copy(const Range& range, const WCHAR* pwszFolderName);
	qs::QSTATUS search(const WCHAR* pwszSearch,
		const WCHAR* pwszCharset, bool bUseCharset, bool bUid);
	qs::QSTATUS expunge();
	
	qs::QSTATUS append(const WCHAR* pwszFolderName,
		const CHAR* pszMessage, const Flags& flags);
	qs::QSTATUS list(bool bSubscribeOnly,
		const WCHAR* pwszRef, const WCHAR* pwszMailbox);
	qs::QSTATUS create(const WCHAR* pwszFolderName);
	qs::QSTATUS remove(const WCHAR* pwszFolderName);
	qs::QSTATUS rename(const WCHAR* pwszOldFolderName,
		const WCHAR* pwszNewFolderName);
	qs::QSTATUS subscribe(const WCHAR* pwszFolderName);
	qs::QSTATUS unsubscribe(const WCHAR* pwszFolderName);
	qs::QSTATUS namespaceList();
	
	
	qs::QSTATUS getFlags(const Range& range);
	qs::QSTATUS setFlags(const Range& range,
		const Flags& flags, const Flags& mask);
	
	qs::QSTATUS getMessageData(const Range& range,
		bool bClientParse, bool bBody, const CHAR* pszFields);
	qs::QSTATUS getMessage(const Range& range, bool bPeek);
	qs::QSTATUS getHeader(const Range& range, bool bPeek);
	qs::QSTATUS getBodyStructure(const Range& range);
	qs::QSTATUS getPart(const Range& range, const PartPath& path);
	qs::QSTATUS getPartMime(const Range& range, const PartPath& path);
	qs::QSTATUS getPartBody(const Range& range, const PartPath& path);
	
	unsigned int getCapability() const;
	
	unsigned int getLastError() const;
	const WCHAR* getLastErrorResponse() const;

private:
	qs::QSTATUS processGreeting();
	qs::QSTATUS processCapability();
	qs::QSTATUS processLogin();
	qs::QSTATUS receive(const CHAR* pszTag,
		bool bAcceptContinue, ParserCallback* pCallback);
	qs::QSTATUS sendCommand(const CHAR* pszCommand, ParserCallback* pCallback);
	qs::QSTATUS sendCommand(const CHAR* pszCommand,
		qs::STRING* pstrTag, ParserCallback* pCallback);
	qs::QSTATUS send(const CHAR* pszContent, const CHAR* pszTag,
		bool bAcceptContinue, ParserCallback* pCallback);
	qs::QSTATUS send(const CHAR** pszContents, size_t nCount,
		const CHAR* pszTag, bool bAcceptContinue, ParserCallback* pCallback);
	qs::QSTATUS sendCommandTokens(const CommandToken* pTokens, size_t nCount);
	qs::QSTATUS getTag(qs::STRING* pstrTag);
	qs::QSTATUS getAuthMethods(unsigned int* pnAuth);

private:
	static qs::QSTATUS getQuotedString(const CHAR* psz, qs::STRING* pstrQuoted);

private:
	Imap4(const Imap4&);
	Imap4& operator=(const Imap4&);

private:
	struct CommandToken
	{
		const CHAR* psz_;
		const WCHAR* pwsz_;
		qs::Converter* pConverter_;
		bool bQuote_;
		bool b_;
	};

private:
	long nTimeout_;
	qs::SocketCallback* pSocketCallback_;
	qs::SSLSocketCallback* pSSLSocketCallback_;
	Imap4Callback* pImap4Callback_;
	qs::Logger* pLogger_;
	qs::SocketBase* pSocket_;
	qs::STRING strOverBuf_;
	unsigned int nCapability_;
	unsigned int nAuth_;
	bool bDisconnected_;
	unsigned int nTag_;
	unsigned int nError_;
	qs::UTF7Converter* pUTF7Converter_;
};


/****************************************************************************
 *
 * Imap4Callback
 *
 */

class Imap4Callback
{
public:
	virtual ~Imap4Callback();

public:
	virtual qs::QSTATUS getUserInfo(qs::WSTRING* pwstrUserName,
		qs::WSTRING* pwstrPassword) = 0;
	virtual qs::QSTATUS setPassword(const WCHAR* pwszPassword) = 0;
	virtual qs::QSTATUS getAuthMethods(qs::WSTRING* pwstrAuthMethods) = 0;
	
	virtual qs::QSTATUS authenticating() = 0;
	virtual qs::QSTATUS setRange(unsigned int nMin, unsigned int nMax) = 0;
	virtual qs::QSTATUS setPos(unsigned int nPos) = 0;
	
	virtual qs::QSTATUS response(Response* pResponse) = 0;
};


/****************************************************************************
 *
 * Range
 *
 */

class Range
{
public:
	virtual ~Range();

public:
	virtual bool isUid() const = 0;
	virtual const CHAR* getRange() const = 0;
};


/****************************************************************************
 *
 * DefaultRange
 *
 */

class DefaultRange : public Range
{
protected:
	DefaultRange(bool bUid);

public:
	virtual ~DefaultRange();

public:
	virtual bool isUid() const;
	virtual const CHAR* getRange() const;

protected:
	qs::QSTATUS setRange(const CHAR* pszRange);

private:
	DefaultRange(const DefaultRange&);
	DefaultRange& operator=(const DefaultRange&);

private:
	bool bUid_;
	qs::STRING strRange_;
};


/****************************************************************************
 *
 * SingleRange
 *
 */

class SingleRange : public DefaultRange
{
public:
	SingleRange(unsigned long n, bool bUid, qs::QSTATUS* pstatus);
	virtual ~SingleRange();

private:
	SingleRange(const SingleRange&);
	SingleRange& operator=(const SingleRange&);
};


/****************************************************************************
 *
 * ContinuousRange
 *
 */

class ContinuousRange : public DefaultRange
{
public:
	ContinuousRange(unsigned long nBegin, unsigned long nEnd,
		bool bUid, qs::QSTATUS* pstatus);
	virtual ~ContinuousRange();

private:
	ContinuousRange(const ContinuousRange&);
	ContinuousRange& operator=(const ContinuousRange&);
};


/****************************************************************************
 *
 * MultipleRange
 *
 */

class MultipleRange : public DefaultRange
{
public:
	MultipleRange(const unsigned long* pn, size_t nCount,
		bool bUid, qs::QSTATUS* pstatus);
	virtual ~MultipleRange();

private:
	MultipleRange(const MultipleRange&);
	MultipleRange& operator=(const MultipleRange&);
};


/****************************************************************************
 *
 * TextRange
 *
 */

class TextRange : public DefaultRange
{
public:
	TextRange(const CHAR* pszRange, bool bUid, qs::QSTATUS* pstatus);
	virtual ~TextRange();

private:
	TextRange(const TextRange&);
	TextRange& operator=(const TextRange&);
};


/****************************************************************************
 *
 * Flags
 *
 */

class Flags
{
public:
	Flags(unsigned int nSystemFlags, qs::QSTATUS* pstatus);
	Flags(unsigned int nSystemFlags, const CHAR** pszUserFlags,
		size_t nCount, qs::QSTATUS* pstatus);
	~Flags();

public:
	qs::QSTATUS getString(qs::STRING* pstr) const;
	qs::QSTATUS getAdded(const Flags& mask, qs::STRING* pstr) const;
	qs::QSTATUS getRemoved(const Flags& mask, qs::STRING* pstr) const;

public:
	bool contains(Imap4::Flag flag) const;
	bool contains(const CHAR* pszFlag) const;

public:
	static const CHAR* getFlagString(Imap4::Flag flag);

private:
	qs::QSTATUS init(unsigned int nSystemFlags,
		const CHAR** pszUserFlags, size_t nCount);
	qs::QSTATUS getString(const Flags& mask, bool bAdd, qs::STRING* pstr) const;

private:
	typedef std::vector<qs::STRING> FlagList;

private:
	Flags(const Flags&);
	Flags& operator=(const Flags&);

private:
	unsigned int nSystemFlags_;
	FlagList listFlag_;
};


/****************************************************************************
 *
 * PartPath
 *
 */

class PartPath
{
public:
	PartPath(const unsigned int* pnPart, size_t nCount, qs::QSTATUS* pstatus);
	~PartPath();

public:
	const CHAR* getPath() const;

private:
	PartPath(const PartPath&);
	PartPath& operator=(const PartPath&);

private:
	qs::STRING strPath_;
};


/****************************************************************************
 *
 * Response
 *
 */

class Response
{
public:
	enum Type {
		TYPE_CAPABILITY,
		TYPE_CONTINUE,
		TYPE_EXISTS,
		TYPE_EXPUNGE,
		TYPE_FETCH,
		TYPE_FLAGS,
		TYPE_LIST,
		TYPE_NAMESPACE,
		TYPE_RECENT,
		TYPE_SEARCH,
		TYPE_STATE,
		TYPE_STATUS
	};

public:
	Response(Type type);
	virtual ~Response();

public:
	Type getType() const;

private:
	Response(const Response&);
	Response& operator=(const Response&);

private:
	Type type_;
};


/****************************************************************************
 *
 * ResponseCapability
 *
 */

class ResponseCapability : public Response
{
public:
	ResponseCapability(qs::QSTATUS* pstatus);
	virtual ~ResponseCapability();

public:
	bool isSupport(const CHAR* pszCapability) const;
	bool isSupportAuth(const CHAR* pszAuth) const;

public:
	qs::QSTATUS add(const CHAR* psz);

private:
	ResponseCapability(const ResponseCapability&);
	ResponseCapability& operator=(const ResponseCapability&);

private:
	typedef std::vector<qs::STRING> CapabilityList;

private:
	CapabilityList listCapability_;
	CapabilityList listAuth_;
};


/****************************************************************************
 *
 * ResponseContinue
 *
 */

class ResponseContinue : public Response
{
public:
	ResponseContinue(State* pState, qs::QSTATUS* pstatus);
	virtual ~ResponseContinue();

public:
	const State* getState() const;

private:
	ResponseContinue(const ResponseContinue&);
	ResponseContinue& operator=(const ResponseContinue&);

private:
	State* pState_;
};


/****************************************************************************
 *
 * ResponseExists
 *
 */

class ResponseExists : public Response
{
public:
	ResponseExists(unsigned long nExists, qs::QSTATUS* pstatus);
	virtual ~ResponseExists();

public:
	unsigned long getExists() const;

private:
	ResponseExists(const ResponseExists&);
	ResponseExists& operator=(const ResponseExists&);

private:
	unsigned long nExists_;
};


/****************************************************************************
 *
 * ResponseExpunge
 *
 */

class ResponseExpunge : public Response
{
public:
	ResponseExpunge(unsigned long nExpunge, qs::QSTATUS* pstatus);
	virtual ~ResponseExpunge();

public:
	unsigned long getExpunge() const;

private:
	ResponseExpunge(const ResponseExpunge&);
	ResponseExpunge& operator=(const ResponseExpunge&);

private:
	unsigned long nExpunge_;
};


/****************************************************************************
 *
 * ResponseFetch
 *
 */

class ResponseFetch : public Response
{
public:
	typedef std::vector<FetchData*> FetchDataList;

public:
	ResponseFetch(unsigned long nNumber, List* pList, qs::QSTATUS* pstatus);
	virtual ~ResponseFetch();

public:
	unsigned long getNumber() const;
	bool isUid(unsigned long nUid) const;
	const FetchDataList& getFetchDataList() const;
	FetchData* detach(FetchData* pFetchData);

private:
	ResponseFetch(const ResponseFetch&);
	ResponseFetch& operator=(const ResponseFetch&);

private:
	unsigned long nNumber_;
	FetchDataList listData_;
};


/****************************************************************************
 *
 * ResponseFlags
 *
 */

class ResponseFlags : public Response
{
public:
	typedef std::vector<qs::STRING> FlagList;

public:
	ResponseFlags(List* pList, qs::QSTATUS* pstatus);
	virtual ~ResponseFlags();

public:
	unsigned int getSystemFlags() const;
	const FlagList& getCustomFlags() const;

private:
	ResponseFlags(const ResponseFlags&);
	ResponseFlags& operator=(const ResponseFlags&);

private:
	unsigned int nSystemFlags_;
	FlagList listCustomFlag_;
};


/****************************************************************************
 *
 * ResponseList
 *
 */

class ResponseList : public Response
{
public:
	enum Attribute {
		ATTRIBUTE_NOINFERIORS	= 0x01,
		ATTRIBUTE_NOSELECT		= 0x02,
		ATTRIBUTE_MARKED		= 0x04,
		ATTRIBUTE_UNMARKED		= 0x08
	};

public:
	ResponseList(bool bList, List* pListAttribute, CHAR cSeparator,
		const CHAR* pszMailbox, qs::QSTATUS* pstatus);
	virtual ~ResponseList();

public:
	bool isList() const;
	unsigned int getAttributes() const;
	WCHAR getSeparator() const;
	const WCHAR* getMailbox() const;

private:
	ResponseList(const ResponseList&);
	ResponseList& operator=(const ResponseList&);

private:
	bool bList_;
	unsigned int nAttributes_;
	WCHAR cSeparator_;
	qs::WSTRING wstrMailbox_;
};


/****************************************************************************
 *
 * ResponseNamespace
 *
 */

class ResponseNamespace : public Response
{
public:
	typedef std::vector<std::pair<qs::WSTRING, WCHAR> > NamespaceList;

public:
	ResponseNamespace(List* pListPersonal, List* pListOthers,
		List* pListShared, qs::QSTATUS* pstatus);
	virtual ~ResponseNamespace();

public:
	const NamespaceList& getPersonal() const;
	const NamespaceList& getOthers() const;
	const NamespaceList& getShared() const;

private:
	ResponseNamespace(const ResponseNamespace&);
	ResponseNamespace& operator=(const ResponseNamespace&);

private:
	NamespaceList listPersonal_;
	NamespaceList listOthers_;
	NamespaceList listShared_;
};


/****************************************************************************
 *
 * ResponseRecent
 *
 */

class ResponseRecent : public Response
{
public:
	ResponseRecent(unsigned long nRecent, qs::QSTATUS* pstatus);
	virtual ~ResponseRecent();

public:
	unsigned long getRecent() const;

private:
	ResponseRecent(const ResponseRecent&);
	ResponseRecent& operator=(const ResponseRecent&);

private:
	unsigned long nRecent_;
};


/****************************************************************************
 *
 * ResponseSearch
 *
 */

class ResponseSearch : public Response
{
public:
	typedef std::vector<unsigned long> ResultList;

public:
	ResponseSearch(qs::QSTATUS* pstatus);
	virtual ~ResponseSearch();

public:
	const ResultList& getResult() const;

public:
	qs::QSTATUS add(unsigned long n);

private:
	ResponseSearch(const ResponseSearch&);
	ResponseSearch& operator=(const ResponseSearch&);

private:
	ResultList listResult_;
};


/****************************************************************************
 *
 * ResponseState
 *
 */

class ResponseState : public Response
{
public:
	enum Flag {
		FLAG_UNKNOWN,
		FLAG_OK,
		FLAG_NO,
		FLAG_BAD,
		FLAG_PREAUTH,
		FLAG_BYE
	};

public:
	ResponseState(Flag flag, qs::QSTATUS* pstatus);
	virtual ~ResponseState();

public:
	Flag getFlag() const;
	State* getState() const;

public:
	void setState(State* pState);

private:
	ResponseState(const ResponseState&);
	ResponseState& operator=(const ResponseState&);

private:
	Flag flag_;
	State* pState_;
};


/****************************************************************************
 *
 * ResponseStatus
 *
 */

class ResponseStatus : public Response
{
public:
	enum Status {
		STATUS_MESSAGES,
		STATUS_RECENT,
		STATUS_UIDNEXT,
		STATUS_UIDVALIDITY,
		STATUS_UNSEEN,
		STATUS_UNKNOWN
	};

public:
	typedef std::vector<std::pair<Status, unsigned int> > StatusList;

public:
	ResponseStatus(const CHAR* pszMailbox, List* pList, qs::QSTATUS* pstatus);
	virtual ~ResponseStatus();

public:
	const WCHAR* getMailbox() const;
	const StatusList& getStatusList() const;

private:
	ResponseStatus(const ResponseStatus&);
	ResponseStatus& operator=(const ResponseStatus&);

private:
	qs::WSTRING wstrMailbox_;
	StatusList listStatus_;
};


/****************************************************************************
 *
 * FetchData
 *
 */

class FetchData
{
public:
	enum Type {
		TYPE_BODY,
		TYPE_BODYSTRUCTURE,
		TYPE_ENVELOPE,
		TYPE_FLAGS,
		TYPE_INTERNALDATE,
		TYPE_SIZE,
		TYPE_UID
	};

public:
	FetchData(Type type);
	virtual ~FetchData();

public:
	Type getType() const;

private:
	FetchData(const FetchData&);
	FetchData& operator=(const FetchData&);

private:
	Type type_;
};


/****************************************************************************
 *
 * FetchDataBody
 *
 */

class FetchDataBody : public FetchData
{
public:
	enum Section {
		SECTION_NONE,
		SECTION_HEADER,
		SECTION_HEADER_FIELDS,
		SECTION_HEADER_FIELDS_NOT,
		SECTION_TEXT,
		SECTION_MIME
	};

public:
	typedef std::vector<unsigned int> PartPath;
	typedef std::vector<qs::STRING> FieldList;

public:
	FetchDataBody(const CHAR* pszSection,
		qs::STRING strContent, qs::QSTATUS* pstatus);
	virtual ~FetchDataBody();

public:
	Section getSection() const;
	const PartPath& getPartPath() const;
	const FieldList& getFieldList() const;
	const CHAR* getContent() const;

private:
	FetchDataBody(const FetchDataBody&);
	FetchDataBody& operator=(const FetchDataBody&);

private:
	Section section_;
	PartPath partPath_;
	FieldList listField_;
	qs::STRING strContent_;
};


/****************************************************************************
 *
 * FetchDataBodyStructure
 *
 */

class FetchDataBodyStructure : public FetchData
{
public:
	typedef std::vector<std::pair<qs::STRING, qs::STRING> > ParamList;
	typedef std::vector<qs::STRING> LanguageList;
	typedef std::vector<FetchDataBodyStructure*> ChildList;

public:
	FetchDataBodyStructure(List* pList, bool bExtended, qs::QSTATUS* pstatus);
	virtual ~FetchDataBodyStructure();

public:
	const CHAR* getContentType() const;
	const CHAR* getContentSubType() const;
	const ParamList& getContentParams() const;
	const CHAR* getId() const;
	const CHAR* getDescription() const;
	const CHAR* getEncoding() const;
	unsigned long getSize() const;
	unsigned long getLine() const;
	const CHAR* getMd5() const;
	const CHAR* getDisposition() const;
	const ParamList& getDispositionParams() const;
	const LanguageList& getLanguages() const;
	const FetchDataEnvelope* getEnvelope() const;
	const ChildList& getChildList() const;

private:
	qs::QSTATUS parseChild(ListItem* pListItem, bool bExtended);
	qs::QSTATUS parseParam(ListItem* pListItem, ParamList* pListParam);
	qs::QSTATUS parseDisposition(ListItem* pListItem);
	qs::QSTATUS parseLanguage(ListItem* pListItem);

private:
	FetchDataBodyStructure(const FetchDataBodyStructure&);
	FetchDataBodyStructure& operator=(const FetchDataBodyStructure&);

private:
	qs::STRING strContentType_;
	qs::STRING strContentSubType_;
	ParamList listContentTypeParam_;
	qs::STRING strId_;
	qs::STRING strDescription_;
	qs::STRING strEncoding_;
	unsigned long nSize_;
	unsigned long nLine_;
	qs::STRING strMd5_;
	qs::STRING strDisposition_;
	ParamList listDispositionParam_;
	LanguageList listLanguage_;
	FetchDataEnvelope* pEnvelope_;
	ChildList listChild_;
};


/****************************************************************************
 *
 * FetchDataEnvelope
 *
 */

class FetchDataEnvelope : public FetchData
{
public:
	enum Address {
		ADDRESS_FROM,
		ADDRESS_SENDER,
		ADDRESS_REPLYTO,
		ADDRESS_TO,
		ADDRESS_CC,
		ADDRESS_BCC
	};

public:
	typedef std::vector<EnvelopeAddress*> AddressList;

public:
	FetchDataEnvelope(List* pList, qs::QSTATUS* pstatus);
	virtual ~FetchDataEnvelope();

public:
	const AddressList& getAddresses(Address address) const;
	const CHAR* getDate() const;
	const CHAR* getSubject() const;
	const CHAR* getMessageId() const;
	const CHAR* getInReplyTo() const;

private:
	FetchDataEnvelope(const FetchDataEnvelope&);
	FetchDataEnvelope& operator=(const FetchDataEnvelope&);

private:
	AddressList listAddress_[6];
	qs::STRING strDate_;
	qs::STRING strSubject_;
	qs::STRING strMessageId_;
	qs::STRING strInReplyTo_;
};


/****************************************************************************
 *
 * FetchDataFlags
 *
 */

class FetchDataFlags : public FetchData
{
public:
	typedef std::vector<qs::STRING> FlagList;

public:
	FetchDataFlags(List* pList, qs::QSTATUS* pstatus);
	virtual ~FetchDataFlags();

public:
	unsigned int getSystemFlags() const;
	const FlagList& getCustomFlags() const;

private:
	FetchDataFlags(const FetchDataFlags&);
	FetchDataFlags& operator=(const FetchDataFlags&);

private:
	unsigned int nSystemFlags_;
	FlagList listCustomFlag_;
};


/****************************************************************************
 *
 * FetchDataInternalDate
 *
 */

class FetchDataInternalDate : public FetchData
{
public:
	FetchDataInternalDate(const CHAR* pszDate, qs::QSTATUS* pstatus);
	virtual ~FetchDataInternalDate();

public:
	const qs::Time& getTime() const;

private:
	FetchDataInternalDate(const FetchDataInternalDate&);
	FetchDataInternalDate& operator=(const FetchDataInternalDate&);

private:
	qs::Time time_;
};


/****************************************************************************
 *
 * FetchDataSize
 *
 */

class FetchDataSize : public FetchData
{
public:
	FetchDataSize(unsigned long nSize, qs::QSTATUS* pstatus);
	virtual ~FetchDataSize();

public:
	unsigned long getSize() const;

private:
	FetchDataSize(const FetchDataSize&);
	FetchDataSize& operator=(const FetchDataSize&);

private:
	unsigned long nSize_;
};


/****************************************************************************
 *
 * FetchDataUid
 *
 */

class FetchDataUid : public FetchData
{
public:
	FetchDataUid(unsigned long nUid, qs::QSTATUS* pstatus);
	virtual ~FetchDataUid();

public:
	unsigned long getUid() const;

private:
	FetchDataUid(const FetchDataUid&);
	FetchDataUid& operator=(const FetchDataUid&);

private:
	unsigned long nUid_;
};


/****************************************************************************
 *
 * EnvelopeAddress
 *
 */

class EnvelopeAddress
{
public:
	EnvelopeAddress(List* pList, qs::QSTATUS* pstatus);
	~EnvelopeAddress();

public:
	const CHAR* getName() const;
	const CHAR* getDomain() const;
	const CHAR* getMailbox() const;
	const CHAR* getHost() const;

private:
	EnvelopeAddress(const EnvelopeAddress&);
	EnvelopeAddress& operator=(const EnvelopeAddress&);

private:
	qs::STRING strName_;
	qs::STRING strDomain_;
	qs::STRING strMailbox_;
	qs::STRING strHost_;
};


/****************************************************************************
 *
 * ListItem
 *
 */

class ListItem
{
public:
	enum Type {
		TYPE_NIL,
		TYPE_TEXT,
		TYPE_LIST
	};

public:
	ListItem(Type type);
	virtual ~ListItem();

public:
	Type getType() const;

private:
	ListItem(const ListItem&);
	ListItem& operator=(const ListItem&);

private:
	Type type_;
};


/****************************************************************************
 *
 * ListItemNil
 *
 */

class ListItemNil : public ListItem
{
public:
	ListItemNil(qs::QSTATUS* pstatus);
	virtual ~ListItemNil();

private:
	ListItemNil(const ListItemNil&);
	ListItemNil& operator=(const ListItemNil&);
};


/****************************************************************************
 *
 * ListItemText
 *
 */

class ListItemText : public ListItem
{
public:
	ListItemText(qs::STRING str, qs::QSTATUS* pstatus);
	virtual ~ListItemText();

public:
	const CHAR* getText() const;
	qs::STRING releaseText();

private:
	ListItemText(const ListItemText&);
	ListItemText& operator=(const ListItemText&);

private:
	qs::STRING str_;
};


/****************************************************************************
 *
 * List
 *
 */

class List : public ListItem
{
public:
	typedef std::vector<ListItem*> ItemList;

public:
	List(qs::QSTATUS* pstatus);
	virtual ~List();

public:
	const ItemList& getList() const;

public:
	qs::QSTATUS add(ListItem* pItem);

private:
	List(const List&);
	List& operator=(const List&);

private:
	ItemList list_;
};


/****************************************************************************
 *
 * State
 *
 */

class State
{
public:
	enum Code {
		CODE_NONE,
		CODE_ALERT,
		CODE_NEWNAME,
		CODE_PARSE,
		CODE_PERMANENTFLAGS,
		CODE_READONLY,
		CODE_READWRITE,
		CODE_TRYCREATE,
		CODE_UIDVALIDITY,
		CODE_UNSEEN,
		CODE_UIDNEXT,
		CODE_OTHER
	};

public:
	State(Code code, qs::QSTATUS* pstatus);
	~State();

public:
	Code getCode() const;
	const CHAR* getMessage() const;
	unsigned long getArgNumber() const;
	const List* getArgList() const;

public:
	void setMessage(qs::STRING str);
	void setArg(unsigned long n);
	void setArg(List* pList);
	
private:
	State(const State&);
	State& operator=(const State&);

private:
	Code code_;
	qs::STRING strMessage_;
	unsigned long n_;
	List* pList_;
};

}

#endif // __IMAP4_H__
