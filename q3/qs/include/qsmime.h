/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QSMIME_H__
#define __QSMIME_H__

#include <qs.h>
#include <qsstl.h>
#include <qsstring.h>
#include <qsutil.h>

#include <vector>

#pragma warning(disable:4251)


namespace qs {

class Part;
class FieldFilter;
	class PrefixFieldFilter;
class FieldParser;
	class UnstructuredParser;
	class DummyParser;
	class NoParseParser;
	class SimpleParser;
	class NumberParser;
	class DateParser;
	class AddressParser;
	class AddressListParser;
	class MessageIdParser;
	class ReferencesParser;
	class ParameterFieldParser;
		class SimpleParameterParser;
		class ContentTypeParser;
		class ContentDispositionParser;
	class ContentTransferEncodingParser;
template<class String> class FieldParserUtil;
template<class Char, class String> class BoundaryFinder;

class Tokenizer;
class Encoder;
class Converter;


/****************************************************************************
 *
 * Part
 *
 */

class QSEXPORTCLASS Part
{
public:
	enum Option {
		O_RFC2231								= 0x00000001,
		O_USE_COMMENT_AS_PHRASE					= 0x00000002,
		O_FORCE_QSTRING_PARAMETER				= 0x00000004,
		O_INTERPRET_FORMAT_FLOWED				= 0x00000008,
		
		O_ALLOW_ENCODED_QSTRING					= 0x00000100,
		O_ALLOW_ENCODED_PARAMETER				= 0x00000200,
		O_ALLOW_PARAMETER_INVALID_SEMICOLON		= 0x00000400,
		O_ALLOW_ADDRESS_WITHOUT_DOMAIN			= 0x00000800,
		O_ALLOW_INCOMPLETE_MULTIPART			= 0x00001000,
		O_ALLOW_RAW_FIELD						= 0x00002000,
		O_ALLOW_SPECIALS_IN_REFERENCES			= 0x00004000,
		O_ALLOW_INVALID_PERIOD_IN_LOCALPART		= 0x00008000,
		O_ALLOW_SINGLE_DIGIT_TIME				= 0x00010000,
		O_ALLOW_DATE_WITH_RUBBISH				= 0x00020000,
		O_ALLOW_RAW_PARAMETER					= 0x00040000,
		O_ALLOW_USE_DEFAULT_ENCODING			= 0x00080000
	};
	
	enum Field {
		FIELD_EXIST,
		FIELD_NOTEXIST,
		FIELD_ERROR
	};
	
	enum Format {
		FORMAT_NONE,
		FORMAT_FLOWED,
		FORMAT_FLOWED_DELSP
	};

public:
	typedef std::vector<Part*> PartList;
	typedef std::vector<std::pair<STRING, STRING> > FieldList;

public:
	class QSEXPORTCLASS FieldListFree
	{
	public:
		FieldListFree(FieldList& l);
		~FieldListFree();
	
	public:
		void free();
	
	private:
		FieldListFree(const FieldListFree&);
		FieldListFree& operator=(const FieldListFree&);
	
	private:
		FieldList& l_;
	};

public:
	Part();
	explicit Part(unsigned int nOptions);
	virtual ~Part();

public:
	bool create(const Part* pParent,
				const CHAR* pszContent,
				size_t nLen);
	void clear();
	std::auto_ptr<Part> clone() const;
	xstring_size_ptr getContent() const;
	bool getContent(XStringBuffer<XSTRING>* pBuf) const;

public:
	const CHAR* getHeader() const;
	bool setHeader(const CHAR* pszHeader);
	Field getField(const WCHAR* pwszName,
				   FieldParser* pParser) const;
	bool hasField(const WCHAR* pwszName) const;
	
	// Set field value.
	// If bAllowMultiple is true, add the specified value,
	// Otherwise if there are fields whose name is strName,
	// Do nothing and return false
	bool setField(const WCHAR* pwszName,
				  const FieldParser& parser);
	bool setField(const WCHAR* pwszName,
				  const FieldParser& parser,
				  bool* pbSet);
	bool setField(const WCHAR* pwszName,
				  const FieldParser& parser,
				  bool bAllowMultiple);
	bool setField(const WCHAR* pwszName,
				  const FieldParser& parser,
				  bool bAllowMultiple,
				  bool* pbSet);
	
