/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
 * All rights reserved.
 *
 */

#pragma warning(disable:4786)

#include <qsconv.h>
#include <qsdevicecontext.h>
#include <qsdialog.h>
#include <qsfile.h>
#include <qsinit.h>
#include <qsstl.h>
#include <qsthread.h>

#include <algorithm>
#include <hash_map>
#include <memory>
#include <vector>

#ifdef _WIN32_WCE_PSPC
#	include <aygshell.h>
#endif
#include <commdlg.h>
#include <tchar.h>
#include <windows.h>

#include "dialog.h"
#include "resourceinc.h"
#include "window.h"

using namespace qs;


/****************************************************************************
 *
 * DialogBaseImpl
 *
 */

DialogBaseImpl::DialogMap* qs::DialogBaseImpl::pMap__;
ThreadLocal* qs::DialogBaseImpl::pModelessList__;
DialogBaseImpl::InitializerImpl qs::DialogBaseImpl::init__;

INT_PTR qs::DialogBaseImpl::dialogProc(UINT uMsg,
									   WPARAM wParam,
									   LPARAM lParam)
{
	INT_PTR nResult = 0;
	switch (uMsg) {
	case WM_COMMAND:
		nResult = notifyCommandHandlers(HIWORD(wParam), LOWORD(wParam));
		if (nResult == 0)
			return TRUE;
		break;
	
#ifdef _WIN32_WCE_PSPC
	case WM_INITDIALOG:
		layout(false);
		break;
#endif
	case WM_NOTIFY:
		{
			bool bHandled = false;
			nResult = notifyNotifyHandlers(reinterpret_cast<NMHDR*>(lParam), &bHandled);
			if (bHandled)
				return nResult;
		}
		break;
	
	case WM_DRAWITEM:
		notifyOwnerDrawHandlers(reinterpret_cast<LPDRAWITEMSTRUCT>(lParam));
		break;
	
	case WM_MEASUREITEM:
		measureOwnerDrawHandlers(reinterpret_cast<LPMEASUREITEMSTRUCT>(lParam));
		break;

#ifdef _WIN32_WCE_PSPC
	case WM_SIZE:
		layout(true);
		break;
#endif
	
	default:
		break;
	}
	
	nResult = pDialogHandler_->dialogProc(uMsg, wParam, lParam);
	
#if !defined _WIN32_WCE || defined _WIN32_WCE_EMULATION
	switch (uMsg) {
	case WM_NCDESTROY:
		destroy();
		break;
	}
#endif
	
	return nResult;
}

void qs::DialogBaseImpl::destroy()
{
	DialogMap* pMap = getDialogMap();
	pMap->removeController(pThis_->getHandle());
	DialogBaseImpl::removeModelessDialog(pThis_);
	assert(listCommandHandler_.size() == 0);
	assert(listNotifyHandler_.size() == 0);
	assert(listOwnerDrawHandler_.size() == 0);
	if (bDeleteThis_)
		delete pThis_;
}

LRESULT qs::DialogBaseImpl::notifyCommandHandlers(WORD wCode,
												  WORD wId) const
{
	for (CommandHandlerList::const_iterator it = listCommandHandler_.begin(); it != listCommandHandler_.end(); ++it) {
		LRESULT lResult = (*it)->onCommand(wCode, wId);
		if (lResult == 0)
			return lResult;
	}
	return 1;
}

LRESULT qs::DialogBaseImpl::notifyNotifyHandlers(NMHDR* pnmhdr,
												 bool* pbHandled) const
{
	assert(pbHandled);
	
	for (NotifyHandlerList::const_iterator it = listNotifyHandler_.begin(); it != listNotifyHandler_.end(); ++it) {
		LRESULT lResult = (*it)->onNotify(pnmhdr, pbHandled);
		if (*pbHandled)
			return lResult;
	}
	return 1;
}

void qs::DialogBaseImpl::notifyOwnerDrawHandlers(DRAWITEMSTRUCT* pDrawItem) const
{
	for (OwnerDrawHandlerList::const_iterator it = listOwnerDrawHandler_.begin(); it != listOwnerDrawHandler_.end(); ++it)
		(*it)->onDrawItem(pDrawItem);
}

void qs::DialogBaseImpl::measureOwnerDrawHandlers(MEASUREITEMSTRUCT* pMeasureItem) const
{
	for (OwnerDrawHandlerList::const_iterator it = listOwnerDrawHandler_.begin(); it != listOwnerDrawHandler_.end(); ++it)
		(*it)->onMeasureItem(pMeasureItem);
}

#ifdef _WIN32_WCE_PSPC
void qs::DialogBaseImpl::layout(bool bNotify)
{
	if (nIdPortrait_ != nIdLandscape_) {
		DRA::DisplayMode mode = DRA::GetDisplayMode();
		if (mode != displayMode_) {
			UINT nId = mode == DRA::Portrait ? nIdPortrait_ : nIdLandscape_;
			if (DRA::RelayoutDialog(hInstResource_, pThis_->getHandle(), MAKEINTRESOURCE(nId))) {
				if (bNotify)
					pDialogHandler_->displayModeChanged();
				displayMode_ = mode;
			}
		}
	}
}
#endif

