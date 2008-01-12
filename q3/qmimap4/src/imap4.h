/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
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

#include "buffer.h"


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
		IMAP4_ERROR_STARTTLS		= 0x00001700,
		IMAP4_ERROR_MASK_HIGHLEVEL	= 0x0000ff00
	};
	
	enum Flag {
		FLAG_NONE		= 0x00,
		FLAG_ANSWERED	= 0x01,
		FLAG_FLAGGED	= 0x02,
		FLAG_DELETED	= 0x04,
		FLAG_SEEN		= 0x08,
		FLAG_DRAFT		= 0x10,
		FLAG_RECENT		= 0x20,
	};
	
	enum Capability {
		CAPABILITY_NAMESPACE	= 0x0001,
		CAPABILITY_STARTTLS		= 0x0002
	};
	
	enum Auth {
		AUTH_LOGIN		= 0x01,
		AUTH_CRAMMD5	= 0x02
	};
	
	enum Secure {
		SECURE_NONE,
		SECURE_SSL,
		SECURE_STARTTLS
	};
	
	enum {
		SEND_BLOCK_SIZE		= 8192,
		RECEIVE_BLOCK_SIZE	= 8192
	};

public:
	Imap4(long nTimeout,
		  qs::SocketCallback* pSocketCallback,
		  qs::SSLSocketCallback* pSSLSocketCallback,
		  Imap4Callback* pImap4Callback,
		  qs::Logger* pLogger);
	~Imap4();

public:
	bool connect(const WCHAR* pwszHost,
				 short nPort,
				 Secure secure);
	void disconnect();
	bool checkConnection();
	
	bool select(const WCHAR* pwszFolderName);
	bool close();
	bool noop();
	
	bool fetch(const Range& range,
			   const CHAR* pszFetch);
	bool store(const Range& range,
			   const CHAR* pszStore);
	bool copy(const Range& range,
			  const WCHAR* pwszFolderName);
	bool search(const WCHAR* pwszSearch,
				const WCHAR* pwszCharset,
				bool bUseCharset,
				bool bUid);
	bool expunge();
	
	bool append(const WCHAR* pwszFolderName,
				const CHAR* pszMessage,
				size_t nLen,
				const Flags& flags);
	bool list(bool bSubscribeOnly,
			  const WCHAR* pwszRef,
			  const WCHAR* pwszMailbox);
	bool create(const WCHAR* pwszFolderName);
	bool remove(const WCHAR* pwszFolderName);
	bool rename(const WCHAR* pwszOldFolderName,
				const WCHAR* pwszNewFolderName);
	bool subscribe(const WCHAR* pwszFolderName);
	bool unsubscribe(const WCHAR* pwszFolderName);
	bool namespaceList();
	
	bool getFlags(const Range& range);
	bool setFlags(const Range& range,
				  const Flags& flags,
				  const Flags& mask);
	
	bool getMessageData(const Range& range,
						bool bClientParse,
						bool bBody,
						const CHAR* pszFields);
	bool getMessage(const Range& range,
					bool bPeek);
	bool getHeader(const Range& range,
				   bool bPeek);
	bool getBodyStructure(const Range& range);
	bool getPart(const Range& range,
				 const PartPath& path);
	bool getPartMime(const Range& range,
					 const PartPath& path);
	bool getPartBody(const Range& range,
					 const PartPath& path);
	
	unsigned int getCapability() const;
	
	unsigned int getLastError() const;
	const WCHAR* getLastErrorResponse() const;

private:
	bool processGreeting();
	bool processCapability();
	bool processLogin();
	bool receive(const CHAR* pszTag,
				 bool bAcceptContinue,
				 ParserCallback* pCallback);
	bool sendCommand(const CHAR* pszCommand,
					 ParserCallback* pCallback);
	bool sendCommand(const CHAR* pszCommand,
					 qs::string_ptr* pstrTag,
					 ParserCallback* pCallback);
	bool send(const CHAR* pszContent,
			  const CHAR* pszTag,
			  bool bAcceptContinue,
			  ParserCallback* pCallback);
	bool send(const CHAR** pszContents,
			  const size_t* pnLen,
			  size_t nCount,
			  const CHAR* pszTag,
			  bool bAcceptContinue,
			  ParserCallback* pCallback);
	bool sendCommandTokens(const CommandToken* pTokens,
						   size_t nCount);
	qs::string_ptr getTag();
	unsigned int getAuthMethods();

