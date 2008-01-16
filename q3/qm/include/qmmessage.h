/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#ifndef __QMMESSAGE_H__
#define __QMMESSAGE_H__

#include <qm.h>

#include <qsmime.h>

#include <vector>
#include <utility>


namespace qm {

class Message;
class MessageCreator;
class PartUtil;
class AttachmentParser;

class Account;
class Signature;
class SignatureManager;
class URIResolver;


/****************************************************************************
 *
 * Message
 *
 */

#pragma warning(push)
#pragma warning(disable:4251)

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
	
	enum Security {
		SECURITY_NONE					= 0x0000,
		
		SECURITY_DECRYPTED				= 0x0001,
		SECURITY_DECRYPT_MASK			= 0x00ff,
		
		SECURITY_VERIFIED				= 0x0100,
		SECURITY_VERIFICATIONFAILED		= 0x0200,
		SECURITY_ADDRESSNOTMATCH		= 0x0400,
		SECURITY_ADDRESSNOTMATCHNOERROR	= 0x0800,
		SECURITY_VERIFY_MASK			= 0xff00
	};

public:
	Message();
	virtual ~Message();

public:
	bool create(const CHAR* pszMessage,
				size_t nLen,
				Flag flag);
	bool create(const CHAR* pszMessage,
				size_t nLen,
				Flag flag,
				unsigned int nSecurity);
	bool createHeader(const CHAR* pszHeader,
					  size_t nLen);
	void clear();
	Flag getFlag() const;
	void setFlag(Flag flag);
	unsigned int getSecurity() const;
	void setSecurity(unsigned int nSecurity);
	const WCHAR* getParam(const WCHAR* pwszName) const;
	void setParam(const WCHAR* pwszName,
				  const WCHAR* pwszValue);
	bool removePrivateFields();

private:
	Message(const Message&);
	Message& operator=(const Message&);

private:
	typedef std::vector<std::pair<qs::WSTRING, qs::WSTRING> > ParamList;

private:
	Flag flag_;
	unsigned int nSecurity_;
	ParamList listParam_;
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
		FLAG_ADDCONTENTTYPE				= 0x0001,
		FLAG_ADDNODEFAULTCONTENTTYPE	= 0x0002,
		FLAG_EXPANDALIAS				= 0x0004,
		FLAG_EXTRACTATTACHMENT			= 0x0008,
		FLAG_ENCODETEXT					= 0x0010,
		FLAG_ADDSIGNATURE				= 0x0020,
		FLAG_RECOVERHEADER				= 0x0100,
		FLAG_RECOVERBODY				= 0x0200,
		FLAG_RECOVER					= 0x0300
	};
	
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
	
	enum {
		MAX_LINE_LENGTH = 998
	};

public:
	typedef std::vector<qs::WSTRING> AttachmentList;
	typedef std::vector<const WCHAR*> FileNameList;

public:
	MessageCreator();
	MessageCreator(unsigned int nFlags,
				   unsigned int nSecurityMode);
	MessageCreator(unsigned int nFlags,
				   unsigned int nSecurityMode,
				   const WCHAR* pwszTransferEncodingFor8Bit);
	MessageCreator(unsigned int nFlags,
				   unsigned int nSecurityMode,
				   const WCHAR* pwszTransferEncodingFor8Bit,
				   const URIResolver* pURIResolver,
				   SignatureManager* pSignatureManager,
				   Account* pAccount,
				   const WCHAR* pwszArchiveAttachmentExcludePattern,
				   const WCHAR* pwszTempDir);
	~MessageCreator();

public:
	unsigned int getFlags() const;
	void setFlags(unsigned int nFlags,
				  unsigned int nMask);
	std::auto_ptr<Message> createMessage(const WCHAR* pwszMessage,
									     size_t nLen) const;
	std::auto_ptr<qs::Part> createPart(const WCHAR* pwszMessage,
									   size_t nLen,
									   qs::Part* pParent,
									   bool bMessage) const;
	bool createHeader(qs::Part* pPart,
					  const WCHAR* pwszMessage,
					  size_t nLen) const;

