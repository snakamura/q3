/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmaccount.h>
#include <qmdocument.h>
#include <qmmessage.h>
#include <qmsecurity.h>

#include <qsstl.h>
#include <qstextutil.h>

#include <algorithm>

#include "editmessage.h"
#include "signature.h"
#include "uri.h"

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
							 Account* pAccount,
							 unsigned int nSecurityMode,
							 const WCHAR* pwszTempDir) :
	pProfile_(pProfile),
	pDocument_(pDocument),
	pAccount_(pAccount),
	pSubAccount_(pAccount->getCurrentSubAccount()),
	nSecurityMode_(nSecurityMode),
	wstrTempDir_(allocWString(pwszTempDir)),
	pMessage_(0),
	pBodyPart_(0),
	bArchiveAttachments_(false),
	bAutoReform_(true),
	nMessageSecurity_(0)
{
	bAutoReform_ = pProfile_->getInt(L"EditWindow", L"AutoReform", 1) != 0;
	nMessageSecurity_ = pProfile_->getInt(L"Security",
		L"DefaultMessageSecurity", MESSAGESECURITY_PGPMIME);
}

qm::EditMessage::~EditMessage()
{
	clear();
}

std::auto_ptr<Message> qm::EditMessage::getMessage(bool bFixup)
{
	fireMessageUpdate();
	
	if (!applyFields())
		return std::auto_ptr<Message>();
	
	xstring_size_ptr strContent(pMessage_->getContent());
	if (!strContent.get())
		return std::auto_ptr<Message>();
	std::auto_ptr<Message> pMessage(new Message());
	if (!pMessage->create(strContent.get(), strContent.size(), Message::FLAG_NONE))
		return std::auto_ptr<Message>();
	
	wxstring_ptr wstrHeader(PartUtil::a2w(pBodyPart_->getHeader()));
	if (!wstrHeader.get())
		return std::auto_ptr<Message>();
	
	const WCHAR* pwszBody = wstrBody_.get();
	wxstring_ptr wstrBody;
	if (bFixup && bAutoReform_) {
		int nLineLen = pProfile_->getInt(L"EditWindow", L"ReformLineLength", 74);
		int nTabWidth = pProfile_->getInt(L"EditWindow", L"TabWidth", 4);
		
		wstrBody = TextUtil::fold(wstrBody_.get(),
			wstrBody_.size(), nLineLen, 0, 0, nTabWidth);
		if (!wstrBody.get())
			return std::auto_ptr<Message>();
		
		pwszBody = wstrBody.get();
	}
	
	XStringBuffer<WSTRING> buf;
	if (!buf.append(wstrHeader.get()) ||
		!buf.append(L"\n") ||
		!buf.append(pwszBody))
		return std::auto_ptr<Message>();
	
	if (bFixup) {
		wstring_ptr wstrSignature(getSignatureText());
		if (wstrSignature.get()) {
			if (!buf.append(wstrSignature.get()))
				return std::auto_ptr<Message>();
		}
	}
	
	MessageCreator creator(MessageCreator::FLAG_ADDCONTENTTYPE |
		MessageCreator::FLAG_EXPANDALIAS | MessageCreator::FLAG_ENCODETEXT,
		SECURITYMODE_NONE);
	std::auto_ptr<Message> pBodyMessage(creator.createMessage(
		pDocument_, buf.getCharArray(), buf.getLength()));
	if (!pBodyMessage.get())
		return std::auto_ptr<Message>();
	
	Part* pBodyPart = getBodyPart(pMessage.get());
	assert(pBodyPart);
	
	PrefixFieldFilter filter("content-");
	if (!pBodyPart->copyFields(*pBodyMessage.get(), &filter))
		return std::auto_ptr<Message>();
	
	if (pBodyMessage->isMultipart()) {
		pBodyPart->setBody(0);
		
		const Part::PartList& l = pBodyMessage->getPartList();
		for (Part::PartList::const_iterator it = l.begin(); it != l.end(); ++it) {
			std::auto_ptr<Part> pPartClone((*it)->clone());
			if (!pPartClone.get())
				return std::auto_ptr<Message>();
			pBodyPart->addPart(pPartClone);
		}
	}
	else {
		if (!pBodyPart->setBody(pBodyMessage->getBody(), -1))
			return std::auto_ptr<Message>();
	}
	
	if (!listAttachmentPath_.empty()) {
		pMessage = makeMultipartMixed(pMessage);
		if (!pMessage.get())
			return std::auto_ptr<Message>();
		
		const WCHAR* pwszArchive = getArchiveName();
		if (pwszArchive) {
			if (!MessageCreator::attachArchivedFile(pMessage.get(),
				pwszArchive, listAttachmentPath_, wstrTempDir_.get()))
				return std::auto_ptr<Message>();
		}
		else {
			if (!MessageCreator::attachFileOrURI(pMessage.get(),
				listAttachmentPath_, pDocument_, nSecurityMode_))
				return std::auto_ptr<Message>();
		}
	}
	
	if (!normalize(pBodyPart))
		return std::auto_ptr<Message>();
	
	if (*pSubAccount_->getIdentity()) {
		UnstructuredParser subaccount(pSubAccount_->getName(), L"utf-8");
		if (!pMessage->setField(L"X-QMAIL-SubAccount", subaccount))
			return std::auto_ptr<Message>();
	}
	
	const WCHAR* pwszSignature = L"";
	if (!bFixup && wstrSignature_.get())
		pwszSignature = wstrSignature_.get();
	UnstructuredParser signature(pwszSignature, L"utf-8");
	if (!pMessage->replaceField(L"X-QMAIL-Signature", signature))
		return std::auto_ptr<Message>();
	
	SimpleParser mimeVersion(L"1.0", 0);
	if (!pMessage->replaceField(L"MIME-Version", mimeVersion))
		return std::auto_ptr<Message>();
	
	return pMessage;
}

