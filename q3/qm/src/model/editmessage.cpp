/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmaccount.h>
#include <qmapplication.h>
#include <qmdocument.h>
#include <qmmessage.h>

#include <qsstl.h>
#include <qstextutil.h>

#include <algorithm>

#include "editmessage.h"
#include "message.h"
#include "signature.h"

#pragma warning(disable:4786)

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * EditMessage
 *
 */

qm::EditMessage::EditMessage(Profile* pProfile,
							 Document* pDocument,
							 Account* pAccount) :
	pProfile_(pProfile),
	pDocument_(pDocument),
	pAccount_(pAccount),
	pSubAccount_(pAccount->getCurrentSubAccount()),
	pMessage_(0),
	pBodyPart_(0),
	wstrBody_(0),
	wstrSignature_(0),
	bAutoReform_(true),
	bEncrypt_(false),
	bSign_(false)
{
	bAutoReform_ = pProfile_->getInt(L"EditWindow", L"AutoReform", 1) != 0;
}

qm::EditMessage::~EditMessage()
{
	clear();
}

Message* qm::EditMessage::getMessage()
{
	if (!fixup())
		return 0;
	return pMessage_.get();
}

bool qm::EditMessage::setMessage(std::auto_ptr<Message> pMessage)
{
	assert(pMessage.get());
	
	clear();
	
	pBodyPart_ = getBodyPart(pMessage.get());
	if (pBodyPart_)
		wstrBody_ = pBodyPart_->getBodyText();
	else
		wstrBody_ = allocWXString(L"");
	if (!wstrBody_.get())
		return false;
	
	UnstructuredParser account;
	if (pMessage->getField(L"X-QMAIL-Account", &account) == Part::FIELD_EXIST) {
		Account* pAccount = pDocument_->getAccount(account.getValue());
		if (pAccount && pAccount != pAccount_) {
			pAccount_ = pAccount;
			pSubAccount_ = pAccount->getCurrentSubAccount();
		}
	}
	UnstructuredParser subaccount;
	if (pMessage->getField(L"X-QMAIL-SubAccount", &subaccount) == Part::FIELD_EXIST) {
		SubAccount* pSubAccount = pAccount_->getSubAccount(subaccount.getValue());
		if (pSubAccount)
			pSubAccount_ = pSubAccount;
	}
	const WCHAR* pwszFields[] = {
		L"X-QMAIL-Account",
		L"X-QMAIL-SubAccount"
	};
	for (int n = 0; n < countof(pwszFields); ++n)
		pMessage->removeField(pwszFields[n]);
	
	SignatureManager* pSignatureManager = pDocument_->getSignatureManager();
	const Signature* pSignature = 0;
	UnstructuredParser signature;
	if (pMessage->getField(L"X-QMAIL-Signature", &signature) == Part::FIELD_EXIST) {
		if (*signature.getValue())
			pSignature = pSignatureManager->getSignature(
				pAccount_, signature.getValue());
	}
	else {
		pSignature = pSignatureManager->getDefaultSignature(pAccount_);
	}
	if (pSignature)
		wstrSignature_ = allocWString(pSignature->getName());
	
	AttachmentParser attachment(*pMessage.get());
	attachment.getAttachments(true, &listAttachment_);
	
	pMessage_ = pMessage;
	
	fireMessageSet();
	
	return true;
}

void qm::EditMessage::update()
{
	fireMessageUpdate();
}

Document* qm::EditMessage::getDocument() const
{
	return pDocument_;
}

Account* qm::EditMessage::getAccount() const
{
	return pAccount_;
}

SubAccount* qm::EditMessage::getSubAccount() const
{
	return pSubAccount_;
}

void qm::EditMessage::setAccount(Account* pAccount,
								 SubAccount* pSubAccount)
{
	assert(pAccount);
	assert(pSubAccount);
	
	if (pAccount != pAccount_ || pSubAccount != pSubAccount_) {
		pAccount_ = pAccount;
		pSubAccount_ = pSubAccount;
		
		fireAccountChanged();
	}
}

