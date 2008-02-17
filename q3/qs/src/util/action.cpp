/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#pragma warning(disable:4786)

#include <qsaction.h>
#include <qsstl.h>
#include <qsthread.h>

#include <algorithm>

#include <boost/bind.hpp>

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
	nBaseId_(nBaseId)
{
}

qs::ActionParam::ActionParam(unsigned int nBaseId,
							 const WCHAR* pwszValue) :
	nBaseId_(nBaseId)
{
	if (pwszValue) {
		listValue_.resize(1);
		listValue_[0] = allocWString(pwszValue).release();
	}
}

qs::ActionParam::ActionParam(unsigned int nBaseId,
							 const WCHAR* pwszValue,
							 bool bParse) :
	nBaseId_(nBaseId)
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
	nBaseId_(nBaseId)
{
	assert(ppwszValue || nCount == 0);
	
	listValue_.resize(nCount);
	for (size_t n = 0; n < nCount; ++n) {
		assert(*(ppwszValue + n));
		listValue_[n] = allocWString(*(ppwszValue + n)).release();
	}
}

qs::ActionParam::ActionParam(const ActionParam& param) :
	nBaseId_(param.nBaseId_)
{
	listValue_.resize(param.listValue_.size());
	for (ValueList::size_type n = 0; n < param.listValue_.size(); ++n)
		listValue_[n] = allocWString(param.listValue_[n]).release();
}

qs::ActionParam::~ActionParam()
{
	std::for_each(listValue_.begin(), listValue_.end(), &freeWString);
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
	
	typedef std::vector<ActionItem> ItemList;
	ItemList listItem_;
};


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
		boost::bind(boost::checked_deleter<Action>(),
			boost::bind(&ActionMapImpl::ActionItem::pAction_, _1)));
	delete pImpl_;
}

Action* qs::ActionMap::getAction(unsigned int nId) const
{
	assert(ID_MIN <= nId && nId < ID_MAX);
	
	const ActionMapImpl::ItemList& l = pImpl_->listItem_;
	if (l.empty())
		return 0;
	
	ActionMapImpl::ActionItem item = {
		nId,
		nId + 1,
		0
	};
	ActionMapImpl::ItemList::const_iterator it = std::lower_bound(
		l.begin(), l.end(), item,
		boost::bind(&ActionMapImpl::ActionItem::nFrom_, _1) <
		boost::bind(&ActionMapImpl::ActionItem::nFrom_, _2));
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
	ActionMapImpl::ItemList::iterator it = std::lower_bound(
		pImpl_->listItem_.begin(), pImpl_->listItem_.end(), item,
		boost::bind(&ActionMapImpl::ActionItem::nFrom_, _1) <
		boost::bind(&ActionMapImpl::ActionItem::nFrom_, _2));
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
		unsigned int nRef_;
	};
	
	typedef std::vector<Item> ItemList;
	ItemList listItem_;
	CriticalSection cs_;
};


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
		boost::bind(boost::checked_deleter<ActionParam>(),
			boost::bind(&ActionParamMapImpl::Item::pParam_, _1)));
	delete pImpl_;
}

std::auto_ptr<ActionParam> qs::ActionParamMap::getActionParam(unsigned int nId) const
{
	Lock<CriticalSection> lock(pImpl_->cs_);
	
	ActionParamMapImpl::Item item = {
		nId,
		0,
		0,
		0
	};
	ActionParamMapImpl::ItemList::const_iterator it = std::lower_bound(
		pImpl_->listItem_.begin(), pImpl_->listItem_.end(), item,
		boost::bind(&ActionParamMapImpl::Item::nId_, _1) <
		boost::bind(&ActionParamMapImpl::Item::nId_, _2));
	if (it == pImpl_->listItem_.end() || (*it).nId_ != nId)
		return std::auto_ptr<ActionParam>();
	return std::auto_ptr<ActionParam>(new ActionParam(*(*it).pParam_));
}

unsigned int qs::ActionParamMap::addActionParam(unsigned int nMaxParamCount,
												std::auto_ptr<ActionParam> pParam)
{
	Lock<CriticalSection> lock(pImpl_->cs_);
	
	unsigned int nBaseId = pParam->getBaseId();
	
	ActionParamMapImpl::Item item = {
		0,
		nBaseId,
		0,
		0
	};
	ActionParamMapImpl::ItemList::iterator it = std::lower_bound(
		pImpl_->listItem_.begin(), pImpl_->listItem_.end(), item,
		boost::bind(&ActionParamMapImpl::Item::nBaseId_, _1) <
		boost::bind(&ActionParamMapImpl::Item::nBaseId_, _2));
	if (it != pImpl_->listItem_.end() && (*it).nBaseId_ == nBaseId) {
		while (it != pImpl_->listItem_.end() && (*it).nBaseId_ == nBaseId) {
			if (*(*it).pParam_ == *pParam.get()) {
				++(*it).nRef_;
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
	
	ActionParamMapImpl::Item itemNew = {
		nId,
		nBaseId,
		pParam.get(),
		1
	};
	pImpl_->listItem_.insert(it, itemNew);
	pParam.release();
	
	return nId;
}

void qs::ActionParamMap::removeActionParam(unsigned int nId)
{
	Lock<CriticalSection> lock(pImpl_->cs_);
	
	ActionParamMapImpl::Item item = {
		nId,
		0,
		0,
		0
	};
	ActionParamMapImpl::ItemList::iterator it = std::lower_bound(
		pImpl_->listItem_.begin(), pImpl_->listItem_.end(), item,
		boost::bind(&ActionParamMapImpl::Item::nId_, _1) <
		boost::bind(&ActionParamMapImpl::Item::nId_, _2));
	if (it != pImpl_->listItem_.end() && (*it).nId_ == nId) {
		if (--(*it).nRef_ == 0) {
			delete (*it).pParam_;
			pImpl_->listItem_.erase(it);
		}
	}
}
