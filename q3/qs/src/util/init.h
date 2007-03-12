/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __INIT_H__
#define __INIT_H__

#include <qswindow.h>

#include <vector>


namespace qs {

/****************************************************************************
 *
 * MultiModalHandler
 *
 */

class MultiModalHandler : public ModalHandler
{
public:
	MultiModalHandler();
	virtual ~MultiModalHandler();

public:
	void add(ModalHandler* pModalHandler);
	void remove(ModalHandler* pModalHandler);

public:
	virtual void preModalDialog(HWND hwndParent);
	virtual void postModalDialog(HWND hwndParent);

private:
	MultiModalHandler(const MultiModalHandler&);
	MultiModalHandler& operator=(const MultiModalHandler&);

private:
	typedef std::vector<ModalHandler*> HandlerList;

private:
	HandlerList listHandler_;
};

}

#endif // __INIT_H__