bool qm::EditMessage::setMessage(std::auto_ptr<Message> pMessage)
{
	assert(pMessage.get());
	
	clear();
	
	pMessage_ = pMessage;
	
	pBodyPart_ = getBodyPart(pMessage_.get());
	if (!pBodyPart_) {
		pMessage_ = makeMultipartMixed(pMessage_);
		if (!pMessage_.get())
			return false;
		
		std::auto_ptr<Part> pBodyPart(new Part());
		pBodyPart_ = pBodyPart.get();
		pMessage_->insertPart(0, pBodyPart);
	}
	
	wstrBody_ = pBodyPart_->getBodyText();
	if (!wstrBody_.get())
		return false;
	
	UnstructuredParser account;
	if (pMessage_->getField(L"X-QMAIL-Account", &account) == Part::FIELD_EXIST) {
		Account* pAccount = pDocument_->getAccount(account.getValue());
		if (pAccount && pAccount != pAccount_) {
			pAccount_ = pAccount;
			pSubAccount_ = pAccount->getCurrentSubAccount();
		}
	}
	UnstructuredParser subaccount;
	if (pMessage_->getField(L"X-QMAIL-SubAccount", &subaccount) == Part::FIELD_EXIST) {
		SubAccount* pSubAccount = pAccount_->getSubAccount(subaccount.getValue());
		if (pSubAccount)
			pSubAccount_ = pSubAccount;
	}
	
	SignatureManager* pSignatureManager = pDocument_->getSignatureManager();
	const Signature* pSignature = 0;
	UnstructuredParser signature;
	if (pMessage_->getField(L"X-QMAIL-Signature", &signature) == Part::FIELD_EXIST) {
		if (*signature.getValue())
			pSignature = pSignatureManager->getSignature(
				pAccount_, signature.getValue());
	}
	else {
		pSignature = pSignatureManager->getDefaultSignature(pAccount_);
	}
	if (pSignature)
		wstrSignature_ = allocWString(pSignature->getName());
	
	AttachmentParser parser(*pMessage_.get());
	parser.getAttachments(true, &listAttachment_);
	
	XQMAILAttachmentParser attachment;
	if (pMessage_->getField(L"X-QMAIL-Attachment", &attachment) == Part::FIELD_EXIST) {
		const XQMAILAttachmentParser::AttachmentList& l = attachment.getAttachments();
		for (XQMAILAttachmentParser::AttachmentList::const_iterator it = l.begin(); it != l.end(); ++it) {
			wstring_ptr wstrURI(allocWString(*it));
			listAttachmentPath_.push_back(wstrURI.get());
			wstrURI.release();
		}
	}
	
	const WCHAR* pwszFields[] = {
		L"X-QMAIL-Account",
		L"X-QMAIL-SubAccount",
		L"X-QMAIL-Attachment"
	};
	for (int n = 0; n < countof(pwszFields); ++n)
		pMessage_->removeField(pwszFields[n]);
	
	fireMessageSet();
	
	return true;
}

