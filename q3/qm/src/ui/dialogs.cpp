/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#pragma warning(disable:4786)

#include <qmaccount.h>
#include <qmapplication.h>
#include <qmfilenames.h>
#include <qmgoround.h>
#include <qmsession.h>
#include <qmsyncfilter.h>
#include <qmuiutil.h>

#include <qsconv.h>
#include <qsmime.h>
#include <qsras.h>
#include <qsuiutil.h>

#include <algorithm>

#include <commdlg.h>
#include <tchar.h>

#include "conditiondialog.h"
#include "dialogs.h"
#include "uimanager.h"
#include "uiutil.h"
#include "../model/templatemanager.h"
#include "../util/util.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * DefaultDialog
 *
 */

qm::DefaultDialog::DefaultDialog(UINT nId) :
	qs::DefaultDialog(Application::getApplication().getResourceHandle(), nId)
{
}

qm::DefaultDialog::~DefaultDialog()
{
}


#ifdef QMZIP
/****************************************************************************
 *
 * ArchiveDialog
 *
 */

qm::ArchiveDialog::ArchiveDialog(const WCHAR* pwszFileName) :
	DefaultDialog(IDD_ARCHIVE)
{
	if (pwszFileName)
		wstrFileName_ = allocWString(pwszFileName);
}

qm::ArchiveDialog::~ArchiveDialog()
{
}

const WCHAR* qm::ArchiveDialog::getFileName() const
{
	return wstrFileName_.get();
}

LRESULT qm::ArchiveDialog::onCommand(WORD nCode,
									 WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID_CODE(IDC_FILENAME, EN_CHANGE, onFileNameChange)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::ArchiveDialog::onInitDialog(HWND hwndFocus,
										LPARAM lParam)
{
	init(false);
	
	if (wstrFileName_.get())
		setDlgItemText(IDC_FILENAME, wstrFileName_.get());
	
	updateState();
	
	return TRUE;
}

LRESULT qm::ArchiveDialog::onOk()
{
	wstrFileName_ = getDlgItemText(IDC_FILENAME);
	return DefaultDialog::onOk();
}

LRESULT qm::ArchiveDialog::onFileNameChange()
{
	updateState();
	return 0;
}

void qm::ArchiveDialog::updateState()
{
	bool bEnable = Window(getDlgItem(IDC_FILENAME)).getWindowTextLength() != 0;
	Window(getDlgItem(IDOK)).enableWindow(bEnable);
}
#endif // QMZIP


/****************************************************************************
 *
 * AttachmentDialog
 *
 */

qm::AttachmentDialog::AttachmentDialog(EditMessage::AttachmentList& listAttachment) :
	DefaultDialog(IDD_ATTACHMENT),
	listAttachment_(listAttachment)
{
	std::sort(listAttachment_.begin(), listAttachment_.end(),
		EditMessage::AttachmentComp());
}

qm::AttachmentDialog::~AttachmentDialog()
{
}

LRESULT qm::AttachmentDialog::onCommand(WORD nCode,
										WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_ADD, onAdd)
		HANDLE_COMMAND_ID(IDC_REMOVE, onRemove)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::AttachmentDialog::onDestroy()
{
	removeNotifyHandler(this);
	return DefaultDialog::onDestroy();
}

LRESULT qm::AttachmentDialog::onInitDialog(HWND hwndFocus,
										   LPARAM lParam)
{
	init(false);
	
	HWND hwndList = getDlgItem(IDC_ATTACHMENT);
	
	SHFILEINFO info = { 0 };
	HIMAGELIST hImageList = reinterpret_cast<HIMAGELIST>(::SHGetFileInfo(
		_T("dummy.txt"), FILE_ATTRIBUTE_NORMAL, &info, sizeof(info),
		SHGFI_USEFILEATTRIBUTES | SHGFI_SYSICONINDEX | SHGFI_SMALLICON));
	ListView_SetImageList(hwndList, hImageList, LVSIL_SMALL);
	
	update();
	updateState();
	
	addNotifyHandler(this);
	
	return TRUE;
}

LRESULT qm::AttachmentDialog::onNotify(NMHDR* pnmhdr,
									   bool* pbHandled)
{
	BEGIN_NOTIFY_HANDLER()
		HANDLE_NOTIFY(LVN_ITEMCHANGED, IDC_ATTACHMENT, onAttachmentItemChanged);
	END_NOTIFY_HANDLER()
	return NotifyHandler::onNotify(pnmhdr, pbHandled);
}

LRESULT qm::AttachmentDialog::onAdd()
{
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	wstring_ptr wstrFilter(loadString(hInst, IDS_FILTER_ATTACHMENT));
	
	FileDialog dialog(true, wstrFilter.get(), 0, 0, 0,
		OFN_EXPLORER | OFN_HIDEREADONLY | OFN_LONGNAMES | OFN_ALLOWMULTISELECT);
	if (dialog.doModal(getHandle()) == IDOK) {
		const WCHAR* pwszPath = dialog.getPath();
		const WCHAR* p = pwszPath;
		while (*p) {
			wstring_ptr wstrName(allocWString(p));
			EditMessage::Attachment attachment = {
				wstrName.get(),
				true
			};
			listAttachment_.push_back(attachment);
			wstrName.release();
			
			p += wcslen(p) + 1;
		}
		
		std::sort(listAttachment_.begin(), listAttachment_.end(),
			EditMessage::AttachmentComp());
		
		update();
	}
	
	updateState();
	
	return 0;
}

LRESULT qm::AttachmentDialog::onRemove()
{
	HWND hwndList = getDlgItem(IDC_ATTACHMENT);
	
	int nDeleted = 0;
	int nItem = -1;
	while (true) {
		nItem = ListView_GetNextItem(hwndList, nItem, LVNI_SELECTED);
		if (nItem == -1)
			break;
		EditMessage::AttachmentList::iterator it = listAttachment_.begin() + nItem - nDeleted;
		freeWString((*it).wstrName_);
		listAttachment_.erase(it);
		++nDeleted;
	}
	
	update();
	updateState();
	
	return 0;
}

LRESULT qm::AttachmentDialog::onAttachmentItemChanged(NMHDR* pnmhdr,
													  bool* pbHandled)
{
	updateState();
	*pbHandled = true;
	return 0;
}

void qm::AttachmentDialog::update()
{
	HWND hwndList = getDlgItem(IDC_ATTACHMENT);
	
	ListView_DeleteAllItems(hwndList);
	
	for (EditMessage::AttachmentList::size_type n = 0; n < listAttachment_.size(); ++n) {
		const EditMessage::Attachment& attachment = listAttachment_[n];
		
		wstring_ptr wstrName;
		int nIcon = 0;
		UIUtil::getAttachmentInfo(attachment, &wstrName, &nIcon);
		
		W2T(wstrName.get(), ptszName);
		LVITEM item = {
			LVIF_TEXT | LVIF_IMAGE,
			static_cast<int>(n),
			0,
			0,
			0,
			const_cast<LPTSTR>(ptszName),
			0,
			nIcon
		};
		ListView_InsertItem(hwndList, &item);
	}
}

void qm::AttachmentDialog::updateState()
{
	Window(getDlgItem(IDC_REMOVE)).enableWindow(
		ListView_GetSelectedCount(getDlgItem(IDC_ATTACHMENT)) != 0);
}


/****************************************************************************
 *
 * CertificateDialog
 *
 */

qm::CertificateDialog::CertificateDialog(const WCHAR* pwszCertificate) :
	DefaultDialog(IDD_CERTIFICATE)
{
	assert(pwszCertificate);
	
	StringBuffer<WSTRING> buf;
	for (const WCHAR* p = pwszCertificate; *p; ++p) {
		if (*p == L'\n')
			buf.append(L'\r');
		buf.append(*p);
	}
	wstrCertificate_ = buf.getString();
}

qm::CertificateDialog::~CertificateDialog()
{
}

LRESULT qm::CertificateDialog::onInitDialog(HWND hwndFocus,
											LPARAM lParam)
{
	init(true);
	
	setDlgItemText(IDC_CERTIFICATE, wstrCertificate_.get());
	Window(getDlgItem(IDC_CERTIFICATE)).setFocus();
	sendDlgItemMessage(IDC_CERTIFICATE, EM_SETSEL, -1, 0);
	
	return FALSE;
}


/****************************************************************************
 *
 * ConfirmSendDialog
 *
 */

qm::ConfirmSendDialog::ConfirmSendDialog() :
	DefaultDialog(IDD_CONFIRMSEND)
{
}
	
qm::ConfirmSendDialog::~ConfirmSendDialog()
{
}

LRESULT qm::ConfirmSendDialog::onCommand(WORD nCode,
										 WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_SEND, onSend)
		HANDLE_COMMAND_ID(IDC_SAVE, onSave)
		HANDLE_COMMAND_ID(IDC_DISCARD, onDiscard)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::ConfirmSendDialog::onInitDialog(HWND hwndFocus,
											LPARAM lParam)
{
	centerWindow(0);
	
#ifndef _WIN32_WCE
	HICON hIcon = ::LoadIcon(0, IDI_QUESTION);
	sendDlgItemMessage(IDC_QUESTION, STM_SETICON, reinterpret_cast<LPARAM>(hIcon));
#endif
	
	return TRUE;
}

LRESULT qm::ConfirmSendDialog::onSend()
{
	endDialog(ID_SEND);
	return 0;
}

LRESULT qm::ConfirmSendDialog::onSave()
{
	endDialog(ID_SAVE);
	return 0;
}

LRESULT qm::ConfirmSendDialog::onDiscard()
{
	endDialog(ID_DISCARD);
	return 0;
}


/****************************************************************************
 *
 * CustomFilterDialog
 *
 */

qm::CustomFilterDialog::CustomFilterDialog(const WCHAR* pwszCondition) :
	DefaultDialog(IDD_CUSTOMFILTER)
{
	if (pwszCondition)
		wstrCondition_ = allocWString(pwszCondition);
}

qm::CustomFilterDialog::~CustomFilterDialog()
{
}

const WCHAR* qm::CustomFilterDialog::getCondition() const
{
	return wstrCondition_.get();
}

LRESULT qm::CustomFilterDialog::onCommand(WORD nCode,
										  WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_EDIT, onEdit)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::CustomFilterDialog::onInitDialog(HWND hwndFocus,
											 LPARAM lParam)
{
	init(false);
	
	if (wstrCondition_.get())
		setDlgItemText(IDC_CONDITION, wstrCondition_.get());
	
	return TRUE;
}

LRESULT qm::CustomFilterDialog::onOk()
{
	wstring_ptr wstrCondition(getDlgItemText(IDC_CONDITION));
	std::auto_ptr<Macro> pCondition(MacroParser().parse(wstrCondition.get()));
	if (!pCondition.get()) {
		messageBox(Application::getApplication().getResourceHandle(),
			IDS_ERROR_INVALIDMACRO, MB_OK | MB_ICONERROR, getHandle());
		return 0;
	}
	wstrCondition_ = wstrCondition;
	
	return DefaultDialog::onOk();
}

LRESULT qm::CustomFilterDialog::onEdit()
{
	wstring_ptr wstrCondition(getDlgItemText(IDC_CONDITION));
	ConditionsDialog dialog(wstrCondition.get());
	if (dialog.doModal(getHandle()) == IDOK)
		setDlgItemText(IDC_CONDITION, dialog.getCondition());
	return 0;
}


/****************************************************************************
 *
 * DetachDialog
 *
 */

qm::DetachDialog::DetachDialog(Profile* pProfile,
							   List& list) :
	DefaultDialog(IDD_DETACH),
	pProfile_(pProfile),
	list_(list),
	bOpenFolder_(false)
{
	wstrFolder_ = pProfile_->getString(L"Global", L"DetachFolder", L"");
	bOpenFolder_ = pProfile_->getInt(L"Global", L"DetachOpenFolder", 0) != 0;
}

qm::DetachDialog::~DetachDialog()
{
}

const WCHAR* qm::DetachDialog::getFolder() const
{
	return wstrFolder_.get();
}

bool qm::DetachDialog::isOpenFolder() const
{
	return bOpenFolder_;
}

LRESULT qm::DetachDialog::onCommand(WORD nCode,
									WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_BROWSE, onBrowse)
		HANDLE_COMMAND_ID(IDC_RENAME, onRename)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::DetachDialog::onDestroy()
{
	removeNotifyHandler(this);
	return DefaultDialog::onDestroy();
}

LRESULT qm::DetachDialog::onInitDialog(HWND hwndFocus,
									   LPARAM lParam)
{
	init(false);
	
	HWND hwndList = getDlgItem(IDC_ATTACHMENT);
	ListView_SetExtendedListViewStyle(hwndList, LVS_EX_CHECKBOXES);
	for (List::size_type n = 0; n < list_.size(); ++n) {
		W2T(list_[n].wstrName_, ptszName);
		LVITEM item = {
			LVIF_TEXT,
			static_cast<int>(n),
			0,
			0,
			0,
			const_cast<LPTSTR>(ptszName),
			0
		};
		ListView_InsertItem(hwndList, &item);
		if (list_[n].bSelected_)
			ListView_SetCheckState(hwndList, n, TRUE);
	}
	
	setDlgItemText(IDC_FOLDER, wstrFolder_.get());
	Button_SetCheck(getDlgItem(IDC_OPENFOLDER),
		bOpenFolder_ ? BST_CHECKED : BST_UNCHECKED);
	
	updateState();
	
	addNotifyHandler(this);
	
	return TRUE;
}

LRESULT qm::DetachDialog::onOk()
{
	HWND hwndList = getDlgItem(IDC_ATTACHMENT);
	for (List::size_type n = 0; n < list_.size(); ++n) {
		if (ListView_GetCheckState(hwndList, n)) {
			TCHAR tszName[MAX_PATH];
			ListView_GetItemText(hwndList, n, 0, tszName, countof(tszName));
			wstring_ptr wstrName(tcs2wcs(tszName));
			if (wstrName.get() && wcscmp(wstrName.get(), list_[n].wstrName_) != 0) {
				freeWString(list_[n].wstrName_);
				list_[n].wstrName_ = wstrName.release();
			}
		}
		else {
			freeWString(list_[n].wstrName_);
			list_[n].wstrName_ = 0;
		}
	}
	
	wstrFolder_ = getDlgItemText(IDC_FOLDER);
	pProfile_->setString(L"Global", L"DetachFolder", wstrFolder_.get());
	
	bOpenFolder_ = Button_GetCheck(getDlgItem(IDC_OPENFOLDER)) == BST_CHECKED;
	pProfile_->setInt(L"Global", L"DetachOpenFolder", bOpenFolder_);
	
	return DefaultDialog::onOk();
}

LRESULT qm::DetachDialog::onNotify(NMHDR* pnmhdr,
								   bool* pbHandled)
{
	BEGIN_NOTIFY_HANDLER()
		HANDLE_NOTIFY(LVN_ENDLABELEDIT, IDC_ATTACHMENT, onAttachmentEndLabelEdit)
		HANDLE_NOTIFY(LVN_ITEMCHANGED, IDC_ATTACHMENT, onAttachmentItemChanged)
	END_NOTIFY_HANDLER()
	return NotifyHandler::onNotify(pnmhdr, pbHandled);
}

LRESULT qm::DetachDialog::onBrowse()
{
	wstring_ptr wstrFolder(getDlgItemText(IDC_FOLDER));
	
	wstring_ptr wstrPath(qs::UIUtil::browseFolder(getHandle(), 0, wstrFolder.get()));
	if (wstrPath.get())
		setDlgItemText(IDC_FOLDER, wstrPath.get());
	
	return 0;
}

LRESULT qm::DetachDialog::onRename()
{
	HWND hwndList = getDlgItem(IDC_ATTACHMENT);
	Window(hwndList).setFocus();
	for (int n = 0; n < ListView_GetItemCount(hwndList); ++n) {
		if (ListView_GetItemState(hwndList, n, LVIS_FOCUSED)) {
			ListView_EditLabel(hwndList, n);
			break;
		}
	}
	
	return 0;
}

LRESULT qm::DetachDialog::onAttachmentEndLabelEdit(NMHDR* pnmhdr,
												   bool* pbHandled)
{
	*pbHandled = true;
	
	NMLVDISPINFO* pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pnmhdr);
	if (pDispInfo->item.iItem == -1 || !pDispInfo->item.pszText)
		return 0;
	
	ListView_SetItemText(pDispInfo->hdr.hwndFrom,
		pDispInfo->item.iItem, 0, pDispInfo->item.pszText);
	
	return 1;
}

