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
#include <qmfolder.h>
#include <qmmacro.h>
#include <qmmessage.h>
#include <qmmessageholder.h>
#include <qmtemplate.h>

#include <qmscript.h>

#include <qsconv.h>
#include <qsnew.h>
#include <qsosutil.h>
#include <qsstl.h>
#include <qsstream.h>
#include <qswindow.h>

#include <algorithm>

#include "macro.h"
#include "../model/addressbook.h"
#include "../script/scriptmanager.h"
#include "../ui/dialogs.h"

#pragma warning(disable:4786)

using namespace qm;
using namespace qmscript;
using namespace qs;


/****************************************************************************
 *
 * MacroFunction
 *
 */

qm::MacroFunction::MacroFunction(QSTATUS* pstatus)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qm::MacroFunction::~MacroFunction()
{
	ArgList::iterator it = listArg_.begin();
	while (it != listArg_.end()) {
		(*it)->release();
		++it;
	}
}

QSTATUS qm::MacroFunction::addArg(MacroExpr* pArg)
{
	return STLWrapper<ArgList>(listArg_).push_back(pArg);
}

QSTATUS qm::MacroFunction::getString(WSTRING* pwstrExpr) const
{
	assert(pwstrExpr);
	
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrArg;
	status = getArgString(&wstrArg);
	CHECK_QSTATUS();
	const ConcatW c[] = {
		{ L"@",				-1 },
		{ getName(),		-1 },
		{ L"(",				-1 },
		{ wstrArg.get(),	-1 },
		{ L")",				-1 }
	};
	*pwstrExpr = concat(c, countof(c));
	return *pwstrExpr ? QSTATUS_SUCCESS : QSTATUS_OUTOFMEMORY;
}

void qm::MacroFunction::release()
{
	delete this;
}

size_t qm::MacroFunction::getArgSize() const
{
	return listArg_.size();
}

const MacroExpr* qm::MacroFunction::getArg(size_t n) const
{
	assert(n < getArgSize());
	return listArg_[n];
}

