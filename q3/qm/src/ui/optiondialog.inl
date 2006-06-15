/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __OPTIONDIALOG_INL__
#define __OPTIONDIALOG_INL__

#include <qmaccount.h>
#include <qmapplication.h>

#include "../model/condition.h"


/****************************************************************************
 *
 * AbstractOptionDialogPanel
 *
 */

template<class Dialog>
qm::AbstractOptionDialogPanel<Dialog>::AbstractOptionDialogPanel()
{
}

template<class Dialog>
qm::AbstractOptionDialogPanel<Dialog>::~AbstractOptionDialogPanel()
{
}

template<class Dialog>
HWND qm::AbstractOptionDialogPanel<Dialog>::getWindow()
{
	return static_cast<Dialog*>(this)->getHandle();
}


/****************************************************************************
 *
 * RuleColorSetsDialog
 *
 */

template<class T, class List, class Manager, class EditDialog>
qm::RuleColorSetsDialog<T, List, Manager, EditDialog>::RuleColorSetsDialog(Manager* pManager,
																		   AccountManager* pAccountManager,
																		   qs::Profile* pProfile,
																		   UINT nTitleId,
																		   PFN_GET pfnGet,
																		   PFN_SET pfnSet) :
	AbstractListDialog<T, List>(IDD_RULECOLORSETS, IDC_RULECOLORSETS, false),
	pManager_(pManager),
	pAccountManager_(pAccountManager),
	pProfile_(pProfile),
	nTitleId_(nTitleId),
	pfnSet_(pfnSet)
{
	const List& l = (pManager->*pfnGet)();
	List& list = getList();
	list.reserve(l.size());
	for (List::const_iterator it = l.begin(); it != l.end(); ++it)
		list.push_back(new T(**it));
}

template<class T, class List, class Manager, class EditDialog>
qm::RuleColorSetsDialog<T, List, Manager, EditDialog>::~RuleColorSetsDialog()
{
}

template<class T, class List, class Manager, class EditDialog>
INT_PTR qm::RuleColorSetsDialog<T, List, Manager, EditDialog>::dialogProc(UINT uMsg,
																		  WPARAM wParam,
																		  LPARAM lParam)
{
	BEGIN_DIALOG_HANDLER()
		HANDLE_SIZE()
	END_DIALOG_HANDLER()
	return AbstractListDialog<T, List>::dialogProc(uMsg, wParam, lParam);
}

template<class T, class List, class Manager, class EditDialog>
LRESULT qm::RuleColorSetsDialog<T, List, Manager, EditDialog>::onInitDialog(HWND hwndFocus,
																			LPARAM lParam)
{
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	wstring_ptr wstrTitle(loadString(hInst, nTitleId_));
	setWindowText(wstrTitle.get());
	
	return AbstractListDialog<T, List>::onInitDialog(hwndFocus, lParam);
}

template<class T, class List, class Manager, class EditDialog>
qs::wstring_ptr qm::RuleColorSetsDialog<T, List, Manager, EditDialog>::getLabel(const T* p) const
{
	StringBuffer<WSTRING> buf;
	if (p->getAccount())
		buf.append(p->getAccount());
	else
		buf.append(L'*');
	buf.append(L'/');
	if (p->getFolder())
		buf.append(p->getFolder());
	else
		buf.append(L'*');
	return buf.getString();
}

template<class T, class List, class Manager, class EditDialog>
std::auto_ptr<T> qm::RuleColorSetsDialog<T, List, Manager, EditDialog>::create() const
{
	std::auto_ptr<T> p(new T());
	EditDialog dialog(p.get(), pAccountManager_, pProfile_);
	if (dialog.doModal(getParentPopup()) != IDOK)
		return std::auto_ptr<T>();
	return p;
}

template<class T, class List, class Manager, class EditDialog>
T* qm::RuleColorSetsDialog<T, List, Manager, EditDialog>::edit(T* p) const
{
	EditDialog dialog(p, pAccountManager_, pProfile_);
	if (dialog.doModal(getParentPopup()) != IDOK)
		return 0;
	return p;
}

