/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __ADDRESSBOOKDIALOG_H__
#define __ADDRESSBOOKDIALOG_H__

#include <qm.h>

#include <qsprofile.h>

#include "dialogs.h"
#include "../model/addressbook.h"


namespace qm {

class AddressBookEntryDialog;
class AddressBookAddressDialog;
class AddAddressDialog;
class SelectAddressDialog;


/****************************************************************************
 *
 * AddressBookEntryDialog
 *
 */

class AddressBookEntryDialog : public AbstractListDialog<AddressBookAddress, AddressBookEntry::AddressList>
{
public:
	AddressBookEntryDialog(AddressBook* pAddressBook,
						   AddressBookEntry* pEntry);
	virtual ~AddressBookEntryDialog();

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

protected:
	virtual LRESULT onOk();

protected:
	virtual qs::wstring_ptr getLabel(const AddressBookAddress* p) const;
	virtual std::auto_ptr<AddressBookAddress> create() const;
	virtual AddressBookAddress* edit(AddressBookAddress* p) const;
	virtual void updateState();

private:
	LRESULT onNameChange();

private:
	AddressBookEntryDialog(const AddressBookEntryDialog&);
	AddressBookEntryDialog& operator=(const AddressBookEntryDialog&);

private:
	AddressBook* pAddressBook_;
	AddressBookEntry* pEntry_;
};


/****************************************************************************
 *
 * AddressBookAddressDialog
 *
 */

class AddressBookAddressDialog : public DefaultDialog
{
public:
	AddressBookAddressDialog(AddressBook* pAddressBook,
							 AddressBookAddress* pAddress);
	virtual ~AddressBookAddressDialog();

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

protected:
	virtual LRESULT onOk();

private:
	LRESULT onAddressChange();

private:
	void updateState();

private:
	AddressBookAddressDialog(const AddressBookAddressDialog&);
	AddressBookAddressDialog& operator=(const AddressBookAddressDialog&);

private:
	AddressBook* pAddressBook_;
	AddressBookAddress* pAddress_;
};


/****************************************************************************
 *
 * AddAddressDialog
 *
 */

class AddAddressDialog : public DefaultDialog
{
public:
	enum Type {
		TYPE_NEWENTRY,
		TYPE_NEWADDRESS
	};

public:
	explicit AddAddressDialog(AddressBook* pAddressBook);
	virtual ~AddAddressDialog();

public:
	Type getType() const;
	AddressBookEntry* getEntry() const;

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

protected:
	virtual LRESULT onOk();

private:
	LRESULT onNewEntry();
	LRESULT onNewAddress();
	LRESULT onEntriesSelChange();

private:
	void updateState();

private:
	AddAddressDialog(const AddAddressDialog&);
	AddAddressDialog& operator=(const AddAddressDialog&);

private:
	AddressBook* pAddressBook_;
	Type type_;
	AddressBookEntry* pEntry_;
};


/****************************************************************************
 *
 * SelectAddressDialog
 *
 */

class SelectAddressDialog :
	public DefaultDialog,
	public qs::NotifyHandler
{
public:
	enum Sort {
		SORT_NAME			= 0x01,
		SORT_ADDRESS		= 0x02,
		SORT_COMMENT		= 0x03,
		SORT_TYPE_MASK		= 0x0f,
		
		SORT_ASCENDING		= 0x10,
		SORT_DESCENDING		= 0x20,
		SORT_DIRECTION_MASK	= 0xf0
	};
	
	enum Type {
		TYPE_TO,
		TYPE_CC,
		TYPE_BCC
	};

public:
	typedef std::vector<qs::WSTRING> AddressList;
	typedef std::vector<qs::WSTRING> CategoryNameList;

public:
	class Item
	{
	public:
		Item(qs::wstring_ptr wstrValue,
			 Type type);
		~Item();
	
	public:
		const WCHAR* getValue() const;
		qs::wstring_ptr releaseValue();
		Type getType() const;
		void setType(Type type);
	
	private:
		Item(const Item&);
		Item& operator=(const Item&);
	
	private:
		qs::wstring_ptr wstrValue_;
		Type type_;
	};

public:
	SelectAddressDialog(AddressBook* pAddressBook,
						qs::Profile* pProfile,
						const WCHAR* pwszAddress[]);
	virtual ~SelectAddressDialog();

public:
	const AddressList& getAddresses(Type type) const;

public:
	virtual INT_PTR dialogProc(UINT uMsg,
							   WPARAM wParam,
							   LPARAM lParam);

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

protected:
	virtual LRESULT onDestroy();
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

protected:
	virtual LRESULT onOk();

public:
	virtual LRESULT onNotify(NMHDR* pnmhdr,
							 bool* pbHandled);

protected:
	LRESULT onSize(UINT nFlags,
				   int cx,
				   int cy);

private:
	LRESULT onCategory();
	LRESULT onSelect(UINT nId);
	LRESULT onRemove();
#if !defined _WIN32_WCE || _WIN32_WCE < 0x300 || !defined _WIN32_WCE_PSPC
	LRESULT onFilterChange();
#endif
	LRESULT onAddressColumnClick(NMHDR* pnmhdr,
								 bool* pbHandled);
	LRESULT onAddressDblClk(NMHDR* pnmhdr,
							bool* pbHandled);

private:
	void update();
	void select(Type type);
	void remove();
	void layout();
	HMENU createCategoryMenu(const AddressBook::CategoryList& l,
							 CategoryNameList* pList);
	void setCurrentCategory(const WCHAR* pwszCategory);
	bool isCategory(const AddressBookAddress::CategoryList& listCategory) const;
	bool isMatchFilter(const AddressBookEntry* pEntry) const;
	bool isMatchFilter(const AddressBookAddress* pAddress) const;

private:
	static size_t getCategoryLevel(const WCHAR* pwszCategory);
	static qs::wstring_ptr getCategoryName(const WCHAR* pwszCategory,
										   size_t nLevel,
										   bool bFull);

private:
	SelectAddressDialog(const SelectAddressDialog&);
	SelectAddressDialog& operator=(const SelectAddressDialog&);

private:
	class AddressListWindow :
		public qs::WindowBase,
		public qs::DefaultWindowHandler
	{
	public:
		explicit AddressListWindow(SelectAddressDialog* pDialog);
		virtual ~AddressListWindow();
	
	public:
		virtual LRESULT windowProc(UINT uMsg,
								   WPARAM wParam,
								   LPARAM lParam);
	
	protected:
		LRESULT onChar(UINT nChar,
					   UINT nRepeat,
					   UINT nFlags);
	
	private:
		AddressListWindow(const AddressListWindow&);
		AddressListWindow& operator=(const AddressListWindow&);
	
	private:
		SelectAddressDialog* pDialog_;
	};
	friend class AddressListWindow;
	
	class SelectedAddressListWindow :
		public qs::WindowBase,
		public qs::DefaultWindowHandler,
		public qs::NotifyHandler
	{
	public:
		explicit SelectedAddressListWindow(SelectAddressDialog* pDialog);
		virtual ~SelectedAddressListWindow();
	
	public:
		virtual bool preSubclassWindow();
		virtual LRESULT windowProc(UINT uMsg,
								   WPARAM wParam,
								   LPARAM lParam);
	
	protected:
		LRESULT onContextMenu(HWND hwnd,
							  const POINT& pt);
		LRESULT onDestroy();
		LRESULT onLButtonDown(UINT nFlags,
							  const POINT& pt);
	
	public:
		virtual LRESULT onNotify(NMHDR* pnmhdr,
								 bool* pbHandled);
	
	private:
#if defined _WIN32_WCE && _WIN32_WCE >= 0x400 && defined _WIN32_WCE_PSPC
		LRESULT onRecognizeGesture(NMHDR* pnmhdr,
								   bool* pbHandled);
#endif
	
	private:
		SelectedAddressListWindow(const SelectedAddressListWindow&);
		SelectedAddressListWindow& operator=(const SelectedAddressListWindow&);
	
	private:
		SelectAddressDialog* pDialog_;
	};
	friend class SelectedAddressListWindow;

private:
	AddressBook* pAddressBook_;
	qs::Profile* pProfile_;
	unsigned int nSort_;
	qs::wstring_ptr wstrCategory_;
	qs::wstring_ptr wstrFilter_;
	AddressList listAddress_[3];
	AddressListWindow wndAddressList_;
	SelectedAddressListWindow wndSelectedAddressList_;
};

}

#endif // __ADDRESSBOOKDIALOG_H__
