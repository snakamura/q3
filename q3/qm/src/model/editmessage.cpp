/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmaccount.h>
#include <qmapplication.h>
#include <qmdocument.h>
#include <qmmessage.h>

#include <qsnew.h>
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

qm::EditMessage::EditMessage(Document* pDocument,
	Account* pAccount, QSTATUS* pstatus) :
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
}

qm::EditMessage::~EditMessage()
{
	clear();
}

QSTATUS qm::EditMessage::getMessage(Message** ppMessage)
{
	assert(ppMessage);
	
	DECLARE_QSTATUS();
	
	status = fixup();
	CHECK_QSTATUS();
	
	*ppMessage = pMessage_;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::EditMessage::setMessage(Message* pMessage)
{
	assert(pMessage);
	
	DECLARE_QSTATUS();
	
	clear();
	
	status = getBodyPart(pMessage, &pBodyPart_);
	CHECK_QSTATUS();
	if (pBodyPart_) {
		status = pBodyPart_->getBodyText(&wstrBody_);
		CHECK_QSTATUS();
	}
	else {
		wstrBody_ = allocWString(L"");
		if (!wstrBody_)
			return QSTATUS_OUTOFMEMORY;
	}
	
	Part::Field f;
	UnstructuredParser account(&status);
	CHECK_QSTATUS();
	status = pMessage->getField(L"X-QMAIL-Account", &account, &f);
	CHECK_QSTATUS();
	if (f == Part::FIELD_EXIST) {
		Account* pAccount = pDocument_->getAccount(account.getValue());
		if (pAccount && pAccount != pAccount_) {
			pAccount_ = pAccount;
			pSubAccount_ = pAccount->getCurrentSubAccount();
		}
	}
	UnstructuredParser subaccount(&status);
	CHECK_QSTATUS();
	status = pMessage->getField(L"X-QMAIL-SubAccount", &subaccount, &f);
	CHECK_QSTATUS();
	if (f == Part::FIELD_EXIST) {
		SubAccount* pSubAccount = pAccount_->getSubAccount(subaccount.getValue());
		if (pSubAccount)
			pSubAccount_ = pSubAccount;
	}
	const WCHAR* pwszFields[] = {
		L"X-QMAIL-Account",
		L"X-QMAIL-SubAccount"
	};
	for (int n = 0; n < countof(pwszFields); ++n) {
		status = pMessage->removeField(pwszFields[n]);
		CHECK_QSTATUS();
	}
	
	SignatureManager* pSignatureManager = pDocument_->getSignatureManager();
	const Signature* pSignature = 0;
	UnstructuredParser signature(&status);
	CHECK_QSTATUS();
	status = pMessage->getField(L"X-QMAIL-Signature", &signature, &f);
	CHECK_QSTATUS();
	if (f == Part::FIELD_EXIST) {
		if (*signature.getValue()) {
			status = pSignatureManager->getSignature(
				pAccount_, signature.getValue(), &pSignature);
			CHECK_QSTATUS();
		}
	}
	else {
		status = pSignatureManager->getDefaultSignature(
			pAccount_, &pSignature);
		CHECK_QSTATUS();
	}
	if (pSignature) {
		wstrSignature_ = allocWString(pSignature->getName());
		if (!wstrSignature_)
			return QSTATUS_OUTOFMEMORY;
	}
	
	AttachmentParser attachment(*pMessage);
	status = attachment.getAttachments(&listAttachment_);
	CHECK_QSTATUS();
	
	pMessage_ = pMessage;
	
	status = fireMessageSet();
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
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

QSTATUS qm::EditMessage::setAccount(Account* pAccount, SubAccount* pSubAccount)
{
	assert(pAccount);
	assert(pSubAccount);
	
	DECLARE_QSTATUS();
	
	if (pAccount != pAccount_ || pSubAccount != pSubAccount_) {
		pAccount_ = pAccount;
		pSubAccount_ = pSubAccount;
		
		status = fireAccountChanged();
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::EditMessage::getField(const WCHAR* pwszName,
	FieldType type, WSTRING* pwstrValue)
{
	assert(pwszName);
	assert(pwstrValue);
	
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrValue;
	
	FieldList::iterator it = std::find_if(
		listField_.begin(), listField_.end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				string_equal<WCHAR>(),
				mem_data_ref(&Field::wstrName_),
				std::identity<const WCHAR*>()),
			pwszName));
	if (it != listField_.end()) {
		wstrValue.reset(allocWString((*it).wstrValue_));
		if (!wstrValue.get())
			return QSTATUS_OUTOFMEMORY;
	}
	else {
		switch (type) {
		case FIELDTYPE_UNSTRUCTURED:
			{
				UnstructuredParser field(&status);
				CHECK_QSTATUS();
				Part::Field f;
				status = pMessage_->getField(pwszName, &field, &f);
				CHECK_QSTATUS();
				if (f == Part::FIELD_EXIST) {
					wstrValue.reset(allocWString(field.getValue()));
					if (!wstrValue.get())
						return QSTATUS_OUTOFMEMORY;
				}
			}
			break;
		case FIELDTYPE_ADDRESSLIST:
			{
				AddressListParser field(0, &status);
				CHECK_QSTATUS();
				Part::Field f;
				status = pMessage_->getField(pwszName, &field, &f);
				CHECK_QSTATUS();
				if (f == Part::FIELD_EXIST) {
					status = field.getValue(&wstrValue);
					CHECK_QSTATUS();
				}
			}
			break;
		default:
			assert(false);
			break;
		}
	}
	
	*pwstrValue = wstrValue.release();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::EditMessage::setField(const WCHAR* pwszName,
	const WCHAR* pwszValue, FieldType type)
{
	assert(pwszName);
	assert(pwszValue);
	
	DECLARE_QSTATUS();
	
	bool bChange = true;
	
	string_ptr<WSTRING> wstrValue(allocWString(pwszValue));
	if (!wstrValue.get())
		return QSTATUS_OUTOFMEMORY;
	
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
		string_ptr<WSTRING> wstrName(allocWString(pwszName));
		if (!wstrName.get())
			return QSTATUS_OUTOFMEMORY;
		Field field = { wstrName.get(), wstrValue.get(), type };
		status = STLWrapper<FieldList>(listField_).push_back(field);
		CHECK_QSTATUS();
		wstrName.release();
		wstrValue.release();
		
		status = pMessage_->removeField(pwszName);
		CHECK_QSTATUS();
	}
	
	if (bChange) {
		status = fireFieldChanged(pwszName, pwszValue);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::EditMessage::getHeader(WSTRING* pwstrHeader)
{
	assert(pwstrHeader);
	
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrHeader;
	status = PartUtil::a2w(pMessage_->getHeader(), &wstrHeader);
	CHECK_QSTATUS();
	
	*pwstrHeader = wstrHeader.release();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::EditMessage::setHeader(const WCHAR* pwszHeader, size_t nLen)
{
	assert(pwszHeader);
	
	DECLARE_QSTATUS();
	
	Message msg(&status);
	CHECK_QSTATUS();
	MessageCreator creator;
	status = creator.createHeader(&msg, pwszHeader, nLen);
	CHECK_QSTATUS();
	
	status = pMessage_->setHeader(msg.getHeader());
	CHECK_QSTATUS();
	
	clearFields();
	
	return QSTATUS_SUCCESS;
}

const WCHAR* qm::EditMessage::getBody() const
{
	return wstrBody_;
}

QSTATUS qm::EditMessage::setBody(const WCHAR* pwszBody)
{
	string_ptr<WSTRING> wstrBody(allocWString(pwszBody));
	if (!wstrBody.get())
		return QSTATUS_OUTOFMEMORY;
	
	freeWString(wstrBody_);
	wstrBody_ = wstrBody.release();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::EditMessage::getAttachments(AttachmentList* pList) const
{
	assert(pList);
	
	DECLARE_QSTATUS();
	
	status = STLWrapper<AttachmentList>(*pList).reserve(
		pList->size() + listAttachment_.size() + listAttachmentPath_.size());
	CHECK_QSTATUS();
	
	AttachmentPathList::const_iterator itP = listAttachmentPath_.begin();
	while (itP != listAttachmentPath_.end()) {
		string_ptr<WSTRING> wstrName(allocWString(*itP));
		if (!wstrName.get())
			return QSTATUS_OUTOFMEMORY;
		
		Attachment attachment = { wstrName.release(), true };
		pList->push_back(attachment);
		
		++itP;
	}
	
	AttachmentParser::AttachmentList::const_iterator itA = listAttachment_.begin();
	while (itA != listAttachment_.end()) {
		string_ptr<WSTRING> wstrName(allocWString((*itA).first));
		if (!wstrName.get())
			return QSTATUS_OUTOFMEMORY;
		
		Attachment attachment = { wstrName.release(), false };
		pList->push_back(attachment);
		
		++itA;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::EditMessage::setAttachments(const AttachmentList& listAttachment)
{
	DECLARE_QSTATUS();
	
	std::for_each(listAttachmentPath_.begin(),
		listAttachmentPath_.end(), string_free<WSTRING>());
	listAttachmentPath_.clear();
	
	status = STLWrapper<AttachmentPathList>(
		listAttachmentPath_).reserve(listAttachment_.size());
	
	AttachmentList::const_iterator itA = listAttachment.begin();
	while (itA != listAttachment.end()) {
		if ((*itA).bNew_) {
			string_ptr<WSTRING> wstrPath(allocWString((*itA).wstrName_));
			if (!wstrPath.get())
				return QSTATUS_SUCCESS;
			listAttachmentPath_.push_back(wstrPath.release());
		}
		++itA;
	}
	
	AttachmentParser::AttachmentList::iterator itO = listAttachment_.begin();
	while (itO != listAttachment_.end()) {
		AttachmentList::const_iterator it = listAttachment.begin();
		while (it != listAttachment.end()) {
			if (!(*it).bNew_ && wcscmp((*it).wstrName_, (*itO).first) == 0)
				break;
			++it;
		}
		
		if (it == listAttachment.end()) {
			status = removePart((*itO).second);
			CHECK_QSTATUS();
			freeWString((*itO).first);
			itO = listAttachment_.erase(itO);
		}
		else {
			++itO;
		}
	}
	
	status = fireAttachmentsChanged();
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::EditMessage::addAttachment(const WCHAR* pwszPath)
{
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrPath(allocWString(pwszPath));
	if (!wstrPath.get())
		return QSTATUS_OUTOFMEMORY;
	
	status = STLWrapper<AttachmentPathList>(
		listAttachmentPath_).push_back(wstrPath.get());
	CHECK_QSTATUS();
	wstrPath.release();
	
	status = fireAttachmentsChanged();
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

const WCHAR* qm::EditMessage::getSignature() const
{
	return wstrSignature_;
}

QSTATUS qm::EditMessage::setSignature(const WCHAR* pwszSignature)
{
	DECLARE_QSTATUS();
	
	if (!((!wstrSignature_ && !pwszSignature) ||
		(wstrSignature_ && pwszSignature &&
		wcscmp(wstrSignature_, pwszSignature) == 0))) {
		string_ptr<WSTRING> wstrSignature;
		if (pwszSignature) {
			wstrSignature.reset(allocWString(pwszSignature));
			if (!wstrSignature.get())
				return QSTATUS_OUTOFMEMORY;
		}
		
		freeWString(wstrSignature_);
		wstrSignature_ = wstrSignature.release();
		
		status = fireSignatureChanged();
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
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

QSTATUS qm::EditMessage::addEditMessageHandler(EditMessageHandler* pHandler)
{
	return STLWrapper<HandlerList>(listHandler_).push_back(pHandler);
}

QSTATUS qm::EditMessage::removeEditMessageHandler(EditMessageHandler* pHandler)
{
	HandlerList::iterator it = std::remove(
		listHandler_.begin(), listHandler_.end(), pHandler);
	listHandler_.erase(it, listHandler_.end());
	return QSTATUS_SUCCESS;
}

QSTATUS qm::EditMessage::getSignatureText(qs::WSTRING* pwstrText) const
{
	assert(pwstrText);
	
	DECLARE_QSTATUS();
	
	*pwstrText = 0;
	
	if (wstrSignature_) {
		SignatureManager* pSignatureManager = pDocument_->getSignatureManager();
		const Signature* pSignature = 0;
		status = pSignatureManager->getSignature(
			pAccount_, wstrSignature_, &pSignature);
		CHECK_QSTATUS();
		if (pSignature) {
			*pwstrText = allocWString(pSignature->getSignature());
			if (!*pwstrText)
				return QSTATUS_OUTOFMEMORY;
		}
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::EditMessage::fixup()
{
	DECLARE_QSTATUS();
	
	status = fireMessageUpdate();
	CHECK_QSTATUS();
	
	if (bAutoReform_) {
		// TODO
		size_t nLineLen = 74;
		size_t nTabWidth = 4;
		string_ptr<WSTRING> wstrBody;
		status = TextUtil::fold(wstrBody_, wcslen(wstrBody_),
			nLineLen, 0, 0, nTabWidth, &wstrBody);
		CHECK_QSTATUS();
		freeWString(wstrBody_);
		wstrBody_ = wstrBody.release();
	}
	
	StringBuffer<WSTRING> buf(L"\n", &status);
	CHECK_QSTATUS();
	status = buf.append(wstrBody_);
	CHECK_QSTATUS();
	
	string_ptr<WSTRING> wstrSignature;
	status = getSignatureText(&wstrSignature);
	CHECK_QSTATUS();
	if (wstrSignature.get()) {
		status = buf.append(wstrSignature.get());
		CHECK_QSTATUS();
	}
	
	Message* p = 0;
	MessageCreator creator(MessageCreator::FLAG_ADDCONTENTTYPE |
		MessageCreator::FLAG_EXPANDALIAS);
	status = creator.createMessage(buf.getCharArray(), buf.getLength(), &p);
	CHECK_QSTATUS();
	std::auto_ptr<Message> pBodyMessage(p);
	
	FieldList::iterator itF = listField_.begin();
	while (itF != listField_.end()) {
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
			for (int n = 0; n < countof(types); ++n) {
				if (types[n].fieldType_ == field.type_) {
					type = types[n].type_;
					break;
				}
			}
			assert(n != countof(types));
			status = MessageCreator::setField(pMessage_,
				field.wstrName_, field.wstrValue_, type);
			CHECK_QSTATUS();
		}
		++itF;
	}
	
	Part* pBodyPart = pBodyPart_;
	std::auto_ptr<Part> pTempPart;
	if (!pBodyPart) {
		status = newQsObject(&pTempPart);
		CHECK_QSTATUS();
		pBodyPart = pTempPart.get();
	}
	Part::Field f;
	ContentTypeParser contentType(&status);
	CHECK_QSTATUS();
	status = pBodyMessage->getField(L"Content-Type", &contentType, &f);
	CHECK_QSTATUS();
	if (f != Part::FIELD_EXIST)
		return QSTATUS_FAIL;
	status = pBodyPart->replaceField(L"Content-Type", contentType);
	CHECK_QSTATUS();
	SimpleParser contentTransferEncoding(
		SimpleParser::FLAG_RECOGNIZECOMMENT | SimpleParser::FLAG_TSPECIAL,
		&status);
	CHECK_QSTATUS();
	status = pBodyMessage->getField(L"Content-Transfer-Encoding",
		&contentTransferEncoding, &f);
	CHECK_QSTATUS();
	if (f != Part::FIELD_EXIST)
		return QSTATUS_FAIL;
	status = pBodyPart->replaceField(L"Content-Transfer-Encoding",
		contentTransferEncoding);
	CHECK_QSTATUS();
	status = pBodyPart->setBody(pBodyMessage->getBody(), -1);
	CHECK_QSTATUS();
	
	if (!pBodyPart_) {
		assert(pBodyPart == pTempPart.get());
		
		status = makeMultipartMixed();
		CHECK_QSTATUS();
		status = pMessage_->insertPart(0, pBodyPart);
		CHECK_QSTATUS();
		pTempPart.release();
		pBodyPart_ = pBodyPart;
	}
	
	if (!listAttachmentPath_.empty()) {
		status = makeMultipartMixed();
		CHECK_QSTATUS();
		
		AttachmentPathList::iterator itA = listAttachmentPath_.begin();
		while (itA != listAttachmentPath_.end()) {
			Part* p = 0;
			status = MessageCreator::createPartFromFile(*itA, &p);
			CHECK_QSTATUS();
			std::auto_ptr<Part> pPart(p);
			status = pMessage_->addPart(pPart.get());
			CHECK_QSTATUS();
			pPart.release();
			++itA;
		}
	}
	
	status = normalize(pBodyPart_);
	CHECK_QSTATUS();
	
	if (*pSubAccount_->getIdentity()) {
		UnstructuredParser subaccount(pSubAccount_->getName(), L"utf-8", &status);
		CHECK_QSTATUS();
		status = pMessage_->setField(L"X-QMAIL-SubAccount", subaccount);
		CHECK_QSTATUS();
	}
	
	UnstructuredParser signature(L"", L"utf-8", &status);
	CHECK_QSTATUS();
	status = pMessage_->replaceField(L"X-QMAIL-Signature", signature);
	CHECK_QSTATUS();
	
	SimpleParser mimeVersion(L"1.0", 0, &status);
	CHECK_QSTATUS();
	status = pMessage_->replaceField(L"MIME-Version", mimeVersion);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

void qm::EditMessage::clear()
{
	delete pMessage_;
	pMessage_ = 0;
	
	pBodyPart_ = 0;
	
	clearFields();
	
	freeWString(wstrBody_);
	wstrBody_ = 0;
	
	AttachmentParser::AttachmentListFree free(listAttachment_);
	
	std::for_each(listAttachmentPath_.begin(),
		listAttachmentPath_.end(), string_free<WSTRING>());
	listAttachmentPath_.clear();
	
	freeWString(wstrSignature_);
	wstrSignature_ = 0;
}

void qm::EditMessage::clearFields()
{
	FieldList::const_iterator it = listField_.begin();
	while (it != listField_.end()) {
		freeWString((*it).wstrName_);
		freeWString((*it).wstrValue_);
		++it;
	}
	listField_.clear();
}

QSTATUS qm::EditMessage::getBodyPart(Part* pPart, Part** ppPart) const
{
	assert(pPart);
	assert(ppPart);
	
	DECLARE_QSTATUS();
	
	*ppPart = 0;
	
	if (pPart->isMultipart()) {
		const Part::PartList& l = pPart->getPartList();
		Part::PartList::const_iterator it = l.begin();
		while (it != l.end() && !*ppPart) {
			status = getBodyPart(*it, ppPart);
			CHECK_QSTATUS();
			++it;
		}
	}
	else {
		PartUtil util(*pPart);
		if (util.isText()) {
			bool bAttachment = false;
			status = util.isAttachment(&bAttachment);
			CHECK_QSTATUS();
			if (!bAttachment)
				*ppPart = pPart;
		}
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::EditMessage::removePart(qs::Part* pPart)
{
	assert(pPart);
	
	DECLARE_QSTATUS();
	
	Part* pParent = pPart->getParentPart();
	
	if (!pParent && pPart != pMessage_) {
		pPart = PartUtil(*pPart).getEnclosingPart(pMessage_);
		assert(pPart);
		if (pPart == pMessage_)
			pPart->setEnclosedPart(0);
		pParent = pPart->getParentPart();
	}
	assert(pParent || pPart == pMessage_);
	
	if (pParent) {
		pParent->removePart(pPart);
		delete pPart;
		
		if (pParent->getPartList().empty()) {
			status = removePart(pParent);
			CHECK_QSTATUS();
		}
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
		for (int n = 0; n < countof(pwszFields); ++n) {
			status = pMessage_->removeField(pwszFields[n]);
			CHECK_QSTATUS();
		}
		status = pMessage_->setBody("", 0);
		CHECK_QSTATUS();
		pBodyPart_ = pMessage_;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::EditMessage::normalize(Part* pPart)
{
	assert(pPart);
	
	DECLARE_QSTATUS();
	
	Part* pParent = pPart;
	while (true) {
		Part* pNextParent = pParent->getParentPart();
		if (!pNextParent || pNextParent->getPartList().size() != 1)
			break;
		pParent = pNextParent;
	}
	if (pParent != pPart) {
		assert(pParent->getPartList().size() == 1);
		status = PartUtil(*pPart).copyContentFields(pParent);
		CHECK_QSTATUS();
		std::auto_ptr<Part> pChild(pParent->getPart(0));
		pParent->removePart(pChild.get());
		status = pParent->setBody(pPart->getBody(), -1);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::EditMessage::makeMultipartMixed()
{
	DECLARE_QSTATUS();
	
	const ContentTypeParser* pContentType = pMessage_->getContentType();
	if (wcsicmp(pContentType->getMediaType(), L"multipart") != 0 ||
		wcsicmp(pContentType->getSubType(), L"mixed") != 0) {
		std::auto_ptr<Message> pMessage;
		status = newQsObject(&pMessage);
		CHECK_QSTATUS();
		status = MessageCreator::makeMultipart(pMessage.get(), pMessage_);
		CHECK_QSTATUS();
		pMessage->setFlag(Message::FLAG_NONE);
		pMessage_ = pMessage.release();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::EditMessage::fireMessageSet()
{
	return fireEvent(EditMessageEvent(this),
		&EditMessageHandler::messageSet);
}

QSTATUS qm::EditMessage::fireMessageUpdate()
{
	return fireEvent(EditMessageEvent(this),
		&EditMessageHandler::messageUpdate);
}

QSTATUS qm::EditMessage::fireAccountChanged()
{
	return fireEvent(EditMessageEvent(this),
		&EditMessageHandler::accountChanged);
}

QSTATUS qm::EditMessage::fireFieldChanged(
	const WCHAR* pwszName, const WCHAR* pwszValue)
{
	DECLARE_QSTATUS();
	
	EditMessageFieldEvent event(this, pwszName, pwszValue);
	
	HandlerList::iterator it = listHandler_.begin();
	while (it != listHandler_.end()) {
		status = (*it)->fieldChanged(event);
		CHECK_QSTATUS();
		++it;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::EditMessage::fireAttachmentsChanged()
{
	return fireEvent(EditMessageEvent(this),
		&EditMessageHandler::attachmentsChanged);
}

QSTATUS qm::EditMessage::fireSignatureChanged()
{
	return fireEvent(EditMessageEvent(this),
		&EditMessageHandler::signatureChanged);
}

QSTATUS qm::EditMessage::fireEvent(const EditMessageEvent& event,
	qs::QSTATUS (EditMessageHandler::*pfn)(const EditMessageEvent&))
{
	DECLARE_QSTATUS();
	
	HandlerList::iterator it = listHandler_.begin();
	while (it != listHandler_.end()) {
		status = ((*it)->*pfn)(event);
		CHECK_QSTATUS();
		++it;
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * EditMessage::AttachmentComp
 *
 */

bool qm::EditMessage::AttachmentComp::operator()(
	const Attachment& lhs, const Attachment& rhs) const
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

QSTATUS qm::DefaultEditMessageHandler::messageSet(
	const EditMessageEvent& event)
{
	return QSTATUS_SUCCESS;
}

QSTATUS qm::DefaultEditMessageHandler::messageUpdate(
	const EditMessageEvent& event)
{
	return QSTATUS_SUCCESS;
}

QSTATUS qm::DefaultEditMessageHandler::accountChanged(
	const EditMessageEvent& event)
{
	return QSTATUS_SUCCESS;
}

QSTATUS qm::DefaultEditMessageHandler::fieldChanged(
	const EditMessageFieldEvent& event)
{
	return QSTATUS_SUCCESS;
}

QSTATUS qm::DefaultEditMessageHandler::attachmentsChanged(
	const EditMessageEvent& event)
{
	return QSTATUS_SUCCESS;
}

QSTATUS qm::DefaultEditMessageHandler::signatureChanged(
	const EditMessageEvent& event)
{
	return QSTATUS_SUCCESS;
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
	const WCHAR* pwszName, const WCHAR* pwszValue) :
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
