/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QSMIME_H__
#define __QSMIME_H__

#include <qs.h>
#include <qsstring.h>
#include <qsutil.h>

#include <vector>

#pragma warning(disable:4251)


namespace qs {

class Part;
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
		
		O_ALLOW_ENCODED_QSTRING					= 0x00000100,
		O_ALLOW_ENCODED_PARAMETER				= 0x00000200,
		O_ALLOW_PARAMETER_INVALID_SEMICOLON		= 0x00000400,
		O_ALLOW_ADDRESS_WITHOUT_DOMAIN			= 0x00000800,
		O_ALLOW_INCOMPLETE_MULTIPART			= 0x00001000,
		O_ALLOW_RAW_FIELD						= 0x00002000,
		O_ALLOW_SPECIALS_IN_REFERENCES			= 0x00004000,
		O_ALLOW_INVALID_PERIOD_IN_LOCALPART		= 0x00008000
	};
	
	enum Field {
		FIELD_EXIST,
		FIELD_NOTEXIST,
		FIELD_ERROR
	};

public:
	typedef std::vector<Part*> PartList;
	typedef std::vector<std::pair<STRING, STRING> > FieldList;

public:
	explicit Part(QSTATUS* pstatus);
	Part(unsigned int nOptions, QSTATUS* pstatus);
	Part(const Part* pParent, const CHAR* pszContent,
		size_t nLen, QSTATUS* pstatus);
	Part(const Part* pParent, const CHAR* pszContent,
		size_t nLen, unsigned int nOptions, QSTATUS* pstatus);
	virtual ~Part();

public:
	QSTATUS create(const Part* pParent, const CHAR* pszContent, size_t nLen);
	QSTATUS clear();
	QSTATUS clone(Part** ppPart) const;
	QSTATUS getContent(STRING* pstrContent) const;

public:
	const CHAR* getHeader() const;
	QSTATUS setHeader(const CHAR* pszHeader);
	QSTATUS getField(const WCHAR* pwszName,
		FieldParser* pParser, Field* pField) const;
	QSTATUS hasField(const WCHAR* pwszName, bool* pbHas) const;
	
	// Set field value.
	// If bAllowMultiple is true, add the specified value,
	// Otherwise if there are fields whose name is strName,
	// Do nothing and return false
	QSTATUS setField(const WCHAR* pwszName, const FieldParser& parser);
	QSTATUS setField(const WCHAR* pwszName,
		const FieldParser& parser, bool* pbSet);
	QSTATUS setField(const WCHAR* pwszName,
		const FieldParser& parser, bool bAllowMultiple);
	QSTATUS setField(const WCHAR* pwszName, const FieldParser& parser,
		bool bAllowMultiple, bool* pbSet);
	
	// Replace the specified field by the specified value provided by parser
	// If nIndex equals to 0xffffffff, remove all fields whose name is strName
	// and set the value. If there is no field which is speicifed by strName
	// and nIndex, replaceField behave like setField with 3rd argument be set to ture.
	QSTATUS replaceField(const WCHAR* pwszName, const FieldParser& parser);
	QSTATUS replaceField(const WCHAR* pwszName,
		const FieldParser& parser, unsigned int nIndex);
	
	// Remove the specified field
	// If nIndex equals to 0xffffffff, remove all fields whose name is strName
	QSTATUS removeField(const WCHAR* pwszName);
	QSTATUS removeField(const WCHAR* pwszName, bool* pbRemove);
	QSTATUS removeField(const WCHAR* pwszName, unsigned int nIndex);
	QSTATUS removeField(const WCHAR* pwszName,
		unsigned int nIndex, bool* pbRemove);
	
	QSTATUS getFields(FieldList* pListField) const;
	QSTATUS sortHeader();
	
	const ContentTypeParser* getContentType() const;
	QSTATUS getCharset(WSTRING* pwstrCharset) const;
	bool isMultipart() const;
	
	QSTATUS getRawField(const WCHAR* pwszName,
		unsigned int nIndex, STRING* pstrValue, bool* pbExist) const;
	QSTATUS getHeaderCharset(WSTRING* pwstrCharset) const;

public:
	const CHAR* getBody() const;
	QSTATUS setBody(const CHAR* pszBody, size_t nLen);
	QSTATUS getBodyText(WSTRING* pwstrBodyText) const;
	QSTATUS getBodyText(const WCHAR* pwszCharset, WSTRING* pwstrBodyText) const;
	QSTATUS getBodyData(unsigned char** ppData, size_t* pnLen) const;

