/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __ADDRESSBOOKMODEL_H__
#define __ADDRESSBOOKMODEL_H__

#include <qm.h>

#include <qs.h>

#include <memory>
#include <vector>


namespace qm {

class AddressBookModel;
class AddressBookModelHandler;
	class DefaultAddressBookModelHandler;
class AddressBookModelEvent;

class AddressBook;
class AddressBookEntry;


/****************************************************************************
 *
 * AddressBookModel
 *
 */

class AddressBookModel
{
public:
	enum Sort {
		SORT_NAME			= 0x0001,
		SORT_ADDRESS		= 0x0002,
		SORT_COMMENT		= 0x0003,
		SORT_COLUMN_MASK	= 0x00ff,
		
		SORT_ASCENDING		= 0x0100,
		SORT_DESCENDING		= 0x0200,
		SORT_DIRECTION_MASK	= 0xff00
	};

public:
	AddressBookModel();
	~AddressBookModel();

public:
	AddressBook* getAddressBook() const;
	unsigned int getCount() const;
	const AddressBookEntry* getEntry(unsigned int nItem) const;
	void add(std::auto_ptr<AddressBookEntry> pEntry);
	void remove(unsigned int nItem);
	void edit(unsigned int nItem,
			  std::auto_ptr<AddressBookEntry> pEntry);
	unsigned int getSort() const;
	void setSort(unsigned int nSort,
				 unsigned int nMask);
	void refresh();
	bool save() const;
	bool isModified() const;
	
	void addAddressBookModelHandler(AddressBookModelHandler* pHandler);
	void removeAddressBookModelHandler(AddressBookModelHandler* pHandler);

private:
	void initEntries();
	void fireItemAdded(unsigned int nItem) const;
	void fireItemRemoved(unsigned int nItem) const;
	void fireItemEdited(unsigned int nItem) const;
	void fireRefreshed() const;
	void fireSorting() const;
	void fireSorted() const;
	void fireSaved() const;
	void fireEvent(const AddressBookModelEvent& event,
				   void (AddressBookModelHandler::*pfn)(const AddressBookModelEvent& event)) const;

private:
	AddressBookModel(const AddressBookModel&);
	AddressBookModel& operator=(const AddressBookModel&);

private:
	class EntryLess : public std::binary_function<AddressBookEntry*, AddressBookEntry*, bool>
	{
	public:
		EntryLess(unsigned int nSort);
		bool operator()(const AddressBookEntry* pLhs,
						const AddressBookEntry* pRhs) const;
	
	private:
		const WCHAR* getSortKey(const AddressBookEntry* pEntry) const;
	
	private:
		unsigned int nSort_;
	};

private:
	typedef std::vector<AddressBookEntry*> EntryList;
	typedef std::vector<AddressBookModelHandler*> HandlerList;

private:
	std::auto_ptr<AddressBook> pAddressBook_;
	EntryList listEntry_;
	unsigned int nSort_;
	mutable bool bModified_;
	HandlerList listHandler_;
};


/****************************************************************************
 *
 * AddressBookModelHandler
 *
 */

class AddressBookModelHandler
{
public:
	virtual ~AddressBookModelHandler();

public:
	virtual void itemAdded(const AddressBookModelEvent& event) = 0;
	virtual void itemRemoved(const AddressBookModelEvent& event) = 0;
	virtual void itemEdited(const AddressBookModelEvent& event) = 0;
	virtual void refreshed(const AddressBookModelEvent& event) = 0;
	virtual void sorting(const AddressBookModelEvent& event) = 0;
	virtual void sorted(const AddressBookModelEvent& event) = 0;
	virtual void saved(const AddressBookModelEvent& event) = 0;
};


/****************************************************************************
 *
 * DefaultAddressBookModelHandler
 *
 */

class DefaultAddressBookModelHandler : public AddressBookModelHandler
{
public:
	virtual ~DefaultAddressBookModelHandler();

public:
	virtual void itemAdded(const AddressBookModelEvent& event);
	virtual void itemRemoved(const AddressBookModelEvent& event);
	virtual void itemEdited(const AddressBookModelEvent& event);
	virtual void refreshed(const AddressBookModelEvent& event);
	virtual void sorting(const AddressBookModelEvent& event);
	virtual void sorted(const AddressBookModelEvent& event);
	virtual void saved(const AddressBookModelEvent& event);
};


/****************************************************************************
 *
 * AddressBookModelEvent
 *
 */

class AddressBookModelEvent
{
public:
	explicit AddressBookModelEvent(const AddressBookModel* pModel);
	AddressBookModelEvent(const AddressBookModel* pModel,
						  unsigned int nItem);
	~AddressBookModelEvent();

public:
	const AddressBookModel* getModel() const;
	unsigned int getItem() const;

private:
	AddressBookModelEvent(const AddressBookModelEvent&);
	AddressBookModelEvent& operator=(const AddressBookModelEvent&);

private:
	const AddressBookModel* pModel_;
	unsigned int nItem_;
};

}

#endif // __ADDRESSBOOKMODEL_H__
