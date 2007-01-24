/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __ADDRESSBOOKACTION_H__
#define __ADDRESSBOOKACTION_H__

#include <qm.h>

#include <qsaction.h>

#include "../uimodel/addressbookmodel.h"


namespace qm {

class AddressBookAddressCreateMessageAction;
class AddressBookAddressDeleteAction;
class AddressBookAddressEditAction;
class AddressBookAddressNewAction;
class AddressBookFileSaveAction;
class AddressBookViewSortAction;

class AddressBookSelectionModel;


/****************************************************************************
 *
 * AddressBookAddressCreateMessageAction
 *
 */

class AddressBookAddressCreateMessageAction : public qs::AbstractAction
{
public:
	explicit AddressBookAddressCreateMessageAction(HWND hwnd);
	virtual ~AddressBookAddressCreateMessageAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	AddressBookAddressCreateMessageAction(const AddressBookAddressCreateMessageAction&);
	AddressBookAddressCreateMessageAction& operator=(const AddressBookAddressCreateMessageAction&);

private:
	HWND hwnd_;
};


/****************************************************************************
 *
 * AddressBookAddressDeleteAction
 *
 */

class AddressBookAddressDeleteAction : public qs::AbstractAction
{
public:
	AddressBookAddressDeleteAction(AddressBookModel* pAddressBookModel,
								   AddressBookSelectionModel* pAddressBookSelectionModel);
	virtual ~AddressBookAddressDeleteAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	AddressBookAddressDeleteAction(const AddressBookAddressDeleteAction&);
	AddressBookAddressDeleteAction& operator=(const AddressBookAddressDeleteAction&);

private:
	AddressBookModel* pAddressBookModel_;
	AddressBookSelectionModel* pAddressBookSelectionModel_;
};


/****************************************************************************
 *
 * AddressBookAddressEditAction
 *
 */

class AddressBookAddressEditAction : public qs::AbstractAction
{
public:
	AddressBookAddressEditAction(AddressBookModel* pAddressBookModel,
								 AddressBookSelectionModel* pAddressBookSelectionModel,
								 HWND hwnd);
	virtual ~AddressBookAddressEditAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isEnabled(const qs::ActionEvent& event);

private:
	AddressBookAddressEditAction(const AddressBookAddressEditAction&);
	AddressBookAddressEditAction& operator=(const AddressBookAddressEditAction&);

private:
	AddressBookModel* pAddressBookModel_;
	AddressBookSelectionModel* pAddressBookSelectionModel_;
	HWND hwnd_;
};


/****************************************************************************
 *
 * AddressBookAddressNewAction
 *
 */

class AddressBookAddressNewAction : public qs::AbstractAction
{
public:
	AddressBookAddressNewAction(AddressBookModel* pAddressBookModel,
								HWND hwnd);
	virtual ~AddressBookAddressNewAction();

public:
	virtual void invoke(const qs::ActionEvent& event);

private:
	AddressBookAddressNewAction(const AddressBookAddressNewAction&);
	AddressBookAddressNewAction& operator=(const AddressBookAddressNewAction&);

private:
	AddressBookModel* pAddressBookModel_;
	HWND hwnd_;
};


/****************************************************************************
 *
 * AddressBookFileSaveAction
 *
 */

class AddressBookFileSaveAction : public qs::AbstractAction
{
public:
	AddressBookFileSaveAction(AddressBookModel* pAddressBookModel,
							  HWND hwnd);
	virtual ~AddressBookFileSaveAction();

public:
	virtual void invoke(const qs::ActionEvent& event);

private:
	AddressBookFileSaveAction(const AddressBookFileSaveAction&);
	AddressBookFileSaveAction& operator=(const AddressBookFileSaveAction&);

private:
	AddressBookModel* pAddressBookModel_;
	HWND hwnd_;
};


/****************************************************************************
 *
 * AddressBookViewRefreshAction
 *
 */

class AddressBookViewRefreshAction : public qs::AbstractAction
{
public:
	explicit AddressBookViewRefreshAction(AddressBookModel* pAddressBookModel);
	virtual ~AddressBookViewRefreshAction();

public:
	virtual void invoke(const qs::ActionEvent& event);

private:
	AddressBookViewRefreshAction(const AddressBookViewRefreshAction&);
	AddressBookViewRefreshAction& operator=(const AddressBookViewRefreshAction&);

private:
	AddressBookModel* pAddressBookModel_;
};


/****************************************************************************
 *
 * AddressBookViewSortAction
 *
 */

class AddressBookViewSortAction : public qs::AbstractAction
{
public:
	AddressBookViewSortAction(AddressBookModel* pAddressBookModel,
							  AddressBookModel::Sort sort_,
							  AddressBookModel::Sort mask_);
	virtual ~AddressBookViewSortAction();

public:
	virtual void invoke(const qs::ActionEvent& event);
	virtual bool isChecked(const qs::ActionEvent& event);

private:
	AddressBookViewSortAction(const AddressBookViewSortAction&);
	AddressBookViewSortAction& operator=(const AddressBookViewSortAction&);

private:
	AddressBookModel* pAddressBookModel_;
	AddressBookModel::Sort sort_;
	AddressBookModel::Sort mask_;
};

}

#endif // __ADDRESSBOOKACTION_H__
