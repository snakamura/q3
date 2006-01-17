/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#pragma warning(disable:4786)

#include <qmaccount.h>
#include <qmapplication.h>
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

#include "modelresource.h"
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
	class Accessor
	{
	public:
		virtual ~Accessor();
	
	public:
		virtual size_t getCount() const = 0;
		virtual MessagePtr getMessagePtr(size_t n) const = 0;
		virtual MessageHolder* getMessageHolder(size_t n) const = 0;
		virtual bool isRequestResult() const = 0;
		virtual void setMessageHolder(size_t n,
									  MessageHolder* pmh) = 0;
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
			   Accessor* pAccessor,
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
								Accessor* pAccessor,
								Document* pDocument,
								HWND hwnd,
								Profile* pProfile,
								unsigned int nFlags,
								unsigned int nSecurityMode,
								RuleCallback* pCallback)
{
	assert(pFolder);
	assert(pAccessor);
	assert(pDocument);
	assert(hwnd || nFlags & FLAG_AUTO);
	
	Log log(InitThread::getInitThread().getLogger(), L"qm::RuleManagerImpl");
	
	if (!load()) {
		log.error(L"Error occured while loading rules.");
		return false;
	}
	
	Account* pAccount = pFolder->getAccount();
	
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
	
	size_t nCount = pAccessor->getCount();
	log.debugf(L"%u messages are to be processed.", nCount);
	if (nCount == 0)
		return true;
	
	pCallback->checkingMessages(pFolder);
	pCallback->setRange(0, nCount);
	
	typedef std::vector<size_t> IndexList;
	typedef std::vector<IndexList> ListList;
	ListList ll(listRule.size());
	
	size_t nMatch = 0;
	MacroVariableHolder globalVariable;
	for (size_t nMessage = 0; nMessage < nCount; ++nMessage) {
		if (nMessage % 10 == 0 && pCallback->isCanceled())
			return true;
		
		pCallback->setPos(nMessage);
		
		MessagePtrLock mpl(pAccessor->getMessagePtr(nMessage));
		MessageHolder* pmh = mpl ? mpl : pAccessor->getMessageHolder(nMessage);
		if (pmh) {
			Message msg;
			for (RuleList::size_type nRule = 0; nRule < listRule.size(); ++nRule) {
				const Rule* pRule = listRule[nRule];
				unsigned int nFlags = bAuto ? MacroContext::FLAG_NONE :
					MacroContext::FLAG_UITHREAD | MacroContext::FLAG_UI;
				MacroContext context(pmh, &msg, MessageHolderList(), pAccount, pDocument,
					hwnd, pProfile, 0, nFlags, nSecurityMode, 0, &globalVariable);
				bool bMatch = pRule->match(&context);
				if (bMatch) {
					ll[nRule].push_back(nMessage);
					++nMatch;
					log.debugf(L"Id=%u matches rule=%u.", pmh->getId(), nRule);
					
					if (!pRule->isContinue() || !pRule->isContinuable())
						break;
				}
			}
		}
	}
	log.debugf(L"%u messages match.", nMatch);
	if (nMatch == 0)
		return true;
	
	pCallback->applyingRule(pFolder);
	pCallback->setRange(0, nMatch);
	pCallback->setPos(0);
	
	bool bRequestResult = pAccessor->isRequestResult();
	UndoItemList undo;
	size_t nPos = 0;
	for (RuleList::size_type nRule = 0; nRule < listRule.size(); ++nRule) {
		if (pCallback->isCanceled())
			return true;
		
		const IndexList& listIndex = ll[nRule];
		if (!listIndex.empty()) {
			const Rule* pRule = listRule[nRule];
			
			{
				Lock<Account> lock(*pAccount);
				
				MessageHolderList l;
				l.reserve(listIndex.size());
				for (IndexList::const_iterator it = listIndex.begin(); it != listIndex.end(); ++it) {
					MessagePtrLock mpl(pAccessor->getMessagePtr(*it));
					MessageHolder* pmh = mpl ? mpl : pAccessor->getMessageHolder(*it);
					l.push_back(pmh);
				}
				
				RuleContext context(l, pDocument, pAccount, pFolder, hwnd,
					pProfile, &globalVariable, bAuto, nSecurityMode, &undo);
				if (!pRule->apply(context))
					return false;
				
				if (bRequestResult) {
					bool bDestroyed = pRule->isMessageDestroyed();
					for (IndexList::size_type n = 0; n < listIndex.size(); ++n)
						pAccessor->setMessageHolder(listIndex[n], bDestroyed ? 0 : l[n]);
				}
			}
			
			nPos += listIndex.size();
			pCallback->setPos(nPos);
		}
	}
	pDocument->getUndoManager()->pushUndoItem(undo.getUndoItem());
	
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
		L"@And(@Not(@Deleted()), @Junk(@Seen()))" : L"@Junk()";
	std::auto_ptr<Macro> pCondition(MacroParser().parse(pwszCondition));
	std::auto_ptr<RuleAction> pAction(new CopyRuleAction(0, wstrJunk.get(), true));
	return std::auto_ptr<Rule>(new Rule(pCondition, pAction, Rule::USE_AUTO, false, 0));
}


