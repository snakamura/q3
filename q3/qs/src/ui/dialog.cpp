/*
 * $Id: dialog.cpp,v 1.3 2003/06/01 16:27:36 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsconv.h>
#include <qsdialog.h>
#include <qserror.h>
#include <qsinit.h>
#include <qsnew.h>
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

#pragma warning(disable:4786)

using namespace qs;


/****************************************************************************
 *
 * DialogBaseImpl
 *
 */

DialogBaseImpl::DialogMap* qs::DialogBaseImpl::pMap__;
ThreadLocal* qs::DialogBaseImpl::pModelessList__;
DialogBaseImpl::InitializerImpl qs::DialogBaseImpl::init__;

LRESULT qs::DialogBaseImpl::notifyCommandHandlers(WORD wCode, WORD wId) const
{
	CommandHandlerList::const_iterator it = listCommandHandler_.begin();
	while (it != listCommandHandler_.end()) {
		LRESULT lResult = (*it)->onCommand(wCode, wId);
		if (lResult == 0)
			return lResult;
		++it;
	}
	return 1;
}

void qs::DialogBaseImpl::updateCommandHandlers(CommandUpdate* pcu) const
{
	CommandHandlerList::const_iterator it = listCommandHandler_.begin();
	while (it != listCommandHandler_.end()) {
		(*it)->updateCommand(pcu);
		++it;
	}
}

LRESULT qs::DialogBaseImpl::notifyNotifyHandlers(
	NMHDR* pnmhdr, bool* pbHandled) const
{
	assert(pbHandled);
	
	NotifyHandlerList::const_iterator it = listNotifyHandler_.begin();
	while (it != listNotifyHandler_.end()) {
		LRESULT lResult = (*it)->onNotify(pnmhdr, pbHandled);
		if (*pbHandled)
			return lResult;
		++it;
	}
	return 1;
}

void qs::DialogBaseImpl::notifyOwnerDrawHandlers(
	DRAWITEMSTRUCT* pDrawItem) const
{
	OwnerDrawHandlerList::const_iterator it = listOwnerDrawHandler_.begin();
	while (it != listOwnerDrawHandler_.end()) {
		(*it)->onDrawItem(pDrawItem);
		++it;
	}
}

void qs::DialogBaseImpl::measureOwnerDrawHandlers(
	MEASUREITEMSTRUCT* pMeasureItem) const
{
	OwnerDrawHandlerList::const_iterator it = listOwnerDrawHandler_.begin();
	while (it != listOwnerDrawHandler_.end()) {
		(*it)->onMeasureItem(pMeasureItem);
		++it;
	}
}

INT_PTR qs::DialogBaseImpl::dialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	DECLARE_QSTATUS();
	
	INT_PTR nResult = 0;
	switch (uMsg) {
	case WM_COMMAND:
		nResult = notifyCommandHandlers(HIWORD(wParam), LOWORD(wParam));
		if (nResult == 0)
			return TRUE;
		break;
	
	case WM_NOTIFY:
		{
			bool bHandled = false;
			nResult = notifyNotifyHandlers(reinterpret_cast<NMHDR*>(lParam), &bHandled);
			if (bHandled)
				return TRUE;
		}
		break;
	
	case WM_DRAWITEM:
		notifyOwnerDrawHandlers(reinterpret_cast<LPDRAWITEMSTRUCT>(lParam));
		break;
	
	case WM_MEASUREITEM:
		measureOwnerDrawHandlers(reinterpret_cast<LPMEASUREITEMSTRUCT>(lParam));
		break;
	
	default:
		break;
	}
	
	nResult = pDialogHandler_->dialogProc(uMsg, wParam, lParam);
	
	switch (uMsg) {
#if defined _WIN32_WCE && !defined _WIN32_WCE_EMULATION
	case WM_DESTROY:
#elif defined _WIN32_WCE_EMULATION
	case 0x82:		// WM_NCDESTROY is not defined winuser.h
#else
	case WM_NCDESTROY:
#endif // _WIN32_WCE
		{
			DialogMap* pMap = 0;
			status = DialogBaseImpl::getDialogMap(&pMap);
			CHECK_QSTATUS_VALUE(FALSE);
			pMap->removeController(pThis_->getHandle());
			DialogBaseImpl::removeModelessDialog(pThis_);
			assert(listCommandHandler_.size() == 0);
			assert(listNotifyHandler_.size() == 0);
			assert(listOwnerDrawHandler_.size() == 0);
			if (bDeleteThis_)
				delete pThis_;
		}
		break;
	}
	
	return nResult;
}

