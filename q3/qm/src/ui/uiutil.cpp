/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmapplication.h>
#include <qmdocument.h>
#include <qmfolder.h>
#include <qmmessagewindow.h>
#include <qmpassword.h>
#include <qmuiutil.h>

#include <qsconv.h>
#include <qsstream.h>
#include <qswindow.h>

#include <tchar.h>

#include "dialogs.h"
#include "resourceinc.h"
#include "statusbar.h"
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

bool qm::UIUtil::openURL(HWND hwnd,
						 const WCHAR* pwszURL)
{
	assert(pwszURL);
	
	W2T(pwszURL, ptszURL);
	
	SHELLEXECUTEINFO info = {
		sizeof(info),
		0,
		hwnd,
		_T("open"),
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
				nImage = 6;
			else if ((nFlags & Folder::FLAG_OUTBOX) ||
				(nFlags & Folder::FLAG_SENTBOX) ||
				(nFlags & Folder::FLAG_DRAFTBOX))
				nImage = 8;
			else if (nFlags & Folder::FLAG_TRASHBOX)
				nImage = 10;
			else if (nFlags & Folder::FLAG_NOSELECT)
				nImage = bSelected ? 16 : 14;
			else if (nFlags & Folder::FLAG_LOCAL)
				nImage = bSelected ? 4 : 2;
			else
				nImage = bSelected ? 20 : 18;
		}
		break;
	case Folder::TYPE_QUERY:
		nImage = 12;
		break;
	default:
		assert(false);
		break;
	}
	return nImage;
}

void qm::UIUtil::updateStatusBar(MessageWindow* pMessageWindow,
								 StatusBar* pStatusBar,
								 int nOffset,
								 MessageHolder* pmh,
								 Message& msg,
								 const ContentTypeParser* pContentType)
{
	if (pmh) {
		const WCHAR* pwszEncoding = pMessageWindow->getEncoding();
		wstring_ptr wstrCharset;
		if (!pwszEncoding) {
			if (!pContentType) {
				if (msg.isMultipart()) {
					const Part::PartList& listPart = msg.getPartList();
					if (!listPart.empty())
						pContentType = listPart.front()->getContentType();
				}
				else {
					pContentType = msg.getContentType();
				}
			}
			if (pContentType)
				wstrCharset = pContentType->getParameter(L"charset");
			pwszEncoding = wstrCharset.get();
		}
		if (!pwszEncoding)
			pwszEncoding = L"us-ascii";
		pStatusBar->setText(nOffset + 1, pwszEncoding);
		
		const WCHAR* pwszTemplate = pMessageWindow->getTemplate();
		wstring_ptr wstrNone;
		if (pwszTemplate) {
			pwszTemplate += 5;
		}
		else {
			wstrNone = loadString(Application::getApplication().getResourceHandle(), IDS_NONE);
			pwszTemplate = wstrNone.get();
		}
		pStatusBar->setText(nOffset + 2, pwszTemplate);
		
		// TODO
		// Use icon
		unsigned int nSecurity = msg.getSecurity();
		if (nSecurity & Message::SECURITY_DECRYPTED)
			pStatusBar->setText(nOffset + 3, L"D");
		else
			pStatusBar->setText(nOffset + 3, L"");
		if (nSecurity & Message::SECURITY_VERIFIED)
			pStatusBar->setText(nOffset + 4, L"V");
		else if ((nSecurity & Message::SECURITY_VERIFICATIONFAILED) ||
				 (nSecurity & Message::SECURITY_ADDRESSNOTMATCH))
			pStatusBar->setText(nOffset + 4, L"X");
		else
			pStatusBar->setText(nOffset + 4, L"");
	}
	else {
		for (int n = 1; n < 5; ++n)
			pStatusBar->setText(nOffset + n, L"");
	}
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
	swprintf(wszName, L"%s-%04d%02d%02d%02d%02d%02d%03d.%s", pwszPrefix,
		time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute,
		time.wSecond, time.wMilliseconds, pwszExtension);
	
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

wstring_ptr qm::UIUtil::convertLFtoCRLF(const WCHAR* pwsz)
{
	StringBuffer<WSTRING> buf;
	while (*pwsz) {
		if (*pwsz == L'\n')
			buf.append(L'\r');
		buf.append(*pwsz);
		++pwsz;
	}
	return buf.getString();
}

wstring_ptr qm::UIUtil::convertCRLFtoLF(const WCHAR* pwsz)
{
	StringBuffer<WSTRING> buf;
	while (*pwsz) {
		if (*pwsz != L'\r')
			buf.append(*pwsz);
		++pwsz;
	}
	return buf.getString();
}

wstring_ptr qm::UIUtil::formatAccount(Account* pAccount)
{
	return concat(L"//", pAccount->getName());
}

wstring_ptr qm::UIUtil::formatFolder(Folder* pFolder)
{
	wstring_ptr wstrName(pFolder->getFullName());
	ConcatW c[] = {
		{ L"//",							2	},
		{ pFolder->getAccount()->getName(),	-1	},
		{ L"/",								1	},
		{ wstrName.get(),					-1	}
	};
	return concat(c, countof(c));
}

std::pair<Account*, Folder*> qm::UIUtil::getAccountOrFolder(Document* pDocument,
															const WCHAR* pwsz)
{
	std::pair<Account*, Folder*> p(0, 0);
	if (wcsncmp(pwsz, L"//", 2) != 0)
		return p;
	
	wstring_ptr wstrAccount;
	const WCHAR* pFolder = wcschr(pwsz + 2, L'/');
	if (pFolder) {
		wstrAccount = allocWString(pwsz + 2, pFolder - (pwsz + 2));
		++pFolder;
	}
	else {
		wstrAccount = allocWString(pwsz + 2);
	}
	
	Account* pAccount = pDocument->getAccount(wstrAccount.get());
	if (!pAccount)
		return p;
	
	if (pFolder)
		p.second = pAccount->getFolder(pFolder);
	else
		p.first = pAccount;
	
	return p;
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
											   Document* pDocument)
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
	
	return pDocument->getMessage(*pURI.get());
}

