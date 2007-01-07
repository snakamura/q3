/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#pragma warning(disable:4786)

#include <qsaction.h>
#include <qsstl.h>

#include <algorithm>

using namespace qs;


/****************************************************************************
 *
 * Action
 *
 */

qs::Action::~Action()
{
}


/****************************************************************************
 *
 * ActionEvent
 *
 */

qs::ActionEvent::ActionEvent(unsigned int nId,
							 unsigned int nModifier) :
	nId_(nId),
	nModifier_(nModifier),
	pParam_(0)
{
}

qs::ActionEvent::ActionEvent(unsigned int nId,
							 unsigned int nModifier,
							 const ActionParam* pParam) :
	nId_(nId),
	nModifier_(nModifier),
	pParam_(pParam && pParam->getCount() != 0 ? pParam : 0)
{
}

qs::ActionEvent::~ActionEvent()
{
}

unsigned int qs::ActionEvent::getId() const
{
	return nId_;
}

unsigned int qs::ActionEvent::getModifier() const
{
	return nModifier_;
}

const ActionParam* qs::ActionEvent::getParam() const
{
	return pParam_;
}

unsigned int qs::ActionEvent::getSystemModifiers()
{
	unsigned int nModifier = 0;
	
	struct {
		int nKey_;
		Modifier modifier_;
	} keys[] = {
		{ VK_SHIFT,		MODIFIER_SHIFT	},
		{ VK_CONTROL,	MODIFIER_CTRL	},
		{ VK_MENU,		MODIFIER_ALT	}
	};
	for (int n = 0; n < countof(keys); ++n) {
		if (::GetKeyState(keys[n].nKey_) < 0)
			nModifier |= keys[n].modifier_;
	}
	
	return nModifier;
}


/****************************************************************************
 *
 * ActionParam
 *
 */

qs::ActionParam::ActionParam(unsigned int nBaseId) :
	nBaseId_(nBaseId),
	nRef_(0)
{
}

qs::ActionParam::ActionParam(unsigned int nBaseId,
							 const WCHAR* pwszValue) :
	nBaseId_(nBaseId),
	nRef_(0)
{
	if (pwszValue) {
		listValue_.resize(1);
		listValue_[0] = allocWString(pwszValue).release();
	}
}

qs::ActionParam::ActionParam(unsigned int nBaseId,
							 const WCHAR* pwszValue,
							 bool bParse) :
	nBaseId_(nBaseId),
	nRef_(0)
{
	if (pwszValue) {
		if (bParse) {
			parse(pwszValue, &listValue_);
		}
		else {
			listValue_.resize(1);
			listValue_[0] = allocWString(pwszValue).release();
		}
	}
}

qs::ActionParam::ActionParam(unsigned int nBaseId,
							 const WCHAR** ppwszValue,
							 size_t nCount) :
	nBaseId_(nBaseId),
	nRef_(0)
{
	assert(ppwszValue || nCount == 0);
	
	listValue_.resize(nCount);
	for (size_t n = 0; n < nCount; ++n) {
		assert(*(ppwszValue + n));
		listValue_[n] = allocWString(*(ppwszValue + n)).release();
	}
}

qs::ActionParam::~ActionParam()
{
	std::for_each(listValue_.begin(), listValue_.end(), qs::string_free<WSTRING>());
}

unsigned int qs::ActionParam::getBaseId() const
{
	return nBaseId_;
}

size_t qs::ActionParam::getCount() const
{
	return listValue_.size();
}

const WCHAR* qs::ActionParam::getValue(size_t n) const
{
	assert(n < listValue_.size());
	return listValue_[n];
}

unsigned int qs::ActionParam::addRef()
{
	return ++nRef_;
}

unsigned int qs::ActionParam::release()
{
	return --nRef_;
}

void qs::ActionParam::parse(const WCHAR* pwszValue,
							ValueList* pList)
{
	assert(pwszValue);
	assert(pList);
	assert(pList->empty());
	
	StringBuffer<WSTRING> buf;
	bool bInQuote = false;
	for (const WCHAR* p = pwszValue; *p; ++p) {
		if (*p == L'\"') {
			bInQuote = !bInQuote;
		}
		else if (*p == L'\\' && bInQuote) {
			if (*(p + 1)) {
				++p;
				buf.append(*p);
			}
		}
		else if (*p == L' ' && !bInQuote) {
			wstring_ptr wstr(buf.getString());
			pList->push_back(wstr.get());
			wstr.release();
			
			while (*(p + 1) == L' ')
				++p;
		}
		else {
			buf.append(*p);
		}
	}
	
	if (!*pwszValue || buf.getLength() != 0) {
		wstring_ptr wstr(buf.getString());
		pList->push_back(wstr.get());
		wstr.release();
	}
}