DialogBaseImpl::DialogMap* qs::DialogBaseImpl::getDialogMap()
{
	return pMap__;
}

const DialogBaseImpl::ModelessList* qs::DialogBaseImpl::getModelessList()
{
	return static_cast<ModelessList*>(pModelessList__->get());
}

void qs::DialogBaseImpl::addModelessDialog(DialogBase* pDialogBase)
{
	ModelessList* pList = static_cast<ModelessList*>(pModelessList__->get());
	pList->push_back(pDialogBase);
}

void qs::DialogBaseImpl::removeModelessDialog(DialogBase* pDialogBase)
{
	ModelessList* pList = static_cast<ModelessList*>(pModelessList__->get());
	ModelessList::iterator it = std::remove(
		pList->begin(), pList->end(), pDialogBase);
	pList->erase(it, pList->end());
}


/****************************************************************************
 *
 * DialogBaseImpl::InitializerImpl
 *
 */

qs::DialogBaseImpl::InitializerImpl::InitializerImpl()
{
}

qs::DialogBaseImpl::InitializerImpl::~InitializerImpl()
{
}

bool qs::DialogBaseImpl::InitializerImpl::init()
{
	DialogBaseImpl::pMap__ = new DialogMap();
	DialogBaseImpl::pModelessList__ = new ThreadLocal();
	return true;
}

void qs::DialogBaseImpl::InitializerImpl::term()
{
	delete DialogBaseImpl::pModelessList__;
	DialogBaseImpl::pModelessList__ = 0;
	
	delete DialogBaseImpl::pMap__;
	DialogBaseImpl::pMap__ = 0;
}

bool qs::DialogBaseImpl::InitializerImpl::initThread()
{
	if (!DialogBaseImpl::pMap__->initThread())
		return false;
	
	pModelessList__->set(new ModelessList());
	
	return true;
}

void qs::DialogBaseImpl::InitializerImpl::termThread()
{
	delete static_cast<ModelessList*>(pModelessList__->get());
	DialogBaseImpl::pMap__->termThread();
}


/****************************************************************************
 *
 * DialogBase
 *
 */

qs::DialogBase::DialogBase(HINSTANCE hInstResource,
						   UINT nIdPortrait,
						   UINT nIdLandscape,
						   bool bDeleteThis) :
	Window(0)
{
	pImpl_ = new DialogBaseImpl();
	pImpl_->pThis_ = this;
#ifdef _WIN32_WCE_PSPC
	pImpl_->hInstResource_ = hInstResource;
	pImpl_->nIdPortrait_ = nIdPortrait;
	pImpl_->nIdLandscape_ = nIdLandscape;
#endif
	pImpl_->bDeleteThis_ = bDeleteThis;
	pImpl_->pDialogHandler_ = 0;
	pImpl_->bDeleteHandler_ = false;
	pImpl_->pInitThread_ = &InitThread::getInitThread();
#ifdef _WIN32_WCE_PSPC
	pImpl_->displayMode_ = DRA::Portrait;
#endif
}

qs::DialogBase::~DialogBase()
{
	if (pImpl_) {
		if (pImpl_->bDeleteHandler_)
			delete pImpl_->pDialogHandler_;
		delete pImpl_;
	}
}

void qs::DialogBase::setDialogHandler(DialogHandler* pDialogHandler,
									  bool bDeleteHandler)
{
	pImpl_->pDialogHandler_ = pDialogHandler;
	pImpl_->bDeleteHandler_ = bDeleteHandler;
	pDialogHandler->setDialogBase(this);
}

void qs::DialogBase::addCommandHandler(CommandHandler* pch)
{
	assert(pch);
	pImpl_->listCommandHandler_.push_back(pch);
}

void qs::DialogBase::removeCommandHandler(CommandHandler* pch)
{
	assert(pch);
	DialogBaseImpl::CommandHandlerList::iterator it =
		std::remove(pImpl_->listCommandHandler_.begin(),
			pImpl_->listCommandHandler_.end(), pch);
	assert(it != pImpl_->listCommandHandler_.end());
	pImpl_->listCommandHandler_.erase(it, pImpl_->listCommandHandler_.end());
}

void qs::DialogBase::addNotifyHandler(NotifyHandler* pnh)
{
	assert(pnh);
	pImpl_->listNotifyHandler_.push_back(pnh);
}

void qs::DialogBase::removeNotifyHandler(NotifyHandler* pnh)
{
	assert(pnh);
	DialogBaseImpl::NotifyHandlerList::iterator it =
		std::remove(pImpl_->listNotifyHandler_.begin(),
			pImpl_->listNotifyHandler_.end(), pnh);
	assert(it != pImpl_->listNotifyHandler_.end());
	pImpl_->listNotifyHandler_.erase(it, pImpl_->listNotifyHandler_.end());
}

void qs::DialogBase::addOwnerDrawHandler(OwnerDrawHandler* podh)
{
	assert(podh);
	pImpl_->listOwnerDrawHandler_.push_back(podh);
}

