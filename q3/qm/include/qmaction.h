/*
 * $Id: qmaction.h,v 1.2 2003/05/14 08:52:16 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
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
	ActionInvoker(const qs::ActionMap* pActionMap, qs::QSTATUS* pstatus);
	~ActionInvoker();

public:
	qs::QSTATUS invoke(const WCHAR* pwszAction,
		VARIANT** ppvarArgs, size_t nArgs) const;

private:
	ActionInvoker(const ActionInvoker&);
	ActionInvoker& operator=(const ActionInvoker&);

private:
	const qs::ActionMap* pActionMap_;
};

}

#endif // __QMACTION_H__
