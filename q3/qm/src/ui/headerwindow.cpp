/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmaccount.h>
#include <qmapplication.h>
#include <qmextensions.h>
#include <qmmessage.h>
#include <qmmessageholder.h>
#include <qmmessagewindow.h>
#include <qmtemplate.h>

#include <qsconv.h>
#include <qsdevicecontext.h>
#include <qserror.h>
#include <qsinit.h>
#include <qsnew.h>
#include <qssax.h>
#include <qsstl.h>
#include <qsuiutil.h>

#include <algorithm>

#include <tchar.h>

#include "headerwindow.h"

#pragma warning(disable:4786)

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * HeaderWindowImpl
 *
 */

class qm::HeaderWindowImpl
{
public:
	enum {
		ID_HEADER_ITEM	= 1000
	};

public:
	QSTATUS load();
	QSTATUS create();

public:
	HeaderWindow* pThis_;
	Document* pDocument_;
	Profile* pProfile_;
	
	HFONT hfont_;
	HFONT hfontBold_;
	HBRUSH hbrBackground_;
	LineLayout* pLayout_;
};

QSTATUS qm::HeaderWindowImpl::load()
{
	DECLARE_QSTATUS();
	
	status = newQsObject(&pLayout_);
	CHECK_QSTATUS();
	
	string_ptr<WSTRING> wstrPath;
	status = Application::getApplication().getProfilePath(
		Extensions::HEADER, &wstrPath);
	CHECK_QSTATUS();
	
	XMLReader reader(&status);
	CHECK_QSTATUS();
	HeaderWindowContentHandler contentHandler(pLayout_, &status);
	CHECK_QSTATUS();
	reader.setContentHandler(&contentHandler);
	W2T(wstrPath.get(), ptszPath);
	if (::GetFileAttributes(ptszPath) != 0xffffffff) {
		status = reader.parse(wstrPath.get());
		CHECK_QSTATUS();
	}
	else {
		// TODO
		// Load default XML from resource.
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::HeaderWindowImpl::create()
{
	DECLARE_QSTATUS();
	
	status = load();
	CHECK_QSTATUS();
	
	std::pair<HFONT, HFONT> fonts(hfont_, hfontBold_);
	UINT nId = ID_HEADER_ITEM;
	status = pLayout_->create(pThis_, fonts, &nId);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * HeaderWindow
 *
 */

qm::HeaderWindow::HeaderWindow(Profile* pProfile, QSTATUS* pstatus) :
	WindowBase(true, pstatus),
	DefaultWindowHandler(pstatus),
	pImpl_(0)
{
	if (*pstatus != QSTATUS_SUCCESS)
		return;
	
	DECLARE_QSTATUS();
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	pImpl_->pThis_ = this;
	pImpl_->pDocument_ = 0;
	pImpl_->pProfile_ = pProfile;
	pImpl_->hfont_ = 0;
	pImpl_->hfontBold_ = 0;
	pImpl_->hbrBackground_ = 0;
	pImpl_->pLayout_ = 0;
	
	setWindowHandler(this, false);
}

qm::HeaderWindow::~HeaderWindow()
{
	if (pImpl_) {
		delete pImpl_->pLayout_;
		delete pImpl_;
		pImpl_ = 0;
	}
}

int qm::HeaderWindow::getHeight() const
{
	return pImpl_->pLayout_->getHeight();
}

QSTATUS qm::HeaderWindow::setMessage(const TemplateContext& context)
{
	DECLARE_QSTATUS();
	
	for (unsigned int n = 0; n < pImpl_->pLayout_->getLineCount(); ++n) {
		HeaderLine* pLine = static_cast<HeaderLine*>(
			pImpl_->pLayout_->getLine(n));
		status = pLine->setMessage(context);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::HeaderWindow::layout()
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

QSTATUS qm::HeaderWindow::getWindowClass(WNDCLASS* pwc)
{
	DECLARE_QSTATUS();
	
	status = DefaultWindowHandler::getWindowClass(pwc);
	CHECK_QSTATUS();
	
	pwc->hbrBackground = reinterpret_cast<HBRUSH>(COLOR_3DFACE + 1);
	
	return QSTATUS_SUCCESS;
}

LRESULT qm::HeaderWindow::windowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	BEGIN_MESSAGE_HANDLER()
		HANDLE_CREATE()
		HANDLE_CTLCOLORSTATIC()
		HANDLE_DESTROY()
		HANDLE_SIZE()
	END_MESSAGE_HANDLER()
	return DefaultWindowHandler::windowProc(uMsg, wParam, lParam);
}

LRESULT qm::HeaderWindow::onCreate(CREATESTRUCT* pCreateStruct)
{
	if (DefaultWindowHandler::onCreate(pCreateStruct) == -1)
		return -1;
	
	DECLARE_QSTATUS();
	
	HeaderWindowCreateContext* pContext =
		static_cast<HeaderWindowCreateContext*>(pCreateStruct->lpCreateParams);
	pImpl_->pDocument_ = pContext->pDocument_;
	
	status = UIUtil::createFontFromProfile(pImpl_->pProfile_,
		L"HeaderWindow", false, &pImpl_->hfont_);
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
	
	status = pImpl_->create();
	CHECK_QSTATUS_VALUE(-1);
	
	return 0;
}

LRESULT qm::HeaderWindow::onCtlColorStatic(HDC hdc, HWND hwnd)
{
	DefaultWindowHandler::onCtlColorStatic(hdc, hwnd);
	DeviceContext dc(hdc);
	dc.setTextColor(::GetSysColor(COLOR_WINDOWTEXT));
	dc.setBkColor(::GetSysColor(COLOR_3DFACE));
	return reinterpret_cast<LRESULT>(pImpl_->hbrBackground_);
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

LRESULT qm::HeaderWindow::onSize(UINT nFlags, int cx, int cy)
{
	layout();
	
	return DefaultWindowHandler::onSize(nFlags, cx, cy);
}


/****************************************************************************
 *
 * HeaderLine
 *
 */

qm::HeaderLine::HeaderLine(QSTATUS* pstatus) :
	LineLayoutLine(pstatus)
{
}

qm::HeaderLine::~HeaderLine()
{
	std::for_each(listHide_.begin(), listHide_.end(),
		unary_compose_f_gx(
			string_free<WSTRING>(),
			std::select1st<HideList::value_type>()));
}

QSTATUS qm::HeaderLine::setMessage(const TemplateContext& context)
{
	DECLARE_QSTATUS();
	
	for (unsigned int n = 0; n < getItemCount(); ++n) {
		status = static_cast<HeaderItem*>(getItem(n))->setMessage(context);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::HeaderLine::setHideIfEmpty(const WCHAR* pwszName)
{
	DECLARE_QSTATUS();
	
	const WCHAR* p = pwszName;
	while (p) {
		const WCHAR* pEnd = wcschr(p, L',');
		string_ptr<WSTRING> wstr;
		if (pEnd)
			wstr.reset(allocWString(p, pEnd - p));
		else
			wstr.reset(allocWString(p));
		if (!wstr.get())
			return QSTATUS_OUTOFMEMORY;
		status = STLWrapper<HideList>(listHide_).push_back(
			HideList::value_type(wstr.get(), 0));
		CHECK_QSTATUS();
		wstr.release();
		
		p = pEnd ? pEnd + 1 : 0;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::HeaderLine::fixup()
{
	HideList::iterator itH = listHide_.begin();
	while (itH != listHide_.end()) {
		unsigned int nItem = 0;
		while (nItem < getItemCount()) {
			HeaderItem* pItem = static_cast<HeaderItem*>(getItem(nItem));
			const WCHAR* pwszName = pItem->getName();
			if (pwszName && wcscmp(pwszName, (*itH).first) == 0) {
				(*itH).second = pItem;
				break;
			}
			++nItem;
		}
		
		++itH;
	}
	
	return QSTATUS_SUCCESS;
}

bool qm::HeaderLine::isHidden() const
{
	HideList::const_iterator itH = listHide_.begin();
	while (itH != listHide_.end()) {
		if ((*itH).second && !(*itH).second->isEmptyValue())
			break;
		++itH;
	}
	return !listHide_.empty() && itH == listHide_.end();
}


/****************************************************************************
 *
 * HeaderItem
 *
 */

qm::HeaderItem::HeaderItem(QSTATUS* pstatus) :
	LineLayoutItem(pstatus),
	wstrName_(0),
	wstrValue_(0),
	nFlags_(0),
	pTemplate_(0)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qm::HeaderItem::~HeaderItem()
{
	freeWString(wstrName_);
	freeWString(wstrValue_);
	delete pTemplate_;
}

const WCHAR* qm::HeaderItem::getName() const
{
	return wstrName_;
}

QSTATUS qm::HeaderItem::setName(const WCHAR* pwszName)
{
	wstrName_ = allocWString(pwszName);
	return wstrName_ ? QSTATUS_SUCCESS : QSTATUS_OUTOFMEMORY;
}

void qm::HeaderItem::setFlags(unsigned int nFlags, unsigned int nMask)
{
	nFlags_ &= ~nMask;
	nFlags_ |= nFlags & nMask;
}

QSTATUS qm::HeaderItem::addValue(const WCHAR* pwszValue, size_t nLen)
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

QSTATUS qm::HeaderItem::fixupValue()
{
	DECLARE_QSTATUS();
	
	if (wstrValue_) {
		TemplateParser parser(&status);
		CHECK_QSTATUS();
		StringReader reader(wstrValue_, &status);
		CHECK_QSTATUS();
		status = parser.parse(&reader, &pTemplate_);
		CHECK_QSTATUS();
		// TODO
		status = reader.close();
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::HeaderItem::copy()
{
	return QSTATUS_SUCCESS;
}

QSTATUS qm::HeaderItem::canCopy(bool* pbCan)
{
	assert(pbCan);
	*pbCan = false;
	return QSTATUS_SUCCESS;
}

QSTATUS qm::HeaderItem::selectAll()
{
	return QSTATUS_SUCCESS;
}

QSTATUS qm::HeaderItem::canSelectAll(bool* pbCan)
{
	assert(pbCan);
	*pbCan = false;
	return QSTATUS_SUCCESS;
}

QSTATUS qm::HeaderItem::getValue(
	const TemplateContext& context, WSTRING* pwstrValue) const
{
	assert(pwstrValue);
	
	DECLARE_QSTATUS();
	
	if (context.getMessageHolder() || (nFlags_ & FLAG_SHOWALWAYS)) {
		status = pTemplate_->getValue(context, pwstrValue);
		CHECK_QSTATUS();
	}
	else {
		*pwstrValue = allocWString(L"");
		if (!*pwstrValue)
			return QSTATUS_OUTOFMEMORY;
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * TextHeaderItem
 *
 */

qm::TextHeaderItem::TextHeaderItem(QSTATUS* pstatus) :
	HeaderItem(pstatus),
	nStyle_(STYLE_NORMAL),
	hwnd_(0)
{
}

qm::TextHeaderItem::~TextHeaderItem()
{
}

QSTATUS qm::TextHeaderItem::setStyle(const WCHAR* pwszStyle)
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

unsigned int qm::TextHeaderItem::getHeight(unsigned int nFontHeight) const
{
	return nFontHeight;
}

QSTATUS qm::TextHeaderItem::create(WindowBase* pParent,
	const std::pair<HFONT, HFONT>& fonts, UINT nId)
{
	assert(!hwnd_);
	
	hwnd_ = ::CreateWindow(getWindowClassName(), 0,
		WS_CHILD | WS_VISIBLE | getWindowStyle(), 0, 0, 0, 0,
		pParent->getHandle(), reinterpret_cast<HMENU>(nId),
		Init::getInit().getInstanceHandle(), 0);
	if (!hwnd_)
		return QSTATUS_FAIL;
	
	Window(hwnd_).setFont((nStyle_ & STYLE_BOLD) ? fonts.second : fonts.first);
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::TextHeaderItem::destroy()
{
	return QSTATUS_SUCCESS;
}

QSTATUS qm::TextHeaderItem::layout(const RECT& rect, unsigned int nFontHeight)
{
	unsigned int nHeight = getHeight(nFontHeight);
	Window(hwnd_).setWindowPos(0, rect.left,
		rect.top + ((rect.bottom - rect.top) - nHeight)/2,
		rect.right - rect.left, nHeight,
		SWP_NOZORDER | SWP_NOACTIVATE);
	return QSTATUS_SUCCESS;
}

QSTATUS qm::TextHeaderItem::show(bool bShow)
{
	Window(hwnd_).showWindow(bShow ? SW_SHOW : SW_HIDE);
	return QSTATUS_SUCCESS;
}

QSTATUS qm::TextHeaderItem::setMessage(const TemplateContext& context)
{
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrValue;
	status = getValue(context, &wstrValue);
	CHECK_QSTATUS();
	
	Window(hwnd_).setWindowText(wstrValue.get());
	
	return QSTATUS_SUCCESS;
}

bool qm::TextHeaderItem::isEmptyValue() const
{
	return Window(hwnd_).getWindowTextLength() == 0;
}

bool qm::TextHeaderItem::isActive() const
{
	return Window(hwnd_).hasFocus();
}

HWND qm::TextHeaderItem::getHandle() const
{
	return hwnd_;
}


/****************************************************************************
 *
 * StaticHeaderItem
 *
 */

qm::StaticHeaderItem::StaticHeaderItem(QSTATUS* pstatus) :
	TextHeaderItem(pstatus)
{
}

qm::StaticHeaderItem::~StaticHeaderItem()
{
}

const TCHAR* qm::StaticHeaderItem::getWindowClassName() const
{
	return _T("STATIC");
}

UINT qm::StaticHeaderItem::getWindowStyle() const
{
	return SS_LEFTNOWORDWRAP | SS_NOPREFIX;
}


/****************************************************************************
 *
 * EditHeaderItem
 *
 */

qm::EditHeaderItem::EditHeaderItem(QSTATUS* pstatus) :
	TextHeaderItem(pstatus)
{
}

qm::EditHeaderItem::~EditHeaderItem()
{
}

const TCHAR* qm::EditHeaderItem::getWindowClassName() const
{
	return _T("EDIT");
}

UINT qm::EditHeaderItem::getWindowStyle() const
{
	return ES_READONLY | ES_AUTOHSCROLL;
}


QSTATUS qm::EditHeaderItem::copy()
{
	Window(getHandle()).sendMessage(WM_COPY);
	return QSTATUS_SUCCESS;
}

QSTATUS qm::EditHeaderItem::canCopy(bool* pbCan)
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

QSTATUS qm::EditHeaderItem::selectAll()
{
	Window(getHandle()).sendMessage(EM_SETSEL, 0, -1);
	return QSTATUS_SUCCESS;
}

QSTATUS qm::EditHeaderItem::canSelectAll(bool* pbCan)
{
	assert(pbCan);
	*pbCan = true;
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * AttachmentHeaderItem
 *
 */

qm::AttachmentHeaderItem::AttachmentHeaderItem(QSTATUS* pstatus) :
	HeaderItem(pstatus),
	hwnd_(0)
{
}

qm::AttachmentHeaderItem::~AttachmentHeaderItem()
{
}

unsigned int qm::AttachmentHeaderItem::getHeight(unsigned int nFontHeight) const
{
	return nFontHeight + 7;
}

QSTATUS qm::AttachmentHeaderItem::create(WindowBase* pParent,
	const std::pair<HFONT, HFONT>& fonts, UINT nId)
{
	assert(!hwnd_);
	
	hwnd_ = ::CreateWindow(WC_LISTVIEW, 0,
		WS_CHILD | WS_VISIBLE | LVS_SMALLICON | LVS_SHAREIMAGELISTS,
		0, 0, 0, 0, pParent->getHandle(), reinterpret_cast<HMENU>(nId),
		Init::getInit().getInstanceHandle(), 0);
	if (!hwnd_)
		return QSTATUS_FAIL;
	
	ListView_SetBkColor(hwnd_, ::GetSysColor(COLOR_3DFACE));
	ListView_SetTextBkColor(hwnd_, ::GetSysColor(COLOR_3DFACE));
	SHFILEINFO info = { 0 };
	HIMAGELIST hImageList = reinterpret_cast<HIMAGELIST>(::SHGetFileInfo(
		_T("dummy.txt"), FILE_ATTRIBUTE_NORMAL, &info, sizeof(info),
		SHGFI_USEFILEATTRIBUTES | SHGFI_SYSICONINDEX | SHGFI_SMALLICON));
	ListView_SetImageList(hwnd_, hImageList, LVSIL_SMALL);
	
	Window(hwnd_).setFont(fonts.first);
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::AttachmentHeaderItem::destroy()
{
	return QSTATUS_SUCCESS;
}

QSTATUS qm::AttachmentHeaderItem::layout(
	const RECT& rect, unsigned int nFontHeight)
{
	Window(hwnd_).setWindowPos(0, rect.left, rect.top,
		rect.right - rect.left, rect.bottom - rect.top,
		SWP_NOZORDER | SWP_NOACTIVATE);
	return QSTATUS_SUCCESS;
}

QSTATUS qm::AttachmentHeaderItem::show(bool bShow)
{
	Window(hwnd_).showWindow(bShow ? SW_SHOW : SW_HIDE);
	return QSTATUS_SUCCESS;
}

QSTATUS qm::AttachmentHeaderItem::setMessage(const TemplateContext& context)
{
	DECLARE_QSTATUS();
	
	ListView_DeleteAllItems(hwnd_);
	
	MessageHolderBase* pmh = context.getMessageHolder();
	if (pmh) {
		Message* pMessage = context.getMessage();
		status = pmh->getMessage(Account::GETMESSAGEFLAG_TEXT, 0, pMessage);
		CHECK_QSTATUS();
		
		AttachmentParser parser(*pMessage);
		AttachmentParser::AttachmentList list;
		AttachmentParser::AttachmentListFree free(list);
		status = parser.getAttachments(&list);
		CHECK_QSTATUS();
		AttachmentParser::AttachmentList::size_type n = 0;
		while (n < list.size()) {
			W2T(list[n].first, ptszName);
			SHFILEINFO info = { 0 };
			::SHGetFileInfo(ptszName, FILE_ATTRIBUTE_NORMAL, &info, sizeof(info),
				SHGFI_USEFILEATTRIBUTES | SHGFI_SYSICONINDEX | SHGFI_SMALLICON);
			LVITEM item = {
				LVIF_TEXT | LVIF_IMAGE,
				n,
				0,
				0,
				0,
				const_cast<LPTSTR>(ptszName),
				0,
				info.iIcon
			};
			ListView_InsertItem(hwnd_, &item);
			++n;
		}
	}
	
	return QSTATUS_SUCCESS;
}

bool qm::AttachmentHeaderItem::isEmptyValue() const
{
	return ListView_GetItemCount(hwnd_) == 0;
}

bool qm::AttachmentHeaderItem::isActive() const
{
	return Window(hwnd_).hasFocus();
}


/****************************************************************************
 *
 * HeaderWindowContentHandler
 *
 */

qm::HeaderWindowContentHandler::HeaderWindowContentHandler(
	LineLayout* pLayout, QSTATUS* pstatus) :
	DefaultHandler(pstatus),
	pLayout_(pLayout),
	pCurrentLine_(0),
	pCurrentItem_(0),
	state_(STATE_ROOT)
{
}

qm::HeaderWindowContentHandler::~HeaderWindowContentHandler()
{
}

QSTATUS qm::HeaderWindowContentHandler::startElement(
	const WCHAR* pwszNamespaceURI, const WCHAR* pwszLocalName,
	const WCHAR* pwszQName, const Attributes& attributes)
{
	DECLARE_QSTATUS();
	
	if (wcscmp(pwszLocalName, L"header") == 0) {
		if (state_ != STATE_ROOT)
			return QSTATUS_FAIL;
		state_ = STATE_HEADER;
	}
	else if (wcscmp(pwszLocalName, L"line") == 0) {
		if (state_ != STATE_HEADER)
			return QSTATUS_FAIL;
		
		std::auto_ptr<HeaderLine> pHeaderLine;
		status = newQsObject(&pHeaderLine);
		CHECK_QSTATUS();
		
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrLocalName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrLocalName, L"hideIfEmpty") == 0) {
				status = pHeaderLine->setHideIfEmpty(attributes.getValue(n));
				CHECK_QSTATUS();
			}
			else {
				return QSTATUS_FAIL;
			}
		}
		
		status = pLayout_->addLine(pHeaderLine.get());
		CHECK_QSTATUS();
		pCurrentLine_ = pHeaderLine.release();
		
		state_ = STATE_LINE;
	}
	else if (wcscmp(pwszLocalName, L"static") == 0 ||
		wcscmp(pwszLocalName, L"edit") == 0) {
		if (state_ != STATE_LINE)
			return QSTATUS_FAIL;
		
		assert(pCurrentLine_);
		
		std::auto_ptr<TextHeaderItem> pItem;
		if (wcscmp(pwszLocalName, L"static") == 0) {
			std::auto_ptr<StaticHeaderItem> p;
			status = newQsObject(&p);
			CHECK_QSTATUS();
			pItem.reset(p.release());
		}
		else {
			std::auto_ptr<EditHeaderItem> p;
			status = newQsObject(&p);
			CHECK_QSTATUS();
			pItem.reset(p.release());
		}
		
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrLocalName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrLocalName, L"name") == 0) {
				status = pItem->setName(attributes.getValue(n));
				CHECK_QSTATUS();
			}
			else if (wcscmp(pwszAttrLocalName, L"width") == 0) {
				status = pItem->setWidth(attributes.getValue(n));
				CHECK_QSTATUS();
			}
			else if (wcscmp(pwszAttrLocalName, L"style") == 0) {
				status = pItem->setStyle(attributes.getValue(n));
				CHECK_QSTATUS();
			}
			else if (wcscmp(pwszAttrLocalName, L"showAlways") == 0) {
				if (wcscmp(attributes.getValue(n), L"true") == 0)
					pItem->setFlags(HeaderItem::FLAG_SHOWALWAYS,
						HeaderItem::FLAG_SHOWALWAYS);
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
	else if (wcscmp(pwszLocalName, L"attachment") == 0) {
		if (state_ != STATE_LINE)
			return QSTATUS_FAIL;
		
		assert(pCurrentLine_);
		
		std::auto_ptr<AttachmentHeaderItem> pItem;
		status = newQsObject(&pItem);
		CHECK_QSTATUS();
		
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrLocalName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrLocalName, L"name") == 0) {
				status = pItem->setName(attributes.getValue(n));
				CHECK_QSTATUS();
			}
			else if (wcscmp(pwszAttrLocalName, L"width") == 0) {
				status = pItem->setWidth(attributes.getValue(n));
				CHECK_QSTATUS();
			}
			else if (wcscmp(pwszAttrLocalName, L"showAlways") == 0) {
				if (wcscmp(attributes.getValue(n), L"true") == 0)
					pItem->setFlags(HeaderItem::FLAG_SHOWALWAYS,
						HeaderItem::FLAG_SHOWALWAYS);
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

QSTATUS qm::HeaderWindowContentHandler::endElement(
	const WCHAR* pwszNamespaceURI, const WCHAR* pwszLocalName,
	const WCHAR* pwszQName)
{
	DECLARE_QSTATUS();
	
	if (wcscmp(pwszLocalName, L"header") == 0) {
		assert(state_ == STATE_HEADER);
		state_ = STATE_ROOT;
	}
	else if (wcscmp(pwszLocalName, L"line") == 0) {
		assert(state_ == STATE_LINE);
		assert(pCurrentLine_);
		status = pCurrentLine_->fixup();
		CHECK_QSTATUS();
		pCurrentLine_ = 0;
		state_ = STATE_HEADER;
	}
	else if (wcscmp(pwszLocalName, L"static") == 0 ||
		wcscmp(pwszLocalName, L"edit") == 0 ||
		wcscmp(pwszLocalName, L"attachment") == 0) {
		assert(state_ == STATE_ITEM);
		assert(pCurrentItem_);
		status = pCurrentItem_->fixupValue();
		CHECK_QSTATUS();
		pCurrentItem_ = 0;
		state_ = STATE_LINE;
	}
	else {
		return QSTATUS_FAIL;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::HeaderWindowContentHandler::characters(
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