void qs::DialogBase::removeOwnerDrawHandler(OwnerDrawHandler* podh)
{
	assert(podh);
	DialogBaseImpl::OwnerDrawHandlerList::iterator it =
		std::remove(pImpl_->listOwnerDrawHandler_.begin(),
			pImpl_->listOwnerDrawHandler_.end(), podh);
	assert(it != pImpl_->listOwnerDrawHandler_.end());
	pImpl_->listOwnerDrawHandler_.erase(it, pImpl_->listOwnerDrawHandler_.end());
}

InitThread* qs::DialogBase::getInitThread() const
{
	return pImpl_->pInitThread_;
}

LRESULT qs::DialogBase::defWindowProc(UINT uMsg,
									  WPARAM wParam,
									  LPARAM lParam)
{
	return ::DefWindowProc(getHandle(), uMsg, wParam, lParam);
}

bool qs::DialogBase::processDialogMessage(MSG* pMsg)
{
	if ((pMsg->message < WM_KEYFIRST || WM_KEYLAST < pMsg->message) &&
		(pMsg->message < WM_MOUSEFIRST || WM_MOUSELAST < pMsg->message))
		return false;
	
	const DialogBaseImpl::ModelessList* pList = DialogBaseImpl::getModelessList();
	
	DialogBaseImpl::ModelessList::const_iterator it = pList->begin();
	while (it != pList->end() && !(*it)->isDialogMessage(pMsg))
		++it;
	return it != pList->end();
}


/****************************************************************************
 *
 * DialogImpl
 *
 */

struct qs::DialogImpl
{
	HINSTANCE hInstResource_;
	UINT nId_;
};


/****************************************************************************
 *
 * Dialog
 *
 */

qs::Dialog::Dialog(HINSTANCE hInstResource,
				   UINT nIdPortrait,
				   UINT nIdLandscape,
				   bool bDeleteThis) :
	DialogBase(hInstResource, nIdPortrait, nIdLandscape, bDeleteThis),
	pImpl_(0)
{
	pImpl_ = new DialogImpl();
	pImpl_->hInstResource_ = hInstResource;
	pImpl_->nId_ = nIdPortrait;
}

qs::Dialog::~Dialog()
{
	delete pImpl_;
}

INT_PTR qs::Dialog::doModal(HWND hwndParent)
{
	return doModal(hwndParent, 0);
}

INT_PTR qs::Dialog::doModal(HWND hwndParent,
							ModalHandler* pModalHandler)
{
	if (!pModalHandler)
		pModalHandler = InitThread::getInitThread().getModalHandler();
	
	ModalHandlerInvoker invoker(pModalHandler, hwndParent);
	DialogBaseImpl::DialogMap* pMap = DialogBaseImpl::getDialogMap();
	pMap->setThis(this);
	return ::DialogBox(pImpl_->hInstResource_,
		MAKEINTRESOURCE(pImpl_->nId_), hwndParent, dialogProc);
}

bool qs::Dialog::create(HWND hwndParent)
{
	DialogBaseImpl::DialogMap* pMap = DialogBaseImpl::getDialogMap();
	pMap->setThis(this);
	HWND hwnd = ::CreateDialog(pImpl_->hInstResource_,
		MAKEINTRESOURCE(pImpl_->nId_), hwndParent, dialogProc);
	if (!hwnd) {
		setHandle(0);
		return false;
	}
	assert(getHandle() == hwnd);
	
	DialogBaseImpl::addModelessDialog(this);
	
	return true;
}

bool qs::Dialog::endDialog(int nCode)
{
	return ::EndDialog(getHandle(), nCode) != 0;
}

INT_PTR CALLBACK qs::dialogProc(HWND hwnd,
								UINT uMsg,
								WPARAM wParam,
								LPARAM lParam)
{
	DialogBaseImpl::DialogMap* pMap = DialogBaseImpl::getDialogMap();
	DialogBase* pThis = pMap->findController(hwnd);
	
	INT_PTR nResult = 0;
	if (pThis)
		nResult = pThis->pImpl_->dialogProc(uMsg, wParam, lParam);
	
#if defined _WIN32_WCE && !defined _WIN32_WCE_EMULATION
	if (uMsg == WM_DESTROY)
		WindowDestroy::getWindowDestroy()->process(hwnd);
#endif
	
	return nResult;
}


/****************************************************************************
 *
 * DialogHandler
 *
 */

qs::DialogHandler::~DialogHandler()
{
}


/****************************************************************************
 *
 * DefaultDialogHandler
 *
 */

qs::DefaultDialogHandler::DefaultDialogHandler() :
	pDialogBase_(0),
	nResult_(TRUE)
{
}

qs::DefaultDialogHandler::~DefaultDialogHandler()
{
}

DialogBase* qs::DefaultDialogHandler::getDialogBase() const
{
	assert(pDialogBase_);
	return pDialogBase_;
}

void qs::DefaultDialogHandler::setDialogBase(DialogBase* pDialogBase)
{
	assert(!pDialogBase_);
	pDialogBase_ = pDialogBase;
}

