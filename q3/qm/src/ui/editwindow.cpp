/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */


#include <qmeditwindow.h>
#include <qmmessage.h>

#include <qsaccelerator.h>
#include <qsconv.h>
#include <qsdragdrop.h>
#include <qskeymap.h>
#include <qsnew.h>

#include <tchar.h>

#include "editwindow.h"
#include "headereditwindow.h"
#include "keymap.h"
#include "resourceinc.h"
#include "uiutil.h"
#include "../model/editmessage.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * EditWindowImpl
 *
 */

class qm::EditWindowImpl :
	public EditWindowItem,
	public EditWindowFocusController,
	public HeaderEditLineCallback,
	public EditTextWindowCallback,
	public EditMessageHolder,
	public DefaultEditMessageHandler,
	public DropTargetHandler
{
public:
	enum {
		ID_HEADEREDITWINDOW	= 1001,
		ID_TEXTWINDOW		= 1002
	};

public:
	QSTATUS layoutChildren();
	QSTATUS layoutChildren(int cx, int cy);
	QSTATUS updateEditMessage() const;

public:
	virtual QSTATUS copy();
	virtual QSTATUS canCopy(bool* pbCan);
	virtual QSTATUS cut();
	virtual QSTATUS canCut(bool* pbCan);
	virtual QSTATUS paste();
	virtual QSTATUS canPaste(bool* pbCan);
	virtual QSTATUS selectAll();
	virtual QSTATUS canSelectAll(bool* pbCan);
	virtual QSTATUS undo();
	virtual QSTATUS canUndo(bool* pbCan);
	virtual QSTATUS redo();
	virtual QSTATUS canRedo(bool* pbCan);
	virtual qs::QSTATUS setFocus();

public:
	virtual qs::QSTATUS setFocus(EditWindowItem* pItem, Focus focus);

public:
	virtual bool isHidden() const;

public:
	virtual QSTATUS layout();

public:
	virtual EditMessage* getEditMessage();
	virtual qs::QSTATUS setEditMessage(EditMessage* pEditMessage);
	virtual void releaseEditMessage();

public:
	virtual qs::QSTATUS messageSet(const EditMessageEvent& event);
	virtual qs::QSTATUS messageUpdate(const EditMessageEvent& event);

public:
	virtual QSTATUS dragEnter(const DropTargetDragEvent& event);
	virtual QSTATUS dragOver(const DropTargetDragEvent& event);
	virtual QSTATUS dragExit(const DropTargetEvent& event);
	virtual QSTATUS drop(const DropTargetDropEvent& event);

public:
	EditWindow* pThis_;
	
	bool bHideHeaderIfNoFocus_;
	
	Profile* pProfile_;
	Accelerator* pAccelerator_;
	EditMessage* pEditMessage_;
	HeaderEditWindow* pHeaderEditWindow_;
	EditTextWindow* pTextWindow_;
	EditWindowItemWindow* pItemWindow_;
	DropTarget* pDropTarget_;
	bool bCreated_;
	
	bool bHeaderEdit_;
	EditWindowItem* pLastFocusedItem_;
	bool bCanDrop_;
};

QSTATUS qm::EditWindowImpl::layoutChildren()
{
	RECT rect;
	pThis_->getClientRect(&rect);
	return layoutChildren(rect.right - rect.left, rect.bottom - rect.top);
}

