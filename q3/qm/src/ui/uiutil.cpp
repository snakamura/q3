/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#pragma warning(disable:4786)

#include <qmapplication.h>
#include <qmdocument.h>
#include <qmfolder.h>
#include <qmpassword.h>
#include <qmuiutil.h>

#include <qsconv.h>
#include <qsstream.h>
#include <qstextutil.h>
#include <qswindow.h>

#include <tchar.h>

#include "dialogs.h"
#include "resourceinc.h"
#include "uiutil.h"
#include "../model/dataobject.h"
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
		items[n].n_ = pProfile->getInt(pwszSection, items[n].pwszKey_, 0);
	
	int nShow = SW_SHOWNORMAL;
	if (items[2].n_ != 0 && items[3].n_ != 0) {
		RECT rect;
		::SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);
		pCreateStruct->x = rect.left + items[0].n_;
		pCreateStruct->y = rect.top + items[1].n_;
		pCreateStruct->cx = items[2].n_;
		pCreateStruct->cy = items[3].n_;
		
		nShow = pProfile->getInt(pwszSection, L"Show", SW_SHOWNORMAL);
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
	
	wstring_ptr wstrEncodings(pProfile->getString(L"Global",
		L"Encodings", L"iso-8859-1 iso-2022-jp shift_jis euc-jp utf-8"));
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

wstring_ptr qm::UIUtil::formatMenu(const WCHAR* pwszText)
{
	assert(pwszText);
	
	StringBuffer<WSTRING> buf;
	buf.append(L'&');
	while (*pwszText) {
		if (*pwszText == L'&')
			buf.append(L'&');
		buf.append(*pwszText);
		++pwszText;
	}
	
	return buf.getString();
}

bool qm::UIUtil::openURL(const WCHAR* pwszURL,
						 Profile* pProfile,
						 HWND hwnd)
{
	assert(pwszURL);
	
	if (wcsncmp(pwszURL, L"file:", 5) == 0 ||
		(wcslen(pwszURL) > 2 && TextUtil::isDriveLetterChar(*pwszURL) &&
		*(pwszURL + 1) == L':' && *(pwszURL + 2) == L'\\') ||
		wcsncmp(pwszURL, L"\\\\", 2) == 0) {
		const WCHAR* pExt = wcsrchr(pwszURL, L'.');
		if (pExt) {
			wstring_ptr wstrExtensions(pProfile->getString(L"Global",
				L"WarnExtensions", L"exe com pif bat scr htm html hta vbs js"));
			if (wcsstr(wstrExtensions.get(), pExt + 1)) {
				int nMsg = messageBox(Application::getApplication().getResourceHandle(),
					IDS_CONFIRM_OPENURL, MB_YESNO | MB_DEFBUTTON2 | MB_ICONWARNING, hwnd, 0, 0);
				if (nMsg != IDYES)
					return true;
			}
		}
	}
	
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

int qm::UIUtil::getFolderImage(Folder* pFolder,
							   bool bSelected)
{
	int nImage = 0;
	switch (pFolder->getType()) {
	case Folder::TYPE_NORMAL:
		{
			unsigned int nFlags = pFolder->getFlags();
			if (nFlags & Folder::FLAG_INBOX)
				nImage = 8;
			else if ((nFlags & Folder::FLAG_OUTBOX) ||
				(nFlags & Folder::FLAG_SENTBOX) ||
				(nFlags & Folder::FLAG_DRAFTBOX))
				nImage = 11;
			else if (nFlags & Folder::FLAG_TRASHBOX ||
				nFlags & Folder::FLAG_JUNKBOX)
				nImage = 14;
			else if (nFlags & Folder::FLAG_NOSELECT)
				nImage = bSelected ? 29 : 26;
			else if (nFlags & Folder::FLAG_LOCAL)
				nImage = bSelected ? 5 : 2;
			else
				nImage = bSelected ? 23 : 20;
		}
		break;
	case Folder::TYPE_QUERY:
		nImage = 17;
		break;
	default:
		assert(false);
		break;
	}
	return nImage;
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
		if (wcsncmp(pwszPath, URI::getScheme(), wcslen(URI::getScheme())) == 0) {
			std::auto_ptr<URI> pURI(URI::parse(pwszPath));
			if (pURI.get() && pURI->getFragment().getName()) {
				wstrFileName = allocWString(pURI->getFragment().getName());
				pwszFileName = wstrFileName.get();
				wstrName = concat(L"<", pwszFileName, L">");
			}
			else
				wstrName = allocWString(pwszPath);
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
	
	wstring_ptr wstrURI(URI(pmh).toString());
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

MessagePtr qm::UIUtil::getMessageFromClipboard(HWND hwnd,
											   AccountManager* pAccountManager)
{
	Clipboard clipboard(hwnd);
	if (!clipboard)
		return MessagePtr();
	
	HANDLE hMem = clipboard.getData(MessageDataObject::nFormats__[MessageDataObject::FORMAT_MESSAGEHOLDERLIST]);
	if (!hMem)
		return MessagePtr();
	
	LockGlobal lock(hMem);
	void* p = lock.get();
	std::auto_ptr<URI> pURI(URI::parse(static_cast<WCHAR*>(p)));
	if (!pURI.get())
		return MessagePtr();
	
	return pAccountManager->getMessage(*pURI.get());
}


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
		bool bValue = pProfile->getInt(pwszSection,
			pProperties[n].pwszKey_, pProperties[n].bDefault_) != 0;
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
		int nValue = pProfile->getInt(pwszSection,
			pProperties[n].pwszKey_, pProperties[n].nDefault_);
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
		if (wcscmp(wstr.get(), pwszValue) == 0)
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

qm::DefaultPasswordManagerCallback::DefaultPasswordManagerCallback()
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
	
	struct ModalHandlerImpl : public ModalHandler
	{
		virtual void preModalDialog(HWND hwndParent)
		{
		}
		
		virtual void postModalDialog(HWND hwndParent)
		{
		}
	} modelHandler;
	
	wstring_ptr wstrHint(condition.getHint());
	PasswordDialog dialog(wstrHint.get());
	if (dialog.doModal(hwnd, hwnd ? 0 : &modelHandler) != IDOK)
		return PASSWORDSTATE_NONE;
	
	*pwstrPassword = allocWString(dialog.getPassword());
	
	return dialog.getState();
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
