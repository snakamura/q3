/*
 * $Id: action.cpp,v 1.3 2003/05/14 08:52:17 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsaction.h>
#include <qsnew.h>
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

qs::ActionEvent::ActionEvent(unsigned int nId, unsigned int nModifier) :
	nId_(nId),
	nModifier_(nModifier),
	pParam_(0)
{
}

qs::ActionEvent::ActionEvent(unsigned int nId,
	unsigned int nModifier, void* pParam) :
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

QSTATUS qs::AbstractAction::isEnabled(
	const ActionEvent& event, bool* pbEnabled)
{
	assert(pbEnabled);
	*pbEnabled = true;
	return QSTATUS_SUCCESS;
}

QSTATUS qs::AbstractAction::isChecked(
	const ActionEvent& event, bool* pbChecked)
{
	assert(pbChecked);
	*pbChecked = false;
	return QSTATUS_SUCCESS;
}

QSTATUS qs::AbstractAction::getText(
	const ActionEvent& event, WSTRING* pwstrText)
{
	assert(pwstrText);
	*pwstrText = 0;
	return QSTATUS_SUCCESS;
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
	
	struct ActionItemLess :
		public std::binary_function<ActionItem, ActionItem, bool>
	{
		bool operator()(const ActionItem& lhs, const ActionItem& rhs) const;
	};
	
	typedef std::vector<ActionItem> ItemList;
	ItemList listItem_;
};


/****************************************************************************
 *
 * ActionMapImpl::ActionItemLess
 *
 */

bool qs::ActionMapImpl::ActionItemLess::operator()(
	const ActionItem& lhs, const ActionItem& rhs) const
{
	return lhs.nFrom_ < rhs.nFrom_;
}


/****************************************************************************
 *
 * ActionMap
 *
 */

qs::ActionMap::ActionMap(QSTATUS* pstatus) :
	pImpl_(0)
{
	assert(pstatus);
	
	DECLARE_QSTATUS();
	
	*pstatus = QSTATUS_SUCCESS;
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
}

qs::ActionMap::~ActionMap()
{
	ActionMapImpl::ItemList& l = pImpl_->listItem_;
	ActionMapImpl::ItemList::iterator it = l.begin();
	while (it != l.end())
		delete (*it++).pAction_;
	delete pImpl_;
}

Action* qs::ActionMap::getAction(unsigned int nId) const
{
	ActionMapImpl::ActionItem item = { nId, nId + 1, 0 };
	const ActionMapImpl::ItemList& l = pImpl_->listItem_;
	ActionMapImpl::ItemList::const_iterator it = std::lower_bound(
		l.begin(), l.end(), item, ActionMapImpl::ActionItemLess());
	if (it == l.end() || ((*it).nFrom_ != nId && it != l.begin()))
		--it;
	if ((*it).nFrom_ <= nId && nId < (*it).nTo_)
		return (*it).pAction_;
	else
		return 0;
}

QSTATUS qs::ActionMap::addAction(unsigned int nId, Action* pAction)
{
	return addAction(nId, nId + 1, pAction);
}

QSTATUS qs::ActionMap::addAction(unsigned int nIdFrom,
	unsigned int nIdTo, Action* pAction)
{
	ActionMapImpl::ActionItem item = { nIdFrom, nIdTo, pAction };
	ActionMapImpl::ItemList& l = pImpl_->listItem_;
	ActionMapImpl::ItemList::iterator it = std::lower_bound(
		l.begin(), l.end(), item, ActionMapImpl::ActionItemLess());
	ActionMapImpl::ItemList::iterator p;
	return STLWrapper<ActionMapImpl::ItemList>(
		pImpl_->listItem_).insert(it, item, &p);
}
