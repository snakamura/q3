/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmaccount.h>
#include <qmdocument.h>
#include <qmfoldercombobox.h>

#include <qsaccelerator.h>
#include <qsconv.h>
#include <qsdevicecontext.h>
#include <qsinit.h>
#include <qskeymap.h>
#include <qsmenu.h>
#include <qsprofile.h>
#include <qsstring.h>
#include <qsuiutil.h>

#include <algorithm>

#include <windowsx.h>

#include "foldercombobox.h"
#include "keymap.h"
#include "resourceinc.h"
#include "uimanager.h"
#include "uiutil.h"
#include "../uimodel/foldermodel.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * FolderComboBoxImpl
 *
 */

class qm::FolderComboBoxImpl :
	public CommandHandler,
	public AccountManagerHandler,
	public DefaultAccountHandler,
	public DefaultFolderHandler,
	public FolderModelHandler
{
public:
	enum {
		WM_FOLDERCOMBOBOX_MESSAGEADDED		= WM_APP + 1401,
		WM_FOLDERCOMBOBOX_MESSAGEREMOVED	= WM_APP + 1402,
		WM_FOLDERCOMBOBOX_MESSAGEREFRESHED	= WM_APP + 1403,
		WM_FOLDERCOMBOBOX_MESSAGECHANGED	= WM_APP + 1404
	};

public:
	Account* getAccount(int nIndex) const;
	Account* getSelectedAccount() const;
	Folder* getFolder(int nIndex) const;
	Folder* getSelectedFolder() const;
	int getIndexFromAccount(Account* pAccount) const;
	int getIndexFromFolder(Folder* pFolder) const;
	void update(Folder* pFolder);
	void handleUpdateMessage(LPARAM lParam);
	void reloadProfiles(bool bInitialize);
#ifdef _WIN32_WCE
	void updateItemHeight();
#endif

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

public:
	virtual void accountListChanged(const AccountManagerEvent& event);

public:
	virtual void folderListChanged(const FolderListChangedEvent& event);

public:
	virtual void messageAdded(const FolderMessageEvent& event);
	virtual void messageRemoved(const FolderMessageEvent& event);
	virtual void messageRefreshed(const FolderEvent& event);
	virtual void unseenCountChanged(const FolderEvent& event);

public:
	virtual void accountSelected(const FolderModelEvent& event);
	virtual void folderSelected(const FolderModelEvent& event);

private:
	void clearAccountList();
	void updateAccountList(bool bDropDown);
	void refreshFolderList(Account* pAccount,
						   bool bDropDown);
	void addAccount(Account* pAccount,
					bool bDropDown);
	void removeAccount(Account* pAccount);
	void insertFolders(int nIndex,
					   Account* pAccount,
					   bool bDropDown);
	void insertFolder(int nIndex,
					  Folder* pFolder,
					  bool bDropDown);

private:
	LRESULT onCloseUp();
	LRESULT onDropDown();
	LRESULT onSelEndOk();
	
public:
	FolderComboBox* pThis_;
	WindowBase* pParentWindow_;
	FolderModel* pFolderModel_;
	MenuManager* pMenuManager_;
	Profile* pProfile_;
	std::auto_ptr<Accelerator> pAccelerator_;
	Document* pDocument_;
	
	UINT nId_;
	HFONT hfont_;
	bool bShowAllCount_;
	bool bShowUnseenCount_;
#ifdef _WIN32_WCE
	int nItemHeight_;
#endif
};

Account* qm::FolderComboBoxImpl::getAccount(int nIndex) const
{
	LPARAM lParam = ComboBox_GetItemData(pThis_->getHandle(), nIndex);
	if (!(lParam & 0x01))
		return reinterpret_cast<Account*>(lParam);
	else
		return 0;
}

Account* qm::FolderComboBoxImpl::getSelectedAccount() const
{
	int nIndex = ComboBox_GetCurSel(pThis_->getHandle());
	if (nIndex == CB_ERR)
		return 0;
	return getAccount(nIndex);
}

Folder* qm::FolderComboBoxImpl::getFolder(int nIndex) const
{
	LPARAM lParam = ComboBox_GetItemData(pThis_->getHandle(), nIndex);
	if (lParam & 0x01)
		return reinterpret_cast<Folder*>(lParam & ~0x01);
	else
		return 0;
}

