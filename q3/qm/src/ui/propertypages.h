/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __PROPERTYPAGES_H__
#define __PROPERTYPAGES_H__

#include <qm.h>
#include <qmmessageholderlist.h>

#include <qs.h>
#include <qsdialog.h>


namespace qm {

class DefaultPropertyPage;
	class MessagePropertyPage;


/****************************************************************************
 *
 * DefaultPropertyPage
 *
 */

class DefaultPropertyPage : public qs::DefaultPropertyPage
{
protected:
	explicit DefaultPropertyPage(UINT nId);

public:
	virtual ~DefaultPropertyPage();

private:
	DefaultPropertyPage(const DefaultPropertyPage&);
	DefaultPropertyPage& operator=(const DefaultPropertyPage&);
};


/****************************************************************************
 *
 * MessagePropertyPage
 *
 */

class MessagePropertyPage : public DefaultPropertyPage
{
public:
	explicit MessagePropertyPage(const MessageHolderList& l);
	virtual ~MessagePropertyPage();

public:
	unsigned int getFlags() const;
	unsigned int getMask() const;

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

protected:
	virtual LRESULT onOk();

private:
	MessagePropertyPage(const MessagePropertyPage&);
	MessagePropertyPage& operator=(const MessagePropertyPage&);

private:
	const MessageHolderList& listMessage_;
	unsigned int nFlags_;
	unsigned int nMask_;
};

}

#endif // __PROPERTYPAGES_H__
