/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#pragma warning(disable:4786)

#include <qmaccount.h>
#include <qmapplication.h>
#include <qmdocument.h>
#include <qmfilenames.h>
#include <qmmessage.h>
#include <qmmessageholder.h>
#include <qmmessagewindow.h>
#include <qmtemplate.h>

#include <qsconv.h>
#include <qsdevicecontext.h>
#include <qsinit.h>
#include <qssax.h>
#include <qsstl.h>
#include <qsuiutil.h>

#include <algorithm>

#include <tchar.h>

#include "actionid.h"
#include "headerwindow.h"
#include "uiutil.h"
#include "../model/dataobject.h"
#include "../model/uri.h"

#pragma warning(disable:4355)

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * HeaderWindowImpl
 *
 */

class qm::HeaderWindowImpl : public TextHeaderItemSite
{
public:
	enum {
		ID_HEADER_ITEM	= 1000
	};

public:
	bool load(MenuManager* pMenuManager,
			  TempFileCleaner* pTempFileCleaner);
	bool create(MenuManager* pMenuManager,
				TempFileCleaner* pTempFileCleaner);
	void reloadProfiles(bool bInitialize);

public:
	virtual void registerCallback(TextHeaderItem* pItem,
								  TextHeaderItemCallback* pCallback);

public:
	typedef std::vector<std::pair<TextHeaderItem*, TextHeaderItemCallback*> > TextHeaderItemCallbackList;

public:
	HeaderWindow* pThis_;
	Profile* pProfile_;
	
	HFONT hfont_;
	HFONT hfontBold_;
	HBRUSH hbrBackground_;
	std::auto_ptr<LineLayout> pLayout_;
	AttachmentSelectionModel* pAttachmentSelectionModel_;
	TextHeaderItemCallbackList listTextHeaderItemCallback_;
};

bool qm::HeaderWindowImpl::load(MenuManager* pMenuManager,
								TempFileCleaner* pTempFileCleaner)
{
	pLayout_.reset(new LineLayout());
	
	wstring_ptr wstrPath(Application::getApplication().getProfilePath(FileNames::HEADER_XML));
	
	XMLReader reader;
	HeaderWindowContentHandler contentHandler(
		pLayout_.get(), pMenuManager, pTempFileCleaner);
	reader.setContentHandler(&contentHandler);
	if (!reader.parse(wstrPath.get()))
		return false;
	pAttachmentSelectionModel_ = contentHandler.getAttachmentSelectionModel();
	
	return true;
}

bool qm::HeaderWindowImpl::create(MenuManager* pMenuManager,
								  TempFileCleaner* pTempFileCleaner)
{
	if (!load(pMenuManager, pTempFileCleaner))
		return false;
	
	std::pair<HFONT, HFONT> fonts(hfont_, hfontBold_);
	UINT nId = ID_HEADER_ITEM;
	if (!pLayout_->create(pThis_, fonts, &nId, this))
		return false;
	
	return true;
}

