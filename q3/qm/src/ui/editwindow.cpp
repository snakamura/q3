/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmeditwindow.h>
#include <qmmessage.h>

#include <qsaccelerator.h>
#include <qsconv.h>
#include <qsdragdrop.h>
#include <qskeymap.h>

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
	public HeaderEditItemCallback,
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
	void layoutChildren();
	void layoutChildren(int cx,
						int cy);
	bool updateEditMessage() const;

public:
	virtual void copy();
	virtual bool canCopy();
	virtual void cut();
	virtual bool canCut();
	virtual void paste();
	virtual bool canPaste();
	virtual void selectAll();
	virtual bool canSelectAll();
	virtual void undo();
	virtual bool canUndo();
	virtual void redo();
	virtual bool canRedo();
	virtual void setFocus();

public:
	virtual void setFocus(EditWindowItem* pItem,
						  Focus focus);

public:
	virtual bool isHidden() const;

public:
	virtual void focusChanged();

public:
	virtual void itemSizeChanged();

public:
	virtual EditMessage* getEditMessage();
	virtual bool setEditMessage(EditMessage* pEditMessage);
	virtual void releaseEditMessage();

public:
	virtual void messageSet(const EditMessageEvent& event);
	virtual void messageUpdate(const EditMessageEvent& event);

public:
	virtual void dragEnter(const DropTargetDragEvent& event);
	virtual void dragOver(const DropTargetDragEvent& event);
	virtual void dragExit(const DropTargetEvent& event);
	virtual void drop(const DropTargetDropEvent& event);

public:
	EditWindow* pThis_;
	
	bool bHideHeaderIfNoFocus_;
	
	Profile* pProfile_;
	std::auto_ptr<Accelerator> pAccelerator_;
	EditMessage* pEditMessage_;
	HeaderEditWindow* pHeaderEditWindow_;
	EditTextWindow* pTextWindow_;
	std::auto_ptr<EditWindowItemWindow> pItemWindow_;
	std::auto_ptr<DropTarget> pDropTarget_;
	bool bCreated_;
	bool bLayouting_;
	
	bool bHeaderEdit_;
	EditWindowItem* pLastFocusedItem_;
	bool bCanDrop_;
};

void qm::EditWindowImpl::layoutChildren()
{
	RECT rect;
	pThis_->getClientRect(&rect);
	layoutChildren(rect.right - rect.left, rect.bottom - rect.top);
}

void qm::EditWindowImpl::layoutChildren(int cx,
										int cy)
{
	bLayouting_ = true;
	
	pHeaderEditWindow_->layout(Rect(0, 0, cx, cy));
	
	int nHeaderHeight = pHeaderEditWindow_->getHeight();
	pHeaderEditWindow_->setWindowPos(0, 0, 0, cx, nHeaderHeight, SWP_NOZORDER);
	pHeaderEditWindow_->showWindow(bHeaderEdit_ ? SW_HIDE : SW_SHOW);
	
	int nY = bHeaderEdit_ ? 0 : nHeaderHeight;
	int nHeight = cy > nY ? cy - nY : 0;
	pTextWindow_->setWindowPos(HWND_TOP, 0, nY, cx, nHeight, 0);
	
	bLayouting_ = false;
}

bool qm::EditWindowImpl::updateEditMessage() const
{
	wxstring_ptr wstrText(pTextWindow_->getEditableTextModel()->getText());
	if (!wstrText.get())
		return false;
	
	if (bHeaderEdit_) {
		const WCHAR* p = wcsstr(wstrText.get(), L"\n\n");
		size_t nHeaderLen = -1;
		const WCHAR* pBody = L"";
		if (p) {
			nHeaderLen = p - wstrText.get() + 1;
			pBody = p + 2;
		}
		if (!pEditMessage_->setHeader(wstrText.get(), nHeaderLen))
			return false;
		if (!pEditMessage_->setBody(pBody))
			return false;
	}
	else {
		pHeaderEditWindow_->updateEditMessage(pEditMessage_);
		if (!pEditMessage_->setBody(wstrText.get()))
			return false;
	}
	
	return true;
}

