/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __DIALOGS_INL__
#define __DIALOGS_INL__


/****************************************************************************
 *
 * AbstractListDialog
 *
 */

template<class T, class List>
qm::AbstractListDialog<T, List>::AbstractListDialog(UINT nId,
													UINT nListId,
													bool bFocus) :
	DefaultDialog(nId),
	nListId_(nListId),
	bFocus_(bFocus)
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
	refresh();
	updateState();
	return bFocus_ ? TRUE : FALSE;
}

template<class T, class List>
List& qm::AbstractListDialog<T, List>::getList()
{
	return list_;
}

template<class T, class List>
void qm::AbstractListDialog<T, List>::refresh()
{
	sendDlgItemMessage(nListId_, LB_RESETCONTENT);
	for (List::const_iterator it = list_.begin(); it != list_.end(); ++it) {
		const T* p = *it;
		wstring_ptr wstrLabel(getLabel(p));
		W2T(wstrLabel.get(), ptszLabel);
		ListBox_AddString(getDlgItem(nListId_), ptszLabel);
	}
	ListBox_SetCurSel(getDlgItem(nListId_), 0);
}

template<class T, class List>
void qm::AbstractListDialog<T, List>::updateState()
{
	int n = ListBox_GetCurSel(getDlgItem(nListId_));
	Window(getDlgItem(IDC_REMOVE)).enableWindow(n != LB_ERR);
	Window(getDlgItem(IDC_EDIT)).enableWindow(n != LB_ERR);
	Window(getDlgItem(IDC_UP)).enableWindow(n != LB_ERR && n != 0);
	Window(getDlgItem(IDC_DOWN)).enableWindow(n != LB_ERR &&
		n != ListBox_GetCount(getDlgItem(nListId_)) - 1);
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
		int nItem = ListBox_AddString(getDlgItem(nListId_), ptszLabel);
		if (nItem != LB_ERR)
			ListBox_SetCurSel(getDlgItem(nListId_), nItem);
	}
	
	updateState();
	
	return 0;
}

template<class T, class List>
LRESULT qm::AbstractListDialog<T, List>::onRemove()
{
	int n = ListBox_GetCurSel(getDlgItem(nListId_));
	if (n == LB_ERR)
		return 0;
	
	delete list_[n];
	list_.erase(list_.begin() + n);
	
	int nCount = ListBox_DeleteString(getDlgItem(nListId_), n);
	if (nCount != LB_ERR && nCount != 0) {
		if (n < nCount)
			ListBox_SetCurSel(getDlgItem(nListId_), n);
		else
			ListBox_SetCurSel(getDlgItem(nListId_), nCount - 1);
	}
	
	updateState();
	
	return 0;
}

template<class T, class List>
LRESULT qm::AbstractListDialog<T, List>::onEdit()
{
	int n = ListBox_GetCurSel(getDlgItem(nListId_));
	if (n == LB_ERR)
		return 0;
	
	T* p = list_[n];
	T* pNew = edit(p);
	if (pNew) {
		if (pNew != p) {
			delete p;
			list_[n] = pNew;
		}
		
		ListBox_DeleteString(getDlgItem(nListId_), n);
		wstring_ptr wstrLabel(getLabel(pNew));
		W2T(wstrLabel.get(), ptszLabel);
		ListBox_InsertString(getDlgItem(nListId_), n, ptszLabel);
		ListBox_SetCurSel(getDlgItem(nListId_), n);
	}
	
	updateState();
	
	return 0;
}

template<class T, class List>
LRESULT qm::AbstractListDialog<T, List>::onUp()
{
	int n = ListBox_GetCurSel(getDlgItem(nListId_));
	if (n == LB_ERR || n == 0)
		return 0;
	
	T* p = list_[n];
	std::swap(list_[n], list_[n - 1]);
	
	ListBox_DeleteString(getDlgItem(nListId_), n);
	wstring_ptr wstrLabel(getLabel(p));
	W2T(wstrLabel.get(), ptszLabel);
	ListBox_InsertString(getDlgItem(nListId_), n - 1, ptszLabel);
	ListBox_SetCurSel(getDlgItem(nListId_), n - 1);
	
	updateState();
	
	return 0;
}

template<class T, class List>
LRESULT qm::AbstractListDialog<T, List>::onDown()
{
	int n = ListBox_GetCurSel(getDlgItem(nListId_));
	if (n == LB_ERR || n == ListBox_GetCount(getDlgItem(nListId_)) - 1)
		return 0;
	
	T* p = list_[n];
	std::swap(list_[n], list_[n + 1]);
	
	ListBox_DeleteString(getDlgItem(nListId_), n);
	wstring_ptr wstrLabel(getLabel(p));
	W2T(wstrLabel.get(), ptszLabel);
	ListBox_InsertString(getDlgItem(nListId_), n + 1, ptszLabel);
	ListBox_SetCurSel(getDlgItem(nListId_), n + 1);
	
	updateState();
	
	return 0;
}

template<class T, class List>
LRESULT qm::AbstractListDialog<T, List>::onSelChange()
{
	updateState();
	return 0;
}

#endif // __DIALOGS_INL__
