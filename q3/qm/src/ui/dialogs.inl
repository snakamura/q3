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
	
	for (List::const_iterator it = list_.begin(); it != list_.end(); ++it) {
		const T* p = *it;
		wstring_ptr wstrLabel(getLabel(p));
		W2T(wstrLabel.get(), ptszLabel);
		sendDlgItemMessage(nListId_, LB_ADDSTRING,
			0, reinterpret_cast<LPARAM>(ptszLabel));
	}
	sendDlgItemMessage(nListId_, LB_SETCURSEL, 0);
	
	updateState();
	
	return bFocus_ ? TRUE : FALSE;
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

#endif // __DIALOGS_INL__
