/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#pragma warning(disable:4786)

#include <qmapplication.h>
#include <qmdocument.h>
#include <qmfolder.h>
#include <qmpassword.h>
#include <qmuiutil.h>

#include <qsconv.h>
#include <qsdevicecontext.h>
#include <qsstream.h>
#include <qstextutil.h>
#include <qsuiutil.h>
#include <qswindow.h>

#include <tchar.h>
#include <urlmon.h>

#include "dialogs.h"
#include "resourceinc.h"
#include "uiutil.h"
#include "../main/main.h"
#include "../model/dataobject.h"
#include "../model/messagecontext.h"
#include "../model/tempfilecleaner.h"
#include "../model/uri.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * UIUtil
 *
 */

int qm::UIUtil::loadWindowPlacement(Profile* pProfile,
									const WCHAR* pwszSection,
									CREATESTRUCT* pCreateStruct)
{
	assert(pProfile);
	assert(pwszSection);
	assert(pCreateStruct);
	
#ifdef _WIN32_WCE
	return SW_SHOW;
#else
	struct {
		const WCHAR* pwszKey_;
		int n_;
	} items[] = {
		{ L"Left",		0 },
		{ L"Top",		0 },
		{ L"Width",		0 },
		{ L"Height",	0 }
	};
	for (int n = 0; n < countof(items); ++n)
		items[n].n_ = pProfile->getInt(pwszSection, items[n].pwszKey_);
	
	int nShow = SW_SHOWNORMAL;
	if (items[2].n_ != 0 && items[3].n_ != 0) {
		int nLeftOffset = 0;
		int nTopOffset = 0;
#if WINVER >= 0x500
		RECT rect = {
			items[0].n_,
			items[1].n_,
			items[0].n_ + items[2].n_,
			items[1].n_ + items[3].n_
		};
		HMONITOR hMonitor = ::MonitorFromRect(&rect, MONITOR_DEFAULTTONEAREST);
		MONITORINFO info = { sizeof(info) };
		if (::GetMonitorInfo(hMonitor, &info)) {
			nLeftOffset = info.rcWork.left - info.rcMonitor.left;
			nTopOffset = info.rcWork.top - info.rcMonitor.top;
		}
#else
		RECT rect;
		::SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);
		nLeftOffset = rect.left;
		nTopOffset = rect.top;
#endif
		pCreateStruct->x = items[0].n_ + nLeftOffset;
		pCreateStruct->y = items[1].n_ + nTopOffset;
		pCreateStruct->cx = items[2].n_;
		pCreateStruct->cy = items[3].n_;
		
		nShow = pProfile->getInt(pwszSection, L"Show");
		if (nShow != SW_MAXIMIZE &&
			nShow != SW_MINIMIZE &&
			nShow != SW_SHOWNORMAL)
			nShow = SW_SHOWNORMAL;
	}
	return nShow;
#endif
}

void qm::UIUtil::saveWindowPlacement(HWND hwnd,
									 Profile* pProfile,
									 const WCHAR* pwszSection)
{
	assert(hwnd);
	assert(pProfile);
	assert(pwszSection);
	
#ifndef _WIN32_WCE
	WINDOWPLACEMENT wp = { sizeof(wp) };
	Window(hwnd).getWindowPlacement(&wp);
	
	const RECT& rect = wp.rcNormalPosition;
	struct {
		const WCHAR* pwszKey_;
		int n_;
	} items[] = {
		{ L"Left",		rect.left				},
		{ L"Top",		rect.top				},
		{ L"Width",		rect.right - rect.left	},
		{ L"Height",	rect.bottom - rect.top	}
	};
	for (int n = 0; n < countof(items); ++n)
		pProfile->setInt(pwszSection, items[n].pwszKey_, items[n].n_);
	
	int nShow = SW_SHOWNORMAL;
	switch (wp.showCmd) {
	case SW_MAXIMIZE:
//	case SW_SHOWMAXIMIZED:
		nShow = SW_MAXIMIZE;
		break;
	case SW_MINIMIZE:
	case SW_SHOWMINIMIZED:
		nShow = SW_MINIMIZE;
		break;
	}
	pProfile->setInt(pwszSection, L"Show", nShow);
#endif
}