public:
	const PartList& getPartList() const;
	size_t getPartCount() const;
	Part* getPart(unsigned int n) const;
	QSTATUS addPart(Part* pPart);
	QSTATUS insertPart(unsigned int n, Part* pPart);
	void removePart(Part* pPart);
	Part* getParentPart() const;

public:
	const Part* getEnclosedPart() const;
	void setEnclosedPart(Part* pPart);

public:
	bool isOption(Option option) const;
	void setOptions(unsigned int nOptions);

public:
	static const WCHAR* getDefaultCharset();
	static QSTATUS setDefaultCharset(const WCHAR* pwszCharset);
	static bool isGlobalOption(Option option);
	static void setGlobalOptions(unsigned int nOptions);

private:
	QSTATUS getHeaderLower(const CHAR** ppHeader) const;
	void clearHeaderLower() const;
	QSTATUS updateContentType();
	QSTATUS getFieldPos(const CHAR* pszName,
		unsigned int nIndex, CHAR** ppBegin) const;
	CHAR* getFieldEndPos(const CHAR* pBegin) const;

private:
	Part(const Part&);
	Part& operator=(const Part&);

private:
	STRING strHeader_;
	STRING strBody_;
	PartList listPart_;
	Part* pPartEnclosed_;
	Part* pParent_;
	unsigned int nOptions_;
	ContentTypeParser* pContentType_;
	mutable STRING strHeaderLower_;

private:
	static WSTRING wstrDefaultCharset__;
	static unsigned int nGlobalOptions__;
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
	virtual QSTATUS parse(const Part& part,
		const WCHAR* pwszName, Part::Field* pField) = 0;
	virtual QSTATUS unparse(const Part& part, STRING* pstrValue) const = 0;

public:
	static QSTATUS decode(const CHAR* psz, size_t nLen,
		WSTRING* pwstrDecoded, bool* pbDecode);
	static QSTATUS encode(const WCHAR* pwsz, size_t nLen,
		const WCHAR* pwszCharset, const WCHAR* pwszEncoding,
		bool bOneBlock, STRING* pstrEncoded);
	static QSTATUS getQString(const CHAR* psz,
		size_t nLen, STRING* pstrValue);
	static QSTATUS getAtomOrQString(const CHAR* psz,
		size_t nLen, STRING* pstrValue);
	static QSTATUS getAtomsOrQString(const CHAR* psz,
		size_t nLen, STRING* pstrValue);
	static bool isAscii(const WCHAR* pwsz);
	static bool isAscii(const WCHAR* pwsz, size_t nLen);
	static bool isNeedQuote(const CHAR* psz,
		size_t nLen, bool bQuoteWhitespace);
	static QSTATUS parseError();

private:
	static QSTATUS encodeLine(const WCHAR* pwsz, size_t nLen,
		const WCHAR* pwszCharset, Converter* pConverter,
		const WCHAR* pwszEncoding, Encoder* pEncoder,
		bool bOneBlock, STRING* pstrEncoded);
};


/****************************************************************************
 *
 * UnstructuredParser
 *
 */

class QSEXPORTCLASS UnstructuredParser : public FieldParser
{
public:
	UnstructuredParser(QSTATUS* pstatus);
	UnstructuredParser(const WCHAR* pwszValue,
		const WCHAR* pwszCharset, QSTATUS* pstatus);
	virtual ~UnstructuredParser();

public:
	const WCHAR* getValue() const;

public:
	virtual QSTATUS parse(const Part& part,
		const WCHAR* pwszName, Part::Field* pField);
	virtual QSTATUS unparse(const Part& part, STRING* pstrValue) const;

private:
	QSTATUS foldValue(const WCHAR* pwszValue, WSTRING* pwstrFolded) const;

private:
	static bool isFirstTokenEncode(const WCHAR* pwsz, size_t nLen);
	static bool isLastTokenEncode(const WCHAR* pwsz, size_t nLen);

private:
	UnstructuredParser(const UnstructuredParser&);
	UnstructuredParser& operator=(const UnstructuredParser&);

private:
	WSTRING wstrValue_;
	WSTRING wstrCharset_;
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
	DummyParser(const WCHAR* pwszValue, unsigned int nFlags, QSTATUS* pstatus);
	virtual ~DummyParser();

public:
	const WCHAR* getValue() const;

public:
	virtual QSTATUS parse(const Part& part,
		const WCHAR* pwszName, Part::Field* pField);
	virtual QSTATUS unparse(const Part& part, STRING* pstrValue) const;

private:
	bool isSpecial(WCHAR c) const;

private:
	DummyParser(const DummyParser&);
	DummyParser& operator=(const DummyParser&);

private:
	unsigned int nFlags_;
	WSTRING wstrValue_;
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
		unsigned int nFlags, QSTATUS* pstatus);
	virtual ~NoParseParser();

