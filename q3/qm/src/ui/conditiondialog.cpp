/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#pragma warning(disable:4786)

#include <qswindow.h>

#include <tchar.h>

#include "conditiondialog.h"
#include "../main/main.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * ConditionsDialog
 *
 */

qm::ConditionsDialog::ConditionsDialog(const WCHAR* pwszCondition) :
	AbstractListDialog<Condition, ConditionList::List>(
		IDD_CONDITIONS, LANDSCAPE(IDD_CONDITIONS), IDC_CONDITIONS, true)
{
	assert(pwszCondition);
	wstrCondition_ = allocWString(pwszCondition);
}

qm::ConditionsDialog::~ConditionsDialog()
{
}

const WCHAR* qm::ConditionsDialog::getCondition() const
{
	return wstrCondition_.get();
}

LRESULT qm::ConditionsDialog::onCommand(WORD nCode,
										WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_AND, onType)
		HANDLE_COMMAND_ID(IDC_CUSTOM, onCustom)
		HANDLE_COMMAND_ID_CODE(IDC_MACRO, EN_CHANGE, onMacroChange)
		HANDLE_COMMAND_ID(IDC_OR, onType)
	END_COMMAND_HANDLER()
	return AbstractListDialog<Condition, ConditionList::List>::onCommand(nCode, nId);
}

LRESULT qm::ConditionsDialog::onInitDialog(HWND hwndFocus,
										   LPARAM lParam)
{
	init(false);
	
	std::auto_ptr<ConditionList> pConditionList;
	if (*wstrCondition_.get()) {
		std::auto_ptr<Macro> pMacro(MacroParser().parse(wstrCondition_.get()));
		if (pMacro.get())
			pConditionList = ConditionFactory::getInstance().parse(pMacro.get());
	}
	setConditionList(pConditionList.get());
	
	bool bCustom = *wstrCondition_.get() && !pConditionList.get();
	Button_SetCheck(getDlgItem(IDC_CUSTOM), bCustom ? BST_CHECKED : BST_UNCHECKED);
	
	UINT nId = IDC_AND;
	if (pConditionList.get())
		nId = pConditionList->getType() == ConditionList::TYPE_AND ? IDC_AND : IDC_OR;
	Button_SetCheck(getDlgItem(nId), BST_CHECKED);
	
	setDlgItemText(IDC_MACRO, wstrCondition_.get());
	
	return AbstractListDialog<Condition, ConditionList::List>::onInitDialog(hwndFocus, lParam);
}

LRESULT qm::ConditionsDialog::onOk()
{
	bool bCustom = Button_GetCheck(getDlgItem(IDC_CUSTOM)) == BST_CHECKED;
	if (bCustom) {
		wstring_ptr wstrCondition = getDlgItemText(IDC_MACRO);
		std::auto_ptr<Macro> pMacro(MacroParser().parse(wstrCondition.get()));
		if (!pMacro.get()) {
			messageBox(getResourceHandle(), IDS_ERROR_INVALIDMACRO,
				MB_OK | MB_ICONERROR, getHandle());
			return 0;
		}
		wstrCondition_ = wstrCondition;
	}
	else {
		wstrCondition_ = getMacroFromConditions();
	}
	return AbstractListDialog<Condition, ConditionList::List>::onOk();
}

wstring_ptr qm::ConditionsDialog::getLabel(const Condition* p) const
{
	return p->getDescription(true);
}

std::auto_ptr<Condition> qm::ConditionsDialog::create() const
{
	ConditionDialog dialog;
	if (dialog.doModal(getHandle()) != IDOK)
		return std::auto_ptr<Condition>();
	return dialog.getCondition();
}

Condition* qm::ConditionsDialog::edit(Condition* p) const
{
	ConditionDialog dialog(p);
	if (dialog.doModal(getHandle()) != IDOK)
		return 0;
	return dialog.getCondition().release();
}