QSTATUS qm::MacroFunction::getPart(MacroContext* pContext,
	size_t n, const Part** ppPart) const
{
	assert(ppPart);
	
	DECLARE_QSTATUS();
	
	*ppPart = 0;
	
	MacroValuePtr pValue;
	status = getArg(n)->value(pContext, &pValue);
	CHECK_QSTATUS();
	if (pValue->getType() != MacroValue::TYPE_PART)
		return error(*pContext, MacroErrorHandler::CODE_INVALIDARGTYPE);
	*ppPart = static_cast<MacroValuePart*>(pValue.get())->getPart();
	if (!*ppPart)
		return error(*pContext, MacroErrorHandler::CODE_INVALIDPART);
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MacroFunction::getArgString(WSTRING* pwstrArg) const
{
	assert(pwstrArg);
	
	DECLARE_QSTATUS();
	
	StringBuffer<WSTRING> buf(&status);
	CHECK_QSTATUS();
	
	ArgList::const_iterator it = listArg_.begin();
	while (it != listArg_.end()) {
		if (it != listArg_.begin()) {
			status = buf.append(L',');
			CHECK_QSTATUS();
		}
		string_ptr<WSTRING> wstrArg;
		status = (*it)->getString(&wstrArg);
		CHECK_QSTATUS();
		status = buf.append(wstrArg.get());
		CHECK_QSTATUS();
		
		++it;
	}
	*pwstrArg = buf.getString();
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * MacroFunctionAccount
 *
 */

qm::MacroFunctionAccount::MacroFunctionAccount(QSTATUS* pstatus) :
	MacroFunction(pstatus)
{
}

qm::MacroFunctionAccount::~MacroFunctionAccount()
{
}

QSTATUS qm::MacroFunctionAccount::value(
	MacroContext* pContext, MacroValue** ppValue) const
{
	assert(pContext);
	assert(ppValue);
	
	DECLARE_QSTATUS();
	
	*ppValue = 0;
	
	size_t nSize = getArgSize();
	if (nSize > 1)
		return error(*pContext, MacroErrorHandler::CODE_INVALIDARGSIZE);
	
	bool bSub = false;
	if (nSize > 0) {
		MacroValuePtr pValue;
		status = getArg(0)->value(pContext, &pValue);
		CHECK_QSTATUS();
		bSub = pValue->boolean();
	}
	
	MessageHolderBase* pmh = pContext->getMessageHolder();
	Account* pAccount = pContext->getAccount();
	assert(pAccount);
	if (pmh)
		pAccount = pmh->getFolder()->getAccount();
	
	const WCHAR* pwszName = 0;
	if (bSub) {
		SubAccount* pSubAccount = pAccount->getCurrentSubAccount();
		pwszName = pSubAccount->getName();
	}
	else {
		pwszName = pAccount->getName();
	}
	
	return MacroValueFactory::getFactory().newString(pwszName,
		reinterpret_cast<MacroValueString**>(ppValue));
}

const WCHAR* qm::MacroFunctionAccount::getName() const
{
	return L"Account";
}


/****************************************************************************
 *
 * MacroFunctionAccountDirectory
 *
 */

qm::MacroFunctionAccountDirectory::MacroFunctionAccountDirectory(QSTATUS* pstatus) :
	MacroFunction(pstatus)
{
}

qm::MacroFunctionAccountDirectory::~MacroFunctionAccountDirectory()
{
}

QSTATUS qm::MacroFunctionAccountDirectory::value(
	MacroContext* pContext, MacroValue** ppValue) const
{
	assert(pContext);
	assert(ppValue);
	
	DECLARE_QSTATUS();
	
	*ppValue = 0;
	
	size_t nSize = getArgSize();
	if (nSize > 1)
		return error(*pContext, MacroErrorHandler::CODE_INVALIDARGSIZE);
	
	string_ptr<WSTRING> wstrAccount;
	if (nSize > 0) {
		MacroValuePtr pValue;
		status = getArg(0)->value(pContext, &pValue);
		CHECK_QSTATUS();
		status = pValue->string(&wstrAccount);
		CHECK_QSTATUS();
	}
	
	Account* pAccount = 0;
	if (wstrAccount.get()) {
		pAccount = pContext->getDocument()->getAccount(wstrAccount.get());
		if (!pAccount)
			return error(*pContext, MacroErrorHandler::CODE_UNKNOWNACCOUNT);
	}
	else {
		MessageHolderBase* pmh = pContext->getMessageHolder();
		Account* pAccount = pContext->getAccount();
		assert(pAccount);
		if (pmh)
			pAccount = pmh->getFolder()->getAccount();
	}
	
	return MacroValueFactory::getFactory().newString(pAccount->getPath(),
		reinterpret_cast<MacroValueString**>(ppValue));
}

const WCHAR* qm::MacroFunctionAccountDirectory::getName() const
{
	return L"AccountDirectory";
}


/****************************************************************************
 *
 * MacroFunctionAdditive
 *
 */

qm::MacroFunctionAdditive::MacroFunctionAdditive(
	bool bAdd, QSTATUS* pstatus) :
	MacroFunction(pstatus),
	bAdd_(bAdd)
{
}

qm::MacroFunctionAdditive::~MacroFunctionAdditive()
{
}

QSTATUS qm::MacroFunctionAdditive::value(
	MacroContext* pContext, MacroValue** ppValue) const
{
	assert(pContext);
	assert(ppValue);
	
	DECLARE_QSTATUS();
	
	*ppValue = 0;
	
	if (getArgSize() != 2)
		return error(*pContext, MacroErrorHandler::CODE_INVALIDARGSIZE);
	
	MacroValuePtr pValueLhs;
	status = getArg(0)->value(pContext, &pValueLhs);
	CHECK_QSTATUS();
	
	MacroValuePtr pValueRhs;
	status = getArg(1)->value(pContext, &pValueRhs);
	CHECK_QSTATUS();
	
	unsigned int nValue = 0;
	if (bAdd_)
		nValue = pValueLhs->number() + pValueRhs->number();
	else
		nValue = pValueLhs->number() - pValueRhs->number();
	
	return MacroValueFactory::getFactory().newNumber(nValue,
		reinterpret_cast<MacroValueNumber**>(ppValue));
}

const WCHAR* qm::MacroFunctionAdditive::getName() const
{
	if (bAdd_)
		return L"Add";
	else
		return L"Minus";
}


/****************************************************************************
 *
 * MacroFunctionAddress
 *
 */

qm::MacroFunctionAddress::MacroFunctionAddress(
	bool bName, QSTATUS* pstatus) :
	MacroFunction(pstatus),
	bName_(bName)
{
}

qm::MacroFunctionAddress::~MacroFunctionAddress()
{
}

QSTATUS qm::MacroFunctionAddress::value(
	MacroContext* pContext, MacroValue** ppValue) const
{
	assert(pContext);
	assert(ppValue);
	
	DECLARE_QSTATUS();
	
	*ppValue = 0;
	
	if (getArgSize() != 1)
		return error(*pContext, MacroErrorHandler::CODE_INVALIDARGSIZE);
	
	MacroValuePtr pValue;
	status = getArg(0)->value(pContext, &pValue);
	CHECK_QSTATUS();
	if (pValue->getType() != MacroValue::TYPE_FIELD)
		return error(*pContext, MacroErrorHandler::CODE_INVALIDARGTYPE);
	
	MacroValueField* pValueField = static_cast<MacroValueField*>(pValue.get());
	std::vector<WSTRING> v;
	struct Deleter
	{
		Deleter(std::vector<WSTRING>& v) : v_(v) {}
		~Deleter()
			{ std::for_each(v_.begin(), v_.end(), string_free<WSTRING>()); }
		std::vector<WSTRING>& v_;
	} deleter(v);
	if (bName_) {
		status = pValueField->getNames(&v);
		CHECK_QSTATUS();
	}
	else {
		status = pValueField->getAddresses(&v);
		CHECK_QSTATUS();
	}
	
	return MacroValueFactory::getFactory().newAddress(v,
		reinterpret_cast<MacroValueAddress**>(ppValue));
}

const WCHAR* qm::MacroFunctionAddress::getName() const
{
	return L"Address";
}


/****************************************************************************
 *
 * MacroFunctionAddressBook
 *
 */

qm::MacroFunctionAddressBook::MacroFunctionAddressBook(QSTATUS* pstatus) :
	MacroFunction(pstatus)
{
}

qm::MacroFunctionAddressBook::~MacroFunctionAddressBook()
{
}

QSTATUS qm::MacroFunctionAddressBook::value(
	MacroContext* pContext, MacroValue** ppValue) const
{
	assert(pContext);
	assert(ppValue);
	
	DECLARE_QSTATUS();
	
	*ppValue = 0;
	
	size_t nSize = getArgSize();
	if (nSize > 3)
		return error(*pContext, MacroErrorHandler::CODE_INVALIDARGSIZE);
	
	string_ptr<WSTRING> wstrAddress[3];
	for (unsigned int nArg = 0; nArg < countof(wstrAddress); ++nArg) {
		if (nSize > 2 - nArg) {
			MacroValuePtr pValue;
			status = getArg(2 - nArg)->value(pContext, &pValue);
			CHECK_QSTATUS();
			status = pValue->string(&wstrAddress[2 - nArg]);
			CHECK_QSTATUS();
		}
	}
	
	const WCHAR* pwszAddress[] = {
		wstrAddress[0].get(),
		wstrAddress[1].get(),
		wstrAddress[2].get()
	};
	AddressBookDialog dialog(pContext->getDocument()->getAddressBook(),
		pContext->getProfile(), pwszAddress, &status);
	CHECK_QSTATUS();
	int nRet = 0;
	status = dialog.doModal(pContext->getWindow(), 0, &nRet);
	CHECK_QSTATUS();
	if (nRet != IDOK) {
		// TODO
		return QSTATUS_FAIL;
	}
	
	StringBuffer<WSTRING> buf(&status);
	CHECK_QSTATUS();
	
	struct Type
	{
		const WCHAR* pwszName_;
		AddressBookDialog::Type type_;
	} types[] = {
		{ L"To: ",	AddressBookDialog::TYPE_TO	},
		{ L"Cc: ",	AddressBookDialog::TYPE_CC,	},
		{ L"Bcc: ",	AddressBookDialog::TYPE_BCC	}
	};
	for (int n = 0; n < countof(types); ++n) {
		const AddressBookDialog::AddressList& l =
			dialog.getAddresses(types[n].type_);
		if (!l.empty()) {
			status = buf.append(types[n].pwszName_);
			CHECK_QSTATUS();
			AddressBookDialog::AddressList::const_iterator it = l.begin();
			while (it != l.end()) {
				if (it != l.begin()) {
					status = buf.append(L", ");
					CHECK_QSTATUS();
				}
				status = buf.append(*it);
				CHECK_QSTATUS();
				++it;
			}
			status = buf.append(L"\n");
			CHECK_QSTATUS();
		}
	}
	
	return MacroValueFactory::getFactory().newString(buf.getCharArray(),
		reinterpret_cast<MacroValueString**>(ppValue));
}

const WCHAR* qm::MacroFunctionAddressBook::getName() const
{
	return L"AddressBook";
}


/****************************************************************************
 *
 * MacroFunctionAnd
 *
 */

qm::MacroFunctionAnd::MacroFunctionAnd(QSTATUS* pstatus) :
	MacroFunction(pstatus)
{
}

qm::MacroFunctionAnd::~MacroFunctionAnd()
{
}

QSTATUS qm::MacroFunctionAnd::value(
	MacroContext* pContext, MacroValue** ppValue) const
{
	assert(pContext);
	assert(ppValue);
	
	DECLARE_QSTATUS();
	
	*ppValue = 0;
	
	size_t nSize = getArgSize();
	if (nSize == 0)
		return error(*pContext, MacroErrorHandler::CODE_INVALIDARGSIZE);
	
	bool bValue = true;
	for (size_t n = 0; n < nSize && bValue; ++n) {
		MacroValuePtr pValue;
		status = getArg(n)->value(pContext, &pValue);
		CHECK_QSTATUS();
		if (!pValue->boolean())
			bValue = false;
	}
	return MacroValueFactory::getFactory().newBoolean(
		bValue, reinterpret_cast<MacroValueBoolean**>(ppValue));
}

const WCHAR* qm::MacroFunctionAnd::getName() const
{
	return L"And";
}


/****************************************************************************
 *
 * MacroFunctionAttachment
 *
 */

qm::MacroFunctionAttachment::MacroFunctionAttachment(QSTATUS* pstatus) :
	MacroFunction(pstatus)
{
}

qm::MacroFunctionAttachment::~MacroFunctionAttachment()
{
}

QSTATUS qm::MacroFunctionAttachment::value(
	MacroContext* pContext, MacroValue** ppValue) const
{
	assert(pContext);
	assert(ppValue);
	
	DECLARE_QSTATUS();
	
	*ppValue = 0;
	
	size_t nSize = getArgSize();
	if (nSize != 0 && nSize != 1)
		return error(*pContext, MacroErrorHandler::CODE_INVALIDARGSIZE);
	
	MessageHolderBase* pmh = pContext->getMessageHolder();
	if (!pmh)
		return error(*pContext, MacroErrorHandler::CODE_NOCONTEXTMESSAGE);
	
	Message* pMessage = 0;
	status = pContext->getMessage(MacroContext::MESSAGETYPE_TEXT, 0, &pMessage);
	CHECK_QSTATUS();
	
	const WCHAR* pwszSep = L", ";
	string_ptr<WSTRING> wstrSep;
	if (nSize == 1) {
		MacroValuePtr pValue;
		status = getArg(0)->value(pContext, &pValue);
		CHECK_QSTATUS();
		status = pValue->string(&wstrSep);
		CHECK_QSTATUS();
		pwszSep = wstrSep.get();
	}
	
	AttachmentParser::AttachmentList l;
	AttachmentParser::AttachmentListFree free(l);
	status = AttachmentParser(*pMessage).getAttachments(&l);
	CHECK_QSTATUS();
	
	StringBuffer<WSTRING> buf(&status);
	CHECK_QSTATUS();
	AttachmentParser::AttachmentList::iterator it = l.begin();
	while (it != l.end()) {
		if (it != l.begin()) {
			status = buf.append(pwszSep);
			CHECK_QSTATUS();
		}
		status = buf.append((*it).first);
		CHECK_QSTATUS();
		
		++it;
	}
	
	return MacroValueFactory::getFactory().newString(buf.getCharArray(),
		reinterpret_cast<MacroValueString**>(ppValue));
}

const WCHAR* qm::MacroFunctionAttachment::getName() const
{
	return L"Attachment";
}


/****************************************************************************
 *
 * MacroFunctionBody
 *
 */

qm::MacroFunctionBody::MacroFunctionBody(QSTATUS* pstatus) :
	MacroFunction(pstatus)
{
}

qm::MacroFunctionBody::~MacroFunctionBody()
{
}

QSTATUS qm::MacroFunctionBody::value(
	MacroContext* pContext, MacroValue** ppValue) const
{
	assert(pContext);
	assert(ppValue);
	
	DECLARE_QSTATUS();
	
	*ppValue = 0;
	
	size_t nSize = getArgSize();
	if (nSize > 3)
		return error(*pContext, MacroErrorHandler::CODE_INVALIDARGSIZE);

	MessageHolderBase* pmh = pContext->getMessageHolder();
	if (!pmh)
		return error(*pContext, MacroErrorHandler::CODE_NOCONTEXTMESSAGE);
	
	const Part* pPart = 0;
	if (nSize > 2) {
		status = getPart(pContext, 2, &pPart);
		CHECK_QSTATUS();
	}
	
	bool bView = false;
	if (nSize > 1) {
		MacroValuePtr pValue;
		status = getArg(1)->value(pContext, &pValue);
		CHECK_QSTATUS();
		bView = pValue->boolean();
	}
	
	string_ptr<WSTRING> wstrQuote;
	if (nSize > 0) {
		MacroValuePtr pValue;
		status = getArg(0)->value(pContext, &pValue);
		CHECK_QSTATUS();
		status = pValue->string(&wstrQuote);
		CHECK_QSTATUS();
	}
	
	Message* pMessage = 0;
	status = pContext->getMessage(bView ? MacroContext::MESSAGETYPE_TEXT :
		MacroContext::MESSAGETYPE_ALL, 0, &pMessage);
	CHECK_QSTATUS();
	
	if (!pPart)
		pPart = pMessage;
	
	string_ptr<WSTRING> wstrBody;
	PartUtil util(*pPart);
	if (bView) {
		status = util.getBodyText(wstrQuote.get(), 0, &wstrBody);
		CHECK_QSTATUS();
	}
	else {
		status = util.getAllText(wstrQuote.get(), 0, true, &wstrBody);
		CHECK_QSTATUS();
	}
	
	return MacroValueFactory::getFactory().newString(wstrBody.get(),
		reinterpret_cast<MacroValueString**>(ppValue));
}

const WCHAR* qm::MacroFunctionBody::getName() const
{
	return L"Body";
}


/****************************************************************************
 *
 * MacroFunctionBoolean
 *
 */

qm::MacroFunctionBoolean::MacroFunctionBoolean(bool b, QSTATUS* pstatus) :
	MacroFunction(pstatus),
	b_(b)
{
}

qm::MacroFunctionBoolean::~MacroFunctionBoolean()
{
}

QSTATUS qm::MacroFunctionBoolean::value(
	MacroContext* pContext, MacroValue** ppValue) const
{
	assert(pContext);
	assert(ppValue);
	
	*ppValue = 0;
	
	if (getArgSize() != 0)
		return error(*pContext, MacroErrorHandler::CODE_INVALIDARGSIZE);
	
	return MacroValueFactory::getFactory().newBoolean(b_,
		reinterpret_cast<MacroValueBoolean**>(ppValue));
}

const WCHAR* qm::MacroFunctionBoolean::getName() const
{
	return b_ ? L"True" : L"False";
}


/****************************************************************************
 *
 * MacroFunctionClipboard
 *
 */

qm::MacroFunctionClipboard::MacroFunctionClipboard(QSTATUS* pstatus) :
	MacroFunction(pstatus)
{
}

qm::MacroFunctionClipboard::~MacroFunctionClipboard()
{
}

QSTATUS qm::MacroFunctionClipboard::value(
	MacroContext* pContext, MacroValue** ppValue) const
{
	assert(pContext);
	assert(ppValue);
	
	DECLARE_QSTATUS();
	
	*ppValue = 0;
	
	size_t nSize = getArgSize();
	if (nSize != 0 && nSize != 1)
		return error(*pContext, MacroErrorHandler::CODE_INVALIDARGSIZE);
	
	const WCHAR* pwszText = L"";
	string_ptr<WSTRING> wstrText;
	if (nSize == 0) {
		status = Clipboard::getText(&wstrText);
		if (status == QSTATUS_FAIL)
			return error(*pContext, MacroErrorHandler::CODE_FAIL);
		CHECK_QSTATUS();
		pwszText = wstrText.get();
	}
	else {
		MacroValuePtr pValueContent;
		status = getArg(0)->value(pContext, &pValueContent);
		CHECK_QSTATUS();
		
		string_ptr<WSTRING> wstrText;
		status = pValueContent->string(&wstrText);
		CHECK_QSTATUS();
		
		status = Clipboard::setText(wstrText.get());
		if (status == QSTATUS_FAIL)
			return error(*pContext, MacroErrorHandler::CODE_FAIL);
		CHECK_QSTATUS();
	}
	
	return MacroValueFactory::getFactory().newString(pwszText,
		reinterpret_cast<MacroValueString**>(ppValue));
}

const WCHAR* qm::MacroFunctionClipboard::getName() const
{
	return L"Clipboard";
}


/****************************************************************************
 *
 * MacroFunctionComputerName
 *
 */

qm::MacroFunctionComputerName::MacroFunctionComputerName(QSTATUS* pstatus) :
	MacroFunction(pstatus)
{
}

qm::MacroFunctionComputerName::~MacroFunctionComputerName()
{
}

QSTATUS qm::MacroFunctionComputerName::value(
	MacroContext* pContext, MacroValue** ppValue) const
{
	assert(pContext);
	assert(ppValue);
	
	DECLARE_QSTATUS();
	
	*ppValue = 0;
	
	if (getArgSize() != 0)
		return error(*pContext, MacroErrorHandler::CODE_INVALIDARGSIZE);
	
	string_ptr<WSTRING> wstrName;
#ifdef _WIN32_WCE
	Registry reg(HKEY_LOCAL_MACHINE, L"Ident", &status);
	CHECK_QSTATUS();
	status = reg.getValue(L"Name", &wstrName);
	CHECK_QSTATUS();
#else
	TCHAR tszComputerName[MAX_COMPUTERNAME_LENGTH + 1];
	DWORD dwSize = countof(tszComputerName);
	::GetComputerName(tszComputerName, &dwSize);
	wstrName.reset(tcs2wcs(tszComputerName));
	if (!wstrName.get())
		return QSTATUS_OUTOFMEMORY;
#endif
	
	return MacroValueFactory::getFactory().newString(wstrName.get(),
		reinterpret_cast<MacroValueString**>(ppValue));
}

const WCHAR* qm::MacroFunctionComputerName::getName() const
{
	return L"ComputerName";
}


/****************************************************************************
 *
 * MacroFunctionConcat
 *
 */

qm::MacroFunctionConcat::MacroFunctionConcat(QSTATUS* pstatus) :
	MacroFunction(pstatus)
{
}

qm::MacroFunctionConcat::~MacroFunctionConcat()
{
}

QSTATUS qm::MacroFunctionConcat::value(
	MacroContext* pContext, MacroValue** ppValue) const
{
	assert(pContext);
	assert(ppValue);
	
	DECLARE_QSTATUS();
	
	*ppValue = 0;
	
	StringBuffer<WSTRING> buf(&status);
	CHECK_QSTATUS();
	for (size_t n = 0; n < getArgSize(); ++n) {
		MacroValuePtr pValue;
		status = getArg(n)->value(pContext, &pValue);
		CHECK_QSTATUS();
		
		string_ptr<WSTRING> wstr;
		status = pValue->string(&wstr);
		CHECK_QSTATUS();
		
		status = buf.append(wstr.get());
		CHECK_QSTATUS();
	}
	
	return MacroValueFactory::getFactory().newString(
		buf.getCharArray(), reinterpret_cast<MacroValueString**>(ppValue));
}

const WCHAR* qm::MacroFunctionConcat::getName() const
{
	return L"Concat";
}


/****************************************************************************
 *
 * MacroFunctionContain
 *
 */

qm::MacroFunctionContain::MacroFunctionContain(bool bBeginWith, QSTATUS* pstatus) :
	MacroFunction(pstatus),
	bBeginWith_(bBeginWith)
{
}

qm::MacroFunctionContain::~MacroFunctionContain()
{
}

QSTATUS qm::MacroFunctionContain::value(
	MacroContext* pContext, MacroValue** ppValue) const
{
	assert(pContext);
	assert(ppValue);
	
	DECLARE_QSTATUS();
	
	*ppValue = 0;
	
	size_t nSize = getArgSize();
	if (nSize != 2 && nSize != 3)
		return error(*pContext, MacroErrorHandler::CODE_INVALIDARGSIZE);
	
	bool bCase = false;
	if (nSize == 3) {
		MacroValuePtr pValue;
		status = getArg(2)->value(pContext, &pValue);
		CHECK_QSTATUS();
		bCase = pValue->boolean();
	}
	
	MacroValuePtr pValueLhs;
	status = getArg(0)->value(pContext, &pValueLhs);
	CHECK_QSTATUS();
	
	MacroValuePtr pValueRhs;
	status = getArg(1)->value(pContext, &pValueRhs);
	CHECK_QSTATUS();
	
	string_ptr<WSTRING> wstrLhs;
	status = pValueLhs->string(&wstrLhs);
	CHECK_QSTATUS();
	
	string_ptr<WSTRING> wstrRhs;
	status = pValueRhs->string(&wstrRhs);
	CHECK_QSTATUS();
	
	size_t nLhsLen = wcslen(wstrLhs.get());
	size_t nRhsLen = wcslen(wstrRhs.get());
	
	bool bResult = false;
	if (nLhsLen >= nRhsLen) {
		if (bBeginWith_) {
			bResult = _wcsnicmp(wstrLhs.get(), wstrRhs.get(), nRhsLen) == 0;
		}
		else {
			if (!bCase) {
				string_ptr<WSTRING> wstrRhsLower(tolower(wstrRhs.get()));
				if (!wstrRhsLower.get())
					return QSTATUS_OUTOFMEMORY;
				string_ptr<WSTRING> wstrRhsUpper(toupper(wstrRhs.get()));
				if (!wstrRhsUpper.get())
					return QSTATUS_OUTOFMEMORY;
				if (wcscmp(wstrRhsLower.get(), wstrRhsUpper.get()) != 0) {
					string_ptr<WSTRING> wstrLhsLower(tolower(wstrLhs.get()));
					if (!wstrLhsLower.get())
						return QSTATUS_OUTOFMEMORY;
					wstrLhs.reset(wstrLhsLower.release());
				}
				wstrRhs.reset(wstrRhsLower.release());
			}
			
			BMFindString<WSTRING> bmfs(wstrRhs.get(), &status);
			CHECK_QSTATUS();
			bResult = bmfs.find(wstrLhs.get()) != 0;
		}
	}
	
	return MacroValueFactory::getFactory().newBoolean(bResult,
		reinterpret_cast<MacroValueBoolean**>(ppValue));
}

const WCHAR* qm::MacroFunctionContain::getName() const
{
	if (bBeginWith_)
		return L"BeginWith";
	else
		return L"Contain";
}


/****************************************************************************
 *
 * MacroFunctionCopy
 *
 */

qm::MacroFunctionCopy::MacroFunctionCopy(
	bool bMove, QSTATUS* pstatus) :
	MacroFunction(pstatus),
	bMove_(bMove)
{
}

qm::MacroFunctionCopy::~MacroFunctionCopy()
{
}

QSTATUS qm::MacroFunctionCopy::value(
	MacroContext* pContext, MacroValue** ppValue) const
{
	assert(pContext);
	assert(ppValue);
	
	DECLARE_QSTATUS();
	
	*ppValue = 0;
	
	size_t nSize = getArgSize();
	if (nSize != 1)
		return error(*pContext, MacroErrorHandler::CODE_INVALIDARGSIZE);
	
	MessageHolderBase* pmh = pContext->getMessageHolder();
	if (!pmh)
		return error(*pContext, MacroErrorHandler::CODE_NOCONTEXTMESSAGE);
	assert(pmh->getMessageHolder());
	
	MacroValuePtr pValue;
	status = getArg(0)->value(pContext, &pValue);
	CHECK_QSTATUS();
	string_ptr<WSTRING> wstrFolder;
	status = pValue->string(&wstrFolder);
	CHECK_QSTATUS();
	
	Folder* pFolderTo = 0;
	status = pContext->getDocument()->getFolder(
		pContext->getAccount(), wstrFolder.get(), &pFolderTo);
	CHECK_QSTATUS();
	if (!pFolderTo || pFolderTo->getType() != Folder::TYPE_NORMAL)
		return QSTATUS_FAIL;
	
	Folder* pFolderFrom = pmh->getFolder();
	assert(pFolderFrom->isLocked());
	
	Folder::MessageHolderList l;
	status = STLWrapper<Folder::MessageHolderList>(
		l).push_back(pmh->getMessageHolder());
	CHECK_QSTATUS();
	status = pFolderFrom->copyMessages(l,
		static_cast<NormalFolder*>(pFolderTo), bMove_, 0);
	CHECK_QSTATUS();
	
	return MacroValueFactory::getFactory().newBoolean(true,
		reinterpret_cast<MacroValueBoolean**>(ppValue));
}

const WCHAR* qm::MacroFunctionCopy::getName() const
{
	if (bMove_)
		return L"Move";
	else
		return L"Copy";
}


/****************************************************************************
 *
 * MacroFunctionDate
 *
 */

qm::MacroFunctionDate::MacroFunctionDate(QSTATUS* pstatus) :
	MacroFunction(pstatus)
{
}

qm::MacroFunctionDate::~MacroFunctionDate()
{
}

QSTATUS qm::MacroFunctionDate::value(
	MacroContext* pContext, MacroValue** ppValue) const
{
	assert(pContext);
	assert(ppValue);
	
	DECLARE_QSTATUS();
	
	*ppValue = 0;
	
	size_t nSize = getArgSize();
	if (nSize > 1)
		return error(*pContext, MacroErrorHandler::CODE_INVALIDARGSIZE);
	
	Time time;
	if (nSize == 1) {
		MacroValuePtr pValue;
		status = getArg(0)->value(pContext, &pValue);
		CHECK_QSTATUS();
		string_ptr<WSTRING> wstr;
		status = pValue->string(&wstr);
		CHECK_QSTATUS();
		
		string_ptr<STRING> str(wcs2mbs(wstr.get()));
		if (!str.get())
			return QSTATUS_OUTOFMEMORY;
		status = DateParser::parse(str.get(), static_cast<size_t>(-1),
			Part::isGlobalOption(Part::O_ALLOW_SINGLE_DIGIT_TIME), &time);
		if (status == QSTATUS_FAIL)
			time = Time::getCurrentTime();
		else
			CHECK_QSTATUS();
	}
	else {
		time = Time::getCurrentTime();
	}
	
	return MacroValueFactory::getFactory().newTime(time,
		reinterpret_cast<MacroValueTime**>(ppValue));
}

const WCHAR* qm::MacroFunctionDate::getName() const
{
	return L"Date";
}


/****************************************************************************
 *
 * MacroFunctionDecode
 *
 */

qm::MacroFunctionDecode::MacroFunctionDecode(QSTATUS* pstatus) :
	MacroFunction(pstatus)
{
}

qm::MacroFunctionDecode::~MacroFunctionDecode()
{
}

QSTATUS qm::MacroFunctionDecode::value(
	MacroContext* pContext, MacroValue** ppValue) const
{
	assert(pContext);
	assert(ppValue);
	
	DECLARE_QSTATUS();
	
	*ppValue = 0;
	
	if (getArgSize() != 1)
		return error(*pContext, MacroErrorHandler::CODE_INVALIDARGSIZE);
	
	MacroValuePtr pValue;
	status = getArg(0)->value(pContext, &pValue);
	CHECK_QSTATUS();
	string_ptr<WSTRING> wstr;
	status = pValue->string(&wstr);
	CHECK_QSTATUS();
	
	if (FieldParser::isAscii(wstr.get())) {
		string_ptr<STRING> str(wcs2mbs(wstr.get()));
		if (!str.get())
			return QSTATUS_OUTOFMEMORY;
		wstr.reset(0);
		status = FieldParser::decode(str.get(),
			static_cast<size_t>(-1), &wstr, 0);
		CHECK_QSTATUS();
	}
	
	return MacroValueFactory::getFactory().newString(wstr.get(),
		reinterpret_cast<MacroValueString**>(ppValue));
}

const WCHAR* qm::MacroFunctionDecode::getName() const
{
	return L"Decode";
}


/****************************************************************************
 *
 * MacroFunctionDefun
 *
 */

qm::MacroFunctionDefun::MacroFunctionDefun(QSTATUS* pstatus) :
	MacroFunction(pstatus)
{
}

qm::MacroFunctionDefun::~MacroFunctionDefun()
{
}

QSTATUS qm::MacroFunctionDefun::value(
	MacroContext* pContext, MacroValue** ppValue) const
{
	assert(pContext);
	assert(ppValue);
	
	DECLARE_QSTATUS();
	
	*ppValue = 0;
	
	if (getArgSize() != 2)
		return error(*pContext, MacroErrorHandler::CODE_INVALIDARGSIZE);
	
	MacroValuePtr pValue;
	status = getArg(0)->value(pContext, &pValue);
	CHECK_QSTATUS();
	
	string_ptr<WSTRING> wstrName;
	status = pValue->string(&wstrName);
	CHECK_QSTATUS();
	
	bool bSet = false;
	status = pContext->setFunction(wstrName.get(), getArg(1), &bSet);
	CHECK_QSTATUS();
	
	return MacroValueFactory::getFactory().newBoolean(bSet,
		reinterpret_cast<MacroValueBoolean**>(ppValue));
}

const WCHAR* qm::MacroFunctionDefun::getName() const
{
	return L"Defun";
}


/****************************************************************************
 *
 * MacroFunctionDelete
 *
 */

qm::MacroFunctionDelete::MacroFunctionDelete(QSTATUS* pstatus) :
	MacroFunction(pstatus)
{
}

qm::MacroFunctionDelete::~MacroFunctionDelete()
{
}

QSTATUS qm::MacroFunctionDelete::value(
	MacroContext* pContext, MacroValue** ppValue) const
{
	assert(pContext);
	assert(ppValue);
	
	DECLARE_QSTATUS();
	
	*ppValue = 0;
	
	if (getArgSize() != 0)
		return error(*pContext, MacroErrorHandler::CODE_INVALIDARGSIZE);
	
	MessageHolderBase* pmh = pContext->getMessageHolder();
	if (!pmh)
		return error(*pContext, MacroErrorHandler::CODE_NOCONTEXTMESSAGE);
	assert(pmh->getMessageHolder());
	
	Folder* pFolder = pmh->getFolder();
	assert(pFolder->isLocked());
	
	Folder::MessageHolderList l;
	status = STLWrapper<Folder::MessageHolderList>(
		l).push_back(pmh->getMessageHolder());
	CHECK_QSTATUS();
	status = pFolder->removeMessages(l, false, 0);
	CHECK_QSTATUS();
	
	return MacroValueFactory::getFactory().newBoolean(true,
		reinterpret_cast<MacroValueBoolean**>(ppValue));
}

const WCHAR* qm::MacroFunctionDelete::getName() const
{
	return L"Delete";
}


/****************************************************************************
 *
 * MacroFunctionEqual
 *
 */

qm::MacroFunctionEqual::MacroFunctionEqual(QSTATUS* pstatus) :
	MacroFunction(pstatus)
{
}

qm::MacroFunctionEqual::~MacroFunctionEqual()
{
}

QSTATUS qm::MacroFunctionEqual::value(
	MacroContext* pContext, MacroValue** ppValue) const
{
	assert(pContext);
	assert(ppValue);
	
	DECLARE_QSTATUS();
	
	*ppValue = 0;
	
	size_t nSize = getArgSize();
	if (nSize != 2 && nSize != 3)
		return error(*pContext, MacroErrorHandler::CODE_INVALIDARGSIZE);
	
	bool bCase = false;
	if (nSize == 3) {
		MacroValuePtr pValueCase;
		status = getArg(2)->value(pContext, &pValueCase);
		CHECK_QSTATUS();
		bCase = pValueCase->boolean();
	}
	
	MacroValuePtr pValueLhs;
	status = getArg(0)->value(pContext, &pValueLhs);
	CHECK_QSTATUS();
	
	MacroValuePtr pValueRhs;
	status = getArg(1)->value(pContext, &pValueRhs);
	CHECK_QSTATUS();
	
	bool bEqual = false;
	if (pValueLhs->getType() == MacroValue::TYPE_BOOLEAN &&
		pValueRhs->getType() == MacroValue::TYPE_BOOLEAN) {
		bEqual = pValueLhs->boolean() == pValueRhs->boolean();
	}
	else if (pValueLhs->getType() == MacroValue::TYPE_NUMBER &&
		pValueRhs->getType() == MacroValue::TYPE_NUMBER) {
		bEqual = pValueLhs->number() == pValueRhs->number();
	}
	else {
		string_ptr<WSTRING> wstrLhs;
		status = pValueLhs->string(&wstrLhs);
		CHECK_QSTATUS();
		
		string_ptr<WSTRING> wstrRhs;
		status = pValueRhs->string(&wstrRhs);
		CHECK_QSTATUS();
		
		bEqual = _wcsicmp(wstrLhs.get(), wstrRhs.get()) == 0;
	}
	
	return MacroValueFactory::getFactory().newBoolean(bEqual,
		reinterpret_cast<MacroValueBoolean**>(ppValue));
}

const WCHAR* qm::MacroFunctionEqual::getName() const
{
	return L"Equal";
}


/****************************************************************************
 *
 * MacroFunctionEval
 *
 */

qm::MacroFunctionEval::MacroFunctionEval(
	MacroParser::Type type, QSTATUS* pstatus) :
	MacroFunction(pstatus),
	type_(type)
{
}

qm::MacroFunctionEval::~MacroFunctionEval()
{
}

QSTATUS qm::MacroFunctionEval::value(
	MacroContext* pContext, MacroValue** ppValue) const
{
	assert(pContext);
	assert(ppValue);
	
	DECLARE_QSTATUS();
	
	*ppValue = 0;
	
	if (getArgSize() != 1)
		return error(*pContext, MacroErrorHandler::CODE_INVALIDARGSIZE);
	
	MacroValuePtr pValue;
	status = getArg(0)->value(pContext, &pValue);
	CHECK_QSTATUS();
	
	string_ptr<WSTRING> wstrExpr;
	status = pValue->string(&wstrExpr);
	CHECK_QSTATUS();
	
	MacroParser parser(type_, &status);
	CHECK_QSTATUS();
	
	Macro* pMacro = 0;
	status = parser.parse(wstrExpr.get(), &pMacro);
	CHECK_QSTATUS();
	std::auto_ptr<Macro> p(pMacro);
	
	return pMacro->value(pContext, ppValue);
}

const WCHAR* qm::MacroFunctionEval::getName() const
{
	return L"Eval";
}


/****************************************************************************
 *
 * MacroFunctionExecute
 *
 */

namespace {
DWORD WINAPI writeProc(void* pParam)
{
	std::pair<const CHAR*, HANDLE> p =
		*static_cast<std::pair<const CHAR*, HANDLE>*>(pParam);
	const CHAR* psz = p.first;
	HANDLE hInput = p.second;
	size_t nLen = strlen(psz);
	DWORD dwWrite = 0;
	BOOL b = ::WriteFile(hInput, psz, nLen, &dwWrite, 0);
	::CloseHandle(hInput);
	return (b && dwWrite == nLen) ? 0 : 1;
}
}

qm::MacroFunctionExecute::MacroFunctionExecute(QSTATUS* pstatus) :
	MacroFunction(pstatus)
{
}

qm::MacroFunctionExecute::~MacroFunctionExecute()
{
}

QSTATUS qm::MacroFunctionExecute::value(
	MacroContext* pContext, MacroValue** ppValue) const
{
	assert(pContext);
	assert(ppValue);
	
	DECLARE_QSTATUS();
	
	*ppValue = 0;
	
	size_t nSize = getArgSize();
#ifdef _WIN32_WCE
	if (nSize != 1)
#else
	if (nSize != 1 && nSize != 2)
#endif
		return error(*pContext, MacroErrorHandler::CODE_INVALIDARGSIZE);
	
	MacroValuePtr pValueCommand;
	status = getArg(0)->value(pContext, &pValueCommand);
	CHECK_QSTATUS();
	string_ptr<WSTRING> wstrCommand;
	status = pValueCommand->string(&wstrCommand);
	CHECK_QSTATUS();
	
	const WCHAR* pwszResult = L"";
	string_ptr<WSTRING> wstrOutput;
	if (nSize > 1) {
#ifndef _WIN32_WCE
		MacroValuePtr pValueInput;
		status = getArg(1)->value(pContext, &pValueInput);
		CHECK_QSTATUS();
		string_ptr<WSTRING> wstrInput;
		status = pValueInput->string(&wstrInput);
		CHECK_QSTATUS();
		
		SECURITY_ATTRIBUTES sa = { sizeof(sa), 0, TRUE };
		AutoHandle hInputRead;
		AutoHandle hInputWrite;
		if (!::CreatePipe(&hInputRead, &hInputWrite, &sa, 0))
			return error(*pContext, MacroErrorHandler::CODE_FAIL);
		AutoHandle hInput;
		if (!::DuplicateHandle(::GetCurrentProcess(), hInputWrite.get(),
			::GetCurrentProcess(), &hInput, 0, FALSE, DUPLICATE_SAME_ACCESS))
			return error(*pContext, MacroErrorHandler::CODE_FAIL);
		hInputWrite.close();
		
		AutoHandle hOutputRead;
		AutoHandle hOutputWrite;
		if (!::CreatePipe(&hOutputRead, &hOutputWrite, &sa, 0))
			return error(*pContext, MacroErrorHandler::CODE_FAIL);
		AutoHandle hOutput;
		if (!::DuplicateHandle(::GetCurrentProcess(), hOutputRead.get(),
			::GetCurrentProcess(), &hOutput, 0, FALSE, DUPLICATE_SAME_ACCESS))
			return error(*pContext, MacroErrorHandler::CODE_FAIL);
		hOutputRead.close();
		
		STARTUPINFO si = { sizeof(si) };
		si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
		si.hStdInput = hInputRead.get();
		si.hStdOutput = hOutputWrite.get();
		si.wShowWindow = SW_HIDE;
		PROCESS_INFORMATION pi;
		W2T(wstrCommand.get(), ptszCommand);
		if (!::CreateProcess(0, const_cast<LPTSTR>(ptszCommand),
			0, 0, TRUE, 0, 0, 0, &si, &pi))
			return error(*pContext, MacroErrorHandler::CODE_FAIL);
		AutoHandle hProcess(pi.hProcess);
		::CloseHandle(pi.hThread);
		hInputRead.close();
		hOutputWrite.close();
		
		string_ptr<STRING> strInput(wcs2mbs(wstrInput.get()));
		if (!strInput.get())
			return QSTATUS_OUTOFMEMORY;
		std::pair<const CHAR*, HANDLE> p(strInput.get(), hInput.get());
		DWORD dwThreadId = 0;
		AutoHandle hThread(::CreateThread(0, 0, writeProc, &p, 0, &dwThreadId));
		if (!hThread.get())
			return error(*pContext, MacroErrorHandler::CODE_FAIL);
		hInput.release();
		
		StringBuffer<STRING> bufRead(&status);
		CHECK_QSTATUS();
		char buf[1024];
		DWORD dwRead = 0;
		while (::ReadFile(hOutput.get(), buf, sizeof(buf), &dwRead, 0) && dwRead != 0) {
			status = bufRead.append(buf, dwRead);
			CHECK_QSTATUS();
		}
		wstrOutput.reset(mbs2wcs(bufRead.getCharArray()));
		if (!wstrOutput.get())
			return QSTATUS_OUTOFMEMORY;
		
		HANDLE hWaits[] = { hProcess.get(), hThread.get() };
		::WaitForMultipleObjects(sizeof(hWaits)/sizeof(hWaits[0]),
			hWaits, TRUE, INFINITE);
		
		DWORD dwExitCode = 0;
		if (!::GetExitCodeThread(hThread.get(), &dwExitCode) || dwExitCode != 0)
			return error(*pContext, MacroErrorHandler::CODE_FAIL);
		
		pwszResult = wstrOutput.get();
#endif
	}
	else {
		WCHAR* pParam = wstrCommand.get();
		bool bQuote = false;
		while (*pParam) {
			if (*pParam == L'\"')
				bQuote = !bQuote;
			else if (*pParam == L' ' && !bQuote)
				break;
			++pParam;
		}
		if (*pParam) {
			assert(*pParam == L' ');
			*pParam = L'\0';
			++pParam;
		}
		W2T(wstrCommand.get(), ptszCommand);
		W2T(pParam, ptszParam);
		
		SHELLEXECUTEINFO sei = {
			sizeof(sei),
			0,
			pContext->getWindow(),
			0,
			ptszCommand,
			ptszParam,
			0,
			SW_SHOW
		};
		if (!::ShellExecuteEx(&sei))
			return error(*pContext, MacroErrorHandler::CODE_FAIL);
	}
	
	return MacroValueFactory::getFactory().newString(pwszResult,
		reinterpret_cast<MacroValueString**>(ppValue));
}

const WCHAR* qm::MacroFunctionExecute::getName() const
{
	return L"Execute";
}


/****************************************************************************
 *
 * MacroFunctionExist
 *
 */

qm::MacroFunctionExist::MacroFunctionExist(QSTATUS* pstatus) :
	MacroFunction(pstatus)
{
}

qm::MacroFunctionExist::~MacroFunctionExist()
{
}

QSTATUS qm::MacroFunctionExist::value(
	MacroContext* pContext, MacroValue** ppValue) const
{
	assert(pContext);
	assert(ppValue);
	
	DECLARE_QSTATUS();
	
	*ppValue = 0;
	
	if (getArgSize() != 1)
		return error(*pContext, MacroErrorHandler::CODE_INVALIDARGSIZE);
	
	MessageHolderBase* pmh = pContext->getMessageHolder();
	if (!pmh)
		return error(*pContext, MacroErrorHandler::CODE_NOCONTEXTMESSAGE);
	
	MacroValuePtr pValue;
	status = getArg(0)->value(pContext, &pValue);
	CHECK_QSTATUS();
	
	string_ptr<WSTRING> wstrName;
	status = pValue->string(&wstrName);
	CHECK_QSTATUS();
	
	Message* pMessage = 0;
	status = pContext->getMessage(MacroContext::MESSAGETYPE_HEADER,
		wstrName.get(), &pMessage);
	CHECK_QSTATUS();
	
	bool bHas = false;
	status = pMessage->hasField(wstrName.get(), &bHas);
	CHECK_QSTATUS();
	
	return MacroValueFactory::getFactory().newBoolean(bHas,
		reinterpret_cast<MacroValueBoolean**>(ppValue));
}

const WCHAR* qm::MacroFunctionExist::getName() const
{
	return L"Exist";
}


/****************************************************************************
 *
 * MacroFunctionExit
 *
 */

qm::MacroFunctionExit::MacroFunctionExit(QSTATUS* pstatus) :
	MacroFunction(pstatus)
{
}

qm::MacroFunctionExit::~MacroFunctionExit()
{
}

QSTATUS qm::MacroFunctionExit::value(
	MacroContext* pContext, MacroValue** ppValue) const
{
	assert(pContext);
	assert(ppValue);
	
	DECLARE_QSTATUS();
	
	*ppValue = 0;
	
	size_t nSize = getArgSize();
	if (nSize < 2 || 4 < nSize)
		return error(*pContext, MacroErrorHandler::CODE_INVALIDARGSIZE);
	
	// TODO
}

const WCHAR* qm::MacroFunctionExit::getName() const
{
	return L"Exit";
}


/****************************************************************************
 *
 * MacroFunctionField
 *
 */

qm::MacroFunctionField::MacroFunctionField(QSTATUS* pstatus) :
	MacroFunction(pstatus)
{
}

qm::MacroFunctionField::~MacroFunctionField()
{
}

QSTATUS qm::MacroFunctionField::value(
	MacroContext* pContext, MacroValue** ppValue) const
{
	assert(pContext);
	assert(ppValue);
	
	DECLARE_QSTATUS();
	
	*ppValue = 0;
	
	size_t nSize = getArgSize();
	if (nSize != 1 && nSize != 2)
		return error(*pContext, MacroErrorHandler::CODE_INVALIDARGSIZE);
	
	MessageHolderBase* pmh = pContext->getMessageHolder();
	if (!pmh)
		return error(*pContext, MacroErrorHandler::CODE_NOCONTEXTMESSAGE);
	
	MacroValuePtr pValue;
	status = getArg(0)->value(pContext, &pValue);
	CHECK_QSTATUS();
	
	string_ptr<WSTRING> wstrName;
	status = pValue->string(&wstrName);
	CHECK_QSTATUS();
	
	const Part* pPart = 0;
	if (nSize > 1) {
		status = getPart(pContext, 1, &pPart);
		CHECK_QSTATUS();
	}
	else {
		Message* pMessage = 0;
		status = pContext->getMessage(MacroContext::MESSAGETYPE_HEADER,
			wstrName.get(), &pMessage);
		CHECK_QSTATUS();
		pPart = pMessage;
	
	}
	
	string_ptr<STRING> strHeader;
	status = PartUtil(*pPart).getHeader(wstrName.get(), &strHeader);
	CHECK_QSTATUS();
	
	return MacroValueFactory::getFactory().newField(wstrName.get(),
		strHeader.get(), reinterpret_cast<MacroValueField**>(ppValue));
}

const WCHAR* qm::MacroFunctionField::getName() const
{
	return L"Field";
}


/****************************************************************************
 *
 * MacroFunctionFieldParameter
 *
 */

qm::MacroFunctionFieldParameter::MacroFunctionFieldParameter(QSTATUS* pstatus) :
	MacroFunction(pstatus)
{
}

qm::MacroFunctionFieldParameter::~MacroFunctionFieldParameter()
{
}

QSTATUS qm::MacroFunctionFieldParameter::value(
	MacroContext* pContext, MacroValue** ppValue) const
{
	assert(pContext);
	assert(ppValue);
	
	DECLARE_QSTATUS();
	
	*ppValue = 0;
	
	size_t nSize = getArgSize();
	if (nSize == 0 || nSize > 3)
		return error(*pContext, MacroErrorHandler::CODE_INVALIDARGSIZE);
	
	MessageHolderBase* pmh = pContext->getMessageHolder();
	if (!pmh)
		return error(*pContext, MacroErrorHandler::CODE_NOCONTEXTMESSAGE);
	
	MacroValuePtr pValue;
	status = getArg(0)->value(pContext, &pValue);
	CHECK_QSTATUS();
	string_ptr<WSTRING> wstrName;
	status = pValue->string(&wstrName);
	CHECK_QSTATUS();
	
	const Part* pPart = 0;
	if (nSize > 2) {
		status = getPart(pContext, 2, &pPart);
		CHECK_QSTATUS();
	}
	else {
		Message* pMessage = 0;
		status = pContext->getMessage(MacroContext::MESSAGETYPE_HEADER,
			wstrName.get(), &pMessage);
		CHECK_QSTATUS();
		pPart = pMessage;
	}
	
	string_ptr<WSTRING> wstrParamName;
	if (nSize > 1) {
		MacroValuePtr pValue;
		status = getArg(1)->value(pContext, &pValue);
		CHECK_QSTATUS();
		status = pValue->string(&wstrParamName);
		CHECK_QSTATUS();
	}
	
	const WCHAR* pwszValue = 0;
	string_ptr<WSTRING> wstrValue;
	SimpleParameterParser parser(&status);
	CHECK_QSTATUS();
	Part::Field field;
	status = pPart->getField(wstrName.get(), &parser, &field);
	CHECK_QSTATUS();
	if (field == Part::FIELD_EXIST) {
		if (wstrParamName.get()) {
			status = parser.getParameter(wstrParamName.get(), &wstrValue);
			CHECK_QSTATUS();
			pwszValue = wstrValue.get();
		}
		else {
			pwszValue = parser.getValue();
		}
	}
	if (!pwszValue)
		pwszValue = L"";
	
	return MacroValueFactory::getFactory().newString(pwszValue,
		reinterpret_cast<MacroValueString**>(ppValue));
}

const WCHAR* qm::MacroFunctionFieldParameter::getName() const
{
	return L"FieldParameter";
}


/****************************************************************************
 *
 * MacroFunctionFind
 *
 */

qm::MacroFunctionFind::MacroFunctionFind(QSTATUS* pstatus) :
	MacroFunction(pstatus)
{
}

qm::MacroFunctionFind::~MacroFunctionFind()
{
}

QSTATUS qm::MacroFunctionFind::value(
	MacroContext* pContext, MacroValue** ppValue) const
{
	assert(pContext);
	assert(ppValue);
	
	DECLARE_QSTATUS();
	
	*ppValue = 0;
	
	size_t nSize = getArgSize();
	if (nSize < 2 || 4 < nSize)
		return error(*pContext, MacroErrorHandler::CODE_INVALIDARGSIZE);
	
	bool bCase = false;
	if (nSize > 3) {
		MacroValuePtr pValue;
		status = getArg(3)->value(pContext, &pValue);
		CHECK_QSTATUS();
		bCase = pValue->boolean();
	}
	
	unsigned int nIndex = 0;
	if (nSize > 2) {
		MacroValuePtr pValue;
		status = getArg(2)->value(pContext, &pValue);
		CHECK_QSTATUS();
		nIndex = pValue->number();
	}
	
	MacroValuePtr pValue;
	status = getArg(0)->value(pContext, &pValue);
	CHECK_QSTATUS();
	
	string_ptr<WSTRING> wstr;
	status = pValue->string(&wstr);
	CHECK_QSTATUS();
	
	MacroValuePtr pValueSep;
	status = getArg(1)->value(pContext, &pValueSep);
	CHECK_QSTATUS();
	
	string_ptr<WSTRING> wstrSep;
	status = pValueSep->string(&wstrSep);
	CHECK_QSTATUS();
	if (!bCase) {
		string_ptr<WSTRING> wstrSepLower(tolower(wstrSep.get()));
		if (!wstrSepLower.get())
			return QSTATUS_OUTOFMEMORY;
		string_ptr<WSTRING> wstrSepUpper(toupper(wstrSep.get()));
		if (!wstrSepUpper.get())
			return QSTATUS_OUTOFMEMORY;
		if (wcscmp(wstrSepLower.get(), wstrSepUpper.get()) != 0) {
			string_ptr<WSTRING> wstrLower(tolower(wstr.get()));
			if (!wstrLower.get())
				return QSTATUS_OUTOFMEMORY;
			wstr.reset(wstrLower.release());
			wstrSep.reset(wstrSepLower.release());
		}
	}
	
	BMFindString<WSTRING> bmfs(wstrSep.get(), &status);
	CHECK_QSTATUS();
	
	const WCHAR* p = bmfs.find(wstr.get(), nIndex);
	nIndex = p ? p - wstr.get() : -1;
	
	return MacroValueFactory::getFactory().newNumber(nIndex,
		reinterpret_cast<MacroValueNumber**>(ppValue));
}

const WCHAR* qm::MacroFunctionFind::getName() const
{
	return L"Find";
}


/****************************************************************************
 *
 * MacroFunctionFlag
 *
 */

qm::MacroFunctionFlag::MacroFunctionFlag(QSTATUS* pstatus) :
	MacroFunction(pstatus),
	flag_(static_cast<MessageHolder::Flag>(0)),
	bCustom_(true)
{
}

qm::MacroFunctionFlag::MacroFunctionFlag(
	MessageHolder::Flag flag, QSTATUS* pstatus) :
	MacroFunction(pstatus),
	flag_(flag),
	bCustom_(false)
{
}

qm::MacroFunctionFlag::~MacroFunctionFlag()
{
}

QSTATUS qm::MacroFunctionFlag::value(
	MacroContext* pContext, MacroValue** ppValue) const
{
	assert(pContext);
	assert(ppValue);
	
	DECLARE_QSTATUS();
	
	*ppValue = 0;
	
	MessageHolderBase* pmh = pContext->getMessageHolder();
	if (!pmh)
		return error(*pContext, MacroErrorHandler::CODE_NOCONTEXTMESSAGE);
	
	size_t nSize = getArgSize();
	unsigned int nFlags = flag_;
	if (bCustom_) {
		if (nSize == 0)
			return error(*pContext, MacroErrorHandler::CODE_INVALIDARGSIZE);
		MacroValuePtr pValue;
		status = getArg(0)->value(pContext, &pValue);
		CHECK_QSTATUS();
		nFlags = pValue->number();
	}
	bool bCanModify = (nFlags & MessageHolder::FLAG_USER_MASK) != 0;
	size_t nBase = bCustom_ ? 1 : 0;
	if (nSize != nBase && !(bCanModify && nSize == nBase + 1))
		return error(*pContext, MacroErrorHandler::CODE_INVALIDARGSIZE);
	
	if (nSize == nBase + 1) {
		if (!pmh->getMessageHolder())
			return QSTATUS_FAIL;
		
		MacroValuePtr pValue;
		status = getArg(nBase)->value(pContext, &pValue);
		CHECK_QSTATUS();
		
		Folder* pFolder = pmh->getFolder();
		MessagePtrList l;
		status = STLWrapper<MessagePtrList>(l).push_back(
			MessagePtr(pmh->getMessageHolder()));
		CHECK_QSTATUS();
		status = pFolder->setMessagesFlags(l,
			pValue->boolean() ? nFlags : 0, nFlags);
		CHECK_QSTATUS();
	}
	
	return MacroValueFactory::getFactory().newBoolean(
		(pmh->getFlags() & nFlags) != 0,
		reinterpret_cast<MacroValueBoolean**>(ppValue));
}

const WCHAR* qm::MacroFunctionFlag::getName() const
{
	if (bCustom_) {
		return L"Flag";
	}
	else {
		switch (flag_) {
		case MessageHolder::FLAG_SEEN:
			return L"Seen";
		case MessageHolder::FLAG_REPLIED:
			return L"Replied";
		case MessageHolder::FLAG_FORWARDED:
			return L"Forwarded";
		case MessageHolder::FLAG_SENT:
			return L"Sent";
		case MessageHolder::FLAG_DRAFT:
			return L"Draft";
		case MessageHolder::FLAG_MARKED:
			return L"Marked";
		case MessageHolder::FLAG_DELETED:
			return L"Deleted";
		case MessageHolder::FLAG_DOWNLOAD:
			return L"Download";
		case MessageHolder::FLAG_DOWNLOADTEXT:
			return L"DownloadText";
		case MessageHolder::FLAG_MULTIPART:
			return L"Multipart";
		case MessageHolder::FLAG_PARTIAL_MASK:
			return L"Partial";
		default:
			assert(false);
			return 0;
		}
	}
}


/****************************************************************************
 *
 * MacroFunctionFolder
 *
 */

qm::MacroFunctionFolder::MacroFunctionFolder(QSTATUS* pstatus) :
	MacroFunction(pstatus)
{
}

qm::MacroFunctionFolder::~MacroFunctionFolder()
{
}

QSTATUS qm::MacroFunctionFolder::value(
	MacroContext* pContext, MacroValue** ppValue) const
{
	assert(pContext);
	assert(ppValue);
	
	DECLARE_QSTATUS();
	
	*ppValue = 0;
	
	size_t nSize = getArgSize();
	if (nSize > 1)
		return error(*pContext, MacroErrorHandler::CODE_INVALIDARGSIZE);
	
	MessageHolderBase* pmh = pContext->getMessageHolder();
	if (!pmh)
		return error(*pContext, MacroErrorHandler::CODE_NOCONTEXTMESSAGE);
	
	bool bFull = true;
	if (nSize > 0) {
		MacroValuePtr pValue;
		status = getArg(0)->value(pContext, &pValue);
		CHECK_QSTATUS();
		bFull = pValue->boolean();
	}
	
	NormalFolder* pFolder = pmh->getFolder();
	const WCHAR* pwszName = 0;
	string_ptr<WSTRING> wstrName;
	if (bFull) {
		status = pFolder->getFullName(&wstrName);
		CHECK_QSTATUS();
		pwszName = wstrName.get();
	}
	else {
		pwszName = pFolder->getName();
	}
	
	return MacroValueFactory::getFactory().newString(pwszName,
		reinterpret_cast<MacroValueString**>(ppValue));
}

const WCHAR* qm::MacroFunctionFolder::getName() const
{
	return L"Folder";
}


/****************************************************************************
 *
 * MacroFunctionForEach
 *
 */

qm::MacroFunctionForEach::MacroFunctionForEach(QSTATUS* pstatus) :
	MacroFunction(pstatus)
{
}

qm::MacroFunctionForEach::~MacroFunctionForEach()
{
}

QSTATUS qm::MacroFunctionForEach::value(
	MacroContext* pContext, MacroValue** ppValue) const
{
	assert(pContext);
	assert(ppValue);
	
	DECLARE_QSTATUS();
	
	*ppValue = 0;
	
	if (getArgSize() != 2)
		return error(*pContext, MacroErrorHandler::CODE_INVALIDARGSIZE);
	
	MacroValuePtr pValueMessages;
	status = getArg(0)->value(pContext, &pValueMessages);
	CHECK_QSTATUS();
	if (pValueMessages->getType() != MacroValue::TYPE_MESSAGELIST)
		return QSTATUS_FAIL;
	
	const MacroValueMessageList::MessageList& l =
		static_cast<MacroValueMessageList*>(pValueMessages.get())->getMessageList();
	MacroValueMessageList::MessageList::const_iterator it = l.begin();
	while (it != l.end()) {
		MessagePtrLock mpl(*it);
		if (mpl) {
			Message msg(&status);
			CHECK_QSTATUS();
			MacroContext context(mpl, &msg, pContext, &status);
			MacroValuePtr pValue;
			status = getArg(1)->value(&context, &pValue);
			CHECK_QSTATUS();
		}
		++it;
	}
	
	return MacroValueFactory::getFactory().newBoolean(true,
		reinterpret_cast<MacroValueBoolean**>(ppValue));
}

const WCHAR* qm::MacroFunctionForEach::getName() const
{
	return L"ForEach";
}


/****************************************************************************
 *
 * MacroFunctionFormatAddress
 *
 */

qm::MacroFunctionFormatAddress::MacroFunctionFormatAddress(QSTATUS* pstatus) :
	MacroFunction(pstatus)
{
}

qm::MacroFunctionFormatAddress::~MacroFunctionFormatAddress()
{
}

QSTATUS qm::MacroFunctionFormatAddress::value(
	MacroContext* pContext, MacroValue** ppValue) const
{
	assert(pContext);
	assert(ppValue);
	
	DECLARE_QSTATUS();
	
	size_t nSize = getArgSize();
	if (nSize < 1 || 3 < nSize)
		return error(*pContext, MacroErrorHandler::CODE_INVALIDARGSIZE);
	
	bool bLookup = false;
	if (nSize > 2) {
		MacroValuePtr pValue;
		status = getArg(2)->value(pContext, &pValue);
		CHECK_QSTATUS();
		bLookup = pValue->boolean();
	}
	
	enum Type {
		TYPE_ALL,
		TYPE_ADDRESS,
		TYPE_NAME
	};
	Type type = TYPE_ALL;
	if (nSize > 1) {
		MacroValuePtr pValue;
		status = getArg(1)->value(pContext, &pValue);
		CHECK_QSTATUS();
		unsigned int n = pValue->number();
		if (n > 2)
			n = 0;
		type = static_cast<Type>(n);
	}
	
	MacroValuePtr pValue;
	status = getArg(0)->value(pContext, &pValue);
	CHECK_QSTATUS();
	if (pValue->getType() != MacroValue::TYPE_FIELD)
		return error(*pContext, MacroErrorHandler::CODE_INVALIDARGTYPE);
	
	MacroValueField* pValueField = static_cast<MacroValueField*>(pValue.get());
	
	string_ptr<WSTRING> wstrValue;
	
	const CHAR* pszField = pValueField->getField();
	if (pszField) {
		Part part(0, pszField, static_cast<size_t>(-1), &status);
		CHECK_QSTATUS();
		AddressListParser address(0, &status);
		CHECK_QSTATUS();
		Part::Field field;
		status = part.getField(pValueField->getName(), &address, &field);
		CHECK_QSTATUS();
		if (field == Part::FIELD_EXIST) {
			if (bLookup) {
				AddressBook* pAddressBook = pContext->getDocument()->getAddressBook();
				status = replacePhrase(pAddressBook, &address);
				CHECK_QSTATUS();
			}
			switch (type) {
			case TYPE_ALL:
				status = address.getValue(&wstrValue);
				CHECK_QSTATUS();
				break;
			case TYPE_ADDRESS:
				status = address.getAddresses(&wstrValue);
				CHECK_QSTATUS();
				break;
			case TYPE_NAME:
				status = address.getNames(&wstrValue);
				CHECK_QSTATUS();
				break;
			default:
				assert(false);
				break;
			}
		}
	}
	
	const WCHAR* pwszValue = wstrValue.get() ? wstrValue.get() : L"";
	return MacroValueFactory::getFactory().newString(pwszValue,
		reinterpret_cast<MacroValueString**>(ppValue));
}

const WCHAR* qm::MacroFunctionFormatAddress::getName() const
{
	return L"FormatAddress";
}

QSTATUS qm::MacroFunctionFormatAddress::replacePhrase(
	AddressBook* pAddressBook, AddressListParser* pAddressList)
{
	assert(pAddressBook);
	assert(pAddressList);
	
	DECLARE_QSTATUS();
	
	const AddressListParser::AddressList& l = pAddressList->getAddressList();
	AddressListParser::AddressList::const_iterator it = l.begin();
	while (it != l.end()) {
		status = replacePhrase(pAddressBook, *it);
		CHECK_QSTATUS();
		++it;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MacroFunctionFormatAddress::replacePhrase(
	AddressBook* pAddressBook, AddressParser* pAddress)
{
	assert(pAddressBook);
	assert(pAddress);
	
	DECLARE_QSTATUS();
	
	AddressListParser* pGroup = pAddress->getGroup();
	if (pGroup) {
		status = replacePhrase(pAddressBook, pGroup);
		CHECK_QSTATUS();
	}
	else {
		if (!pAddress->getPhrase()) {
			string_ptr<WSTRING> wstrAddress(concat(
				pAddress->getMailbox(), L"@", pAddress->getHost()));
			if (!wstrAddress.get())
				return QSTATUS_OUTOFMEMORY;
			
			const AddressBookEntry* pEntry = 0;
			status = pAddressBook->getEntry(wstrAddress.get(), &pEntry);
			CHECK_QSTATUS();
			if (pEntry) {
				status = pAddress->setPhrase(pEntry->getName());
				CHECK_QSTATUS();
			}
		}
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * MacroFunctionFormatDate
 *
 */

qm::MacroFunctionFormatDate::MacroFunctionFormatDate(QSTATUS* pstatus) :
	MacroFunction(pstatus)
{
}

qm::MacroFunctionFormatDate::~MacroFunctionFormatDate()
{
}

QSTATUS qm::MacroFunctionFormatDate::value(
	MacroContext* pContext, MacroValue** ppValue) const
{
	assert(pContext);
	assert(ppValue);
	
	DECLARE_QSTATUS();
	
	*ppValue = 0;
	
	size_t nSize = getArgSize();
	if (nSize != 2 && nSize != 3)
		return error(*pContext, MacroErrorHandler::CODE_INVALIDARGSIZE);
	
	Time::Format format = Time::FORMAT_LOCAL;
	if (nSize == 3) {
		MacroValuePtr pValue;
		status = getArg(2)->value(pContext, &pValue);
		CHECK_QSTATUS();
		
		unsigned int n = pValue->number();
		if (n > 2)
			return error(*pContext, MacroErrorHandler::CODE_INVALIDARGVALUE);
		
		format = static_cast<Time::Format>(n);
	}
	
	MacroValuePtr pValueDate;
	status = getArg(0)->value(pContext, &pValueDate);
	CHECK_QSTATUS();
	if (pValueDate->getType() != MacroValue::TYPE_TIME)
		return error(*pContext, MacroErrorHandler::CODE_INVALIDARGTYPE);
	MacroValueTime* pTime = static_cast<MacroValueTime*>(pValueDate.get());
	
	MacroValuePtr pValueFormat;
	status = getArg(1)->value(pContext, &pValueFormat);
	CHECK_QSTATUS();
	string_ptr<WSTRING> wstrFormat;
	status = pValueFormat->string(&wstrFormat);
	CHECK_QSTATUS();
	
	string_ptr<WSTRING> wstrValue;
	status = pTime->getTime().format(wstrFormat.get(), format, &wstrValue);
	CHECK_QSTATUS();
	
	return MacroValueFactory::getFactory().newString(wstrValue.get(),
		reinterpret_cast<MacroValueString**>(ppValue));
}

const WCHAR* qm::MacroFunctionFormatDate::getName() const
{
	return L"FormatDate";
}


/****************************************************************************
 *
 * MacroFunctionFunction
 *
 */

qm::MacroFunctionFunction::MacroFunctionFunction(
	const WCHAR* pwszName, QSTATUS* pstatus) :
	MacroFunction(pstatus),
	wstrName_(0)
{
	if (*pstatus != QSTATUS_SUCCESS)
		return;
	
	wstrName_ = allocWString(pwszName);
	if (!wstrName_) {
		*pstatus = QSTATUS_OUTOFMEMORY;
		return;
	}
}

qm::MacroFunctionFunction::~MacroFunctionFunction()
{
	freeWString(wstrName_);
}

QSTATUS qm::MacroFunctionFunction::value(
	MacroContext* pContext, MacroValue** ppValue) const
{
	assert(pContext);
	assert(ppValue);
	
	DECLARE_QSTATUS();
	
	*ppValue = 0;
	
	const MacroExpr* pExpr = 0;
	status = pContext->getFunction(wstrName_, &pExpr);
	CHECK_QSTATUS();
	if (!pExpr)
		return error(*pContext, MacroErrorHandler::CODE_UNKNOWNFUNCTION);
	
	struct Args
	{
		Args(MacroContext* pContext) : pContext_(pContext) {}
		~Args() { pContext_->popArgumentContext(); }
		MacroContext* pContext_;
	} args(pContext);
	
	status = pContext->pushArgumentContext();
	CHECK_QSTATUS();
	
	MacroValuePtr pValueName;
	status = MacroValueFactory::getFactory().newString(
		wstrName_, reinterpret_cast<MacroValueString**>(&pValueName));
	CHECK_QSTATUS();
	status = pContext->addArgument(pValueName.get());
	CHECK_QSTATUS();
	pValueName.release();
	
	for (size_t n = 0; n < getArgSize(); ++n) {
		MacroValuePtr pValue;
		status = getArg(n)->value(pContext, &pValue);
		CHECK_QSTATUS();
		status = pContext->addArgument(pValue.get());
		CHECK_QSTATUS();
		pValue.release();
	}
	
	return pExpr->value(pContext, ppValue);
}

void qm::MacroFunctionFunction::release()
{
	delete this;
}

const WCHAR* qm::MacroFunctionFunction::getName() const
{
	return L"Function";
}


/****************************************************************************
 *
 * MacroFunctionHeader
 *
 */

qm::MacroFunctionHeader::MacroFunctionHeader(QSTATUS* pstatus) :
	MacroFunction(pstatus)
{
}

qm::MacroFunctionHeader::~MacroFunctionHeader()
{
}

QSTATUS qm::MacroFunctionHeader::value(
	MacroContext* pContext, MacroValue** ppValue) const
{
	assert(pContext);
	assert(ppValue);
	
	DECLARE_QSTATUS();
	
	*ppValue = 0;
	
	size_t nSize = getArgSize();
	if (nSize > 2)
		return error(*pContext, MacroErrorHandler::CODE_INVALIDARGSIZE);
	
	MessageHolderBase* pmh = pContext->getMessageHolder();
	if (!pmh)
		return error(*pContext, MacroErrorHandler::CODE_NOCONTEXTMESSAGE);
	
	Message* pMessage = 0;
	status = pContext->getMessage(MacroContext::MESSAGETYPE_HEADER, 0, &pMessage);
	CHECK_QSTATUS();
	
	const Part* pPart = pMessage;
	if (nSize > 1) {
		status = getPart(pContext, 1, &pPart);
		CHECK_QSTATUS();
	}
	
	string_ptr<WSTRING> wstrRemoveField;
	if (nSize > 0) {
		MacroValuePtr pValue;
		status = getArg(0)->value(pContext, &pValue);
		CHECK_QSTATUS();
		status = pValue->string(&wstrRemoveField);
		CHECK_QSTATUS();
	}
	
	const CHAR* pszHeader = pPart->getHeader();
	Part partTemp(&status);
	if (wstrRemoveField.get()) {
		status = partTemp.create(0, pszHeader, static_cast<size_t>(-1));
		CHECK_QSTATUS();
		
		const WCHAR* p = wstrRemoveField.get();
		const WCHAR* pEnd = wcschr(p, L',');
		do {
			string_ptr<WSTRING> wstrField(trim(
				p, pEnd ? pEnd - p : static_cast<size_t>(-1)));
			if (!wstrField.get())
				return QSTATUS_OUTOFMEMORY;
			
			partTemp.removeField(wstrField.get(), 0xffffffff);
			
			if (pEnd) {
				p = pEnd + 1;
				pEnd = wcschr(p, L',');
			}
		} while (pEnd);
		
		pszHeader = partTemp.getHeader();
	}
	
	string_ptr<WSTRING> wstrHeader;
	status = PartUtil::a2w(pszHeader, &wstrHeader);
	CHECK_QSTATUS();
	
	return MacroValueFactory::getFactory().newString(wstrHeader.get(),
		reinterpret_cast<MacroValueString**>(ppValue));
}

const WCHAR* qm::MacroFunctionHeader::getName() const
{
	return L"Header";
}


/****************************************************************************
 *
 * MacroFunctionI
 *
 */

qm::MacroFunctionI::MacroFunctionI(QSTATUS* pstatus) :
	MacroFunction(pstatus)
{
}

qm::MacroFunctionI::~MacroFunctionI()
{
}

QSTATUS qm::MacroFunctionI::value(
	MacroContext* pContext, MacroValue** ppValue) const
{
	assert(pContext);
	assert(ppValue);
	
	DECLARE_QSTATUS();
	
	*ppValue = 0;
	
	size_t nSize = getArgSize();
	if (nSize > 2)
		return error(*pContext, MacroErrorHandler::CODE_INVALIDARGSIZE);
	
	string_ptr<WSTRING> wstrSubAccount;
	if (nSize > 1) {
		MacroValuePtr pValue;
		status = getArg(1)->value(pContext, &pValue);
		CHECK_QSTATUS();
		status = pValue->string(&wstrSubAccount);
		CHECK_QSTATUS();
	}
	
	Account* pAccount = pContext->getAccount();
	if (nSize > 0) {
		MacroValuePtr pValue;
		status = getArg(0)->value(pContext, &pValue);
		CHECK_QSTATUS();
		string_ptr<WSTRING> wstrAccount;
		status = pValue->string(&wstrAccount);
		CHECK_QSTATUS();
		pAccount = pContext->getDocument()->getAccount(wstrAccount.get());
		if (!pAccount)
			return error(*pContext, MacroErrorHandler::CODE_UNKNOWNACCOUNT);
	}
	assert(pAccount);
	
	SubAccount* pSubAccount = 0;
	if (wstrSubAccount.get())
		pSubAccount = pAccount->getSubAccount(wstrSubAccount.get());
	if (!pSubAccount)
		pSubAccount = pAccount->getCurrentSubAccount();
	
	AddressParser address(pSubAccount->getSenderName(),
		pSubAccount->getSenderAddress(), &status);
	CHECK_QSTATUS();
	Part part(&status);
	CHECK_QSTATUS();
	status = part.setField(L"I", address);
	CHECK_QSTATUS();
	
	string_ptr<STRING> strHeader;
	status = PartUtil(part).getHeader(L"I", &strHeader);
	CHECK_QSTATUS();
	
	return MacroValueFactory::getFactory().newField(L"I",
		strHeader.get(), reinterpret_cast<MacroValueField**>(ppValue));
}

const WCHAR* qm::MacroFunctionI::getName() const
{
	return L"I";
}


/****************************************************************************
 *
 * MacroFunctionId
 *
 */

qm::MacroFunctionId::MacroFunctionId(QSTATUS* pstatus) :
	MacroFunction(pstatus)
{
}

qm::MacroFunctionId::~MacroFunctionId()
{
}

QSTATUS qm::MacroFunctionId::value(
	MacroContext* pContext, MacroValue** ppValue) const
{
	assert(pContext);
	assert(ppValue);
	
	DECLARE_QSTATUS();
	
	*ppValue = 0;
	
	if (getArgSize() != 0)
		return error(*pContext, MacroErrorHandler::CODE_INVALIDARGSIZE);
	
	MessageHolderBase* pmh = pContext->getMessageHolder();
	if (!pmh)
		return error(*pContext, MacroErrorHandler::CODE_NOCONTEXTMESSAGE);
	
	return MacroValueFactory::getFactory().newNumber(pmh->getId(),
		reinterpret_cast<MacroValueNumber**>(ppValue));
}

const WCHAR* qm::MacroFunctionId::getName() const
{
	return L"Id";
}


/****************************************************************************
 *
 * MacroFunctionIdentity
 *
 */

qm::MacroFunctionIdentity::MacroFunctionIdentity(QSTATUS* pstatus) :
	MacroFunction(pstatus)
{
}

qm::MacroFunctionIdentity::~MacroFunctionIdentity()
{
}

QSTATUS qm::MacroFunctionIdentity::value(
	MacroContext* pContext, MacroValue** ppValue) const
{
	assert(pContext);
	assert(ppValue);
	
	DECLARE_QSTATUS();
	
	*ppValue = 0;
	
	if (getArgSize() != 0)
		return error(*pContext, MacroErrorHandler::CODE_INVALIDARGSIZE);
	
	Account* pAccount = pContext->getAccount();
	MessageHolderBase* pmh = pContext->getMessageHolder();
	if (pmh)
		pAccount = pmh->getFolder()->getAccount();
	const WCHAR* pwszIdentity = pAccount->getCurrentSubAccount()->getIdentity();
	if (!pwszIdentity)
		pwszIdentity = L"";
	
	return MacroValueFactory::getFactory().newString(pwszIdentity,
		reinterpret_cast<MacroValueString**>(ppValue));
}

const WCHAR* qm::MacroFunctionIdentity::getName() const
{
	return L"Identity";
}


/****************************************************************************
 *
 * MacroFunctionIf
 *
 */

qm::MacroFunctionIf::MacroFunctionIf(QSTATUS* pstatus) :
	MacroFunction(pstatus)
{
}

qm::MacroFunctionIf::~MacroFunctionIf()
{
}

QSTATUS qm::MacroFunctionIf::value(
	MacroContext* pContext, MacroValue** ppValue) const
{
	assert(pContext);
	assert(ppValue);
	
	DECLARE_QSTATUS();
	
	*ppValue = 0;
	
	size_t nSize = getArgSize();
	if (nSize < 3 || nSize % 2 == 0)
		return error(*pContext, MacroErrorHandler::CODE_INVALIDARGSIZE);
	
	for (size_t n = 0; n < nSize - 1; n += 2) {
		MacroValuePtr pValue;
		status = getArg(n)->value(pContext, &pValue);
		CHECK_QSTATUS();
		if (pValue->boolean())
			return getArg(n + 1)->value(pContext, ppValue);
	}
	return getArg(n)->value(pContext, ppValue);
}

const WCHAR* qm::MacroFunctionIf::getName() const
{
	return L"If";
}


/****************************************************************************
 *
 * MacroFunctionInclude
 *
 */

qm::MacroFunctionInclude::MacroFunctionInclude(
	MacroParser::Type type, QSTATUS* pstatus) :
	MacroFunction(pstatus),
	type_(type)
{
}

qm::MacroFunctionInclude::~MacroFunctionInclude()
{
}

QSTATUS qm::MacroFunctionInclude::value(
	MacroContext* pContext, MacroValue** ppValue) const
{
	assert(pContext);
	assert(ppValue);
	
	DECLARE_QSTATUS();
	
	*ppValue = 0;
	
	if (getArgSize() != 1)
		return error(*pContext, MacroErrorHandler::CODE_INVALIDARGSIZE);
	
	string_ptr<WSTRING> wstrPath;
	MacroValuePtr pValuePath;
	status = getArg(0)->value(pContext, &pValuePath);
	CHECK_QSTATUS();
	status = pValuePath->string(&wstrPath);
	CHECK_QSTATUS();
	
	string_ptr<WSTRING> wstrAbsolutePath;
	status = pContext->resolvePath(wstrPath.get(), &wstrAbsolutePath);
	FileInputStream stream(wstrAbsolutePath.get(), &status);
	CHECK_QSTATUS();
	InputStreamReader reader(&stream, false, 0, &status);
	CHECK_QSTATUS();
	BufferedReader bufferedReader(&reader, false, &status);
	CHECK_QSTATUS();
	
	StringBuffer<WSTRING> buf(&status);
	WCHAR wsz[1024];
	while (true) {
		size_t nRead = 0;
		status = bufferedReader.read(wsz, countof(wsz), &nRead);
		CHECK_QSTATUS();
		if (nRead == -1)
			break;
		status = buf.append(wsz, nRead);
		CHECK_QSTATUS();
	}
	
	MacroParser parser(type_, &status);
	CHECK_QSTATUS();
	Macro* p = 0;
	status = parser.parse(buf.getCharArray(), &p);
	CHECK_QSTATUS();
	std::auto_ptr<Macro> pMacro(p);
	return pMacro->value(pContext, ppValue);
}

const WCHAR* qm::MacroFunctionInclude::getName() const
{
	return L"Include";
}


/****************************************************************************
 *
 * MacroFunctionInputBox
 *
 */

qm::MacroFunctionInputBox::MacroFunctionInputBox(QSTATUS* pstatus) :
	MacroFunction(pstatus)
{
}

qm::MacroFunctionInputBox::~MacroFunctionInputBox()
{
}

QSTATUS qm::MacroFunctionInputBox::value(
	MacroContext* pContext, MacroValue** ppValue) const
{
	assert(pContext);
	assert(ppValue);
	
	DECLARE_QSTATUS();
	
	*ppValue = 0;
	
	size_t nSize = getArgSize();
	if (nSize < 1 || 3 < nSize)
		return error(*pContext, MacroErrorHandler::CODE_INVALIDARGSIZE);
	
	string_ptr<WSTRING> wstrDefault;
	if (nSize > 2) {
		MacroValuePtr pValue;
		status = getArg(2)->value(pContext, &pValue);
		CHECK_QSTATUS();
		status = pValue->string(&wstrDefault);
		CHECK_QSTATUS();
	}
	
	bool bMultiline = false;
	if (nSize > 1) {
		MacroValuePtr pValue;
		status = getArg(1)->value(pContext, &pValue);
		CHECK_QSTATUS();
		bMultiline = pValue->boolean();
	}
	
	string_ptr<WSTRING> wstrMessage;
	MacroValuePtr pValue;
	status = getArg(0)->value(pContext, &pValue);
	CHECK_QSTATUS();
	status = pValue->string(&wstrMessage);
	CHECK_QSTATUS();
	
	InputBoxDialog dialog(bMultiline, wstrMessage.get(),
		wstrDefault.get(), &status);
	CHECK_QSTATUS();
	int nRet = 0;
	status = dialog.doModal(pContext->getWindow(), 0, &nRet);
	CHECK_QSTATUS();
	if (nRet != IDOK) {
		// TODO
		return QSTATUS_FAIL;
	}
	
	return MacroValueFactory::getFactory().newString(dialog.getValue(),
		reinterpret_cast<MacroValueString**>(ppValue));
}

const WCHAR* qm::MacroFunctionInputBox::getName() const
{
	return L"InputBox";
}


/****************************************************************************
 *
 * MacroFunctionLength
 *
 */

qm::MacroFunctionLength::MacroFunctionLength(QSTATUS* pstatus) :
	MacroFunction(pstatus)
{
}

qm::MacroFunctionLength::~MacroFunctionLength()
{
}

QSTATUS qm::MacroFunctionLength::value(
	MacroContext* pContext, MacroValue** ppValue) const
{
	assert(pContext);
	assert(ppValue);
	
	DECLARE_QSTATUS();
	
	*ppValue = 0;
	
	size_t nSize = getArgSize();
	if (nSize != 1 && nSize != 2)
		return error(*pContext, MacroErrorHandler::CODE_INVALIDARGSIZE);
	
	bool bByte = false;
	if (nSize > 1) {
		MacroValuePtr pValue;
		status = getArg(1)->value(pContext, &pValue);
		CHECK_QSTATUS();
		bByte = pValue->boolean();
	}
	MacroValuePtr pValue;
	status = getArg(0)->value(pContext, &pValue);
	CHECK_QSTATUS();
	
	string_ptr<WSTRING> wstrValue;
	status = pValue->string(&wstrValue);
	CHECK_QSTATUS();
	
	long nLen = 0;
	if (bByte) {
		string_ptr<STRING> str(wcs2mbs(wstrValue.get()));
		if (!str.get())
			return QSTATUS_OUTOFMEMORY;
		nLen = strlen(str.get());
	}
	else {
		nLen = wcslen(wstrValue.get());
	}
	
	return MacroValueFactory::getFactory().newNumber(
		nLen, reinterpret_cast<MacroValueNumber**>(ppValue));
}

const WCHAR* qm::MacroFunctionLength::getName() const
{
	return L"Length";
}


/****************************************************************************
 *
 * MacroFunctionLoad
 *
 */

qm::MacroFunctionLoad::MacroFunctionLoad(QSTATUS* pstatus) :
	MacroFunction(pstatus)
{
}

qm::MacroFunctionLoad::~MacroFunctionLoad()
{
}

QSTATUS qm::MacroFunctionLoad::value(
	MacroContext* pContext, MacroValue** ppValue) const
{
	assert(pContext);
	assert(ppValue);
	
	DECLARE_QSTATUS();
	
	*ppValue = 0;
	
	size_t nSize = getArgSize();
	if (nSize != 1 && nSize != 2 && nSize != 3)
		return error(*pContext, MacroErrorHandler::CODE_INVALIDARGSIZE);
	
	string_ptr<WSTRING> wstrPath;
	MacroValuePtr pValuePath;
	status = getArg(0)->value(pContext, &pValuePath);
	CHECK_QSTATUS();
	status = pValuePath->string(&wstrPath);
	CHECK_QSTATUS();
	
	bool bTemplate = false;
	if (nSize > 1) {
		MacroValuePtr pValueTemplate;
		status = getArg(1)->value(pContext, &pValueTemplate);
		CHECK_QSTATUS();
		bTemplate = pValueTemplate->boolean();
	}
	
	string_ptr<WSTRING> wstrEncoding;
	if (nSize > 2) {
		MacroValuePtr pValueEncoding;
		status = getArg(2)->value(pContext, &pValueEncoding);
		CHECK_QSTATUS();
		status = pValueEncoding->string(&wstrEncoding);
		CHECK_QSTATUS();
	}
	
	string_ptr<WSTRING> wstrAbsolutePath;
	status = pContext->resolvePath(wstrPath.get(), &wstrAbsolutePath);
	FileInputStream stream(wstrAbsolutePath.get(), &status);
	CHECK_QSTATUS();
	InputStreamReader reader(&stream, false, wstrEncoding.get(), &status);
	CHECK_QSTATUS();
	BufferedReader bufferedReader(&reader, false, &status);
	CHECK_QSTATUS();
	
	string_ptr<WSTRING> wstr;
	if (bTemplate) {
		TemplateParser parser(&status);
		CHECK_QSTATUS();
		Template* p = 0;
		status = parser.parse(&bufferedReader, &p);
		CHECK_QSTATUS();
		std::auto_ptr<Template> pTemplate(p);
		
		TemplateContext context(pContext->getMessageHolder(),
			pContext->getMessage(), pContext->getAccount(),
			pContext->getDocument(), pContext->getWindow(),
			pContext->getProfile(), pContext->getErrorHandler(),
			TemplateContext::ArgumentList(), &status);
		CHECK_QSTATUS();
		status = pTemplate->getValue(context, &wstr);
		CHECK_QSTATUS();
	}
	else {
		StringBuffer<WSTRING> buf(&status);
		CHECK_QSTATUS();
		WCHAR wsz[1024];
		while (true) {
			size_t nRead = 0;
			status = bufferedReader.read(wsz, countof(wsz), &nRead);
			CHECK_QSTATUS();
			if (nRead == -1)
				break;
			status = buf.append(wsz, nRead);
			CHECK_QSTATUS();
		}
		wstr.reset(buf.getString());
	}
	
	return MacroValueFactory::getFactory().newString(wstr.get(),
		reinterpret_cast<MacroValueString**>(ppValue));
}

const WCHAR* qm::MacroFunctionLoad::getName() const
{
	return L"Load";
}


/****************************************************************************
 *
 * MacroFunctionMessageBox
 *
 */

qm::MacroFunctionMessageBox::MacroFunctionMessageBox(QSTATUS* pstatus) :
	MacroFunction(pstatus)
{
}

qm::MacroFunctionMessageBox::~MacroFunctionMessageBox()
{
}

QSTATUS qm::MacroFunctionMessageBox::value(
	MacroContext* pContext, MacroValue** ppValue) const
{
	assert(pContext);
	assert(ppValue);
	
	DECLARE_QSTATUS();
	
	*ppValue = 0;
	
	size_t nSize = getArgSize();
	if (nSize != 1 && nSize != 2)
		return error(*pContext, MacroErrorHandler::CODE_INVALIDARGSIZE);
	
	int nType = MB_OK | MB_ICONINFORMATION;
	if (nSize > 1) {
		MacroValuePtr pValue;
		status = getArg(1)->value(pContext, &pValue);
		CHECK_QSTATUS();
		nType = pValue->number();
	}
	
	MacroValuePtr pValue;
	status = getArg(0)->value(pContext, &pValue);
	CHECK_QSTATUS();
	
	string_ptr<WSTRING> wstrMessage;
	status = pValue->string(&wstrMessage);
	CHECK_QSTATUS();
	
	HWND hwnd = Window::getActiveWindow();
	if (!hwnd)
		hwnd = pContext->getWindow();
	
	int nValue = 0;
	status = messageBox(wstrMessage.get(), nType, hwnd, 0, 0, &nValue);
	CHECK_QSTATUS();
	
	return MacroValueFactory::getFactory().newNumber(
		nValue, reinterpret_cast<MacroValueNumber**>(ppValue));
}

const WCHAR* qm::MacroFunctionMessageBox::getName() const
{
	return L"MessageBox";
}


/****************************************************************************
 *
 * MacroFunctionMessages
 *
 */

qm::MacroFunctionMessages::MacroFunctionMessages(QSTATUS* pstatus) :
	MacroFunction(pstatus)
{
}

qm::MacroFunctionMessages::~MacroFunctionMessages()
{
}

QSTATUS qm::MacroFunctionMessages::value(
	MacroContext* pContext, MacroValue** ppValue) const
{
	assert(pContext);
	assert(ppValue);
	
	DECLARE_QSTATUS();
	
	*ppValue = 0;
	
	size_t nSize = getArgSize();
	if (nSize > 2)
		return error(*pContext, MacroErrorHandler::CODE_INVALIDARGSIZE);
	
	unsigned int nId = 0;
	if (nSize > 1) {
		MacroValuePtr pValueId;
		status = getArg(1)->value(pContext, &pValueId);
		CHECK_QSTATUS();
		nId = pValueId->number();
	}
	
	string_ptr<WSTRING> wstrFolder;
	if (nSize > 0) {
		MacroValuePtr pValueFolder;
		status = getArg(0)->value(pContext, &pValueFolder);
		CHECK_QSTATUS();
		status = pValueFolder->string(&wstrFolder);
		CHECK_QSTATUS();
	}
	
	typedef MacroValueMessageList::MessageList List;
	List l;
	if (wstrFolder.get()) {
		Folder* pFolder = 0;
		status = pContext->getDocument()->getFolder(
			pContext->getAccount(), wstrFolder.get(), &pFolder);
		CHECK_QSTATUS();
		if (pFolder->getType() != Folder::TYPE_NORMAL)
			return QSTATUS_FAIL;
		Lock<Folder> lock(*pFolder);
		if (nSize > 1) {
			MessageHolder* pmh = 0;
			status = static_cast<NormalFolder*>(pFolder)->getMessageById(nId, &pmh);
			CHECK_QSTATUS();
			if (pmh) {
				status = STLWrapper<List>(l).push_back(MessagePtr(pmh));
				CHECK_QSTATUS();
			}
		}
		else {
			status = STLWrapper<List>(l).resize(pFolder->getCount());
			CHECK_QSTATUS();
			for (unsigned int n = 0; n < pFolder->getCount(); ++n)
				l[n] = MessagePtr(pFolder->getMessage(n));
		}
	}
	else {
		// TODO
	}
	
	return MacroValueFactory::getFactory().newMessageList(l,
		reinterpret_cast<MacroValueMessageList**>(ppValue));
}

const WCHAR* qm::MacroFunctionMessages::getName() const
{
	return L"Messages";
}


/****************************************************************************
 *
 * MacroFunctionNot
 *
 */

qm::MacroFunctionNot::MacroFunctionNot(QSTATUS* pstatus) :
	MacroFunction(pstatus)
{
}

qm::MacroFunctionNot::~MacroFunctionNot()
{
}

QSTATUS qm::MacroFunctionNot::value(
	MacroContext* pContext, MacroValue** ppValue) const
{
	assert(pContext);
	assert(ppValue);
	
	DECLARE_QSTATUS();
	
	*ppValue = 0;
	
	if (getArgSize() != 1)
		return error(*pContext, MacroErrorHandler::CODE_INVALIDARGSIZE);
	
	MacroValuePtr pValue;
	status = getArg(0)->value(pContext, &pValue);
	CHECK_QSTATUS();
	
	return MacroValueFactory::getFactory().newBoolean(
		!pValue->boolean(), reinterpret_cast<MacroValueBoolean**>(ppValue));
}

const WCHAR* qm::MacroFunctionNot::getName() const
{
	return L"Not";
}


/****************************************************************************
 *
 * MacroFunctionOr
 *
 */

qm::MacroFunctionOr::MacroFunctionOr(QSTATUS* pstatus) :
	MacroFunction(pstatus)
{
}

qm::MacroFunctionOr::~MacroFunctionOr()
{
}

QSTATUS qm::MacroFunctionOr::value(
	MacroContext* pContext, MacroValue** ppValue) const
{
	assert(pContext);
	assert(ppValue);
	
	DECLARE_QSTATUS();
	
	*ppValue = 0;
	
	size_t nSize = getArgSize();
	if (nSize == 0)
		return error(*pContext, MacroErrorHandler::CODE_INVALIDARGSIZE);
	
	bool bValue = false;
	for (size_t n = 0; n < nSize && !bValue; ++n) {
		MacroValuePtr pValue;
		status = getArg(n)->value(pContext, &pValue);
		CHECK_QSTATUS();
		if (pValue->boolean())
			bValue = true;
	}
	return MacroValueFactory::getFactory().newBoolean(
		bValue, reinterpret_cast<MacroValueBoolean**>(ppValue));
}

const WCHAR* qm::MacroFunctionOr::getName() const
{
	return L"Or";
}


/****************************************************************************
 *
 * MacroFunctionOSVersion
 *
 */

qm::MacroFunctionOSVersion::MacroFunctionOSVersion(QSTATUS* pstatus) :
	MacroFunction(pstatus)
{
}

qm::MacroFunctionOSVersion::~MacroFunctionOSVersion()
{
}

QSTATUS qm::MacroFunctionOSVersion::value(
	MacroContext* pContext, MacroValue** ppValue) const
{
	assert(pContext);
	assert(ppValue);
	
	DECLARE_QSTATUS();
	
	*ppValue = 0;
	
	if (getArgSize() != 0)
		return error(*pContext, MacroErrorHandler::CODE_INVALIDARGSIZE);
	
	string_ptr<WSTRING> wstrOSVersion;
	status = Application::getApplication().getOSVersion(&wstrOSVersion);
	CHECK_QSTATUS();
	
	return MacroValueFactory::getFactory().newString(wstrOSVersion.get(),
		reinterpret_cast<MacroValueString**>(ppValue));
}

const WCHAR* qm::MacroFunctionOSVersion::getName() const
{
	return L"OSVersion";
}


/****************************************************************************
 *
 * MacroFunctionParseURL
 *
 */

qm::MacroFunctionParseURL::MacroFunctionParseURL(QSTATUS* pstatus) :
	MacroFunction(pstatus)
{
}

qm::MacroFunctionParseURL::~MacroFunctionParseURL()
{
}

QSTATUS qm::MacroFunctionParseURL::value(
	MacroContext* pContext, MacroValue** ppValue) const
{
	assert(pContext);
	assert(ppValue);
	
	DECLARE_QSTATUS();
	
	*ppValue = 0;
	
	if (getArgSize() != 1)
		return error(*pContext, MacroErrorHandler::CODE_INVALIDARGSIZE);
	
	MacroValuePtr pValue;
	status = getArg(0)->value(pContext, &pValue);
	CHECK_QSTATUS();
	string_ptr<WSTRING> wstrURL;
	status = pValue->string(&wstrURL);
	CHECK_QSTATUS();
	
	StringBuffer<WSTRING> buf(&status);
	CHECK_QSTATUS();
	
	if (wcslen(wstrURL.get()) >= 7 &&
		wcsncmp(wstrURL.get(), L"mailto:", 7) == 0) {
		const WCHAR* p = wstrURL.get() + 7;
		const WCHAR* pAddress = p;
		while (*p && *p != L'?')
			++p;
		if (p != pAddress) {
			string_ptr<WSTRING> wstrTo;
			status = decode(pAddress, p - pAddress, &wstrTo);
			CHECK_QSTATUS();
			status = buf.append(L"To: ");
			CHECK_QSTATUS();
			status = buf.append(wstrTo.get());
			CHECK_QSTATUS();
			status = buf.append(L"\n");
			CHECK_QSTATUS();
		}
		
		string_ptr<WSTRING> wstrBody;
		if (*p) {
			assert(*p == L'?');
			
			const WCHAR* pwszFields[] = {
				L"to",
				L"cc",
				L"bcc",
				L"subject"
			};
			
			const WCHAR* pName = p + 1;
			const WCHAR* pValue = 0;
			string_ptr<WSTRING> wstrName;
			do {
				++p;
				if (!wstrName.get() && *p == L'=') {
					status = decode(pName, p - pName, &wstrName);
					CHECK_QSTATUS();
					pValue = p + 1;
				}
				else if (*p == L'&' || *p == L'\0') {
					if (wstrName.get()) {
						for (int n = 0; n < countof(pwszFields); ++n) {
							if (_wcsicmp(wstrName.get(), pwszFields[n]) == 0)
								break;
						}
						if (n != countof(pwszFields)) {
							string_ptr<WSTRING> wstrValue;
							status = decode(pValue, p - pValue, &wstrValue);
							CHECK_QSTATUS();
							status = buf.append(wstrName.get());
							CHECK_QSTATUS();
							status = buf.append(L": ");
							CHECK_QSTATUS();
							status = buf.append(wstrValue.get());
							CHECK_QSTATUS();
							status = buf.append(L"\n");
							CHECK_QSTATUS();
						}
						else if (_wcsicmp(wstrName.get(), L"body") == 0) {
							status = decode(pValue, p - pValue, &wstrBody);
							CHECK_QSTATUS();
						}
					}
					pName = p + 1;
					pValue = 0;
					wstrName.reset(0);
				}
			} while(*p);
		}
		
		status = buf.append(L"\n");
		CHECK_QSTATUS();
		if (wstrBody.get()) {
			status = buf.append(wstrBody.get());
			CHECK_QSTATUS();
		}
	}
	else {
		status = buf.append(L"To: ");
		CHECK_QSTATUS();
		status = buf.append(wstrURL.get());
		CHECK_QSTATUS();
		status = buf.append(L"\n\n");
		CHECK_QSTATUS();
	}
	
	return MacroValueFactory::getFactory().newString(buf.getCharArray(),
		reinterpret_cast<MacroValueString**>(ppValue));
}

const WCHAR* qm::MacroFunctionParseURL::getName() const
{
	return L"ParseURL";
}

QSTATUS qm::MacroFunctionParseURL::decode(
	const WCHAR* p, size_t nLen, WSTRING* pwstr)
{
	assert(p);
	assert(pwstr);
	
	DECLARE_QSTATUS();
	
	UTF8Converter converter(&status);
	CHECK_QSTATUS();
	
	StringBuffer<STRING> buf(&status);
	CHECK_QSTATUS();
	for (; nLen > 0; --nLen, ++p) {
		if (*p == L'%' && nLen > 2 && isHex(*(p + 1)) && isHex(*(p + 2))) {
			WCHAR wsz[3] = { *(p + 1), *(p + 2), L'\0' };
			WCHAR* pEnd = 0;
			long n = wcstol(wsz, &pEnd, 16);
			if (n > 0 && n != 0x0d) {
				status = buf.append(static_cast<CHAR>(n));
				CHECK_QSTATUS();
			}
			p += 2;
			nLen -= 2;
		}
		else if (*p <= 0x7f) {
			status = buf.append(static_cast<CHAR>(*p));
			CHECK_QSTATUS();
		}
		else {
			size_t nLen = 1;
			string_ptr<STRING> str;
			status = converter.encode(p, &nLen, &str, 0);
			CHECK_QSTATUS();
			status = buf.append(str.get());
			CHECK_QSTATUS();
		}
	}
	
	size_t n = buf.getLength();
	status = converter.decode(buf.getCharArray(), &n, pwstr, 0);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

bool qm::MacroFunctionParseURL::isHex(WCHAR c)
{
	return (L'0' <= c && c <= L'9') ||
		(L'a' <= c && c <= 'f') ||
		(L'A' <= c && c <= 'F');
}


/****************************************************************************
 *
 * MacroFunctionPart
 *
 */

qm::MacroFunctionPart::MacroFunctionPart(QSTATUS* pstatus) :
	MacroFunction(pstatus)
{
}

qm::MacroFunctionPart::~MacroFunctionPart()
{
}

QSTATUS qm::MacroFunctionPart::value(
	MacroContext* pContext, MacroValue** ppValue) const
{
	assert(pContext);
	assert(ppValue);
	
	DECLARE_QSTATUS();
	
	*ppValue = 0;
	
	size_t nSize = getArgSize();
	if (nSize != 1 && nSize != 2)
		return error(*pContext, MacroErrorHandler::CODE_INVALIDARGSIZE);
	
	MessageHolderBase* pmh = pContext->getMessageHolder();
	if (!pmh)
		return error(*pContext, MacroErrorHandler::CODE_NOCONTEXTMESSAGE);
	
	Message* pMessage = 0;
	status = pContext->getMessage(MacroContext::MESSAGETYPE_ALL, 0, &pMessage);
	CHECK_QSTATUS();
	
	const Part* pPart = pMessage;
	if (nSize > 1) {
		status = getPart(pContext, 1, &pPart);
		CHECK_QSTATUS();
	}
	
	MacroValuePtr pValue;
	status = getArg(0)->value(pContext, &pValue);
	CHECK_QSTATUS();
	
	unsigned int nPart = pValue->number();
	if (nPart < pPart->getPartCount())
		pPart = pPart->getPart(nPart);
	else
		pPart = 0;
	
	return MacroValueFactory::getFactory().newPart(pPart,
		reinterpret_cast<MacroValuePart**>(ppValue));
}

const WCHAR* qm::MacroFunctionPart::getName() const
{
	return L"Part";
}


/****************************************************************************
 *
 * MacroFunctionPassed
 *
 */

qm::MacroFunctionPassed::MacroFunctionPassed(QSTATUS* pstatus) :
	MacroFunction(pstatus)
{
}

qm::MacroFunctionPassed::~MacroFunctionPassed()
{
}

QSTATUS qm::MacroFunctionPassed::value(
	MacroContext* pContext, MacroValue** ppValue) const
{
	assert(pContext);
	assert(ppValue);
	
	DECLARE_QSTATUS();
	
	*ppValue = 0;
	
	if (getArgSize() != 1)
		return error(*pContext, MacroErrorHandler::CODE_INVALIDARGSIZE);
	
	MessageHolderBase* pmh = pContext->getMessageHolder();
	if (!pmh)
		return error(*pContext, MacroErrorHandler::CODE_NOCONTEXTMESSAGE);
	
	MacroValuePtr pValue;
	status = getArg(0)->value(pContext, &pValue);
	CHECK_QSTATUS();
	
	unsigned int nDay = pValue->number();
	
	Time time;
	status = pmh->getDate(&time);
	CHECK_QSTATUS();
	time.addDay(nDay);
	CHECK_QSTATUS();
	
	Time timeNow(Time::getCurrentTime());
	
	return MacroValueFactory::getFactory().newBoolean(time < timeNow,
		reinterpret_cast<MacroValueBoolean**>(ppValue));
}

const WCHAR* qm::MacroFunctionPassed::getName() const
{
	return L"Passed";
}


/****************************************************************************
 *
 * MacroFunctionProcessId
 *
 */

qm::MacroFunctionProcessId::MacroFunctionProcessId(QSTATUS* pstatus) :
	MacroFunction(pstatus)
{
}

qm::MacroFunctionProcessId::~MacroFunctionProcessId()
{
}

QSTATUS qm::MacroFunctionProcessId::value(
	MacroContext* pContext, MacroValue** ppValue) const
{
	assert(pContext);
	assert(ppValue);
	
	DECLARE_QSTATUS();
	
	*ppValue = 0;
	
	if (getArgSize() != 0)
		return error(*pContext, MacroErrorHandler::CODE_INVALIDARGSIZE);
	
	return MacroValueFactory::getFactory().newNumber(::GetCurrentProcessId(),
		reinterpret_cast<MacroValueNumber**>(ppValue));
}

const WCHAR* qm::MacroFunctionProcessId::getName() const
{
	return L"ProcessId";
}


/****************************************************************************
 *
 * MacroFunctionProfile
 *
 */

qm::MacroFunctionProfile::MacroFunctionProfile(QSTATUS* pstatus) :
	MacroFunction(pstatus)
{
}

qm::MacroFunctionProfile::~MacroFunctionProfile()
{
}

QSTATUS qm::MacroFunctionProfile::value(
	MacroContext* pContext, MacroValue** ppValue) const
{
	assert(pContext);
	assert(ppValue);
	
	DECLARE_QSTATUS();
	
	*ppValue = 0;
	
	size_t nSize = getArgSize();
	if (nSize != 3 && nSize != 4)
		return error(*pContext, MacroErrorHandler::CODE_INVALIDARGSIZE);
	
	MacroValuePtr pValuePath;
	status = getArg(0)->value(pContext, &pValuePath);
	CHECK_QSTATUS();
	string_ptr<WSTRING> wstrPath;
	status = pValuePath->string(&wstrPath);
	CHECK_QSTATUS();
	
	MacroValuePtr pValueSection;
	status = getArg(1)->value(pContext, &pValueSection);
	CHECK_QSTATUS();
	string_ptr<WSTRING> wstrSection;
	status = pValueSection->string(&wstrSection);
	CHECK_QSTATUS();
	
	MacroValuePtr pValueKey;
	status = getArg(2)->value(pContext, &pValueKey);
	CHECK_QSTATUS();
	string_ptr<WSTRING> wstrKey;
	status = pValueKey->string(&wstrKey);
	CHECK_QSTATUS();
	
	string_ptr<WSTRING> wstrDefault;
	if (nSize > 3) {
		MacroValuePtr pValueDefault;
		status = getArg(3)->value(pContext, &pValueDefault);
		CHECK_QSTATUS();
		status = pValueDefault->string(&wstrDefault);
		CHECK_QSTATUS();
	}
	
	string_ptr<WSTRING> wstrValue;
	if (!*wstrPath.get()) {
		status = pContext->getProfile()->getString(wstrSection.get(),
			wstrKey.get(), wstrDefault.get(), &wstrValue);
		CHECK_QSTATUS();
	}
	else {
		string_ptr<WSTRING> wstrAbsolutePath;
		status = pContext->resolvePath(wstrPath.get(), &wstrAbsolutePath);
		CHECK_QSTATUS();
		XMLProfile profile(wstrAbsolutePath.get(), &status);
		CHECK_QSTATUS();
		status = profile.load();
		CHECK_QSTATUS();
		status = profile.getString(wstrSection.get(),
			wstrKey.get(), wstrDefault.get(), &wstrValue);
		CHECK_QSTATUS();
	}
	
	return MacroValueFactory::getFactory().newString(wstrValue.get(),
		reinterpret_cast<MacroValueString**>(ppValue));
}

const WCHAR* qm::MacroFunctionProfile::getName() const
{
	return L"Profile";
}


/****************************************************************************
 *
 * MacroFunctionProfileName
 *
 */

qm::MacroFunctionProfileName::MacroFunctionProfileName(QSTATUS* pstatus) :
	MacroFunction(pstatus)
{
}

qm::MacroFunctionProfileName::~MacroFunctionProfileName()
{
}

QSTATUS qm::MacroFunctionProfileName::value(
	MacroContext* pContext, MacroValue** ppValue) const
{
	assert(pContext);
	assert(ppValue);
	
	DECLARE_QSTATUS();
	
	*ppValue = 0;
	
	if (getArgSize() != 0)
		return error(*pContext, MacroErrorHandler::CODE_INVALIDARGSIZE);
	
	return MacroValueFactory::getFactory().newString(
		Application::getApplication().getProfileName(),
		reinterpret_cast<MacroValueString**>(ppValue));
}

const WCHAR* qm::MacroFunctionProfileName::getName() const
{
	return L"ProfileName";
}


/****************************************************************************
 *
 * MacroFunctionProgn
 *
 */

qm::MacroFunctionProgn::MacroFunctionProgn(QSTATUS* pstatus) :
	MacroFunction(pstatus)
{
}

qm::MacroFunctionProgn::~MacroFunctionProgn()
{
}

QSTATUS qm::MacroFunctionProgn::value(
	MacroContext* pContext, MacroValue** ppValue) const
{
	assert(pContext);
	assert(ppValue);
	
	DECLARE_QSTATUS();
	
	*ppValue = 0;
	
	if (getArgSize() == 0)
		return error(*pContext, MacroErrorHandler::CODE_INVALIDARGSIZE);
	
	for (size_t n = 0; n < getArgSize(); ++n) {
		MacroValuePtr pValue;
		status = getArg(n)->value(pContext, &pValue);
		CHECK_QSTATUS();
		
		if (n == getArgSize() - 1)
			*ppValue = pValue.release();
	}
	
	return QSTATUS_SUCCESS;
}

const WCHAR* qm::MacroFunctionProgn::getName() const
{
	return L"Progn";
}


/****************************************************************************
 *
 * MacroFunctionReferences
 *
 */

qm::MacroFunctionReferences::MacroFunctionReferences(QSTATUS* pstatus) :
	MacroFunction(pstatus)
{
}

qm::MacroFunctionReferences::~MacroFunctionReferences()
{
}

QSTATUS qm::MacroFunctionReferences::value(
	MacroContext* pContext, MacroValue** ppValue) const
{
	assert(pContext);
	assert(ppValue);
	
	DECLARE_QSTATUS();
	
	*ppValue = 0;
	
	size_t nSize = getArgSize();
	if (nSize != 0 && nSize != 1)
		return error(*pContext, MacroErrorHandler::CODE_INVALIDARGSIZE);
	
	MessageHolderBase* pmh = pContext->getMessageHolder();
	if (!pmh)
		return error(*pContext, MacroErrorHandler::CODE_NOCONTEXTMESSAGE);
	
	Message* pMessage = 0;
	status = pContext->getMessage(MacroContext::MESSAGETYPE_HEADER,
		L"References", &pMessage);
	CHECK_QSTATUS();
	
	unsigned int nReferences = -1;
	if (nSize > 0) {
		MacroValuePtr pValue;
		status = getArg(0)->value(pContext, &pValue);
		CHECK_QSTATUS();
		nReferences = pValue->number();
	}
	
	PartUtil::ReferenceList l;
	struct Deleter
	{
		Deleter(PartUtil::ReferenceList& l) : l_(l) {}
		~Deleter()
			{ std::for_each(l_.begin(), l_.end(), string_free<WSTRING>()); }
		PartUtil::ReferenceList& l_;
	} deleter(l);
	status = PartUtil(*pMessage).getReferences(&l);
	CHECK_QSTATUS();
	
	PartUtil::ReferenceList::size_type n = 0;
	if (nReferences != -1 && l.size() > nReferences)
		n = l.size() - nReferences;
	else
		n = 0;
	
	StringBuffer<WSTRING> buf(&status);
	CHECK_QSTATUS();
	
	while (n < l.size()) {
		if (buf.getLength() != 0) {
			status = buf.append(L" ");
			CHECK_QSTATUS();
		}
		status = buf.append(L"<");
		CHECK_QSTATUS();
		status = buf.append(l[n]);
		CHECK_QSTATUS();
		status = buf.append(L">");
		CHECK_QSTATUS();
		
		++n;
	}
	
	return MacroValueFactory::getFactory().newString(buf.getCharArray(),
		reinterpret_cast<MacroValueString**>(ppValue));
}

const WCHAR* qm::MacroFunctionReferences::getName() const
{
	return L"References";
}


/****************************************************************************
 *
 * MacroFunctionRelative
 *
 */

qm::MacroFunctionRelative::MacroFunctionRelative(
	bool bLess, QSTATUS* pstatus) :
	MacroFunction(pstatus),
	bLess_(bLess)
{
}

qm::MacroFunctionRelative::~MacroFunctionRelative()
{
}

QSTATUS qm::MacroFunctionRelative::value(
	MacroContext* pContext, MacroValue** ppValue) const
{
	assert(pContext);
	assert(ppValue);
	
	DECLARE_QSTATUS();
	
	*ppValue = 0;
	
	size_t nSize = getArgSize();
	if (nSize != 2 && nSize != 3)
		return error(*pContext, MacroErrorHandler::CODE_INVALIDARGSIZE);
	
	bool bCase = false;
	if (nSize > 2) {
		MacroValuePtr pValueCase;
		status = getArg(2)->value(pContext, &pValueCase);
		CHECK_QSTATUS();
		bCase = pValueCase->boolean();
	}
	
	MacroValuePtr pValueLhs;
	status = getArg(0)->value(pContext, &pValueLhs);
	CHECK_QSTATUS();
	
	MacroValuePtr pValueRhs;
	status = getArg(1)->value(pContext, &pValueRhs);
	CHECK_QSTATUS();
	
	long nComp = 0;
	if (pValueLhs->getType() == MacroValue::TYPE_BOOLEAN &&
		pValueRhs->getType() == MacroValue::TYPE_BOOLEAN) {
		nComp = static_cast<long>(pValueLhs->boolean()) -
			static_cast<long>(pValueRhs->boolean());
	}
	else if (pValueLhs->getType() == MacroValue::TYPE_NUMBER &&
		pValueRhs->getType() == MacroValue::TYPE_NUMBER) {
		nComp = pValueLhs->number() - pValueRhs->number();
	}
	else {
		string_ptr<WSTRING> wstrLhs;
		status = pValueLhs->string(&wstrLhs);
		CHECK_QSTATUS();
		
		string_ptr<WSTRING> wstrRhs;
		status = pValueRhs->string(&wstrRhs);
		CHECK_QSTATUS();
		
		int (*pfn)(const WCHAR*, const WCHAR*) = bCase ? &wcscmp : &_wcsicmp;
		int nComp = (*pfn)(wstrLhs.get(), wstrRhs.get());
	}
	
	return MacroValueFactory::getFactory().newBoolean(
		bLess_ ? nComp < 0 : nComp > 0,
		reinterpret_cast<MacroValueBoolean**>(ppValue));
}

const WCHAR* qm::MacroFunctionRelative::getName() const
{
	if (bLess_)
		return L"Less";
	else
		return L"Greater";
}


/****************************************************************************
 *
 * MacroFunctionRemove
 *
 */

qm::MacroFunctionRemove::MacroFunctionRemove(QSTATUS* pstatus) :
	MacroFunction(pstatus)
{
}

qm::MacroFunctionRemove::~MacroFunctionRemove()
{
}

QSTATUS qm::MacroFunctionRemove::value(
	MacroContext* pContext, MacroValue** ppValue) const
{
	assert(pContext);
	assert(ppValue);
	
	DECLARE_QSTATUS();
	
	*ppValue = 0;
	
	size_t nSize = getArgSize();
	if (nSize < 2)
		return error(*pContext, MacroErrorHandler::CODE_INVALIDARGSIZE);
	
	MacroValuePtr pValue;
	status = getArg(0)->value(pContext, &pValue);
	CHECK_QSTATUS();
	string_ptr<WSTRING> wstrValue;
	status = pValue->string(&wstrValue);
	CHECK_QSTATUS();
	
	Part part(&status);
	CHECK_QSTATUS();
	status = MessageCreator::setField(&part, L"Dummy", wstrValue.get(),
		MessageCreator::FIELDTYPE_ADDRESSLIST);
	CHECK_QSTATUS();
	AddressListParser addressList(0, &status);
	CHECK_QSTATUS();
	Part::Field f;
	status = part.getField(L"Dummy", &addressList, &f);
	CHECK_QSTATUS();
	if (f != Part::FIELD_EXIST) {
		*ppValue = pValue.release();
	}
	else {
		for (size_t n = 1; n < nSize; ++n) {
			MacroValuePtr pValue;
			status = getArg(n)->value(pContext, &pValue);
			CHECK_QSTATUS();
			if (pValue->getType() == MacroValue::TYPE_ADDRESS) {
				const MacroValueAddress::AddressList& l =
					static_cast<MacroValueAddress*>(pValue.get())->getAddress();
				MacroValueAddress::AddressList::const_iterator it = l.begin();
				while (it != l.end()) {
					status = remove(&addressList, *it);
					CHECK_QSTATUS();
					++it;
				}
			}
			else {
				string_ptr<WSTRING> wstr;
				status = pValue->string(&wstr);
				CHECK_QSTATUS();
				status = remove(&addressList, wstr.get());
				CHECK_QSTATUS();
			}
		}
		
		string_ptr<WSTRING> wstrValue;
		status = addressList.getValue(&wstrValue);
		CHECK_QSTATUS();
		
		status = MacroValueFactory::getFactory().newString(wstrValue.get(),
			reinterpret_cast<MacroValueString**>(ppValue));
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

const WCHAR* qm::MacroFunctionRemove::getName() const
{
	return L"Remove";
}

QSTATUS qm::MacroFunctionRemove::remove(
	AddressListParser* pAddressList, const WCHAR* pwszAddress)
{
	DECLARE_QSTATUS();
	
	const AddressListParser::AddressList& l = pAddressList->getAddressList();
	AddressListParser::AddressList::size_type n = 0;
	while (n < l.size()) {
		AddressParser* pAddress = l[n];
		string_ptr<WSTRING> wstrAddress;
		status = pAddress->getAddress(&wstrAddress);
		CHECK_QSTATUS();
		if (wcscmp(wstrAddress.get(), pwszAddress) == 0) {
			status = pAddressList->removeAddress(pAddress);
			CHECK_QSTATUS();
			delete pAddress;
		}
		else {
			++n;
		}
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * MacroFunctionSave
 *
 */

qm::MacroFunctionSave::MacroFunctionSave(QSTATUS* pstatus) :
	MacroFunction(pstatus)
{
}

qm::MacroFunctionSave::~MacroFunctionSave()
{
}

QSTATUS qm::MacroFunctionSave::value(
	MacroContext* pContext, MacroValue** ppValue) const
{
	assert(pContext);
	assert(ppValue);
	
	DECLARE_QSTATUS();
	
	*ppValue = 0;
	
	size_t nSize = getArgSize();
	if (nSize != 2 && nSize != 3)
		return error(*pContext, MacroErrorHandler::CODE_INVALIDARGSIZE);
	
	string_ptr<WSTRING> wstrPath;
	MacroValuePtr pValuePath;
	status = getArg(0)->value(pContext, &pValuePath);
	CHECK_QSTATUS();
	status = pValuePath->string(&wstrPath);
	CHECK_QSTATUS();
	
	string_ptr<WSTRING> wstrContent;
	MacroValuePtr pValueContent;
	status = getArg(1)->value(pContext, &pValueContent);
	CHECK_QSTATUS();
	status = pValueContent->string(&wstrContent);
	CHECK_QSTATUS();
	
	string_ptr<WSTRING> wstrEncoding;
	if (nSize > 2) {
		MacroValuePtr pValueEncoding;
		status = getArg(2)->value(pContext, &pValueEncoding);
		CHECK_QSTATUS();
		status = pValueEncoding->string(&wstrEncoding);
		CHECK_QSTATUS();
	}
	
	string_ptr<WSTRING> wstrAbsolutePath;
	status = pContext->resolvePath(wstrPath.get(), &wstrAbsolutePath);
	CHECK_QSTATUS();
	FileOutputStream stream(wstrAbsolutePath.get(), &status);
	CHECK_QSTATUS();
	OutputStreamWriter writer(&stream, false, wstrEncoding.get(), &status);
	CHECK_QSTATUS();
	BufferedWriter bufferedWriter(&writer, false, &status);
	CHECK_QSTATUS();
	status = bufferedWriter.write(wstrContent.get(), wcslen(wstrContent.get()));
	CHECK_QSTATUS();
	status = bufferedWriter.close();
	CHECK_QSTATUS();
	
	return MacroValueFactory::getFactory().newBoolean(true,
		reinterpret_cast<MacroValueBoolean**>(ppValue));
}

const WCHAR* qm::MacroFunctionSave::getName() const
{
	return L"Save";
}


/****************************************************************************
 *
 * MacroFunctionScript
 *
 */

qm::MacroFunctionScript::MacroFunctionScript(qs::QSTATUS* pstatus) :
	MacroFunction(pstatus)
{
}

qm::MacroFunctionScript::~MacroFunctionScript()
{
}

QSTATUS MacroFunctionScript::value(
	MacroContext* pContext, MacroValue** ppValue) const
{
	assert(pContext);
	assert(ppValue);
	
	DECLARE_QSTATUS();
	
	size_t nSize = getArgSize();
	if (nSize < 2)
		return error(*pContext, MacroErrorHandler::CODE_INVALIDARGSIZE);
	
	MacroValuePtr pValueScript;
	status = getArg(0)->value(pContext, &pValueScript);
	CHECK_QSTATUS();
	string_ptr<WSTRING> wstrScript;
	status = pValueScript->string(&wstrScript);
	CHECK_QSTATUS();
	
	MacroValuePtr pValueLanguage;
	status = getArg(1)->value(pContext, &pValueLanguage);
	CHECK_QSTATUS();
	string_ptr<WSTRING> wstrLanguage;
	status = pValueLanguage->string(&wstrLanguage);
	CHECK_QSTATUS();
	
	ScriptManager* pScriptManager = pContext->getDocument()->getScriptManager();
	Script* p = 0;
	status = pScriptManager->createScript(wstrScript.get(),
		wstrLanguage.get(), pContext->getDocument(), pContext->getProfile(),
		pContext->getWindow(), getModalHandler(), &p);
	CHECK_QSTATUS();
	std::auto_ptr<Script> pScript(p);
	if (!pScript.get())
		return QSTATUS_FAIL;
	
	typedef std::vector<VARIANT> ArgumentList;
	ArgumentList listArgs;
	struct Deleter
	{
		typedef std::vector<VARIANT> ArgumentList;
		Deleter(ArgumentList& l) : l_(l) {}
		~Deleter()
		{
			ArgumentList::iterator it = l_.begin();
			while (it != l_.end()) {
				::VariantClear(&(*it));
				++it;
			}
		}
		ArgumentList& l_;
	} deleter(listArgs);
	status = STLWrapper<ArgumentList>(listArgs).resize(nSize - 2);
	CHECK_QSTATUS();
	Variant v;
	std::fill(listArgs.begin(), listArgs.end(), v);
	for (size_t n = 0; n < listArgs.size(); ++n) {
		MacroValuePtr pValue;
		status = getArg(n + 2)->value(pContext, &pValue);
		CHECK_QSTATUS();
		switch (pValue->getType()) {
		case MacroValue::TYPE_BOOLEAN:
			listArgs[n].vt = VT_BOOL;
			listArgs[n].boolVal = pValue->boolean() ? VARIANT_TRUE : VARIANT_FALSE;
			break;
		case MacroValue::TYPE_NUMBER:
			listArgs[n].vt = VT_I4;
			listArgs[n].lVal = pValue->number();
			break;
		default:
			{
				string_ptr<WSTRING> wstrValue;
				status = pValue->string(&wstrValue);
				CHECK_QSTATUS();
				listArgs[n].vt = VT_BSTR;
				listArgs[n].bstrVal = ::SysAllocString(wstrValue.get());
				if (!listArgs[n].bstrVal)
					return QSTATUS_OUTOFMEMORY;
			}
			break;
		}
	}
	
	Variant varResult;
	status = pScript->run(&listArgs[0], listArgs.size(), &varResult);
	CHECK_QSTATUS();
	
	Variant var;
	HRESULT hr = ::VariantChangeType(&var, &varResult, 0, VT_BSTR);
	if (FAILED(hr))
		return QSTATUS_FAIL;
	
	return MacroValueFactory::getFactory().newString(var.bstrVal,
		reinterpret_cast<MacroValueString**>(ppValue));
}

const WCHAR* MacroFunctionScript::getName() const
{
	return L"Script";
}


/****************************************************************************
 *
 * MacroFunctionSet
 *
 */

qm::MacroFunctionSet::MacroFunctionSet(QSTATUS* pstatus) :
	MacroFunction(pstatus)
{
}

qm::MacroFunctionSet::~MacroFunctionSet()
{
}

QSTATUS qm::MacroFunctionSet::value(
	MacroContext* pContext, MacroValue** ppValue) const
{
	assert(pContext);
	assert(ppValue);
	
	DECLARE_QSTATUS();
	
	*ppValue = 0;
	
	size_t nSize = getArgSize();
	if (nSize != 2 && nSize != 3)
		return error(*pContext, MacroErrorHandler::CODE_INVALIDARGSIZE);
	
	bool bGlobal = false;
	if (nSize > 2) {
		MacroValuePtr pValue;
		status = getArg(2)->value(pContext, &pValue);
		CHECK_QSTATUS();
		bGlobal = pValue->boolean();
	}
	
	MacroValuePtr pValueName;
	status = getArg(0)->value(pContext, &pValueName);
	CHECK_QSTATUS();
	
	string_ptr<WSTRING> wstrName;
	status = pValueName->string(&wstrName);
	CHECK_QSTATUS();
	
	MacroValuePtr pValue;
	status = getArg(1)->value(pContext, &pValue);
	CHECK_QSTATUS();
	
	status = pContext->setVariable(wstrName.get(), pValue.get(), bGlobal);
	CHECK_QSTATUS();
	
	*ppValue = pValue.release();
	
	return QSTATUS_SUCCESS;
}

const WCHAR* qm::MacroFunctionSet::getName() const
{
	return L"Set";
}


/****************************************************************************
 *
 * MacroFunctionSize
 *
 */

qm::MacroFunctionSize::MacroFunctionSize(QSTATUS* pstatus) :
	MacroFunction(pstatus)
{
}

qm::MacroFunctionSize::~MacroFunctionSize()
{
}

QSTATUS qm::MacroFunctionSize::value(
	MacroContext* pContext, MacroValue** ppValue) const
{
	assert(pContext);
	assert(ppValue);
	
	DECLARE_QSTATUS();
	
	*ppValue = 0;
	
	size_t nArgSize = getArgSize();
	if (nArgSize != 0 && nArgSize != 1)
		return error(*pContext, MacroErrorHandler::CODE_INVALIDARGSIZE);
	
	MessageHolderBase* pmh = pContext->getMessageHolder();
	if (!pmh)
		return error(*pContext, MacroErrorHandler::CODE_NOCONTEXTMESSAGE);
	
	bool bTextOnly = false;
	if (nArgSize == 1) {
		MacroValuePtr pValue;
		status = getArg(0)->value(pContext, &pValue);
		CHECK_QSTATUS();
		bTextOnly = pValue->boolean();
	}
	
	unsigned int nSize = bTextOnly ? pmh->getTextSize() : pmh->getSize();
	return MacroValueFactory::getFactory().newNumber(nSize,
		reinterpret_cast<MacroValueNumber**>(ppValue));
}

const WCHAR* qm::MacroFunctionSize::getName() const
{
	return L"Size";
}


/****************************************************************************
 *
 * MacroFunctionSubAccount
 *
 */

qm::MacroFunctionSubAccount::MacroFunctionSubAccount(QSTATUS* pstatus) :
	MacroFunction(pstatus)
{
}

qm::MacroFunctionSubAccount::~MacroFunctionSubAccount()
{
}

QSTATUS qm::MacroFunctionSubAccount::value(
	MacroContext* pContext, MacroValue** ppValue) const
{
	assert(pContext);
	assert(ppValue);
	
	DECLARE_QSTATUS();
	
	*ppValue = 0;
	
	if (getArgSize() != 0)
		return error(*pContext, MacroErrorHandler::CODE_INVALIDARGSIZE);
	
	Account* pAccount = pContext->getAccount();
	MessageHolderBase* pmh = pContext->getMessageHolder();
	if (pmh)
		pAccount = pmh->getFolder()->getAccount();
	
	return MacroValueFactory::getFactory().newString(
		pAccount->getCurrentSubAccount()->getName(),
		reinterpret_cast<MacroValueString**>(ppValue));
}

const WCHAR* qm::MacroFunctionSubAccount::getName() const
{
	return L"SubAccount";
}


/****************************************************************************
 *
 * MacroFunctionSubject
 *
 */

qm::MacroFunctionSubject::MacroFunctionSubject(QSTATUS* pstatus) :
	MacroFunction(pstatus)
{
}

qm::MacroFunctionSubject::~MacroFunctionSubject()
{
}

QSTATUS qm::MacroFunctionSubject::value(
	MacroContext* pContext, MacroValue** ppValue) const
{
	assert(pContext);
	assert(ppValue);
	
	DECLARE_QSTATUS();
	
	*ppValue = 0;
	
	size_t nSize = getArgSize();
	if (nSize > 2)
		return error(*pContext, MacroErrorHandler::CODE_INVALIDARGSIZE);
	
	MessageHolderBase* pmh = pContext->getMessageHolder();
	if (!pmh)
		return error(*pContext, MacroErrorHandler::CODE_NOCONTEXTMESSAGE);
	
	Message* pMessage = 0;
	status = pContext->getMessage(MacroContext::MESSAGETYPE_HEADER,
		L"Subject", &pMessage);
	CHECK_QSTATUS();
	
	bool bRemoveRe = false;
	bool bRemoveMl = false;
	if (nSize > 1) {
		MacroValuePtr pValue;
		status = getArg(1)->value(pContext, &pValue);
		CHECK_QSTATUS();
		bRemoveMl = pValue->boolean();
	}
	if (nSize > 0) {
		MacroValuePtr pValue;
		status = getArg(0)->value(pContext, &pValue);
		CHECK_QSTATUS();
		bRemoveRe = pValue->boolean();
	}
	
	UnstructuredParser subject(&status);
	CHECK_QSTATUS();
	Part::Field field;
	status = pMessage->getField(L"Subject", &subject, &field);
	CHECK_QSTATUS();
	string_ptr<WSTRING> wstrSubject;
	const WCHAR* pwszSubject = 0;
	if (field == Part::FIELD_EXIST) {
		pwszSubject = subject.getValue();
		if (bRemoveRe || bRemoveMl) {
			const WCHAR* pwszSep = L"^[: ";
			for (int n = 0; n < 2; ++n) {
				WCHAR c = *pwszSubject;
				if (bRemoveMl && (c == L'[' || c == L'(')) {
					WCHAR cEnd = c == L'[' ? L']' : L')';
					bool bFind = false;
					for (const WCHAR* p = pwszSubject + 2; *p; ++p) {
						if (bFind && *p != L' ')
							break;
						else if (*p == cEnd)
							bFind = true;
					}
					if (bFind) {
						wstrSubject.reset(allocWString(p));
						if (!wstrSubject.get())
							return QSTATUS_OUTOFMEMORY;
						pwszSubject = wstrSubject.get();
					}
					bRemoveMl = false;
				}
				else if (bRemoveRe && wcslen(pwszSubject) > 2 &&
					_wcsnicmp(pwszSubject, L"re", 2) == 0 &&
					wcschr(pwszSep, pwszSubject[2])) {
					bool bFind = false;
					for (const WCHAR* p = pwszSubject + 2; *p; ++p) {
						if (bFind && *p != L' ')
							break;
						else if (*p == L' ' || *p == L':')
							bFind = true;
					}
					if (bFind) {
						wstrSubject.reset(allocWString(p));
						if (!wstrSubject.get())
							return QSTATUS_OUTOFMEMORY;
						pwszSubject = wstrSubject.get();
					}
					bRemoveRe = false;
				}
			}
		}
	}
	else {
		pwszSubject = L"";
	}
	
	return MacroValueFactory::getFactory().newString(pwszSubject,
		reinterpret_cast<MacroValueString**>(ppValue));
}

const WCHAR* qm::MacroFunctionSubject::getName() const
{
	return L"Subject";
}


/****************************************************************************
 *
 * MacroFunctionSubstring
 *
 */

qm::MacroFunctionSubstring::MacroFunctionSubstring(QSTATUS* pstatus) :
	MacroFunction(pstatus)
{
}

qm::MacroFunctionSubstring::~MacroFunctionSubstring()
{
}

QSTATUS qm::MacroFunctionSubstring::value(
	MacroContext* pContext, MacroValue** ppValue) const
{
	assert(pContext);
	assert(ppValue);
	
	DECLARE_QSTATUS();
	
	*ppValue = 0;
	
	size_t nSize = getArgSize();
	if (nSize != 2 && nSize != 3)
		return error(*pContext, MacroErrorHandler::CODE_INVALIDARGSIZE);
	
	size_t nLength = static_cast<size_t>(-1);
	if (nSize == 3) {
		MacroValuePtr pValue;
		status = getArg(2)->value(pContext, &pValue);
		CHECK_QSTATUS();
		nLength = pValue->number();
	}
	
	MacroValuePtr pValue;
	status = getArg(0)->value(pContext, &pValue);
	CHECK_QSTATUS();
	
	string_ptr<WSTRING> wstr;
	status = pValue->string(&wstr);
	CHECK_QSTATUS();
	size_t nLen = wcslen(wstr.get());
	
	MacroValuePtr pValueBegin;
	status = getArg(1)->value(pContext, &pValueBegin);
	CHECK_QSTATUS();
	
	unsigned int nBegin = pValueBegin->number();
	const WCHAR* pwsz = 0;
	if (static_cast<size_t>(nBegin) >= nLen) {
		pwsz = L"";
		nLength = 0;
	}
	else {
		if (nLength == static_cast<size_t>(-1))
			nLength = nLen - nBegin;
		pwsz = wstr.get() + nBegin;
	}
	
	return MacroValueFactory::getFactory().newString(pwsz, nLength,
		reinterpret_cast<MacroValueString**>(ppValue));
}

const WCHAR* qm::MacroFunctionSubstring::getName() const
{
	return L"Substring";
}


/****************************************************************************
 *
 * MacroFunctionSubstringSep
 *
 */

qm::MacroFunctionSubstringSep::MacroFunctionSubstringSep(
	bool bAfter, QSTATUS* pstatus) :
	MacroFunction(pstatus),
	bAfter_(bAfter)
{
}

qm::MacroFunctionSubstringSep::~MacroFunctionSubstringSep()
{
}

QSTATUS qm::MacroFunctionSubstringSep::value(
	MacroContext* pContext, MacroValue** ppValue) const
{
	assert(pContext);
	assert(ppValue);
	
	DECLARE_QSTATUS();
	
	*ppValue = 0;
	
	size_t nSize = getArgSize();
	if (nSize != 2 && nSize != 3)
		return error(*pContext, MacroErrorHandler::CODE_INVALIDARGSIZE);
	
	bool bCase = false;
	if (nSize == 3) {
		MacroValuePtr pValue;
		status = getArg(2)->value(pContext, &pValue);
		CHECK_QSTATUS();
		bCase = pValue->boolean();
	}
	
	MacroValuePtr pValue;
	status = getArg(0)->value(pContext, &pValue);
	CHECK_QSTATUS();
	
	string_ptr<WSTRING> wstr;
	status = pValue->string(&wstr);
	CHECK_QSTATUS();
	
	MacroValuePtr pValueSep;
	status = getArg(1)->value(pContext, &pValueSep);
	CHECK_QSTATUS();
	
	string_ptr<WSTRING> wstrSep;
	status = pValueSep->string(&wstrSep);
	CHECK_QSTATUS();
	
	string_ptr<WSTRING> wstrLower;
	const WCHAR* pwsz = wstr.get();
	if (!bCase) {
		string_ptr<WSTRING> wstrSepLower(tolower(wstrSep.get()));
		if (!wstrSepLower.get())
			return QSTATUS_OUTOFMEMORY;
		
		string_ptr<WSTRING> wstrSepUpper(toupper(wstrSep.get()));
		if (!wstrSepUpper.get())
			return QSTATUS_OUTOFMEMORY;
		
		if (wcscmp(wstrSepLower.get(), wstrSepUpper.get()) == 0) {
			wstrLower.reset(tolower(wstr.get()));
			if (!wstrLower.get())
				return QSTATUS_OUTOFMEMORY;
			wstrSep.reset(wstrSepLower.release());
			pwsz = wstrLower.get();
		}
	}
	
	BMFindString<WSTRING> bmfs(wstrSep.get(), &status);
	CHECK_QSTATUS();
	const WCHAR* p = bmfs.find(pwsz);
	if (!p) {
		return MacroValueFactory::getFactory().newString(L"",
			reinterpret_cast<MacroValueString**>(ppValue));
	}
	else if (bAfter_) {
		return MacroValueFactory::getFactory().newString(
			wstr.get() + (p - pwsz) + wcslen(wstrSep.get()),
			reinterpret_cast<MacroValueString**>(ppValue));
	}
	else {
		return MacroValueFactory::getFactory().newString(
			wstr.get(), p - pwsz,
			reinterpret_cast<MacroValueString**>(ppValue));
	}
}

const WCHAR* qm::MacroFunctionSubstringSep::getName() const
{
	if (bAfter_)
		return L"SubstringAfter";
	else
		return L"SubstringBefore";
}


/****************************************************************************
 *
 * MacroFunctionVariable
 *
 */

qm::MacroFunctionVariable::MacroFunctionVariable(QSTATUS* pstatus) :
	MacroFunction(pstatus)
{
}

qm::MacroFunctionVariable::~MacroFunctionVariable()
{
}

QSTATUS qm::MacroFunctionVariable::value(
	MacroContext* pContext, MacroValue** ppValue) const
{
	assert(pContext);
	assert(ppValue);
	
	DECLARE_QSTATUS();
	
	*ppValue = 0;
	
	size_t nSize = getArgSize();
	if (nSize < 1 && 3 < nSize)
		return error(*pContext, MacroErrorHandler::CODE_INVALIDARGSIZE);
	
	MacroValuePtr pValueName;
	status = getArg(0)->value(pContext, &pValueName);
	CHECK_QSTATUS();
	
	string_ptr<WSTRING> wstrName;
	status = pValueName->string(&wstrName);
	CHECK_QSTATUS();
	
	MacroValuePtr pValue;
	status = pContext->getVariable(wstrName.get(), &pValue);
	CHECK_QSTATUS();
	
	if (!pValue.get()) {
		if (nSize != 1) {
			bool bGlobal = false;
			if (nSize > 2) {
				MacroValuePtr pValueGlobal;
				status = getArg(2)->value(pContext, &pValueGlobal);
				CHECK_QSTATUS();
				bGlobal = pValueGlobal->boolean();
			}
			status = getArg(1)->value(pContext, &pValue);
			CHECK_QSTATUS();
			
			status = pContext->setVariable(wstrName.get(), pValue.get(), bGlobal);
			CHECK_QSTATUS();
		}
		else {
			status = MacroValueFactory::getFactory().newString(L"",
				reinterpret_cast<MacroValueString**>(&pValue));
			CHECK_QSTATUS();
		}
	}
	*ppValue = pValue.release();
	
	return QSTATUS_SUCCESS;
}

const WCHAR* qm::MacroFunctionVariable::getName() const
{
	return L"Variable";
}


/****************************************************************************
 *
 * MacroFunctionWhile
 *
 */

qm::MacroFunctionWhile::MacroFunctionWhile(QSTATUS* pstatus) :
	MacroFunction(pstatus)
{
}

qm::MacroFunctionWhile::~MacroFunctionWhile()
{
}

QSTATUS qm::MacroFunctionWhile::value(
	MacroContext* pContext, MacroValue** ppValue) const
{
	assert(pContext);
	assert(ppValue);
	
	DECLARE_QSTATUS();
	
	*ppValue = 0;
	
	if (getArgSize() != 2)
		return error(*pContext, MacroErrorHandler::CODE_INVALIDARGSIZE);
	
	while (true) {
		MacroValuePtr pValueCondition;
		status = getArg(0)->value(pContext, &pValueCondition);
		CHECK_QSTATUS();
		if (!pValueCondition->boolean()) {
			*ppValue = pValueCondition.release();
			break;
		}
		
		MacroValuePtr pValue;
		status = getArg(1)->value(pContext, &pValue);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

const WCHAR* qm::MacroFunctionWhile::getName() const
{
	return L"While";
}


/****************************************************************************
 *
 * MacroFunctionFactory
 *
 */

MacroFunctionFactory qm::MacroFunctionFactory::factory__;

qm::MacroFunctionFactory::MacroFunctionFactory()
{
}

qm::MacroFunctionFactory::~MacroFunctionFactory()
{
}

QSTATUS qm::MacroFunctionFactory::newFunction(MacroParser::Type type,
	const WCHAR* pwszName, MacroFunction** ppFunction) const
{
	assert(pwszName);
	assert(ppFunction);
	
	DECLARE_QSTATUS();
	
	*ppFunction = 0;
	
#define BEGIN_DECLARE_FUNCTION() \
	if (false) { \
	} \

#define END_DECLARE_FUNCTION() \
	else { \
		MacroFunctionFunction* pFunction = 0; \
		status = newQsObject(pwszName, &pFunction); \
		CHECK_QSTATUS(); \
		*ppFunction = pFunction; \
	} \

#define DECLARE_FUNCTION0(classname, name) \
	else if (_wcsicmp(pwszName, name) == 0) { \
		MacroFunction##classname* pFunction = 0; \
		status = newQsObject(&pFunction); \
		CHECK_QSTATUS(); \
		*ppFunction = pFunction; \
	} \
	
#define DECLARE_FUNCTION1(classname, name, arg1) \
	else if (_wcsicmp(pwszName, name) == 0) { \
		MacroFunction##classname* pFunction = 0; \
		status = newQsObject(arg1, &pFunction); \
		CHECK_QSTATUS(); \
		*ppFunction = pFunction; \
	} \
	
#define DECLARE_FUNCTION_TYPE0(classname, name, typename) \
	else if (_wcsicmp(pwszName, name) == 0 && (type & typename)) { \
		MacroFunction##classname* pFunction = 0; \
		status = newQsObject(&pFunction); \
		CHECK_QSTATUS(); \
		*ppFunction = pFunction; \
	} \
	
#define DECLARE_FUNCTION_TYPE1(classname, name, arg1, typename) \
	else if (_wcsicmp(pwszName, name) == 0 && (type & typename)) { \
		MacroFunction##classname* pFunction = 0; \
		status = newQsObject(arg1, &pFunction); \
		CHECK_QSTATUS(); \
		*ppFunction = pFunction; \
	} \

#define M MacroParser::TYPE_MESSAGE
#define RT MacroParser::TYPE_RULE | MacroParser::TYPE_TEMPLATE
#define T MacroParser::TYPE_TEMPLATE
	
	BEGIN_DECLARE_FUNCTION()
		DECLARE_FUNCTION0(		Account,			L"account"												)
		DECLARE_FUNCTION0(		AccountDirectory,	L"accountdirectory"										)
		DECLARE_FUNCTION1(		Address,			L"address",			false								)
		DECLARE_FUNCTION_TYPE0(	AddressBook,		L"addressbook",										T	)
		DECLARE_FUNCTION1(		Additive,			L"add",				true								)
		DECLARE_FUNCTION0(		And,				L"and"													)
		DECLARE_FUNCTION0(		Attachment,			L"attachment"											)
		DECLARE_FUNCTION0(		Body,				L"body"													)
		DECLARE_FUNCTION1(		Contain,			L"beginwith",		true								)
		DECLARE_FUNCTION0(		Clipboard,			L"clipboard"											)
		DECLARE_FUNCTION0(		ComputerName,		L"computername"											)
		DECLARE_FUNCTION0(		Concat,				L"concat"												)
		DECLARE_FUNCTION1(		Contain,			L"contain",			false								)
		DECLARE_FUNCTION_TYPE1(	Copy,				L"copy",			false,							M	)
		DECLARE_FUNCTION0(		Date,				L"date"													)
		DECLARE_FUNCTION0(		Decode,				L"decode"												)
		DECLARE_FUNCTION0(		Defun,				L"defun"												)
		DECLARE_FUNCTION_TYPE0(	Delete,				L"delete",											M	)
		DECLARE_FUNCTION1(		Flag,				L"deleted",			MessageHolder::FLAG_DELETED			)
		DECLARE_FUNCTION1(		Flag,				L"download",		MessageHolder::FLAG_DOWNLOAD		)
		DECLARE_FUNCTION1(		Flag,				L"downloadtext",	MessageHolder::FLAG_DOWNLOADTEXT	)
		DECLARE_FUNCTION1(		Flag,				L"draft",			MessageHolder::FLAG_DRAFT			)
		DECLARE_FUNCTION0(		Equal, 				L"equal"												)
		DECLARE_FUNCTION1(		Eval, 				L"eval",			type								)
		DECLARE_FUNCTION0(		Execute, 			L"execute"												)
		DECLARE_FUNCTION0(		Exist, 				L"exist"												)
		DECLARE_FUNCTION0(		Exit, 				L"exit"													)
		DECLARE_FUNCTION1(		Boolean,			L"false",			false								)
		DECLARE_FUNCTION0(		Field, 				L"field"												)
		DECLARE_FUNCTION0(		FieldParameter,		L"fieldparameter"										)
		DECLARE_FUNCTION0(		Find, 				L"find"													)
		DECLARE_FUNCTION0(		Flag,				L"flag"													)
		DECLARE_FUNCTION0(		Folder, 			L"folder"												)
		DECLARE_FUNCTION0(		ForEach,			L"foreach"												)
		DECLARE_FUNCTION1(		Flag,				L"forwarded",		MessageHolder::FLAG_FORWARDED		)
		DECLARE_FUNCTION_TYPE0(	FormatAddress, 		L"formataddress",									T	)
		DECLARE_FUNCTION0(		FormatDate, 		L"formatdate"											)
		DECLARE_FUNCTION1(		Relative,			L"greater",			false								)
		DECLARE_FUNCTION0(		Header, 			L"header"												)
		DECLARE_FUNCTION0(		I, 					L"i"													)
		DECLARE_FUNCTION0(		Id, 				L"id"													)
		DECLARE_FUNCTION0(		Identity, 			L"identity"												)
		DECLARE_FUNCTION0(		If, 				L"if"													)
		DECLARE_FUNCTION1(		Include,			L"include",			type								)
		DECLARE_FUNCTION_TYPE0(	InputBox,	 		L"inputbox",										RT	)
		DECLARE_FUNCTION0(		Length,				L"length"												)
		DECLARE_FUNCTION1(		Relative,			L"less",			true								)
		DECLARE_FUNCTION0(		Load,				L"load"													)
		DECLARE_FUNCTION1(		Flag,				L"marked",			MessageHolder::FLAG_MARKED			)
		DECLARE_FUNCTION_TYPE0(	MessageBox,			L"messagebox",										RT	)
		DECLARE_FUNCTION0(		Messages,			L"messages"												)
		DECLARE_FUNCTION1(		Additive,			L"minus",			false								)
		DECLARE_FUNCTION_TYPE1(	Copy,				L"move",			true,							M	)
		DECLARE_FUNCTION1(		Flag,				L"multipart",		MessageHolder::FLAG_MULTIPART		)
		DECLARE_FUNCTION1(		Address,			L"name",			true								)
		DECLARE_FUNCTION0(		Not,				L"not"													)
		DECLARE_FUNCTION0(		Or,					L"or"													)
		DECLARE_FUNCTION0(		OSVersion,			L"osversion"											)
		DECLARE_FUNCTION0(		ParseURL,			L"parseurl"												)
		DECLARE_FUNCTION0(		Part,				L"part"													)
		DECLARE_FUNCTION0(		Passed,				L"passed"												)
		DECLARE_FUNCTION1(		Flag,				L"partial",			MessageHolder::FLAG_PARTIAL_MASK	)
		DECLARE_FUNCTION0(		ProcessId,			L"processid"											)
		DECLARE_FUNCTION0(		Profile,			L"profile"												)
		DECLARE_FUNCTION0(		ProfileName,		L"profilename"											)
		DECLARE_FUNCTION0(		Progn,				L"progn"												)
		DECLARE_FUNCTION0(		References,			L"references"											)
		DECLARE_FUNCTION0(		Remove,				L"remove"												)
		DECLARE_FUNCTION1(		Flag,				L"replied",			MessageHolder::FLAG_REPLIED			)
		DECLARE_FUNCTION0(		Save,				L"save"													)
		DECLARE_FUNCTION1(		Flag,				L"seen",			MessageHolder::FLAG_SEEN			)
		DECLARE_FUNCTION1(		Flag,				L"sent",			MessageHolder::FLAG_SENT			)
		DECLARE_FUNCTION0(		Script,				L"script"												)
		DECLARE_FUNCTION0(		Set,				L"set"													)
		DECLARE_FUNCTION0(		Size,				L"size"													)
		DECLARE_FUNCTION0(		SubAccount,			L"subaccount"											)
		DECLARE_FUNCTION0(		Subject,			L"subject"												)
		DECLARE_FUNCTION0(		Substring,			L"substring"											)
		DECLARE_FUNCTION1(		SubstringSep,		L"substringafter",	true								)
		DECLARE_FUNCTION1(		SubstringSep,		L"substringbefore",	false								)
		DECLARE_FUNCTION1(		Boolean,			L"true",			true								)
		DECLARE_FUNCTION0(		Variable,			L"variable"												)
		DECLARE_FUNCTION0(		While,				L"while"												)
	END_DECLARE_FUNCTION()
	
	return QSTATUS_SUCCESS;
}

const MacroFunctionFactory& qm::MacroFunctionFactory::getFactory()
{
	return factory__;
}
