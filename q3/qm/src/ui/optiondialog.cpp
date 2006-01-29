/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#pragma warning(disable:4786)

#include <qmdocument.h>
#include <qmeditwindow.h>
#include <qmfoldercombobox.h>
#include <qmfolderlistwindow.h>
#include <qmfolderwindow.h>
#include <qmjunk.h>
#include <qmlistwindow.h>
#include <qmmainwindow.h>
#include <qmmessagewindow.h>
#include <qmrecents.h>
#include <qmsecurity.h>
#include <qmtabwindow.h>

#include <qsdevicecontext.h>
#include <qsinit.h>
#include <qsras.h>
#include <qsuiutil.h>

#include <commdlg.h>
#include <tchar.h>

#include "addressbookwindow.h"
#include "conditiondialog.h"
#include "editframewindow.h"
#include "messageframewindow.h"
#include "optiondialog.h"
#include "resourceinc.h"
#include "../model/addressbook.h"
#include "../sync/syncmanager.h"
#include "../uimodel/tabmodel.h"
#include "../util/util.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * TextColorDialog
 *
 */

qm::TextColorDialog::TextColorDialog(const Data& data) :
	DefaultDialog(IDD_TEXTCOLOR),
	data_(data),
	hbrBackground_(0)
{
}

qm::TextColorDialog::~TextColorDialog()
{
}

const TextColorDialog::Data& qm::TextColorDialog::getData() const
{
	return data_;
}

INT_PTR qm::TextColorDialog::dialogProc(UINT uMsg,
										WPARAM wParam,
										LPARAM lParam)
{
	BEGIN_DIALOG_HANDLER()
		HANDLE_CTLCOLOREDIT()
		HANDLE_CTLCOLORSTATIC()
	END_DIALOG_HANDLER()
	return DefaultDialog::dialogProc(uMsg, wParam, lParam);
}

