/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __EDITMESSAGE_H__
#define __EDITMESSAGE_H__

#include <qm.h>
#include <qmmessage.h>

#include <qs.h>

#include "message.h"

namespace qm {

class EditMessage;
class EditMessageHandler;
	class DefaultEditMessageHandler;
class EditMessageEvent;
	class EditMessageFieldEvent;
class EditMessageHolder;

class Account;
class Document;
class Message;
class SubAccount;


/****************************************************************************
 *
 * EditMessageHandler
 *
 */

class EditMessageHandler
{
public:
	virtual ~EditMessageHandler();

public:
	virtual void messageSet(const EditMessageEvent& event) = 0;
	virtual void messageUpdate(const EditMessageEvent& event) = 0;
	virtual void accountChanged(const EditMessageEvent& event) = 0;
	virtual void fieldChanged(const EditMessageFieldEvent& event) = 0;
	virtual void attachmentsChanged(const EditMessageEvent& event) = 0;
	virtual void encodingChanged(const EditMessageEvent& event) = 0;
	virtual void signatureChanged(const EditMessageEvent& event) = 0;
};


/****************************************************************************
 *
 * EditMessage
 *
 */

class EditMessage
{
public:
	enum FieldType {
		FIELDTYPE_UNSTRUCTURED,
		FIELDTYPE_ADDRESSLIST,
		FIELDTYPE_REFERENCES
	};

public:
	struct Attachment
	{
		qs::WSTRING wstrName_;
		bool bNew_;
	};
	
