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
#include <qmfilenames.h>
#include <qmfolder.h>
#include <qmmacro.h>
#include <qmmessage.h>
#include <qmmessageholder.h>

#include <qsconv.h>
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

qm::RuleManager::RuleManager()
{
	SYSTEMTIME st;
	::GetSystemTime(&st);
	::SystemTimeToFileTime(&st, &ft_);
}

qm::RuleManager::~RuleManager()
{
	clear();
}

bool qm::RuleManager::apply(const Folder* pFolder,
							const MessageHolderList* pList,
							Document* pDocument,
							HWND hwnd,
							Profile* pProfile,
							RuleCallback* pCallback)
{
	assert(pFolder);
	assert(pDocument);
	assert(hwnd);
	
	if (!load())
		return false;
	
	Account* pAccount = pFolder->getAccount();
	
	Lock<Account> lock(*pAccount);
	
	const RuleSet* pRuleSet = getRuleSet(pFolder);
	if (!pRuleSet)
		return true;
	
	typedef std::vector<MessageHolderList> ListList;
	ListList ll(pRuleSet->getCount());
	
	struct Accessor
	{
		virtual unsigned int getCount() const = 0;
		virtual MessageHolder* getMessage(unsigned int n) const = 0;
	};
	
	struct FolderAccessor : public Accessor
	{
		FolderAccessor(const Folder* pFolder) :
			pFolder_(pFolder)
		{
		}
		
		virtual unsigned int getCount() const
		{
			return pFolder_->getCount();
		}
		
		virtual MessageHolder* getMessage(unsigned int n) const
		{
			return pFolder_->getMessage(n);
		}
		
		const Folder* pFolder_;
	};
	
	struct ListAccessor : public Accessor
	{
		ListAccessor(const MessageHolderList& l) :
			l_(l)
		{
		}
		
		virtual unsigned int getCount() const
		{
			return l_.size();
		}
		
		virtual MessageHolder* getMessage(unsigned int n) const
		{
			return l_[n];
		}
		
		const MessageHolderList& l_;
	};
	
	FolderAccessor folderAccessor(pFolder);
	ListAccessor listAccessor(*pList);
	const Accessor& accessor = pList ?
		static_cast<const Accessor&>(listAccessor) :
		static_cast<const Accessor&>(folderAccessor);
	
	pCallback->checkingMessages();
	pCallback->setRange(0, accessor.getCount());
	
	int nMatch = 0;
	MacroVariableHolder globalVariable;
	for (unsigned int n = 0; n < accessor.getCount(); ++n) {
		if (n % 10 == 0 && pCallback->isCanceled())
			return true;
		
		pCallback->setPos(n);
		
		MessageHolder* pmh = accessor.getMessage(n);
		Message msg;
		for (size_t m = 0; m < pRuleSet->getCount(); ++m) {
			const Rule* pRule = pRuleSet->getRule(m);
			MacroContext context(pmh, &msg, pAccount, pDocument,
				hwnd, pProfile, false, 0, &globalVariable);
			bool bMatch = pRule->match(&context);
			if (bMatch) {
				ll[m].push_back(pmh);
				++nMatch;
				break;
			}
		}
	}
	
	pCallback->applyingRule();
	pCallback->setRange(0, nMatch);
	pCallback->setPos(0);
	
	int nMessage = 0;
	for (size_t m = 0; m < pRuleSet->getCount(); ++m) {
		if (pCallback->isCanceled())
			return true;
		
		const MessageHolderList& l = ll[m];
		if (!l.empty()) {
			const Rule* pRule = pRuleSet->getRule(m);
			RuleContext context(l, pDocument, pAccount);
			if (!pRule->apply(context))
				return false;
			
			nMessage += l.size();
			pCallback->setPos(nMessage);
		}
	}
	
	return true;
}

void qm::RuleManager::addRuleSet(std::auto_ptr<RuleSet> pRuleSet)
{
	listRuleSet_.push_back(pRuleSet.get());
	pRuleSet.release();
}

