/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#pragma warning(disable:4786)

#include <qmaccount.h>
#include <qmdocument.h>
#include <qmfolder.h>
#include <qmmacro.h>
#include <qmmessage.h>
#include <qmmessageholder.h>
#include <qmsecurity.h>
#include <qmtemplate.h>

#include <qsconv.h>
#include <qsfile.h>
#include <qsinit.h>
#include <qsosutil.h>
#include <qsstream.h>
#include <qsthread.h>

#include <algorithm>

#include "rule.h"
#include "templatemanager.h"
#include "undo.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * RuleManagerImpl
 *
 */

class qm::RuleManagerImpl
{
public:
	enum Flag {
		FLAG_NONE		= 0x00,
		FLAG_AUTO		= 0x01,
		FLAG_JUNK		= 0x02,
		FLAG_JUNKONLY	= 0x04
	};

public:
	typedef std::vector<Rule*> RuleList;

public:
	RuleManagerImpl(RuleManager* pThis,
					const WCHAR* pwszPath);

public:
	bool load();
	void getRules(const Folder* pFolder,
				  bool bAuto,
				  RuleList* pList) const;
	bool apply(Folder* pFolder,
			   MessageHolderList* pList,
			   const MessageHolderList* pConstList,
			   Document* pDocument,
			   HWND hwnd,
			   Profile* pProfile,
			   unsigned int nFlags,
			   unsigned int nSecurityMode,
			   RuleCallback* pCallback);

private:
	static std::auto_ptr<Rule> createJunkRule(Account* pAccount);

public:
	RuleManager* pThis_;
	RuleManager::RuleSetList listRuleSet_;
	ReadWriteLock lock_;
	ReadWriteWriteLock writeLock_;
	ConfigHelper<RuleManager, RuleContentHandler, RuleWriter, ReadWriteWriteLock> helper_;
};

qm::RuleManagerImpl::RuleManagerImpl(RuleManager* pThis,
									 const WCHAR* pwszPath) :
	pThis_(pThis),
	writeLock_(lock_),
	helper_(pwszPath, writeLock_)
{
}

bool qm::RuleManagerImpl::load()
{
	RuleContentHandler handler(pThis_);
	return helper_.load(pThis_, &handler);
}

void qm::RuleManagerImpl::getRules(const Folder* pFolder,
								   bool bAuto,
								   RuleList* pList) const
{
	assert(pFolder);
	assert(pList);
	assert(pList->empty());
	
	Rule::Use use = bAuto ? Rule::USE_AUTO : Rule::USE_MANUAL;
	for (RuleManager::RuleSetList::const_iterator itS = listRuleSet_.begin(); itS != listRuleSet_.end(); ++itS) {
		const RuleSet* pRuleSet = *itS;
		if (pRuleSet->matchName(pFolder)) {
			const RuleSet::RuleList& l = pRuleSet->getRules();
			std::remove_copy_if(l.begin(), l.end(), std::back_inserter(*pList),
				std::not1(std::bind2nd(std::mem_fun(&Rule::isUse), use)));
		}
	}
}