void qm::EditMessage::update()
{
	fireMessageUpdate();
}

const WCHAR* qm::EditMessage::getPreviousURI() const
{
	return wstrPreviousURI_.get();
}

void qm::EditMessage::setPreviousURI(const WCHAR* pwszURI)
{
	wstrPreviousURI_ = allocWString(pwszURI);
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
			break;
		case FIELDTYPE_ADDRESSLIST:
			{
				AddressListParser field(0);
				if (pMessage_->getField(pwszName, &field) == Part::FIELD_EXIST)
					wstrValue = field.getValue();
			}
			break;
		case FIELDTYPE_REFERENCES:
			{
				ReferencesParser field;
				if (pMessage_->getField(pwszName, &field) == Part::FIELD_EXIST)
					wstrValue = field.getValue();
			}
			break;
		default:
			assert(false);
			break;
		}
		if (!wstrValue.get()) {
			UnstructuredParser field;
			if (pMessage_->getField(pwszName, &field) == Part::FIELD_EXIST)
				wstrValue = allocWString(field.getValue());
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

void qm::EditMessage::removeField(const WCHAR* pwszName)
{
	assert(pwszName);
	
	FieldList::iterator it = std::find_if(
		listField_.begin(), listField_.end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				string_equal<WCHAR>(),
				mem_data_ref(&Field::wstrName_),
				std::identity<const WCHAR*>()),
			pwszName));
	if (it != listField_.end()) {
		freeWString((*it).wstrName_);
		freeWString((*it).wstrValue_);
		listField_.erase(it);
	}
	else {
		pMessage_->removeField(pwszName);
	}
	
	fireFieldChanged(pwszName, 0);
}

wxstring_ptr qm::EditMessage::getBodyPartHeader()
{
	if (!applyFields())
		return 0;
	
	if (pBodyPart_ == pMessage_.get()) {
		return PartUtil::a2w(pMessage_->getHeader());
	}
	else {
		std::auto_ptr<Part> pPart(new Part());
		
		PrefixFieldFilter contentFilter("content-");
		if (!pPart->copyFields(*pBodyPart_, &contentFilter))
			return 0;
		
		PrefixFieldFilter notContentFilter("content-", true);
		if (!pPart->copyFields(*pMessage_.get(), &notContentFilter))
			return 0;
		
		return PartUtil::a2w(pPart->getHeader());
	}
}

bool qm::EditMessage::setBodyPartHeader(const WCHAR* pwszHeader,
										size_t nLen)
{
	assert(pMessage_.get());
	assert(pBodyPart_);
	
	clearFields();
	
	MessageCreator creator;
	std::auto_ptr<Message> pMessage(creator.createMessage(pDocument_, pwszHeader, nLen));
	if (!pMessage.get())
		return false;
	
	if (pBodyPart_ == pMessage_.get()) {
		if (!pMessage_->setHeader(pMessage->getHeader()))
			return false;
	}
	else {
		PrefixFieldFilter contentFilter("content-");
		if (!pBodyPart_->copyFields(*pMessage.get(), &contentFilter))
			return false;
		
		PrefixFieldFilter notContentFilter("content-", true);
		if (!pMessage_->copyFields(*pMessage.get(), &notContentFilter))
			return false;
	}
	
	return true;
}

const WCHAR* qm::EditMessage::getBodyPartBody() const
{
	return wstrBody_.get();
}