private:
	static qs::string_ptr getQuotedString(const CHAR* psz);
	static qs::string_ptr encodeSearchString(const WCHAR* pwsz,
											 const WCHAR* pwszCharset);

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
	std::auto_ptr<qs::SocketBase> pSocket_;
	qs::string_ptr strOverBuf_;
	unsigned int nCapability_;
	unsigned int nAuth_;
	bool bDisconnected_;
	unsigned int nTag_;
	unsigned int nError_;
	qs::UTF7Converter utf7Converter_;
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
	virtual bool getUserInfo(qs::wstring_ptr* pwstrUserName,
							 qs::wstring_ptr* pwstrPassword) = 0;
	virtual void setPassword(const WCHAR* pwszPassword) = 0;
	virtual qs::wstring_ptr getAuthMethods() = 0;
	
	virtual void authenticating() = 0;
	virtual void setRange(size_t nMin,
						  size_t nMax) = 0;
	virtual void setPos(size_t nPos) = 0;
	
	virtual bool response(Response* pResponse) = 0;
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
	void setRange(const CHAR* pszRange);

private:
	DefaultRange(const DefaultRange&);
	DefaultRange& operator=(const DefaultRange&);

private:
	bool bUid_;
	qs::string_ptr strRange_;
};


/****************************************************************************
 *
 * SingleRange
 *
 */

class SingleRange : public DefaultRange
{
public:
	SingleRange(unsigned long n,
				bool bUid);
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
	ContinuousRange(unsigned long nBegin,
					unsigned long nEnd,
					bool bUid);
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
	MultipleRange(const unsigned long* pn,
				  size_t nCount,
				  bool bUid);
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
	TextRange(const CHAR* pszRange,
			  bool bUid);
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
	explicit Flags(unsigned int nSystemFlags);
	Flags(unsigned int nSystemFlags,
		  const CHAR** pszUserFlags,
		  size_t nCount);
	~Flags();

public:
	qs::string_ptr getString() const;
	qs::string_ptr getAdded(const Flags& mask) const;
	qs::string_ptr getRemoved(const Flags& mask) const;

public:
	bool contains(Imap4::Flag flag) const;
	bool contains(const CHAR* pszFlag) const;

public:
	static const CHAR* getFlagString(Imap4::Flag flag);

private:
	void init(unsigned int nSystemFlags,
			  const CHAR** pszUserFlags,
			  size_t nCount);
	qs::string_ptr getString(const Flags& mask,
							 bool bAdd) const;

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
	PartPath(const unsigned int* pnPart,
			 size_t nCount);
	~PartPath();

public:
	const CHAR* getPath() const;

private:
	PartPath(const PartPath&);
	PartPath& operator=(const PartPath&);

private:
	qs::string_ptr strPath_;
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

protected:
	Response(Type type);

public:
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
	ResponseCapability();
	virtual ~ResponseCapability();

public:
	bool isSupport(const CHAR* pszCapability) const;
	bool isSupportAuth(const CHAR* pszAuth) const;

public:
	void add(const CHAR* psz,
			 size_t nLen);

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
	ResponseContinue(std::auto_ptr<State> pState);
	virtual ~ResponseContinue();

public:
	const State* getState() const;

private:
	ResponseContinue(const ResponseContinue&);
	ResponseContinue& operator=(const ResponseContinue&);

private:
	std::auto_ptr<State> pState_;
};


/****************************************************************************
 *
 * ResponseExists
 *
 */