bool qs::operator==(const ActionParam& lhs,
					const ActionParam& rhs)
{
	if (lhs.getCount() != rhs.getCount())
		return false;
	
	for (size_t n = 0; n < lhs.getCount(); ++n) {
		if (wcscmp(lhs.getValue(n), rhs.getValue(n)) != 0)
			return false;
	}
	
	return true;
}

bool qs::operator!=(const ActionParam& lhs,
					const ActionParam& rhs)
{
	return !(lhs == rhs);
}


/****************************************************************************
 *
 * AbstractAction
 *
 */

qs::AbstractAction::AbstractAction()
{
}

qs::AbstractAction::~AbstractAction()
{
}

bool qs::AbstractAction::isEnabled(const ActionEvent& event)
{
	return true;
}

bool qs::AbstractAction::isChecked(const ActionEvent& event)
{
	return false;
}

wstring_ptr qs::AbstractAction::getText(const ActionEvent& event)
{
	return 0;
}


/****************************************************************************
 *
 * ActionMapImpl
 *
 */

struct qs::ActionMapImpl
{
	struct ActionItem
	{
		unsigned int nFrom_;
		unsigned int nTo_;
		Action* pAction_;
	};
	
	struct ActionItemLess : public std::binary_function<ActionItem, ActionItem, bool>
	{
		bool operator()(const ActionItem& lhs,
						const ActionItem& rhs) const;
	};
	
	typedef std::vector<ActionItem> ItemList;
	ItemList listItem_;
};


/****************************************************************************
 *
 * ActionMapImpl::ActionItemLess
 *
 */

bool qs::ActionMapImpl::ActionItemLess::operator()(const ActionItem& lhs,
												   const ActionItem& rhs) const
{
	return lhs.nFrom_ < rhs.nFrom_;
}


/****************************************************************************
 *
 * ActionMap
 *
 */

qs::ActionMap::ActionMap() :
	pImpl_(0)
{
	pImpl_ = new ActionMapImpl();
}

qs::ActionMap::~ActionMap()
{
	std::for_each(pImpl_->listItem_.begin(), pImpl_->listItem_.end(),
		unary_compose_f_gx(
			deleter<Action>(),
			mem_data_ref(&ActionMapImpl::ActionItem::pAction_)));
	delete pImpl_;
}

Action* qs::ActionMap::getAction(unsigned int nId) const
{
	assert(ID_MIN <= nId && nId < ID_MAX);
	
	const ActionMapImpl::ItemList& l = pImpl_->listItem_;
	if (l.empty())
		return 0;
	
	ActionMapImpl::ActionItem item = { nId, nId + 1, 0 };
	ActionMapImpl::ItemList::const_iterator it = std::lower_bound(
		l.begin(), l.end(), item, ActionMapImpl::ActionItemLess());
	if (it == l.end() || ((*it).nFrom_ != nId && it != l.begin()))
		--it;
	if ((*it).nFrom_ <= nId && nId < (*it).nTo_)
		return (*it).pAction_;
	else
		return 0;
}

void qs::ActionMap::addAction(unsigned int nId,
							  std::auto_ptr<Action> pAction)
{
	addAction(nId, nId + 1, pAction);
}

void qs::ActionMap::addAction(unsigned int nIdFrom,
							  unsigned int nIdTo,
							  std::auto_ptr<Action> pAction)
{
	assert(ID_MIN <= nIdFrom && nIdFrom < ID_MAX);
	assert(ID_MIN < nIdTo && nIdTo <= ID_MAX);
	
	ActionMapImpl::ActionItem item = {
		nIdFrom,
		nIdTo,
		pAction.get()
	};
	ActionMapImpl::ItemList& l = pImpl_->listItem_;
	ActionMapImpl::ItemList::iterator it = std::lower_bound(
		l.begin(), l.end(), item, ActionMapImpl::ActionItemLess());
	pImpl_->listItem_.insert(it, item);
	pAction.release();
}