wstring_ptr qm::EditMessage::getField(const WCHAR* pwszName,
									  FieldType type)
{
	assert(pwszName);
	
	wstring_ptr wstrValue;
	
	FieldList::iterator it = std::find_if(
		listField_.begin(), listField_.end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				string_equal<WCHAR>(),
				mem_data_ref(&Field::wstrName_),
				std::identity<const WCHAR*>()),
			pwszName));
	if (it != listField_.end()) {
		wstrValue = allocWString((*it).wstrValue_);
	}
	else {
		switch (type) {
		case FIELDTYPE_UNSTRUCTURED:
			{
				UnstructuredParser field;
				if (pMessage_->getField(pwszName, &field) == Part::FIELD_EXIST)
					wstrValue = allocWString(field.getValue());
			}
			break;
		case FIELDTYPE_ADDRESSLIST:
			{
				AddressListParser field(0);
				if (pMessage_->getField(pwszName, &field) == Part::FIELD_EXIST)
					wstrValue = field.getValue();
			}
			break;
		default:
			assert(false);
			break;
		}
	}
	
	return wstrValue;
}

void qm::EditMessage::setField(const WCHAR* pwszName,
							   const WCHAR* pwszValue,
							   FieldType type)
{
	assert(pwszName);
	assert(pwszValue);
	
	bool bChange = true;
	
	wstring_ptr wstrValue(allocWString(pwszValue));
	
	FieldList::iterator it = std::find_if(
		listField_.begin(), listField_.end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				string_equal<WCHAR>(),
				mem_data_ref(&Field::wstrName_),
				std::identity<const WCHAR*>()),
			pwszName));
	if (it != listField_.end()) {
		if (wcscmp((*it).wstrValue_, wstrValue.get()) == 0) {
			bChange = false;
		}
		else {
			freeWString((*it).wstrValue_);
			(*it).wstrValue_ = wstrValue.release();
			(*it).type_ = type;
		}
	}
	else {
		wstring_ptr wstrName(allocWString(pwszName));
		Field field = {
			wstrName.get(),
			wstrValue.get(),
			type
		};
		listField_.push_back(field);
		wstrName.release();
		wstrValue.release();
		
		pMessage_->removeField(pwszName);
	}
	
	if (bChange)
		fireFieldChanged(pwszName, pwszValue);
}

wxstring_ptr qm::EditMessage::getHeader()
{
	return PartUtil::a2w(pMessage_->getHeader());
}

bool qm::EditMessage::setHeader(const WCHAR* pwszHeader,
								size_t nLen)
{
	assert(pwszHeader);
	
	Message msg;
	if (!MessageCreator().createHeader(&msg, pwszHeader, nLen))
		return false;
	
	if (!pMessage_->setHeader(msg.getHeader()))
		return false;
	
	clearFields();
	
	return true;
}

const WCHAR* qm::EditMessage::getBody() const
{
	return wstrBody_.get();
}

bool qm::EditMessage::setBody(const WCHAR* pwszBody)
{
	wstrBody_ = allocWXString(pwszBody);
	return wstrBody_.get() != 0;
}

void qm::EditMessage::getAttachments(AttachmentList* pList) const
{
	assert(pList);
	
	pList->reserve(pList->size() + listAttachment_.size() + listAttachmentPath_.size());
	
	for (AttachmentPathList::const_iterator itP = listAttachmentPath_.begin(); itP != listAttachmentPath_.end(); ++itP) {
		wstring_ptr wstrName(allocWString(*itP));
		Attachment attachment = {
			wstrName.release(),
			true
		};
		pList->push_back(attachment);
	}
	
	for (AttachmentParser::AttachmentList::const_iterator itA = listAttachment_.begin(); itA != listAttachment_.end(); ++itA) {
		wstring_ptr wstrName(allocWString((*itA).first));
		Attachment attachment = {
			wstrName.release(),
			false
		};
		pList->push_back(attachment);
	}
}

