/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __ADDRESSBOOKSELECTIONMODEL_H__
#define __ADDRESSBOOKSELECTIONMODEL_H__

#include <qm.h>

#include <vector>


namespace qm {

/****************************************************************************
 *
 * AddressBookSelectionModel
 *
 */

class AddressBookSelectionModel
{
public:
	typedef std::vector<unsigned int> ItemList;

public:
	virtual ~AddressBookSelectionModel();

public:
	virtual void getSelectedItems(ItemList* pList) = 0;
	virtual bool hasSelectedItem() = 0;
	virtual unsigned int getFocusedItem() = 0;
};

}

#endif // __ADDRESSBOOKSELECTIONMODEL_H__