bool qm::RuleManagerImpl::apply(Folder* pFolder,
								MessageHolderList* pList,
								const MessageHolderList* pConstList,
								Document* pDocument,
								HWND hwnd,
								Profile* pProfile,
								unsigned int nFlags,
								unsigned int nSecurityMode,
								RuleCallback* pCallback)
{
	assert(pFolder);
	assert(pDocument);
	assert(hwnd || nFlags & FLAG_AUTO);
	
	Log log(InitThread::getInitThread().getLogger(), L"qm::RuleManagerImpl");
	
	if (!load()) {
		log.error(L"Error occured while loading rules.");
		return false;
	}
	
	Account* pAccount = pFolder->getAccount();
	
	Lock<Account> lock(*pAccount);
	
	if (!pFolder->loadMessageHolders())
		return false;
	
	ReadWriteReadLock readLock(lock_);
	Lock<ReadWriteReadLock> ruleLock(readLock);
	
	bool bAuto = (nFlags & FLAG_AUTO) != 0;
	
	RuleManagerImpl::RuleList listRule;
	if (!(nFlags & FLAG_JUNKONLY))
		getRules(pFolder, bAuto, &listRule);
	
	std::auto_ptr<Rule> pRuleJunk;
	if (nFlags & FLAG_JUNK) {
		pRuleJunk = createJunkRule(pAccount);
		if (pRuleJunk.get())
			listRule.insert(listRule.begin(), pRuleJunk.get());
	}
	
	if (listRule.empty()) {
		log.debug(L"No rule for this folder was found.");
		return true;
	}
	
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
		ListAccessor(const MessageHolderList* p) :
			p_(p)
		{
		}
		
		virtual unsigned int getCount() const
		{
			return p_->size();
		}
		
		virtual MessageHolder* getMessage(unsigned int n) const
		{
			return (*p_)[n];
		}
		
		const MessageHolderList* p_;
	};
	
	FolderAccessor folderAccessor(pFolder);
	ListAccessor listAccessor(pList ? pList : pConstList);
	const Accessor& accessor = pList || pConstList ?
		static_cast<const Accessor&>(listAccessor) :
		static_cast<const Accessor&>(folderAccessor);
	
	unsigned int nCount = accessor.getCount();
	log.debugf(L"%u messages are to be processed.", nCount);
	if (nCount == 0)
		return true;
	
	pCallback->checkingMessages(pFolder);
	pCallback->setRange(0, accessor.getCount());
	
	typedef std::vector<MessageHolderList> ListList;
	ListList ll(listRule.size());
	
	typedef std::vector<unsigned int> IndexList;
	IndexList listDestroyed;
	
	int nMatch = 0;
	MacroVariableHolder globalVariable;
	for (unsigned int n = 0; n < accessor.getCount(); ++n) {
		if (n % 10 == 0 && pCallback->isCanceled())
			return true;
		
		pCallback->setPos(n);
		
		MessageHolder* pmh = accessor.getMessage(n);
		Message msg;
		for (RuleList::size_type m = 0; m < listRule.size(); ++m) {
			const Rule* pRule = listRule[m];
			MacroContext context(pmh, &msg, MessageHolderList(),
				pAccount, pDocument, hwnd, pProfile, 0,
				bAuto ? MacroContext::FLAG_NONE : MacroContext::FLAG_UITHREAD | MacroContext::FLAG_UI,
				nSecurityMode, 0, &globalVariable);
			bool bMatch = pRule->match(&context);
			if (bMatch) {
				ll[m].push_back(pmh);
				++nMatch;
				if (pList && pRule->isMessageDestroyed())
					listDestroyed.push_back(n);
				log.debugf(L"Id=%u matches rule=%u.", pmh->getId(), m);
				break;
			}
		}
	}
	log.debugf(L"%u messages match.", nMatch);
	if (nMatch == 0)
		return true;
	
	pCallback->applyingRule(pFolder);
	pCallback->setRange(0, nMatch);
	pCallback->setPos(0);
	
	UndoItemList undo;
	int nMessage = 0;
	for (RuleList::size_type m = 0; m < listRule.size(); ++m) {
		if (pCallback->isCanceled())
			return true;
		
		const MessageHolderList& l = ll[m];
		if (!l.empty()) {
			const Rule* pRule = listRule[m];
			RuleContext context(l, pDocument, pAccount, pFolder, hwnd,
				pProfile, &globalVariable, bAuto, nSecurityMode, &undo);
			if (!pRule->apply(context))
				return false;
			
			nMessage += l.size();
			pCallback->setPos(nMessage);
		}
	}
	pDocument->getUndoManager()->pushUndoItem(undo.getUndoItem());
	
	assert(pList || listDestroyed.empty());
	for (IndexList::const_iterator it = listDestroyed.begin(); it != listDestroyed.end(); ++it)
		(*pList)[*it] = 0;
	
	return true;
}

std::auto_ptr<Rule> RuleManagerImpl::createJunkRule(Account* pAccount)
{
	const NormalFolder* pJunk = static_cast<NormalFolder*>(
		pAccount->getFolderByBoxFlag(Folder::FLAG_JUNKBOX));
	if (!pJunk)
		return std::auto_ptr<Rule>();
	wstring_ptr wstrJunk(pJunk->getFullName());
	
	const WCHAR* pwszCondition = pAccount->isSupport(Account::SUPPORT_DELETEDMESSAGE) ?
		L"@And(@Not(@Deleted()), @Junk(@Profile('', 'JunkFilter', 'WhiteList'), @Profile('', 'JunkFilter', 'BlackList')))" :
		L"@Junk(@Profile('', 'JunkFilter', 'WhiteList'), @Profile('', 'JunkFilter', 'BlackList'))";
	std::auto_ptr<Macro> pCondition(MacroParser().parse(pwszCondition));
	std::auto_ptr<RuleAction> pAction(new CopyRuleAction(0, wstrJunk.get(), true));
	return std::auto_ptr<Rule>(new Rule(pCondition, pAction, Rule::USE_AUTO));
}


/****************************************************************************
 *
 * RuleManager
 *
 */

qm::RuleManager::RuleManager(const WCHAR* pwszPath) :
	pImpl_(0)
{
	pImpl_ = new RuleManagerImpl(this, pwszPath);
}

qm::RuleManager::~RuleManager()
{
	clear();
	delete pImpl_;
}

const RuleManager::RuleSetList& qm::RuleManager::getRuleSets()
{
	return getRuleSets(true);
}

const RuleManager::RuleSetList& qm::RuleManager::getRuleSets(bool bReload)
{
	if (bReload)
		pImpl_->load();
	return pImpl_->listRuleSet_;
}

void qm::RuleManager::setRuleSets(RuleSetList& listRuleSet)
{
	clear();
	pImpl_->listRuleSet_.swap(listRuleSet);
}

bool qm::RuleManager::apply(Folder* pFolder,
							Document* pDocument,
							HWND hwnd,
							Profile* pProfile,
							unsigned int nSecurityMode,
							RuleCallback* pCallback)
{
	return pImpl_->apply(pFolder, 0, 0, pDocument, hwnd,
		pProfile, RuleManagerImpl::FLAG_NONE, nSecurityMode, pCallback);
}

bool qm::RuleManager::apply(Folder* pFolder,
							const MessageHolderList& l,
							Document* pDocument,
							HWND hwnd,
							Profile* pProfile,
							unsigned int nSecurityMode,
							RuleCallback* pCallback)
{
	return pImpl_->apply(pFolder, 0, &l, pDocument, hwnd,
		pProfile, RuleManagerImpl::FLAG_NONE, nSecurityMode, pCallback);
}

