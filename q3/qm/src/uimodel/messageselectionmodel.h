/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
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

class MessageSelectionModel;

class AccountLock;
class Folder;
class MessageEnumerator;


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
	virtual void getSelectedMessageHolders(AccountLock* pAccountLock,
										   Folder** ppFolder,
										   MessageHolderList* pList) = 0;
	virtual bool hasSelectedMessageHolders() = 0;
	virtual MessagePtr getFocusedMessagePtr() = 0;
	virtual bool hasFocusedMessagePtr() = 0;
	virtual std::auto_ptr<MessageEnumerator> getSelectedMessages() = 0;
	virtual bool hasSelectedMessages() = 0;
	virtual std::auto_ptr<MessageEnumerator> getFocusedMessage() = 0;
	virtual bool hasFocusedMessage() = 0;
	virtual void selectAll() = 0;
	virtual bool canSelect() = 0;
};

}

#endif // __MESSAGESELECTIONMODEL_H__
