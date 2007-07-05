/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
 * All rights reserved.
 *
 */

#pragma warning(disable:4786)

#include <qmapplication.h>
#include <qmsearch.h>

#include "folderdialog.h"
#include "resourceinc.h"
#include "uiutil.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * CreateFolderDialog
 *
 */

qm::CreateFolderDialog::CreateFolderDialog(Type type,
										   unsigned int nFlags) :
	DefaultDialog(IDD_CREATEFOLDER, LANDSCAPE(IDD_CREATEFOLDER)),
	type_(type),
	nFlags_(nFlags)
{
}

qm::CreateFolderDialog::~CreateFolderDialog()
{
}

CreateFolderDialog::Type qm::CreateFolderDialog::getType() const
{
	return type_;
}

const WCHAR* qm::CreateFolderDialog::getName() const
{
	return wstrName_.get();
}

LRESULT qm::CreateFolderDialog::onCommand(WORD nCode,
										  WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID_CODE(IDC_NAME, EN_CHANGE, onNameChange)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::CreateFolderDialog::onInitDialog(HWND hwndFocus,
											 LPARAM lParam)
{
	init(false);
	
	Window(getDlgItem(IDC_REMOTEFOLDER)).enableWindow(
		(nFlags_ & FLAG_ALLOWREMOTE) != 0);
	
	UINT nIds[] = {
		IDC_LOCALFOLDER,
		IDC_REMOTEFOLDER,
		IDC_QUERYFOLDER
	};
	sendDlgItemMessage(nIds[type_], BM_SETCHECK, BST_CHECKED);
	
	updateState();
	
	return TRUE;
}

LRESULT qm::CreateFolderDialog::onOk()
{
	UINT nIds[] = {
		IDC_LOCALFOLDER,
		IDC_REMOTEFOLDER,
		IDC_QUERYFOLDER
	};
	for (int n = 0; n < countof(nIds); ++n) {
		if (sendDlgItemMessage(nIds[n], BM_GETCHECK) == BST_CHECKED) {
			type_ = static_cast<Type>(n);
			break;
		}
	}
	
	wstrName_ = getDlgItemText(IDC_NAME);
	
	return DefaultDialog::onOk();
}

LRESULT qm::CreateFolderDialog::onNameChange()
{
	updateState();
	return 0;
}

void qm::CreateFolderDialog::updateState()
{
	Window(getDlgItem(IDOK)).enableWindow(
		Window(getDlgItem(IDC_NAME)).getWindowTextLength() != 0);
}


/****************************************************************************
 *
 * FolderPropertyPage
 *
 */

namespace {
struct
{
	Folder::Flag flag_;
	UINT nId_;
	Folder::Flag enableFlag_;
	bool bEnableQuery_;
	bool bReverse_;
} folderFlags[] = {
	{ Folder::FLAG_ACTIVESYNC,		IDC_ACTIVEUPDATE,	Folder::FLAG_SYNCABLE,	true,	false	},
	{ Folder::FLAG_CACHEWHENREAD,	IDC_CACHEWHENREAD,	Folder::FLAG_SYNCABLE,	false,	false	},
	{ Folder::FLAG_IGNOREUNSEEN,	IDC_IGNOREUNSEEN,	Folder::FLAG_NOSELECT,	true,	true	},
	{ Folder::FLAG_INBOX,			IDC_INBOX,			Folder::FLAG_NOSELECT,	false,	true	},
	{ Folder::FLAG_OUTBOX,			IDC_OUTBOX,			Folder::FLAG_NOSELECT,	false,	true	},
	{ Folder::FLAG_SENTBOX,			IDC_SENTBOX,		Folder::FLAG_NOSELECT,	false,	true	},
	{ Folder::FLAG_DRAFTBOX,		IDC_DRAFTBOX,		Folder::FLAG_NOSELECT,	false,	true	},
	{ Folder::FLAG_TRASHBOX,		IDC_TRASHBOX,		Folder::FLAG_NOSELECT,	false,	true	},
	{ Folder::FLAG_JUNKBOX,			IDC_JUNKBOX,		Folder::FLAG_NOSELECT,	false,	true	}
};
};

qm::FolderPropertyPage::FolderPropertyPage(const Account::FolderList& l) :
	DefaultPropertyPage(IDD_FOLDERPROPERTY),
	listFolder_(l)
{
	assert(!l.empty());
}

qm::FolderPropertyPage::~FolderPropertyPage()
{
}

LRESULT qm::FolderPropertyPage::onInitDialog(HWND hwndFocus,
											 LPARAM lParam)
{
	if (listFolder_.size() == 1) {
		Folder* pFolder = listFolder_.front();
		
		wstring_ptr wstrName(pFolder->getFullName());
		setDlgItemText(IDC_NAME, wstrName.get());
		
		setDlgItemInt(IDC_ID, pFolder->getId());
		
		HINSTANCE hInst = Application::getApplication().getResourceHandle();
		wstring_ptr wstrType;
		switch (pFolder->getType()) {
		case Folder::TYPE_NORMAL:
			if (pFolder->isFlag(Folder::FLAG_LOCAL)) {
				if (pFolder->isFlag(Folder::FLAG_SYNCABLE))
					wstrType = loadString(hInst, IDS_SYNCABLELOCALFOLDER);
				else
					wstrType = loadString(hInst, IDS_LOCALFOLDER);
			}
			else {
				wstrType = loadString(hInst, IDS_REMOTEFOLDER);
			}
			break;
		case Folder::TYPE_QUERY:
			wstrType = loadString(hInst, IDS_QUERYFOLDER);
			break;
		default:
			assert(false);
			break;
		}
		setDlgItemText(IDC_TYPE, wstrType.get());
		
		bool bQuery = pFolder->getType() == Folder::TYPE_QUERY;
		unsigned int nFlags = pFolder->getFlags();
		for (int n = 0; n < countof(folderFlags); ++n) {
			sendDlgItemMessage(folderFlags[n].nId_, BM_SETCHECK,
				nFlags & folderFlags[n].flag_ ? BST_CHECKED : BST_UNCHECKED);
			Window(getDlgItem(folderFlags[n].nId_)).setStyle(
				BS_AUTOCHECKBOX, BS_AUTOCHECKBOX | BS_AUTO3STATE);
			
			bool bEnable = false;
			if (bQuery) {
				bEnable = folderFlags[n].bEnableQuery_;
			}
			else {
				bEnable = pFolder->isFlag(folderFlags[n].enableFlag_);
				if (folderFlags[n].bReverse_)
					bEnable = !bEnable;
			}
			if (!bEnable)
				Window(getDlgItem(folderFlags[n].nId_)).enableWindow(false);
		}
	}
	else {
		for (int n = 0; n < countof(folderFlags); ++n) {
			unsigned int nCount = 0;
			bool bEnable = false;
			for (Account::FolderList::const_iterator it = listFolder_.begin(); it != listFolder_.end(); ++it) {
				Folder* pFolder = *it;
				if (pFolder->getFlags() & folderFlags[n].flag_)
					++nCount;
				
				bool b = pFolder->getType() != Folder::TYPE_QUERY || folderFlags[n].bEnableQuery_;
				if (b) {
					b = pFolder->isFlag(folderFlags[n].enableFlag_);
					if (folderFlags[n].bReverse_)
						b = !b;
				}
				if (b)
					bEnable = true;
			}
			sendDlgItemMessage(folderFlags[n].nId_, BM_SETCHECK,
				nCount == 0 ? BST_UNCHECKED :
				nCount == listFolder_.size() ? BST_CHECKED : BST_INDETERMINATE);
			
			if (!bEnable)
				Window(getDlgItem(folderFlags[n].nId_)).enableWindow(false);
		}
	}
	
	return TRUE;
}

LRESULT qm::FolderPropertyPage::onOk()
{
	unsigned int nFlags = 0;
	unsigned int nMask = 0;
	
	for (int n = 0; n < countof(folderFlags); ++n) {
		int nCheck = Button_GetCheck(getDlgItem(folderFlags[n].nId_));
		switch (nCheck) {
		case BST_CHECKED:
			nFlags |= folderFlags[n].flag_;
			nMask |= folderFlags[n].flag_;
			break;
		case BST_UNCHECKED:
			nMask |= folderFlags[n].flag_;
			break;
		case BST_INDETERMINATE:
			break;
		default:
			assert(false);
			break;
		}
	}
	
	for (Account::FolderList::const_iterator it = listFolder_.begin(); it != listFolder_.end(); ++it) {
		Folder* pFolder = *it;
		
		unsigned int nFolderMask = nMask;
		
		switch (pFolder->getType()) {
		case Folder::TYPE_NORMAL:
			if (!pFolder->isFlag(Folder::FLAG_SYNCABLE))
				nFolderMask &= ~(Folder::FLAG_ACTIVESYNC | Folder::FLAG_CACHEWHENREAD);
			if (pFolder->isFlag(Folder::FLAG_NOSELECT))
				nFolderMask &= ~(Folder::FLAG_INBOX | Folder::FLAG_OUTBOX |
					Folder::FLAG_SENTBOX | Folder::FLAG_DRAFTBOX | Folder::FLAG_TRASHBOX);
			break;
		case Folder::TYPE_QUERY:
			nFolderMask &= ~(Folder::FLAG_CACHEWHENREAD | Folder::FLAG_INBOX | Folder::FLAG_OUTBOX |
				Folder::FLAG_SENTBOX | Folder::FLAG_DRAFTBOX | Folder::FLAG_TRASHBOX);
			break;
		default:
			assert(false);
			break;
		}
		
		pFolder->getAccount()->setFolderFlags(pFolder, nFlags, nFolderMask);
	}
	
	return DefaultPropertyPage::onOk();
}


/****************************************************************************
 *
 * FolderConditionPage
 *
 */

qm::FolderConditionPage::FolderConditionPage(QueryFolder* pFolder,
											 Profile* pProfile) :
	DefaultPropertyPage(IDD_FOLDERCONDITION),
	pFolder_(pFolder),
	pProfile_(pProfile)
{
}

qm::FolderConditionPage::~FolderConditionPage()
{
	std::for_each(listUI_.begin(), listUI_.end(), deleter<SearchUI>());
}

LRESULT qm::FolderConditionPage::onInitDialog(HWND hwndFocus,
											  LPARAM lParam)
{
	initDriver();
	initFolder();
	
	setDlgItemText(IDC_CONDITION, pFolder_->getCondition());
	sendDlgItemMessage(IDC_RECURSIVE, BM_SETCHECK,
		pFolder_->isRecursive() ? BST_CHECKED : BST_UNCHECKED);
	
	return TRUE;
}

LRESULT qm::FolderConditionPage::onOk()
{
	int nDriver = ComboBox_GetCurSel(getDlgItem(IDC_DRIVER));
	const WCHAR* pwszDriver = listUI_[nDriver]->getName();
	
	wstring_ptr wstrCondition = getDlgItemText(IDC_CONDITION);
	
	wstring_ptr wstrTargetFolder;
	const Folder* pFolder = FolderListComboBox(getDlgItem(IDC_FOLDER)).getSelectedFolder();
	if (pFolder)
		wstrTargetFolder = pFolder->getFullName();
	
	bool bRecursive = sendDlgItemMessage(IDC_RECURSIVE, BM_GETCHECK) == BST_CHECKED;
	
	pFolder_->set(pwszDriver, wstrCondition.get(),
		wstrTargetFolder.get(), bRecursive);
	
	return DefaultPropertyPage::onOk();
}

void qm::FolderConditionPage::initDriver()
{
	SearchDriverFactory::NameList listName;
	SearchDriverFactory::getNames(&listName);
	for (SearchDriverFactory::NameList::const_iterator itN = listName.begin(); itN != listName.end(); ++itN) {
		std::auto_ptr<SearchUI> pUI(SearchDriverFactory::getUI(
			*itN, pFolder_->getAccount(), pProfile_));
		if (pUI.get()) {
			listUI_.push_back(pUI.get());
			pUI.release();
		}
	}
	std::sort(listUI_.begin(), listUI_.end(),
		boost::bind(&SearchUI::getIndex, _1) <
		boost::bind(&SearchUI::getIndex, _2));
	int nIndex = 0;
	for (UIList::size_type n = 0; n < listUI_.size(); ++n) {
		SearchUI* pUI = listUI_[n];
		wstring_ptr wstrName(pUI->getDisplayName());
		W2T(wstrName.get(), ptszName);
		ComboBox_AddString(getDlgItem(IDC_DRIVER), ptszName);
		if (wcscmp(pUI->getName(), pFolder_->getDriver()) == 0)
			nIndex = static_cast<int>(n);
	}
	ComboBox_SetCurSel(getDlgItem(IDC_DRIVER), nIndex);
}

void qm::FolderConditionPage::initFolder()
{
	Account* pAccount = pFolder_->getAccount();
	Folder* pFolder = 0;
	const WCHAR* pwszTargetFolder = pFolder_->getTargetFolder();
	if (pwszTargetFolder)
		pFolder = pAccount->getFolder(pwszTargetFolder);
	FolderListComboBox(getDlgItem(IDC_FOLDER)).addFolders(pAccount, pFolder);
}


/****************************************************************************
 *
 * FolderParameterPage
 *
 */

qm::FolderParameterPage::FolderParameterPage(Folder* pFolder,
											 const WCHAR** ppwszParams,
											 size_t nParamCount) :
	DefaultPropertyPage(IDD_FOLDERPARAMETER),
	pFolder_(pFolder),
	ppwszParams_(ppwszParams),
	nParamCount_(nParamCount)
{
	assert(ppwszParams_);
	assert(nParamCount_ != 0);
}

qm::FolderParameterPage::~FolderParameterPage()
{
}

LRESULT qm::FolderParameterPage::onCommand(WORD nCode,
										   WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_EDIT, onEdit)
	END_COMMAND_HANDLER()
	return DefaultPropertyPage::onCommand(nCode, nId);
}