bool qm::RuleManager::apply(Folder* pFolder,
							MessageHolderList* pList,
							Document* pDocument,
							Profile* pProfile,
							bool bJunkFilter,
							bool bJunkFilterOnly,
							RuleCallback* pCallback)
{
	unsigned int nFlags = RuleManagerImpl::FLAG_AUTO;
	if (bJunkFilter)
		nFlags |= RuleManagerImpl::FLAG_JUNK;
	if (bJunkFilterOnly)
		nFlags |= RuleManagerImpl::FLAG_JUNKONLY;
	return pImpl_->apply(pFolder, pList, 0, pDocument, 0,
		pProfile, nFlags, SECURITYMODE_NONE, pCallback);
}

bool qm::RuleManager::save() const
{
	return pImpl_->helper_.save(this);
}

void qm::RuleManager::addRuleSet(std::auto_ptr<RuleSet> pRuleSet)
{
	pImpl_->listRuleSet_.push_back(pRuleSet.get());
	pRuleSet.release();
}

void qm::RuleManager::clear()
{
	std::for_each(pImpl_->listRuleSet_.begin(),
		pImpl_->listRuleSet_.end(), deleter<RuleSet>());
	pImpl_->listRuleSet_.clear();
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

qm::RuleSet::RuleSet()
{
}

qm::RuleSet::RuleSet(const WCHAR* pwszAccount,
					 std::auto_ptr<RegexPattern> pAccount,
					 const WCHAR* pwszFolder,
					 std::auto_ptr<RegexPattern> pFolder) :
	pAccount_(pAccount),
	pFolder_(pFolder)
{
	if (pwszAccount)
		wstrAccount_ = allocWString(pwszAccount);
	if (pwszFolder)
		wstrFolder_ = allocWString(pwszFolder);
}

qm::RuleSet::RuleSet(const RuleSet& ruleset)
{
	RegexCompiler compiler;
	if (ruleset.wstrAccount_.get()) {
		wstrAccount_ = allocWString(ruleset.wstrAccount_.get());
		pAccount_ = compiler.compile(wstrAccount_.get());
		assert(pAccount_.get());
	}
	if (ruleset.wstrFolder_.get()) {
		wstrFolder_ = allocWString(ruleset.wstrFolder_.get());
		pFolder_ = compiler.compile(wstrFolder_.get());
		assert(pFolder_.get());
	}
	
	for (RuleList::const_iterator it = ruleset.listRule_.begin(); it != ruleset.listRule_.end(); ++it)
		listRule_.push_back(new Rule(**it));
}

qm::RuleSet::~RuleSet()
{
	clear();
}

const WCHAR* qm::RuleSet::getAccount() const
{
	return wstrAccount_.get();
}

void qm::RuleSet::setAccount(const WCHAR* pwszAccount,
							 std::auto_ptr<RegexPattern> pAccount)
{
	assert((pwszAccount && pAccount.get()) || (!pwszAccount && !pAccount.get()));
	if (pwszAccount)
		wstrAccount_ = allocWString(pwszAccount);
	else
		wstrAccount_.reset(0);
	pAccount_ = pAccount;
}

const WCHAR* qm::RuleSet::getFolder() const
{
	return wstrFolder_.get();
}

void qm::RuleSet::setFolder(const WCHAR* pwszFolder,
							std::auto_ptr<RegexPattern> pFolder)
{
	assert((pwszFolder && pFolder.get()) || (!pwszFolder && !pFolder.get()));
	if (pwszFolder)
		wstrFolder_ = allocWString(pwszFolder);
	else
		wstrFolder_.reset(0);
	pFolder_ = pFolder;
}

bool qm::RuleSet::matchName(const Folder* pFolder) const
{
	assert(pFolder);
	
	if (pAccount_.get()) {
		const WCHAR* pwszName = pFolder->getAccount()->getName();
		if (!pAccount_->match(pwszName))
			return false;
	}
	
	if (pFolder_.get()) {
		wstring_ptr wstrName(pFolder->getFullName());
		if (!pFolder_->match(wstrName.get()))
			return false;
	}
	
	return true;
}

const RuleSet::RuleList& qm::RuleSet::getRules() const
{
	return listRule_;
}

void qm::RuleSet::setRules(RuleList& listRule)
{
	clear();
	listRule_.swap(listRule);
}

void qm::RuleSet::addRule(std::auto_ptr<Rule> pRule)
{
	listRule_.push_back(pRule.get());
	pRule.release();
}

void qm::RuleSet::clear()
{
	std::for_each(listRule_.begin(), listRule_.end(), deleter<Rule>());
	listRule_.clear();
}


/****************************************************************************
 *
 * Rule
 *
 */

qm::Rule::Rule() :
	nUse_(USE_MANUAL | USE_AUTO)
{
}

qm::Rule::Rule(std::auto_ptr<Macro> pCondition,
			   std::auto_ptr<RuleAction> pAction,
			   unsigned int nUse) :
	pCondition_(pCondition),
	pAction_(pAction),
	nUse_(nUse)
{
}

qm::Rule::Rule(const Rule& rule) :
	nUse_(rule.nUse_)
{
	wstring_ptr wstrCondition(rule.pCondition_->getString());
	pCondition_ = MacroParser().parse(wstrCondition.get());
	if (rule.pAction_.get())
		pAction_ = rule.pAction_->clone();
}

qm::Rule::~Rule()
{
}

const Macro* qm::Rule::getCondition() const
{
	return pCondition_.get();
}

void qm::Rule::setCondition(std::auto_ptr<Macro> pCondition)
{
	pCondition_ = pCondition;
}

RuleAction* qm::Rule::getAction() const
{
	return pAction_.get();
}

void qm::Rule::setAction(std::auto_ptr<RuleAction> pAction)
{
	pAction_ = pAction;
}

bool qm::Rule::isUse(Use use) const
{
	return (nUse_ & use) != 0;
}

unsigned int qm::Rule::getUse() const
{
	return nUse_;
}

void qm::Rule::setUse(unsigned int nUse)
{
	nUse_ = nUse;
}

bool qm::Rule::match(MacroContext* pContext) const
{
	MacroValuePtr pValue(pCondition_->value(pContext));
	return pValue.get() && pValue->boolean();
}

bool qm::Rule::apply(const RuleContext& context) const
{
	return pAction_.get() ? pAction_->apply(context) : true;
}

bool qm::Rule::isMessageDestroyed() const
{
	return pAction_.get() ? pAction_->isMessageDestroyed() : false;
}


/****************************************************************************
 *
 * RuleAction
 *
 */

qm::RuleAction::~RuleAction()
{
}


/****************************************************************************
 *
 * CopyRuleAction
 *
 */

qm::CopyRuleAction::CopyRuleAction(const WCHAR* pwszAccount,
								   const WCHAR* pwszFolder,
								   bool bMove) :
	wstrAccount_(0),
	wstrFolder_(0),
	bMove_(bMove)
{
	if (pwszAccount)
		wstrAccount_ = allocWString(pwszAccount);
	wstrFolder_ = allocWString(pwszFolder);
}

qm::CopyRuleAction::CopyRuleAction(const CopyRuleAction& action) :
	bMove_(action.bMove_)
{
	if (action.wstrAccount_.get())
		wstrAccount_ = allocWString(action.wstrAccount_.get());
	wstrFolder_ = allocWString(action.wstrFolder_.get());
	
	if (action.wstrTemplate_.get())
		wstrTemplate_ = allocWString(action.wstrTemplate_.get());
	
	listArgument_.reserve(action.listArgument_.size());
	for (ArgumentList::const_iterator it = action.listArgument_.begin(); it != action.listArgument_.end(); ++it)
		listArgument_.push_back(std::make_pair(
			allocWString((*it).first).release(), allocWString((*it).second).release()));
}

qm::CopyRuleAction::~CopyRuleAction()
{
	clearTemplateArguments();
}

const WCHAR* qm::CopyRuleAction::getAccount() const
{
	return wstrAccount_.get();
}

const WCHAR* qm::CopyRuleAction::getFolder() const
{
	return wstrFolder_.get();
}

const WCHAR* qm::CopyRuleAction::getTemplate() const
{
	return wstrTemplate_.get();
}

const CopyRuleAction::ArgumentList& qm::CopyRuleAction::getArguments() const
{
	return listArgument_;
}

RuleAction::Type qm::CopyRuleAction::getType() const
{
	return bMove_ ? TYPE_MOVE : TYPE_COPY;
}

bool qm::CopyRuleAction::apply(const RuleContext& context) const
{
	Log log(InitThread::getInitThread().getLogger(), L"qm::CopyRuleAction");
	
	Account* pAccountTo = context.getAccount();
	if (wstrAccount_.get()) {
		pAccountTo = context.getDocument()->getAccount(wstrAccount_.get());
		if (!pAccountTo) {
			log.errorf(L"The specified account is not found: %s.", wstrAccount_.get());
			return false;
		}
	}
	
	Folder* pFolderTo = pAccountTo->getFolder(wstrFolder_.get());
	if (!pFolderTo || pFolderTo->getType() != Folder::TYPE_NORMAL) {
		log.errorf(L"The specified folder is not found or it's a query folder: %s.", wstrFolder_.get());
		return false;
	}
	
	log.debugf(L"%u messages are %s to %s.", context.getMessageHolderList().size(),
		bMove_ ? L"moved" : L"copied", wstrFolder_.get());
	
	if (wstrTemplate_.get()) {
		const TemplateManager* pTemplateManager = context.getDocument()->getTemplateManager();
		const Template* pTemplate = pTemplateManager->getTemplate(
			context.getAccount(), context.getFolder(), wstrTemplate_.get());
		if (!pTemplate) {
			log.errorf(L"The specified template is not found: %s.", wstrTemplate_.get());
			return false;
		}
		
		TemplateContext::ArgumentList listArgument;
		listArgument.resize(listArgument_.size());
		for (ArgumentList::size_type n = 0; n < listArgument_.size(); ++n) {
			listArgument[n].pwszName_ = listArgument_[n].first;
			listArgument[n].pwszValue_ = listArgument_[n].second;
		}
		
		const MessageHolderList& l = context.getMessageHolderList();
		for (MessageHolderList::const_iterator it = l.begin(); it != l.end(); ++it) {
			MessageHolder* pmh = *it;
			
			Message msg;
			TemplateContext templateContext(pmh, &msg, MessageHolderList(),
				context.getAccount(), context.getDocument(), context.getWindow(), 0,
				context.isAuto() ? MacroContext::FLAG_NONE : MacroContext::FLAG_UITHREAD | MacroContext::FLAG_UI,
				context.getSecurityMode(), context.getProfile(), 0, listArgument);
			wstring_ptr wstrValue;
			if (pTemplate->getValue(templateContext, &wstrValue) != Template::RESULT_SUCCESS) {
				log.errorf(L"Error occured while processing template.");
				return false;
			}
			
			std::auto_ptr<Message> pMessage(MessageCreator().createMessage(
				context.getDocument(), wstrValue.get(), wcslen(wstrValue.get())));
			if (!pAccountTo->appendMessage(static_cast<NormalFolder*>(pFolderTo),
				*pMessage, pmh->getFlags() & MessageHolder::FLAG_USER_MASK,
				context.getUndoItemList(), 0))
				return false;
			
			if (bMove_) {
				if (!context.getAccount()->removeMessages(MessageHolderList(1, pmh),
					context.getFolder(), false, 0, context.getUndoItemList()))
					return false;
			}
		}
		
		return true;
	}
	else {
		unsigned int nFlags = bMove_ ? Account::COPYFLAG_MOVE : Account::COPYFLAG_NONE;
		return context.getAccount()->copyMessages(context.getMessageHolderList(),
			context.getFolder(), static_cast<NormalFolder*>(pFolderTo),
			nFlags, 0, context.getUndoItemList());
	}
}

bool qm::CopyRuleAction::isMessageDestroyed() const
{
	return false;
}

std::auto_ptr<RuleAction> qm::CopyRuleAction::clone() const
{
	return std::auto_ptr<RuleAction>(new CopyRuleAction(*this));
}

void qm::CopyRuleAction::setTemplate(const WCHAR* pwszName)
{
	assert(!wstrTemplate_.get());
	wstrTemplate_ = allocWString(pwszName);
}

void qm::CopyRuleAction::setTemplateArguments(ArgumentList& listArgument)
{
	clearTemplateArguments();
	listArgument_.swap(listArgument);
}

void qm::CopyRuleAction::addTemplateArgument(wstring_ptr wstrName,
											 wstring_ptr wstrValue)
{
	listArgument_.push_back(std::make_pair(wstrName.get(), wstrValue.get()));
	wstrName.release();
	wstrValue.release();
}

void qm::CopyRuleAction::clearTemplateArguments()
{
	for (ArgumentList::iterator it = listArgument_.begin(); it != listArgument_.end(); ++it) {
		freeWString((*it).first);
		freeWString((*it).second);
	}
	listArgument_.clear();
}


/****************************************************************************
 *
 * DeleteRuleAction
 *
 */

qm::DeleteRuleAction::DeleteRuleAction(bool bDirect) :
	bDirect_(bDirect)
{
}

qm::DeleteRuleAction::DeleteRuleAction(const DeleteRuleAction& action) :
	bDirect_(action.bDirect_)
{
}

qm::DeleteRuleAction::~DeleteRuleAction()
{
}

bool qm::DeleteRuleAction::isDirect() const
{
	return bDirect_;
}

RuleAction::Type qm::DeleteRuleAction::getType() const
{
	return TYPE_DELETE;
}

bool qm::DeleteRuleAction::apply(const RuleContext& context) const
{
	return context.getAccount()->removeMessages(context.getMessageHolderList(),
		context.getFolder(), bDirect_, 0, context.getUndoItemList());
}

bool qm::DeleteRuleAction::isMessageDestroyed() const
{
	return bDirect_;
}

std::auto_ptr<RuleAction> qm::DeleteRuleAction::clone() const
{
	return std::auto_ptr<RuleAction>(new DeleteRuleAction(*this));
}


/****************************************************************************
 *
 * DeleteCacheRuleAction
 *
 */

qm::DeleteCacheRuleAction::DeleteCacheRuleAction()
{
}

qm::DeleteCacheRuleAction::DeleteCacheRuleAction(const DeleteCacheRuleAction& action)
{
}

qm::DeleteCacheRuleAction::~DeleteCacheRuleAction()
{
}

RuleAction::Type qm::DeleteCacheRuleAction::getType() const
{
	return TYPE_DELETECACHE;
}

bool qm::DeleteCacheRuleAction::apply(const RuleContext& context) const
{
	return context.getAccount()->deleteMessagesCache(
		context.getMessageHolderList());
}

bool qm::DeleteCacheRuleAction::isMessageDestroyed() const
{
	return false;
}

std::auto_ptr<RuleAction> qm::DeleteCacheRuleAction::clone() const
{
	return std::auto_ptr<RuleAction>(new DeleteCacheRuleAction(*this));
}


/****************************************************************************
 *
 * ApplyRuleAction
 *
 */

qm::ApplyRuleAction::ApplyRuleAction(std::auto_ptr<Macro> pMacro) :
	pMacro_(pMacro)
{
}

qm::ApplyRuleAction::ApplyRuleAction(const ApplyRuleAction& action)
{
	wstring_ptr wstrMacro(action.pMacro_->getString());
	pMacro_ = MacroParser().parse(wstrMacro.get());
}

qm::ApplyRuleAction::~ApplyRuleAction()
{
}

const Macro* qm::ApplyRuleAction::getMacro() const
{
	return pMacro_.get();
}

RuleAction::Type qm::ApplyRuleAction::getType() const
{
	return TYPE_APPLY;
}

bool qm::ApplyRuleAction::apply(const RuleContext& context) const
{
	const MessageHolderList& l = context.getMessageHolderList();
	for (MessageHolderList::const_iterator it = l.begin(); it != l.end(); ++it) {
		Message msg;
		MacroContext c(*it, &msg, MessageHolderList(), context.getAccount(),
			context.getDocument(), context.getWindow(), context.getProfile(), 0,
			context.isAuto() ? MacroContext::FLAG_NONE : MacroContext::FLAG_UITHREAD | MacroContext::FLAG_UI,
			context.getSecurityMode(), 0, context.getGlobalVariable());
		MacroValuePtr pValue(pMacro_->value(&c));
		if (!pValue.get())
			return false;
	}
	return true;
}

bool qm::ApplyRuleAction::isMessageDestroyed() const
{
	return false;
}

std::auto_ptr<RuleAction> qm::ApplyRuleAction::clone() const
{
	return std::auto_ptr<RuleAction>(new ApplyRuleAction(*this));
}


/****************************************************************************
 *
 * RuleContext
 *
 */

qm::RuleContext::RuleContext(const MessageHolderList& l,
							 Document* pDocument,
							 Account* pAccount,
							 Folder* pFolder,
							 HWND hwnd,
							 Profile* pProfile,
							 MacroVariableHolder* pGlobalVariable,
							 bool bAuto,
							 unsigned int nSecurityMode,
							 UndoItemList* pUndoItemList) :
	listMessageHolder_(l),
	pDocument_(pDocument),
	pAccount_(pAccount),
	pFolder_(pFolder),
	hwnd_(hwnd),
	pProfile_(pProfile),
	pGlobalVariable_(pGlobalVariable),
	bAuto_(bAuto),
	nSecurityMode_(nSecurityMode),
	pUndoItemList_(pUndoItemList)
{
	assert(!l.empty());
	assert(pDocument);
	assert(pAccount);
	assert(pAccount->isLocked());
	assert(pFolder);
	assert(pProfile);
	assert(pGlobalVariable);
	assert(pUndoItemList);
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

Folder* qm::RuleContext::getFolder() const
{
	return pFolder_;
}

HWND qm::RuleContext::getWindow() const
{
	return hwnd_;
}

Profile* qm::RuleContext::getProfile() const
{
	return pProfile_;
}

MacroVariableHolder* qm::RuleContext::getGlobalVariable() const
{
	return pGlobalVariable_;
}

bool qm::RuleContext::isAuto() const
{
	return bAuto_;
}

unsigned int qm::RuleContext::getSecurityMode() const
{
	return nSecurityMode_;
}

UndoItemList* qm::RuleContext::getUndoItemList() const
{
	return pUndoItemList_;
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
	pCurrentCopyRuleAction_(0),
	nUse_(0)
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
		std::auto_ptr<RegexPattern> pAccount;
		if (pwszAccount) {
			pAccount = compiler.compile(pwszAccount);
			if (!pAccount.get())
				return false;
		}
		std::auto_ptr<RegexPattern> pFolder;
		if (pwszFolder) {
			pFolder = compiler.compile(pwszFolder);
			if (!pFolder.get())
				return false;
		}
		
		std::auto_ptr<RuleSet> pSet(new RuleSet(
			pwszAccount, pAccount, pwszFolder, pFolder));
		assert(!pCurrentRuleSet_);
		pCurrentRuleSet_ = pSet.get();
		pManager_->addRuleSet(pSet);
		
		state_ = STATE_RULESET;
	}
	else if (wcscmp(pwszLocalName, L"rule") == 0) {
		if (state_ != STATE_RULESET)
			return false;
		
		const WCHAR* pwszMatch = 0;
		const WCHAR* pwszUse = 0;
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrName, L"match") == 0)
				pwszMatch = attributes.getValue(n);
			else if (wcscmp(pwszAttrName, L"use") == 0)
				pwszUse = attributes.getValue(n);
			else
				return false;
		}
		if (!pwszMatch)
			return false;
		
		assert(!pCondition_.get());
		pCondition_ = MacroParser().parse(pwszMatch);
		if (!pCondition_.get())
			return false;
		
		nUse_ = 0;
		if (pwszUse) {
			if (wcsstr(pwszUse, L"manual"))
				nUse_ |= Rule::USE_MANUAL;
			if (wcsstr(pwszUse, L"auto"))
				nUse_ |= Rule::USE_AUTO;
		}
		else {
			nUse_ = Rule::USE_MANUAL | Rule::USE_AUTO;
		}
		
		state_ = STATE_RULE;
	}
	else if (wcscmp(pwszLocalName, L"copy") == 0 ||
		wcscmp(pwszLocalName, L"move") == 0) {
		if (state_ != STATE_RULE)
			return false;
		assert(pCondition_.get());
		
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
		
		std::auto_ptr<RuleAction> pAction(new CopyRuleAction(
			pwszAccount, pwszFolder, bMove));
		assert(!pCurrentCopyRuleAction_);
		pCurrentCopyRuleAction_ = static_cast<CopyRuleAction*>(pAction.get());
		std::auto_ptr<Rule> pRule(new Rule(pCondition_, pAction, nUse_));
		pCurrentRuleSet_->addRule(pRule);
		
		state_ = STATE_MOVE;
	}
	else if (wcscmp(pwszLocalName, L"template") == 0) {
		if (state_ != STATE_MOVE)
			return false;
		
		const WCHAR* pwszName = 0;
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrName, L"name") == 0)
				pwszName = attributes.getValue(n);
			else
				return false;
		}
		if (!pwszName)
			return false;
		
		assert(pCurrentCopyRuleAction_);
		pCurrentCopyRuleAction_->setTemplate(pwszName);
		
		state_ = STATE_TEMPLATE;
	}
	else if (wcscmp(pwszLocalName, L"argument") == 0) {
		if (state_ != STATE_TEMPLATE)
			return false;
		
		const WCHAR* pwszName = 0;
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrName, L"name") == 0)
				pwszName = attributes.getValue(n);
			else
				return false;
		}
		if (!pwszName)
			return false;
		
		assert(!wstrTemplateArgumentName_.get());
		wstrTemplateArgumentName_ = allocWString(pwszName);
		
		state_ = STATE_ARGUMENT;
	}
	else if (wcscmp(pwszLocalName, L"delete") == 0) {
		if (state_ != STATE_RULE)
			return false;
		assert(pCondition_.get());
		
		bool bDirect = false;
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrName, L"direct") == 0)
				bDirect = wcscmp(attributes.getValue(n), L"true") == 0;
			else
				return false;
		}
		
		std::auto_ptr<RuleAction> pAction(new DeleteRuleAction(bDirect));
		std::auto_ptr<Rule> pRule(new Rule(pCondition_, pAction, nUse_));
		pCurrentRuleSet_->addRule(pRule);
		
		state_ = STATE_DELETE;
	}
	else if (wcscmp(pwszLocalName, L"deleteCache") == 0) {
		if (state_ != STATE_RULE)
			return false;
		assert(pCondition_.get());
		
		std::auto_ptr<RuleAction> pAction(new DeleteCacheRuleAction());
		std::auto_ptr<Rule> pRule(new Rule(pCondition_, pAction, nUse_));
		pCurrentRuleSet_->addRule(pRule);
		
		state_ = STATE_DELETECACHE;
	}
	else if (wcscmp(pwszLocalName, L"apply") == 0) {
		if (state_ != STATE_RULE)
			return false;
		assert(pCondition_.get());
		
		const WCHAR* pwszMacro = 0;
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrName, L"macro") == 0)
				pwszMacro = attributes.getValue(n);
			else
				return false;
		}
		if (!pwszMacro)
			return false;
		
		std::auto_ptr<Macro> pMacroApply(MacroParser().parse(pwszMacro));
		if (!pMacroApply.get())
			return false;
		
		std::auto_ptr<RuleAction> pAction(new ApplyRuleAction(pMacroApply));
		std::auto_ptr<Rule> pRule(new Rule(pCondition_, pAction, nUse_));
		pCurrentRuleSet_->addRule(pRule);
		
		state_ = STATE_APPLY;
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
		
		if (pCondition_.get()) {
			std::auto_ptr<Rule> pRule(new Rule(pCondition_, std::auto_ptr<RuleAction>(), nUse_));
			pCurrentRuleSet_->addRule(pRule);
		}
		
		state_ = STATE_RULESET;
	}
	else if (wcscmp(pwszLocalName, L"copy") == 0 ||
		wcscmp(pwszLocalName, L"move") == 0) {
		assert(state_ == STATE_MOVE);
		assert(pCurrentCopyRuleAction_);
		pCurrentCopyRuleAction_ = 0;
		state_ = STATE_RULE;
	}
	else if (wcscmp(pwszLocalName, L"template") == 0) {
		assert(state_ == STATE_TEMPLATE);
		state_ = STATE_MOVE;
	}
	else if (wcscmp(pwszLocalName, L"argument") == 0) {
		assert(state_ == STATE_ARGUMENT);
		assert(pCurrentCopyRuleAction_);
		assert(wstrTemplateArgumentName_.get());
		
		pCurrentCopyRuleAction_->addTemplateArgument(
			wstrTemplateArgumentName_, buffer_.getString());
		
		state_ = STATE_TEMPLATE;
	}
	else if (wcscmp(pwszLocalName, L"delete") == 0) {
		assert(state_ == STATE_DELETE);
		state_ = STATE_RULE;
	}
	else if (wcscmp(pwszLocalName, L"deleteCache") == 0) {
		assert(state_ == STATE_DELETECACHE);
		state_ = STATE_RULE;
	}
	else if (wcscmp(pwszLocalName, L"apply") == 0) {
		assert(state_ == STATE_APPLY);
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
	if (state_ == STATE_ARGUMENT) {
		buffer_.append(pwsz + nStart, nLength);
	}
	else {
		const WCHAR* p = pwsz + nStart;
		for (size_t n = 0; n < nLength; ++n, ++p) {
			if (*p != L' ' && *p != L'\t' && *p != '\n')
				return false;
		}
	}
	
	return true;
}


