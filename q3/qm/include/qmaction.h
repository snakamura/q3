/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QMACTION_H__
#define __QMACTION_H__

#include <qm.h>

#include <qsaction.h>


namespace qm {

class ActionInvoker;


/****************************************************************************
 *
 * ActionInvoker
 *
 */

class QMEXPORTCLASS ActionInvoker
{
public:
	explicit ActionInvoker(const qs::ActionMap* pActionMap);
	~ActionInvoker();

public:
	void invoke(UINT nId,
				const WCHAR** ppParams,
				size_t nParams) const;
	void invoke(const WCHAR* pwszAction,
				const WCHAR** ppParams,
				size_t nParams) const;

private:
	ActionInvoker(const ActionInvoker&);
	ActionInvoker& operator=(const ActionInvoker&);

private:
	const qs::ActionMap* pActionMap_;
};

}

#endif // __QMACTION_H__
