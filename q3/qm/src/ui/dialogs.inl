/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __DIALOGS_INL__
#define __DIALOGS_INL__

#include <qmapplication.h>
#include <qmdocument.h>


/****************************************************************************
 *
 * AbstractListDialog
 *
 */

template<class T, class List>
qm::AbstractListDialog<T, List>::AbstractListDialog(UINT nId,
													UINT nListId) :
	DefaultDialog(nId),
	nListId_(nListId)
{
}

template<class T, class List>
qm::AbstractListDialog<T, List>::~AbstractListDialog()
{
	std::for_each(list_.begin(), list_.end(), qs::deleter<T>());
}

template<class T, class List>
LRESULT qm::AbstractListDialog<T, List>::onCommand(WORD nCode,
												   WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_ADD, onAdd)
		HANDLE_COMMAND_ID(IDC_DOWN, onDown)
		HANDLE_COMMAND_ID(IDC_EDIT, onEdit)
		HANDLE_COMMAND_ID(IDC_REMOVE, onRemove)
		HANDLE_COMMAND_ID(IDC_UP, onUp)
		HANDLE_COMMAND_ID_CODE(nListId_, LBN_SELCHANGE, onSelChange)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

template<class T, class List>
LRESULT qm::AbstractListDialog<T, List>::onInitDialog(HWND hwndFocus,
													  LPARAM lParam)
{
	init(false);
	
	for (List::const_iterator it = list_.begin(); it != list_.end(); ++it) {
		const T* p = *it;
		wstring_ptr wstrLabel(getLabel(p));
		W2T(wstrLabel.get(), ptszLabel);
		sendDlgItemMessage(nListId_, LB_ADDSTRING,
			0, reinterpret_cast<LPARAM>(ptszLabel));
	}
	sendDlgItemMessage(nListId_, LB_SETCURSEL, 0);
	
	updateState();
	
	return TRUE;
}

template<class T, class List>
List& qm::AbstractListDialog<T, List>::getList()
{
	return list_;
}

template<class T, class List>
void qm::AbstractListDialog<T, List>::updateState()
{
	int n = sendDlgItemMessage(nListId_, LB_GETCURSEL);
	Window(getDlgItem(IDC_REMOVE)).enableWindow(n != LB_ERR);
	Window(getDlgItem(IDC_EDIT)).enableWindow(n != LB_ERR);
	Window(getDlgItem(IDC_UP)).enableWindow(n != LB_ERR && n != 0);
	Window(getDlgItem(IDC_DOWN)).enableWindow(n != LB_ERR &&
		n != sendDlgItemMessage(nListId_, LB_GETCOUNT) - 1);
}

template<class T, class List>
LRESULT qm::AbstractListDialog<T, List>::onAdd()
{
	std::auto_ptr<T> pNew(create());
	if (pNew.get()) {
		list_.push_back(pNew.get());
		T* p = pNew.release();
		
		wstring_ptr wstrLabel(getLabel(p));
		W2T(wstrLabel.get(), ptszLabel);
		int nItem = sendDlgItemMessage(nListId_, LB_ADDSTRING,
			0, reinterpret_cast<LPARAM>(ptszLabel));
		if (nItem != LB_ERR)
			sendDlgItemMessage(nListId_, LB_SETCURSEL, nItem);
	}
	
	updateState();
	
	return 0;
}

template<class T, class List>
LRESULT qm::AbstractListDialog<T, List>::onRemove()
{
	int n = sendDlgItemMessage(nListId_, LB_GETCURSEL);
	if (n == LB_ERR)
		return 0;
	
	delete list_[n];
	list_.erase(list_.begin() + n);
	
	int nCount = sendDlgItemMessage(nListId_, LB_DELETESTRING, n);
	if (nCount != LB_ERR && nCount != 0) {
		if (n < nCount)
			sendDlgItemMessage(nListId_, LB_SETCURSEL, n);
		else
			sendDlgItemMessage(nListId_, LB_SETCURSEL, nCount - 1);
	}
	
	updateState();
	
	return 0;
}

