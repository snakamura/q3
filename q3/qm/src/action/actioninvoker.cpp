/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#pragma warning(disable:4786)

#include <qmaction.h>

#include <qsaction.h>

#include <algorithm>

#include "../ui/actionitem.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * ActionInvoker
 *
 */

qm::ActionInvoker::ActionInvoker(const ActionMap* pActionMap) :
	pActionMap_(pActionMap)
{
}

qm::ActionInvoker::~ActionInvoker()
{
}

void qm::ActionInvoker::invoke(UINT nId,
							   const WCHAR** ppParams,
							   size_t nParams) const
{
	Action* pAction = pActionMap_->getAction(nId);
	if (pAction) {
		ActionParam param(nId, ppParams, nParams);
		ActionEvent event(nId, 0, nParams != 0 ? &param : 0);
		bool bEnabled = pAction->isEnabled(event);
		if (bEnabled)
			pAction->invoke(event);
	}
}

void qm::ActionInvoker::invoke(const WCHAR* pwszAction,
							   const WCHAR** ppParams,
							   size_t nParams) const
{
	assert(pwszAction);
	
	ActionItem item = {
		pwszAction,
		0
	};
	
	const ActionItem* pItem = std::lower_bound(
		actionItems, actionItems + countof(actionItems), item,
		binary_compose_f_gx_hy(
			string_less<WCHAR>(),
			mem_data_ref(&ActionItem::pwszAction_),
			mem_data_ref(&ActionItem::pwszAction_)));
	if (pItem != actionItems + countof(actionItems) &&
		wcscmp(pItem->pwszAction_, pwszAction) == 0)
		invoke(pItem->nId_, ppParams, nParams);
}