LRESULT qm::DetachDialog::onAttachmentItemChanged(NMHDR* pnmhdr,
												  bool* pbHandled)
{
	updateState();
	*pbHandled = true;
	return 0;
}

void qm::DetachDialog::updateState()
{
	HWND hwndList = getDlgItem(IDC_ATTACHMENT);
	Window(getDlgItem(IDC_RENAME)).enableWindow(
		ListView_GetSelectedCount(hwndList) != 0);
}


/****************************************************************************
 *
 * DialupDialog
 *
 */

qm::DialupDialog::DialupDialog(const WCHAR* pwszEntry,
							   const WCHAR* pwszUserName,
							   const WCHAR* pwszPassword,
							   const WCHAR* pwszDomain) :
	DefaultDialog(IDD_DIALUP)
{
	wstrEntry_ = allocWString(pwszEntry);
	wstrUserName_ = allocWString(pwszUserName);
	wstrPassword_ = allocWString(pwszPassword);
	wstrDomain_ = allocWString(pwszDomain);
}

qm::DialupDialog::~DialupDialog()
{
}

const WCHAR* qm::DialupDialog::getUserName() const
{
	return wstrUserName_.get();
}

const WCHAR* qm::DialupDialog::getPassword() const
{
	return wstrPassword_.get();
}

const WCHAR* qm::DialupDialog::getDomain() const
{
	return wstrDomain_.get();
}

LRESULT qm::DialupDialog::onCommand(WORD nCode,
									WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_DIALPROPERTY, onDialProperty)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::DialupDialog::onInitDialog(HWND hwndFocus,
									   LPARAM lParam)
{
	init(false);
	
	struct {
		UINT nId_;
		const WCHAR* pwsz_;
	} items[] = {
		{ IDC_ENTRY,	wstrEntry_.get()	},
		{ IDC_USERNAME,	wstrUserName_.get()	},
		{ IDC_PASSWORD,	wstrPassword_.get()	},
		{ IDC_DOMAIN,	wstrDomain_.get()	}
	};
	for (int n = 0; n < countof(items); ++n)
		setDlgItemText(items[n].nId_, items[n].pwsz_);
	
	updateLocation();
	
	if (!*wstrPassword_.get()) {
		Window(getDlgItem(IDC_PASSWORD)).setFocus();
		return FALSE;
	}
	
	return TRUE;
}

LRESULT qm::DialupDialog::onOk()
{
	struct {
		UINT nId_;
		wstring_ptr* pwstr_;
	} items[] = {
		{ IDC_ENTRY,	&wstrEntry_		},
		{ IDC_USERNAME,	&wstrUserName_	},
		{ IDC_PASSWORD,	&wstrPassword_	},
		{ IDC_DOMAIN,	&wstrDomain_	}
	};
	for (int n = 0; n < countof(items); ++n) {
		wstring_ptr wstr(getDlgItemText(items[n].nId_));
		if (wstr.get())
			*items[n].pwstr_ = wstr;
	}
	
	return DefaultDialog::onOk();
}

LRESULT qm::DialupDialog::onDialProperty()
{
	RasConnection::selectLocation(getHandle());
	updateLocation();
	return 0;
}

void qm::DialupDialog::updateLocation()
{
	wstring_ptr wstrLocation(RasConnection::getLocation());
	if (wstrLocation.get())
		setDlgItemText(IDC_DIALFROM, wstrLocation.get());
}


/****************************************************************************
 *
 * ExportDialog
 *
 */

qm::ExportDialog::ExportDialog(Account* pAccount,
							   const TemplateManager* pTemplateManager,
							   Profile* pProfile,
							   bool bSingleMessage) :
	DefaultDialog(IDD_EXPORT),
	pAccount_(pAccount),
	pTemplateManager_(pTemplateManager),
	pProfile_(pProfile),
	bSingleMessage_(bSingleMessage),
	nFlags_(0)
{
}

qm::ExportDialog::~ExportDialog()
{
}

const WCHAR* qm::ExportDialog::getPath() const
{
	return wstrPath_.get();
}

bool qm::ExportDialog::isFilePerMessage() const
{
	return (nFlags_ & FLAG_FILEPERMESSAGE) != 0;
}

bool qm::ExportDialog::isExportFlags() const
{
	return (nFlags_ & FLAG_EXPORTFLAGS) != 0;
}

const WCHAR* qm::ExportDialog::getTemplate() const
{
	return wstrTemplate_.get();
}

const WCHAR* qm::ExportDialog::getEncoding() const
{
	return wstrEncoding_.get();
}

LRESULT qm::ExportDialog::onCommand(WORD nCode,
									WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_BROWSE, onBrowse)
		HANDLE_COMMAND_ID_CODE(IDC_PATH, EN_CHANGE, onPathChange)
		HANDLE_COMMAND_ID_CODE(IDC_TEMPLATE, CBN_SELCHANGE, onTemplateSelChange)
		HANDLE_COMMAND_ID_CODE(IDC_ENCODING, CBN_EDITCHANGE, onEncodingEditChange)
		HANDLE_COMMAND_ID_CODE(IDC_ENCODING, CBN_SELCHANGE, onEncodingSelChange)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::ExportDialog::onInitDialog(HWND hwndFocus,
									   LPARAM lParam)
{
	init(false);
	
	if (bSingleMessage_)
		Window(getDlgItem(IDC_FILEPERMESSAGE)).enableWindow(false);
	
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	wstring_ptr wstrNone(loadString(hInst, IDS_MENU_NONE));
	W2T(wstrNone.get(), ptszNone);
	ComboBox_AddString(getDlgItem(IDC_TEMPLATE), ptszNone);
	
	TemplateManager::NameList listTemplate;
	StringListFree<TemplateManager::NameList> freeTemplate(listTemplate);
	pTemplateManager_->getTemplateNames(pAccount_->getClass(), L"export", &listTemplate);
	for (TemplateManager::NameList::const_iterator it = listTemplate.begin(); it != listTemplate.end(); ++it) {
		W2T(*it + 7, ptszTemplate);
		ComboBox_AddString(getDlgItem(IDC_TEMPLATE), ptszTemplate);
	}
	ComboBox_SetCurSel(getDlgItem(IDC_TEMPLATE), 0);
	
	UIUtil::EncodingList listEncoding;
	StringListFree<UIUtil::EncodingList> freeEncoding(listEncoding);
	UIUtil::loadEncodings(pProfile_, &listEncoding);
	for (UIUtil::EncodingList::const_iterator it = listEncoding.begin(); it != listEncoding.end(); ++it) {
		W2T(*it, ptszEncoding);
		ComboBox_AddString(getDlgItem(IDC_ENCODING), ptszEncoding);
	}
	ComboBox_SetCurSel(getDlgItem(IDC_ENCODING), 0);
	
	updateState();
	
	return TRUE;
}

LRESULT qm::ExportDialog::onOk()
{
	wstrPath_ = getDlgItemText(IDC_PATH);
	
	nFlags_ = 0;
	struct {
		UINT nId_;
		Flag flag_;
	} flags[] = {
		{ IDC_FILEPERMESSAGE,		FLAG_FILEPERMESSAGE	},
		{ IDC_EXPORTFLAGS,			FLAG_EXPORTFLAGS	}
	};
	for (int n = 0; n < countof(flags); ++n) {
		if (Button_GetCheck(getDlgItem(flags[n].nId_)) == BST_CHECKED)
			nFlags_ |= flags[n].flag_;
	}
	
	int nIndex = ComboBox_GetCurSel(getDlgItem(IDC_TEMPLATE));
	if (nIndex != CB_ERR && nIndex != 0) {
		int nLen = ComboBox_GetLBTextLen(getDlgItem(IDC_TEMPLATE), nIndex);
		tstring_ptr tstrTemplate(allocTString(nLen + 1));
		ComboBox_GetLBText(getDlgItem(IDC_TEMPLATE), nIndex, tstrTemplate.get());
		wstring_ptr wstrTemplate(tcs2wcs(tstrTemplate.get()));
		wstrTemplate_ = concat(L"export_", wstrTemplate.get());
		
		wstrEncoding_ = getDlgItemText(IDC_ENCODING);
	}
	
	return DefaultDialog::onOk();
}