template<class T, class List>
LRESULT qm::AbstractListDialog<T, List>::onEdit()
{
	int n = sendDlgItemMessage(nListId_, LB_GETCURSEL);
	if (n == LB_ERR)
		return 0;
	
	T* p = list_[n];
	if (edit(p)) {
		sendDlgItemMessage(nListId_, LB_DELETESTRING, n);
		wstring_ptr wstrLabel(getLabel(p));
		W2T(wstrLabel.get(), ptszLabel);
		sendDlgItemMessage(nListId_, LB_INSERTSTRING,
			n, reinterpret_cast<LPARAM>(ptszLabel));
		sendDlgItemMessage(nListId_, LB_SETCURSEL, n);
	}
	
	updateState();
	
	return 0;
}

template<class T, class List>
LRESULT qm::AbstractListDialog<T, List>::onUp()
{
	int n = sendDlgItemMessage(nListId_, LB_GETCURSEL);
	if (n == LB_ERR || n == 0)
		return 0;
	
	T* p = list_[n];
	std::swap(list_[n], list_[n - 1]);
	
	sendDlgItemMessage(nListId_, LB_DELETESTRING, n);
	wstring_ptr wstrLabel(getLabel(p));
	W2T(wstrLabel.get(), ptszLabel);
	sendDlgItemMessage(nListId_, LB_INSERTSTRING,
		n - 1, reinterpret_cast<LPARAM>(ptszLabel));
	sendDlgItemMessage(nListId_, LB_SETCURSEL, n - 1);
	
	updateState();
	
	return 0;
}

template<class T, class List>
LRESULT qm::AbstractListDialog<T, List>::onDown()
{
	int n = sendDlgItemMessage(nListId_, LB_GETCURSEL);
	if (n == LB_ERR || n == sendDlgItemMessage(nListId_, LB_GETCOUNT) - 1)
		return 0;
	
	T* p = list_[n];
	std::swap(list_[n], list_[n + 1]);
	
	sendDlgItemMessage(nListId_, LB_DELETESTRING, n);
	wstring_ptr wstrLabel(getLabel(p));
	W2T(wstrLabel.get(), ptszLabel);
	sendDlgItemMessage(nListId_, LB_INSERTSTRING,
		n + 1, reinterpret_cast<LPARAM>(ptszLabel));
	sendDlgItemMessage(nListId_, LB_SETCURSEL, n + 1);
	
	updateState();
	
	return 0;
}

template<class T, class List>
LRESULT qm::AbstractListDialog<T, List>::onSelChange()
{
	updateState();
	return 0;
}


/****************************************************************************
 *
 * RulesColorsDialog
 *
 */