void qm::ConditionsDialog::updateState()
{
	bool bCustom = Button_GetCheck(getDlgItem(IDC_CUSTOM)) == BST_CHECKED;
	
	UINT nIds[] = {
		IDC_CONDITIONS,
		IDC_ADD,
		IDC_REMOVE,
		IDC_EDIT,
		IDC_UP,
		IDC_DOWN,
		IDC_AND,
		IDC_OR
	};
	for (int n = 0; n < countof(nIds); ++n)
		Window(getDlgItem(nIds[n])).enableWindow(!bCustom);
	
	if (!bCustom)
		AbstractListDialog<Condition, ConditionList::List>::updateState();
	
	sendDlgItemMessage(IDC_MACRO, EM_SETREADONLY, !bCustom);
	
	bool bEnable = false;
	if (!bCustom) {
		setDlgItemText(IDC_MACRO, getMacroFromConditions().get());
		bEnable = ListBox_GetCount(getDlgItem(IDC_CONDITIONS)) != 0;
	}
	else {
		bEnable = Window(getDlgItem(IDC_MACRO)).getWindowTextLength() != 0;
	}
	Window(getDlgItem(IDOK)).enableWindow(bEnable);
}

LRESULT qm::ConditionsDialog::onCustom()
{
	bool bCustom = Button_GetCheck(getDlgItem(IDC_CUSTOM)) == BST_CHECKED;
	if (!bCustom) {
		std::auto_ptr<ConditionList> pConditionList;
		wstring_ptr wstrMacro(getDlgItemText(IDC_MACRO));
		if (*wstrMacro.get()) {
			std::auto_ptr<Macro> pMacro(MacroParser().parse(wstrMacro.get()));
			if (!pMacro.get()) {
				messageBox(getResourceHandle(), IDS_ERROR_INVALIDMACRO,
					MB_OK | MB_ICONERROR, getHandle());
			}
			else {
				pConditionList = ConditionFactory::getInstance().parse(pMacro.get());
				if (!pConditionList.get())
					messageBox(getResourceHandle(), IDS_ERROR_COMPLEXMACRO,
						MB_OK | MB_ICONERROR, getHandle());
			}
		}
		else {
			pConditionList.reset(new ConditionList());
		}
		if (pConditionList.get()) {
			setConditionList(pConditionList.get());
			refresh();
		}
		else {
			Button_SetCheck(getDlgItem(IDC_CUSTOM), BST_CHECKED);
			Window(getDlgItem(IDC_MACRO)).setFocus();
		}
	}
	else {
		setDlgItemText(IDC_MACRO, getMacroFromConditions().get());
		setConditionList(0);
		refresh();
	}
	
	updateState();
	
	return 0;
}

LRESULT qm::ConditionsDialog::onMacroChange()
{
	bool bCustom = Button_GetCheck(getDlgItem(IDC_CUSTOM)) == BST_CHECKED;
	if (bCustom)
		updateState();
	
	return 0;
}

LRESULT qm::ConditionsDialog::onType()
{
	updateState();
	return 0;
}

void qm::ConditionsDialog::setConditionList(const ConditionList* pConditionList)
{
	ConditionList::List& list = getList();
	std::for_each(list.begin(), list.end(), qs::deleter<Condition>());
	list.clear();
	
	if (pConditionList) {
		const ConditionList::List& l = pConditionList->getConditions();
		list.resize(l.size());
		for (ConditionList::List::size_type n = 0; n < l.size(); ++n)
			list[n] = l[n]->clone().release();
	}
}

wstring_ptr qm::ConditionsDialog::getMacroFromConditions()
{
	ConditionList::Type type = Button_GetCheck(getDlgItem(IDC_AND)) == BST_CHECKED ?
		ConditionList::TYPE_AND : ConditionList::TYPE_OR;
	std::auto_ptr<ConditionList> pConditionList(new ConditionList(getList(), type));
	return pConditionList->getMacro();
}


/****************************************************************************
 *
 * ConditionDialog
 *
 */