void qm::HeaderWindowImpl::reloadProfiles(bool bInitialize)
{
	HFONT hfont = qs::UIUtil::createFontFromProfile(pProfile_,
		L"HeaderWindow", qs::UIUtil::DEFAULTFONT_UI);
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

void qm::HeaderWindowImpl::registerCallback(TextHeaderItem* pItem,
											TextHeaderItemCallback* pCallback)
{
	listTextHeaderItemCallback_.push_back(std::make_pair(pItem, pCallback));
}


/****************************************************************************
 *
 * HeaderWindow
 *
 */

qm::HeaderWindow::HeaderWindow(Profile* pProfile) :
	WindowBase(true),
	pImpl_(0)
{
	pImpl_ = new HeaderWindowImpl();
	pImpl_->pThis_ = this;
	pImpl_->pProfile_ = pProfile;
	pImpl_->hfont_ = 0;
	pImpl_->hfontBold_ = 0;
	pImpl_->hbrBackground_ = 0;
	pImpl_->pAttachmentSelectionModel_ = 0;
	
	pImpl_->reloadProfiles(true);
	
	setWindowHandler(this, false);
}

qm::HeaderWindow::~HeaderWindow()
{
	delete pImpl_;
}

int qm::HeaderWindow::getHeight() const
{
	return pImpl_->pLayout_->getHeight();
}

void qm::HeaderWindow::setMessage(const TemplateContext* pContext)
{
	for (unsigned int n = 0; n < pImpl_->pLayout_->getLineCount(); ++n) {
		HeaderLine* pLine = static_cast<HeaderLine*>(
			pImpl_->pLayout_->getLine(n));
		pLine->setMessage(pContext);
	}
}

void qm::HeaderWindow::layout(const RECT& rect)
{
	ClientDeviceContext dc(getHandle());
	ObjectSelector<HFONT> fontSelecter(dc, pImpl_->hfont_);
	TEXTMETRIC tm;
	dc.getTextMetrics(&tm);
	unsigned int nFontHeight = tm.tmHeight + tm.tmExternalLeading;
	pImpl_->pLayout_->layout(rect, nFontHeight);
}

bool qm::HeaderWindow::isActive() const
{
	for (unsigned int n = 0; n < pImpl_->pLayout_->getLineCount(); ++n) {
		HeaderLine* pLine = static_cast<HeaderLine*>(
			pImpl_->pLayout_->getLine(n));
		for (unsigned int m = 0; m < pLine->getItemCount(); ++m) {
			HeaderItem* pItem = static_cast<HeaderItem*>(pLine->getItem(m));
			if (pItem->isActive())
				return true;
		}
	}
	return false;
}

MessageWindowItem* qm::HeaderWindow::getFocusedItem() const
{
	for (unsigned int n = 0; n < pImpl_->pLayout_->getLineCount(); ++n) {
		HeaderLine* pLine = static_cast<HeaderLine*>(
			pImpl_->pLayout_->getLine(n));
		for (unsigned int m = 0; m < pLine->getItemCount(); ++m) {
			HeaderItem* pItem = static_cast<HeaderItem*>(pLine->getItem(m));
			if (pItem->isActive())
				return pItem;
		}
	}
	return 0;
}

MessageWindowItem* qm::HeaderWindow::getNextFocusItem(MessageWindowItem* pItem) const
{
	for (unsigned int n = 0; n < pImpl_->pLayout_->getLineCount(); ++n) {
		HeaderLine* pLine = static_cast<HeaderLine*>(pImpl_->pLayout_->getLine(n));
		MessageWindowItem* pNewItem = pLine->getNextFocusItem(&pItem);
		if (pNewItem)
			return pNewItem;
	}
	return 0;
}

MessageWindowItem* qm::HeaderWindow::getPrevFocusItem(MessageWindowItem* pItem) const
{
	for (unsigned int n = pImpl_->pLayout_->getLineCount(); n > 0; --n) {
		HeaderLine* pLine = static_cast<HeaderLine*>(pImpl_->pLayout_->getLine(n - 1));
		MessageWindowItem* pNewItem = pLine->getPrevFocusItem(&pItem);
		if (pNewItem)
			return pNewItem;
	}
	return 0;
}

MessageWindowItem* qm::HeaderWindow::getItemByNumber(unsigned int nNumber) const
{
	for (unsigned int n = 0; n < pImpl_->pLayout_->getLineCount(); ++n) {
		HeaderLine* pLine = static_cast<HeaderLine*>(pImpl_->pLayout_->getLine(n));
		MessageWindowItem* pItem = pLine->getItemByNumber(nNumber);
		if (pItem)
			return pItem;
	}
	return 0;
}

AttachmentSelectionModel* qm::HeaderWindow::getAttachmentSelectionModel() const
{
	return pImpl_->pAttachmentSelectionModel_;
}

void qm::HeaderWindow::reloadProfiles()
{
	pImpl_->reloadProfiles(false);
}

void qm::HeaderWindow::getWindowClass(WNDCLASS* pwc)
{
	DefaultWindowHandler::getWindowClass(pwc);
	pwc->hbrBackground = reinterpret_cast<HBRUSH>(COLOR_3DFACE + 1);
}

LRESULT qm::HeaderWindow::windowProc(UINT uMsg,
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

LRESULT qm::HeaderWindow::onCreate(CREATESTRUCT* pCreateStruct)
{
	if (DefaultWindowHandler::onCreate(pCreateStruct) == -1)
		return -1;
	
	HeaderWindowCreateContext* pContext =
		static_cast<HeaderWindowCreateContext*>(pCreateStruct->lpCreateParams);
	
	WNDCLASS wc;
	if (::GetClassInfo(pCreateStruct->hInstance, pCreateStruct->lpszClass, &wc))
		pImpl_->hbrBackground_ = wc.hbrBackground;
	
	if (!pImpl_->create(pContext->pMenuManager_, pContext->pTempFileCleaner_))
		return false;
	
	return 0;
}

LRESULT qm::HeaderWindow::onCtlColorStatic(HDC hdc,
										   HWND hwnd)
{
	DefaultWindowHandler::onCtlColorStatic(hdc, hwnd);
	
	DeviceContext dc(hdc);
	HBRUSH hbr = 0;
	
	typedef HeaderWindowImpl::TextHeaderItemCallbackList List;
	const List& l = pImpl_->listTextHeaderItemCallback_;
	List::const_iterator it = std::find_if(l.begin(), l.end(),
		boost::bind(&TextHeaderItem::getHandle,
			boost::bind(&List::value_type::first, _1)) == hwnd);
	if (it != l.end())
		hbr = (*it).second->getColor(&dc);
	
	if (!hbr) {
		dc.setTextColor(::GetSysColor(COLOR_WINDOWTEXT));
		dc.setBkColor(::GetSysColor(COLOR_3DFACE));
		hbr = pImpl_->hbrBackground_;
	}
	return reinterpret_cast<LRESULT>(hbr);
}

LRESULT qm::HeaderWindow::onDestroy()
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
 * HeaderLine
 *
 */

qm::HeaderLine::HeaderLine(const WCHAR* pwszHideIfEmpty,
						   std::auto_ptr<RegexPattern> pClass) :
	pClass_(pClass),
	bHide_(false)
{
	const WCHAR* p = pwszHideIfEmpty;
	while (p) {
		const WCHAR* pEnd = wcschr(p, L',');
		wstring_ptr wstr;
		if (pEnd)
			wstr = allocWString(p, pEnd - p);
		else
			wstr = allocWString(p);
		listHide_.push_back(HideList::value_type(wstr.get(), 0));
		wstr.release();
		
		p = pEnd ? pEnd + 1 : 0;
	}
}

qm::HeaderLine::~HeaderLine()
{
	std::for_each(listHide_.begin(), listHide_.end(),
		boost::bind(&freeWString, boost::bind(&HideList::value_type::first, _1)));
}

void qm::HeaderLine::setMessage(const TemplateContext* pContext)
{
	if (pContext && pClass_.get()) {
		Account* pAccount = pContext->getAccount();
		bHide_ = !pClass_->match(pAccount ? pAccount->getClass() : L"mail");
	}
	else {
		bHide_ = false;
	}
	
	if (!bHide_) {
		for (unsigned int n = 0; n < getItemCount(); ++n) {
			HeaderItem* pItem = static_cast<HeaderItem*>(getItem(n));
			pItem->setMessage(pContext);
		}
	}
}

void qm::HeaderLine::fixup()
{
	for (HideList::iterator itH = listHide_.begin(); itH != listHide_.end(); ++itH) {
		for (unsigned int nItem = 0; nItem < getItemCount(); ++nItem) {
			HeaderItem* pItem = static_cast<HeaderItem*>(getItem(nItem));
			const WCHAR* pwszName = pItem->getName();
			if (pwszName && wcscmp(pwszName, (*itH).first) == 0) {
				(*itH).second = pItem;
				break;
			}
		}
	}
}

MessageWindowItem* qm::HeaderLine::getNextFocusItem(MessageWindowItem** ppItem) const
{
	assert(ppItem);
	
	if (!isHidden()) {
		for (unsigned int n = 0; n < getItemCount(); ++n) {
			HeaderItem* pItem = static_cast<HeaderItem*>(getItem(n));
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

MessageWindowItem* qm::HeaderLine::getPrevFocusItem(MessageWindowItem** ppItem) const
{
	assert(ppItem);
	
	if (!isHidden()) {
		for (unsigned int n = getItemCount(); n > 0; --n) {
			HeaderItem* pItem = static_cast<HeaderItem*>(getItem(n - 1));
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

MessageWindowItem* qm::HeaderLine::getItemByNumber(unsigned int nNumber) const
{
	if (isHidden())
		return 0;
	
	for (unsigned int n = 0; n < getItemCount(); ++n) {
		HeaderItem* pItem = static_cast<HeaderItem*>(getItem(n));
		if (pItem->getNumber() == nNumber)
			return pItem;
	}
	
	return 0;
}

bool qm::HeaderLine::isHidden() const
{
	if (bHide_)
		return true;
	
	if (listHide_.empty())
		return false;
	
	for (HideList::const_iterator itH = listHide_.begin(); itH != listHide_.end(); ++itH) {
		if ((*itH).second && !(*itH).second->isEmptyValue())
			return false;
	}
	
	return true;
}


/****************************************************************************
 *
 * HeaderItem
 *
 */

qm::HeaderItem::HeaderItem() :
	nFlags_(0),
	nNumber_(-1)
{
}

qm::HeaderItem::~HeaderItem()
{
}

const WCHAR* qm::HeaderItem::getName() const
{
	return wstrName_.get();
}

unsigned int qm::HeaderItem::getFlags() const
{
	return nFlags_;
}

unsigned int qm::HeaderItem::getNumber() const
{
	return nNumber_;
}

void qm::HeaderItem::setName(const WCHAR* pwszName)
{
	wstrName_ = allocWString(pwszName);
}

void qm::HeaderItem::setFlags(unsigned int nFlags,
							  unsigned int nMask)
{
	nFlags_ &= ~nMask;
	nFlags_ |= nFlags & nMask;
}

void qm::HeaderItem::setNumber(unsigned int nNumber)
{
	nNumber_ = nNumber;
}

void qm::HeaderItem::setValue(std::auto_ptr<Template> pValue)
{
	pValue_ = pValue;
}

void qm::HeaderItem::copy()
{
}

bool qm::HeaderItem::canCopy()
{
	return false;
}

void qm::HeaderItem::selectAll()
{
}

bool qm::HeaderItem::canSelectAll()
{
	return false;
}

wstring_ptr qm::HeaderItem::getValue(const TemplateContext& context) const
{
	wstring_ptr wstrValue;
	if (context.getMessage() || (nFlags_ & FLAG_SHOWALWAYS)) {
		if (pValue_->getValue(context, &wstrValue) != Template::RESULT_SUCCESS)
			return 0;
	}
	else {
		wstrValue = allocWString(L"");
	}
	return wstrValue;
}


/****************************************************************************
 *
 * TextHeaderItemCallback
 *
 */

qm::TextHeaderItemCallback::~TextHeaderItemCallback()
{
}


/****************************************************************************
 *
 * TextHeaderItemSite
 *
 */

qm::TextHeaderItemSite::~TextHeaderItemSite()
{
}


/****************************************************************************
 *
 * TextHeaderItem
 *
 */

qm::TextHeaderItem::TextHeaderItem() :
	nStyle_(STYLE_NORMAL),
	align_(ALIGN_LEFT),
	hwnd_(0),
	crBackground_(0xffffffff),
	hbrBackground_(0)
{
}

qm::TextHeaderItem::~TextHeaderItem()
{
}

HWND qm::TextHeaderItem::getHandle() const
{
	return hwnd_;
}

void qm::TextHeaderItem::setStyle(unsigned int nStyle)
{
	nStyle_ = nStyle;
}

void qm::TextHeaderItem::setAlign(Align align)
{
	align_ = align;
}

void qm::TextHeaderItem::setBackground(std::auto_ptr<Template> pBackground)
{
	pBackground_ = pBackground;
}

unsigned int qm::TextHeaderItem::getHeight(unsigned int nWidth,
										   unsigned int nFontHeight) const
{
	return nFontHeight;
}

bool qm::TextHeaderItem::create(WindowBase* pParent,
								const std::pair<HFONT, HFONT>& fonts,
								UINT nId,
								void* pParam)
{
	assert(!hwnd_);
	
	hwnd_ = ::CreateWindow(getWindowClassName(), 0,
		WS_CHILD | WS_VISIBLE | getWindowStyle(), 0, 0, 0, 0,
		pParent->getHandle(), reinterpret_cast<HMENU>(nId),
		Init::getInit().getInstanceHandle(), 0);
	if (!hwnd_)
		return false;
	
	Window(hwnd_).setFont((nStyle_ & STYLE_BOLD) ? fonts.second : fonts.first);
	
	if (pBackground_.get()) {
		TextHeaderItemSite* pSite = static_cast<TextHeaderItemSite*>(pParam);
		pSite->registerCallback(this, this);
	}
	
	return true;
}

void qm::TextHeaderItem::destroy()
{
	if (hbrBackground_)
		::DeleteObject(hbrBackground_);
}

HDWP qm::TextHeaderItem::layout(HDWP hdwp,
								const RECT& rect,
								unsigned int nFontHeight)
{
	unsigned int nHeight = getHeight(rect.right - rect.left, nFontHeight);
	unsigned int nFlags = SWP_NOZORDER | SWP_NOACTIVATE;
#ifndef _WIN32_WCE
	nFlags |= SWP_NOCOPYBITS;
#endif
	hdwp = Window(hwnd_).deferWindowPos(hdwp, 0, rect.left,
		rect.top, rect.right - rect.left, nHeight, nFlags);
	
	postLayout();
	
#ifdef _WIN32_WCE
	Window(hwnd_).invalidate();
#endif
	
	return hdwp;
}

void qm::TextHeaderItem::show(bool bShow)
{
	Window(hwnd_).showWindow(bShow ? SW_SHOW : SW_HIDE);
}

void qm::TextHeaderItem::setFont(const std::pair<HFONT, HFONT>& fonts)
{
	Window(hwnd_).setFont((nStyle_ & STYLE_BOLD) ? fonts.second : fonts.first);
}

void qm::TextHeaderItem::setMessage(const TemplateContext* pContext)
{
	if (pContext) {
		wstring_ptr wstrValue(getValue(*pContext));
		
		const WCHAR* pwszValue = L"";
		if (wstrValue.get())
			pwszValue = wstrValue.get();
		
		const WCHAR* p = wcschr(pwszValue, L'\n');
		if (p) {
			StringBuffer<WSTRING> buf(pwszValue, p - pwszValue);
			while (*p) {
				if (*p == L'\n')
					buf.append(L'\r');
				buf.append(*p);
				++p;
			}
			Window(hwnd_).setWindowText(buf.getCharArray());
		}
		else {
			Window(hwnd_).setWindowText(pwszValue);
		}
	}
	else {
		Window(hwnd_).setWindowText(L"");
	}
	updateColor(pContext);
}

bool qm::TextHeaderItem::isEmptyValue() const
{
	return Window(hwnd_).getWindowTextLength() == 0;
}

bool qm::TextHeaderItem::isActive() const
{
	return Window(hwnd_).hasFocus();
}

TextHeaderItem::Align qm::TextHeaderItem::getAlign() const
{
	return align_;
}

void qm::TextHeaderItem::postLayout()
{
}

HBRUSH qm::TextHeaderItem::getColor(qs::DeviceContext* pdc)
{
	if (hbrBackground_)
		pdc->setBkColor(crBackground_);
	return hbrBackground_;
}

unsigned int qm::TextHeaderItem::parseStyle(const WCHAR* pwszStyle)
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

TextHeaderItem::Align qm::TextHeaderItem::parseAlign(const WCHAR* pwszAlign)
{
	if (wcscmp(pwszAlign, L"right") == 0)
		return ALIGN_RIGHT;
	else if (wcscmp(pwszAlign, L"center") == 0)
		return ALIGN_CENTER;
	else
		return ALIGN_LEFT;
}

void qm::TextHeaderItem::updateColor(const TemplateContext* pContext)
{
	COLORREF crBackground = 0xffffffff;
	if (pBackground_.get()) {
		if (pContext && (pContext->getMessageHolder() || (getFlags() & FLAG_SHOWALWAYS))) {
			wstring_ptr wstrBackground;
			if (pBackground_->getValue(*pContext, &wstrBackground) == Template::RESULT_SUCCESS)
				crBackground = Color(wstrBackground.get()).getColor();
		}
	}
	
	if (crBackground != crBackground_) {
		crBackground_ = crBackground;
		if (hbrBackground_)
			::DeleteObject(hbrBackground_);
		if (crBackground_ != 0xffffffff)
			hbrBackground_ = ::CreateSolidBrush(crBackground_);
		else
			hbrBackground_ = 0;
	}
}


/****************************************************************************
 *
 * StaticHeaderItem
 *
 */

qm::StaticHeaderItem::StaticHeaderItem()
{
}

qm::StaticHeaderItem::~StaticHeaderItem()
{
}

unsigned int qm::StaticHeaderItem::getPreferredWidth() const
{
	return UIUtil::getPreferredWidth(getHandle(), true);
}

bool qm::StaticHeaderItem::isFocusItem() const
{
	return false;
}

const TCHAR* qm::StaticHeaderItem::getWindowClassName() const
{
	return _T("STATIC");
}

UINT qm::StaticHeaderItem::getWindowStyle() const
{
	UINT nStyle = SS_NOPREFIX;
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

void qm::StaticHeaderItem::setFocus()
{
}


/****************************************************************************
 *
 * EditHeaderItem
 *
 */

qm::EditHeaderItem::EditHeaderItem() :
	nMultiline_(-1),
	bWrap_(false)
{
}

qm::EditHeaderItem::~EditHeaderItem()
{
}

void qm::EditHeaderItem::setMultiline(unsigned int nMultiline)
{
	nMultiline_ = nMultiline;
}

void qm::EditHeaderItem::setWrap(bool bWrap)
{
	bWrap_ = bWrap;
}

unsigned int qm::EditHeaderItem::getHeight(unsigned int nWidth,
										   unsigned int nFontHeight) const
{
	if (nMultiline_ == -1) {
		return TextHeaderItem::getHeight(nWidth, nFontHeight);
	}
	else {
		unsigned int nLineCount = getLineCount(nWidth, nFontHeight);
		if (nMultiline_ == 0)
			return nLineCount*nFontHeight;
		else
			return QSMIN(nLineCount, nMultiline_)*nFontHeight;
	}
}

unsigned int qm::EditHeaderItem::getPreferredWidth() const
{
	return UIUtil::getPreferredWidth(getHandle(), true);
}

bool qm::EditHeaderItem::isFocusItem() const
{
	return true;
}

const TCHAR* qm::EditHeaderItem::getWindowClassName() const
{
	return _T("EDIT");
}

UINT qm::EditHeaderItem::getWindowStyle() const
{
	UINT nStyle = ES_READONLY;
	
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
	
	if (nMultiline_ == -1 || !bWrap_)
		nStyle |= ES_AUTOHSCROLL;
	if (nMultiline_ != -1)
		nStyle |= ES_MULTILINE | WS_VSCROLL;
	
	return nStyle;
}

void qm::EditHeaderItem::postLayout()
{
	if (nMultiline_ != -1)
		Window(getHandle()).showScrollBar(SB_VERT,
			nMultiline_ != 0 && getLineCount(-1, -1) > nMultiline_);
}

void qm::EditHeaderItem::copy()
{
	Window(getHandle()).sendMessage(WM_COPY);
}

bool qm::EditHeaderItem::canCopy()
{
	DWORD dwStart = 0;
	DWORD dwEnd = 0;
	Window(getHandle()).sendMessage(EM_GETSEL,
		reinterpret_cast<WPARAM>(&dwStart),
		reinterpret_cast<LPARAM>(&dwEnd));
	return dwStart != dwEnd;
}

void qm::EditHeaderItem::selectAll()
{
	Window(getHandle()).sendMessage(EM_SETSEL, 0, -1);
}

bool qm::EditHeaderItem::canSelectAll()
{
	return true;
}

void qm::EditHeaderItem::setFocus()
{
	Window(getHandle()).setFocus();
}

unsigned int qm::EditHeaderItem::getLineCount(unsigned int nWidth,
											  unsigned int nFontHeight) const
{
	if (nMultiline_ == -1)
		return 1;
	
	Window wnd(getHandle());
	if (bWrap_ && nWidth != -1) {
		RECT rect;
		wnd.getWindowRect(&rect);
		if (static_cast<unsigned int>(rect.right - rect.left) != nWidth)
			wnd.setWindowPos(0, 0, 0, nWidth, nFontHeight,
				SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
	}
	return static_cast<unsigned int>(wnd.sendMessage(EM_GETLINECOUNT));
}

unsigned int qm::EditHeaderItem::parseMultiline(const WCHAR* pwszMultiline)
{
	WCHAR* pEnd = 0;
	long n = wcstol(pwszMultiline, &pEnd, 10);
	if (*pEnd)
		return -1;
	return n;
}


/****************************************************************************
 *
 * AttachmentHeaderItem
 *
 */

qm::AttachmentHeaderItem::AttachmentHeaderItem(MenuManager* pMenuManager,
											   TempFileCleaner* pTempFileCleaner) :
	wnd_(this),
	pMenuManager_(pMenuManager),
	pTempFileCleaner_(pTempFileCleaner),
	pParent_(0),
	pURIResolver_(0),
	nSecurityMode_(SECURITYMODE_NONE),
	bAttachmentDeleted_(false)
{
}

qm::AttachmentHeaderItem::~AttachmentHeaderItem()
{
}

void qm::AttachmentHeaderItem::setBackground(std::auto_ptr<Template> pBackground)
{
	pBackground_ = pBackground;
}

unsigned int qm::AttachmentHeaderItem::getHeight(unsigned int nWidth,
												 unsigned int nFontHeight) const
{
	int nCount = ListView_GetItemCount(wnd_.getHandle());
	unsigned int nHeight = QSMAX(nFontHeight,
		static_cast<unsigned int>(::GetSystemMetrics(SM_CYSMICON))) + 2;
	return QSMIN(nCount*nHeight, nHeight*4 + 2);
}

bool qm::AttachmentHeaderItem::create(WindowBase* pParent,
									  const std::pair<HFONT, HFONT>& fonts,
									  UINT nId,
									  void* pParam)
{
	assert(!wnd_.getHandle());
	
	pParent_ = pParent;
	
	DWORD dwStyle = WS_CHILD | WS_VISIBLE | LVS_REPORT |
		LVS_SHAREIMAGELISTS | LVS_NOCOLUMNHEADER;
	if (!wnd_.create(L"QmAttachmentWindow", 0, dwStyle,
		0, 0, 0, 0, pParent->getHandle(), 0, WC_LISTVIEWW, nId, 0))
		return false;
	
	ListView_SetBkColor(wnd_.getHandle(), ::GetSysColor(COLOR_3DFACE));
	ListView_SetTextBkColor(wnd_.getHandle(), ::GetSysColor(COLOR_3DFACE));
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
	
#ifndef _WIN32_WCE
	pParent_->addNotifyHandler(this);
#endif
	
	return true;
}

void qm::AttachmentHeaderItem::destroy()
{
#ifndef _WIN32_WCE
	pParent_->removeNotifyHandler(this);
#endif
	clear();
}

HDWP qm::AttachmentHeaderItem::layout(HDWP hdwp,
									  const RECT& rect,
									  unsigned int nFontHeight)
{
	unsigned int nFlags = SWP_NOZORDER | SWP_NOACTIVATE;
	return wnd_.deferWindowPos(hdwp, 0, rect.left, rect.top,
		rect.right - rect.left, rect.bottom - rect.top, nFlags);
}

void qm::AttachmentHeaderItem::show(bool bShow)
{
	wnd_.showWindow(bShow ? SW_SHOW : SW_HIDE);
}

void qm::AttachmentHeaderItem::setFont(const std::pair<HFONT, HFONT>& fonts)
{
	wnd_.setFont(fonts.first);
}

void qm::AttachmentHeaderItem::setMessage(const TemplateContext* pContext)
{
	HWND hwnd = wnd_.getHandle();
	
	clear();
	
	if (pContext) {
		pURIResolver_ = pContext->getDocument()->getURIResolver();
		nSecurityMode_ = pContext->getSecurityMode();
		
		MessageHolderBase* pmh = pContext->getMessageHolder();
		Message* pMessage = pContext->getMessage();
		if (pMessage) {
			bool b = true;
			if (pmh)
				b = pmh->getMessage(Account::GMF_TEXT, 0, nSecurityMode_, pMessage);
			if (b) {
				AttachmentParser parser(*pMessage);
				AttachmentParser::AttachmentList list;
				AttachmentParser::AttachmentListFree free(list);
				parser.getAttachments(AttachmentParser::GAF_INCLUDEDELETED, &list);
				for (AttachmentParser::AttachmentList::size_type n = 0; n < list.size(); ++n) {
					W2T(list[n].first, ptszName);
					std::auto_ptr<URI> pURI;
					if (pmh)
						pURI.reset(new MessageHolderURI(pmh->getMessageHolder(),
							pMessage, list[n].second, URIFragment::TYPE_BODY));
					else
						pURI = pURIResolver_->getTemporaryURI(list[n].second,
							URIFragment::TYPE_BODY, nSecurityMode_);
					assert(pURI.get());
					
					SHFILEINFO info = { 0 };
					::SHGetFileInfo(ptszName, FILE_ATTRIBUTE_NORMAL, &info, sizeof(info),
						SHGFI_USEFILEATTRIBUTES | SHGFI_SYSICONINDEX | SHGFI_SMALLICON);
					LVITEM item = {
						LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM,
						static_cast<int>(n),
						0,
						0,
						0,
						const_cast<LPTSTR>(ptszName),
						0,
						info.iIcon,
						reinterpret_cast<LPARAM>(pURI.get())
					};
					ListView_InsertItem(hwnd, &item);
					pURI.release();
				}
				
				bAttachmentDeleted_ = parser.isAttachmentDeleted();
			}
		}
	}
	
	updateColor(pContext);
}

bool qm::AttachmentHeaderItem::isEmptyValue() const
{
	return ListView_GetItemCount(wnd_.getHandle()) == 0;
}

bool qm::AttachmentHeaderItem::isActive() const
{
	return wnd_.hasFocus();
}

bool qm::AttachmentHeaderItem::isFocusItem() const
{
	return true;
}

void qm::AttachmentHeaderItem::setFocus()
{
	wnd_.setFocus();
}

bool qm::AttachmentHeaderItem::hasAttachment()
{
	return ListView_GetItemCount(wnd_.getHandle()) != 0;
}

bool qm::AttachmentHeaderItem::hasSelectedAttachment()
{
	return ListView_GetSelectedCount(wnd_.getHandle()) != 0;
}

void qm::AttachmentHeaderItem::getSelectedAttachment(NameList* pList)
{
	assert(pList);
	
	HWND hwnd = wnd_.getHandle();
	
	int nItem = ListView_GetNextItem(hwnd, -1, LVNI_ALL | LVNI_SELECTED);
	while (nItem != -1) {
		TCHAR tszName[MAX_PATH];
		ListView_GetItemText(hwnd, nItem, 0, tszName, countof(tszName));
		wstring_ptr wstrName(tcs2wcs(tszName));
		pList->push_back(wstrName.get());
		wstrName.release();
		nItem = ListView_GetNextItem(hwnd, nItem, LVNI_ALL | LVNI_SELECTED);
	}
}

bool qm::AttachmentHeaderItem::isAttachmentDeleted()
{
	return bAttachmentDeleted_;
}

#ifndef _WIN32_WCE
LRESULT qm::AttachmentHeaderItem::onNotify(NMHDR* pnmhdr,
										   bool* pbHandled)
{
	BEGIN_NOTIFY_HANDLER()
		HANDLE_NOTIFY_CODE(LVN_BEGINDRAG, onBeginDrag);
	END_NOTIFY_HANDLER()
	return NotifyHandler::onNotify(pnmhdr, pbHandled);
}

LRESULT qm::AttachmentHeaderItem::onBeginDrag(NMHDR* pnmhdr,
											  bool* pbHandled)
{
	assert(pURIResolver_);
	
	if (bAttachmentDeleted_)
		return 0;
	
	NMLISTVIEW* pnmlv = reinterpret_cast<NMLISTVIEW*>(pnmhdr);
	
	URIDataObject::URIList listURI;
	
	HWND hwnd = wnd_.getHandle();
	int nFirstItem = ListView_GetNextItem(hwnd, -1, LVNI_ALL | LVNI_SELECTED);
	int nItem = nFirstItem;
	while (nItem != -1) {
		LVITEM item = {
			LVIF_PARAM,
			nItem,
			0
		};
		ListView_GetItem(hwnd, &item);
		listURI.push_back(reinterpret_cast<URI*>(item.lParam)->clone().release());
		nItem = ListView_GetNextItem(hwnd, nItem, LVNI_ALL | LVNI_SELECTED);
	}
	if (listURI.empty())
		return 0;
	
	std::auto_ptr<URIDataObject> p(new URIDataObject(pURIResolver_,
		pTempFileCleaner_, nSecurityMode_, listURI));
	p->AddRef();
	ComPtr<IDataObject> pDataObject(p.release());
	
	POINT pt;
	HIMAGELIST hImageList = ListView_CreateDragImage(hwnd, nFirstItem, &pt);
	ImageList_BeginDrag(hImageList, 0, pnmlv->ptAction.x - pt.x, pnmlv->ptAction.y - pt.y);
	
	DragSource source;
	source.setDragSourceHandler(this);
	DragGestureEvent event(wnd_.getHandle(), pnmlv->ptAction);
	source.startDrag(event, pDataObject.get(), DROPEFFECT_COPY);
	
	ImageList_EndDrag();
	ImageList_Destroy(hImageList);
	
	return 0;
}

void qm::AttachmentHeaderItem::dragDropEnd(const DragSourceDropEvent& event)
{
}
#endif

void qm::AttachmentHeaderItem::clear()
{
	HWND hwnd = wnd_.getHandle();
	
	int nItem = ListView_GetNextItem(hwnd, -1, LVNI_ALL);
	while (nItem != -1) {
		LVITEM item = { LVIF_PARAM, nItem, 0 };
		ListView_GetItem(hwnd, &item);
		delete reinterpret_cast<URI*>(item.lParam);
		nItem = ListView_GetNextItem(hwnd, nItem, LVNI_ALL);
	}
	ListView_DeleteAllItems(hwnd);
	
	pURIResolver_ = 0;
	nSecurityMode_ = SECURITYMODE_NONE;
	bAttachmentDeleted_ = false;
}

void qm::AttachmentHeaderItem::updateColor(const TemplateContext* pContext)
{
	COLORREF crBackground = 0xffffffff;
	if (pBackground_.get()) {
		if (pContext && (pContext->getMessageHolder() || (getFlags() & FLAG_SHOWALWAYS))) {
			wstring_ptr wstrBackground;
			if (pBackground_->getValue(*pContext, &wstrBackground) == Template::RESULT_SUCCESS)
				crBackground = Color(wstrBackground.get()).getColor();
		}
	}
	if (crBackground == 0xffffffff)
		crBackground = ::GetSysColor(COLOR_3DFACE);
	
	ListView_SetBkColor(wnd_.getHandle(), crBackground);
	ListView_SetTextBkColor(wnd_.getHandle(), crBackground);
}


/****************************************************************************
 *
 * AttachmentHeaderItem::AttachmentWindow
 *
 */

qm::AttachmentHeaderItem::AttachmentWindow::AttachmentWindow(AttachmentHeaderItem* pItem) :
	WindowBase(false),
	pItem_(pItem)
{
	setWindowHandler(this, false);
}

qm::AttachmentHeaderItem::AttachmentWindow::~AttachmentWindow()
{
}

LRESULT qm::AttachmentHeaderItem::AttachmentWindow::windowProc(UINT uMsg,
															   WPARAM wParam,
															   LPARAM lParam)
{
	BEGIN_MESSAGE_HANDLER()
		HANDLE_CONTEXTMENU()
		HANDLE_LBUTTONDBLCLK()
		HANDLE_LBUTTONDOWN()
		HANDLE_SIZE()
	END_MESSAGE_HANDLER()
	return DefaultWindowHandler::windowProc(uMsg, wParam, lParam);
}

LRESULT qm::AttachmentHeaderItem::AttachmentWindow::onContextMenu(HWND hwnd,
																  const POINT& pt)
{
	POINT ptMenu = UIUtil::getListViewContextMenuPosition(getHandle(), pt);
	HMENU hmenu = pItem_->pMenuManager_->getMenu(L"attachment", false, false);
	if (hmenu) {
		UINT nFlags = TPM_LEFTALIGN | TPM_TOPALIGN;
#ifndef _WIN32_WCE
		nFlags |= TPM_LEFTBUTTON | TPM_RIGHTBUTTON;
#endif
		::TrackPopupMenu(hmenu, nFlags, ptMenu.x, ptMenu.y, 0, getParentFrame(), 0);
	}
	return 0;
}

LRESULT qm::AttachmentHeaderItem::AttachmentWindow::onLButtonDblClk(UINT nFlags,
																	const POINT& pt)
{
	Window(getParentFrame()).postMessage(WM_COMMAND,
		MAKEWPARAM(IDM_ATTACHMENT_OPEN, 0), 0);
	return DefaultWindowHandler::onLButtonDblClk(nFlags, pt);
}

LRESULT qm::AttachmentHeaderItem::AttachmentWindow::onLButtonDown(UINT nFlags,
																  const POINT& pt)
{
#if defined _WIN32_WCE && _WIN32_WCE >= 0x300 && _WIN32_WCE < 0x400 && defined _WIN32_WCE_PSPC
	if (tapAndHold(pt))
		return 0;
#endif
	return DefaultWindowHandler::onLButtonDown(nFlags, pt);
}

LRESULT qm::AttachmentHeaderItem::AttachmentWindow::onSize(UINT nFlags,
														   int cx,
														   int cy)
{
	RECT rect;
	getWindowRect(&rect);
	int nWidth = rect.right - rect.left - ::GetSystemMetrics(SM_CXVSCROLL) - 5;
	ListView_SetColumnWidth(getHandle(), 0, nWidth);
	return DefaultWindowHandler::onSize(nFlags, cx, cy);
}


/****************************************************************************
 *
 * HeaderWindowContentHandler
 *
 */

qm::HeaderWindowContentHandler::HeaderWindowContentHandler(LineLayout* pLayout,
														   MenuManager* pMenuManager,
														   TempFileCleaner* pTempFileCleaner) :
	pLayout_(pLayout),
	pMenuManager_(pMenuManager),
	pTempFileCleaner_(pTempFileCleaner),
	pCurrentLine_(0),
	pCurrentItem_(0),
	state_(STATE_ROOT),
	pAttachmentSelectionModel_(0)
{
}

qm::HeaderWindowContentHandler::~HeaderWindowContentHandler()
{
}

AttachmentSelectionModel* qm::HeaderWindowContentHandler::getAttachmentSelectionModel() const
{
	return pAttachmentSelectionModel_;
}

bool qm::HeaderWindowContentHandler::startElement(const WCHAR* pwszNamespaceURI,
												  const WCHAR* pwszLocalName,
												  const WCHAR* pwszQName,
												  const Attributes& attributes)
{
	if (wcscmp(pwszLocalName, L"header") == 0) {
		if (state_ != STATE_ROOT)
			return false;
		state_ = STATE_HEADER;
	}
	else if (wcscmp(pwszLocalName, L"line") == 0) {
		if (state_ != STATE_HEADER)
			return false;
		
		const WCHAR* pwszHideIfEmpty = 0;
		const WCHAR* pwszClass = 0;
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrLocalName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrLocalName, L"hideIfEmpty") == 0)
				pwszHideIfEmpty = attributes.getValue(n);
			else if (wcscmp(pwszAttrLocalName, L"class") == 0)
				pwszClass = attributes.getValue(n);
			else
				return false;
		}
		
		std::auto_ptr<RegexPattern> pClass;
		if (pwszClass) {
			pClass = RegexCompiler().compile(pwszClass);
			if (!pClass.get())
				return false;
		}
		
		std::auto_ptr<HeaderLine> pHeaderLine(
			new HeaderLine(pwszHideIfEmpty, pClass));
		pCurrentLine_ = pHeaderLine.get();
		pLayout_->addLine(std::auto_ptr<LineLayoutLine>(pHeaderLine));
		
		state_ = STATE_LINE;
	}
	else if (wcscmp(pwszLocalName, L"static") == 0 ||
		wcscmp(pwszLocalName, L"edit") == 0) {
		if (state_ != STATE_LINE)
			return false;
		
		assert(pCurrentLine_);
		
		bool bStatic = wcscmp(pwszLocalName, L"static") == 0;
		std::auto_ptr<TextHeaderItem> pItem;
		if (bStatic)
			pItem.reset(new StaticHeaderItem());
		else
			pItem.reset(new EditHeaderItem());
		
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrLocalName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrLocalName, L"name") == 0) {
				pItem->setName(attributes.getValue(n));
			}
			else if (wcscmp(pwszAttrLocalName, L"width") == 0) {
				setWidth(pItem.get(), attributes.getValue(n));
			}
			else if (wcscmp(pwszAttrLocalName, L"style") == 0) {
				pItem->setStyle(TextHeaderItem::parseStyle(attributes.getValue(n)));
			}
			else if (wcscmp(pwszAttrLocalName, L"align") == 0) {
				pItem->setAlign(TextHeaderItem::parseAlign(attributes.getValue(n)));
			}
			else if (wcscmp(pwszAttrLocalName, L"showAlways") == 0) {
				if (wcscmp(attributes.getValue(n), L"true") == 0)
					pItem->setFlags(HeaderItem::FLAG_SHOWALWAYS,
						HeaderItem::FLAG_SHOWALWAYS);
			}
			else if (wcscmp(pwszAttrLocalName, L"number") == 0) {
				setNumber(pItem.get(), attributes.getValue(n));
			}
			else if (!bStatic && wcscmp(pwszAttrLocalName, L"multiline") == 0) {
				static_cast<EditHeaderItem*>(pItem.get())->setMultiline(
					EditHeaderItem::parseMultiline(attributes.getValue(n)));
			}
			else if (!bStatic && wcscmp(pwszAttrLocalName, L"wrap") == 0) {
				if (wcscmp(attributes.getValue(n), L"true") == 0)
					static_cast<EditHeaderItem*>(pItem.get())->setWrap(true);
			}
			else if (wcscmp(pwszAttrLocalName, L"background") == 0) {
				std::auto_ptr<Template> pBackground(parseTemplate(attributes.getValue(n)));
				if (!pBackground.get())
					return false;
				pItem->setBackground(pBackground);
			}
			else {
				return false;
			}
		}
		
		pCurrentItem_ = pItem.get();
		pCurrentLine_->addItem(std::auto_ptr<LineLayoutItem>(pItem));
		
		state_ = STATE_ITEM;
	}
	else if (wcscmp(pwszLocalName, L"attachment") == 0) {
		if (state_ != STATE_LINE)
			return false;
		
		assert(pCurrentLine_);
		
		std::auto_ptr<AttachmentHeaderItem> pItem(new AttachmentHeaderItem(
			pMenuManager_, pTempFileCleaner_));
		
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrLocalName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrLocalName, L"name") == 0) {
				pItem->setName(attributes.getValue(n));
			}
			else if (wcscmp(pwszAttrLocalName, L"width") == 0) {
				setWidth(pItem.get(), attributes.getValue(n));
			}
			else if (wcscmp(pwszAttrLocalName, L"showAlways") == 0) {
				if (wcscmp(attributes.getValue(n), L"true") == 0)
					pItem->setFlags(HeaderItem::FLAG_SHOWALWAYS,
						HeaderItem::FLAG_SHOWALWAYS);
			}
			else if (wcscmp(pwszAttrLocalName, L"number") == 0) {
				setNumber(pItem.get(), attributes.getValue(n));
			}
			else if (wcscmp(pwszAttrLocalName, L"background") == 0) {
				std::auto_ptr<Template> pBackground(parseTemplate(attributes.getValue(n)));
				if (!pBackground.get())
					return false;
				pItem->setBackground(pBackground);
			}
			else {
				return false;
			}
		}
		
		pAttachmentSelectionModel_ = pItem.get();
		pCurrentItem_ = pItem.get();
		pCurrentLine_->addItem(std::auto_ptr<LineLayoutItem>(pItem));
		
		state_ = STATE_ITEM;
	}
	else {
		return false;
	}
	
	return true;
}

bool qm::HeaderWindowContentHandler::endElement(const WCHAR* pwszNamespaceURI,
												const WCHAR* pwszLocalName,
												const WCHAR* pwszQName)
{
	if (wcscmp(pwszLocalName, L"header") == 0) {
		assert(state_ == STATE_HEADER);
		state_ = STATE_ROOT;
	}
	else if (wcscmp(pwszLocalName, L"line") == 0) {
		assert(state_ == STATE_LINE);
		assert(pCurrentLine_);
		pCurrentLine_->fixup();
		pCurrentLine_ = 0;
		state_ = STATE_HEADER;
	}
	else if (wcscmp(pwszLocalName, L"static") == 0 ||
		wcscmp(pwszLocalName, L"edit") == 0 ||
		wcscmp(pwszLocalName, L"attachment") == 0) {
		assert(state_ == STATE_ITEM);
		assert(pCurrentItem_);
		
		std::auto_ptr<Template> pValue(parseTemplate(buffer_.getCharArray()));
		if (!pValue.get())
			return false;
		pCurrentItem_->setValue(pValue);
		buffer_.remove();
		
		pCurrentItem_ = 0;
		state_ = STATE_LINE;
	}
	else {
		return false;
	}
	
	return true;
}

bool qm::HeaderWindowContentHandler::characters(const WCHAR* pwsz,
												size_t nStart,
												size_t nLength)
{
	if (state_ == STATE_ITEM) {
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

void qm::HeaderWindowContentHandler::setWidth(LineLayoutItem* pItem,
											  const WCHAR* pwszWidth)
{
	double dWidth = 0;
	LineLayoutItem::Unit unit;
	if (LineLayoutItem::parseWidth(pwszWidth, &dWidth, &unit))
		pItem->setWidth(dWidth, unit);
}

void qm::HeaderWindowContentHandler::setNumber(HeaderItem* pItem,
											   const WCHAR* pwszNumber)
{
	WCHAR* pEnd = 0;
	unsigned int nNumber = wcstol(pwszNumber, &pEnd, 10);
	if (!*pEnd)
		pItem->setNumber(nNumber);
}

std::auto_ptr<Template> qm::HeaderWindowContentHandler::parseTemplate(const WCHAR* pwszTemplate)
{
	StringReader reader(pwszTemplate, false);
	std::auto_ptr<Template> pTemplate(TemplateParser().parse(&reader, 0));
	if (!pTemplate.get())
		return std::auto_ptr<Template>();
	if (!reader.close())
		return std::auto_ptr<Template>();
	return pTemplate;
}