INT_PTR qs::DefaultDialogHandler::dialogProc(UINT uMsg,
											 WPARAM wParam,
											 LPARAM lParam)
{
	return FALSE;
}

void qs::DefaultDialogHandler::initProcResult()
{
	nResult_ = TRUE;
}

INT_PTR qs::DefaultDialogHandler::getProcResult() const
{
	return nResult_;
}

#ifdef _WIN32_WCE_PSPC
void qs::DefaultDialogHandler::displayModeChanged()
{
}
#endif

DefWindowProcHolder* qs::DefaultDialogHandler::getDefWindowProcHolder()
{
	assert(pDialogBase_);
	return pDialogBase_;
}

void qs::DefaultDialogHandler::setProcResult(INT_PTR nResult)
{
	nResult_ = nResult;
}


/****************************************************************************
 *
 * DefaultDialog
 *
 */

qs::DefaultDialog::DefaultDialog(HINSTANCE hInst,
								 UINT nIdPortrait,
								 UINT nIdLandscape) :
	Dialog(hInst, nIdPortrait, nIdLandscape, false)
{
	addCommandHandler(this);
	setDialogHandler(this, false);
}

qs::DefaultDialog::~DefaultDialog()
{
}

void qs::DefaultDialog::init(bool bDoneButton)
{
#if defined _WIN32_WCE && _WIN32_WCE >= 0x300 && defined _WIN32_WCE_PSPC
#if defined QSDIALOGMENU
	if (bDoneButton) {
		SHMENUBARINFO shmbi = {
			sizeof(shmbi),
			getHandle(),
			SHCMBF_HMENU,
			IDR_OK,
			getResourceDllInstanceHandle(),
			0,
			0,
			0,
			0
		};
		::SHCreateMenuBar(&shmbi);
		bDoneButton = false;
	}
#endif
	SHINITDLGINFO shidi = {
		SHIDIM_FLAGS,
		getHandle(),
		SHIDIF_SIZEDLGFULLSCREEN | (bDoneButton ? SHIDIF_DONEBUTTON : 0)
	};
	::SHInitDialog(&shidi);
#else
	centerWindow(0);
#endif
}

INT_PTR qs::DefaultDialog::dialogProc(UINT uMsg,
									  WPARAM wParam,
									  LPARAM lParam)
{
	BEGIN_DIALOG_HANDLER()
		HANDLE_DESTROY()
		HANDLE_INITDIALOG()
	END_DIALOG_HANDLER()
	return DefaultDialogHandler::dialogProc(uMsg, wParam, lParam);
}

LRESULT qs::DefaultDialog::onCommand(WORD nCode,
									 WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID_CODE(IDCANCEL, BN_CLICKED, onCancel)
		HANDLE_COMMAND_ID_CODE(IDOK, BN_CLICKED, onOk)
#ifdef _WIN32_WCE_PSPC
		HANDLE_COMMAND_ID_CODE(IDCANCEL, 0x1000, onCancel)
		HANDLE_COMMAND_ID_CODE(IDOK, 0x1000, onOk)
#endif
	END_COMMAND_HANDLER()
	return CommandHandler::onCommand(nCode, nId);
}

LRESULT qs::DefaultDialog::onDestroy()
{
	removeCommandHandler(this);
	return 0;
}

LRESULT qs::DefaultDialog::onInitDialog(HWND hwndFocus,
										LPARAM lParam)
{
	init(false);
	return TRUE;
}

LRESULT qs::DefaultDialog::onOk()
{
	endDialog(IDOK);
	return 0;
}

LRESULT qs::DefaultDialog::onCancel()
{
	endDialog(IDCANCEL);
	return 0;
}


/****************************************************************************
 *
 * FileDialogImpl
 *
 */

struct qs::FileDialogImpl
{
	bool bOpen_;
	const WCHAR* pwszFilter_;
	const WCHAR* pwszInitialDir_;
	const WCHAR* pwszDefaultExt_;
	const WCHAR* pwszFileName_;
	DWORD dwFlags_;
	wstring_ptr wstrPath_;
};


/****************************************************************************
 *
 * FileDialog
 *
 */

qs::FileDialog::FileDialog(bool bOpen,
						   const WCHAR* pwszFilter,
						   const WCHAR* pwszInitialDir,
						   const WCHAR* pwszDefaultExt,
						   const WCHAR* pwszFileName,
						   DWORD dwFlags) :
	pImpl_(0)
{
	pImpl_ = new FileDialogImpl();
	pImpl_->bOpen_ = bOpen;
	pImpl_->pwszFilter_ = pwszFilter;
	pImpl_->pwszInitialDir_ = pwszInitialDir ? pwszInitialDir : L"";
	pImpl_->pwszDefaultExt_ = pwszDefaultExt ? pwszDefaultExt : L"";
	pImpl_->pwszFileName_ = pwszFileName;
	pImpl_->dwFlags_ = dwFlags;
}

qs::FileDialog::~FileDialog()
{
	delete pImpl_;
}

const WCHAR* qs::FileDialog::getPath() const
{
	return pImpl_->wstrPath_.get();
}