void qm::UIUtil::loadEncodings(Profile* pProfile,
							   EncodingList* pList)
{
	assert(pProfile);
	
	wstring_ptr wstrEncodings(pProfile->getString(L"Global", L"Encodings"));
	parseEncodings(wstrEncodings.get(), pList);
}

void qm::UIUtil::parseEncodings(const WCHAR* pwszEncodings,
								EncodingList* pList)
{
	wstring_ptr wstrEncodings(allocWString(pwszEncodings));
	
	WCHAR* p = wcstok(wstrEncodings.get(), L" ");
	while (p) {
		wstring_ptr wstrEncoding(allocWString(p));
		pList->push_back(wstrEncoding.get());
		wstrEncoding.release();
		p = wcstok(0, L" ");
	}
}

wstring_ptr qm::UIUtil::formatMenu(const WCHAR* pwszText,
								   int* pnMnemonic)
{
	assert(pwszText);
	assert(pnMnemonic);
	
	bool bAddMnemonic = false;
	StringBuffer<WSTRING> buf;
	while (*pwszText) {
		WCHAR c = *pwszText;
		if (!bAddMnemonic && ((L'A' <= c && c <= L'Z') || (L'a' <= c && c <= L'z'))) {
			buf.append(L'&');
			bAddMnemonic = true;
		}
		if (c == L'&')
			buf.append(L'&');
		buf.append(c);
		++pwszText;
	}
	if (!bAddMnemonic) {
		if (1 <= *pnMnemonic && *pnMnemonic <= 9) {
			buf.append(L"(&");
			buf.append(L'1' + (*pnMnemonic - 1));
			buf.append(L')');
		}
		++(*pnMnemonic);
	}
	
	return buf.getString();
}

bool qm::UIUtil::openURLWithWarning(const WCHAR* pwszURL,
									Profile* pProfile,
									HWND hwnd)
{
	assert(pwszURL);
	assert(pProfile);
	
	if (wcsncmp(pwszURL, L"file:", 5) == 0 ||
		(wcslen(pwszURL) > 2 && TextUtil::isDriveLetterChar(*pwszURL) &&
		*(pwszURL + 1) == L':' && *(pwszURL + 2) == L'\\') ||
		wcsncmp(pwszURL, L"\\\\", 2) == 0) {
		const WCHAR* pExt = wcsrchr(pwszURL, L'.');
		if (pExt) {
			wstring_ptr wstrExtensions(pProfile->getString(L"Global", L"WarnExtensions"));
			if (wcsstr(wstrExtensions.get(), pExt + 1)) {
				int nMsg = messageBox(getResourceHandle(), IDS_CONFIRM_OPENURL,
					MB_YESNO | MB_DEFBUTTON2 | MB_ICONWARNING, hwnd, 0, 0);
				if (nMsg != IDYES)
					return true;
			}
		}
	}
	
	return openURL(pwszURL, pProfile, hwnd);
}

bool qm::UIUtil::openURL(const WCHAR* pwszURL,
						 qs::Profile* pProfile,
						 HWND hwnd)
{
	assert(pwszURL);
	
	wstring_ptr wstrAssoc;
	if (pProfile) {
		WCHAR wszSchema[16];
		DWORD dwLen = 0;
		HRESULT hr = ::CoInternetParseUrl(pwszURL, PARSE_SCHEMA,
			0, wszSchema, countof(wszSchema) - 1, &dwLen, 0);
		if (hr == S_OK && dwLen != 0) {
			wszSchema[dwLen] = L'\0';
			wstring_ptr wstr = pProfile->getString(L"Association", wszSchema);
			if (*wstr.get())
				wstrAssoc = wstr;
		}
	}
	
	if (wstrAssoc.get()) {
		wstring_ptr wstrCommand = TextUtil::replaceAll(wstrAssoc.get(), L"%1", pwszURL);
		if (!wstrCommand.get())
			return false;
		return Process::shellExecute(wstrCommand.get(), hwnd);
	}
	else {
		W2T(pwszURL, ptszURL);
		SHELLEXECUTEINFO info = {
			sizeof(info),
			0,
			hwnd,
#ifdef _WIN32_WCE
			_T("open"),
#else
			0,
#endif
			ptszURL,
			0,
			0,
			SW_SHOW
		};
		return ::ShellExecuteEx(&info) != 0;
	}
}

