/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __MESSAGESELECTIONMODEL_H__
#define __MESSAGESELECTIONMODEL_H__

#include <qm.h>
#include <qmmessageholder.h>

#include <qs.h>

#include <vector>


namespace qm {

class AccountLock;
class Folder;


/****************************************************************************
 *
 * MessageSelectionModel
 *
 */

class MessageSelectionModel
{
public:
	virtual ~MessageSelectionModel();

public:
	virtual void getSelectedMessages(AccountLock* pAccountLock,
									 Folder** ppFolder,
									 MessageHolderList* pList) = 0;
	virtual bool hasSelectedMessage() = 0;
	virtual MessagePtr getFocusedMessage() = 0;
	virtual bool hasFocusedMessage() = 0;
	virtual void selectAll() = 0;
	virtual bool canSelect() = 0;
};

}

#endif // __MESSAGESELECTIONMODEL_H__