LRESULT qm::FolderParameterPage::onInitDialog(HWND hwndFocus,
											  LPARAM lParam)
{
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	HWND hwndList = getDlgItem(IDC_PARAMETER);
	
	ListView_SetExtendedListViewStyle(hwndList, LVS_EX_FULLROWSELECT);
	
	struct {
		UINT nId_;
		int nWidth_;
	} columns[] = {
		{ IDS_PROPERTY_NAME,	120	},
		{ IDS_PROPERTY_VALUE,	200	}
	};
	for (int n = 0; n < countof(columns); ++n) {
		wstring_ptr wstrName(loadString(hInst, columns[n].nId_));
		W2T(wstrName.get(), ptszName);
		
		LVCOLUMN column = {
			LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH,
			LVCFMT_LEFT,
			columns[n].nWidth_,
			const_cast<LPTSTR>(ptszName),
			0,
			n,
		};
		ListView_InsertColumn(hwndList, n, &column);
	}
	
	for (size_t n = 0; n < nParamCount_; ++n) {
		const WCHAR* pwszName = ppwszParams_[n];
		W2T(pwszName, ptszName);
		LVITEM item = {
			LVIF_TEXT,
			static_cast<int>(n),
			0,
			0,
			0,
			const_cast<LPTSTR>(ptszName),
		};
		ListView_InsertItem(hwndList, &item);
		
		const WCHAR* pwszValue = pFolder_->getParam(pwszName);
		if (!pwszValue)
			pwszValue = L"";
		W2T(pwszValue, ptszValue);
		ListView_SetItemText(hwndList, n, 1, const_cast<LPTSTR>(ptszValue));
	}
	
	updateState();
	
	return TRUE;
}