class ResponseExists : public Response
{
public:
	ResponseExists(unsigned long nExists);
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
	ResponseExpunge(unsigned long nExpunge);
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
	ResponseFetch(unsigned long nNumber,
				  unsigned long nUid,
				  FetchDataList& listData);
	virtual ~ResponseFetch();

public:
	unsigned long getNumber() const;
	unsigned long getUid() const;
	const FetchDataList& getFetchDataList() const;
	FetchData* detach(FetchData* pFetchData);

public:
	static std::auto_ptr<ResponseFetch> create(unsigned long nNumber,
											   List* pList);

private:
	ResponseFetch(const ResponseFetch&);
	ResponseFetch& operator=(const ResponseFetch&);

private:
	unsigned long nNumber_;
	unsigned long nUid_;
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
	ResponseFlags(unsigned int nSystemFlags,
				  FlagList& listCustomFlag);
	virtual ~ResponseFlags();

public:
	unsigned int getSystemFlags() const;
	const FlagList& getCustomFlags() const;

public:
	static std::auto_ptr<ResponseFlags> create(List* pList);

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
	ResponseList(bool bList,
				 unsigned int nAttributes,
				 WCHAR cSeparator,
				 const WCHAR* pwszMailbox);
	virtual ~ResponseList();

public:
	bool isList() const;
	unsigned int getAttributes() const;
	WCHAR getSeparator() const;
	const WCHAR* getMailbox() const;

public:
	static std::auto_ptr<ResponseList> create(bool bList,
											  List* pListAttribute,
											  CHAR cSeparator,
											  const CHAR* pszMailbox,
											  size_t nMailboxLen);

private:
	ResponseList(const ResponseList&);
	ResponseList& operator=(const ResponseList&);

private:
	bool bList_;
	unsigned int nAttributes_;
	WCHAR cSeparator_;
	qs::wstring_ptr wstrMailbox_;
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
	ResponseNamespace(NamespaceList& listPersonal,
					  NamespaceList& listOthers,
					  NamespaceList& listShared);
	virtual ~ResponseNamespace();

public:
	const NamespaceList& getPersonal() const;
	const NamespaceList& getOthers() const;
	const NamespaceList& getShared() const;

public:
	static std::auto_ptr<ResponseNamespace> create(List* pListPersonal,
												   List* pListOthers,
												   List* pListShared);

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
	explicit ResponseRecent(unsigned long nRecent);
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
	ResponseSearch();
	virtual ~ResponseSearch();

public:
	const ResultList& getResult() const;

public:
	void add(unsigned long n);

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
	explicit ResponseState(Flag flag);
	virtual ~ResponseState();

public:
	Flag getFlag() const;
	State* getState() const;

public:
	void setState(std::auto_ptr<State> pState);

private:
	ResponseState(const ResponseState&);
	ResponseState& operator=(const ResponseState&);

private:
	Flag flag_;
	std::auto_ptr<State> pState_;
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
	ResponseStatus(const WCHAR* pwszMailbox,
				   StatusList& listStatus);
	virtual ~ResponseStatus();

public:
	const WCHAR* getMailbox() const;
	const StatusList& getStatusList() const;

public:
	static std::auto_ptr<ResponseStatus> create(const CHAR* pszMailbox,
												size_t nMailboxLen,
												List* pList);

private:
	ResponseStatus(const ResponseStatus&);
	ResponseStatus& operator=(const ResponseStatus&);

private:
	qs::wstring_ptr wstrMailbox_;
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
		TYPE_SIZE
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
	FetchDataBody(Section section,
				  PartPath& partPath,
				  FieldList& listField,
				  const TokenValue& content);
	virtual ~FetchDataBody();

public:
	Section getSection() const;
	const PartPath& getPartPath() const;
	const FieldList& getFieldList() const;
	const TokenValue& getContent() const;

public:
	static std::auto_ptr<FetchDataBody> create(const CHAR* pszSection,
											   size_t nSectionLen,
											   const TokenValue& content);

private:
	FetchDataBody(const FetchDataBody&);
	FetchDataBody& operator=(const FetchDataBody&);

private:
	Section section_;
	PartPath partPath_;
	FieldList listField_;
	TokenValue content_;
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
	FetchDataBodyStructure(qs::string_ptr strContentType,
						   qs::string_ptr strContentSubType,
						   ParamList& listContentTypeParam,
						   qs::string_ptr strId,
						   qs::string_ptr strDescription,
						   qs::string_ptr strEncoding,
						   unsigned long nSize,
						   unsigned long nLine,
						   qs::string_ptr strMd5,
						   qs::string_ptr strDisposition,
						   ParamList& listDispositionParam,
						   LanguageList& listLanguage,
						   std::auto_ptr<FetchDataEnvelope> pEnvelope,
						   ChildList& listChild);
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

public:
	static std::auto_ptr<FetchDataBodyStructure> create(List* pList,
														bool bExtended);

private:
	static std::auto_ptr<FetchDataBodyStructure> parseChild(ListItem* pListItem,
															bool bExtended);
	static bool parseParam(ListItem* pListItem,
						   ParamList* pListParam);
	static bool parseDisposition(ListItem* pListItem,
								 qs::string_ptr* pstrDisposition,
								 ParamList* pListParam);
	static bool parseLanguage(ListItem* pListItem,
							  LanguageList* pListLanguage);

private:
	FetchDataBodyStructure(const FetchDataBodyStructure&);
	FetchDataBodyStructure& operator=(const FetchDataBodyStructure&);

private:
	qs::string_ptr strContentType_;
	qs::string_ptr strContentSubType_;
	ParamList listContentTypeParam_;
	qs::string_ptr strId_;
	qs::string_ptr strDescription_;
	qs::string_ptr strEncoding_;
	unsigned long nSize_;
	unsigned long nLine_;
	qs::string_ptr strMd5_;
	qs::string_ptr strDisposition_;
	ParamList listDispositionParam_;
	LanguageList listLanguage_;
	std::auto_ptr<FetchDataEnvelope> pEnvelope_;
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
	FetchDataEnvelope(AddressList* pListAddress,
					  qs::string_ptr strDate,
					  qs::string_ptr strSubject,
					  qs::string_ptr strInReplyTo,
					  qs::string_ptr strMessageId);
	virtual ~FetchDataEnvelope();

public:
	const AddressList& getAddresses(Address address) const;
	const CHAR* getDate() const;
	const CHAR* getSubject() const;
	const CHAR* getMessageId() const;
	const CHAR* getInReplyTo() const;

public:
	static std::auto_ptr<FetchDataEnvelope> create(List* pList);

private:
	FetchDataEnvelope(const FetchDataEnvelope&);
	FetchDataEnvelope& operator=(const FetchDataEnvelope&);

private:
	AddressList listAddress_[6];
	qs::string_ptr strDate_;
	qs::string_ptr strSubject_;
	qs::string_ptr strInReplyTo_;
	qs::string_ptr strMessageId_;
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
	FetchDataFlags(unsigned int nSystemFlags,
				   FlagList& listCustomFlag);
	virtual ~FetchDataFlags();

public:
	unsigned int getSystemFlags() const;
	const FlagList& getCustomFlags() const;

public:
	static std::auto_ptr<FetchDataFlags> create(List* pList);

public:
	static Imap4::Flag parseFlag(const std::pair<const CHAR*, size_t>& flag,
								 bool bAllowRecent);

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
	FetchDataInternalDate(const qs::Time& time);
	virtual ~FetchDataInternalDate();

public:
	const qs::Time& getTime() const;

public:
	static std::auto_ptr<FetchDataInternalDate> create(const CHAR* pszDate,
													   size_t nDateLen);

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
	FetchDataSize(unsigned long nSize);
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
 * EnvelopeAddress
 *
 */

class EnvelopeAddress
{
public:
	EnvelopeAddress(qs::string_ptr strName,
					qs::string_ptr strDomain,
					qs::string_ptr strMailbox,
					qs::string_ptr strHost);
	~EnvelopeAddress();

public:
	const CHAR* getName() const;
	const CHAR* getDomain() const;
	const CHAR* getMailbox() const;
	const CHAR* getHost() const;

public:
	static std::auto_ptr<EnvelopeAddress> create(List* pList);

private:
	EnvelopeAddress(const EnvelopeAddress&);
	EnvelopeAddress& operator=(const EnvelopeAddress&);

private:
	qs::string_ptr strName_;
	qs::string_ptr strDomain_;
	qs::string_ptr strMailbox_;
	qs::string_ptr strHost_;
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
	ListItemNil();
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
	ListItemText(const TokenValue& text);
	virtual ~ListItemText();

public:
	const TokenValue& getText() const;
	qs::string_ptr getTextString();

private:
	ListItemText(const ListItemText&);
	ListItemText& operator=(const ListItemText&);

private:
	TokenValue text_;
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
	List();
	virtual ~List();

public:
	const ItemList& getList() const;

public:
	void add(std::auto_ptr<ListItem> pItem);

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
	State(Code code);
	~State();

public:
	Code getCode() const;
	const CHAR* getMessage() const;
	unsigned long getArgNumber() const;
	const List* getArgList() const;

public:
	void setMessage(qs::string_ptr str);
	void setArg(unsigned long n);
	void setArg(std::auto_ptr<List> pList);
	
private:
	State(const State&);
	State& operator=(const State&);

private:
	Code code_;
	qs::string_ptr strMessage_;
	unsigned long n_;
	std::auto_ptr<List> pList_;
};

}

#endif // __IMAP4_H__
