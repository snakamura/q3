/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmaccount.h>
#include <qmapplication.h>
#include <qmdocument.h>
#include <qmeditwindow.h>
#include <qmextensions.h>
#include <qmmessage.h>

#include <qsconv.h>
#include <qsdevicecontext.h>
#include <qsinit.h>
#include <qsnew.h>
#include <qsuiutil.h>

#include <algorithm>

#include <tchar.h>

#include "headereditwindow.h"
#include "../model/addressbook.h"
#include "../model/editmessage.h"
#include "../model/signature.h"

#pragma warning(disable:4355)

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * HeaderEditWindowImpl
 *
 */

class qm::HeaderEditWindowImpl
{
public:
	enum {
		ID_HEADEREDIT_ITEM	= 1000
	};

public:
	QSTATUS load(MenuManager* pMenuManager, HeaderEditLineCallback* pCallback);
	QSTATUS create(MenuManager* pMenuManager, HeaderEditLineCallback* pCallback);

public:
	HeaderEditWindow* pThis_;
	Profile* pProfile_;
	
	HFONT hfont_;
	HFONT hfontBold_;
	HBRUSH hbrBackground_;
	LineLayout* pLayout_;
	EditWindowFocusController* pController_;
	AttachmentSelectionModel* pAttachmentSelectionModel_;
};

