/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#pragma warning(disable:4786)

#include <qmapplication.h>

#include <tchar.h>

#include "addressbookdialog.h"
#include "actionid.h"
#include "resourceinc.h"
#include "uiutil.h"
#include "../model/addressbook.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * AddressBookEntryDialog
 *
 */

qm::AddressBookEntryDialog::AddressBookEntryDialog(AddressBook* pAddressBook,
												   AddressBookEntry* pEntry) :
	AbstractListDialog<AddressBookAddress, AddressBookEntry::AddressList>(IDD_ADDRESSBOOKENTRY, IDC_ADDRESSES, true),
	pAddressBook_(pAddressBook),
	pEntry_(pEntry)
{
	const AddressBookEntry::AddressList& l = pEntry->getAddresses();
	AddressBookEntry::AddressList& list = getList();
	list.reserve(l.size());
	for (AddressBookEntry::AddressList::const_iterator it = l.begin(); it != l.end(); ++it)
		list.push_back(new AddressBookAddress(**it));
}

qm::AddressBookEntryDialog::~AddressBookEntryDialog()
{
}

LRESULT qm::AddressBookEntryDialog::onCommand(WORD nCode,
											  WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID_CODE(IDC_NAME, EN_CHANGE, onNameChange)
	END_COMMAND_HANDLER()
	return AbstractListDialog<AddressBookAddress, AddressBookEntry::AddressList>::onCommand(nCode, nId);
}

LRESULT qm::AddressBookEntryDialog::onInitDialog(HWND hwndFocus,
												 LPARAM lParam)
{
	init(false);
	
	setDlgItemText(IDC_NAME, pEntry_->getName());
	if (pEntry_->getSortKey())
		setDlgItemText(IDC_SORTKEY, pEntry_->getSortKey());
	
	return AbstractListDialog<AddressBookAddress, AddressBookEntry::AddressList>::onInitDialog(hwndFocus, lParam);
}

LRESULT qm::AddressBookEntryDialog::onOk()
{
	wstring_ptr wstrName(getDlgItemText(IDC_NAME));
	pEntry_->setName(wstrName.get());
	
	wstring_ptr wstrSortKey(getDlgItemText(IDC_SORTKEY));
	pEntry_->setSortKey(*wstrSortKey.get() ? wstrSortKey.get() : 0);
	
	pEntry_->setAddresses(getList());
	
	return AbstractListDialog<AddressBookAddress, AddressBookEntry::AddressList>::onOk();
}

wstring_ptr qm::AddressBookEntryDialog::getLabel(const AddressBookAddress* p) const
{
	return allocWString(p->getAddress());
}

std::auto_ptr<AddressBookAddress> qm::AddressBookEntryDialog::create() const
{
	std::auto_ptr<AddressBookAddress> pAddress(new AddressBookAddress(pEntry_));
	AddressBookAddressDialog dialog(pAddressBook_, pAddress.get());
	if (dialog.doModal(getHandle()) != IDOK)
		return std::auto_ptr<AddressBookAddress>();
	return pAddress;
}

bool qm::AddressBookEntryDialog::edit(AddressBookAddress* p) const
{
	AddressBookAddressDialog dialog(pAddressBook_, p);
	return dialog.doModal(getHandle()) == IDOK;
}

void qm::AddressBookEntryDialog::updateState()
{
	AbstractListDialog<AddressBookAddress, AddressBookEntry::AddressList>::updateState();
	
	bool bEnable = Window(getDlgItem(IDC_NAME)).getWindowTextLength() != 0 &&
		ListBox_GetCount(getDlgItem(IDC_ADDRESSES)) != 0;
	Window(getDlgItem(IDOK)).enableWindow(bEnable);
}

LRESULT qm::AddressBookEntryDialog::onNameChange()
{
	updateState();
	return 0;
}


/****************************************************************************
 *
 * AddressBookAddressDialog
 *
 */

qm::AddressBookAddressDialog::AddressBookAddressDialog(AddressBook* pAddressBook,
													   AddressBookAddress* pAddress) :
	DefaultDialog(IDD_ADDRESSBOOKADDRESS),
	pAddressBook_(pAddressBook),
	pAddress_(pAddress)
{
}

qm::AddressBookAddressDialog::~AddressBookAddressDialog()
{
}