int qs::FileDialog::doModal(HWND hwndParent)
{
	return doModal(hwndParent, 0);
}

int qs::FileDialog::doModal(HWND hwndParent,
							ModalHandler* pModalHandler)
{
#ifdef _WIN32_WCE
	bool bMultiSelect = false;
#else
	bool bMultiSelect = (pImpl_->dwFlags_ & OFN_ALLOWMULTISELECT) != 0;
#endif
	
	if (!pModalHandler)
		pModalHandler = InitThread::getInitThread().getModalHandler();
	
	OPENFILENAME ofn = {
		sizeof(ofn),
		hwndParent,
		getInstanceHandle()
	};
	
	tstring_ptr ptstrFilter;
	if (pImpl_->pwszFilter_) {
		ptstrFilter = wcs2tcs(pImpl_->pwszFilter_);
		for (TCHAR* p = ptstrFilter.get(); *p; ++p) {
			if (*p == _T('|'))
				*p = _T('\0');
		}
		ofn.lpstrFilter = ptstrFilter.get();
	}
	
	int nLen = MAX_PATH;
	if (bMultiSelect)
		nLen *= 10;
	tstring_ptr tstrPath(allocTString(nLen));
	ofn.lpstrFile = tstrPath.get(),
	ofn.nMaxFile = nLen;
	if (pImpl_->pwszFileName_) {
		W2T(pImpl_->pwszFileName_, ptszFileName);
		_tcsncpy(tstrPath.get(), ptszFileName, nLen);
	}
	else {
		*tstrPath.get() = _T('\0');
	}
	ofn.Flags = pImpl_->dwFlags_;
	
	W2T(pImpl_->pwszInitialDir_, ptszInitialDir);
	if (*ptszInitialDir)
		ofn.lpstrInitialDir = ptszInitialDir;
	
	W2T(pImpl_->pwszDefaultExt_, ptszDefaultExt);
	if (*ptszDefaultExt)
		ofn.lpstrDefExt = ptszDefaultExt;
	
	ModalHandlerInvoker invoker(pModalHandler, hwndParent);
	if (!(pImpl_->bOpen_ ? ::GetOpenFileName(&ofn) : ::GetSaveFileName(&ofn)))
		return IDCANCEL;
	
	T2W(ofn.lpstrFile, pwszPath);
	wstring_ptr wstrPath(allocWString(pwszPath, wcslen(pwszPath) + 2));
	*(wstrPath.get() + wcslen(wstrPath.get()) + 1) = L'\0';
	if (bMultiSelect) {
		if (*(ofn.lpstrFile + _tcslen(ofn.lpstrFile) + 1) != _T('\0')) {
			size_t nPathLen = wcslen(wstrPath.get());
			typedef std::vector<WCHAR> Buffer;
			Buffer buf;
			TCHAR* p = ofn.lpstrFile + _tcslen(ofn.lpstrFile) + 1;
			while (*p) {
				T2W(p, pwszFileName);
				size_t nFileNameLen = wcslen(pwszFileName);
				buf.reserve(nPathLen + nFileNameLen + 3);
				std::copy(wstrPath.get(), wstrPath.get() + nPathLen,
					std::back_inserter(buf));
				buf.push_back(L'\\');
				std::copy(pwszFileName, pwszFileName + nFileNameLen,
					std::back_inserter(buf));
				buf.push_back(L'\0');
				
				p += _tcslen(p) + 1;
			}
			buf.push_back(L'\0');
			
			wstrPath = allocWString(buf.size());
			memcpy(wstrPath.get(), &buf[0], buf.size()*sizeof(WCHAR));
		}
	}
	pImpl_->wstrPath_ = wstrPath;
	
	return IDOK;
}


#ifdef _WIN32_WCE

/****************************************************************************
 *
 * BrowseFolderDialogImpl
 *
 */

struct qs::BrowseFolderDialogImpl : public qs::NotifyHandler
{
public:
	wstring_ptr getPath(HWND hwnd,
						HTREEITEM hItem);
	void selectFolder(HWND hwnd,
					  const WCHAR* pwszPath);
	void selectFolder(HWND hwnd,
					  HTREEITEM hItem,
					  const WCHAR* pwszPath);
	void refreshFolder(HWND hwnd,
					   HTREEITEM hItem);

public:
	virtual LRESULT onNotify(NMHDR* pnmhdr,
							 bool* pbHandled);

private:
	LRESULT onItemExpanding(NMHDR* pnmhdr,
							bool* pbHandled);

public:
	wstring_ptr wstrPath_;
};

wstring_ptr qs::BrowseFolderDialogImpl::getPath(HWND hwnd,
												HTREEITEM hItem)
{
	assert(hwnd);
	assert(hItem);
	
	wstring_ptr wstrPath(allocWString(L""));
	
	while (true) {
		HTREEITEM hParent = TreeView_GetParent(hwnd, hItem);
		if (!hParent)
			break;
		
		TCHAR tszPath[MAX_PATH + 1] = _T("\\");
		TVITEM ti = {
			TVIF_HANDLE | TVIF_TEXT,
			hItem,
			0,
			0,
			tszPath + 1,
			countof(tszPath) - 1
		};
		TreeView_GetItem(hwnd, &ti);
		
		T2W(tszPath, pwszPath);
		wstrPath = concat(pwszPath, wstrPath.get());
		
		hItem = hParent;
	}
	
	return wstrPath;
}

