/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QMMESSAGE_H__
#define __QMMESSAGE_H__

#include <qm.h>

#include <qsmime.h>

#include <vector>
#include <utility>


namespace qm {

/****************************************************************************
 *
 * Message
 *
 */

class QMEXPORTCLASS Message : public qs::Part
{
public:
	enum Flag {
		FLAG_EMPTY,
		FLAG_NONE,
		FLAG_HEADERONLY,
		FLAG_TEXTONLY,
		FLAG_HTMLONLY,
		FLAG_TEMPORARY
	};

public:
	explicit Message(qs::QSTATUS* pstatus);
	Message(const CHAR* pwszMessage, size_t nLen,
		Flag flag, qs::QSTATUS* pstatus);
	virtual ~Message();

public:
	qs::QSTATUS create(const CHAR* pszMessage, size_t nLen, Flag flag);
	qs::QSTATUS clear();
	Flag getFlag() const;
	void setFlag(Flag flag);

private:
	Message(const Message&);
	Message& operator=(const Message&);

private:
	Flag flag_;
};


/****************************************************************************
 *
 * MessageCreator
 *
 */

class QMEXPORTCLASS MessageCreator
{
public:
	enum Flag {
		FLAG_EXTRACTATTACHMENT	= 0x01,
		FLAG_ADDCONTENTTYPE		= 0x02,
		FLAG_EXPANDALIAS		= 0x04
	};

public:
	enum FieldType {
		FIELDTYPE_ADDRESSLIST,
		FIELDTYPE_CONTENTTYPE,
		FIELDTYPE_CONTENTTRANSFERENCODING,
		FIELDTYPE_CONTENTDISPOSITION,
		FIELDTYPE_MESSAGEID,
		FIELDTYPE_REFERENCES,
		FIELDTYPE_XQMAILATTACHMENT,
		FIELDTYPE_SINGLEUNSTRUCTURED,
		FIELDTYPE_MULTIUNSTRUCTURED
	};

public:
	MessageCreator();
	MessageCreator(unsigned int nFlags);
	~MessageCreator();

public:
	unsigned int getFlags() const;
	void setFlags(unsigned int nFlags, unsigned int nMask);
	qs::QSTATUS createMessage(const WCHAR* pwszMessage,
		size_t nLen, Message** ppMessage) const;
	qs::QSTATUS createPart(const WCHAR* pwszMessage, size_t nLen,
		qs::Part* pParent, bool bMessage, qs::Part** ppPart) const;
	qs::QSTATUS createHeader(qs::Part* pPart,
		const WCHAR* pwszMessage, size_t nLen) const;

private:
	qs::QSTATUS convertBody(qs::Converter* pConverter, const WCHAR* pwszBody,
		size_t nBodyLen, qs::STRING* pstrBody, size_t* pnLen) const;

public:
	static qs::QSTATUS setField(qs::Part* pPart, const WCHAR* pwszName,
		const WCHAR* pwszValue, FieldType type);
	static qs::QSTATUS makeMultipart(qs::Part* pParentPart, qs::Part* pPart);
	static qs::QSTATUS createPartFromFile(const WCHAR* pwszPath, qs::Part** ppPart);
	static qs::QSTATUS getContentTypeFromExtension(
		const WCHAR* pwszExtension, qs::WSTRING* pwstrContentType);

private:
	MessageCreator(const MessageCreator&);
	MessageCreator& operator=(const MessageCreator&);

private:
	unsigned int nFlags_;
};


/****************************************************************************
 *
 * PartUtil
 *
 */

class QMEXPORTCLASS PartUtil
{
public:
	enum DigestMode {
		DIGEST_NONE,
		DIGEST_MULTIPART,
		DIGEST_RFC1153
	};

public:
	typedef std::vector<qs::WSTRING> ReferenceList;
	typedef std::vector<Message*> MessageList;
	typedef std::vector<const qs::ContentTypeParser*> ContentTypeList;

public:
	PartUtil(const qs::Part& part);
	~PartUtil();

public:
	qs::QSTATUS isResent(bool* pbResent) const;
	bool isMultipart() const;
	bool isText() const;
	qs::QSTATUS isAttachment(bool* pbAttachment) const;
	qs::QSTATUS getNames(const WCHAR* pwszField, qs::WSTRING* pwstrNames) const;
	qs::QSTATUS getReference(qs::WSTRING* pwstrReference) const;
	qs::QSTATUS getReferences(ReferenceList* pList) const;
	qs::QSTATUS getAllText(const WCHAR* pwszQuote, const WCHAR* pwszCharset,
		bool bBodyOnly, qs::WSTRING* pwstrText) const;
	qs::QSTATUS getBodyText(const WCHAR* pwszQuote,
		const WCHAR* pwszCharset, qs::WSTRING* pwstrText) const;
	qs::QSTATUS getFormattedText(bool bUseSendersTimeZone,
		const WCHAR* pwszCharset, qs::WSTRING* pwstrText) const;
	qs::QSTATUS getDigest(MessageList* pList) const;
	qs::QSTATUS getDigestMode(DigestMode* pMode) const;
	qs::QSTATUS getHeader(const WCHAR* pwszName, qs::STRING* pstrHeader) const;
	qs::QSTATUS getAlternativeContentTypes(ContentTypeList* pList) const;
	qs::QSTATUS getAlternativePart(const WCHAR* pwszMediaType,
		const WCHAR* pwszSubType, const qs::Part** ppPart) const;
	qs::QSTATUS getPartByContentId(const WCHAR* pwszContentId,
		const qs::Part** ppPart) const;
	qs::Part* getEnclosingPart(qs::Part* pCandidatePart) const;
	qs::QSTATUS copyContentFields(qs::Part* pPart) const;

public:
	static qs::QSTATUS a2w(const CHAR* psz, qs::WSTRING* pwstr);
	static qs::QSTATUS a2w(const CHAR* psz, size_t nLen, qs::WSTRING* pwstr);
	static qs::QSTATUS w2a(const WCHAR* pwsz, qs::STRING* pstr);
	static qs::QSTATUS w2a(const WCHAR* pwsz, size_t nLen, qs::STRING* pstr);
	static qs::QSTATUS quote(const WCHAR* pwsz,
		const WCHAR* pwszQuote, qs::WSTRING* pwstrQuoted);
	static qs::QSTATUS expandNames(const WCHAR** ppwszNames,
		unsigned int nCount, qs::WSTRING* pwstrNames);
	static bool isContentType(const qs::ContentTypeParser* pContentType,
		const WCHAR* pwszMediaType, const WCHAR* pwszSubType);

private:
	const qs::Part& part_;
};


/****************************************************************************
 *
 * AttachmentParser
 *
 */

class QMEXPORTCLASS AttachmentParser
{
public:
	typedef std::vector<std::pair<qs::WSTRING, qs::Part*> > AttachmentList;

public:
	class DetachCallback
	{
	public:
		virtual ~DetachCallback();
	
	public:
		virtual qs::QSTATUS confirmOverwrite(
			const WCHAR* pwszPath, qs::WSTRING* pwstrPath) = 0;
	};
	
	struct AttachmentListFree
	{
		AttachmentListFree(AttachmentList& l);
		~AttachmentListFree();
		void free();
		AttachmentList& l_;
	};

public:
	AttachmentParser(const qs::Part& part);
	~AttachmentParser();

public:
	qs::QSTATUS hasAttachment(bool* pbHas) const;
	qs::QSTATUS getName(qs::WSTRING* pwstrName) const;
	qs::QSTATUS getAttachments(AttachmentList* pList) const;
	qs::QSTATUS detach(const WCHAR* pwszDir, const WCHAR* pwszName,
		DetachCallback* pCallback, qs::WSTRING* pwstrPath) const;
	qs::QSTATUS remove(Message* pMessage) const;

private:
	AttachmentParser(const AttachmentParser&);
	AttachmentParser& operator=(const AttachmentParser&);

private:
	const qs::Part& part_;
};

}

#endif // __QMMESSAGE_H__
