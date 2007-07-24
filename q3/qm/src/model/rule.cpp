/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
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
		FLAG_BACKGROUND	= 0x01,
		FLAG_JUNK		= 0x02,
		FLAG_JUNKONLY	= 0x04,
		FLAG_NEW		= 0x08
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
	
	class FolderAccessor : public Accessor
	{
	public:
		explicit FolderAccessor(const Folder* pFolder);
		virtual ~FolderAccessor();
	
	public:
		virtual size_t getCount() const;
		virtual MessagePtr getMessagePtr(size_t n) const;
		virtual MessageHolder* getMessageHolder(size_t n) const;
		virtual bool isRequestResult() const;
		virtual void setMessageHolder(size_t n,
									  MessageHolder* pmh);
	
	private:
		const Folder* pFolder_;
	};
	
	class ConstListAccessor : public Accessor
	{
	public:
		explicit ConstListAccessor(const MessageHolderList& l);
		virtual ~ConstListAccessor();
	
	public:
		virtual size_t getCount() const;
		virtual MessagePtr getMessagePtr(size_t n) const;
		virtual MessageHolder* getMessageHolder(size_t n) const;
		virtual bool isRequestResult() const;
		virtual void setMessageHolder(size_t n,
									  MessageHolder* pmh);
	
	private:
		const MessageHolderList& l_;
	};
	
	class ListAccessor : public Accessor
	{
	public:
		ListAccessor(MessagePtrList& l,
					 bool bRequestResult);
		virtual ~ListAccessor();
	
	public:
		virtual size_t getCount() const;
		virtual MessagePtr getMessagePtr(size_t n) const;
		virtual MessageHolder* getMessageHolder(size_t n) const;
		virtual bool isRequestResult() const;
		virtual void setMessageHolder(size_t n,
									  MessageHolder* pmh);
	
	private:
		MessagePtrList& l_;
		bool bRequestResult_;
	};

public:
	typedef std::vector<Rule*> RuleList;

public:
	RuleManagerImpl(RuleManager* pThis,
					const WCHAR* pwszPath);

public:
	bool load();
	void getRules(const Folder* pFolder,
				  Rule::Use use,
				  RuleList* pList) const;
	bool apply(Folder* pFolder,
			   Accessor* pAccessor,
			   Document* pDocument,
			   const ActionInvoker* pActionInvoker,
			   HWND hwnd,
			   Profile* pProfile,
			   Rule::Use use,
			   unsigned int nFlags,
			   unsigned int nSecurityMode,
			   UndoItemList* pUndoItemList,
			   RuleCallback* pCallback,
			   unsigned int* pnResultFlags);