public:
	const WCHAR* getValue() const;

public:
	virtual QSTATUS parse(const Part& part,
		const WCHAR* pwszName, Part::Field* pField);
	virtual QSTATUS unparse(const Part& part, STRING* pstrValue) const;

private:
	NoParseParser(const NoParseParser&);
	NoParseParser& operator=(const NoParseParser&);

private:
	unsigned int nFlags_;
	STRING strSeparator_;
	WSTRING wstrValue_;
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
	SimpleParser(unsigned int nFlags, QSTATUS* pstatus);
	SimpleParser(const WCHAR* pwszValue, unsigned int nFlags, QSTATUS* pstatus);
	virtual ~SimpleParser();

public:
	const WCHAR* getValue() const;

public:
	virtual QSTATUS parse(const Part& part,
		const WCHAR* pwszName, Part::Field* pField);
	virtual QSTATUS unparse(const Part& part, STRING* pstrValue) const;

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
	WSTRING wstrValue_;
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
	NumberParser(unsigned int nFlags, QSTATUS* pstatus);
	NumberParser(unsigned int n, unsigned int nFlags, QSTATUS* pstatus);
	virtual ~NumberParser();

public:
	unsigned int getValue() const;

public:
	virtual QSTATUS parse(const Part& part,
		const WCHAR* pwszName, Part::Field* pField);
	virtual QSTATUS unparse(const Part& part, STRING* pstrValue) const;

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
	explicit DateParser(QSTATUS* pstatus);
	DateParser(const Time& date, QSTATUS* pstatus);
	virtual ~DateParser();

public:
	const Time& getTime() const;

public:
	virtual QSTATUS parse(const Part& part,
		const WCHAR* pwszName, Part::Field* pField);
	virtual QSTATUS unparse(const Part& part, STRING* pstrValue) const;

public:
	static QSTATUS parse(const CHAR* psz, size_t nLen, Time* pTime);

private:
	static QSTATUS getWeek(const CHAR* psz, int* pnWeek);
	static QSTATUS getDay(const CHAR* psz, int* pnDay);
	static QSTATUS getMonth(const CHAR* psz, int* pnMonth);
	static QSTATUS getYear(const CHAR* psz, int* pnYear);
	static QSTATUS getHour(const CHAR* psz, int* pnHour);
	static QSTATUS getMinute(const CHAR* psz, int* pnMinute);
	static QSTATUS getSecond(const CHAR* psz, int* pnSecond);
	static QSTATUS getTimeZone(const CHAR* psz, int* pnTimezone);
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
		FLAG_INGROUP	= 0x01
	};

private:
	typedef std::vector<std::pair<STRING, bool> > Phrases;

public:
	AddressParser(unsigned int nFlags, QSTATUS* pstatus);
	AddressParser(const WCHAR* pwszPhrase,
		const WCHAR* pwszAddress, QSTATUS* pstatus);
	AddressParser(const WCHAR* pwszPhrase, const WCHAR* pwszMailbox,
		const WCHAR* pwszHost, QSTATUS* pstatus);
	virtual ~AddressParser();

public:
	const WCHAR* getPhrase() const;
	const WCHAR* getMailbox() const;
	const WCHAR* getHost() const;
	AddressListParser* getGroup() const;
	QSTATUS getAddress(WSTRING* pwstrAddress) const;
	QSTATUS getValue(WSTRING* pwstrValue) const;

public:
	virtual QSTATUS parse(const Part& part,
		const WCHAR* pwszName, Part::Field* pField);
	virtual QSTATUS unparse(const Part& part, STRING* pstrValue) const;

