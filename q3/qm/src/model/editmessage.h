/*
 * $Id: editmessage.h,v 1.1.1.1 2003/04/29 08:07:31 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __EDITMESSAGE_H__
#define __EDITMESSAGE_H__

#include <qm.h>
#include <qmmessage.h>

#include <qs.h>

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
	virtual qs::QSTATUS messageSet(const EditMessageEvent& event) = 0;
	virtual qs::QSTATUS messageUpdate(const EditMessageEvent& event) = 0;
	virtual qs::QSTATUS accountChanged(const EditMessageEvent& event) = 0;
	virtual qs::QSTATUS fieldChanged(const EditMessageFieldEvent& event) = 0;
	virtual qs::QSTATUS attachmentsChanged(const EditMessageEvent& event) = 0;
	virtual qs::QSTATUS signatureChanged(const EditMessageEvent& event) = 0;
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
		FIELDTYPE_ADDRESSLIST
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
		bool operator()(const Attachment& lhs, const Attachment& rhs) const;
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
	EditMessage(Document* pDocument, Account* pAccount, qs::QSTATUS* pstatus);
	~EditMessage();

public:
	qs::QSTATUS getMessage(Message** ppMessage);
	qs::QSTATUS setMessage(Message* pMessage);

public:
	Document* getDocument() const;
	Account* getAccount() const;
	SubAccount* getSubAccount() const;
	qs::QSTATUS setAccount(Account* pAccount, SubAccount* pSubAccount);
	qs::QSTATUS getField(const WCHAR* pwszName,
		FieldType type, qs::WSTRING* pwstrValue);
	qs::QSTATUS setField(const WCHAR* pwszName,
		const WCHAR* pwszValue, FieldType type);
	const WCHAR* getBody() const;
	qs::QSTATUS setBody(const WCHAR* pwszBody);
	qs::QSTATUS getAttachments(AttachmentList* pList) const;
	qs::QSTATUS setAttachments(const AttachmentList& listAttachment);
	qs::QSTATUS addAttachment(const WCHAR* pwszPath);
	const WCHAR* getSignature() const;
	qs::QSTATUS setSignature(const WCHAR* pwszSignature);
	bool isAutoReform() const;
	void setAutoReform(bool bAutoReform);
	bool isEncrypt() const;
	void setEncrypt(bool bEncrypt);
	bool isSign() const;
	void setSign(bool bSign);

public:
	qs::QSTATUS addEditMessageHandler(EditMessageHandler* pHandler);
	qs::QSTATUS removeEditMessageHandler(EditMessageHandler* pHandler);

public:
	qs::QSTATUS getSignatureText(qs::WSTRING* pwstrText) const;

private:
	qs::QSTATUS fixup();
	void clear();
	qs::QSTATUS getBodyPart(qs::Part* pPart, qs::Part** ppPart) const;
	qs::QSTATUS removePart(qs::Part* pPart);
	qs::QSTATUS normalize(qs::Part* pPart);
	qs::QSTATUS makeMultipartMixed();
	qs::QSTATUS fireMessageSet();
	qs::QSTATUS fireMessageUpdate();
	qs::QSTATUS fireAccountChanged();
	qs::QSTATUS fireFieldChanged(const WCHAR* pwszName, const WCHAR* pwszValue);
	qs::QSTATUS fireAttachmentsChanged();
	qs::QSTATUS fireSignatureChanged();
	qs::QSTATUS fireEvent(const EditMessageEvent& event,
		qs::QSTATUS (EditMessageHandler::*pfn)(const EditMessageEvent&));

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
	Document* pDocument_;
	Account* pAccount_;
	SubAccount* pSubAccount_;
	Message* pMessage_;
	qs::Part* pBodyPart_;
	FieldList listField_;
	qs::WSTRING wstrBody_;
	AttachmentParser::AttachmentList listAttachment_;
	AttachmentPathList listAttachmentPath_;
	qs::WSTRING wstrSignature_;
	bool bAutoReform_;
	bool bEncrypt_;
	bool bSign_;
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
	virtual qs::QSTATUS messageSet(const EditMessageEvent& event);
	virtual qs::QSTATUS messageUpdate(const EditMessageEvent& event);
	virtual qs::QSTATUS accountChanged(const EditMessageEvent& event);
	virtual qs::QSTATUS fieldChanged(const EditMessageFieldEvent& event);
	virtual qs::QSTATUS attachmentsChanged(const EditMessageEvent& event);
	virtual qs::QSTATUS signatureChanged(const EditMessageEvent& event);

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
		const WCHAR* pwszName, const WCHAR* pwszValue);
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
	virtual qs::QSTATUS setEditMessage(EditMessage* pEditMessage) = 0;
	virtual void releaseEditMessage() = 0;
};

}

#endif // __EDITMESSAGE_H__