	struct AttachmentComp :
		public std::binary_function<Attachment, Attachment, bool>
	{
		bool operator()(const Attachment& lhs,
						const Attachment& rhs) const;
	};

public:
	typedef std::vector<Attachment> AttachmentList;

public:
	struct AttachmentListFree
	{
		AttachmentListFree(AttachmentList& l);
		~AttachmentListFree();
		AttachmentList& l_;
	};

public:
	EditMessage(qs::Profile* pProfile,
				Document* pDocument,
				Account* pAccount,
				unsigned int nSecurityMode,
				const WCHAR* pwszTempDir);
	~EditMessage();

public:
	std::auto_ptr<Message> getMessage(bool bFixup);
	bool setMessage(std::auto_ptr<Message> pMessage);
	void update();
	const WCHAR* getPreviousURI() const;
	void setPreviousURI(const WCHAR* pwszURI);

public:
	Document* getDocument() const;
	Account* getAccount() const;
	SubAccount* getSubAccount() const;
	void setAccount(Account* pAccount,
					SubAccount* pSubAccount);
	qs::wstring_ptr getField(const WCHAR* pwszName,
							 FieldType type);
	void setField(const WCHAR* pwszName,
				  const WCHAR* pwszValue,
				  FieldType type);
	void removeField(const WCHAR* pwszName);
	qs::wxstring_ptr getBodyPartHeader();
	bool setBodyPartHeader(const WCHAR* pwszHeader,
						   size_t nLen);
	const WCHAR* getBodyPartBody() const;
	bool setBodyPartBody(const WCHAR* pwszBody,
						 size_t nLen);
	void getAttachments(AttachmentList* pList) const;
	void setAttachments(const AttachmentList& listAttachment);
	void addAttachment(const WCHAR* pwszPath);
	void removeAttachment(const WCHAR* pwszPath);
#ifdef QMZIP
	bool isArchiveAttachments() const;
	void setArchiveAttachments(bool bArchive);
	const WCHAR* getArchiveName() const;
	void setArchiveName(const WCHAR* pwszName);
#endif
	const WCHAR* getEncoding() const;
	void setEncoding(const WCHAR* pwszEncoding);
	const WCHAR* getSignature() const;
	void setSignature(const WCHAR* pwszSignature);
	bool isAutoReform() const;
	void setAutoReform(bool bAutoReform);
	unsigned int getMessageSecurity() const;
	void setMessageSecurity(MessageSecurity security,
							bool b);

public:
	void addEditMessageHandler(EditMessageHandler* pHandler);
	void removeEditMessageHandler(EditMessageHandler* pHandler);

public:
	qs::wstring_ptr getSignatureText() const;

private:
	void clear();
	bool applyFields();
	void clearFields();
	void removePart(qs::Part* pPart);
	void fireMessageSet();
	void fireMessageUpdate();
	void fireAccountChanged();
	void fireFieldChanged(const WCHAR* pwszName,
						  const WCHAR* pwszValue);
	void fireAttachmentsChanged();
	void fireEncodingChanged();
	void fireSignatureChanged();
	void fireEvent(const EditMessageEvent& event,
				   void (EditMessageHandler::*pfn)(const EditMessageEvent&));

private:
	static qs::Part* getBodyPart(qs::Part* pPart);
	static bool normalize(qs::Part* pPart);
	static std::auto_ptr<Message> makeMultipartMixed(std::auto_ptr<Message> pMessage);

private:
	EditMessage(const EditMessage&);
	EditMessage& operator=(const EditMessage&);

private:
	struct Field
	{
		qs::WSTRING wstrName_;
		qs::WSTRING wstrValue_;
		FieldType type_;
	};

private:
	typedef std::vector<Field> FieldList;
	typedef std::vector<qs::WSTRING> AttachmentPathList;
	typedef std::vector<EditMessageHandler*> HandlerList;

private:
	qs::Profile* pProfile_;
	Document* pDocument_;
	Account* pAccount_;
	SubAccount* pSubAccount_;
	unsigned int nSecurityMode_;
	qs::wstring_ptr wstrTempDir_;
	std::auto_ptr<Message> pMessage_;
	qs::Part* pBodyPart_;
	FieldList listField_;
	qs::wxstring_size_ptr wstrBody_;
	AttachmentParser::AttachmentList listAttachment_;
	AttachmentPathList listAttachmentPath_;
#ifdef QMZIP
	bool bArchiveAttachments_;
	qs::wstring_ptr wstrArchiveName_;
#endif
	qs::wstring_ptr wstrEncoding_;
	qs::wstring_ptr wstrSignature_;
	bool bAutoReform_;
	unsigned int nMessageSecurity_;
	qs::wstring_ptr wstrPreviousURI_;
	HandlerList listHandler_;
};


/****************************************************************************
 *
 * DefaultEditMessageHandler
 *
 */

class DefaultEditMessageHandler : public EditMessageHandler
{
public:
	DefaultEditMessageHandler();
	virtual ~DefaultEditMessageHandler();

public:
	virtual void messageSet(const EditMessageEvent& event);
	virtual void messageUpdate(const EditMessageEvent& event);
	virtual void accountChanged(const EditMessageEvent& event);
	virtual void fieldChanged(const EditMessageFieldEvent& event);
	virtual void attachmentsChanged(const EditMessageEvent& event);
	virtual void encodingChanged(const EditMessageEvent& event);
	virtual void signatureChanged(const EditMessageEvent& event);

private:
	DefaultEditMessageHandler(const DefaultEditMessageHandler&);
	DefaultEditMessageHandler& operator=(const DefaultEditMessageHandler&);
};


/****************************************************************************
 *
 * EditMessageEvent
 *
 */

class EditMessageEvent
{
public:
	EditMessageEvent(EditMessage* pEditMessage);
	~EditMessageEvent();

public:
	EditMessage* getEditMessage() const;

private:
	EditMessageEvent(const EditMessageEvent&);
	EditMessageEvent& operator=(const EditMessageEvent&);

private:
	EditMessage* pEditMessage_;
};


/****************************************************************************
 *
 * EditMessageFieldEvent
 *
 */

class EditMessageFieldEvent : public EditMessageEvent
{
public:
	EditMessageFieldEvent(EditMessage* pEditMessage,
						  const WCHAR* pwszName,
						  const WCHAR* pwszValue);
	~EditMessageFieldEvent();

public:
	const WCHAR* getName() const;
	const WCHAR* getValue() const;

private:
	EditMessageFieldEvent(const EditMessageFieldEvent&);
	EditMessageFieldEvent& operator=(const EditMessageFieldEvent&);

private:
	const WCHAR* pwszName_;
	const WCHAR* pwszValue_;
};


/****************************************************************************
 *
 * EditMessageHolder
 *
 */

class EditMessageHolder
{
public:
	virtual ~EditMessageHolder();

public:
	virtual EditMessage* getEditMessage() = 0;
	virtual bool setEditMessage(EditMessage* pEditMessage) = 0;
	virtual void releaseEditMessage() = 0;
};

}

#endif // __EDITMESSAGE_H__