QSTATUS qm::EditWindowImpl::layoutChildren(int cx, int cy)
{
	DECLARE_QSTATUS();
	
	status = pHeaderEditWindow_->layout();
	CHECK_QSTATUS();
	int nHeaderHeight = pHeaderEditWindow_->getHeight();
	
	pHeaderEditWindow_->setWindowPos(0, 0, 0, cx, nHeaderHeight, SWP_NOZORDER);
	pHeaderEditWindow_->showWindow(bHeaderEdit_ ? SW_HIDE : SW_SHOW);
	
	int nY = bHeaderEdit_ ? 0 : nHeaderHeight;
	int nHeight = cy > nY ? cy - nY : 0;
	pTextWindow_->setWindowPos(HWND_TOP, 0, nY, cx, nHeight, 0);
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::EditWindowImpl::updateEditMessage() const
{
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrText;
	status = pTextWindow_->getEditableTextModel()->getText(&wstrText);
	CHECK_QSTATUS();
	
	if (bHeaderEdit_) {
		const WCHAR* p = wcsstr(wstrText.get(), L"\n\n");
		size_t nHeaderLen = -1;
		const WCHAR* pBody = L"";
		if (p) {
			nHeaderLen = p - wstrText.get() + 1;
			pBody = p + 2;
		}
		status = pEditMessage_->setHeader(wstrText.get(), nHeaderLen);
		CHECK_QSTATUS();
		status = pEditMessage_->setBody(pBody);
		CHECK_QSTATUS();
	}
	else {
		status = pHeaderEditWindow_->updateEditMessage(pEditMessage_);
		CHECK_QSTATUS();
		
		status = pEditMessage_->setBody(wstrText.get());
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::EditWindowImpl::copy()
{
	return pTextWindow_->copy();
}

QSTATUS qm::EditWindowImpl::canCopy(bool* pbCan)
{
	return pTextWindow_->canCopy(pbCan);
}

QSTATUS qm::EditWindowImpl::cut()
{
	return pTextWindow_->cut();
}

QSTATUS qm::EditWindowImpl::canCut(bool* pbCan)
{
	return pTextWindow_->canCut(pbCan);
}

QSTATUS qm::EditWindowImpl::paste()
{
	return pTextWindow_->paste();
}

QSTATUS qm::EditWindowImpl::canPaste(bool* pbCan)
{
	return pTextWindow_->canPaste(pbCan);
}

QSTATUS qm::EditWindowImpl::selectAll()
{
	return pTextWindow_->selectAll();
}

QSTATUS qm::EditWindowImpl::canSelectAll(bool* pbCan)
{
	assert(pbCan);
	*pbCan = true;
	return QSTATUS_SUCCESS;
}

QSTATUS qm::EditWindowImpl::undo()
{
	return pTextWindow_->undo();
}

QSTATUS qm::EditWindowImpl::canUndo(bool* pbCan)
{
	return pTextWindow_->canUndo(pbCan);
}

QSTATUS qm::EditWindowImpl::redo()
{
	return pTextWindow_->redo();
}

QSTATUS qm::EditWindowImpl::canRedo(bool* pbCan)
{
	return pTextWindow_->canRedo(pbCan);
}

QSTATUS qm::EditWindowImpl::setFocus()
{
	pTextWindow_->setFocus();
	return QSTATUS_SUCCESS;
}

QSTATUS qm::EditWindowImpl::setFocus(EditWindowItem* pItem, Focus focus)
{
	EditWindowItem* pNewItem = 0;
	if (pItem == this) {
		pNewItem = focus == FOCUS_PREV ?
			pHeaderEditWindow_->getPrevFocusItem(0) :
			pHeaderEditWindow_->getNextFocusItem(0);
	}
	else {
		pNewItem = focus == FOCUS_PREV ?
			pHeaderEditWindow_->getPrevFocusItem(pItem) :
			pHeaderEditWindow_->getNextFocusItem(pItem);
		if (!pNewItem)
			pNewItem = this;
	}
	pNewItem->setFocus();
	
	return QSTATUS_SUCCESS;
}

bool qm::EditWindowImpl::isHidden() const
{
	return bHideHeaderIfNoFocus_ && pTextWindow_ && pTextWindow_->hasFocus();
}

QSTATUS qm::EditWindowImpl::layout()
{
	DECLARE_QSTATUS();
	
	if (bHideHeaderIfNoFocus_) {
		status = layoutChildren();
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

EditMessage* qm::EditWindowImpl::getEditMessage()
{
	return pEditMessage_;
}

QSTATUS qm::EditWindowImpl::setEditMessage(EditMessage* pEditMessage)
{
	assert(pEditMessage);
	assert(!pEditMessage_ || pEditMessage_ == pEditMessage);
	
	DECLARE_QSTATUS();
	
	EditableTextModel* pTextModel = pTextWindow_->getEditableTextModel();
	if (bHeaderEdit_) {
		string_ptr<WSTRING> wstrHeader;
		status = pEditMessage->getHeader(&wstrHeader);
		CHECK_QSTATUS();
		
		StringBuffer<WSTRING> buf(&status);
		CHECK_QSTATUS();
		status = buf.append(wstrHeader.get());
		CHECK_QSTATUS();
		status = buf.append(L"\n");
		CHECK_QSTATUS();
		status = buf.append(pEditMessage->getBody());
		CHECK_QSTATUS();
		
		status = pTextModel->setText(buf.getCharArray(), buf.getLength());
		CHECK_QSTATUS();
	}
	else {
		status = pHeaderEditWindow_->setEditMessage(
			pEditMessage, pEditMessage_ != 0);
		CHECK_QSTATUS();
		
		status = pTextModel->setText(pEditMessage->getBody(), -1);
		CHECK_QSTATUS();
		
		if (!pEditMessage_) {
			status = pEditMessage->addEditMessageHandler(this);
			CHECK_QSTATUS();
		}
	}
	
	pEditMessage_ = pEditMessage;
	
	return QSTATUS_SUCCESS;
}

void qm::EditWindowImpl::releaseEditMessage()
{
	pEditMessage_->removeEditMessageHandler(this);
	pHeaderEditWindow_->releaseEditMessage(pEditMessage_);
	delete pEditMessage_;
	pEditMessage_ = 0;
}

QSTATUS qm::EditWindowImpl::messageSet(const EditMessageEvent& event)
{
	return setEditMessage(event.getEditMessage());
}

QSTATUS qm::EditWindowImpl::messageUpdate(const EditMessageEvent& event)
{
	return updateEditMessage();
}

QSTATUS qm::EditWindowImpl::dragEnter(const DropTargetDragEvent& event)
{
	IDataObject* pDataObject = event.getDataObject();
	
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
			for (UINT n = 0; n < nCount && !bCanDrop_; ++n) {
				TCHAR tszPath[MAX_PATH];
				::DragQueryFile(hDrop, n, tszPath, countof(tszPath));
				DWORD dwAttributes = ::GetFileAttributes(tszPath);
				if (dwAttributes != 0xffffffff &&
					!(dwAttributes & FILE_ATTRIBUTE_DIRECTORY))
					bCanDrop_ = true;
			}
		}
	}
#endif
	
	if (bCanDrop_)
		event.setEffect(DROPEFFECT_COPY);
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::EditWindowImpl::dragOver(const DropTargetDragEvent& event)
{
	if (bCanDrop_) 
		event.setEffect(DROPEFFECT_COPY);
	return QSTATUS_SUCCESS;
}

QSTATUS qm::EditWindowImpl::dragExit(const DropTargetEvent& event)
{
	bCanDrop_ = false;
	return QSTATUS_SUCCESS;
}

QSTATUS qm::EditWindowImpl::drop(const DropTargetDropEvent& event)
{
	DECLARE_QSTATUS();
	
	IDataObject* pDataObject = event.getDataObject();
	
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
					!(dwAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
					T2W(tszPath, pwszPath);
					status = pEditMessage_->addAttachment(pwszPath);
					CHECK_QSTATUS();
				}
			}
		}
	}
#endif
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * EditWindow
 *
 */

qm::EditWindow::EditWindow(Profile* pProfile, QSTATUS* pstatus) :
	WindowBase(true, pstatus),
	DefaultWindowHandler(pstatus),
	pImpl_(0)
{
	if (*pstatus != QSTATUS_SUCCESS)
		return;
	
	DECLARE_QSTATUS();
	
	int nHideHeaderIfNoFocus = 0;
	status = pProfile->getInt(L"EditWindow",
		L"HideHeaderIfNoFocus", 0, &nHideHeaderIfNoFocus);
	CHECK_QSTATUS_SET(pstatus);
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	pImpl_->pThis_ = this;
	pImpl_->bHideHeaderIfNoFocus_ = nHideHeaderIfNoFocus != 0;
	pImpl_->pProfile_ = pProfile;
	pImpl_->pAccelerator_ = 0;
	pImpl_->pEditMessage_ = 0;
	pImpl_->pHeaderEditWindow_ = 0;
	pImpl_->pTextWindow_ = 0;
	pImpl_->pItemWindow_ = 0;
	pImpl_->pDropTarget_ = 0;
	pImpl_->bCreated_ = false;
	pImpl_->bHeaderEdit_ = false;
	pImpl_->pLastFocusedItem_ = 0;
	pImpl_->bCanDrop_ = false;
	
	setWindowHandler(this, false);
}

qm::EditWindow::~EditWindow()
{
	if (pImpl_) {
		delete pImpl_->pItemWindow_;
		delete pImpl_->pAccelerator_;
		delete pImpl_;
		pImpl_ = 0;
	}
}

EditMessageHolder* qm::EditWindow::getEditMessageHolder() const
{
	return pImpl_;
}

TextWindow* qm::EditWindow::getTextWindow() const
{
	return pImpl_->pTextWindow_;
}

EditWindowItem* qm::EditWindow::getItemByNumber(unsigned int nNumber) const
{
	return pImpl_->pHeaderEditWindow_->getItemByNumber(nNumber);
}

EditWindowItem* qm::EditWindow::getFocusedItem() const
{
	if (pImpl_->pTextWindow_->hasFocus())
		return pImpl_;
	else
		return pImpl_->pHeaderEditWindow_->getFocusedItem();
}

void qm::EditWindow::saveFocusedItem()
{
	pImpl_->pLastFocusedItem_ = pImpl_->pHeaderEditWindow_->getFocusedItem();
	if (!pImpl_->pLastFocusedItem_)
		pImpl_->pLastFocusedItem_ = pImpl_;
}

void qm::EditWindow::restoreFocusedItem()
{
	EditWindowItem* pItem = pImpl_->pLastFocusedItem_;
	if (!pItem) {
		pItem = pImpl_->pHeaderEditWindow_->getInitialFocusedItem();
		if (!pItem)
			pItem = pImpl_;
	}
	assert(pItem);
	
	pItem->setFocus();
}

bool qm::EditWindow::isHeaderEdit() const
{
	return pImpl_->bHeaderEdit_;
}

QSTATUS qm::EditWindow::setHeaderEdit(bool bHeaderEdit)
{
	DECLARE_QSTATUS();
	
	if (bHeaderEdit != pImpl_->bHeaderEdit_) {
		Message* pMessage = 0;
		status = pImpl_->pEditMessage_->getMessage(&pMessage);
		CHECK_QSTATUS();
		
		pImpl_->bHeaderEdit_ = bHeaderEdit;
		
		status = pImpl_->setEditMessage(pImpl_->pEditMessage_);
		CHECK_QSTATUS();
		
		pImpl_->layoutChildren();
		pImpl_->pTextWindow_->setFocus();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::EditWindow::getAccelerator(Accelerator** ppAccelerator)
{
	assert(ppAccelerator);
	*ppAccelerator = pImpl_->pAccelerator_;
	return QSTATUS_SUCCESS;
}

LRESULT qm::EditWindow::windowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	BEGIN_MESSAGE_HANDLER()
		HANDLE_CREATE()
		HANDLE_DESTROY()
		HANDLE_SIZE()
	END_MESSAGE_HANDLER()
	return DefaultWindowHandler::windowProc(uMsg, wParam, lParam);
}

LRESULT qm::EditWindow::onCreate(CREATESTRUCT* pCreateStruct)
{
	if (DefaultWindowHandler::onCreate(pCreateStruct) == -1)
		return -1;
	
	DECLARE_QSTATUS();
	
	EditWindowCreateContext* pContext =
		static_cast<EditWindowCreateContext*>(pCreateStruct->lpCreateParams);
	
	status = pContext->pKeyMap_->createAccelerator(
		CustomAcceleratorFactory(), L"EditWindow",
		mapKeyNameToId, countof(mapKeyNameToId), &pImpl_->pAccelerator_);
	CHECK_QSTATUS_VALUE(-1);
	
	std::auto_ptr<HeaderEditWindow> pHeaderEditWindow;
	status = newQsObject(pImpl_->pProfile_, &pHeaderEditWindow);
	CHECK_QSTATUS_VALUE(-1);
	HeaderEditWindowCreateContext headerEditContext = {
		pImpl_,
		pImpl_
	};
	status = pHeaderEditWindow->create(L"QmHeaderEditWindow", 0,
		WS_VISIBLE | WS_CHILD, CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT, getHandle(), 0, 0,
		EditWindowImpl::ID_HEADEREDITWINDOW, &headerEditContext);
	CHECK_QSTATUS_VALUE(-1);
	pImpl_->pHeaderEditWindow_ = pHeaderEditWindow.release();
	
	EditTextWindowCreateContext editTextContext = {
		pContext->pMenuManager_,
		pImpl_
	};
	std::auto_ptr<EditTextWindow> pTextWindow;
	status = newQsObject(pImpl_->pProfile_, L"EditWindow", &pTextWindow);
	CHECK_QSTATUS_VALUE(-1);
#if defined _WIN32_WCE && _WIN32_WCE >= 300 && defined _WIN32_WCE_PSPC
	DWORD dwExStyle = 0;
#else
	DWORD dwExStyle = WS_EX_CLIENTEDGE;
#endif
	status = pTextWindow->create(L"QmEditTextWindow", 0,
		WS_VISIBLE | WS_CHILD, CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT, getHandle(), dwExStyle, 0,
		EditWindowImpl::ID_TEXTWINDOW, &editTextContext);
	CHECK_QSTATUS_VALUE(-1);
	pImpl_->pTextWindow_ = pTextWindow.release();
	
	status = newQsObject(pImpl_, pImpl_,
		pImpl_->pTextWindow_->getHandle(), true, &pImpl_->pItemWindow_);
	CHECK_QSTATUS_VALUE(-1);
	
	status = newQsObject(getHandle(), &pImpl_->pDropTarget_);
	CHECK_QSTATUS_VALUE(-1);
	pImpl_->pDropTarget_->setDropTargetHandler(pImpl_);
	
	pImpl_->bCreated_ = true;
	
	return 0;
}

LRESULT qm::EditWindow::onDestroy()
{
	pImpl_->pItemWindow_->unsubclassWindow();
	
	delete pImpl_->pDropTarget_;
	
	return DefaultWindowHandler::onDestroy();
}

LRESULT qm::EditWindow::onSize(UINT nFlags, int cx, int cy)
{
	if (pImpl_->bCreated_)
		pImpl_->layoutChildren(cx, cy);
	return DefaultWindowHandler::onSize(nFlags, cx, cy);
}


/****************************************************************************
 *
 * EditWindowItem
 *
 */

qm::EditWindowItem::~EditWindowItem()
{
}


/****************************************************************************
 *
 * EditWindowItemWindow
 *
 */

qm::EditWindowItemWindow::EditWindowItemWindow(
	EditWindowFocusController* pController, EditWindowItem* pItem,
	HWND hwnd, QSTATUS* pstatus) :
	WindowBase(false, pstatus),
	DefaultWindowHandler(pstatus),
	pController_(pController),
	pItem_(pItem),
	bPrevOnly_(false)
{
	setWindowHandler(this, false);
	
	subclassWindow(hwnd);
}

qm::EditWindowItemWindow::EditWindowItemWindow(
	EditWindowFocusController* pController, EditWindowItem* pItem,
	HWND hwnd, bool bPrevOnly, QSTATUS* pstatus) :
	WindowBase(false, pstatus),
	DefaultWindowHandler(pstatus),
	pController_(pController),
	pItem_(pItem),
	bPrevOnly_(bPrevOnly)
{
	setWindowHandler(this, false);
	
	subclassWindow(hwnd);
}

qm::EditWindowItemWindow::~EditWindowItemWindow()
{
}

LRESULT qm::EditWindowItemWindow::windowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	BEGIN_MESSAGE_HANDLER()
		HANDLE_CHAR()
	END_MESSAGE_HANDLER()
	return DefaultWindowHandler::windowProc(uMsg, wParam, lParam);
}

LRESULT qm::EditWindowItemWindow::onChar(UINT nChar, UINT nRepeat, UINT nFlags)
{
	if (nChar == _T('\t')) {
		bool bShift = ::GetKeyState(VK_SHIFT) < 0;
		if (bShift || !bPrevOnly_) {
			pController_->setFocus(pItem_,
				bShift ? EditWindowFocusController::FOCUS_PREV :
					EditWindowFocusController::FOCUS_NEXT);
			return 0;
		}
	}
	return DefaultWindowHandler::onChar(nChar, nRepeat, nFlags);
}


/****************************************************************************
 *
 * EditWindowFocusController
 *
 */

qm::EditWindowFocusController::~EditWindowFocusController()
{
}


/****************************************************************************
 *
 * EditTextWindow
 *
 */

qm::EditTextWindow::EditTextWindow(Profile* pProfile,
	const WCHAR* pwszSection, QSTATUS* pstatus) :
	TextWindow(0, pProfile, pwszSection, true, pstatus),
	pMenuManager_(0),
	pCallback_(0)
{
	DECLARE_QSTATUS();
	
	std::auto_ptr<EditableTextModel> pTextModel;
	status = newQsObject(&pTextModel);
	CHECK_QSTATUS_SET(pstatus);
	setTextModel(pTextModel.release());
	setLinkHandler(this);
}

qm::EditTextWindow::~EditTextWindow()
{
	delete getTextModel();
}

EditableTextModel* qm::EditTextWindow::getEditableTextModel() const
{
	return static_cast<EditableTextModel*>(getTextModel());
}

LRESULT qm::EditTextWindow::windowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	BEGIN_MESSAGE_HANDLER()
		HANDLE_CONTEXTMENU()
		HANDLE_CREATE()
		HANDLE_KILLFOCUS()
		HANDLE_LBUTTONDOWN()
		HANDLE_SETFOCUS()
	END_MESSAGE_HANDLER()
	return TextWindow::windowProc(uMsg, wParam, lParam);
}

LRESULT qm::EditTextWindow::onContextMenu(HWND hwnd, const POINT& pt)
{
	DECLARE_QSTATUS();
	
	HMENU hmenu = 0;
	status = pMenuManager_->getMenu(L"edit", false, false, &hmenu);
	if (status == QSTATUS_SUCCESS) {
		UINT nFlags = TPM_LEFTALIGN | TPM_TOPALIGN;
#ifndef _WIN32_WCE
		nFlags |= TPM_LEFTBUTTON | TPM_RIGHTBUTTON;
#endif
		::TrackPopupMenu(hmenu, nFlags, pt.x, pt.y, 0, getParentFrame(), 0);
	}
	
	return TextWindow::onContextMenu(hwnd, pt);
}

LRESULT qm::EditTextWindow::onCreate(CREATESTRUCT* pCreateStruct)
{
	if (TextWindow::onCreate(pCreateStruct) == -1)
		return -1;
	
	EditTextWindowCreateContext* pContext =
		static_cast<EditTextWindowCreateContext*>(pCreateStruct->lpCreateParams);
	pMenuManager_ = pContext->pMenuManager_;
	pCallback_ = pContext->pCallback_;
	
	return 0;
}

LRESULT qm::EditTextWindow::onKillFocus(HWND hwnd)
{
	if (hwnd && Window(hwnd).getParentPopup() == getParentFrame())
		pCallback_->layout();
	return TextWindow::onKillFocus(hwnd);
}

LRESULT qm::EditTextWindow::onLButtonDown(UINT nFlags, const POINT& pt)
{
#if defined _WIN32_WCE && _WIN32_WCE >= 300 && _WIN32_WCE_PSPC
	if (tapAndHold(pt))
		return 0;
#endif
	return TextWindow::onLButtonDown(nFlags, pt);
}

LRESULT qm::EditTextWindow::onSetFocus(HWND hwnd)
{
	if (hwnd && Window(hwnd).getParentPopup() == getParentFrame())
		pCallback_->layout();
	return TextWindow::onSetFocus(hwnd);
}

QSTATUS qm::EditTextWindow::openLink(const WCHAR* pwszURL)
{
	return UIUtil::openURL(getParentFrame(), pwszURL);
}


/****************************************************************************
 *
 * EditTextWindowCallback
 *
 */

qm::EditTextWindowCallback::~EditTextWindowCallback()
{
}