LRESULT qm::TextColorDialog::onCommand(WORD nCode,
									   WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID_RANGE(IDC_CHOOSEFOREGROUND, IDC_CHOOSELINK, onChoose)
		HANDLE_COMMAND_ID_RANGE_CODE(IDC_SYSTEMCOLOR,
			IDC_CUSTOMCOLOR, BN_CLICKED, onColor)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::TextColorDialog::onDestroy()
{
	if (hbrBackground_)
		::DeleteObject(hbrBackground_);
	
	return DefaultDialog::onDestroy();
}

LRESULT qm::TextColorDialog::onInitDialog(HWND hwndFocus,
										  LPARAM lParam)
{
	Button_SetCheck(getDlgItem(data_.bSystemColor_ ? IDC_SYSTEMCOLOR : IDC_CUSTOMCOLOR), BST_CHECKED);
	setDlgItemText(IDC_COLOR, L"Sample");
	
	if (data_.bText_) {
		setDlgItemText(IDC_QUOTE1, data_.wstrQuote_[0].get());
		setDlgItemText(IDC_QUOTE2, data_.wstrQuote_[1].get());
		setDlgItemText(IDC_LINK, L"http://example.com/");
	}
	else {
		UINT nIds[] = {
			IDC_LABELQUOTE1,
			IDC_QUOTE1,
			IDC_CHOOSEQUOTE1,
			IDC_LABELQUOTE2,
			IDC_QUOTE2,
			IDC_CHOOSEQUOTE2,
			IDC_LABELLINK,
			IDC_LINK,
			IDC_CHOOSELINK
		};
		for (int n = 0; n < countof(nIds); ++n) {
			Window wnd(getDlgItem(nIds[n]));
			wnd.showWindow(SW_HIDE);
			wnd.enableWindow(false);
		}
	}
	
	init(false);
	updateBackgroundBrush();
	updateState();
	
	return TRUE;
}

LRESULT qm::TextColorDialog::onCtlColorEdit(HDC hdc,
											HWND hwnd)
{
	return onCtlColorStatic(hdc, hwnd);
}

LRESULT qm::TextColorDialog::onCtlColorStatic(HDC hdc,
											  HWND hwnd)
{
	bool bSystemColor = Button_GetCheck(getDlgItem(IDC_SYSTEMCOLOR)) == BST_CHECKED;
	
	COLORREF crForeground;
	switch (Window(hwnd).getId()) {
	case IDC_COLOR:
		crForeground = bSystemColor ? ::GetSysColor(COLOR_WINDOWTEXT) : data_.crForeground_;
		break;
	case IDC_QUOTE1:
		crForeground = data_.crQuote_[0];
		break;
	case IDC_QUOTE2:
		crForeground = data_.crQuote_[1];
		break;
	case IDC_LINK:
		crForeground = data_.crLink_;
		break;
	default:
		return DefaultDialog::onCtlColorStatic(hdc, hwnd);
	}
	
	COLORREF crBackground = bSystemColor ? ::GetSysColor(COLOR_WINDOW) : data_.crBackground_;
	
	DeviceContext dc(hdc);
	dc.setTextColor(crForeground);
	dc.setBkColor(crBackground);
	return reinterpret_cast<LRESULT>(hbrBackground_);
}

LRESULT qm::TextColorDialog::onOk()
{
	data_.bSystemColor_ = Button_GetCheck(getDlgItem(IDC_SYSTEMCOLOR)) == BST_CHECKED;
	
	if (data_.bText_) {
		data_.wstrQuote_[0] = getDlgItemText(IDC_QUOTE1);
		data_.wstrQuote_[1] = getDlgItemText(IDC_QUOTE2);
	}
	
	return DefaultDialog::onOk();
}

LRESULT qm::TextColorDialog::onChoose(UINT nId)
{
	COLORREF* pcrs[] = {
		&data_.crForeground_,
		&data_.crBackground_,
		&data_.crQuote_[0],
		&data_.crQuote_[1],
		&data_.crLink_
	};
	COLORREF& cr = *pcrs[nId - IDC_CHOOSEFOREGROUND];
	
	COLORREF crCustom[16];
	CHOOSECOLOR cc = {
		sizeof(cc),
		getHandle(),
		0,
		cr,
		crCustom,
		CC_ANYCOLOR | CC_RGBINIT,
		0,
		0,
		0
	};
	if (::ChooseColor(&cc)) {
		cr = cc.rgbResult;
		
		if (nId == IDC_CHOOSEBACKGROUND)
			updateBackgroundBrush();
		
		invalidate();
	}
	
	return 0;
	
}

LRESULT qm::TextColorDialog::onColor(UINT nId)
{
	updateBackgroundBrush();
	updateState();
	return 0;
}

void qm::TextColorDialog::updateState()
{
	bool bEnable = Button_GetCheck(getDlgItem(IDC_CUSTOMCOLOR)) == BST_CHECKED;
	Window(getDlgItem(IDC_CHOOSEFOREGROUND)).enableWindow(bEnable);
	Window(getDlgItem(IDC_CHOOSEBACKGROUND)).enableWindow(bEnable);
}

void qm::TextColorDialog::updateBackgroundBrush()
{
	if (hbrBackground_)
		::DeleteObject(hbrBackground_);
	
	bool bSystemColor = Button_GetCheck(getDlgItem(IDC_SYSTEMCOLOR)) == BST_CHECKED;
	COLORREF crBackground = bSystemColor ? ::GetSysColor(COLOR_WINDOW) : data_.crBackground_;
	hbrBackground_ = ::CreateSolidBrush(crBackground);
	
	invalidate();
}


/****************************************************************************
 *
 * TextColorDialog::Data
 *
 */

qm::TextColorDialog::Data::Data(Profile* pProfile,
								const WCHAR* pwszSection,
								bool bText) :
	bText_(bText)
{
	bSystemColor_ = pProfile->getInt(pwszSection, L"UseSystemColor", 1) != 0;
	
	struct
	{
		const WCHAR* pwszKey_;
		const WCHAR* pwszDefault_;
		COLORREF* pcr_;
	} colors[] = {
		{ L"ForegroundColor",	L"000000",	&crForeground_	},
		{ L"BackgroundColor",	L"ffffff",	&crBackground_	},
		{ L"QuoteColor1",		L"008000",	&crQuote_[0]	},
		{ L"QuoteColor2",		L"000080",	&crQuote_[1]	},
		{ L"LinkColor",			L"0000ff",	&crLink_		}
	};
	int nCount = bText_ ? countof(colors) : 2;
	for (int n = 0; n < nCount; ++n) {
		wstring_ptr wstr(pProfile->getString(pwszSection,
			colors[n].pwszKey_, colors[n].pwszDefault_));
		Color color(wstr.get());
		if (color.getColor() != 0xffffffff)
			*colors[n].pcr_ = color.getColor();
	}
	
	if (bText_) {
		struct
		{
			const WCHAR* pwszKey_;
			const WCHAR* pwszDefault_;
			wstring_ptr* pwstrValue_;
		} strings[] = {
			{ L"Quote1",	L">",	&wstrQuote_[0]	},
			{ L"Quote2",	L"#",	&wstrQuote_[1]	}
		};
		for (int n = 0; n < countof(strings); ++n)
			*strings[n].pwstrValue_ = pProfile->getString(pwszSection,
				strings[n].pwszKey_, strings[n].pwszDefault_);
	}
}

qm::TextColorDialog::Data::Data(const Data& data) :
	bText_(data.bText_),
	bSystemColor_(data.bSystemColor_),
	crForeground_(data.crForeground_),
	crBackground_(data.crBackground_),
	crLink_(data.crLink_)
{
	if (bText_) {
		for (int n = 0; n < countof(crQuote_); ++n) {
			wstrQuote_[n] = allocWString(data.wstrQuote_[n].get());
			crQuote_[n] = data.crQuote_[n];
		}
	}
}

qm::TextColorDialog::Data::~Data()
{
}

TextColorDialog::Data& qm::TextColorDialog::Data::operator=(const Data& data)
{
	if (&data != this) {
		bText_ = data.bText_;
		bSystemColor_= data.bSystemColor_;
		crForeground_ = data.crForeground_;
		crBackground_ = data.crBackground_;
		if (bText_) {
			crLink_ = data.crLink_;
			for (int n = 0; n < countof(crQuote_); ++n) {
				wstrQuote_[n] = allocWString(data.wstrQuote_[n].get());
				crQuote_[n] = data.crQuote_[n];
			}
		}
	}
	return *this;
}

void qm::TextColorDialog::Data::save(Profile* pProfile,
									 const WCHAR* pwszSection) const
{
	pProfile->setInt(pwszSection, L"UseSystemColor", bSystemColor_);
	
	struct
	{
		const WCHAR* pwszKey_;
		COLORREF cr_;
	} colors[] = {
		{ L"ForegroundColor",	crForeground_	},
		{ L"BackgroundColor",	crBackground_	},
		{ L"QuoteColor1",		crQuote_[0]		},
		{ L"QuoteColor2",		crQuote_[1]		},
		{ L"LinkColor",			crLink_			}
	};
	int nCount = bText_ ? countof(colors) : 2;
	for (int n = 0; n < nCount; ++n) {
		wstring_ptr wstrColor(Color(colors[n].cr_).getString());
		pProfile->setString(pwszSection, colors[n].pwszKey_, wstrColor.get());
	}
	
	if (bText_) {
		struct
		{
			const WCHAR* pwszKey_;
			const WCHAR* pwszValue_;
		} strings[] = {
			{ L"Quote1",	wstrQuote_[0].get()	},
			{ L"Quote2",	wstrQuote_[1].get()	}
		};
		for (int n = 0; n < countof(strings); ++n)
			pProfile->setString(pwszSection, strings[n].pwszKey_, strings[n].pwszValue_);
	}
}


/****************************************************************************
 *
 * OptionDialog
 *
 */

qm::OptionDialog::OptionDialog(Document* pDocument,
							   GoRound* pGoRound,
							   FilterManager* pFilterManager,
							   ColorManager* pColorManager,
							   SyncFilterManager* pSyncFilterManager,
							   AutoPilotManager* pAutoPilotManager,
							   MainWindow* pMainWindow,
							   FolderWindow* pFolderWindow,
							   FolderComboBox* pFolderComboBox,
							   ListWindow* pListWindow,
							   FolderListWindow* pFolderListWindow,
							   MessageWindow* pPreviewWindow,
							   MessageFrameWindowManager* pMessageFrameWindowManager,
							   EditFrameWindowManager* pEditFrameWindowManager,
#ifdef QMTABWINDOW
							   TabWindow* pTabWindow,
#endif
							   AddressBookFrameWindowManager* pAddressBookFrameWindowManager,
							   Profile* pProfile,
							   Panel panel) :
	DefaultDialog(IDD_OPTION),
	pDocument_(pDocument),
	pGoRound_(pGoRound),
	pFilterManager_(pFilterManager),
	pColorManager_(pColorManager),
	pSyncFilterManager_(pSyncFilterManager),
	pAutoPilotManager_(pAutoPilotManager),
	pMainWindow_(pMainWindow),
	pFolderWindow_(pFolderWindow),
	pFolderComboBox_(pFolderComboBox),
	pListWindow_(pListWindow),
	pFolderListWindow_(pFolderListWindow),
	pPreviewWindow_(pPreviewWindow),
	pMessageFrameWindowManager_(pMessageFrameWindowManager),
	pEditFrameWindowManager_(pEditFrameWindowManager),
#ifdef QMTABWINDOW
	pTabWindow_(pTabWindow),
#endif
	pAddressBookFrameWindowManager_(pAddressBookFrameWindowManager),
	pProfile_(pProfile),
	panel_(panel),
	pCurrentPanel_(0),
	nEnd_(-1)
{
	listPanel_.resize(MAX_PANEL);
	
	if (panel_ == PANEL_NONE) {
		int nPanel = pProfile_->getInt(L"OptionDialog", L"Panel", 0);
		if (nPanel < 0 || MAX_PANEL <= nPanel)
			nPanel = 0;
		panel_ = static_cast<Panel>(nPanel);
	}
}

qm::OptionDialog::~OptionDialog()
{
	std::for_each(listPanel_.begin(), listPanel_.end(), qs::deleter<OptionDialogPanel>());
}

int qm::OptionDialog::doModal(HWND hwndParent)
{
	ModalHandler* pModalHandler = getModalHandler();
	ModalHandlerInvoker invoker(pModalHandler, hwndParent);
	
	if (!create(hwndParent))
		return -1;
	Window(hwndParent).enableWindow(false);
	showWindow();
	
	MSG msg;
	while (nEnd_ == -1 && ::GetMessage(&msg, 0, 0, 0)) {
		if (!processDialogMessage(msg)) {
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
	}
	Window(hwndParent).enableWindow(true);
	destroyWindow();
	
	return nEnd_;
}

INT_PTR qm::OptionDialog::dialogProc(UINT uMsg,
									 WPARAM wParam,
									 LPARAM lParam)
{
	BEGIN_DIALOG_HANDLER()
		HANDLE_SIZE()
	END_DIALOG_HANDLER()
	return DefaultDialog::dialogProc(uMsg, wParam, lParam);
}

LRESULT qm::OptionDialog::onCommand(WORD nCode,
									WORD nId)
{
#ifdef _WIN32_WCE_PSPC
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID_CODE(IDC_SELECTOR, CBN_SELCHANGE, onSelectorSelChange)
	END_COMMAND_HANDLER()
#endif
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::OptionDialog::onDestroy()
{
	removeNotifyHandler(this);
	
	pProfile_->setInt(L"OptionDialog", L"Panel", panel_);
	
#ifndef _WIN32_WCE
	RECT rect;
	getWindowRect(&rect);
	pProfile_->setInt(L"OptionDialog", L"Width", rect.right - rect.left);
	pProfile_->setInt(L"OptionDialog", L"Height", rect.bottom - rect.top);
#endif
	
	return DefaultDialog::onDestroy();
}

LRESULT qm::OptionDialog::onInitDialog(HWND hwndFocus,
									   LPARAM lParam)
{
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	
	HWND hwndSelector = getDlgItem(IDC_SELECTOR);
	struct {
		Panel panel_;
		UINT nId_;
	} items[] = {
		{ PANEL_FOLDER,			IDS_PANEL_FOLDER			},
		{ PANEL_LIST,			IDS_PANEL_LIST				},
		{ PANEL_PREVIEW,		IDS_PANEL_PREVIEW			},
		{ PANEL_MESSAGE,		IDS_PANEL_MESSAGE			},
		{ PANEL_HEADER,			IDS_PANEL_HEADER			},
		{ PANEL_EDIT,			IDS_PANEL_EDIT				},
		{ PANEL_EDIT2,			IDS_PANEL_EDIT2				},
#ifdef QMTABWINDOW
		{ PANEL_TAB,			IDS_PANEL_TAB				},
#endif
		{ PANEL_ADDRESSBOOK,	IDS_PANEL_ADDRESSBOOK		},
		{ PANEL_RULES,			IDS_PANEL_RULES				},
		{ PANEL_COLORS,			IDS_PANEL_COLORS			},
		{ PANEL_GOROUND,		IDS_PANEL_GOROUND			},
		{ PANEL_SIGNATURES,		IDS_PANEL_SIGNATURES		},
		{ PANEL_FIXEDFORMTEXTS,	IDS_PANEL_FIXEDFORMTEXTS	},
		{ PANEL_FILTERS,		IDS_PANEL_FILTERS			},
		{ PANEL_SYNCFILTERS,	IDS_PANEL_SYNCFILTERS		},
		{ PANEL_AUTOPILOT,		IDS_PANEL_AUTOPILOT			},
#ifndef _WIN32_WCE
		{ PANEL_SEARCH,			IDS_PANEL_SEARCH			},
		{ PANEL_JUNK,			IDS_PANEL_JUNK				},
#endif
		{ PANEL_SECURITY,		IDS_PANEL_SECURITY			},
		{ PANEL_CONFIRM,		IDS_PANEL_CONFIRM			},
		{ PANEL_MISC,			IDS_PANEL_MISC				},
		{ PANEL_MISC2,			IDS_PANEL_MISC2				}
	};
	for (int n = 0; n < countof(items); ++n) {
		wstring_ptr wstrName(loadString(hInst, items[n].nId_));
		W2T(wstrName.get(), ptszName);
#ifndef _WIN32_WCE_PSPC
		TVINSERTSTRUCT tvis = {
			TVI_ROOT,
			TVI_LAST,
			{
				TVIF_TEXT | TVIF_PARAM,
				0,
				0,
				0,
				const_cast<LPTSTR>(ptszName),
				0,
				0,
				0,
				0,
				items[n].panel_
			}
		};
		TreeView_InsertItem(hwndSelector, &tvis);
#else
		int nItem = ComboBox_AddString(hwndSelector, ptszName);
		ComboBox_SetItemData(hwndSelector, nItem, items[n].panel_);
#endif
	}
	
	setCurrentPanel(panel_, true);
	
#ifdef _WIN32_WCE
	RECT rectWorkArea;
	::SystemParametersInfo(SPI_GETWORKAREA, 0, &rectWorkArea, 0);
	int nWidth = rectWorkArea.right - rectWorkArea.left;
	int nHeight = rectWorkArea.bottom - rectWorkArea.top;
	setWindowPos(0, 0, 0, nWidth, nHeight, SWP_NOZORDER | SWP_NOACTIVATE);
#else
	int nWidth = pProfile_->getInt(L"OptionDialog", L"Width", 620);
	int nHeight = pProfile_->getInt(L"OptionDialog", L"Height", 450);
	setWindowPos(0, 0, 0, nWidth, nHeight,
		SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
#endif
	
	addNotifyHandler(this);
	
	init(false);
	
	return TRUE;
}

LRESULT qm::OptionDialog::onOk()
{
	OptionDialogContext context;
	for (PanelList::const_iterator it = listPanel_.begin(); it != listPanel_.end(); ++it) {
		OptionDialogPanel* pPanel = *it;
		if (pPanel) {
			if (!pPanel->save(&context)) {
				// TODO
			}
		}
	}
	
	unsigned int nFlags = context.getFlags();
	if (nFlags & OptionDialogContext::FLAG_RELOADMAIN)
		pMainWindow_->reloadProfiles();
	if (nFlags & OptionDialogContext::FLAG_RELOADFOLDER) {
		pFolderWindow_->reloadProfiles();
		pFolderComboBox_->reloadProfiles();
	}
	if (nFlags & OptionDialogContext::FLAG_RELOADLIST) {
		pListWindow_->reloadProfiles();
		pFolderListWindow_->reloadProfiles();
	}
	if (nFlags & OptionDialogContext::FLAG_RELOADMESSAGE)
		pMessageFrameWindowManager_->reloadProfiles();
	if (nFlags & OptionDialogContext::FLAG_RELOADPREVIEW)
		pPreviewWindow_->reloadProfiles();
	if (nFlags & OptionDialogContext::FLAG_RELOADEDIT)
		pEditFrameWindowManager_->reloadProfiles();
#ifdef QMTABWINDOW
	if (nFlags & OptionDialogContext::FLAG_RELOADTAB)
		pTabWindow_->reloadProfiles();
#endif
	if (nFlags & OptionDialogContext::FLAG_RELOADADDRESSBOOK) {
		pDocument_->getAddressBook()->reloadProfiles();
		pAddressBookFrameWindowManager_->reloadProfiles();
	}
	if (nFlags & OptionDialogContext::FLAG_RELOADSECURITY)
		pDocument_->getSecurity()->reload();
	
	if (nFlags & OptionDialogContext::FLAG_LAYOUTMAINWINDOW)
		pMainWindow_->layout();
	if (nFlags & OptionDialogContext::FLAG_LAYOUTMESSAGEWINDOW)
		pMessageFrameWindowManager_->layout();
	if (nFlags & OptionDialogContext::FLAG_LAYOUTEDITWINDOW)
		pEditFrameWindowManager_->layout();
	
	nEnd_ = IDOK;
	
	return 0;
}

LRESULT qm::OptionDialog::onCancel()
{
	nEnd_ = IDCANCEL;
	return 0;
}

LRESULT qm::OptionDialog::onNotify(NMHDR* pnmhdr,
								   bool* pbHandled)
{
#ifndef _WIN32_WCE_PSPC
	BEGIN_NOTIFY_HANDLER()
		HANDLE_NOTIFY(TVN_SELCHANGED, IDC_SELECTOR, onSelectorSelChanged)
	END_NOTIFY_HANDLER()
#endif
	return NotifyHandler::onNotify(pnmhdr, pbHandled);
}

LRESULT qm::OptionDialog::onSize(UINT nFlags,
								 int cx,
								 int cy)
{
	layout();
	return DefaultDialog::onSize(nFlags, cx, cy);
}

#ifndef _WIN32_WCE_PSPC
LRESULT qm::OptionDialog::onSelectorSelChanged(NMHDR* pnmhdr,
											   bool* pbHandled)
{
	NMTREEVIEW* pnmtv = reinterpret_cast<NMTREEVIEW*>(pnmhdr);
	
	if (pnmtv->itemNew.hItem && pnmtv->itemNew.state & TVIS_SELECTED) {
		Panel panel = static_cast<Panel>(pnmtv->itemNew.lParam);
		setCurrentPanel(panel, false);
	}
	
	return 1;
}
#else
LRESULT qm::OptionDialog::onSelectorSelChange()
{
	HWND hwndSelector = getDlgItem(IDC_SELECTOR);
	int nItem = ComboBox_GetCurSel(hwndSelector);
	Panel panel = static_cast<Panel>(ComboBox_GetItemData(hwndSelector, nItem));
	setCurrentPanel(panel, false);
	
	return 0;
}
#endif

void qm::OptionDialog::layout()
{
	RECT rect;
	getClientRect(&rect);
	
	Window selector(getDlgItem(IDC_SELECTOR));
#ifndef _WIN32_WCE_PSPC
#ifndef _WIN32_WCE
	Window ok(getDlgItem(IDOK));
	Window cancel(getDlgItem(IDCANCEL));
#endif
	
	RECT rectSelector;
	selector.getWindowRect(&rectSelector);
	int nSelectorWidth = rectSelector.right - rectSelector.left;
	
#ifndef _WIN32_WCE
	RECT rectButton;
	ok.getWindowRect(&rectButton);
	int nButtonWidth = rectButton.right - rectButton.left;
	int nButtonHeight = rectButton.bottom - rectButton.top;
#else
	int nButtonHeight = -5;
#endif
	
	HDWP hdwp = beginDeferWindowPos(5);
	hdwp = selector.deferWindowPos(hdwp, 0, 5, 5, nSelectorWidth,
		rect.bottom - 15 - nButtonHeight, SWP_NOACTIVATE | SWP_NOZORDER);
	hdwp = Window(pCurrentPanel_->getWindow()).deferWindowPos(hdwp, 0,
		nSelectorWidth + 5, 0, rect.right - nSelectorWidth - 5,
		rect.bottom - 5 - nButtonHeight, SWP_NOACTIVATE | SWP_NOZORDER);
#ifndef _WIN32_WCE
	hdwp = ok.deferWindowPos(hdwp, 0, rect.right - (nButtonWidth + 5)*2 - 15,
		rect.bottom - nButtonHeight - 5, nButtonWidth, nButtonHeight,
		SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
	hdwp = cancel.deferWindowPos(hdwp, 0, rect.right - (nButtonWidth + 5) - 15,
		rect.bottom - nButtonHeight - 5, nButtonWidth, nButtonHeight,
		SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
	hdwp = Window(getDlgItem(IDC_SIZEGRIP)).deferWindowPos(hdwp, 0,
		rect.right - rect.left - 13, rect.bottom - rect.top - 12,
		13, 12, SWP_NOZORDER | SWP_NOACTIVATE);
#endif
	
	endDeferWindowPos(hdwp);
#else
	RECT rectSelector;
	selector.getWindowRect(&rectSelector);
	screenToClient(&rectSelector);
	
	RECT rectButton;
	Window(getDlgItem(IDOK)).getWindowRect(&rectButton);
	screenToClient(&rectButton);
	
	Window(pCurrentPanel_->getWindow()).setWindowPos(0,
		0, rectSelector.bottom, rect.right - rect.left,
		rectButton.top - rectSelector.bottom, SWP_NOACTIVATE | SWP_NOZORDER);
#endif
}

void qm::OptionDialog::setCurrentPanel(Panel panel,
									   bool bForce)
{
	if (!bForce && panel == panel_)
		return;
	
#define BEGIN_PANEL() \
	switch (panel) {

#define END_PANEL() \
	default: \
		assert(false); \
		break; \
	}

#define PANEL1(name, dialog, arg1) \
	case name: \
		{ \
			std::auto_ptr<dialog##Dialog> pDialog(new dialog##Dialog(arg1)); \
			pDialog->Dialog::create(getHandle()); \
			listPanel_[panel] = pDialog.release(); \
		} \
		break
	
#define PANEL2(name, dialog, arg1, arg2) \
	case name: \
		{ \
			std::auto_ptr<dialog##Dialog> pDialog(new dialog##Dialog(arg1, arg2)); \
			pDialog->Dialog::create(getHandle()); \
			listPanel_[panel] = pDialog.release(); \
		} \
		break
	
#define PANEL3(name, dialog, arg1, arg2, arg3) \
	case name: \
		{ \
			std::auto_ptr<dialog##Dialog> pDialog(new dialog##Dialog(arg1, arg2, arg3)); \
			pDialog->Dialog::create(getHandle()); \
			listPanel_[panel] = pDialog.release(); \
		} \
		break
	
#define PANEL4(name, dialog, arg1, arg2, arg3, arg4) \
	case name: \
		{ \
			std::auto_ptr<dialog##Dialog> pDialog(new dialog##Dialog(arg1, arg2, arg3, arg4)); \
			pDialog->Dialog::create(getHandle()); \
			listPanel_[panel] = pDialog.release(); \
		} \
		break
	
	if (!listPanel_[panel]) {
		BEGIN_PANEL()
			PANEL3(PANEL_FOLDER, OptionFolder, pFolderWindow_, pFolderComboBox_, pProfile_);
			PANEL3(PANEL_LIST, OptionList, pListWindow_, pFolderListWindow_, pProfile_);
			PANEL2(PANEL_PREVIEW, OptionPreview, pPreviewWindow_, pProfile_);
			PANEL2(PANEL_MESSAGE, OptionMessage, pMessageFrameWindowManager_, pProfile_);
			PANEL3(PANEL_HEADER, OptionHeader, pMessageFrameWindowManager_, pPreviewWindow_, pProfile_);
			PANEL2(PANEL_EDIT, OptionEdit, pEditFrameWindowManager_, pProfile_);
			PANEL2(PANEL_EDIT2, OptionEdit2, pEditFrameWindowManager_, pProfile_);
#ifdef QMTABWINDOW
			PANEL2(PANEL_TAB, OptionTab, pTabWindow_, pProfile_);
#endif
			PANEL3(PANEL_ADDRESSBOOK, OptionAddressBook, pDocument_->getAddressBook(), pAddressBookFrameWindowManager_, pProfile_);
			PANEL3(PANEL_RULES, RuleSets, pDocument_->getRuleManager(), pDocument_, pProfile_);
			PANEL3(PANEL_COLORS, ColorSets, pColorManager_, pDocument_, pProfile_);
			PANEL4(PANEL_GOROUND, GoRound, pGoRound_, pDocument_, pSyncFilterManager_, pProfile_);
			PANEL3(PANEL_SIGNATURES, Signatures, pDocument_->getSignatureManager(), pDocument_, pProfile_);
			PANEL2(PANEL_FIXEDFORMTEXTS, FixedFormTexts, pDocument_->getFixedFormTextManager(), pProfile_);
			PANEL1(PANEL_FILTERS, Filters, pFilterManager_);
			PANEL2(PANEL_SYNCFILTERS, SyncFilterSets, pSyncFilterManager_, pProfile_);
			PANEL4(PANEL_AUTOPILOT, AutoPilot, pAutoPilotManager_, pGoRound_, pDocument_->getRecents(), pProfile_);
#ifndef _WIN32_WCE
			PANEL1(PANEL_SEARCH, OptionSearch, pProfile_);
			PANEL1(PANEL_JUNK, OptionJunk, pDocument_->getJunkFilter());
#endif
			PANEL2(PANEL_SECURITY, OptionSecurity, pDocument_->getSecurity(), pProfile_);
			PANEL1(PANEL_CONFIRM, OptionConfirm, pProfile_);
			PANEL1(PANEL_MISC, OptionMisc, pProfile_);
			PANEL1(PANEL_MISC2, OptionMisc2, pProfile_);
		END_PANEL()
	}
	
	HWND hwndSelector = getDlgItem(IDC_SELECTOR);
#ifndef _WIN32_WCE_PSPC
	HTREEITEM hItem = TreeView_GetRoot(hwndSelector);
	while (hItem) {
		TVITEM item = {
			TVIF_HANDLE | TVIF_PARAM,
			hItem
		};
		TreeView_GetItem(hwndSelector, &item);
		if (item.lParam == panel) {
			TreeView_SelectItem(hwndSelector, hItem);
			break;
		}
		
		hItem = TreeView_GetNextSibling(hwndSelector, hItem);
	}
#else
	int nCount = ComboBox_GetCount(hwndSelector);
	for (int n = 0; n < nCount; ++n) {
		Panel p = static_cast<Panel>(ComboBox_GetItemData(hwndSelector, n));
		if (p == panel) {
			ComboBox_SetCurSel(hwndSelector, n);
			break;
		}
	}
#endif
	
	if (pCurrentPanel_)
		Window(pCurrentPanel_->getWindow()).showWindow(SW_HIDE);
	panel_ = panel;
	pCurrentPanel_ = listPanel_[panel_];
	Window(pCurrentPanel_->getWindow()).showWindow();
	layout();
}

bool qm::OptionDialog::processDialogMessage(const MSG& msg)
{
	if (msg.message == WM_KEYDOWN) {
		Window wndFocus(Window::getFocus());
		int nCode = 0;
		if (wndFocus.getHandle())
			nCode = static_cast<int>(wndFocus.sendMessage(WM_GETDLGCODE));
			
		UINT nKey = static_cast<UINT>(msg.wParam);
		switch (nKey) {
		case VK_TAB:
			if (!(nCode & DLGC_WANTTAB)) {
				bool bShift = ::GetKeyState(VK_SHIFT) < 0;
				processTab(bShift);
				return true;
			}
			break;
		case VK_RETURN:
			if (!(nCode & DLGC_WANTALLKEYS)) {
				DWORD dwDefId = 0;
				if (pCurrentPanel_)
					dwDefId = static_cast<DWORD>(Window(pCurrentPanel_->getWindow()).sendMessage(DM_GETDEFID));
				if (HIWORD(dwDefId) == DC_HASDEFID && LOWORD(dwDefId) != IDOK && LOWORD(dwDefId) != IDCANCEL)
					Window(pCurrentPanel_->getWindow()).postMessage(WM_COMMAND, MAKEWPARAM(LOWORD(dwDefId), 0), 0);
				else
					postMessage(WM_COMMAND, MAKEWPARAM(IDOK, 0), 0);
				return true;
			}
			break;
		case VK_ESCAPE:
			postMessage(WM_COMMAND, MAKEWPARAM(IDCANCEL, 0), 0);
			return true;
		case VK_LEFT:
		case VK_RIGHT:
		case VK_UP:
		case VK_DOWN:
			if (!(nCode & DLGC_WANTARROWS) && nCode & DLGC_BUTTON) {
				processTab(nKey == VK_LEFT || nKey == VK_UP);
				return true;
			}
			break;
		}
	}
	else if (msg.message == WM_SYSKEYDOWN) {
		UINT nKey = static_cast<UINT>(msg.wParam);
		if ((('A' <= nKey && nKey <= 'Z') || ('0' <= nKey && nKey <= '9')) &&
			::GetKeyState(VK_MENU) < 0) {
			processMnemonic(static_cast<char>(nKey));
			return true;
		}
	}
	
	return false;
}

void qm::OptionDialog::processTab(bool bShift)
{
	HWND hwnd = getFocus();
	if (!hwnd)
		return;
	
	UINT nId = Window(hwnd).getId();
	if (nId == IDC_SELECTOR) {
		if (bShift) {
#ifndef _WIN32_WCE
			hwnd = getDlgItem(IDCANCEL);
#else
			HWND hwndChild = Window(pCurrentPanel_->getWindow()).getWindow(GW_CHILD);
			if (hwndChild)
				hwndChild = Window(hwndChild).getWindow(GW_HWNDLAST);
			while (hwndChild) {
				if (isTabStop(hwndChild)) {
					hwnd = hwndChild;
					break;
				}
				hwndChild = Window(hwndChild).getWindow(GW_HWNDPREV);
			}
#endif
		}
		else {
			HWND hwndChild = Window(pCurrentPanel_->getWindow()).getWindow(GW_CHILD);
			while (hwndChild) {
				if (isTabStop(hwndChild)) {
					hwnd = hwndChild;
					break;
				}
				hwndChild = Window(hwndChild).getWindow(GW_HWNDNEXT);
			}
		}
	}
#ifndef _WIN32_WCE
	else if (nId == IDOK) {
		if (bShift) {
			HWND hwndChild = Window(pCurrentPanel_->getWindow()).getWindow(GW_CHILD);
			if (hwndChild)
				hwndChild = Window(hwndChild).getWindow(GW_HWNDLAST);
			while (hwndChild) {
				if (isTabStop(hwndChild)) {
					hwnd = hwndChild;
					break;
				}
				hwndChild = Window(hwndChild).getWindow(GW_HWNDPREV);
			}
		}
		else {
			hwnd = getDlgItem(IDCANCEL);
		}
	}
	else if (nId == IDCANCEL) {
		hwnd = getDlgItem(bShift ? IDOK : IDC_SELECTOR);
	}
#endif
	else if (pCurrentPanel_ && Window(hwnd).getParent() == pCurrentPanel_->getWindow()) {
		HWND hwndSibling = Window(hwnd).getWindow(bShift ? GW_HWNDPREV : GW_HWNDNEXT);
		while (hwndSibling) {
			if (isTabStop(hwndSibling)) {
				hwnd = hwndSibling;
				break;
			}
			hwndSibling = Window(hwndSibling).getWindow(bShift ? GW_HWNDPREV : GW_HWNDNEXT);
		}
		if (!hwndSibling) {
#ifndef _WIN32_WCE
			hwnd = getDlgItem(bShift ? IDC_SELECTOR : IDOK);
#else
			hwnd = getDlgItem(IDC_SELECTOR);
#endif
		}
	}
	
	setFocus(hwnd);
}

bool qm::OptionDialog::isTabStop(HWND hwnd) const
{
	int nStyle = Window(hwnd).getStyle();
	return nStyle & WS_TABSTOP && nStyle & WS_VISIBLE && !(nStyle & WS_DISABLED);
}

void qm::OptionDialog::processMnemonic(char c)
{
	HWND hwnd = Window(pCurrentPanel_->getWindow()).getWindow(GW_CHILD);
	while (hwnd) {
		if (getMnemonic(hwnd) == c)
			break;
		hwnd = Window(hwnd).getWindow(GW_HWNDNEXT);
	}
	if (hwnd) {
		if (Window(hwnd).sendMessage(WM_GETDLGCODE) & DLGC_STATIC)
			hwnd = Window(hwnd).getWindow(GW_HWNDNEXT);
		if (hwnd) {
			int nCode = static_cast<int>(Window(hwnd).sendMessage(WM_GETDLGCODE));
			if (nCode & DLGC_DEFPUSHBUTTON || nCode & DLGC_UNDEFPUSHBUTTON)
				Window(pCurrentPanel_->getWindow()).postMessage(WM_COMMAND,
					MAKEWPARAM(Window(hwnd).getId(), BN_CLICKED),
					reinterpret_cast<LPARAM>(hwnd));
			else if (nCode & DLGC_BUTTON)
				Window(hwnd).sendMessage(BM_CLICK);
			else
				setFocus(hwnd);
		}
	}
}

void qm::OptionDialog::setFocus(HWND hwnd)
{
	Window wnd(hwnd);
	
	int nCode = static_cast<int>(wnd.sendMessage(WM_GETDLGCODE));
	if (nCode & DLGC_DEFPUSHBUTTON || nCode & DLGC_UNDEFPUSHBUTTON) {
		Window parent(wnd.getParent());
		parent.sendMessage(DM_SETDEFID, wnd.getId());
		
		if (parent.getHandle() == getHandle())
			clearDefaultButton(pCurrentPanel_->getWindow());
		else
			clearDefaultButton(getHandle());
	}
	else {
		sendMessage(DM_SETDEFID, IDOK);
		clearDefaultButton(pCurrentPanel_->getWindow());
	}
	
	wnd.setFocus();
	
	if (nCode & DLGC_HASSETSEL)
		wnd.sendMessage(EM_SETSEL, 0, -1);
}

WCHAR qm::OptionDialog::getMnemonic(HWND hwnd)
{
	wstring_ptr wstrText(Window(hwnd).getWindowText());
	const WCHAR* p = wstrText.get();
	while (true) {
		p = wcschr(p, L'&');
		if (!p)
			return L'\0';
		if (*(p + 1) != L'&')
			return getMnemonic(*(p + 1));
		++p;
	}
	return L'\0';
}

WCHAR qm::OptionDialog::getMnemonic(WCHAR c)
{
	return (L'a' <= c && c <= L'z') ? c - L'a' + L'A' : c;
}

void qm::OptionDialog::clearDefaultButton(HWND hwnd)
{
	Window wnd(hwnd);
	DWORD dwDefId = static_cast<DWORD>(wnd.sendMessage(DM_GETDEFID));
	if (HIWORD(dwDefId) == DC_HASDEFID) {
		wnd.sendDlgItemMessage(LOWORD(dwDefId), BM_SETSTYLE, BS_PUSHBUTTON, TRUE);
		wnd.sendMessage(DM_SETDEFID, IDOK);
	}
}


/****************************************************************************
 *
 * OptionDialogPanel
 *
 */

qm::OptionDialogPanel::~OptionDialogPanel()
{
}


/****************************************************************************
 *
 * OptionDialogContext
 *
 */

qm::OptionDialogContext::OptionDialogContext() :
	nFlags_(0)
{
}

qm::OptionDialogContext::~OptionDialogContext()
{
}

unsigned int qm::OptionDialogContext::getFlags() const
{
	return nFlags_;
}

void qm::OptionDialogContext::setFlags(unsigned int nFlags)
{
	setFlags(nFlags, nFlags);
}

void qm::OptionDialogContext::setFlags(unsigned int nFlags,
									   unsigned int nMask)
{
	nFlags_ = (nFlags_ & ~nMask) | (nFlags & nMask);
}


/****************************************************************************
 *
 * OptionDialogManager
 *
 */

qm::OptionDialogManager::OptionDialogManager(Document* pDocument,
											 GoRound* pGoRound,
											 FilterManager* pFilterManager,
											 ColorManager* pColorManager,
											 SyncManager* pSyncManager,
											 AutoPilotManager* pAutoPilotManager,
											 Profile* pProfile) :
	pDocument_(pDocument),
	pGoRound_(pGoRound),
	pFilterManager_(pFilterManager),
	pColorManager_(pColorManager),
	pSyncManager_(pSyncManager),
	pAutoPilotManager_(pAutoPilotManager),
	pProfile_(pProfile),
	pMainWindow_(0),
	pFolderWindow_(0),
	pFolderComboBox_(0),
	pListWindow_(0),
	pFolderListWindow_(0),
	pPreviewWindow_(0),
	pMessageFrameWindowManager_(0),
	pEditFrameWindowManager_(0),
#ifdef QMTABWINDOW
	pTabWindow_(0),
#endif
	pAddressBookFrameWindowManager_(0)
{
}

qm::OptionDialogManager::~OptionDialogManager()
{
}

void qm::OptionDialogManager::initUIs(MainWindow* pMainWindow,
									  FolderWindow* pFolderWindow,
									  FolderComboBox* pFolderComboBox,
									  ListWindow* pListWindow,
									  FolderListWindow* pFolderListWindow,
									  MessageWindow* pPreviewWindow,
									  MessageFrameWindowManager* pMessageFrameWindowManager,
									  EditFrameWindowManager* pEditFrameWindowManager,
#ifdef QMTABWINDOW
									  TabWindow* pTabWindow,
#endif
									  AddressBookFrameWindowManager* pAddressBookFrameWindowManager)
{
	pMainWindow_ = pMainWindow;
	pFolderWindow_ = pFolderWindow;
	pFolderComboBox_ = pFolderComboBox;
	pListWindow_ = pListWindow;
	pFolderListWindow_ = pFolderListWindow;
	pPreviewWindow_ = pPreviewWindow;
	pMessageFrameWindowManager_ = pMessageFrameWindowManager;
	pEditFrameWindowManager_ = pEditFrameWindowManager;
#ifdef QMTABWINDOW
	pTabWindow_ = pTabWindow;
#endif
	pAddressBookFrameWindowManager_ = pAddressBookFrameWindowManager;
}

int qm::OptionDialogManager::showDialog(HWND hwndParent,
										OptionDialog::Panel panel) const
{
	assert(pDocument_);
	assert(pGoRound_);
	assert(pFilterManager_);
	assert(pColorManager_);
	assert(pSyncManager_);
	assert(pAutoPilotManager_);
	assert(pProfile_);
	assert(pMainWindow_);
	assert(pFolderWindow_);
	assert(pFolderComboBox_);
	assert(pListWindow_);
	assert(pFolderListWindow_);
	assert(pPreviewWindow_);
	assert(pMessageFrameWindowManager_);
	assert(pEditFrameWindowManager_);
#ifdef QMTABWINDOW
	assert(pTabWindow_);
#endif
	assert(pAddressBookFrameWindowManager_);
	
	OptionDialog dialog(pDocument_, pGoRound_, pFilterManager_,
		pColorManager_, pSyncManager_->getSyncFilterManager(),
		pAutoPilotManager_, pMainWindow_, pFolderWindow_, pFolderComboBox_,
		pListWindow_, pFolderListWindow_, pPreviewWindow_,
		pMessageFrameWindowManager_, pEditFrameWindowManager_,
#ifdef QMTABWINDOW
		pTabWindow_,
#endif
		pAddressBookFrameWindowManager_, pProfile_, panel);
	return dialog.doModal(hwndParent);
}

bool qm::OptionDialogManager::canShowDialog() const
{
	return !pSyncManager_->isSyncing();
}


/****************************************************************************
 *
 * OptionAddressBookDialog
 *
 */

OptionAddressBookDialog::External qm::OptionAddressBookDialog::externals__[] = {
#ifndef _WIN32_WCE
	{ IDC_WAB,				L"WAB"				},
	{ IDC_OUTLOOK,			L"Outlook"			}
#else
	{ IDC_POCKETOUTLOOK,	L"PocketOutlook"	}
#endif
};

qm::OptionAddressBookDialog::OptionAddressBookDialog(AddressBook* pAddressBook,
													 AddressBookFrameWindowManager* pAddressBookFrameWindowManager,
													 Profile* pProfile) :
	DefaultDialog(IDD_OPTIONADDRESSBOOK),
	pAddressBook_(pAddressBook),
	pAddressBookFrameWindowManager_(pAddressBookFrameWindowManager),
	pProfile_(pProfile)
{
	qs::UIUtil::getLogFontFromProfile(pProfile_, L"AddressBookListWindow", false, &lf_);
}

qm::OptionAddressBookDialog::~OptionAddressBookDialog()
{
}

LRESULT qm::OptionAddressBookDialog::onCommand(WORD nCode,
											   WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_FONT, onFont)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::OptionAddressBookDialog::onInitDialog(HWND hwndFocus,
												  LPARAM lParam)
{
	wstring_ptr wstrExternals(pProfile_->getString(L"AddressBook",
		L"Externals", L"WAB Outlook PocketOutlook"));
	const WCHAR* p = wcstok(wstrExternals.get(), L" ");
	while (p) {
		for (int n = 0; n < countof(externals__); ++n) {
			if (wcscmp(p, externals__[n].pwszName_) == 0) {
				Button_SetCheck(getDlgItem(externals__[n].nId_), BST_CHECKED);
				break;
			}
		}
		p = wcstok(0, L" ");
	}
	
	if (pProfile_->getInt(L"AddressBook", L"AddressOnly", 0))
		Button_SetCheck(getDlgItem(IDC_ADDRESSONLY), BST_CHECKED);
	
	return FALSE;
}

bool qm::OptionAddressBookDialog::save(OptionDialogContext* pContext)
{
	StringBuffer<WSTRING> buf;
	for (int n = 0; n < countof(externals__); ++n) {
		if (Button_GetCheck(getDlgItem(externals__[n].nId_)) == BST_CHECKED) {
			if (buf.getLength() != 0)
				buf.append(L" ");
			buf.append(externals__[n].pwszName_);
		}
	}
	pProfile_->setString(L"AddressBook", L"Externals", buf.getCharArray());
	
	bool bAddressOnly = Button_GetCheck(getDlgItem(IDC_ADDRESSONLY)) == BST_CHECKED;
	pProfile_->setInt(L"AddressBook", L"AddressOnly", bAddressOnly);
	
	qs::UIUtil::setLogFontToProfile(pProfile_, L"AddressBookListWindow", lf_);
	
	pContext->setFlags(OptionDialogContext::FLAG_RELOADADDRESSBOOK);
	
	return true;
}

LRESULT qm::OptionAddressBookDialog::onFont()
{
	qs::UIUtil::browseFont(getParentPopup(), &lf_);
	return 0;
}


/****************************************************************************
 *
 * OptionConfirmDialog
 *
 */

DialogUtil::BoolProperty qm::OptionConfirmDialog::boolProperties__[] = {
	{ L"ConfirmDeleteMessage",			IDC_CONFIRMDELETEMESSAGE,			false	},
	{ L"ConfirmEmptyFolder",			IDC_CONFIRMEMPTYFOLDER,				true	},
	{ L"ConfirmEmptyTrash",				IDC_CONFIRMEMPTYTRASH,				true	}
};

qm::OptionConfirmDialog::OptionConfirmDialog(Profile* pProfile) :
	DefaultDialog(IDD_OPTIONCONFIRM),
	pProfile_(pProfile)
{
}

qm::OptionConfirmDialog::~OptionConfirmDialog()
{
}

LRESULT qm::OptionConfirmDialog::onInitDialog(HWND hwndFocus,
											  LPARAM lParam)
{
	DialogUtil::loadBoolProperties(this, pProfile_,
		L"Global", boolProperties__, countof(boolProperties__));
	
	return FALSE;
}

bool qm::OptionConfirmDialog::save(OptionDialogContext* pContext)
{
	DialogUtil::saveBoolProperties(this, pProfile_,
		L"Global", boolProperties__, countof(boolProperties__));
	
	return true;
}


/****************************************************************************
 *
 * OptionFolderDialog
 *
 */

DialogUtil::BoolProperty qm::OptionFolderDialog::windowBoolProperties__[] = {
	{ L"FolderShowAllCount",		IDC_FOLDERSHOWALL,		true	},
	{ L"FolderShowUnseenCount",		IDC_FOLDERSHOWUNSEEN,	true	},
	{ L"AccountShowAllCount",		IDC_ACCOUNTSHOWALL,		true	},
	{ L"AccountShowUnseenCount",	IDC_ACCOUNTSHOWUNSEEN,	true	}
};

DialogUtil::BoolProperty qm::OptionFolderDialog::comboBoxBoolProperties__[] = {
	{ L"ShowAllCount",		IDC_SHOWALL,	true	},
	{ L"ShowUnseenCount",	IDC_SHOWUNSEEN,	true	}
};

qm::OptionFolderDialog::OptionFolderDialog(FolderWindow* pFolderWindow,
										   FolderComboBox* pFolderComboBox,
										   Profile* pProfile) :
	DefaultDialog(IDD_OPTIONFOLDER),
	pFolderWindow_(pFolderWindow),
	pFolderComboBox_(pFolderComboBox),
	pProfile_(pProfile)
#ifndef _WIN32_WCE
	,
	color_(pProfile, L"FolderWindow", false)
#endif
{
	qs::UIUtil::getLogFontFromProfile(pProfile_, L"FolderWindow", false, &lfWindow_);
	qs::UIUtil::getLogFontFromProfile(pProfile_, L"FolderComboBox", false, &lfComboBox_);
}

qm::OptionFolderDialog::~OptionFolderDialog()
{
}

LRESULT qm::OptionFolderDialog::onCommand(WORD nCode,
										  WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_COMBOBOXFONT, onComboBoxFont)
#ifndef _WIN32_WCE
		HANDLE_COMMAND_ID(IDC_WINDOWCOLORS, onWindowColors)
#endif
		HANDLE_COMMAND_ID(IDC_WINDOWFONT, onWindowFont)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::OptionFolderDialog::onInitDialog(HWND hwndFocus,
											 LPARAM lParam)
{
	DialogUtil::loadBoolProperties(this, pProfile_,
		L"FolderWindow", windowBoolProperties__, countof(windowBoolProperties__));
	DialogUtil::loadBoolProperties(this, pProfile_,
		L"FolderComboBox", comboBoxBoolProperties__, countof(comboBoxBoolProperties__));
	
	return FALSE;
}

bool qm::OptionFolderDialog::save(OptionDialogContext* pContext)
{
	DialogUtil::saveBoolProperties(this, pProfile_,
		L"FolderWindow", windowBoolProperties__, countof(windowBoolProperties__));
	qs::UIUtil::setLogFontToProfile(pProfile_, L"FolderWindow", lfWindow_);
#ifndef _WIN32_WCE
	color_.save(pProfile_, L"FolderWindow");
#endif
	
	DialogUtil::saveBoolProperties(this, pProfile_,
		L"FolderComboBox", comboBoxBoolProperties__, countof(comboBoxBoolProperties__));
	qs::UIUtil::setLogFontToProfile(pProfile_, L"FolderComboBox", lfComboBox_);
	
	pContext->setFlags(OptionDialogContext::FLAG_RELOADFOLDER);
	
	return true;
}

LRESULT qm::OptionFolderDialog::onComboBoxFont()
{
	qs::UIUtil::browseFont(getParentPopup(), &lfComboBox_);
	return 0;
}

#ifndef _WIN32_WCE
LRESULT qm::OptionFolderDialog::onWindowColors()
{
	TextColorDialog dialog(color_);
	if (dialog.doModal(getParentPopup()) == IDOK)
		color_ = dialog.getData();
	return 0;
}
#endif

LRESULT qm::OptionFolderDialog::onWindowFont()
{
	qs::UIUtil::browseFont(getParentPopup(), &lfWindow_);
	return 0;
}


/****************************************************************************
 *
 * OptionHeaderDialog
 *
 */

qm::OptionHeaderDialog::OptionHeaderDialog(MessageFrameWindowManager* pMessageFrameWindowManager,
										   MessageWindow* pPreviewWindow,
										   Profile* pProfile) :
	DefaultDialog(IDD_OPTIONHEADER),
	pMessageFrameWindowManager_(pMessageFrameWindowManager),
	pPreviewWindow_(pPreviewWindow),
	pProfile_(pProfile)
{
	qs::UIUtil::getLogFontFromProfile(pProfile_, L"HeaderWindow", false, &lf_);
}

qm::OptionHeaderDialog::~OptionHeaderDialog()
{
}

LRESULT qm::OptionHeaderDialog::onCommand(WORD nCode,
										  WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_FONT, onFont)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::OptionHeaderDialog::onInitDialog(HWND hwndFocus,
											 LPARAM lParam)
{
	return FALSE;
}

bool qm::OptionHeaderDialog::save(OptionDialogContext* pContext)
{
	qs::UIUtil::setLogFontToProfile(pProfile_, L"HeaderWindow", lf_);
	
	pContext->setFlags(OptionDialogContext::FLAG_RELOADMESSAGE |
		OptionDialogContext::FLAG_RELOADPREVIEW |
		OptionDialogContext::FLAG_LAYOUTMAINWINDOW |
		OptionDialogContext::FLAG_LAYOUTMESSAGEWINDOW);
	
	return true;
}

LRESULT qm::OptionHeaderDialog::onFont()
{
	qs::UIUtil::browseFont(getParentPopup(), &lf_);
	return 0;
}


#ifndef _WIN32_WCE
/****************************************************************************
 *
 * OptionJunkDialog
 *
 */

qm::OptionJunkDialog::OptionJunkDialog(JunkFilter* pJunkFilter) :
	DefaultDialog(IDD_OPTIONJUNK),
	pJunkFilter_(pJunkFilter)
{
}

qm::OptionJunkDialog::~OptionJunkDialog()
{
}

INT_PTR qm::OptionJunkDialog::dialogProc(UINT uMsg,
										 WPARAM wParam,
										 LPARAM lParam)
{
	BEGIN_DIALOG_HANDLER()
		HANDLE_SIZE()
	END_DIALOG_HANDLER()
	return DefaultDialog::dialogProc(uMsg, wParam, lParam);
}

LRESULT qm::OptionJunkDialog::onInitDialog(HWND hwndFocus,
										   LPARAM lParam)
{
	if (pJunkFilter_) {
		unsigned int nFlags = pJunkFilter_->getFlags();
		if (nFlags & JunkFilter::FLAG_MANUALLEARN)
			Button_SetCheck(getDlgItem(IDC_MANUALLEARN), BST_CHECKED);
		if (nFlags & JunkFilter::FLAG_AUTOLEARN)
			Button_SetCheck(getDlgItem(IDC_AUTOLEARN), BST_CHECKED);
		
		float fThreshold = pJunkFilter_->getThresholdScore();
		WCHAR wszThreshold[32];
		_snwprintf(wszThreshold, countof(wszThreshold), L"%.2f", fThreshold);
		setDlgItemText(IDC_THRESHOLD, wszThreshold);
		
		unsigned int nMaxSize = pJunkFilter_->getMaxTextLength();
		setDlgItemInt(IDC_MAXSIZE, nMaxSize);
		
		wstring_ptr wstrWhiteList(pJunkFilter_->getWhiteList(L"\r\n"));
		setDlgItemText(IDC_WHITELIST, wstrWhiteList.get());
		
		wstring_ptr wstrBlackList(pJunkFilter_->getBlackList(L"\r\n"));
		setDlgItemText(IDC_BLACKLIST, wstrBlackList.get());
	}
	else {
		UINT nIds[] = {
			IDC_MANUALLEARN,
			IDC_AUTOLEARN,
			IDC_THRESHOLD,
			IDC_MAXSIZE,
			IDC_WHITELIST,
			IDC_BLACKLIST
		};
		for (int n = 0; n < countof(nIds); ++n)
			Window(getDlgItem(nIds[n])).enableWindow(false);
	}
	
	return FALSE;
}

bool qm::OptionJunkDialog::save(OptionDialogContext* pContext)
{
	if (pJunkFilter_) {
		unsigned int nFlags = 0;
		if (Button_GetCheck(getDlgItem(IDC_MANUALLEARN)) == BST_CHECKED)
			nFlags |= JunkFilter::FLAG_MANUALLEARN;
		if (Button_GetCheck(getDlgItem(IDC_AUTOLEARN)) == BST_CHECKED)
			nFlags |= JunkFilter::FLAG_AUTOLEARN;
		pJunkFilter_->setFlags(nFlags, JunkFilter::FLAG_MANUALLEARN | JunkFilter::FLAG_AUTOLEARN);
		
		wstring_ptr wstrThreshold(getDlgItemText(IDC_THRESHOLD));
		WCHAR* pEnd = 0;
		double dThreshold = wcstod(wstrThreshold.get(), &pEnd);
		if (!*pEnd)
			pJunkFilter_->setThresholdScore(static_cast<float>(dThreshold));
		
		unsigned int nMaxSize = getDlgItemInt(IDC_MAXSIZE);
		pJunkFilter_->setMaxTextLength(nMaxSize);
		
		wstring_ptr wstrWhiteList(getDlgItemText(IDC_WHITELIST));
		pJunkFilter_->setWhiteList(wstrWhiteList.get());
		
		wstring_ptr wstrBlackList(getDlgItemText(IDC_BLACKLIST));
		pJunkFilter_->setBlackList(wstrBlackList.get());
	}
	
	return true;
}

LRESULT qm::OptionJunkDialog::onSize(UINT nFlags,
									 int cx,
									 int cy)
{
	layout();
	return DefaultDialog::onSize(nFlags, cx, cy);
}

void qm::OptionJunkDialog::layout()
{
	RECT rect;
	getClientRect(&rect);
	
	for (UINT nId = IDC_WHITELIST; nId <= IDC_BLACKLIST; ++nId) {
		Window wnd(getDlgItem(nId));
		RECT r;
		wnd.getWindowRect(&r);
		screenToClient(&r);
		wnd.setWindowPos(0, 0, 0, rect.right - r.left - 5, r.bottom - r.top,
			SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);
	}
}
#endif // _WIN32_WCE


/****************************************************************************
 *
 * OptionListDialog
 *
 */

DialogUtil::BoolProperty qm::OptionListDialog::boolProperties__[] = {
#ifdef _WIN32_WCE_PSPC
	{ L"SingleClickOpen",	IDC_SINGLECLICK,	true	}
#else
	{ L"SingleClickOpen",	IDC_SINGLECLICK,	false	}
#endif
};

qm::OptionListDialog::OptionListDialog(ListWindow* pListWindow,
									   FolderListWindow* pFolderListWindow,
									   Profile* pProfile) :
	DefaultDialog(IDD_OPTIONLIST),
	pListWindow_(pListWindow),
	pFolderListWindow_(pFolderListWindow),
	pProfile_(pProfile),
	color_(pProfile, L"ListWindow", false)
{
	qs::UIUtil::getLogFontFromProfile(pProfile_, L"ListWindow", false, &lf_);
}

qm::OptionListDialog::~OptionListDialog()
{
}

LRESULT qm::OptionListDialog::onCommand(WORD nCode,
										WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_COLORS, onColors)
		HANDLE_COMMAND_ID(IDC_FONT, onFont)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::OptionListDialog::onInitDialog(HWND hwndFocus,
										   LPARAM lParam)
{
	DialogUtil::loadBoolProperties(this, pProfile_,
		L"ListWindow", boolProperties__, countof(boolProperties__));
	
	return FALSE;
}

bool qm::OptionListDialog::save(OptionDialogContext* pContext)
{
	DialogUtil::saveBoolProperties(this, pProfile_,
		L"ListWindow", boolProperties__, countof(boolProperties__));
	qs::UIUtil::setLogFontToProfile(pProfile_, L"ListWindow", lf_);
	qs::UIUtil::setLogFontToProfile(pProfile_, L"FolderListWindow", lf_);
	color_.save(pProfile_, L"ListWindow");
	color_.save(pProfile_, L"FolderListWindow");
	
	pContext->setFlags(OptionDialogContext::FLAG_RELOADLIST);
	
	return true;
}

LRESULT qm::OptionListDialog::onColors()
{
	TextColorDialog dialog(color_);
	if (dialog.doModal(getParentPopup()) == IDOK)
		color_ = dialog.getData();
	return 0;
}

LRESULT qm::OptionListDialog::onFont()
{
	qs::UIUtil::browseFont(getParentPopup(), &lf_);
	return 0;
}


/****************************************************************************
 *
 * OptionMiscDialog
 *
 */

DialogUtil::BoolProperty qm::OptionMiscDialog::boolProperties__[] = {
	{ L"NextUnseenWhenScrollEnd",		IDC_SHOWNEXTUNSEENWHENSCROLLEND,	false	},
	{ L"EmptyTrashOnExit",				IDC_EMPTYTRASHONEXIT,				false	},
	{ L"SaveMessageViewModePerFolder",	IDC_SAVEMESSAGEVIEWMODEPERFOLDER,	true	},
	{ L"SaveOnDeactivate",				IDC_SAVEONDEACTIVATE,				true	},
#ifndef _WIN32_WCE_PSPC
	{ L"HideWhenMinimized",				IDC_HIDEWHENMINIMIZED,				false	},
#endif
#ifndef _WIN32_WCE
	{ L"ShowUnseenCountOnWelcome",		IDC_SHOWUNSEENCOUNTONWELCOME,		false	},
#endif
};

qm::OptionMiscDialog::OptionMiscDialog(Profile* pProfile) :
	DefaultDialog(IDD_OPTIONMISC),
	pProfile_(pProfile)
{
}

qm::OptionMiscDialog::~OptionMiscDialog()
{
}

LRESULT qm::OptionMiscDialog::onCommand(WORD nCode,
										WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_BROWSE, onBrowse)
		HANDLE_COMMAND_ID_CODE(IDC_DEFAULTENCODING, CBN_DROPDOWN, onDropDown)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::OptionMiscDialog::onInitDialog(HWND hwndFocus,
										   LPARAM lParam)
{
	DialogUtil::loadBoolProperties(this, pProfile_,
		L"Global", boolProperties__, countof(boolProperties__));
	
	wstring_ptr wstrTempFolder(pProfile_->getString(L"Global", L"TemporaryFolder", L""));
	setDlgItemText(IDC_TEMPORARYFOLDER, wstrTempFolder.get());
	
	wstring_ptr wstrEncodings(pProfile_->getString(L"Global",
		L"Encodings", L"iso-8859-1 iso-2022-jp shift_jis euc-jp utf-8"));
	setDlgItemText(IDC_ENCODING, wstrEncodings.get());
	
	updateDefaultEncodings();
	wstring_ptr wstrDefaultEncoding(pProfile_->getString(L"Global", L"DefaultCharset", L""));
	if (*wstrDefaultEncoding.get())
		setDlgItemText(IDC_DEFAULTENCODING, wstrDefaultEncoding.get());
	else
		ComboBox_SetCurSel(getDlgItem(IDC_DEFAULTENCODING), 0);
	
	HWND hwndLog = getDlgItem(IDC_LOG);
	const WCHAR* pwszLog[] = {
		L"NONE",
		L"FATAL",
		L"ERROR",
		L"WARN",
		L"INFO",
		L"DEBUG"
	};
	for (int n = 0; n < countof(pwszLog); ++n) {
		W2T(pwszLog[n], ptszLog);
		ComboBox_AddString(hwndLog, ptszLog);
	}
	int nLog = 0;
	Init& init = Init::getInit();
	if (init.isLogEnabled())
		nLog = init.getLogLevel() + 1;
	ComboBox_SetCurSel(hwndLog, nLog);
	
	HWND hwndPlacement = getDlgItem(IDC_VIEWPLACEMENT);
	const WCHAR* pwszPlacement[] = {
		L"F|(L-P)",
		L"(F|L)-P",
		L"F|(L|P)",
		L"F-(L-P)",
		L"(F-L)|P"
	};
	for (int n = 0; n < countof(pwszPlacement); ++n) {
		W2T(pwszPlacement[n], ptszPlacement);
		ComboBox_AddString(hwndPlacement, ptszPlacement);
	}
	wstring_ptr wstrPlacement(pProfile_->getString(L"MainWindow", L"Placement", L"F|(L-P"));
	setDlgItemText(IDC_VIEWPLACEMENT, wstrPlacement.get());
	
	return FALSE;
}

bool qm::OptionMiscDialog::save(OptionDialogContext* pContext)
{
	DialogUtil::saveBoolProperties(this, pProfile_,
		L"Global", boolProperties__, countof(boolProperties__));
	
	wstring_ptr wstrTempFolder(getDlgItemText(IDC_TEMPORARYFOLDER));
	pProfile_->setString(L"Global", L"TemporaryFolder", wstrTempFolder.get());
	
	wstring_ptr wstrEncodings(getDlgItemText(IDC_ENCODING));
	pProfile_->setString(L"Global", L"Encodings", wstrEncodings.get());
	
	wstring_ptr wstrDefaultEncoding(getDlgItemText(IDC_DEFAULTENCODING));
	wstring_ptr wstrOSDefault(loadString(Application::getApplication().getResourceHandle(), IDS_OSDEFAULT));
	if (wcscmp(wstrDefaultEncoding.get(), wstrOSDefault.get()) == 0 ||
		!ConverterFactory::getInstance(wstrDefaultEncoding.get()).get())
		pProfile_->setString(L"Global", L"DefaultCharset", L"");
	else
		pProfile_->setString(L"Global", L"DefaultCharset", wstrDefaultEncoding.get());
	
	int nLog = ComboBox_GetCurSel(getDlgItem(IDC_LOG));
	pProfile_->setInt(L"Global", L"Log", nLog - 1);
	
	Init& init = Init::getInit();
	bool bLogEnabled = init.isLogEnabled();
	Logger::Level logLevel = init.getLogLevel();
	if (nLog != 0)
		init.setLogLevel(static_cast<Logger::Level>(nLog - 1));
	init.setLogEnabled(nLog != 0);
	if (bLogEnabled != init.isLogEnabled() ||
		logLevel != init.getLogLevel())
		InitThread::getInitThread().resetLogger();
	
	wstring_ptr wstrPlacement(getDlgItemText(IDC_VIEWPLACEMENT));
	pProfile_->setString(L"MainWindow", L"Placement", wstrPlacement.get());
	
	pContext->setFlags(OptionDialogContext::FLAG_RELOADMAIN);
	
	return true;
}

LRESULT qm::OptionMiscDialog::onBrowse()
{
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	wstring_ptr wstrTitle(loadString(hInst, IDS_TITLE_TEMPORARYFOLDER));
	wstring_ptr wstrPath(getDlgItemText(IDC_TEMPORARYFOLDER));
	wstrPath = qs::UIUtil::browseFolder(getParentPopup(), wstrTitle.get(), wstrPath.get());
	if (wstrPath.get())
		setDlgItemText(IDC_TEMPORARYFOLDER, wstrPath.get());
	
	return 0;
}

LRESULT qm::OptionMiscDialog::onDropDown()
{
	updateDefaultEncodings();
	return 0;
}

void qm::OptionMiscDialog::updateDefaultEncodings()
{
	HWND hwnd = getDlgItem(IDC_DEFAULTENCODING);
	
	wstring_ptr wstrCurrent(getDlgItemText(IDC_DEFAULTENCODING));
	
	ComboBox_ResetContent(hwnd);
	
	wstring_ptr wstrOSDefault(loadString(Application::getApplication().getResourceHandle(), IDS_OSDEFAULT));
	W2T(wstrOSDefault.get(), ptszOSDefault);
	ComboBox_AddString(hwnd, ptszOSDefault);
	
	wstring_ptr wstrEncodings(getDlgItemText(IDC_ENCODING));
	UIUtil::EncodingList listEncoding;
	StringListFree<UIUtil::EncodingList> free(listEncoding);
	UIUtil::parseEncodings(wstrEncodings.get(), &listEncoding);
	for (UIUtil::EncodingList::const_iterator it = listEncoding.begin(); it != listEncoding.end(); ++it) {
		W2T(*it, ptszEncoding);
		ComboBox_AddString(hwnd, ptszEncoding);
	}
	
	setDlgItemText(IDC_DEFAULTENCODING, wstrCurrent.get());
}


/****************************************************************************
 *
 * OptionMisc2Dialog
 *
 */

DialogUtil::BoolProperty qm::OptionMisc2Dialog::boolProperties__[] = {
	{ L"IncrementalSearch",	IDC_INCREMENTALSEARCH,	false	},
	{ L"Bcc",				IDC_BCC,				true	},
	{ L"NoBccForML",		IDC_NOBCCFORML,			false	},
	{ L"ForwardRfc822",		IDC_FORWARDRFC822,		false	},
	{ L"OpenAddressBook",	IDC_OPENADDRESSBOOK,	false	}
};

qm::OptionMisc2Dialog::OptionMisc2Dialog(Profile* pProfile) :
	DefaultDialog(IDD_OPTIONMISC2),
	pProfile_(pProfile)
{
}

qm::OptionMisc2Dialog::~OptionMisc2Dialog()
{
}

LRESULT qm::OptionMisc2Dialog::onInitDialog(HWND hwndFocus,
											LPARAM lParam)
{
	DialogUtil::loadBoolProperties(this, pProfile_,
		L"Global", boolProperties__, countof(boolProperties__));
	
	wstring_ptr wstrQuote(pProfile_->getString(L"Global", L"Quote", L"> "));
	setDlgItemText(IDC_QUOTE, wstrQuote.get());
	
	return FALSE;
}

bool qm::OptionMisc2Dialog::save(OptionDialogContext* pContext)
{
	DialogUtil::saveBoolProperties(this, pProfile_,
		L"Global", boolProperties__, countof(boolProperties__));
	
	wstring_ptr wstrQuote(getDlgItemText(IDC_QUOTE));
	pProfile_->setString(L"Global", L"Quote", wstrQuote.get());
	
	return true;
}


#ifndef _WIN32_WCE
/****************************************************************************
 *
 * OptionSearchDialog
 *
 */

namespace {
struct {
	UINT nId_;
	const WCHAR* pwszSearch_;
	const WCHAR* pwszUpdate_;
} engines[] = {
	{
		IDC_NAMAZU,
		L"namazu -l -a \"$condition\" \"$index\"",
		L"mknmz.bat -a -h -O \"$index\" \"$msg\""
	},
	{
		IDC_HYPERESTRAIER,
		L"estcmd search -ic $encoding -vu -sf -max -1 \"$index\" \"$condition\"",
		L"estcmd gather -cl -fm -cm -sd \"$index\" \"$msg\""
	}
};
}

qm::OptionSearchDialog::OptionSearchDialog(Profile* pProfile) :
	DefaultDialog(IDD_OPTIONSEARCH),
	pProfile_(pProfile)
{
}

qm::OptionSearchDialog::~OptionSearchDialog()
{
}

LRESULT qm::OptionSearchDialog::onCommand(WORD nCode,
										  WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID_CODE(IDC_NAMAZU, BN_CLICKED, onClicked)
		HANDLE_COMMAND_ID_CODE(IDC_HYPERESTRAIER, BN_CLICKED, onClicked)
		HANDLE_COMMAND_ID_CODE(IDC_CUSTOM, BN_CLICKED, onClicked)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::OptionSearchDialog::onInitDialog(HWND hwndFocus,
											 LPARAM lParam)
{
	wstring_ptr wstrSearch(pProfile_->getString(L"FullTextSearch", L"Command", engines[0].pwszSearch_));
	wstring_ptr wstrUpdate(pProfile_->getString(L"FullTextSearch", L"IndexCommand", engines[0].pwszUpdate_));
	
	UINT nId = IDC_CUSTOM;
	for (int n = 0; n < countof(engines); ++n) {
		if (wcscmp(wstrSearch.get(), engines[n].pwszSearch_) == 0 &&
			wcscmp(wstrUpdate.get(), engines[n].pwszUpdate_) == 0)
			nId = engines[n].nId_;
	}
	sendDlgItemMessage(nId, BM_SETCHECK, BST_CHECKED);
	
	setDlgItemText(IDC_SEARCH, wstrSearch.get());
	setDlgItemText(IDC_UPDATE, wstrUpdate.get());
	
	updateState();
	
	return FALSE;
}

bool qm::OptionSearchDialog::save(OptionDialogContext* pContext)
{
	bool bCustom = true;
	for (int n = 0; n < countof(engines) && bCustom; ++n) {
		if (sendDlgItemMessage(engines[n].nId_, BM_GETCHECK) == BST_CHECKED) {
			pProfile_->setString(L"FullTextSearch", L"Command", engines[n].pwszSearch_);
			pProfile_->setString(L"FullTextSearch", L"IndexCommand", engines[n].pwszUpdate_);
			bCustom = false;
		}
	}
	if (bCustom) {
		wstring_ptr wstrSearch(getDlgItemText(IDC_SEARCH));
		wstring_ptr wstrUpdate(getDlgItemText(IDC_UPDATE));
		pProfile_->setString(L"FullTextSearch", L"Command", wstrSearch.get());
		pProfile_->setString(L"FullTextSearch", L"IndexCommand", wstrUpdate.get());
	}
	
	return true;
}

LRESULT qm::OptionSearchDialog::onClicked()
{
	updateState();
	return 0;
}

void qm::OptionSearchDialog::updateState()
{
	bool bEnable = sendDlgItemMessage(IDC_CUSTOM, BM_GETCHECK) == BST_CHECKED;
	UINT nIds[] = {
		IDC_SEARCH,
		IDC_UPDATE
	};
	for (int n = 0; n < countof(nIds); ++n)
		Window(getDlgItem(nIds[n])).enableWindow(bEnable);
}
#endif // _WIN32_WCE


/****************************************************************************
 *
 * OptionSecurityDialog
 *
 */

DialogUtil::BoolProperty qm::OptionSecurityDialog::boolProperties__[] = {
	{ L"LoadSystemStore",	IDC_SYSTEMSTORE,		true	},
};

qm::OptionSecurityDialog::OptionSecurityDialog(Security* pSecurity,
											   Profile* pProfile) :
	DefaultDialog(IDD_OPTIONSECURITY),
	pSecurity_(pSecurity),
	pProfile_(pProfile)
{
}

qm::OptionSecurityDialog::~OptionSecurityDialog()
{
}

LRESULT qm::OptionSecurityDialog::onInitDialog(HWND hwndFocus,
											   LPARAM lParam)
{
	DialogUtil::loadBoolProperties(this, pProfile_,
		L"Security", boolProperties__, countof(boolProperties__));
	
#ifndef _WIN32_WCE
	bool bGPG = pProfile_->getInt(L"PGP", L"UseGPG", 1) != 0;
	Button_SetCheck(getDlgItem(bGPG ? IDC_GNUPG : IDC_PGP), BST_CHECKED);
#endif
	
	if (!Security::isSSLEnabled() && !Security::isSMIMEEnabled())
		Window(getDlgItem(IDC_SYSTEMSTORE)).enableWindow(false);
#ifndef _WIN32_WCE
	if (!Security::isPGPEnabled()) {
		Window(getDlgItem(IDC_PGP)).enableWindow(false);
		Window(getDlgItem(IDC_GNUPG)).enableWindow(false);
	}
#endif
	
	wstring_ptr wstrExtensions(pProfile_->getString(L"Global",
		L"WarnExtensions", L"exe com pif bat scr htm html hta vbs js"));
	setDlgItemText(IDC_WARNEXTENSION, wstrExtensions.get());
	
	return FALSE;
}

bool qm::OptionSecurityDialog::save(OptionDialogContext* pContext)
{
	DialogUtil::saveBoolProperties(this, pProfile_,
		L"Security", boolProperties__, countof(boolProperties__));
	
#ifndef _WIN32_WCE
	bool bGPG = Button_GetCheck(getDlgItem(IDC_GNUPG)) == BST_CHECKED;
	pProfile_->setInt(L"PGP", L"UseGPG", bGPG);
#endif
	
	wstring_ptr wstrExtensions(getDlgItemText(IDC_WARNEXTENSION));
	pProfile_->setString(L"Global", L"WarnExtensions", wstrExtensions.get());
	
	pContext->setFlags(OptionDialogContext::FLAG_RELOADSECURITY);
	
	return true;
}


/****************************************************************************
 *
 * AbstractOptionTextDialog
 *
 */

DialogUtil::BoolProperty qm::AbstractOptionTextDialog::boolProperties__[] = {
	{ L"WordWrap",					IDC_WORDWRAP,					false	},
	{ L"ShowRuler",					IDC_SHOWRULER,					false	},
	{ L"ShowVerticalScrollBar",		IDC_SHOWVERTICALSCROLLBAR,		true	},
	{ L"ShowHorizontalScrollBar",	IDC_SHOWHORIZONTALSCROLLBAR,	false	}
};

DialogUtil::IntProperty qm::AbstractOptionTextDialog::intProperties__[] = {
	{ L"TabWidth",	IDC_TABWIDTH,	4	}
};

qm::AbstractOptionTextDialog::AbstractOptionTextDialog(UINT nId,
													   Profile* pProfile,
													   const WCHAR* pwszSection) :
	DefaultDialog(nId),
	pProfile_(pProfile),
	pwszSection_(pwszSection),
	color_(pProfile, pwszSection, true)
{
	qs::UIUtil::getLogFontFromProfile(pProfile_, pwszSection_, false, &lf_);
}

qm::AbstractOptionTextDialog::~AbstractOptionTextDialog()
{
}

LRESULT qm::AbstractOptionTextDialog::onCommand(WORD nCode,
												WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_COLORS, onColors)
		HANDLE_COMMAND_ID(IDC_FONT, onFont)
		HANDLE_COMMAND_ID_RANGE_CODE(IDC_WRAPWINDOWWIDTH,
			IDC_WRAPCOLUMN, BN_CLICKED, onWrapChange)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::AbstractOptionTextDialog::onInitDialog(HWND hwndFocus,
												   LPARAM lParam)
{
	unsigned int nCharInLine = pProfile_->getInt(pwszSection_, L"CharInLine", 0);
	if (nCharInLine == 0) {
		Button_SetCheck(getDlgItem(IDC_WRAPWINDOWWIDTH), BST_CHECKED);
		setDlgItemInt(IDC_CHARINLINE, 80);
	}
	else {
		Button_SetCheck(getDlgItem(IDC_WRAPCOLUMN), BST_CHECKED);
		setDlgItemInt(IDC_CHARINLINE, nCharInLine, false);
	}
	
	DialogUtil::loadBoolProperties(this, pProfile_,
		pwszSection_, boolProperties__, countof(boolProperties__));
	DialogUtil::loadIntProperties(this, pProfile_,
		pwszSection_, intProperties__, countof(intProperties__));
	
	updateState();
	
	return FALSE;
}

bool qm::AbstractOptionTextDialog::save(OptionDialogContext* pContext)
{
	unsigned int nCharInLine = 0;
	if (Button_GetCheck(getDlgItem(IDC_WRAPCOLUMN)) == BST_CHECKED)
		nCharInLine = getDlgItemInt(IDC_CHARINLINE);
	pProfile_->setInt(pwszSection_, L"CharInLine", nCharInLine);
	
	DialogUtil::saveBoolProperties(this, pProfile_,
		pwszSection_, boolProperties__, countof(boolProperties__));
	DialogUtil::saveIntProperties(this, pProfile_,
		pwszSection_, intProperties__, countof(intProperties__));
	
	qs::UIUtil::setLogFontToProfile(pProfile_, pwszSection_, lf_);
	color_.save(pProfile_, pwszSection_);
	
	return true;
}

void qm::AbstractOptionTextDialog::updateState()
{
	bool bEnable = Button_GetCheck(getDlgItem(IDC_WRAPCOLUMN)) == BST_CHECKED;
	Window(getDlgItem(IDC_CHARINLINE)).enableWindow(bEnable);
}

LRESULT qm::AbstractOptionTextDialog::onColors()
{
	TextColorDialog dialog(color_);
	if (dialog.doModal(getParentPopup()) == IDOK)
		color_ = dialog.getData();
	return 0;
}

LRESULT qm::AbstractOptionTextDialog::onFont()
{
	qs::UIUtil::browseFont(getParentPopup(), &lf_);
	return 0;
}

LRESULT qm::AbstractOptionTextDialog::onWrapChange(UINT nId)
{
	updateState();
	return 0;
}


/****************************************************************************
 *
 * OptionEditDialog
 *
 */

DialogUtil::BoolProperty qm::OptionEditDialog::boolProperties__[] = {
	{ L"ShowTab",				IDC_SHOWTAB,				true	},
	{ L"ShowNewLine",			IDC_SHOWNEWLINE,			true	},
#ifdef _WIN32_WCE
	{ L"HideHeaderIfNoFocus",	IDC_HIDEHEADERIFNOFOCUS,	true	}
#else
	{ L"HideHeaderIfNoFocus",	IDC_HIDEHEADERIFNOFOCUS,	false	}
#endif
};

qm::OptionEditDialog::OptionEditDialog(EditFrameWindowManager* pEditFrameWindowManager,
									   Profile* pProfile) :
	AbstractOptionTextDialog(IDD_OPTIONEDIT, pProfile, L"EditWindow"),
	pEditFrameWindowManager_(pEditFrameWindowManager),
	pProfile_(pProfile)
{
	qs::UIUtil::getLogFontFromProfile(pProfile_, L"HeaderEditWindow", false, &lfHeader_);
}

qm::OptionEditDialog::~OptionEditDialog()
{
}

LRESULT qm::OptionEditDialog::onCommand(WORD nCode,
										WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_HEADERFONT, onHeaderFont)
	END_COMMAND_HANDLER()
	return AbstractOptionTextDialog::onCommand(nCode, nId);
}

LRESULT qm::OptionEditDialog::onInitDialog(HWND hwndFocus,
										   LPARAM lParam)
{
	DialogUtil::loadBoolProperties(this, pProfile_,
		L"EditWindow", boolProperties__, countof(boolProperties__));
	
	return AbstractOptionTextDialog::onInitDialog(hwndFocus, lParam);
}

bool qm::OptionEditDialog::save(OptionDialogContext* pContext)
{
	if (!AbstractOptionTextDialog::save(pContext))
		return false;
	
	DialogUtil::saveBoolProperties(this, pProfile_,
		L"EditWindow", boolProperties__, countof(boolProperties__));
	qs::UIUtil::setLogFontToProfile(pProfile_, L"HeaderEditWindow", lfHeader_);
	
	pContext->setFlags(OptionDialogContext::FLAG_RELOADEDIT |
		OptionDialogContext::FLAG_LAYOUTEDITWINDOW);
	
	return true;
}

LRESULT qm::OptionEditDialog::onHeaderFont()
{
	qs::UIUtil::browseFont(getParentPopup(), &lfHeader_);
	return 0;
}


/****************************************************************************
 *
 * SecurityDialog
 *
 */

SecurityDialog::Item qm::SecurityDialog::items__[] = {
	{ IDC_SMIMEENCRYPT,			MESSAGESECURITY_SMIMEENCRYPT			},
	{ IDC_SMIMESIGN,			MESSAGESECURITY_SMIMESIGN				},
	{ IDC_SMIMEMULTIPARTSIGNED,	MESSAGESECURITY_SMIMEMULTIPARTSIGNED	},
	{ IDC_SMIMEENCRYPTFORSELF,	MESSAGESECURITY_SMIMEENCRYPTFORSELF		},
#ifndef _WIN32_WCE
	{ IDC_PGPENCRYPT,			MESSAGESECURITY_PGPENCRYPT				},
	{ IDC_PGPSIGN,				MESSAGESECURITY_PGPSIGN					},
	{ IDC_PGPMIME,				MESSAGESECURITY_PGPMIME					}
#endif
};

qm::SecurityDialog::SecurityDialog(unsigned int nMessageSecurity) :
	DefaultDialog(IDD_SECURITY),
	nMessageSecurity_(nMessageSecurity)
{
}

qm::SecurityDialog::~SecurityDialog()
{
}

unsigned int qm::SecurityDialog::getMessageSecurity() const
{
	return nMessageSecurity_;
}

LRESULT qm::SecurityDialog::onInitDialog(HWND hwndFocus,
										 LPARAM lParam)
{
	for (int n = 0; n < countof(items__); ++n) {
		if (nMessageSecurity_ & items__[n].security_)
			Button_SetCheck(getDlgItem(items__[n].nId_), BST_CHECKED);
	}
	
	if (!Security::isSMIMEEnabled()) {
		for (UINT n = IDC_SMIMEENCRYPT; n <= IDC_SMIMEENCRYPTFORSELF; ++n)
			Window(getDlgItem(n)).enableWindow(false);
	}
	
#ifndef _WIN32_WCE
	if (!Security::isPGPEnabled()) {
		for (UINT n = IDC_PGPENCRYPT; n <= IDC_PGPMIME; ++n)
			Window(getDlgItem(n)).enableWindow(false);
	}
#endif
	
	init(false);
	
	return TRUE;
}

LRESULT qm::SecurityDialog::onOk()
{
	unsigned int nMessageSecurity = 0;
	for (int n = 0; n < countof(items__); ++n) {
		if (Button_GetCheck(getDlgItem(items__[n].nId_)) == BST_CHECKED)
			nMessageSecurity |= items__[n].security_;
	}
	nMessageSecurity_ = nMessageSecurity;
	
	return DefaultDialog::onOk();
}


/****************************************************************************
 *
 * OptionEdit2Dialog
 *
 */

DialogUtil::BoolProperty qm::OptionEdit2Dialog::globalBoolProperties__[] = {
	{ L"UseExternalEditor",			IDC_USEEXTERNALEDITOR,		false	},
	{ L"ExternalEditorAutoCreate",	IDC_AUTOCREATE,				true	}
};

DialogUtil::BoolProperty qm::OptionEdit2Dialog::editBoolProperties__[] = {
	{ L"AutoReform",			IDC_AUTOREFORM,			true	},
#ifdef QMZIP
	{ L"ArchiveAttachments",	IDC_ARCHIVEATTACHMENTS,	false	}
#endif
};

DialogUtil::IntProperty qm::OptionEdit2Dialog::intProperties__[] = {
	{ L"ReformLineLength",	IDC_COLUMN,	74	}
};

qm::OptionEdit2Dialog::OptionEdit2Dialog(EditFrameWindowManager* pEditFrameWindowManager,
										 Profile* pProfile) :
	DefaultDialog(IDD_OPTIONEDIT2),
	pEditFrameWindowManager_(pEditFrameWindowManager),
	pProfile_(pProfile),
	nMessageSecurity_(0)
{
	nMessageSecurity_ = pProfile_->getInt(L"Security", L"DefaultMessageSecurity",
		MESSAGESECURITY_SMIMEMULTIPARTSIGNED | MESSAGESECURITY_PGPMIME);
}

qm::OptionEdit2Dialog::~OptionEdit2Dialog()
{
}

LRESULT qm::OptionEdit2Dialog::onCommand(WORD nCode,
										 WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_BROWSE, onBrowse)
		HANDLE_COMMAND_ID(IDC_SECURITY, onSecurity)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::OptionEdit2Dialog::onInitDialog(HWND hwndFocus,
											LPARAM lParam)
{
	DialogUtil::loadBoolProperties(this, pProfile_,
		L"Global", globalBoolProperties__, countof(globalBoolProperties__));
	DialogUtil::loadBoolProperties(this, pProfile_,
		L"EditWindow", editBoolProperties__, countof(editBoolProperties__));
	DialogUtil::loadIntProperties(this, pProfile_,
		L"EditWindow", intProperties__, countof(intProperties__));
	
	wstring_ptr wstrEditor(pProfile_->getString(L"Global", L"Editor", L"notepad.exe"));
	setDlgItemText(IDC_EDITOR, wstrEditor.get());
	
	return FALSE;
}

bool qm::OptionEdit2Dialog::save(OptionDialogContext* pContext)
{
	DialogUtil::saveBoolProperties(this, pProfile_,
		L"Global", globalBoolProperties__, countof(globalBoolProperties__));
	DialogUtil::saveBoolProperties(this, pProfile_,
		L"EditWindow", editBoolProperties__, countof(editBoolProperties__));
	DialogUtil::saveIntProperties(this, pProfile_,
		L"EditWindow", intProperties__, countof(intProperties__));
	
	wstring_ptr wstrEditor(getDlgItemText(IDC_EDITOR));
	pProfile_->setString(L"Global", L"Editor", wstrEditor.get());
	
	pProfile_->setInt(L"Security", L"DefaultMessageSecurity", nMessageSecurity_);
	
	return true;
}

LRESULT qm::OptionEdit2Dialog::onBrowse()
{
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	wstring_ptr wstrFilter(loadString(hInst, IDS_FILTER_EXECUTABLE));
	
	FileDialog dialog(true, wstrFilter.get(), 0, 0, 0,
		OFN_EXPLORER | OFN_HIDEREADONLY | OFN_LONGNAMES);
	if (dialog.doModal(getHandle()) == IDOK)
		setDlgItemText(IDC_EDITOR, dialog.getPath());
	
	return 0;
}

LRESULT qm::OptionEdit2Dialog::onSecurity()
{
	SecurityDialog dialog(nMessageSecurity_);
	if (dialog.doModal(getParentPopup()) == IDOK)
		nMessageSecurity_ = dialog.getMessageSecurity();
	return 0;
}


/****************************************************************************
 *
 * OptionMessageDialog
 *
 */

qm::OptionMessageDialog::OptionMessageDialog(MessageFrameWindowManager* pMessageFrameWindowManager,
											 Profile* pProfile) :
	AbstractOptionTextDialog(IDD_OPTIONMESSAGE, pProfile, L"MessageWindow"),
	pMessageFrameWindowManager_(pMessageFrameWindowManager),
	pProfile_(pProfile)
{
}

qm::OptionMessageDialog::~OptionMessageDialog()
{
}

LRESULT qm::OptionMessageDialog::onInitDialog(HWND hwndFocus,
											  LPARAM lParam)
{
	return AbstractOptionTextDialog::onInitDialog(hwndFocus, lParam);
}

bool qm::OptionMessageDialog::save(OptionDialogContext* pContext)
{
	if (!AbstractOptionTextDialog::save(pContext))
		return false;
	
	pContext->setFlags(OptionDialogContext::FLAG_RELOADMESSAGE);
	
	return true;
}


/****************************************************************************
 *
 * OptionPreviewDialog
 *
 */

DialogUtil::IntProperty qm::OptionPreviewDialog::intProperties__[] = {
	{ L"SeenWait",	IDC_SEENWAIT,	0	}
};

qm::OptionPreviewDialog::OptionPreviewDialog(MessageWindow* pPreviewWindow,
											 Profile* pProfile) :
	AbstractOptionTextDialog(IDD_OPTIONPREVIEW, pProfile, L"PreviewWindow"),
	pPreviewWindow_(pPreviewWindow),
	pProfile_(pProfile)
{
}

qm::OptionPreviewDialog::~OptionPreviewDialog()
{
}

LRESULT qm::OptionPreviewDialog::onInitDialog(HWND hwndFocus,
											  LPARAM lParam)
{
	DialogUtil::loadIntProperties(this, pProfile_,
		L"PreviewWindow", intProperties__, countof(intProperties__));
	
	return AbstractOptionTextDialog::onInitDialog(hwndFocus, lParam);
}

bool qm::OptionPreviewDialog::save(OptionDialogContext* pContext)
{
	if (!AbstractOptionTextDialog::save(pContext))
		return false;
	
	DialogUtil::saveIntProperties(this, pProfile_,
		L"PreviewWindow", intProperties__, countof(intProperties__));
	
	pContext->setFlags(OptionDialogContext::FLAG_RELOADPREVIEW);
	
	return true;
}


#ifdef QMTABWINDOW
/****************************************************************************
 *
 * OptionTabDialog
 *
 */

DialogUtil::BoolProperty qm::OptionTabDialog::boolProperties__[] = {
	{ L"Multiline",			IDC_MULTILINE,	false	},
	{ L"ShowAllCount",		IDC_SHOWALL,	true	},
	{ L"ShowUnseenCount",	IDC_SHOWUNSEEN,	true	}
};

qm::OptionTabDialog::OptionTabDialog(TabWindow* pTabWindow,
									 Profile* pProfile) :
	DefaultDialog(IDD_OPTIONTAB),
	pTabWindow_(pTabWindow),
	pProfile_(pProfile)
{
	qs::UIUtil::getLogFontFromProfile(pProfile_, L"TabWindow", false, &lf_);
}

qm::OptionTabDialog::~OptionTabDialog()
{
}

LRESULT qm::OptionTabDialog::onCommand(WORD nCode,
									   WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_FONT, onFont)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::OptionTabDialog::onInitDialog(HWND hwndFocus,
										  LPARAM lParam)
{
	DialogUtil::loadBoolProperties(this, pProfile_,
		L"TabWindow", boolProperties__, countof(boolProperties__));
	
	DefaultTabModel* pTabModel = static_cast<DefaultTabModel*>(pTabWindow_->getTabModel());
	unsigned int nReuse = pTabModel->getReuse();
	if (nReuse & DefaultTabModel::REUSE_OPEN)
		Button_SetCheck(getDlgItem(IDC_REUSEOPEN), BST_CHECKED);
	if (nReuse & DefaultTabModel::REUSE_CHANGE)
		Button_SetCheck(getDlgItem(IDC_REUSECHANGE), BST_CHECKED);
	
	return FALSE;
}

bool qm::OptionTabDialog::save(OptionDialogContext* pContext)
{
	DialogUtil::saveBoolProperties(this, pProfile_,
		L"TabWindow", boolProperties__, countof(boolProperties__));
	
	DefaultTabModel* pTabModel = static_cast<DefaultTabModel*>(pTabWindow_->getTabModel());
	unsigned int nReuse = DefaultTabModel::REUSE_NONE;
	if (Button_GetCheck(getDlgItem(IDC_REUSEOPEN)) == BST_CHECKED)
		nReuse |= DefaultTabModel::REUSE_OPEN;
	if (Button_GetCheck(getDlgItem(IDC_REUSECHANGE)) == BST_CHECKED)
		nReuse |= DefaultTabModel::REUSE_CHANGE;
	pTabModel->setReuse(nReuse);
	
	qs::UIUtil::setLogFontToProfile(pProfile_, L"TabWindow", lf_);
	
	pContext->setFlags(OptionDialogContext::FLAG_RELOADTAB);
	
	return true;
}

LRESULT qm::OptionTabDialog::onFont()
{
	qs::UIUtil::browseFont(getParentPopup(), &lf_);
	return 0;
}
#endif // QMTABWINDOW


/****************************************************************************
 *
 * ColorSetsDialog
 *
 */

qm::ColorSetsDialog::ColorSetsDialog(ColorManager* pColorManager,
									 AccountManager* pAccountManager,
									 Profile* pProfile) :
	RuleColorSetsDialog<ColorSet, ColorManager::ColorSetList, ColorManager, ColorsDialog>(
		pColorManager, pAccountManager, pProfile, IDS_TITLE_COLORSETS,
		&ColorManager::getColorSets, &ColorManager::setColorSets)
{
}


/****************************************************************************
 *
 * ColorsDialog
 *
 */

qm::ColorsDialog::ColorsDialog(ColorSet* pColorSet,
							   AccountManager* pAccountManager,
							   Profile* pProfile) :
	RulesColorsDialog<ColorEntry, ColorSet::ColorList, ColorSet, ColorDialog>(
		pColorSet, pAccountManager, pProfile, IDS_TITLE_COLORS,
		&ColorSet::getColors, &ColorSet::setColors)
{
}

const WCHAR* qm::ColorsDialog::getName() const
{
	return L"ColorsDialog";
}

wstring_ptr qm::ColorsDialog::getLabelPrefix(const ColorEntry* p) const
{
	return concat(L"#", Color(p->getColor()).getString().get());
}


/****************************************************************************
 *
 * ColorDialog
 *
 */

qm::ColorDialog::ColorDialog(ColorEntry* pColor,
							 AccountManager* pAccountManager) :
	DefaultDialog(IDD_COLOR),
	pColor_(pColor)
{
}

qm::ColorDialog::~ColorDialog()
{
}

LRESULT qm::ColorDialog::onCommand(WORD nCode,
								   WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_EDIT, onEdit)
		HANDLE_COMMAND_ID(IDC_CHOOSE, onChoose)
		HANDLE_COMMAND_ID_CODE(IDC_COLOR, EN_CHANGE, onColorChange)
		HANDLE_COMMAND_ID_CODE(IDC_CONDITION, EN_CHANGE, onConditionChange)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::ColorDialog::onInitDialog(HWND hwndFocus,
									  LPARAM lParam)
{
	const Macro* pCondition = pColor_->getCondition();
	if (pCondition) {
		wstring_ptr wstrCondition(pCondition->getString());
		setDlgItemText(IDC_CONDITION, wstrCondition.get());
	}
	
	Color color(pColor_->getColor());
	wstring_ptr wstrColor(color.getString());
	setDlgItemText(IDC_COLOR, wstrColor.get());
	
	
	const WCHAR* pwszDescription = pColor_->getDescription();
	if (pwszDescription)
		setDlgItemText(IDC_DESCRIPTION, pwszDescription);
	
	init(false);
	updateState();
	
	return TRUE;
}

LRESULT qm::ColorDialog::onOk()
{
	wstring_ptr wstrCondition(getDlgItemText(IDC_CONDITION));
	std::auto_ptr<Macro> pCondition(MacroParser().parse(wstrCondition.get()));
	if (!pCondition.get()) {
		messageBox(Application::getApplication().getResourceHandle(),
			IDS_ERROR_INVALIDMACRO, MB_OK | MB_ICONERROR, getHandle());
		return 0;
	}
	
	wstring_ptr wstrColor(getDlgItemText(IDC_COLOR));
	Color color(wstrColor.get());
	if (color.getColor() == 0xffffffff) {
		// TODO MSG
		return 0;
	}
	
	wstring_ptr wstrDescription(getDlgItemText(IDC_DESCRIPTION));
	
	pColor_->setCondition(pCondition);
	pColor_->setColor(color.getColor());
	pColor_->setDescription(wstrDescription.get());
	
	return DefaultDialog::onOk();
}

LRESULT qm::ColorDialog::onEdit()
{
	wstring_ptr wstrCondition(getDlgItemText(IDC_CONDITION));
	ConditionsDialog dialog(wstrCondition.get());
	if (dialog.doModal(getHandle()) == IDOK)
		setDlgItemText(IDC_CONDITION, dialog.getCondition());
	return 0;
}

LRESULT qm::ColorDialog::onChoose()
{
	wstring_ptr wstrColor(getDlgItemText(IDC_COLOR));
	Color color(wstrColor.get());
	COLORREF cr = color.getColor();
	if (cr == 0xffffffff)
		cr = RGB(0, 0, 0);
	
	COLORREF crCustom[16];
	CHOOSECOLOR cc = {
		sizeof(cc),
		getHandle(),
		0,
		cr,
		crCustom,
		CC_ANYCOLOR | CC_RGBINIT,
		0,
		0,
		0
	};
	if (::ChooseColor(&cc)) {
		Color color(cc.rgbResult);
		wstring_ptr wstrColor(color.getString());
		setDlgItemText(IDC_COLOR, wstrColor.get());
	}
	return 0;
}

LRESULT qm::ColorDialog::onConditionChange()
{
	updateState();
	return 0;
}

LRESULT qm::ColorDialog::onColorChange()
{
	updateState();
	return 0;
}

void qm::ColorDialog::updateState()
{
	Window(getDlgItem(IDOK)).enableWindow(
		Window(getDlgItem(IDC_CONDITION)).getWindowTextLength() != 0 &&
		Window(getDlgItem(IDC_COLOR)).getWindowTextLength() != 0);
}


/****************************************************************************
 *
 * RuleSetsDialog
 *
 */

qm::RuleSetsDialog::RuleSetsDialog(RuleManager* pRuleManager,
								   AccountManager* pAccountManager,
								   Profile* pProfile) :
	RuleColorSetsDialog<RuleSet, RuleManager::RuleSetList, RuleManager, RulesDialog>(
		pRuleManager, pAccountManager, pProfile, IDS_TITLE_RULESETS,
		&RuleManager::getRuleSets, &RuleManager::setRuleSets)
{
}


/****************************************************************************
 *
 * RulesDialog
 *
 */

qm::RulesDialog::RulesDialog(RuleSet* pRuleSet,
							 AccountManager* pAccountManager,
							 Profile* pProfile) :
	RulesColorsDialog<Rule, RuleSet::RuleList, RuleSet, RuleDialog>(
		pRuleSet, pAccountManager, pProfile, IDS_TITLE_RULES,
		&RuleSet::getRules, &RuleSet::setRules)
{
}

const WCHAR* qm::RulesDialog::getName() const
{
	return L"RulesDialog";
}

wstring_ptr qm::RulesDialog::getLabelPrefix(const Rule* p) const
{
	return p->getAction()->getDescription();
}


/****************************************************************************
 *
 * RuleDialog
 *
 */

qm::RuleDialog::RuleDialog(Rule* pRule,
						   AccountManager* pAccountManager) :
	DefaultDialog(IDD_RULE),
	pRule_(pRule),
	pAccountManager_(pAccountManager),
	bInit_(false)
{
	RuleAction* pAction = pRule_->getAction();
	if ((pAction->getType() == RuleAction::TYPE_MOVE ||
		pAction->getType() == RuleAction::TYPE_COPY)) {
		CopyRuleAction* pCopyAction = static_cast<CopyRuleAction*>(pAction);
		
		if (pCopyAction->getTemplate())
			wstrTemplate_ = allocWString(pCopyAction->getTemplate());
		
		const CopyRuleAction::ArgumentList& l = pCopyAction->getArguments();
		listArgument_.reserve(l.size());
		for (CopyRuleAction::ArgumentList::const_iterator it = l.begin(); it != l.end(); ++it)
			listArgument_.push_back(CopyRuleAction::ArgumentList::value_type(
				allocWString((*it).first).release(), allocWString((*it).second).release()));
	}
}

qm::RuleDialog::~RuleDialog()
{
	for (CopyRuleAction::ArgumentList::iterator it = listArgument_.begin(); it != listArgument_.end(); ++it) {
		freeWString((*it).first);
		freeWString((*it).second);
	}
}

LRESULT qm::RuleDialog::onCommand(WORD nCode,
								  WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_EDIT, onEdit)
		HANDLE_COMMAND_ID(IDC_TEMPLATE, onTemplate)
		HANDLE_COMMAND_ID_CODE(IDC_ACCOUNT, CBN_EDITCHANGE, onAccountEditChange)
		HANDLE_COMMAND_ID_CODE(IDC_ACCOUNT, CBN_SELCHANGE, onAccountSelChange)
		HANDLE_COMMAND_ID_CODE(IDC_ACTION, CBN_SELCHANGE, onActionSelChange)
		HANDLE_COMMAND_ID_CODE(IDC_CONDITION, EN_CHANGE, onConditionChange)
		HANDLE_COMMAND_ID_CODE(IDC_FOLDER, CBN_EDITCHANGE, onFolderEditChange)
		HANDLE_COMMAND_ID_CODE(IDC_FOLDER, CBN_SELCHANGE, onFolderSelChange)
		HANDLE_COMMAND_ID_CODE(IDC_MACRO, EN_CHANGE, onMacroChange)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::RuleDialog::onInitDialog(HWND hwndFocus,
									 LPARAM lParam)
{
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	
	const Macro* pCondition = pRule_->getCondition();
	if (pCondition) {
		wstring_ptr wstrCondition(pCondition->getString());
		setDlgItemText(IDC_CONDITION, wstrCondition.get());
	}
	
	const UINT nTypeIds[] = {
		IDS_RULE_NONE,
		IDS_RULE_MOVE,
		IDS_RULE_COPY,
		IDS_RULE_DELETE,
		IDS_RULE_LABEL,
		IDS_RULE_DELETECACHE,
		IDS_RULE_APPLY
	};
	for (int n = 0; n < countof(nTypeIds); ++n) {
		wstring_ptr wstrType(loadString(hInst, nTypeIds[n]));
		W2T(wstrType.get(), ptszType);
		ComboBox_AddString(getDlgItem(IDC_ACTION), ptszType);
	}
	
	const AccountManager::AccountList& listAccount = pAccountManager_->getAccounts();
	for (AccountManager::AccountList::const_iterator it = listAccount.begin(); it != listAccount.end(); ++it) {
		Account* pAccount = *it;
		W2T(pAccount->getName(), ptszName);
		ComboBox_AddString(getDlgItem(IDC_ACCOUNT), ptszName);
	}
	
	const UINT nLabelTypeIds[] = {
		IDS_LABEL_SET,
		IDS_LABEL_ADD,
		IDS_LABEL_REMOVE
	};
	for (int n = 0; n < countof(nLabelTypeIds); ++n) {
		wstring_ptr wstrLabelType(loadString(hInst, nLabelTypeIds[n]));
		W2T(wstrLabelType.get(), ptszLabelType);
		ComboBox_AddString(getDlgItem(IDC_LABELTYPE), ptszLabelType);
	}
	ComboBox_SetCurSel(getDlgItem(IDC_LABELTYPE), 0);
	
	RuleAction* pAction = pRule_->getAction();
	RuleAction::Type type = pAction->getType();
	switch (type) {
	case RuleAction::TYPE_NONE:
		break;
	case RuleAction::TYPE_MOVE:
	case RuleAction::TYPE_COPY:
		{
			CopyRuleAction* pCopy = static_cast<CopyRuleAction*>(pAction);
			const WCHAR* pwszAccount = pCopy->getAccount();
			if (pwszAccount)
				setDlgItemText(IDC_ACCOUNT, pwszAccount);
			setDlgItemText(IDC_FOLDER, pCopy->getFolder());
		}
		break;
	case RuleAction::TYPE_DELETE:
		{
			bool bDirect = static_cast<DeleteRuleAction*>(pAction)->isDirect();
			Button_SetCheck(getDlgItem(IDC_DIRECT), bDirect ? BST_CHECKED : BST_UNCHECKED);
		}
		break;
	case RuleAction::TYPE_LABEL:
		setDlgItemText(IDC_LABEL, static_cast<LabelRuleAction*>(pAction)->getLabel());
		ComboBox_SetCurSel(getDlgItem(IDC_LABELTYPE),
			static_cast<LabelRuleAction*>(pAction)->getLabelType());
		break;
	case RuleAction::TYPE_DELETECACHE:
		break;
	case RuleAction::TYPE_APPLY:
		{
			wstring_ptr wstrMacro(static_cast<ApplyRuleAction*>(pAction)->getMacro()->getString());
			setDlgItemText(IDC_MACRO, wstrMacro.get());
		}
		break;
	default:
		assert(false);
		break;
	}
	ComboBox_SetCurSel(getDlgItem(IDC_ACTION), type);
	
	Button_SetCheck(getDlgItem(IDC_CONTINUE), pRule_->isContinue() ? BST_CHECKED : BST_UNCHECKED);
	
	unsigned int nUse = pRule_->getUse();
	Button_SetCheck(getDlgItem(IDC_MANUAL), nUse & Rule::USE_MANUAL ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(getDlgItem(IDC_AUTO), nUse & Rule::USE_AUTO ? BST_CHECKED : BST_UNCHECKED);
	
	const WCHAR* pwszDescription = pRule_->getDescription();
	if (pwszDescription)
		setDlgItemText(IDC_DESCRIPTION, pwszDescription);
	
	bInit_ = true;
	
	init(false);
	updateState(true);
	
	return TRUE;
}

LRESULT qm::RuleDialog::onOk()
{
	wstring_ptr wstrCondition(getDlgItemText(IDC_CONDITION));
	std::auto_ptr<Macro> pCondition(MacroParser().parse(wstrCondition.get()));
	if (!pCondition.get()) {
		messageBox(Application::getApplication().getResourceHandle(),
			IDS_ERROR_INVALIDMACRO, MB_OK | MB_ICONERROR, getHandle());
		return 0;
	}
	
	std::auto_ptr<RuleAction> pAction;
	RuleAction::Type type = static_cast<RuleAction::Type>(
		ComboBox_GetCurSel(getDlgItem(IDC_ACTION)));
	switch (type) {
	case RuleAction::TYPE_NONE:
		pAction.reset(new NoneRuleAction());
		break;
	case RuleAction::TYPE_MOVE:
	case RuleAction::TYPE_COPY:
		{
			wstring_ptr wstrAccount(getDlgItemText(IDC_ACCOUNT));
			if (!*wstrAccount.get())
				wstrAccount.reset(0);
			wstring_ptr wstrFolder(getDlgItemText(IDC_FOLDER));
			if (!*wstrFolder.get())
				return 0;
			
			std::auto_ptr<CopyRuleAction> pCopyAction(new CopyRuleAction(
				wstrAccount.get(), wstrFolder.get(), type == RuleAction::TYPE_MOVE));
			if (wstrTemplate_.get()) {
				pCopyAction->setTemplate(wstrTemplate_.get());
				pCopyAction->setTemplateArguments(listArgument_);
			}
			
			pAction.reset(pCopyAction.release());
		}
		break;
	case RuleAction::TYPE_DELETE:
		{
			bool bDirect = Button_GetCheck(getDlgItem(IDC_DIRECT)) == BST_CHECKED;
			pAction.reset(new DeleteRuleAction(bDirect));
		}
		break;
	case RuleAction::TYPE_LABEL:
		{
			LabelRuleAction::LabelType labelType = static_cast<LabelRuleAction::LabelType>(
				ComboBox_GetCurSel(getDlgItem(IDC_LABELTYPE)));
			wstring_ptr wstrLabel(getDlgItemText(IDC_LABEL));
			pAction.reset(new LabelRuleAction(labelType, wstrLabel.get()));
		}
		break;
	case RuleAction::TYPE_DELETECACHE:
		pAction.reset(new DeleteCacheRuleAction());
		break;
	case RuleAction::TYPE_APPLY:
		{
			wstring_ptr wstrMacro(getDlgItemText(IDC_MACRO));
			std::auto_ptr<Macro> pMacro(MacroParser().parse(wstrMacro.get()));
			if (!pMacro.get()) {
				messageBox(Application::getApplication().getResourceHandle(),
					IDS_ERROR_INVALIDMACRO, MB_OK | MB_ICONERROR, getHandle());
				return 0;
			}
			pAction.reset(new ApplyRuleAction(pMacro));
		}
		break;
	default:
		assert(false);
		break;
	}
	
	bool bContinue = Button_GetCheck(getDlgItem(IDC_CONTINUE)) == BST_CHECKED;
	
	unsigned int nUse = 0;
	if (Button_GetCheck(getDlgItem(IDC_MANUAL)) == BST_CHECKED)
		nUse |= Rule::USE_MANUAL;
	if (Button_GetCheck(getDlgItem(IDC_AUTO)) == BST_CHECKED)
		nUse |= Rule::USE_AUTO;
	
	wstring_ptr wstrDescription(getDlgItemText(IDC_DESCRIPTION));
	
	pRule_->setCondition(pCondition);
	pRule_->setAction(pAction);
	pRule_->setContinue(bContinue);
	pRule_->setUse(nUse);
	pRule_->setDescription(wstrDescription.get());
	
	return DefaultDialog::onOk();
}

LRESULT qm::RuleDialog::onEdit()
{
	wstring_ptr wstrCondition(getDlgItemText(IDC_CONDITION));
	ConditionsDialog dialog(wstrCondition.get());
	if (dialog.doModal(getHandle()) == IDOK)
		setDlgItemText(IDC_CONDITION, dialog.getCondition());
	return 0;
}

LRESULT qm::RuleDialog::onTemplate()
{
	CopyRuleTemplateDialog dialog(wstrTemplate_.get(), &listArgument_);
	if (dialog.doModal(getHandle()) == IDOK) {
		const WCHAR* pwszTemplate = dialog.getName();
		if (pwszTemplate)
			wstrTemplate_ = allocWString(pwszTemplate);
		else
			wstrTemplate_.reset(0);
	}
	return 0;
}

LRESULT qm::RuleDialog::onActionSelChange()
{
	updateState(true);
	return 0;
}

LRESULT qm::RuleDialog::onAccountEditChange()
{
	updateState(true);
	return 0;
}

LRESULT qm::RuleDialog::onAccountSelChange()
{
	postMessage(WM_COMMAND, MAKEWPARAM(IDC_ACCOUNT, CBN_EDITCHANGE));
	return 0;
}

LRESULT qm::RuleDialog::onConditionChange()
{
	updateState(false);
	return 0;
}

LRESULT qm::RuleDialog::onFolderEditChange()
{
	updateState(false);
	return 0;
}

LRESULT qm::RuleDialog::onFolderSelChange()
{
	postMessage(WM_COMMAND, MAKEWPARAM(IDC_FOLDER, CBN_EDITCHANGE));
	return 0;
}

LRESULT qm::RuleDialog::onMacroChange()
{
	updateState(false);
	return 0;
}

void qm::RuleDialog::updateState(bool bUpdateFolder)
{
	if (!bInit_)
		return;
	
	struct {
		UINT nId_;
		bool b_;
	} items[] = {
		{ IDC_ACCOUNTLABEL,		false	},
		{ IDC_ACCOUNT,			false	},
		{ IDC_FOLDERLABEL,		false	},
		{ IDC_FOLDER,			false	},
		{ IDC_TEMPLATE,			false	},
		{ IDC_DIRECT,			false	},
		{ IDC_LABELLABEL,		false	},
		{ IDC_LABEL,			false	},
		{ IDC_LABELTYPELABEL,	false	},
		{ IDC_LABELTYPE,		false	},
		{ IDC_MACROLABEL,		false	},
		{ IDC_MACRO,			false	}
	};
	
	int nStart = 0;
	int nEnd = 0;
	bool bEnable = true;
	bool bEnableContinue = true;
	switch (ComboBox_GetCurSel(getDlgItem(IDC_ACTION))) {
	case RuleAction::TYPE_NONE:
		break;
	case RuleAction::TYPE_MOVE:
	case RuleAction::TYPE_COPY:
		nStart = 0;
		nEnd = 5;
		bEnable = Window(getDlgItem(IDC_FOLDER)).getWindowTextLength() != 0;
		bEnableContinue = false;
		break;
	case RuleAction::TYPE_DELETE:
		nStart = 5;
		nEnd = 6;
		bEnableContinue = false;
		break;
	case RuleAction::TYPE_LABEL:
		nStart = 6;
		nEnd = 10;
		break;
	case RuleAction::TYPE_DELETECACHE:
		break;
	case RuleAction::TYPE_APPLY:
		nStart = 10;
		nEnd = 12;
		bEnable = Window(getDlgItem(IDC_MACRO)).getWindowTextLength() != 0;
		break;
	default:
		assert(false);
		break;
	}
	for (int n = nStart; n < nEnd; ++n)
		items[n].b_ = true;
	for (int n = 0; n < countof(items); ++n)
		Window(getDlgItem(items[n].nId_)).showWindow(items[n].b_ ? SW_SHOW : SW_HIDE);
	
	Window(getDlgItem(IDOK)).enableWindow(
		bEnable && Window(getDlgItem(IDC_CONDITION)).getWindowTextLength() != 0);
	Window(getDlgItem(IDC_CONTINUE)).enableWindow(bEnableContinue);
	
	if (bUpdateFolder) {
		Account* pAccount = 0;
		wstring_ptr wstrAccount(getDlgItemText(IDC_ACCOUNT));
		if (wstrAccount.get())
			pAccount = pAccountManager_->getAccount(wstrAccount.get());
		updateFolder(pAccount);
	}
}

void qm::RuleDialog::updateFolder(Account* pAccount)
{
	HWND hwnd = getDlgItem(IDC_FOLDER);
	
	wstring_ptr wstrFolder(getDlgItemText(IDC_FOLDER));
	
	ComboBox_ResetContent(hwnd);
	
	if (pAccount) {
		Account::FolderList l(pAccount->getFolders());
		std::sort(l.begin(), l.end(), FolderLess());
		for (Account::FolderList::const_iterator it = l.begin(); it != l.end(); ++it) {
			Folder* pFolder = *it;
			
			wstring_ptr wstrName(pFolder->getFullName());
			W2T(wstrName.get(), ptszName);
			ComboBox_AddString(hwnd, ptszName);
		}
	}
	
	setDlgItemText(IDC_FOLDER, wstrFolder.get());
}


/****************************************************************************
 *
 * CopyRuleTemplateDialog
 *
 */

qm::CopyRuleTemplateDialog::CopyRuleTemplateDialog(const WCHAR* pwszName,
												   CopyRuleAction::ArgumentList* pListArgument) :
	DefaultDialog(IDD_COPYRULETEMPLATE),
	pListArgument_(pListArgument)
{
	if (pwszName)
		wstrName_ = allocWString(pwszName);
}

qm::CopyRuleTemplateDialog::~CopyRuleTemplateDialog()
{
}

const WCHAR* qm::CopyRuleTemplateDialog::getName() const
{
	return wstrName_.get();
}

LRESULT qm::CopyRuleTemplateDialog::onCommand(WORD nCode,
											  WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_ADD, onAdd)
		HANDLE_COMMAND_ID(IDC_EDIT, onEdit)
		HANDLE_COMMAND_ID(IDC_REMOVE, onRemove)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::CopyRuleTemplateDialog::onDestroy()
{
	removeNotifyHandler(this);
	return DefaultDialog::onDestroy();
}

LRESULT qm::CopyRuleTemplateDialog::onInitDialog(HWND hwndFocus,
												 LPARAM lParam)
{
	if (wstrName_.get())
		setDlgItemText(IDC_NAME, wstrName_.get());
	
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	HWND hwndList = getDlgItem(IDC_ARGUMENT);
	
	ListView_SetExtendedListViewStyle(hwndList, LVS_EX_FULLROWSELECT);
	
	struct {
		UINT nId_;
		int nWidth_;
	} columns[] = {
		{ IDS_ARGUMENT_NAME,	100	},
		{ IDS_ARGUMENT_VALUE,	100	},
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
	
	for (CopyRuleAction::ArgumentList::size_type n = 0; n < pListArgument_->size(); ++n) {
		W2T((*pListArgument_)[n].first, ptszName);
		
		LVITEM item = {
			LVIF_TEXT,
			static_cast<int>(n),
			0,
			0,
			0,
			const_cast<LPTSTR>(ptszName),
			0,
			0,
			0
		};
		int nItem = ListView_InsertItem(hwndList, &item);
		
		W2T((*pListArgument_)[n].second, ptszValue);
		ListView_SetItemText(hwndList, nItem, 1, const_cast<LPTSTR>(ptszValue));
	}
	
	init(false);
	updateState();
	
	addNotifyHandler(this);
	
	return TRUE;
}

LRESULT qm::CopyRuleTemplateDialog::onOk()
{
	wstring_ptr wstrName(getDlgItemText(IDC_NAME));
	if (*wstrName.get())
		wstrName_ = wstrName;
	else
		wstrName_.reset(0);
	
	HWND hwndList = getDlgItem(IDC_ARGUMENT);
	CopyRuleAction::ArgumentList listArgument;
	int nCount = ListView_GetItemCount(hwndList);
	listArgument.reserve(nCount);
	for (int n = 0; n < nCount; ++n) {
		TCHAR tsz[256];
		ListView_GetItemText(hwndList, n, 0, tsz, countof(tsz) - 1);
		wstring_ptr wstrName(tcs2wcs(tsz));
		ListView_GetItemText(hwndList, n, 1, tsz, countof(tsz) - 1);
		wstring_ptr wstrValue(tcs2wcs(tsz));
		listArgument.push_back(CopyRuleAction::ArgumentList::value_type(
			wstrName.release(), wstrValue.release()));
	}
	pListArgument_->swap(listArgument);
	for (CopyRuleAction::ArgumentList::iterator it = listArgument.begin(); it != listArgument.end(); ++it) {
		freeWString((*it).first);
		freeWString((*it).second);
	}
	
	return DefaultDialog::onOk();
}

LRESULT qm::CopyRuleTemplateDialog::onNotify(NMHDR* pnmhdr,
											 bool* pbHandled)
{
	BEGIN_NOTIFY_HANDLER()
		HANDLE_NOTIFY(LVN_ITEMCHANGED, IDC_ARGUMENT, onArgumentItemChanged);
	END_NOTIFY_HANDLER()
	return NotifyHandler::onNotify(pnmhdr, pbHandled);
}

LRESULT qm::CopyRuleTemplateDialog::onAdd()
{
	ArgumentDialog dialog(0, 0);
	if (dialog.doModal(getHandle()) == IDOK) {
		HWND hwndList = getDlgItem(IDC_ARGUMENT);
		int nCount = ListView_GetItemCount(hwndList);
		
		W2T(dialog.getName(), ptszName);
		LVITEM item = {
			LVIF_TEXT | LVIF_STATE,
			nCount,
			0,
			LVIS_SELECTED | LVIS_FOCUSED,
			LVIS_SELECTED | LVIS_FOCUSED,
			const_cast<LPTSTR>(ptszName),
			0,
			0,
			0
		};
		int nItem = ListView_InsertItem(hwndList, &item);
		
		W2T(dialog.getValue(), ptszValue);
		ListView_SetItemText(hwndList, nItem, 1, const_cast<LPTSTR>(ptszValue));
	}
	return 0;
}

LRESULT qm::CopyRuleTemplateDialog::onRemove()
{
	HWND hwndList = getDlgItem(IDC_ARGUMENT);
	int nItem = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);
	if (nItem != -1) {
		ListView_DeleteItem(hwndList, nItem);
		
		int nCount = ListView_GetItemCount(hwndList);
		if (nCount != 0) {
			if (nItem < nCount) {
				ListView_SetItemState(hwndList, nItem,
					LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
			}
			else {
				ListView_SetItemState(hwndList, nCount - 1,
					LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
			}
		}
	}
	return 0;
}

LRESULT qm::CopyRuleTemplateDialog::onEdit()
{
	HWND hwndList = getDlgItem(IDC_ARGUMENT);
	int nItem = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);
	if (nItem != -1) {
		TCHAR tsz[256];
		ListView_GetItemText(hwndList, nItem, 0, tsz, countof(tsz) - 1);
		wstring_ptr wstrName(tcs2wcs(tsz));
		ListView_GetItemText(hwndList, nItem, 1, tsz, countof(tsz) - 1);
		wstring_ptr wstrValue(tcs2wcs(tsz));
		
		ArgumentDialog dialog(wstrName.get(), wstrValue.get());
		if (dialog.doModal(getHandle()) == IDOK) {
			W2T(dialog.getName(), ptszName);
			ListView_SetItemText(hwndList, nItem, 0, const_cast<LPTSTR>(ptszName));
			W2T(dialog.getValue(), ptszValue);
			ListView_SetItemText(hwndList, nItem, 1, const_cast<LPTSTR>(ptszValue));
		}
	}
	return 0;
}

LRESULT qm::CopyRuleTemplateDialog::onArgumentItemChanged(NMHDR* pnmhdr,
														  bool* pbHandled)
{
	updateState();
	*pbHandled = true;
	return 0;
}

void qm::CopyRuleTemplateDialog::updateState()
{
	HWND hwndList = getDlgItem(IDC_ARGUMENT);
	int nItem = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);
	Window(getDlgItem(IDC_EDIT)).enableWindow(nItem != -1);
	Window(getDlgItem(IDC_REMOVE)).enableWindow(nItem != -1);
}


/****************************************************************************
 *
 * ArgumentDialog
 *
 */

qm::ArgumentDialog::ArgumentDialog(const WCHAR* pwszName,
								   const WCHAR* pwszValue) :
	DefaultDialog(IDD_ARGUMENT)
{
	if (pwszName)
		wstrName_ = allocWString(pwszName);
	if (pwszValue)
		wstrValue_ = allocWString(pwszValue);
}

qm::ArgumentDialog::~ArgumentDialog()
{
}

const WCHAR* qm::ArgumentDialog::getName() const
{
	return wstrName_.get();
}

const WCHAR* qm::ArgumentDialog::getValue() const
{
	return wstrValue_.get();
}

LRESULT qm::ArgumentDialog::onCommand(WORD nCode,
									  WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID_CODE(IDC_NAME, EN_CHANGE, onNameChange)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::ArgumentDialog::onInitDialog(HWND hwndFocus,
										 LPARAM lParam)
{
	if (wstrName_.get())
		setDlgItemText(IDC_NAME, wstrName_.get());
	if (wstrValue_.get())
		setDlgItemText(IDC_VALUE, wstrValue_.get());
	
	init(false);
	updateState();
	
	return TRUE;
}

LRESULT qm::ArgumentDialog::onOk()
{
	wstrName_ = getDlgItemText(IDC_NAME);
	wstrValue_ = getDlgItemText(IDC_VALUE);
	
	return DefaultDialog::onOk();
}

LRESULT qm::ArgumentDialog::onNameChange()
{
	updateState();
	return 0;
}

void qm::ArgumentDialog::updateState()
{
	Window(getDlgItem(IDOK)).enableWindow(
		Window(getDlgItem(IDC_NAME)).getWindowTextLength() != 0);
}


/****************************************************************************
 *
 * AutoPilotDialog
 *
 */

qm::AutoPilotDialog::AutoPilotDialog(AutoPilotManager* pManager,
									 GoRound* pGoRound,
									 Recents* pRecents,
									 Profile* pProfile) :
	AbstractListDialog<AutoPilotEntry, AutoPilotManager::EntryList>(IDD_AUTOPILOT, IDC_ENTRIES, false),
	pManager_(pManager),
	pGoRound_(pGoRound),
	pRecents_(pRecents),
	pProfile_(pProfile)
{
	const AutoPilotManager::EntryList& l = pManager->getEntries();
	AutoPilotManager::EntryList& list = getList();
	list.reserve(l.size());
	for (AutoPilotManager::EntryList::const_iterator it = l.begin(); it != l.end(); ++it)
		list.push_back(new AutoPilotEntry(**it));
}

qm::AutoPilotDialog::~AutoPilotDialog()
{
}

INT_PTR qm::AutoPilotDialog::dialogProc(UINT uMsg,
										WPARAM wParam,
										LPARAM lParam)
{
	BEGIN_DIALOG_HANDLER()
		HANDLE_SIZE()
	END_DIALOG_HANDLER()
	return AbstractListDialog<AutoPilotEntry, AutoPilotManager::EntryList>::dialogProc(uMsg, wParam, lParam);
}

LRESULT qm::AutoPilotDialog::onCommand(WORD nCode,
									   WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_BROWSE, onBrowse)
	END_COMMAND_HANDLER()
	return AbstractListDialog<AutoPilotEntry, AutoPilotManager::EntryList>::onCommand(nCode, nId);
}

LRESULT qm::AutoPilotDialog::onInitDialog(HWND hwndFocus,
										  LPARAM lParam)
{
	wstring_ptr wstrSound = pProfile_->getString(L"AutoPilot", L"Sound", L"");
	setDlgItemText(IDC_SOUND, wstrSound.get());
	
	if (pProfile_->getInt(L"AutoPilot", L"OnlyWhenConnected", 0))
		Button_SetCheck(getDlgItem(IDC_ONLYWHENCONNECTED), BST_CHECKED);
	if (pRecents_->isEnabled())
		Button_SetCheck(getDlgItem(IDC_ADDTORECENTS), BST_CHECKED);
	
	return AbstractListDialog<AutoPilotEntry, AutoPilotManager::EntryList>::onInitDialog(hwndFocus, lParam);
}

wstring_ptr qm::AutoPilotDialog::getLabel(const AutoPilotEntry* p) const
{
	return allocWString(p->getCourse());
}

std::auto_ptr<AutoPilotEntry> qm::AutoPilotDialog::create() const
{
	std::auto_ptr<AutoPilotEntry> pEntry(new AutoPilotEntry());
	AutoPilotEntryDialog dialog(pEntry.get(), pGoRound_);
	if (dialog.doModal(getParentPopup()) != IDOK)
		return std::auto_ptr<AutoPilotEntry>();
	return pEntry;
}

AutoPilotEntry* qm::AutoPilotDialog::edit(AutoPilotEntry* p) const
{
	AutoPilotEntryDialog dialog(p, pGoRound_);
	if (dialog.doModal(getParentPopup()) != IDOK)
		return 0;
	return p;
}

bool qm::AutoPilotDialog::save(OptionDialogContext* pContext)
{
	pManager_->setEntries(getList());
	if (!pManager_->save(false))
		return false;
	
	wstring_ptr wstrSound(getDlgItemText(IDC_SOUND));
	if (wstrSound.get())
		pProfile_->setString(L"AutoPilot", L"Sound", wstrSound.get());
	
	bool bConnected = Button_GetCheck(getDlgItem(IDC_ONLYWHENCONNECTED)) == BST_CHECKED;
	pProfile_->setInt(L"AutoPilot", L"OnlyWhenConnected", bConnected);
	
	pRecents_->setEnabled(Button_GetCheck(getDlgItem(IDC_ADDTORECENTS)) == BST_CHECKED);
	
	return true;
}

LRESULT qm::AutoPilotDialog::onSize(UINT nFlags,
									int cx,
									int cy)
{
	layout();
	return AbstractListDialog<AutoPilotEntry, AutoPilotManager::EntryList>::onSize(nFlags, cx, cy);
}

LRESULT qm::AutoPilotDialog::onBrowse()
{
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	wstring_ptr wstrFilter(loadString(hInst, IDS_FILTER_SOUND));
	
	FileDialog dialog(true, wstrFilter.get(), 0, 0, 0,
		OFN_EXPLORER | OFN_HIDEREADONLY | OFN_LONGNAMES);
	if (dialog.doModal(getHandle()) == IDOK)
		setDlgItemText(IDC_SOUND, dialog.getPath());
	
	return 0;
}

void qm::AutoPilotDialog::layout()
{
#ifndef _WIN32_WCE_PSPC
	RECT rect;
	getClientRect(&rect);
	
	RECT rectSoundLabel;
	Window(getDlgItem(IDC_SOUNDLABEL)).getWindowRect(&rectSoundLabel);
	screenToClient(&rectSoundLabel);
	
	RECT rectSound;
	Window(getDlgItem(IDC_SOUND)).getWindowRect(&rectSound);
	screenToClient(&rectSound);
	int nSoundHeight = rectSound.bottom - rectSound.top;
	
	RECT rectBrowse;
	Window(getDlgItem(IDC_BROWSE)).getWindowRect(&rectBrowse);
	screenToClient(&rectBrowse);
	int nBrowseWidth = rectBrowse.right - rectBrowse.left;
	
	RECT rectConnected;
	Window(getDlgItem(IDC_ONLYWHENCONNECTED)).getWindowRect(&rectConnected);
	screenToClient(&rectConnected);
	int nConnectedHeight = rectConnected.bottom - rectConnected.top;
	
	RECT rectRecents;
	Window(getDlgItem(IDC_ADDTORECENTS)).getWindowRect(&rectRecents);
	screenToClient(&rectRecents);
	
	HDWP hdwp = beginDeferWindowPos(11);
	
	hdwp = LayoutUtil::layout(this, IDC_ENTRIES, hdwp, 0,
		nSoundHeight + nConnectedHeight + 10);
	
#ifdef _WIN32_WCE
	int nLabelOffset = 2;
#else
	int nLabelOffset = 3;
#endif
	hdwp = Window(getDlgItem(IDC_SOUNDLABEL)).deferWindowPos(hdwp, 0,
		rectSoundLabel.left, rect.bottom - nSoundHeight - nConnectedHeight - 10 + nLabelOffset,
		0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
	hdwp = Window(getDlgItem(IDC_SOUND)).deferWindowPos(hdwp, 0,
		rectSound.left, rect.bottom - nSoundHeight - nConnectedHeight - 10,
		rect.right - rectSound.left - nBrowseWidth - 10, nSoundHeight,
		SWP_NOZORDER | SWP_NOACTIVATE);
	hdwp = Window(getDlgItem(IDC_BROWSE)).deferWindowPos(hdwp, 0,
		rect.right - nBrowseWidth - 5, rect.bottom - nSoundHeight - nConnectedHeight - 10,
		0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
	hdwp = Window(getDlgItem(IDC_ONLYWHENCONNECTED)).deferWindowPos(hdwp, 0,
		rectConnected.left, rect.bottom - nConnectedHeight - 5,
		0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
	hdwp = Window(getDlgItem(IDC_ADDTORECENTS)).deferWindowPos(hdwp, 0,
		rectRecents.left, rect.bottom - nConnectedHeight - 5,
		0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
	
	endDeferWindowPos(hdwp);
#endif
}


/****************************************************************************
 *
 * AutoPilotEntryDialog
 *
 */

qm::AutoPilotEntryDialog::AutoPilotEntryDialog(AutoPilotEntry* pEntry,
											   GoRound* pGoRound) :
	DefaultDialog(IDD_AUTOPILOTENTRY),
	pEntry_(pEntry),
	pGoRound_(pGoRound)
{
}

qm::AutoPilotEntryDialog::~AutoPilotEntryDialog()
{
}

LRESULT qm::AutoPilotEntryDialog::onCommand(WORD nCode,
											WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID_CODE(IDC_COURSE, CBN_EDITCHANGE, onCourseEditChange)
		HANDLE_COMMAND_ID_CODE(IDC_COURSE, CBN_SELCHANGE, onCourseSelChange)
		HANDLE_COMMAND_ID_CODE(IDC_INTERVAL, EN_CHANGE, onIntervalChange)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::AutoPilotEntryDialog::onInitDialog(HWND hwndFocus,
											   LPARAM lParam)
{
	const GoRound::CourseList& listCourse = pGoRound_->getCourses();
	for (GoRound::CourseList::const_iterator it = listCourse.begin(); it != listCourse.end(); ++it) {
		W2T((*it)->getName(), ptszCourse);
		ComboBox_AddString(getDlgItem(IDC_COURSE), ptszCourse);
	}
	setDlgItemText(IDC_COURSE, pEntry_->getCourse());
	setDlgItemInt(IDC_INTERVAL, pEntry_->getInterval());
	
	init(false);
	updateState();
	
	return TRUE;
}

LRESULT qm::AutoPilotEntryDialog::onOk()
{
	wstring_ptr wstrCourse(getDlgItemText(IDC_COURSE));
	if (!*wstrCourse.get())
		return 0;
	
	int nInterval = getDlgItemInt(IDC_INTERVAL);
	if (nInterval == 0)
		return 0;
	
	pEntry_->setCourse(wstrCourse.get());
	pEntry_->setInterval(nInterval);
	
	return DefaultDialog::onOk();
}

LRESULT qm::AutoPilotEntryDialog::onCourseEditChange()
{
	updateState();
	return 0;
}

LRESULT qm::AutoPilotEntryDialog::onCourseSelChange()
{
	postMessage(WM_COMMAND, MAKEWPARAM(IDC_COURSE, CBN_EDITCHANGE));
	return 0;
}

LRESULT qm::AutoPilotEntryDialog::onIntervalChange()
{
	updateState();
	return 0;
}

void qm::AutoPilotEntryDialog::updateState()
{
	Window(getDlgItem(IDOK)).enableWindow(
		Window(getDlgItem(IDC_COURSE)).getWindowTextLength() != 0 &&
		getDlgItemInt(IDC_INTERVAL) != 0);
}


/****************************************************************************
 *
 * FiltersDialog
 *
 */

qm::FiltersDialog::FiltersDialog(FilterManager* pManager) :
	AbstractListDialog<Filter, FilterManager::FilterList>(IDD_FILTERS, IDC_FILTERS, false),
	pManager_(pManager)
{
	const FilterManager::FilterList& l = pManager->getFilters();
	FilterManager::FilterList& list = getList();
	list.reserve(l.size());
	for (FilterManager::FilterList::const_iterator it = l.begin(); it != l.end(); ++it)
		list.push_back(new Filter(**it));
}

qm::FiltersDialog::~FiltersDialog()
{
}

INT_PTR qm::FiltersDialog::dialogProc(UINT uMsg,
									  WPARAM wParam,
									  LPARAM lParam)
{
	BEGIN_DIALOG_HANDLER()
		HANDLE_SIZE()
	END_DIALOG_HANDLER()
	return DefaultDialog::dialogProc(uMsg, wParam, lParam);
}

wstring_ptr qm::FiltersDialog::getLabel(const Filter* p) const
{
	return allocWString(p->getName());
}

std::auto_ptr<Filter> qm::FiltersDialog::create() const
{
	std::auto_ptr<Filter> pFilter(new Filter());
	FilterDialog dialog(pFilter.get());
	if (dialog.doModal(getParentPopup()) != IDOK)
		return std::auto_ptr<Filter>();
	return pFilter;
}

Filter* qm::FiltersDialog::edit(Filter* p) const
{
	FilterDialog dialog(p);
	if (dialog.doModal(getParentPopup()) != IDOK)
		return 0;
	return p;
}

bool qm::FiltersDialog::save(OptionDialogContext* pContext)
{
	pManager_->setFilters(getList());
	return pManager_->save();
}

LRESULT qm::FiltersDialog::onSize(UINT nFlags,
								  int cx,
								  int cy)
{
	layout();
	return DefaultDialog::onSize(nFlags, cx, cy);
}

void qm::FiltersDialog::layout()
{
#ifndef _WIN32_WCE_PSPC
	LayoutUtil::layout(this, IDC_FILTERS);
#endif
}


/****************************************************************************
 *
 * FilterDialog
 *
 */

qm::FilterDialog::FilterDialog(Filter* pFilter) :
	DefaultDialog(IDD_FILTER),
	pFilter_(pFilter)
{
}

qm::FilterDialog::~FilterDialog()
{
}

LRESULT qm::FilterDialog::onCommand(WORD nCode,
										   WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_EDIT, onEdit)
		HANDLE_COMMAND_ID_CODE(IDC_CONDITION, EN_CHANGE, onConditionChange)
		HANDLE_COMMAND_ID_CODE(IDC_NAME, EN_CHANGE, onNameChange)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::FilterDialog::onInitDialog(HWND hwndFocus,
											  LPARAM lParam)
{
	setDlgItemText(IDC_NAME, pFilter_->getName());
	
	const Macro* pCondition = pFilter_->getCondition();
	wstring_ptr wstrCondition(pCondition->getString());
	setDlgItemText(IDC_CONDITION, wstrCondition.get());
	
	init(false);
	
	return TRUE;
}

LRESULT qm::FilterDialog::onOk()
{
	wstring_ptr wstrCondition(getDlgItemText(IDC_CONDITION));
	std::auto_ptr<Macro> pCondition(MacroParser().parse(wstrCondition.get()));
	if (!pCondition.get()) {
		messageBox(Application::getApplication().getResourceHandle(),
			IDS_ERROR_INVALIDMACRO, MB_OK | MB_ICONERROR, getHandle());
		return 0;
	}
	pFilter_->setCondition(pCondition);
	
	wstring_ptr wstrName(getDlgItemText(IDC_NAME));
	pFilter_->setName(wstrName.get());
	
	return DefaultDialog::onOk();
}

LRESULT qm::FilterDialog::onEdit()
{
	wstring_ptr wstrCondition(getDlgItemText(IDC_CONDITION));
	ConditionsDialog dialog(wstrCondition.get());
	if (dialog.doModal(getHandle()) == IDOK)
		setDlgItemText(IDC_CONDITION, dialog.getCondition());
	return 0;
}

LRESULT qm::FilterDialog::onConditionChange()
{
	updateState();
	return 0;
}

LRESULT qm::FilterDialog::onNameChange()
{
	updateState();
	return 0;
}

void qm::FilterDialog::updateState()
{
	Window(getDlgItem(IDOK)).enableWindow(
		Window(getDlgItem(IDC_NAME)).getWindowTextLength() != 0 &&
		Window(getDlgItem(IDC_CONDITION)).getWindowTextLength() != 0);
}


/****************************************************************************
*
* FixedFormTextsDialog
*
*/

qm::FixedFormTextsDialog::FixedFormTextsDialog(FixedFormTextManager* pManager,
											   qs::Profile* pProfile) :
	AbstractListDialog<FixedFormText, FixedFormTextManager::TextList>(IDD_FIXEDFORMTEXTS, IDC_TEXTS, false),
	pManager_(pManager),
	pProfile_(pProfile)
{
	const FixedFormTextManager::TextList& l = pManager->getTexts();
	FixedFormTextManager::TextList& list = getList();
	list.reserve(l.size());
	for (FixedFormTextManager::TextList::const_iterator it = l.begin(); it != l.end(); ++it)
		list.push_back(new FixedFormText(**it));
}

qm::FixedFormTextsDialog::~FixedFormTextsDialog()
{
}

INT_PTR qm::FixedFormTextsDialog::dialogProc(UINT uMsg,
											 WPARAM wParam,
											 LPARAM lParam)
{
	BEGIN_DIALOG_HANDLER()
		HANDLE_SIZE()
	END_DIALOG_HANDLER()
	return DefaultDialog::dialogProc(uMsg, wParam, lParam);
}

wstring_ptr qm::FixedFormTextsDialog::getLabel(const FixedFormText* p) const
{
	return allocWString(p->getName());
}

std::auto_ptr<FixedFormText> qm::FixedFormTextsDialog::create() const
{
	std::auto_ptr<FixedFormText> pText(new FixedFormText());
	FixedFormTextDialog dialog(pText.get(), pProfile_);
	if (dialog.doModal(getParentPopup()) != IDOK)
		return std::auto_ptr<FixedFormText>();
	return pText;
}

FixedFormText* qm::FixedFormTextsDialog::edit(FixedFormText* p) const
{
	FixedFormTextDialog dialog(p, pProfile_);
	if (dialog.doModal(getParentPopup()) != IDOK)
		return 0;
	return p;
}

bool qm::FixedFormTextsDialog::save(OptionDialogContext* pContext)
{
	pManager_->setTexts(getList());
	return pManager_->save();
}

LRESULT qm::FixedFormTextsDialog::onSize(UINT nFlags,
										 int cx,
										 int cy)
{
	layout();
	return DefaultDialog::onSize(nFlags, cx, cy);
}

void qm::FixedFormTextsDialog::layout()
{
#ifndef _WIN32_WCE_PSPC
	LayoutUtil::layout(this, IDC_TEXTS);
#endif
}


/****************************************************************************
 *
 * FixedFormTextDialog
 *
 */

qm::FixedFormTextDialog::FixedFormTextDialog(FixedFormText* pText,
											 qs::Profile* pProfile) :
	DefaultDialog(IDD_FIXEDFORMTEXT),
	pText_(pText),
	pProfile_(pProfile)
{
}

qm::FixedFormTextDialog::~FixedFormTextDialog()
{
}

INT_PTR qm::FixedFormTextDialog::dialogProc(UINT uMsg,
											WPARAM wParam,
											LPARAM lParam)
{
	BEGIN_DIALOG_HANDLER()
		HANDLE_SIZE()
	END_DIALOG_HANDLER()
	return DefaultDialog::dialogProc(uMsg, wParam, lParam);
}

LRESULT qm::FixedFormTextDialog::onCommand(WORD nCode,
										   WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID_CODE(IDC_NAME, EN_CHANGE, onNameChange)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::FixedFormTextDialog::onDestroy()
{
#ifndef _WIN32_WCE
	RECT rect;
	getWindowRect(&rect);
	pProfile_->setInt(L"FixedFormTextDialog", L"Width", rect.right - rect.left);
	pProfile_->setInt(L"FixedFormTextDialog", L"Height", rect.bottom - rect.top);
#endif
	return DefaultDialog::onDestroy();
}

LRESULT qm::FixedFormTextDialog::onInitDialog(HWND hwndFocus,
											  LPARAM lParam)
{
	setDlgItemText(IDC_NAME, pText_->getName());
	
	wstring_ptr wstrText(Util::convertLFtoCRLF(pText_->getText()));
	setDlgItemText(IDC_TEXT, wstrText.get());
	
#ifdef _WIN32_WCE
	RECT rectWorkArea;
	::SystemParametersInfo(SPI_GETWORKAREA, 0, &rectWorkArea, 0);
	int nWidth = rectWorkArea.right - rectWorkArea.left;
	int nHeight = rectWorkArea.bottom - rectWorkArea.top;
	setWindowPos(0, 0, 0, nWidth, nHeight, SWP_NOZORDER | SWP_NOACTIVATE);
#else
	int nWidth = pProfile_->getInt(L"FixedFormTextDialog", L"Width", 620);
	int nHeight = pProfile_->getInt(L"FixedFormTextDialog", L"Height", 450);
	setWindowPos(0, 0, 0, nWidth, nHeight,
		SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
#endif
	
	init(false);
	
	return TRUE;
}

LRESULT qm::FixedFormTextDialog::onOk()
{
	wstring_ptr wstrName(getDlgItemText(IDC_NAME));
	
	wstring_ptr wstrText(getDlgItemText(IDC_TEXT));
	wstrText = Util::convertCRLFtoLF(wstrText.get());
	
	pText_->setName(wstrName.get());
	pText_->setText(wstrText.get());
	
	return DefaultDialog::onOk();
}

LRESULT qm::FixedFormTextDialog::onSize(UINT nFlags,
										int cx,
										int cy)
{
	layout();
	return DefaultDialog::onSize(nFlags, cx, cy);
}

LRESULT qm::FixedFormTextDialog::onNameChange()
{
	updateState();
	return 0;
}

void qm::FixedFormTextDialog::updateState()
{
	Window(getDlgItem(IDOK)).enableWindow(
		Window(getDlgItem(IDC_NAME)).getWindowTextLength() != 0);
}

void qm::FixedFormTextDialog::layout()
{
#ifndef _WIN32_WCE_PSPC
	RECT rect;
	getClientRect(&rect);
	
#ifndef _WIN32_WCE
	RECT rectButton;
	Window(getDlgItem(IDOK)).getWindowRect(&rectButton);
	int nButtonWidth = rectButton.right - rectButton.left;
	int nButtonHeight = rectButton.bottom - rectButton.top;
#else
	int nButtonHeight = -5;
#endif
	
	RECT rectName;
	Window(getDlgItem(IDC_NAME)).getWindowRect(&rectName);
	screenToClient(&rectName);
	
	RECT rectText;
	Window(getDlgItem(IDC_TEXT)).getWindowRect(&rectText);
	screenToClient(&rectText);
	
	HDWP hdwp = beginDeferWindowPos(5);
	hdwp = Window(getDlgItem(IDC_NAME)).deferWindowPos(hdwp, 0, 0, 0,
		rect.right - rectName.left - 5, rectName.bottom - rectName.top,
		SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
	hdwp = Window(getDlgItem(IDC_TEXT)).deferWindowPos(hdwp, 0, 0, 0,
		rect.right - rectText.left - 5, rect.bottom - rectText.top - nButtonHeight - 10,
		SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
	
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


/****************************************************************************
 *
 * GoRoundDialog
 *
 */

qm::GoRoundDialog::GoRoundDialog(GoRound* pGoRound,
								 AccountManager* pAccountManager,
								 SyncFilterManager* pSyncFilterManager,
								 Profile* pProfile) :
	AbstractListDialog<GoRoundCourse, GoRound::CourseList>(IDD_GOROUND, IDC_COURSE, false),
	pGoRound_(pGoRound),
	pAccountManager_(pAccountManager),
	pSyncFilterManager_(pSyncFilterManager),
	pProfile_(pProfile)
{
	const GoRound::CourseList& listCourse = pGoRound_->getCourses();
	GoRound::CourseList& list = getList();
	list.reserve(listCourse.size());
	for (GoRound::CourseList::const_iterator it = listCourse.begin(); it != listCourse.end(); ++it)
		list.push_back(new GoRoundCourse(**it));
}

qm::GoRoundDialog::~GoRoundDialog()
{
}

INT_PTR qm::GoRoundDialog::dialogProc(UINT uMsg,
									  WPARAM wParam,
									  LPARAM lParam)
{
	BEGIN_DIALOG_HANDLER()
		HANDLE_SIZE()
	END_DIALOG_HANDLER()
	return DefaultDialog::dialogProc(uMsg, wParam, lParam);
}

wstring_ptr qm::GoRoundDialog::getLabel(const GoRoundCourse* p) const
{
	return allocWString(p->getName());
}

std::auto_ptr<GoRoundCourse> qm::GoRoundDialog::create() const
{
	std::auto_ptr<GoRoundCourse> pCourse(new GoRoundCourse());
	GoRoundCourseDialog dialog(pCourse.get(), pAccountManager_, pSyncFilterManager_, pProfile_);
	if (dialog.doModal(getParentPopup()) != IDOK)
		return std::auto_ptr<GoRoundCourse>();
	return pCourse;
}

GoRoundCourse* qm::GoRoundDialog::edit(GoRoundCourse* p) const
{
	GoRoundCourseDialog dialog(p, pAccountManager_, pSyncFilterManager_, pProfile_);
	if (dialog.doModal(getParentPopup()) != IDOK)
		return 0;
	return p;
}

bool qm::GoRoundDialog::save(OptionDialogContext* pContext)
{
	pGoRound_->setCourses(getList());
	return pGoRound_->save();
}

LRESULT qm::GoRoundDialog::onSize(UINT nFlags,
								  int cx,
								  int cy)
{
	layout();
	return DefaultDialog::onSize(nFlags, cx, cy);
}

void qm::GoRoundDialog::layout()
{
#ifndef _WIN32_WCE_PSPC
	LayoutUtil::layout(this, IDC_COURSE);
#endif
}


/****************************************************************************
 *
 * GoRoundCourseDialog
 *
 */

qm::GoRoundCourseDialog::GoRoundCourseDialog(GoRoundCourse* pCourse,
											 AccountManager* pAccountManager,
											 SyncFilterManager* pSyncFilterManager,
											 Profile* pProfile) :
	AbstractListDialog<GoRoundEntry, GoRoundCourse::EntryList>(IDD_GOROUNDCOURSE, IDC_ENTRY, true),
	pCourse_(pCourse),
	pAccountManager_(pAccountManager),
	pSyncFilterManager_(pSyncFilterManager),
	pProfile_(pProfile)
{
	const GoRoundCourse::EntryList& listEntry = pCourse_->getEntries();
	GoRoundCourse::EntryList& list = getList();
	list.reserve(listEntry.size());
	for (GoRoundCourse::EntryList::const_iterator it = listEntry.begin(); it != listEntry.end(); ++it)
		list.push_back(new GoRoundEntry(**it));
}

qm::GoRoundCourseDialog::~GoRoundCourseDialog()
{
}

INT_PTR qm::GoRoundCourseDialog::dialogProc(UINT uMsg,
											WPARAM wParam,
											LPARAM lParam)
{
	BEGIN_DIALOG_HANDLER()
		HANDLE_SIZE()
	END_DIALOG_HANDLER()
	return AbstractListDialog<GoRoundEntry, GoRoundCourse::EntryList>::dialogProc(uMsg, wParam, lParam);
}

LRESULT qm::GoRoundCourseDialog::onCommand(WORD nCode,
										   WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_DIALUP, onDialup)
		HANDLE_COMMAND_ID_CODE(IDC_NAME, EN_CHANGE, onNameChange)
	END_COMMAND_HANDLER()
	return AbstractListDialog<GoRoundEntry, GoRoundCourse::EntryList>::onCommand(nCode, nId);
}

LRESULT qm::GoRoundCourseDialog::onDestroy()
{
#ifndef _WIN32_WCE
	RECT rect;
	getWindowRect(&rect);
	pProfile_->setInt(L"GoRoundCourseDialog", L"Width", rect.right - rect.left);
	pProfile_->setInt(L"GoRoundCourseDialog", L"Height", rect.bottom - rect.top);
#endif
	
	return AbstractListDialog<GoRoundEntry, GoRoundCourse::EntryList>::onDestroy();
}

LRESULT qm::GoRoundCourseDialog::onInitDialog(HWND hwndFocus,
											  LPARAM lParam)
{
	setDlgItemText(IDC_NAME, pCourse_->getName());
	
	switch (pCourse_->getType()) {
	case GoRoundCourse::TYPE_SEQUENTIAL:
		Button_SetCheck(getDlgItem(IDC_SEQUENTIAL), BST_CHECKED);
		break;
	case GoRoundCourse::TYPE_PARALLEL:
		Button_SetCheck(getDlgItem(IDC_PARALLEL), BST_CHECKED);
		break;
	default:
		assert(false);
		break;
	}
	
	Button_SetCheck(getDlgItem(IDC_CONFIRM),
		pCourse_->isFlag(GoRoundCourse::FLAG_CONFIRM) ? BST_CHECKED : BST_UNCHECKED);
	
#ifdef _WIN32_WCE
	RECT rectWorkArea;
	::SystemParametersInfo(SPI_GETWORKAREA, 0, &rectWorkArea, 0);
	int nWidth = rectWorkArea.right - rectWorkArea.left;
	int nHeight = rectWorkArea.bottom - rectWorkArea.top;
	setWindowPos(0, 0, 0, nWidth, nHeight, SWP_NOZORDER | SWP_NOACTIVATE);
#else
	int nWidth = pProfile_->getInt(L"GoRoundCourseDialog", L"Width", 620);
	int nHeight = pProfile_->getInt(L"GoRoundCourseDialog", L"Height", 450);
	setWindowPos(0, 0, 0, nWidth, nHeight,
		SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
#endif
	
	return AbstractListDialog<GoRoundEntry, GoRoundCourse::EntryList>::onInitDialog(hwndFocus, lParam);
}

LRESULT qm::GoRoundCourseDialog::onOk()
{
	wstring_ptr wstrName(getDlgItemText(IDC_NAME));
	if (!wstrName.get())
		return 0;
	pCourse_->setName(wstrName.get());
	
	pCourse_->setEntries(getList());
	
	pCourse_->setType(Button_GetCheck(getDlgItem(IDC_SEQUENTIAL)) == BST_CHECKED ?
		GoRoundCourse::TYPE_SEQUENTIAL : GoRoundCourse::TYPE_PARALLEL);
	pCourse_->setFlags(Button_GetCheck(getDlgItem(IDC_CONFIRM)) == BST_CHECKED ?
		GoRoundCourse::FLAG_CONFIRM : 0);
	
	return AbstractListDialog<GoRoundEntry, GoRoundCourse::EntryList>::onOk();
}

wstring_ptr qm::GoRoundCourseDialog::getLabel(const GoRoundEntry* p) const
{
	StringBuffer<WSTRING> buf(p->getAccount());
	if (p->getSubAccount()) {
		buf.append(L'/');
		buf.append(p->getSubAccount());
	}
	if (p->getFolder()) {
		buf.append(L" [");
		buf.append(p->getFolder());
		buf.append(L']');
	}
	return buf.getString();
}

std::auto_ptr<GoRoundEntry> qm::GoRoundCourseDialog::create() const
{
	std::auto_ptr<GoRoundEntry> pEntry(new GoRoundEntry());
	GoRoundEntryDialog dialog(pEntry.get(), pAccountManager_, pSyncFilterManager_);
	if (dialog.doModal(getHandle()) != IDOK)
		return std::auto_ptr<GoRoundEntry>();
	return pEntry;
}

GoRoundEntry* qm::GoRoundCourseDialog::edit(GoRoundEntry* p) const
{
	GoRoundEntryDialog dialog(p, pAccountManager_, pSyncFilterManager_);
	if (dialog.doModal(getHandle()) != IDOK)
		return 0;
	return p;
}

void qm::GoRoundCourseDialog::updateState()
{
	AbstractListDialog<GoRoundEntry, GoRoundCourse::EntryList>::updateState();
	
	wstring_ptr wstrName(getDlgItemText(IDC_NAME));
	Window(getDlgItem(IDOK)).enableWindow(*wstrName.get() != L'\0');
}

LRESULT qm::GoRoundCourseDialog::onSize(UINT nFlags,
										int cx,
										int cy)
{
	layout();
	return AbstractListDialog<GoRoundEntry, GoRoundCourse::EntryList>::onSize(nFlags, cx, cy);
}

LRESULT qm::GoRoundCourseDialog::onDialup()
{
	if (pCourse_->getDialup()) {
		GoRoundDialupDialog dialog(pCourse_->getDialup(), false);
		if (dialog.doModal(getHandle()) == IDOK) {
			if (dialog.isNoDialup()) {
				std::auto_ptr<GoRoundDialup> pDialup;
				pCourse_->setDialup(pDialup);
			}
		}
	}
	else {
		std::auto_ptr<GoRoundDialup> pDialup(new GoRoundDialup());
		GoRoundDialupDialog dialog(pDialup.get(), true);
		if (dialog.doModal(getHandle()) == IDOK) {
			if (!dialog.isNoDialup())
				pCourse_->setDialup(pDialup);
		}
	}
	
	return 0;
}

LRESULT qm::GoRoundCourseDialog::onNameChange()
{
	updateState();
	return 0;
}

void qm::GoRoundCourseDialog::layout()
{
#ifndef _WIN32_WCE_PSPC
	RECT rect;
	getClientRect(&rect);
	
	RECT rectName;
	Window(getDlgItem(IDC_NAME)).getWindowRect(&rectName);
	screenToClient(&rectName);
	
	RECT rectType;
	Window(getDlgItem(IDC_TYPE)).getWindowRect(&rectType);
	screenToClient(&rectType);
	int nTypeHeight = rectType.bottom - rectType.top;
	
	RECT rectSequential;
	Window(getDlgItem(IDC_SEQUENTIAL)).getWindowRect(&rectSequential);
	screenToClient(&rectSequential);
	RECT rectParallel;
	Window(getDlgItem(IDC_PARALLEL)).getWindowRect(&rectParallel);
	screenToClient(&rectParallel);
	
	RECT rectButton;
	Window(getDlgItem(IDC_DIALUP)).getWindowRect(&rectButton);
	int nButtonWidth = rectButton.right - rectButton.left;
#ifndef _WIN32_WCE
	int nButtonHeight = rectButton.bottom - rectButton.top;
#else
	int nButtonHeight = -5;
#endif
	
	HDWP hdwp = beginDeferWindowPos(15);
	
	hdwp = Window(getDlgItem(IDC_NAME)).deferWindowPos(hdwp, 0, 0, 0,
		rect.right - rectName.left - 5, rectName.bottom - rectName.top,
		SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
	
	hdwp = LayoutUtil::layout(this, IDC_ENTRY, hdwp, rectName.bottom,
		nButtonHeight + nTypeHeight + 10);
	
	hdwp = Window(getDlgItem(IDC_TYPE)).deferWindowPos(hdwp, 0,
		rectType.left, rect.bottom - nButtonHeight - nTypeHeight - 10,
		0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
	hdwp = Window(getDlgItem(IDC_SEQUENTIAL)).deferWindowPos(hdwp, 0,
		rectSequential.left, rect.bottom - nButtonHeight - nTypeHeight - 10 + (rectSequential.top - rectType.top),
		0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
	hdwp = Window(getDlgItem(IDC_PARALLEL)).deferWindowPos(hdwp, 0,
		rectParallel.left, rect.bottom - nButtonHeight - nTypeHeight - 10 + (rectParallel.top - rectType.top),
		0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
	hdwp = Window(getDlgItem(IDC_CONFIRM)).deferWindowPos(hdwp, 0,
		rectType.right + 5, rect.bottom - nButtonHeight - nTypeHeight,
		0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
	hdwp = Window(getDlgItem(IDC_DIALUP)).deferWindowPos(hdwp, 0,
		rect.right - (nButtonWidth + 5), rect.bottom - nButtonHeight - nTypeHeight - 10,
		0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
	
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


/****************************************************************************
 *
 * GoRoundEntryDialog
 *
 */

qm::GoRoundEntryDialog::GoRoundEntryDialog(GoRoundEntry* pEntry,
										   AccountManager* pAccountManager,
										   SyncFilterManager* pSyncFilterManager) :
	DefaultDialog(IDD_GOROUNDENTRY),
	pEntry_(pEntry),
	pAccountManager_(pAccountManager),
	pSyncFilterManager_(pSyncFilterManager)
{
}

qm::GoRoundEntryDialog::~GoRoundEntryDialog()
{
}

LRESULT qm::GoRoundEntryDialog::onCommand(WORD nCode,
										  WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_EDIT, onEdit)
		HANDLE_COMMAND_ID_CODE(IDC_ACCOUNT, CBN_EDITCHANGE, onAccountEditChange)
		HANDLE_COMMAND_ID_CODE(IDC_ACCOUNT, CBN_SELCHANGE, onAccountSelChange)
		HANDLE_COMMAND_ID_CODE(IDC_SELECTFOLDER, BN_CLICKED, onSelectFolderClicked)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::GoRoundEntryDialog::onInitDialog(HWND hwndFocus,
											 LPARAM lParam)
{
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	
	const AccountManager::AccountList& listAccount = pAccountManager_->getAccounts();
	for (AccountManager::AccountList::const_iterator it = listAccount.begin(); it != listAccount.end(); ++it) {
		Account* pAccount = *it;
		W2T(pAccount->getName(), ptszName);
		ComboBox_AddString(getDlgItem(IDC_ACCOUNT), ptszName);
	}
	setDlgItemText(IDC_ACCOUNT, pEntry_->getAccount());
	
	const WCHAR* pwszSubAccount = pEntry_->getSubAccount();
	if (pwszSubAccount) {
		setDlgItemText(IDC_SUBACCOUNT, pwszSubAccount);
	}
	else {
		wstring_ptr wstrUnspecified(loadString(hInst, IDS_UNSPECIFIED));
		setDlgItemText(IDC_SUBACCOUNT, wstrUnspecified.get());
	}
	
	const WCHAR* pwszFolder = pEntry_->getFolder();
	if (pwszFolder) {
		setDlgItemText(IDC_FOLDER, pwszFolder);
	}
	else {
		wstring_ptr wstrAll(loadString(hInst, IDS_ALLFOLDER));
		setDlgItemText(IDC_FOLDER, wstrAll.get());
	}
	
	if (pEntry_->isFlag(GoRoundEntry::FLAG_SELECTFOLDER))
		Button_SetCheck(getDlgItem(IDC_SELECTFOLDER), BST_CHECKED);
	
	const WCHAR* pwszFilter = pEntry_->getFilter();
	if (pwszFilter)
		setDlgItemText(IDC_SYNCFILTER, pwszFilter);
	sendDlgItemMessage(IDC_SYNCFILTER, CB_SETDROPPEDWIDTH, 150);
	
	if (pEntry_->isFlag(GoRoundEntry::FLAG_SEND) &&
		pEntry_->isFlag(GoRoundEntry::FLAG_RECEIVE))
		Button_SetCheck(getDlgItem(IDC_SENDRECEIVE), BST_CHECKED);
	else if (pEntry_->isFlag(GoRoundEntry::FLAG_SEND))
		Button_SetCheck(getDlgItem(IDC_SEND), BST_CHECKED);
	else if (pEntry_->isFlag(GoRoundEntry::FLAG_RECEIVE))
		Button_SetCheck(getDlgItem(IDC_RECEIVE), BST_CHECKED);
	
	if (pEntry_->getConnectReceiveBeforeSend() == GoRoundEntry::CRBS_TRUE)
		Button_SetCheck(getDlgItem(IDC_CONNECTRECEIVEBEFORESEND), BST_CHECKED);
	
	init(false);
	updateState();
	
	return TRUE;
}

LRESULT qm::GoRoundEntryDialog::onOk()
{
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	
	wstring_ptr wstrAccount(getDlgItemText(IDC_ACCOUNT));
	if (!wstrAccount.get())
		return 0;
	
	wstring_ptr wstrSubAccount(getDlgItemText(IDC_SUBACCOUNT));
	const WCHAR* pwszSubAccount = wstrSubAccount.get();
	wstring_ptr wstrUnspecified(loadString(hInst, IDS_UNSPECIFIED));
	wstring_ptr wstrDefault(loadString(hInst, IDS_DEFAULTSUBACCOUNT));
	if (wcscmp(pwszSubAccount, wstrUnspecified.get()) == 0)
		pwszSubAccount = 0;
	else if (wcscmp(pwszSubAccount, wstrDefault.get()) == 0)
		pwszSubAccount = L"";
	
	wstring_ptr wstrFolder(getDlgItemText(IDC_FOLDER));
	const WCHAR* pwszFolder = wstrFolder.get();
	wstring_ptr wstrAll(loadString(hInst, IDS_ALLFOLDER));
	if (wcscmp(pwszFolder, wstrAll.get()) == 0)
		pwszFolder = 0;
	std::auto_ptr<RegexPattern> pFolderPattern;
	if (pwszFolder) {
		pFolderPattern = RegexCompiler().compile(pwszFolder);
		if (!pFolderPattern.get())
			return 0;
	}
	
	unsigned int nFlags = 0;
	if (Button_GetCheck(getDlgItem(IDC_SELECTFOLDER)) == BST_CHECKED)
		nFlags |= GoRoundEntry::FLAG_SELECTFOLDER;
	if (Button_GetCheck(getDlgItem(IDC_SENDRECEIVE)) == BST_CHECKED)
		nFlags |= GoRoundEntry::FLAG_SEND | GoRoundEntry::FLAG_RECEIVE;
	else if (Button_GetCheck(getDlgItem(IDC_RECEIVE)) == BST_CHECKED)
		nFlags |= GoRoundEntry::FLAG_RECEIVE;
	else if (Button_GetCheck(getDlgItem(IDC_SEND)) == BST_CHECKED)
		nFlags |= GoRoundEntry::FLAG_SEND;
	
	wstring_ptr wstrFilter(getDlgItemText(IDC_SYNCFILTER));
	const WCHAR* pwszFilter = wstrFilter.get();
	if (!*pwszFilter)
		pwszFilter = 0;
	
	GoRoundEntry::ConnectReceiveBeforeSend crbs =
		Button_GetCheck(getDlgItem(IDC_CONNECTRECEIVEBEFORESEND)) == BST_CHECKED ?
		GoRoundEntry::CRBS_TRUE : GoRoundEntry::CRBS_NONE;
	
	pEntry_->setAccount(wstrAccount.get());
	pEntry_->setSubAccount(pwszSubAccount);
	pEntry_->setFolder(pwszFolder, pFolderPattern);
	pEntry_->setFlags(nFlags);
	pEntry_->setFilter(pwszFilter);
	pEntry_->setConnectReceiveBeforeSend(crbs);
	
	return DefaultDialog::onOk();
}

LRESULT qm::GoRoundEntryDialog::onEdit()
{
//	SyncFilterSetsDialog dialog(pSyncFilterManager_);
//	if (dialog.doModal(getHandle()) == IDOK)
//		updateFilter();
	return 0;
}

LRESULT qm::GoRoundEntryDialog::onAccountEditChange()
{
	updateState();
	return 0;
}

LRESULT qm::GoRoundEntryDialog::onAccountSelChange()
{
	postMessage(WM_COMMAND, MAKEWPARAM(IDC_ACCOUNT, CBN_EDITCHANGE));
	return 0;
}

LRESULT qm::GoRoundEntryDialog::onSelectFolderClicked()
{
	updateState();
	return 0;
}

void qm::GoRoundEntryDialog::updateState()
{
	Account* pAccount = 0;
	
	wstring_ptr wstrAccount(getDlgItemText(IDC_ACCOUNT));
	if (wstrAccount.get())
		pAccount = pAccountManager_->getAccount(wstrAccount.get());
	
	updateSubAccount(pAccount);
	updateFolder(pAccount);
	updateFilter();
	
	Window(getDlgItem(IDC_FOLDER)).enableWindow(
		Button_GetCheck(getDlgItem(IDC_SELECTFOLDER)) != BST_CHECKED);
	Window(getDlgItem(IDOK)).enableWindow(*wstrAccount.get() != L'\0');
}

void qm::GoRoundEntryDialog::updateSubAccount(Account* pAccount)
{
	HWND hwnd = getDlgItem(IDC_SUBACCOUNT);
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	
	wstring_ptr wstrSubAccount(getDlgItemText(IDC_SUBACCOUNT));
	
	ComboBox_ResetContent(hwnd);
	
	wstring_ptr wstrUnspecified(loadString(hInst, IDS_UNSPECIFIED));
	W2T(wstrUnspecified.get(), ptszUnspecified);
	ComboBox_AddString(hwnd, ptszUnspecified);
	
	if (pAccount) {
		Account::SubAccountList l(pAccount->getSubAccounts());
		std::sort(l.begin(), l.end(),
			binary_compose_f_gx_hy(
				string_equal<WCHAR>(),
				std::mem_fun(&SubAccount::getName),
				std::mem_fun(&SubAccount::getName)));
		for (Account::SubAccountList::const_iterator it = l.begin(); it != l.end(); ++it) {
			SubAccount* pSubAccount = *it;
			if (*pSubAccount->getName()) {
				W2T(pSubAccount->getName(), ptszName);
				ComboBox_AddString(hwnd, ptszName);
			}
			else {
				wstring_ptr wstrDefault(loadString(hInst, IDS_DEFAULTSUBACCOUNT));
				W2T(wstrDefault.get(), ptszDefault);
				ComboBox_AddString(hwnd, ptszDefault);
			}
		}
	}
	
	setDlgItemText(IDC_SUBACCOUNT, wstrSubAccount.get());
}

void qm::GoRoundEntryDialog::updateFolder(Account* pAccount)
{
	HWND hwnd = getDlgItem(IDC_FOLDER);
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	
	wstring_ptr wstrFolder(getDlgItemText(IDC_FOLDER));
	
	ComboBox_ResetContent(hwnd);
	
	wstring_ptr wstrAll(loadString(hInst, IDS_ALLFOLDER));
	W2T(wstrAll.get(), ptszAll);
	ComboBox_AddString(hwnd, ptszAll);
	
	if (pAccount) {
		Account::FolderList l(pAccount->getFolders());
		std::sort(l.begin(), l.end(), FolderLess());
		for (Account::FolderList::const_iterator it = l.begin(); it != l.end(); ++it) {
			Folder* pFolder = *it;
			
			wstring_ptr wstrName(pFolder->getFullName());
			W2T(wstrName.get(), ptszName);
			ComboBox_AddString(hwnd, ptszName);
		}
	}
	
	setDlgItemText(IDC_FOLDER, wstrFolder.get());
}

void qm::GoRoundEntryDialog::updateFilter()
{
	HWND hwnd = getDlgItem(IDC_SYNCFILTER);
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	
	wstring_ptr wstrFilter(getDlgItemText(IDC_SYNCFILTER));
	
	ComboBox_ResetContent(hwnd);
	
	ComboBox_AddString(hwnd, _T(""));
	const SyncFilterManager::FilterSetList& l = pSyncFilterManager_->getFilterSets();
	for (SyncFilterManager::FilterSetList::const_iterator it = l.begin(); it != l.end(); ++it) {
		SyncFilterSet* pSet = *it;
		W2T(pSet->getName(), ptszName);
		ComboBox_AddString(hwnd, ptszName);
	}
	
	setDlgItemText(IDC_SYNCFILTER, wstrFilter.get());
}


/****************************************************************************
 *
 * GoRoundDialupDialog
 *
 */

qm::GoRoundDialupDialog::GoRoundDialupDialog(GoRoundDialup* pDialup,
											 bool bNoDialup) :
	DefaultDialog(IDD_GOROUNDDIALUP),
	pDialup_(pDialup),
	bNoDialup_(bNoDialup)
{
}

qm::GoRoundDialupDialog::~GoRoundDialupDialog()
{
}

bool qm::GoRoundDialupDialog::isNoDialup() const
{
	return bNoDialup_;
}

LRESULT qm::GoRoundDialupDialog::onCommand(WORD nCode,
										   WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID_RANGE(IDC_NEVER, IDC_CONNECT, onTypeSelect)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::GoRoundDialupDialog::onInitDialog(HWND hwndFocus,
											  LPARAM lParam)
{
	if (bNoDialup_)
		Button_SetCheck(getDlgItem(IDC_NEVER), BST_CHECKED);
	else if (pDialup_->isFlag(GoRoundDialup::FLAG_WHENEVERNOTCONNECTED))
		Button_SetCheck(getDlgItem(IDC_WHENEVERNOTCONNECTED), BST_CHECKED);
	else
		Button_SetCheck(getDlgItem(IDC_CONNECT), BST_CHECKED);
	
	if (pDialup_->getName())
		setDlgItemText(IDC_ENTRY, pDialup_->getName());
	if (pDialup_->getDialFrom())
		setDlgItemText(IDC_DIALFROM, pDialup_->getDialFrom());
	Button_SetCheck(getDlgItem(IDC_SHOWDIALOG),
		pDialup_->isFlag(GoRoundDialup::FLAG_SHOWDIALOG) ? BST_CHECKED : BST_UNCHECKED);
	setDlgItemInt(IDC_WAITBEFOREDISCONNECT, pDialup_->getDisconnectWait());
	
	RasConnection::EntryList listEntry;
	StringListFree<RasConnection::EntryList> free(listEntry);
	RasConnection::getEntries(&listEntry);
	for (RasConnection::EntryList::const_iterator it = listEntry.begin(); it != listEntry.end(); ++it) {
		W2T(*it, ptszEntry);
		ComboBox_AddString(getDlgItem(IDC_ENTRY), ptszEntry);
	}
	
	init(false);
	updateState();
	
	return TRUE;
}

LRESULT qm::GoRoundDialupDialog::onOk()
{
	bNoDialup_ = Button_GetCheck(getDlgItem(IDC_NEVER)) == BST_CHECKED;
	if (!bNoDialup_) {
		wstring_ptr wstrEntry(getDlgItemText(IDC_ENTRY));
		pDialup_->setName(*wstrEntry.get() ? wstrEntry.get() : 0);
		
		wstring_ptr wstrDialFrom(getDlgItemText(IDC_DIALFROM));
		pDialup_->setDialFrom(*wstrDialFrom.get() ? wstrDialFrom.get() : 0);
		
		unsigned int nFlags = 0;
		if (Button_GetCheck(getDlgItem(IDC_WHENEVERNOTCONNECTED)) == BST_CHECKED)
			nFlags |= GoRoundDialup::FLAG_WHENEVERNOTCONNECTED;
		if (Button_GetCheck(getDlgItem(IDC_SHOWDIALOG)) == BST_CHECKED)
			nFlags |= GoRoundDialup::FLAG_SHOWDIALOG;
		pDialup_->setFlags(nFlags);
		
		pDialup_->setDisconnectWait(getDlgItemInt(IDC_WAITBEFOREDISCONNECT));
	}
	return DefaultDialog::onOk();
}

LRESULT qm::GoRoundDialupDialog::onTypeSelect(UINT nId)
{
	updateState();
	return 0;
}

void qm::GoRoundDialupDialog::updateState()
{
	bool bEnable = Button_GetCheck(getDlgItem(IDC_NEVER)) != BST_CHECKED;
	
	UINT nIds[] = {
		IDC_ENTRY,
		IDC_SHOWDIALOG,
		IDC_DIALFROM,
		IDC_WAITBEFOREDISCONNECT
	};
	for (int n = 0; n < countof(nIds); ++n)
		Window(getDlgItem(nIds[n])).enableWindow(bEnable);
}


/****************************************************************************
*
* SignaturesDialog
*
*/

qm::SignaturesDialog::SignaturesDialog(SignatureManager* pSignatureManager,
									   AccountManager* pAccountManager,
									   Profile* pProfile) :
	AbstractListDialog<Signature, SignatureManager::SignatureList>(IDD_SIGNATURES, IDC_SIGNATURES, false),
	pSignatureManager_(pSignatureManager),
	pAccountManager_(pAccountManager),
	pProfile_(pProfile)
{
	const SignatureManager::SignatureList& l = pSignatureManager_->getSignatures();
	SignatureManager::SignatureList& list = getList();
	list.reserve(l.size());
	for (SignatureManager::SignatureList::const_iterator it = l.begin(); it != l.end(); ++it)
		list.push_back(new Signature(**it));
}

qm::SignaturesDialog::~SignaturesDialog()
{
}

INT_PTR qm::SignaturesDialog::dialogProc(UINT uMsg,
										 WPARAM wParam,
										 LPARAM lParam)
{
	BEGIN_DIALOG_HANDLER()
		HANDLE_SIZE()
	END_DIALOG_HANDLER()
	return DefaultDialog::dialogProc(uMsg, wParam, lParam);
}

wstring_ptr qm::SignaturesDialog::getLabel(const Signature* p) const
{
	StringBuffer<WSTRING> buf(p->getName());
	if (p->getAccount()) {
		buf.append(L" [");
		buf.append(p->getAccount());
		buf.append(L"]");
	}
	if (p->isDefault())
		buf.append(L" *");
	return buf.getString();
}

std::auto_ptr<Signature> qm::SignaturesDialog::create() const
{
	std::auto_ptr<Signature> pSignature(new Signature());
	SignatureDialog dialog(pSignature.get(), pAccountManager_, pProfile_);
	if (dialog.doModal(getParentPopup()) != IDOK)
		return std::auto_ptr<Signature>();
	return pSignature;
}

Signature* qm::SignaturesDialog::edit(Signature* p) const
{
	SignatureDialog dialog(p, pAccountManager_, pProfile_);
	if (dialog.doModal(getParentPopup()) != IDOK)
		return 0;
	return p;
}

bool qm::SignaturesDialog::save(OptionDialogContext* pContext)
{
	pSignatureManager_->setSignatures(getList());
	return pSignatureManager_->save();
}

LRESULT qm::SignaturesDialog::onSize(UINT nFlags,
									 int cx,
									 int cy)
{
	layout();
	return DefaultDialog::onSize(nFlags, cx, cy);
}

void qm::SignaturesDialog::layout()
{
#ifndef _WIN32_WCE_PSPC
	LayoutUtil::layout(this, IDC_SIGNATURES);
#endif
}


/****************************************************************************
 *
 * SignatureDialog
 *
 */

qm::SignatureDialog::SignatureDialog(Signature* pSignature,
									 AccountManager* pAccountManager,
									 Profile* pProfile) :
	DefaultDialog(IDD_SIGNATURE),
	pSignature_(pSignature),
	pAccountManager_(pAccountManager),
	pProfile_(pProfile)
{
}

qm::SignatureDialog::~SignatureDialog()
{
}

INT_PTR qm::SignatureDialog::dialogProc(UINT uMsg,
										WPARAM wParam,
										LPARAM lParam)
{
	BEGIN_DIALOG_HANDLER()
		HANDLE_SIZE()
	END_DIALOG_HANDLER()
	return DefaultDialog::dialogProc(uMsg, wParam, lParam);
}

LRESULT qm::SignatureDialog::onCommand(WORD nCode,
									   WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID_CODE(IDC_NAME, EN_CHANGE, onNameChange)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::SignatureDialog::onDestroy()
{
#ifndef _WIN32_WCE
	RECT rect;
	getWindowRect(&rect);
	pProfile_->setInt(L"SignatureDialog", L"Width", rect.right - rect.left);
	pProfile_->setInt(L"SignatureDialog", L"Height", rect.bottom - rect.top);
#endif
	return DefaultDialog::onDestroy();
}

LRESULT qm::SignatureDialog::onInitDialog(HWND hwndFocus,
										  LPARAM lParam)
{
	setDlgItemText(IDC_NAME, pSignature_->getName());
	
	const AccountManager::AccountList& l = pAccountManager_->getAccounts();
	for (AccountManager::AccountList::const_iterator it = l.begin(); it != l.end(); ++it) {
		W2T((*it)->getName(), ptszName);
		ComboBox_AddString(getDlgItem(IDC_ACCOUNT), ptszName);
	}
	if (pSignature_->getAccount())
		setDlgItemText(IDC_ACCOUNT, pSignature_->getAccount());
	
	if (pSignature_->isDefault())
		Button_SetCheck(getDlgItem(IDC_DEFAULT), BST_CHECKED);
	
	wstring_ptr wstrSignature(Util::convertLFtoCRLF(pSignature_->getSignature()));
	setDlgItemText(IDC_SIGNATURE, wstrSignature.get());
	
#ifdef _WIN32_WCE
	RECT rectWorkArea;
	::SystemParametersInfo(SPI_GETWORKAREA, 0, &rectWorkArea, 0);
	int nWidth = rectWorkArea.right - rectWorkArea.left;
	int nHeight = rectWorkArea.bottom - rectWorkArea.top;
	setWindowPos(0, 0, 0, nWidth, nHeight, SWP_NOZORDER | SWP_NOACTIVATE);
#else
	int nWidth = pProfile_->getInt(L"SignatureDialog", L"Width", 620);
	int nHeight = pProfile_->getInt(L"SignatureDialog", L"Height", 450);
	setWindowPos(0, 0, 0, nWidth, nHeight,
		SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
#endif
	
	init(false);
	updateState();
	
	return TRUE;
}

LRESULT qm::SignatureDialog::onOk()
{
	wstring_ptr wstrName(getDlgItemText(IDC_NAME));
	
	wstring_ptr wstrAccount(getDlgItemText(IDC_ACCOUNT));
	const WCHAR* pwszAccount = 0;
	std::auto_ptr<RegexPattern> pAccount;
	if (*wstrAccount.get()) {
		pAccount = RegexCompiler().compile(wstrAccount.get());
		if (!pAccount.get())
			return 0;
		pwszAccount = wstrAccount.get();
	}
	
	bool bDefault = Button_GetCheck(getDlgItem(IDC_DEFAULT)) == BST_CHECKED;
	
	wstring_ptr wstrSignature(getDlgItemText(IDC_SIGNATURE));
	wstrSignature = Util::convertCRLFtoLF(wstrSignature.get());
	
	pSignature_->setName(wstrName.get());
	pSignature_->setAccount(pwszAccount, pAccount);
	pSignature_->setDefault(bDefault);
	pSignature_->setSignature(wstrSignature.get());
	
	return DefaultDialog::onOk();
}

LRESULT qm::SignatureDialog::onSize(UINT nFlags,
									int cx,
									int cy)
{
	layout();
	return DefaultDialog::onSize(nFlags, cx, cy);
}

LRESULT qm::SignatureDialog::onNameChange()
{
	updateState();
	return 0;
}

void qm::SignatureDialog::updateState()
{
	Window(getDlgItem(IDOK)).enableWindow(
		Window(getDlgItem(IDC_NAME)).getWindowTextLength() != 0);
}

void qm::SignatureDialog::layout()
{
#ifndef _WIN32_WCE_PSPC
	RECT rect;
	getClientRect(&rect);
	
#ifndef _WIN32_WCE
	RECT rectButton;
	Window(getDlgItem(IDOK)).getWindowRect(&rectButton);
	int nButtonWidth = rectButton.right - rectButton.left;
	int nButtonHeight = rectButton.bottom - rectButton.top;
#else
	int nButtonHeight = -5;
#endif
	
	RECT rectName;
	Window(getDlgItem(IDC_NAME)).getWindowRect(&rectName);
	screenToClient(&rectName);
	
	RECT rectText;
	Window(getDlgItem(IDC_SIGNATURE)).getWindowRect(&rectText);
	screenToClient(&rectText);
	
	HDWP hdwp = beginDeferWindowPos(6);
	hdwp = Window(getDlgItem(IDC_NAME)).deferWindowPos(hdwp, 0, 0, 0,
		rect.right - rectName.left - 5, rectName.bottom - rectName.top,
		SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
	hdwp = Window(getDlgItem(IDC_ACCOUNT)).deferWindowPos(hdwp, 0, 0, 0,
		rect.right - rectName.left - 5, rectName.bottom - rectName.top,
		SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
	hdwp = Window(getDlgItem(IDC_SIGNATURE)).deferWindowPos(hdwp, 0, 0, 0,
		rect.right - rectText.left - 5, rect.bottom - rectText.top - nButtonHeight - 10,
		SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
	
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


/****************************************************************************
*
* SyncFilterSetsDialog
*
*/

qm::SyncFilterSetsDialog::SyncFilterSetsDialog(SyncFilterManager* pSyncFilterManager,
											   Profile* pProfile) :
	AbstractListDialog<SyncFilterSet, SyncFilterManager::FilterSetList>(IDD_SYNCFILTERSETS, IDC_FILTERSETS, false),
	pSyncFilterManager_(pSyncFilterManager),
	pProfile_(pProfile)
{
	const SyncFilterManager::FilterSetList& l = pSyncFilterManager->getFilterSets();
	SyncFilterManager::FilterSetList& list = getList();
	list.reserve(l.size());
	for (SyncFilterManager::FilterSetList::const_iterator it = l.begin(); it != l.end(); ++it)
		list.push_back(new SyncFilterSet(**it));
}

qm::SyncFilterSetsDialog::~SyncFilterSetsDialog()
{
}

INT_PTR qm::SyncFilterSetsDialog::dialogProc(UINT uMsg,
											 WPARAM wParam,
											 LPARAM lParam)
{
	BEGIN_DIALOG_HANDLER()
		HANDLE_SIZE()
	END_DIALOG_HANDLER()
	return DefaultDialog::dialogProc(uMsg, wParam, lParam);
}

wstring_ptr qm::SyncFilterSetsDialog::getLabel(const SyncFilterSet* p) const
{
	return allocWString(p->getName());
}

std::auto_ptr<SyncFilterSet> qm::SyncFilterSetsDialog::create() const
{
	std::auto_ptr<SyncFilterSet> pFilterSet(new SyncFilterSet());
	SyncFiltersDialog dialog(pFilterSet.get(), pProfile_);
	if (dialog.doModal(getParentPopup()) != IDOK)
		return std::auto_ptr<SyncFilterSet>();
	return pFilterSet;
}

SyncFilterSet* qm::SyncFilterSetsDialog::edit(SyncFilterSet* p) const
{
	SyncFiltersDialog dialog(p, pProfile_);
	if (dialog.doModal(getParentPopup()) != IDOK)
		return 0;
	return p;
}

bool qm::SyncFilterSetsDialog::save(OptionDialogContext* pContext)
{
	pSyncFilterManager_->setFilterSets(getList());
	return pSyncFilterManager_->save();
}

LRESULT qm::SyncFilterSetsDialog::onSize(UINT nFlags,
										 int cx,
										 int cy)
{
	layout();
	return DefaultDialog::onSize(nFlags, cx, cy);
}

void qm::SyncFilterSetsDialog::layout()
{
#ifndef _WIN32_WCE_PSPC
	LayoutUtil::layout(this, IDC_FILTERSETS);
#endif
}


/****************************************************************************
*
* SyncFiltersDialog
*
*/

qm::SyncFiltersDialog::SyncFiltersDialog(SyncFilterSet* pSyncFilterSet,
										 Profile* pProfile) :
	AbstractListDialog<SyncFilter, SyncFilterSet::FilterList>(IDD_SYNCFILTERS, IDC_FILTERS, true),
	pSyncFilterSet_(pSyncFilterSet),
	pProfile_(pProfile)
{
	const SyncFilterSet::FilterList& l = pSyncFilterSet->getFilters();
	SyncFilterSet::FilterList& list = getList();
	list.reserve(l.size());
	for (SyncFilterSet::FilterList::const_iterator it = l.begin(); it != l.end(); ++it)
		list.push_back(new SyncFilter(**it));
}

qm::SyncFiltersDialog::~SyncFiltersDialog()
{
}

INT_PTR qm::SyncFiltersDialog::dialogProc(UINT uMsg,
										  WPARAM wParam,
										  LPARAM lParam)
{
	BEGIN_DIALOG_HANDLER()
		HANDLE_SIZE()
	END_DIALOG_HANDLER()
	return AbstractListDialog<SyncFilter, SyncFilterSet::FilterList>::dialogProc(uMsg, wParam, lParam);
}

LRESULT qm::SyncFiltersDialog::onCommand(WORD nCode,
										 WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID_CODE(IDC_NAME, EN_CHANGE, onNameChange)
	END_COMMAND_HANDLER()
	return AbstractListDialog<SyncFilter, SyncFilterSet::FilterList>::onCommand(nCode, nId);
}

LRESULT qm::SyncFiltersDialog::onDestroy()
{
#ifndef _WIN32_WCE
	RECT rect;
	getWindowRect(&rect);
	pProfile_->setInt(L"SyncFiltersDialog", L"Width", rect.right - rect.left);
	pProfile_->setInt(L"SyncFiltersDialog", L"Height", rect.bottom - rect.top);
#endif
	
	return AbstractListDialog<SyncFilter, SyncFilterSet::FilterList>::onDestroy();
}

LRESULT qm::SyncFiltersDialog::onInitDialog(HWND hwndFocus,
											LPARAM lParam)
{
	setDlgItemText(IDC_NAME, pSyncFilterSet_->getName());
	
#ifdef _WIN32_WCE
	RECT rectWorkArea;
	::SystemParametersInfo(SPI_GETWORKAREA, 0, &rectWorkArea, 0);
	int nWidth = rectWorkArea.right - rectWorkArea.left;
	int nHeight = rectWorkArea.bottom - rectWorkArea.top;
	setWindowPos(0, 0, 0, nWidth, nHeight, SWP_NOZORDER | SWP_NOACTIVATE);
#else
	int nWidth = pProfile_->getInt(L"SyncFiltersDialog", L"Width", 620);
	int nHeight = pProfile_->getInt(L"SyncFiltersDialog", L"Height", 450);
	setWindowPos(0, 0, 0, nWidth, nHeight,
		SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
#endif
	
	return AbstractListDialog<SyncFilter, SyncFilterSet::FilterList>::onInitDialog(hwndFocus, lParam);
}

LRESULT qm::SyncFiltersDialog::onOk()
{
	wstring_ptr wstrName(getDlgItemText(IDC_NAME));
	
	pSyncFilterSet_->setName(wstrName.get());
	pSyncFilterSet_->setFilters(getList());
	
	return AbstractListDialog<SyncFilter, SyncFilterSet::FilterList>::onOk();
}

wstring_ptr qm::SyncFiltersDialog::getLabel(const SyncFilter* p) const
{
	StringBuffer<WSTRING> buf;
	
	const WCHAR* pwszDescription = p->getDescription();
	if (pwszDescription) {
		buf.append(pwszDescription);
		buf.append(L": ");
	}
	
	const WCHAR* pwszFolder = p->getFolder();
	if (pwszFolder) {
		buf.append(L'[');
		buf.append(pwszFolder);
		buf.append(L"] ");
	}
	
	const SyncFilter::ActionList& listAction = p->getActions();
	if (!listAction.empty()) {
		SyncFilterAction* pAction = listAction.front();
		buf.append(pAction->getName());
		const SyncFilterAction::ParamList& listParam = pAction->getParams();
		if (!listParam.empty()) {
			buf.append(L" (");
			for (SyncFilterAction::ParamList::const_iterator it = listParam.begin(); it != listParam.end(); ++it) {
				if (it != listParam.begin())
					buf.append(L',');
				buf.append((*it).first);
				buf.append(L'=');
				buf.append((*it).second);
			}
			buf.append(L')');
		}
	}
	buf.append(L" <- ");
	
	const Macro* pMacro = p->getCondition();
	std::auto_ptr<ConditionList> pConditionList(ConditionFactory::getInstance().parse(pMacro));
	if (pConditionList.get())
		buf.append(pConditionList->getDescription(true).get());
	else
		buf.append(pMacro->getString().get());
	
	return buf.getString();
}

std::auto_ptr<SyncFilter> qm::SyncFiltersDialog::create() const
{
	std::auto_ptr<SyncFilter> pFilter(new SyncFilter());
	SyncFilterDialog dialog(pFilter.get());
	if (dialog.doModal(getHandle()) != IDOK)
		return std::auto_ptr<SyncFilter>();
	return pFilter;
}

SyncFilter* qm::SyncFiltersDialog::edit(SyncFilter* p) const
{
	SyncFilterDialog dialog(p);
	if (dialog.doModal(getHandle()) != IDOK)
		return 0;
	return p;
}

void qm::SyncFiltersDialog::updateState()
{
	AbstractListDialog<SyncFilter, SyncFilterSet::FilterList>::updateState();
	
	Window(getDlgItem(IDOK)).enableWindow(
		Window(getDlgItem(IDC_NAME)).getWindowTextLength() != 0);
}

LRESULT qm::SyncFiltersDialog::onSize(UINT nFlags,
									  int cx,
									  int cy)
{
	layout();
	return AbstractListDialog<SyncFilter, SyncFilterSet::FilterList>::onSize(nFlags, cx, cy);
}

LRESULT qm::SyncFiltersDialog::onNameChange()
{
	updateState();
	return 0;
}

void qm::SyncFiltersDialog::layout()
{
#ifndef _WIN32_WCE_PSPC
	RECT rect;
	getClientRect(&rect);
	
	RECT rectName;
	Window(getDlgItem(IDC_NAME)).getWindowRect(&rectName);
	screenToClient(&rectName);
	
#ifndef _WIN32_WCE
	RECT rectButton;
	Window(getDlgItem(IDOK)).getWindowRect(&rectButton);
	int nButtonWidth = rectButton.right - rectButton.left;
	int nButtonHeight = rectButton.bottom - rectButton.top;
#else
	int nButtonHeight = -5;
#endif
	
	HDWP hdwp = beginDeferWindowPos(10);
	
	hdwp = Window(getDlgItem(IDC_NAME)).deferWindowPos(hdwp, 0, 0, 0,
		rect.right - rectName.left - 5, rectName.bottom - rectName.top,
		SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
	
	hdwp = LayoutUtil::layout(this, IDC_FILTERS, hdwp,
		rectName.bottom, nButtonHeight + 5);
	
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


/****************************************************************************
 *
 * SyncFilterDialog
 *
 */

qm::SyncFilterDialog::SyncFilterDialog(SyncFilter* pSyncFilter) :
	DefaultDialog(IDD_SYNCFILTER),
	pSyncFilter_(pSyncFilter)
{
}

qm::SyncFilterDialog::~SyncFilterDialog()
{
}

LRESULT qm::SyncFilterDialog::onCommand(WORD nCode,
										WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_EDIT, onEdit)
		HANDLE_COMMAND_ID_CODE(IDC_ACTION, CBN_SELCHANGE, onActionSelChange)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qm::SyncFilterDialog::onInitDialog(HWND hwndFocus,
										   LPARAM lParam)
{
	wstring_ptr wstrCondition(pSyncFilter_->getCondition()->getString());
	setDlgItemText(IDC_CONDITION, wstrCondition.get());
	
	setDlgItemText(IDC_FOLDER, pSyncFilter_->getFolder());
	
	const TCHAR* ptszActions[] = {
		_T("Download (POP3)"),
		_T("Download (IMAP4)"),
		_T("Download (NNTP)"),
		_T("Delete (POP3, IMAP4)"),
		_T("Ignore (POP3, NNTP)")
	};
	for (int n = 0; n < countof(ptszActions); ++n)
		ComboBox_AddString(getDlgItem(IDC_ACTION), ptszActions[n]);
	
	const TCHAR* ptszTypes[] = {
		_T("All"),
		_T("Text"),
		_T("Html"),
		_T("Header")
	};
	for (int n = 0; n < countof(ptszTypes); ++n)
		ComboBox_AddString(getDlgItem(IDC_TYPE), ptszTypes[n]);
	
	int nAction = 0;
	int nMaxLine = 0;
	int nType = 0;
	
	const SyncFilter::ActionList& listAction = pSyncFilter_->getActions();
	if (!listAction.empty()) {
		const SyncFilterAction* pAction = listAction.front();
		const WCHAR* pwszAction = pAction->getName();
		if (wcscmp(pwszAction, L"download") == 0) {
			const WCHAR* pwszProtocol = L"pop3";
			if (pAction->getParam(L"line"))
				pwszProtocol = L"pop3";
			else if (pAction->getParam(L"type"))
				pwszProtocol = L"imap4";
			else
				pwszProtocol = L"nntp";
			
			if (wcscmp(pwszProtocol, L"imap4") == 0) {
				nAction = 1;
				const WCHAR* pwszType = pAction->getParam(L"type");
				if (pwszType) {
					const WCHAR* pwszTypes[] = {
						L"all",
						L"text",
						L"html",
						L"header"
					};
					for (int n = 0; n < countof(pwszTypes); ++n) {
						if (wcscmp(pwszType, pwszTypes[n]) == 0) {
							nType = n;
							break;
						}
					}
				}
			}
			else if (wcscmp(pwszProtocol, L"nntp") == 0) {
				nAction = 2;
			}
			else {
				const WCHAR* pwszLine = pAction->getParam(L"line");
				if (pwszLine) {
					WCHAR* pEnd = 0;
					long nLine = wcstol(pwszLine, &pEnd, 10);
					if (!*pEnd)
						nMaxLine = nLine;
				}
			}
		}
		else if (wcscmp(pwszAction, L"delete") == 0) {
			nAction = 3;
		}
		else if (wcscmp(pwszAction, L"ignore") == 0) {
			nAction = 4;
		}
	}
	
	ComboBox_SetCurSel(getDlgItem(IDC_ACTION), nAction);
	setDlgItemInt(IDC_MAXLINE, nMaxLine);
	ComboBox_SetCurSel(getDlgItem(IDC_TYPE), nType);
	
	const WCHAR* pwszDescription = pSyncFilter_->getDescription();
	if (pwszDescription)
		setDlgItemText(IDC_DESCRIPTION, pwszDescription);
	
	init(false);
	updateState();
	
	return TRUE;
}

LRESULT qm::SyncFilterDialog::onOk()
{
	wstring_ptr wstrCondition(getDlgItemText(IDC_CONDITION));
	std::auto_ptr<Macro> pCondition(MacroParser().parse(wstrCondition.get()));
	if (!pCondition.get()) {
		messageBox(Application::getApplication().getResourceHandle(),
			IDS_ERROR_INVALIDMACRO, MB_OK | MB_ICONERROR, getHandle());
		return 0;
	}
	
	wstring_ptr wstrFolder(getDlgItemText(IDC_FOLDER));
	const WCHAR* pwszFolder = 0;
	std::auto_ptr<RegexPattern> pFolder;
	if (*wstrFolder.get()) {
		pFolder = RegexCompiler().compile(wstrFolder.get());
		if (!pFolder.get()) {
			// TODO MSG
			return 0;
		}
		pwszFolder = wstrFolder.get();
	}
	
	int nAction = ComboBox_GetCurSel(getDlgItem(IDC_ACTION));
	if (nAction == CB_ERR) {
		// TODO MSG
		return 0;
	}
	
	const WCHAR* pwszName = 0;
	switch (nAction) {
	case 0:
	case 1:
	case 2:
		pwszName = L"download";
		break;
	case 3:
		pwszName = L"delete";
		break;
	case 4:
		pwszName = L"ignore";
		break;
	default:
		assert(false);
		return 0;
	}
	
	std::auto_ptr<SyncFilterAction> pAction(new SyncFilterAction(pwszName));
	
	switch (nAction) {
	case 0:
		{
			int nLine = getDlgItemInt(IDC_MAXLINE);
			const size_t nLen = 32;
			wstring_ptr wstrLine(allocWString(nLen));
			_snwprintf(wstrLine.get(), nLen, L"%d", nLine);
			pAction->addParam(allocWString(L"line"), wstrLine);
		}
		break;
	case 1:
		{
			int nType = ComboBox_GetCurSel(getDlgItem(IDC_TYPE));
			if (nType == CB_ERR) {
				// TODO MSG
				return 0;
			}
			const WCHAR* pwszTypes[] = {
				L"all",
				L"text",
				L"html",
				L"header"
			};
			pAction->addParam(allocWString(L"type"), allocWString(pwszTypes[nType]));
		}
		break;
	default:
		break;
	}
	
	SyncFilter::ActionList listAction(1, pAction.get());
	pAction.release();
	
	const SyncFilter::ActionList& l = pSyncFilter_->getActions();
	listAction.reserve(l.size());
	for (SyncFilter::ActionList::size_type n = 1; n < l.size(); ++n)
		listAction.push_back(new SyncFilterAction(*l[n]));
	
	wstring_ptr wstrDescription(getDlgItemText(IDC_DESCRIPTION));
	
	pSyncFilter_->setFolder(pwszFolder, pFolder);
	pSyncFilter_->setCondition(pCondition);
	pSyncFilter_->setActions(listAction);
	pSyncFilter_->setDescription(wstrDescription.get());
	
	return DefaultDialog::onOk();
}

LRESULT qm::SyncFilterDialog::onEdit()
{
	wstring_ptr wstrCondition(getDlgItemText(IDC_CONDITION));
	ConditionsDialog dialog(wstrCondition.get());
	if (dialog.doModal(getHandle()) == IDOK)
		setDlgItemText(IDC_CONDITION, dialog.getCondition());
	return 0;
}

LRESULT qm::SyncFilterDialog::onActionSelChange()
{
	updateState();
	return 0;
}

void qm::SyncFilterDialog::updateState()
{
	int nItem = ComboBox_GetCurSel(getDlgItem(IDC_ACTION));
	Window(getDlgItem(IDC_MAXLINELABEL)).showWindow(nItem == 0 ? SW_SHOW : SW_HIDE);
	Window(getDlgItem(IDC_MAXLINE)).showWindow(nItem == 0 ? SW_SHOW : SW_HIDE);
	Window(getDlgItem(IDC_TYPELABEL)).showWindow(nItem == 1 ? SW_SHOW : SW_HIDE);
	Window(getDlgItem(IDC_TYPE)).showWindow(nItem == 1 ? SW_SHOW : SW_HIDE);
}


/****************************************************************************
 *
 * LayoutUtil
 *
 */

void qm::LayoutUtil::layout(Window* pWindow,
							UINT nId)
{
	HDWP hdwp = Window::beginDeferWindowPos(6);
	hdwp = layout(pWindow, nId, hdwp, 0, 0);
	Window::endDeferWindowPos(hdwp);
}

HDWP qm::LayoutUtil::layout(qs::Window* pWindow,
							UINT nId,
							HDWP hdwp,
							int nTopMargin,
							int nBottomMargin)
{
	RECT rect;
	pWindow->getClientRect(&rect);
	
	RECT rectButton;
	Window(pWindow->getDlgItem(IDC_ADD)).getWindowRect(&rectButton);
	
	int nButtonWidth = rectButton.right - rectButton.left;
	int nButtonHeight = rectButton.bottom - rectButton.top;
	
	hdwp = Window(pWindow->getDlgItem(nId)).deferWindowPos(
		hdwp, 0, 5, nTopMargin + 5, rect.right - nButtonWidth - 15,
		rect.bottom - nTopMargin - nBottomMargin - 10,
		SWP_NOZORDER | SWP_NOACTIVATE);
	
	UINT nIds[] = {
		IDC_ADD,
		IDC_REMOVE,
		IDC_EDIT,
		IDC_UP,
		IDC_DOWN
	};
	int nTop = nTopMargin + 5;
	for (int n = 0; n < countof(nIds); ++n) {
		hdwp = Window(pWindow->getDlgItem(nIds[n])).deferWindowPos(hdwp,
			0, rect.right - nButtonWidth - 5, nTop, nButtonWidth,
			nButtonHeight, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
		nTop += nButtonHeight + (n == 2 ? 5 : 2);
	}
	
	return hdwp;
}