qm::ConditionDialog::ConditionDialog() :
	DefaultDialog(IDD_CONDITION, LANDSCAPE(IDD_CONDITION))
{
}

qm::ConditionDialog::ConditionDialog(const Condition* pCondition) :
	DefaultDialog(IDD_CONDITION, LANDSCAPE(IDD_CONDITION)),
	pCondition_(pCondition->clone())
{
}

qm::ConditionDialog::~ConditionDialog()
{
}

std::auto_ptr<Condition> qm::ConditionDialog::getCondition() const
{
	return pCondition_->clone();
}

LRESULT qm::ConditionDialog::onCommand(WORD nCode,
									   WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID_CODE(IDC_CONDITION, LBN_SELCHANGE, onConditionSelChange)
		HANDLE_COMMAND_ID(IDC_EDIT, onEdit)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::ConditionDialog::onDestroy()
{
	HWND hwndCondition = getDlgItem(IDC_CONDITION);
	int nCount = ComboBox_GetCount(hwndCondition);
	for (int n = 0; n < nCount; ++n) {
		Condition* pCondition = reinterpret_cast<Condition*>(
			ComboBox_GetItemData(hwndCondition, n));
		delete pCondition;
	}
	
	removeNotifyHandler(this);
	
	return DefaultDialog::onDestroy();
}

LRESULT qm::ConditionDialog::onInitDialog(HWND hwndFocus,
										  LPARAM lParam)
{
	init(false);
	
	HWND hwndCondition = getDlgItem(IDC_CONDITION);
	int nSelected = 0;
	const ConditionFactory::List& l = ConditionFactory::getInstance().getConditions();
	for (ConditionFactory::List::const_iterator it = l.begin(); it != l.end(); ++it) {
		const Condition* p = *it;
		
		std::auto_ptr<Condition> pCondition;
		bool bSelected = pCondition_.get() && wcscmp(p->getName(), pCondition_->getName()) == 0;
		pCondition = bSelected ? pCondition_ : p->clone();
		wstring_ptr wstrDescription(pCondition->getDescription(false));
		W2T(wstrDescription.get(), ptszDescription);
		int nItem = ComboBox_AddString(hwndCondition, ptszDescription);
		ComboBox_SetItemData(hwndCondition, nItem, pCondition.release());
		if (bSelected)
			nSelected = nItem;
	}
	ComboBox_SetCurSel(hwndCondition, nSelected);
	
	HWND hwndArguments = getDlgItem(IDC_ARGUMENTS);
	ListView_SetExtendedListViewStyle(hwndArguments, LVS_EX_FULLROWSELECT);
	
	struct {
		UINT nId_;
		int nWidth_;
	} columns[] = {
#ifndef _WIN32_WCE_PSPC
		{ IDS_ARGUMENT_NAME,	150	},
		{ IDS_ARGUMENT_VALUE,	150	}
#else
		{ IDS_ARGUMENT_NAME,	80	},
		{ IDS_ARGUMENT_VALUE,	80	}
#endif
	};
	for (int n = 0; n < countof(columns); ++n) {
		wstring_ptr wstrColumn(loadString(getResourceHandle(), columns[n].nId_));
		W2T(wstrColumn.get(), ptszColumn);
		
		LVCOLUMN column = {
			LVCF_FMT | LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM,
			LVCFMT_LEFT,
			columns[n].nWidth_,
			const_cast<LPTSTR>(ptszColumn),
			0,
			n,
		};
		ListView_InsertColumn(hwndArguments, n, &column);
	}
	
	updateArguments();
	addNotifyHandler(this);
	
	return TRUE;
}

LRESULT qm::ConditionDialog::onOk()
{
	Condition* pCondition = getCurrentCondition();
	if (!pCondition)
		return 0;
	pCondition_ = pCondition->clone();
	
	return DefaultDialog::onOk();
}

LRESULT qm::ConditionDialog::onNotify(NMHDR* pnmhdr,
									  bool* pbHandled)
{
	BEGIN_NOTIFY_HANDLER()
		HANDLE_NOTIFY(NM_DBLCLK, IDC_ARGUMENTS, onArgumentsDblClk)
		HANDLE_NOTIFY(LVN_ITEMCHANGED, IDC_ARGUMENTS, onArgumentsItemChanged)
	END_NOTIFY_HANDLER()
	return NotifyHandler::onNotify(pnmhdr, pbHandled);
}

LRESULT qm::ConditionDialog::onConditionSelChange()
{
	updateArguments();
	return 0;
}

LRESULT qm::ConditionDialog::onEdit()
{
	edit();
	return 0;
}

LRESULT qm::ConditionDialog::onArgumentsDblClk(NMHDR* pnmhdr,
											   bool* pbHandled)
{
	edit();
	*pbHandled = true;
	return 0;
}

LRESULT qm::ConditionDialog::onArgumentsItemChanged(NMHDR* pnmhdr,
													bool* pbHandled)
{
	updateState();
	*pbHandled = true;
	return 0;
}

void qm::ConditionDialog::edit()
{
	HWND hwnd = getDlgItem(IDC_ARGUMENTS);
	
	int nItem = ListView_GetNextItem(hwnd, -1, LVNI_SELECTED);
	if (nItem == -1)
		return;
	
	LVITEM item = {
		LVIF_PARAM,
		nItem
	};
	ListView_GetItem(hwnd, &item);
	Condition* pCondition = reinterpret_cast<Condition*>(item.lParam);
	
	wstring_ptr wstrName(pCondition->getArgumentName(nItem));
	wstring_ptr wstrValue(pCondition->getArgumentValue(nItem));
	ConditionArgumentDialog dialog(wstrName.get(),
		pCondition->getArgumentType(nItem), wstrValue.get());
	if (dialog.doModal(getHandle()) != IDOK)
		return;
	
	const WCHAR* pwszValue = dialog.getValue();
	pCondition->setArgumentValue(nItem, pwszValue);
	W2T(pwszValue, ptszValue);
	ListView_SetItemText(hwnd, nItem, 1, const_cast<LPTSTR>(ptszValue));
	
	updateState();
}

Condition* qm::ConditionDialog::getCurrentCondition() const
{
	HWND hwnd = getDlgItem(IDC_CONDITION);
	int nItem = ComboBox_GetCurSel(hwnd);
	if (nItem == CB_ERR)
		return 0;
	return reinterpret_cast<Condition*>(ComboBox_GetItemData(hwnd, nItem));
}

void qm::ConditionDialog::updateArguments()
{
	Condition* pCondition = getCurrentCondition();
	if (!pCondition)
		return;
	
	HWND hwnd = getDlgItem(IDC_ARGUMENTS);
	
	ListView_DeleteAllItems(hwnd);
	
	size_t nCount = pCondition->getArgumentCount();
	for (size_t n = 0; n < nCount; ++n) {
		wstring_ptr wstrName(pCondition->getArgumentName(n));
		W2T(wstrName.get(), ptszName);
		LVITEM item = {
			LVIF_TEXT | LVIF_PARAM,
			static_cast<int>(n),
			0,
			0,
			0,
			const_cast<LPTSTR>(ptszName),
			0,
			0,
			reinterpret_cast<LPARAM>(pCondition)
		};
		ListView_InsertItem(hwnd, &item);
		
		wstring_ptr wstrValue(pCondition->getArgumentValue(n));
		if (wstrValue.get()) {
			W2T(wstrValue.get(), ptszValue);
			ListView_SetItemText(hwnd, n, 1, const_cast<LPTSTR>(ptszValue));
		}
	}
	
	updateState();
}

void qm::ConditionDialog::updateState()
{
	bool bEnable = true;
	Condition* pCondition = getCurrentCondition();
	if (pCondition) {
		size_t nCount = pCondition->getArgumentCount();
		for (size_t n = 0; n < nCount && bEnable; ++n) {
			wstring_ptr wstrValue(pCondition->getArgumentValue(n));
			if (!wstrValue.get())
				bEnable = false;
		}
	}
	else {
		bEnable = false;
	}
	Window(getDlgItem(IDOK)).enableWindow(bEnable);
	
	int nItem = ListView_GetNextItem(getDlgItem(IDC_ARGUMENTS), -1, LVNI_SELECTED);
	Window(getDlgItem(IDC_EDIT)).enableWindow(nItem != -1);
}


/****************************************************************************
 *
 * ConditionArgumentDialog
 *
 */

qm::ConditionArgumentDialog::ConditionArgumentDialog(const WCHAR* pwszName,
													 Condition::Type type,
													 const WCHAR* pwszValue) :
	DefaultDialog(IDD_CONDITIONARGUMENT, LANDSCAPE(IDD_CONDITIONARGUMENT)),
	type_(type)
{
	assert(pwszName);
	
	wstrName_ = allocWString(pwszName);
	if (pwszValue)
		wstrValue_ = allocWString(pwszValue);
}

qm::ConditionArgumentDialog::~ConditionArgumentDialog()
{
}

const WCHAR* qm::ConditionArgumentDialog::getValue() const
{
	return wstrValue_.get();
}

LRESULT qm::ConditionArgumentDialog::onCommand(WORD nCode,
											   WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID_CODE(IDC_FIELD, CBN_EDITCHANGE, onChange)
		HANDLE_COMMAND_ID_CODE(IDC_FIELD, CBN_SELCHANGE, onFieldSelChange)
		HANDLE_COMMAND_ID_CODE(IDC_NUMBER, EN_CHANGE, onChange)
		HANDLE_COMMAND_ID_CODE(IDC_TEXT, EN_CHANGE, onChange)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::ConditionArgumentDialog::onInitDialog(HWND hwndFocus,
												  LPARAM lParam)
{
	init(false);
	
	setDlgItemText(IDC_NAME, wstrName_.get());
	
	if (type_ == Condition::TYPE_FIELD) {
		const TCHAR* ptszFields[] = {
			_T("%From"),
			_T("%To"),
			_T("%FromTo"),
			_T("%Subject"),
			_T("From"),
			_T("To"),
			_T("Cc"),
			_T("Sender"),
			_T("Subject"),
			_T("X-ML-Name"),
			_T("List-Id"),
			_T("Mailing-List")
		};
		for (int n = 0; n < countof(ptszFields); ++n)
			ComboBox_AddString(getDlgItem(IDC_FIELD), ptszFields[n]);
	}
	
	UINT nId = getId();
	Window(getDlgItem(nId)).showWindow();
	if (wstrValue_.get())
		setDlgItemText(nId, wstrValue_.get());
	
	updateState();
	
	return TRUE;
}

LRESULT qm::ConditionArgumentDialog::onOk()
{
	wstrValue_ = getDlgItemText(getId());
	return DefaultDialog::onOk();
}

LRESULT qm::ConditionArgumentDialog::onChange()
{
	updateState();
	return 0;
}

LRESULT qm::ConditionArgumentDialog::onFieldSelChange()
{
	postMessage(WM_COMMAND, MAKEWPARAM(IDC_FIELD, CBN_EDITCHANGE));
	return 0;
}

UINT qm::ConditionArgumentDialog::getId() const
{
	struct {
		Condition::Type type_;
		UINT nId_;
	} types[] = {
		{ Condition::TYPE_FIELD,	IDC_FIELD	},
		{ Condition::TYPE_TEXT,		IDC_TEXT	},
		{ Condition::TYPE_NUMBER,	IDC_NUMBER	}
	};
	for (int n = 0; n < countof(types); ++n) {
		if (types[n].type_ == type_)
			return types[n].nId_;
	}
	assert(false);
	return 0;
}

void qm::ConditionArgumentDialog::updateState()
{
	Window(getDlgItem(IDOK)).enableWindow(sendDlgItemMessage(getId(), WM_GETTEXTLENGTH) != 0);
}