Folder* qm::FolderComboBoxImpl::getSelectedFolder() const
{
	int nIndex = ComboBox_GetCurSel(pThis_->getHandle());
	if (nIndex == CB_ERR)
		return 0;
	return getFolder(nIndex);
}

int qm::FolderComboBoxImpl::getIndexFromAccount(Account* pAccount) const
{
	int nCount = ComboBox_GetCount(pThis_->getHandle());
	for (int n = 0; n < nCount; ++n) {
		if (getAccount(n) == pAccount)
			return n;
	}
	return -1;
}

int qm::FolderComboBoxImpl::getIndexFromFolder(Folder* pFolder) const
{
	int nCount = ComboBox_GetCount(pThis_->getHandle());
	for (int n = 0; n < nCount; ++n) {
		if (getFolder(n) == pFolder)
			return n;
	}
	return -1;
}

void qm::FolderComboBoxImpl::update(Folder* pFolder)
{
	assert(pFolder);
	
	bool bDropDown = ComboBox_GetDroppedState(pThis_->getHandle()) != 0;

	int nSelectedItem = ComboBox_GetCurSel(pThis_->getHandle());

	pFolder->removeFolderHandler(this);
	
	int nIndex = getIndexFromFolder(pFolder);
	ComboBox_DeleteString(pThis_->getHandle(), nIndex);
	insertFolder(nIndex, pFolder, bDropDown);
	
	ComboBox_SetCurSel(pThis_->getHandle(), nSelectedItem);
}

void qm::FolderComboBoxImpl::handleUpdateMessage(LPARAM lParam)
{
	MSG msg;
	while (true) {
		if (!::PeekMessage(&msg, pThis_->getHandle(),
			FolderComboBoxImpl::WM_FOLDERCOMBOBOX_MESSAGEADDED,
			FolderComboBoxImpl::WM_FOLDERCOMBOBOX_MESSAGECHANGED, PM_NOREMOVE))
			break;
		else if (msg.lParam != lParam)
			break;
		::PeekMessage(&msg, pThis_->getHandle(),
			FolderComboBoxImpl::WM_FOLDERCOMBOBOX_MESSAGEADDED,
			FolderComboBoxImpl::WM_FOLDERCOMBOBOX_MESSAGECHANGED, PM_REMOVE);
	}
	
	update(reinterpret_cast<Folder*>(lParam));
}

void qm::FolderComboBoxImpl::reloadProfiles(bool bInitialize)
{
	bShowAllCount_ = pProfile_->getInt(L"FolderComboBox", L"ShowAllCount", 1) != 0;
	bShowUnseenCount_ = pProfile_->getInt(L"FolderComboBox", L"ShowUnseenCount", 1) != 0;
	
	HFONT hfont = qs::UIUtil::createFontFromProfile(pProfile_, L"FolderComboBox", false);
	if (!bInitialize) {
		assert(hfont_);
		pThis_->setFont(hfont);
		::DeleteObject(hfont_);
	}
	hfont_ = hfont;
#ifdef _WIN32_WCE
	if (!bInitialize)
		updateItemHeight();
#endif
	
	if (!bInitialize) {
		int nIndex = ComboBox_GetCurSel(pThis_->getHandle());
		updateAccountList(false);
		ComboBox_SetCurSel(pThis_->getHandle(), nIndex);
	}
}

#ifdef _WIN32_WCE
void qm::FolderComboBoxImpl::updateItemHeight()
{
	ClientDeviceContext dc(pThis_->getHandle());
	ObjectSelector<HFONT> selector(dc, hfont_);
	TEXTMETRIC tm;
	dc.getTextMetrics(&tm);
	nItemHeight_ = tm.tmHeight + tm.tmExternalLeading + 2;
}
#endif

LRESULT qm::FolderComboBoxImpl::onCommand(WORD nCode,
										  WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID_CODE(nId_, CBN_CLOSEUP, onCloseUp)
		HANDLE_COMMAND_ID_CODE(nId_, CBN_DROPDOWN, onDropDown)
		HANDLE_COMMAND_ID_CODE(nId_, CBN_SELENDOK, onSelEndOk)
	END_COMMAND_HANDLER()
	return 1;
}

