/*
 * $Id: messageselectionmodel.h,v 1.1.1.1 2003/04/29 08:07:32 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
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
	virtual qs::QSTATUS getSelectedMessages(
		Folder** ppFolder, MessagePtrList* pList) = 0;
	virtual qs::QSTATUS hasSelectedMessage(bool* pbHas) = 0;
	virtual qs::QSTATUS getFocusedMessage(MessagePtr* pptr) = 0;
	virtual qs::QSTATUS hasFocusedMessage(bool* pbHas) = 0;
	virtual qs::QSTATUS selectAll() = 0;
	virtual qs::QSTATUS canSelect(bool* pbCan) = 0;
};

}

#endif // __MESSAGESELECTIONMODEL_H__