bool qm::RuleManager::load()
{
	wstring_ptr wstrPath(Application::getApplication().getProfilePath(FileNames::RULES_XML));
	
	W2T(wstrPath.get(), ptszPath);
	AutoHandle hFile(::CreateFile(ptszPath, GENERIC_READ, 0, 0,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0));
	if (hFile.get()) {
		FILETIME ft;
		::GetFileTime(hFile.get(), 0, 0, &ft);
		hFile.close();
		
		if (::CompareFileTime(&ft, &ft_) != 0) {
			clear();
			
			XMLReader reader;
			RuleContentHandler handler(this);
			reader.setContentHandler(&handler);
			if (!reader.parse(wstrPath.get()))
				return false;
			
			ft_ = ft;
		}
	}
	else {
		clear();
	}
	
	return true;
}

void qm::RuleManager::clear()
{
	std::for_each(listRuleSet_.begin(),
		listRuleSet_.end(), deleter<RuleSet>());
	listRuleSet_.clear();
}

const RuleSet* qm::RuleManager::getRuleSet(const Folder* pFolder) const
{
	assert(pFolder);
	
	for (RuleSetList::const_iterator it = listRuleSet_.begin(); it != listRuleSet_.end(); ++it) {
		const RuleSet* pRuleSet = *it;
		if (pRuleSet->matchName(pFolder))
			return pRuleSet;
	}
	
	return 0;
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

qm::RuleSet::RuleSet(std::auto_ptr<RegexPattern> pAccountName,
					 std::auto_ptr<RegexPattern> pFolderName) :
	pAccountName_(pAccountName),
	pFolderName_(pFolderName)
{
}

qm::RuleSet::~RuleSet()
{
	std::for_each(listRule_.begin(), listRule_.end(), deleter<Rule>());
}

bool qm::RuleSet::matchName(const Folder* pFolder) const
{
	assert(pFolder);
	
	if (pAccountName_.get()) {
		const WCHAR* pwszName = pFolder->getAccount()->getName();
		if (!pAccountName_->match(pwszName))
			return false;
	}
	
	if (pFolderName_.get()) {
		wstring_ptr wstrName(pFolder->getFullName());
		if (!pFolderName_->match(wstrName.get()))
			return false;
	}
	
	return true;
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

void qm::RuleSet::addRule(std::auto_ptr<Rule> pRule)
{
	listRule_.push_back(pRule.get());
	pRule.release();
}


/****************************************************************************
 *
 * Rule
 *
 */

qm::Rule::Rule(std::auto_ptr<Macro> pMacro) :
	pMacro_(pMacro)
{
}

qm::Rule::~Rule()
{
}

bool qm::Rule::match(MacroContext* pContext) const
{
	MacroValuePtr pValue(pMacro_->value(pContext));
	return pValue->boolean();
}


/****************************************************************************
 *
 * NullRule
 *
 */

qm::NullRule::NullRule(std::auto_ptr<Macro> pMacro) :
	Rule(pMacro)
{
}

qm::NullRule::~NullRule()
{
}

bool qm::NullRule::apply(const RuleContext& context) const
{
	return true;
}


/****************************************************************************
 *
 * CopyRule
 *
 */

qm::CopyRule::CopyRule(std::auto_ptr<Macro> pMacro,
					   const WCHAR* pwszAccount,
					   const WCHAR* pwszFolder,
					   bool bMove) :
	Rule(pMacro),
	wstrAccount_(0),
	wstrFolder_(0),
	bMove_(bMove)
{
	if (pwszAccount)
		wstrAccount_ = allocWString(pwszAccount);
	wstrFolder_ = allocWString(pwszFolder);
}

qm::CopyRule::~CopyRule()
{
}

bool qm::CopyRule::apply(const RuleContext& context) const
{
	Account* pAccount = context.getAccount();
	if (wstrAccount_.get()) {
		pAccount = context.getDocument()->getAccount(wstrAccount_.get());
		if (!pAccount)
			return false;
	}
	
	Folder* pFolderTo = pAccount->getFolder(wstrFolder_.get());
	if (!pFolderTo || pFolderTo->getType() != Folder::TYPE_NORMAL)
		return false;
	
	return context.getAccount()->copyMessages(context.getMessageHolderList(),
		static_cast<NormalFolder*>(pFolderTo), bMove_, 0);
}


/****************************************************************************
 *
 * RuleContext
 *
 */

qm::RuleContext::RuleContext(const MessageHolderList& l,
							 Document* pDocument,
							 Account* pAccount) :
	listMessageHolder_(l),
	pDocument_(pDocument),
	pAccount_(pAccount)
{
	assert(!l.empty());
	assert(pDocument);
	assert(pAccount);
	assert(pAccount->isLocked());
}

qm::RuleContext::~RuleContext()
{
}

const MessageHolderList& qm::RuleContext::getMessageHolderList() const
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

qm::RuleContentHandler::RuleContentHandler(RuleManager* pManager) :
	pManager_(pManager),
	state_(STATE_ROOT),
	pCurrentRuleSet_(0),
	parser_(MacroParser::TYPE_RULE)
{
}

qm::RuleContentHandler::~RuleContentHandler()
{
}

bool qm::RuleContentHandler::startElement(const WCHAR* pwszNamespaceURI,
										  const WCHAR* pwszLocalName,
										  const WCHAR* pwszQName,
										  const Attributes& attributes)
{
	if (wcscmp(pwszLocalName, L"rules") == 0) {
		if (state_ != STATE_ROOT)
			return false;
		if (attributes.getLength() != 0)
			return false;
		state_ = STATE_RULES;
	}
	else if (wcscmp(pwszLocalName, L"ruleSet") == 0) {
		if (state_ != STATE_RULES)
			return false;
		
		const WCHAR* pwszAccount = 0;
		const WCHAR* pwszFolder = 0;
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrName, L"account") == 0)
				pwszAccount = attributes.getValue(n);
			else if (wcscmp(pwszAttrName, L"folder") == 0)
				pwszFolder = attributes.getValue(n);
			else
				return false;
		}
		
		RegexCompiler compiler;
		std::auto_ptr<RegexPattern> pAccountName;
		if (pwszAccount) {
			pAccountName = compiler.compile(pwszAccount);
			if (!pAccountName.get())
				return false;
		}
		std::auto_ptr<RegexPattern> pFolderName;
		if (pwszFolder) {
			pFolderName = compiler.compile(pwszFolder);
			if (!pFolderName.get())
				return false;
		}
		
		std::auto_ptr<RuleSet> pSet(new RuleSet(pAccountName, pFolderName));
		assert(!pCurrentRuleSet_);
		pCurrentRuleSet_ = pSet.get();
		pManager_->addRuleSet(pSet);
		
		state_ = STATE_RULESET;
	}
	else if (wcscmp(pwszLocalName, L"rule") == 0) {
		if (state_ != STATE_RULESET)
			return false;
		
		const WCHAR* pwszMatch = 0;
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrName, L"match") == 0)
				pwszMatch = attributes.getValue(n);
			else
				return false;
		}
		if (!pwszMatch)
			return false;
		
		assert(!pMacro_.get());
		pMacro_ = parser_.parse(pwszMatch);
		if (!pMacro_.get())
			return false;
		
		state_ = STATE_RULE;
	}
	else if (wcscmp(pwszLocalName, L"copy") == 0 ||
		wcscmp(pwszLocalName, L"move") == 0) {
		if (state_ != STATE_RULE)
			return false;
		if (!pMacro_.get())
			return false;
		
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
				return false;
		}
		if (!pwszFolder)
			return false;
		
		std::auto_ptr<Rule> pRule(new CopyRule(
			pMacro_, pwszAccount, pwszFolder, bMove));
		pCurrentRuleSet_->addRule(pRule);
		
		state_ = STATE_MOVE;
	}
	else {
		return false;
	}
	
	return true;
}

bool qm::RuleContentHandler::endElement(const WCHAR* pwszNamespaceURI,
										const WCHAR* pwszLocalName,
										const WCHAR* pwszQName)
{
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
		
		if (pMacro_.get()) {
			std::auto_ptr<Rule> pRule(new NullRule(pMacro_));
			pCurrentRuleSet_->addRule(pRule);
		}
		
		state_ = STATE_RULESET;
	}
	else if (wcscmp(pwszLocalName, L"copy") == 0 ||
		wcscmp(pwszLocalName, L"move") == 0) {
		assert(state_ == STATE_MOVE);
		state_ = STATE_RULE;
	}
	else {
		return false;
	}
	
	return true;
}

bool qm::RuleContentHandler::characters(const WCHAR* pwsz,
										size_t nStart,
										size_t nLength)
{
	const WCHAR* p = pwsz + nStart;
	for (size_t n = 0; n < nLength; ++n, ++p) {
		if (*p != L' ' && *p != L'\t' && *p != '\n')
			return false;
	}
	
	return true;
}