template<class T, class List, class Container, class EditDialog>
qm::RulesColorsDialog<T, List, Container, EditDialog>::RulesColorsDialog(Container* pContainer,
																		 Document* pDocument,
																		 UINT nTitleId,
																		 PFN_GET pfnGet,
																		 PFN_SET pfnSet) :
	AbstractListDialog<T, List>(IDD_RULESCOLORS, IDC_RULESCOLORS),
	pContainer_(pContainer),
	pDocument_(pDocument),
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
LRESULT qm::RulesColorsDialog<T, List, Container, EditDialog>::onInitDialog(HWND hwndFocus,
																			LPARAM lParam)
{
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	wstring_ptr wstrTitle(loadString(hInst, nTitleId_));
	setWindowText(wstrTitle.get());
	
	const Document::AccountList& listAccount = pDocument_->getAccounts();
	for (Document::AccountList::const_iterator it = listAccount.begin(); it != listAccount.end(); ++it) {
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
	
	return AbstractListDialog<T, List>::onInitDialog(hwndFocus, lParam);
}

template<class T, class List, class Container, class EditDialog>
LRESULT qm::RulesColorsDialog<T, List, Container, EditDialog>::onOk()
{
	RegexCompiler compiler;
	
	wstring_ptr wstrAccount(getDlgItemText(IDC_ACCOUNT));
	std::auto_ptr<RegexPattern> pAccount;
	if (*wstrAccount.get()) {
		pAccount = compiler.compile(wstrAccount.get());
		if (!pAccount.get()) {
			// TODO MSG
			return 0;
		}
	}
	else {
		wstrAccount.reset(0);
	}
	pContainer_->setAccount(wstrAccount.get(), pAccount);
	
	wstring_ptr wstrFolder(getDlgItemText(IDC_FOLDER));
	std::auto_ptr<RegexPattern> pFolder;
	if (*wstrFolder.get()) {
		pFolder = compiler.compile(wstrFolder.get());
		if (!pFolder.get()) {
			// TODO MSG
			return 0;
		}
	}
	else {
		wstrFolder.reset(0);
	}
	pContainer_->setFolder(wstrFolder.get(), pFolder);
	
	(pContainer_->*pfnSet_)(getList());
	
	return AbstractListDialog<T, List>::onOk();
}

template<class T, class List, class Container, class EditDialog>
qs::wstring_ptr qm::RulesColorsDialog<T, List, Container, EditDialog>::getLabel(const T* p) const
{
	return p->getCondition()->getString();
}

template<class T, class List, class Container, class EditDialog>
std::auto_ptr<T> qm::RulesColorsDialog<T, List, Container, EditDialog>::create() const
{
	std::auto_ptr<T> p(new T());
	EditDialog dialog(p.get(), pDocument_);
	if (dialog.doModal(getHandle()) != IDOK)
		return std::auto_ptr<T>();
	return p;
}

template<class T, class List, class Container, class EditDialog>
bool qm::RulesColorsDialog<T, List, Container, EditDialog>::edit(T* p) const
{
	EditDialog dialog(p, pDocument_);
	return dialog.doModal(getHandle()) == IDOK;
}

template<class T, class List, class Container, class EditDialog>
void qm::RulesColorsDialog<T, List, Container, EditDialog>::updateState()
{
	AbstractListDialog<T, List>::updateState();
	
	Account* pAccount = 0;
	wstring_ptr wstrAccount(getDlgItemText(IDC_ACCOUNT));
	if (wstrAccount.get())
		pAccount = pDocument_->getAccount(wstrAccount.get());
	updateFolder(pAccount);
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


/****************************************************************************
 *
 * RuleColorSetsDialog
 *
 */

template<class T, class List, class Manager, class EditDialog>
qm::RuleColorSetsDialog<T, List, Manager, EditDialog>::RuleColorSetsDialog(Manager* pManager,
																		   Document* pDocument,
																		   UINT nTitleId,
																		   PFN_GET pfnGet,
																		   PFN_SET pfnSet) :
	AbstractListDialog<T, List>(IDD_RULECOLORSETS, IDC_RULECOLORSETS),
	pManager_(pManager),
	pDocument_(pDocument),
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
LRESULT qm::RuleColorSetsDialog<T, List, Manager, EditDialog>::onInitDialog(HWND hwndFocus,
																			LPARAM lParam)
{
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	wstring_ptr wstrTitle(loadString(hInst, nTitleId_));
	setWindowText(wstrTitle.get());
	
	return AbstractListDialog<T, List>::onInitDialog(hwndFocus, lParam);
}

template<class T, class List, class Manager, class EditDialog>
LRESULT qm::RuleColorSetsDialog<T, List, Manager, EditDialog>::onOk()
{
	(pManager_->*pfnSet_)(getList());
	if (!pManager_->save()) {
		// TODO
	}
	
	return AbstractListDialog<T, List>::onOk();
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
	EditDialog dialog(p.get(), pDocument_);
	if (dialog.doModal(getHandle()) != IDOK)
		return std::auto_ptr<T>();
	return p;
}

template<class T, class List, class Manager, class EditDialog>
bool qm::RuleColorSetsDialog<T, List, Manager, EditDialog>::edit(T* p) const
{
	EditDialog dialog(p, pDocument_);
	return dialog.doModal(getHandle()) == IDOK;
}

#endif // __DIALOGS_INL__