void qs::BrowseFolderDialogImpl::selectFolder(HWND hwnd,
											  const WCHAR* pwszPath)
{
	assert(hwnd);
	assert(pwszPath);
	
	if (pwszPath[0] != L'\\')
		return;
	selectFolder(hwnd, TreeView_GetRoot(hwnd), pwszPath + 1);
}

void qs::BrowseFolderDialogImpl::selectFolder(HWND hwnd,
											  HTREEITEM hItem,
											  const WCHAR* pwszPath)
{
	assert(hwnd);
	assert(hItem);
	assert(pwszPath);
	
	wstring_ptr wstrName;
	const WCHAR* p = wcschr(pwszPath, L'\\');
	if (p)
		wstrName = allocWString(pwszPath, p - pwszPath);
	else
		wstrName = allocWString(pwszPath);
	
	hItem = TreeView_GetChild(hwnd, hItem);
	while (hItem) {
		WCHAR wszName[MAX_PATH + 1];
		TVITEM item = {
			TVIF_HANDLE | TVIF_TEXT,
			hItem,
			0,
			0,
			wszName,
			countof(wszName) - 1,
		};
		TreeView_GetItem(hwnd, &item);
		if (wcsicmp(wstrName.get(), wszName) == 0)
			break;
		hItem = TreeView_GetNextSibling(hwnd, hItem);
	}
	if (!hItem)
		return;
	
	TreeView_SelectItem(hwnd, hItem);
	if (p) {
		TreeView_Expand(hwnd, hItem, TVE_EXPAND);
		selectFolder(hwnd, hItem, p + 1);
	}
}

void qs::BrowseFolderDialogImpl::refreshFolder(HWND hwnd,
											   HTREEITEM hItem)
{
	TreeView_Expand(hwnd, hItem, TVE_COLLAPSE | TVE_COLLAPSERESET);
	
	wstring_ptr wstrPath(getPath(hwnd, hItem));
	wstrPath = concat(wstrPath.get(), L"\\*.*");
	
	WIN32_FIND_DATA fd;
	AutoFindHandle hFind(::FindFirstFile(wstrPath.get(), &fd));
	if (hFind.get()) {
		TV_INSERTSTRUCT tis = {
			hItem,
			TVI_SORT,
			{
				TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE,
				0,
				0,
				0,
				0,
				0,
				1,
				2
			}
		};
		do {
			if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
				_tcscmp(fd.cFileName, _T(".")) != 0 &&
				_tcscmp(fd.cFileName, _T("..")) != 0) {
				tis.item.pszText = fd.cFileName;
				TreeView_InsertItem(hwnd, &tis);
			}
		} while (::FindNextFile(hFind.get(), &fd));
	}
}

LRESULT qs::BrowseFolderDialogImpl::onNotify(NMHDR* pnmhdr,
											 bool* pbHandled)
{
	BEGIN_NOTIFY_HANDLER()
		HANDLE_NOTIFY(TVN_ITEMEXPANDING, IDC_FOLDER, onItemExpanding);
	END_NOTIFY_HANDLER()
	return NotifyHandler::onNotify(pnmhdr, pbHandled);
}

LRESULT qs::BrowseFolderDialogImpl::onItemExpanding(NMHDR* pnmhdr,
													bool* pbHandled)
{
	NMTREEVIEW* pnmtv = reinterpret_cast<NMTREEVIEW*>(pnmhdr);
	
	HWND hwnd = pnmtv->hdr.hwndFrom;
	HTREEITEM hItem = pnmtv->itemNew.hItem;
	
	TVITEM item = {
		TVIF_HANDLE | TVIF_STATE,
		hItem
	};
	TreeView_GetItem(hwnd, &item);
	if ((item.state & TVIS_EXPANDEDONCE) == 0) {
		hItem = TreeView_GetChild(hwnd, hItem);
		while (hItem) {
			refreshFolder(hwnd, hItem);
			hItem = TreeView_GetNextSibling(hwnd, hItem);
		}
	}
	
	*pbHandled = true;
	
	return 0;
}


/****************************************************************************
 *
 * BrowseFolderDialog
 *
 */

qs::BrowseFolderDialog::BrowseFolderDialog(const WCHAR* pwszTitle,
										   const WCHAR* pwszInitialPath) :
	DefaultDialog(getResourceDllInstanceHandle(), IDD_BROWSEFOLDER, LANDSCAPE(IDD_BROWSEFOLDER)),
	pImpl_(0)
{
	wstring_ptr wstrPath;
	if (pwszInitialPath)
		wstrPath = allocWString(pwszInitialPath);
	
	pImpl_ = new BrowseFolderDialogImpl();
	pImpl_->wstrPath_ = wstrPath;
	
	addNotifyHandler(pImpl_);
}