HIMAGELIST qm::UIUtil::createImageListFromFile(const WCHAR* pwszName,
											   int nWidth,
											   COLORREF crMask)
{
	wstring_ptr wstrPath(Application::getApplication().getImagePath(pwszName));
	W2T(wstrPath.get(), ptszPath);
#ifdef _WIN32_WCE
	GdiObject<HBITMAP> hBitmap(::SHLoadDIBitmap(ptszPath));
#else
	GdiObject<HBITMAP> hBitmap(reinterpret_cast<HBITMAP>(
		::LoadImage(0, ptszPath, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE)));
#endif
	BITMAP bm;
	::GetObject(hBitmap.get(), sizeof(bm), &bm);
	
#if !defined _WIN32_WCE || _WIN32_WCE >= 0x500
	UINT nFlags = ILC_COLOR32 | ILC_MASK;
#else
	UINT nFlags = ILC_COLOR | ILC_MASK;
#endif
	HIMAGELIST hImageList = ImageList_Create(nWidth,
		bm.bmHeight, nFlags, bm.bmWidth/nWidth, 0);
	ImageList_AddMasked(hImageList, hBitmap.get(), crMask);
	return hImageList;
}

wstring_ptr qm::UIUtil::writeTemporaryFile(const WCHAR* pwszValue,
										   const WCHAR* pwszPrefix,
										   const WCHAR* pwszExtension,
										   TempFileCleaner* pTempFileCleaner)
{
	assert(pwszValue);
	assert(pwszPrefix);
	assert(pwszExtension);
	assert(pTempFileCleaner);
	
	Time time(Time::getCurrentTime());
	WCHAR wszName[128];
	_snwprintf(wszName, countof(wszName), L"%s-%04d%02d%02d%02d%02d%02d%03d.%s",
		pwszPrefix, time.wYear, time.wMonth, time.wDay, time.wHour,
		time.wMinute, time.wSecond, time.wMilliseconds, pwszExtension);
	
	wstring_ptr wstrPath(concat(Application::getApplication().getTemporaryFolder(), wszName));
	
	FileOutputStream stream(wstrPath.get());
	if (!stream)
		return 0;
	BufferedOutputStream bufferedStream(&stream, false);
	OutputStreamWriter writer(&bufferedStream, false, getSystemEncoding());
	if (!writer)
		return 0;
	if (writer.write(pwszValue, wcslen(pwszValue)) == -1)
		return 0;
	if (!writer.close())
		return 0;
	
	pTempFileCleaner->add(wstrPath.get());
	
	return wstrPath;
}

void qm::UIUtil::getAttachmentInfo(const EditMessage::Attachment& attachment,
								   wstring_ptr* pwstrName,
								   int* pnSysIconIndex)
{
	const WCHAR* pwszPath = attachment.wstrName_;
	const WCHAR* pwszFileName = pwszPath;
	wstring_ptr wstrName;
	wstring_ptr wstrFileName;
	if (attachment.bNew_) {
		std::auto_ptr<URI> pURI(URIFactory::parseURI(pwszPath));
		if (pURI.get()) {
			if (pURI->getFragment().getName()) {
				wstrFileName = allocWString(pURI->getFragment().getName());
				pwszFileName = wstrFileName.get();
				wstrName = concat(L"<", pwszFileName, L">");
			}
			else {
				wstrName = allocWString(pwszPath);
			}
		}
		else {
			const WCHAR* p  = wcsrchr(pwszPath, L'\\');
			if (p)
				pwszFileName = p + 1;
			wstrName = allocWString(pwszFileName);
		}
	}
	else {
		wstrName = concat(L"<", pwszPath, L">");
	}
	
	W2T(pwszFileName, ptszName);
	SHFILEINFO info = { 0 };
	::SHGetFileInfo(ptszName, FILE_ATTRIBUTE_NORMAL, &info, sizeof(info),
		SHGFI_USEFILEATTRIBUTES | SHGFI_SYSICONINDEX | SHGFI_SMALLICON);
	
	*pwstrName = wstrName;
	*pnSysIconIndex = info.iIcon;
}