void qm::EditMessage::setAttachments(const AttachmentList& listAttachment)
{
	std::for_each(listAttachmentPath_.begin(),
		listAttachmentPath_.end(), string_free<WSTRING>());
	listAttachmentPath_.clear();
	
	listAttachmentPath_.reserve(listAttachment_.size());
	
	for (AttachmentList::const_iterator itA = listAttachment.begin(); itA != listAttachment.end(); ++itA) {
		if ((*itA).bNew_) {
			wstring_ptr wstrPath(allocWString((*itA).wstrName_));
			listAttachmentPath_.push_back(wstrPath.release());
		}
	}
	
	for (AttachmentParser::AttachmentList::iterator itO = listAttachment_.begin(); itO != listAttachment_.end(); ) {
		AttachmentList::const_iterator it = listAttachment.begin();
		while (it != listAttachment.end()) {
			if (!(*it).bNew_ && wcscmp((*it).wstrName_, (*itO).first) == 0)
				break;
			++it;
		}
		
		if (it == listAttachment.end()) {
			removePart((*itO).second);
			freeWString((*itO).first);
			itO = listAttachment_.erase(itO);
		}
		else {
			++itO;
		}
	}
	
	fireAttachmentsChanged();
}

void qm::EditMessage::addAttachment(const WCHAR* pwszPath)
{
	wstring_ptr wstrPath(allocWString(pwszPath));
	listAttachmentPath_.push_back(wstrPath.get());
	wstrPath.release();
	
	fireAttachmentsChanged();
}

void qm::EditMessage::removeAttachment(const WCHAR* pwszPath)
{
	AttachmentPathList::iterator itP = std::find_if(
		listAttachmentPath_.begin(), listAttachmentPath_.end(),
		std::bind2nd(string_equal<WCHAR>(), pwszPath));
	if (itP != listAttachmentPath_.end()) {
		freeWString(*itP);
		listAttachmentPath_.erase(itP);
	}
	else {
		AttachmentParser::AttachmentList::iterator itO = std::find_if(
			listAttachment_.begin(), listAttachment_.end(),
			std::bind2nd(
				binary_compose_f_gx_hy(
					string_equal<WCHAR>(),
					std::select1st<AttachmentParser::AttachmentList::value_type>(),
					std::identity<const WCHAR*>()),
				pwszPath));
		if (itO != listAttachment_.end()) {
			removePart((*itO).second);
			freeWString((*itO).first);
			listAttachment_.erase(itO);
		}
	}
	
	fireAttachmentsChanged();
}

const WCHAR* qm::EditMessage::getSignature() const
{
	return wstrSignature_.get();
}

void qm::EditMessage::setSignature(const WCHAR* pwszSignature)
{
	if (!((!wstrSignature_.get() && !pwszSignature) ||
		(wstrSignature_.get() && pwszSignature &&
		wcscmp(wstrSignature_.get(), pwszSignature) == 0))) {
		wstring_ptr wstrSignature;
		if (pwszSignature)
			wstrSignature = allocWString(pwszSignature);
		wstrSignature_ = wstrSignature;
		
		fireSignatureChanged();
	}
}

bool qm::EditMessage::isAutoReform() const
{
	return bAutoReform_;
}

void qm::EditMessage::setAutoReform(bool bAutoReform)
{
	bAutoReform_ = bAutoReform;
}

bool qm::EditMessage::isEncrypt() const
{
	return bEncrypt_;
}

void qm::EditMessage::setEncrypt(bool bEncrypt)
{
	bEncrypt_ = bEncrypt;
}

bool qm::EditMessage::isSign() const
{
	return bSign_;
}

void qm::EditMessage::setSign(bool bSign)
{
	bSign_ = bSign;
}

void qm::EditMessage::addEditMessageHandler(EditMessageHandler* pHandler)
{
	listHandler_.push_back(pHandler);
}

void qm::EditMessage::removeEditMessageHandler(EditMessageHandler* pHandler)
{
	HandlerList::iterator it = std::remove(
		listHandler_.begin(), listHandler_.end(), pHandler);
	listHandler_.erase(it, listHandler_.end());
}

wstring_ptr qm::EditMessage::getSignatureText() const
{
	if (wstrSignature_.get()) {
		SignatureManager* pSignatureManager = pDocument_->getSignatureManager();
		const Signature* pSignature = pSignatureManager->getSignature(
			pAccount_, wstrSignature_.get());
		if (pSignature)
			return allocWString(pSignature->getSignature());
	}
	return 0;
}