LRESULT qm::ExportDialog::onBrowse()
{
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	wstring_ptr wstrFilter(loadString(hInst, IDS_FILTER_EXPORT));
	
	FileDialog dialog(false, wstrFilter.get(), 0, 0, 0,
		OFN_EXPLORER | OFN_HIDEREADONLY | OFN_LONGNAMES | OFN_OVERWRITEPROMPT);
	
	if (dialog.doModal(getHandle()) == IDOK) {
		setDlgItemText(IDC_PATH, dialog.getPath());
		updateState();
	}
	
	return 0;
}

LRESULT qm::ExportDialog::onPathChange()
{
	updateState();
	return 0;
}

LRESULT qm::ExportDialog::onTemplateSelChange()
{
	updateState();
	return 0;
}

LRESULT qm::ExportDialog::onEncodingEditChange()
{
	updateState();
	return 0;
}

LRESULT qm::ExportDialog::onEncodingSelChange()
{
	postMessage(WM_COMMAND, MAKEWPARAM(IDC_ENCODING, CBN_EDITCHANGE));
	return 0;
}

void qm::ExportDialog::updateState()
{
	int nIndex = ComboBox_GetCurSel(getDlgItem(IDC_TEMPLATE));
	Window(getDlgItem(IDC_ENCODING)).enableWindow(nIndex != 0 && nIndex != CB_ERR);
	
	bool bEnable = sendDlgItemMessage(IDC_PATH, WM_GETTEXTLENGTH) != 0 &&
		(nIndex == 0 || sendDlgItemMessage(IDC_ENCODING, WM_GETTEXTLENGTH) != 0);
	Window(getDlgItem(IDOK)).enableWindow(bEnable);
}


/****************************************************************************
 *
 * FindDialog
 *
 */

qm::FindDialog::FindDialog(Profile* pProfile,
						   bool bSupportRegex,
						   Callback* pCallback) :
	DefaultDialog(IDD_FIND),
	pProfile_(pProfile),
	bSupportRegex_(bSupportRegex),
	pCallback_(pCallback),
	wndFind_(pProfile, L"Find", L"", false),
	bMatchCase_(false),
	bRegex_(false),
	bPrev_(false)
{
}

qm::FindDialog::~FindDialog()
{
}

const WCHAR* qm::FindDialog::getFind() const
{
	return wstrFind_.get();
}

bool qm::FindDialog::isMatchCase() const
{
	return bMatchCase_;
}

bool qm::FindDialog::isRegex() const
{
	return bRegex_;
}

bool qm::FindDialog::isPrev() const
{
	return bPrev_;
}

