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
#include <qmdocument.h>
#include <qmeditwindow.h>
#include <qmfilenames.h>
#include <qmmessage.h>

#include <qsconv.h>
#include <qsdevicecontext.h>
#include <qsinit.h>
#include <qsuiutil.h>

#include <algorithm>

#include <tchar.h>

#include "headereditwindow.h"
#include "resourceinc.h"
#include "uiutil.h"
#include "../model/addressbook.h"
#include "../model/dataobject.h"
#include "../model/editmessage.h"
#include "../model/recentaddress.h"
#include "../model/signature.h"
#include "../model/uri.h"
#include "../util/util.h"

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
	bool load(const WCHAR* pwszClass,
			  MenuManager* pMenuManager,
			  HeaderEditLineCallback* pLineCallback,
			  HeaderEditItemCallback* pItemCallback);
	bool create(const WCHAR* pwszClass,
				MenuManager* pMenuManager,
				HeaderEditLineCallback* pLineCallback,
				HeaderEditItemCallback* pItemCallback);
	void reloadProfiles(bool bInitialize);

public:
	HeaderEditWindow* pThis_;
	Profile* pProfile_;
	
	HFONT hfont_;
	HFONT hfontBold_;
	HBRUSH hbrBackground_;
	std::auto_ptr<LineLayout> pLayout_;
	EditWindowFocusController* pController_;
	AttachmentSelectionModel* pAttachmentSelectionModel_;
};

bool qm::HeaderEditWindowImpl::load(const WCHAR* pwszClass,
									MenuManager* pMenuManager,
									HeaderEditLineCallback* pLineCallback,
									HeaderEditItemCallback* pItemCallback)
{
	pLayout_.reset(new LineLayout());
	pLayout_->setLineSpacing(1);
	
	wstring_ptr wstrPath(Application::getApplication().getProfilePath(FileNames::HEADEREDIT_XML));
	
	XMLReader reader;
	HeaderEditWindowContentHandler contentHandler(pLayout_.get(), pwszClass,
		pController_, pMenuManager, pProfile_, pLineCallback, pItemCallback);
	reader.setContentHandler(&contentHandler);
	if (!reader.parse(wstrPath.get()))
		return false;
	pAttachmentSelectionModel_ = contentHandler.getAttachmentSelectionModel();
	
	return true;
}

bool qm::HeaderEditWindowImpl::create(const WCHAR* pwszClass,
									  MenuManager* pMenuManager,
									  HeaderEditLineCallback* pLineCallback,
									  HeaderEditItemCallback* pItemCallback)
{
	if (!load(pwszClass, pMenuManager, pLineCallback, pItemCallback))
		return false;
	
	std::pair<HFONT, HFONT> fonts(hfont_, hfontBold_);
	UINT nId = ID_HEADEREDIT_ITEM;
	return pLayout_->create(pThis_, fonts, &nId, 0);
}

void qm::HeaderEditWindowImpl::reloadProfiles(bool bInitialize)
{
	HFONT hfont = qs::UIUtil::createFontFromProfile(pProfile_,
		L"HeaderEditWindow", qs::UIUtil::DEFAULTFONT_UI);
	LOGFONT lf;
	::GetObject(hfont, sizeof(lf), &lf);
	lf.lfWeight = FW_BOLD;
	HFONT hfontBold = ::CreateFontIndirect(&lf);
	if (!bInitialize) {
		assert(hfont_);
		::DeleteObject(hfont_);
		assert(hfontBold_);
		::DeleteObject(hfontBold_);
		
		std::pair<HFONT, HFONT> fonts(hfont, hfontBold);
		pLayout_->setFont(fonts);
	}
	hfont_ = hfont;
	hfontBold_ = hfontBold;
}


/****************************************************************************
 *
 * HeaderEditWindow
 *
 */

qm::HeaderEditWindow::HeaderEditWindow(Profile* pProfile) :
	WindowBase(true),
	pImpl_(0)
{
	pImpl_ = new HeaderEditWindowImpl();
	pImpl_->pThis_ = this;
	pImpl_->pProfile_ = pProfile;
	pImpl_->hfont_ = 0;
	pImpl_->hfontBold_ = 0;
	pImpl_->hbrBackground_ = 0;
	pImpl_->pController_ = 0;
	
	pImpl_->reloadProfiles(true);
	
	setWindowHandler(this, false);
}

qm::HeaderEditWindow::~HeaderEditWindow()
{
	delete pImpl_;
}