bool qm::EditMessage::fixup()
{
	fireMessageUpdate();
	
	if (bAutoReform_) {
		int nLineLen = pProfile_->getInt(L"EditWindow", L"ReformLineLength", 74);
		int nTabWidth = pProfile_->getInt(L"EditWindow", L"TabWidth", 4);
		
		wxstring_ptr wstrBody(TextUtil::fold(wstrBody_.get(),
			wcslen(wstrBody_.get()), nLineLen, 0, 0, nTabWidth));
		if (!wstrBody.get())
			return false;
		wstrBody_ = wstrBody;
	}
	
	StringBuffer<WSTRING> buf(L"\n");
	buf.append(wstrBody_.get());
	
	wstring_ptr wstrSignature(getSignatureText());
	if (wstrSignature.get())
		buf.append(wstrSignature.get());
	
	MessageCreator creator(MessageCreator::FLAG_ADDCONTENTTYPE |
		MessageCreator::FLAG_EXPANDALIAS);
	std::auto_ptr<Message> pBodyMessage(creator.createMessage(
		buf.getCharArray(), buf.getLength()));
	if (!pBodyMessage.get())
		return false;
	
	for (FieldList::iterator itF = listField_.begin(); itF != listField_.end(); ++itF) {
		const Field& field = *itF;
		if (field.wstrValue_ && *field.wstrValue_) {
			struct Type
			{
				FieldType fieldType_;
				MessageCreator::FieldType type_;
			} types[] = {
				{ FIELDTYPE_UNSTRUCTURED,	MessageCreator::FIELDTYPE_SINGLEUNSTRUCTURED	},
				{ FIELDTYPE_ADDRESSLIST,	MessageCreator::FIELDTYPE_ADDRESSLIST			}
			};
			
			MessageCreator::FieldType type;
			int n = 0;
			while (n < countof(types)) {
				if (types[n].fieldType_ == field.type_) {
					type = types[n].type_;
					break;
				}
				++n;
			}
			assert(n != countof(types));
			if (!MessageCreator::setField(pMessage_.get(),
				field.wstrName_, field.wstrValue_, type))
				return false;
		}
	}
	
	Part* pBodyPart = pBodyPart_;
	std::auto_ptr<Part> pTempPart;
	if (!pBodyPart) {
		pTempPart.reset(new Part());
		pBodyPart = pTempPart.get();
	}
	
	ContentTypeParser contentType;
	if (pBodyMessage->getField(L"Content-Type", &contentType) != Part::FIELD_EXIST)
		return false;
	if (!pBodyPart->replaceField(L"Content-Type", contentType))
		return false;
	ContentTransferEncodingParser contentTransferEncoding;
	if (pBodyMessage->getField(L"Content-Transfer-Encoding", &contentTransferEncoding) != Part::FIELD_EXIST)
		return false;
	if (!pBodyPart->replaceField(L"Content-Transfer-Encoding", contentTransferEncoding))
		return false;
	if (!pBodyPart->setBody(pBodyMessage->getBody(), -1))
		return false;
	
	if (!pBodyPart_) {
		assert(pBodyPart == pTempPart.get());
		
		if (!makeMultipartMixed())
			return false;
		pMessage_->insertPart(0, pTempPart);
		pBodyPart_ = pBodyPart;
	}
	
	if (!listAttachmentPath_.empty()) {
		if (!makeMultipartMixed())
			return false;
		
		for (AttachmentPathList::iterator itA = listAttachmentPath_.begin(); itA != listAttachmentPath_.end(); ++itA) {
			std::auto_ptr<Part> pPart(MessageCreator::createPartFromFile(*itA));
			if (!pPart.get())
				return false;
			pMessage_->addPart(pPart);
		}
	}
	
	if (!normalize(pBodyPart_))
		return false;
	
	if (*pSubAccount_->getIdentity()) {
		UnstructuredParser subaccount(pSubAccount_->getName(), L"utf-8");
		if (!pMessage_->setField(L"X-QMAIL-SubAccount", subaccount))
			return false;
	}
	
	UnstructuredParser signature(L"", L"utf-8");
	if (!pMessage_->replaceField(L"X-QMAIL-Signature", signature))
		return false;
	
	SimpleParser mimeVersion(L"1.0", 0);
	if (!pMessage_->replaceField(L"MIME-Version", mimeVersion))
		return false;
	
	return true;
}