public:
	static QSTATUS isNeedQuoteMailbox(const CHAR* pszMailbox, bool* pbNeed);
	static QSTATUS getAddrSpec(const CHAR* pszMailbox,
		const CHAR* pszHost, STRING* pstrAddrSpec);
	static QSTATUS getAddrSpec(const WCHAR* pwszMailbox,
		const WCHAR* pwszHost, STRING* pstrAddrSpec);

private:
	QSTATUS parseAddress(const Part& part, Tokenizer& t,
		Part::Field* pField, bool* pbEnd);

private:
	static QSTATUS decodePhrase(const CHAR* psz, bool bAtom,
		bool bAllowEncodedQString, WSTRING* pwstrDecoded, bool* pbDecode);
	static QSTATUS getMailboxFromPhrases(const Phrases& phrases,
		bool bAllowInvalidPeriod, STRING* pstrMailbox);

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
	WSTRING wstrPhrase_;
	WSTRING wstrMailbox_;
	WSTRING wstrHost_;
	AddressListParser* pGroup_;

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
		FLAG_SINGLEFIELD		= 0x01,
		FLAG_GROUP				= 0x02
	};

public:
	typedef std::vector<AddressParser*> AddressList;

public:
	AddressListParser(unsigned int nFlags, QSTATUS* pstatus);
	virtual ~AddressListParser();

public:
	QSTATUS getValue(WSTRING* pwstrValue) const;
	QSTATUS getNames(WSTRING* pwstrNames) const;
	QSTATUS getAddresses(WSTRING* pwstrAddresses) const;
	const AddressList& getAddressList() const;
	QSTATUS appendAddress(AddressParser* pAddress);
	QSTATUS insertAddress(AddressParser* pAddressRef,
		AddressParser* pAddress);
	QSTATUS removeAddress(AddressParser* pAddress);
	void removeAllAddresses();
	QSTATUS replaceAddress(AddressParser* pAddressOld,
		AddressParser* pAddressNew);

public:
	virtual QSTATUS parse(const Part& part,
		const WCHAR* pwszName, Part::Field* pField);
	virtual QSTATUS unparse(const Part& part, STRING* pstrValue) const;

private:
	QSTATUS parseAddressList(const Part& part,
		Tokenizer& t, Part::Field* pField);

private:
	AddressListParser(const AddressListParser&);
	AddressListParser& operator=(const AddressListParser&);

private:
	unsigned int nFlags_;
	AddressList listAddress_;

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
	explicit MessageIdParser(QSTATUS* pstatus);
	MessageIdParser(const WCHAR* pwszMessageId, QSTATUS* pstatus);
	virtual ~MessageIdParser();

public:
	const WCHAR* getMessageId() const;

public:
	virtual QSTATUS parse(const Part& part,
		const WCHAR* pwszName, Part::Field* pField);
	virtual QSTATUS unparse(const Part& part, STRING* pstrValue) const;

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
	WSTRING wstrMessageId_;
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
	explicit ReferencesParser(QSTATUS* pstatus);
	virtual ~ReferencesParser();

public:
	const ReferenceList& getReferences() const;

public:
	virtual QSTATUS parse(const Part& part,
		const WCHAR* pwszName, Part::Field* pField);
	virtual QSTATUS unparse(const Part& part, STRING* pstrValue) const;

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
	explicit ParameterFieldParser(QSTATUS* pstatus);
	virtual ~ParameterFieldParser();

public:
	QSTATUS getParameter(const WCHAR* pwszName, WSTRING* pwstrValue) const;
	QSTATUS setParameter(const WCHAR* pwszName, const WCHAR* pwszValue);

protected:
	QSTATUS parseParameter(const Part& part,
		Tokenizer& t, State state, Part::Field* pField);
	QSTATUS unparseParameter(const Part& part, STRING* pstrValue) const;

private:
	const WCHAR* getRawParameter(const WCHAR* pwszName) const;
	QSTATUS setRawParameter(const WCHAR* pwszName, const WCHAR* pwszValue);

public:
	static QSTATUS decode2231FirstValue(const WCHAR* pwszValue,
		STRING* pstrDecode, Converter** ppConverter);
	static QSTATUS decode2231Value(const WCHAR* pwszValue, STRING* pstrDecoded);
	static QSTATUS encode2231Value(const CHAR* pszValue,
		size_t nLen, STRING* pstrEncoded);
	static bool isHex(WCHAR c);
	static unsigned char decodeHex(const WCHAR* pwszValue);
	static QSTATUS encodeHex(unsigned char c, CHAR* pszEncoded);