LRESULT qm::AddressBookAddressDialog::onCommand(WORD nCode,
												WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID_CODE(IDC_ADDRESS, EN_CHANGE, onAddressChange)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::AddressBookAddressDialog::onInitDialog(HWND hwndFocus,
												   LPARAM lParam)
{
	init(false);
	
	setDlgItemText(IDC_ADDRESS, pAddress_->getAddress());
	sendDlgItemMessage(IDC_RFC2822, BM_SETCHECK,
		pAddress_->isRFC2822() ? BST_CHECKED : BST_UNCHECKED);
	if (pAddress_->getAlias())
		setDlgItemText(IDC_ALIAS, pAddress_->getAlias());
	
	AddressBook::CategoryList listAllCategory(pAddressBook_->getCategories());
	std::sort(listAllCategory.begin(), listAllCategory.end(), AddressBookCategoryLess());
	for (AddressBook::CategoryList::const_iterator it = listAllCategory.begin(); it != listAllCategory.end(); ++it) {
		W2T((*it)->getName(), ptszName);
		sendDlgItemMessage(IDC_CATEGORY, CB_ADDSTRING,
			0, reinterpret_cast<LPARAM>(ptszName));
	}
	
	StringBuffer<WSTRING> bufCategory;
	const AddressBookAddress::CategoryList& listCategory = pAddress_->getCategories();
	for (AddressBookAddress::CategoryList::const_iterator it = listCategory.begin(); it != listCategory.end(); ++it) {
		if (bufCategory.getLength() != 0)
			bufCategory.append(L',');
		bufCategory.append((*it)->getName());
	}
	setDlgItemText(IDC_CATEGORY, bufCategory.getCharArray());
	
	if (pAddress_->getComment())
		setDlgItemText(IDC_COMMENT, pAddress_->getComment());
	if (pAddress_->getCertificate())
		setDlgItemText(IDC_CERTIFICATE, pAddress_->getCertificate());
	
	updateState();
	
	return TRUE;
}

LRESULT qm::AddressBookAddressDialog::onOk()
{
	wstring_ptr wstrAddress(getDlgItemText(IDC_ADDRESS));
	if (!*wstrAddress.get())
		return 0;
	bool bRFC2822 = sendDlgItemMessage(IDC_RFC2822, BM_GETCHECK) == BST_CHECKED;
	wstring_ptr wstrAlias(getDlgItemText(IDC_ALIAS));
	
	wstring_ptr wstrCategory(getDlgItemText(IDC_CATEGORY));
	AddressBookAddress::CategoryList listCategory;
	const WCHAR* p = wcstok(wstrCategory.get(), L", ");
	while (p) {
		listCategory.push_back(pAddressBook_->getCategory(p));
		p = wcstok(0, L", ");
	}
	
	wstring_ptr wstrComment(getDlgItemText(IDC_COMMENT));
	wstring_ptr wstrCertificate(getDlgItemText(IDC_CERTIFICATE));
	
	pAddress_->setAddress(wstrAddress.get());
	pAddress_->setAlias(*wstrAlias.get() ? wstrAlias.get() : 0);
	pAddress_->setCategories(listCategory);
	pAddress_->setComment(*wstrComment.get() ? wstrComment.get() : 0);
	pAddress_->setCertificate(*wstrCertificate.get() ? wstrCertificate.get() : 0);
	pAddress_->setRFC2822(bRFC2822);
	
	return DefaultDialog::onOk();
}

LRESULT qm::AddressBookAddressDialog::onAddressChange()
{
	updateState();
	return 0;
}

void qm::AddressBookAddressDialog::updateState()
{
	Window(getDlgItem(IDOK)).enableWindow(
		Window(getDlgItem(IDC_ADDRESS)).getWindowTextLength() != 0);
}


/****************************************************************************
 *
 * AddAddressDialog
 *
 */

qm::AddAddressDialog::AddAddressDialog(AddressBook* pAddressBook) :
	DefaultDialog(IDD_ADDADDRESS),
	pAddressBook_(pAddressBook),
	type_(TYPE_NEWENTRY),
	pEntry_(0)
{
	pAddressBook_->reload();
}

qm::AddAddressDialog::~AddAddressDialog()
{
}

AddAddressDialog::Type qm::AddAddressDialog::getType() const
{
	return type_;
}

AddressBookEntry* qm::AddAddressDialog::getEntry() const
{
	return pEntry_;
}

LRESULT qm::AddAddressDialog::onCommand(WORD nCode,
										WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_NEWADDRESS, onNewAddress)
		HANDLE_COMMAND_ID(IDC_NEWENTRY, onNewEntry)
		HANDLE_COMMAND_ID_CODE(IDC_ENTRIES, LBN_SELCHANGE, onEntriesSelChange)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::AddAddressDialog::onInitDialog(HWND hwndFocus,
										   LPARAM lParam)
{
	init(false);
	
	switch (type_) {
	case TYPE_NEWENTRY:
		sendDlgItemMessage(IDC_NEWENTRY, BM_SETCHECK, BST_CHECKED);
		break;
	case TYPE_NEWADDRESS:
		sendDlgItemMessage(IDC_NEWADDRESS, BM_SETCHECK, BST_CHECKED);
		break;
	}
	
	const AddressBook::EntryList& l = pAddressBook_->getEntries();
	AddressBook::EntryList listEntry;
	listEntry.reserve(l.size());
	std::remove_copy_if(l.begin(), l.end(),
		std::back_inserter(listEntry),
		std::mem_fun(&AddressBookEntry::isExternal));
	std::sort(listEntry.begin(), listEntry.end(),
		binary_compose_f_gx_hy(
			string_less_i<WCHAR>(),
			std::mem_fun(&AddressBookEntry::getActualSortKey),
			std::mem_fun(&AddressBookEntry::getActualSortKey)));
	for (AddressBook::EntryList::const_iterator it = listEntry.begin(); it != listEntry.end(); ++it) {
		const AddressBookEntry* pEntry = *it;
		W2T(pEntry->getName(), ptszName);
		int nItem = ListBox_AddString(getDlgItem(IDC_ENTRIES), ptszName);
		ListBox_SetItemData(getDlgItem(IDC_ENTRIES), nItem, pEntry);
	}
	ListBox_SetCurSel(getDlgItem(IDC_ENTRIES), 0);
	
	updateState();
	
	return TRUE;
}

LRESULT qm::AddAddressDialog::onOk()
{
	if (sendDlgItemMessage(IDC_NEWADDRESS, BM_GETCHECK) == BST_CHECKED) {
		type_ = TYPE_NEWADDRESS;
		
		int nItem = ListBox_GetCurSel(getDlgItem(IDC_ENTRIES));
		if (nItem == LB_ERR)
			return 0;
		pEntry_ = reinterpret_cast<AddressBookEntry*>(
			ListBox_GetItemData(getDlgItem(IDC_ENTRIES), nItem));
	}
	else {
		type_ = TYPE_NEWENTRY;
	}
	
	return DefaultDialog::onOk();
}

LRESULT qm::AddAddressDialog::onNewEntry()
{
	updateState();
	return 0;
}

LRESULT qm::AddAddressDialog::onNewAddress()
{
	updateState();
	return 0;
}

LRESULT qm::AddAddressDialog::onEntriesSelChange()
{
	updateState();
	return 0;
}

void qm::AddAddressDialog::updateState()
{
	bool bNewAddress = sendDlgItemMessage(IDC_NEWADDRESS, BM_GETCHECK) == BST_CHECKED;
	Window(getDlgItem(IDC_ENTRIES)).enableWindow(bNewAddress);
	Window(getDlgItem(IDOK)).enableWindow(!bNewAddress ||
		ListBox_GetCurSel(getDlgItem(IDC_ENTRIES)) != LB_ERR);
}


/****************************************************************************
 *
 * SelectAddressDialog
 *
 */

namespace {
int CALLBACK itemComp(LPARAM lParam1,
					  LPARAM lParam2,
					  LPARAM lParamSort)
{
	AddressBookAddress* pAddress1 = reinterpret_cast<AddressBookAddress*>(lParam1);
	AddressBookAddress* pAddress2 = reinterpret_cast<AddressBookAddress*>(lParam2);
	
	const WCHAR* p1 = 0;
	const WCHAR* p2 = 0;
	switch (lParamSort & SelectAddressDialog::SORT_TYPE_MASK) {
	case SelectAddressDialog::SORT_NAME:
		p1 = pAddress1->getEntry()->getActualSortKey();
		p2 = pAddress2->getEntry()->getActualSortKey();
		break;
	case SelectAddressDialog::SORT_ADDRESS:
		p1 = pAddress1->getAddress();
		p2 = pAddress2->getAddress();
		break;
	case SelectAddressDialog::SORT_COMMENT:
		p1 = pAddress1->getComment();
		p2 = pAddress2->getComment();
		break;
	default:
		assert(false);
		break;
	}
	
	int nComp = p1 == p2 ? 0 : !p1 ? -1 : !p2 ? 1 : _wcsicmp(p1, p2);
	return (lParamSort & SelectAddressDialog::SORT_DIRECTION_MASK)
		== SelectAddressDialog::SORT_ASCENDING ? nComp : -nComp;
}

int CALLBACK selectedItemComp(LPARAM lParam1,
							  LPARAM lParam2,
							  LPARAM lParamSort)
{
	SelectAddressDialog::Item* pItem1 = reinterpret_cast<SelectAddressDialog::Item*>(lParam1);
	SelectAddressDialog::Item* pItem2 = reinterpret_cast<SelectAddressDialog::Item*>(lParam2);
	if (pItem1->getType() < pItem2->getType())
		return -1;
	else if (pItem1->getType() > pItem2->getType())
		return 1;
	else
		return wcscmp(pItem1->getValue(), pItem2->getValue());
}
}

#pragma warning(push)
#pragma warning(disable:4355)

qm::SelectAddressDialog::SelectAddressDialog(AddressBook* pAddressBook,
											 Profile* pProfile,
											 const WCHAR* pwszAddress[]) :
	DefaultDialog(IDD_SELECTADDRESS),
	pAddressBook_(pAddressBook),
	pProfile_(pProfile),
	nSort_(SORT_NAME | SORT_ASCENDING),
	wndAddressList_(this),
	wndSelectedAddressList_(this)
{
	pAddressBook_->reload();
	
	Type types[] = {
		TYPE_TO,
		TYPE_CC,
		TYPE_BCC
	};
	for (int n = 0; n < countof(listAddress_); ++n) {
		UTF8Parser field(pwszAddress[n]);
		Part part;
		if (part.setField(L"Dummy", field)) {
			AddressListParser addressList(AddressListParser::FLAG_ALLOWUTF8);
			if (part.getField(L"Dummy", &addressList) == Part::FIELD_EXIST) {
				const AddressListParser::AddressList& l = addressList.getAddressList();
				for (AddressListParser::AddressList::const_iterator it = l.begin(); it != l.end(); ++it) {
					wstring_ptr wstrValue((*it)->getValue());
					listAddress_[n].push_back(wstrValue.get());
					wstrValue.release();
				}
			}
		}
	}
}

#pragma warning(pop)

qm::SelectAddressDialog::~SelectAddressDialog()
{
	for (int n = 0; n < countof(listAddress_); ++n)
		std::for_each(listAddress_[n].begin(),
			listAddress_[n].end(), string_free<WSTRING>());
}

const SelectAddressDialog::AddressList& qm::SelectAddressDialog::getAddresses(Type type) const
{
	return listAddress_[type];
}

INT_PTR qm::SelectAddressDialog::dialogProc(UINT uMsg,
											WPARAM wParam,
											LPARAM lParam)
{
	BEGIN_DIALOG_HANDLER()
		HANDLE_SIZE()
	END_DIALOG_HANDLER()
	return DefaultDialog::dialogProc(uMsg, wParam, lParam);
}

LRESULT qm::SelectAddressDialog::onCommand(WORD nCode,
										   WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_CATEGORY, onCategory)
		HANDLE_COMMAND_ID_RANGE(IDC_TO, IDC_BCC, onSelect)
		HANDLE_COMMAND_ID(IDC_REMOVE, onRemove)
#if !defined _WIN32_WCE || _WIN32_WCE < 300 || !defined _WIN32_WCE_PSPC
		HANDLE_COMMAND_ID_CODE(IDC_FILTER, EN_CHANGE, onFilterChange)
#endif
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::SelectAddressDialog::onDestroy()
{
	int n = 0;
	
	HWND hwndSelected = getDlgItem(IDC_SELECTEDADDRESS);
	int nCount = ListView_GetItemCount(hwndSelected);
	for (n = 0; n < nCount; ++n) {
		LVITEM item = {
			LVIF_PARAM,
			n
		};
		ListView_GetItem(hwndSelected, &item);
		delete reinterpret_cast<Item*>(item.lParam);
	}
	
	const WCHAR* pwszKeys[] = {
		L"NameWidth",
		L"AddressWidth",
		L"CommentWidth"
	};
	HWND hwndAddress = getDlgItem(IDC_ADDRESS);
	for (n = 0; n < countof(pwszKeys); ++n) {
		int nColumnWidth = ListView_GetColumnWidth(hwndAddress, n);
		pProfile_->setInt(L"AddressBook", pwszKeys[n], nColumnWidth);
	}
	
	int nColumnWidth = ListView_GetColumnWidth(hwndSelected, 0);
	pProfile_->setInt(L"AddressBook", L"SelectedAddressWidth", nColumnWidth);
	
#ifndef _WIN32_WCE
	RECT rect;
	getWindowRect(&rect);
	pProfile_->setInt(L"AddressBook", L"Width", rect.right - rect.left);
	pProfile_->setInt(L"AddressBook", L"Height", rect.bottom - rect.top);
#endif
	
	const WCHAR* pwszCategory = L"";
	if (wstrCategory_.get())
		pwszCategory = wstrCategory_.get();
	pProfile_->setString(L"AddressBook", L"Category", pwszCategory);
	
	removeNotifyHandler(this);
	
	return DefaultDialog::onDestroy();
}

LRESULT qm::SelectAddressDialog::onInitDialog(HWND hwndFocus,
											  LPARAM lParam)
{
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	HWND hwndList = getDlgItem(IDC_ADDRESS);
	
	ListView_SetExtendedListViewStyle(hwndList, LVS_EX_FULLROWSELECT);
	
	struct {
		UINT nId_;
		const WCHAR* pwszKey_;
		int nWidth_;
	} columns[] = {
		{ IDS_ADDRESSBOOK_NAME,		L"NameWidth",		120	},
		{ IDS_ADDRESSBOOK_ADDRESS,	L"AddressWidth",	130	},
		{ IDS_ADDRESSBOOK_COMMENT,	L"CommentWidth",	60	}
	};
	for (int n = 0; n < countof(columns); ++n) {
		wstring_ptr wstrName(loadString(hInst, columns[n].nId_));
		W2T(wstrName.get(), ptszName);
		
		int nWidth = pProfile_->getInt(L"AddressBook",
			columns[n].pwszKey_, columns[n].nWidth_);
		LVCOLUMN column = {
			LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH,
			LVCFMT_LEFT,
			nWidth,
			const_cast<LPTSTR>(ptszName),
			0,
			n,
		};
		ListView_InsertColumn(hwndList, n, &column);
	}
	
	HWND hwndSelected = getDlgItem(IDC_SELECTEDADDRESS);
	
	HIMAGELIST hImageList = ImageList_LoadBitmap(hInst,
		MAKEINTRESOURCE(IDB_ADDRESSBOOK), 16, 0, RGB(255, 255, 255));
	ListView_SetImageList(hwndSelected, hImageList, LVSIL_SMALL);
	
	int nColumnWidth = pProfile_->getInt(L"AddressBook", L"SelectedAddressWidth", 150);
	LVCOLUMN column = {
		LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH,
		LVCFMT_LEFT,
		nColumnWidth,
		_T(""),
		0,
		0
	};
	ListView_InsertColumn(hwndSelected, 0, &column);
	
	Type types[] = {
		TYPE_TO,
		TYPE_CC,
		TYPE_BCC
	};
	for (int n = 0; n < countof(listAddress_); ++n) {
		for (AddressList::iterator it = listAddress_[n].begin(); it != listAddress_[n].end(); ++it) {
			wstring_ptr wstrValue(*it);
			W2T(wstrValue.get(), ptszValue);
			*it = 0;
			
			std::auto_ptr<Item> pItem(new Item(wstrValue, types[n]));
			LVITEM newItem = {
				LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM,
				ListView_GetItemCount(hwndSelected),
				0,
				0,
				0,
				const_cast<LPTSTR>(ptszValue),
				0,
				types[n],
				reinterpret_cast<LPARAM>(pItem.get())
			};
			ListView_InsertItem(hwndSelected, &newItem);
			pItem.release();
		}
		listAddress_[n].clear();
	}
	ListView_SortItems(hwndSelected, &selectedItemComp, 0);
	
	wstring_ptr wstrCategory(pProfile_->getString(L"AddressBook", L"Category", L""));
	setCurrentCategory(*wstrCategory.get() ? wstrCategory.get() : 0);
	
#ifdef _WIN32_WCE
	RECT rectWorkArea;
	::SystemParametersInfo(SPI_GETWORKAREA, 0, &rectWorkArea, 0);
	int nWidth = rectWorkArea.right - rectWorkArea.left;
	int nHeight = rectWorkArea.bottom - rectWorkArea.top;
	setWindowPos(0, 0, 0, nWidth, nHeight, SWP_NOZORDER | SWP_NOACTIVATE);
#else
	int nWidth = pProfile_->getInt(L"AddressBook", L"Width", 620);
	int nHeight = pProfile_->getInt(L"AddressBook", L"Height", 450);
	setWindowPos(0, 0, 0, nWidth, nHeight,
		SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
#endif
	
	addNotifyHandler(this);
	init(false);
	wndAddressList_.subclassWindow(hwndList);
	wndSelectedAddressList_.subclassWindow(hwndSelected);
	Window(hwndList).setFocus();
	
	return FALSE;
}

LRESULT qm::SelectAddressDialog::onOk()
{
	HWND hwndSelected = getDlgItem(IDC_SELECTEDADDRESS);
	
	int nCount = ListView_GetItemCount(hwndSelected);
	for (int n = 0; n < nCount; ++n) {
		LVITEM item = {
			LVIF_PARAM,
			n
		};
		ListView_GetItem(hwndSelected, &item);
		
		Item* pItem = reinterpret_cast<Item*>(item.lParam);
		
		wstring_ptr wstrValue(pItem->releaseValue());
		listAddress_[pItem->getType()].push_back(wstrValue.get());
		wstrValue.release();
	}
	
	return DefaultDialog::onOk();
}

LRESULT qm::SelectAddressDialog::onNotify(NMHDR* pnmhdr,
										  bool* pbHandled)
{
	BEGIN_NOTIFY_HANDLER()
		HANDLE_NOTIFY(LVN_COLUMNCLICK, IDC_ADDRESS, onAddressColumnClick)
		HANDLE_NOTIFY(NM_DBLCLK, IDC_ADDRESS, onAddressDblClk)
	END_NOTIFY_HANDLER()
	return NotifyHandler::onNotify(pnmhdr, pbHandled);
}

LRESULT qm::SelectAddressDialog::onSize(UINT nFlags,
										int cx,
										int cy)
{
	layout();
	return 0;
}

LRESULT qm::SelectAddressDialog::onCategory()
{
	RECT rect;
	Window(getDlgItem(IDC_CATEGORY)).getWindowRect(&rect);
	
	AddressBook::CategoryList listCategory(pAddressBook_->getCategories());
	std::sort(listCategory.begin(), listCategory.end(), AddressBookCategoryLess());
	
	CategoryNameList listName;
	StringListFree<CategoryNameList> free(listName);
	AutoMenuHandle hmenu(createCategoryMenu(listCategory, &listName));
	if (hmenu.get()) {
		unsigned int nFlags = TPM_LEFTALIGN |
			TPM_TOPALIGN | TPM_NONOTIFY | TPM_RETURNCMD;
#ifndef _WIN32_WCE
		nFlags |= TPM_LEFTBUTTON | TPM_RIGHTBUTTON;
#endif
		UINT nCommand = ::TrackPopupMenu(hmenu.get(), nFlags,
			rect.left, rect.bottom, 0, getHandle(), 0);
		if (nCommand == 0)
			;
		else if (nCommand == IDM_ADDRESSBOOK_ALLCATEGORY)
			setCurrentCategory(0);
		else if (nCommand - IDM_ADDRESSBOOK_CATEGORY < listName.size())
			setCurrentCategory(listName[nCommand - IDM_ADDRESSBOOK_CATEGORY]);
	}
	
	return 0;
}

LRESULT qm::SelectAddressDialog::onSelect(UINT nId)
{
	Type types[] = { TYPE_TO, TYPE_CC, TYPE_BCC };
	select(types[nId - IDC_TO]);
	return 0;
}

LRESULT qm::SelectAddressDialog::onRemove()
{
	remove();
	return 0;
}

#if !defined _WIN32_WCE || _WIN32_WCE < 300 || !defined _WIN32_WCE_PSPC
LRESULT qm::SelectAddressDialog::onFilterChange()
{
	wstring_ptr wstrFilter(getDlgItemText(IDC_FILTER));
	wstrFilter_ = tolower(wstrFilter.get());
	update();
	return 0;
}
#endif

LRESULT qm::SelectAddressDialog::onAddressColumnClick(NMHDR* pnmhdr,
													  bool* pbHandled)
{
	NMLISTVIEW* pnm = reinterpret_cast<NMLISTVIEW*>(pnmhdr);
	
	Sort sorts[] = {
		SORT_NAME,
		SORT_ADDRESS,
		SORT_COMMENT
	};
	unsigned int nSort = sorts[pnm->iSubItem];
	if ((nSort_ & SORT_TYPE_MASK) == nSort)
		nSort |= (nSort_ & SORT_DIRECTION_MASK) == SORT_ASCENDING ?
			SORT_DESCENDING : SORT_ASCENDING;
	else
		nSort |= SORT_ASCENDING;
	nSort_ = nSort;
	ListView_SortItems(getDlgItem(IDC_ADDRESS), &itemComp, nSort);
	
	return 0;
}

LRESULT qm::SelectAddressDialog::onAddressDblClk(NMHDR* pnmhdr,
												 bool* pbHandled)
{
	select(TYPE_TO);
	return 0;
}

void qm::SelectAddressDialog::update()
{
	HWND hwndList = getDlgItem(IDC_ADDRESS);
	
	ListView_DeleteAllItems(hwndList);
	
	const AddressBook::EntryList& listEntry = pAddressBook_->getEntries();
	
	size_t nCategoryLen = 0;
	if (wstrCategory_.get())
		nCategoryLen = wcslen(wstrCategory_.get());
	
	int n = 0;
	for (AddressBook::EntryList::const_iterator itE = listEntry.begin(); itE != listEntry.end(); ++itE) {
		AddressBookEntry* pEntry = *itE;
		bool bMatchEntry = isMatchFilter(pEntry);
		W2T(pEntry->getName(), ptszName);
		const AddressBookEntry::AddressList& l = pEntry->getAddresses();
		for (AddressBookEntry::AddressList::const_iterator itA = l.begin(); itA != l.end(); ++itA) {
			AddressBookAddress* pAddress = *itA;
			
			if (isCategory(pAddress->getCategories()) &&
				(bMatchEntry || isMatchFilter(pAddress))) {
				LVITEM item = {
					LVIF_TEXT | LVIF_PARAM,
					n,
					0,
					0,
					0,
					const_cast<LPTSTR>(ptszName),
					0,
					0,
					reinterpret_cast<LPARAM>(pAddress)
				};
				ListView_InsertItem(hwndList, &item);
				
				W2T(pAddress->getAddress(), ptszAddress);
				ListView_SetItemText(hwndList, n, 1, const_cast<LPTSTR>(ptszAddress));
				if (pAddress->getComment()) {
					W2T(pAddress->getComment(), ptszComment);
					ListView_SetItemText(hwndList, n, 2, const_cast<LPTSTR>(ptszComment));
				}
				++n;
			}
		}
	}
	
	ListView_SortItems(hwndList, &itemComp, nSort_);
}

void qm::SelectAddressDialog::select(Type type)
{
	HWND hwndList = getDlgItem(IDC_ADDRESS);
	HWND hwndSelected = getDlgItem(IDC_SELECTEDADDRESS);
	
	bool bFilter = getFocus() == getDlgItem(IDC_FILTER);
	
	int nItem = -1;
	while (true) {
		nItem = ListView_GetNextItem(hwndList, nItem,
			bFilter ? LVNI_ALL : LVNI_SELECTED);
		if (nItem == -1)
			break;
		
		LVITEM item = {
			LVIF_STATE | LVIF_PARAM,
			nItem,
			0,
			0,
			LVIS_SELECTED
		};
		ListView_GetItem(hwndList, &item);
		
		AddressBookAddress* pAddress =
			reinterpret_cast<AddressBookAddress*>(item.lParam);
		wstring_ptr wstrValue(pAddress->getValue());
		W2T(wstrValue.get(), ptszValue);
		
		std::auto_ptr<Item> pItem(new Item(wstrValue, type));
		LVITEM newItem = {
			LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM,
			ListView_GetItemCount(hwndSelected),
			0,
			0,
			0,
			const_cast<LPTSTR>(ptszValue),
			0,
			type,
			reinterpret_cast<LPARAM>(pItem.get())
		};
		ListView_InsertItem(hwndSelected, &newItem);
		pItem.release();
	}
	
	ListView_SortItems(hwndSelected, &selectedItemComp, 0);
}

void qm::SelectAddressDialog::remove()
{
	HWND hwndSelected = getDlgItem(IDC_SELECTEDADDRESS);
	
	for (int n = ListView_GetItemCount(hwndSelected) - 1; n >= 0; --n) {
		LVITEM item = {
			LVIF_STATE | LVIF_PARAM,
			n,
			0,
			0,
			LVIS_SELECTED
		};
		ListView_GetItem(hwndSelected, &item);
		if (item.state & LVIS_SELECTED) {
			delete reinterpret_cast<Item*>(item.lParam);
			ListView_DeleteItem(hwndSelected, n);
		}
	}
}

void qm::SelectAddressDialog::layout()
{
#if !defined _WIN32_WCE || _WIN32_WCE < 300 || !defined _WIN32_WCE_PSPC
	RECT rect;
	getClientRect(&rect);
	
	HWND hwnds[] = {
		getDlgItem(IDOK),
		getDlgItem(IDCANCEL),
		getDlgItem(IDC_TO),
		getDlgItem(IDC_CC),
		getDlgItem(IDC_BCC),
		getDlgItem(IDC_REMOVE)
	};
	
	RECT rectButton;
	Window(hwnds[0]).getWindowRect(&rectButton);
	
	int nWidth = (rect.right - rect.left) -
		(rectButton.right - rectButton.left) - 5*3;
	int nLeftWidth = (nWidth - 5)*2/3;
	int nRightWidth = nWidth - 5 - nLeftWidth;
	int nHeight = rect.bottom - rect.top;
	int nButtonHeight = rectButton.bottom - rectButton.top;
	
	HDWP hdwp = beginDeferWindowPos(12);
	hdwp = Window(getDlgItem(IDC_CATEGORY)).deferWindowPos(hdwp, 0, 5, 5,
		nLeftWidth, nButtonHeight, SWP_NOZORDER | SWP_NOACTIVATE);
	hdwp = Window(getDlgItem(IDC_FILTER)).deferWindowPos(hdwp, 0, nLeftWidth + 5*2,
		5, nRightWidth, nButtonHeight, SWP_NOZORDER | SWP_NOACTIVATE);
	hdwp = Window(getDlgItem(IDC_ADDRESS)).deferWindowPos(hdwp, 0, 5, nButtonHeight + 5*2,
		nLeftWidth, nHeight - nButtonHeight - 5*3, SWP_NOZORDER | SWP_NOACTIVATE);
	hdwp = Window(getDlgItem(IDC_SELECTEDADDRESS)).deferWindowPos(
		hdwp, 0, nLeftWidth + 5*2, nButtonHeight + 5*2, nRightWidth,
		nHeight - nButtonHeight - 5*3, SWP_NOZORDER | SWP_NOACTIVATE);
	
	int nDx[] = { 1, 5, 1, 1, 5, 0 };
	int nTop = 5;
	for (int n = 0; n < countof(hwnds); ++n) {
		hdwp = Window(hwnds[n]).deferWindowPos(hdwp, 0, nLeftWidth + nRightWidth + 5*3,
			nTop, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
		nTop += nButtonHeight + nDx[n];
	}
	
	hdwp = Window(getDlgItem(IDC_FILTERLABEL)).deferWindowPos(
		hdwp, 0, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOACTIVATE);
	
#ifndef _WIN32_WCE
	hdwp = Window(getDlgItem(IDC_SIZEGRIP)).deferWindowPos(hdwp, 0,
		rect.right - rect.left - 13, rect.bottom - rect.top - 12,
		13, 12, SWP_NOZORDER | SWP_NOACTIVATE);
#endif
	
	endDeferWindowPos(hdwp);
#endif
}

HMENU qm::SelectAddressDialog::createCategoryMenu(const AddressBook::CategoryList& l,
												  CategoryNameList* pList)
{
	assert(pList);
	
	AutoMenuHandle hmenu(::CreatePopupMenu());
	
	typedef std::vector<std::pair<HMENU, WSTRING> > MenuStack;
	MenuStack stackMenu;
	stackMenu.push_back(MenuStack::value_type(hmenu.get(), 0));
	
	struct Deleter
	{
		typedef std::vector<std::pair<HMENU, WSTRING> > MenuStack;
		
		Deleter(MenuStack& s) :
			s_(s)
		{
		}
		
		~Deleter()
		{
			std::for_each(s_.begin(), s_.end(),
				unary_compose_f_gx(
					string_free<WSTRING>(),
					std::select2nd<MenuStack::value_type>()));
		}
		
		MenuStack& s_;
	} deleter(stackMenu);
	
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	wstring_ptr wstrThisCategory(loadString(hInst, IDS_THISCATEGORY));
	W2T(wstrThisCategory.get(), ptszThisCategory);
	
	UINT nId = IDM_ADDRESSBOOK_CATEGORY;
	for (AddressBook::CategoryList::const_iterator it = l.begin(); it != l.end(); ++it) {
		const AddressBookCategory* pCategory = *it;
		
		size_t nLevel = getCategoryLevel(pCategory->getName());
		
		while (true) {
			bool bPop = false;
			if (nLevel < stackMenu.size()) {
				bPop = true;
			}
			else if (stackMenu.size() > 1) {
				wstring_ptr wstrName(getCategoryName(
					pCategory->getName(), stackMenu.size() - 2, false));
				if (wcscmp(wstrName.get(), stackMenu.back().second) != 0)
					bPop = true;
			}
			if (!bPop)
				break;
			freeWString(stackMenu.back().second);
			stackMenu.pop_back();
		}
		
		while (nLevel >= stackMenu.size()) {
			wstring_ptr wstrName(getCategoryName(
				pCategory->getName(), stackMenu.size() - 1, false));
			
			wstring_ptr wstrText(UIUtil::formatMenu(wstrName.get()));
			W2T(wstrText.get(), ptszText);
			
			bool bSubMenu = false;
			if (nLevel > stackMenu.size()) {
				bSubMenu = true;
			}
			else {
				if (it + 1 != l.end()) {
					const WCHAR* pwszNext = (*(it + 1))->getName();
					size_t nLen = wcslen(pCategory->getName());
					bSubMenu = wcsncmp(pCategory->getName(), pwszNext, nLen) == 0 &&
						*(pwszNext + nLen) == L'/';
				}
			}
			
			wstring_ptr wstrFullName(getCategoryName(
				pCategory->getName(), stackMenu.size() - 1, true));
			bool bCheck = wstrCategory_.get() &&
				wcscmp(wstrFullName.get(), wstrCategory_.get()) == 0;
			pList->push_back(wstrFullName.get());
			wstrFullName.release();
			
			unsigned int nFlags = MF_STRING | (bCheck ? MF_CHECKED : 0);
			if (bSubMenu) {
				HMENU hSubMenu = ::CreatePopupMenu();
				::AppendMenu(stackMenu.back().first, MF_POPUP,
					reinterpret_cast<UINT_PTR>(hSubMenu), ptszText);
				stackMenu.push_back(std::make_pair(hSubMenu, wstrName.get()));
				wstrName.release();
				::AppendMenu(hSubMenu, nFlags, nId++, ptszThisCategory);
			}
			else {
				::AppendMenu(stackMenu.back().first, nFlags, nId++, ptszText);
				break;
			}
		}
	}
	
	::AppendMenu(hmenu.get(), MF_SEPARATOR, -1, 0);
	
	wstring_ptr wstrAll(loadString(hInst, IDS_ALLCATEGORY));
	W2T(wstrAll.get(), ptszAll);
	::AppendMenu(hmenu.get(), MF_STRING | (!wstrCategory_.get() ? MF_CHECKED : 0),
		IDM_ADDRESSBOOK_ALLCATEGORY, ptszAll);
	
	return hmenu.release();
}

void qm::SelectAddressDialog::setCurrentCategory(const WCHAR* pwszCategory)
{
	if (pwszCategory)
		wstrCategory_ = allocWString(pwszCategory);
	else
		wstrCategory_.reset(0);
	
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	wstring_ptr wstrTitle(loadString(hInst, IDS_CATEGORY));
	wstring_ptr wstrAll(loadString(hInst, IDS_CATEGORYALL));
	
	ConcatW c[] = {
		{ wstrTitle.get(),												-1	},
		{ L" (",														2	},
		{ wstrCategory_.get() ? wstrCategory_.get() : wstrAll.get(),	-1	},
		{ L")",															1	}
	};
	wstring_ptr wstrButton(concat(c, countof(c)));
	setDlgItemText(IDC_CATEGORY, wstrButton.get());
	
	update();
}

bool qm::SelectAddressDialog::isCategory(const AddressBookAddress::CategoryList& listCategory) const
{
	if (!wstrCategory_.get())
		return true;
	
	size_t nLen = wcslen(wstrCategory_.get());
	
	for (AddressBookAddress::CategoryList::const_iterator it = listCategory.begin(); it != listCategory.end(); ++it) {
		const AddressBookCategory* pCategory = *it;
		const WCHAR* pwszCategory = pCategory->getName();
		
		if (wcscmp(pwszCategory, wstrCategory_.get()) == 0)
			return true;
		else if (wcslen(pwszCategory) > nLen &&
			wcsncmp(pwszCategory, wstrCategory_.get(), nLen) == 0 &&
			*(pwszCategory + nLen) == L'/')
			return true;
	}
	
	return false;
}

bool qm::SelectAddressDialog::isMatchFilter(const AddressBookEntry* pEntry) const
{
	if (!wstrFilter_.get())
		return true;
	
	wstring_ptr wstrName(tolower(pEntry->getName()));
	return wcsstr(wstrName.get(), wstrFilter_.get()) != 0;
}

bool qm::SelectAddressDialog::isMatchFilter(const AddressBookAddress* pAddress) const
{
	if (!wstrFilter_.get())
		return true;
	return wcsstr(pAddress->getAddress(), wstrFilter_.get()) != 0;
}

size_t qm::SelectAddressDialog::getCategoryLevel(const WCHAR* pwszCategory)
{
	assert(pwszCategory);
	return std::count(pwszCategory, pwszCategory + wcslen(pwszCategory), L'/') + 1;
}

wstring_ptr qm::SelectAddressDialog::getCategoryName(const WCHAR* pwszCategory,
													 size_t nLevel,
													 bool bFull)
{
	assert(pwszCategory);
	
	const WCHAR* p = pwszCategory;
	while (nLevel != 0) {
		while (*p++ != L'/')
			;
		--nLevel;
	}
	
	const WCHAR* pEnd = wcschr(p, L'/');
	
	wstring_ptr wstrName;
	if (bFull) {
		size_t nLen = pEnd ? pEnd - pwszCategory : wcslen(pwszCategory);
		wstrName = allocWString(pwszCategory, nLen);
	}
	else {
		size_t nLen = pEnd ? pEnd - p : wcslen(p);
		wstrName = allocWString(p, nLen);
	}
	return wstrName;
}


/****************************************************************************
 *
 * SelectAddressDialog::Item
 *
 */

qm::SelectAddressDialog::Item::Item(wstring_ptr wstrValue,
									Type type) :
	wstrValue_(wstrValue),
	type_(type)
{
}

qm::SelectAddressDialog::Item::~Item()
{
}

const WCHAR* qm::SelectAddressDialog::Item::getValue() const
{
	return wstrValue_.get();
}

wstring_ptr qm::SelectAddressDialog::Item::releaseValue()
{
	return wstrValue_;
}

SelectAddressDialog::Type qm::SelectAddressDialog::Item::getType() const
{
	return type_;
}

void qm::SelectAddressDialog::Item::setType(Type type)
{
	type_ = type;
}


/****************************************************************************
 *
 * SelectAddressDialog::AddressListWindow
 *
 */

qm::SelectAddressDialog::AddressListWindow::AddressListWindow(SelectAddressDialog* pDialog) :
	WindowBase(false),
	pDialog_(pDialog)
{
	setWindowHandler(this, false);
}

qm::SelectAddressDialog::AddressListWindow::~AddressListWindow()
{
}

LRESULT qm::SelectAddressDialog::AddressListWindow::windowProc(UINT uMsg,
															   WPARAM wParam,
															   LPARAM lParam)
{
	BEGIN_MESSAGE_HANDLER()
		HANDLE_CHAR()
	END_MESSAGE_HANDLER()
	return DefaultWindowHandler::windowProc(uMsg, wParam, lParam);
}

LRESULT qm::SelectAddressDialog::AddressListWindow::onChar(UINT nChar,
														   UINT nRepeat,
														   UINT nFlags)
{
	if (nChar == L' ') {
		pDialog_->select(SelectAddressDialog::TYPE_TO);
		return 0;
	}
	return DefaultWindowHandler::onChar(nChar, nRepeat, nFlags);
}


/****************************************************************************
 *
 * SelectAddressDialog::SelectedAddressListWindow
 *
 */

qm::SelectAddressDialog::SelectedAddressListWindow::SelectedAddressListWindow(SelectAddressDialog* pDialog) :
	WindowBase(false),
	pDialog_(pDialog)
{
	setWindowHandler(this, false);
}

qm::SelectAddressDialog::SelectedAddressListWindow::~SelectedAddressListWindow()
{
}

bool qm::SelectAddressDialog::SelectedAddressListWindow::preSubclassWindow()
{
#if defined _WIN32_WCE && _WIN32_WCE >= 400 && defined _WIN32_WCE_PSPC
	pDialog_->addNotifyHandler(this);
#endif
	return true;
}

LRESULT qm::SelectAddressDialog::SelectedAddressListWindow::windowProc(UINT uMsg,
																	   WPARAM wParam,
																	   LPARAM lParam)
{
	BEGIN_MESSAGE_HANDLER()
		HANDLE_CONTEXTMENU()
		HANDLE_DESTROY()
		HANDLE_LBUTTONDOWN()
	END_MESSAGE_HANDLER()
	return DefaultWindowHandler::windowProc(uMsg, wParam, lParam);
}

LRESULT qm::SelectAddressDialog::SelectedAddressListWindow::onContextMenu(HWND hwnd,
																		  const POINT& pt)
{
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	AutoMenuHandle hmenu(::LoadMenu(hInst, MAKEINTRESOURCE(IDR_ADDRESSBOOK)));
	HMENU hmenuSub = ::GetSubMenu(hmenu.get(), 0);
	UINT nFlags = TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD;
#ifndef _WIN32_WCE
	nFlags |= TPM_LEFTBUTTON | TPM_RIGHTBUTTON;
#endif
	UINT nId = ::TrackPopupMenu(hmenuSub, nFlags, pt.x, pt.y, 0, getHandle(), 0);
	if (nId == IDM_ADDRESSBOOK_REMOVE) {
		pDialog_->remove();
	}
	else {
		struct {
			UINT nId_;
			Type type_;
		} types[] = {
			{ IDM_ADDRESSBOOK_CHANGETO,		TYPE_TO		},
			{ IDM_ADDRESSBOOK_CHANGECC,		TYPE_CC		},
			{ IDM_ADDRESSBOOK_CHANGEBCC,	TYPE_BCC	}
		};
		Type type = static_cast<Type>(-1);
		for (int n = 0; n < countof(types) && type == -1; ++n) {
			if (types[n].nId_ == nId)
				type = types[n].type_;
		}
		if (type == -1)
			return 0;
		
		int nItem = -1;
		while (true) {
			nItem = ListView_GetNextItem(getHandle(), nItem, LVNI_ALL | LVNI_SELECTED);
			if (nItem == -1)
				break;
			
			LVITEM item = {
				LVIF_PARAM,
				nItem
			};
			ListView_GetItem(getHandle(), &item);
			
			Item* pItem = reinterpret_cast<Item*>(item.lParam);
			pItem->setType(type);
			
			item.mask = LVIF_IMAGE;
			item.iImage = type;
			ListView_SetItem(getHandle(), &item);
		}
		ListView_SortItems(getHandle(), &selectedItemComp, 0);
	}
	
	return 0;
}

LRESULT qm::SelectAddressDialog::SelectedAddressListWindow::onDestroy()
{
#if defined _WIN32_WCE && _WIN32_WCE >= 400 && defined _WIN32_WCE_PSPC
	pDialog_->removeNotifyHandler(this);
#endif
	return DefaultWindowHandler::onDestroy();
}

LRESULT qm::SelectAddressDialog::SelectedAddressListWindow::onLButtonDown(UINT nFlags,
																		  const POINT& pt)
{
#if defined _WIN32_WCE && _WIN32_WCE >= 300 && _WIN32_WCE < 400 && defined _WIN32_WCE_PSPC
	if (tapAndHold(pt))
		return 0;
#endif
	return DefaultWindowHandler::onLButtonDown(nFlags, pt);
}

LRESULT qm::SelectAddressDialog::SelectedAddressListWindow::onNotify(NMHDR* pnmhdr,
																	 bool* pbHandled)
{
	BEGIN_NOTIFY_HANDLER()
#if defined _WIN32_WCE && _WIN32_WCE >= 400 && defined _WIN32_WCE_PSPC
		HANDLE_NOTIFY(NM_RECOGNIZEGESTURE, IDC_SELECTEDADDRESS, onRecognizeGesture)
#endif
	END_NOTIFY_HANDLER()
	return NotifyHandler::onNotify(pnmhdr, pbHandled);
}

#if defined _WIN32_WCE && _WIN32_WCE >= 400 && defined _WIN32_WCE_PSPC
LRESULT qm::SelectAddressDialog::SelectedAddressListWindow::onRecognizeGesture(NMHDR* pnmhdr,
																			   bool* pbHandled)
{
	*pbHandled = true;
	return TRUE;
}
#endif