LRESULT qm::FindDialog::onCommand(WORD nCode,
								  WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID_RANGE(IDC_FINDNEXT, IDC_FINDPREV, onFind)
		HANDLE_COMMAND_ID_CODE(IDC_FIND, CBN_EDITCHANGE, onFindChange)
		HANDLE_COMMAND_ID_CODE(IDC_FIND, CBN_SELCHANGE, onFindSelChange)
		HANDLE_COMMAND_ID(IDC_MATCHCASE, onMatchCaseChange)
		HANDLE_COMMAND_ID(IDC_REGEX, onRegexChange)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::FindDialog::onInitDialog(HWND hwndFocus,
									 LPARAM lParam)
{
	init(false);
	
	History history(pProfile_, L"Find");
	for (unsigned int n = 0; n < history.getSize(); ++n) {
		wstring_ptr wstr(history.getValue(n));
		if (*wstr.get()) {
			W2T(wstr.get(), ptsz);
			ComboBox_AddString(getDlgItem(IDC_FIND), ptsz);
		}
	}
	if (ComboBox_GetCount(getDlgItem(IDC_FIND)) != 0)
		ComboBox_SetCurSel(getDlgItem(IDC_FIND), 0);
	
	int nMatchCase = pProfile_->getInt(L"Find", L"MatchCase", 0);
	Button_SetCheck(getDlgItem(IDC_MATCHCASE), nMatchCase ? BST_CHECKED : BST_UNCHECKED);
	
	if (bSupportRegex_) {
		int nRegex = pProfile_->getInt(L"Find", L"Regex", 0);
		Button_SetCheck(getDlgItem(IDC_REGEX), nRegex ? BST_CHECKED : BST_UNCHECKED);
	}
	
	wndFind_.subclassWindow(::GetWindow(getDlgItem(IDC_FIND), GW_CHILD));
	
	updateState();
	
	return TRUE;
}

LRESULT qm::FindDialog::onFind(UINT nId)
{
	wstrFind_ = getDlgItemText(IDC_FIND);
	History(pProfile_, L"Find").addValue(wstrFind_.get());
	
	bMatchCase_ = Button_GetCheck(getDlgItem(IDC_MATCHCASE)) == BST_CHECKED;
	pProfile_->setInt(L"Find", L"MatchCase", bMatchCase_ ? 1 : 0);
	
	bRegex_ = Button_GetCheck(getDlgItem(IDC_REGEX)) == BST_CHECKED;
	pProfile_->setInt(L"Find", L"Regex", bRegex_ ? 1 : 0);
	
	bPrev_ = nId == IDC_FINDPREV;
	
	endDialog(IDOK);
	
	return 0;
}

LRESULT qm::FindDialog::onFindChange()
{
	updateState();
	notifyCallback();
	return 0;
}

LRESULT qm::FindDialog::onFindSelChange()
{
	postMessage(WM_COMMAND, MAKEWPARAM(IDC_FIND, CBN_EDITCHANGE));
	return 0;
}

LRESULT qm::FindDialog::onMatchCaseChange()
{
	notifyCallback();
	return 0;
}

LRESULT qm::FindDialog::onRegexChange()
{
	updateState();
	notifyCallback();
	return 0;
}

void qm::FindDialog::updateState()
{
	Window(getDlgItem(IDC_REGEX)).enableWindow(bSupportRegex_);
	bool bRegex = bSupportRegex_ &&
		Button_GetCheck(getDlgItem(IDC_REGEX)) == BST_CHECKED;
	Window(getDlgItem(IDC_MATCHCASE)).enableWindow(!bRegex);
	
	Window edit(Window(getDlgItem(IDC_FIND)).getWindow(GW_CHILD));
	bool bEnable = edit.getWindowTextLength() != 0;
	Window(getDlgItem(IDC_FINDNEXT)).enableWindow(bEnable);
	Window(getDlgItem(IDC_FINDPREV)).enableWindow(bEnable);
}

void qm::FindDialog::notifyCallback()
{
	if (pCallback_) {
		wstring_ptr wstrFind(getDlgItemText(IDC_FIND));
		bool bMatchCase = Button_GetCheck(getDlgItem(IDC_MATCHCASE)) == BST_CHECKED;
		bool bRegex = Button_GetCheck(getDlgItem(IDC_REGEX)) == BST_CHECKED;
		pCallback_->statusChanged(wstrFind.get(), bMatchCase, bRegex);
	}
}


/****************************************************************************
 *
 * FindDialog::Callback
 *
 */

qm::FindDialog::Callback::~Callback()
{
}


/****************************************************************************
 *
 * ImportDialog
 *
 */

qm::ImportDialog::ImportDialog(const WCHAR* pwszPath,
							   Profile* pProfile) :
	DefaultDialog(IDD_IMPORT),
	pProfile_(pProfile),
	bMultiple_(false),
	nFlags_(0)
{
	if (pwszPath)
		wstrPath_ = allocWString(pwszPath);
}

qm::ImportDialog::~ImportDialog()
{
}

const WCHAR* qm::ImportDialog::getPath() const
{
	return wstrPath_.get();
}

bool qm::ImportDialog::isMultiple() const
{
	return bMultiple_;
}

const WCHAR* qm::ImportDialog::getEncoding() const
{
	return *wstrEncoding_.get() ? wstrEncoding_.get() : 0;
}

unsigned int qm::ImportDialog::getFlags() const
{
	return nFlags_;
}

LRESULT qm::ImportDialog::onCommand(WORD nCode,
									WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_BROWSE, onBrowse)
		HANDLE_COMMAND_ID_CODE(IDC_PATH, EN_CHANGE, onPathChange)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::ImportDialog::onInitDialog(HWND hwndFocus,
									   LPARAM lParam)
{
	init(false);
	
	if (wstrPath_.get())
		setDlgItemText(IDC_PATH, wstrPath_.get());
	
	ComboBox_AddString(getDlgItem(IDC_ENCODING), _T(""));
	UIUtil::EncodingList listEncoding;
	StringListFree<UIUtil::EncodingList> freeEncoding(listEncoding);
	UIUtil::loadEncodings(pProfile_, &listEncoding);
	for (UIUtil::EncodingList::const_iterator it = listEncoding.begin(); it != listEncoding.end(); ++it) {
		W2T(*it, ptszEncoding);
		ComboBox_AddString(getDlgItem(IDC_ENCODING), ptszEncoding);
	}
	ComboBox_SetCurSel(getDlgItem(IDC_ENCODING), 0);
	
	Button_SetCheck(getDlgItem(IDC_NORMAL), BST_CHECKED);
	
	updateState();
	
	return TRUE;
}

LRESULT qm::ImportDialog::onOk()
{
	wstrPath_ = getDlgItemText(IDC_PATH);
	
	bMultiple_ = Button_GetCheck(getDlgItem(IDC_MULTIMESSAGES)) == BST_CHECKED;
	
	wstrEncoding_ = getDlgItemText(IDC_ENCODING);
	
	nFlags_ = 0;
	struct {
		UINT nId_;
		Account::ImportFlag flag_;
	} flags[] = {
		{ IDC_NORMAL,				Account::IMPORTFLAG_NORMALFLAGS		},
		{ IDC_QMAIL20COMPATIBLE,	Account::IMPORTFLAG_QMAIL20FLAGS	},
		{ IDC_IGNORE,				Account::IMPORTFLAG_IGNOREFLAGS		}
	};
	for (int n = 0; n < countof(flags); ++n) {
		if (Button_GetCheck(getDlgItem(flags[n].nId_)) == BST_CHECKED)
			nFlags_ |= flags[n].flag_;
	}
	
	return DefaultDialog::onOk();
}

LRESULT qm::ImportDialog::onBrowse()
{
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	wstring_ptr wstrFilter(loadString(hInst, IDS_FILTER_IMPORT));
	
	FileDialog dialog(true, wstrFilter.get(), 0, 0, 0,
		OFN_EXPLORER | OFN_HIDEREADONLY | OFN_LONGNAMES | OFN_ALLOWMULTISELECT);
	
	if (dialog.doModal(getHandle()) != IDOK)
		return 0;
	
	const WCHAR* pwszPath = dialog.getPath();
	if (*(pwszPath + wcslen(pwszPath) + 1)) {
		StringBuffer<WSTRING> buf;
		const WCHAR* p = pwszPath;
		while (true) {
			buf.append(p);
			p += wcslen(p) + 1;
			if (!*p)
				break;
			buf.append(L';');
		}
		setDlgItemText(IDC_PATH, buf.getCharArray());
	}
	else {
		setDlgItemText(IDC_PATH, pwszPath);
	}
	
	updateState();
	
	return 0;
}

LRESULT qm::ImportDialog::onPathChange()
{
	updateState();
	return 0;
}

void qm::ImportDialog::updateState()
{
	Window(getDlgItem(IDOK)).enableWindow(
		sendDlgItemMessage(IDC_PATH, WM_GETTEXTLENGTH) != 0);
}


/****************************************************************************
 *
 * InputBoxDialog
 *
 */

qm::InputBoxDialog::InputBoxDialog(bool bMultiLine,
								   const WCHAR* pwszMessage,
								   const WCHAR* pwszValue) :
	DefaultDialog(bMultiLine ? IDD_MULTIINPUTBOX : IDD_SINGLEINPUTBOX),
	bMultiLine_(bMultiLine)
{
	if (pwszMessage)
		wstrMessage_ = allocWString(pwszMessage);
	if (pwszValue)
		wstrValue_ = allocWString(pwszValue);
}

qm::InputBoxDialog::~InputBoxDialog()
{
}

const WCHAR* qm::InputBoxDialog::getValue() const
{
	return wstrValue_.get();
}

LRESULT qm::InputBoxDialog::onInitDialog(HWND hwndFocus,
										 LPARAM lParam)
{
	init(false);
	
	if (wstrMessage_.get())
		setDlgItemText(IDC_MESSAGE, wstrMessage_.get());
	
	if (wstrValue_.get()) {
		if (bMultiLine_ && wcschr(wstrValue_.get(), L'\n')) {
			StringBuffer<WSTRING> buf;
			
			const WCHAR* p = wstrValue_.get();
			while (*p) {
				if (*p == L'\n')
					buf.append(L'\r');
				buf.append(*p);
				++p;
			}
			
			setDlgItemText(IDC_VALUE, buf.getCharArray());
		}
		else {
			setDlgItemText(IDC_VALUE, wstrValue_.get());
		}
	}
	
	return TRUE;
}

LRESULT qm::InputBoxDialog::onOk()
{
	wstrValue_ = getDlgItemText(IDC_VALUE);
	if (bMultiLine_ && wstrValue_.get()) {
		WCHAR* pSrc = wstrValue_.get();
		WCHAR* pDst = wstrValue_.get();
		while (*pSrc) {
			if (*pSrc != L'\r')
				*pDst++ = *pSrc;
			++pSrc;
		}
		*pDst = L'\0';
	}
	return DefaultDialog::onOk();
}


/****************************************************************************
 *
 * LabelDialog
 *
 */

qm::LabelDialog::LabelDialog(const WCHAR* pwszLabel,
							 Profile* pProfile) :
	DefaultDialog(IDD_LABEL),
	pProfile_(pProfile)
{
	if (pwszLabel)
		wstrLabel_ = allocWString(pwszLabel);
}

qm::LabelDialog::~LabelDialog()
{
}

const WCHAR* qm::LabelDialog::getLabel() const
{
	return wstrLabel_.get();
}

LRESULT qm::LabelDialog::onInitDialog(HWND hwndFocus,
									  LPARAM lParam)
{
	init(false);
	
	History history(pProfile_, L"Label");
	for (unsigned int n = 0; n < history.getSize(); ++n) {
		wstring_ptr wstr(history.getValue(n));
		if (*wstr.get()) {
			W2T(wstr.get(), ptsz);
			ComboBox_AddString(getDlgItem(IDC_LABEL), ptsz);
		}
	}
	
	if (wstrLabel_.get())
		setDlgItemText(IDC_LABEL, wstrLabel_.get());
	
	return TRUE;
}

LRESULT qm::LabelDialog::onOk()
{
	wstrLabel_ = getDlgItemText(IDC_LABEL);
	History(pProfile_, L"Label").addValue(wstrLabel_.get());
	
	return DefaultDialog::onOk();
}


/****************************************************************************
 *
 * MacroDialog
 *
 */

qm::MacroDialog::MacroDialog(const WCHAR* pwszMacro) :
	DefaultDialog(IDD_MACRO)
{
	if (pwszMacro) {
		StringBuffer<WSTRING> buf;
		while (*pwszMacro) {
			if (*pwszMacro == L'\n')
				buf.append(L'\r');
			buf.append(*pwszMacro++);
		}
		wstrMacro_ = buf.getString();
	}
}

qm::MacroDialog::~MacroDialog()
{
}

const WCHAR* qm::MacroDialog::getMacro() const
{
	return wstrMacro_.get();
}

LRESULT qm::MacroDialog::onInitDialog(HWND hwndFocus,
									  LPARAM lParam)
{
	init(false);
	
	if (wstrMacro_.get())
		setDlgItemText(IDC_MACRO, wstrMacro_.get());
	
	return TRUE;
}

LRESULT qm::MacroDialog::onOk()
{
	wstrMacro_ = getDlgItemText(IDC_MACRO);
	
	WCHAR* pDst = wstrMacro_.get();
	for (const WCHAR* p = wstrMacro_.get(); *p; ++p) {
		if (*p != L'\r')
			*pDst++ = *p;
	}
	*pDst = L'\0';
	
	return DefaultDialog::onOk();
}


/****************************************************************************
 *
 * MailFolderDialog
 *
 */

qm::MailFolderDialog::MailFolderDialog(HINSTANCE hInstResource,
									   const WCHAR* pwszMailFolder) :
	DefaultDialog(hInstResource, IDD_MAILFOLDER)
{
	if (pwszMailFolder)
		wstrMailFolder_ = allocWString(pwszMailFolder);
}

qm::MailFolderDialog::~MailFolderDialog()
{
}

const WCHAR* qm::MailFolderDialog::getMailFolder() const
{
	return wstrMailFolder_.get();
}

LRESULT qm::MailFolderDialog::onCommand(WORD nCode,
										WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_BROWSE, onBrowse)
		HANDLE_COMMAND_ID_CODE(IDC_MAILFOLDER, EN_CHANGE, onMailFolderChange)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::MailFolderDialog::onInitDialog(HWND hwndFocus,
										   LPARAM lParam)
{
	init(false);
	
	if (wstrMailFolder_.get())
		setDlgItemText(IDC_MAILFOLDER, wstrMailFolder_.get());
	
	updateState();
	
	return TRUE;
}

LRESULT qm::MailFolderDialog::onOk()
{
	wstrMailFolder_ = getDlgItemText(IDC_MAILFOLDER);
	return DefaultDialog::onOk();
}

LRESULT qm::MailFolderDialog::onMailFolderChange()
{
	updateState();
	return 0;
}

LRESULT qm::MailFolderDialog::onBrowse()
{
	wstring_ptr wstrPath(qs::UIUtil::browseFolder(getHandle(), 0, 0));
	if (wstrPath.get())
		setDlgItemText(IDC_MAILFOLDER, wstrPath.get());
	
	return 0;
}

void qm::MailFolderDialog::updateState()
{
	Window(getDlgItem(IDOK)).enableWindow(
		Window(getDlgItem(IDC_MAILFOLDER)).getWindowTextLength() != 0);
}


/****************************************************************************
 *
 * MoveMessageDialog
 *
 */

qm::MoveMessageDialog::MoveMessageDialog(AccountManager* pAccountManager,
										 Account* pAccount,
										 Profile* pProfile) :
	DefaultDialog(IDD_MOVEMESSAGE),
	pAccountManager_(pAccountManager),
	pAccount_(pAccount),
	pProfile_(pProfile),
	pFolder_(0),
	bCopy_(false),
	bShowHidden_(false)
{
	bShowHidden_ = pProfile->getInt(L"MoveMessageDialog", L"ShowHidden", 0) != 0;
}

qm::MoveMessageDialog::~MoveMessageDialog()
{
}

NormalFolder* qm::MoveMessageDialog::getFolder() const
{
	return pFolder_;
}

bool qm::MoveMessageDialog::isCopy() const
{
	return bCopy_;
}

LRESULT qm::MoveMessageDialog::onCommand(WORD nCode,
										 WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_SHOWHIDDEN, onShowHidden)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::MoveMessageDialog::onNotify(NMHDR* pnmhdr,
										bool* pbHandled)
{
	BEGIN_NOTIFY_HANDLER()
		HANDLE_NOTIFY(TVN_SELCHANGED, IDC_FOLDER, onFolderSelChanged);
	END_NOTIFY_HANDLER()
	return NotifyHandler::onNotify(pnmhdr, pbHandled);
}

LRESULT qm::MoveMessageDialog::onInitDialog(HWND hwndFocus,
											LPARAM lParam)
{
	init(false);
	
	HIMAGELIST hImageList = ImageList_LoadImage(
		Application::getApplication().getResourceHandle(),
		MAKEINTRESOURCE(IDB_FOLDER), 16, 0, CLR_DEFAULT, IMAGE_BITMAP, 0);
	TreeView_SetImageList(getDlgItem(IDC_FOLDER), hImageList, TVSIL_NORMAL);
	
	if (bShowHidden_)
		Button_SetCheck(getDlgItem(IDC_SHOWHIDDEN), BST_CHECKED);
	
	Folder* pFolderSelected = 0;
	wstring_ptr wstrFolder(pAccount_->getProperty(L"UI", L"FolderTo", L""));
	if (*wstrFolder.get())
		pFolderSelected = pAccountManager_->getFolder(0, wstrFolder.get());
	
	update(pFolderSelected);
	updateState();
	addNotifyHandler(this);
	
	return TRUE;
}

LRESULT qm::MoveMessageDialog::onDestroy()
{
	pProfile_->setInt(L"MoveMessageDialog", L"ShowHidden", bShowHidden_);
	
	HIMAGELIST hImageList = TreeView_SetImageList(getHandle(), 0, TVSIL_NORMAL);
	ImageList_Destroy(hImageList);
	
	removeNotifyHandler(this);
	
	return DefaultDialog::onDestroy();
}

LRESULT qm::MoveMessageDialog::onOk()
{
	HWND hwnd = getDlgItem(IDC_FOLDER);
	HTREEITEM hItem = TreeView_GetSelection(hwnd);
	if (!TreeView_GetParent(hwnd, hItem))
		return 0;
	
	TVITEM item = {
		TVIF_HANDLE | TVIF_PARAM,
		hItem
	};
	TreeView_GetItem(hwnd, &item);
	
	Folder* pFolder = reinterpret_cast<Folder*>(item.lParam);
	if (pFolder->getType() != Folder::TYPE_NORMAL)
		return 0;
	pFolder_ = static_cast<NormalFolder*>(pFolder);
	
	bCopy_ = Button_GetCheck(getDlgItem(IDC_COPY)) == BST_CHECKED;
	
	wstring_ptr wstrFolderName(pFolder_->getFullName());
	ConcatW c[] = {
		{ L"//",								2	},
		{ pFolder_->getAccount()->getName(),	-1	},
		{ L"/",									1	},
		{ wstrFolderName.get(),					-1	}
	};
	wstring_ptr wstrName(concat(c, countof(c)));
	pAccount_->setProperty(L"UI", L"FolderTo", wstrName.get());
	
	return DefaultDialog::onOk();
}

LRESULT qm::MoveMessageDialog::onShowHidden()
{
	bool bShowHidden = Button_GetCheck(getDlgItem(IDC_SHOWHIDDEN)) == BST_CHECKED;
	if (bShowHidden != bShowHidden_) {
		bShowHidden_ = bShowHidden;
		
		Folder* pFolderSelected = 0;
		
		HWND hwndFolder = getDlgItem(IDC_FOLDER);
		HTREEITEM hItem = TreeView_GetSelection(hwndFolder);
		if (hItem && TreeView_GetParent(hwndFolder, hItem)) {
			TVITEM item = {
				TVIF_HANDLE | TVIF_PARAM,
				hItem
			};
			TreeView_GetItem(hwndFolder, &item);
			pFolderSelected = reinterpret_cast<Folder*>(item.lParam);
		}
		
		update(pFolderSelected);
	}
	return 0;
}

LRESULT qm::MoveMessageDialog::onFolderSelChanged(NMHDR* pnmhdr, bool* pbHandled)
{
	updateState();
	*pbHandled = true;
	return 0;
}

bool qm::MoveMessageDialog::update(Folder* pFolderSelected)
{
	HWND hwndFolder = getDlgItem(IDC_FOLDER);
	{
		DisableRedraw disable(hwndFolder);
		
		TreeView_DeleteAllItems(hwndFolder);
		
		const AccountManager::AccountList& listAccount = pAccountManager_->getAccounts();
		for (AccountManager::AccountList::const_iterator it = listAccount.begin(); it != listAccount.end(); ++it) {
			if (!insertAccount(hwndFolder, *it, pFolderSelected))
				return false;
		}
	}
	
	HTREEITEM hItem = TreeView_GetSelection(hwndFolder);
	if (hItem)
		TreeView_EnsureVisible(hwndFolder, hItem);
	
	return true;
}

bool qm::MoveMessageDialog::insertAccount(HWND hwnd,
										  Account* pAccount,
										  Folder* pFolderSelected)
{
	assert(pAccount);
	
	W2T(pAccount->getName(), ptszName);
	
	TVINSERTSTRUCT tvisAccount = {
		TVI_ROOT,
		TVI_SORT,
		{
			TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE,
			0,
			0,
			0,
			const_cast<LPTSTR>(ptszName),
			0,
			0,
			0,
			0,
			reinterpret_cast<LPARAM>(pAccount)
		}
	};
	HTREEITEM hItemAccount = TreeView_InsertItem(hwnd, &tvisAccount);
	if (!hItemAccount)
		return false;
	
	if (!insertFolders(hwnd, hItemAccount, pAccount, pFolderSelected))
		return false;
	
	return true;
}

bool qm::MoveMessageDialog::insertFolders(HWND hwnd,
										  HTREEITEM hItem,
										  Account* pAccount,
										  Folder* pFolderSelected)
{
	assert(hItem);
	assert(pAccount);
	
	const Account::FolderList& l = pAccount->getFolders();
	Account::FolderList listFolder;
	listFolder.reserve(l.size());
	if (bShowHidden_)
		std::copy(l.begin(), l.end(), std::back_inserter(listFolder));
	else
		std::remove_copy_if(l.begin(), l.end(),
			std::back_inserter(listFolder), std::mem_fun(&Folder::isHidden));
	std::sort(listFolder.begin(), listFolder.end(), FolderLess());
	
	typedef std::vector<std::pair<Folder*, HTREEITEM> > Stack;
	Stack stack;
	stack.push_back(Stack::value_type(0, hItem));
	
	for (Account::FolderList::const_iterator it = listFolder.begin(); it != listFolder.end(); ++it) {
		Folder* pFolder = *it;
		
		W2T(pFolder->getName(), ptszName);
		
		TVINSERTSTRUCT tvisFolder = {
			hItem,
			TVI_LAST,
			{
				TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE,
				0,
				0,
				0,
				const_cast<LPTSTR>(ptszName),
				0,
				UIUtil::getFolderImage(pFolder, false),
				UIUtil::getFolderImage(pFolder, true),
				0,
				reinterpret_cast<LPARAM>(pFolder)
			}
		};
		Folder* pParentFolder = pFolder->getParentFolder();
		while (stack.back().first != pParentFolder)
			stack.pop_back();
		assert(!stack.empty());
		tvisFolder.hParent = stack.back().second;
		
		HTREEITEM hItemFolder = TreeView_InsertItem(hwnd, &tvisFolder);
		if (!hItemFolder)
			return false;
		
		if (pFolder == pFolderSelected)
			TreeView_SelectItem(hwnd, hItemFolder);
		
		stack.push_back(Stack::value_type(pFolder, hItemFolder));
	}
	
	return true;
}

void qm::MoveMessageDialog::updateState()
{
	bool bEnable = false;
	
	HWND hwnd = getDlgItem(IDC_FOLDER);
	HTREEITEM hItem = TreeView_GetSelection(hwnd);
	if (TreeView_GetParent(hwnd, hItem)) {
		TVITEM item = {
			TVIF_HANDLE | TVIF_PARAM,
			hItem
		};
		TreeView_GetItem(hwnd, &item);
		
		Folder* pFolder = reinterpret_cast<Folder*>(item.lParam);
		if (pFolder->getType() == Folder::TYPE_NORMAL)
			bEnable = true;
	}
	
	Window(getDlgItem(IDOK)).enableWindow(bEnable);
}


/****************************************************************************
 *
 * PasswordDialog
 *
 */

qm::PasswordDialog::PasswordDialog(const WCHAR* pwszHint) :
	DefaultDialog(IDD_PASSWORD),
	state_(PASSWORDSTATE_ONETIME)
{
	wstrHint_ = allocWString(pwszHint);
}

qm::PasswordDialog::~PasswordDialog()
{
}

const WCHAR* qm::PasswordDialog::getPassword() const
{
	return wstrPassword_.get();
}

PasswordState qm::PasswordDialog::getState() const
{
	return state_;
}

LRESULT qm::PasswordDialog::onCommand(WORD nCode,
									  WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID_CODE(IDC_PASSWORD, EN_CHANGE, onPasswordChange)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::PasswordDialog::onInitDialog(HWND hwndFocus,
										 LPARAM lParam)
{
	init(false);
	
	setDlgItemText(IDC_HINT, wstrHint_.get());
	Button_SetCheck(getDlgItem(IDC_SESSION), BST_CHECKED);
	
	return TRUE;
}

LRESULT qm::PasswordDialog::onOk()
{
	wstrPassword_ = getDlgItemText(IDC_PASSWORD);
	
	if (Button_GetCheck(getDlgItem(IDC_DONTSAVE)) == BST_CHECKED)
		state_ = PASSWORDSTATE_ONETIME;
	else if (Button_GetCheck(getDlgItem(IDC_SESSION)) == BST_CHECKED)
		state_ = PASSWORDSTATE_SESSION;
	else if (Button_GetCheck(getDlgItem(IDC_SAVE)) == BST_CHECKED)
		state_ = PASSWORDSTATE_SAVE;
	
	return DefaultDialog::onOk();
}

LRESULT qm::PasswordDialog::onPasswordChange()
{
	updateState();
	return 0;
}

void qm::PasswordDialog::updateState()
{
	Window(getDlgItem(IDOK)).enableWindow(
		Window(getDlgItem(IDC_PASSWORD)).getWindowTextLength() != 0);
}


/****************************************************************************
 *
 * ProgressDialog
 *
 */

qm::ProgressDialog::ProgressDialog() :
	DefaultDialog(IDD_PROGRESS),
	bCancelable_(true),
	bCanceled_(false),
	nLastMessagePumpPos_(0)
{
}

qm::ProgressDialog::~ProgressDialog()
{
}

bool qm::ProgressDialog::init(HWND hwnd)
{
	if (!create(hwnd))
		return false;
	showWindow(SW_SHOW);
	getModalHandler()->preModalDialog(0);
	
	return true;
}

void qm::ProgressDialog::term()
{
	getModalHandler()->postModalDialog(0);
	destroyWindow();
}

bool qm::ProgressDialog::isCanceled()
{
	HWND hwnd = getHandle();
	if (sendDlgItemMessage(IDC_PROGRESS, PBM_GETPOS) % 10 == 0)
		hwnd = 0;
	pumpMessage(hwnd);
	
	return bCanceled_;
}

void qm::ProgressDialog::setCancelable(bool bCancelable)
{
	bCancelable_ = bCancelable;
	Window(getDlgItem(IDCANCEL)).enableWindow(bCancelable);
	pumpMessage(0);
}

void qm::ProgressDialog::setTitle(UINT nId)
{
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	wstring_ptr wstrMessage(loadString(hInst, nId));
	setWindowText(wstrMessage.get());
}

void qm::ProgressDialog::setMessage(UINT nId)
{
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	wstring_ptr wstrMessage(loadString(hInst, nId));
	setDlgItemText(IDC_MESSAGE, wstrMessage.get());
}

void qm::ProgressDialog::setMessage(const WCHAR* pwszMessage)
{
	setDlgItemText(IDC_MESSAGE, pwszMessage);
}

void qm::ProgressDialog::setRange(size_t nMin,
								  size_t nMax)
{
	sendDlgItemMessage(IDC_PROGRESS, PBM_SETRANGE32, nMin, nMax);
}

void qm::ProgressDialog::setPos(size_t n)
{
	sendDlgItemMessage(IDC_PROGRESS, PBM_SETPOS, n);
	
	if (!bCancelable_) {
		if (n - nLastMessagePumpPos_ >= 10) {
			pumpMessage(0);
			nLastMessagePumpPos_ = n;
		}
	}
}

void qm::ProgressDialog::setStep(size_t n)
{
	sendDlgItemMessage(IDC_PROGRESS, PBM_SETSTEP, n);
}

void qm::ProgressDialog::step()
{
	sendDlgItemMessage(IDC_PROGRESS, PBM_STEPIT);
}

LRESULT qm::ProgressDialog::onDestroy()
{
	return DefaultDialog::onDestroy();
}

LRESULT qm::ProgressDialog::onInitDialog(HWND hwndFocus,
										 LPARAM lParam)
{
	DefaultDialog::init(false);
	return DefaultDialog::onInitDialog(hwndFocus, lParam);
}

LRESULT qm::ProgressDialog::onCancel()
{
	bCanceled_ = true;
	return 0;
}

void qm::ProgressDialog::pumpMessage(HWND hwnd)
{
	MSG msg;
	while (::PeekMessage(&msg, hwnd, 0, 0, PM_REMOVE)) {
		::TranslateMessage(&msg);
		::DispatchMessage(&msg);
	}
}


/****************************************************************************
 *
 * RenameDialog
 *
 */

qm::RenameDialog::RenameDialog(const WCHAR* pwszName) :
	DefaultDialog(IDD_RENAME)
{
	wstrName_ = allocWString(pwszName);
}

qm::RenameDialog::~RenameDialog()
{
}

const WCHAR* qm::RenameDialog::getName() const
{
	return wstrName_.get();
}

LRESULT qm::RenameDialog::onCommand(WORD nCode,
									WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID_CODE(IDC_NAME, EN_CHANGE, onNameChange)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::RenameDialog::onInitDialog(HWND hwndFocus,
									   LPARAM lParam)
{
	init(false);
	setDlgItemText(IDC_NAME, wstrName_.get());
	return TRUE;
}

LRESULT qm::RenameDialog::onOk()
{
	wstrName_ = getDlgItemText(IDC_NAME);
	return DefaultDialog::onOk();
}

LRESULT qm::RenameDialog::onNameChange()
{
	updateState();
	return 0;
}

void qm::RenameDialog::updateState()
{
	Window(getDlgItem(IDOK)).enableWindow(
		Window(getDlgItem(IDC_NAME)).getWindowTextLength() != 0);
}


/****************************************************************************
 *
 * ReplaceDialog
 *
 */

qm::ReplaceDialog::ReplaceDialog(Profile* pProfile) :
	DefaultDialog(IDD_REPLACE),
	pProfile_(pProfile),
	bMatchCase_(false),
	bRegex_(false),
	type_(TYPE_NEXT)
{
}

qm::ReplaceDialog::~ReplaceDialog()
{
}

const WCHAR* qm::ReplaceDialog::getFind() const
{
	return wstrFind_.get();
}

const WCHAR* qm::ReplaceDialog::getReplace() const
{
	return wstrReplace_.get();
}

bool qm::ReplaceDialog::isMatchCase() const
{
	return bMatchCase_;
}

bool qm::ReplaceDialog::isRegex() const
{
	return bRegex_;
}

ReplaceDialog::Type qm::ReplaceDialog::getType() const
{
	return type_;
}

LRESULT qm::ReplaceDialog::onCommand(WORD nCode,
									 WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID_RANGE(IDC_REPLACENEXT, IDC_REPLACEALL, onReplace)
		HANDLE_COMMAND_ID_CODE(IDC_FIND, CBN_EDITCHANGE, onFindChange)
		HANDLE_COMMAND_ID_CODE(IDC_FIND, CBN_SELCHANGE, onFindSelChange)
		HANDLE_COMMAND_ID(IDC_REGEX, onRegexChange)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::ReplaceDialog::onInitDialog(HWND hwndFocus,
										LPARAM lParam)
{
	init(false);
	
	for (int n = 0; n < HISTORY_SIZE; ++n) {
		WCHAR wszKey[32];
		_snwprintf(wszKey, countof(wszKey), L"History%d", n);
		
		struct {
			const WCHAR* pwszSection_;
			UINT nId_;
		} items[] = {
			{ L"Find",		IDC_FIND	},
			{ L"Replace",	IDC_REPLACE	}
		};
		for (int m = 0; m < countof(items); ++m) {
			wstring_ptr wstr(pProfile_->getString(items[m].pwszSection_, wszKey, L""));
			if (*wstr.get()) {
				W2T(wstr.get(), ptsz);
				ComboBox_AddString(getDlgItem(items[m].nId_), ptsz);
			}
		}
	}
	
	int nMatchCase = pProfile_->getInt(L"Find", L"MatchCase", 0);
	Button_SetCheck(getDlgItem(IDC_MATCHCASE),
		nMatchCase ? BST_CHECKED : BST_UNCHECKED);
	
	int nRegex = pProfile_->getInt(L"Find", L"Regex", 0);
	Button_SetCheck(getDlgItem(IDC_REGEX),
		nRegex ? BST_CHECKED : BST_UNCHECKED);
	
	updateState();
	
	return TRUE;
}

LRESULT qm::ReplaceDialog::onReplace(UINT nId)
{
	struct {
		UINT nId_;
		wstring_ptr* pwstr_;
		const WCHAR* pwszSection_;
	} items[] = {
		{ IDC_FIND,		&wstrFind_,		L"Find"		},
		{ IDC_REPLACE,	&wstrReplace_,	L"Replace"	}
	};
	for (int m = 0; m < countof(items); ++m) {
		wstring_ptr wstrText(getDlgItemText(items[m].nId_));
		if (wstrText.get()) {
			for (int n = HISTORY_SIZE - 1; n > 0; --n) {
				WCHAR wszFromKey[32];
				_snwprintf(wszFromKey, countof(wszFromKey), L"History%d", n - 1);
				wstring_ptr wstr(pProfile_->getString(items[m].pwszSection_, wszFromKey, L""));
				
				WCHAR wszToKey[32];
				_snwprintf(wszToKey, countof(wszToKey), L"History%d", n);
				pProfile_->setString(items[m].pwszSection_, wszToKey, wstr.get());
			}
			
			pProfile_->setString(items[m].pwszSection_, L"History0", wstrText.get());
		}
		*items[m].pwstr_ = wstrText;
	}
	
	bMatchCase_ = Button_GetCheck(getDlgItem(IDC_MATCHCASE)) == BST_CHECKED;
	pProfile_->setInt(L"Find", L"MatchCase", bMatchCase_ ? 1 : 0);
	
	bRegex_ = Button_GetCheck(getDlgItem(IDC_REGEX)) == BST_CHECKED;
	pProfile_->setInt(L"Find", L"Regex", bRegex_ ? 1 : 0);
	
	type_ = nId == IDC_REPLACEPREV ? TYPE_PREV :
		nId == IDC_REPLACEALL ? TYPE_ALL : TYPE_NEXT;
	
	endDialog(IDOK);
	
	return 0;
}

LRESULT qm::ReplaceDialog::onFindChange()
{
	updateState();
	return 0;
}

LRESULT qm::ReplaceDialog::onFindSelChange()
{
	postMessage(WM_COMMAND, MAKEWPARAM(IDC_FIND, CBN_EDITCHANGE));
	return 0;
}

LRESULT qm::ReplaceDialog::onRegexChange()
{
	updateState();
	return 0;
}

void qm::ReplaceDialog::updateState()
{
	bool bRegex = Button_GetCheck(getDlgItem(IDC_REGEX)) == BST_CHECKED;
	Window(getDlgItem(IDC_MATCHCASE)).enableWindow(!bRegex);
	
	Window edit(Window(getDlgItem(IDC_FIND)).getWindow(GW_CHILD));
	bool bEnable = edit.getWindowTextLength() != 0;
	Window(getDlgItem(IDC_REPLACENEXT)).enableWindow(bEnable);
	Window(getDlgItem(IDC_REPLACEPREV)).enableWindow(bEnable);
	Window(getDlgItem(IDC_REPLACEALL)).enableWindow(bEnable);
}


/****************************************************************************
 *
 * ResourceDialog
 *
 */

qm::ResourceDialog::ResourceDialog(ResourceList& listResource) :
	DefaultDialog(IDD_RESOURCE),
	listResource_(listResource),
	bBackup_(false)
{
}

qm::ResourceDialog::~ResourceDialog()
{
}

bool qm::ResourceDialog::isBackup() const
{
	return bBackup_;
}

LRESULT qm::ResourceDialog::onCommand(WORD nCode,
									  WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_CHECKALL, onCheckAll)
		HANDLE_COMMAND_ID(IDC_CLEARALL, onClearAll)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::ResourceDialog::onInitDialog(HWND hwndFocus,
										 LPARAM lParam)
{
	init(false);
	
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	HWND hwnd = getDlgItem(IDC_RESOURCE);
	
	ListView_SetExtendedListViewStyle(hwnd, LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT);
	
	RECT rect;
	Window(hwnd).getClientRect(&rect);
	int nWidth = rect.right - rect.left - ::GetSystemMetrics(SM_CXVSCROLL);
	wstring_ptr wstrPath(loadString(hInst, IDS_RESOURCEPATH));
	W2T(wstrPath.get(), ptszPath);
	LVCOLUMN column = {
		LVCF_FMT | LVCF_TEXT | LVCF_WIDTH,
		LVCFMT_LEFT,
		nWidth,
		const_cast<LPTSTR>(ptszPath),
		0,
	};
	ListView_InsertColumn(hwnd, 0, &column);
	
	for (ResourceList::size_type n = 0; n < listResource_.size(); ++n) {
		W2T(listResource_[n].first, ptszName);
		
		LVITEM item = {
			LVIF_TEXT,
			static_cast<int>(n),
			0,
			0,
			0,
			const_cast<LPTSTR>(ptszName),
		};
		ListView_InsertItem(hwnd, &item);
		ListView_SetCheckState(hwnd, n, TRUE);
	}
	
	Button_SetCheck(getDlgItem(IDC_BACKUP), BST_CHECKED);
	
	return TRUE;
}

LRESULT qm::ResourceDialog::onOk()
{
	HWND hwnd = getDlgItem(IDC_RESOURCE);
	
	for (ResourceList::size_type n = 0; n < listResource_.size(); ++n)
		listResource_[n].second = ListView_GetCheckState(hwnd, n) != 0;
	
	bBackup_ = Button_GetCheck(getDlgItem(IDC_BACKUP)) == BST_CHECKED;
	
	return DefaultDialog::onOk();
}

LRESULT qm::ResourceDialog::onCheckAll()
{
	HWND hwnd = getDlgItem(IDC_RESOURCE);
	
	for (int n = 0; n < ListView_GetItemCount(hwnd); ++n)
		ListView_SetCheckState(hwnd, n, TRUE);
	
	return 0;
}

LRESULT qm::ResourceDialog::onClearAll()
{
	HWND hwnd = getDlgItem(IDC_RESOURCE);
	
	for (int n = 0; n < ListView_GetItemCount(hwnd); ++n)
		ListView_SetCheckState(hwnd, n, FALSE);
	
	return 0;
}


/****************************************************************************
 *
 * SelectBoxDialog
 *
 */

qm::SelectBoxDialog::SelectBoxDialog(Type type,
									 const WCHAR* pwszMessage,
									 const CandidateList& listCandidate,
									 const WCHAR* pwszValue) :
	DefaultDialog(type == TYPE_LIST ? IDD_LISTSELECTBOX : IDD_COMBOSELECTBOX),
	type_(type),
	listCandidate_(listCandidate)
{
	if (pwszMessage)
		wstrMessage_ = allocWString(pwszMessage);
	if (pwszValue)
		wstrValue_ = allocWString(pwszValue);
}

qm::SelectBoxDialog::~SelectBoxDialog()
{
}

const WCHAR* qm::SelectBoxDialog::getValue() const
{
	return wstrValue_.get();
}

LRESULT qm::SelectBoxDialog::onInitDialog(HWND hwndFocus,
										 LPARAM lParam)
{
	init(false);
	
	if (wstrMessage_.get())
		setDlgItemText(IDC_MESSAGE, wstrMessage_.get());
	
	switch (type_) {
	case TYPE_DROPDOWNLIST:
		Window(getDlgItem(IDC_VALUE)).showWindow(SW_HIDE);
		break;
	case TYPE_DROPDOWN:
		Window(getDlgItem(IDC_VALUELIST)).showWindow(SW_HIDE);
		break;
	}
	
	HWND hwnd = getList();
	for (CandidateList::const_iterator it = listCandidate_.begin(); it != listCandidate_.end(); ++it) {
		W2T(*it, ptsz);
		if (type_ == TYPE_LIST)
			ListBox_AddString(hwnd, ptsz);
		else
			ComboBox_AddString(hwnd, ptsz);
	}
	if (wstrValue_.get()) {
		W2T(wstrValue_.get(), ptszValue);
		switch (type_) {
		case TYPE_LIST:
			ListBox_SelectString(hwnd, -1, ptszValue);
			break;
		case TYPE_DROPDOWNLIST:
			ComboBox_SelectString(hwnd, -1, ptszValue);
			break;
		case TYPE_DROPDOWN:
			setDlgItemText(IDC_VALUE, wstrValue_.get());
			break;
		default:
			assert(false);
			break;
		}
	}
	else {
		if (type_ == TYPE_LIST)
			ListBox_SetCurSel(hwnd, 0);
		else
			ComboBox_SetCurSel(hwnd, 0);
	}
	
	return TRUE;
}

LRESULT qm::SelectBoxDialog::onOk()
{
	HWND hwnd = getList();
	switch (type_) {
	case TYPE_LIST:
		{
			int nIndex = ListBox_GetCurSel(hwnd);
			int nLen = ListBox_GetTextLen(hwnd, nIndex);
			if (nLen == LB_ERR)
				return 0;
			tstring_ptr tstrValue(allocTString(nLen + 1));
			ListBox_GetText(hwnd, nIndex, tstrValue.get());
			wstrValue_ = tcs2wcs(tstrValue.get());
		}
		break;
	case TYPE_DROPDOWNLIST:
		{
			int nIndex = ComboBox_GetCurSel(hwnd);
			int nLen = ComboBox_GetLBTextLen(hwnd, nIndex);
			if (nLen == LB_ERR)
				return 0;
			tstring_ptr tstrValue(allocTString(nLen + 1));
			ComboBox_GetLBText(hwnd, nIndex, tstrValue.get());
			wstrValue_ = tcs2wcs(tstrValue.get());
		}
		break;
	case TYPE_DROPDOWN:
		wstrValue_ = getDlgItemText(IDC_VALUE);
		break;
	default:
		assert(false);
		break;
	}
	return DefaultDialog::onOk();
}

HWND qm::SelectBoxDialog::getList()
{
	return getDlgItem(type_ == TYPE_DROPDOWNLIST ? IDC_VALUELIST : IDC_VALUE);
}


/****************************************************************************
 *
 * SelectDialupEntryDialog
 *
 */

qm::SelectDialupEntryDialog::SelectDialupEntryDialog(Profile* pProfile) :
	DefaultDialog(IDD_SELECTDIALUPENTRY),
	pProfile_(pProfile)
{
}

qm::SelectDialupEntryDialog::~SelectDialupEntryDialog()
{
}

const WCHAR* qm::SelectDialupEntryDialog::getEntry() const
{
	return wstrEntry_.get();
}

LRESULT qm::SelectDialupEntryDialog::onCommand(WORD nCode,
											   WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID_CODE(IDC_ENTRY, LBN_SELCHANGE, onSelChange)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::SelectDialupEntryDialog::onInitDialog(HWND hwndFocus,
												  LPARAM lParam)
{
	init(false);
	
	typedef RasConnection::EntryList List;
	List listEntry;
	StringListFree<List> free(listEntry);
	RasConnection::getEntries(&listEntry);
	
	for (List::const_iterator it = listEntry.begin(); it != listEntry.end(); ++it) {
		W2T(*it, ptszEntry);
		ListBox_AddString(getDlgItem(IDC_ENTRY), ptszEntry);
	}
	
	wstring_ptr wstrEntry(pProfile_->getString(L"Dialup", L"Entry", 0));
	W2T(wstrEntry.get(), ptszEntry);
	ListBox_SelectString(getDlgItem(IDC_ENTRY), -1, ptszEntry);
	
	updateState();
	
	return TRUE;
}

LRESULT qm::SelectDialupEntryDialog::onOk()
{
	int nIndex = ListBox_GetCurSel(getDlgItem(IDC_ENTRY));
	if (nIndex == LB_ERR)
		return 0;
	
	int nLen = ListBox_GetTextLen(getDlgItem(IDC_ENTRY), nIndex);
	if (nLen == LB_ERR)
		return 0;
	
	tstring_ptr tstrEntry(allocTString(nLen + 1));
	ListBox_GetText(getDlgItem(IDC_ENTRY), nIndex, tstrEntry.get());
	
	wstrEntry_ = tcs2wcs(tstrEntry.get());
	pProfile_->setString(L"Dialup", L"Entry", wstrEntry_.get());
	
	return DefaultDialog::onOk();
}

LRESULT qm::SelectDialupEntryDialog::onSelChange()
{
	updateState();
	return 0;
}

void qm::SelectDialupEntryDialog::updateState()
{
	bool bEnable = ListBox_GetCurSel(getDlgItem(IDC_ENTRY)) != LB_ERR;
	Window(getDlgItem(IDOK)).enableWindow(bEnable);
}


/****************************************************************************
 *
 * SelectSyncFilterDialog
 *
 */

qm::SelectSyncFilterDialog::SelectSyncFilterDialog(SyncFilterManager* pManager,
												   const WCHAR* pwszDefaultName) :
	DefaultDialog(IDD_SELECTSYNCFILTER),
	pManager_(pManager)
{
	if (pwszDefaultName)
		wstrName_ = allocWString(pwszDefaultName);
}

qm::SelectSyncFilterDialog::~SelectSyncFilterDialog()
{
}

const WCHAR* qm::SelectSyncFilterDialog::getName() const
{
	return wstrName_.get();
}

LRESULT qm::SelectSyncFilterDialog::onInitDialog(HWND hwndFocus,
												 LPARAM lParam)
{
	init(false);
	
	const SyncFilterManager::FilterSetList& l = pManager_->getFilterSets();
	
	if (l.empty()) {
		endDialog(IDOK);
	}
	else {
		typedef SyncFilterManager::FilterSetList List;
		for (List::const_iterator it = l.begin(); it != l.end(); ++it) {
			W2T((*it)->getName(), ptszName);
			ListBox_AddString(getDlgItem(IDC_FILTERSETLIST), ptszName);
		}
		if (wstrName_.get()) {
			W2T(wstrName_.get(), ptszName);
			ListBox_SelectString(getDlgItem(IDC_FILTERSETLIST), -1, ptszName);
		}
		else {
			ListBox_SetCurSel(getDlgItem(IDC_FILTERSETLIST), 0);
		}
	}
	
	return TRUE;
}

LRESULT qm::SelectSyncFilterDialog::onOk()
{
	int nItem = ListBox_GetCurSel(getDlgItem(IDC_FILTERSETLIST));
	if (nItem == LB_ERR)
		return onCancel();
	
	int nLen = ListBox_GetTextLen(getDlgItem(IDC_FILTERSETLIST), nItem);
	tstring_ptr tstrName(allocTString(nLen + 10));
	ListBox_GetText(getDlgItem(IDC_FILTERSETLIST), nItem, tstrName.get());
	
	wstrName_ = tcs2wcs(tstrName.get());
	
	return DefaultDialog::onOk();
}


#ifdef QMTABWINDOW
/****************************************************************************
 *
 * TabTitleDialog
 *
 */

qm::TabTitleDialog::TabTitleDialog(const WCHAR* pwszTitle) :
	DefaultDialog(IDD_TABTITLE)
{
	if (pwszTitle)
		wstrTitle_ = allocWString(pwszTitle);
}

qm::TabTitleDialog::~TabTitleDialog()
{
}

const WCHAR* qm::TabTitleDialog::getTitle() const
{
	return wstrTitle_.get();
}

LRESULT qm::TabTitleDialog::onInitDialog(HWND hwndFocus,
										 LPARAM lParam)
{
	init(false);
	
	if (wstrTitle_.get())
		setDlgItemText(IDC_TITLE, wstrTitle_.get());
	
	return TRUE;
}

LRESULT qm::TabTitleDialog::onOk()
{
	wstrTitle_ = getDlgItemText(IDC_TITLE);
	return DefaultDialog::onOk();
}
#endif // TABWINDOW


/****************************************************************************
 *
 * ViewsColumnDialog
 *
 */

qm::ViewsColumnDialog::ViewsColumnDialog(ViewColumn* pColumn) :
	DefaultDialog(IDD_VIEWSCOLUMN),
	pColumn_(pColumn)
{
}

qm::ViewsColumnDialog::~ViewsColumnDialog()
{
}

LRESULT qm::ViewsColumnDialog::onCommand(WORD nCode,
										 WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID_CODE(IDC_TYPE, CBN_SELCHANGE, onTypeSelChange)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::ViewsColumnDialog::onInitDialog(HWND hwndFocus,
											LPARAM lParam)
{
	init(false);
	
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	
	setDlgItemText(IDC_TITLE, pColumn_->getTitle());
	
	UINT nTypes[] = {
		IDS_COLUMNTYPE_ID,
		IDS_COLUMNTYPE_DATE,
		IDS_COLUMNTYPE_FROM,
		IDS_COLUMNTYPE_TO,
		IDS_COLUMNTYPE_FROMTO,
		IDS_COLUMNTYPE_SUBJECT,
		IDS_COLUMNTYPE_SIZE,
		IDS_COLUMNTYPE_FLAGS,
		IDS_COLUMNTYPE_LABEL,
		IDS_COLUMNTYPE_OTHER
	};
	for (int n = 0; n < countof(nTypes); ++n) {
		wstring_ptr wstrType(loadString(hInst, nTypes[n]));
		W2T(wstrType.get(), ptszType);
		ComboBox_AddString(getDlgItem(IDC_TYPE), ptszType);
	}
	ComboBox_SetCurSel(getDlgItem(IDC_TYPE), pColumn_->getType() - 1);
	
	setDlgItemInt(IDC_WIDTH, pColumn_->getWidth());
	
	if (pColumn_->getType() == ViewColumn::TYPE_OTHER) {
		wstring_ptr wstrMacro(pColumn_->getMacro()->getString());
		setDlgItemText(IDC_MACRO, wstrMacro.get());
	}
	
	unsigned int nFlags = pColumn_->getFlags();
	if (nFlags & ViewColumn::FLAG_INDENT)
		Button_SetCheck(getDlgItem(IDC_INDENT), BST_CHECKED);
	if (nFlags & ViewColumn::FLAG_LINE)
		Button_SetCheck(getDlgItem(IDC_LINE), BST_CHECKED);
	if (nFlags & ViewColumn::FLAG_ICON)
		Button_SetCheck(getDlgItem(IDC_ASICON), BST_CHECKED);
	if (nFlags & ViewColumn::FLAG_CACHE)
		Button_SetCheck(getDlgItem(IDC_CACHE), BST_CHECKED);
	
	if (nFlags & ViewColumn::FLAG_RIGHTALIGN)
		Button_SetCheck(getDlgItem(IDC_RIGHTALIGN), BST_CHECKED);
	else
		Button_SetCheck(getDlgItem(IDC_LEFTALIGN), BST_CHECKED);
	
	switch (nFlags & ViewColumn::FLAG_SORT_MASK) {
	case ViewColumn::FLAG_SORT_TEXT:
		Button_SetCheck(getDlgItem(IDC_SORTTEXT), BST_CHECKED);
		break;
	case ViewColumn::FLAG_SORT_NUMBER:
		Button_SetCheck(getDlgItem(IDC_SORTNUMBER), BST_CHECKED);
		break;
	case ViewColumn::FLAG_SORT_DATE:
		Button_SetCheck(getDlgItem(IDC_SORTDATE), BST_CHECKED);
		break;
	default:
		Button_SetCheck(getDlgItem(IDC_SORTTEXT), BST_CHECKED);
		break;
	}
	
	updateState();
	
	return TRUE;
}

LRESULT qm::ViewsColumnDialog::onOk()
{
	wstring_ptr wstrTitle(getDlgItemText(IDC_TITLE));
	ViewColumn::Type type = static_cast<ViewColumn::Type>(
		ComboBox_GetCurSel(getDlgItem(IDC_TYPE)) + 1);
	std::auto_ptr<Macro> pMacro;
	if (type == ViewColumn::TYPE_OTHER) {
		wstring_ptr wstrMacro(getDlgItemText(IDC_MACRO));
		pMacro = MacroParser().parse(wstrMacro.get());
		if (!pMacro.get()) {
			messageBox(Application::getApplication().getResourceHandle(),
				IDS_ERROR_INVALIDMACRO, MB_OK | MB_ICONERROR, getHandle());
			return 0;
		}
	}
	
	unsigned int nWidth = getDlgItemInt(IDC_WIDTH);
	
	unsigned int nFlags = 0;
	if (Button_GetCheck(getDlgItem(IDC_INDENT)) == BST_CHECKED)
		nFlags |= ViewColumn::FLAG_INDENT;
	if (Button_GetCheck(getDlgItem(IDC_LINE)) == BST_CHECKED)
		nFlags |= ViewColumn::FLAG_LINE;
	if (Button_GetCheck(getDlgItem(IDC_ASICON)) == BST_CHECKED)
		nFlags |= ViewColumn::FLAG_ICON;
	if (Button_GetCheck(getDlgItem(IDC_CACHE)) == BST_CHECKED)
		nFlags |= ViewColumn::FLAG_CACHE;
	if (Button_GetCheck(getDlgItem(IDC_RIGHTALIGN)) == BST_CHECKED)
		nFlags |= ViewColumn::FLAG_RIGHTALIGN;
	if (Button_GetCheck(getDlgItem(IDC_SORTNUMBER)) == BST_CHECKED)
		nFlags |= ViewColumn::FLAG_SORT_NUMBER;
	else if (Button_GetCheck(getDlgItem(IDC_SORTDATE)) == BST_CHECKED)
		nFlags |= ViewColumn::FLAG_SORT_DATE;
	else
		nFlags |= ViewColumn::FLAG_SORT_TEXT;
	
	pColumn_->set(wstrTitle.get(), type, pMacro, nFlags, nWidth);
	
	return DefaultDialog::onOk();
}

LRESULT qm::ViewsColumnDialog::onTypeSelChange()
{
	updateState();
	return 0;
}

void qm::ViewsColumnDialog::updateState()
{
	bool bEnable = ComboBox_GetCurSel(getDlgItem(IDC_TYPE)) ==
		ComboBox_GetCount(getDlgItem(IDC_TYPE)) - 1;
	Window(getDlgItem(IDC_MACRO)).enableWindow(bEnable);
	Window(getDlgItem(IDC_CACHE)).enableWindow(bEnable);
}


/****************************************************************************
 *
 * ViewsDialog
 *
 */

qm::ViewsDialog::ViewsDialog(ViewModelManager* pViewModelManager,
							 ViewModel* pViewModel) :
	DefaultDialog(IDD_VIEWS),
	pViewModelManager_(pViewModelManager),
	pViewModel_(pViewModel),
	nSort_(pViewModel->getSort()),
	nMode_(pViewModel->getMode())
{
	const ViewColumnList& listColumn = pViewModel->getColumns();
	listColumn_.reserve(listColumn.size());
	for (ViewColumnList::const_iterator it = listColumn.begin(); it != listColumn.end(); ++it) {
		std::auto_ptr<ViewColumn> pColumn((*it)->clone());
		listColumn_.push_back(pColumn.release());
	}
}

qm::ViewsDialog::~ViewsDialog()
{
	std::for_each(listColumn_.begin(), listColumn_.end(), deleter<ViewColumn>());
}

LRESULT qm::ViewsDialog::onCommand(WORD nCode,
								   WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_ADD, onAdd)
		HANDLE_COMMAND_ID(IDC_REMOVE, onRemove)
		HANDLE_COMMAND_ID(IDC_EDIT, onEdit)
		HANDLE_COMMAND_ID(IDC_UP, onUp)
		HANDLE_COMMAND_ID(IDC_DOWN, onDown)
		HANDLE_COMMAND_ID(IDC_ASDEFAULT, onAsDefault)
		HANDLE_COMMAND_ID(IDC_APPLYDEFAULT, onApplyDefault)
		HANDLE_COMMAND_ID(IDC_INHERIT, onInherit)
		HANDLE_COMMAND_ID(IDC_APPLYTOALL, onApplyToAll)
		HANDLE_COMMAND_ID(IDC_APPLYTOCHILDREN, onApplyToChildren)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::ViewsDialog::onNotify(NMHDR* pnmhdr,
								  bool* pbHandled)
{
	BEGIN_NOTIFY_HANDLER()
		HANDLE_NOTIFY(NM_DBLCLK, IDC_COLUMNS, onColumnsDblClk)
		HANDLE_NOTIFY(LVN_ITEMCHANGED, IDC_COLUMNS, onColumnsItemChanged)
	END_NOTIFY_HANDLER()
	return NotifyHandler::onNotify(pnmhdr, pbHandled);
}

LRESULT qm::ViewsDialog::onDestroy()
{
	removeNotifyHandler(this);
	return DefaultDialog::onDestroy();
}

LRESULT qm::ViewsDialog::onInitDialog(HWND hwndFocus,
									  LPARAM lParam)
{
	init(false);
	
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	
	HWND hwndList = getDlgItem(IDC_COLUMNS);
	ListView_SetExtendedListViewStyle(hwndList, LVS_EX_FULLROWSELECT);
	
	struct {
		UINT nId_;
		int nWidth_;
	} columns[] = {
#ifndef _WIN32_WCE_PSPC
		{ IDS_COLUMN_TITLE,	150	},
		{ IDS_COLUMN_TYPE,	150	},
		{ IDS_COLUMN_WIDTH,	50	}
#else
		{ IDS_COLUMN_TITLE,	80	},
		{ IDS_COLUMN_TYPE,	80	},
		{ IDS_COLUMN_WIDTH,	30	}
#endif
	};
	for (int n = 0; n < countof(columns); ++n) {
		wstring_ptr wstrColumn(loadString(hInst, columns[n].nId_));
		W2T(wstrColumn.get(), ptszColumn);
		
		LVCOLUMN column = {
			LVCF_FMT | LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM,
			LVCFMT_LEFT,
			columns[n].nWidth_,
			const_cast<LPTSTR>(ptszColumn),
			0,
			n,
		};
		ListView_InsertColumn(hwndList, n, &column);
	}
	
	update();
	updateState();
	addNotifyHandler(this);
	
	return TRUE;
}

LRESULT qm::ViewsDialog::onOk()
{
	pViewModel_->setColumns(listColumn_);
	listColumn_.clear();
	pViewModel_->setSort(nSort_, 0xffffffff);
	pViewModel_->setMode(nMode_, 0xffffffff);
	return DefaultDialog::onOk();
}

LRESULT qm::ViewsDialog::onAdd()
{
	HWND hwndList = getDlgItem(IDC_COLUMNS);
	
	std::auto_ptr<Macro> pMacro;
	std::auto_ptr<ViewColumn> pColumn(new ViewColumn(L"New Column",
		ViewColumn::TYPE_ID, pMacro, ViewColumn::FLAG_SORT_TEXT, 100));
	ViewsColumnDialog dialog(pColumn.get());
	if (dialog.doModal(getHandle()) == IDOK) {
		listColumn_.push_back(pColumn.get());
		pColumn.release();
		update();
		ListView_SetItemState(hwndList, ListView_GetItemCount(hwndList) - 1,
			LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
	}
	return 0;
}

LRESULT qm::ViewsDialog::onRemove()
{
	HWND hwndList = getDlgItem(IDC_COLUMNS);
	if (ListView_GetItemCount(hwndList) > 1) {
		int nItem = ListView_GetNextItem(hwndList, -1, LVNI_ALL | LVNI_SELECTED);
		if (nItem != -1) {
			ViewColumnList::iterator it = listColumn_.begin() + nItem;
			delete *it;
			listColumn_.erase(it);
			update();
			ListView_SetItemState(hwndList, nItem,
				LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
		}
	}
	return 0;
}

LRESULT qm::ViewsDialog::onEdit()
{
	HWND hwndList = getDlgItem(IDC_COLUMNS);
	int nItem = ListView_GetNextItem(hwndList, -1, LVNI_ALL | LVNI_SELECTED);
	if (nItem != -1) {
		LVITEM item = {
			LVIF_PARAM,
			nItem
		};
		ListView_GetItem(hwndList, &item);
		ViewColumn* pColumn = reinterpret_cast<ViewColumn*>(item.lParam);
		
		ViewsColumnDialog dialog(pColumn);
		if (dialog.doModal(getHandle()) == IDOK) {
			update();
			ListView_SetItemState(hwndList, nItem,
				LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
		}
	}
	
	return 0;
}

LRESULT qm::ViewsDialog::onUp()
{
	HWND hwndList = getDlgItem(IDC_COLUMNS);
	int nItem = ListView_GetNextItem(hwndList, -1, LVNI_ALL | LVNI_SELECTED);
	if (nItem != -1 && nItem != 0) {
		std::swap(listColumn_[nItem - 1], listColumn_[nItem]);
		update();
		ListView_SetItemState(hwndList, nItem - 1,
			LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
	}
	return 0;
}

LRESULT qm::ViewsDialog::onDown()
{
	HWND hwndList = getDlgItem(IDC_COLUMNS);
	int nItem = ListView_GetNextItem(hwndList, -1, LVNI_ALL | LVNI_SELECTED);
	if (nItem != -1 && nItem != ListView_GetItemCount(hwndList) - 1) {
		std::swap(listColumn_[nItem + 1], listColumn_[nItem]);
		update();
		ListView_SetItemState(hwndList, nItem + 1,
			LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
	}
	return 0;
}

LRESULT qm::ViewsDialog::onAsDefault()
{
	ViewDataItem* pItem = getDefaultItem();
	ViewColumnList listColumn;
	cloneColumns(listColumn_, &listColumn);
	pItem->setColumns(listColumn);
	pItem->setSort(nSort_);
	pItem->setMode(nMode_);
	return 0;
}

LRESULT qm::ViewsDialog::onApplyDefault()
{
	ViewDataItem* pItem = getDefaultItem();
	setColumns(pItem->getColumns());
	nSort_ = pItem->getSort();
	nMode_ = pItem->getMode();
	update();
	return 0;
}

LRESULT qm::ViewsDialog::onInherit()
{
	Folder* pFolder = pViewModel_->getFolder()->getParentFolder();
	if (pFolder) {
		ViewModel* pViewModel = pViewModelManager_->getViewModel(pFolder);
		setColumns(pViewModel->getColumns());
		nSort_ = pViewModel->getSort();
		nMode_ = pViewModel->getMode();
		update();
	}
	return 0;
}

LRESULT qm::ViewsDialog::onApplyToAll()
{
	Account* pAccount = pViewModel_->getFolder()->getAccount();
	const Account::FolderList& listFolder = pAccount->getFolders();
	for (Account::FolderList::const_iterator it = listFolder.begin(); it != listFolder.end(); ++it) {
		Folder* pFolder = *it;
		ViewModel* pViewModel = pViewModelManager_->getViewModel(pFolder);
		ViewColumnList listColumn;
		cloneColumns(listColumn_, &listColumn);
		pViewModel->setColumns(listColumn);
		pViewModel->setSort(nSort_, 0xffffffff);
		pViewModel->setMode(nMode_, 0xffffffff);
	}
	
	Window(getDlgItem(IDCANCEL)).enableWindow(false);
	
	return 0;
}

LRESULT qm::ViewsDialog::onApplyToChildren()
{
	Folder* pCurrentFolder = pViewModel_->getFolder();
	Account* pAccount = pCurrentFolder->getAccount();
	const Account::FolderList& listFolder = pAccount->getFolders();
	for (Account::FolderList::const_iterator it = listFolder.begin(); it != listFolder.end(); ++it) {
		Folder* pFolder = *it;
		if (pCurrentFolder->isAncestorOf(pFolder)) {
			ViewModel* pViewModel = pViewModelManager_->getViewModel(pFolder);
			ViewColumnList listColumn;
			cloneColumns(listColumn_, &listColumn);
			pViewModel->setColumns(listColumn);
			pViewModel->setSort(nSort_, 0xffffffff);
			pViewModel->setMode(nMode_, 0xffffffff);
		}
	}
	return 0;
}

LRESULT qm::ViewsDialog::onColumnsDblClk(NMHDR* pnmhdr,
										 bool* pbHandled)
{
	onEdit();
	*pbHandled = true;
	return 0;
}

LRESULT qm::ViewsDialog::onColumnsItemChanged(NMHDR* pnmhdr,
											  bool* pbHandled)
{
	updateState();
	*pbHandled = true;
	return 0;
}

void qm::ViewsDialog::update()
{
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	HWND hwndList = getDlgItem(IDC_COLUMNS);
	
	DisableRedraw disable(hwndList);
	
	ListView_DeleteAllItems(hwndList);
	
	UINT nTypes[] = {
		0,
		IDS_COLUMNTYPE_ID,
		IDS_COLUMNTYPE_DATE,
		IDS_COLUMNTYPE_FROM,
		IDS_COLUMNTYPE_TO,
		IDS_COLUMNTYPE_FROMTO,
		IDS_COLUMNTYPE_SUBJECT,
		IDS_COLUMNTYPE_SIZE,
		IDS_COLUMNTYPE_FLAGS,
		IDS_COLUMNTYPE_LABEL,
		IDS_COLUMNTYPE_OTHER
	};
	
	for (ViewColumnList::size_type n = 0; n < listColumn_.size(); ++n) {
		ViewColumn* pColumn = listColumn_[n];
		
		W2T(pColumn->getTitle(), ptszTitle);
		
		LVITEM item = {
			LVIF_TEXT | LVIF_PARAM,
			static_cast<int>(n),
			0,
			0,
			0,
			const_cast<LPTSTR>(ptszTitle),
			0,
			0,
			reinterpret_cast<LPARAM>(pColumn)
		};
		ListView_InsertItem(hwndList, &item);
		
		wstring_ptr wstrType(loadString(hInst, nTypes[pColumn->getType()]));
		W2T(wstrType.get(), ptszType);
		ListView_SetItemText(hwndList, n, 1, const_cast<LPTSTR>(ptszType));
		
		WCHAR wszWidth[32];
		_snwprintf(wszWidth, countof(wszWidth), L"%u", pColumn->getWidth());
		W2T(wszWidth, ptszWidth);
		ListView_SetItemText(hwndList, n, 2, const_cast<LPTSTR>(ptszWidth));
	}
}

void qm::ViewsDialog::updateState()
{
	struct {
		UINT nId_;
		bool bEnable_;
	} items[] = {
		{ IDC_REMOVE,	true	},
		{ IDC_EDIT,		true	},
		{ IDC_UP,		true	},
		{ IDC_DOWN,		true	}
	};
	
	HWND hwndList = getDlgItem(IDC_COLUMNS);
	int nItem = ListView_GetNextItem(hwndList, -1, LVNI_ALL | LVNI_SELECTED);
	if (nItem == -1) {
		for (int n = 0; n < countof(items); ++n)
			items[n].bEnable_ = false;
	}
	else if (nItem == 0) {
		items[2].bEnable_ = false;
	}
	else if (nItem == ListView_GetItemCount(hwndList) - 1) {
		items[3].bEnable_ = false;
	}
	
	for (int n = 0; n < countof(items); ++n)
		Window(getDlgItem(items[n].nId_)).enableWindow(items[n].bEnable_);
	
	Window(getDlgItem(IDC_INHERIT)).enableWindow(pViewModel_->getFolder()->getParentFolder() != 0);
}

void qm::ViewsDialog::setColumns(const ViewColumnList& listColumn)
{
	std::for_each(listColumn_.begin(), listColumn_.end(), deleter<ViewColumn>());
	listColumn_.clear();
	cloneColumns(listColumn, &listColumn_);
}

void qm::ViewsDialog::cloneColumns(const ViewColumnList& listColumn,
								   ViewColumnList* pListColumn)
{
	assert(pListColumn);
	assert(pListColumn->empty());
	
	pListColumn->reserve(listColumn.size());
	for (ViewColumnList::const_iterator it = listColumn.begin(); it != listColumn.end(); ++it) {
		std::auto_ptr<ViewColumn> pColumn((*it)->clone());
		pListColumn->push_back(pColumn.release());
	}
}

ViewDataItem* qm::ViewsDialog::getDefaultItem()
{
	Folder* pFolder = pViewModel_->getFolder();
	Account* pAccount = pFolder->getAccount();
	DefaultViewData* pDefaultViewData = pViewModelManager_->getDefaultViewData();
	return pDefaultViewData->getItem(pAccount->getClass());
}