qs::BrowseFolderDialog::~BrowseFolderDialog()
{
	delete pImpl_;
}

const WCHAR* qs::BrowseFolderDialog::getPath() const
{
	return pImpl_->wstrPath_.get();
}

LRESULT qs::BrowseFolderDialog::onCommand(WORD nCode,
										  WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDC_NEWFOLDER, onNewFolder)
	END_COMMAND_HANDLER()
	return DefaultDialog::onCommand(nCode, nId);
}

LRESULT qs::BrowseFolderDialog::onDestroy()
{
	HWND hwnd = getDlgItem(IDC_FOLDER);
	HIMAGELIST hImageList = TreeView_GetImageList(hwnd, TVSIL_NORMAL);
	ImageList_Destroy(hImageList);
	
	removeNotifyHandler(pImpl_);
	
	return DefaultDialog::onDestroy();
}

LRESULT qs::BrowseFolderDialog::onInitDialog(HWND hwndFocus,
											 LPARAM lParam)
{
	init(false);
	
	HWND hwnd = getDlgItem(IDC_FOLDER);
	
	HIMAGELIST hImageList = ImageList_LoadBitmap(getResourceDllInstanceHandle(),
		MAKEINTRESOURCE(IDB_BROWSEFOLDER), 16, 10, RGB(255, 255, 255));
	TreeView_SetImageList(hwnd, hImageList, TVSIL_NORMAL);
	
	TV_INSERTSTRUCT tis = {
		TVI_ROOT,
		TVI_SORT,
		{
			TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE,
			0,
			0,
			0,
			_T("My Computer"),
			0,
			0,
			0
		}
	};
	HTREEITEM hItem = TreeView_InsertItem(hwnd, &tis);
	
	tis.hParent = hItem;
	tis.item.iImage = 1;
	tis.item.iSelectedImage = 2;
	
	WIN32_FIND_DATA fd;
	AutoFindHandle hFind(::FindFirstFile(_T("\\*.*"), &fd));
	if (!hFind.get())
		return TRUE;
	do {
		if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
			_tcscmp(fd.cFileName, _T(".")) != 0 &&
			_tcscmp(fd.cFileName, _T("..")) != 0) {
			tis.item.pszText = fd.cFileName;
			TreeView_InsertItem(hwnd, &tis);
		}
	} while (::FindNextFile(hFind.get(), &fd));
	
	TreeView_Expand(hwnd, TreeView_GetRoot(hwnd), TVE_EXPAND);
	
	if (pImpl_->wstrPath_.get())
		pImpl_->selectFolder(hwnd, pImpl_->wstrPath_.get());
	
	return TRUE;
}

LRESULT qs::BrowseFolderDialog::onOk()
{
	HWND hwnd = getDlgItem(IDC_FOLDER);
	HTREEITEM hItem = TreeView_GetSelection(hwnd);
	if (hItem)
		pImpl_->wstrPath_ = pImpl_->getPath(hwnd, hItem);
	
	return DefaultDialog::onOk();
}

LRESULT qs::BrowseFolderDialog::onNewFolder()
{
	HWND hwnd = getDlgItem(IDC_FOLDER);
	HTREEITEM hItem = TreeView_GetSelection(hwnd);
	if (hItem) {
		wstring_ptr wstrPath = pImpl_->getPath(hwnd, hItem);
		FolderNameDialog dialog;
		if (dialog.doModal(getHandle())) {
			const WCHAR* pwszName = dialog.getName();
			wstring_ptr wstrNewPath(concat(wstrPath.get(), L"\\", pwszName));
			if (File::createDirectory(wstrNewPath.get())) {
				pImpl_->refreshFolder(hwnd, hItem);
				TreeView_Expand(hwnd, hItem, TVE_EXPAND);
			}
			else {
				messageBox(getResourceDllInstanceHandle(),
					IDS_ERROR_CREATEFOLDER, MB_OK | MB_ICONERROR, getHandle());
			}
		}
	}
	
	return 0;
}

#endif


/****************************************************************************
 *
 * FontDialog
 *
 */

static int CALLBACK enumFontFamProc(ENUMLOGFONT* pelf,
									NEWTEXTMETRIC* pntm,
									int nFontType,
									LPARAM lParam)
{
	HWND hwnd = reinterpret_cast<HWND>(lParam);
	Window(hwnd).sendMessage(CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(pelf->elfLogFont.lfFaceName));
	return 1;
}

qs::FontDialog::FontDialog(const LOGFONT& lf) :
	DefaultDialog(getResourceDllInstanceHandle(), IDD_FONT, LANDSCAPE(IDD_FONT)),
	lf_(lf)
{
}

qs::FontDialog::~FontDialog()
{
}

const LOGFONT& qs::FontDialog::getLogFont() const
{
	return lf_;
}