bool qm::UIUtil::addMessageToClipboard(HWND hwnd,
									   MessageHolder* pmh)
{
	assert(hwnd);
	assert(pmh);
	
	Clipboard clipboard(hwnd);
	if (!clipboard)
		return false;
	
	wstring_ptr wstrURI(MessageHolderURI(pmh).toString());
	size_t nLen = wcslen(wstrURI.get());
	HANDLE hMem = ::GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT,
		(nLen + 2)*sizeof(WCHAR));
	if (!hMem)
		return false;
	{
		LockGlobal lock(hMem);
		void* p = lock.get();
		memcpy(p, wstrURI.get(), (nLen + 1)*sizeof(WCHAR));
		*(reinterpret_cast<WCHAR*>(p) + nLen + 1) = L'\0';
	}
	clipboard.setData(MessageDataObject::nFormats__[MessageDataObject::FORMAT_MESSAGEHOLDERLIST], hMem);
	
	return true;
}

std::auto_ptr<MessageContext> qm::UIUtil::getMessageFromClipboard(HWND hwnd,
																  const URIResolver* pURIResolver)
{
	Clipboard clipboard(hwnd);
	if (!clipboard)
		return std::auto_ptr<MessageContext>();
	
	HANDLE hMem = clipboard.getData(MessageDataObject::nFormats__[MessageDataObject::FORMAT_MESSAGEHOLDERLIST]);
	if (!hMem)
		return std::auto_ptr<MessageContext>();
	
	LockGlobal lock(hMem);
	void* p = lock.get();
	std::auto_ptr<URI> pURI(URIFactory::parseURI(static_cast<WCHAR*>(p)));
	if (!pURI.get())
		return std::auto_ptr<MessageContext>();
	
	return pURI->resolve(pURIResolver);
}

unsigned int qm::UIUtil::getPreferredWidth(HWND hwnd,
										   bool bNoPrefix)
{
	Window wnd(hwnd);
	wstring_ptr wstrText(wnd.getWindowText());
	if (!bNoPrefix) {
		WCHAR* pDst = wstrText.get();
		for (const WCHAR* p = wstrText.get(); *p; ++p) {
			if (*p != L'&')
				*pDst++ = *p;
		}
		*pDst = L'\0';
	}
	ClientDeviceContext dc(hwnd);
	ObjectSelector<HFONT> fontSelecter(dc, wnd.getFont());
	SIZE size;
	dc.getTextExtent(wstrText.get(), static_cast<int>(wcslen(wstrText.get())), &size);
	return size.cx + 2*qs::UIUtil::getLogPixel()/96;
}

POINT qm::UIUtil::getContextMenuPosition(HWND hwnd,
										 const POINT& pt)
{
	POINT ptMenu;
	if (pt.x == -1 && pt.y == -1) {
		RECT rect;
		Window(hwnd).getWindowRect(&rect);
		ptMenu.x = rect.left + 5;
		ptMenu.y = rect.top + 5;
	}
	else {
		ptMenu = pt;
	}
	return ptMenu;
}

POINT qm::UIUtil::getListViewContextMenuPosition(HWND hwnd,
												 const POINT& pt)
{
	POINT ptMenu;
	if (pt.x == -1 && pt.y == -1) {
		int nItem = ListView_GetNextItem(hwnd, -1, LVNI_ALL | LVNI_FOCUSED);
		RECT rect;
		if (nItem != -1) {
			ListView_GetItemRect(hwnd, nItem, &rect, LVIR_BOUNDS);
			Window(hwnd).clientToScreen(&rect);
		}
		else {
			Window(hwnd).getWindowRect(&rect);
		}
		ptMenu.x = rect.left + 5;
		ptMenu.y = rect.top + 5;
	}
	else {
		ptMenu = pt;
	}
	return ptMenu;
}