/****************************************************************************
 *
 * ActionParamMapImpl
 *
 */

struct qs::ActionParamMapImpl
{
	struct Item
	{
		unsigned int nId_;
		unsigned int nBaseId_;
		ActionParam* pParam_;
	};
	
	struct ItemLess : public std::binary_function<Item, Item, bool>
	{
		bool operator()(const Item& lhs,
						const Item& rhs) const;
	};
	
	struct ItemBaseLess : public std::binary_function<Item, Item, bool>
	{
		bool operator()(const Item& lhs,
						const Item& rhs) const;
	};
	
	typedef std::vector<Item> ItemList;
	ItemList listItem_;
};


/****************************************************************************
 *
 * ActionParamMapImpl::ItemLess
 *
 */

bool qs::ActionParamMapImpl::ItemLess::operator()(const Item& lhs,
												  const Item& rhs) const
{
	return lhs.nId_ < rhs.nId_;
}


/****************************************************************************
 *
 * ActionParamMapImpl::ItemBaseLess
 *
 */

bool qs::ActionParamMapImpl::ItemBaseLess::operator()(const Item& lhs,
													  const Item& rhs) const
{
	return lhs.nBaseId_ < rhs.nBaseId_;
}


/****************************************************************************
 *
 * ActionParamMap
 *
 */

qs::ActionParamMap::ActionParamMap() :
	pImpl_(0)
{
	pImpl_ = new ActionParamMapImpl();
}

qs::ActionParamMap::~ActionParamMap()
{
	std::for_each(pImpl_->listItem_.begin(), pImpl_->listItem_.end(),
		unary_compose_f_gx(
			deleter<ActionParam>(),
			mem_data_ref(&ActionParamMapImpl::Item::pParam_)));
	delete pImpl_;
}

const ActionParam* qs::ActionParamMap::getActionParam(unsigned int nId) const
{
	ActionParamMapImpl::Item item = { nId, 0, 0 };
	ActionParamMapImpl::ItemList::const_iterator it = std::lower_bound(
		pImpl_->listItem_.begin(), pImpl_->listItem_.end(),
		item, ActionParamMapImpl::ItemLess());
	if (it == pImpl_->listItem_.end() || (*it).nId_ != nId)
		return 0;
	return (*it).pParam_;
}

unsigned int qs::ActionParamMap::addActionParam(unsigned int nMaxParamCount,
												std::auto_ptr<ActionParam> pParam)
{
	unsigned int nBaseId = pParam->getBaseId();
	
	ActionParamMapImpl::Item item = { 0, nBaseId, 0 };
	ActionParamMapImpl::ItemList::iterator it = std::lower_bound(
		pImpl_->listItem_.begin(), pImpl_->listItem_.end(),
		item, ActionParamMapImpl::ItemBaseLess());
	if (it != pImpl_->listItem_.end() && (*it).nBaseId_ == nBaseId) {
		while (it != pImpl_->listItem_.end() && (*it).nBaseId_ == nBaseId) {
			if (*(*it).pParam_ == *pParam.get()) {
				(*it).pParam_->addRef();
				return (*it).nId_;
			}
			++it;
		}
	}
	
	unsigned int nId = nBaseId + 1;
	if (!pImpl_->listItem_.empty() && (*(it - 1)).nBaseId_ == nBaseId)
		nId = (*(it - 1)).nId_ + 1;
	if (nId - nBaseId >= nMaxParamCount)
		return -1;
	
	ActionParamMapImpl::Item itemNew = { nId, nBaseId, pParam.get() };
	pImpl_->listItem_.insert(it, itemNew);
	pParam->addRef();
	pParam.release();
	
	return nId;
}

void qs::ActionParamMap::removeActionParam(unsigned int nId)
{
	ActionParamMapImpl::Item item = { nId, 0, 0 };
	ActionParamMapImpl::ItemList::iterator it = std::lower_bound(
		pImpl_->listItem_.begin(), pImpl_->listItem_.end(),
		item, ActionParamMapImpl::ItemLess());
	if (it != pImpl_->listItem_.end() && (*it).nId_ == nId) {
		if ((*it).pParam_->release() == 0) {
			delete (*it).pParam_;
			pImpl_->listItem_.erase(it);
		}
	}
}