LRESULT qs::FontDialog::onInitDialog(HWND hwndFocus,
									 LPARAM lParam)
{
	init(false);
	
	ClientDeviceContext dc(0);
	dc.enumFontFamilies(0, reinterpret_cast<FONTENUMPROC>(enumFontFamProc),
		reinterpret_cast<LPARAM>(getDlgItem(IDC_FONTFACE)));
	sendDlgItemMessage(IDC_FONTFACE, CB_SELECTSTRING, 0, reinterpret_cast<LPARAM>(lf_.lfFaceName));
	T2W(lf_.lfFaceName, pwszFaceName);
	setDlgItemText(IDC_FONTFACE, pwszFaceName);
	
	UINT nStyleIds[] = {
		IDS_REGULAR,
		IDS_ITALIC,
		IDS_BOLD,
		IDS_BOLDITALIC
	};
	for (int n = 0; n < countof(nStyleIds); ++n) {
		wstring_ptr wstrStyle(loadString(getResourceDllInstanceHandle(), nStyleIds[n]));
		W2T(wstrStyle.get(), ptszStyle);
		sendDlgItemMessage(IDC_FONTSTYLE, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(ptszStyle));
	}
	UINT nId = IDS_REGULAR;
	if (lf_.lfWeight >= FW_BOLD)
		nId = lf_.lfItalic ? IDS_BOLDITALIC : IDS_BOLD;
	else
		nId = lf_.lfItalic ? IDS_ITALIC : IDS_REGULAR;
	wstring_ptr wstrStyle(loadString(getResourceDllInstanceHandle(), nId));
	W2T(wstrStyle.get(), ptszStyle);
	sendDlgItemMessage(IDC_FONTSTYLE, CB_SELECTSTRING, 0, reinterpret_cast<LPARAM>(ptszStyle));
	setDlgItemText(IDC_FONTSTYLE, wstrStyle.get());
	
	const WCHAR* pwszSizes[] = {
		L"8",
		L"9",
		L"10",
		L"10.5",
		L"11",
		L"12",
		L"14",
		L"16",
		L"18",
		L"20",
		L"24",
		L"26",
		L"28",
		L"36",
		L"48",
		L"72"
	};
	for (int n = 0; n < countof(pwszSizes); ++n) {
		W2T(pwszSizes[n], ptszSize);
		sendDlgItemMessage(IDC_FONTSIZE, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(ptszSize));
	}
	double dPointSize = -lf_.lfHeight*72.0/dc.getDeviceCaps(LOGPIXELSY);
	WCHAR wszSize[64];
	_snwprintf(wszSize, countof(wszSize), L"%.1lf", dPointSize);
	size_t nLen = wcslen(wszSize);
	if (nLen > 2 && wcscmp(wszSize + nLen - 2, L".0") == 0)
		wszSize[nLen - 2] = L'\0';
	W2T(wszSize, ptszSize);
	sendDlgItemMessage(IDC_FONTSIZE, CB_SELECTSTRING, 0, reinterpret_cast<LPARAM>(ptszSize));
	setDlgItemText(IDC_FONTSIZE, wszSize);
	
	return TRUE;
}

LRESULT qs::FontDialog::onOk()
{
	wstring_ptr wstrFaceName(getDlgItemText(IDC_FONTFACE));
	W2T(wstrFaceName.get(), ptszFaceName);
	_tcsncpy(lf_.lfFaceName, ptszFaceName, countof(lf_.lfFaceName));
	
	wstring_ptr wstrStyle(getDlgItemText(IDC_FONTSTYLE));
	wstring_ptr wstrBold(loadString(getResourceDllInstanceHandle(), IDS_BOLD));
	lf_.lfWeight = wcsstr(wstrStyle.get(), wstrBold.get()) ? FW_BOLD : FW_NORMAL;
	wstring_ptr wstrItalic(loadString(getResourceDllInstanceHandle(), IDS_ITALIC));
	lf_.lfItalic = wcsstr(wstrStyle.get(), wstrItalic.get()) ? TRUE : FALSE;
	
	wstring_ptr wstrSize(getDlgItemText(IDC_FONTSIZE));
	WCHAR* pEnd = 0;
	double dPointSize = wcstod(wstrSize.get(), &pEnd);
	if (*pEnd)
		dPointSize = 9;
	ClientDeviceContext dc(0);
	double dHeight = dPointSize*dc.getDeviceCaps(LOGPIXELSY)/72;
	long nHeight = static_cast<long>(dHeight);
	if (dHeight - nHeight > 0.5)
		++nHeight;
	lf_.lfHeight = -nHeight;
	
	return DefaultDialog::onOk();
}


#ifdef _WIN32_WCE

/****************************************************************************
 *
 * FolderNameDialog
 *
 */

qs::FolderNameDialog::FolderNameDialog() :
	DefaultDialog(getResourceDllInstanceHandle(), IDD_FOLDERNAME, LANDSCAPE(IDD_FOLDERNAME))
{
}

qs::FolderNameDialog::~FolderNameDialog()
{
}

const WCHAR* qs::FolderNameDialog::getName() const
{
	return wstrName_.get();
}

LRESULT qs::FolderNameDialog::onOk()
{
	wstrName_ = getDlgItemText(IDC_FOLDERNAME);
	return DefaultDialog::onOk();
}

#endif