void qm::FolderComboBoxImpl::accountListChanged(const AccountManagerEvent& event)
{
	bool bDropDown = ComboBox_GetDroppedState(pThis_->getHandle()) != 0;
	
	switch (event.getType()) {
	case AccountManagerEvent::TYPE_ALL:
		updateAccountList(bDropDown);
		break;
	case AccountManagerEvent::TYPE_ADD:
		addAccount(event.getAccount(), bDropDown);
		break;
	case AccountManagerEvent::TYPE_REMOVE:
		removeAccount(event.getAccount());
		break;
	default:
		break;
	}
}

void qm::FolderComboBoxImpl::folderListChanged(const FolderListChangedEvent& event)
{
	bool bDropDown = ComboBox_GetDroppedState(pThis_->getHandle()) != 0;
	
	switch (event.getType()) {
	case FolderListChangedEvent::TYPE_ALL:
		refreshFolderList(event.getAccount(), bDropDown);
		break;
	case FolderListChangedEvent::TYPE_ADD:
		// TODO
		refreshFolderList(event.getAccount(), bDropDown);
		break;
	case FolderListChangedEvent::TYPE_REMOVE:
		// TODO
		refreshFolderList(event.getAccount(), bDropDown);
		break;
	case FolderListChangedEvent::TYPE_RENAME:
		// TODO
		refreshFolderList(event.getAccount(), bDropDown);
		break;
	case FolderListChangedEvent::TYPE_FLAGS:
		if ((event.getOldFlags() & (Folder::FLAG_HIDE | Folder::FLAG_BOX_MASK)) !=
			(event.getNewFlags() & (Folder::FLAG_HIDE | Folder::FLAG_BOX_MASK))) {
			// TODO
			refreshFolderList(event.getAccount(), bDropDown);
		}
		break;
	default:
		assert(false);
		break;
	}
}

void qm::FolderComboBoxImpl::messageAdded(const FolderMessageEvent& event)
{
	pThis_->postMessage(WM_FOLDERCOMBOBOX_MESSAGEADDED,
		0, reinterpret_cast<LPARAM>(event.getFolder()));
}

void qm::FolderComboBoxImpl::messageRemoved(const FolderMessageEvent& event)
{
	pThis_->postMessage(WM_FOLDERCOMBOBOX_MESSAGEREMOVED,
		0, reinterpret_cast<LPARAM>(event.getFolder()));
}

void qm::FolderComboBoxImpl::messageRefreshed(const FolderEvent& event)
{
	pThis_->postMessage(WM_FOLDERCOMBOBOX_MESSAGEREFRESHED,
		0, reinterpret_cast<LPARAM>(event.getFolder()));
}

void qm::FolderComboBoxImpl::unseenCountChanged(const FolderEvent& event)
{
	pThis_->postMessage(WM_FOLDERCOMBOBOX_MESSAGECHANGED,
		0, reinterpret_cast<LPARAM>(event.getFolder()));
}

void qm::FolderComboBoxImpl::accountSelected(const FolderModelEvent& event)
{
	Account* pAccount = event.getAccount();
	if (pAccount) {
		int nIndex = getIndexFromAccount(pAccount);
		if (nIndex != -1 && nIndex != ComboBox_GetCurSel(pThis_->getHandle()))
			ComboBox_SetCurSel(pThis_->getHandle(), nIndex);
	}
}

void qm::FolderComboBoxImpl::folderSelected(const FolderModelEvent& event)
{
	int nIndex = getIndexFromFolder(event.getFolder());
	if (nIndex != -1 && nIndex != ComboBox_GetCurSel(pThis_->getHandle()))
		ComboBox_SetCurSel(pThis_->getHandle(), nIndex);
}

void qm::FolderComboBoxImpl::clearAccountList()
{
	int nCount = ComboBox_GetCount(pThis_->getHandle());
	for (int n = 0; n < nCount; ++n) {
		Account* pAccount = getAccount(n);
		if (pAccount) {
			pAccount->removeAccountHandler(this);
		}
		else {
			Folder* pFolder = getFolder(n);
			assert(pFolder);
			pFolder->removeFolderHandler(this);
		}
	}
	
	ComboBox_ResetContent(pThis_->getHandle());
}

void qm::FolderComboBoxImpl::updateAccountList(bool bDropDown)
{
	clearAccountList();
	
	const Document::AccountList& l = pDocument_->getAccounts();
	for (Document::AccountList::const_iterator it = l.begin(); it != l.end(); ++it)
		addAccount(*it, bDropDown);
}