private:
	static std::auto_ptr<Rule> createJunkRule(Account* pAccount);
	static bool isNeedPrepare(Accessor* pAccessor,
							  MacroContext::MessageType type);
	static MacroContext::MessageType getMessageType(const RuleList& l);

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
								   Rule::Use use,
								   RuleList* pList) const
{
	assert(pFolder);
	assert(pList);
	assert(pList->empty());
	
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
								const ActionInvoker* pActionInvoker,
								HWND hwnd,
								Profile* pProfile,
								Rule::Use use,
								unsigned int nFlags,
								unsigned int nSecurityMode,
								UndoItemList* pUndoItemList,
								RuleCallback* pCallback,
								unsigned int* pnResultFlags)
{
	assert(pFolder);
	assert(pAccessor);
	assert(pDocument);
	
	Log log(InitThread::getInitThread().getLogger(), L"qm::RuleManagerImpl");
	
	if (!load()) {
		log.error(L"Error occurred while loading rules.");
		return false;
	}
	
	Account* pAccount = pFolder->getAccount();
	
	ReadWriteReadLock readLock(lock_);
	Lock<ReadWriteReadLock> ruleLock(readLock);
	
	RuleManagerImpl::RuleList listRule;
	if (!(nFlags & FLAG_JUNKONLY))
		getRules(pFolder, use, &listRule);
	
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
	
	if (pCallback) {
		pCallback->checkingMessages(pFolder);
		pCallback->setRange(0, nCount);
	}
	
	bool bBackground = (nFlags & FLAG_BACKGROUND) != 0;
	if (bBackground &&
		pFolder->getType() == Folder::TYPE_NORMAL &&
		pAccount->isRemoteMessageFolder(static_cast<NormalFolder*>(pFolder)) &&
		isNeedPrepare(pAccessor, getMessageType(listRule)))
		pAccount->prepareGetMessage(static_cast<NormalFolder*>(pFolder));
	
	typedef std::vector<size_t> IndexList;
	typedef std::vector<IndexList> ListList;
	ListList ll(listRule.size());
	
	size_t nMatch = 0;
	MacroVariableHolder globalVariable;
	for (size_t nMessage = 0; nMessage < nCount; ++nMessage) {
		if (pCallback && nMessage % 10 == 0 && pCallback->isCanceled())
			return true;
		
		if (pCallback)
			pCallback->setPos(nMessage);
		
		MessagePtrLock mpl(pAccessor->getMessagePtr(nMessage));
		MessageHolder* pmh = mpl ? mpl : pAccessor->getMessageHolder(nMessage);
		if (pmh) {
			Message msg;
			for (RuleList::size_type nRule = 0; nRule < listRule.size(); ++nRule) {
				const Rule* pRule = listRule[nRule];
				if (!pRule->isEnabled())
					continue;
				
				unsigned int nMacroFlags = (bBackground ? MacroContext::FLAG_NONE :
					MacroContext::FLAG_UITHREAD | MacroContext::FLAG_UI) |
					(nFlags & FLAG_NEW ? MacroContext::FLAG_NEW : 0);
				MacroContext context(pmh, &msg, pAccount, MessageHolderList(),
					pFolder, pDocument, pActionInvoker, hwnd, pProfile, 0,
					nMacroFlags, nSecurityMode, 0, &globalVariable);
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
	
	if (pCallback) {
		pCallback->applyingRule(pFolder);
		pCallback->setRange(0, nMatch);
		pCallback->setPos(0);
	}
	
	bool bRequestResult = pAccessor->isRequestResult();
	unsigned int nResultFlags = 0;
	size_t nPos = 0;
	for (RuleList::size_type nRule = 0; nRule < listRule.size(); ++nRule) {
		if (pCallback && pCallback->isCanceled())
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
					if (pmh)
						l.push_back(pmh);
				}
				
				RuleContext context(l, pDocument, pAccount, pFolder,
					pActionInvoker, hwnd, pProfile, &globalVariable, bBackground,
					(nFlags & FLAG_NEW) != 0, nSecurityMode, pUndoItemList);
				if (!pRule->apply(&context))
					return false;
				nResultFlags |= context.getResultFlags();
				
				if (bRequestResult) {
					bool bDestroyed = (context.getResultFlags() & Account::RESULTFLAG_DESTROYED) != 0;
					for (IndexList::size_type n = 0; n < listIndex.size(); ++n)
						pAccessor->setMessageHolder(listIndex[n], bDestroyed ? 0 : l[n]);
				}
			}
			
			nPos += listIndex.size();
			if (pCallback)
				pCallback->setPos(nPos);
		}
	}
	if (pnResultFlags)
		*pnResultFlags = nResultFlags;
	
	return true;
}

std::auto_ptr<Rule> qm::RuleManagerImpl::createJunkRule(Account* pAccount)
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
	return std::auto_ptr<Rule>(new Rule(pCondition, pAction, Rule::USE_AUTO, false, 0, false));
}

bool qm::RuleManagerImpl::isNeedPrepare(Accessor* pAccessor,
										MacroContext::MessageType type)
{
	if (type == MacroContext::MESSAGETYPE_NONE)
		return false;
	
	size_t nCount = pAccessor->getCount();
	for (size_t nMessage = 0; nMessage < nCount; ++nMessage) {
		MessagePtrLock mpl(pAccessor->getMessagePtr(nMessage));
		MessageHolder* pmh = mpl ? mpl : pAccessor->getMessageHolder(nMessage);
		if (pmh) {
			unsigned int nFlags = pmh->getFlags() & MessageHolder::FLAG_PARTIAL_MASK;
			switch (type) {
			case MacroContext::MESSAGETYPE_HEADER:
				if (nFlags == MessageHolder::FLAG_INDEXONLY)
					return true;
				break;
			case MacroContext::MESSAGETYPE_TEXT:
				if (nFlags == MessageHolder::FLAG_INDEXONLY ||
					nFlags == MessageHolder::FLAG_HEADERONLY)
					return true;
				break;
			case MacroContext::MESSAGETYPE_ALL:
				if (nFlags)
					return true;
				break;
			default:
				assert(false);
				break;
			}
		}
	}
	
	return false;
}

