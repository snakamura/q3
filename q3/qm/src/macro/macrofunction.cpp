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
#include <qmfolder.h>
#include <qmmacro.h>
#include <qmmessage.h>
#include <qmmessageholder.h>
#include <qmtemplate.h>

#include <qmscript.h>

#include <qsconv.h>
#include <qsinit.h>
#include <qsosutil.h>
#include <qsregex.h>
#include <qsstl.h>
#include <qsstream.h>
#include <qswindow.h>

#include <algorithm>

#include <tchar.h>

#include "macro.h"
#include "../model/addressbook.h"
#include "../model/uri.h"
#include "../script/scriptmanager.h"
#include "../ui/dialogs.h"

#pragma warning(disable:4786)

using namespace qm;
using namespace qmscript;
using namespace qs;

#define VALUE(name, expr) \
	MacroValuePtr name(expr); \
	if (!name.get()) \
		return MacroValuePtr(); \

#define ARG(name, index) \
	MacroValuePtr name(getArg(index)->value(pContext)); \
	if (!name.get()) \
		return MacroValuePtr(); \

#define LOG(name) \
	do { \
		Log log(InitThread::getInitThread().getLogger(), L"qm::MacroFunction" L#name); \
		if (log.isDebugEnabled()) { \
			wstring_ptr wstr(getString()); \
			wstring_ptr wstrLog(concat(L"Processing: ", wstr.get())); \
			log.debug(wstrLog.get()); \
		} \
	} while (false)


/****************************************************************************
 *
 * MacroFunction
 *
 */

qm::MacroFunction::MacroFunction()
{
}

qm::MacroFunction::~MacroFunction()
{
	for (ArgList::iterator it = listArg_.begin(); it != listArg_.end(); ++it)
		(*it)->release();
}

void qm::MacroFunction::addArg(MacroExprPtr pArg)
{
	listArg_.push_back(pArg.get());
	pArg.release();
}

wstring_ptr qm::MacroFunction::getString() const
{
	wstring_ptr wstrArg(getArgString());
	const ConcatW c[] = {
		{ L"@",				-1 },
		{ getName(),		-1 },
		{ L"(",				-1 },
		{ wstrArg.get(),	-1 },
		{ L")",				-1 }
	};
	return concat(c, countof(c));
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

bool qm::MacroFunction::checkArgSize(MacroContext* pContext,
									 size_t n) const
{
	size_t nSize = getArgSize();
	if (nSize != n) {
		error(*pContext, MacroErrorHandler::CODE_INVALIDARGSIZE);
		return false;
	}
	else {
		return true;
	}
}

bool qm::MacroFunction::checkArgSizeRange(MacroContext* pContext,
										  size_t nMin,
										  size_t nMax) const
{
	size_t nSize = getArgSize();
	if (nSize < nMin || nMax < nSize) {
		error(*pContext, MacroErrorHandler::CODE_INVALIDARGSIZE);
		return false;
	}
	else {
		return true;
	}
}

bool qm::MacroFunction::checkArgSizeMin(MacroContext* pContext,
										size_t nMin) const
{
	size_t nSize = getArgSize();
	if (nSize < nMin) {
		error(*pContext, MacroErrorHandler::CODE_INVALIDARGSIZE);
		return false;
	}
	else {
		return true;
	}
}

const Part* qm::MacroFunction::getPart(MacroContext* pContext,
									   size_t n) const
{
	MacroValuePtr pValue(getArg(n)->value(pContext));
	if (pValue->getType() != MacroValue::TYPE_PART) {
		error(*pContext, MacroErrorHandler::CODE_INVALIDARGTYPE);
		return 0;
	}
	
	const Part* pPart = static_cast<MacroValuePart*>(pValue.get())->getPart();
	if (!pPart) {
		error(*pContext, MacroErrorHandler::CODE_INVALIDPART);
		return 0;
	}
	return pPart;
}

Message* qm::MacroFunction::getMessage(MacroContext* pContext,
									   MacroContext::MessageType type,
									   const WCHAR* pwszField) const
{
	Message* pMessage = pContext->getMessage(type, pwszField);
	if (!pMessage)
		error(*pContext, MacroErrorHandler::CODE_GETMESSAGE);
	return pMessage;
}

wstring_ptr qm::MacroFunction::getArgString() const
{
	StringBuffer<WSTRING> buf;
	
	for (ArgList::const_iterator it = listArg_.begin(); it != listArg_.end(); ++it) {
		if (it != listArg_.begin())
			buf.append(L',');
		wstring_ptr wstrArg((*it)->getString());
		buf.append(wstrArg.get());
	}
	return buf.getString();
}


/****************************************************************************
 *
 * MacroFunctionAccount
 *
 */

qm::MacroFunctionAccount::MacroFunctionAccount()
{
}

qm::MacroFunctionAccount::~MacroFunctionAccount()
{
}

MacroValuePtr qm::MacroFunctionAccount::value(MacroContext* pContext) const
{
	assert(pContext);
	
	LOG(Account);
	
	if (!checkArgSizeRange(pContext, 0, 1))
		return MacroValuePtr();
	
	size_t nSize = getArgSize();
	
	bool bSub = false;
	if (nSize > 0) {
		ARG(pValue, 0);
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
	
	return MacroValueFactory::getFactory().newString(pwszName);
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

qm::MacroFunctionAccountDirectory::MacroFunctionAccountDirectory()
{
}

qm::MacroFunctionAccountDirectory::~MacroFunctionAccountDirectory()
{
}

MacroValuePtr qm::MacroFunctionAccountDirectory::value(MacroContext* pContext) const
{
	assert(pContext);
	
	LOG(AccountDirectory);
	
	if (!checkArgSizeRange(pContext, 0, 1))
		return MacroValuePtr();
	size_t nSize = getArgSize();
	
	wstring_ptr wstrAccount;
	if (nSize > 0) {
		ARG(pValue, 0);
		wstrAccount = pValue->string();
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
	
	return MacroValueFactory::getFactory().newString(pAccount->getPath());
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

qm::MacroFunctionAdditive::MacroFunctionAdditive(bool bAdd) :
	bAdd_(bAdd)
{
}

qm::MacroFunctionAdditive::~MacroFunctionAdditive()
{
}

MacroValuePtr qm::MacroFunctionAdditive::value(MacroContext* pContext) const
{
	assert(pContext);
	
	LOG(Additive);
	
	if (!checkArgSize(pContext, 2))
		return MacroValuePtr();
	
	ARG(pValueLhs, 0);
	ARG(pValueRhs, 1);
	
	unsigned int nValue = 0;
	if (bAdd_)
		nValue = pValueLhs->number() + pValueRhs->number();
	else
		nValue = pValueLhs->number() - pValueRhs->number();
	
	return MacroValueFactory::getFactory().newNumber(nValue);
}

const WCHAR* qm::MacroFunctionAdditive::getName() const
{
	if (bAdd_)
		return L"Add";
	else
		return L"Subtract";
}


/****************************************************************************
 *
 * MacroFunctionAddress
 *
 */

qm::MacroFunctionAddress::MacroFunctionAddress(bool bName) :
	bName_(bName)
{
}

qm::MacroFunctionAddress::~MacroFunctionAddress()
{
}

MacroValuePtr qm::MacroFunctionAddress::value(MacroContext* pContext) const
{
	assert(pContext);
	
	LOG(Address);
	
	if (!checkArgSize(pContext, 1))
		return MacroValuePtr();
	
	ARG(pValue, 0);
	if (pValue->getType() != MacroValue::TYPE_FIELD)
		return error(*pContext, MacroErrorHandler::CODE_INVALIDARGTYPE);
	
	MacroValueField* pValueField = static_cast<MacroValueField*>(pValue.get());
	std::vector<WSTRING> v;
	StringListFree<std::vector<WSTRING> > free(v);
	if (bName_)
		pValueField->getNames(&v);
	else
		pValueField->getAddresses(&v);
	
	return MacroValueFactory::getFactory().newAddress(v);
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

qm::MacroFunctionAddressBook::MacroFunctionAddressBook()
{
}

qm::MacroFunctionAddressBook::~MacroFunctionAddressBook()
{
}

MacroValuePtr qm::MacroFunctionAddressBook::value(MacroContext* pContext) const
{
	assert(pContext);
	
	LOG(AddressBook);
	
	if (!checkArgSizeRange(pContext, 0, 3))
		return MacroValuePtr();
	
	size_t nSize = getArgSize();
	
	wstring_ptr wstrAddress[3];
	for (unsigned int nArg = 0; nArg < countof(wstrAddress); ++nArg) {
		if (nSize > 2 - nArg) {
			ARG(pValue, 2 - nArg);
			wstrAddress[2 - nArg] = pValue->string();
		}
	}
	
	const WCHAR* pwszAddress[] = {
		wstrAddress[0].get(),
		wstrAddress[1].get(),
		wstrAddress[2].get()
	};
	SelectAddressDialog dialog(pContext->getDocument()->getAddressBook(),
		pContext->getProfile(), pwszAddress);
	if (dialog.doModal(pContext->getWindow()) != IDOK) {
		pContext->setReturnType(MacroContext::RETURNTYPE_CANCEL);
		return MacroValuePtr();
	}
	
	StringBuffer<WSTRING> buf;
	
	struct Type
	{
		const WCHAR* pwszName_;
		SelectAddressDialog::Type type_;
	} types[] = {
		{ L"To: ",	SelectAddressDialog::TYPE_TO	},
		{ L"Cc: ",	SelectAddressDialog::TYPE_CC,	},
		{ L"Bcc: ",	SelectAddressDialog::TYPE_BCC	}
	};
	for (int n = 0; n < countof(types); ++n) {
		const SelectAddressDialog::AddressList& l =
			dialog.getAddresses(types[n].type_);
		if (!l.empty()) {
			buf.append(types[n].pwszName_);
			for (SelectAddressDialog::AddressList::const_iterator it = l.begin(); it != l.end(); ++it) {
				if (it != l.begin())
					buf.append(L", ");
				buf.append(*it);
			}
			buf.append(L"\n");
		}
	}
	
	return MacroValueFactory::getFactory().newString(buf.getCharArray());
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

qm::MacroFunctionAnd::MacroFunctionAnd()
{
}

qm::MacroFunctionAnd::~MacroFunctionAnd()
{
}

MacroValuePtr qm::MacroFunctionAnd::value(MacroContext* pContext) const
{
	assert(pContext);
	
	LOG(And);
	
	if (!checkArgSizeMin(pContext, 1))
		return MacroValuePtr();
	
	size_t nSize = getArgSize();
	
	bool bValue = true;
	for (size_t n = 0; n < nSize && bValue; ++n) {
		ARG(pValue, n);
		if (!pValue->boolean())
			bValue = false;
	}
	return MacroValueFactory::getFactory().newBoolean(bValue);
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

qm::MacroFunctionAttachment::MacroFunctionAttachment()
{
}

qm::MacroFunctionAttachment::~MacroFunctionAttachment()
{
}

MacroValuePtr qm::MacroFunctionAttachment::value(MacroContext* pContext) const
{
	assert(pContext);
	
	LOG(Attachment);
	
	if (!checkArgSizeRange(pContext, 0, 2))
		return MacroValuePtr();
	
	size_t nSize = getArgSize();
	
	MessageHolderBase* pmh = pContext->getMessageHolder();
	if (!pmh)
		return error(*pContext, MacroErrorHandler::CODE_NOCONTEXTMESSAGE);
	
	Message* pMessage = getMessage(pContext, MacroContext::MESSAGETYPE_TEXT, 0);
	if (!pMessage)
		return error(*pContext, MacroErrorHandler::CODE_GETMESSAGE);
	
	const WCHAR* pwszSep = L", ";
	wstring_ptr wstrSep;
	if (nSize > 0) {
		ARG(pValue, 0);
		wstrSep = pValue->string();
		pwszSep = wstrSep.get();
	}
	
	bool bURI = false;
	if (nSize > 1) {
		ARG(pValue, 1);
		bURI = pValue->boolean();
	}
	if (bURI && !pmh->getMessageHolder())
		return error(*pContext, MacroErrorHandler::CODE_FAIL);
	
	AttachmentParser::AttachmentList l;
	AttachmentParser::AttachmentListFree free(l);
	AttachmentParser(*pMessage).getAttachments(true, &l);
	
	StringBuffer<WSTRING> buf;
	for (AttachmentParser::AttachmentList::iterator it = l.begin(); it != l.end(); ++it) {
		if (it != l.begin())
			buf.append(pwszSep);
		
		if (bURI) {
			URI uri(pmh->getMessageHolder(), pMessage,
				(*it).second, URIFragment::TYPE_BODY);
			wstring_ptr wstrURI(uri.toString());
			buf.append(wstrURI.get());
		}
		else {
			buf.append((*it).first);
		}
	}
	
	return MacroValueFactory::getFactory().newString(buf.getCharArray());
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

qm::MacroFunctionBody::MacroFunctionBody()
{
}

qm::MacroFunctionBody::~MacroFunctionBody()
{
}

MacroValuePtr qm::MacroFunctionBody::value(MacroContext* pContext) const
{
	assert(pContext);
	
	LOG(Body);
	
	if (!checkArgSizeRange(pContext, 0, 3))
		return MacroValuePtr();
	
	size_t nSize = getArgSize();

	MessageHolderBase* pmh = pContext->getMessageHolder();
	if (!pmh)
		return error(*pContext, MacroErrorHandler::CODE_NOCONTEXTMESSAGE);
	
	const Part* pPart = 0;
	if (nSize > 2) {
		pPart = getPart(pContext, 2);
		if (!pPart)
			return MacroValuePtr();
	}
	
	enum View {
		VIEW_NONE,
		VIEW_FORCERFC822INLINE,
		VIEW_INLINE
	};
	unsigned int nView = 0;
	if (nSize > 1) {
		ARG(pValue, 1);
		nView = pValue->number();
	}
	if (nView > 2)
		nView = 0;
	
	wstring_ptr wstrQuote;
	if (nSize > 0) {
		ARG(pValue, 0);
		wstrQuote = pValue->string();
	}
	
	Message* pMessage = getMessage(pContext,
		nView != VIEW_NONE ? MacroContext::MESSAGETYPE_TEXT : MacroContext::MESSAGETYPE_ALL, 0);
	if (!pMessage)
		return error(*pContext, MacroErrorHandler::CODE_GETMESSAGE);
	
	if (!pPart)
		pPart = pMessage;
	
	wxstring_ptr wstrBody;
	PartUtil util(*pPart);
	if (nView == VIEW_NONE)
		wstrBody = util.getAllText(wstrQuote.get(), pContext->getBodyCharset(), true);
	else
		wstrBody = util.getBodyText(wstrQuote.get(),
			pContext->getBodyCharset(), nView == VIEW_FORCERFC822INLINE);
	if (!wstrBody.get())
		return MacroValuePtr();
	
	return MacroValueFactory::getFactory().newString(wstrBody.get());
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

qm::MacroFunctionBoolean::MacroFunctionBoolean(bool b) :
	b_(b)
{
}

qm::MacroFunctionBoolean::~MacroFunctionBoolean()
{
}

MacroValuePtr qm::MacroFunctionBoolean::value(MacroContext* pContext) const
{
	assert(pContext);
	
	LOG(Boolean);
	
	if (!checkArgSize(pContext, 0))
		return MacroValuePtr();
	
	return MacroValueFactory::getFactory().newBoolean(b_);
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

qm::MacroFunctionClipboard::MacroFunctionClipboard()
{
}

qm::MacroFunctionClipboard::~MacroFunctionClipboard()
{
}

MacroValuePtr qm::MacroFunctionClipboard::value(MacroContext* pContext) const
{
	assert(pContext);
	
	LOG(Clipboard);
	
	if (!checkArgSizeRange(pContext, 0, 1))
		return MacroValuePtr();
	
	size_t nSize = getArgSize();
	
	const WCHAR* pwszText = L"";
	wstring_ptr wstrText;
	if (nSize == 0) {
		wstrText = Clipboard::getText();
		if (wstrText.get())
			pwszText = wstrText.get();
	}
	else {
		MacroValuePtr pValueContent(getArg(0)->value(pContext));
		
		wstring_ptr wstrText(pValueContent->string());
		
		if (!Clipboard::setText(wstrText.get()))
			return error(*pContext, MacroErrorHandler::CODE_FAIL);
	}
	
	return MacroValueFactory::getFactory().newString(pwszText);
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

qm::MacroFunctionComputerName::MacroFunctionComputerName()
{
}

qm::MacroFunctionComputerName::~MacroFunctionComputerName()
{
}

MacroValuePtr qm::MacroFunctionComputerName::value(MacroContext* pContext) const
{
	assert(pContext);
	
	LOG(ComputerName);
	
	if (!checkArgSize(pContext, 0))
		return MacroValuePtr();
	
	wstring_ptr wstrName;
#ifdef _WIN32_WCE
	Registry reg(HKEY_LOCAL_MACHINE, L"Ident");
	if (!reg)
		return MacroValuePtr();
	if (!reg.getValue(L"Name", &wstrName))
		return MacroValuePtr();
#else
	TCHAR tszComputerName[MAX_COMPUTERNAME_LENGTH + 1];
	DWORD dwSize = countof(tszComputerName);
	::GetComputerName(tszComputerName, &dwSize);
	wstrName = tcs2wcs(tszComputerName);
#endif
	
	return MacroValueFactory::getFactory().newString(wstrName.get());
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

qm::MacroFunctionConcat::MacroFunctionConcat()
{
}

qm::MacroFunctionConcat::~MacroFunctionConcat()
{
}

MacroValuePtr qm::MacroFunctionConcat::value(MacroContext* pContext) const
{
	assert(pContext);
	
	LOG(Concat);
	
	StringBuffer<WSTRING> buf;
	for (size_t n = 0; n < getArgSize(); ++n) {
		ARG(pValue, n);
		wstring_ptr wstr(pValue->string());
		buf.append(wstr.get());
	}
	
	return MacroValueFactory::getFactory().newString(buf.getCharArray());
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

qm::MacroFunctionContain::MacroFunctionContain(bool bBeginWith) :
	bBeginWith_(bBeginWith)
{
}

qm::MacroFunctionContain::~MacroFunctionContain()
{
}

MacroValuePtr qm::MacroFunctionContain::value(MacroContext* pContext) const
{
	assert(pContext);
	
	LOG(Contain);
	
	if (!checkArgSizeRange(pContext, 2, 3))
		return MacroValuePtr();
	
	size_t nSize = getArgSize();
	
	bool bCase = false;
	if (nSize == 3) {
		ARG(pValue, 2);
		bCase = pValue->boolean();
	}
	
	ARG(pValueLhs, 0);
	ARG(pValueRhs, 1);
	
	wstring_ptr wstrLhs(pValueLhs->string());
	wstring_ptr wstrRhs(pValueRhs->string());
	
	size_t nLhsLen = wcslen(wstrLhs.get());
	size_t nRhsLen = wcslen(wstrRhs.get());
	
	bool bResult = false;
	if (nRhsLen == 0) {
		bResult = true;
	}
	else if (nLhsLen >= nRhsLen) {
		if (bBeginWith_) {
			bResult = _wcsnicmp(wstrLhs.get(), wstrRhs.get(), nRhsLen) == 0;
		}
		else {
			BMFindString<WSTRING> bmfs(wstrRhs.get(), wcslen(wstrRhs.get()),
				bCase ? 0 : BMFindString<WSTRING>::FLAG_IGNORECASE);
			bResult = bmfs.find(wstrLhs.get()) != 0;
		}
	}
	
	return MacroValueFactory::getFactory().newBoolean(bResult);
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

qm::MacroFunctionCopy::MacroFunctionCopy(bool bMove) :
	bMove_(bMove)
{
}

qm::MacroFunctionCopy::~MacroFunctionCopy()
{
}

MacroValuePtr qm::MacroFunctionCopy::value(MacroContext* pContext) const
{
	assert(pContext);
	
	LOG(Copy);
	
	if (!checkArgSize(pContext, 1))
		return MacroValuePtr();
	
	MessageHolderBase* pmh = pContext->getMessageHolder();
	if (!pmh)
		return error(*pContext, MacroErrorHandler::CODE_NOCONTEXTMESSAGE);
	assert(pmh->getMessageHolder());
	
	ARG(pValue, 0);
	wstring_ptr wstrFolder(pValue->string());
	
	Folder* pFolderTo = pContext->getDocument()->getFolder(
		pContext->getAccount(), wstrFolder.get());
	if (!pFolderTo || pFolderTo->getType() != Folder::TYPE_NORMAL)
		return error(*pContext, MacroErrorHandler::CODE_FAIL);
	
	Account* pAccount = pmh->getAccount();
	assert(pAccount->isLocked());
	
	MessageHolderList l(1, pmh->getMessageHolder());
	if (!pAccount->copyMessages(l, 0, static_cast<NormalFolder*>(pFolderTo), bMove_, 0))
		return error(*pContext, MacroErrorHandler::CODE_FAIL);
	
	return MacroValueFactory::getFactory().newBoolean(true);
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

qm::MacroFunctionDate::MacroFunctionDate()
{
}

qm::MacroFunctionDate::~MacroFunctionDate()
{
}

MacroValuePtr qm::MacroFunctionDate::value(MacroContext* pContext) const
{
	assert(pContext);
	
	LOG(Date);
	
	if (!checkArgSizeRange(pContext, 0, 1))
		return MacroValuePtr();
	
	size_t nSize = getArgSize();
	
	Time time;
	if (nSize == 1) {
		ARG(pValue, 0);
		wstring_ptr wstr(pValue->string());
		string_ptr str(wcs2mbs(wstr.get()));
		if (!DateParser::parse(str.get(), -1,
			Part::isGlobalOption(Part::O_ALLOW_SINGLE_DIGIT_TIME),
			Part::isGlobalOption(Part::O_ALLOW_DATE_WITH_RUBBISH), &time))
			time = Time::getCurrentTime();
	}
	else {
		time = Time::getCurrentTime();
	}
	
	return MacroValueFactory::getFactory().newTime(time);
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

qm::MacroFunctionDecode::MacroFunctionDecode()
{
}

qm::MacroFunctionDecode::~MacroFunctionDecode()
{
}

MacroValuePtr qm::MacroFunctionDecode::value(MacroContext* pContext) const
{
	assert(pContext);
	
	LOG(Decode);
	
	if (!checkArgSize(pContext, 1))
		return MacroValuePtr();
	
	ARG(pValue, 0);
	wstring_ptr wstr(pValue->string());
	
	if (FieldParserUtil<WSTRING>::isAscii(wstr.get())) {
		string_ptr str(wcs2mbs(wstr.get()));
		wstr = FieldParser::decode(str.get(), -1, false, 0);
	}
	
	return MacroValueFactory::getFactory().newString(wstr.get());
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

qm::MacroFunctionDefun::MacroFunctionDefun()
{
}

qm::MacroFunctionDefun::~MacroFunctionDefun()
{
}

MacroValuePtr qm::MacroFunctionDefun::value(MacroContext* pContext) const
{
	assert(pContext);
	
	LOG(Defun);
	
	if (!checkArgSize(pContext, 2))
		return MacroValuePtr();
	
	ARG(pValue, 0);
	wstring_ptr wstrName(pValue->string());
	
	bool bSet = pContext->setFunction(wstrName.get(), getArg(1));
	return MacroValueFactory::getFactory().newBoolean(bSet);
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

qm::MacroFunctionDelete::MacroFunctionDelete()
{
}

qm::MacroFunctionDelete::~MacroFunctionDelete()
{
}

MacroValuePtr qm::MacroFunctionDelete::value(MacroContext* pContext) const
{
	assert(pContext);
	
	LOG(Delete);
	
	if (!checkArgSize(pContext, 0))
		return MacroValuePtr();
	
	MessageHolderBase* pmh = pContext->getMessageHolder();
	if (!pmh)
		return error(*pContext, MacroErrorHandler::CODE_NOCONTEXTMESSAGE);
	assert(pmh->getMessageHolder());
	
	Account* pAccount = pmh->getAccount();
	assert(pAccount->isLocked());
	
	MessageHolderList l(1, pmh->getMessageHolder());
	if (!pAccount->removeMessages(l, 0, false, 0))
		return error(*pContext, MacroErrorHandler::CODE_FAIL);
	
	return MacroValueFactory::getFactory().newBoolean(true);
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

qm::MacroFunctionEqual::MacroFunctionEqual()
{
}

qm::MacroFunctionEqual::~MacroFunctionEqual()
{
}

MacroValuePtr qm::MacroFunctionEqual::value(MacroContext* pContext) const
{
	assert(pContext);
	
	LOG(Equal);
	
	if (!checkArgSizeRange(pContext, 2, 3))
		return MacroValuePtr();
	
	size_t nSize = getArgSize();
	
	bool bCase = false;
	if (nSize == 3) {
		ARG(pValueCase, 2);
		bCase = pValueCase->boolean();
	}
	
	ARG(pValueLhs, 0);
	ARG(pValueRhs, 1);
	
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
		wstring_ptr wstrLhs(pValueLhs->string());
		wstring_ptr wstrRhs(pValueRhs->string());
		if (bCase)
			bEqual = wcscmp(wstrLhs.get(), wstrRhs.get()) == 0;
		else
			bEqual = _wcsicmp(wstrLhs.get(), wstrRhs.get()) == 0;
	}
	
	return MacroValueFactory::getFactory().newBoolean(bEqual);
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

qm::MacroFunctionEval::MacroFunctionEval(MacroParser::Type type) :
	type_(type)
{
}

qm::MacroFunctionEval::~MacroFunctionEval()
{
}

MacroValuePtr qm::MacroFunctionEval::value(MacroContext* pContext) const
{
	assert(pContext);
	
	LOG(Eval);
	
	if (!checkArgSize(pContext, 1))
		return MacroValuePtr();
	
	ARG(pValue, 0);
	wstring_ptr wstrExpr(pValue->string());
	
	MacroParser parser(type_);
	
	std::auto_ptr<Macro> pMacro(parser.parse(wstrExpr.get()));
	if (!pMacro.get())
		return error(*pContext, MacroErrorHandler::CODE_FAIL);
	
	return pMacro->value(pContext);
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

qm::MacroFunctionExecute::MacroFunctionExecute()
{
}

qm::MacroFunctionExecute::~MacroFunctionExecute()
{
}

MacroValuePtr qm::MacroFunctionExecute::value(MacroContext* pContext) const
{
	assert(pContext);
	
	LOG(Execute);
	
	size_t nSize = getArgSize();
#ifdef _WIN32_WCE
	if (!checkArgSize(pContext, 1))
		return MacroValuePtr();
#else
	if (!checkArgSizeRange(pContext, 1, 2))
		return MacroValuePtr();
#endif
	
	ARG(pValueCommand, 0);
	wstring_ptr wstrCommand(pValueCommand->string());
	
	const WCHAR* pwszResult = L"";
	wstring_ptr wstrOutput;
	if (nSize > 1) {
#ifndef _WIN32_WCE
		ARG(pValueInput, 1);
		wstring_ptr wstrInput(pValueInput->string());
		
		wstrOutput = Process::exec(wstrCommand.get(), wstrInput.get());
		if (!wstrOutput.get())
			return error(*pContext, MacroErrorHandler::CODE_FAIL);
		pwszResult = wstrOutput.get();
#endif
	}
	else {
		const WCHAR* pCommand = wstrCommand.get();
		WCHAR* pParam = 0;
		if (*pCommand == L'\"') {
			++pCommand;
			
			pParam = wstrCommand.get() + 1;
			while (*pParam) {
				if (*pParam == L'\"') {
					if (*(pParam + 1) == L' ' || *(pParam + 1) == L'\0')
						break;
				}
				++pParam;
			}
			if (pParam) {
				*pParam = L'\0';
				++pParam;
			}
		}
		else {
			pParam = wstrCommand.get();
			while (*pParam && *pParam != L' ')
				++pParam;
			if (*pParam) {
				*pParam = L'\0';
				++pParam;
			}
		}
		while (*pParam == L' ')
			++pParam;
		
		W2T(pCommand, ptszCommand);
		W2T(pParam, ptszParam);
		
		SHELLEXECUTEINFO sei = {
			sizeof(sei),
			0,
			pContext->getWindow(),
			_T("open"),
			ptszCommand,
			ptszParam,
			0,
			SW_SHOW
		};
		if (!::ShellExecuteEx(&sei))
			return error(*pContext, MacroErrorHandler::CODE_FAIL);
	}
	
	return MacroValueFactory::getFactory().newString(pwszResult);
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

qm::MacroFunctionExist::MacroFunctionExist()
{
}

qm::MacroFunctionExist::~MacroFunctionExist()
{
}

MacroValuePtr qm::MacroFunctionExist::value(MacroContext* pContext) const
{
	assert(pContext);
	
	LOG(Exist);
	
	if (!checkArgSize(pContext, 1))
		return MacroValuePtr();
	
	MessageHolderBase* pmh = pContext->getMessageHolder();
	if (!pmh)
		return error(*pContext, MacroErrorHandler::CODE_NOCONTEXTMESSAGE);
	
	ARG(pValue, 0);
	
	wstring_ptr wstrName(pValue->string());
	
	Message* pMessage = getMessage(pContext,
		MacroContext::MESSAGETYPE_HEADER, wstrName.get());
	if (!pMessage)
		return error(*pContext, MacroErrorHandler::CODE_GETMESSAGE);
	
	bool bHas = pMessage->hasField(wstrName.get());
	return MacroValueFactory::getFactory().newBoolean(bHas);
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

qm::MacroFunctionExit::MacroFunctionExit()
{
}

qm::MacroFunctionExit::~MacroFunctionExit()
{
}

MacroValuePtr qm::MacroFunctionExit::value(MacroContext* pContext) const
{
	assert(pContext);
	
	LOG(Exit);
	
	if (!checkArgSizeRange(pContext, 2, 4))
		return MacroValuePtr();
	
	pContext->setReturnType(MacroContext::RETURNTYPE_EXIT);
	
	return MacroValuePtr();
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

qm::MacroFunctionField::MacroFunctionField()
{
}

qm::MacroFunctionField::~MacroFunctionField()
{
}

MacroValuePtr qm::MacroFunctionField::value(MacroContext* pContext) const
{
	assert(pContext);
	
	LOG(Field);
	
	if (!checkArgSizeRange(pContext, 1, 2))
		return MacroValuePtr();

	size_t nSize = getArgSize();
	
	MessageHolderBase* pmh = pContext->getMessageHolder();
	if (!pmh)
		return error(*pContext, MacroErrorHandler::CODE_NOCONTEXTMESSAGE);
	
	ARG(pValue, 0);
	wstring_ptr wstrName(pValue->string());
	
	const Part* pPart = 0;
	if (nSize > 1) {
		pPart = getPart(pContext, 1);
		if (!pPart)
			return MacroValuePtr();
	}
	else {
		Message* pMessage = getMessage(pContext,
			MacroContext::MESSAGETYPE_HEADER, wstrName.get());
		if (!pMessage)
			return error(*pContext, MacroErrorHandler::CODE_GETMESSAGE);
		pPart = pMessage;
	}
	
	string_ptr strHeader(PartUtil(*pPart).getHeader(wstrName.get()));
	return MacroValueFactory::getFactory().newField(
		wstrName.get(), strHeader.get());
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

qm::MacroFunctionFieldParameter::MacroFunctionFieldParameter()
{
}

qm::MacroFunctionFieldParameter::~MacroFunctionFieldParameter()
{
}

MacroValuePtr qm::MacroFunctionFieldParameter::value(MacroContext* pContext) const
{
	assert(pContext);
	
	LOG(FieldParameter);
	
	if (!checkArgSizeRange(pContext, 1, 3))
		return MacroValuePtr();

	size_t nSize = getArgSize();
	
	MessageHolderBase* pmh = pContext->getMessageHolder();
	if (!pmh)
		return error(*pContext, MacroErrorHandler::CODE_NOCONTEXTMESSAGE);
	
	ARG(pValue, 0);
	wstring_ptr wstrName(pValue->string());
	
	const Part* pPart = 0;
	if (nSize > 2) {
		pPart = getPart(pContext, 2);
		if (!pPart)
			return MacroValuePtr();
	}
	else {
		Message* pMessage = getMessage(pContext,
			MacroContext::MESSAGETYPE_HEADER, wstrName.get());
		if (!pMessage)
			return error(*pContext, MacroErrorHandler::CODE_GETMESSAGE);
		pPart = pMessage;
	}
	
	wstring_ptr wstrParamName;
	if (nSize > 1) {
		ARG(pValue, 1);
		wstrParamName = pValue->string();
	}
	
	const WCHAR* pwszValue = 0;
	wstring_ptr wstrValue;
	SimpleParameterParser parser;
	Part::Field field = pPart->getField(wstrName.get(), &parser);
	if (field == Part::FIELD_EXIST) {
		if (wstrParamName.get()) {
			wstrValue = parser.getParameter(wstrParamName.get());
			pwszValue = wstrValue.get();
		}
		else {
			pwszValue = parser.getValue();
		}
	}
	if (!pwszValue)
		pwszValue = L"";
	
	return MacroValueFactory::getFactory().newString(pwszValue);
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

qm::MacroFunctionFind::MacroFunctionFind()
{
}

qm::MacroFunctionFind::~MacroFunctionFind()
{
}

MacroValuePtr qm::MacroFunctionFind::value(MacroContext* pContext) const
{
	assert(pContext);
	
	LOG(Find);
	
	if (!checkArgSizeRange(pContext, 2, 4))
		return MacroValuePtr();
	
	size_t nSize = getArgSize();
	
	bool bCase = false;
	if (nSize > 3) {
		ARG(pValue, 3);
		bCase = pValue->boolean();
	}
	
	unsigned int nIndex = 0;
	if (nSize > 2) {
		ARG(pValue, 2);
		nIndex = pValue->number();
	}
	
	ARG(pValue, 0);
	wstring_ptr wstr(pValue->string());
	unsigned int nResult = -1;
	size_t nLen = wcslen(wstr.get());
	if (nIndex < nLen) {
		ARG(pValueSep, 1);
		wstring_ptr wstrSep(pValueSep->string());
		if (*wstrSep.get()) {
			const WCHAR* p = 0;
			if (*(wstrSep.get() + 1) == L'\0') {
				if (bCase) {
					p = wcschr(wstr.get() + nIndex, *wstrSep.get());
				}
				else {
					WCHAR c = ::towlower(*wstrSep.get());
					p = wstr.get() + nIndex;
					while (*p) {
						if (::tolower(*p) == c)
							break;
						++p;
					}
					if (!*p)
						p = 0;
				}
			}
			else {
				BMFindString<WSTRING> bmfs(wstrSep.get(), wcslen(wstrSep.get()),
					bCase ? 0 : BMFindString<WSTRING>::FLAG_IGNORECASE);
				p = bmfs.find(wstr.get() + nIndex);
			}
			nResult = p ? p - wstr.get() : -1;
		}
	}
	
	return MacroValueFactory::getFactory().newNumber(nResult);
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

qm::MacroFunctionFlag::MacroFunctionFlag() :
	flag_(static_cast<MessageHolder::Flag>(0)),
	bCustom_(true)
{
}

qm::MacroFunctionFlag::MacroFunctionFlag(MessageHolder::Flag flag) :
	flag_(flag),
	bCustom_(false)
{
}

qm::MacroFunctionFlag::~MacroFunctionFlag()
{
}

MacroValuePtr qm::MacroFunctionFlag::value(MacroContext* pContext) const
{
	assert(pContext);
	
	LOG(Flag);
	
	MessageHolderBase* pmh = pContext->getMessageHolder();
	if (!pmh)
		return error(*pContext, MacroErrorHandler::CODE_NOCONTEXTMESSAGE);
	
	size_t nSize = getArgSize();
	unsigned int nFlags = flag_;
	if (bCustom_) {
		if (nSize == 0)
			return error(*pContext, MacroErrorHandler::CODE_INVALIDARGSIZE);
		ARG(pValue, 0);
		nFlags = pValue->number();
	}
	bool bCanModify = (nFlags & MessageHolder::FLAG_USER_MASK) != 0;
	size_t nBase = bCustom_ ? 1 : 0;
	if (nSize != nBase && !(bCanModify && nSize == nBase + 1))
		return error(*pContext, MacroErrorHandler::CODE_INVALIDARGSIZE);
	
	if (nSize == nBase + 1) {
		if (!pmh->getMessageHolder())
			return error(*pContext, MacroErrorHandler::CODE_FAIL);
		
		ARG(pValue, nBase);
		
		Account* pAccount = pmh->getFolder()->getAccount();
		MessageHolderList l(1, pmh->getMessageHolder());
		if (!pAccount->setMessagesFlags(l,
			pValue->boolean() ? nFlags : 0, nFlags))
			return error(*pContext, MacroErrorHandler::CODE_FAIL);
	}
	
	return MacroValueFactory::getFactory().newBoolean(
		(pmh->getFlags() & nFlags) != 0);
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

qm::MacroFunctionFolder::MacroFunctionFolder()
{
}

qm::MacroFunctionFolder::~MacroFunctionFolder()
{
}

MacroValuePtr qm::MacroFunctionFolder::value(MacroContext* pContext) const
{
	assert(pContext);
	
	LOG(Folder);
	
	if (!checkArgSizeRange(pContext, 0, 1))
		return MacroValuePtr();
	
	size_t nSize = getArgSize();
	
	MessageHolderBase* pmh = pContext->getMessageHolder();
	if (!pmh)
		return error(*pContext, MacroErrorHandler::CODE_NOCONTEXTMESSAGE);
	
	bool bFull = true;
	if (nSize > 0) {
		ARG(pValue, 0);
		bFull = pValue->boolean();
	}
	
	NormalFolder* pFolder = pmh->getFolder();
	const WCHAR* pwszName = 0;
	wstring_ptr wstrName;
	if (bFull) {
		wstrName = pFolder->getFullName();
		pwszName = wstrName.get();
	}
	else {
		pwszName = pFolder->getName();
	}
	
	return MacroValueFactory::getFactory().newString(pwszName);
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

qm::MacroFunctionForEach::MacroFunctionForEach()
{
}

qm::MacroFunctionForEach::~MacroFunctionForEach()
{
}

MacroValuePtr qm::MacroFunctionForEach::value(MacroContext* pContext) const
{
	assert(pContext);
	
	LOG(ForEach);
	
	if (!checkArgSize(pContext, 2))
		return MacroValuePtr();
	
	ARG(pValueMessages, 0);
	if (pValueMessages->getType() != MacroValue::TYPE_MESSAGELIST)
		return error(*pContext, MacroErrorHandler::CODE_INVALIDARGTYPE);
	
	const MacroValueMessageList::MessageList& l =
		static_cast<MacroValueMessageList*>(pValueMessages.get())->getMessageList();
	for (MacroValueMessageList::MessageList::const_iterator it = l.begin(); it != l.end(); ++it) {
		MessagePtrLock mpl(*it);
		if (mpl) {
			Message msg;
			MacroContext context(mpl, &msg, pContext);
			MacroValuePtr pValue(getArg(1)->value(&context));
			if (!pValue.get())
				return MacroValuePtr();
		}
	}
	
	return MacroValueFactory::getFactory().newBoolean(true);
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

qm::MacroFunctionFormatAddress::MacroFunctionFormatAddress()
{
}

qm::MacroFunctionFormatAddress::~MacroFunctionFormatAddress()
{
}

MacroValuePtr qm::MacroFunctionFormatAddress::value(MacroContext* pContext) const
{
	assert(pContext);
	
	LOG(FormatAddress);
	
	if (!checkArgSizeRange(pContext, 1, 3))
		return MacroValuePtr();
	
	size_t nSize = getArgSize();
	
	enum Lookup {
		LOOKUP_NONE,
		LOOKUP_EMPTY,
		LOOKUP_FORCE
	} lookup = LOOKUP_NONE;
	if (nSize > 2) {
		ARG(pValue, 2);
		unsigned int n = pValue->number();
		if (n > 2)
			n = 0;
		lookup = static_cast<Lookup>(n);
	}
	
	enum Type {
		TYPE_ALL,
		TYPE_ADDRESS,
		TYPE_NAME,
		TYPE_VIEW
	} type = TYPE_ALL;
	if (nSize > 1) {
		ARG(pValue, 1);
		unsigned int n = pValue->number();
		if (n > 3)
			n = 0;
		type = static_cast<Type>(n);
	}
	
	ARG(pValue, 0);
	if (pValue->getType() != MacroValue::TYPE_FIELD)
		return error(*pContext, MacroErrorHandler::CODE_INVALIDARGTYPE);
	
	MacroValueField* pValueField = static_cast<MacroValueField*>(pValue.get());
	
	wstring_ptr wstrValue;
	
	const CHAR* pszField = pValueField->getField();
	if (pszField) {
		Part part;
		if (!part.create(0, pszField, -1))
			return error(*pContext, MacroErrorHandler::CODE_FAIL);
		AddressListParser address(0);
		Part::Field field = part.getField(pValueField->getName(), &address);
		if (field == Part::FIELD_EXIST) {
			if (lookup != LOOKUP_NONE) {
				AddressBook* pAddressBook = pContext->getDocument()->getAddressBook();
				replacePhrase(pAddressBook, &address, lookup == LOOKUP_FORCE);
			}
			switch (type) {
			case TYPE_ALL:
				wstrValue = address.getValue();
				break;
			case TYPE_ADDRESS:
				wstrValue = address.getAddresses();
				break;
			case TYPE_NAME:
				wstrValue = address.getNames();
				break;
			case TYPE_VIEW:
				wstrValue = address.getValue(false);
				break;
			default:
				assert(false);
				break;
			}
		}
	}
	
	const WCHAR* pwszValue = wstrValue.get() ? wstrValue.get() : L"";
	return MacroValueFactory::getFactory().newString(pwszValue);
}

const WCHAR* qm::MacroFunctionFormatAddress::getName() const
{
	return L"FormatAddress";
}

void qm::MacroFunctionFormatAddress::replacePhrase(AddressBook* pAddressBook,
												   AddressListParser* pAddressList,
												   bool bForce)
{
	assert(pAddressBook);
	assert(pAddressList);
	
	const AddressListParser::AddressList& l = pAddressList->getAddressList();
	for (AddressListParser::AddressList::const_iterator it = l.begin(); it != l.end(); ++it)
		replacePhrase(pAddressBook, *it, bForce);
}

void qm::MacroFunctionFormatAddress::replacePhrase(AddressBook* pAddressBook,
												   AddressParser* pAddress,
												   bool bForce)
{
	assert(pAddressBook);
	assert(pAddress);
	
	AddressListParser* pGroup = pAddress->getGroup();
	if (pGroup) {
		replacePhrase(pAddressBook, pGroup, bForce);
	}
	else {
		if (!pAddress->getPhrase() || bForce) {
			wstring_ptr wstrAddress(concat(
				pAddress->getMailbox(), L"@", pAddress->getHost()));
			
			const AddressBookEntry* pEntry = pAddressBook->getEntry(wstrAddress.get());
			if (pEntry)
				pAddress->setPhrase(pEntry->getName());
		}
	}
}


/****************************************************************************
 *
 * MacroFunctionFormatDate
 *
 */

qm::MacroFunctionFormatDate::MacroFunctionFormatDate()
{
}

qm::MacroFunctionFormatDate::~MacroFunctionFormatDate()
{
}

MacroValuePtr qm::MacroFunctionFormatDate::value(MacroContext* pContext) const
{
	assert(pContext);
	
	LOG(FormatDate);
	
	if (!checkArgSizeRange(pContext, 2, 3))
		return MacroValuePtr();

	size_t nSize = getArgSize();
	
	Time::Format format = Time::FORMAT_LOCAL;
	if (nSize == 3) {
		ARG(pValue, 2);
		unsigned int n = pValue->number();
		if (n > 2)
			return error(*pContext, MacroErrorHandler::CODE_INVALIDARGVALUE);
		format = static_cast<Time::Format>(n);
	}
	
	ARG(pValueDate, 0);
	if (pValueDate->getType() != MacroValue::TYPE_TIME)
		return error(*pContext, MacroErrorHandler::CODE_INVALIDARGTYPE);
	MacroValueTime* pTime = static_cast<MacroValueTime*>(pValueDate.get());
	
	ARG(pValueFormat, 1);
	wstring_ptr wstrFormat(pValueFormat->string());
	
	wstring_ptr wstrValue(pTime->getTime().format(wstrFormat.get(), format));
	return MacroValueFactory::getFactory().newString(wstrValue.get());
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

qm::MacroFunctionFunction::MacroFunctionFunction(const WCHAR* pwszName)
{
	wstrName_ = allocWString(pwszName);
}

qm::MacroFunctionFunction::~MacroFunctionFunction()
{
}

MacroValuePtr qm::MacroFunctionFunction::value(MacroContext* pContext) const
{
	assert(pContext);
	
	LOG(Function);
	
	const MacroExpr* pExpr = pContext->getFunction(wstrName_.get());
	if (!pExpr)
		return error(*pContext, MacroErrorHandler::CODE_UNKNOWNFUNCTION);
	
	struct Args
	{
		Args(MacroContext* pContext) : pContext_(pContext) {}
		~Args() { pContext_->popArgumentContext(); }
		MacroContext* pContext_;
	} args(pContext);
	
	pContext->pushArgumentContext();
	
	MacroValuePtr pValueName(MacroValueFactory::getFactory().newString(wstrName_.get()));
	pContext->addArgument(pValueName);
	
	for (size_t n = 0; n < getArgSize(); ++n) {
		ARG(pValue, n);
		pContext->addArgument(pValue);
	}
	
	return pExpr->value(pContext);
}

const WCHAR* qm::MacroFunctionFunction::getName() const
{
	return wstrName_.get();
}


/****************************************************************************
 *
 * MacroFunctionHeader
 *
 */

qm::MacroFunctionHeader::MacroFunctionHeader()
{
}

qm::MacroFunctionHeader::~MacroFunctionHeader()
{
}

MacroValuePtr qm::MacroFunctionHeader::value(MacroContext* pContext) const
{
	assert(pContext);
	
	LOG(Header);
	
	if (!checkArgSizeRange(pContext, 0, 2))
		return MacroValuePtr();
	
	size_t nSize = getArgSize();
	
	MessageHolderBase* pmh = pContext->getMessageHolder();
	if (!pmh)
		return error(*pContext, MacroErrorHandler::CODE_NOCONTEXTMESSAGE);
	
	Message* pMessage = getMessage(pContext,
		MacroContext::MESSAGETYPE_HEADER, 0);
	if (!pMessage)
		return error(*pContext, MacroErrorHandler::CODE_GETMESSAGE);
	
	const Part* pPart = pMessage;
	if (nSize > 1) {
		pPart = getPart(pContext, 1);
		if (!pPart)
			return MacroValuePtr();
	}
	
	wstring_ptr wstrRemoveField;
	if (nSize > 0) {
		ARG(pValue, 0);
		wstrRemoveField = pValue->string();
	}
	
	const CHAR* pszHeader = pPart->getHeader();
	Part partTemp;
	if (wstrRemoveField.get()) {
		if (!partTemp.create(0, pszHeader, -1))
			return error(*pContext, MacroErrorHandler::CODE_FAIL);
		
		const WCHAR* p = wstrRemoveField.get();
		const WCHAR* pEnd = wcschr(p, L',');
		while (true) {
			wstring_ptr wstrField(trim(
				p, pEnd ? pEnd - p : static_cast<size_t>(-1)));
			partTemp.removeField(wstrField.get(), 0xffffffff);
			if (!pEnd)
				break;
			p = pEnd + 1;
			pEnd = wcschr(p, L',');
		}
		
		pszHeader = partTemp.getHeader();
	}
	
	wxstring_ptr wstrHeader(PartUtil::a2w(pszHeader));
	if (!wstrHeader.get())
		return MacroValuePtr();
	return MacroValueFactory::getFactory().newString(wstrHeader.get());
}

const WCHAR* qm::MacroFunctionHeader::getName() const
{
	return L"Header";
}


/****************************************************************************
 *
 * MacroFunctionHtmlEscape
 *
 */

qm::MacroFunctionHtmlEscape::MacroFunctionHtmlEscape()
{
}

qm::MacroFunctionHtmlEscape::~MacroFunctionHtmlEscape()
{
}

MacroValuePtr qm::MacroFunctionHtmlEscape::value(MacroContext* pContext) const
{
	assert(pContext);
	
	LOG(HtmlEscape);
	
	if (!checkArgSize(pContext, 1))
		return MacroValuePtr();
	
	ARG(pValue, 0);
	wstring_ptr wstrValue(pValue->string());
	
	StringBuffer<WSTRING> buf;
	for (const WCHAR* p = wstrValue.get(); *p; ++p) {
		switch (*p) {
		case L'<':
			buf.append(L"&lt;");
			break;
		case L'>':
			buf.append(L"&gt;");
			break;
		case L'&':
			buf.append(L"&amp;");
			break;
		case L'\"':
			buf.append(L"&quot;");
			break;
		default:
			buf.append(*p);
			break;
		}
	}
	
	return MacroValueFactory::getFactory().newString(buf.getCharArray());
}

const WCHAR* qm::MacroFunctionHtmlEscape::getName() const
{
	return L"HtmlEscape";
}


/****************************************************************************
 *
 * MacroFunctionI
 *
 */

qm::MacroFunctionI::MacroFunctionI()
{
}

qm::MacroFunctionI::~MacroFunctionI()
{
}

MacroValuePtr qm::MacroFunctionI::value(MacroContext* pContext) const
{
	assert(pContext);
	
	LOG(I);
	
	if (!checkArgSizeRange(pContext, 0, 2))
		return MacroValuePtr();
	
	size_t nSize = getArgSize();
	
	wstring_ptr wstrSubAccount;
	if (nSize > 1) {
		ARG(pValue, 1);
		wstrSubAccount = pValue->string();
	}
	
	Account* pAccount = pContext->getAccount();
	if (nSize > 0) {
		ARG(pValue, 0);
		wstring_ptr wstrAccount(pValue->string());
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
		pSubAccount->getSenderAddress());
	Part part;
	if (!part.setField(L"I", address))
		return error(*pContext, MacroErrorHandler::CODE_FAIL);
	
	string_ptr strHeader(PartUtil(part).getHeader(L"I"));
	return MacroValueFactory::getFactory().newField(L"I", strHeader.get());
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

qm::MacroFunctionId::MacroFunctionId()
{
}

qm::MacroFunctionId::~MacroFunctionId()
{
}

MacroValuePtr qm::MacroFunctionId::value(MacroContext* pContext) const
{
	assert(pContext);
	
	LOG(Id);
	
	if (!checkArgSize(pContext, 0))
		return MacroValuePtr();
	
	MessageHolderBase* pmh = pContext->getMessageHolder();
	if (!pmh)
		return error(*pContext, MacroErrorHandler::CODE_NOCONTEXTMESSAGE);
	
	return MacroValueFactory::getFactory().newNumber(pmh->getId());
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

qm::MacroFunctionIdentity::MacroFunctionIdentity()
{
}

qm::MacroFunctionIdentity::~MacroFunctionIdentity()
{
}

MacroValuePtr qm::MacroFunctionIdentity::value(MacroContext* pContext) const
{
	assert(pContext);
	
	LOG(Identity);
	
	if (!checkArgSize(pContext, 0))
		return MacroValuePtr();
	
	Account* pAccount = pContext->getAccount();
	MessageHolderBase* pmh = pContext->getMessageHolder();
	if (pmh)
		pAccount = pmh->getFolder()->getAccount();
	const WCHAR* pwszIdentity = pAccount->getCurrentSubAccount()->getIdentity();
	if (!pwszIdentity)
		pwszIdentity = L"";
	
	return MacroValueFactory::getFactory().newString(pwszIdentity);
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

qm::MacroFunctionIf::MacroFunctionIf()
{
}

qm::MacroFunctionIf::~MacroFunctionIf()
{
}

MacroValuePtr qm::MacroFunctionIf::value(MacroContext* pContext) const
{
	assert(pContext);
	
	LOG(If);
	
	size_t nSize = getArgSize();
	if (nSize < 3 || nSize % 2 == 0)
		return error(*pContext, MacroErrorHandler::CODE_INVALIDARGSIZE);

	size_t n = 0;
	while (n < nSize - 1) {
		ARG(pValue, n);
		if (pValue->boolean())
			return getArg(n + 1)->value(pContext);
		n += 2;
	}
	return getArg(n)->value(pContext);
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

qm::MacroFunctionInclude::MacroFunctionInclude(MacroParser::Type type) :
	type_(type)
{
}

qm::MacroFunctionInclude::~MacroFunctionInclude()
{
}

MacroValuePtr qm::MacroFunctionInclude::value(MacroContext* pContext) const
{
	assert(pContext);
	
	LOG(Include);
	
	if (!checkArgSize(pContext, 1))
		return MacroValuePtr();
	
	ARG(pValuePath, 0);
	wstring_ptr wstrPath(pValuePath->string());
	
	wstring_ptr wstrAbsolutePath(pContext->resolvePath(wstrPath.get()));
	FileInputStream stream(wstrAbsolutePath.get());
	if (!stream)
		return error(*pContext, MacroErrorHandler::CODE_FAIL);
	InputStreamReader reader(&stream, false, 0);
	if (!reader)
		return error(*pContext, MacroErrorHandler::CODE_FAIL);
	BufferedReader bufferedReader(&reader, false);
	
	StringBuffer<WSTRING> buf;
	WCHAR wsz[1024];
	while (true) {
		size_t nRead = bufferedReader.read(wsz, countof(wsz));
		if (nRead == -1)
			return error(*pContext, MacroErrorHandler::CODE_FAIL);
		else if (nRead == 0)
			break;
		buf.append(wsz, nRead);
	}
	
	MacroParser parser(type_);
	std::auto_ptr<Macro> pMacro(parser.parse(buf.getCharArray()));
	if (!pMacro.get())
		return error(*pContext, MacroErrorHandler::CODE_FAIL);
	return pMacro->value(pContext);
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

qm::MacroFunctionInputBox::MacroFunctionInputBox()
{
}

qm::MacroFunctionInputBox::~MacroFunctionInputBox()
{
}

MacroValuePtr qm::MacroFunctionInputBox::value(MacroContext* pContext) const
{
	assert(pContext);
	
	LOG(InputBox);
	
	if (!checkArgSizeRange(pContext, 1, 3))
		return MacroValuePtr();
	
	size_t nSize = getArgSize();
	
	wstring_ptr wstrDefault;
	if (nSize > 2) {
		ARG(pValue, 2);
		wstrDefault = pValue->string();
	}
	
	bool bMultiline = false;
	if (nSize > 1) {
		ARG(pValue, 1);
		bMultiline = pValue->boolean();
	}
	
	ARG(pValue, 0);
	wstring_ptr wstrMessage(pValue->string());
	
	InputBoxDialog dialog(bMultiline, wstrMessage.get(), wstrDefault.get());
	if (dialog.doModal(pContext->getWindow()) != IDOK) {
		pContext->setReturnType(MacroContext::RETURNTYPE_CANCEL);
		return MacroValuePtr();
	}
	
	return MacroValueFactory::getFactory().newString(dialog.getValue());
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

qm::MacroFunctionLength::MacroFunctionLength()
{
}

qm::MacroFunctionLength::~MacroFunctionLength()
{
}

MacroValuePtr qm::MacroFunctionLength::value(MacroContext* pContext) const
{
	assert(pContext);
	
	LOG(Length);
	
	if (!checkArgSizeRange(pContext, 1, 2))
		return MacroValuePtr();
	
	size_t nSize = getArgSize();
	
	bool bByte = false;
	if (nSize > 1) {
		ARG(pValue, 1);
		bByte = pValue->boolean();
	}
	
	ARG(pValue, 0);
	wstring_ptr wstrValue(pValue->string());
	
	long nLen = 0;
	if (bByte) {
		string_ptr str(wcs2mbs(wstrValue.get()));
		nLen = strlen(str.get());
	}
	else {
		nLen = wcslen(wstrValue.get());
	}
	
	return MacroValueFactory::getFactory().newNumber(nLen);
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

qm::MacroFunctionLoad::MacroFunctionLoad()
{
}

qm::MacroFunctionLoad::~MacroFunctionLoad()
{
}

MacroValuePtr qm::MacroFunctionLoad::value(MacroContext* pContext) const
{
	assert(pContext);
	
	LOG(Load);
	
	if (!checkArgSizeRange(pContext, 1, 3))
		return MacroValuePtr();
	
	size_t nSize = getArgSize();
	
	ARG(pValuePath, 0);
	wstring_ptr wstrPath(pValuePath->string());
	
	bool bTemplate = false;
	if (nSize > 1) {
		ARG(pValueTemplate, 1);
		bTemplate = pValueTemplate->boolean();
	}
	
	wstring_ptr wstrEncoding;
	if (nSize > 2) {
		ARG(pValueEncoding, 2);
		wstrEncoding = pValueEncoding->string();
	}
	
	wstring_ptr wstrAbsolutePath(pContext->resolvePath(wstrPath.get()));
	FileInputStream stream(wstrAbsolutePath.get());
	if (!stream)
		return error(*pContext, MacroErrorHandler::CODE_FAIL);
	InputStreamReader reader(&stream, false, wstrEncoding.get());
	if (!reader)
		return error(*pContext, MacroErrorHandler::CODE_FAIL);
	BufferedReader bufferedReader(&reader, false);
	
	wstring_ptr wstr;
	if (bTemplate) {
		TemplateParser parser;
		std::auto_ptr<Template> pTemplate(parser.parse(&bufferedReader));
		if (!pTemplate.get())
			return error(*pContext, MacroErrorHandler::CODE_FAIL);
		
		TemplateContext context(pContext->getMessageHolder(),
			pContext->getMessage(), pContext->getSelectedMessageHolders(),
			pContext->getAccount(), pContext->getDocument(), pContext->getWindow(),
			pContext->getBodyCharset(), pContext->getSecurityMode(), pContext->getProfile(),
			pContext->getErrorHandler(), TemplateContext::ArgumentList());
		switch (pTemplate->getValue(context, &wstr)) {
		case Template::RESULT_SUCCESS:
			break;
		case Template::RESULT_ERROR:
			return error(*pContext, MacroErrorHandler::CODE_FAIL);
		case Template::RESULT_CANCEL:
			pContext->setReturnType(MacroContext::RETURNTYPE_CANCEL);
			return MacroValuePtr();
		default:
			assert(false);
			return error(*pContext, MacroErrorHandler::CODE_FAIL);
		}
	}
	else {
		StringBuffer<WSTRING> buf;
		WCHAR wsz[1024];
		while (true) {
			size_t nRead = bufferedReader.read(wsz, countof(wsz));
			if (nRead == -1)
				return error(*pContext, MacroErrorHandler::CODE_FAIL);
			else if (nRead == 0)
				break;
			buf.append(wsz, nRead);
		}
		wstr = buf.getString();
	}
	
	return MacroValueFactory::getFactory().newString(wstr.get());
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

qm::MacroFunctionMessageBox::MacroFunctionMessageBox()
{
}

qm::MacroFunctionMessageBox::~MacroFunctionMessageBox()
{
}

MacroValuePtr qm::MacroFunctionMessageBox::value(MacroContext* pContext) const
{
	assert(pContext);
	
	LOG(MessageBox);
	
	if (!checkArgSizeRange(pContext, 1, 2))
		return MacroValuePtr();
	
	size_t nSize = getArgSize();
	
	int nType = MB_OK | MB_ICONINFORMATION;
	if (nSize > 1) {
		ARG(pValue, 1);
		nType = pValue->number();
	}
	
	ARG(pValue, 0);
	wstring_ptr wstrMessage(pValue->string());
	
	HWND hwnd = Window::getActiveWindow();
	if (!hwnd)
		hwnd = pContext->getWindow();
	
	int nValue = messageBox(wstrMessage.get(), nType, hwnd, 0, 0);
	return MacroValueFactory::getFactory().newNumber(nValue);
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

qm::MacroFunctionMessages::MacroFunctionMessages()
{
}

qm::MacroFunctionMessages::~MacroFunctionMessages()
{
}

MacroValuePtr qm::MacroFunctionMessages::value(MacroContext* pContext) const
{
	assert(pContext);
	
	LOG(Messages);
	
	if (!checkArgSizeRange(pContext, 0, 2))
		return MacroValuePtr();
	
	size_t nSize = getArgSize();
	
	unsigned int nId = 0;
	if (nSize > 1) {
		ARG(pValueId, 1);
		nId = pValueId->number();
	}
	
	wstring_ptr wstrFolder;
	if (nSize > 0) {
		ARG(pValueFolder, 0);
		wstrFolder = pValueFolder->string();
	}
	
	typedef MacroValueMessageList::MessageList List;
	List l;
	if (wstrFolder.get()) {
		Folder* pFolder = pContext->getDocument()->getFolder(
			pContext->getAccount(), wstrFolder.get());
		if (pFolder->getType() != Folder::TYPE_NORMAL)
			return error(*pContext, MacroErrorHandler::CODE_FAIL);
		
		if (nSize > 1) {
			l.push_back(static_cast<NormalFolder*>(pFolder)->getMessageById(nId));
		}
		else {
			Lock<Account> lock(*pFolder->getAccount());
			l.resize(pFolder->getCount());
			for (unsigned int n = 0; n < pFolder->getCount(); ++n)
				l[n] = MessagePtr(pFolder->getMessage(n));
		}
	}
	else {
		// TODO
	}
	
	return MacroValueFactory::getFactory().newMessageList(l);
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

qm::MacroFunctionNot::MacroFunctionNot()
{
}

qm::MacroFunctionNot::~MacroFunctionNot()
{
}

MacroValuePtr qm::MacroFunctionNot::value(MacroContext* pContext) const
{
	assert(pContext);
	
	LOG(Not);
	
	if (!checkArgSize(pContext, 1))
		return MacroValuePtr();
	
	ARG(pValue, 0);
	return MacroValueFactory::getFactory().newBoolean(!pValue->boolean());
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

qm::MacroFunctionOr::MacroFunctionOr()
{
}

qm::MacroFunctionOr::~MacroFunctionOr()
{
}

MacroValuePtr qm::MacroFunctionOr::value(MacroContext* pContext) const
{
	assert(pContext);
	
	LOG(Or);
	
	if (!checkArgSizeMin(pContext, 1))
		return MacroValuePtr();

	size_t nSize = getArgSize();
	
	bool bValue = false;
	for (size_t n = 0; n < nSize && !bValue; ++n) {
		ARG(pValue, n);
		if (pValue->boolean())
			bValue = true;
	}
	return MacroValueFactory::getFactory().newBoolean(bValue);
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

qm::MacroFunctionOSVersion::MacroFunctionOSVersion()
{
}

qm::MacroFunctionOSVersion::~MacroFunctionOSVersion()
{
}

MacroValuePtr qm::MacroFunctionOSVersion::value(MacroContext* pContext) const
{
	assert(pContext);
	
	LOG(OSVersion);
	
	if (!checkArgSize(pContext, 0))
		return MacroValuePtr();
	
	wstring_ptr wstrOSVersion(Application::getApplication().getOSVersion());
	return MacroValueFactory::getFactory().newString(wstrOSVersion.get());
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

qm::MacroFunctionParseURL::MacroFunctionParseURL()
{
}

qm::MacroFunctionParseURL::~MacroFunctionParseURL()
{
}

MacroValuePtr qm::MacroFunctionParseURL::value(MacroContext* pContext) const
{
	assert(pContext);
	
	LOG(ParseURL);
	
	if (!checkArgSize(pContext, 1))
		return MacroValuePtr();
	
	ARG(pValue, 0);
	wstring_ptr wstrURL(pValue->string());
	
	StringBuffer<WSTRING> buf;
	if (wcslen(wstrURL.get()) >= 7 &&
		wcsncmp(wstrURL.get(), L"mailto:", 7) == 0) {
		const WCHAR* p = wstrURL.get() + 7;
		const WCHAR* pAddress = p;
		while (*p && *p != L'?')
			++p;
		if (p != pAddress) {
			wstring_ptr wstrTo(decode(pAddress, p - pAddress));
			if (!wstrTo.get())
				return error(*pContext, MacroErrorHandler::CODE_FAIL);
			buf.append(L"To: ");
			buf.append(wstrTo.get());
			buf.append(L"\n");
		}
		
		wstring_ptr wstrBody;
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
			wstring_ptr wstrName;
			do {
				++p;
				if (!wstrName.get() && *p == L'=') {
					wstrName = decode(pName, p - pName);
					if (!wstrName.get())
						return error(*pContext, MacroErrorHandler::CODE_FAIL);
					pValue = p + 1;
				}
				else if (*p == L'&' || *p == L'\0') {
					if (wstrName.get()) {
						int n = 0;
						while (n < countof(pwszFields)) {
							if (_wcsicmp(wstrName.get(), pwszFields[n]) == 0)
								break;
							++n;
						}
						if (n != countof(pwszFields)) {
							wstring_ptr wstrValue(decode(pValue, p - pValue));
							if (!wstrValue.get())
								return error(*pContext, MacroErrorHandler::CODE_FAIL);
							buf.append(wstrName.get());
							buf.append(L": ");
							buf.append(wstrValue.get());
							buf.append(L"\n");
						}
						else if (_wcsicmp(wstrName.get(), L"body") == 0) {
							wstrBody = decode(pValue, p - pValue);
							if (!wstrBody.get())
								return error(*pContext, MacroErrorHandler::CODE_FAIL);
						}
					}
					pName = p + 1;
					pValue = 0;
					wstrName.reset(0);
				}
			} while(*p);
		}
		
		buf.append(L"\n");
		if (wstrBody.get())
			buf.append(wstrBody.get());
	}
	else {
		buf.append(L"To: ");
		buf.append(wstrURL.get());
		buf.append(L"\n\n");
	}
	
	return MacroValueFactory::getFactory().newString(buf.getCharArray());
}

const WCHAR* qm::MacroFunctionParseURL::getName() const
{
	return L"ParseURL";
}

wstring_ptr qm::MacroFunctionParseURL::decode(const WCHAR* p,
													  size_t nLen)
{
	assert(p);
	
	UTF8Converter converter;
	
	StringBuffer<STRING> buf;
	for (; nLen > 0; --nLen, ++p) {
		if (*p == L'%' && nLen > 2 && isHex(*(p + 1)) && isHex(*(p + 2))) {
			WCHAR wsz[3] = { *(p + 1), *(p + 2), L'\0' };
			WCHAR* pEnd = 0;
			long n = wcstol(wsz, &pEnd, 16);
			if (n > 0 && n != 0x0d)
				buf.append(static_cast<CHAR>(n));
			p += 2;
			nLen -= 2;
		}
		else if (*p <= 0x7f) {
			buf.append(static_cast<CHAR>(*p));
		}
		else {
			size_t nLen = 1;
			xstring_size_ptr encoded(converter.encode(p, &nLen));
			if (!encoded.get())
				return 0;
			buf.append(encoded.get());
		}
	}
	
	size_t n = buf.getLength();
	wxstring_size_ptr decoded(converter.decode(buf.getCharArray(), &n));
	if (!decoded.get())
		return 0;
	return allocWString(decoded.get(), decoded.size());
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

qm::MacroFunctionPart::MacroFunctionPart()
{
}

qm::MacroFunctionPart::~MacroFunctionPart()
{
}

MacroValuePtr qm::MacroFunctionPart::value(MacroContext* pContext) const
{
	assert(pContext);
	
	LOG(Part);
	
	if (!checkArgSizeRange(pContext, 1, 2))
		return MacroValuePtr();
	
	size_t nSize = getArgSize();
	
	MessageHolderBase* pmh = pContext->getMessageHolder();
	if (!pmh)
		return error(*pContext, MacroErrorHandler::CODE_NOCONTEXTMESSAGE);
	
	Message* pMessage = getMessage(pContext, MacroContext::MESSAGETYPE_ALL, 0);
	if (!pMessage)
		return error(*pContext, MacroErrorHandler::CODE_GETMESSAGE);
	
	const Part* pPart = pMessage;
	if (nSize > 1) {
		pPart = getPart(pContext, 1);
		if (!pPart)
			return MacroValuePtr();
	}
	
	ARG(pValue, 0);
	
	unsigned int nPart = pValue->number();
	if (nPart < pPart->getPartCount())
		pPart = pPart->getPart(nPart);
	else
		pPart = 0;
	
	return MacroValueFactory::getFactory().newPart(pPart);
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

qm::MacroFunctionPassed::MacroFunctionPassed()
{
}

qm::MacroFunctionPassed::~MacroFunctionPassed()
{
}

MacroValuePtr qm::MacroFunctionPassed::value(MacroContext* pContext) const
{
	assert(pContext);
	
	LOG(Passed);
	
	if (!checkArgSize(pContext, 1))
		return MacroValuePtr();
	
	MessageHolderBase* pmh = pContext->getMessageHolder();
	if (!pmh)
		return error(*pContext, MacroErrorHandler::CODE_NOCONTEXTMESSAGE);
	
	ARG(pValue, 0);
	unsigned int nDay = pValue->number();
	
	Time time;
	pmh->getDate(&time);
	time.addDay(nDay);
	
	Time timeNow(Time::getCurrentTime());
	
	return MacroValueFactory::getFactory().newBoolean(time < timeNow);
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

qm::MacroFunctionProcessId::MacroFunctionProcessId()
{
}

qm::MacroFunctionProcessId::~MacroFunctionProcessId()
{
}

MacroValuePtr qm::MacroFunctionProcessId::value(MacroContext* pContext) const
{
	assert(pContext);
	
	LOG(ProcessId);
	
	if (!checkArgSize(pContext, 0))
		return MacroValuePtr();
	
	return MacroValueFactory::getFactory().newNumber(::GetCurrentProcessId());
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

qm::MacroFunctionProfile::MacroFunctionProfile()
{
}

qm::MacroFunctionProfile::~MacroFunctionProfile()
{
}

MacroValuePtr qm::MacroFunctionProfile::value(MacroContext* pContext) const
{
	assert(pContext);
	
	LOG(Profile);
	
	if (!checkArgSizeRange(pContext, 3, 4))
		return MacroValuePtr();
	
	size_t nSize = getArgSize();
	
	ARG(pValuePath, 0);
	wstring_ptr wstrPath(pValuePath->string());
	
	ARG(pValueSection, 1);
	wstring_ptr wstrSection(pValueSection->string());
	
	ARG(pValueKey, 2);
	wstring_ptr wstrKey(pValueKey->string());
	
	wstring_ptr wstrDefault;
	if (nSize > 3) {
		ARG(pValueDefault, 3);
		wstrDefault = pValueDefault->string();
	}
	
	wstring_ptr wstrValue;
	if (!*wstrPath.get()) {
		wstrValue = pContext->getProfile()->getString(wstrSection.get(),
			wstrKey.get(), wstrDefault.get());
	}
	else {
		wstring_ptr wstrAbsolutePath(pContext->resolvePath(wstrPath.get()));
		XMLProfile profile(wstrAbsolutePath.get());
		if (!profile.load())
			return error(*pContext, MacroErrorHandler::CODE_FAIL);
		wstrValue = profile.getString(wstrSection.get(),
			wstrKey.get(), wstrDefault.get());
	}
	
	return MacroValueFactory::getFactory().newString(wstrValue.get());
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

qm::MacroFunctionProfileName::MacroFunctionProfileName()
{
}

qm::MacroFunctionProfileName::~MacroFunctionProfileName()
{
}

MacroValuePtr qm::MacroFunctionProfileName::value(MacroContext* pContext) const
{
	assert(pContext);
	
	LOG(ProfileName);
	
	if (!checkArgSize(pContext, 0))
		return MacroValuePtr();
	
	return MacroValueFactory::getFactory().newString(
		Application::getApplication().getProfileName());
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

qm::MacroFunctionProgn::MacroFunctionProgn()
{
}

qm::MacroFunctionProgn::~MacroFunctionProgn()
{
}

MacroValuePtr qm::MacroFunctionProgn::value(MacroContext* pContext) const
{
	assert(pContext);
	
	LOG(Progn);
	
	if (!checkArgSizeMin(pContext, 1))
		return MacroValuePtr();
	
	size_t nSize = getArgSize();
	
	for (size_t n = 0; n < nSize; ++n) {
		ARG(pValue, n);
		if (n == nSize - 1)
			return pValue;
	}
	
	return MacroValuePtr();
}

const WCHAR* qm::MacroFunctionProgn::getName() const
{
	return L"Progn";
}


/****************************************************************************
 *
 * MacroFunctionQuote
 *
 */

qm::MacroFunctionQuote::MacroFunctionQuote()
{
}

qm::MacroFunctionQuote::~MacroFunctionQuote()
{
}

MacroValuePtr qm::MacroFunctionQuote::value(MacroContext* pContext) const
{
	assert(pContext);
	
	LOG(Quote);
	
	if (!checkArgSize(pContext, 2))
		return MacroValuePtr();
	
	ARG(pValueText, 0);
	wstring_ptr wstrText(pValueText->string());
	
	ARG(pValueQuote, 1);
	wstring_ptr wstrQuote(pValueQuote->string());
	
	wxstring_ptr str(PartUtil::quote(wstrText.get(), wstrQuote.get()));
	if (!str.get())
		return error(*pContext, MacroErrorHandler::CODE_FAIL);
	
	return MacroValueFactory::getFactory().newString(str.get());
}

const WCHAR* qm::MacroFunctionQuote::getName() const
{
	return L"Quote";
}


/****************************************************************************
 *
 * MacroFunctionReferences
 *
 */

qm::MacroFunctionReferences::MacroFunctionReferences()
{
}

qm::MacroFunctionReferences::~MacroFunctionReferences()
{
}

MacroValuePtr qm::MacroFunctionReferences::value(MacroContext* pContext) const
{
	assert(pContext);
	
	LOG(References);
	
	if (!checkArgSizeRange(pContext, 0, 1))
		return MacroValuePtr();
	
	size_t nSize = getArgSize();
	
	MessageHolderBase* pmh = pContext->getMessageHolder();
	if (!pmh)
		return error(*pContext, MacroErrorHandler::CODE_NOCONTEXTMESSAGE);
	
	Message* pMessage = getMessage(pContext,
		MacroContext::MESSAGETYPE_HEADER, L"References");
	if (!pMessage)
		return error(*pContext, MacroErrorHandler::CODE_GETMESSAGE);
	
	unsigned int nReferences = -1;
	if (nSize > 0) {
		ARG(pValue, 0);
		nReferences = pValue->number();
	}
	
	PartUtil::ReferenceList l;
	StringListFree<PartUtil::ReferenceList> free(l);
	PartUtil(*pMessage).getReferences(&l);
	
	PartUtil::ReferenceList::size_type n = 0;
	if (nReferences != -1 && l.size() > nReferences)
		n = l.size() - nReferences;
	else
		n = 0;
	
	StringBuffer<WSTRING> buf;
	
	while (n < l.size()) {
		if (buf.getLength() != 0)
			buf.append(L" ");
		buf.append(L"<");
		buf.append(l[n]);
		buf.append(L">");
		++n;
	}
	
	return MacroValueFactory::getFactory().newString(buf.getCharArray());
}

const WCHAR* qm::MacroFunctionReferences::getName() const
{
	return L"References";
}


/****************************************************************************
 *
 * MacroFunctionRegexFind
 *
 */

qm::MacroFunctionRegexFind::MacroFunctionRegexFind()
{
}

qm::MacroFunctionRegexFind::~MacroFunctionRegexFind()
{
}

MacroValuePtr qm::MacroFunctionRegexFind::value(MacroContext* pContext) const
{
	assert(pContext);
	
	LOG(RegexFind);
	
	if (!checkArgSizeRange(pContext, 2, 3))
		return MacroValuePtr();
	
	size_t nSize = getArgSize();
	
	ARG(pValue, 0);
	wstring_ptr wstrValue(pValue->string());
	
	const RegexPattern* pPattern = 0;
	std::auto_ptr<RegexPattern> p;
	ARG(pValuePattern, 1);
	if (pValuePattern->getType() == MacroValue::TYPE_REGEX) {
		pPattern = static_cast<MacroValueRegex*>(pValuePattern.get())->getPattern();
	}
	else {
		wstring_ptr wstrPattern(pValuePattern->string());
		p = RegexCompiler().compile(wstrPattern.get());
		if (!p.get())
			return error(*pContext, MacroErrorHandler::CODE_FAIL);
		pPattern = p.get();
	}
	
	unsigned int nIndex = 0;
	if (nSize > 2) {
		ARG(pValue, 2);
		nIndex = pValue->number();
	}
	
	const WCHAR* pStart = 0;
	const WCHAR* pEnd = 0;
	RegexRangeList listRange;
	pPattern->search(wstrValue.get(), -1,
		wstrValue.get() + nIndex, false, &pStart, &pEnd, &listRange);
	if (pStart) {
		if (!pContext->setRegexResult(listRange))
			return MacroValuePtr();
	}
	else {
		pContext->clearRegexResult();
	}
	
	return MacroValueFactory::getFactory().newNumber(
		pStart ? pStart - wstrValue.get() : -1);
}

const WCHAR* qm::MacroFunctionRegexFind::getName() const
{
	return L"RegexFind";
}


/****************************************************************************
 *
 * MacroFunctionRegexMatch
 *
 */

qm::MacroFunctionRegexMatch::MacroFunctionRegexMatch()
{
}

qm::MacroFunctionRegexMatch::~MacroFunctionRegexMatch()
{
}

MacroValuePtr qm::MacroFunctionRegexMatch::value(MacroContext* pContext) const
{
	assert(pContext);
	
	LOG(RegexMatch);
	
	if (!checkArgSize(pContext, 2))
		return MacroValuePtr();
	
	ARG(pValue, 0);
	wstring_ptr wstrValue(pValue->string());
	
	const RegexPattern* pPattern = 0;
	std::auto_ptr<RegexPattern> p;
	ARG(pValuePattern, 1);
	if (pValuePattern->getType() == MacroValue::TYPE_REGEX) {
		pPattern = static_cast<MacroValueRegex*>(pValuePattern.get())->getPattern();
	}
	else {
		wstring_ptr wstrPattern(pValuePattern->string());
		p = RegexCompiler().compile(wstrPattern.get());
		if (!p.get())
			return error(*pContext, MacroErrorHandler::CODE_FAIL);
		pPattern = p.get();
	}
	
	const WCHAR* pStart = 0;
	const WCHAR* pEnd = 0;
	RegexRangeList listRange;
	pPattern->search(wstrValue.get(), -1,
		wstrValue.get(), false, &pStart, &pEnd, &listRange);
	if (pStart) {
		if (!pContext->setRegexResult(listRange))
			return MacroValuePtr();
	}
	else {
		pContext->clearRegexResult();
	}
	
	return MacroValueFactory::getFactory().newBoolean(pStart != 0);
}

const WCHAR* qm::MacroFunctionRegexMatch::getName() const
{
	return L"RegexMatch";
}


/****************************************************************************
 *
 * MacroFunctionRegexReplace
 *
 */

qm::MacroFunctionRegexReplace::MacroFunctionRegexReplace()
{
}

qm::MacroFunctionRegexReplace::~MacroFunctionRegexReplace()
{
}

MacroValuePtr qm::MacroFunctionRegexReplace::value(MacroContext* pContext) const
{
	assert(pContext);
	
	LOG(RegexReplace);
	
	if (!checkArgSizeRange(pContext, 3, 4))
		return MacroValuePtr();
	
	size_t nSize = getArgSize();
	
	ARG(pValue, 0);
	wstring_ptr wstrValue(pValue->string());
	
	const RegexPattern* pPattern = 0;
	std::auto_ptr<RegexPattern> p;
	ARG(pValuePattern, 1);
	if (pValuePattern->getType() == MacroValue::TYPE_REGEX) {
		pPattern = static_cast<MacroValueRegex*>(pValuePattern.get())->getPattern();
	}
	else {
		wstring_ptr wstrPattern(pValuePattern->string());
		p = RegexCompiler().compile(wstrPattern.get());
		if (!p.get())
			return error(*pContext, MacroErrorHandler::CODE_FAIL);
		pPattern = p.get();
	}
	
	ARG(pValueReplace, 2);
	wstring_ptr wstrReplace(pValueReplace->string());
	
	bool bGlobal = false;
	if (nSize > 3) {
		ARG(pValueGlobal, 3);
		bGlobal = pValueGlobal->boolean();
	}
	
	StringBuffer<WSTRING> buf;
	
	pContext->clearRegexResult();
	
	const WCHAR* pStart = wstrValue.get();
	const WCHAR* pEnd = pStart + wcslen(pStart);
	while (pStart < pEnd) {
		const WCHAR* pMatchStart = 0;
		const WCHAR* pMatchEnd = 0;
		RegexRangeList listRange;
		pPattern->search(pStart, pEnd - pStart, pStart,
			false, &pMatchStart, &pMatchEnd, &listRange);
		
		if (pMatchStart) {
			buf.append(pStart, pMatchStart - pStart);
			listRange.getReplace(wstrReplace.get(), &buf);
			if (!bGlobal) {
				buf.append(pMatchEnd);
				pStart = pEnd;
			}
			else {
				pStart = pMatchEnd;
			}
			pContext->setRegexResult(listRange);
		}
		else {
			buf.append(pStart);
			pStart = pEnd;
		}
	}
	
	return MacroValueFactory::getFactory().newString(buf.getCharArray());
}

const WCHAR* qm::MacroFunctionRegexReplace::getName() const
{
	return L"RegexReplace";
}


/****************************************************************************
 *
 * MacroFunctionRelative
 *
 */

qm::MacroFunctionRelative::MacroFunctionRelative(bool bLess) :
	bLess_(bLess)
{
}

qm::MacroFunctionRelative::~MacroFunctionRelative()
{
}

MacroValuePtr qm::MacroFunctionRelative::value(MacroContext* pContext) const
{
	assert(pContext);
	
	LOG(Relative);
	
	if (!checkArgSizeRange(pContext, 2, 3))
		return MacroValuePtr();
	
	size_t nSize = getArgSize();
	
	bool bCase = false;
	if (nSize > 2) {
		ARG(pValueCase, 2);
		bCase = pValueCase->boolean();
	}
	
	ARG(pValueLhs, 0);
	ARG(pValueRhs, 1);
	
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
		wstring_ptr wstrLhs(pValueLhs->string());
		wstring_ptr wstrRhs(pValueRhs->string());
		
		int (*pfn)(const WCHAR*, const WCHAR*) = bCase ? &wcscmp : &_wcsicmp;
		int nComp = (*pfn)(wstrLhs.get(), wstrRhs.get());
	}
	
	return MacroValueFactory::getFactory().newBoolean(
		bLess_ ? nComp < 0 : nComp > 0);
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

qm::MacroFunctionRemove::MacroFunctionRemove()
{
}

qm::MacroFunctionRemove::~MacroFunctionRemove()
{
}

MacroValuePtr qm::MacroFunctionRemove::value(MacroContext* pContext) const
{
	assert(pContext);
	
	LOG(Remove);
	
	if (!checkArgSizeMin(pContext, 2))
		return MacroValuePtr();
	
	size_t nSize = getArgSize();
	
	ARG(pValue, 0);
	wstring_ptr wstrValue(pValue->string());
	
	Part part;
	if (!MessageCreator::setField(&part, L"Dummy", wstrValue.get(),
		MessageCreator::FIELDTYPE_ADDRESSLIST))
		return error(*pContext, MacroErrorHandler::CODE_FAIL);
	AddressListParser addressList(0);
	Part::Field f = part.getField(L"Dummy", &addressList);
	if (f != Part::FIELD_EXIST) {
		return pValue;
	}
	else {
		for (size_t n = 1; n < nSize; ++n) {
			ARG(pValue, n);
			if (pValue->getType() == MacroValue::TYPE_ADDRESS) {
				const MacroValueAddress::AddressList& l =
					static_cast<MacroValueAddress*>(pValue.get())->getAddress();
				for (MacroValueAddress::AddressList::const_iterator it = l.begin(); it != l.end(); ++it)
					remove(&addressList, *it);
			}
			else {
				wstring_ptr wstr(pValue->string());
				remove(&addressList, wstr.get());
			}
		}
		
		wstring_ptr wstrValue(addressList.getValue());
		return MacroValueFactory::getFactory().newString(wstrValue.get());
	}
}

const WCHAR* qm::MacroFunctionRemove::getName() const
{
	return L"Remove";
}

void qm::MacroFunctionRemove::remove(AddressListParser* pAddressList,
									 const WCHAR* pwszAddress)
{
	const AddressListParser::AddressList& l = pAddressList->getAddressList();
	for (AddressListParser::AddressList::size_type n = 0; n < l.size(); ) {
		AddressParser* pAddress = l[n];
		wstring_ptr wstrAddress(pAddress->getAddress());
		if (wcscmp(wstrAddress.get(), pwszAddress) == 0) {
			pAddressList->removeAddress(pAddress);
			delete pAddress;
		}
		else {
			++n;
		}
	}
}


/****************************************************************************
 *
 * MacroFunctionSave
 *
 */

qm::MacroFunctionSave::MacroFunctionSave()
{
}

qm::MacroFunctionSave::~MacroFunctionSave()
{
}

MacroValuePtr qm::MacroFunctionSave::value(MacroContext* pContext) const
{
	assert(pContext);
	
	LOG(Save);
	
	if (!checkArgSizeRange(pContext, 2, 3))
		return MacroValuePtr();
	
	size_t nSize = getArgSize();
	
	ARG(pValuePath, 0);
	wstring_ptr wstrPath(pValuePath->string());
	
	ARG(pValueContent, 1);
	wstring_ptr wstrContent(pValueContent->string());
	
	wstring_ptr wstrEncoding;
	if (nSize > 2) {
		ARG(pValueEncoding, 2);
		wstrEncoding = pValueEncoding->string();
	}
	
	wstring_ptr wstrAbsolutePath(pContext->resolvePath(wstrPath.get()));
	FileOutputStream stream(wstrAbsolutePath.get());
	if (!stream)
		return error(*pContext, MacroErrorHandler::CODE_FAIL);
	OutputStreamWriter writer(&stream, false, wstrEncoding.get());
	if (!writer)
		return error(*pContext, MacroErrorHandler::CODE_FAIL);
	BufferedWriter bufferedWriter(&writer, false);
	if (bufferedWriter.write(wstrContent.get(), wcslen(wstrContent.get())) == -1)
		return error(*pContext, MacroErrorHandler::CODE_FAIL);
	if (!bufferedWriter.close())
		return error(*pContext, MacroErrorHandler::CODE_FAIL);
	
	return MacroValueFactory::getFactory().newBoolean(true);
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

qm::MacroFunctionScript::MacroFunctionScript()
{
}

qm::MacroFunctionScript::~MacroFunctionScript()
{
}

MacroValuePtr MacroFunctionScript::value(MacroContext* pContext) const
{
	assert(pContext);
	
	LOG(Script);
	
	if (!checkArgSizeMin(pContext, 3))
		return MacroValuePtr();
	
	size_t nSize = getArgSize();
	
	ARG(pValueScript, 0);
	wstring_ptr wstrScript(pValueScript->string());
	
	ARG(pValueLanguage, 1);
	wstring_ptr wstrLanguage(pValueLanguage->string());
	
	ScriptManager* pScriptManager = pContext->getDocument()->getScriptManager();
	std::auto_ptr<Script> pScript(pScriptManager->createScript(wstrScript.get(),
		wstrLanguage.get(), pContext->getDocument(), pContext->getProfile(),
		pContext->getWindow(), getModalHandler()));
	if (!pScript.get())
		return error(*pContext, MacroErrorHandler::CODE_FAIL);
	
	typedef std::vector<VARIANT> ArgumentList;
	ArgumentList listArgs;
	struct Deleter
	{
		typedef std::vector<VARIANT> ArgumentList;
		Deleter(ArgumentList& l) : l_(l) {}
		~Deleter()
		{
			for (ArgumentList::iterator it = l_.begin(); it != l_.end(); ++it)
				::VariantClear(&(*it));
		}
		ArgumentList& l_;
	} deleter(listArgs);
	listArgs.resize(nSize - 2);
	Variant v;
	std::fill(listArgs.begin(), listArgs.end(), v);
	for (size_t n = 0; n < listArgs.size(); ++n) {
		ARG(pValue, n + 2);
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
				wstring_ptr wstrValue(pValue->string());
				listArgs[n].vt = VT_BSTR;
				listArgs[n].bstrVal = ::SysAllocString(wstrValue.get());
				if (!listArgs[n].bstrVal)
					return error(*pContext, MacroErrorHandler::CODE_FAIL);
			}
			break;
		}
	}
	
	Variant varResult;
	if (!pScript->run(&listArgs[0], listArgs.size(), &varResult))
		return error(*pContext, MacroErrorHandler::CODE_FAIL);
	
	Variant var;
	HRESULT hr = ::VariantChangeType(&var, &varResult, 0, VT_BSTR);
	if (FAILED(hr))
		return error(*pContext, MacroErrorHandler::CODE_FAIL);
	
	return MacroValueFactory::getFactory().newString(var.bstrVal);
}

const WCHAR* MacroFunctionScript::getName() const
{
	return L"Script";
}


/****************************************************************************
 *
 * MacroFunctionSelected
 *
 */

qm::MacroFunctionSelected::MacroFunctionSelected()
{
}

qm::MacroFunctionSelected::~MacroFunctionSelected()
{
}

MacroValuePtr qm::MacroFunctionSelected::value(MacroContext* pContext) const
{
	assert(pContext);
	
	LOG(Selected);
	
	if (!checkArgSize(pContext, 0))
		return MacroValuePtr();
	
	MacroValueMessageList::MessageList l;
	const MessageHolderList& listSelected = pContext->getSelectedMessageHolders();
	l.reserve(listSelected.size());
	for (MessageHolderList::const_iterator it = listSelected.begin(); it != listSelected.end(); ++it)
		l.push_back(MessagePtr(*it));
	
	return MacroValueFactory::getFactory().newMessageList(l);
}

const WCHAR* qm::MacroFunctionSelected::getName() const
{
	return L"Selected";
}


/****************************************************************************
 *
 * MacroFunctionSet
 *
 */

qm::MacroFunctionSet::MacroFunctionSet()
{
}

qm::MacroFunctionSet::~MacroFunctionSet()
{
}

MacroValuePtr qm::MacroFunctionSet::value(MacroContext* pContext) const
{
	assert(pContext);
	
	LOG(Set);
	
	if (!checkArgSizeRange(pContext, 2, 3))
		return MacroValuePtr();
	
	size_t nSize = getArgSize();
	
	bool bGlobal = false;
	if (nSize > 2) {
		ARG(pValue, 2);
		bGlobal = pValue->boolean();
	}
	
	ARG(pValueName, 0);
	wstring_ptr wstrName(pValueName->string());
	ARG(pValue, 1);
	
	if (!pContext->setVariable(wstrName.get(), pValue.get(), bGlobal))
		return error(*pContext, MacroErrorHandler::CODE_FAIL);
	
	return pValue;
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

qm::MacroFunctionSize::MacroFunctionSize()
{
}

qm::MacroFunctionSize::~MacroFunctionSize()
{
}

MacroValuePtr qm::MacroFunctionSize::value(MacroContext* pContext) const
{
	assert(pContext);
	
	LOG(Size);
	
	if (!checkArgSizeRange(pContext, 0, 1))
		return MacroValuePtr();
	
	size_t nArgSize = getArgSize();
	
	MessageHolderBase* pmh = pContext->getMessageHolder();
	if (!pmh)
		return error(*pContext, MacroErrorHandler::CODE_NOCONTEXTMESSAGE);
	
	bool bTextOnly = false;
	if (nArgSize == 1) {
		ARG(pValue, 0);
		bTextOnly = pValue->boolean();
	}
	
	unsigned int nSize = bTextOnly ? pmh->getTextSize() : pmh->getSize();
	return MacroValueFactory::getFactory().newNumber(nSize);
}

const WCHAR* qm::MacroFunctionSize::getName() const
{
	return L"Size";
}


/****************************************************************************
 *
 * MacroFunctionSpecialFolder
 *
 */

qm::MacroFunctionSpecialFolder::MacroFunctionSpecialFolder()
{
}

qm::MacroFunctionSpecialFolder::~MacroFunctionSpecialFolder()
{
}

MacroValuePtr qm::MacroFunctionSpecialFolder::value(MacroContext* pContext) const
{
	assert(pContext);
	
	LOG(SpecialFolder);
	
	if (!checkArgSizeRange(pContext, 1, 2))
		return MacroValuePtr();
	
	size_t nSize = getArgSize();
	
	Account* pAccount = pContext->getAccount();
	if (nSize > 1) {
		ARG(pValue, 1);
		wstring_ptr wstrAccount(pValue->string());
		pAccount = pContext->getDocument()->getAccount(wstrAccount.get());
		if (!pAccount)
			return error(*pContext, MacroErrorHandler::CODE_UNKNOWNACCOUNT);
	}
	assert(pAccount);
	
	ARG(pValue, 0);
	wstring_ptr wstrName(pValue->string());
	
	struct {
		const WCHAR* pwszName_;
		Folder::Flag flag_;
	} flags[] = {
		{ L"Inbox",		Folder::FLAG_INBOX		},
		{ L"Outbox",	Folder::FLAG_OUTBOX		},
		{ L"Sentbox",	Folder::FLAG_SENTBOX	},
		{ L"Trashbox",	Folder::FLAG_TRASHBOX	},
		{ L"Draftbox",	Folder::FLAG_DRAFTBOX	},
		{ L"Searchbox",	Folder::FLAG_SEARCHBOX	}
	};
	
	Folder::Flag flag = static_cast<Folder::Flag>(0);
	for (int n = 0; n < countof(flags) && flag == 0; ++n) {
		if (wcscmp(wstrName.get(), flags[n].pwszName_) == 0)
			flag = flags[n].flag_;
	}
	if (flag == 0)
		return error(*pContext, MacroErrorHandler::CODE_FAIL);
	
	const WCHAR* pwszFolderName = L"";
	wstring_ptr wstrFolderName;
	Folder* pFolder = pAccount->getFolderByFlag(flag);
	if (pFolder) {
		wstrFolderName = pFolder->getFullName();
		pwszFolderName = wstrFolderName.get();
	}
	
	return MacroValueFactory::getFactory().newString(pwszFolderName);
}

const WCHAR* qm::MacroFunctionSpecialFolder::getName() const
{
	return L"SpecialFolder";
}


/****************************************************************************
 *
 * MacroFunctionSubAccount
 *
 */

qm::MacroFunctionSubAccount::MacroFunctionSubAccount()
{
}

qm::MacroFunctionSubAccount::~MacroFunctionSubAccount()
{
}

MacroValuePtr qm::MacroFunctionSubAccount::value(MacroContext* pContext) const
{
	assert(pContext);
	
	LOG(SubAccount);
	
	if (!checkArgSize(pContext, 0))
		return MacroValuePtr();
	
	Account* pAccount = pContext->getAccount();
	MessageHolderBase* pmh = pContext->getMessageHolder();
	if (pmh)
		pAccount = pmh->getFolder()->getAccount();
	
	return MacroValueFactory::getFactory().newString(
		pAccount->getCurrentSubAccount()->getName());
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

qm::MacroFunctionSubject::MacroFunctionSubject()
{
}

qm::MacroFunctionSubject::~MacroFunctionSubject()
{
}

MacroValuePtr qm::MacroFunctionSubject::value(MacroContext* pContext) const
{
	assert(pContext);
	
	LOG(Subject);
	
	if (!checkArgSizeRange(pContext, 0, 2))
		return MacroValuePtr();
	
	size_t nSize = getArgSize();
	
	MessageHolderBase* pmh = pContext->getMessageHolder();
	if (!pmh)
		return error(*pContext, MacroErrorHandler::CODE_NOCONTEXTMESSAGE);
	
	Message* pMessage = getMessage(pContext,
		MacroContext::MESSAGETYPE_HEADER, L"Subject");
	if (!pMessage)
		return error(*pContext, MacroErrorHandler::CODE_GETMESSAGE);
	
	bool bRemoveRe = false;
	bool bRemoveMl = false;
	if (nSize > 1) {
		ARG(pValue, 1);
		bRemoveMl = pValue->boolean();
	}
	if (nSize > 0) {
		ARG(pValue, 0);
		bRemoveRe = pValue->boolean();
	}
	
	UnstructuredParser subject;
	Part::Field field = pMessage->getField(L"Subject", &subject);
	wstring_ptr wstrSubject;
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
					const WCHAR* p = pwszSubject + 2;
					while (*p) {
						if (bFind && *p != L' ')
							break;
						else if (*p == cEnd)
							bFind = true;
						++p;
					}
					if (bFind) {
						wstrSubject = allocWString(p);
						pwszSubject = wstrSubject.get();
					}
					bRemoveMl = false;
				}
				else if (bRemoveRe && wcslen(pwszSubject) > 2 &&
					_wcsnicmp(pwszSubject, L"re", 2) == 0 &&
					wcschr(pwszSep, pwszSubject[2])) {
					bool bFind = false;
					const WCHAR* p = pwszSubject + 2;
					while (*p) {
						if (bFind && *p != L' ')
							break;
						else if (*p == L' ' || *p == L':')
							bFind = true;
						++p;
					}
					if (bFind) {
						wstrSubject = allocWString(p);
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
	
	return MacroValueFactory::getFactory().newString(pwszSubject);
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

qm::MacroFunctionSubstring::MacroFunctionSubstring()
{
}

qm::MacroFunctionSubstring::~MacroFunctionSubstring()
{
}

MacroValuePtr qm::MacroFunctionSubstring::value(MacroContext* pContext) const
{
	assert(pContext);
	
	LOG(Substring);
	
	if (!checkArgSizeRange(pContext, 2, 3))
		return MacroValuePtr();
	
	size_t nSize = getArgSize();
	
	size_t nLength = static_cast<size_t>(-1);
	if (nSize == 3) {
		ARG(pValue, 2);
		nLength = pValue->number();
	}
	
	ARG(pValue, 0);
	wstring_ptr wstr(pValue->string());
	size_t nLen = wcslen(wstr.get());
	
	ARG(pValueBegin, 1);
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
	
	return MacroValueFactory::getFactory().newString(pwsz, nLength);
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

qm::MacroFunctionSubstringSep::MacroFunctionSubstringSep(bool bAfter) :
	bAfter_(bAfter)
{
}

qm::MacroFunctionSubstringSep::~MacroFunctionSubstringSep()
{
}

MacroValuePtr qm::MacroFunctionSubstringSep::value(MacroContext* pContext) const
{
	assert(pContext);
	
	LOG(SubstringSep);
	
	if (!checkArgSizeRange(pContext, 2, 3))
		return MacroValuePtr();
	
	size_t nSize = getArgSize();
	
	bool bCase = false;
	if (nSize == 3) {
		ARG(pValue, 2);
		bCase = pValue->boolean();
	}
	
	ARG(pValue, 0);
	wstring_ptr wstr(pValue->string());
	
	ARG(pValueSep, 1);
	wstring_ptr wstrSep(pValueSep->string());
	
	wstring_ptr wstrLower;
	const WCHAR* pwsz = wstr.get();
	
	BMFindString<WSTRING> bmfs(wstrSep.get(), wcslen(wstrSep.get()),
		bCase ? 0 : BMFindString<WSTRING>::FLAG_IGNORECASE);
	const WCHAR* p = bmfs.find(pwsz);
	if (!p)
		return MacroValueFactory::getFactory().newString(L"");
	else if (bAfter_)
		return MacroValueFactory::getFactory().newString(
			wstr.get() + (p - pwsz) + wcslen(wstrSep.get()));
	else
		return MacroValueFactory::getFactory().newString(wstr.get(), p - pwsz);
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
 * MacroFunctionURI
 *
 */

qm::MacroFunctionURI::MacroFunctionURI()
{
}

qm::MacroFunctionURI::~MacroFunctionURI()
{
}

MacroValuePtr qm::MacroFunctionURI::value(MacroContext* pContext) const
{
	assert(pContext);
	
	LOG(URI);
	
	if (!checkArgSizeRange(pContext, 0, 1))
		return MacroValuePtr();
	
	size_t nSize = getArgSize();
	
	MessageHolderBase* pmh = pContext->getMessageHolder();
	if (!pmh || !pmh->getMessageHolder())
		return error(*pContext, MacroErrorHandler::CODE_NOCONTEXTMESSAGE);
	
	wstring_ptr wstrURI;
	if (nSize > 0) {
		const Part* pPart = getPart(pContext, 0);
		if (!pPart)
			return MacroValuePtr();
		
		Message* pMessage = getMessage(pContext, MacroContext::MESSAGETYPE_ALL, 0);
		if (!pMessage)
			return error(*pContext, MacroErrorHandler::CODE_GETMESSAGE);
		
		wstrURI = URI(pmh->getMessageHolder(), pMessage, pPart, URIFragment::TYPE_NONE).toString();
	}
	else {
		wstrURI = URI(pmh->getMessageHolder()).toString();
	}
	
	return MacroValueFactory::getFactory().newString(wstrURI.get());
}

const WCHAR* qm::MacroFunctionURI::getName() const
{
	return L"URI";
}


/****************************************************************************
 *
 * MacroFunctionVariable
 *
 */

qm::MacroFunctionVariable::MacroFunctionVariable()
{
}

qm::MacroFunctionVariable::~MacroFunctionVariable()
{
}

MacroValuePtr qm::MacroFunctionVariable::value(MacroContext* pContext) const
{
	assert(pContext);
	
	LOG(Variable);
	
	if (!checkArgSizeRange(pContext, 1, 3))
		return MacroValuePtr();
	
	size_t nSize = getArgSize();
	
	ARG(pValueName, 0);
	wstring_ptr wstrName(pValueName->string());
	
	MacroValuePtr pValue(pContext->getVariable(wstrName.get()));
	
	if (!pValue.get()) {
		if (nSize != 1) {
			bool bGlobal = false;
			if (nSize > 2) {
				ARG(pValueGlobal, 2);
				bGlobal = pValueGlobal->boolean();
			}
			pValue = getArg(1)->value(pContext);
			if (!pValue.get())
				return MacroValuePtr();
			
			if (!pContext->setVariable(wstrName.get(), pValue.get(), bGlobal))
				return error(*pContext, MacroErrorHandler::CODE_FAIL);
		}
		else {
			return MacroValueFactory::getFactory().newString(L"");
		}
	}
	
	return pValue;
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

qm::MacroFunctionWhile::MacroFunctionWhile()
{
}

qm::MacroFunctionWhile::~MacroFunctionWhile()
{
}

MacroValuePtr qm::MacroFunctionWhile::value(MacroContext* pContext) const
{
	assert(pContext);
	
	LOG(While);
	
	if (!checkArgSize(pContext, 2))
		return error(*pContext, MacroErrorHandler::CODE_INVALIDARGSIZE);
	
	while (true) {
		ARG(pValueCondition, 0);
		if (!pValueCondition->boolean())
			return pValueCondition;
		ARG(pValue, 1);
	}

	return MacroValuePtr();
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

std::auto_ptr<MacroFunction> qm::MacroFunctionFactory::newFunction(MacroParser::Type type,
																   const WCHAR* pwszName) const
{
	assert(pwszName);
	
#define BEGIN_DECLARE_FUNCTION() \
	switch (*pwszName) { \

#define END_DECLARE_FUNCTION() \
	} \
	return std::auto_ptr<MacroFunction>(new MacroFunctionFunction(pwszName)); \

#define BEGIN_BLOCK(c0, c1) \
	case c0: \
	case c1: \
		if (false) { \
		} \

#define END_BLOCK() \
	break; \

#define DECLARE_FUNCTION0(classname, name) \
		else if (_wcsicmp(pwszName, name) == 0) { \
			return std::auto_ptr<MacroFunction>(new MacroFunction##classname()); \
		} \
	
#define DECLARE_FUNCTION1(classname, name, arg1) \
		else if (_wcsicmp(pwszName, name) == 0) { \
			return std::auto_ptr<MacroFunction>(new MacroFunction##classname(arg1)); \
		} \
	
#define DECLARE_FUNCTION_TYPE0(classname, name, typename) \
		else if (_wcsicmp(pwszName, name) == 0 && (type & typename)) { \
			return std::auto_ptr<MacroFunction>(new MacroFunction##classname()); \
		} \
	
#define DECLARE_FUNCTION_TYPE1(classname, name, arg1, typename) \
		else if (_wcsicmp(pwszName, name) == 0 && (type & typename)) { \
			return std::auto_ptr<MacroFunction>(new MacroFunction##classname(arg1)); \
		} \

#define M MacroParser::TYPE_MESSAGE
#define RT MacroParser::TYPE_RULE | MacroParser::TYPE_TEMPLATE
#define T MacroParser::TYPE_TEMPLATE
	
	BEGIN_DECLARE_FUNCTION()
		BEGIN_BLOCK(L'a', L'A')
			DECLARE_FUNCTION0(		Account,			L"account"												)
			DECLARE_FUNCTION0(		AccountDirectory,	L"accountdirectory"										)
			DECLARE_FUNCTION1(		Address,			L"address",			false								)
			DECLARE_FUNCTION_TYPE0(	AddressBook,		L"addressbook",										T	)
			DECLARE_FUNCTION1(		Additive,			L"add",				true								)
			DECLARE_FUNCTION0(		And,				L"and"													)
			DECLARE_FUNCTION0(		Attachment,			L"attachment"											)
		END_BLOCK()
		BEGIN_BLOCK(L'b', L'B')
			DECLARE_FUNCTION0(		Body,				L"body"													)
			DECLARE_FUNCTION1(		Contain,			L"beginwith",		true								)
		END_BLOCK()
		BEGIN_BLOCK(L'c', L'C')
			DECLARE_FUNCTION0(		Clipboard,			L"clipboard"											)
			DECLARE_FUNCTION0(		ComputerName,		L"computername"											)
			DECLARE_FUNCTION0(		Concat,				L"concat"												)
			DECLARE_FUNCTION1(		Contain,			L"contain",			false								)
			DECLARE_FUNCTION_TYPE1(	Copy,				L"copy",			false,							M	)
		END_BLOCK()
		BEGIN_BLOCK(L'd', L'D')
			DECLARE_FUNCTION0(		Date,				L"date"													)
			DECLARE_FUNCTION0(		Decode,				L"decode"												)
			DECLARE_FUNCTION0(		Defun,				L"defun"												)
			DECLARE_FUNCTION_TYPE0(	Delete,				L"delete",											M	)
			DECLARE_FUNCTION1(		Flag,				L"deleted",			MessageHolder::FLAG_DELETED			)
			DECLARE_FUNCTION1(		Flag,				L"download",		MessageHolder::FLAG_DOWNLOAD		)
			DECLARE_FUNCTION1(		Flag,				L"downloadtext",	MessageHolder::FLAG_DOWNLOADTEXT	)
			DECLARE_FUNCTION1(		Flag,				L"draft",			MessageHolder::FLAG_DRAFT			)
		END_BLOCK()
		BEGIN_BLOCK(L'e', L'E')
			DECLARE_FUNCTION0(		Equal, 				L"equal"												)
			DECLARE_FUNCTION1(		Eval, 				L"eval",			type								)
			DECLARE_FUNCTION0(		Execute, 			L"execute"												)
			DECLARE_FUNCTION0(		Exist, 				L"exist"												)
			DECLARE_FUNCTION0(		Exit, 				L"exit"													)
		END_BLOCK()
		BEGIN_BLOCK(L'f', L'F')
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
		END_BLOCK()
		BEGIN_BLOCK(L'g', L'G')
			DECLARE_FUNCTION1(		Relative,			L"greater",			false								)
		END_BLOCK()
		BEGIN_BLOCK(L'h', L'H')
			DECLARE_FUNCTION0(		Header, 			L"header"												)
			DECLARE_FUNCTION0(		HtmlEscape, 		L"htmlescape"											)
		END_BLOCK()
		BEGIN_BLOCK(L'i', L'I')
			DECLARE_FUNCTION0(		I, 					L"i"													)
			DECLARE_FUNCTION0(		Id, 				L"id"													)
			DECLARE_FUNCTION0(		Identity, 			L"identity"												)
			DECLARE_FUNCTION0(		If, 				L"if"													)
			DECLARE_FUNCTION1(		Include,			L"include",			type								)
			DECLARE_FUNCTION_TYPE0(	InputBox,	 		L"inputbox",										RT	)
		END_BLOCK()
		BEGIN_BLOCK(L'l', L'L')
			DECLARE_FUNCTION0(		Length,				L"length"												)
			DECLARE_FUNCTION1(		Relative,			L"less",			true								)
			DECLARE_FUNCTION0(		Load,				L"load"													)
		END_BLOCK()
		BEGIN_BLOCK(L'm', L'M')
			DECLARE_FUNCTION1(		Flag,				L"marked",			MessageHolder::FLAG_MARKED			)
			DECLARE_FUNCTION_TYPE0(	MessageBox,			L"messagebox",										RT	)
			DECLARE_FUNCTION0(		Messages,			L"messages"												)
			DECLARE_FUNCTION1(		Additive,			L"minus",			false								)
			DECLARE_FUNCTION_TYPE1(	Copy,				L"move",			true,							M	)
			DECLARE_FUNCTION1(		Flag,				L"multipart",		MessageHolder::FLAG_MULTIPART		)
		END_BLOCK()
		BEGIN_BLOCK(L'n', L'N')
			DECLARE_FUNCTION1(		Address,			L"name",			true								)
			DECLARE_FUNCTION0(		Not,				L"not"													)
		END_BLOCK()
		BEGIN_BLOCK(L'o', L'O')
			DECLARE_FUNCTION0(		Or,					L"or"													)
			DECLARE_FUNCTION0(		OSVersion,			L"osversion"											)
		END_BLOCK()
		BEGIN_BLOCK(L'p', L'P')
			DECLARE_FUNCTION0(		ParseURL,			L"parseurl"												)
			DECLARE_FUNCTION0(		Part,				L"part"													)
			DECLARE_FUNCTION0(		Passed,				L"passed"												)
			DECLARE_FUNCTION1(		Flag,				L"partial",			MessageHolder::FLAG_PARTIAL_MASK	)
			DECLARE_FUNCTION0(		ProcessId,			L"processid"											)
			DECLARE_FUNCTION0(		Profile,			L"profile"												)
			DECLARE_FUNCTION0(		ProfileName,		L"profilename"											)
			DECLARE_FUNCTION0(		Progn,				L"progn"												)
		END_BLOCK()
		BEGIN_BLOCK(L'q', L'Q')
			DECLARE_FUNCTION0(		Quote,				L"quote"												)
		END_BLOCK()
		BEGIN_BLOCK(L'r', L'R')
			DECLARE_FUNCTION0(		References,			L"references"											)
			DECLARE_FUNCTION0(		RegexFind,			L"regexfind"											)
			DECLARE_FUNCTION0(		RegexMatch,			L"regexmatch"											)
			DECLARE_FUNCTION0(		RegexReplace,		L"regexreplace"											)
			DECLARE_FUNCTION0(		Remove,				L"remove"												)
			DECLARE_FUNCTION1(		Flag,				L"replied",			MessageHolder::FLAG_REPLIED			)
		END_BLOCK()
		BEGIN_BLOCK(L's', L'S')
			DECLARE_FUNCTION0(		Save,				L"save"													)
			DECLARE_FUNCTION1(		Flag,				L"seen",			MessageHolder::FLAG_SEEN			)
			DECLARE_FUNCTION1(		Flag,				L"sent",			MessageHolder::FLAG_SENT			)
			DECLARE_FUNCTION0(		Script,				L"script"												)
			DECLARE_FUNCTION_TYPE0(	Selected,			L"selected",										T	)
			DECLARE_FUNCTION0(		Set,				L"set"													)
			DECLARE_FUNCTION0(		Size,				L"size"													)
			DECLARE_FUNCTION0(		SpecialFolder,		L"specialfolder"										)
			DECLARE_FUNCTION0(		SubAccount,			L"subaccount"											)
			DECLARE_FUNCTION0(		Subject,			L"subject"												)
			DECLARE_FUNCTION0(		Substring,			L"substring"											)
			DECLARE_FUNCTION1(		SubstringSep,		L"substringafter",	true								)
			DECLARE_FUNCTION1(		SubstringSep,		L"substringbefore",	false								)
			DECLARE_FUNCTION1(		Additive,			L"subtract",		false								)
		END_BLOCK()
		BEGIN_BLOCK(L't', L'T')
			DECLARE_FUNCTION1(		Boolean,			L"true",			true								)
		END_BLOCK()
		BEGIN_BLOCK(L'u', L'U')
			DECLARE_FUNCTION0(		URI,				L"uri"													)
		END_BLOCK()
		BEGIN_BLOCK(L'v', L'V')
			DECLARE_FUNCTION0(		Variable,			L"variable"												)
		END_BLOCK()
		BEGIN_BLOCK(L'w', L'W')
			DECLARE_FUNCTION0(		While,				L"while"												)
		END_BLOCK()
	END_DECLARE_FUNCTION()
}

const MacroFunctionFactory& qm::MacroFunctionFactory::getFactory()
{
	return factory__;
}