void qm::FolderComboBoxImpl::refreshFolderList(Account* pAccount,
											   bool bDropDown)
{
	int nIndex = getIndexFromAccount(pAccount);
	assert(nIndex != -1);
	++nIndex;
	
	while (nIndex < ComboBox_GetCount(pThis_->getHandle())) {
		Folder* pFolder = getFolder(nIndex);
		if (!pFolder)
			break;
		pFolder->removeFolderHandler(this);
		ComboBox_DeleteString(pThis_->getHandle(), nIndex);
	}
	
	insertFolders(nIndex - 1, pAccount, bDropDown);
	
	std::pair<Account*, Folder*> p(pFolderModel_->getCurrent());
	if (p.first)
		ComboBox_SetCurSel(pThis_->getHandle(), getIndexFromAccount(p.first));
	else if (p.second)
		ComboBox_SetCurSel(pThis_->getHandle(), getIndexFromFolder(p.second));
}

void qm::FolderComboBoxImpl::addAccount(Account* pAccount,
										bool bDropDown)
{
	assert(pAccount);
	
	int nCount = ComboBox_GetCount(pThis_->getHandle());
	int nInsert = 0;
	for (nInsert = 0; nInsert < nCount; ++nInsert) {
		Account* p = getAccount(nInsert);
		if (p && wcscmp(p->getName(), pAccount->getName()) > 0)
			break;
	}
	
	wstring_ptr wstrName(concat(L"[", pAccount->getName(), L"]"));
	W2T(wstrName.get(), ptszName);
	int nIndex = ComboBox_InsertString(pThis_->getHandle(), nInsert, ptszName);
	if (nIndex == CB_ERR)
		return;
	ComboBox_SetItemData(pThis_->getHandle(), nIndex,
		reinterpret_cast<LPARAM>(pAccount));
	
	insertFolders(nIndex, pAccount, bDropDown);
	
	pAccount->addAccountHandler(this);
}

void qm::FolderComboBoxImpl::removeAccount(Account* pAccount)
{
	pAccount->removeAccountHandler(this);
	
	int nIndex = getIndexFromAccount(pAccount);
	assert(nIndex != -1);
	ComboBox_DeleteString(pThis_->getHandle(), nIndex);
	
	while (nIndex < ComboBox_GetCount(pThis_->getHandle())) {
		Folder* pFolder = getFolder(nIndex);
		if (!pFolder)
			break;
		
		pFolder->removeFolderHandler(this);
		ComboBox_DeleteString(pThis_->getHandle(), nIndex);
	}
	ComboBox_SetCurSel(pThis_->getHandle(), -1);
}

void qm::FolderComboBoxImpl::insertFolders(int nIndex,
										   Account* pAccount,
										   bool bDropDown)
{
	assert(nIndex != CB_ERR);
	assert(pAccount);
	
	Account::FolderList listFolder(pAccount->getFolders());
	std::sort(listFolder.begin(), listFolder.end(), FolderLess());
	
	for (Account::FolderList::const_iterator it = listFolder.begin(); it != listFolder.end(); ++it) {
		Folder* pFolder = *it;
		
		if (!pFolder->isHidden()) {
			++nIndex;
			insertFolder(nIndex, pFolder, bDropDown);
		}
	}
}

void qm::FolderComboBoxImpl::insertFolder(int nIndex,
										  Folder* pFolder,
										  bool bDropDown)
{
	assert(pFolder);
	
	StringBuffer<WSTRING> buf;
	if (bDropDown) {
		for (unsigned int n = 0; n <= pFolder->getLevel(); ++n)
			buf.append(L" ");
		buf.append(pFolder->getName());
		
	}
	else {
		buf.append(L"[");
		buf.append(pFolder->getAccount()->getName());
		buf.append(L"] ");
		
		wstring_ptr wstrFullName(pFolder->getFullName());
		buf.append(wstrFullName.get());
	}
	
	WCHAR wsz[64] = L"";
	if (bShowAllCount_ && bShowUnseenCount_)
		swprintf(wsz, L" (%d/%d)",
			pFolder->getUnseenCount(), pFolder->getCount());
	else if (bShowAllCount_)
		swprintf(wsz, L" (%d)", pFolder->getCount());
	else if (bShowUnseenCount_)
		swprintf(wsz, L" (%d)", pFolder->getUnseenCount());
	
	buf.append(wsz);
	
	W2T(buf.getCharArray(), ptszName);
	int nFolderIndex = ComboBox_InsertString(
		pThis_->getHandle(), nIndex, ptszName);
	if (nFolderIndex == CB_ERR)
		return;
	ComboBox_SetItemData(pThis_->getHandle(), nFolderIndex,
		reinterpret_cast<LPARAM>(pFolder) | 0x01);
	
	pFolder->addFolderHandler(this);
}

