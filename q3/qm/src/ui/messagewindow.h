/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __MESSAGEWINDOW_H__
#define __MESSAGEWINDOW_H__

#include <qskeymap.h>
#include <qsmenu.h>


namespace qm {

class Document;
class SecurityModel;


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
	virtual void copy() = 0;
	virtual bool canCopy() = 0;
	virtual void selectAll() = 0;
	virtual bool canSelectAll() = 0;
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
	SecurityModel* pSecurityModel_;
};

}

#endif // __MESSAGEWINDOW_H__
