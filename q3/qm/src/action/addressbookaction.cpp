/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
 * All rights reserved.
 *
 */

#pragma warning(disable:4786)

#include <qstextutil.h>

#include "action.h"
#include "addressbookaction.h"
#include "../model/addressbook.h"
#include "../ui/addressbookdialog.h"
#include "../ui/dialogs.h"
#include "../ui/resourceinc.h"
#include "../ui/uiutil.h"
#include "../uimodel/addressbookselectionmodel.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * AddressBookAddressCreateMessageAction
 *
 */

qm::AddressBookAddressCreateMessageAction::AddressBookAddressCreateMessageAction(Profile* pProfile,
																				 HWND hwnd) :
	pProfile_(pProfile),
	hwnd_(hwnd)
{
}

qm::AddressBookAddressCreateMessageAction::~AddressBookAddressCreateMessageAction()
{
}

void qm::AddressBookAddressCreateMessageAction::invoke(const ActionEvent& event)
{
	const WCHAR* pwszAddress = ActionParamUtil::getString(event.getParam(), 0);
	if (!pwszAddress)
		return;
	
	wstring_ptr wstrURI(concat(L"mailto:", TextUtil::escapeIURIComponent(pwszAddress).get()));
	UIUtil::openURL(wstrURI.get(), pProfile_, hwnd_);
}

bool qm::AddressBookAddressCreateMessageAction::isEnabled(const qs::ActionEvent& event)
{
	return ActionParamUtil::getString(event.getParam(), 0) != 0;
}


/****************************************************************************
 *
 * AddressBookAddressDeleteAction
 *
 */

qm::AddressBookAddressDeleteAction::AddressBookAddressDeleteAction(AddressBookModel* pAddressBookModel,
																   AddressBookSelectionModel* pAddressBookSelectionModel) :
	pAddressBookModel_(pAddressBookModel),
	pAddressBookSelectionModel_(pAddressBookSelectionModel)
{
}

qm::AddressBookAddressDeleteAction::~AddressBookAddressDeleteAction()
{
}

void qm::AddressBookAddressDeleteAction::invoke(const ActionEvent& event)
{
	typedef AddressBookSelectionModel::ItemList ItemList;
	ItemList listItem;
	pAddressBookSelectionModel_->getSelectedItems(&listItem);
	for (ItemList::reverse_iterator it = listItem.rbegin(); it != listItem.rend(); ++it) {
		const AddressBookEntry* pEntry = pAddressBookModel_->getEntry(*it);
		if (!pEntry->isExternal())
			pAddressBookModel_->remove(*it);
	}
}

bool qm::AddressBookAddressDeleteAction::isEnabled(const ActionEvent& event)
{
	typedef AddressBookSelectionModel::ItemList ItemList;
	ItemList listItem;
	pAddressBookSelectionModel_->getSelectedItems(&listItem);
	for (ItemList::reverse_iterator it = listItem.rbegin(); it != listItem.rend(); ++it) {
		const AddressBookEntry* pEntry = pAddressBookModel_->getEntry(*it);
		if (!pEntry->isExternal())
			return true;
	}
	return false;
}


/****************************************************************************
 *
 * AddressBookAddressEditAction
 *
 */

qm::AddressBookAddressEditAction::AddressBookAddressEditAction(AddressBookModel* pAddressBookModel,
															   AddressBookSelectionModel* pAddressBookSelectionModel,
															   HWND hwnd) :
	pAddressBookModel_(pAddressBookModel),
	pAddressBookSelectionModel_(pAddressBookSelectionModel),
	hwnd_(hwnd)
{
}

qm::AddressBookAddressEditAction::~AddressBookAddressEditAction()
{
}

void qm::AddressBookAddressEditAction::invoke(const ActionEvent& event)
{
	unsigned int nItem = pAddressBookSelectionModel_->getFocusedItem();
	if (nItem == -1)
		return;
	
	std::auto_ptr<AddressBookEntry> pEntry(new AddressBookEntry(
		*pAddressBookModel_->getEntry(nItem)));
	AddressBookEntryDialog dialog(pAddressBookModel_->getAddressBook(), pEntry.get());
	if (dialog.doModal(hwnd_) == IDOK && !pEntry->isExternal())
		pAddressBookModel_->edit(nItem, pEntry);
}

bool qm::AddressBookAddressEditAction::isEnabled(const ActionEvent& event)
{
	return pAddressBookSelectionModel_->getFocusedItem() != -1;
}


/****************************************************************************
 *
 * AddressBookAddressNewAction
 *
 */

qm::AddressBookAddressNewAction::AddressBookAddressNewAction(AddressBookModel* pAddressBookModel,
															 HWND hwnd) :
	pAddressBookModel_(pAddressBookModel),
	hwnd_(hwnd)
{
}

qm::AddressBookAddressNewAction::~AddressBookAddressNewAction()
{
}

void qm::AddressBookAddressNewAction::invoke(const qs::ActionEvent& event)
{
	std::auto_ptr<AddressBookEntry> pEntry(new AddressBookEntry(L"", 0, false));
	AddressBookEntryDialog dialog(pAddressBookModel_->getAddressBook(), pEntry.get());
	if (dialog.doModal(hwnd_) == IDOK)
		pAddressBookModel_->add(pEntry);
}


/****************************************************************************
 *
 * AddressBookFileSaveAction
 *
 */

qm::AddressBookFileSaveAction::AddressBookFileSaveAction(AddressBookModel* pAddressBookModel,
														 HWND hwnd) :
	pAddressBookModel_(pAddressBookModel),
	hwnd_(hwnd)
{
}

qm::AddressBookFileSaveAction::~AddressBookFileSaveAction()
{
}

void qm::AddressBookFileSaveAction::invoke(const ActionEvent& event)
{
	if (!pAddressBookModel_->save())
		ActionUtil::error(hwnd_, IDS_ERROR_SAVEADDRESSBOOK);
}


/****************************************************************************
 *
 * AddressBookViewRefreshAction
 *
 */

qm::AddressBookViewRefreshAction::AddressBookViewRefreshAction(AddressBookModel* pAddressBookModel) :
	pAddressBookModel_(pAddressBookModel)
{
}

qm::AddressBookViewRefreshAction::~AddressBookViewRefreshAction()
{
}

void qm::AddressBookViewRefreshAction::invoke(const ActionEvent& event)
{
	pAddressBookModel_->refresh();
}


/****************************************************************************
 *
 * AddressBookViewSortAction
 *
 */

qm::AddressBookViewSortAction::AddressBookViewSortAction(AddressBookModel* pAddressBookModel,
														 AddressBookModel::Sort sort,
														 AddressBookModel::Sort mask) :
	pAddressBookModel_(pAddressBookModel),
	sort_(sort),
	mask_(mask)
{
	assert((sort_ & mask_) == sort_);
}

qm::AddressBookViewSortAction::~AddressBookViewSortAction()
{
}

void qm::AddressBookViewSortAction::invoke(const ActionEvent& event)
{
	pAddressBookModel_->setSort(sort_, mask_);
}

bool qm::AddressBookViewSortAction::isChecked(const ActionEvent& event)
{
	return (pAddressBookModel_->getSort() & mask_) == static_cast<unsigned int>(sort_);
}