LRESULT qm::FolderComboBoxImpl::onCloseUp()
{
	int nItem = ComboBox_GetCurSel(pThis_->getHandle());
	updateAccountList(false);
	ComboBox_SetCurSel(pThis_->getHandle(), nItem);
	return 0;
}

LRESULT qm::FolderComboBoxImpl::onDropDown()
{
	int nItem = ComboBox_GetCurSel(pThis_->getHandle());
	updateAccountList(true);
	ComboBox_SetCurSel(pThis_->getHandle(), nItem);
	return 0;
}

LRESULT qm::FolderComboBoxImpl::onSelEndOk()
{
	bool bDropDown = ComboBox_GetDroppedState(pThis_->getHandle()) != 0;
	Folder* pFolder = getSelectedFolder();
	if (pFolder)
		pFolderModel_->setCurrent(0, pFolder, !bDropDown);
	else
		pFolderModel_->setCurrent(getSelectedAccount(), 0, !bDropDown);
	return 0;
}


/****************************************************************************
 *
 * FolderComboBox
 *
 */

qm::FolderComboBox::FolderComboBox(WindowBase* pParentWindow,
								   FolderModel* pFolderModel,
								   Profile* pProfile) :
	WindowBase(true),
	pImpl_(0)
{
	pImpl_ = new FolderComboBoxImpl();
	pImpl_->pThis_ = this;
	pImpl_->pParentWindow_ = pParentWindow;
	pImpl_->pFolderModel_ = pFolderModel;
	pImpl_->pMenuManager_ = 0;
	pImpl_->pProfile_ = pProfile;
	pImpl_->pDocument_ = 0;
	pImpl_->nId_ = 0;
	pImpl_->hfont_ = 0;
	pImpl_->bShowAllCount_ = true;
	pImpl_->bShowUnseenCount_ = true;
#ifdef _WIN32_WCE
	pImpl_->nItemHeight_ = 0;
#endif
	
	pImpl_->reloadProfiles(true);
	
	setWindowHandler(this, false);
	
	pParentWindow->addCommandHandler(pImpl_);
	pFolderModel->addFolderModelHandler(pImpl_);
}

qm::FolderComboBox::~FolderComboBox()
{
	delete pImpl_;
}

void qm::FolderComboBox::reloadProfiles()
{
	pImpl_->reloadProfiles(false);
}

wstring_ptr qm::FolderComboBox::getSuperClass()
{
	return allocWString(L"COMBOBOX");
}

bool qm::FolderComboBox::preCreateWindow(CREATESTRUCT* pCreateStruct)
{
	if (!DefaultWindowHandler::preCreateWindow(pCreateStruct))
		return false;
	pCreateStruct->style |= WS_VSCROLL | CBS_DROPDOWNLIST;
	return true;
}

Accelerator* qm::FolderComboBox::getAccelerator()
{
	return pImpl_->pAccelerator_.get();
}

LRESULT qm::FolderComboBox::windowProc(UINT uMsg,
									   WPARAM wParam,
									   LPARAM lParam)
{
	BEGIN_MESSAGE_HANDLER()
		HANDLE_CONTEXTMENU()
		HANDLE_CREATE()
		HANDLE_DESTROY()
		HANDLE_LBUTTONDOWN()
#ifdef _WIN32_WCE
		HANDLE_WINDOWPOSCHANGED()
#endif
		HANDLE_MESSAGE(FolderComboBoxImpl::WM_FOLDERCOMBOBOX_MESSAGEADDED, onMessageAdded)
		HANDLE_MESSAGE(FolderComboBoxImpl::WM_FOLDERCOMBOBOX_MESSAGEREMOVED, onMessageRemoved)
		HANDLE_MESSAGE(FolderComboBoxImpl::WM_FOLDERCOMBOBOX_MESSAGEREFRESHED, onMessageRefreshed)
		HANDLE_MESSAGE(FolderComboBoxImpl::WM_FOLDERCOMBOBOX_MESSAGECHANGED, onMessageChanged)
	END_MESSAGE_HANDLER()
	return DefaultWindowHandler::windowProc(uMsg, wParam, lParam);
}

