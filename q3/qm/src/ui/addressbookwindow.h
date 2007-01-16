/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __ADDRESSBOOKWINDOW_H__
#define __ADDRESSBOOKWINDOW_H__

#include <qm.h>

#include <qsprofile.h>
#include <qsthread.h>

#include "../uimodel/addressbookmodel.h"


namespace qm {

class AddressBookFrameWindowManager;
struct AddressBookFrameWindowCreateContext;
struct AddressBookListWindowCreateContext;

class AddressBook;
class AddressBookFrameWindow;
class UIManager;


/****************************************************************************
 *
 * AddressBookFrameWindowManager
 *
 */

class AddressBookFrameWindowManager : public DefaultAddressBookModelHandler
{
public:
	AddressBookFrameWindowManager(AddressBook* pAddressBook,
								  UIManager* pUIManager,
								  qs::Profile* pProfile);
	virtual ~AddressBookFrameWindowManager();

public:
	void open();
	bool closeAll();
	void showAll();
	void hideAll();
	void reloadProfiles();

public:
	void close(AddressBookFrameWindow* pFrameWindow);

public:
	virtual void saved(const AddressBookModelEvent& event);

private:
	AddressBookFrameWindowManager(const AddressBookFrameWindowManager&);
	AddressBookFrameWindowManager& operator=(const AddressBookFrameWindowManager&);

private:
	AddressBook* pAddressBook_;
	UIManager* pUIManager_;
	qs::Profile* pProfile_;
	qs::Synchronizer* pSynchronizer_;
	qs::Thread* pThread_;
	AddressBookFrameWindow* pFrameWindow_;
	bool bClosing_;
	qs::CriticalSection cs_;
};


/****************************************************************************
 *
 * AddressBookFrameWindowCreateContext
 *
 */

struct AddressBookFrameWindowCreateContext
{
	AddressBookModel* pAddressBookModel_;
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