void qm::EditMessage::clear()
{
	pMessage_.reset(0);
	pBodyPart_ = 0;
	clearFields();
	wstrBody_.reset(0);
	
	AttachmentParser::AttachmentListFree free(listAttachment_);
	
	std::for_each(listAttachmentPath_.begin(),
		listAttachmentPath_.end(), string_free<WSTRING>());
	listAttachmentPath_.clear();
	
	wstrSignature_.reset(0);
}

void qm::EditMessage::clearFields()
{
	for (FieldList::const_iterator it = listField_.begin(); it != listField_.end(); ++it) {
		freeWString((*it).wstrName_);
		freeWString((*it).wstrValue_);
	}
	listField_.clear();
}

Part* qm::EditMessage::getBodyPart(Part* pPart) const
{
	assert(pPart);
	
	if (pPart->isMultipart()) {
		const Part::PartList& l = pPart->getPartList();
		for (Part::PartList::const_iterator it = l.begin(); it != l.end(); ++it) {
			Part* pBodyPart = getBodyPart(*it);
			if (pBodyPart)
				return pBodyPart;
		}
	}
	else {
		PartUtil util(*pPart);
		if (util.isText()) {
			if (!util.isAttachment())
				return pPart;
		}
	}
	
	return 0;
}

void qm::EditMessage::removePart(qs::Part* pPart)
{
	assert(pPart);
	
	Part* pParent = pPart->getParentPart();
	
	if (!pParent && pPart != pMessage_.get()) {
		pPart = PartUtil(*pPart).getEnclosingPart(pMessage_.get());
		assert(pPart);
		if (pPart == pMessage_.get())
			pPart->setEnclosedPart(std::auto_ptr<Part>(0));
		pParent = pPart->getParentPart();
	}
	assert(pParent || pPart == pMessage_.get());
	
	if (pParent) {
		pParent->removePart(pPart);
		delete pPart;
		
		if (pParent->getPartList().empty())
			removePart(pParent);
	}
	else {
		assert(!pBodyPart_);
		const WCHAR* pwszFields[] = {
			L"Content-Type",
			L"Content-Transfer-Encoding",
			L"Content-ID",
			L"Content-Disposition",
			L"Content-Description"
		};
		for (int n = 0; n < countof(pwszFields); ++n)
			pMessage_->removeField(pwszFields[n]);
		pMessage_->setBody("", 0);
		pBodyPart_ = pMessage_.get();
	}
}

bool qm::EditMessage::normalize(Part* pPart)
{
	assert(pPart);
	
	Part* pParent = pPart;
	while (true) {
		Part* pNextParent = pParent->getParentPart();
		if (!pNextParent || pNextParent->getPartList().size() != 1)
			break;
		pParent = pNextParent;
	}
	if (pParent != pPart) {
		assert(pParent->getPartList().size() == 1);
		if (!PartUtil(*pPart).copyContentFields(pParent))
			return false;
		std::auto_ptr<Part> pChild(pParent->getPart(0));
		pParent->removePart(pChild.get());
		if (!pParent->setBody(pPart->getBody(), -1))
			return false;
	}
	
	return true;
}

bool qm::EditMessage::makeMultipartMixed()
{
	const ContentTypeParser* pContentType = pMessage_->getContentType();
	if (wcsicmp(pContentType->getMediaType(), L"multipart") != 0 ||
		wcsicmp(pContentType->getSubType(), L"mixed") != 0) {
		std::auto_ptr<Message> pMessage(new Message());
		if (!MessageCreator::makeMultipart(pMessage.get(), pMessage_))
			return false;
		pMessage->setFlag(Message::FLAG_NONE);
		pMessage_ = pMessage;
	}
	return true;
}

void qm::EditMessage::fireMessageSet()
{
	fireEvent(EditMessageEvent(this), &EditMessageHandler::messageSet);
}

void qm::EditMessage::fireMessageUpdate()
{
	fireEvent(EditMessageEvent(this), &EditMessageHandler::messageUpdate);
}

void qm::EditMessage::fireAccountChanged()
{
	fireEvent(EditMessageEvent(this), &EditMessageHandler::accountChanged);
}

void qm::EditMessage::fireFieldChanged(const WCHAR* pwszName,
									   const WCHAR* pwszValue)
{
	EditMessageFieldEvent event(this, pwszName, pwszValue);
	for (HandlerList::iterator it = listHandler_.begin(); it != listHandler_.end(); ++it)
		(*it)->fieldChanged(event);
}