POINT qm::UIUtil::getTreeViewContextMenuPosition(HWND hwnd,
												 const POINT& pt)
{
	POINT ptMenu;
	if (pt.x == -1 && pt.y == -1) {
		HTREEITEM hItem = TreeView_GetSelection(hwnd);
		RECT rect;
		if (hItem) {
			TreeView_GetItemRect(hwnd, hItem, &rect, TRUE);
			Window(hwnd).clientToScreen(&rect);
		}
		else {
			Window(hwnd).getWindowRect(&rect);
		}
		ptMenu.x = rect.left + 5;
		ptMenu.y = rect.top + 5;
	}
	else {
		ptMenu = pt;
	}
	return ptMenu;
}

POINT qm::UIUtil::getTabCtrlContextMenuPosition(HWND hwnd,
												const POINT& pt)
{
	POINT ptMenu;
	if (pt.x == -1 && pt.y == -1) {
		int nItem = TabCtrl_GetCurFocus(hwnd);
		RECT rect;
		if (nItem != -1) {
			TabCtrl_GetItemRect(hwnd, nItem, &rect);
			Window(hwnd).clientToScreen(&rect);
		}
		else {
			Window(hwnd).getWindowRect(&rect);
		}
		ptMenu.x = rect.left + 5;
		ptMenu.y = rect.top + 5;
	}
	else {
		ptMenu = pt;
	}
	return ptMenu;
}

POINT qm::UIUtil::getTextWindowContextMenuPosition(TextWindow* pTextWindow,
												   const POINT& pt)
{
	POINT ptMenu;
	if (pt.x == -1 && pt.y == -1) {
		if (pTextWindow->isShowCaret() && ::GetCaretPos(&ptMenu)) {
			pTextWindow->clientToScreen(&ptMenu);
			ptMenu.x += 5;
			ptMenu.y += 5;
		}
		else {
			RECT rect;
			pTextWindow->getWindowRect(&rect);
			ptMenu.x = rect.left + 5;
			ptMenu.y = rect.top + 5;
		}
	}
	else {
		ptMenu = pt;
	}
	return ptMenu;
}

#if !defined _WIN32_WCE && _WIN32_WINNT >= 0x500
void qm::UIUtil::setWindowAlpha(HWND hwnd,
								Profile* pProfile,
								const WCHAR* pwszSection)
{
	unsigned int nAlpha = pProfile->getInt(pwszSection, L"Alpha");
	if (nAlpha > 255)
		return;
	
	Window wnd(hwnd);
	DWORD dwExStyle = wnd.getExStyle();
	if (dwExStyle & WS_EX_LAYERED && nAlpha == 0)
		wnd.setExStyle(0, WS_EX_LAYERED);
	else if (!(dwExStyle & WS_EX_LAYERED) && nAlpha != 0)
		wnd.setExStyle(WS_EX_LAYERED, WS_EX_LAYERED);
	if (nAlpha != 0)
		wnd.setLayeredWindowAttributes(0, static_cast<BYTE>(nAlpha), LWA_ALPHA);
}
#endif


/****************************************************************************
 *
 * DialogUtil
 *
 */

void qm::DialogUtil::loadBoolProperties(Dialog* pDialog,
										Profile* pProfile,
										const WCHAR* pwszSection,
										const BoolProperty* pProperties,
										size_t nCount)
{
	for (size_t n = 0; n < nCount; ++n) {
		bool bValue = pProfile->getInt(pwszSection, pProperties[n].pwszKey_) != 0;
		pDialog->sendDlgItemMessage(pProperties[n].nId_,
			BM_SETCHECK, bValue ? BST_CHECKED : BST_UNCHECKED);
	}
}

void qm::DialogUtil::saveBoolProperties(Dialog* pDialog,
										Profile* pProfile,
										const WCHAR* pwszSection,
										const BoolProperty* pProperties,
										size_t nCount)
{
	for (size_t n = 0; n < nCount; ++n) {
		bool bValue = pDialog->sendDlgItemMessage(pProperties[n].nId_, BM_GETCHECK) == BST_CHECKED;
		pProfile->setInt(pwszSection, pProperties[n].pwszKey_, bValue);
	}
}

void qm::DialogUtil::loadIntProperties(Dialog* pDialog,
									   Profile* pProfile,
									   const WCHAR* pwszSection,
									   const IntProperty* pProperties,
									   size_t nCount)
{
	for (size_t n = 0; n < nCount; ++n) {
		int nValue = pProfile->getInt(pwszSection, pProperties[n].pwszKey_);
		pDialog->setDlgItemInt(pProperties[n].nId_, nValue);
	}
}