bool qm::EditMessage::setBodyPartBody(const WCHAR* pwszBody,
									  size_t nLen)
{
	wxstring_ptr wstrBody(allocWXString(pwszBody, nLen));
	if (!wstrBody.get())
		return false;
	
	wstrBody_.reset(wstrBody, nLen);
	
	return true;
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

bool qm::EditMessage::isArchiveAttachments() const
{
	return bArchiveAttachments_;
}

void qm::EditMessage::setArchiveAttachments(bool bArchive)
{
	bArchiveAttachments_ = bArchive;
}

const WCHAR* qm::EditMessage::getArchiveName() const
{
	return wstrArchiveName_.get();
}

void qm::EditMessage::setArchiveName(const WCHAR* pwszName)
{
	assert(pwszName);
	wstrArchiveName_ = allocWString(pwszName);
}

const WCHAR* qm::EditMessage::getEncoding() const
{
	return wstrEncoding_.get();
}

void qm::EditMessage::setEncoding(const WCHAR* pwszEncoding)
{
	if (pwszEncoding)
		wstrEncoding_ = allocWString(pwszEncoding);
	else
		wstrEncoding_.reset(0);
	
	fireEncodingChanged();
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

unsigned int qm::EditMessage::getMessageSecurity() const
{
	return nMessageSecurity_;
}

void qm::EditMessage::setMessageSecurity(MessageSecurity security,
										 bool b)
{
	if (b)
		nMessageSecurity_ |= security;
	else
		nMessageSecurity_ &= ~security;
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

void qm::EditMessage::clear()
{
	pMessage_.reset(0);
	pBodyPart_ = 0;
	clearFields();
	wstrBody_.reset(0, -1);
	
	AttachmentParser::AttachmentListFree free(listAttachment_);
	
	std::for_each(listAttachmentPath_.begin(),
		listAttachmentPath_.end(), string_free<WSTRING>());
	listAttachmentPath_.clear();
	
	wstrSignature_.reset(0);
}

bool qm::EditMessage::applyFields()
{
	assert(pMessage_.get());
	assert(pBodyPart_);
	
	if (wstrEncoding_.get()) {
		ContentTypeParser contentType(L"text", L"plain");
		contentType.setParameter(L"charset", wstrEncoding_.get());
		pBodyPart_->replaceField(L"Content-Type", contentType);
	}
	
	for (FieldList::iterator it = listField_.begin(); it != listField_.end(); ) {
		Field& field = *it;
		
		Part* pPart = pMessage_.get();
		if (wcslen(field.wstrName_) > 8 && wcsnicmp(field.wstrName_, L"content-", 8) == 0)
			pPart = pBodyPart_;
		
		if (field.wstrValue_ && *field.wstrValue_) {
			struct Type
			{
				FieldType fieldType_;
				MessageCreator::FieldType type_;
			} types[] = {
				{ FIELDTYPE_UNSTRUCTURED,	MessageCreator::FIELDTYPE_SINGLEUNSTRUCTURED	},
				{ FIELDTYPE_ADDRESSLIST,	MessageCreator::FIELDTYPE_ADDRESSLIST			},
				{ FIELDTYPE_REFERENCES,		MessageCreator::FIELDTYPE_REFERENCES			}
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
			
			if (!MessageCreator::setField(pPart, field.wstrName_, field.wstrValue_, type))
				return false;
		}
		else {
			pPart->removeField(field.wstrName_);
		}
		
		freeWString(field.wstrName_);
		freeWString(field.wstrValue_);
		
		it = listField_.erase(it);
	}
	
	if (!pMessage_->sortHeader())
		return false;
	if (pMessage_.get() != pBodyPart_) {
		if (!pBodyPart_->sortHeader())
			return false;
	}
	
	return true;
}

void qm::EditMessage::clearFields()
{
	for (FieldList::const_iterator it = listField_.begin(); it != listField_.end(); ++it) {
		freeWString((*it).wstrName_);
		freeWString((*it).wstrValue_);
	}
	listField_.clear();
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
		PrefixFieldFilter filter("content-");
		pMessage_->removeFields(&filter);
		pMessage_->setBody("", 0);
		pBodyPart_ = pMessage_.get();
	}
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

void qm::EditMessage::fireEncodingChanged()
{
	fireEvent(EditMessageEvent(this), &EditMessageHandler::encodingChanged);
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

Part* qm::EditMessage::getBodyPart(Part* pPart)
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
	else if (pPart->isText() && !pPart->isAttachment()) {
		return pPart;
	}
	
	return 0;
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

std::auto_ptr<Message> qm::EditMessage::makeMultipartMixed(std::auto_ptr<Message> pMessage)
{
	const ContentTypeParser* pContentType = pMessage->getContentType();
	if (!pContentType ||
		wcsicmp(pContentType->getMediaType(), L"multipart") != 0 ||
		wcsicmp(pContentType->getSubType(), L"mixed") != 0) {
		std::auto_ptr<Message> pNewMessage(new Message());
		if (!MessageCreator::makeMultipart(pNewMessage.get(), pMessage))
			return std::auto_ptr<Message>();
		pNewMessage->setFlag(Message::FLAG_NONE);
		pMessage = pNewMessage;
	}
	return pMessage;
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

void qm::DefaultEditMessageHandler::encodingChanged(const EditMessageEvent& event)
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
