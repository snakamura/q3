/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsaction.h>
#include <qsstl.h>

#include <algorithm>

#pragma warning(disable:4786)

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
							 void* pParam) :
	nId_(nId),
	nModifier_(nModifier),
	pParam_(pParam)
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

void* qs::ActionEvent::getParam() const
{
	return pParam_;
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