QSTATUS qm::HeaderEditWindowImpl::load(MenuManager* pMenuManager,
	HeaderEditLineCallback* pCallback)
{
	DECLARE_QSTATUS();
	
	status = newQsObject(&pLayout_);
	CHECK_QSTATUS();
	pLayout_->setLineSpacing(1);
	
	string_ptr<WSTRING> wstrPath;
	status = Application::getApplication().getProfilePath(
		Extensions::HEADEREDIT, &wstrPath);
	CHECK_QSTATUS();
	
	XMLReader reader(&status);
	CHECK_QSTATUS();
	HeaderEditWindowContentHandler contentHandler(
		pLayout_, pController_, pMenuManager, pCallback, &status);
	CHECK_QSTATUS();
	reader.setContentHandler(&contentHandler);
	W2T(wstrPath.get(), ptszPath);
	if (::GetFileAttributes(ptszPath) != 0xffffffff) {
		status = reader.parse(wstrPath.get());
		CHECK_QSTATUS();
		pAttachmentSelectionModel_ = contentHandler.getAttachmentSelectionModel();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::HeaderEditWindowImpl::create(MenuManager* pMenuManager,
	HeaderEditLineCallback* pCallback)
{
	DECLARE_QSTATUS();
	
	status = load(pMenuManager, pCallback);
	CHECK_QSTATUS();
	
	std::pair<HFONT, HFONT> fonts(hfont_, hfontBold_);
	UINT nId = ID_HEADEREDIT_ITEM;
	status = pLayout_->create(pThis_, fonts, &nId);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * HeaderEditWindow
 *
 */

qm::HeaderEditWindow::HeaderEditWindow(Profile* pProfile, QSTATUS* pstatus) :
	WindowBase(true, pstatus),
	DefaultWindowHandler(pstatus),
	pImpl_(0)
{
	if (*pstatus != QSTATUS_SUCCESS)
		return;
	
	DECLARE_QSTATUS();
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus)
	pImpl_->pThis_ = this;
	pImpl_->pProfile_ = pProfile;
	pImpl_->hfont_ = 0;
	pImpl_->hfontBold_ = 0;
	pImpl_->hbrBackground_ = 0;
	pImpl_->pLayout_ = 0;
	pImpl_->pController_ = 0;
	
	setWindowHandler(this, false);
}

qm::HeaderEditWindow::~HeaderEditWindow()
{
	if (pImpl_) {
		delete pImpl_->pLayout_;
		delete pImpl_;
	}
}

QSTATUS qm::HeaderEditWindow::setEditMessage(
	EditMessage* pEditMessage, bool bReset)
{
	DECLARE_QSTATUS();
	
	for (unsigned int n = 0; n < pImpl_->pLayout_->getLineCount(); ++n) {
		HeaderEditLine* pLine = static_cast<HeaderEditLine*>(
			pImpl_->pLayout_->getLine(n));
		status = pLine->setEditMessage(pEditMessage, bReset);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

void qm::HeaderEditWindow::releaseEditMessage(EditMessage* pEditMessage)
{
	for (unsigned int n = 0; n < pImpl_->pLayout_->getLineCount(); ++n) {
		HeaderEditLine* pLine = static_cast<HeaderEditLine*>(
			pImpl_->pLayout_->getLine(n));
		for (unsigned int m = 0; m < pLine->getItemCount(); ++m) {
			HeaderEditItem* pItem = static_cast<HeaderEditItem*>(pLine->getItem(m));
			pItem->releaseEditMessage(pEditMessage);
		}
	}
}

QSTATUS qm::HeaderEditWindow::updateEditMessage(EditMessage* pEditMessage)
{
	DECLARE_QSTATUS();
	
	for (unsigned int n = 0; n < pImpl_->pLayout_->getLineCount(); ++n) {
		HeaderEditLine* pLine = static_cast<HeaderEditLine*>(
			pImpl_->pLayout_->getLine(n));
		for (unsigned int m = 0; m < pLine->getItemCount(); ++m) {
			HeaderEditItem* pItem = static_cast<HeaderEditItem*>(pLine->getItem(m));
			status = pItem->updateEditMessage(pEditMessage);
			CHECK_QSTATUS();
		}
	}
	
	return QSTATUS_SUCCESS;
}

int qm::HeaderEditWindow::getHeight() const
{
	return pImpl_->pLayout_->getHeight();
}

QSTATUS qm::HeaderEditWindow::layout()
{
	DECLARE_QSTATUS();
	
	ClientDeviceContext dc(getHandle(), &status);
	CHECK_QSTATUS();
	ObjectSelector<HFONT> fontSelecter(dc, pImpl_->hfont_);
	TEXTMETRIC tm;
	dc.getTextMetrics(&tm);
	unsigned int nFontHeight = tm.tmHeight + tm.tmExternalLeading;
	
	RECT rect;
	getClientRect(&rect);
	
	pImpl_->pLayout_->layout(rect, nFontHeight);
	
	return QSTATUS_SUCCESS;
}

EditWindowItem* qm::HeaderEditWindow::getFocusedItem() const
{
	for (unsigned int n = 0; n < pImpl_->pLayout_->getLineCount(); ++n) {
		HeaderEditLine* pLine = static_cast<HeaderEditLine*>(
			pImpl_->pLayout_->getLine(n));
		HeaderEditItem* pItem = pLine->getFocusedItem();
		if (pItem)
			return pItem;
	}
	return 0;
}

EditWindowItem* qm::HeaderEditWindow::getInitialFocusedItem() const
{
	for (unsigned int n = 0; n < pImpl_->pLayout_->getLineCount(); ++n) {
		HeaderEditLine* pLine = static_cast<HeaderEditLine*>(
			pImpl_->pLayout_->getLine(n));
		HeaderEditItem* pItem = pLine->getInitialFocusItem();
		if (pItem)
			return pItem;
	}
	return 0;
}

EditWindowItem* qm::HeaderEditWindow::getNextFocusItem(
	EditWindowItem* pItem) const
{
	for (unsigned int n = 0; n < pImpl_->pLayout_->getLineCount(); ++n) {
		HeaderEditLine* pLine = static_cast<HeaderEditLine*>(
			pImpl_->pLayout_->getLine(n));
		EditWindowItem* pNewItem = pLine->getNextFocusItem(&pItem);
		if (pNewItem)
			return pNewItem;
	}
	return 0;
}

EditWindowItem* qm::HeaderEditWindow::getPrevFocusItem(
	EditWindowItem* pItem) const
{
	for (unsigned int n = pImpl_->pLayout_->getLineCount(); n > 0; --n) {
		HeaderEditLine* pLine = static_cast<HeaderEditLine*>(
			pImpl_->pLayout_->getLine(n - 1));
		EditWindowItem* pNewItem = pLine->getPrevFocusItem(&pItem);
		if (pNewItem)
			return pNewItem;
	}
	return 0;
}

EditWindowItem* qm::HeaderEditWindow::getItemByNumber(unsigned int nNumber) const
{
	for (unsigned int n = 0; n < pImpl_->pLayout_->getLineCount(); ++n) {
		HeaderEditLine* pLine = static_cast<HeaderEditLine*>(
			pImpl_->pLayout_->getLine(n));
		for (unsigned int m = 0; m < pLine->getItemCount(); ++m) {
			HeaderEditItem* pItem = static_cast<HeaderEditItem*>(pLine->getItem(m));
			if (pItem->getNumber() == nNumber)
				return pItem;
		}
	}
	return 0;
}

AttachmentSelectionModel* qm::HeaderEditWindow::getAttachmentSelectionModel() const
{
	return pImpl_->pAttachmentSelectionModel_;
}

QSTATUS qm::HeaderEditWindow::getWindowClass(WNDCLASS* pwc)
{
	DECLARE_QSTATUS();
	
	status = DefaultWindowHandler::getWindowClass(pwc);
	CHECK_QSTATUS();
	
	pwc->hbrBackground = reinterpret_cast<HBRUSH>(COLOR_3DFACE + 1);
	
	return QSTATUS_SUCCESS;
}

LRESULT qm::HeaderEditWindow::windowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	BEGIN_MESSAGE_HANDLER()
		HANDLE_CREATE()
		HANDLE_CTLCOLORSTATIC()
		HANDLE_DESTROY()
		HANDLE_SIZE()
	END_MESSAGE_HANDLER()
	return DefaultWindowHandler::windowProc(uMsg, wParam, lParam);
}

LRESULT qm::HeaderEditWindow::onCreate(CREATESTRUCT* pCreateStruct)
{
	if (DefaultWindowHandler::onCreate(pCreateStruct) == -1)
		return -1;
	
	DECLARE_QSTATUS();
	
	HeaderEditWindowCreateContext* pContext =
		static_cast<HeaderEditWindowCreateContext*>(pCreateStruct->lpCreateParams);
	pImpl_->pController_ = pContext->pController_;
	
	status = UIUtil::createFontFromProfile(pImpl_->pProfile_,
		L"HeaderEditWindow", false, &pImpl_->hfont_);
	CHECK_QSTATUS_VALUE(-1);
	LOGFONT lf;
	::GetObject(pImpl_->hfont_, sizeof(lf), &lf);
	lf.lfWeight = FW_BOLD;
	pImpl_->hfontBold_ = ::CreateFontIndirect(&lf);
	
	string_ptr<WSTRING> wstrClassName(getClassName());
	W2T_STATUS(wstrClassName.get(), ptszClassName);
	CHECK_QSTATUS_VALUE(-1);
	WNDCLASS wc;
	::GetClassInfo(getInstanceHandle(), ptszClassName, &wc);
	pImpl_->hbrBackground_ = wc.hbrBackground;
	
	status = pImpl_->create(pContext->pMenuManager_,
		pContext->pHeaderEditLineCallback_);
	CHECK_QSTATUS_VALUE(-1);
	
	return 0;
}

LRESULT qm::HeaderEditWindow::onCtlColorStatic(HDC hdc, HWND hwnd)
{
	DefaultWindowHandler::onCtlColorStatic(hdc, hwnd);
	DeviceContext dc(hdc);
	dc.setTextColor(::GetSysColor(COLOR_WINDOWTEXT));
	dc.setBkColor(::GetSysColor(COLOR_3DFACE));
	return reinterpret_cast<LRESULT>(pImpl_->hbrBackground_);
}

LRESULT qm::HeaderEditWindow::onDestroy()
{
	pImpl_->pLayout_->destroy();
	
	if (pImpl_->hfont_) {
		::DeleteObject(pImpl_->hfont_);
		pImpl_->hfont_ = 0;
		::DeleteObject(pImpl_->hfontBold_);
		pImpl_->hfontBold_ = 0;
	}
	
	return DefaultWindowHandler::onDestroy();
}

LRESULT qm::HeaderEditWindow::onSize(UINT nFlags, int cx, int cy)
{
	layout();
	
	return DefaultWindowHandler::onSize(nFlags, cx, cy);
}


/****************************************************************************
 *
 * HeaderEditLine
 *
 */

qm::HeaderEditLine::HeaderEditLine(HeaderEditLineCallback* pCallback,
	unsigned int nFlags, RegexPattern* pClass, QSTATUS* pstatus) :
	LineLayoutLine(pstatus),
	pCallback_(pCallback),
	nFlags_(nFlags),
	pClass_(pClass),
	bHide_(false)
{
}

qm::HeaderEditLine::~HeaderEditLine()
{
	delete pClass_;
}

QSTATUS qm::HeaderEditLine::setEditMessage(EditMessage* pEditMessage, bool bReset)
{
	DECLARE_QSTATUS();
	
	if (pClass_) {
		const WCHAR* pwszClass = pEditMessage->getAccount()->getClass();
		bool bMatch = false;
		status = pClass_->match(pwszClass, &bMatch);
		CHECK_QSTATUS();
		bHide_ = !bMatch;
	}
	
	if (!bHide_) {
		for (unsigned int n = 0; n < getItemCount(); ++n) {
			HeaderEditItem* pItem = static_cast<HeaderEditItem*>(getItem(n));
			status = pItem->setEditMessage(pEditMessage, bReset);
			CHECK_QSTATUS();
		}
	}
	
	return QSTATUS_SUCCESS;
}

EditWindowItem* qm::HeaderEditLine::getNextFocusItem(
	EditWindowItem** ppItem) const
{
	assert(ppItem);
	
	if (!bHide_) {
		for (unsigned int n = 0; n < getItemCount(); ++n) {
			HeaderEditItem* pItem = static_cast<HeaderEditItem*>(getItem(n));
			if (*ppItem) {
				if (*ppItem == pItem)
					*ppItem = 0;
			}
			else {
				if (pItem->isFocusItem())
					return pItem;
			}
		}
	}
	
	return 0;
}

EditWindowItem* qm::HeaderEditLine::getPrevFocusItem(
	EditWindowItem** ppItem) const
{
	assert(ppItem);
	
	if (!bHide_) {
		for (unsigned int n = getItemCount(); n > 0; --n) {
			HeaderEditItem* pItem = static_cast<HeaderEditItem*>(getItem(n - 1));
			if (*ppItem) {
				if (*ppItem == pItem)
					*ppItem = 0;
			}
			else {
				if (pItem->isFocusItem())
					return pItem;
			}
		}
	}
	
	return 0;
}

HeaderEditItem* qm::HeaderEditLine::getFocusedItem() const
{
	if (!bHide_) {
		for (unsigned int n = 0; n < getItemCount(); ++n) {
			HeaderEditItem* pItem = static_cast<HeaderEditItem*>(getItem(n));
			if (pItem->hasFocus())
				return pItem;
		}
	}
	return 0;
}

HeaderEditItem* qm::HeaderEditLine::getInitialFocusItem() const
{
	if (!bHide_) {
		for (unsigned int n = 0; n < getItemCount(); ++n) {
			HeaderEditItem* pItem = static_cast<HeaderEditItem*>(getItem(n));
			if (pItem->hasInitialFocus())
				return pItem;
		}
	}
	return 0;
}

bool qm::HeaderEditLine::isHidden() const
{
	return bHide_ || (nFlags_ & FLAG_HIDEIFNOFOCUS && pCallback_->isHidden());
}


/****************************************************************************
 *
 * HeaderEditLineCallback
 *
 */

qm::HeaderEditLineCallback::~HeaderEditLineCallback()
{
}


/****************************************************************************
 *
 * HeaderEditItem
 *
 */

qm::HeaderEditItem::HeaderEditItem(
	EditWindowFocusController* pController, QSTATUS* pstatus) :
	LineLayoutItem(pstatus),
	pController_(pController),
	nNumber_(-1),
	initialFocus_(INITIALFOCUS_NONE),
	wstrValue_(0)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qm::HeaderEditItem::~HeaderEditItem()
{
	freeWString(wstrValue_);
}

unsigned int qm::HeaderEditItem::getNumber() const
{
	return nNumber_;
}

QSTATUS qm::HeaderEditItem::setNumber(const WCHAR* pwszNumber)
{
	WCHAR* pEnd = 0;
	nNumber_ = wcstol(pwszNumber, &pEnd, 10);
	return *pEnd ? QSTATUS_FAIL : QSTATUS_SUCCESS;
}

void qm::HeaderEditItem::setInitialFocus(bool bInitialFocus)
{
	initialFocus_ = bInitialFocus ? INITIALFOCUS_TRUE : INITIALFOCUS_FALSE;
}

QSTATUS qm::HeaderEditItem::addValue(const WCHAR* pwszValue, size_t nLen)
{
	if (wstrValue_) {
		size_t nOrgLen = wcslen(wstrValue_);
		string_ptr<WSTRING> wstrValue(allocWString(
			wstrValue_,  nOrgLen + nLen + 1));
		if (!wstrValue.get())
			return QSTATUS_OUTOFMEMORY;
		wcsncpy(wstrValue.get() + nOrgLen, pwszValue, nLen);
		*(wstrValue.get() + nOrgLen + nLen) = L'\0';
		wstrValue_ = wstrValue.release();
	}
	else {
		wstrValue_ = allocWString(pwszValue, nLen);
		if (!wstrValue_)
			return QSTATUS_OUTOFMEMORY;
	}
	return QSTATUS_SUCCESS;
}

EditWindowFocusController* qm::HeaderEditItem::getController() const
{
	return pController_;
}

HeaderEditItem::InitialFocus qm::HeaderEditItem::getInitialFocus() const
{
	return initialFocus_;
}

const WCHAR* qm::HeaderEditItem::getValue() const
{
	return wstrValue_;
}

QSTATUS qm::HeaderEditItem::copy()
{
	return QSTATUS_SUCCESS;
}

QSTATUS qm::HeaderEditItem::canCopy(bool* pbCan)
{
	assert(pbCan);
	*pbCan = false;
	return QSTATUS_SUCCESS;
}

QSTATUS qm::HeaderEditItem::cut()
{
	return QSTATUS_SUCCESS;
}

QSTATUS qm::HeaderEditItem::canCut(bool* pbCan)
{
	assert(pbCan);
	*pbCan = false;
	return QSTATUS_SUCCESS;
}

QSTATUS qm::HeaderEditItem::paste()
{
	return QSTATUS_SUCCESS;
}

QSTATUS qm::HeaderEditItem::canPaste(bool* pbCan)
{
	assert(pbCan);
	*pbCan = false;
	return QSTATUS_SUCCESS;
}

QSTATUS qm::HeaderEditItem::selectAll()
{
	return QSTATUS_SUCCESS;
}

QSTATUS qm::HeaderEditItem::canSelectAll(bool* pbCan)
{
	assert(pbCan);
	*pbCan = false;
	return QSTATUS_SUCCESS;
}

QSTATUS qm::HeaderEditItem::undo()
{
	return QSTATUS_SUCCESS;
}

QSTATUS qm::HeaderEditItem::canUndo(bool* pbCan)
{
	assert(pbCan);
	*pbCan = false;
	return QSTATUS_SUCCESS;
}

QSTATUS qm::HeaderEditItem::redo()
{
	return QSTATUS_SUCCESS;
}

QSTATUS qm::HeaderEditItem::canRedo(bool* pbCan)
{
	assert(pbCan);
	*pbCan = false;
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * TextHeaderEditItem
 *
 */

qm::TextHeaderEditItem::TextHeaderEditItem(
	EditWindowFocusController* pController, QSTATUS* pstatus) :
	HeaderEditItem(pController, pstatus),
	nStyle_(0),
	wstrField_(0),
	type_(TYPE_UNSTRUCTURED),
	hwnd_(0),
	pItemWindow_(0)
{
}

qm::TextHeaderEditItem::~TextHeaderEditItem()
{
	freeWString(wstrField_);
	delete pItemWindow_;
}

QSTATUS qm::TextHeaderEditItem::setEditMessage(
	EditMessage* pEditMessage, bool bReset)
{
	DECLARE_QSTATUS();
	
	const WCHAR* pwszValue = getValue();
	if (pwszValue) {
		Window(hwnd_).setWindowText(pwszValue);
	}
	else {
		string_ptr<WSTRING> wstrValue;
		status = pEditMessage->getField(wstrField_,
			static_cast<EditMessage::FieldType>(type_), &wstrValue);
		CHECK_QSTATUS();
		Window(hwnd_).setWindowText(wstrValue.get());
	}
	
	if (!bReset) {
		status = pEditMessage->addEditMessageHandler(this);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

void qm::TextHeaderEditItem::releaseEditMessage(EditMessage* pEditMessage)
{
	pEditMessage->removeEditMessageHandler(this);
}

bool qm::TextHeaderEditItem::hasFocus() const
{
	return Window(hwnd_).hasFocus();
}

QSTATUS qm::TextHeaderEditItem::setStyle(const WCHAR* pwszStyle)
{
	const WCHAR* p = pwszStyle;
	while (p) {
		const WCHAR* pEnd = wcschr(p, L',');
		size_t nLen = pEnd ? pEnd - p : wcslen(p);
		struct {
			const WCHAR* pwszName_;
			Style style_;
		} styles[] = {
			{ L"bold",		STYLE_BOLD		},
			{ L"italic",	STYLE_ITALIC	}
		};
		for (int n = 0; n < countof(styles); ++n) {
			if (wcsncmp(p, styles[n].pwszName_, nLen) == 0) {
				nStyle_ |= styles[n].style_;
				break;
			}
		}
		if (n == countof(styles))
			return QSTATUS_FAIL;
		p = pEnd ? pEnd + 1 : 0;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::TextHeaderEditItem::setField(const WCHAR* pwszField)
{
	wstrField_ = allocWString(pwszField);
	if (!wstrField_)
		return QSTATUS_OUTOFMEMORY;
	return QSTATUS_SUCCESS;
}

QSTATUS qm::TextHeaderEditItem::setType(const WCHAR* pwszType)
{
	struct {
		const WCHAR* pwszName_;
		Type type_;
	} types[] = {
		{ L"unstructured",	TYPE_UNSTRUCTURED	},
		{ L"addressList",	TYPE_ADDRESSLIST	}
	};
	for (int n = 0; n < countof(types); ++n) {
		if (wcscmp(pwszType, types[n].pwszName_) == 0) {
			type_ = types[n].type_;
			break;
		}
	}
	if (n == countof(types))
		return QSTATUS_FAIL;
	return QSTATUS_SUCCESS;
}

QSTATUS qm::TextHeaderEditItem::create(WindowBase* pParent,
	const std::pair<HFONT, HFONT>& fonts, UINT nId)
{
	assert(!hwnd_);
	
	DECLARE_QSTATUS();
	
	hwnd_ = ::CreateWindowEx(getWindowExStyle(),
		getWindowClassName(), 0, WS_CHILD | WS_VISIBLE | getWindowStyle(),
		0, 0, 0, 0, pParent->getHandle(), reinterpret_cast<HMENU>(nId),
		Init::getInit().getInstanceHandle(), 0);
	if (!hwnd_)
		return QSTATUS_FAIL;
	
	Window(hwnd_).setFont((nStyle_ & STYLE_BOLD) ? fonts.second : fonts.first);
	
	status = newQsObject(getController(), this, hwnd_, &pItemWindow_);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::TextHeaderEditItem::destroy()
{
	return QSTATUS_SUCCESS;
}

QSTATUS qm::TextHeaderEditItem::layout(
	const RECT& rect, unsigned int nFontHeight)
{
	unsigned int nHeight = getHeight(nFontHeight);
	Window(hwnd_).setWindowPos(0, rect.left,
		rect.top + ((rect.bottom - rect.top) - nHeight)/2,
		rect.right - rect.left, nHeight,
		SWP_NOZORDER | SWP_NOACTIVATE);
	return QSTATUS_SUCCESS;
}

QSTATUS qm::TextHeaderEditItem::show(bool bShow)
{
	Window(hwnd_).showWindow(bShow ? SW_SHOW : SW_HIDE);
	return QSTATUS_SUCCESS;
}

QSTATUS qm::TextHeaderEditItem::fieldChanged(const EditMessageFieldEvent& event)
{
	if (!getValue() && wcsicmp(event.getName(), wstrField_) == 0)
		Window(hwnd_).setWindowText(event.getValue());
	return QSTATUS_SUCCESS;
}

HWND qm::TextHeaderEditItem::getHandle() const
{
	return hwnd_;
}

const WCHAR* qm::TextHeaderEditItem::getField() const
{
	return wstrField_;
}

TextHeaderEditItem::Type qm::TextHeaderEditItem::getType() const
{
	return type_;
}

QSTATUS qm::TextHeaderEditItem::setFocus()
{
	Window(hwnd_).setFocus();
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * StaticHeaderEditItem
 *
 */

qm::StaticHeaderEditItem::StaticHeaderEditItem(
	EditWindowFocusController* pController, QSTATUS* pstatus) :
	TextHeaderEditItem(pController, pstatus)
{
	if (*pstatus != QSTATUS_SUCCESS)
		return;
}

qm::StaticHeaderEditItem::~StaticHeaderEditItem()
{
}

QSTATUS qm::StaticHeaderEditItem::updateEditMessage(EditMessage* pEditMessage)
{
	return QSTATUS_SUCCESS;
}

bool qm::StaticHeaderEditItem::hasInitialFocus() const
{
	return false;
}

bool qm::StaticHeaderEditItem::isFocusItem() const
{
	return false;
}

unsigned int qm::StaticHeaderEditItem::getHeight(unsigned int nFontHeight) const
{
	return nFontHeight;
}

const TCHAR* qm::StaticHeaderEditItem::getWindowClassName() const
{
	return _T("STATIC");
}

UINT qm::StaticHeaderEditItem::getWindowStyle() const
{
	return SS_LEFTNOWORDWRAP | (getValue() ? 0 : SS_NOPREFIX);
}

UINT qm::StaticHeaderEditItem::getWindowExStyle() const
{
	return 0;
}


/****************************************************************************
 *
 * EditHeaderEditItem
 *
 */

qm::EditHeaderEditItem::EditHeaderEditItem(
	EditWindowFocusController* pController, QSTATUS* pstatus) :
	TextHeaderEditItem(pController, pstatus),
	DefaultCommandHandler(pstatus),
	pEditMessage_(0),
	bExpandAlias_(false),
	pAddressBook_(0),
	pParent_(0),
	nId_(0)
{
}

qm::EditHeaderEditItem::~EditHeaderEditItem()
{
}

void qm::EditHeaderEditItem::setExpandAlias(bool bExpandAlias)
{
	bExpandAlias_ = bExpandAlias;
}

QSTATUS qm::EditHeaderEditItem::setEditMessage(
	EditMessage* pEditMessage, bool bReset)
{
	DECLARE_QSTATUS();
	
	pEditMessage_ = pEditMessage;
	pAddressBook_ = pEditMessage->getDocument()->getAddressBook();
	
	return TextHeaderEditItem::setEditMessage(pEditMessage, bReset);
}

QSTATUS qm::EditHeaderEditItem::updateEditMessage(EditMessage* pEditMessage)
{
	DECLARE_QSTATUS();
	
	if (!getValue()) {
		string_ptr<WSTRING> wstrText(Window(getHandle()).getWindowText());
		if (!wstrText.get())
			return QSTATUS_OUTOFMEMORY;
		status = pEditMessage->setField(getField(), wstrText.get(),
			static_cast<EditMessage::FieldType>(getType()));
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

void qm::EditHeaderEditItem::releaseEditMessage(EditMessage* pEditMessage)
{
	pEditMessage_ = 0;
	TextHeaderEditItem::releaseEditMessage(pEditMessage);
}

bool qm::EditHeaderEditItem::hasInitialFocus() const
{
	InitialFocus f = getInitialFocus();
	return f == INITIALFOCUS_NONE ?
		Window(getHandle()).getWindowTextLength() == 0 :
		f == INITIALFOCUS_TRUE;
}

bool qm::EditHeaderEditItem::isFocusItem() const
{
	return true;
}

unsigned int qm::EditHeaderEditItem::getHeight(unsigned int nFontHeight) const
{
	return nFontHeight + 7;
}

QSTATUS qm::EditHeaderEditItem::create(qs::WindowBase* pParent,
	const std::pair<HFONT, HFONT>& fonts, UINT nId)
{
	DECLARE_QSTATUS();
	
	status = TextHeaderEditItem::create(pParent, fonts, nId);
	CHECK_QSTATUS();
	status = pParent->addCommandHandler(this);
	CHECK_QSTATUS();
	
	pParent_ = pParent;
	nId_ = nId;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::EditHeaderEditItem::destroy()
{
	if (pParent_)
		pParent_->removeCommandHandler(this);
	return TextHeaderEditItem::destroy();
}

const TCHAR* qm::EditHeaderEditItem::getWindowClassName() const
{
	return _T("EDIT");
}

UINT qm::EditHeaderEditItem::getWindowStyle() const
{
#if defined _WIN32_WCE && _WIN32_WCE >= 300 && defined _WIN32_WCE_PSPC
	return WS_BORDER | ES_AUTOHSCROLL;
#else
	return ES_AUTOHSCROLL;
#endif
}

UINT qm::EditHeaderEditItem::getWindowExStyle() const
{
#if defined _WIN32_WCE && _WIN32_WCE >=300 && defined _WIN32_WCE_PSPC
	return 0;
#else
	return WS_EX_CLIENTEDGE;
#endif
}

QSTATUS qm::EditHeaderEditItem::copy()
{
	Window(getHandle()).sendMessage(WM_COPY);
	return QSTATUS_SUCCESS;
}

QSTATUS qm::EditHeaderEditItem::canCopy(bool* pbCan)
{
	assert(pbCan);
	DWORD dwStart = 0;
	DWORD dwEnd = 0;
	Window(getHandle()).sendMessage(EM_GETSEL,
		reinterpret_cast<WPARAM>(&dwStart),
		reinterpret_cast<LPARAM>(&dwEnd));
	*pbCan = dwStart != dwEnd;
	return QSTATUS_SUCCESS;
}

QSTATUS qm::EditHeaderEditItem::cut()
{
	Window(getHandle()).sendMessage(WM_CUT);
	return QSTATUS_SUCCESS;
}

QSTATUS qm::EditHeaderEditItem::canCut(bool* pbCan)
{
	assert(pbCan);
	DWORD dwStart = 0;
	DWORD dwEnd = 0;
	Window(getHandle()).sendMessage(EM_GETSEL,
		reinterpret_cast<WPARAM>(&dwStart),
		reinterpret_cast<LPARAM>(&dwEnd));
	*pbCan = dwStart != dwEnd;
	return QSTATUS_SUCCESS;
}

QSTATUS qm::EditHeaderEditItem::paste()
{
	Window(getHandle()).sendMessage(WM_PASTE);
	return QSTATUS_SUCCESS;
}

QSTATUS qm::EditHeaderEditItem::canPaste(bool* pbCan)
{
	assert(pbCan);
	return Clipboard::isFormatAvailable(Clipboard::CF_QSTEXT, pbCan);
}

QSTATUS qm::EditHeaderEditItem::selectAll()
{
	Window(getHandle()).sendMessage(EM_SETSEL, 0, -1);
	return QSTATUS_SUCCESS;
}

QSTATUS qm::EditHeaderEditItem::canSelectAll(bool* pbCan)
{
	assert(pbCan);
	*pbCan = true;
	return QSTATUS_SUCCESS;
}

QSTATUS qm::EditHeaderEditItem::undo()
{
	Window(getHandle()).sendMessage(EM_UNDO);
	return QSTATUS_SUCCESS;
}

QSTATUS qm::EditHeaderEditItem::canUndo(bool* pbCan)
{
	assert(pbCan);
	*pbCan = Window(getHandle()).sendMessage(EM_CANUNDO) != 0;
	return QSTATUS_SUCCESS;
}

LRESULT qm::EditHeaderEditItem::onCommand(WORD nCode, WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID_CODE(nId_, EN_KILLFOCUS, onKillFocus)
	END_COMMAND_HANDLER()
	return DefaultCommandHandler::onCommand(nCode, nId);
}

LRESULT qm::EditHeaderEditItem::onKillFocus()
{
	DECLARE_QSTATUS();
	
	if (bExpandAlias_) {
		Window wnd(getHandle());
		string_ptr<WSTRING> wstrText(wnd.getWindowText());
		if (!wstrText.get())
			return 0;
		if (*wstrText.get()) {
			string_ptr<WSTRING> wstr;
			status = pAddressBook_->expandAlias(wstrText.get(), &wstr);
			CHECK_QSTATUS_VALUE(0);
			wnd.setWindowText(wstr.get());
		}
	}
	
	if (pEditMessage_) {
		status = updateEditMessage(pEditMessage_);
		CHECK_QSTATUS_VALUE(0);
	}
	
	return 0;
}


/****************************************************************************
 *
 * AttachmentHeaderEditItem
 *
 */

qm::AttachmentHeaderEditItem::AttachmentHeaderEditItem(
	EditWindowFocusController* pController,
	MenuManager* pMenuManager, QSTATUS* pstatus) :
	HeaderEditItem(pController, pstatus),
	wnd_(this, pstatus),
	pMenuManager_(pMenuManager),
	pItemWindow_(0),
	pEditMessage_(0)
{
}

qm::AttachmentHeaderEditItem::~AttachmentHeaderEditItem()
{
	delete pItemWindow_;
}

QSTATUS qm::AttachmentHeaderEditItem::setEditMessage(
	EditMessage* pEditMessage, bool bReset)
{
	DECLARE_QSTATUS();
	
	status = update(pEditMessage);
	CHECK_QSTATUS();
	
	if (!bReset) {
		status = pEditMessage->addEditMessageHandler(this);
		CHECK_QSTATUS();
	}
	
	pEditMessage_ = pEditMessage;
	
	return QSTATUS_SUCCESS;
}

void qm::AttachmentHeaderEditItem::releaseEditMessage(EditMessage* pEditMessage)
{
	pEditMessage->removeEditMessageHandler(this);
	pEditMessage_ = 0;
}

QSTATUS qm::AttachmentHeaderEditItem::updateEditMessage(EditMessage* pEditMessage)
{
	// TODO
	return QSTATUS_SUCCESS;
}

bool qm::AttachmentHeaderEditItem::hasFocus() const
{
	return wnd_.hasFocus();
}

bool qm::AttachmentHeaderEditItem::hasInitialFocus() const
{
	return getInitialFocus() == INITIALFOCUS_TRUE;
}

bool qm::AttachmentHeaderEditItem::isFocusItem() const
{
	return true;
}

unsigned int qm::AttachmentHeaderEditItem::getHeight(unsigned int nFontHeight) const
{
	return nFontHeight + 7;
}

QSTATUS qm::AttachmentHeaderEditItem::create(WindowBase* pParent,
	const std::pair<HFONT, HFONT>& fonts, UINT nId)
{
	assert(!wnd_.getHandle());
	
	DECLARE_QSTATUS();
	
	DWORD dwStyle = WS_CHILD | WS_VISIBLE | LVS_SMALLICON | LVS_SHAREIMAGELISTS;
#if defined _WIN32_WCE && _WIN32_WCE >= 300 && defined _WIN32_WCE_PSPC
	dwStyle |= WS_BORDER;
#endif
#if defined _WIN32_WCE && _WIN32_WCE >= 300 && defined _WIN32_WCE_PSPC
	DWORD dwExStyle = 0;
#else
	DWORD dwExStyle = WS_EX_CLIENTEDGE;
#endif
	status = wnd_.create(L"QmAttachmentWindow", 0, dwStyle,
		0, 0, 0, 0, pParent->getHandle(), dwExStyle, WC_LISTVIEWW, nId, 0);
	CHECK_QSTATUS();
	
	SHFILEINFO info = { 0 };
	HIMAGELIST hImageList = reinterpret_cast<HIMAGELIST>(::SHGetFileInfo(
		_T("dummy.txt"), FILE_ATTRIBUTE_NORMAL, &info, sizeof(info),
		SHGFI_USEFILEATTRIBUTES | SHGFI_SYSICONINDEX | SHGFI_SMALLICON));
	ListView_SetImageList(wnd_.getHandle(), hImageList, LVSIL_SMALL);
	
	wnd_.setFont(fonts.first);
	
	status = newQsObject(getController(), this, wnd_.getHandle(), &pItemWindow_);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::AttachmentHeaderEditItem::destroy()
{
	clear();
	return QSTATUS_SUCCESS;
}

QSTATUS qm::AttachmentHeaderEditItem::layout(
	const RECT& rect, unsigned int nFontHeight)
{
	wnd_.setWindowPos(0, rect.left, rect.top, rect.right - rect.left,
		rect.bottom - rect.top, SWP_NOZORDER | SWP_NOACTIVATE);
	return QSTATUS_SUCCESS;
}

QSTATUS qm::AttachmentHeaderEditItem::show(bool bShow)
{
	wnd_.showWindow(bShow ? SW_SHOW : SW_HIDE);
	return QSTATUS_SUCCESS;
}

QSTATUS qm::AttachmentHeaderEditItem::setFocus()
{
	wnd_.setFocus();
	return QSTATUS_SUCCESS;
}

QSTATUS qm::AttachmentHeaderEditItem::attachmentsChanged(const EditMessageEvent& event)
{
	return update(event.getEditMessage());
}

QSTATUS qm::AttachmentHeaderEditItem::hasAttachment(bool* pbHas)
{
	assert(pbHas);
	*pbHas = ListView_GetItemCount(wnd_.getHandle()) != 0;
	return QSTATUS_SUCCESS;
}

QSTATUS qm::AttachmentHeaderEditItem::hasSelectedAttachment(bool* pbHas)
{
	assert(pbHas);
	*pbHas = ListView_GetSelectedCount(wnd_.getHandle()) != 0;
	return QSTATUS_SUCCESS;
}

QSTATUS qm::AttachmentHeaderEditItem::getSelectedAttachment(NameList* pList)
{
	assert(pList);
	
	DECLARE_QSTATUS();
	
	HWND hwnd = wnd_.getHandle();
	
	int nItem = ListView_GetNextItem(hwnd, -1, LVNI_ALL | LVNI_SELECTED);
	while (nItem != -1) {
		LVITEM item = {
			LVIF_PARAM,
			nItem
		};
		ListView_GetItem(hwnd, &item);
		string_ptr<WSTRING> wstrName(allocWString(
			reinterpret_cast<WSTRING>(item.lParam)));
		if (!wstrName.get())
			return QSTATUS_OUTOFMEMORY;
		status = STLWrapper<NameList>(*pList).push_back(wstrName.get());
		CHECK_QSTATUS();
		wstrName.release();
		nItem = ListView_GetNextItem(hwnd, nItem, LVNI_ALL | LVNI_SELECTED);
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::AttachmentHeaderEditItem::update(EditMessage* pEditMessage)
{
	DECLARE_QSTATUS();
	
	HWND hwnd = wnd_.getHandle();
	
	clear();
	
	EditMessage::AttachmentList l;
	EditMessage::AttachmentListFree free(l);
	status = pEditMessage->getAttachments(&l);
	CHECK_QSTATUS();
	EditMessage::AttachmentList::size_type n = 0;
	while (n < l.size()) {
		string_ptr<WSTRING> wstrPath(l[n].wstrName_);
		l[n].wstrName_ = 0;
		
		const WCHAR* pwszName = 0;
		string_ptr<WSTRING> wstrName;
		if (l[n].bNew_) {
			pwszName = wcsrchr(wstrPath.get(), L'\\');
			pwszName = pwszName ? pwszName + 1 : wstrPath.get();
		}
		else {
			wstrName.reset(concat(L"<", wstrPath.get(), L">"));
			if (!wstrName.get())
				return QSTATUS_OUTOFMEMORY;
			pwszName = wstrName.get();
		}
		
		W2T(pwszName, ptszName);
		SHFILEINFO info = { 0 };
		::SHGetFileInfo(ptszName, FILE_ATTRIBUTE_NORMAL, &info, sizeof(info),
			SHGFI_USEFILEATTRIBUTES | SHGFI_SYSICONINDEX | SHGFI_SMALLICON);
		LVITEM item = {
			LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM,
			n,
			0,
			0,
			0,
			const_cast<LPTSTR>(ptszName),
			0,
			info.iIcon,
			reinterpret_cast<LPARAM>(wstrPath.get())
		};
		ListView_InsertItem(hwnd, &item);
		wstrPath.release();
		++n;
	}
	
	return QSTATUS_SUCCESS;
}

void qm::AttachmentHeaderEditItem::clear()
{
	HWND hwnd = wnd_.getHandle();
	
	for (int n = 0; n < ListView_GetItemCount(hwnd); ++n) {
		LVITEM item = {
			LVIF_PARAM,
			n
		};
		ListView_GetItem(hwnd, &item);
		freeWString(reinterpret_cast<WSTRING>(item.lParam));
	}
	ListView_DeleteAllItems(hwnd);
}


/****************************************************************************
 *
 * AttachmentHeaderEditItem::AttachmentEditWindow
 *
 */

qm::AttachmentHeaderEditItem::AttachmentEditWindow::AttachmentEditWindow(
	AttachmentHeaderEditItem* pItem, QSTATUS* pstatus) :
	WindowBase(false, pstatus),
	DefaultWindowHandler(pstatus),
	pItem_(pItem)
{
	setWindowHandler(this, false);
}

qm::AttachmentHeaderEditItem::AttachmentEditWindow::~AttachmentEditWindow()
{
}

LRESULT qm::AttachmentHeaderEditItem::AttachmentEditWindow::windowProc(
	UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	BEGIN_MESSAGE_HANDLER()
		HANDLE_CONTEXTMENU()
		HANDLE_LBUTTONDOWN()
	END_MESSAGE_HANDLER()
	return DefaultWindowHandler::windowProc(uMsg, wParam, lParam);
}

LRESULT qm::AttachmentHeaderEditItem::AttachmentEditWindow::onContextMenu(
	HWND hwnd, const POINT& pt)
{
	DECLARE_QSTATUS();
	
	HMENU hmenu = 0;
	status = pItem_->pMenuManager_->getMenu(
		L"attachmentedit", false, false, &hmenu);
	if (status == QSTATUS_SUCCESS) {
		UINT nFlags = TPM_LEFTALIGN | TPM_TOPALIGN;
#ifndef _WIN32_WCE
		nFlags |= TPM_LEFTBUTTON | TPM_RIGHTBUTTON;
#endif
		::TrackPopupMenu(hmenu, nFlags, pt.x, pt.y, 0, getParentFrame(), 0);
	}
	return DefaultWindowHandler::onContextMenu(hwnd, pt);
}

LRESULT qm::AttachmentHeaderEditItem::AttachmentEditWindow::onLButtonDown(
	UINT nFlags, const POINT& pt)
{
#if defined _WIN32_WCE && _WIN32_WCE >= 300 && _WIN32_WCE_PSPC
	if (tapAndHold(pt))
		return 0;
#endif
	return DefaultWindowHandler::onLButtonDown(nFlags, pt);
}


/****************************************************************************
 *
 * ComboBoxHeaderEditItem
 *
 */

qm::ComboBoxHeaderEditItem::ComboBoxHeaderEditItem(
	EditWindowFocusController* pController, QSTATUS* pstatus) :
	HeaderEditItem(pController, pstatus),
	DefaultCommandHandler(pstatus),
	hwnd_(0),
	pItemWindow_(0),
	pParent_(0),
	nId_(0)
{
}

qm::ComboBoxHeaderEditItem::~ComboBoxHeaderEditItem()
{
	delete pItemWindow_;
}

void qm::ComboBoxHeaderEditItem::releaseEditMessage(EditMessage* pEditMessage)
{
}

bool qm::ComboBoxHeaderEditItem::hasFocus() const
{
	return Window(hwnd_).hasFocus();
}

bool qm::ComboBoxHeaderEditItem::hasInitialFocus() const
{
	return getInitialFocus() == INITIALFOCUS_TRUE;
}

bool qm::ComboBoxHeaderEditItem::isFocusItem() const
{
	return true;
}

unsigned int qm::ComboBoxHeaderEditItem::getHeight(unsigned int nFontHeight) const
{
	RECT rect;
	Window(hwnd_).getWindowRect(&rect);
	return rect.bottom - rect.top + 1;
}

QSTATUS qm::ComboBoxHeaderEditItem::create(WindowBase* pParent,
	const std::pair<HFONT, HFONT>& fonts, UINT nId)
{
	assert(!hwnd_);
	
	DECLARE_QSTATUS();
	
	DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST;
#if defined _WIN32_WCE && _WIN32_WCE >= 300 && defined _WIN32_WCE_PSPC
	DWORD dwExStyle = 0;
#else
	DWORD dwExStyle = WS_EX_CLIENTEDGE;
#endif
	hwnd_ = ::CreateWindowEx(dwExStyle, _T("COMBOBOX"), 0, dwStyle,
		0, 0, 0, 0, pParent->getHandle(), reinterpret_cast<HMENU>(nId),
		Init::getInit().getInstanceHandle(), 0);
	if (!hwnd_)
		return QSTATUS_FAIL;
	
	Window(hwnd_).setFont(fonts.first);
	
	status = newQsObject(getController(), this, hwnd_, &pItemWindow_);
	CHECK_QSTATUS();
	
	status = pParent->addCommandHandler(this);
	CHECK_QSTATUS();
	
	pParent_ = pParent;
	nId_ = nId;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ComboBoxHeaderEditItem::destroy()
{
	if (pParent_)
		pParent_->removeCommandHandler(this);
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ComboBoxHeaderEditItem::layout(
	const RECT& rect, unsigned int nFontHeight)
{
	Window(hwnd_).setWindowPos(0, rect.left, rect.top,
		rect.right - rect.left, 100,
		SWP_NOZORDER | SWP_NOACTIVATE);
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ComboBoxHeaderEditItem::show(bool bShow)
{
	Window(hwnd_).showWindow(bShow ? SW_SHOW : SW_HIDE);
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ComboBoxHeaderEditItem::setFocus()
{
	Window(hwnd_).setFocus();
	return QSTATUS_SUCCESS;
}

LRESULT qm::ComboBoxHeaderEditItem::onCommand(WORD nCode, WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID_CODE(nId_, CBN_SELCHANGE, onChange)
	END_COMMAND_HANDLER()
	return DefaultCommandHandler::onCommand(nCode, nId);
}

LRESULT qm::ComboBoxHeaderEditItem::onChange()
{
	return 0;
}

HWND qm::ComboBoxHeaderEditItem::getHandle() const
{
	return hwnd_;
}


/****************************************************************************
 *
 * SignatureHeaderEditItem
 *
 */

qm::SignatureHeaderEditItem::SignatureHeaderEditItem(
	EditWindowFocusController* pController, QSTATUS* pstatus) :
	ComboBoxHeaderEditItem(pController, pstatus),
	pEditMessage_(0)
{
}

qm::SignatureHeaderEditItem::~SignatureHeaderEditItem()
{
}

QSTATUS qm::SignatureHeaderEditItem::setEditMessage(
	EditMessage* pEditMessage, bool bReset)
{
	DECLARE_QSTATUS();
	
	status = update(pEditMessage);
	CHECK_QSTATUS();
	if (!bReset) {
		status = pEditMessage->addEditMessageHandler(this);
		CHECK_QSTATUS();
	}
	
	pEditMessage_ = pEditMessage;
	
	return QSTATUS_SUCCESS;
}

void qm::SignatureHeaderEditItem::releaseEditMessage(EditMessage* pEditMessage)
{
	pEditMessage->removeEditMessageHandler(this);
	pEditMessage_ = 0;
}

QSTATUS qm::SignatureHeaderEditItem::updateEditMessage(EditMessage* pEditMessage)
{
	DECLARE_QSTATUS();
	
	Window combo(getHandle());
	int nItem = combo.sendMessage(CB_GETCURSEL);
	int nLen = combo.sendMessage(CB_GETLBTEXTLEN, nItem);
	string_ptr<TSTRING> tstrName(allocTString(nLen + 1));
	if (!tstrName.get())
		return QSTATUS_OUTOFMEMORY;
	combo.sendMessage(CB_GETLBTEXT, nItem, reinterpret_cast<LPARAM>(tstrName.get()));
	T2W(tstrName.get(), pwszName);
	status = pEditMessage->setSignature(pwszName);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::SignatureHeaderEditItem::accountChanged(const EditMessageEvent& event)
{
	return update(event.getEditMessage());
}

QSTATUS qm::SignatureHeaderEditItem::signatureChanged(const EditMessageEvent& event)
{
	DECLARE_QSTATUS();
	
	EditMessage* pEditMessage = event.getEditMessage();
	const WCHAR* pwszName = pEditMessage->getSignature();
	if (pwszName) {
		W2T(pwszName, ptszName);
		Window(getHandle()).sendMessage(CB_SELECTSTRING, 0,
			reinterpret_cast<LPARAM>(ptszName));
	}
	else {
		Window(getHandle()).sendMessage(CB_SETCURSEL);
	}
	
	return QSTATUS_SUCCESS;
}

LRESULT qm::SignatureHeaderEditItem::onChange()
{
	if (pEditMessage_)
		updateEditMessage(pEditMessage_);
	return 0;
}

QSTATUS qm::SignatureHeaderEditItem::update(EditMessage* pEditMessage)
{
	DECLARE_QSTATUS();
	
	const WCHAR* pwszSignature = pEditMessage->getSignature();
	
	SignatureManager* pSignatureManager =
		pEditMessage->getDocument()->getSignatureManager();
	SignatureManager::SignatureList l;
	status = pSignatureManager->getSignatures(pEditMessage->getAccount(), &l);
	CHECK_QSTATUS();
	
	int nSelect = 0;
	Window combo(getHandle());
	combo.sendMessage(CB_RESETCONTENT);
	combo.sendMessage(CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(_T("None")));
	
	SignatureManager::SignatureList::iterator it = l.begin();
	while (it != l.end()) {
		const Signature* pSignature = *it;
		const WCHAR* pwszName = pSignature->getName();
		W2T(pwszName, ptszName);
		int nItem = combo.sendMessage(CB_ADDSTRING, 0,
			reinterpret_cast<LPARAM>(ptszName));
		if (nSelect == 0) {
			if (pwszSignature && wcscmp(pwszName, pwszSignature) == 0)
				nSelect = nItem;
		}
		++it;
	}
	
	combo.sendMessage(CB_SETCURSEL, nSelect);
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * AccountHeaderEditItem
 *
 */

qm::AccountHeaderEditItem::AccountHeaderEditItem(
	EditWindowFocusController* pController, QSTATUS* pstatus) :
	ComboBoxHeaderEditItem(pController, pstatus),
	bShowFrom_(true),
	pEditMessage_(0)
{
}

qm::AccountHeaderEditItem::~AccountHeaderEditItem()
{
}

QSTATUS qm::AccountHeaderEditItem::setEditMessage(
	EditMessage* pEditMessage, bool bReset)
{
	DECLARE_QSTATUS();
	
	Window combo(getHandle());
	
	const WCHAR* pwszClass = pEditMessage->getAccount()->getClass();
	SubAccount* pCurrentSubAccount = pEditMessage->getSubAccount();
	
	combo.sendMessage(CB_RESETCONTENT);
	
	int nCurrentItem = -1;
	const Document::AccountList& listAccount =
		pEditMessage->getDocument()->getAccounts();
	Document::AccountList::const_iterator itA = listAccount.begin();
	while (itA != listAccount.end()) {
		Account* pAccount = *itA;
		if (wcscmp(pAccount->getClass(), pwszClass) == 0) {
			const Account::SubAccountList& listSubAccount =
				pAccount->getSubAccounts();
			Account::SubAccountList::const_iterator itS = listSubAccount.begin();
			while (itS != listSubAccount.end()) {
				SubAccount* pSubAccount = *itS;
				const WCHAR* pwszName = 0;
				string_ptr<WSTRING> wstrName;
				if (pSubAccount->getName() && *pSubAccount->getName()) {
					wstrName.reset(concat(
						pAccount->getName(), L"/", pSubAccount->getName()));
					if (!wstrName.get())
						return QSTATUS_OUTOFMEMORY;
					pwszName = wstrName.get();
				}
				else {
					pwszName = pAccount->getName();
				}
				
				if (bShowFrom_) {
					AddressParser address(pSubAccount->getSenderName(),
						pSubAccount->getSenderAddress(), &status);
					CHECK_QSTATUS();
					string_ptr<WSTRING> wstrValue;
					status = address.getValue(&wstrValue);
					CHECK_QSTATUS();
					
					wstrName.reset(concat(pwszName, L" - ", wstrValue.get()));
					if (!wstrName.get())
						return QSTATUS_OUTOFMEMORY;
					pwszName = wstrName.get();
				}
				
				W2T(pwszName, ptszName);
				int nItem = combo.sendMessage(CB_ADDSTRING, 0,
					reinterpret_cast<LPARAM>(ptszName));
				combo.sendMessage(CB_SETITEMDATA, nItem,
					reinterpret_cast<LPARAM>(pSubAccount));
				
				if (pSubAccount == pCurrentSubAccount)
					nCurrentItem = nItem;
				
				++itS;
			}
		}
		++itA;
	}
	
	combo.sendMessage(CB_SETCURSEL, nCurrentItem);
	
	if (!bReset) {
		status = pEditMessage->addEditMessageHandler(this);
		CHECK_QSTATUS();
	}
	
	pEditMessage_ = pEditMessage;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::AccountHeaderEditItem::updateEditMessage(EditMessage* pEditMessage)
{
	return QSTATUS_SUCCESS;
}

void qm::AccountHeaderEditItem::releaseEditMessage(EditMessage* pEditMessage)
{
	pEditMessage->removeEditMessageHandler(this);
	pEditMessage_ = 0;
}

void qm::AccountHeaderEditItem::setShowFrom(bool bShow)
{
	bShowFrom_ = bShow;
}

QSTATUS qm::AccountHeaderEditItem::accountChanged(const EditMessageEvent& event)
{
	Window combo(getHandle());
	
	SubAccount* pCurrentSubAccount = event.getEditMessage()->getSubAccount();
	
	int nCount = combo.sendMessage(CB_GETCOUNT);
	for (int n = 0; n < nCount; ++n) {
		SubAccount* pSubAccount = reinterpret_cast<SubAccount*>(
			combo.sendMessage(CB_GETITEMDATA, n));
		if (pSubAccount == pCurrentSubAccount)
			break;
	}
	combo.sendMessage(CB_SETCURSEL, n);
	
	return QSTATUS_SUCCESS;
}

LRESULT qm::AccountHeaderEditItem::onChange()
{
	if (pEditMessage_) {
		Window combo(getHandle());
		int nItem = combo.sendMessage(CB_GETCURSEL);
		SubAccount* pSubAccount = reinterpret_cast<SubAccount*>(
			combo.sendMessage(CB_GETITEMDATA, nItem));
		pEditMessage_->setAccount(pSubAccount->getAccount(), pSubAccount);
	}
	return 0;
}


/****************************************************************************
 *
 * HeaderEditWindowContentHandler
 *
 */

qm::HeaderEditWindowContentHandler::HeaderEditWindowContentHandler(
	LineLayout* pLayout, EditWindowFocusController* pController,
	MenuManager* pMenuManager, HeaderEditLineCallback* pCallback, QSTATUS* pstatus) :
	DefaultHandler(pstatus),
	pLayout_(pLayout),
	pController_(pController),
	pMenuManager_(pMenuManager),
	pCallback_(pCallback),
	pCurrentLine_(0),
	pCurrentItem_(0),
	state_(STATE_ROOT)
{
}

qm::HeaderEditWindowContentHandler::~HeaderEditWindowContentHandler()
{
}

AttachmentSelectionModel* qm::HeaderEditWindowContentHandler::getAttachmentSelectionModel() const
{
	return pAttachmentSelectionModel_;
}

QSTATUS qm::HeaderEditWindowContentHandler::startElement(
	const WCHAR* pwszNamespaceURI, const WCHAR* pwszLocalName,
	const WCHAR* pwszQName, const Attributes& attributes)
{
	DECLARE_QSTATUS();
	
	if (wcscmp(pwszLocalName, L"headerEdit") == 0) {
		if (state_ != STATE_ROOT)
			return QSTATUS_FAIL;
		state_ = STATE_HEADEREDIT;
	}
	else if (wcscmp(pwszLocalName, L"line") == 0) {
		if (state_ != STATE_HEADEREDIT)
			return QSTATUS_FAIL;
		
		unsigned int nFlags = 0;
		const WCHAR* pwszClass = 0;
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrLocalName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrLocalName, L"hideIfNoFocus") == 0) {
				if (wcscmp(attributes.getValue(n), L"true") == 0)
					nFlags = HeaderEditLine::FLAG_HIDEIFNOFOCUS;
			}
			else if (wcscmp(pwszAttrLocalName, L"class") == 0) {
				pwszClass = attributes.getValue(n);
			}
			else {
				return QSTATUS_FAIL;
			}
		}
		
		std::auto_ptr<RegexPattern> pClass;
		if (pwszClass) {
			RegexCompiler compiler;
			RegexPattern* p = 0;
			status = compiler.compile(pwszClass, &p);
			CHECK_QSTATUS();
			pClass.reset(p);
		}
		std::auto_ptr<HeaderEditLine> pHeaderEditLine;
		status = newQsObject(pCallback_, nFlags,
			pClass.get(), &pHeaderEditLine);
		CHECK_QSTATUS();
		pClass.release();
		status = pLayout_->addLine(pHeaderEditLine.get());
		CHECK_QSTATUS();
		pCurrentLine_ = pHeaderEditLine.release();
		
		state_ = STATE_LINE;
	}
	else if (wcscmp(pwszLocalName, L"static") == 0 ||
		wcscmp(pwszLocalName, L"edit") == 0) {
		if (state_ != STATE_LINE)
			return QSTATUS_FAIL;
		
		assert(pCurrentLine_);
		
		bool bEdit = wcscmp(pwszLocalName, L"edit") == 0;
		std::auto_ptr<TextHeaderEditItem> pItem;
		if (!bEdit) {
			std::auto_ptr<StaticHeaderEditItem> p;
			status = newQsObject(pController_, &p);
			CHECK_QSTATUS();
			pItem.reset(p.release());
		}
		else {
			std::auto_ptr<EditHeaderEditItem> p;
			status = newQsObject(pController_, &p);
			CHECK_QSTATUS();
			pItem.reset(p.release());
		}
		
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrLocalName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrLocalName, L"width") == 0) {
				status = pItem->setWidth(attributes.getValue(n));
				CHECK_QSTATUS();
			}
			else if (wcscmp(pwszAttrLocalName, L"style") == 0) {
				status = pItem->setStyle(attributes.getValue(n));
				CHECK_QSTATUS();
			}
			else if (wcscmp(pwszAttrLocalName, L"field") == 0) {
				status = pItem->setField(attributes.getValue(n));
				CHECK_QSTATUS();
			}
			else if (wcscmp(pwszAttrLocalName, L"type") == 0) {
				status = pItem->setType(attributes.getValue(n));
				CHECK_QSTATUS();
			}
			else if (wcscmp(pwszAttrLocalName, L"number") == 0) {
				status = pItem->setNumber(attributes.getValue(n));
				CHECK_QSTATUS();
			}
			else if (wcscmp(pwszAttrLocalName, L"initialFocus") == 0) {
				pItem->setInitialFocus(wcscmp(attributes.getValue(n), L"true") == 0);
			}
			else if (wcscmp(pwszAttrLocalName, L"expandAlias") == 0 && bEdit) {
				static_cast<EditHeaderEditItem*>(pItem.get())->setExpandAlias(
					wcscmp(attributes.getValue(n), L"true") == 0);
			}
			else {
				return QSTATUS_FAIL;
			}
		}
		
		status = pCurrentLine_->addItem(pItem.get());
		CHECK_QSTATUS();
		pCurrentItem_ = pItem.release();
		
		state_ = STATE_ITEM;
	}
	else if (wcscmp(pwszLocalName, L"attachment") == 0 ||
		wcscmp(pwszLocalName, L"signature") == 0) {
		if (state_ != STATE_LINE)
			return QSTATUS_FAIL;
		
		assert(pCurrentLine_);
		
		std::auto_ptr<HeaderEditItem> pItem;
		if (wcscmp(pwszLocalName, L"attachment") == 0) {
			std::auto_ptr<AttachmentHeaderEditItem> p;
			status = newQsObject(pController_, pMenuManager_, &p);
			CHECK_QSTATUS();
			pAttachmentSelectionModel_ = p.get();
			pItem.reset(p.release());
		}
		else if (wcscmp(pwszLocalName, L"signature") == 0) {
			std::auto_ptr<SignatureHeaderEditItem> p;
			status = newQsObject(pController_, &p);
			CHECK_QSTATUS();
			pItem.reset(p.release());
		}
		else {
			assert(false);
		}
		
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrLocalName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrLocalName, L"width") == 0) {
				status = pItem->setWidth(attributes.getValue(n));
				CHECK_QSTATUS();
			}
			else if (wcscmp(pwszAttrLocalName, L"number") == 0) {
				status = pItem->setNumber(attributes.getValue(n));
				CHECK_QSTATUS();
			}
			else if (wcscmp(pwszAttrLocalName, L"initialFocus") == 0) {
				pItem->setInitialFocus(wcscmp(attributes.getValue(n), L"true") == 0);
			}
			else {
				return QSTATUS_FAIL;
			}
		}
		
		status = pCurrentLine_->addItem(pItem.get());
		CHECK_QSTATUS();
		pCurrentItem_ = pItem.release();
		
		state_ = STATE_ITEM;
	}
	else if (wcscmp(pwszLocalName, L"account") == 0) {
		if (state_ != STATE_LINE)
			return QSTATUS_FAIL;
		
		assert(pCurrentLine_);
		
		std::auto_ptr<AccountHeaderEditItem> pItem;
		status = newQsObject(pController_, &pItem);
		CHECK_QSTATUS();
		
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrLocalName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrLocalName, L"width") == 0) {
				status = pItem->setWidth(attributes.getValue(n));
				CHECK_QSTATUS();
			}
			else if (wcscmp(pwszAttrLocalName, L"number") == 0) {
				status = pItem->setNumber(attributes.getValue(n));
				CHECK_QSTATUS();
			}
			else if (wcscmp(pwszAttrLocalName, L"initialFocus") == 0) {
				pItem->setInitialFocus(wcscmp(attributes.getValue(n), L"true") == 0);
			}
			else if (wcscmp(pwszAttrLocalName, L"showFrom") == 0) {
				pItem->setShowFrom(wcscmp(attributes.getValue(n), L"true") == 0);
			}
			else {
				return QSTATUS_FAIL;
			}
		}
		
		status = pCurrentLine_->addItem(pItem.get());
		CHECK_QSTATUS();
		pCurrentItem_ = pItem.release();
		
		state_ = STATE_ITEM;
	}
	else {
		return QSTATUS_FAIL;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::HeaderEditWindowContentHandler::endElement(
	const WCHAR* pwszNamespaceURI, const WCHAR* pwszLocalName,
	const WCHAR* pwszQName)
{
	DECLARE_QSTATUS();
	
	if (wcscmp(pwszLocalName, L"headerEdit") == 0) {
		assert(state_ == STATE_HEADEREDIT);
		state_ = STATE_ROOT;
	}
	else if (wcscmp(pwszLocalName, L"line") == 0) {
		assert(state_ == STATE_LINE);
		assert(pCurrentLine_);
		pCurrentLine_ = 0;
		state_ = STATE_HEADEREDIT;
	}
	else if (wcscmp(pwszLocalName, L"static") == 0 ||
		wcscmp(pwszLocalName, L"edit") == 0 ||
		wcscmp(pwszLocalName, L"attachment") == 0 ||
		wcscmp(pwszLocalName, L"signature") == 0 ||
		wcscmp(pwszLocalName, L"account") == 0) {
		assert(state_ == STATE_ITEM);
		assert(pCurrentItem_);
		pCurrentItem_ = 0;
		state_ = STATE_LINE;
	}
	else {
		return QSTATUS_FAIL;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::HeaderEditWindowContentHandler::characters(
	const WCHAR* pwsz, size_t nStart, size_t nLength)
{
	DECLARE_QSTATUS();
	
	if (state_ == STATE_ITEM) {
		assert(pCurrentItem_);
		
		status = pCurrentItem_->addValue(pwsz + nStart, nLength);
		CHECK_QSTATUS();
	}
	else {
		const WCHAR* p = pwsz + nStart;
		for (size_t n = 0; n < nLength; ++n, ++p) {
			if (*p != L' ' && *p != L'\t' && *p != L'\n')
				return QSTATUS_FAIL;
		}
	}
	
	return QSTATUS_SUCCESS;
}