QSTATUS qs::DialogBaseImpl::getDialogMap(DialogMap** ppMap)
{
	assert(ppMap);
	*ppMap = pMap__;
	return QSTATUS_SUCCESS;
}

QSTATUS qs::DialogBaseImpl::getModelessList(const ModelessList** ppList)
{
	assert(ppList);
	
	DECLARE_QSTATUS();
	
	*ppList = 0;
	
	void* pValue = 0;
	status = pModelessList__->get(&pValue);
	CHECK_QSTATUS();
	
	*ppList = static_cast<ModelessList*>(pValue);
	assert(*ppList);
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::DialogBaseImpl::addModelessDialog(DialogBase* pDialogBase)
{
	DECLARE_QSTATUS();
	
	void* pValue = 0;
	status = pModelessList__->get(&pValue);
	CHECK_QSTATUS();
	
	ModelessList* pList = static_cast<ModelessList*>(pValue);
	assert(pList);
	
	return STLWrapper<ModelessList>(*pList).push_back(pDialogBase);
}

QSTATUS qs::DialogBaseImpl::removeModelessDialog(DialogBase* pDialogBase)
{
	DECLARE_QSTATUS();
	
	void* pValue = 0;
	status = pModelessList__->get(&pValue);
	CHECK_QSTATUS();
	
	ModelessList* pList = static_cast<ModelessList*>(pValue);
	assert(pList);
	
	ModelessList::iterator it = std::remove(
		pList->begin(), pList->end(), pDialogBase);
	pList->erase(it, pList->end());
	
	return QSTATUS_SUCCESS;
}

qs::DialogBaseImpl::InitializerImpl::InitializerImpl()
{
}

qs::DialogBaseImpl::InitializerImpl::~InitializerImpl()
{
}

QSTATUS qs::DialogBaseImpl::InitializerImpl::init()
{
	DECLARE_QSTATUS();
	
	status = newQsObject(&DialogBaseImpl::pMap__);
	CHECK_QSTATUS();
	
	status = newQsObject(&DialogBaseImpl::pModelessList__);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::DialogBaseImpl::InitializerImpl::term()
{
	delete DialogBaseImpl::pModelessList__;
	DialogBaseImpl::pModelessList__ = 0;
	
	delete DialogBaseImpl::pMap__;
	DialogBaseImpl::pMap__ = 0;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::DialogBaseImpl::InitializerImpl::initThread()
{
	DECLARE_QSTATUS();
	
	status = DialogBaseImpl::pMap__->initThread();
	CHECK_QSTATUS();
	
	ModelessList* pList = 0;
	status = newObject(&pList);
	CHECK_QSTATUS();
	assert(pList);
	std::auto_ptr<ModelessList> apList(pList);
	status = pModelessList__->set(pList);
	CHECK_QSTATUS();
	apList.release();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::DialogBaseImpl::InitializerImpl::termThread()
{
	void* pValue = 0;
	QSTATUS status = pModelessList__->get(&pValue);
	if (status == QSTATUS_SUCCESS)
		delete static_cast<ModelessList*>(pValue);
	
	DialogBaseImpl::pMap__->termThread();
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * DialogBase
 *
 */

qs::DialogBase::DialogBase(bool bDeleteThis, QSTATUS* pstatus) :
	Window(0)
{
	assert(pstatus);
	
	*pstatus = QSTATUS_SUCCESS;
	
	DECLARE_QSTATUS();
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	assert(pImpl_);
	pImpl_->pThis_ = this;
	pImpl_->bDeleteThis_ = bDeleteThis;
	pImpl_->pDialogHandler_ = 0;
	pImpl_->bDeleteHandler_ = false;
}

qs::DialogBase::~DialogBase()
{
	if (pImpl_) {
		if (pImpl_->bDeleteHandler_)
			delete pImpl_->pDialogHandler_;
		delete pImpl_;
	}
}

QSTATUS qs::DialogBase::setDialogHandler(
	DialogHandler* pDialogHandler, bool bDeleteHandler)
{
	DECLARE_QSTATUS();
	
	pImpl_->pDialogHandler_ = pDialogHandler;
	pImpl_->bDeleteHandler_ = bDeleteHandler;
	
	status = pDialogHandler->setDialogBase(this);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::DialogBase::addCommandHandler(CommandHandler* pch)
{
	assert(pImpl_);
	return STLWrapper<DialogBaseImpl::CommandHandlerList>
		(pImpl_->listCommandHandler_).push_back(pch);
}

QSTATUS qs::DialogBase::removeCommandHandler(CommandHandler* pch)
{
	assert(pImpl_);
	DialogBaseImpl::CommandHandlerList::iterator it =
		std::remove(pImpl_->listCommandHandler_.begin(),
			pImpl_->listCommandHandler_.end(), pch);
	assert(it != pImpl_->listCommandHandler_.end());
	pImpl_->listCommandHandler_.erase(it, pImpl_->listCommandHandler_.end());
	return QSTATUS_SUCCESS;
}

QSTATUS qs::DialogBase::addNotifyHandler(NotifyHandler* pnh)
{
	assert(pImpl_);
	return STLWrapper<DialogBaseImpl::NotifyHandlerList>
		(pImpl_->listNotifyHandler_).push_back(pnh);
}

QSTATUS qs::DialogBase::removeNotifyHandler(NotifyHandler* pnh)
{
	assert(pImpl_);
	DialogBaseImpl::NotifyHandlerList::iterator it =
		std::remove(pImpl_->listNotifyHandler_.begin(),
			pImpl_->listNotifyHandler_.end(), pnh);
	assert(it != pImpl_->listNotifyHandler_.end());
	pImpl_->listNotifyHandler_.erase(it, pImpl_->listNotifyHandler_.end());
	return QSTATUS_SUCCESS;
}

QSTATUS qs::DialogBase::addOwnerDrawHandler(OwnerDrawHandler* podh)
{
	assert(pImpl_);
	return STLWrapper<DialogBaseImpl::OwnerDrawHandlerList>
		(pImpl_->listOwnerDrawHandler_).push_back(podh);
}

QSTATUS qs::DialogBase::removeOwnerDrawHandler(OwnerDrawHandler* podh)
{
	assert(pImpl_);
	DialogBaseImpl::OwnerDrawHandlerList::iterator it =
		std::remove(pImpl_->listOwnerDrawHandler_.begin(),
			pImpl_->listOwnerDrawHandler_.end(), podh);
	assert(it != pImpl_->listOwnerDrawHandler_.end());
	pImpl_->listOwnerDrawHandler_.erase(it, pImpl_->listOwnerDrawHandler_.end());
	return QSTATUS_SUCCESS;
}

LRESULT qs::DialogBase::defWindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return ::DefWindowProc(getHandle(), uMsg, wParam, lParam);
}

QSTATUS qs::DialogBase::processDialogMessage(MSG* pMsg, bool* pbProcessed)
{
	assert(pbProcessed);
	
	*pbProcessed = false;
	
	if ((pMsg->message < WM_KEYFIRST || WM_KEYLAST < pMsg->message) &&
		(pMsg->message < WM_MOUSEFIRST || WM_MOUSELAST < pMsg->message))
		return QSTATUS_SUCCESS;
	
	DECLARE_QSTATUS();
	
	const DialogBaseImpl::ModelessList* pList = 0;
	status = DialogBaseImpl::getModelessList(&pList);
	CHECK_QSTATUS();
	
	DialogBaseImpl::ModelessList::const_iterator it = pList->begin();
	while (it != pList->end() && !(*it)->isDialogMessage(pMsg))
		++it;
	*pbProcessed = it != pList->end();
	
	return QSTATUS_SUCCESS;
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

qs::Dialog::Dialog(HINSTANCE hInstResource, UINT nId,
	bool bDeleteThis, QSTATUS* pstatus) :
	DialogBase(bDeleteThis, pstatus),
	pImpl_(0)
{
	assert(pstatus);
	
	if (*pstatus != QSTATUS_SUCCESS)
		return;
	
	DECLARE_QSTATUS();
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	assert(pImpl_);
	pImpl_->hInstResource_ = hInstResource;
	pImpl_->nId_ = nId;
}

qs::Dialog::~Dialog()
{
	delete pImpl_;
}

QSTATUS qs::Dialog::doModal(HWND hwndParent,
	ModalHandler* pModalHandler, int* pnRet)
{
	assert(pnRet);
	
	DECLARE_QSTATUS();
	
	if (!pModalHandler)
		pModalHandler = getModalHandler();
	
	ModalHandlerInvoker invoker(pModalHandler, hwndParent, &status);
	CHECK_QSTATUS();
	DialogBaseImpl::DialogMap* pMap = 0;
	status = DialogBaseImpl::getDialogMap(&pMap);
	CHECK_QSTATUS();
	status = pMap->setThis(this);
	CHECK_QSTATUS();
	*pnRet = ::DialogBox(pImpl_->hInstResource_,
		MAKEINTRESOURCE(pImpl_->nId_), hwndParent, dialogProc);
	if (*pnRet == 0 || *pnRet == -1)
		return QSTATUS_FAIL;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::Dialog::create(HWND hwndParent)
{
	DECLARE_QSTATUS();
	
	DialogBaseImpl::DialogMap* pMap = 0;
	status = DialogBaseImpl::getDialogMap(&pMap);
	CHECK_QSTATUS();
	status = pMap->setThis(this);
	CHECK_QSTATUS();
	HWND hwnd = ::CreateDialog(pImpl_->hInstResource_,
		MAKEINTRESOURCE(pImpl_->nId_), hwndParent, dialogProc);
	if (!hwnd) {
		setHandle(0);
		return QSTATUS_FAIL;
	}
	assert(getHandle() == hwnd);
	
	DialogBaseImpl::addModelessDialog(this);
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::Dialog::endDialog(int nCode)
{
	return ::EndDialog(getHandle(), nCode) ? QSTATUS_SUCCESS : QSTATUS_FAIL;
}

INT_PTR CALLBACK qs::dialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	DECLARE_QSTATUS();
	
	DialogBaseImpl::DialogMap* pMap = 0;
	status = DialogBaseImpl::getDialogMap(&pMap);
	CHECK_QSTATUS();
	DialogBase* pThis = 0;
	status = pMap->findController(hwnd, &pThis);
	CHECK_QSTATUS_VALUE(0);
	
	return pThis->pImpl_->dialogProc(uMsg, wParam, lParam);
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

qs::DefaultDialogHandler::DefaultDialogHandler(QSTATUS* pstatus) :
	pDialogBase_(0),
	nResult_(TRUE)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qs::DefaultDialogHandler::~DefaultDialogHandler()
{
}

DialogBase* qs::DefaultDialogHandler::getDialogBase() const
{
	assert(pDialogBase_);
	return pDialogBase_;
}

QSTATUS qs::DefaultDialogHandler::setDialogBase(DialogBase* pDialogBase)
{
	assert(!pDialogBase_);
	pDialogBase_ = pDialogBase;
	return QSTATUS_SUCCESS;
}

INT_PTR qs::DefaultDialogHandler::dialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
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

qs::DefaultDialog::DefaultDialog(HINSTANCE hInst, UINT nId, QSTATUS* pstatus) :
	Dialog(hInst, nId, false, pstatus),
	DefaultDialogHandler(pstatus),
	DefaultCommandHandler(pstatus)
{
	DECLARE_QSTATUS();
	
	status = addCommandHandler(this);
	CHECK_QSTATUS_SET(pstatus);
	
	setDialogHandler(this, false);
}

qs::DefaultDialog::~DefaultDialog()
{
}

void qs::DefaultDialog::init(bool bDoneButton)
{
#if defined _WIN32_WCE && _WIN32_WCE >= 300 && defined _WIN32_WCE_PSPC
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

INT_PTR qs::DefaultDialog::dialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	BEGIN_DIALOG_HANDLER()
		HANDLE_DESTROY()
		HANDLE_INITDIALOG()
	END_DIALOG_HANDLER()
	return DefaultDialogHandler::dialogProc(uMsg, wParam, lParam);
}

LRESULT qs::DefaultDialog::onCommand(WORD nCode, WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID(IDCANCEL, onCancel)
		HANDLE_COMMAND_ID(IDOK, onOk)
	END_COMMAND_HANDLER()
	return 1;
}

LRESULT qs::DefaultDialog::onDestroy()
{
	removeCommandHandler(this);
	return 0;
}

LRESULT qs::DefaultDialog::onInitDialog(HWND hwndFocus, LPARAM lParam)
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
	WSTRING wstrPath_;
};


/****************************************************************************
 *
 * FileDialog
 *
 */

qs::FileDialog::FileDialog(bool bOpen, const WCHAR* pwszFilter,
	const WCHAR* pwszInitialDir, const WCHAR* pwszDefaultExt,
	const WCHAR* pwszFileName, DWORD dwFlags, QSTATUS* pstatus) :
	pImpl_(0)
{
	DECLARE_QSTATUS();
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	pImpl_->bOpen_ = bOpen;
	pImpl_->pwszFilter_ = pwszFilter;
	pImpl_->pwszInitialDir_ = pwszInitialDir ? pwszInitialDir : L"";
	pImpl_->pwszDefaultExt_ = pwszDefaultExt ? pwszDefaultExt : L"";
	pImpl_->pwszFileName_ = pwszFileName;
	pImpl_->dwFlags_ = dwFlags;
	pImpl_->wstrPath_ = 0;
}

qs::FileDialog::~FileDialog()
{
	if (pImpl_) {
		freeWString(pImpl_->wstrPath_);
		delete pImpl_;
	}
}

const WCHAR* qs::FileDialog::getPath() const
{
	return pImpl_->wstrPath_;
}

QSTATUS qs::FileDialog::doModal(HWND hwndParent,
	ModalHandler* pModalHandler, int* pnRet)
{
	assert(pnRet);
	
	DECLARE_QSTATUS();
	
#ifdef _WIN32_WCE
	bool bMultiSelect = false;
#else
	bool bMultiSelect = (pImpl_->dwFlags_ & OFN_ALLOWMULTISELECT) != 0;
#endif
	
	if (!pModalHandler)
		pModalHandler = getModalHandler();
	
	OPENFILENAME ofn = {
		sizeof(ofn),
		hwndParent,
		getInstanceHandle()
	};
	
	string_ptr<TSTRING> ptstrFilter;
	if (pImpl_->pwszFilter_) {
		ptstrFilter.reset(wcs2tcs(pImpl_->pwszFilter_));
		for (TCHAR* p = ptstrFilter.get(); *p; ++p) {
			if (*p == _T('|'))
				*p = _T('\0');
		}
		ofn.lpstrFilter = ptstrFilter.get();
	}
	
	int nLen = MAX_PATH;
	if (bMultiSelect)
		nLen *= 10;
	string_ptr<TSTRING> tstrPath(allocTString(nLen));
	if (!tstrPath.get())
		return QSTATUS_OUTOFMEMORY;
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
	
	if (pModalHandler)
		pModalHandler->preModalDialog(hwndParent);
	
	BOOL b = pImpl_->bOpen_ ? ::GetOpenFileName(&ofn) :
		::GetSaveFileName(&ofn);
	
	if (pModalHandler)
		pModalHandler->postModalDialog(hwndParent);
	
	T2W(ofn.lpstrFile, pwszPath);
	string_ptr<WSTRING> wstrPath(allocWString(pwszPath, wcslen(pwszPath) + 2));
	if (!wstrPath.get())
		return QSTATUS_OUTOFMEMORY;
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
				status = STLWrapper<Buffer>(buf).reserve(
					nPathLen + nFileNameLen + 3);
				CHECK_QSTATUS();
				std::copy(wstrPath.get(), wstrPath.get() + nPathLen,
					std::back_inserter(buf));
				buf.push_back(L'\\');
				std::copy(pwszFileName, pwszFileName + nFileNameLen,
					std::back_inserter(buf));
				buf.push_back(L'\0');
				
				p += _tcslen(p) + 1;
			}
			buf.push_back(L'\0');
			
			wstrPath.reset(allocWString(buf.size()));
			if (!wstrPath.get())
				return QSTATUS_OUTOFMEMORY;
			memcpy(wstrPath.get(), &buf[0], buf.size()*sizeof(WCHAR));
		}
	}
	pImpl_->wstrPath_ = wstrPath.release();
	
	*pnRet = b ? IDOK : IDCANCEL;
	
	return QSTATUS_SUCCESS;
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
	QSTATUS getPath(HWND hwnd, HTREEITEM hItem, WSTRING* pwstrPath);
	QSTATUS selectFolder(HWND hwnd, const WCHAR* pwszPath);
	QSTATUS selectFolder(HWND hwnd, HTREEITEM hItem, const WCHAR* pwszPath);

public:
	virtual LRESULT onNotify(NMHDR* pnmhdr, bool* pbHandled);

private:
	LRESULT onItemExpanding(NMHDR* pnmhdr, bool* pbHandled);

public:
	WSTRING wstrPath_;
};

QSTATUS qs::BrowseFolderDialogImpl::getPath(
	HWND hwnd, HTREEITEM hItem, WSTRING* pwstrPath)
{
	assert(hwnd);
	assert(hItem);
	assert(pwstrPath);
	
	DECLARE_QSTATUS();
	
	*pwstrPath = 0;
	
	string_ptr<WSTRING> wstrPath;
	
	while (hItem) {
		TCHAR tszPath[MAX_PATH + 1];
		*tszPath = _T('\\');
		TVITEM ti = {
			TVIF_HANDLE | TVIF_TEXT,
			hItem,
			0,
			0,
			tszPath + 1,
			countof(tszPath) - 1
		};
		TreeView_GetItem(hwnd, &ti);
		hItem = TreeView_GetParent(hwnd, hItem);
		if (hItem) {
			if (wstrPath.get())
				wstrPath.reset(concat(tszPath, wstrPath.get()));
			else
				wstrPath.reset(allocWString(tszPath));
			if (!wstrPath.get())
				return QSTATUS_OUTOFMEMORY;
		}
	}
	
	*pwstrPath = wstrPath.release();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::BrowseFolderDialogImpl::selectFolder(
	HWND hwnd, const WCHAR* pwszPath)
{
	assert(hwnd);
	assert(pwszPath);
	
	if (pwszPath[0] != L'\\')
		return QSTATUS_SUCCESS;
	return selectFolder(hwnd, TreeView_GetRoot(hwnd), pwszPath + 1);
}

QSTATUS qs::BrowseFolderDialogImpl::selectFolder(
	HWND hwnd, HTREEITEM hItem, const WCHAR* pwszPath)
{
	assert(hwnd);
	assert(hItem);
	assert(pwszPath);
	
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrName;
	const WCHAR* p = wcsrchr(pwszPath, L'\\');
	if (p)
		wstrName.reset(allocWString(pwszPath, p - pwszPath));
	else
		wstrName.reset(allocWString(pwszPath));
	if (!wstrName.get())
		return QSTATUS_OUTOFMEMORY;
	
	hItem = TreeView_GetChild(hwnd, hItem);
	while (hItem) {
		TCHAR tszName[MAX_PATH + 1];
		TVITEM item = {
			TVIF_HANDLE | TVIF_TEXT,
			hItem,
			0,
			0,
			tszName,
			countof(tszName) - 1,
		};
		TreeView_GetItem(hwnd, &item);
		if (wcsicmp(wstrName.get(), tszName) == 0)
			break;
		hItem = TreeView_GetNextSibling(hwnd, hItem);
	}
	if (!hItem)
		return QSTATUS_SUCCESS;
	
	TreeView_SelectItem(hwnd, hItem);
	if (p) {
		TreeView_Expand(hwnd, hItem, TVE_EXPAND);
		status = selectFolder(hwnd, hItem, p + 1);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

LRESULT qs::BrowseFolderDialogImpl::onNotify(NMHDR* pnmhdr, bool* pbHandled)
{
	BEGIN_NOTIFY_HANDLER()
		HANDLE_NOTIFY(TVN_ITEMEXPANDING, IDC_FOLDER, onItemExpanding);
	END_NOTIFY_HANDLER()
	return 1;
}

LRESULT qs::BrowseFolderDialogImpl::onItemExpanding(NMHDR* pnmhdr, bool* pbHandled)
{
	DECLARE_QSTATUS();
	
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
			string_ptr<WSTRING> wstrPath;
			status = getPath(hwnd, hItem, &wstrPath);
			CHECK_QSTATUS_VALUE(0);
			wstrPath.reset(concat(wstrPath.get(), L"\\*.*"));
			if (!wstrPath.get())
				return 0;
			
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
	const WCHAR* pwszInitialPath, QSTATUS* pstatus) :
	DefaultDialog(getDllInstanceHandle(), IDD_BROWSEFOLDER, pstatus),
	pImpl_(0)
{
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrPath;
	if (pwszInitialPath) {
		wstrPath.reset(allocWString(pwszInitialPath));
		if (!wstrPath.get()) {
			*pstatus = QSTATUS_OUTOFMEMORY;
			return;
		}
	}
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	pImpl_->wstrPath_ = wstrPath.release();
	
	status = addNotifyHandler(pImpl_);
	CHECK_QSTATUS_SET(pstatus);
}

qs::BrowseFolderDialog::~BrowseFolderDialog()
{
	if (pImpl_) {
		freeWString(pImpl_->wstrPath_);
		delete pImpl_;
	}
}

const WCHAR* qs::BrowseFolderDialog::getPath() const
{
	return pImpl_->wstrPath_;
}

LRESULT qs::BrowseFolderDialog::onDestroy()
{
	HWND hwnd = getDlgItem(IDC_FOLDER);
	HIMAGELIST hImageList = TreeView_GetImageList(hwnd, TVSIL_NORMAL);
	ImageList_Destroy(hImageList);
	
	removeNotifyHandler(pImpl_);
	
	return DefaultDialog::onDestroy();
}

LRESULT qs::BrowseFolderDialog::onInitDialog(HWND hwndFocus, LPARAM lParam)
{
	init(false);
	
	HWND hwnd = getDlgItem(IDC_FOLDER);
	
	HIMAGELIST hImageList = ImageList_LoadBitmap(getDllInstanceHandle(),
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
	
	if (pImpl_->wstrPath_)
		pImpl_->selectFolder(hwnd, pImpl_->wstrPath_);
	
	return TRUE;
}

LRESULT qs::BrowseFolderDialog::onOk()
{
	DECLARE_QSTATUS();
	
	HWND hwnd = getDlgItem(IDC_FOLDER);
	HTREEITEM hItem = TreeView_GetSelection(hwnd);
	if (hItem) {
		string_ptr<WSTRING> wstrPath;
		status = pImpl_->getPath(hwnd, hItem, &wstrPath);
		if (status == QSTATUS_SUCCESS) {
			freeWString(pImpl_->wstrPath_);
			pImpl_->wstrPath_ = wstrPath.release();
		}
	}
	
	return DefaultDialog::onOk();
}

#endif
