/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __ADDRESSBOOKWINDOW_H__
#define __ADDRESSBOOKWINDOW_H__

#include <qm.h>

#include <qsprofile.h>
#include <qsthread.h>


namespace qm {

class AddressBookFrameWindowManager;
struct AddressBookFrameWindowCreateContext;
struct AddressBookListWindowCreateContext;

class AddressBook;
class AddressBookFrameWindow;
class AddressBookModel;
class UIManager;


/****************************************************************************
 *
 * AddressBookFrameWindowManager
 *
 */

class AddressBookFrameWindowManager
{
public:
	AddressBookFrameWindowManager(UIManager* pUIManager,
								  qs::Profile* pProfile);
	~AddressBookFrameWindowManager();

public:
	void open();

public:
	void close();

private:
	AddressBookFrameWindowManager(const AddressBookFrameWindowManager&);
	AddressBookFrameWindowManager& operator=(const AddressBookFrameWindowManager&);

private:
	UIManager* pUIManager_;
	qs::Profile* pProfile_;
	AddressBookFrameWindow* pFrameWindow_;
	qs::CriticalSection cs_;
};


/****************************************************************************
 *
 * AddressBookFrameWindowCreateContext
 *
 */

struct AddressBookFrameWindowCreateContext
{
	UIManager* pUIManager_;
};


/****************************************************************************
 *
 * AddressBookListWindowCreateContext
 *
 */

struct AddressBookListWindowCreateContext
{
	UIManager* pUIManager_;
};

}

#endif // __ADDRESSBOOKWINDOW_H__