MacroContext::MessageType qm::RuleManagerImpl::getMessageType(const RuleList& l)
{
	MacroContext::MessageType type = MacroContext::MESSAGETYPE_NONE;
	for (RuleList::const_iterator it = l.begin(); it != l.end(); ++it)
		type = QSMAX(type, (*it)->getMessageType());
	return type;
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
 * RuleManagerImpl::FolderAccessor
 *
 */

qm::RuleManagerImpl::FolderAccessor::FolderAccessor(const Folder* pFolder) :
	pFolder_(pFolder)
{
}

qm::RuleManagerImpl::FolderAccessor::~FolderAccessor()
{
}

size_t qm::RuleManagerImpl::FolderAccessor::getCount() const
{
	return pFolder_->getCount();
}

MessagePtr qm::RuleManagerImpl::FolderAccessor::getMessagePtr(size_t n) const
{
	return MessagePtr();
}

MessageHolder* qm::RuleManagerImpl::FolderAccessor::getMessageHolder(size_t n) const
{
	return pFolder_->getMessage(static_cast<unsigned int>(n));
}

bool qm::RuleManagerImpl::FolderAccessor::isRequestResult() const
{
	return false;
}

void qm::RuleManagerImpl::FolderAccessor::setMessageHolder(size_t n,
														   MessageHolder* pmh)
{
	assert(false);
}


/****************************************************************************
 *
 * RuleManagerImpl::ConstListAccessor
 *
 */

qm::RuleManagerImpl::ConstListAccessor::ConstListAccessor(const MessageHolderList& l) :
	l_(l)
{
}

qm::RuleManagerImpl::ConstListAccessor::~ConstListAccessor()
{
}

size_t qm::RuleManagerImpl::ConstListAccessor::getCount() const
{
	return l_.size();
}

MessagePtr qm::RuleManagerImpl::ConstListAccessor::getMessagePtr(size_t n) const
{
	return MessagePtr();
}

MessageHolder* qm::RuleManagerImpl::ConstListAccessor::getMessageHolder(size_t n) const
{
	return l_[n];
}

bool qm::RuleManagerImpl::ConstListAccessor::isRequestResult() const
{
	return false;
}

void qm::RuleManagerImpl::ConstListAccessor::setMessageHolder(size_t n,
															  MessageHolder* pmh)
{
	assert(false);
}


/****************************************************************************
 *
 * RuleManagerImpl::ListAccessor
 *
 */

qm::RuleManagerImpl::ListAccessor::ListAccessor(MessagePtrList& l,
												bool bRequestResult) :
	l_(l),
	bRequestResult_(bRequestResult)
{
}

qm::RuleManagerImpl::ListAccessor::~ListAccessor()
{
}

size_t qm::RuleManagerImpl::ListAccessor::getCount() const
{
	return l_.size();
}

MessagePtr qm::RuleManagerImpl::ListAccessor::getMessagePtr(size_t n) const
{
	return l_[n];
}

MessageHolder* qm::RuleManagerImpl::ListAccessor::getMessageHolder(size_t n) const
{
	return 0;
}

bool qm::RuleManagerImpl::ListAccessor::isRequestResult() const
{
	return bRequestResult_;
}

void qm::RuleManagerImpl::ListAccessor::setMessageHolder(size_t n,
														 MessageHolder* pmh)
{
	l_[n] = pmh;
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

bool qm::RuleManager::applyManual(Folder* pFolder,
								  Document* pDocument,
								  const ActionInvoker* pActionInvoker,
								  HWND hwnd,
								  Profile* pProfile,
								  unsigned int nSecurityMode,
								  UndoItemList* pUndoItemList,
								  RuleCallback* pCallback)
{
	if (!pFolder->loadMessageHolders())
		return false;
	
	RuleManagerImpl::FolderAccessor accessor(pFolder);
	return pImpl_->apply(pFolder, &accessor, pDocument, pActionInvoker,
		hwnd, pProfile, Rule::USE_MANUAL, RuleManagerImpl::FLAG_NONE,
		nSecurityMode, pUndoItemList, pCallback, 0);
}

bool qm::RuleManager::applyManual(Folder* pFolder,
								  const MessageHolderList& l,
								  Document* pDocument,
								  const ActionInvoker* pActionInvoker,
								  HWND hwnd,
								  Profile* pProfile,
								  unsigned int nSecurityMode,
								  UndoItemList* pUndoItemList,
								  RuleCallback* pCallback)
{
	RuleManagerImpl::ConstListAccessor accessor(l);
	return pImpl_->apply(pFolder, &accessor, pDocument, pActionInvoker,
		hwnd, pProfile, Rule::USE_MANUAL, RuleManagerImpl::FLAG_NONE,
		nSecurityMode, pUndoItemList, pCallback, 0);
}

bool qm::RuleManager::applyAuto(Folder* pFolder,
								MessagePtrList* pList,
								Document* pDocument,
								Profile* pProfile,
								unsigned int nAutoFlags,
								RuleCallback* pCallback)
{
	unsigned int nFlags = RuleManagerImpl::FLAG_BACKGROUND |
		(nAutoFlags & AUTOFLAG_JUNKFILTER ? RuleManagerImpl::FLAG_JUNK : 0) |
		(nAutoFlags & AUTOFLAG_JUNKFILTERONLY ? RuleManagerImpl::FLAG_JUNKONLY : 0) |
		(nAutoFlags & AUTOFLAG_EXISTING ? 0 : RuleManagerImpl::FLAG_NEW);
	RuleManagerImpl::ListAccessor accessor(*pList, !(nAutoFlags & AUTOFLAG_EXISTING));
	return pImpl_->apply(pFolder, &accessor, pDocument, 0, 0, pProfile,
		Rule::USE_AUTO, nFlags, SECURITYMODE_NONE, 0, pCallback, 0);
}

bool qm::RuleManager::applyActive(Folder* pFolder,
								  const MessageHolderList& l,
								  Document* pDocument,
								  const ActionInvoker* pActionInvoker,
								  HWND hwnd,
								  Profile* pProfile,
								  unsigned int nSecurityMode,
								  bool bBackground,
								  unsigned int* pnResultFlags)
{
	unsigned int nFlags = bBackground ?
		RuleManagerImpl::FLAG_BACKGROUND : RuleManagerImpl::FLAG_NONE;
	RuleManagerImpl::ConstListAccessor accessor(l);
	return pImpl_->apply(pFolder, &accessor, pDocument, pActionInvoker, hwnd,
		pProfile, Rule::USE_ACTIVE, nFlags, nSecurityMode, 0, 0, pnResultFlags);
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

qm::RuleSet::RuleSet(Term& account,
					 Term& folder)
{
	account_.assign(account);
	folder_.assign(folder);
}

qm::RuleSet::RuleSet(const RuleSet& ruleset) :
	account_(ruleset.account_),
	folder_(ruleset.folder_)
{
	for (RuleList::const_iterator it = ruleset.listRule_.begin(); it != ruleset.listRule_.end(); ++it)
		listRule_.push_back(new Rule(**it));
}

qm::RuleSet::~RuleSet()
{
	clear();
}

const WCHAR* qm::RuleSet::getAccount() const
{
	return account_.getValue();
}

void qm::RuleSet::setAccount(Term& account)
{
	account_.assign(account);
}

const WCHAR* qm::RuleSet::getFolder() const
{
	return folder_.getValue();
}

void qm::RuleSet::setFolder(Term& folder)
{
	folder_.assign(folder);
}

bool qm::RuleSet::matchName(const Folder* pFolder) const
{
	assert(pFolder);
	
	if (!account_.match(pFolder->getAccount()->getName()))
		return false;
	
	if (folder_.isSpecified()) {
		wstring_ptr wstrName(pFolder->getFullName());
		if (!folder_.match(wstrName.get()))
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
	bContinue_(false),
	bEnabled_(true)
{
	pAction_.reset(new NoneRuleAction());
}

qm::Rule::Rule(std::auto_ptr<Macro> pCondition,
			   std::auto_ptr<RuleAction> pAction,
			   unsigned int nUse,
			   bool bContinue,
			   const WCHAR* pwszDescription,
			   bool bEnabled) :
	pCondition_(pCondition),
	pAction_(pAction),
	nUse_(nUse),
	bContinue_(bContinue),
	bEnabled_(bEnabled)
{
	if (pwszDescription)
		wstrDescription_ = allocWString(pwszDescription);
}

qm::Rule::Rule(const Rule& rule) :
	nUse_(rule.nUse_),
	bContinue_(rule.bContinue_),
	bEnabled_(rule.bEnabled_)
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

bool qm::Rule::isEnabled() const
{
	return bEnabled_;
}

void qm::Rule::setEnabled(bool bEnabled)
{
	bEnabled_ = bEnabled;
}

bool qm::Rule::match(MacroContext* pContext) const
{
	MacroValuePtr pValue(pCondition_->value(pContext));
	return pValue.get() && pValue->boolean();
}

bool qm::Rule::apply(RuleContext* pContext) const
{
	return pAction_->apply(pContext);
}

bool qm::Rule::isContinuable() const
{
	return (pAction_->getFlags() & RuleAction::FLAG_CONTINUABLE) != 0;
}

MacroContext::MessageType qm::Rule::getMessageType() const
{
	return pCondition_->getMessageTypeHint();
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

bool qm::NoneRuleAction::apply(RuleContext* pContext) const
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

bool qm::CopyRuleAction::apply(RuleContext* pContext) const
{
	Log log(InitThread::getInitThread().getLogger(), L"qm::CopyRuleAction");
	
	Account* pAccountTo = pContext->getAccount();
	if (wstrAccount_.get()) {
		pAccountTo = pContext->getDocument()->getAccount(wstrAccount_.get());
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
	
	log.debugf(L"%u messages are %s to %s.", pContext->getMessageHolderList().size(),
		bMove_ ? L"moved" : L"copied", wstrFolder_.get());
	
	if (wstrTemplate_.get()) {
		const TemplateManager* pTemplateManager = pContext->getDocument()->getTemplateManager();
		const Template* pTemplate = pTemplateManager->getTemplate(
			pContext->getAccount(), pContext->getFolder(), wstrTemplate_.get());
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
		
		const MessageHolderList& l = pContext->getMessageHolderList();
		for (MessageHolderList::const_iterator it = l.begin(); it != l.end(); ++it) {
			MessageHolder* pmh = *it;
			
			Message msg;
			TemplateContext templateContext(pmh, &msg, MessageHolderList(),
				pContext->getFolder(), pContext->getAccount(), pContext->getDocument(),
				pContext->getActionInvoker(), pContext->getWindow(), 0, pContext->getMacroFlags(),
				pContext->getSecurityMode(), pContext->getProfile(), 0, listArgument);
			wstring_ptr wstrValue;
			if (pTemplate->getValue(templateContext, &wstrValue) != Template::RESULT_SUCCESS) {
				log.errorf(L"Error occurred while processing template.");
				return false;
			}
			
			std::auto_ptr<Message> pMessage(MessageCreator().createMessage(
				pContext->getDocument(), wstrValue.get(), wcslen(wstrValue.get())));
			unsigned int nAppendFlags = pContext->isBackground() ?
				Account::OPFLAG_BACKGROUND : Account::OPFLAG_NONE;
			if (!pAccountTo->appendMessage(static_cast<NormalFolder*>(pFolderTo),
				*pMessage, pmh->getFlags() & MessageHolder::FLAG_USER_MASK,
				pmh->getLabel().get(), nAppendFlags, pContext->getUndoItemList(), 0))
				return false;
			
			if (bMove_) {
				unsigned int nRemoveFlags = Account::OPFLAG_ACTIVE |
					(pContext->isBackground() ? Account::OPFLAG_BACKGROUND : Account::REMOVEFLAG_NONE);
				unsigned int nResultFlags = 0;
				if (!pContext->getAccount()->removeMessages(
					MessageHolderList(1, pmh), pContext->getFolder(), nRemoveFlags,
					0, pContext->getUndoItemList(), &nResultFlags))
					return false;
				pContext->setResultFlags(nResultFlags);
			}
		}
	}
	else {
		unsigned int nCopyFlags = Account::OPFLAG_ACTIVE |
			(bMove_ ? Account::COPYFLAG_MOVE : Account::COPYFLAG_NONE);
		if (pContext->isBackground())
			nCopyFlags |= Account::OPFLAG_BACKGROUND;
		unsigned int nResultFlags = 0;
		if (!pContext->getAccount()->copyMessages(pContext->getMessageHolderList(),
			pContext->getFolder(), static_cast<NormalFolder*>(pFolderTo),
			nCopyFlags, 0, pContext->getUndoItemList(), &nResultFlags))
			return false;
		pContext->setResultFlags(nResultFlags);
	}
	
	return true;
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

bool qm::DeleteRuleAction::apply(RuleContext* pContext) const
{
	unsigned int nRemoveFlags = Account::OPFLAG_ACTIVE |
		(bDirect_ ? Account::REMOVEFLAG_DIRECT : Account::REMOVEFLAG_NONE);
	if (pContext->isBackground())
		nRemoveFlags |= Account::OPFLAG_BACKGROUND;
	unsigned int nResultFlags = 0;
	if (!pContext->getAccount()->removeMessages(pContext->getMessageHolderList(),
		pContext->getFolder(), nRemoveFlags, 0, pContext->getUndoItemList(), &nResultFlags))
		return false;
	pContext->setResultFlags(nResultFlags);
	return true;
}

wstring_ptr qm::DeleteRuleAction::getDescription() const
{
	return loadString(Application::getApplication().getResourceHandle(),
		bDirect_ ? IDS_RULEACTION_DELETEDIRECT : IDS_RULEACTION_DELETE);
}

unsigned int qm::DeleteRuleAction::getFlags() const
{
	return FLAG_NONE;
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

qm::LabelRuleAction::LabelRuleAction(Util::LabelType type,
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

Util::LabelType qm::LabelRuleAction::getLabelType() const
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

bool qm::LabelRuleAction::apply(RuleContext* pContext) const
{
	return Util::setMessagesLabel(pContext->getAccount(),
		pContext->getMessageHolderList(), type_,
		wstrLabel_.get(), pContext->getUndoItemList());
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

bool qm::DeleteCacheRuleAction::apply(RuleContext* pContext) const
{
	return pContext->getAccount()->deleteMessagesCache(
		pContext->getMessageHolderList());
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

bool qm::ApplyRuleAction::apply(RuleContext* pContext) const
{
	const MessageHolderList& l = pContext->getMessageHolderList();
	unsigned int nMacroFlags = pContext->getMacroFlags();
	for (MessageHolderList::const_iterator it = l.begin(); it != l.end(); ++it) {
		Message msg;
		MacroContext c(*it, &msg, pContext->getAccount(), MessageHolderList(),
			pContext->getFolder(), pContext->getDocument(), pContext->getActionInvoker(),
			pContext->getWindow(), pContext->getProfile(), 0, nMacroFlags,
			pContext->getSecurityMode(), 0, pContext->getGlobalVariable());
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
							 const ActionInvoker* pActionInvoker,
							 HWND hwnd,
							 Profile* pProfile,
							 MacroVariableHolder* pGlobalVariable,
							 bool bBackground,
							 bool bNew,
							 unsigned int nSecurityMode,
							 UndoItemList* pUndoItemList) :
	listMessageHolder_(l),
	pDocument_(pDocument),
	pAccount_(pAccount),
	pFolder_(pFolder),
	pActionInvoker_(pActionInvoker),
	hwnd_(hwnd),
	pProfile_(pProfile),
	pGlobalVariable_(pGlobalVariable),
	bBackground_(bBackground),
	bNew_(bNew),
	nSecurityMode_(nSecurityMode),
	pUndoItemList_(pUndoItemList),
	nResultFlags_(Account::RESULTFLAG_NONE)
{
	assert(!l.empty());
	assert(pDocument);
	assert(pAccount);
	assert(pAccount->isLocked());
	assert(pFolder);
	assert(pProfile);
	assert(pGlobalVariable);
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

const ActionInvoker* qm::RuleContext::getActionInvoker() const
{
	return pActionInvoker_;
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

bool qm::RuleContext::isBackground() const
{
	return bBackground_;
}

bool qm::RuleContext::isNew() const
{
	return bNew_;
}

unsigned int qm::RuleContext::getMacroFlags() const
{
	return (bBackground_ ? MacroContext::FLAG_NONE :
		MacroContext::FLAG_UITHREAD | MacroContext::FLAG_UI) |
		(bNew_ ? MacroContext::FLAG_NEW : 0);
}

unsigned int qm::RuleContext::getSecurityMode() const
{
	return nSecurityMode_;
}

UndoItemList* qm::RuleContext::getUndoItemList() const
{
	return pUndoItemList_;
}

unsigned int qm::RuleContext::getResultFlags() const
{
	return nResultFlags_;
}

void qm::RuleContext::setResultFlags(unsigned int nResultFlags)
{
	nResultFlags_ |= nResultFlags;
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
	labelType_(Util::LABELTYPE_SET),
	nUse_(0),
	bContinue_(false),
	bEnabled_(true)
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
		
		Term account;
		if (pwszAccount && !account.setValue(pwszAccount))
			return false;
		
		Term folder;
		if (pwszFolder && !folder.setValue(pwszFolder))
			return false;
		
		std::auto_ptr<RuleSet> pSet(new RuleSet(account, folder));
		assert(!pCurrentRuleSet_);
		pCurrentRuleSet_ = pSet.get();
		pManager_->addRuleSet(pSet);
		
		state_ = STATE_RULESET;
	}
	else if (wcscmp(pwszLocalName, L"rule") == 0) {
		if (state_ != STATE_RULESET)
			return false;
		
		bContinue_ = false;
		bEnabled_ = true;
		
		const WCHAR* pwszMatch = 0;
		const WCHAR* pwszUse = 0;
		const WCHAR* pwszDescription = 0;
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrName, L"match") == 0)
				pwszMatch = attributes.getValue(n);
			else if (wcscmp(pwszAttrName, L"use") == 0)
				pwszUse = attributes.getValue(n);
			else if (wcscmp(pwszAttrName, L"continue") == 0)
				bContinue_ = wcscmp(attributes.getValue(n), L"true") == 0;
			else if (wcscmp(pwszAttrName, L"description") == 0)
				pwszDescription = attributes.getValue(n);
			else if (wcscmp(pwszAttrName, L"enabled") == 0)
				bEnabled_ = wcscmp(attributes.getValue(n), L"false") != 0;
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
			if (wcsstr(pwszUse, L"active"))
				nUse_ |= Rule::USE_ACTIVE;
		}
		else {
			nUse_ = Rule::USE_MANUAL | Rule::USE_AUTO;
		}
		
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
		std::auto_ptr<Rule> pRule(new Rule(pCondition_, pAction,
			nUse_, bContinue_, wstrDescription_.get(), bEnabled_));
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
		std::auto_ptr<Rule> pRule(new Rule(pCondition_, pAction,
			nUse_, bContinue_, wstrDescription_.get(), bEnabled_));
		wstrDescription_.reset(0);
		pCurrentRuleSet_->addRule(pRule);
		
		state_ = STATE_DELETE;
	}
	else if (wcscmp(pwszLocalName, L"label") == 0) {
		if (state_ != STATE_RULE)
			return false;
		assert(pCondition_.get());
		
		// TODO
		// Accept label attribute for compatibility.
		// Remove it in the future.
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
		
		Util::LabelType type = Util::LABELTYPE_SET;
		if (pwszType) {
			if (wcscmp(pwszType, L"set") == 0)
				type = Util::LABELTYPE_SET;
			else if (wcscmp(pwszType, L"add") == 0)
				type = Util::LABELTYPE_ADD;
			else if (wcscmp(pwszType, L"remove") == 0)
				type = Util::LABELTYPE_REMOVE;
			else
				return false;
		}
		
		labelType_ = type;
		if (pwszLabel)
			wstrLabel_ = allocWString(pwszLabel);
		
		state_ = STATE_LABEL;
	}
	else if (wcscmp(pwszLocalName, L"deleteCache") == 0) {
		if (state_ != STATE_RULE)
			return false;
		assert(pCondition_.get());
		
		std::auto_ptr<RuleAction> pAction(new DeleteCacheRuleAction());
		std::auto_ptr<Rule> pRule(new Rule(pCondition_, pAction,
			nUse_, bContinue_, wstrDescription_.get(), bEnabled_));
		wstrDescription_.reset(0);
		pCurrentRuleSet_->addRule(pRule);
		
		state_ = STATE_DELETECACHE;
	}
	else if (wcscmp(pwszLocalName, L"apply") == 0) {
		if (state_ != STATE_RULE)
			return false;
		assert(pCondition_.get());
		
		// TODO
		// Accept macro attribute for compatibility.
		// Remove it in the future.
		const WCHAR* pwszMacro = 0;
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrName, L"macro") == 0)
				pwszMacro = attributes.getValue(n);
			else
				return false;
		}
		
		if (pwszMacro)
			wstrApply_ = allocWString(pwszMacro);
		
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
			std::auto_ptr<Rule> pRule(new Rule(pCondition_, pAction,
				nUse_, bContinue_, wstrDescription_.get(), bEnabled_));
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
		
		if (!wstrLabel_.get())
			wstrLabel_ = buffer_.getString();
		
		std::auto_ptr<RuleAction> pAction(new LabelRuleAction(labelType_, wstrLabel_.get()));
		std::auto_ptr<Rule> pRule(new Rule(pCondition_, pAction,
			nUse_, bContinue_, wstrDescription_.get(), bEnabled_));
		labelType_ = Util::LABELTYPE_SET;
		wstrLabel_.reset(0);
		wstrDescription_.reset(0);
		pCurrentRuleSet_->addRule(pRule);
		
		state_ = STATE_RULE;
	}
	else if (wcscmp(pwszLocalName, L"deleteCache") == 0) {
		assert(state_ == STATE_DELETECACHE);
		state_ = STATE_RULE;
	}
	else if (wcscmp(pwszLocalName, L"apply") == 0) {
		assert(state_ == STATE_APPLY);
		
		if (!wstrApply_.get())
			wstrApply_ = buffer_.getString();
		
		std::auto_ptr<Macro> pMacroApply(MacroParser().parse(wstrApply_.get()));
		if (!pMacroApply.get())
			return false;
		
		std::auto_ptr<RuleAction> pAction(new ApplyRuleAction(pMacroApply));
		std::auto_ptr<Rule> pRule(new Rule(pCondition_, pAction,
			nUse_, bContinue_, wstrDescription_.get(), bEnabled_));
		wstrApply_.reset(0);
		wstrDescription_.reset(0);
		pCurrentRuleSet_->addRule(pRule);
		
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
	if (state_ == STATE_ARGUMENT ||
		state_ == STATE_LABEL ||
		state_ == STATE_APPLY) {
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

qm::RuleWriter::RuleWriter(Writer* pWriter,
						   const WCHAR* pwszEncoding) :
	handler_(pWriter, pwszEncoding)
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
		if (pRule->getUse() & Rule::USE_ACTIVE) {
			if (bufUse.getLength() != 0)
				bufUse.append(L' ');
			bufUse.append(L"active");
		}
	}
	const WCHAR* pwszDescription = pRule->getDescription();
	const SimpleAttributes::Item items[] = {
		{ L"match",			wstrCondition.get(),	false					},
		{ L"use",			bufUse.getCharArray(),	bufUse.getLength() == 0	},
		{ L"continue",		L"true",				!pRule->isContinue()	},
		{ L"description",	pwszDescription,		!pwszDescription		},
		{ L"enabled",		L"false",				pRule->isEnabled()		}
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
		return false;
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
	bool bDirect = pAction->isDirect();
	const SimpleAttributes::Item items[] = {
		{ L"direct",	L"true",	!bDirect	}
	};
	SimpleAttributes attrs(items, countof(items));
	return handler_.startElement(0, 0, L"delete", attrs) &&
		handler_.endElement(0, 0, L"delete");
}

bool qm::RuleWriter::write(const LabelRuleAction* pAction)
{
	const WCHAR* pwszType = 0;
	switch (pAction->getLabelType()) {
	case Util::LABELTYPE_SET:
		break;
	case Util::LABELTYPE_ADD:
		pwszType = L"add";
		break;
	case Util::LABELTYPE_REMOVE:
		pwszType = L"remove";
		break;
	default:
		assert(false);
		return false;
	}
	const SimpleAttributes::Item items[] = {
		{ L"type",	pwszType,	pwszType == 0	}
	};
	SimpleAttributes attrs(items, countof(items));
	const WCHAR* pwszLabel = pAction->getLabel();
	return handler_.startElement(0, 0, L"label", attrs) &&
		handler_.characters(pwszLabel, 0, wcslen(pwszLabel)) &&
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
	return handler_.startElement(0, 0, L"apply", DefaultAttributes()) &&
		handler_.characters(wstrMacro.get(), 0, wcslen(wstrMacro.get())) &&
		handler_.endElement(0, 0, L"apply");
}