void qm::DialogUtil::saveIntProperties(Dialog* pDialog,
									   Profile* pProfile,
									   const WCHAR* pwszSection,
									   const IntProperty* pProperties,
									   size_t nCount)
{
	for (size_t n = 0; n < nCount; ++n) {
		int nValue = pDialog->getDlgItemInt(pProperties[n].nId_);
		pProfile->setInt(pwszSection, pProperties[n].pwszKey_, nValue);
	}
}


/****************************************************************************
 *
 * History
 *
 */

qm::History::History(Profile* pProfile,
					 const WCHAR* pwszSection) :
	pProfile_(pProfile),
	pwszSection_(pwszSection),
	nSize_(10)
{
	nSize_ = pProfile_->getInt(pwszSection_, L"HistorySize", 10);
}

qm::History::~History()
{
}

wstring_ptr qm::History::getValue(unsigned int n) const
{
	WCHAR wszKey[32];
	_snwprintf(wszKey, countof(wszKey), L"History%u", n);
	return pProfile_->getString(pwszSection_, wszKey, L"");
}

unsigned int qm::History::getSize() const
{
	return nSize_;
}

void qm::History::addValue(const WCHAR* pwszValue)
{
	assert(pwszValue);
	
	if (!*pwszValue)
		return;
	
	wstring_ptr wstrValue(allocWString(pwszValue));
	
	for (unsigned int n = 0; n < nSize_; ++n) {
		WCHAR wszKey[32];
		_snwprintf(wszKey, countof(wszKey), L"History%u", n);
		wstring_ptr wstr(pProfile_->getString(pwszSection_, wszKey, L""));
		pProfile_->setString(pwszSection_, wszKey, wstrValue.get());
		if (!*wstr.get() || wcscmp(wstr.get(), pwszValue) == 0)
			break;
		wstrValue = wstr;
	}
}


/****************************************************************************
 *
 * ProgressDialogInit
 *
 */

qm::ProgressDialogInit::ProgressDialogInit(ProgressDialog* pDialog,
										   HWND hwnd,
										   UINT nTitle) :
	pDialog_(0)
{
	pDialog->init(hwnd);
	pDialog_ = pDialog;
	
	pDialog->setTitle(nTitle);
}

qm::ProgressDialogInit::ProgressDialogInit(ProgressDialog* pDialog,
										   HWND hwnd,
										   UINT nTitle,
										   UINT nMessage,
										   size_t nMin,
										   size_t nMax,
										   size_t nPos) :
	pDialog_(0)
{
	pDialog->init(hwnd);
	pDialog_ = pDialog;
	
	pDialog->setTitle(nTitle);
	pDialog->setMessage(nMessage);
	pDialog->setRange(nMin, nMax);
	pDialog->setPos(nPos);
	pDialog->setStep(1);
}

qm::ProgressDialogInit::~ProgressDialogInit()
{
	if (pDialog_)
		pDialog_->term();
}


/****************************************************************************
 *
 * DefaultPasswordManagerCallback
 *
 */

qm::DefaultPasswordManagerCallback::DefaultPasswordManagerCallback(Profile* pProfile) :
	pProfile_(pProfile)
{
}

qm::DefaultPasswordManagerCallback::~DefaultPasswordManagerCallback()
{
}

PasswordState qm::DefaultPasswordManagerCallback::getPassword(const PasswordCondition& condition,
															  wstring_ptr* pwstrPassword)
{
	HWND hwnd = Window::getForegroundWindow();
	HWND hwndMain = getMainWindow()->getHandle();
	if (::GetCurrentThreadId() != ::GetWindowThreadProcessId(hwndMain, 0))
		hwnd = 0;
	else if (::GetWindowThreadProcessId(hwnd, 0) != ::GetCurrentThreadId())
		hwnd = hwndMain;
	
	wstring_ptr wstrHint(condition.getHint());
	
	int nState = pProfile_->getInt(L"Global", L"DefaultPasswordState");
	if (nState < PASSWORDSTATE_ONETIME || PASSWORDSTATE_SAVE < nState)
		nState = PASSWORDSTATE_SESSION;
	
	PasswordDialog dialog(wstrHint.get(), static_cast<PasswordState>(nState));
	if (dialog.doModal(hwnd) != IDOK)
		return PASSWORDSTATE_NONE;
	
	PasswordState state = dialog.getState();
	pProfile_->setInt(L"Global", L"DefaultPasswordState", state);
	
	*pwstrPassword = allocWString(dialog.getPassword());
	
	return state;
}