template<class T, class List, class Manager, class EditDialog>
bool qm::RuleColorSetsDialog<T, List, Manager, EditDialog>::save(OptionDialogContext* pContext)
{
	(pManager_->*pfnSet_)(getList());
	return pManager_->save();
}

template<class T, class List, class Manager, class EditDialog>
LRESULT qm::RuleColorSetsDialog<T, List, Manager, EditDialog>::onSize(UINT nFlags,
																	  int cx,
																	  int cy)
{
	layout();
	return AbstractListDialog<T, List>::onSize(nFlags, cx, cy);
}

template<class T, class List, class Manager, class EditDialog>
void qm::RuleColorSetsDialog<T, List, Manager, EditDialog>::layout()
{
#if !defined _WIN32_WCE_PSPC
	LayoutUtil::layout(this, IDC_RULECOLORSETS);
#endif
}


/****************************************************************************
 *
 * RulesColorsDialog
 *
 */

template<class T, class List, class Container, class EditDialog>
qm::RulesColorsDialog<T, List, Container, EditDialog>::RulesColorsDialog(Container* pContainer,
																		 AccountManager* pAccountManager,
																		 qs::Profile* pProfile,
																		 UINT nTitleId,
																		 PFN_GET pfnGet,
																		 PFN_SET pfnSet) :
	AbstractListDialog<T, List>(IDD_RULESCOLORS, IDC_RULESCOLORS, false),
	pContainer_(pContainer),
	pAccountManager_(pAccountManager),
	pProfile_(pProfile),
	nTitleId_(nTitleId),
	pfnSet_(pfnSet)
{
	const List& l = (pContainer->*pfnGet)();
	List& list = getList();
	list.reserve(l.size());
	for (List::const_iterator it = l.begin(); it != l.end(); ++it)
		list.push_back(new T(**it));
}

template<class T, class List, class Container, class EditDialog>
qm::RulesColorsDialog<T, List, Container, EditDialog>::~RulesColorsDialog()
{
}

template<class T, class List, class Container, class EditDialog>
INT_PTR qm::RulesColorsDialog<T, List, Container, EditDialog>::dialogProc(UINT uMsg,
																		  WPARAM wParam,
																		  LPARAM lParam)
{
	BEGIN_MESSAGE_HANDLER()
		HANDLE_SIZE()
	END_MESSAGE_HANDLER()
	return AbstractListDialog<T, List>::dialogProc(uMsg, wParam, lParam);
}

template<class T, class List, class Container, class EditDialog>
LRESULT qm::RulesColorsDialog<T, List, Container, EditDialog>::onCommand(WORD nCode,
																		 WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID_CODE(IDC_ACCOUNT, CBN_EDITCHANGE, onAccountEditChange)
		HANDLE_COMMAND_ID_CODE(IDC_ACCOUNT, CBN_SELCHANGE, onAccountSelChange)
	END_COMMAND_HANDLER()
	return AbstractListDialog<T, List>::onCommand(nCode, nId);
}

template<class T, class List, class Container, class EditDialog>
LRESULT qm::RulesColorsDialog<T, List, Container, EditDialog>::onDestroy()
{
#ifndef _WIN32_WCE
	RECT rect;
	getWindowRect(&rect);
	pProfile_->setInt(getName(), L"Width", rect.right - rect.left);
	pProfile_->setInt(getName(), L"Height", rect.bottom - rect.top);
#endif
	return AbstractListDialog<T, List>::onDestroy();
}