void qm::EditMessage::fireAttachmentsChanged()
{
	fireEvent(EditMessageEvent(this), &EditMessageHandler::attachmentsChanged);
}

void qm::EditMessage::fireSignatureChanged()
{
	fireEvent(EditMessageEvent(this), &EditMessageHandler::signatureChanged);
}

void qm::EditMessage::fireEvent(const EditMessageEvent& event,
								void (EditMessageHandler::*pfn)(const EditMessageEvent&))
{
	for (HandlerList::iterator it = listHandler_.begin(); it != listHandler_.end(); ++it)
		((*it)->*pfn)(event);
}


/****************************************************************************
 *
 * EditMessage::AttachmentComp
 *
 */

bool qm::EditMessage::AttachmentComp::operator()(const Attachment& lhs,
												 const Attachment& rhs) const
{
	if (!lhs.bNew_ && rhs.bNew_) {
		return true;
	}
	else if (lhs.bNew_ == rhs.bNew_) {
		const WCHAR* pLhs = wcsrchr(lhs.wstrName_, L'\\');
		pLhs = pLhs ? pLhs + 1 : lhs.wstrName_;
		const WCHAR* pRhs = wcsrchr(rhs.wstrName_, L'\\');
		pRhs = pRhs ? pRhs + 1 : rhs.wstrName_;
		return wcscmp(pLhs, pRhs) < 0;
	}
	else {
		return false;
	}
}


/****************************************************************************
 *
 * EditMessage::AttachmentListFree
 *
 */

qm::EditMessage::AttachmentListFree::AttachmentListFree(AttachmentList& l) :
	l_(l)
{
}

qm::EditMessage::AttachmentListFree::~AttachmentListFree()
{
	std::for_each(l_.begin(), l_.end(),
		unary_compose_f_gx(
			string_free<WSTRING>(),
			mem_data_ref(&EditMessage::Attachment::wstrName_)));
}


/****************************************************************************
 *
 * EditMessageHandler
 *
 */

qm::EditMessageHandler::~EditMessageHandler()
{
}


/****************************************************************************
 *
 * DefaultEditMessageHandler
 *
 */

qm::DefaultEditMessageHandler::DefaultEditMessageHandler()
{
}

qm::DefaultEditMessageHandler::~DefaultEditMessageHandler()
{
}

void qm::DefaultEditMessageHandler::messageSet(const EditMessageEvent& event)
{
}

void qm::DefaultEditMessageHandler::messageUpdate(const EditMessageEvent& event)
{
}

void qm::DefaultEditMessageHandler::accountChanged(const EditMessageEvent& event)
{
}

void qm::DefaultEditMessageHandler::fieldChanged(const EditMessageFieldEvent& event)
{
}

void qm::DefaultEditMessageHandler::attachmentsChanged(const EditMessageEvent& event)
{
}

void qm::DefaultEditMessageHandler::signatureChanged(const EditMessageEvent& event)
{
}


/****************************************************************************
 *
 * EditMessageEvent
 *
 */

qm::EditMessageEvent::EditMessageEvent(EditMessage* pEditMessage) :
	pEditMessage_(pEditMessage)
{
}

qm::EditMessageEvent::~EditMessageEvent()
{
}

EditMessage* qm::EditMessageEvent::getEditMessage() const
{
	return pEditMessage_;
}


/****************************************************************************
 *
 * EditMessageFieldEvent
 *
 */

qm::EditMessageFieldEvent::EditMessageFieldEvent(EditMessage* pEditMessage,
												 const WCHAR* pwszName,
												 const WCHAR* pwszValue) :
	EditMessageEvent(pEditMessage),
	pwszName_(pwszName),
	pwszValue_(pwszValue)
{
}

qm::EditMessageFieldEvent::~EditMessageFieldEvent()
{
}

const WCHAR* qm::EditMessageFieldEvent::getName() const
{
	return pwszName_;
}

const WCHAR* qm::EditMessageFieldEvent::getValue() const
{
	return pwszValue_;
}


/****************************************************************************
 *
 * EditMessageHolder
 *
 */

qm::EditMessageHolder::~EditMessageHolder()
{
}