/****************************************************************************
 *
 * RuleWriter
 *
 */

qm::RuleWriter::RuleWriter(Writer* pWriter) :
	handler_(pWriter)
{
}

qm::RuleWriter::~RuleWriter()
{
}

bool qm::RuleWriter::write(const RuleManager* pManager)
{
	if (!handler_.startDocument())
		return false;
	if (!handler_.startElement(0, 0, L"rules", DefaultAttributes()))
		return false;
	
	const RuleManager::RuleSetList& listRuleSet =
		const_cast<RuleManager*>(pManager)->getRuleSets(false);
	for (RuleManager::RuleSetList::const_iterator it = listRuleSet.begin(); it != listRuleSet.end(); ++it) {
		const RuleSet* pRuleSet = *it;
		if (!write(pRuleSet))
			return false;
	}
	
	if (!handler_.endElement(0, 0, L"rules"))
		return false;
	if (!handler_.endDocument())
		return false;
	
	return true;
}

bool qm::RuleWriter::write(const RuleSet* pRuleSet)
{
	const WCHAR* pwszAccount = pRuleSet->getAccount();
	const WCHAR* pwszFolder = pRuleSet->getFolder();
	const SimpleAttributes::Item items[] = {
		{ L"account",	pwszAccount,	pwszAccount == 0	},
		{ L"folder",	pwszFolder,		pwszFolder == 0		}
	};
	SimpleAttributes attrs(items, countof(items));
	if (!handler_.startElement(0, 0, L"ruleSet", attrs))
		return false;
	
	const RuleSet::RuleList& listRule = pRuleSet->getRules();
	for (RuleSet::RuleList::const_iterator it = listRule.begin(); it != listRule.end(); ++it) {
		const Rule* pRule = *it;
		if (!write(pRule))
			return false;
	}
	
	if (!handler_.endElement(0, 0, L"ruleSet"))
		return false;
	
	return true;
}