void qm::EditWindowImpl::copy()
{
	pTextWindow_->copy();
}

bool qm::EditWindowImpl::canCopy()
{
	return pTextWindow_->canCopy();
}

void qm::EditWindowImpl::cut()
{
	pTextWindow_->cut();
}

bool qm::EditWindowImpl::canCut()
{
	return pTextWindow_->canCut();
}

void qm::EditWindowImpl::paste()
{
	pTextWindow_->paste();
}

bool qm::EditWindowImpl::canPaste()
{
	return pTextWindow_->canPaste();
}

void qm::EditWindowImpl::selectAll()
{
	pTextWindow_->selectAll();
}

bool qm::EditWindowImpl::canSelectAll()
{
	return true;
}

void qm::EditWindowImpl::undo()
{
	pTextWindow_->undo();
}

bool qm::EditWindowImpl::canUndo()
{
	return pTextWindow_->canUndo();
}

void qm::EditWindowImpl::redo()
{
	pTextWindow_->redo();
}

bool qm::EditWindowImpl::canRedo()
{
	return pTextWindow_->canRedo();
}

void qm::EditWindowImpl::setFocus()
{
	pTextWindow_->setFocus();
}

void qm::EditWindowImpl::setFocus(EditWindowItem* pItem,
								  Focus focus)
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
}

bool qm::EditWindowImpl::isHidden() const
{
	return bHideHeaderIfNoFocus_ && pTextWindow_ && pTextWindow_->hasFocus();
}

void qm::EditWindowImpl::focusChanged()
{
	if (bHideHeaderIfNoFocus_)
		layoutChildren();
}

void qm::EditWindowImpl::itemSizeChanged()
{
	layoutChildren();
}

EditMessage* qm::EditWindowImpl::getEditMessage()
{
	return pEditMessage_;
}

bool qm::EditWindowImpl::setEditMessage(EditMessage* pEditMessage)
{
	assert(pEditMessage);
	assert(!pEditMessage_ || pEditMessage_ == pEditMessage);
	
	EditableTextModel* pTextModel = pTextWindow_->getEditableTextModel();
	if (bHeaderEdit_) {
		wxstring_ptr wstrHeader(pEditMessage->getHeader());
		if (!wstrHeader.get())
			return false;
		
		XStringBuffer<WXSTRING> buf;
		if (!buf.append(wstrHeader.get()) ||
			!buf.append(L"\n") ||
			!buf.append(pEditMessage->getBody()))
			return false;
		
		if (!pTextModel->setText(buf.getCharArray(), buf.getLength()))
			return false;
	}
	else {
		pHeaderEditWindow_->setEditMessage(pEditMessage, pEditMessage_ != 0);
		if (!pTextModel->setText(pEditMessage->getBody(), -1))
			return false;
		
		if (!pEditMessage_)
			pEditMessage->addEditMessageHandler(this);
	}
	
	pEditMessage_ = pEditMessage;
	
	layoutChildren();
	
	return true;
}

void qm::EditWindowImpl::releaseEditMessage()
{
	pEditMessage_->removeEditMessageHandler(this);
	pHeaderEditWindow_->releaseEditMessage(pEditMessage_);
	delete pEditMessage_;
	pEditMessage_ = 0;
}

void qm::EditWindowImpl::messageSet(const EditMessageEvent& event)
{
	// TODO
	// Error handling
	setEditMessage(event.getEditMessage());
}

void qm::EditWindowImpl::messageUpdate(const EditMessageEvent& event)
{
	// TODO
	// Error handling
	updateEditMessage();
}

void qm::EditWindowImpl::dragEnter(const DropTargetDragEvent& event)
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
}

void qm::EditWindowImpl::dragOver(const DropTargetDragEvent& event)
{
	if (bCanDrop_) 
		event.setEffect(DROPEFFECT_COPY);
}