private:
	ParameterFieldParser(const ParameterFieldParser&);
	ParameterFieldParser& operator=(const ParameterFieldParser&);

private:
	typedef std::vector<std::pair<WSTRING, WSTRING> > ParameterList;

private:
	ParameterList listParameter_;
};


/****************************************************************************
 *
 * SimpleParameterParser
 *
 */

class QSEXPORTCLASS SimpleParameterParser : public ParameterFieldParser
{
public:
	explicit SimpleParameterParser(QSTATUS* pstatus);
	virtual ~SimpleParameterParser();

public:
	const WCHAR* getValue() const;

public:
	virtual QSTATUS parse(const Part& part,
		const WCHAR* pwszName, Part::Field* pField);
	virtual QSTATUS unparse(const Part& part, STRING* pstrValue) const;

private:
	SimpleParameterParser(const SimpleParameterParser&);
	SimpleParameterParser& operator=(const SimpleParameterParser&);

private:
	WSTRING wstrValue_;
};


/****************************************************************************
 *
 * ContentTypeParser
 *
 */

class QSEXPORTCLASS ContentTypeParser : public ParameterFieldParser
{
public:
	explicit ContentTypeParser(QSTATUS* pstatus);
	ContentTypeParser(const WCHAR* pwszMediaType,
		const WCHAR* pwszSubType, QSTATUS* pstatus);
	virtual ~ContentTypeParser();

public:
	const WCHAR* getMediaType() const;
	const WCHAR* getSubType() const;

public:
	virtual QSTATUS parse(const Part& part,
		const WCHAR* pwszName, Part::Field* pField);
	virtual QSTATUS unparse(const Part& part, STRING* pstrValue) const;

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
	WSTRING wstrMediaType_;
	WSTRING wstrSubType_;
};


/****************************************************************************
 *
 * ContentDispositionParser
 *
 */

class QSEXPORTCLASS ContentDispositionParser : public ParameterFieldParser
{
public:
	explicit ContentDispositionParser(QSTATUS* pstatus);
	ContentDispositionParser(const WCHAR* pwszDispositionType, QSTATUS* pstatus);
	virtual ~ContentDispositionParser();

public:
	const WCHAR* getDispositionType() const;

public:
	virtual QSTATUS parse(const Part& part,
		const WCHAR* pwszName, Part::Field* pField);
	virtual QSTATUS unparse(const Part& part, STRING* pstrValue) const;

private:
	ContentDispositionParser(const ContentDispositionParser&);
	ContentDispositionParser& operator=(const ContentDispositionParser&);

private:
	WSTRING wstrDispositionType_;
};


/****************************************************************************
 *
 * ContentTransferEncodingParser
 *
 */

class QSEXPORTCLASS ContentTransferEncodingParser : public FieldParser
{
public:
	explicit ContentTransferEncodingParser(QSTATUS* pstatus);
	ContentTransferEncodingParser(const WCHAR* pwszEncoding, QSTATUS* pstatus);
	virtual ~ContentTransferEncodingParser();

public:
	const WCHAR* getEncoding() const;

public:
	virtual QSTATUS parse(const Part& part,
		const WCHAR* pwszName, Part::Field* pField);
	virtual QSTATUS unparse(const Part& part, STRING* pstrValue) const;

private:
	ContentTransferEncodingParser(const ContentTransferEncodingParser&);
	ContentTransferEncodingParser& operator=(const ContentTransferEncodingParser&);

private:
	SimpleParser parser_;
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
	BoundaryFinder(const Char* pszMessage, size_t nLen,
		const Char* pszBoundary, const Char* pszNewLine,
		bool bAllowIncomplete, QSTATUS* pstatus);
	~BoundaryFinder();

public:
	QSTATUS getNext(const Char** ppBegin, const Char** ppEnd, bool* pbEnd);

private:
	void getNextBoundary(const Char* p, size_t nLen,
		const Char** ppBegin, const Char** ppEnd);

private:
	BoundaryFinder(const BoundaryFinder&);
	BoundaryFinder& operator=(const BoundaryFinder&);

private:
	const Char* p_;
	size_t nLen_;
	BMFindString<String>* pFindString_;
	size_t nBoundaryLen_;
	const Char* pszNewLine_;
	bool bAllowIncomplete_;
};

}

#include <qsmime.inl>

#pragma warning(default:4251)

#endif // __QSMIME_H__