	// Replace the specified field by the specified value provided by parser
	// If nIndex equals to 0xffffffff, remove all fields whose name is strName
	// and set the value. If there is no field which is speicifed by strName
	// and nIndex, replaceField behave like setField with 3rd argument be set to ture.
	bool replaceField(const WCHAR* pwszName,
					  const FieldParser& parser);
	bool replaceField(const WCHAR* pwszName,
					  const FieldParser& parser,
					  unsigned int nIndex);
	
	// Remove the specified field
	// If nIndex equals to 0xffffffff, remove all fields whose name is strName
	bool removeField(const WCHAR* pwszName);
	bool removeField(const WCHAR* pwszName,
					 unsigned int nIndex);
	
	void getFields(FieldList* pListField) const;
	bool copyFields(const Part& part,
					FieldFilter* pFilter);
	bool removeFields(FieldFilter* pFilter);
	bool sortHeader();
	
	const ContentTypeParser* getContentType() const;
	wstring_ptr getCharset() const;
	bool isMultipart() const;
	bool isText() const;
	bool isAttachment() const;
	Format getFormat() const;
	
	string_ptr getRawField(const WCHAR* pwszName,
						   unsigned int nIndex) const;
	wstring_ptr getHeaderCharset() const;

public:
	const CHAR* getBody() const;
	bool setBody(const CHAR* pszBody,
				 size_t nLen);
	void setBody(xstring_ptr strBody);
	wxstring_size_ptr getBodyText() const;
	wxstring_size_ptr getBodyText(const WCHAR* pwszCharset) const;
	bool getBodyText(const WCHAR* pwszCharset,
					 XStringBuffer<WXSTRING>* pBuf) const;
	malloc_size_ptr<unsigned char> getBodyData() const;

public:
	const PartList& getPartList() const;
	size_t getPartCount() const;
	Part* getPart(unsigned int n) const;
	void addPart(std::auto_ptr<Part> pPart);
	void insertPart(unsigned int n,
					std::auto_ptr<Part> pPart);
	void removePart(Part* pPart);
	Part* getParentPart() const;

public:
	Part* getEnclosedPart() const;
	void setEnclosedPart(std::auto_ptr<Part> pPart);

public:
	bool isOption(Option option) const;
	void setOptions(unsigned int nOptions);

public:
	static const WCHAR* getDefaultCharset();
	static void setDefaultCharset(const WCHAR* pwszCharset);
	static bool isGlobalOption(Option option);
	static void setGlobalOptions(unsigned int nOptions);
	static size_t getMaxHeaderLength();
	static void setMaxHeaderLength(size_t nMax);
	static size_t getMaxPartCount();
	static void setMaxPartCount(size_t nMax);

public:
	static const CHAR* getBody(const CHAR* pszContent,
							   size_t nLen);

private:
	bool create(const Part* pParent,
				const CHAR* pszContent,
				size_t nLen,
				size_t* pnMaxPartCount);
	const CHAR* getHeaderLower() const;
	void clearHeaderLower() const;
	void updateContentType();
	CHAR* getFieldPos(const CHAR* pszName,
					  unsigned int nIndex) const;
	CHAR* getFieldEndPos(const CHAR* pBegin) const;

private:
	static bool interpretFlowedFormat(const WCHAR* pwszText,
									  bool bDelSp,
									  XStringBuffer<WXSTRING>* pBuf);

private:
	Part(const Part&);
	Part& operator=(const Part&);

private:
	xstring_ptr strHeader_;
	xstring_ptr strBody_;
	PartList listPart_;
	std::auto_ptr<Part> pPartEnclosed_;
	Part* pParent_;
	unsigned int nOptions_;
	std::auto_ptr<ContentTypeParser> pContentType_;
	mutable xstring_ptr strHeaderLower_;

private:
	static wstring_ptr wstrDefaultCharset__;
	static unsigned int nGlobalOptions__;
	static size_t nMaxHeaderLength__;
	static size_t nMaxPartCount__;
};


/****************************************************************************
 *
 * FieldFilter
 *
 */

class QSEXPORTCLASS FieldFilter
{
public:
	virtual ~FieldFilter();

public:
	virtual bool accept(const CHAR* pszName) = 0;
};


/****************************************************************************
 *
 * PrefixFieldFilter
 *
 */

class QSEXPORTCLASS PrefixFieldFilter : public FieldFilter
{
public:
	explicit PrefixFieldFilter(const CHAR* pszPrefix);
	PrefixFieldFilter(const CHAR* pszPrefix,
					  bool bNot);
	virtual ~PrefixFieldFilter();

public:
	virtual bool accept(const CHAR* pszName);

private:
	PrefixFieldFilter(const PrefixFieldFilter&);
	PrefixFieldFilter& operator=(const PrefixFieldFilter&);

private:
	const CHAR* pszPrefix_;
	size_t nLen_;
	bool bNot_;
};


/****************************************************************************
 *
 * FieldParser
 *
 */

class QSEXPORTCLASS FieldParser
{
public:
	enum {
		FOLD_LENGTH				= 72,
		ENCODE_CHAR_LENGTH		= 3,
		ENCODE_MARKER_LENGTH	= 20
	};

public:
	virtual ~FieldParser();
	
public:
	virtual Part::Field parse(const Part& part,
							  const WCHAR* pwszName) = 0;
	virtual string_ptr unparse(const Part& part) const = 0;

public:
	static wstring_ptr decode(const CHAR* psz,
							  size_t nLen,
							  bool bAllowUTF8,
							  bool* pbDecoded);
	static string_ptr encode(const WCHAR* pwsz,
							 size_t nLen,
							 const WCHAR* pwszCharset,
							 const WCHAR* pwszEncoding,
							 bool bOneBlock);
	static string_ptr convertToUTF8(const CHAR* psz);
	static bool isSpecial(CHAR c);
	static Part::Field parseError();

private:
	static string_ptr encodeLine(const WCHAR* pwsz,
								 size_t nLen,
								 const WCHAR* pwszCharset,
								 Converter* pConverter,
								 const WCHAR* pwszEncoding,
								 Encoder* pEncoder,
								 bool bOneBlock);
};


/****************************************************************************
 *
 * UnstructuredParser
 *
 */

class QSEXPORTCLASS UnstructuredParser : public FieldParser
{
public:
	UnstructuredParser();
	UnstructuredParser(const WCHAR* pwszValue);
	UnstructuredParser(const WCHAR* pwszValue,
					   const WCHAR* pwszCharset);
	virtual ~UnstructuredParser();

public:
	const WCHAR* getValue() const;

public:
	virtual Part::Field parse(const Part& part,
							  const WCHAR* pwszName);
	virtual string_ptr unparse(const Part& part) const;

private:
	wstring_ptr foldValue(const WCHAR* pwszValue) const;

private:
	static bool isFirstTokenEncode(const WCHAR* pwsz,
								   size_t nLen);
	static bool isLastTokenEncode(const WCHAR* pwsz,
								  size_t nLen);
	static bool isRawValue(const CHAR* psz);

private:
	UnstructuredParser(const UnstructuredParser&);
	UnstructuredParser& operator=(const UnstructuredParser&);

private:
	wstring_ptr wstrValue_;
	wstring_ptr wstrCharset_;
};


/****************************************************************************
 *
 * DummyParser
 *
 */

class QSEXPORTCLASS DummyParser : public FieldParser
{
public:
	enum Flag {
		FLAG_TSPECIAL	= 0x01,
		FLAG_ESPECIAL	= 0x02
	};

public:
	DummyParser(const WCHAR* pwszValue,
				unsigned int nFlags);
	virtual ~DummyParser();

public:
	virtual Part::Field parse(const Part& part,
							  const WCHAR* pwszName);
	virtual string_ptr unparse(const Part& part) const;

private:
	bool isSpecial(WCHAR c) const;

private:
	DummyParser(const DummyParser&);
	DummyParser& operator=(const DummyParser&);

private:
	unsigned int nFlags_;
	wstring_ptr wstrValue_;
};


/****************************************************************************
 *
 * UTF8Parser
 *
 */

class QSEXPORTCLASS UTF8Parser : public FieldParser
{
public:
	explicit UTF8Parser(const WCHAR* pwszValue);
	virtual ~UTF8Parser();

public:
	virtual Part::Field parse(const Part& part,
							  const WCHAR* pwszName);
	virtual string_ptr unparse(const Part& part) const;

private:
	UTF8Parser(const UTF8Parser&);
	UTF8Parser& operator=(const UTF8Parser&);

private:
	wstring_ptr wstrValue_;
};


/****************************************************************************
 *
 * NoParseParser
 *
 */

class QSEXPORTCLASS NoParseParser : public FieldParser
{
public:
	enum Flag {
		FLAG_SINGLEFIELD	= 0x01
	};

public:
	NoParseParser(const WCHAR* pwszSeparator,
				  unsigned int nFlags);
	virtual ~NoParseParser();

public:
	const WCHAR* getValue() const;

public:
	virtual Part::Field parse(const Part& part,
							  const WCHAR* pwszName);
	virtual string_ptr unparse(const Part& part) const;

private:
	NoParseParser(const NoParseParser&);
	NoParseParser& operator=(const NoParseParser&);

private:
	unsigned int nFlags_;
	string_ptr strSeparator_;
	wstring_ptr wstrValue_;
};


/****************************************************************************
 *
 * SimpleParser
 *
 */

class QSEXPORTCLASS SimpleParser : public FieldParser
{
public:
	enum Flag {
		FLAG_RECOGNIZECOMMENT	= 0x01,
		FLAG_TSPECIAL			= 0x02,
		FLAG_ACCEPTQSTRING		= 0x04,
		FLAG_DECODE				= 0x08,
		FLAG_ENCODE				= 0x10
	};

public:
	explicit SimpleParser(unsigned int nFlags);
	SimpleParser(const WCHAR* pwszValue,
				 unsigned int nFlags);
	virtual ~SimpleParser();

public:
	const WCHAR* getValue() const;

public:
	virtual Part::Field parse(const Part& part,
							  const WCHAR* pwszName);
	virtual string_ptr unparse(const Part& part) const;

private:
	SimpleParser(const SimpleParser&);
	SimpleParser& operator=(const SimpleParser&);

private:
	enum State {
		S_BEGIN,
		S_ATOM,
		S_END
	};

private:
	unsigned int nFlags_;
	wstring_ptr wstrValue_;
};


/****************************************************************************
 *
 * NumberParser
 *
 */

class QSEXPORTCLASS NumberParser : public FieldParser
{
public:
	enum Flag {
		FLAG_RECOGNIZECOMMENT	= 0x01,
		FLAG_HEX				= 0x02
	};

public:
	explicit NumberParser(unsigned int nFlags);
	NumberParser(unsigned int n,
				 unsigned int nFlags);
	virtual ~NumberParser();

public:
	unsigned int getValue() const;

public:
	virtual Part::Field parse(const Part& part,
							  const WCHAR* pwszName);
	virtual string_ptr unparse(const Part& part) const;

private:
	NumberParser(const NumberParser&);
	NumberParser& operator=(const NumberParser&);

private:
	enum State {
		S_BEGIN,
		S_ATOM,
		S_END
	};

private:
	unsigned int nFlags_;
	unsigned int n_;
};


/****************************************************************************
 *
 * DateParser
 *
 */

class QSEXPORTCLASS DateParser : public FieldParser
{
public:
	DateParser();
	DateParser(const Time& date);
	virtual ~DateParser();

public:
	const Time& getTime() const;

public:
	virtual Part::Field parse(const Part& part,
							  const WCHAR* pwszName);
	virtual string_ptr unparse(const Part& part) const;

public:
	static bool parse(const CHAR* psz,
					  size_t nLen,
					  bool bAllowSingleDigitTime,
					  bool bAllowRubbish,
					  Time* pTime);
	static wstring_ptr unparse(const Time& time);

private:
	static int getWeek(const CHAR* psz);
	static int getDay(const CHAR* psz);
	static int getMonth(const CHAR* psz);
	static int getYear(const CHAR* psz);
	static int getHour(const CHAR* psz,
					   bool bAllowSingleDigit);
	static int getMinute(const CHAR* psz,
						 bool bAllowSingleDigit);
	static int getSecond(const CHAR* psz,
						 bool bAllowSingleDigit);
	static int getTimeZone(const CHAR* psz);
	static bool isDigit(const CHAR* psz);

private:
	DateParser(const DateParser&);
	DateParser& operator=(const DateParser&);

private:
	enum State {
		S_BEGIN,
		S_FIRST,
		S_WEEK,
		S_DAY,
		S_MONTH,
		S_YEAR,
		S_HOUR,
		S_HOURSEP,
		S_MINUTE,
		S_MINUTESEP,
		S_SECOND,
		S_TIMEZONE,
		S_END
	};

private:
	Time date_;
};


/****************************************************************************
 *
 * AddressParser
 *
 */

class QSEXPORTCLASS AddressParser : public FieldParser
{
public:
	enum Flag {
		FLAG_DISALLOWGROUP	= 0x01,
		FLAG_INGROUP		= 0x03,
		FLAG_ALLOWUTF8		= 0x04
	};

private:
	typedef std::vector<std::pair<STRING, bool> > Phrases;

public:
	AddressParser();
	explicit AddressParser(unsigned int nFlags);
	AddressParser(const WCHAR* pwszPhrase,
				  const WCHAR* pwszAddress);
	AddressParser(const WCHAR* pwszPhrase,
				  const WCHAR* pwszMailbox,
				  const WCHAR* pwszHost);
	virtual ~AddressParser();

public:
	const WCHAR* getPhrase() const;
	const WCHAR* getMailbox() const;
	const WCHAR* getHost() const;
	AddressListParser* getGroup() const;
	wstring_ptr getAddress() const;
	wstring_ptr getValue() const;
	wstring_ptr getValue(bool bAutoQuote) const;
	void setPhrase(const WCHAR* pwszPhrase);

public:
	virtual Part::Field parse(const Part& part,
							  const WCHAR* pwszName);
	virtual string_ptr unparse(const Part& part) const;

public:
	static bool isNeedQuoteMailbox(const CHAR* pszMailbox);
	static string_ptr getAddrSpec(const CHAR* pszMailbox,
								  const CHAR* pszHost);
	static string_ptr getAddrSpec(const WCHAR* pwszMailbox,
								  const WCHAR* pwszHost);

private:
	Part::Field parseAddress(const Part& part,
							 Tokenizer& t,
							 bool* pbEnd);
	wstring_ptr convertMailbox(const CHAR* pszMailbox);

private:
	static wstring_ptr decodePhrase(const CHAR* psz,
									bool bAtom,
									bool bAllowEncodedQString,
									bool bAllowUTF8,
									bool* pbDecoded);
	static string_ptr getMailboxFromPhrases(const Phrases& phrases,
											bool bAllowInvalidPeriod);

private:
	AddressParser(const AddressParser&);
	AddressParser& operator=(const AddressParser&);

private:
	enum State {
		S_BEGIN,
		S_PHRASE,
		S_LEFTANGLE,
		S_RIGHTANGLE,
		S_ROUTEAT,
		S_ROUTEDOMAIN,
		S_ROUTECANMA,
		S_ROUTECOLON,
		S_SEMICOLON,
		S_END
	};

private:
	unsigned int nFlags_;
	wstring_ptr wstrPhrase_;
	wstring_ptr wstrMailbox_;
	wstring_ptr wstrHost_;
	std::auto_ptr<AddressListParser> pGroup_;

friend class AddressListParser;
};


/****************************************************************************
 *
 * AddressListParser
 *
 */

class QSEXPORTCLASS AddressListParser : public FieldParser
{
public:
	enum Flag{
		FLAG_SINGLEFIELD	= 0x01,
		FLAG_DISALLOWGROUP	= 0x02,
		FLAG_GROUP			= 0x06,
		FLAG_ALLOWUTF8		= 0x08
	};

public:
	typedef std::vector<AddressParser*> AddressList;

public:
	AddressListParser();
	explicit AddressListParser(unsigned int nFlags);
	AddressListParser(unsigned int nFlags,
					  size_t nMax);
	virtual ~AddressListParser();

public:
	wstring_ptr getValue() const;
	wstring_ptr getValue(bool bAutoQuote) const;
	wstring_ptr getNames() const;
	wstring_ptr getAddresses() const;
	const AddressList& getAddressList() const;
	void appendAddress(std::auto_ptr<AddressParser> pAddress);
	void insertAddress(AddressParser* pAddressRef,
					   std::auto_ptr<AddressParser> pAddress);
	void removeAddress(AddressParser* pAddress);
	void removeAllAddresses();
	void replaceAddress(AddressParser* pAddressOld,
						std::auto_ptr<AddressParser> pAddressNew);

public:
	virtual Part::Field parse(const Part& part,
							  const WCHAR* pwszName);
	virtual string_ptr unparse(const Part& part) const;

public:
	static size_t getMaxAddresses();
	static void setMaxAddresses(size_t nMax);

private:
	Part::Field parseAddressList(const Part& part,
								 Tokenizer& t);

private:
	AddressListParser(const AddressListParser&);
	AddressListParser& operator=(const AddressListParser&);

private:
	unsigned int nFlags_;
	AddressList listAddress_;
	size_t nMax_;

private:
	static size_t nMax__;

friend class AddressParser;
};


/****************************************************************************
 *
 * MessageIdParser
 *
 */

class QSEXPORTCLASS MessageIdParser : public FieldParser
{
public:
	MessageIdParser();
	explicit MessageIdParser(const WCHAR* pwszMessageId);
	virtual ~MessageIdParser();

public:
	const WCHAR* getMessageId() const;

public:
	virtual Part::Field parse(const Part& part,
							  const WCHAR* pwszName);
	virtual string_ptr unparse(const Part& part) const;

private:
	MessageIdParser(const MessageIdParser&);
	MessageIdParser& operator=(const MessageIdParser&);

private:
	enum State {
		S_BEGIN,
		S_ADDRSPEC,
		S_END
	};

private:
	wstring_ptr wstrMessageId_;
};


/****************************************************************************
 *
 * ReferencesParser
 *
 */

class QSEXPORTCLASS ReferencesParser : public FieldParser
{
public:
	enum Type {
		T_MSGID,
		T_PHRASE
	};
	
public:
	typedef std::vector<std::pair<WSTRING, Type> > ReferenceList;

public:
	ReferencesParser();
	explicit ReferencesParser(size_t nMax);
	virtual ~ReferencesParser();

public:
	const ReferenceList& getReferences() const;
	wstring_ptr getValue() const;

public:
	virtual Part::Field parse(const Part& part,
							  const WCHAR* pwszName);
	virtual string_ptr unparse(const Part& part) const;

public:
	static size_t getMaxReferences();
	static void setMaxReferences(size_t nMax);

private:
	ReferencesParser(const ReferencesParser&);
	ReferencesParser& operator=(const ReferencesParser&);

private:
	enum State {
		S_BEGIN,
		S_PHRASE,
		S_END
	};

private:
	ReferenceList listReference_;
	size_t nMax_;

private:
	static size_t nMax__;
};


/****************************************************************************
 *
 * ParameterFieldParser
 *
 */

class QSEXPORTCLASS ParameterFieldParser : public FieldParser
{
protected:
	enum State {
		S_BEGIN,
		S_SEMICOLON,
		S_NAME,
		S_EQUAL,
		S_VALUE,
		S_END
	};

public:
	explicit ParameterFieldParser(size_t nMax);
	virtual ~ParameterFieldParser();

public:
	wstring_ptr getParameter(const WCHAR* pwszName) const;
	void setParameter(const WCHAR* pwszName,
					  const WCHAR* pwszValue);

protected:
	Part::Field parseParameter(const Part& part,
							   Tokenizer& t,
							   State state);
	string_ptr unparseParameter(const Part& part) const;

private:
	const WCHAR* getRawParameter(const WCHAR* pwszName) const;
	void setRawParameter(const WCHAR* pwszName,
						 const WCHAR* pwszValue);

public:
	static string_ptr decode2231FirstValue(const WCHAR* pwszValue,
										   std::auto_ptr<Converter>* ppConverter);
	static string_ptr decode2231Value(const WCHAR* pwszValue);
	static string_ptr encode2231Value(const CHAR* pszValue,
									  size_t nLen);
	static bool isHex(WCHAR c);
	static unsigned char decodeHex(const WCHAR* pwszValue);
	static void encodeHex(unsigned char c, CHAR* pszEncoded);

public:
	static size_t getMaxParameters();
	static void setMaxParameters(size_t nMax);

private:
	ParameterFieldParser(const ParameterFieldParser&);
	ParameterFieldParser& operator=(const ParameterFieldParser&);

private:
	typedef std::vector<std::pair<WSTRING, WSTRING> > ParameterList;

private:
	ParameterList listParameter_;
	size_t nMax_;

private:
	static size_t nMax__;
};


/****************************************************************************
 *
 * SimpleParameterParser
 *
 */

class QSEXPORTCLASS SimpleParameterParser : public ParameterFieldParser
{
public:
	SimpleParameterParser();
	explicit SimpleParameterParser(size_t nMax);
	virtual ~SimpleParameterParser();

public:
	const WCHAR* getValue() const;

public:
	virtual Part::Field parse(const Part& part,
							  const WCHAR* pwszName);
	virtual string_ptr unparse(const Part& part) const;

private:
	SimpleParameterParser(const SimpleParameterParser&);
	SimpleParameterParser& operator=(const SimpleParameterParser&);

private:
	wstring_ptr wstrValue_;
};


/****************************************************************************
 *
 * ContentTypeParser
 *
 */

class QSEXPORTCLASS ContentTypeParser : public ParameterFieldParser
{
public:
	ContentTypeParser();
	explicit ContentTypeParser(size_t nMax);
	ContentTypeParser(const WCHAR* pwszMediaType,
					  const WCHAR* pwszSubType);
	virtual ~ContentTypeParser();

public:
	const WCHAR* getMediaType() const;
	const WCHAR* getSubType() const;

public:
	virtual Part::Field parse(const Part& part,
							  const WCHAR* pwszName);
	virtual string_ptr unparse(const Part& part) const;

private:
	ContentTypeParser(const ContentTypeParser&);
	ContentTypeParser& operator=(const ContentTypeParser&);

private:
	enum State {
		S_BEGIN,
		S_MEDIATYPE,
		S_SLASH,
		S_END
	};

private:
	wstring_ptr wstrMediaType_;
	wstring_ptr wstrSubType_;
};


/****************************************************************************
 *
 * ContentDispositionParser
 *
 */

class QSEXPORTCLASS ContentDispositionParser : public ParameterFieldParser
{
public:
	ContentDispositionParser();
	explicit ContentDispositionParser(size_t nMax);
	explicit ContentDispositionParser(const WCHAR* pwszDispositionType);
	virtual ~ContentDispositionParser();

public:
	const WCHAR* getDispositionType() const;

public:
	virtual Part::Field parse(const Part& part,
							  const WCHAR* pwszName);
	virtual string_ptr unparse(const Part& part) const;

private:
	ContentDispositionParser(const ContentDispositionParser&);
	ContentDispositionParser& operator=(const ContentDispositionParser&);

private:
	wstring_ptr wstrDispositionType_;
};


/****************************************************************************
 *
 * ContentTransferEncodingParser
 *
 */

class QSEXPORTCLASS ContentTransferEncodingParser : public FieldParser
{
public:
	ContentTransferEncodingParser();
	explicit ContentTransferEncodingParser(const WCHAR* pwszEncoding);
	virtual ~ContentTransferEncodingParser();

public:
	const WCHAR* getEncoding() const;

public:
	virtual Part::Field parse(const Part& part,
							  const WCHAR* pwszName);
	virtual string_ptr unparse(const Part& part) const;

private:
	ContentTransferEncodingParser(const ContentTransferEncodingParser&);
	ContentTransferEncodingParser& operator=(const ContentTransferEncodingParser&);

private:
	SimpleParser parser_;
};


/****************************************************************************
 *
 * FieldParserUtil
 *
 */

template<class String>
class FieldParserUtil
{
public:
	typedef typename StringTraits<String>::char_type Char;

public:
	static bool isAscii(const Char* psz);
	static bool isAscii(const Char* psz,
						size_t nLen);
	static basic_string_ptr<String> getQString(const Char* psz,
											   size_t nLen);
	static basic_string_ptr<String> getAtomOrQString(const Char* psz,
													 size_t nLen);
	static basic_string_ptr<String> getAtomsOrQString(const Char* psz,
													  size_t nLen);
	static bool isNeedQuote(const Char* psz,
							size_t nLen,
							bool bQuoteWhitespace);
	static basic_string_ptr<String> resolveQuotedPairs(const Char* psz,
													   size_t nLen);
};


/****************************************************************************
 *
 * BoundaryFinder
 *
 */

template<class Char, class String>
class BoundaryFinder
{
public:
	BoundaryFinder(const Char* pszMessage,
				   size_t nLen,
				   const Char* pszBoundary,
				   const Char* pszNewLine,
				   bool bAllowIncomplete);
	~BoundaryFinder();

public:
	bool getNext(const Char** ppBegin,
				 const Char** ppEnd,
				 bool* pbEnd);

private:
	void getNextBoundary(const Char* p,
						 size_t nLen,
						 const Char** ppBegin,
						 const Char** ppEnd);

private:
	BoundaryFinder(const BoundaryFinder&);
	BoundaryFinder& operator=(const BoundaryFinder&);

private:
	const Char* p_;
	size_t nLen_;
	std::auto_ptr<BMFindString<String> > pFindString_;
	size_t nBoundaryLen_;
	const Char* pszNewLine_;
	bool bAllowIncomplete_;
};

}

#include <qsmime.inl>

#pragma warning(default:4251)

#endif // __QSMIME_H__