void qm::HeaderEditWindow::setEditMessage(EditMessage* pEditMessage,
										  bool bReset)
{
	for (unsigned int n = 0; n < pImpl_->pLayout_->getLineCount(); ++n) {
		HeaderEditLine* pLine = static_cast<HeaderEditLine*>(
			pImpl_->pLayout_->getLine(n));
		pLine->setEditMessage(pEditMessage, bReset);
	}
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

void qm::HeaderEditWindow::updateEditMessage(EditMessage* pEditMessage)
{
	for (unsigned int n = 0; n < pImpl_->pLayout_->getLineCount(); ++n) {
		HeaderEditLine* pLine = static_cast<HeaderEditLine*>(
			pImpl_->pLayout_->getLine(n));
		for (unsigned int m = 0; m < pLine->getItemCount(); ++m) {
			HeaderEditItem* pItem = static_cast<HeaderEditItem*>(pLine->getItem(m));
			pItem->updateEditMessage(pEditMessage);
		}
	}
}

int qm::HeaderEditWindow::getHeight() const
{
	return pImpl_->pLayout_->getHeight();
}

void qm::HeaderEditWindow::layout(const RECT& rect)
{
	ClientDeviceContext dc(getHandle());
	ObjectSelector<HFONT> fontSelecter(dc, pImpl_->hfont_);
	TEXTMETRIC tm;
	dc.getTextMetrics(&tm);
	unsigned int nFontHeight = tm.tmHeight + tm.tmExternalLeading;
	pImpl_->pLayout_->layout(rect, nFontHeight);
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

EditWindowItem* qm::HeaderEditWindow::getNextFocusItem(EditWindowItem* pItem) const
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

EditWindowItem* qm::HeaderEditWindow::getPrevFocusItem(EditWindowItem* pItem) const
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

void qm::HeaderEditWindow::reloadProfiles()
{
	pImpl_->reloadProfiles(false);
}

void qm::HeaderEditWindow::getWindowClass(WNDCLASS* pwc)
{
	DefaultWindowHandler::getWindowClass(pwc);
	pwc->hbrBackground = reinterpret_cast<HBRUSH>(COLOR_3DFACE + 1);
}

LRESULT qm::HeaderEditWindow::windowProc(UINT uMsg,
										 WPARAM wParam,
										 LPARAM lParam)
{
	BEGIN_MESSAGE_HANDLER()
		HANDLE_CREATE()
		HANDLE_CTLCOLORSTATIC()
		HANDLE_DESTROY()
	END_MESSAGE_HANDLER()
	return DefaultWindowHandler::windowProc(uMsg, wParam, lParam);
}

LRESULT qm::HeaderEditWindow::onCreate(CREATESTRUCT* pCreateStruct)
{
	if (DefaultWindowHandler::onCreate(pCreateStruct) == -1)
		return -1;
	
	HeaderEditWindowCreateContext* pContext =
		static_cast<HeaderEditWindowCreateContext*>(pCreateStruct->lpCreateParams);
	pImpl_->pController_ = pContext->pController_;
	
	wstring_ptr wstrClassName(getClassName());
	W2T(wstrClassName.get(), ptszClassName);
	WNDCLASS wc;
	::GetClassInfo(getInstanceHandle(), ptszClassName, &wc);
	pImpl_->hbrBackground_ = wc.hbrBackground;
	
	if (!pImpl_->create(pContext->pwszClass_, pContext->pMenuManager_,
		pContext->pHeaderEditLineCallback_, pContext->pHeaderEditItemCallback_))
		return -1;
	
	return 0;
}

LRESULT qm::HeaderEditWindow::onCtlColorStatic(HDC hdc,
											   HWND hwnd)
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


/****************************************************************************
 *
 * HeaderEditLine
 *
 */

qm::HeaderEditLine::HeaderEditLine(HeaderEditLineCallback* pCallback,
								   unsigned int nFlags) :
	pCallback_(pCallback),
	nFlags_(nFlags),
	bHide_(false)
{
}

qm::HeaderEditLine::~HeaderEditLine()
{
}

void qm::HeaderEditLine::setEditMessage(EditMessage* pEditMessage,
										bool bReset)
{
	if (!bHide_) {
		for (unsigned int n = 0; n < getItemCount(); ++n) {
			HeaderEditItem* pItem = static_cast<HeaderEditItem*>(getItem(n));
			pItem->setEditMessage(pEditMessage, bReset);
		}
	}
}

EditWindowItem* qm::HeaderEditLine::getNextFocusItem(EditWindowItem** ppItem) const
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

EditWindowItem* qm::HeaderEditLine::getPrevFocusItem(EditWindowItem** ppItem) const
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

qm::HeaderEditItem::HeaderEditItem(EditWindowFocusController* pController) :
	pController_(pController),
	nNumber_(-1),
	initialFocus_(INITIALFOCUS_NONE)
{
}

qm::HeaderEditItem::~HeaderEditItem()
{
}

unsigned int qm::HeaderEditItem::getNumber() const
{
	return nNumber_;
}

void qm::HeaderEditItem::setNumber(unsigned int nNumber)
{
	nNumber_ = nNumber;
}

void qm::HeaderEditItem::setInitialFocus(bool bInitialFocus)
{
	initialFocus_ = bInitialFocus ? INITIALFOCUS_TRUE : INITIALFOCUS_FALSE;
}

void qm::HeaderEditItem::setValue(const WCHAR* pwszValue)
{
	wstrValue_ = allocWString(pwszValue);
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
	return wstrValue_.get();
}

void qm::HeaderEditItem::copy()
{
}

bool qm::HeaderEditItem::canCopy()
{
	return false;
}

void qm::HeaderEditItem::cut()
{
}

bool qm::HeaderEditItem::canCut()
{
	return false;
}

void qm::HeaderEditItem::paste()
{
}

bool qm::HeaderEditItem::canPaste()
{
	return false;
}

void qm::HeaderEditItem::selectAll()
{
}

bool qm::HeaderEditItem::canSelectAll()
{
	return false;
}

void qm::HeaderEditItem::undo()
{
}

bool qm::HeaderEditItem::canUndo()
{
	return false;
}

void qm::HeaderEditItem::redo()
{
}

bool qm::HeaderEditItem::canRedo()
{
	return false;
}


/****************************************************************************
 *
 * TextHeaderEditItem
 *
 */

qm::TextHeaderEditItem::TextHeaderEditItem(EditWindowFocusController* pController) :
	HeaderEditItem(pController),
	nStyle_(STYLE_NORMAL),
	align_(ALIGN_LEFT),
	type_(TYPE_UNSTRUCTURED),
	hwnd_(0)
{
}

qm::TextHeaderEditItem::~TextHeaderEditItem()
{
}

void qm::TextHeaderEditItem::setEditMessage(EditMessage* pEditMessage,
											bool bReset)
{
	const WCHAR* pwszValue = getValue();
	if (pwszValue) {
		Window(hwnd_).setWindowText(pwszValue);
	}
	else {
		wstring_ptr wstrValue(pEditMessage->getField(wstrField_.get(),
			static_cast<EditMessage::FieldType>(type_)));
		if (wstrValue.get())
			Window(hwnd_).setWindowText(wstrValue.get());
		else
			Window(hwnd_).setWindowText(L"");
	}
	
	if (!bReset)
		pEditMessage->addEditMessageHandler(this);
}

void qm::TextHeaderEditItem::releaseEditMessage(EditMessage* pEditMessage)
{
	pEditMessage->removeEditMessageHandler(this);
}

bool qm::TextHeaderEditItem::hasFocus() const
{
	return Window(hwnd_).hasFocus();
}

void qm::TextHeaderEditItem::setStyle(unsigned int nStyle)
{
	nStyle_ = nStyle;
}

void qm::TextHeaderEditItem::setAlign(Align align)
{
	align_ = align;
}

void qm::TextHeaderEditItem::setField(const WCHAR* pwszField)
{
	wstrField_ = allocWString(pwszField);
}

void qm::TextHeaderEditItem::setType(Type type)
{
	type_ = type;
}

bool qm::TextHeaderEditItem::create(WindowBase* pParent,
									const std::pair<HFONT, HFONT>& fonts,
									UINT nId,
									void* pParam)
{
	assert(!hwnd_);
	
	hwnd_ = ::CreateWindowEx(getWindowExStyle(),
		getWindowClassName(), 0, WS_CHILD | WS_VISIBLE | getWindowStyle(),
		0, 0, 0, 0, pParent->getHandle(), reinterpret_cast<HMENU>(nId),
		Init::getInit().getInstanceHandle(), 0);
	if (!hwnd_)
		return false;
	
	Window(hwnd_).setFont((nStyle_ & STYLE_BOLD) ? fonts.second : fonts.first);
	
	pItemWindow_.reset(new EditWindowItemWindow(getController(), this, hwnd_));
	
	return true;
}

void qm::TextHeaderEditItem::destroy()
{
}

HDWP qm::TextHeaderEditItem::layout(HDWP hdwp,
									const RECT& rect,
									unsigned int nFontHeight)
{
	unsigned int nHeight = getHeight(rect.right - rect.left, nFontHeight);
	unsigned int nFlags = SWP_NOZORDER | SWP_NOACTIVATE;
#ifndef _WIN32_WCE
	nFlags |= SWP_NOCOPYBITS;
#endif
	hdwp = Window(hwnd_).deferWindowPos(hdwp, 0, rect.left,
		rect.top + ((rect.bottom - rect.top) - nHeight)/2,
		rect.right - rect.left, nHeight, nFlags);
#ifdef _WIN32_WCE
	Window(hwnd_).invalidate();
#endif
	return hdwp;
}

void qm::TextHeaderEditItem::show(bool bShow)
{
	Window(hwnd_).showWindow(bShow ? SW_SHOW : SW_HIDE);
}

void qm::TextHeaderEditItem::setFont(const std::pair<HFONT, HFONT>& fonts)
{
	Window(hwnd_).setFont((nStyle_ & STYLE_BOLD) ? fonts.second : fonts.first);
}

void qm::TextHeaderEditItem::fieldChanged(const EditMessageFieldEvent& event)
{
	if (!getValue() && _wcsicmp(event.getName(), wstrField_.get()) == 0)
		Window(hwnd_).setWindowText(event.getValue());
}

HWND qm::TextHeaderEditItem::getHandle() const
{
	return hwnd_;
}

TextHeaderEditItem::Align qm::TextHeaderEditItem::getAlign() const
{
	return align_;
}

const WCHAR* qm::TextHeaderEditItem::getField() const
{
	return wstrField_.get();
}

TextHeaderEditItem::Type qm::TextHeaderEditItem::getType() const
{
	return type_;
}

void qm::TextHeaderEditItem::setFocus()
{
	Window(hwnd_).setFocus();
}

unsigned int qm::TextHeaderEditItem::parseStyle(const WCHAR* pwszStyle)
{
	unsigned int nStyle = 0;
	
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
				nStyle |= styles[n].style_;
				break;
			}
		}
		p = pEnd ? pEnd + 1 : 0;
	}
	
	return nStyle;
}

TextHeaderEditItem::Align qm::TextHeaderEditItem::parseAlign(const WCHAR* pwszAlign)
{
	if (wcscmp(pwszAlign, L"right") == 0)
		return ALIGN_RIGHT;
	else if (wcscmp(pwszAlign, L"center") == 0)
		return ALIGN_CENTER;
	else
		return ALIGN_LEFT;
}

TextHeaderEditItem::Type qm::TextHeaderEditItem::parseType(const WCHAR* pwszType)
{
	struct {
		const WCHAR* pwszName_;
		Type type_;
	} types[] = {
		{ L"unstructured",	TYPE_UNSTRUCTURED	},
		{ L"addressList",	TYPE_ADDRESSLIST	},
		{ L"references",	TYPE_REFERENCES		}
	};
	for (int n = 0; n < countof(types); ++n) {
		if (wcscmp(pwszType, types[n].pwszName_) == 0)
			return types[n].type_;
	}
	
	return TYPE_UNSTRUCTURED;
}


/****************************************************************************
 *
 * StaticHeaderEditItem
 *
 */