void qm::EditWindowImpl::dragExit(const DropTargetEvent& event)
{
	bCanDrop_ = false;
}

void qm::EditWindowImpl::drop(const DropTargetDropEvent& event)
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
			for (UINT n = 0; n < nCount; ++n) {
				TCHAR tszPath[MAX_PATH];
				::DragQueryFile(hDrop, n, tszPath, countof(tszPath));
				DWORD dwAttributes = ::GetFileAttributes(tszPath);
				if (dwAttributes != 0xffffffff &&
					!(dwAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
					T2W(tszPath, pwszPath);
					pEditMessage_->addAttachment(pwszPath);
				}
			}
		}
	}
#endif
}


/****************************************************************************
 *
 * EditWindow
 *
 */

qm::EditWindow::EditWindow(Profile* pProfile) :
	WindowBase(true),
	pImpl_(0)
{
	pImpl_ = new EditWindowImpl();
	pImpl_->pThis_ = this;
	pImpl_->bHideHeaderIfNoFocus_ = pProfile->getInt(L"EditWindow", L"HideHeaderIfNoFocus", 0) != 0;
	pImpl_->pProfile_ = pProfile;
	pImpl_->pEditMessage_ = 0;
	pImpl_->pHeaderEditWindow_ = 0;
	pImpl_->pTextWindow_ = 0;
	pImpl_->bCreated_ = false;
	pImpl_->bLayouting_ = false;
	pImpl_->bHeaderEdit_ = false;
	pImpl_->pLastFocusedItem_ = 0;
	pImpl_->bCanDrop_ = false;
	
	setWindowHandler(this, false);
}

qm::EditWindow::~EditWindow()
{
	delete pImpl_;
}

EditMessageHolder* qm::EditWindow::getEditMessageHolder() const
{
	return pImpl_;
}

TextWindow* qm::EditWindow::getTextWindow() const
{
	return pImpl_->pTextWindow_;
}

AttachmentSelectionModel* qm::EditWindow::getAttachmentSelectionModel() const
{
	return pImpl_->pHeaderEditWindow_->getAttachmentSelectionModel();
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

void qm::EditWindow::setHeaderEdit(bool bHeaderEdit)
{
	if (bHeaderEdit != pImpl_->bHeaderEdit_) {
		Message* pMessage = pImpl_->pEditMessage_->getMessage();
		pImpl_->bHeaderEdit_ = bHeaderEdit;
		pImpl_->setEditMessage(pImpl_->pEditMessage_);
		pImpl_->layoutChildren();
		pImpl_->pTextWindow_->setFocus();
	}
}

Accelerator* qm::EditWindow::getAccelerator()
{
	return pImpl_->pAccelerator_.get();
}

LRESULT qm::EditWindow::windowProc(UINT uMsg,
								   WPARAM wParam,
								   LPARAM lParam)
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
	
	EditWindowCreateContext* pContext =
		static_cast<EditWindowCreateContext*>(pCreateStruct->lpCreateParams);
	
	CustomAcceleratorFactory acceleratorFactory;
	pImpl_->pAccelerator_ = pContext->pKeyMap_->createAccelerator(
		&acceleratorFactory, L"EditWindow", mapKeyNameToId, countof(mapKeyNameToId));
	if (!pImpl_->pAccelerator_.get())
		return -1;
	
	std::auto_ptr<HeaderEditWindow> pHeaderEditWindow(
		new HeaderEditWindow(pImpl_->pProfile_));
	HeaderEditWindowCreateContext headerEditContext = {
		pImpl_,
		pContext->pMenuManager_,
		pImpl_,
		pImpl_
	};
	if (!pHeaderEditWindow->create(L"QmHeaderEditWindow", 0,
		WS_VISIBLE | WS_CHILD, CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT, getHandle(), 0, 0,
		EditWindowImpl::ID_HEADEREDITWINDOW, &headerEditContext))
		return -1;
	pImpl_->pHeaderEditWindow_ = pHeaderEditWindow.release();
	
	EditTextWindowCreateContext editTextContext = {
		pContext->pMenuManager_,
		pImpl_
	};
	std::auto_ptr<EditTextWindow> pTextWindow(
		new EditTextWindow(pImpl_->pProfile_, L"EditWindow"));
#if defined _WIN32_WCE && _WIN32_WCE >= 300 && defined _WIN32_WCE_PSPC
	DWORD dwExStyle = 0;
#else
	DWORD dwExStyle = WS_EX_CLIENTEDGE;
#endif
	if (!pTextWindow->create(L"QmEditTextWindow", 0,
		WS_VISIBLE | WS_CHILD, CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT, getHandle(), dwExStyle, 0,
		EditWindowImpl::ID_TEXTWINDOW, &editTextContext))
		return -1;
	pImpl_->pTextWindow_ = pTextWindow.release();
	
	pImpl_->pItemWindow_.reset(new EditWindowItemWindow(pImpl_,
		pImpl_, pImpl_->pTextWindow_->getHandle(), true));
	
	pImpl_->pDropTarget_.reset(new DropTarget(getHandle()));
	pImpl_->pDropTarget_->setDropTargetHandler(pImpl_);
	
	pImpl_->bCreated_ = true;
	
	return 0;
}