LRESULT qm::FolderComboBox::onContextMenu(HWND hwnd,
										  const POINT& pt)
{
	HMENU hmenu = pImpl_->pMenuManager_->getMenu(L"folder", false, false);
	if (hmenu) {
		UINT nFlags = TPM_LEFTALIGN | TPM_TOPALIGN;
#ifndef _WIN32_WCE
		nFlags |= TPM_LEFTBUTTON | TPM_RIGHTBUTTON;
#endif
		::TrackPopupMenu(hmenu, nFlags, pt.x, pt.y, 0, getParentFrame(), 0);
	}
	
	return 0;
}

LRESULT qm::FolderComboBox::onCreate(CREATESTRUCT* pCreateStruct)
{
	if (DefaultWindowHandler::onCreate(pCreateStruct) == -1)
		return -1;
	
	FolderComboBoxCreateContext* pContext =
		static_cast<FolderComboBoxCreateContext*>(pCreateStruct->lpCreateParams);
	pImpl_->pDocument_ = pContext->pDocument_;
	pImpl_->pMenuManager_ = pContext->pUIManager_->getMenuManager();
	pImpl_->pDocument_->addAccountManagerHandler(pImpl_);
	
	CustomAcceleratorFactory acceleratorFactory;
	pImpl_->pAccelerator_ = pContext->pUIManager_->getKeyMap()->createAccelerator(
		&acceleratorFactory, L"FolderComboBox", mapKeyNameToId, countof(mapKeyNameToId));
	if (!pImpl_->pAccelerator_.get())
		return -1;
	
	pImpl_->nId_ = getWindowLong(GWL_ID);
	
	setFont(pImpl_->hfont_);
#ifdef _WIN32_WCE
	pImpl_->updateItemHeight();
#endif
	
	return 0;
}

LRESULT qm::FolderComboBox::onDestroy()
{
	if (pImpl_->hfont_) {
		::DeleteObject(pImpl_->hfont_);
		pImpl_->hfont_ = 0;
	}
	
	pImpl_->pParentWindow_->removeCommandHandler(pImpl_);
	pImpl_->pFolderModel_->removeFolderModelHandler(pImpl_);
	pImpl_->pDocument_->removeAccountManagerHandler(pImpl_);
	
	return DefaultWindowHandler::onDestroy();
}

LRESULT qm::FolderComboBox::onLButtonDown(UINT nFlags,
										  const POINT& pt)
{
#if defined _WIN32_WCE && _WIN32_WCE >= 300 && defined _WIN32_WCE_PSPC
	if (tapAndHold(pt))
		return 0;
#endif
	return DefaultWindowHandler::onLButtonDown(nFlags, pt);
}

#ifdef _WIN32_WCE
LRESULT qm::FolderComboBox::onWindowPosChanged(WINDOWPOS* pWindowPos)
{
	if (sendMessage(CB_GETITEMHEIGHT, -1) != pImpl_->nItemHeight_)
		sendMessage(CB_SETITEMHEIGHT, -1, pImpl_->nItemHeight_);
	return DefaultWindowHandler::onWindowPosChanged(pWindowPos);
}
#endif

LRESULT qm::FolderComboBox::onMessageAdded(WPARAM wParam,
										   LPARAM lParam)
{
	pImpl_->handleUpdateMessage(lParam);
	return 0;
}

LRESULT qm::FolderComboBox::onMessageRemoved(WPARAM wParam,
											 LPARAM lParam)
{
	pImpl_->handleUpdateMessage(lParam);
	return 0;
}

LRESULT qm::FolderComboBox::onMessageRefreshed(WPARAM wParam,
											   LPARAM lParam)
{
	pImpl_->handleUpdateMessage(lParam);
	return 0;
}

LRESULT qm::FolderComboBox::onMessageChanged(WPARAM wParam,
											 LPARAM lParam)
{
	pImpl_->handleUpdateMessage(lParam);
	return 0;
}

bool qm::FolderComboBox::isShow() const
{
	return (getStyle() & WS_VISIBLE) != 0;
}

bool qm::FolderComboBox::isActive() const
{
	return hasFocus();
}

void qm::FolderComboBox::setActive()
{
	setFocus();
}
