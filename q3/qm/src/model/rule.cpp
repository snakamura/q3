/*
 * $Id: rule.cpp,v 1.2 2003/05/18 19:16:10 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmaccount.h>
#include <qmdocument.h>
#include <qmextensions.h>
#include <qmfolder.h>
#include <qmmacro.h>
#include <qmmessage.h>
#include <qmmessageholder.h>

#include <qsconv.h>
#include <qsnew.h>
#include <qsosutil.h>

#include <algorithm>

#include "rule.h"

#pragma warning(disable:4786)

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * RuleManager
 *
 */

qm::RuleManager::RuleManager(const WCHAR* pwszPath, QSTATUS* pstatus) :
	wstrPath_(0)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
	
	wstrPath_ = concat(pwszPath, L"\\", Extensions::RULES);
	if (!wstrPath_) {
		*pstatus = QSTATUS_OUTOFMEMORY;
		return;
	}
	
	SYSTEMTIME st;
	::GetSystemTime(&st);
	::SystemTimeToFileTime(&st, &ft_);
}

qm::RuleManager::~RuleManager()
{
	freeWString(wstrPath_);
	clear();
}

QSTATUS qm::RuleManager::apply(NormalFolder* pFolder, Document* pDocument,
	HWND hwnd, Profile* pProfile, RuleCallback* pCallback)
{
	assert(pFolder);
	assert(pDocument);
	assert(hwnd);
	
	DECLARE_QSTATUS();
	
	status = load();
	CHECK_QSTATUS();
	
	Lock<Folder> lock(*pFolder);
	
	const RuleSet* pRuleSet = 0;
	status = getRuleSet(pFolder, &pRuleSet);
	CHECK_QSTATUS();
	if (!pRuleSet)
		return QSTATUS_SUCCESS;
	
	typedef std::vector<Folder::MessageHolderList> ListList;
	ListList ll;
	status = STLWrapper<ListList>(ll).resize(pRuleSet->getCount());
	CHECK_QSTATUS();
	
	Account* pAccount = pFolder->getAccount();
	
	status = pCallback->checkingMessages();
	CHECK_QSTATUS();
	status = pCallback->setRange(0, pFolder->getCount());
	CHECK_QSTATUS();
	
	int nMatch = 0;
	MacroVariableHolder globalVariable(&status);
	CHECK_QSTATUS();
	for (unsigned int n = 0; n < pFolder->getCount(); ++n) {
		if (pCallback->isCanceled())
			return QSTATUS_SUCCESS;
		
		status = pCallback->setPos(n);
		CHECK_QSTATUS();
		
		MessageHolder* pmh = pFolder->getMessage(n);
		Message msg(&status);
		CHECK_QSTATUS();
		for (size_t m = 0; m < pRuleSet->getCount(); ++m) {
			const Rule* pRule = pRuleSet->getRule(m);
			MacroContext::Init init = {
				pmh,
				&msg,
				pAccount,
				pDocument,
				hwnd,
				pProfile,
				false,
				0,
				&globalVariable
			};
			MacroContext context(init, &status);
			CHECK_QSTATUS();
			bool bMatch = false;
			status = pRule->match(&context, &bMatch);
			CHECK_QSTATUS();
			if (bMatch) {
				status = STLWrapper<Folder::MessageHolderList>(
					ll[m]).push_back(pmh);
				CHECK_QSTATUS();
				++nMatch;
				break;
			}
		}
	}
	
	status = pCallback->applyingRule();
	CHECK_QSTATUS();
	status = pCallback->setRange(0, nMatch);
	CHECK_QSTATUS();
	status = pCallback->setPos(0);
	CHECK_QSTATUS();
	
	int nMessage = 0;
	for (size_t m = 0; m < pRuleSet->getCount(); ++m) {
		if (pCallback->isCanceled())
			return QSTATUS_SUCCESS;
		
		const Folder::MessageHolderList& l = ll[m];
		if (!l.empty()) {
			const Rule* pRule = pRuleSet->getRule(m);
			RuleContext context(pFolder, l, pDocument, pAccount);
			status = pRule->apply(context);
			CHECK_QSTATUS();
			
			nMessage += l.size();
			status = pCallback->setPos(nMessage);
			CHECK_QSTATUS();
		}
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::RuleManager::addRuleSet(RuleSet* pRuleSet)
{
	return STLWrapper<RuleSetList>(listRuleSet_).push_back(pRuleSet);
}

QSTATUS qm::RuleManager::load()
{
	DECLARE_QSTATUS();
	
	W2T(wstrPath_, ptszPath);
	AutoHandle hFile(::CreateFile(ptszPath, GENERIC_READ, 0, 0,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0));
	if (hFile.get()) {
		FILETIME ft;
		::GetFileTime(hFile.get(), 0, 0, &ft);
		hFile.close();
		
		if (::CompareFileTime(&ft, &ft_) != 0) {
			clear();
			
			XMLReader reader(&status);
			CHECK_QSTATUS();
			RuleContentHandler handler(this, &status);
			CHECK_QSTATUS();
			reader.setContentHandler(&handler);
			status = reader.parse(wstrPath_);
			CHECK_QSTATUS();
			
			ft_ = ft;
		}
	}
	else {
		clear();
	}
	
	return QSTATUS_SUCCESS;
}

void qm::RuleManager::clear()
{
	std::for_each(listRuleSet_.begin(),
		listRuleSet_.end(), deleter<RuleSet>());
	listRuleSet_.clear();
}

QSTATUS qm::RuleManager::getRuleSet(NormalFolder* pFolder,
	const RuleSet** ppRuleSet) const
{
	assert(pFolder);
	assert(ppRuleSet);
	
	DECLARE_QSTATUS();
	
	*ppRuleSet = 0;
	
	RuleSetList::const_iterator it = listRuleSet_.begin();
	while (it != listRuleSet_.end() && !*ppRuleSet) {
		bool bMatch = false;
		status = (*it)->matchName(pFolder, &bMatch);
		CHECK_QSTATUS();
		if (bMatch)
			*ppRuleSet = *it;
		++it;
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * RuleCallback
 *
 */

qm::RuleCallback::~RuleCallback()
{
}


/****************************************************************************
 *
 * RuleSet
 *
 */

qm::RuleSet::RuleSet(const WCHAR* pwszAccount,
	const WCHAR* pwszFolder, QSTATUS* pstatus) :
	pAccountName_(0),
	pFolderName_(0)
{
	assert(pstatus);
	
	DECLARE_QSTATUS();
	
	*pstatus = QSTATUS_SUCCESS;
	
	RegexCompiler compiler;
	std::auto_ptr<RegexPattern> pAccountName;
	if (pwszAccount) {
		RegexPattern* p = 0;
		status = compiler.compile(pwszAccount, &p);
		CHECK_QSTATUS_SET(pstatus);
		pAccountName.reset(p);
	}
	std::auto_ptr<RegexPattern> pFolderName;
	if (pwszFolder) {
		RegexPattern* p = 0;
		status = compiler.compile(pwszFolder, &p);
		CHECK_QSTATUS_SET(pstatus);
		pFolderName.reset(p);
	}
	
	pAccountName_ = pAccountName.release();
	pFolderName_ = pFolderName.release();
}

qm::RuleSet::~RuleSet()
{
	delete pAccountName_;
	delete pFolderName_;
	std::for_each(listRule_.begin(), listRule_.end(), deleter<Rule>());
}

QSTATUS qm::RuleSet::matchName(const NormalFolder* pFolder, bool* pbMatch) const
{
	assert(pFolder);
	assert(pbMatch);
	
	DECLARE_QSTATUS();
	
	*pbMatch = false;
	
	if (pAccountName_) {
		const WCHAR* pwszName = pFolder->getAccount()->getName();
		status = pAccountName_->match(pwszName, pbMatch);
		CHECK_QSTATUS();
		if (!*pbMatch)
			return QSTATUS_SUCCESS;
	}
	
	if (pFolderName_) {
		string_ptr<WSTRING> wstrName;
		status = pFolder->getFullName(&wstrName);
		CHECK_QSTATUS();
		status = pFolderName_->match(wstrName.get(), pbMatch);
		CHECK_QSTATUS();
	}
	else {
		*pbMatch = true;
	}
	
	return QSTATUS_SUCCESS;
}

size_t qm::RuleSet::getCount() const
{
	return listRule_.size();
}

const Rule* qm::RuleSet::getRule(size_t nIndex) const
{
	assert(nIndex < listRule_.size());
	return listRule_[nIndex];
}

QSTATUS qm::RuleSet::addRule(Rule* pRule)
{
	return STLWrapper<RuleList>(listRule_).push_back(pRule);
}


/****************************************************************************
 *
 * Rule
 *
 */

qm::Rule::Rule(Macro* pMacro, QSTATUS* pstatus) :
	pMacro_(pMacro)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qm::Rule::~Rule()
{
	delete pMacro_;
}

QSTATUS qm::Rule::match(MacroContext* pContext, bool* pbMatch) const
{
	assert(pbMatch);
	
	DECLARE_QSTATUS();
	
	MacroValuePtr pValue;
	status = pMacro_->value(pContext, &pValue);
	CHECK_QSTATUS();
	*pbMatch = pValue->boolean();
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * NullRule
 *
 */

qm::NullRule::NullRule(Macro* pMacro, QSTATUS* pstatus) :
	Rule(pMacro, pstatus)
{
}

qm::NullRule::~NullRule()
{
}

QSTATUS qm::NullRule::apply(const RuleContext& context) const
{
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * CopyRule
 *
 */

qm::CopyRule::CopyRule(Macro* pMacro, const WCHAR* pwszAccount,
	const WCHAR* pwszFolder, bool bMove, QSTATUS* pstatus) :
	Rule(pMacro, pstatus),
	wstrAccount_(0),
	wstrFolder_(0),
	bMove_(bMove)
{
	if (*pstatus != QSTATUS_SUCCESS)
		return;
	
	string_ptr<WSTRING> wstrAccount;
	if (pwszAccount) {
		wstrAccount.reset(allocWString(pwszAccount));
		if (!wstrAccount.get()) {
			*pstatus = QSTATUS_OUTOFMEMORY;
			return;
		}
	}
	string_ptr<WSTRING> wstrFolder(allocWString(pwszFolder));
	if (!wstrFolder.get()) {
		*pstatus = QSTATUS_OUTOFMEMORY;
		return;
	}
	
	wstrAccount_ = wstrAccount.release();
	wstrFolder_ = wstrFolder.release();
}

qm::CopyRule::~CopyRule()
{
	freeWString(wstrAccount_);
	freeWString(wstrFolder_);
}

QSTATUS qm::CopyRule::apply(const RuleContext& context) const
{
	DECLARE_QSTATUS();
	
	Account* pAccount = context.getAccount();
	if (wstrAccount_) {
		pAccount = context.getDocument()->getAccount(wstrAccount_);
		if (!pAccount)
			return QSTATUS_FAIL;
	}
	
	Folder* pFolderTo = 0;
	status = pAccount->getFolder(wstrFolder_, &pFolderTo);
	CHECK_QSTATUS();
	if (!pFolderTo || pFolderTo->getType() != Folder::TYPE_NORMAL)
		return QSTATUS_FAIL;
	
	status = context.getFolder()->copyMessages(context.getMessageHolderList(),
		static_cast<NormalFolder*>(pFolderTo), bMove_);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * RuleContext
 *
 */

qm::RuleContext::RuleContext(Folder* pFolder,
	const Folder::MessageHolderList& l, Document* pDocument, Account* pAccount) :
	pFolder_(pFolder),
	listMessageHolder_(l),
	pDocument_(pDocument),
	pAccount_(pAccount)
{
	assert(pFolder);
	assert(pFolder->isLocked());
	assert(!l.empty());
	assert(pDocument);
	assert(pAccount);
}

qm::RuleContext::~RuleContext()
{
}

Folder* qm::RuleContext::getFolder() const
{
	return pFolder_;
}

const Folder::MessageHolderList& qm::RuleContext::getMessageHolderList() const
{
	return listMessageHolder_;
}

Document* qm::RuleContext::getDocument() const
{
	return pDocument_;
}

Account* qm::RuleContext::getAccount() const
{
	return pAccount_;
}


/****************************************************************************
 *
 * RuleContentHandler
 *
 */

qm::RuleContentHandler::RuleContentHandler(
	RuleManager* pManager, QSTATUS* pstatus) :
	DefaultHandler(pstatus),
	pManager_(pManager),
	state_(STATE_ROOT),
	pCurrentRuleSet_(0),
	pMacro_(0),
	pParser_(0)
{
	if (*pstatus != QSTATUS_SUCCESS)
		return;
	
	DECLARE_QSTATUS();
	
	status = newQsObject(MacroParser::TYPE_RULE, &pParser_);
	CHECK_QSTATUS_SET(pstatus);
}

qm::RuleContentHandler::~RuleContentHandler()
{
	delete pMacro_;
	delete pParser_;
}

QSTATUS qm::RuleContentHandler::startElement(const WCHAR* pwszNamespaceURI,
	const WCHAR* pwszLocalName, const WCHAR* pwszQName,
	const Attributes& attributes)
{
	DECLARE_QSTATUS();
	
	if (wcscmp(pwszLocalName, L"rules") == 0) {
		if (state_ != STATE_ROOT)
			return QSTATUS_FAIL;
		if (attributes.getLength() != 0)
			return QSTATUS_FAIL;
		state_ = STATE_RULES;
	}
	else if (wcscmp(pwszLocalName, L"ruleSet") == 0) {
		if (state_ != STATE_RULES)
			return QSTATUS_FAIL;
		
		const WCHAR* pwszAccount = 0;
		const WCHAR* pwszFolder = 0;
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrName, L"account") == 0)
				pwszAccount = attributes.getValue(n);
			else if (wcscmp(pwszAttrName, L"folder") == 0)
				pwszFolder = attributes.getValue(n);
			else
				return QSTATUS_FAIL;
		}
		
		std::auto_ptr<RuleSet> pSet;
		status = newQsObject(pwszAccount, pwszFolder, &pSet);
		CHECK_QSTATUS();
		
		assert(!pCurrentRuleSet_);
		status = pManager_->addRuleSet(pSet.get());
		CHECK_QSTATUS();
		pCurrentRuleSet_ = pSet.release();
		
		state_ = STATE_RULESET;
	}
	else if (wcscmp(pwszLocalName, L"rule") == 0) {
		if (state_ != STATE_RULESET)
			return QSTATUS_FAIL;
		
		const WCHAR* pwszMatch = 0;
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrName, L"match") == 0)
				pwszMatch = attributes.getValue(n);
			else
				return QSTATUS_FAIL;
		}
		if (!pwszMatch)
			return QSTATUS_FAIL;
		
		assert(!pMacro_);
		status = pParser_->parse(pwszMatch, &pMacro_);
		CHECK_QSTATUS();
		
		state_ = STATE_RULE;
	}
	else if (wcscmp(pwszLocalName, L"copy") == 0 ||
		wcscmp(pwszLocalName, L"move") == 0) {
		if (state_ != STATE_RULE)
			return QSTATUS_FAIL;
		if (!pMacro_)
			return QSTATUS_FAIL;
		
		bool bMove = wcscmp(pwszLocalName, L"move") == 0;
		const WCHAR* pwszAccount = 0;
		const WCHAR* pwszFolder = 0;
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrName, L"account") == 0)
				pwszAccount = attributes.getValue(n);
			else if (wcscmp(pwszAttrName, L"folder") == 0)
				pwszFolder = attributes.getValue(n);
			else
				return QSTATUS_FAIL;
		}
		if (!pwszFolder)
			return QSTATUS_FAIL;
		
		std::auto_ptr<CopyRule> pRule;
		status = newQsObject(pMacro_, pwszAccount, pwszFolder, bMove, &pRule);
		CHECK_QSTATUS();
		pMacro_ = 0;
		
		status = pCurrentRuleSet_->addRule(pRule.get());
		CHECK_QSTATUS();
		pRule.release();
		
		state_ = STATE_MOVE;
	}
	else {
		return QSTATUS_FAIL;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::RuleContentHandler::endElement(const WCHAR* pwszNamespaceURI,
	const WCHAR* pwszLocalName, const WCHAR* pwszQName)
{
	DECLARE_QSTATUS();
	
	if (wcscmp(pwszLocalName, L"rules") == 0) {
		assert(state_ == STATE_RULES);
		state_ = STATE_ROOT;
	}
	else if (wcscmp(pwszLocalName, L"ruleSet") == 0) {
		assert(state_ == STATE_RULESET);
		assert(pCurrentRuleSet_);
		pCurrentRuleSet_ = 0;
		state_ = STATE_RULES;
	}
	else if (wcscmp(pwszLocalName, L"rule") == 0) {
		assert(state_ == STATE_RULE);
		
		if (pMacro_) {
			std::auto_ptr<NullRule> pRule;
			status = newQsObject(pMacro_, &pRule);
			CHECK_QSTATUS();
			pMacro_ = 0;
			
			status = pCurrentRuleSet_->addRule(pRule.get());
			CHECK_QSTATUS();
			pRule.release();
		}
		
		state_ = STATE_RULESET;
	}
	else if (wcscmp(pwszLocalName, L"copy") == 0 ||
		wcscmp(pwszLocalName, L"move") == 0) {
		assert(state_ == STATE_MOVE);
		state_ = STATE_RULE;
	}
	else {
		return QSTATUS_FAIL;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::RuleContentHandler::characters(
	const WCHAR* pwsz, size_t nStart, size_t nLength)
{
	DECLARE_QSTATUS();
	
	const WCHAR* p = pwsz + nStart;
	for (size_t n = 0; n < nLength; ++n, ++p) {
		if (*p != L' ' && *p != L'\t' && *p != '\n')
			return QSTATUS_FAIL;
	}
	
	return QSTATUS_SUCCESS;
}