LRESULT qm::FolderParameterPage::onOk()
{
	HWND hwndList = getDlgItem(IDC_PARAMETER);
	
	for (size_t n = 0; n < nParamCount_; ++n) {
		const WCHAR* pwszName = ppwszParams_[n];
		
		TCHAR tszValue[1024];
		ListView_GetItemText(hwndList, n, 1, tszValue, countof(tszValue) - 1);
		T2W(tszValue, pwszValue);
		
		pFolder_->setParam(pwszName, pwszValue);
	}
	
	return DefaultPropertyPage::onOk();
}

LRESULT qm::FolderParameterPage::onNotify(NMHDR* pnmhdr,
										  bool* pbHandled)
{
	BEGIN_NOTIFY_HANDLER()
		HANDLE_NOTIFY(NM_DBLCLK, IDC_PARAMETER, onParameterDblClk)
		HANDLE_NOTIFY(LVN_ITEMCHANGED, IDC_PARAMETER, onParameterItemChanged)
	END_NOTIFY_HANDLER()
	return DefaultPropertyPage::onNotify(pnmhdr, pbHandled);
}

LRESULT qm::FolderParameterPage::onEdit()
{
	edit();
	return 0;
}

LRESULT qm::FolderParameterPage::onParameterDblClk(NMHDR* pnmhdr,
												   bool* pbHandled)
{
	edit();
	*pbHandled = true;
	return 0;
}