LRESULT qm::EditWindow::onDestroy()
{
	pImpl_->pItemWindow_->unsubclassWindow();
	pImpl_->pDropTarget_.reset(0);
	return DefaultWindowHandler::onDestroy();
}

LRESULT qm::EditWindow::onSize(UINT nFlags,
							   int cx,
							   int cy)
{
	if (pImpl_->bCreated_ && !pImpl_->bLayouting_)
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

qm::EditWindowItemWindow::EditWindowItemWindow(EditWindowFocusController* pController,
											   EditWindowItem* pItem,
											   HWND hwnd) :
	WindowBase(false),
	pController_(pController),
	pItem_(pItem),
	bPrevOnly_(false)
{
	setWindowHandler(this, false);
	subclassWindow(hwnd);
}

qm::EditWindowItemWindow::EditWindowItemWindow(EditWindowFocusController* pController,
											   EditWindowItem* pItem,
											   HWND hwnd,
											   bool bPrevOnly) :
	WindowBase(false),
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

LRESULT qm::EditWindowItemWindow::windowProc(UINT uMsg,
											 WPARAM wParam,
											 LPARAM lParam)
{
	BEGIN_MESSAGE_HANDLER()
		HANDLE_CHAR()
	END_MESSAGE_HANDLER()
	return DefaultWindowHandler::windowProc(uMsg, wParam, lParam);
}

LRESULT qm::EditWindowItemWindow::onChar(UINT nChar,
										 UINT nRepeat,
										 UINT nFlags)
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
								   const WCHAR* pwszSection) :
	TextWindow(0, pProfile, pwszSection, true),
	pMenuManager_(0),
	pCallback_(0)
{
	std::auto_ptr<EditableTextModel> pTextModel(new EditableTextModel());
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

LRESULT qm::EditTextWindow::windowProc(UINT uMsg,
									   WPARAM wParam,
									   LPARAM lParam)
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

LRESULT qm::EditTextWindow::onContextMenu(HWND hwnd,
										  const POINT& pt)
{
	HMENU hmenu = pMenuManager_->getMenu(L"edit", false, false);
	if (hmenu) {
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
		pCallback_->focusChanged();
	return TextWindow::onKillFocus(hwnd);
}

LRESULT qm::EditTextWindow::onLButtonDown(UINT nFlags,
										  const POINT& pt)
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
		pCallback_->focusChanged();
	return TextWindow::onSetFocus(hwnd);
}

bool qm::EditTextWindow::openLink(const WCHAR* pwszURL)
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