private:
	qs::xstring_size_ptr convertBody(qs::Converter* pConverter,
									 const WCHAR* pwszBody,
									 size_t nBodyLen) const;
	std::auto_ptr<qs::Part> createPartWithSignature(const qs::Part* pPart,
													qs::Part* pParent,
													const Signature* pSignature) const;
	MessageCreator getCreatorForChild() const;

public:
	static bool setField(qs::Part* pPart,
						 const WCHAR* pwszName,
						 const WCHAR* pwszValue,
						 FieldType type);
	static bool getAddressList(const WCHAR* pwszAddresses,
							   qs::AddressListParser* pAddressList);
	static bool getAddress(const WCHAR* pwszAddress,
						   qs::AddressParser* pAddress);
	static bool makeMultipart(qs::Part* pParentPart,
							  std::auto_ptr<qs::Part> pPart);
	static bool attachFilesOrURIs(qs::Part* pPart,
								  const AttachmentList& l,
								  const URIResolver* pURIResolver,
								  unsigned int nSecurityMode,
								  const WCHAR* pwszArchiveName,
								  const WCHAR* pwszExcludePattern,
								  const WCHAR* pwszTempDir);
	static bool attachFileOrURI(qs::Part* pPart,
								const WCHAR* pwszFileOrURI,
								const URIResolver* pURIResolver,
								unsigned int nSecurityMode);
#ifdef QMZIP
	static bool attachArchivedFile(qs::Part* pPart,
								   const WCHAR* pwszFileName,
								   const FileNameList& l,
								   const WCHAR* pwszTempDir);
#endif
	static std::auto_ptr<qs::Part> createPartFromFile(qs::Part* pParentPart,
													  const WCHAR* pwszPath);
	static std::auto_ptr<qs::Part> createRfc822Part(const qs::Part& part,
													bool bHeaderOnly);
	static std::auto_ptr<qs::Part> createClonedPart(const qs::Part& part);
	static qs::wstring_ptr getContentTypeFromExtension(const WCHAR* pwszExtension);
	static const WCHAR* getEncodingForCharset(const WCHAR* pwszCharset);
	static bool isAttachmentURI(const WCHAR* pwszAttachment);

private:
	MessageCreator(const MessageCreator&);
	MessageCreator& operator=(const MessageCreator&);

private:
	unsigned int nFlags_;
	unsigned int nSecurityMode_;
	qs::wstring_ptr wstrTransferEncodingFor8Bit_;
	const URIResolver* pURIResolver_;
	SignatureManager* pSignatureManager_;
	Account* pAccount_;
	qs::wstring_ptr wstrArchiveAttachmentExcludePattern_;
	qs::wstring_ptr wstrTempDir_;
};

#pragma warning(pop)


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
	
	enum Rfc822Mode {
		RFC822_AUTO,
		RFC822_INLINE,
		RFC822_ATTACHMENT
	};

public:
	typedef std::vector<qs::WSTRING> ReferenceList;
	typedef std::vector<Message*> MessageList;
	typedef std::vector<const qs::ContentTypeParser*> ContentTypeList;

public:
	PartUtil(const qs::Part& part);
	~PartUtil();