qm::StaticHeaderEditItem::StaticHeaderEditItem(EditWindowFocusController* pController) :
	TextHeaderEditItem(pController)
{
}

qm::StaticHeaderEditItem::~StaticHeaderEditItem()
{
}

void qm::StaticHeaderEditItem::updateEditMessage(EditMessage* pEditMessage)
{
}

bool qm::StaticHeaderEditItem::hasInitialFocus() const
{
	return false;
}

bool qm::StaticHeaderEditItem::isFocusItem() const
{
	return false;
}

unsigned int qm::StaticHeaderEditItem::getHeight(unsigned int nWidth,
												 unsigned int nFontHeight) const
{
	return nFontHeight;
}

unsigned int qm::StaticHeaderEditItem::getPreferredWidth() const
{
	return UIUtil::getPreferredWidth(getHandle(), !getValue());
}

const TCHAR* qm::StaticHeaderEditItem::getWindowClassName() const
{
	return _T("STATIC");
}

UINT qm::StaticHeaderEditItem::getWindowStyle() const
{
	UINT nStyle = getValue() ? 0 : SS_NOPREFIX;
#ifndef _WIN32_WCE
	nStyle |= SS_ENDELLIPSIS;
#endif
	switch (getAlign()) {
	case ALIGN_LEFT:
		nStyle |= SS_LEFT;
		break;
	case ALIGN_CENTER:
		nStyle |= SS_CENTER;
		break;
	case ALIGN_RIGHT:
		nStyle |= SS_RIGHT;
		break;
	default:
		assert(false);
		break;
	}
	return nStyle;
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

qm::EditHeaderEditItem::EditHeaderEditItem(EditWindowFocusController* pController,
										   Profile* pProfile) :
	TextHeaderEditItem(pController),
	pProfile_(pProfile),
	pEditMessage_(0),
	pParent_(0),
	nId_(0)
{
}

qm::EditHeaderEditItem::~EditHeaderEditItem()
{
}

void qm::EditHeaderEditItem::setEditMessage(EditMessage* pEditMessage,
											bool bReset)
{
	pEditMessage_ = pEditMessage;
	TextHeaderEditItem::setEditMessage(pEditMessage, bReset);
}

void qm::EditHeaderEditItem::updateEditMessage(EditMessage* pEditMessage)
{
	if (!getValue()) {
		wstring_ptr wstrText(Window(getHandle()).getWindowText());
		pEditMessage->setField(getField(), wstrText.get(),
			static_cast<EditMessage::FieldType>(getType()));
	}
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

unsigned int qm::EditHeaderEditItem::getHeight(unsigned int nWidth,
											   unsigned int nFontHeight) const
{
	return nFontHeight + 7;
}

bool qm::EditHeaderEditItem::create(qs::WindowBase* pParent,
									const std::pair<HFONT, HFONT>& fonts,
									UINT nId,
									void* pParam)
{
	if (!TextHeaderEditItem::create(pParent, fonts, nId, pParam))
		return false;
	pParent->addCommandHandler(this);
	
	pParent_ = pParent;
	nId_ = nId;
	
	const WCHAR* pwszField = getField();
	if (pwszField) {
		pImeWindow_.reset(new ImeWindow(pProfile_, L"HeaderEditWindow", pwszField, false));
		pImeWindow_->subclassWindow(getHandle());
	}
	
	return true;
}

void qm::EditHeaderEditItem::destroy()
{
	if (pParent_)
		pParent_->removeCommandHandler(this);
	TextHeaderEditItem::destroy();
}

const TCHAR* qm::EditHeaderEditItem::getWindowClassName() const
{
	return _T("EDIT");
}

UINT qm::EditHeaderEditItem::getWindowStyle() const
{
	UINT nStyle = ES_AUTOHSCROLL;
#if defined _WIN32_WCE && _WIN32_WCE >= 300 && defined _WIN32_WCE_PSPC
	nStyle |= WS_BORDER;
#endif
	switch (getAlign()) {
	case ALIGN_LEFT:
		nStyle |= ES_LEFT;
		break;
	case ALIGN_CENTER:
		nStyle |= ES_CENTER;
		break;
	case ALIGN_RIGHT:
		nStyle |= ES_RIGHT;
		break;
	default:
		assert(false);
		break;
	}
	return nStyle;
}

UINT qm::EditHeaderEditItem::getWindowExStyle() const
{
#if defined _WIN32_WCE && _WIN32_WCE >=300 && defined _WIN32_WCE_PSPC
	return 0;
#else
	return WS_EX_CLIENTEDGE;
#endif
}

void qm::EditHeaderEditItem::copy()
{
	Window(getHandle()).sendMessage(WM_COPY);
}

bool qm::EditHeaderEditItem::canCopy()
{
	DWORD dwStart = 0;
	DWORD dwEnd = 0;
	Window(getHandle()).sendMessage(EM_GETSEL,
		reinterpret_cast<WPARAM>(&dwStart),
		reinterpret_cast<LPARAM>(&dwEnd));
	return dwStart != dwEnd;
}

void qm::EditHeaderEditItem::cut()
{
	Window(getHandle()).sendMessage(WM_CUT);
}

bool qm::EditHeaderEditItem::canCut()
{
	DWORD dwStart = 0;
	DWORD dwEnd = 0;
	Window(getHandle()).sendMessage(EM_GETSEL,
		reinterpret_cast<WPARAM>(&dwStart),
		reinterpret_cast<LPARAM>(&dwEnd));
	return dwStart != dwEnd;
}

void qm::EditHeaderEditItem::paste()
{
	Window(getHandle()).sendMessage(WM_PASTE);
}

bool qm::EditHeaderEditItem::canPaste()
{
	return Clipboard::isFormatAvailable(Clipboard::CF_QSTEXT);
}

void qm::EditHeaderEditItem::selectAll()
{
	Window(getHandle()).sendMessage(EM_SETSEL, 0, -1);
}

bool qm::EditHeaderEditItem::canSelectAll()
{
	return true;
}

void qm::EditHeaderEditItem::undo()
{
	Window(getHandle()).sendMessage(EM_UNDO);
}

bool qm::EditHeaderEditItem::canUndo()
{
	return Window(getHandle()).sendMessage(EM_CANUNDO) != 0;
}

LRESULT qm::EditHeaderEditItem::onCommand(WORD nCode,
										  WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID_CODE(nId_, EN_KILLFOCUS, onKillFocus)
	END_COMMAND_HANDLER()
	return CommandHandler::onCommand(nCode, nId);
}

LRESULT qm::EditHeaderEditItem::onKillFocus()
{
	if (pEditMessage_)
		updateEditMessage(pEditMessage_);
	return 0;
}


/****************************************************************************
 *
 * AddressHeaderEditItem
 *
 */

qm::AddressHeaderEditItem::AddressHeaderEditItem(EditWindowFocusController* pController,
												 Profile* pProfile) :
	EditHeaderEditItem(pController, pProfile),
	nFlags_(FLAG_EXPANDALIAS | FLAG_AUTOCOMPLETE),
	pAddressBook_(0),
	pRecentAddress_(0)
{
	setType(TYPE_ADDRESSLIST);
}

qm::AddressHeaderEditItem::~AddressHeaderEditItem()
{
}

void qm::AddressHeaderEditItem::setExpandAlias(bool bExpandAlias)
{
	if (bExpandAlias)
		nFlags_ |= FLAG_EXPANDALIAS;
	else
		nFlags_ &= ~FLAG_EXPANDALIAS;
}

void qm::AddressHeaderEditItem::setAutoComplete(bool bAutoComplete)
{
	if (bAutoComplete)
		nFlags_ |= FLAG_AUTOCOMPLETE;
	else
		nFlags_ &= ~FLAG_AUTOCOMPLETE;
}

void qm::AddressHeaderEditItem::setEditMessage(EditMessage* pEditMessage,
											   bool bReset)
{
	Document* pDocument = pEditMessage->getDocument();
	pAddressBook_ = pDocument->getAddressBook();
	pRecentAddress_ = pDocument->getRecentAddress();
	EditHeaderEditItem::setEditMessage(pEditMessage, bReset);
}

void qm::AddressHeaderEditItem::releaseEditMessage(EditMessage* pEditMessage)
{
	pAddressBook_ = 0;
	pRecentAddress_ = 0;
	EditHeaderEditItem::releaseEditMessage(pEditMessage);
}

bool qm::AddressHeaderEditItem::create(qs::WindowBase* pParent,
									   const std::pair<HFONT, HFONT>& fonts,
									   UINT nId,
									   void* pParam)
{
	if (!EditHeaderEditItem::create(pParent, fonts, nId, pParam))
		return false;
	
	if (nFlags_ & FLAG_AUTOCOMPLETE)
		pAutoComplete_.reset(new AutoComplete(getHandle(), pParent, this));
	
	return true;
}

void qm::AddressHeaderEditItem::destroy()
{
	pAutoComplete_.reset(0);
	EditHeaderEditItem::destroy();
}

LRESULT qm::AddressHeaderEditItem::onKillFocus()
{
	if (nFlags_ & FLAG_EXPANDALIAS && pAddressBook_) {
		Window wnd(getHandle());
		wstring_ptr wstrText(wnd.getWindowText());
		if (*wstrText.get()) {
			wstring_ptr wstr(pAddressBook_->expandAlias(wstrText.get()));
			if (wstr.get())
				wnd.setWindowText(wstr.get());
		}
	}
	return EditHeaderEditItem::onKillFocus();
}

std::pair<size_t, size_t> qm::AddressHeaderEditItem::getInput(const WCHAR* pwszText,
															  size_t nCaret)
{
	const WCHAR* pBegin = pwszText + nCaret;
	if (nCaret != 0) {
		while (true) {
			--pBegin;
			if (pBegin == pwszText) {
				break;
			}
			else if (*pBegin == L',') {
				++pBegin;
				while (*pBegin == L' ')
					++pBegin;
				break;
			}
		}
	}
	
	const WCHAR* pEnd = pwszText + nCaret;
	while (*pEnd) {
		if (*pEnd == L',')
			break;
		++pEnd;
	}
	
	return std::pair<size_t, size_t>(pBegin - pwszText, pEnd - pBegin);
}

void qm::AddressHeaderEditItem::getCandidates(const WCHAR* pwszInput,
											  CandidateList* pList)
{
	getCandidates(pwszInput, pAddressBook_, pList);
	getCandidates(pwszInput, pRecentAddress_, pAddressBook_, pList);
	std::sort(pList->begin(), pList->end(), string_less_i<WCHAR>());
}

void qm::AddressHeaderEditItem::getCandidates(const WCHAR* pwszInput,
											  const AddressBook* pAddressBook,
											  CandidateList* pList)
{
	const AddressBook::EntryList& listEntry = pAddressBook->getEntries();
	for (AddressBook::EntryList::const_iterator it = listEntry.begin(); it != listEntry.end(); ++it)
		getCandidates(pwszInput, *it, pList);
}

void qm::AddressHeaderEditItem::getCandidates(const WCHAR* pwszInput,
											  const AddressBookEntry* pEntry,
											  CandidateList* pList)
{
	size_t nLen = wcslen(pwszInput);
	bool bMatchName = matchName(pEntry->getName(), pwszInput, nLen);
	
	const AddressBookEntry::AddressList& listAddress = pEntry->getAddresses();
	for (AddressBookEntry::AddressList::const_iterator it = listAddress.begin(); it != listAddress.end(); ++it) {
		const AddressBookAddress* pAddress = *it;
		if (bMatchName || match(pwszInput, nLen, pAddress)) {
			wstring_ptr wstrValue(pAddress->getValue());
			pList->push_back(wstrValue.get());
			wstrValue.release();
		}
	}
}

void qm::AddressHeaderEditItem::getCandidates(const WCHAR* pwszInput,
											  const RecentAddress* pRecentAddress,
											  const AddressBook* pAddressBook,
											  CandidateList* pList)
{
	size_t nLen = wcslen(pwszInput);
	
	const RecentAddress::AddressList& l = pRecentAddress->getAddresses();
	for (RecentAddress::AddressList::const_iterator it = l.begin(); it != l.end(); ++it) {
		const AddressParser* pAddress = *it;
		if (match(pwszInput, nLen, *pAddress) &&
			!pAddressBook->getEntry(pAddress->getAddress().get())) {
			wstring_ptr wstrValue(pAddress->getValue());
			pList->push_back(wstrValue.get());
			wstrValue.release();
		}
	}
}

bool qm::AddressHeaderEditItem::match(const WCHAR* pwszInput,
									  size_t nLen,
									  const AddressBookAddress* pAddress)
{
	if (pAddress->isRFC2822()) {
		AddressListParser addressList;
		if (MessageCreator::getAddressList(pAddress->getAddress(), &addressList)) {
			if (match(pwszInput, nLen, addressList))
				return true;
		}
	}
	else {
		if (matchAddress(pAddress->getAddress(), pwszInput, nLen))
			return true;
	}
	return false;
}

bool qm::AddressHeaderEditItem::match(const WCHAR* pwszInput,
									  size_t nLen,
									  const AddressListParser& addressList)
{
	const AddressListParser::AddressList& addresses = addressList.getAddressList();
	for (AddressListParser::AddressList::const_iterator it = addresses.begin(); it != addresses.end(); ++it) {
		if (match(pwszInput, nLen, **it))
			return true;
	}
	return false;
}

bool qm::AddressHeaderEditItem::match(const WCHAR* pwszInput,
									  size_t nLen,
									  const AddressParser& address)
{
	const AddressListParser* pGroup = address.getGroup();
	if (pGroup) {
		return match(pwszInput, nLen, *pGroup);
	}
	else {
		const WCHAR* pwszPhrase = address.getPhrase();
		bool bMatchName = matchName(pwszPhrase, pwszInput, nLen);
		wstring_ptr wstrAddress(address.getAddress());
		bool bMatchAddress = matchAddress(wstrAddress.get(), pwszInput, nLen);
		return (bMatchName || bMatchAddress) &&
			((pwszPhrase && *pwszPhrase) || !bMatchAddress || wcslen(wstrAddress.get()) != nLen);
	}
}

bool qm::AddressHeaderEditItem::matchName(const WCHAR* pwszName,
										  const WCHAR* pwszInput,
										  size_t nInputLen)
{
	if (!pwszName)
		return false;
	
	if (_wcsnicmp(pwszName, pwszInput, nInputLen) == 0)
		return true;
	
	const WCHAR* p = wcschr(pwszName, L' ');
	while (p) {
		if (_wcsnicmp(p + 1, pwszInput, nInputLen) == 0)
			return true;
		p = wcschr(p + 1, L' ');
	}
	return false;
}

bool qm::AddressHeaderEditItem::matchAddress(const WCHAR* pwszAddress,
											 const WCHAR* pwszInput,
											 size_t nInputLen)
{
	if (!pwszAddress)
		return false;
	
	if (_wcsnicmp(pwszAddress, pwszInput, nInputLen) == 0)
		return true;
	
	const WCHAR* p = wcschr(pwszAddress, L'@');
	if (p) {
		if (_wcsnicmp(p + 1, pwszInput, nInputLen) == 0)
			return true;
	}
	return false;
}


/****************************************************************************
 *
 * AttachmentHeaderEditItem
 *
 */

qm::AttachmentHeaderEditItem::AttachmentHeaderEditItem(EditWindowFocusController* pController,
													   MenuManager* pMenuManager,
													   HeaderEditItemCallback* pCallback) :
	HeaderEditItem(pController),
	wnd_(this),
	pMenuManager_(pMenuManager),
	pCallback_(pCallback),
	pEditMessage_(0)
{
}

qm::AttachmentHeaderEditItem::~AttachmentHeaderEditItem()
{
}

void qm::AttachmentHeaderEditItem::setEditMessage(EditMessage* pEditMessage,
												  bool bReset)
{
	update(pEditMessage);
	
	if (!bReset)
		pEditMessage->addEditMessageHandler(this);
	pEditMessage_ = pEditMessage;
}

void qm::AttachmentHeaderEditItem::releaseEditMessage(EditMessage* pEditMessage)
{
	pEditMessage->removeEditMessageHandler(this);
	pEditMessage_ = 0;
}

void qm::AttachmentHeaderEditItem::updateEditMessage(EditMessage* pEditMessage)
{
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

unsigned int qm::AttachmentHeaderEditItem::getHeight(unsigned int nWidth,
													 unsigned int nFontHeight) const
{
	int nCount = ListView_GetItemCount(wnd_.getHandle());
	unsigned int nHeight = QSMAX(nFontHeight,
		static_cast<unsigned int>(::GetSystemMetrics(SM_CYSMICON))) + 1;
//	return QSMAX(nFontHeight + 7, QSMIN(nCount*nHeight + 4, nHeight*4 + 7));
	return QSMIN(nCount*nHeight + 4, nHeight*4 + 7);
}

bool qm::AttachmentHeaderEditItem::create(WindowBase* pParent,
										  const std::pair<HFONT, HFONT>& fonts,
										  UINT nId,
										  void* pParam)
{
	assert(!wnd_.getHandle());
	
	DWORD dwStyle = WS_CHILD | WS_VISIBLE | LVS_REPORT |
		LVS_SHAREIMAGELISTS | LVS_NOCOLUMNHEADER;
#if defined _WIN32_WCE && _WIN32_WCE >= 300 && defined _WIN32_WCE_PSPC
	dwStyle |= WS_BORDER;
#endif
#if defined _WIN32_WCE && _WIN32_WCE >= 300 && defined _WIN32_WCE_PSPC
	DWORD dwExStyle = 0;
#else
	DWORD dwExStyle = WS_EX_CLIENTEDGE;
#endif
	if (!wnd_.create(L"QmAttachmentWindow", 0, dwStyle, 0, 0, 0, 0,
		pParent->getHandle(), dwExStyle, WC_LISTVIEWW, nId, 0))
		return false;
	
	SHFILEINFO info = { 0 };
	HIMAGELIST hImageList = reinterpret_cast<HIMAGELIST>(::SHGetFileInfo(
		_T("dummy.txt"), FILE_ATTRIBUTE_NORMAL, &info, sizeof(info),
		SHGFI_USEFILEATTRIBUTES | SHGFI_SYSICONINDEX | SHGFI_SMALLICON));
	ListView_SetImageList(wnd_.getHandle(), hImageList, LVSIL_SMALL);
	
	LVCOLUMN column = {
		LVCF_FMT,
		LVCFMT_LEFT | LVCFMT_IMAGE,
	};
	ListView_InsertColumn(wnd_.getHandle(), 0, &column);
	
	wnd_.setFont(fonts.first);
	
	pItemWindow_.reset(new EditWindowItemWindow(getController(), this, wnd_.getHandle()));
	
	return true;
}

void qm::AttachmentHeaderEditItem::destroy()
{
	clear();
}

HDWP qm::AttachmentHeaderEditItem::layout(HDWP hdwp,
										  const RECT& rect,
										  unsigned int nFontHeight)
{
	unsigned int nFlags = SWP_NOZORDER | SWP_NOACTIVATE;
	return wnd_.deferWindowPos(hdwp, 0, rect.left, rect.top,
		rect.right - rect.left, rect.bottom - rect.top, nFlags);
}

void qm::AttachmentHeaderEditItem::show(bool bShow)
{
	wnd_.showWindow(bShow ? SW_SHOW : SW_HIDE);
}

void qm::AttachmentHeaderEditItem::setFont(const std::pair<HFONT, HFONT>& fonts)
{
	wnd_.setFont(fonts.first);
}

void qm::AttachmentHeaderEditItem::setFocus()
{
	wnd_.setFocus();
}

void qm::AttachmentHeaderEditItem::paste()
{
#ifdef _WIN32_WCE
	ComPtr<IDataObject> pDataObject(MessageDataObject::getClipboard(pEditMessage_->getDocument()));
	if (!pDataObject.get())
		return;
#else
	ComPtr<IDataObject> pDataObject;
	HRESULT hr = ::OleGetClipboard(&pDataObject);
	if (hr != S_OK)
		return;
#endif
	Util::PathList listPath;
	StringListFree<Util::PathList> free(listPath);
	Util::getFilesOrURIs(pDataObject.get(), &listPath);
	for (Util::PathList::const_iterator it = listPath.begin(); it != listPath.end(); ++it)
		pEditMessage_->addAttachment(*it);
}

bool qm::AttachmentHeaderEditItem::canPaste()
{
#ifdef _WIN32_WCE
	ComPtr<IDataObject> pDataObject(MessageDataObject::getClipboard(pEditMessage_->getDocument()));
	if (!pDataObject.get())
		return false;
#else
	ComPtr<IDataObject> pDataObject;
	HRESULT hr = ::OleGetClipboard(&pDataObject);
	if (hr != S_OK)
		return false;
#endif
	return Util::hasFilesOrURIs(pDataObject.get());
}

void qm::AttachmentHeaderEditItem::selectAll()
{
	HWND hwnd = wnd_.getHandle();
	for (int n = 0; n < ListView_GetItemCount(hwnd); ++n)
		ListView_SetItemState(hwnd, n, LVIS_SELECTED, LVIS_SELECTED);
}

bool qm::AttachmentHeaderEditItem::canSelectAll()
{
	return true;
}

void qm::AttachmentHeaderEditItem::attachmentsChanged(const EditMessageEvent& event)
{
	update(event.getEditMessage());
}

bool qm::AttachmentHeaderEditItem::hasAttachment()
{
	return ListView_GetItemCount(wnd_.getHandle()) != 0;
}

bool qm::AttachmentHeaderEditItem::hasSelectedAttachment()
{
	return ListView_GetSelectedCount(wnd_.getHandle()) != 0;
}

void qm::AttachmentHeaderEditItem::getSelectedAttachment(NameList* pList)
{
	assert(pList);
	
	HWND hwnd = wnd_.getHandle();
	
	int nItem = ListView_GetNextItem(hwnd, -1, LVNI_ALL | LVNI_SELECTED);
	while (nItem != -1) {
		LVITEM item = {
			LVIF_PARAM,
			nItem
		};
		ListView_GetItem(hwnd, &item);
		wstring_ptr wstrName(allocWString(reinterpret_cast<WSTRING>(item.lParam)));
		pList->push_back(wstrName.get());
		wstrName.release();
		nItem = ListView_GetNextItem(hwnd, nItem, LVNI_ALL | LVNI_SELECTED);
	}
}

bool qm::AttachmentHeaderEditItem::isAttachmentDeleted()
{
	return false;
}

void qm::AttachmentHeaderEditItem::update(EditMessage* pEditMessage)
{
	HWND hwnd = wnd_.getHandle();
	
	clear();
	
	EditMessage::AttachmentList l;
	EditMessage::AttachmentListFree free(l);
	pEditMessage->getAttachments(&l);
	for (EditMessage::AttachmentList::size_type n = 0; n < l.size(); ++n) {
		EditMessage::Attachment& attachment = l[n];
		
		wstring_ptr wstrName;
		int nIcon = 0;
		UIUtil::getAttachmentInfo(attachment, &wstrName, &nIcon);
		
		wstring_ptr wstrPath(attachment.wstrName_);
		attachment.wstrName_ = 0;
		
		W2T(wstrName.get(), ptszName);
		LVITEM item = {
			LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM,
			static_cast<int>(n),
			0,
			0,
			0,
			const_cast<LPTSTR>(ptszName),
			0,
			nIcon,
			reinterpret_cast<LPARAM>(wstrPath.get())
		};
		ListView_InsertItem(hwnd, &item);
		wstrPath.release();
	}
	
	pCallback_->itemSizeChanged();
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

qm::AttachmentHeaderEditItem::AttachmentEditWindow::AttachmentEditWindow(AttachmentHeaderEditItem* pItem) :
	WindowBase(false),
	pItem_(pItem)
{
	setWindowHandler(this, false);
}

qm::AttachmentHeaderEditItem::AttachmentEditWindow::~AttachmentEditWindow()
{
}

LRESULT qm::AttachmentHeaderEditItem::AttachmentEditWindow::windowProc(UINT uMsg,
																	   WPARAM wParam,
																	   LPARAM lParam)
{
	BEGIN_MESSAGE_HANDLER()
		HANDLE_CONTEXTMENU()
		HANDLE_LBUTTONDOWN()
		HANDLE_SIZE()
	END_MESSAGE_HANDLER()
	return DefaultWindowHandler::windowProc(uMsg, wParam, lParam);
}

LRESULT qm::AttachmentHeaderEditItem::AttachmentEditWindow::onContextMenu(HWND hwnd,
																		  const POINT& pt)
{
	HMENU hmenu = pItem_->pMenuManager_->getMenu(L"attachmentedit", false, false);
	if (hmenu) {
		UINT nFlags = TPM_LEFTALIGN | TPM_TOPALIGN;
#ifndef _WIN32_WCE
		nFlags |= TPM_LEFTBUTTON | TPM_RIGHTBUTTON;
#endif
		::TrackPopupMenu(hmenu, nFlags, pt.x, pt.y, 0, getParentFrame(), 0);
	}
	return 0;
}

LRESULT qm::AttachmentHeaderEditItem::AttachmentEditWindow::onLButtonDown(UINT nFlags,
																		  const POINT& pt)
{
#if defined _WIN32_WCE && _WIN32_WCE >= 300 && _WIN32_WCE < 400 && defined _WIN32_WCE_PSPC
	if (tapAndHold(pt))
		return 0;
#endif
	return DefaultWindowHandler::onLButtonDown(nFlags, pt);
}

LRESULT qm::AttachmentHeaderEditItem::AttachmentEditWindow::onSize(UINT nFlags,
																   int cx,
																   int cy)
{
	RECT rect;
	getWindowRect(&rect);
	int nWidth = rect.right - rect.left - ::GetSystemMetrics(SM_CXVSCROLL) - 9;
	ListView_SetColumnWidth(getHandle(), 0, nWidth);
	return DefaultWindowHandler::onSize(nFlags, cx, cy);
}


/****************************************************************************
 *
 * ComboBoxHeaderEditItem
 *
 */

qm::ComboBoxHeaderEditItem::ComboBoxHeaderEditItem(EditWindowFocusController* pController) :
	HeaderEditItem(pController),
	hwnd_(0),
	pParent_(0),
	nId_(0)
{
#ifdef _WIN32_WCE
	pComboBoxEditWindow_ = 0;
#endif
}

qm::ComboBoxHeaderEditItem::~ComboBoxHeaderEditItem()
{
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

unsigned int qm::ComboBoxHeaderEditItem::getHeight(unsigned int nWidth,
												   unsigned int nFontHeight) const
{
	RECT rect;
	Window(hwnd_).getWindowRect(&rect);
	return rect.bottom - rect.top + 1;
}

bool qm::ComboBoxHeaderEditItem::create(WindowBase* pParent,
										const std::pair<HFONT, HFONT>& fonts,
										UINT nId,
										void* pParam)
{
	assert(!hwnd_);
	
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
		return false;
	
	Window(hwnd_).setFont(fonts.first);
	
#ifdef _WIN32_WCE
	pComboBoxEditWindow_ = new ComboBoxEditWindow(hwnd_, calcItemHeight(fonts.first));
#endif
	
	pItemWindow_.reset(new EditWindowItemWindow(getController(), this, hwnd_));
	
	pParent->addCommandHandler(this);
	
	pParent_ = pParent;
	nId_ = nId;
	
	return true;
}

void qm::ComboBoxHeaderEditItem::destroy()
{
	if (pParent_)
		pParent_->removeCommandHandler(this);
}

HDWP qm::ComboBoxHeaderEditItem::layout(HDWP hdwp,
										const RECT& rect,
										unsigned int nFontHeight)
{
	unsigned int nFlags = SWP_NOZORDER | SWP_NOACTIVATE;
	return Window(hwnd_).deferWindowPos(hdwp, 0,
		rect.left, rect.top, rect.right - rect.left,
		static_cast<int>(100*(qs::UIUtil::getLogPixel()/96.0)), nFlags);
}

void qm::ComboBoxHeaderEditItem::show(bool bShow)
{
	Window(hwnd_).showWindow(bShow ? SW_SHOW : SW_HIDE);
}

void qm::ComboBoxHeaderEditItem::setFont(const std::pair<HFONT, HFONT>& fonts)
{
	Window(hwnd_).setFont(fonts.first);
#ifdef _WIN32_WCE
	pComboBoxEditWindow_->setItemHeight(calcItemHeight(fonts.first));
#endif
}

void qm::ComboBoxHeaderEditItem::setFocus()
{
	Window(hwnd_).setFocus();
}

LRESULT qm::ComboBoxHeaderEditItem::onCommand(WORD nCode,
											  WORD nId)
{
	BEGIN_COMMAND_HANDLER()
		HANDLE_COMMAND_ID_CODE(nId_, CBN_SELCHANGE, onChange)
	END_COMMAND_HANDLER()
	return CommandHandler::onCommand(nCode, nId);
}

LRESULT qm::ComboBoxHeaderEditItem::onChange()
{
	return 0;
}

HWND qm::ComboBoxHeaderEditItem::getHandle() const
{
	return hwnd_;
}

#ifdef _WIN32_WCE
int qm::ComboBoxHeaderEditItem::calcItemHeight(HFONT hfont)
{
	ClientDeviceContext dc(hwnd_);
	ObjectSelector<HFONT> selector(dc, hfont);
	TEXTMETRIC tm;
	dc.getTextMetrics(&tm);
	return tm.tmHeight + tm.tmExternalLeading + 2;
}
#endif

#ifdef _WIN32_WCE
/****************************************************************************
 *
 * ComboBoxHeaderEditItem::ComboBoxEditWindow
 *
 */

qm::ComboBoxHeaderEditItem::ComboBoxEditWindow::ComboBoxEditWindow(HWND hwnd,
																   int nItemHeight) :
	WindowBase(true),
	nItemHeight_(nItemHeight)
{
	setWindowHandler(this, false);
	subclassWindow(hwnd);
}

qm::ComboBoxHeaderEditItem::ComboBoxEditWindow::~ComboBoxEditWindow()
{
}

void qm::ComboBoxHeaderEditItem::ComboBoxEditWindow::setItemHeight(int nItemHeight)
{
	nItemHeight_ = nItemHeight;
}

LRESULT qm::ComboBoxHeaderEditItem::ComboBoxEditWindow::windowProc(UINT uMsg,
																   WPARAM wParam,
																   LPARAM lParam)
{
	BEGIN_MESSAGE_HANDLER()
		HANDLE_WINDOWPOSCHANGED()
	END_MESSAGE_HANDLER()
	return DefaultWindowHandler::windowProc(uMsg, wParam, lParam);
}

LRESULT qm::ComboBoxHeaderEditItem::ComboBoxEditWindow::onWindowPosChanged(WINDOWPOS* pWindowPos)
{
	if (sendMessage(CB_GETITEMHEIGHT, -1) != nItemHeight_)
		sendMessage(CB_SETITEMHEIGHT, -1, nItemHeight_);
	return DefaultWindowHandler::onWindowPosChanged(pWindowPos);
}
#endif // _WIN32_WCE


/****************************************************************************
 *
 * SignatureHeaderEditItem
 *
 */

qm::SignatureHeaderEditItem::SignatureHeaderEditItem(EditWindowFocusController* pController) :
	ComboBoxHeaderEditItem(pController),
	pEditMessage_(0)
{
}

qm::SignatureHeaderEditItem::~SignatureHeaderEditItem()
{
}

void qm::SignatureHeaderEditItem::setEditMessage(EditMessage* pEditMessage,
												 bool bReset)
{
	update(pEditMessage);
	
	if (!bReset)
		pEditMessage->addEditMessageHandler(this);
	pEditMessage_ = pEditMessage;
}

void qm::SignatureHeaderEditItem::releaseEditMessage(EditMessage* pEditMessage)
{
	pEditMessage->removeEditMessageHandler(this);
	pEditMessage_ = 0;
}

void qm::SignatureHeaderEditItem::updateEditMessage(EditMessage* pEditMessage)
{
	int nItem = ComboBox_GetCurSel(getHandle());
	int nLen = ComboBox_GetLBTextLen(getHandle(), nItem);
	tstring_ptr tstrName(allocTString(nLen + 1));
	ComboBox_GetLBText(getHandle(), nItem, tstrName.get());
	T2W(tstrName.get(), pwszName);
	pEditMessage->setSignature(pwszName);
}

void qm::SignatureHeaderEditItem::accountChanged(const EditMessageEvent& event)
{
	update(event.getEditMessage());
}

void qm::SignatureHeaderEditItem::signatureChanged(const EditMessageEvent& event)
{
	EditMessage* pEditMessage = event.getEditMessage();
	const WCHAR* pwszName = pEditMessage->getSignature();
	if (pwszName) {
		W2T(pwszName, ptszName);
		ComboBox_SelectString(getHandle(), -1, ptszName);
	}
	else {
		ComboBox_SetCurSel(getHandle(), 0);
	}
}

LRESULT qm::SignatureHeaderEditItem::onChange()
{
	if (pEditMessage_)
		updateEditMessage(pEditMessage_);
	return 0;
}

void qm::SignatureHeaderEditItem::update(EditMessage* pEditMessage)
{
	const WCHAR* pwszSignature = pEditMessage->getSignature();
	
	SignatureManager* pSignatureManager = pEditMessage->getDocument()->getSignatureManager();
	SignatureManager::SignatureList l;
	pSignatureManager->getSignatures(pEditMessage->getAccount(), &l);
	
	ComboBox_ResetContent(getHandle());
	
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	wstring_ptr wstrNone(loadString(hInst, IDS_SIGNATURE_NONE));
	W2T(wstrNone.get(), ptszNone);
	ComboBox_AddString(getHandle(), ptszNone);
	
	int nSelect = 0;
	int nDefault = -1;
	for (SignatureManager::SignatureList::iterator it = l.begin(); it != l.end(); ++it) {
		const Signature* pSignature = *it;
		const WCHAR* pwszName = pSignature->getName();
		W2T(pwszName, ptszName);
		int nItem = ComboBox_AddString(getHandle(), ptszName);
		if (nSelect == 0 && pwszSignature) {
			if (wcscmp(pwszName, pwszSignature) == 0)
				nSelect = nItem;
		}
		if (nDefault == -1 && pSignature->isDefault())
			nDefault = nItem;
	}
	if (pwszSignature && nSelect == 0 && nDefault != -1)
		nSelect = nDefault;
	
	ComboBox_SetCurSel(getHandle(), nSelect);
}


/****************************************************************************
 *
 * AccountHeaderEditItem
 *
 */

qm::AccountHeaderEditItem::AccountHeaderEditItem(EditWindowFocusController* pController) :
	ComboBoxHeaderEditItem(pController),
	bShowFrom_(true),
	pEditMessage_(0)
{
}

qm::AccountHeaderEditItem::~AccountHeaderEditItem()
{
}

void qm::AccountHeaderEditItem::setEditMessage(EditMessage* pEditMessage,
											   bool bReset)
{
	const WCHAR* pwszClass = pEditMessage->getAccount()->getClass();
	SubAccount* pCurrentSubAccount = pEditMessage->getSubAccount();
	
	ComboBox_ResetContent(getHandle());
	
	int nCurrentItem = -1;
	const Document::AccountList& listAccount = pEditMessage->getDocument()->getAccounts();
	for (Document::AccountList::const_iterator itA = listAccount.begin(); itA != listAccount.end(); ++itA) {
		Account* pAccount = *itA;
		if (wcscmp(pAccount->getClass(), pwszClass) == 0) {
			const Account::SubAccountList& listSubAccount = pAccount->getSubAccounts();
			for (Account::SubAccountList::const_iterator itS = listSubAccount.begin(); itS != listSubAccount.end(); ++itS) {
				SubAccount* pSubAccount = *itS;
				const WCHAR* pwszName = 0;
				wstring_ptr wstrName;
				if (pSubAccount->getName() && *pSubAccount->getName()) {
					wstrName = concat(pAccount->getName(), L"/", pSubAccount->getName());
					pwszName = wstrName.get();
				}
				else {
					pwszName = pAccount->getName();
				}
				
				if (bShowFrom_) {
					AddressParser address(pSubAccount->getSenderName(),
						pSubAccount->getSenderAddress());
					wstring_ptr wstrValue(address.getValue());
					wstrName = concat(pwszName, L" - ", wstrValue.get());
					pwszName = wstrName.get();
				}
				
				W2T(pwszName, ptszName);
				int nItem = ComboBox_AddString(getHandle(), ptszName);
				ComboBox_SetItemData(getHandle(), nItem, pSubAccount);
				
				if (pSubAccount == pCurrentSubAccount)
					nCurrentItem = nItem;
			}
		}
	}
	
	ComboBox_SetCurSel(getHandle(), nCurrentItem);
	
	if (!bReset)
		pEditMessage->addEditMessageHandler(this);
	pEditMessage_ = pEditMessage;
}

void qm::AccountHeaderEditItem::updateEditMessage(EditMessage* pEditMessage)
{
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

void qm::AccountHeaderEditItem::accountChanged(const EditMessageEvent& event)
{
	SubAccount* pCurrentSubAccount = event.getEditMessage()->getSubAccount();
	
	int nCount = ComboBox_GetCount(getHandle());
	int n = 0;
	while (n < nCount) {
		SubAccount* pSubAccount = reinterpret_cast<SubAccount*>(
			ComboBox_GetItemData(getHandle(), n));
		if (pSubAccount == pCurrentSubAccount)
			break;
		++n;
	}
	ComboBox_SetCurSel(getHandle(), n);
}

LRESULT qm::AccountHeaderEditItem::onChange()
{
	if (pEditMessage_) {
		int nItem = ComboBox_GetCurSel(getHandle());
		SubAccount* pSubAccount = reinterpret_cast<SubAccount*>(
			ComboBox_GetItemData(getHandle(), nItem));
		pEditMessage_->setAccount(pSubAccount->getAccount(), pSubAccount);
	}
	return 0;
}


/****************************************************************************
 *
 * HeaderEditItemCallback
 *
 */

qm::HeaderEditItemCallback::~HeaderEditItemCallback()
{
}


/****************************************************************************
 *
 * HeaderEditWindowContentHandler
 *
 */

qm::HeaderEditWindowContentHandler::HeaderEditWindowContentHandler(LineLayout* pLayout,
																   const WCHAR* pwszClass,
																   EditWindowFocusController* pController,
																   MenuManager* pMenuManager,
																   Profile* pProfile,
																   HeaderEditLineCallback* pLineCallback,
																   HeaderEditItemCallback* pItemCallback) :
	pLayout_(pLayout),
	pwszClass_(pwszClass),
	pController_(pController),
	pMenuManager_(pMenuManager),
	pProfile_(pProfile),
	pLineCallback_(pLineCallback),
	pItemCallback_(pItemCallback),
	pCurrentLine_(0),
	pCurrentItem_(0),
	state_(STATE_ROOT),
	bIgnore_(false)
{
}

qm::HeaderEditWindowContentHandler::~HeaderEditWindowContentHandler()
{
}

AttachmentSelectionModel* qm::HeaderEditWindowContentHandler::getAttachmentSelectionModel() const
{
	return pAttachmentSelectionModel_;
}

bool qm::HeaderEditWindowContentHandler::startElement(const WCHAR* pwszNamespaceURI,
													  const WCHAR* pwszLocalName,
													  const WCHAR* pwszQName,
													  const Attributes& attributes)
{
	if (wcscmp(pwszLocalName, L"headerEdit") == 0) {
		if (state_ != STATE_ROOT)
			return false;
		state_ = STATE_HEADEREDIT;
	}
	else if (wcscmp(pwszLocalName, L"line") == 0) {
		if (state_ != STATE_HEADEREDIT)
			return false;
		
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
				return false;
			}
		}
		
		if (pwszClass) {
			std::auto_ptr<RegexPattern> pClass(RegexCompiler().compile(pwszClass));
			if (!pClass.get())
				return false;
			bIgnore_ = !pClass->match(pwszClass_);
		}
		
		if (!bIgnore_) {
			std::auto_ptr<HeaderEditLine> pHeaderEditLine(
				new HeaderEditLine(pLineCallback_, nFlags));
			pCurrentLine_ = pHeaderEditLine.get();
			pLayout_->addLine(pHeaderEditLine);
		}
		
		state_ = STATE_LINE;
	}
	else if (wcscmp(pwszLocalName, L"static") == 0 ||
		wcscmp(pwszLocalName, L"edit") == 0 ||
		wcscmp(pwszLocalName, L"address") == 0) {
		if (state_ != STATE_LINE)
			return false;
		
		if (!bIgnore_) {
			assert(pCurrentLine_);
			
			enum Type {
				TYPE_STATIC,
				TYPE_EDIT,
				TYPE_ADDRESS
			};
			Type type = TYPE_STATIC;
			if (wcscmp(pwszLocalName, L"edit") == 0)
				type = TYPE_EDIT;
			else if (wcscmp(pwszLocalName, L"address") == 0)
				type = TYPE_ADDRESS;
			
			std::auto_ptr<TextHeaderEditItem> pItem;
			switch (type) {
			case TYPE_STATIC:
				pItem.reset(new StaticHeaderEditItem(pController_));
				break;
			case TYPE_EDIT:
				pItem.reset(new EditHeaderEditItem(pController_, pProfile_));
				break;
			case TYPE_ADDRESS:
				pItem.reset(new AddressHeaderEditItem(pController_, pProfile_));
				break;
			}
			
			for (int n = 0; n < attributes.getLength(); ++n) {
				const WCHAR* pwszAttrLocalName = attributes.getLocalName(n);
				if (wcscmp(pwszAttrLocalName, L"width") == 0) {
					setWidth(pItem.get(), attributes.getValue(n));
				}
				else if (wcscmp(pwszAttrLocalName, L"style") == 0) {
					pItem->setStyle(TextHeaderEditItem::parseStyle(attributes.getValue(n)));
				}
				else if (wcscmp(pwszAttrLocalName, L"align") == 0) {
					pItem->setAlign(TextHeaderEditItem::parseAlign(attributes.getValue(n)));
				}
				else if (wcscmp(pwszAttrLocalName, L"field") == 0) {
					pItem->setField(attributes.getValue(n));
				}
				else if (wcscmp(pwszAttrLocalName, L"type") == 0 && type != TYPE_ADDRESS) {
					pItem->setType(TextHeaderEditItem::parseType(attributes.getValue(n)));
				}
				else if (wcscmp(pwszAttrLocalName, L"number") == 0) {
					setNumber(pItem.get(), attributes.getValue(n));
				}
				else if (wcscmp(pwszAttrLocalName, L"initialFocus") == 0) {
					pItem->setInitialFocus(wcscmp(attributes.getValue(n), L"true") == 0);
				}
				else if (wcscmp(pwszAttrLocalName, L"expandAlias") == 0 && type == TYPE_ADDRESS) {
					static_cast<AddressHeaderEditItem*>(pItem.get())->setExpandAlias(
						wcscmp(attributes.getValue(n), L"true") == 0);
				}
				else if (wcscmp(pwszAttrLocalName, L"autoComplete") == 0 && type == TYPE_ADDRESS) {
					static_cast<AddressHeaderEditItem*>(pItem.get())->setAutoComplete(
						wcscmp(attributes.getValue(n), L"true") == 0);
				}
				else {
					return false;
				}
			}
			
			pCurrentItem_ = pItem.get();
			pCurrentLine_->addItem(pItem);
		}
		
		state_ = STATE_ITEM;
	}
	else if (wcscmp(pwszLocalName, L"attachment") == 0 ||
		wcscmp(pwszLocalName, L"signature") == 0) {
		if (state_ != STATE_LINE)
			return false;
		
		if (!bIgnore_) {
			assert(pCurrentLine_);
			
			std::auto_ptr<HeaderEditItem> pItem;
			if (wcscmp(pwszLocalName, L"attachment") == 0) {
				std::auto_ptr<AttachmentHeaderEditItem> p(
					new AttachmentHeaderEditItem(pController_,
						pMenuManager_, pItemCallback_));
				pAttachmentSelectionModel_ = p.get();
				pItem.reset(p.release());
			}
			else if (wcscmp(pwszLocalName, L"signature") == 0) {
				pItem.reset(new SignatureHeaderEditItem(pController_));
			}
			
			for (int n = 0; n < attributes.getLength(); ++n) {
				const WCHAR* pwszAttrLocalName = attributes.getLocalName(n);
				if (wcscmp(pwszAttrLocalName, L"width") == 0) {
					setWidth(pItem.get(), attributes.getValue(n));
				}
				else if (wcscmp(pwszAttrLocalName, L"number") == 0) {
					setNumber(pItem.get(), attributes.getValue(n));
				}
				else if (wcscmp(pwszAttrLocalName, L"initialFocus") == 0) {
					pItem->setInitialFocus(wcscmp(attributes.getValue(n), L"true") == 0);
				}
				else {
					return false;
				}
			}
			
			pCurrentItem_ = pItem.get();
			pCurrentLine_->addItem(pItem);
		}
		
		state_ = STATE_ITEM;
	}
	else if (wcscmp(pwszLocalName, L"account") == 0) {
		if (state_ != STATE_LINE)
			return false;
		
		if (!bIgnore_) {
			assert(pCurrentLine_);
			
			std::auto_ptr<AccountHeaderEditItem> pItem(
				new AccountHeaderEditItem(pController_));
			
			for (int n = 0; n < attributes.getLength(); ++n) {
				const WCHAR* pwszAttrLocalName = attributes.getLocalName(n);
				if (wcscmp(pwszAttrLocalName, L"width") == 0) {
					setWidth(pItem.get(), attributes.getValue(n));
				}
				else if (wcscmp(pwszAttrLocalName, L"number") == 0) {
					setNumber(pItem.get(), attributes.getValue(n));
				}
				else if (wcscmp(pwszAttrLocalName, L"initialFocus") == 0) {
					pItem->setInitialFocus(wcscmp(attributes.getValue(n), L"true") == 0);
				}
				else if (wcscmp(pwszAttrLocalName, L"showFrom") == 0) {
					pItem->setShowFrom(wcscmp(attributes.getValue(n), L"true") == 0);
				}
				else {
					return false;
				}
			}
			
			pCurrentItem_ = pItem.get();
			pCurrentLine_->addItem(pItem);
		}
		
		state_ = STATE_ITEM;
	}
	else {
		return false;
	}
	
	return true;
}

bool qm::HeaderEditWindowContentHandler::endElement(const WCHAR* pwszNamespaceURI,
													const WCHAR* pwszLocalName,
													const WCHAR* pwszQName)
{
	if (wcscmp(pwszLocalName, L"headerEdit") == 0) {
		assert(state_ == STATE_HEADEREDIT);
		state_ = STATE_ROOT;
	}
	else if (wcscmp(pwszLocalName, L"line") == 0) {
		assert(state_ == STATE_LINE);
		if (!bIgnore_) {
			assert(pCurrentLine_);
			pCurrentLine_ = 0;
		}
		state_ = STATE_HEADEREDIT;
		bIgnore_ = false;
	}
	else if (wcscmp(pwszLocalName, L"static") == 0 ||
		wcscmp(pwszLocalName, L"edit") == 0 ||
		wcscmp(pwszLocalName, L"address") == 0 ||
		wcscmp(pwszLocalName, L"attachment") == 0 ||
		wcscmp(pwszLocalName, L"signature") == 0 ||
		wcscmp(pwszLocalName, L"account") == 0) {
		assert(state_ == STATE_ITEM);
		
		if (!bIgnore_) {
			assert(pCurrentItem_);
			if (buffer_.getLength() != 0) {
				pCurrentItem_->setValue(buffer_.getCharArray());
				buffer_.remove();
			}
			pCurrentItem_ = 0;
		}
		
		state_ = STATE_LINE;
	}
	else {
		return false;
	}
	
	return true;
}

bool qm::HeaderEditWindowContentHandler::characters(const WCHAR* pwsz,
													size_t nStart,
													size_t nLength)
{
	if (state_ == STATE_ITEM) {
		if (!bIgnore_)
			buffer_.append(pwsz + nStart, nLength);
	}
	else {
		const WCHAR* p = pwsz + nStart;
		for (size_t n = 0; n < nLength; ++n, ++p) {
			if (*p != L' ' && *p != L'\t' && *p != L'\n')
				return false;
		}
	}
	
	return true;
}

void qm::HeaderEditWindowContentHandler::setWidth(LineLayoutItem* pItem,
												  const WCHAR* pwszWidth)
{
	double dWidth = 0;
	LineLayoutItem::Unit unit;
	if (LineLayoutItem::parseWidth(pwszWidth, &dWidth, &unit))
		pItem->setWidth(dWidth, unit);
}

void qm::HeaderEditWindowContentHandler::setNumber(HeaderEditItem* pItem,
												   const WCHAR* pwszNumber)
{
	WCHAR* pEnd = 0;
	unsigned int nNumber = wcstol(pwszNumber, &pEnd, 10);
	if (!*pEnd)
		pItem->setNumber(nNumber);
}
