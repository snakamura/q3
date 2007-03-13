/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __ACTIONINVOKER_H__
#define __ACTIONINVOKER_H__

#include <qm.h>

#include <qs.h>

#include <vector>


namespace qm {

class ActionInvokerQueue;
class ActionInvokerQueueItem;


/****************************************************************************
 *
 * ActionInvokerQueue
 *
 */

class ActionInvokerQueue
{
public:
	ActionInvokerQueue();
	~ActionInvokerQueue();

public:
	void add(std::auto_ptr<ActionInvokerQueueItem> pItem);
	std::auto_ptr<ActionInvokerQueueItem> remove();
	bool isEmpty() const;

private:
	ActionInvokerQueue(const ActionInvokerQueue&);
	ActionInvokerQueue& operator=(const ActionInvokerQueue&);

private:
	typedef std::vector<ActionInvokerQueueItem*> ItemList;

private:
	ItemList listItem_;
};


/****************************************************************************
 *
 * ActionInvokerQueueItem
 *
 */

class ActionInvokerQueueItem
{
public:
	ActionInvokerQueueItem(UINT nId,
						   const WCHAR** ppParams,
						   size_t nParams);
	~ActionInvokerQueueItem();

public:
	UINT getId() const;
	const WCHAR** getParams() const;
	size_t getParamCount() const;

private:
	ActionInvokerQueueItem(const ActionInvokerQueueItem&);
	ActionInvokerQueueItem& operator=(const ActionInvokerQueueItem&);

private:
	typedef std::vector<qs::WSTRING> ParamList;

private:
	UINT nId_;
	ParamList listParam_;
};

}

#endif // __ACTIONINVOKER_H__