bool qm::RuleWriter::write(const Rule* pRule)
{
	wstring_ptr wstrCondition(pRule->getCondition()->getString());
	StringBuffer<WSTRING> bufUse;
	if (pRule->getUse() != (Rule::USE_MANUAL | Rule::USE_AUTO)) {
		if (pRule->getUse() & Rule::USE_MANUAL)
			bufUse.append(L"manual");
		if (pRule->getUse() & Rule::USE_AUTO) {
			if (bufUse.getLength() != 0)
				bufUse.append(L' ');
			bufUse.append(L"auto");
		}
	}
	const SimpleAttributes::Item items[] = {
		{ L"match",	wstrCondition.get(),	false					},
		{ L"use",	bufUse.getCharArray(),	bufUse.getLength() == 0	}
	};
	SimpleAttributes attrs(items, countof(items));
	if (!handler_.startElement(0, 0, L"rule", attrs))
		return false;
	
	const RuleAction* pAction = pRule->getAction();
	if (pAction) {
		switch (pAction->getType()) {
		case RuleAction::TYPE_MOVE:
		case RuleAction::TYPE_COPY:
			if (!write(static_cast<const CopyRuleAction*>(pAction)))
				return false;
			break;
		case RuleAction::TYPE_DELETE:
			if (!write(static_cast<const DeleteRuleAction*>(pAction)))
				return false;
			break;
		case RuleAction::TYPE_DELETECACHE:
			if (!write(static_cast<const DeleteCacheRuleAction*>(pAction)))
				return false;
			break;
		case RuleAction::TYPE_APPLY:
			if (!write(static_cast<const ApplyRuleAction*>(pAction)))
				return false;
			break;
		default:
			assert(false);
			break;
		}
	}
	
	if (!handler_.endElement(0, 0, L"rule"))
		return false;
	
	return true;
}