/****************************************************************************
 *
 * DefaultPasswordCallback
 *
 */

qm::DefaultPasswordCallback::DefaultPasswordCallback(PasswordManager* pPasswordManager) :
	pPasswordManager_(pPasswordManager)
{
}

qm::DefaultPasswordCallback::~DefaultPasswordCallback()
{
}

PasswordState qm::DefaultPasswordCallback::getPassword(SubAccount* pSubAccount,
													   Account::Host host,
													   wstring_ptr* pwstrPassword)
{
	Account* pAccount = pSubAccount->getAccount();
	AccountPasswordCondition condition(pAccount, pSubAccount, host);
	PasswordState state = PASSWORDSTATE_ONETIME;
	*pwstrPassword = pPasswordManager_->getPassword(condition, false, &state);
	return state;
}

void qm::DefaultPasswordCallback::setPassword(SubAccount* pSubAccount,
											  Account::Host host,
											  const WCHAR* pwszPassword,
											  bool bPermanent)
{
	Account* pAccount = pSubAccount->getAccount();
	AccountPasswordCondition condition(pAccount, pSubAccount, host);
	pPasswordManager_->setPassword(condition, pwszPassword, bPermanent);
}


/****************************************************************************
 *
 * FolderListComboBox
 *
 */

qm::FolderListComboBox::FolderListComboBox(HWND hwnd) :
	Window(hwnd)
{
}

qm::FolderListComboBox::~FolderListComboBox()
{
}

void qm::FolderListComboBox::addFolders(const Account* pAccount,
										const Folder* pSelectFolder)
{
	assert(pAccount);
	
	HWND hwnd = getHandle();
	
	wstring_ptr wstrAllFolder(loadString(getResourceHandle(), IDS_ALLFOLDER));
	W2T(wstrAllFolder.get(), ptszAllFolder);
	ComboBox_AddString(hwnd, ptszAllFolder);
	
	Account::FolderList l(pAccount->getFolders());
	std::sort(l.begin(), l.end(), FolderLess());
	
	int nSelectIndex = 0;
	for (Account::FolderList::size_type n = 0; n < l.size(); ++n) {
		Folder* pFolder = l[n];
		
		unsigned int nLevel = pFolder->getLevel();
		StringBuffer<WSTRING> buf;
		while (nLevel != 0) {
			buf.append(L"  ");
			--nLevel;
		}
		buf.append(pFolder->getName());
		
		W2T(buf.getCharArray(), ptszName);
		int nIndex = ComboBox_AddString(hwnd, ptszName);
		ComboBox_SetItemData(hwnd, nIndex, pFolder);
		
		if (pSelectFolder == pFolder)
			nSelectIndex = static_cast<int>(n) + 1;
	}
	ComboBox_SetCurSel(hwnd, nSelectIndex);
}

const Folder* qm::FolderListComboBox::getSelectedFolder() const
{
	HWND hwnd = getHandle();
	
	int nIndex = ComboBox_GetCurSel(hwnd);
	if (nIndex == 0 || nIndex == CB_ERR)
		return 0;
	return reinterpret_cast<const Folder*>(ComboBox_GetItemData(hwnd, nIndex));
}

void qm::FolderListComboBox::selectFolder(const Folder* pFolder)
{
	HWND hwnd = getHandle();
	
	int nItem = 0;
	if (pFolder) {
		int nCount = ComboBox_GetCount(hwnd);
		while (nItem < nCount) {
			const Folder* p = reinterpret_cast<const Folder*>(
				ComboBox_GetItemData(hwnd, nItem));
			if (p == pFolder)
				break;
			++nItem;
		}
		if (nItem == nCount)
			return;
	}
	ComboBox_SetCurSel(hwnd, nItem);
}
