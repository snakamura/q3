/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmaccount.h>
#include <qmdocument.h>
#include <qmfoldercombobox.h>

#include <qsaccelerator.h>
#include <qsconv.h>
#include <qsdevicecontext.h>
#include <qserror.h>
#include <qsinit.h>
#include <qskeymap.h>
#include <qsmenu.h>
#include <qsnew.h>
#include <qsprofile.h>
#include <qsstring.h>
#include <qsuiutil.h>

#include <algorithm>

#include <windowsx.h>

#include "foldercombobox.h"
#include "foldermodel.h"
#include "keymap.h"
#include "resourceinc.h"
#include "uiutil.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * FolderComboBoxImpl
 *
 */

class qm::FolderComboBoxImpl :
	public CommandHandler,
	public DefaultDocumentHandler,
	public DefaultAccountHandler,
	public FolderHandler,
	public FolderModelHandler
{
public:
	enum {
		WM_FOLDERCOMBOBOX_MESSAGEADDED		= WM_APP + 1101,
		WM_FOLDERCOMBOBOX_MESSAGEREMOVED	= WM_APP + 1102,
		WM_FOLDERCOMBOBOX_MESSAGECHANGED	= WM_APP + 1103
	};

public:
	Account* getAccount(int nIndex) const;
	Account* getSelectedAccount() const;
	Folder* getFolder(int nIndex) const;
	Folder* getSelectedFolder() const;
	int getIndexFromAccount(Account* pAccount) const;
	int getIndexFromFolder(Folder* pFolder) const;
	QSTATUS update(Folder* pFolder);

public:
	virtual LRESULT onCommand(WORD nCode, WORD nId);
	virtual void updateCommand(CommandUpdate* pcu);

public:
	virtual QSTATUS accountListChanged(
		const AccountListChangedEvent& event);

public:
	virtual QSTATUS folderListChanged(const FolderListChangedEvent& event);

public:
	virtual QSTATUS messageAdded(const FolderEvent& event);
	virtual QSTATUS messageRemoved(const FolderEvent& event);
	virtual QSTATUS messageChanged(const MessageEvent& event);
	virtual QSTATUS folderDestroyed(const FolderEvent& event);

public:
	virtual QSTATUS accountSelected(const FolderModelEvent& event);
	virtual QSTATUS folderSelected(const FolderModelEvent& event);

private:
	QSTATUS clearAccountList();
	QSTATUS updateAccountList(bool bDropDown);
	QSTATUS refreshFolderList(Account* pAccount, bool bDropDown);
	QSTATUS addAccount(Account* pAccount, bool bDropDown);
	QSTATUS removeAccount(Account* pAccount);
	QSTATUS insertFolders(int nIndex, Account* pAccount, bool bDropDown);
	QSTATUS insertFolder(int nIndex, Folder* pFolder, bool bDropDown);

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
	Accelerator* pAccelerator_;
	Document* pDocument_;
	
	UINT nId_;
	HFONT hfont_;
	bool bShowAllCount_;
	bool bShowUnseenCount_;
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

QSTATUS qm::FolderComboBoxImpl::update(Folder* pFolder)
{
	assert(pFolder);
	
	DECLARE_QSTATUS();
	
	bool bDropDown = ComboBox_GetDroppedState(pThis_->getHandle()) != 0;
	
	int nSelectedItem = ComboBox_GetCurSel(pThis_->getHandle());
	
	status = pFolder->removeFolderHandler(this);
	CHECK_QSTATUS();
	
	int nIndex = getIndexFromFolder(pFolder);
	ComboBox_DeleteString(pThis_->getHandle(), nIndex);
	status = insertFolder(nIndex, pFolder, bDropDown);
	CHECK_QSTATUS();
	
	ComboBox_SetCurSel(pThis_->getHandle(), nSelectedItem);
	
	return QSTATUS_SUCCESS;
}

LRESULT qm::FolderComboBoxImpl::onCommand(WORD nCode, WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID_CODE(nId_, CBN_CLOSEUP, onCloseUp)
		HANDLE_COMMAND_ID_CODE(nId_, CBN_DROPDOWN, onDropDown)
		HANDLE_COMMAND_ID_CODE(nId_, CBN_SELENDOK, onSelEndOk)
	END_COMMAND_HANDLER()
	return 1;
}

void qm::FolderComboBoxImpl::updateCommand(CommandUpdate* pcu)
{
}

QSTATUS qm::FolderComboBoxImpl::accountListChanged(
	const AccountListChangedEvent& event)
{
	DECLARE_QSTATUS();
	
	bool bDropDown = ComboBox_GetDroppedState(pThis_->getHandle()) != 0;
	
	switch (event.getType()) {
	case AccountListChangedEvent::TYPE_ALL:
		status = updateAccountList(bDropDown);
		CHECK_QSTATUS();
		break;
	case AccountListChangedEvent::TYPE_ADD:
		status = addAccount(event.getAccount(), bDropDown);
		CHECK_QSTATUS();
		break;
	case AccountListChangedEvent::TYPE_REMOVE:
		status = removeAccount(event.getAccount());
		CHECK_QSTATUS();
		break;
	case AccountListChangedEvent::TYPE_RENAME:
		// TODO
		break;
	default:
		break;
	}
	
	return status;
}

QSTATUS qm::FolderComboBoxImpl::folderListChanged(const FolderListChangedEvent& event)
{
	DECLARE_QSTATUS();
	
	bool bDropDown = ComboBox_GetDroppedState(pThis_->getHandle()) != 0;
	
	switch (event.getType()) {
	case FolderListChangedEvent::TYPE_ALL:
		status = refreshFolderList(event.getAccount(), bDropDown);
		CHECK_QSTATUS();
		break;
	case FolderListChangedEvent::TYPE_ADD:
		// TODO
		status = refreshFolderList(event.getAccount(), bDropDown);
		CHECK_QSTATUS();
		break;
	case FolderListChangedEvent::TYPE_REMOVE:
		// TODO
		status = refreshFolderList(event.getAccount(), bDropDown);
		CHECK_QSTATUS();
		break;
	case FolderListChangedEvent::TYPE_RENAME:
		// TODO
		status = refreshFolderList(event.getAccount(), bDropDown);
		CHECK_QSTATUS();
		break;
	case FolderListChangedEvent::TYPE_SHOW:
	case FolderListChangedEvent::TYPE_HIDE:
		// TODO
		status = refreshFolderList(event.getAccount(), bDropDown);
		CHECK_QSTATUS();
		break;
	default:
		assert(false);
		return QSTATUS_FAIL;
	}
	
	return status;
}

QSTATUS qm::FolderComboBoxImpl::messageAdded(const FolderEvent& event)
{
	pThis_->postMessage(WM_FOLDERCOMBOBOX_MESSAGEADDED,
		0, reinterpret_cast<LPARAM>(event.getFolder()));
	return QSTATUS_SUCCESS;
}

QSTATUS qm::FolderComboBoxImpl::messageRemoved(const FolderEvent& event)
{
	pThis_->postMessage(WM_FOLDERCOMBOBOX_MESSAGEREMOVED,
		0, reinterpret_cast<LPARAM>(event.getFolder()));
	return QSTATUS_SUCCESS;
}

QSTATUS qm::FolderComboBoxImpl::messageChanged(const MessageEvent& event)
{
	if ((event.getOldFlags() & MessageHolder::FLAG_SEEN) !=
		(event.getNewFlags() & MessageHolder::FLAG_SEEN))
		pThis_->postMessage(WM_FOLDERCOMBOBOX_MESSAGECHANGED,
			0, reinterpret_cast<LPARAM>(event.getFolder()));
	return QSTATUS_SUCCESS;
}

QSTATUS qm::FolderComboBoxImpl::folderDestroyed(const FolderEvent& event)
{
	return QSTATUS_SUCCESS;
}

QSTATUS qm::FolderComboBoxImpl::accountSelected(const FolderModelEvent& event)
{
	int nIndex = getIndexFromAccount(event.getAccount());
	if (nIndex != -1 && nIndex != ComboBox_GetCurSel(pThis_->getHandle()))
		ComboBox_SetCurSel(pThis_->getHandle(), nIndex);
	return QSTATUS_SUCCESS;
}

QSTATUS qm::FolderComboBoxImpl::folderSelected(const FolderModelEvent& event)
{
	int nIndex = getIndexFromFolder(event.getFolder());
	if (nIndex != -1 && nIndex != ComboBox_GetCurSel(pThis_->getHandle()))
		ComboBox_SetCurSel(pThis_->getHandle(), nIndex);
	return QSTATUS_SUCCESS;
}

QSTATUS qm::FolderComboBoxImpl::clearAccountList()
{
	DECLARE_QSTATUS();
	
	int nCount = ComboBox_GetCount(pThis_->getHandle());
	for (int n = 0; n < nCount; ++n) {
		Account* pAccount = getAccount(n);
		if (pAccount) {
			status = pAccount->removeAccountHandler(this);
			CHECK_QSTATUS();
		}
		else {
			Folder* pFolder = getFolder(n);
			assert(pFolder);
			status = pFolder->removeFolderHandler(this);
			CHECK_QSTATUS();
		}
	}
	
	ComboBox_ResetContent(pThis_->getHandle());
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::FolderComboBoxImpl::updateAccountList(bool bDropDown)
{
	DECLARE_QSTATUS();
	
	status = clearAccountList();
	CHECK_QSTATUS();
	
	const Document::AccountList& l = pDocument_->getAccounts();
	Document::AccountList::const_iterator it = l.begin();
	while (it != l.end()) {
		status = addAccount(*it++, bDropDown);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::FolderComboBoxImpl::refreshFolderList(Account* pAccount, bool bDropDown)
{
	DECLARE_QSTATUS();
	
	int nIndex = getIndexFromAccount(pAccount);
	assert(nIndex != -1);
	++nIndex;
	
	while (nIndex < ComboBox_GetCount(pThis_->getHandle())) {
		Folder* pFolder = getFolder(nIndex);
		if (!pFolder)
			break;
		status = pFolder->removeFolderHandler(this);
		CHECK_QSTATUS();
		ComboBox_DeleteString(pThis_->getHandle(), nIndex);
	}
	
	status = insertFolders(nIndex - 1, pAccount, bDropDown);
	CHECK_QSTATUS();
	
	ComboBox_SetCurSel(pThis_->getHandle(), nIndex - 1);
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::FolderComboBoxImpl::addAccount(Account* pAccount, bool bDropDown)
{
	assert(pAccount);
	
	DECLARE_QSTATUS();
	
	int nCount = ComboBox_GetCount(pThis_->getHandle());
	int nInsert = 0;
	for (nInsert = 0; nInsert < nCount; ++nInsert) {
		Account* p = getAccount(nInsert);
		if (p && wcscmp(p->getName(), pAccount->getName()) > 0)
			break;
	}
	
	string_ptr<WSTRING> wstrName(concat(L"[", pAccount->getName(), L"]"));
	if (!wstrName.get())
		return QSTATUS_OUTOFMEMORY;
	
	W2T(wstrName.get(), ptszName);
	int nIndex = ComboBox_InsertString(pThis_->getHandle(), nInsert, ptszName);
	if (nIndex == CB_ERR)
		return QSTATUS_FAIL;
	ComboBox_SetItemData(pThis_->getHandle(), nIndex,
		reinterpret_cast<LPARAM>(pAccount));
	
	status = insertFolders(nIndex, pAccount, bDropDown);
	CHECK_QSTATUS();
	
	status = pAccount->addAccountHandler(this);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::FolderComboBoxImpl::removeAccount(Account* pAccount)
{
	DECLARE_QSTATUS();
	
	int nIndex = getIndexFromAccount(pAccount);
	assert(nIndex != -1);
	++nIndex;
	
	while (nIndex < ComboBox_GetCount(pThis_->getHandle())) {
		Folder* pFolder = getFolder(nIndex);
		if (!pFolder)
			break;
		ComboBox_DeleteString(pThis_->getHandle(), nIndex);
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::FolderComboBoxImpl::insertFolders(
	int nIndex, Account* pAccount, bool bDropDown)
{
	assert(nIndex != CB_ERR);
	assert(pAccount);
	
	DECLARE_QSTATUS();
	
	const Account::FolderList& l = pAccount->getFolders();
	Account::FolderList listFolder;
	status = STLWrapper<Account::FolderList>(listFolder).resize(l.size());
	CHECK_QSTATUS();
	std::copy(l.begin(), l.end(), listFolder.begin());
	std::sort(listFolder.begin(), listFolder.end(), FolderLess());
	
	Account::FolderList::const_iterator it = listFolder.begin();
	while (it != listFolder.end()) {
		Folder* pFolder = *it;
		
		if (!pFolder->isHidden()) {
			++nIndex;
			status = insertFolder(nIndex, pFolder, bDropDown);
			CHECK_QSTATUS();
		}
		
		++it;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::FolderComboBoxImpl::insertFolder(
	int nIndex, Folder* pFolder, bool bDropDown)
{
	assert(pFolder);
	
	DECLARE_QSTATUS();
	
	StringBuffer<WSTRING> buf(&status);
	CHECK_QSTATUS();
	if (bDropDown) {
		for (unsigned int n = 0; n <= pFolder->getLevel(); ++n) {
			status = buf.append(L" ");
			CHECK_QSTATUS();
		}
		status = buf.append(pFolder->getName());
		CHECK_QSTATUS();
		
	}
	else {
		status = buf.append(L"[");
		CHECK_QSTATUS();
		status = buf.append(pFolder->getAccount()->getName());
		CHECK_QSTATUS();
		status = buf.append(L"] ");
		CHECK_QSTATUS();
		
		string_ptr<WSTRING> wstrFullName;
		status = pFolder->getFullName(&wstrFullName);
		CHECK_QSTATUS();
		status = buf.append(wstrFullName.get());
		CHECK_QSTATUS();
	}
	
	WCHAR wsz[64] = L"";
	if (bShowAllCount_ && bShowUnseenCount_)
		swprintf(wsz, L" (%d/%d)",
			pFolder->getUnseenCount(), pFolder->getCount());
	else if (bShowAllCount_)
		swprintf(wsz, L" (%d)", pFolder->getCount());
	else if (bShowUnseenCount_)
		swprintf(wsz, L" (%d)", pFolder->getUnseenCount());
	
	status = buf.append(wsz);
	CHECK_QSTATUS();
	
	W2T(buf.getCharArray(), ptszName);
	int nFolderIndex = ComboBox_InsertString(
		pThis_->getHandle(), nIndex, ptszName);
	if (nFolderIndex == CB_ERR)
		return QSTATUS_FAIL;
	ComboBox_SetItemData(pThis_->getHandle(), nFolderIndex,
		reinterpret_cast<LPARAM>(pFolder) | 0x01);
	
	status = pFolder->addFolderHandler(this);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
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
	DECLARE_QSTATUS();
	
	Folder* pFolder = getSelectedFolder();
	if (pFolder)
		status = pFolderModel_->setCurrentFolder(pFolder, false);
	else
		status = pFolderModel_->setCurrentAccount(getSelectedAccount(), false);
	
	return 0;
}


/****************************************************************************
 *
 * FolderComboBox
 *
 */

qm::FolderComboBox::FolderComboBox(WindowBase* pParentWindow,
	FolderModel* pFolderModel, Profile* pProfile, QSTATUS* pstatus) :
	WindowBase(true, pstatus),
	DefaultWindowHandler(pstatus),
	pImpl_(0)
{
	if (*pstatus != QSTATUS_SUCCESS)
		return;
	
	DECLARE_QSTATUS();
	
	int nShowAllCount = 0;
	status = pProfile->getInt(L"FolderComboBox",
		L"ShowAllCount", 1, &nShowAllCount);
	CHECK_QSTATUS_SET(pstatus);
	int nShowUnseenCount = 0;
	status = pProfile->getInt(L"FolderComboBox",
		L"ShowUnseenCount", 1, &nShowUnseenCount);
	CHECK_QSTATUS_SET(pstatus);
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	pImpl_->pThis_ = this;
	pImpl_->pParentWindow_ = pParentWindow;
	pImpl_->pFolderModel_ = pFolderModel;
	pImpl_->pMenuManager_ = 0;
	pImpl_->pProfile_ = pProfile;
	pImpl_->pAccelerator_ = 0;
	pImpl_->pDocument_ = 0;
	pImpl_->nId_ = 0;
	pImpl_->hfont_ = 0;
	pImpl_->bShowAllCount_ = nShowAllCount != 0;
	pImpl_->bShowUnseenCount_ = nShowUnseenCount != 0;
	
	setWindowHandler(this, false);
	
	status = pParentWindow->addCommandHandler(pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	status = pFolderModel->addFolderModelHandler(pImpl_);
	CHECK_QSTATUS_SET(pstatus);
}

qm::FolderComboBox::~FolderComboBox()
{
	if (pImpl_) {
		delete pImpl_->pAccelerator_;
		delete pImpl_;
		pImpl_ = 0;
	}
}

QSTATUS qm::FolderComboBox::getSuperClass(WSTRING* pwstrSuperClass)
{
	assert(pwstrSuperClass);
	
	*pwstrSuperClass = allocWString(L"COMBOBOX");
	if (!*pwstrSuperClass)
		return QSTATUS_OUTOFMEMORY;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::FolderComboBox::preCreateWindow(CREATESTRUCT* pCreateStruct)
{
	pCreateStruct->style |= WS_VSCROLL | CBS_DROPDOWNLIST;
	return QSTATUS_SUCCESS;
}

QSTATUS qm::FolderComboBox::getAccelerator(Accelerator** ppAccelerator)
{
	assert(ppAccelerator);
	*ppAccelerator = pImpl_->pAccelerator_;
	return QSTATUS_SUCCESS;
}

LRESULT qm::FolderComboBox::windowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	BEGIN_MESSAGE_HANDLER()
		HANDLE_CONTEXTMENU()
		HANDLE_CREATE()
		HANDLE_DESTROY()
		HANDLE_LBUTTONDOWN()
		HANDLE_MESSAGE(FolderComboBoxImpl::WM_FOLDERCOMBOBOX_MESSAGEADDED, onMessageAdded)
		HANDLE_MESSAGE(FolderComboBoxImpl::WM_FOLDERCOMBOBOX_MESSAGEREMOVED, onMessageRemoved)
		HANDLE_MESSAGE(FolderComboBoxImpl::WM_FOLDERCOMBOBOX_MESSAGECHANGED, onMessageChanged)
	END_MESSAGE_HANDLER()
	return DefaultWindowHandler::windowProc(uMsg, wParam, lParam);
}

LRESULT qm::FolderComboBox::onContextMenu(HWND hwnd, const POINT& pt)
{
	DECLARE_QSTATUS();
	
	HMENU hmenu = 0;
	status = pImpl_->pMenuManager_->getMenu(L"folder", false, false, &hmenu);
	if (status == QSTATUS_SUCCESS) {
		UINT nFlags = TPM_LEFTALIGN | TPM_TOPALIGN;
#ifndef _WIN32_WCE
		nFlags |= TPM_LEFTBUTTON | TPM_RIGHTBUTTON;
#endif
		::TrackPopupMenu(hmenu, nFlags, pt.x, pt.y, 0, getParentFrame(), 0);
	}
	
	return DefaultWindowHandler::onContextMenu(hwnd, pt);
}

LRESULT qm::FolderComboBox::onCreate(CREATESTRUCT* pCreateStruct)
{
	if (DefaultWindowHandler::onCreate(pCreateStruct) == -1)
		return -1;
	
	DECLARE_QSTATUS();
	
	FolderComboBoxCreateContext* pContext =
		static_cast<FolderComboBoxCreateContext*>(pCreateStruct->lpCreateParams);
	pImpl_->pDocument_ = pContext->pDocument_;
	pImpl_->pMenuManager_ = pContext->pMenuManager_;
	pImpl_->pDocument_->addDocumentHandler(pImpl_);
	
	status = pContext->pKeyMap_->createAccelerator(
		CustomAcceleratorFactory(), L"FolderComboBox",
		mapKeyNameToId, countof(mapKeyNameToId), &pImpl_->pAccelerator_);
	CHECK_QSTATUS_VALUE(-1);
	
	pImpl_->nId_ = getWindowLong(GWL_ID);
	
	status = qs::UIUtil::createFontFromProfile(pImpl_->pProfile_,
		L"FolderComboBox", false, &pImpl_->hfont_);
	CHECK_QSTATUS_VALUE(-1);
	setFont(pImpl_->hfont_);
	
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
	
	return DefaultWindowHandler::onDestroy();
}

LRESULT qm::FolderComboBox::onLButtonDown(UINT nFlags, const POINT& pt)
{
#if defined _WIN32_WCE && _WIN32_WCE >= 300 && _WIN32_WCE_PSPC
	if (tapAndHold(pt))
		return 0;
#endif
	return DefaultWindowHandler::onLButtonDown(nFlags, pt);
}

LRESULT qm::FolderComboBox::onMessageAdded(WPARAM wParam, LPARAM lParam)
{
	pImpl_->update(reinterpret_cast<Folder*>(lParam));
	return 0;
}

LRESULT qm::FolderComboBox::onMessageRemoved(WPARAM wParam, LPARAM lParam)
{
	pImpl_->update(reinterpret_cast<Folder*>(lParam));
	return 0;
}

LRESULT qm::FolderComboBox::onMessageChanged(WPARAM wParam, LPARAM lParam)
{
	pImpl_->update(reinterpret_cast<NormalFolder*>(lParam));
	return 0;
}

bool qm::FolderComboBox::isShow() const
{
	return isVisible();
}

bool qm::FolderComboBox::isActive() const
{
	return hasFocus();
}

QSTATUS qm::FolderComboBox::setActive()
{
	setFocus();
	return QSTATUS_SUCCESS;
}