bool qm::RuleWriter::write(const CopyRuleAction* pAction)
{
	const WCHAR* pwszName = pAction->getType() == RuleAction::TYPE_MOVE ? L"move" : L"copy";
	
	const WCHAR* pwszAccount = pAction->getAccount();
	const WCHAR* pwszFolder = pAction->getFolder();
	const SimpleAttributes::Item items[] = {
		{ L"account",	pwszAccount,	pwszAccount == 0	},
		{ L"folder",	pwszFolder,		pwszFolder == 0		}
	};
	SimpleAttributes attrs(items, countof(items));
	if (!handler_.startElement(0, 0, pwszName, attrs))
		return false;
	
	const WCHAR* pwszTemplate = pAction->getTemplate();
	if (pwszTemplate) {
		SimpleAttributes attrs(L"name", pwszTemplate);
		if (!handler_.startElement(0, 0, L"template", attrs))
			return false;
		
		const CopyRuleAction::ArgumentList& l = pAction->getArguments();
		for (CopyRuleAction::ArgumentList::const_iterator it = l.begin(); it != l.end(); ++it) {
			SimpleAttributes attrs(L"name", (*it).first);
			if (!handler_.startElement(0, 0, L"argument", attrs) ||
				!handler_.characters((*it).second, 0, wcslen((*it).second)) ||
				!handler_.endElement(0, 0, L"argument"))
				return false;
		}
		
		if (!handler_.endElement(0, 0, L"template"))
			return false;
	}
	
	if (!handler_.endElement(0, 0, pwszName))
		return false;
	
	return true;
}

bool qm::RuleWriter::write(const DeleteRuleAction* pAction)
{
	SimpleAttributes attrs(L"direct", pAction->isDirect() ? L"true" : L"false");
	return handler_.startElement(0, 0, L"delete", attrs) &&
		handler_.endElement(0, 0, L"delete");
}

bool qm::RuleWriter::write(const DeleteCacheRuleAction* pAction)
{
	return handler_.startElement(0, 0, L"deleteCache", DefaultAttributes()) &&
		handler_.endElement(0, 0, L"deleteCache");
}

bool qm::RuleWriter::write(const ApplyRuleAction* pAction)
{
	wstring_ptr wstrMacro(pAction->getMacro()->getString());
	SimpleAttributes attrs(L"macro", wstrMacro.get());
	return handler_.startElement(0, 0, L"apply", attrs) &&
		handler_.endElement(0, 0, L"apply");
}
