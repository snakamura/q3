/*
 * $Id: messagewindow.h,v 1.1.1.1 2003/04/29 08:07:32 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __MESSAGEWINDOW_H__
#define __MESSAGEWINDOW_H__

#include <qskeymap.h>
#include <qsmenu.h>


namespace qm {

class Document;


/****************************************************************************
 *
 * MessageWindowItem
 *
 */

class MessageWindowItem
{
public:
	virtual ~MessageWindowItem();

public:
	virtual qs::QSTATUS copy() = 0;
	virtual qs::QSTATUS canCopy(bool* pbCan) = 0;
	virtual qs::QSTATUS selectAll() = 0;
	virtual qs::QSTATUS canSelectAll(bool* pbCan) = 0;
};


/****************************************************************************
 *
 * MessageWindowCreateContext
 *
 */

struct MessageWindowCreateContext
{
	Document* pDocument_;
	qs::MenuManager* pMenuManager_;
	qs::KeyMap* pKeyMap_;
};

}

#endif // __MESSAGEWINDOW_H__