LRESULT qm::FolderParameterPage::onParameterItemChanged(NMHDR* pnmhdr,
														bool* pbHandled)
{
	updateState();
	*pbHandled = true;
	return 0;
}

void qm::FolderParameterPage::edit()
{
	HWND hwndList = getDlgItem(IDC_PARAMETER);
	
	int nItem = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);
	if (nItem != -1) {
		const WCHAR* pwszName = ppwszParams_[nItem];
		
		TCHAR tszValue[1024];
		ListView_GetItemText(hwndList, nItem, 1, tszValue, countof(tszValue) - 1);
		T2W(tszValue, pwszValue);
		
		ParameterDialog dialog(pwszName, pwszValue);
		if (dialog.doModal(getHandle()) == IDOK) {
			W2T(dialog.getValue(), ptszValue);
			ListView_SetItemText(hwndList, nItem, 1, const_cast<LPTSTR>(ptszValue));
		}
	}
}

void qm::FolderParameterPage::updateState()
{
	HWND hwndList = getDlgItem(IDC_PARAMETER);
	int nItem = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);
	Window(getDlgItem(IDC_EDIT)).enableWindow(nItem != -1);
}


/****************************************************************************
 *
 * ParameterDialog
 *
 */

qm::ParameterDialog::ParameterDialog(const WCHAR* pwszName,
									 const WCHAR* pwszValue) :
	DefaultDialog(IDD_PARAMETER)
{
	wstrName_ = allocWString(pwszName);
	wstrValue_ = allocWString(pwszValue);
}

qm::ParameterDialog::~ParameterDialog()
{
}

const WCHAR* qm::ParameterDialog::getValue() const
{
	return wstrValue_.get();
}

LRESULT qm::ParameterDialog::onInitDialog(HWND hwndFocus,
										  LPARAM lParam)
{
	init(false);
	
	setDlgItemText(IDC_NAME, wstrName_.get());
	setDlgItemText(IDC_VALUE, wstrValue_.get());
	
	return TRUE;
}

LRESULT qm::ParameterDialog::onOk()
{
	wstrValue_ = getDlgItemText(IDC_VALUE);
	return DefaultDialog::onOk();
}