public:
	bool isResent() const;
	qs::wstring_ptr getNames(const WCHAR* pwszField) const;
	qs::wstring_ptr getReference() const;
	void getReferences(ReferenceList* pList) const;
	qs::wxstring_size_ptr getAllText(const WCHAR* pwszQuote,
									 const WCHAR* pwszCharset,
									 bool bBodyOnly) const;
	bool getAllText(const WCHAR* pwszQuote,
					const WCHAR* pwszCharset,
					bool bBodyOnly,
					qs::XStringBuffer<qs::WXSTRING>* pBuf) const;
	qs::wstring_ptr getAllTextCharset() const;
	qs::wxstring_size_ptr getBodyText(const WCHAR* pwszQuote,
									  const WCHAR* pwszCharset,
									  Rfc822Mode rfc822Mode) const;
	bool getBodyText(const WCHAR* pwszQuote,
					 const WCHAR* pwszCharset,
					 Rfc822Mode rfc822Mode,
					 qs::XStringBuffer<qs::WXSTRING>* pBuf) const;
	qs::wstring_ptr getBodyTextCharset(Rfc822Mode rfc822Mode) const;
	qs::wxstring_size_ptr getFormattedText(bool bUseSendersTimeZone,
										   const WCHAR* pwszCharset,
										   Rfc822Mode rfc822Mode) const;
	bool getFormattedText(bool bUseSendersTimeZone,
						  const WCHAR* pwszCharset,
						  Rfc822Mode rfc822Mode,
						  qs::XStringBuffer<qs::WXSTRING>* pBuf) const;
	bool getDigest(MessageList* pList) const;
	DigestMode getDigestMode() const;
	qs::string_ptr getHeader(const WCHAR* pwszName) const;
	void getAlternativeContentTypes(ContentTypeList* pList) const;
	const qs::Part* getAlternativePart(const WCHAR* pwszMediaType,
									   const WCHAR* pwszSubType) const;
	const qs::Part* getPartByContentId(const WCHAR* pwszContentId) const;
	const qs::Part* getEnclosingPart(const qs::Part* pCandidatePart) const;
	qs::Part* getEnclosingPart(qs::Part* pCandidatePart) const;
	bool copyContentFields(qs::Part* pPart) const;

public:
	static qs::wxstring_ptr a2w(const CHAR* psz);
	static qs::wxstring_ptr a2w(const CHAR* psz,
								size_t nLen);
	static bool a2w(const CHAR* psz,
					size_t nLen,
					qs::XStringBuffer<qs::WXSTRING>* pBuf);
	static qs::xstring_ptr w2a(const WCHAR* pwsz);
	static qs::xstring_ptr w2a(const WCHAR* pwsz,
							   size_t nLen);
	static bool w2a(const WCHAR* pwsz,
					size_t nLen,
					qs::XStringBuffer<qs::XSTRING>* pBuf);
	static bool normalizeHeader(const CHAR* pszHeader,
								qs::XStringBuffer<qs::WXSTRING>* pBuf);
	static qs::wxstring_ptr quote(const WCHAR* pwsz,
								  const WCHAR* pwszQuote);
	static bool quote(const WCHAR* pwsz,
					  const WCHAR* pwszQuote,
					  qs::XStringBuffer<qs::WXSTRING>* pBuf);
	static qs::wstring_ptr expandNames(const WCHAR** ppwszNames,
									   unsigned int nCount);
	static bool isContentType(const qs::ContentTypeParser* pContentType,
							  const WCHAR* pwszMediaType,
							  const WCHAR* pwszSubType);
	static bool isMultipart(const qs::ContentTypeParser* pContentType);

private:
	static bool isRfc822Rendered(Rfc822Mode rfc822Mode,
								 bool bAttachment);

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
	enum Result {
		RESULT_OK,
		RESULT_FAIL,
		RESULT_CANCEL
	};
	
	enum GetAttachmentsFlag {
		GAF_NONE				= 0x00,
		GAF_INCLUDEDELETED		= 0x01,
		GAF_INCLUDEAPPLEFILE	= 0x02,
		GAF_INCLUDERFC822		= 0x04
	};

public:
	typedef std::vector<std::pair<qs::WSTRING, qs::Part*> > AttachmentList;

public:
	class DetachCallback
	{
	public:
		virtual ~DetachCallback();
	
	public:
		virtual qs::wstring_ptr confirmOverwrite(const WCHAR* pwszPath) = 0;
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
	bool hasAttachment() const;
	qs::wstring_ptr getName() const;
	void getAttachments(unsigned int nFlags,
						AttachmentList* pList) const;
	Result detach(const WCHAR* pwszDir,
				  const WCHAR* pwszName,
				  bool bAddZoneId,
				  DetachCallback* pCallback,
				  qs::wstring_ptr* pwstrPath) const;
	bool isAttachmentDeleted() const;

public:
	static void removeAttachments(qs::Part* pPart);
	static void setAttachmentDeleted(qs::Part* pPart);

private:
	AttachmentParser(const AttachmentParser&);
	AttachmentParser& operator=(const AttachmentParser&);

private:
	const qs::Part& part_;
};

}

#endif // __QMMESSAGE_H__