template<class T, class List, class Container, class EditDialog>
LRESULT qm::RulesColorsDialog<T, List, Container, EditDialog>::onInitDialog(HWND hwndFocus,
																			LPARAM lParam)
{
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	wstring_ptr wstrTitle(loadString(hInst, nTitleId_));
	setWindowText(wstrTitle.get());
	
	AccountManager::AccountList listAccount(pAccountManager_->getAccounts());
	std::sort(listAccount.begin(), listAccount.end(),
		binary_compose_f_gx_hy(
			string_less_i<WCHAR>(),
			std::mem_fun(&Account::getName),
			std::mem_fun(&Account::getName)));
	for (AccountManager::AccountList::const_iterator it = listAccount.begin(); it != listAccount.end(); ++it) {
		Account* pAccount = *it;
		W2T(pAccount->getName(), ptszName);
		sendDlgItemMessage(IDC_ACCOUNT, CB_ADDSTRING,
			0, reinterpret_cast<LPARAM>(ptszName));
	}
	const WCHAR* pwszAccount = pContainer_->getAccount();
	if (pwszAccount)
		setDlgItemText(IDC_ACCOUNT, pwszAccount);
	
	const WCHAR* pwszFolder = pContainer_->getFolder();
	if (pwszFolder)
		setDlgItemText(IDC_FOLDER, pwszFolder);
	
#ifdef _WIN32_WCE
	RECT rectWorkArea;
	::SystemParametersInfo(SPI_GETWORKAREA, 0, &rectWorkArea, 0);
	int nWidth = rectWorkArea.right - rectWorkArea.left;
	int nHeight = rectWorkArea.bottom - rectWorkArea.top;
	setWindowPos(0, 0, 0, nWidth, nHeight, SWP_NOZORDER | SWP_NOACTIVATE);
#else
	int nWidth = pProfile_->getInt(getName(), L"Width");
	int nHeight = pProfile_->getInt(getName(), L"Height");
	setWindowPos(0, 0, 0, nWidth, nHeight,
		SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
#endif
	
	return AbstractListDialog<T, List>::onInitDialog(hwndFocus, lParam);
}

template<class T, class List, class Container, class EditDialog>
LRESULT qm::RulesColorsDialog<T, List, Container, EditDialog>::onOk()
{
	wstring_ptr wstrAccount(getDlgItemText(IDC_ACCOUNT));
	Term account;
	if (*wstrAccount.get() && !account.setValue(wstrAccount.get())) {
		// TODO MSG
		return 0;
	}
	
	wstring_ptr wstrFolder(getDlgItemText(IDC_FOLDER));
	Term folder;
	if (*wstrFolder.get() && !folder.setValue(wstrFolder.get())) {
		// TODO MSG
		return 0;
	}
	
	pContainer_->setAccount(account);
	pContainer_->setFolder(folder);
	(pContainer_->*pfnSet_)(getList());
	
	return AbstractListDialog<T, List>::onOk();
}

template<class T, class List, class Container, class EditDialog>
qs::wstring_ptr qm::RulesColorsDialog<T, List, Container, EditDialog>::getLabel(const T* p) const
{
	StringBuffer<WSTRING> buf;
	
	const WCHAR* pwszDescription = p->getDescription();
	if (pwszDescription) {
		buf.append(pwszDescription);
		buf.append(L": ");
	}
	
	buf.append(getLabelPrefix(p).get());
	buf.append(L" <- ");
	
	const Macro* pMacro = p->getCondition();
	std::auto_ptr<ConditionList> pConditionList(ConditionFactory::getInstance().parse(pMacro));
	if (pConditionList.get())
		buf.append(pConditionList->getDescription(true).get());
	else
		buf.append(pMacro->getString().get());
	
	return buf.getString();
}

template<class T, class List, class Container, class EditDialog>
std::auto_ptr<T> qm::RulesColorsDialog<T, List, Container, EditDialog>::create() const
{
	std::auto_ptr<T> p(new T());
	EditDialog dialog(p.get(), pAccountManager_);
	if (dialog.doModal(getHandle()) != IDOK)
		return std::auto_ptr<T>();
	return p;
}

template<class T, class List, class Container, class EditDialog>
T* qm::RulesColorsDialog<T, List, Container, EditDialog>::edit(T* p) const
{
	EditDialog dialog(p, pAccountManager_);
	if (dialog.doModal(getHandle()) != IDOK)
		return 0;
	return p;
}

template<class T, class List, class Container, class EditDialog>
void qm::RulesColorsDialog<T, List, Container, EditDialog>::updateState()
{
	AbstractListDialog<T, List>::updateState();
	
	Account* pAccount = 0;
	wstring_ptr wstrAccount(getDlgItemText(IDC_ACCOUNT));
	if (wstrAccount.get())
		pAccount = pAccountManager_->getAccount(wstrAccount.get());
	updateFolder(pAccount);
}

template<class T, class List, class Container, class EditDialog>
LRESULT qm::RulesColorsDialog<T, List, Container, EditDialog>::onSize(UINT nFlags,
																	  int cx,
																	  int cy)
{
	layout();
	return AbstractListDialog<T, List>::onSize(nFlags, cx, cy);
}

template<class T, class List, class Container, class EditDialog>
LRESULT qm::RulesColorsDialog<T, List, Container, EditDialog>::onAccountEditChange()
{
	updateState();
	return 0;
}

template<class T, class List, class Container, class EditDialog>
LRESULT qm::RulesColorsDialog<T, List, Container, EditDialog>::onAccountSelChange()
{
	postMessage(WM_COMMAND, MAKEWPARAM(IDC_ACCOUNT, CBN_EDITCHANGE));
	return 0;
}

template<class T, class List, class Container, class EditDialog>
void qm::RulesColorsDialog<T, List, Container, EditDialog>::updateFolder(Account* pAccount)
{
	wstring_ptr wstrFolder(getDlgItemText(IDC_FOLDER));
	
	sendDlgItemMessage(IDC_FOLDER, CB_RESETCONTENT);
	
	if (pAccount) {
		Account::FolderList l(pAccount->getFolders());
		std::sort(l.begin(), l.end(), FolderLess());
		for (Account::FolderList::const_iterator it = l.begin(); it != l.end(); ++it) {
			Folder* pFolder = *it;
			
			wstring_ptr wstrName(pFolder->getFullName());
			W2T(wstrName.get(), ptszName);
			sendDlgItemMessage(IDC_FOLDER, CB_ADDSTRING,
				0, reinterpret_cast<LPARAM>(ptszName));
		}
	}
	
	setDlgItemText(IDC_FOLDER, wstrFolder.get());
}

template<class T, class List, class Container, class EditDialog>
void qm::RulesColorsDialog<T, List, Container, EditDialog>::layout()
{
#if !defined _WIN32_WCE_PSPC
	RECT rect;
	getClientRect(&rect);
	
	RECT rectFolder;
	Window(getDlgItem(IDC_FOLDER)).getWindowRect(&rectFolder);
	screenToClient(&rectFolder);
	
#ifndef _WIN32_WCE
	RECT rectButton;
	Window(getDlgItem(IDOK)).getWindowRect(&rectButton);
	int nButtonWidth = rectButton.right - rectButton.left;
	int nButtonHeight = rectButton.bottom - rectButton.top;
#else
	int nButtonHeight = -5;
#endif
	
	HDWP hdwp = beginDeferWindowPos(11);
	
	hdwp = Window(getDlgItem(IDC_ACCOUNT)).deferWindowPos(hdwp, 0, 0, 0,
		rect.right - rectFolder.left - 5, rectFolder.bottom - rectFolder.top,
		SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
	hdwp = Window(getDlgItem(IDC_FOLDER)).deferWindowPos(hdwp, 0, 0, 0,
		rect.right - rectFolder.left - 5, rectFolder.bottom - rectFolder.top,
		SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
	
	hdwp = LayoutUtil::layout(this, IDC_RULESCOLORS, hdwp,
		rectFolder.bottom, nButtonHeight + 5);
	
#ifndef _WIN32_WCE
	hdwp = Window(getDlgItem(IDOK)).deferWindowPos(hdwp, 0,
		rect.right - (nButtonWidth + 5)*2 - 15, rect.bottom - nButtonHeight - 5,
		nButtonWidth, nButtonHeight, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
	hdwp = Window(getDlgItem(IDCANCEL)).deferWindowPos(hdwp, 0,
		rect.right - (nButtonWidth + 5) - 15, rect.bottom - nButtonHeight - 5,
		nButtonWidth, nButtonHeight, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
	hdwp = Window(getDlgItem(IDC_SIZEGRIP)).deferWindowPos(hdwp, 0,
		rect.right - rect.left - 13, rect.bottom - rect.top - 12,
		13, 12, SWP_NOZORDER | SWP_NOACTIVATE);
#endif
	
	endDeferWindowPos(hdwp);
#endif
}

#endif // __OPTIONDIALOG_INL__