bool qm::UIUtil::hasFilesOrURIs(IDataObject* pDataObject)
{
#ifndef _WIN32_WCE
	FORMATETC fe = {
		CF_HDROP,
		0,
		DVASPECT_CONTENT,
		-1,
		TYMED_HGLOBAL
	};
	StgMedium stm;
	if (pDataObject->GetData(&fe, &stm) == S_OK) {
		if (stm.tymed == TYMED_HGLOBAL) {
			HDROP hDrop = reinterpret_cast<HDROP>(stm.hGlobal);
			UINT nCount = ::DragQueryFile(hDrop, 0xffffffff, 0, 0);
			for (UINT n = 0; n < nCount; ++n) {
				TCHAR tszPath[MAX_PATH];
				::DragQueryFile(hDrop, n, tszPath, countof(tszPath));
				DWORD dwAttributes = ::GetFileAttributes(tszPath);
				if (dwAttributes != 0xffffffff &&
					!(dwAttributes & FILE_ATTRIBUTE_DIRECTORY))
					return true;
			}
		}
	}
#endif
	
	return MessageDataObject::canPasteMessage(pDataObject);
}

void qm::UIUtil::getFilesOrURIs(IDataObject* pDataObject,
								PathList* pList)
{
#ifndef _WIN32_WCE
	FORMATETC fe = {
		CF_HDROP,
		0,
		DVASPECT_CONTENT,
		-1,
		TYMED_HGLOBAL
	};
	StgMedium stm;
	if (pDataObject->GetData(&fe, &stm) == S_OK && stm.tymed == TYMED_HGLOBAL) {
		HDROP hDrop = reinterpret_cast<HDROP>(stm.hGlobal);
		UINT nCount = ::DragQueryFile(hDrop, 0xffffffff, 0, 0);
		for (UINT n = 0; n < nCount; ++n) {
			TCHAR tszPath[MAX_PATH];
			::DragQueryFile(hDrop, n, tszPath, countof(tszPath));
			DWORD dwAttributes = ::GetFileAttributes(tszPath);
			if (dwAttributes != 0xffffffff &&
				!(dwAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
				wstring_ptr wstrPath(tcs2wcs(tszPath));
				pList->push_back(wstrPath.get());
				wstrPath.release();
			}
		}
	}
#endif
	
	MessageDataObject::URIList listURI;
	struct Deleter
	{
		Deleter(MessageDataObject::URIList& l) : l_(l) {}
		~Deleter() { std::for_each(l_.begin(), l_.end(), qs::deleter<URI>()); }
		MessageDataObject::URIList& l_;
	} deleter(listURI);
	if (MessageDataObject::getURIs(pDataObject, &listURI)) {
		for (MessageDataObject::URIList::const_iterator it = listURI.begin(); it != listURI.end(); ++it) {
			wstring_ptr wstrURI((*it)->toString());
			pList->push_back(wstrURI.get());
			wstrURI.release();
		}
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
	swprintf(wszKey, L"History%u", n);
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
		swprintf(wszKey, L"History%u", n);
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
										   HWND hwnd) :
	pDialog_(0)
{
	pDialog->init(hwnd);
	pDialog_ = pDialog;
}

qm::ProgressDialogInit::ProgressDialogInit(ProgressDialog* pDialog,
										   HWND hwnd,
										   UINT nTitle,
										   UINT nMessage,
										   unsigned int nMin,
										   unsigned int nMax,
										   unsigned int nPos) :
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
	if (::GetWindowThreadProcessId(hwnd, 0) != ::GetCurrentThreadId())
		hwnd = getMainWindow()->getHandle();
	
	wstring_ptr wstrHint(condition.getHint());
	PasswordDialog dialog(wstrHint.get());
	if (dialog.doModal(hwnd) != IDOK)
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