/****************************************************************************
 *
 * RuleManagerImpl::Accessor
 *
 */

qm::RuleManagerImpl::Accessor::~Accessor()
{
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
	if (!pFolder->loadMessageHolders())
		return false;
	
	class FolderAccessor : public RuleManagerImpl::Accessor
	{
	public:
		FolderAccessor(const Folder* pFolder) :
			pFolder_(pFolder)
		{
		}
		
		virtual ~FolderAccessor()
		{
		}
	
	public:
		virtual size_t getCount() const
		{
			return pFolder_->getCount();
		}
		
		virtual MessagePtr getMessagePtr(size_t n) const
		{
			return MessagePtr();
		}
		
		virtual MessageHolder* getMessageHolder(size_t n) const
		{
			return pFolder_->getMessage(static_cast<unsigned int>(n));
		}
		
		virtual bool isRequestResult() const
		{
			return false;
		}
		
		virtual void setMessageHolder(size_t n,
									  MessageHolder* pmh)
		{
			assert(false);
		}
	
	private:
		const Folder* pFolder_;
	} accessor(pFolder);
	
	return pImpl_->apply(pFolder, &accessor, pDocument, hwnd,
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
	class ConstListAccessor : public RuleManagerImpl::Accessor
	{
	public:
		ConstListAccessor(const MessageHolderList& l) :
			l_(l)
		{
		}
		
		virtual ~ConstListAccessor()
		{
		}
	
	public:
		virtual size_t getCount() const
		{
			return l_.size();
		}
		
		virtual MessagePtr getMessagePtr(size_t n) const
		{
			return MessagePtr();
		}
		
		virtual MessageHolder* getMessageHolder(size_t n) const
		{
			return l_[n];
		}
		
		virtual bool isRequestResult() const
		{
			return false;
		}
		
		virtual void setMessageHolder(size_t n,
									  MessageHolder* pmh)
		{
			assert(false);
		}
	
	private:
		const MessageHolderList& l_;
	} accessor(l);
	
	return pImpl_->apply(pFolder, &accessor, pDocument, hwnd,
		pProfile, RuleManagerImpl::FLAG_NONE, nSecurityMode, pCallback);
}

bool qm::RuleManager::apply(Folder* pFolder,
							MessagePtrList* pList,
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
	
	class ListAccessor : public RuleManagerImpl::Accessor
	{
	public:
		ListAccessor(MessagePtrList& l) :
			l_(l)
		{
		}
		
		virtual ~ListAccessor()
		{
		}
	
	public:
		virtual size_t getCount() const
		{
			return l_.size();
		}
		
		virtual MessagePtr getMessagePtr(size_t n) const
		{
			return l_[n];
		}
		
		virtual MessageHolder* getMessageHolder(size_t n) const
		{
			return 0;
		}
		
		virtual bool isRequestResult() const
		{
			return true;
		}
		
		virtual void setMessageHolder(size_t n,
									  MessageHolder* pmh)
		{
			l_[n] = pmh;
		}
	
	private:
		MessagePtrList& l_;
	} accessor(*pList);
	
	return pImpl_->apply(pFolder, &accessor, pDocument, 0,
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
	nUse_(USE_MANUAL | USE_AUTO),
	bContinue_(false)
{
	pAction_.reset(new NoneRuleAction());
}

qm::Rule::Rule(std::auto_ptr<Macro> pCondition,
			   std::auto_ptr<RuleAction> pAction,
			   unsigned int nUse,
			   bool bContinue,
			   const WCHAR* pwszDescription) :
	pCondition_(pCondition),
	pAction_(pAction),
	nUse_(nUse),
	bContinue_(bContinue)
{
	if (pwszDescription)
		wstrDescription_ = allocWString(pwszDescription);
}

qm::Rule::Rule(const Rule& rule) :
	nUse_(rule.nUse_),
	bContinue_(rule.bContinue_)
{
	wstring_ptr wstrCondition(rule.pCondition_->getString());
	pCondition_ = MacroParser().parse(wstrCondition.get());
	pAction_ = rule.pAction_->clone();
	if (rule.wstrDescription_.get())
		wstrDescription_ = allocWString(rule.wstrDescription_.get());
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
	assert(pAction_.get());
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

bool qm::Rule::isContinue() const
{
	return bContinue_;
}

void qm::Rule::setContinue(bool bContinue)
{
	bContinue_ = bContinue;
}

const WCHAR* qm::Rule::getDescription() const
{
	return wstrDescription_.get();
}

void qm::Rule::setDescription(const WCHAR* pwszDescription)
{
	if (pwszDescription && *pwszDescription)
		wstrDescription_ = allocWString(pwszDescription);
	else
		wstrDescription_.reset(0);
}

bool qm::Rule::match(MacroContext* pContext) const
{
	MacroValuePtr pValue(pCondition_->value(pContext));
	return pValue.get() && pValue->boolean();
}

bool qm::Rule::apply(const RuleContext& context) const
{
	return pAction_->apply(context);
}

bool qm::Rule::isMessageDestroyed() const
{
	return (pAction_->getFlags() & RuleAction::FLAG_MESSAGEDESTROYED) != 0;
}

bool qm::Rule::isContinuable() const
{
	return (pAction_->getFlags() & RuleAction::FLAG_CONTINUABLE) != 0;
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
 * NoneRuleAction
 *
 */

qm::NoneRuleAction::NoneRuleAction()
{
}

qm::NoneRuleAction::~NoneRuleAction()
{
}

NoneRuleAction::Type qm::NoneRuleAction::getType() const
{
	return TYPE_NONE;
}

bool qm::NoneRuleAction::apply(const RuleContext& context) const
{
	return true;
}

unsigned int qm::NoneRuleAction::getFlags() const
{
	return FLAG_CONTINUABLE;
}

wstring_ptr qm::NoneRuleAction::getDescription() const
{
	return loadString(Application::getApplication().getResourceHandle(),
		IDS_RULEACTION_NONE);
}

std::auto_ptr<RuleAction> qm::NoneRuleAction::clone() const
{
	return std::auto_ptr<RuleAction>(new NoneRuleAction());
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
			log.errorf(L"The specified template cannot be loaded: %s.", wstrTemplate_.get());
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

unsigned int qm::CopyRuleAction::getFlags() const
{
	return FLAG_NONE;
}

wstring_ptr qm::CopyRuleAction::getDescription() const
{
	wstring_ptr wstrTemplate(loadString(Application::getApplication().getResourceHandle(),
		bMove_ ? IDS_RULEACTION_MOVE : IDS_RULEACTION_COPY));
	StringBuffer<WSTRING> buf;
	if (wstrAccount_.get()) {
		buf.append(L"//");
		buf.append(wstrAccount_.get());
		buf.append(L"/");
	}
	buf.append(wstrFolder_.get());
	return MessageFormat::format(wstrTemplate.get(), buf.getCharArray());
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

wstring_ptr qm::DeleteRuleAction::getDescription() const
{
	return loadString(Application::getApplication().getResourceHandle(),
		bDirect_ ? IDS_RULEACTION_DELETEDIRECT : IDS_RULEACTION_DELETE);
}

unsigned int qm::DeleteRuleAction::getFlags() const
{
	return bDirect_ ? FLAG_MESSAGEDESTROYED : FLAG_NONE;
}

std::auto_ptr<RuleAction> qm::DeleteRuleAction::clone() const
{
	return std::auto_ptr<RuleAction>(new DeleteRuleAction(*this));
}


/****************************************************************************
 *
 * LabelRuleAction
 *
 */

qm::LabelRuleAction::LabelRuleAction(LabelType type,
									 const WCHAR* pwszLabel) :
	type_(type)
{
	assert(pwszLabel);
	wstrLabel_ = allocWString(pwszLabel);
}

qm::LabelRuleAction::LabelRuleAction(const LabelRuleAction& action) :
	type_(action.type_)
{
	wstrLabel_ = allocWString(action.wstrLabel_.get());
}

qm::LabelRuleAction::~LabelRuleAction()
{
}

LabelRuleAction::LabelType qm::LabelRuleAction::getLabelType() const
{
	return type_;
}

const WCHAR* qm::LabelRuleAction::getLabel() const
{
	return wstrLabel_.get();
}

RuleAction::Type qm::LabelRuleAction::getType() const
{
	return TYPE_LABEL;
}

bool qm::LabelRuleAction::apply(const RuleContext& context) const
{
	Account* pAccount = context.getAccount();
	const MessageHolderList& l = context.getMessageHolderList();
	UndoItemList* pUndoItemList = context.getUndoItemList();
	switch (type_) {
	case LABELTYPE_SET:
		if (!pAccount->setMessagesLabel(l, wstrLabel_.get(), pUndoItemList))
			return false;
		break;
	case LABELTYPE_ADD:
		for (MessageHolderList::const_iterator it = l.begin(); it != l.end(); ++it) {
			MessageHolder* pmh = *it;
			wstring_ptr wstrLabel(pmh->getLabel());
			if (*wstrLabel.get()) {
				wstrLabel = concat(wstrLabel.get(), L" ", wstrLabel_.get());
				if (!pAccount->setMessagesLabel(MessageHolderList(1, pmh), wstrLabel.get(), pUndoItemList))
					return false;
			}
			else {
				if (!pAccount->setMessagesLabel(MessageHolderList(1, pmh), wstrLabel_.get(), pUndoItemList))
					return false;
			}
		}
		break;
	case LABELTYPE_REMOVE:
		for (MessageHolderList::const_iterator it = l.begin(); it != l.end(); ++it) {
			MessageHolder* pmh = *it;
			wstring_ptr wstrLabel(pmh->getLabel());
			if (*wstrLabel.get()) {
				StringBuffer<WSTRING> buf;
				const WCHAR* p = wcstok(wstrLabel.get(), L" ");
				while (p) {
					if (wcscmp(p, wstrLabel_.get()) != 0) {
						if (buf.getLength() != 0)
							buf.append(L' ');
						buf.append(p);
					}
					p = wcstok(0, L" ");
				}
				if (!pAccount->setMessagesLabel(MessageHolderList(1, pmh), buf.getCharArray(), pUndoItemList))
					return false;
			}
		}
		break;
	default:
		assert(false);
		break;
	}
	
	return true;
}

wstring_ptr qm::LabelRuleAction::getDescription() const
{
	wstring_ptr wstrTemplate(loadString(Application::getApplication().getResourceHandle(),
		IDS_RULEACTION_LABEL));
	const WCHAR* pwszType = L"=+-";
	WCHAR szType[] = { pwszType[type_], L'\0' };
	return MessageFormat::format(wstrTemplate.get(), wstrLabel_.get(), szType);
}

unsigned int qm::LabelRuleAction::getFlags() const
{
	return FLAG_CONTINUABLE;
}

std::auto_ptr<RuleAction> qm::LabelRuleAction::clone() const
{
	return std::auto_ptr<RuleAction>(new LabelRuleAction(*this));
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

wstring_ptr qm::DeleteCacheRuleAction::getDescription() const
{
	return loadString(Application::getApplication().getResourceHandle(),
		IDS_RULEACTION_DELETECACHE);
}

unsigned int qm::DeleteCacheRuleAction::getFlags() const
{
	return FLAG_CONTINUABLE;
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

wstring_ptr qm::ApplyRuleAction::getDescription() const
{
	wstring_ptr wstrTemplate(loadString(Application::getApplication().getResourceHandle(),
		IDS_RULEACTION_APPLYMACRO));
	wstring_ptr wstrMacro(pMacro_->getString());
	return MessageFormat::format(wstrTemplate.get(), wstrMacro.get());
}

unsigned int qm::ApplyRuleAction::getFlags() const
{
	return FLAG_CONTINUABLE;
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
	nUse_(0),
	bContinue_(false)
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
		const WCHAR* pwszContinue = 0;
		const WCHAR* pwszDescription = 0;
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrName, L"match") == 0)
				pwszMatch = attributes.getValue(n);
			else if (wcscmp(pwszAttrName, L"use") == 0)
				pwszUse = attributes.getValue(n);
			else if (wcscmp(pwszAttrName, L"continue") == 0)
				pwszContinue = attributes.getValue(n);
			else if (wcscmp(pwszAttrName, L"description") == 0)
				pwszDescription = attributes.getValue(n);
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
		
		bContinue_ = pwszContinue && wcscmp(pwszContinue, L"true") == 0;
		
		if (pwszDescription)
			wstrDescription_ = allocWString(pwszDescription);
		
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
		std::auto_ptr<Rule> pRule(new Rule(pCondition_,
			pAction, nUse_, bContinue_, wstrDescription_.get()));
		wstrDescription_.reset(0);
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
		std::auto_ptr<Rule> pRule(new Rule(pCondition_,
			pAction, nUse_, bContinue_, wstrDescription_.get()));
		wstrDescription_.reset(0);
		pCurrentRuleSet_->addRule(pRule);
		
		state_ = STATE_DELETE;
	}
	else if (wcscmp(pwszLocalName, L"label") == 0) {
		if (state_ != STATE_RULE)
			return false;
		assert(pCondition_.get());
		
		const WCHAR* pwszType = 0;
		const WCHAR* pwszLabel = 0;
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrName, L"type") == 0)
				pwszType = attributes.getValue(n);
			else if (wcscmp(pwszAttrName, L"label") == 0)
				pwszLabel = attributes.getValue(n);
			else
				return false;
		}
		if (!pwszLabel)
			return false;
		
		LabelRuleAction::LabelType type = LabelRuleAction::LABELTYPE_SET;
		if (pwszType) {
			if (wcscmp(pwszType, L"set") == 0)
				type = LabelRuleAction::LABELTYPE_SET;
			else if (wcscmp(pwszType, L"add") == 0)
				type = LabelRuleAction::LABELTYPE_ADD;
			else if (wcscmp(pwszType, L"remove") == 0)
				type = LabelRuleAction::LABELTYPE_REMOVE;
			else
				return false;
		}
		
		std::auto_ptr<RuleAction> pAction(new LabelRuleAction(type, pwszLabel));
		std::auto_ptr<Rule> pRule(new Rule(pCondition_,
			pAction, nUse_, bContinue_, wstrDescription_.get()));
		wstrDescription_.reset(0);
		pCurrentRuleSet_->addRule(pRule);
		
		state_ = STATE_LABEL;
	}
	else if (wcscmp(pwszLocalName, L"deleteCache") == 0) {
		if (state_ != STATE_RULE)
			return false;
		assert(pCondition_.get());
		
		std::auto_ptr<RuleAction> pAction(new DeleteCacheRuleAction());
		std::auto_ptr<Rule> pRule(new Rule(pCondition_,
			pAction, nUse_, bContinue_, wstrDescription_.get()));
		wstrDescription_.reset(0);
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
		std::auto_ptr<Rule> pRule(new Rule(pCondition_,
			pAction, nUse_, bContinue_, wstrDescription_.get()));
		wstrDescription_.reset(0);
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
			std::auto_ptr<RuleAction> pAction(new NoneRuleAction());
			std::auto_ptr<Rule> pRule(new Rule(pCondition_,
				pAction, nUse_, bContinue_, wstrDescription_.get()));
			wstrDescription_.reset(0);
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
	else if (wcscmp(pwszLocalName, L"label") == 0) {
		assert(state_ == STATE_LABEL);
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
	bool bContinue = pRule->isContinue();
	const WCHAR* pwszDescription = pRule->getDescription();
	const SimpleAttributes::Item items[] = {
		{ L"match",			wstrCondition.get(),			false					},
		{ L"use",			bufUse.getCharArray(),			bufUse.getLength() == 0	},
		{ L"continue",		bContinue ? L"true" : L"false",	!bContinue				},
		{ L"description",	pwszDescription,				!pwszDescription		}
	};
	SimpleAttributes attrs(items, countof(items));
	if (!handler_.startElement(0, 0, L"rule", attrs))
		return false;
	
	const RuleAction* pAction = pRule->getAction();
	switch (pAction->getType()) {
	case RuleAction::TYPE_NONE:
		break;
	case RuleAction::TYPE_MOVE:
	case RuleAction::TYPE_COPY:
		if (!write(static_cast<const CopyRuleAction*>(pAction)))
			return false;
		break;
	case RuleAction::TYPE_DELETE:
		if (!write(static_cast<const DeleteRuleAction*>(pAction)))
			return false;
		break;
	case RuleAction::TYPE_LABEL:
		if (!write(static_cast<const LabelRuleAction*>(pAction)))
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

bool qm::RuleWriter::write(const LabelRuleAction* pAction)
{
	const WCHAR* pwszType = 0;
	switch (pAction->getLabelType()) {
	case LabelRuleAction::LABELTYPE_SET:
		break;
	case LabelRuleAction::LABELTYPE_ADD:
		pwszType = L"add";
		break;
	case LabelRuleAction::LABELTYPE_REMOVE:
		pwszType = L"remove";
		break;
	default:
		assert(false);
		break;
	}
	const SimpleAttributes::Item items[] = {
		{ L"type",	pwszType,				pwszType == 0	},
		{ L"label",	pAction->getLabel(),	false			}
	};
	SimpleAttributes attrs(items, countof(items));
	return handler_.startElement(0, 0, L"label", attrs) &&
		handler_.endElement(0, 0, L"label");
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
