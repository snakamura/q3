/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmaccount.h>
#include <qmapplication.h>
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
#include "../model/dataobject.h"
#include "../model/uri.h"

#pragma warning(disable:4355)
#pragma warning(disable:4786)

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
	bool load(MenuManager* pMenuManager);
	bool create(MenuManager* pMenuManager);

public:
	virtual void registerCallback(TextHeaderItem* pItem,
								  TextHeaderItemCallback* pCallback);

public:
	typedef std::vector<std::pair<TextHeaderItem*, TextHeaderItemCallback*> > TextHeaderItemCallbackList;

public:
	HeaderWindow* pThis_;
	Document* pDocument_;
	Profile* pProfile_;
	
	HFONT hfont_;
	HFONT hfontBold_;
	HBRUSH hbrBackground_;
	std::auto_ptr<LineLayout> pLayout_;
	AttachmentSelectionModel* pAttachmentSelectionModel_;
	TextHeaderItemCallbackList listTextHeaderItemCallback_;
};

bool qm::HeaderWindowImpl::load(MenuManager* pMenuManager)
{
	pLayout_.reset(new LineLayout());
	
	wstring_ptr wstrPath(Application::getApplication().getProfilePath(FileNames::HEADER_XML));
	
	XMLReader reader;
	HeaderWindowContentHandler contentHandler(pLayout_.get(), pMenuManager);
	reader.setContentHandler(&contentHandler);
	if (!reader.parse(wstrPath.get()))
		return false;
	pAttachmentSelectionModel_ = contentHandler.getAttachmentSelectionModel();
	
	return true;
}

bool qm::HeaderWindowImpl::create(MenuManager* pMenuManager)
{
	if (!load(pMenuManager))
		return false;
	
	std::pair<HFONT, HFONT> fonts(hfont_, hfontBold_);
	UINT nId = ID_HEADER_ITEM;
	if (!pLayout_->create(pThis_, fonts, &nId, this))
		return false;
	
	return true;
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
	pImpl_->pDocument_ = 0;
	pImpl_->pProfile_ = pProfile;
	pImpl_->hfont_ = 0;
	pImpl_->hfontBold_ = 0;
	pImpl_->hbrBackground_ = 0;
	pImpl_->pAttachmentSelectionModel_ = 0;
	
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

AttachmentSelectionModel* qm::HeaderWindow::getAttachmentSelectionModel() const
{
	return pImpl_->pAttachmentSelectionModel_;
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
	pImpl_->pDocument_ = pContext->pDocument_;
	
	pImpl_->hfont_ = UIUtil::createFontFromProfile(
		pImpl_->pProfile_, L"HeaderWindow", false);
	LOGFONT lf;
	::GetObject(pImpl_->hfont_, sizeof(lf), &lf);
	lf.lfWeight = FW_BOLD;
	pImpl_->hfontBold_ = ::CreateFontIndirect(&lf);
	
	wstring_ptr wstrClassName(getClassName());
	W2T(wstrClassName.get(), ptszClassName);
	WNDCLASS wc;
	::GetClassInfo(getInstanceHandle(), ptszClassName, &wc);
	pImpl_->hbrBackground_ = wc.hbrBackground;
	
	if (!pImpl_->create(pContext->pMenuManager_))
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
		std::bind2nd(
			binary_compose_f_gx_hy(
				std::equal_to<HWND>(),
				unary_compose_f_gx(
					std::mem_fun(&TextHeaderItem::getHandle),
					std::select1st<List::value_type>()),
				std::identity<HWND>()),
			hwnd));
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
		unary_compose_f_gx(
			string_free<WSTRING>(),
			std::select1st<HideList::value_type>()));
}

void qm::HeaderLine::setMessage(const TemplateContext* pContext)
{
	if (pContext && pClass_.get())
		bHide_ = !pClass_->match(pContext->getAccount()->getClass());
	else
		bHide_ = false;
	
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
	nFlags_(0)
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
	if (context.getMessageHolder() || (nFlags_ & FLAG_SHOWALWAYS)) {
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
	return Window(hwnd_).deferWindowPos(hdwp, 0, rect.left,
		rect.top + ((rect.bottom - rect.top) - nHeight)/2,
		rect.right - rect.left, nHeight, SWP_NOZORDER | SWP_NOACTIVATE);
}

void qm::TextHeaderItem::show(bool bShow)
{
	Window(hwnd_).showWindow(bShow ? SW_SHOW : SW_HIDE);
}

void qm::TextHeaderItem::setMessage(const TemplateContext* pContext)
{
	if (pContext) {
		wstring_ptr wstrValue(getValue(*pContext));
		updateColor(*pContext);
		Window(hwnd_).setWindowText(wstrValue.get() ? wstrValue.get() : L"");
	}
	else {
		Window(hwnd_).setWindowText(L"");
	}
}

bool qm::TextHeaderItem::isEmptyValue() const
{
	return Window(hwnd_).getWindowTextLength() == 0;
}

bool qm::TextHeaderItem::isActive() const
{
	return Window(hwnd_).hasFocus();
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

void qm::TextHeaderItem::updateColor(const TemplateContext& context)
{
	COLORREF crBackground = 0xffffffff;
	if (context.getMessageHolder() || (getFlags() & FLAG_SHOWALWAYS)) {
		if (pBackground_.get()) {
			wstring_ptr wstrBackground;
			if (pBackground_->getValue(context, &wstrBackground) == Template::RESULT_SUCCESS)
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

qm::EditHeaderItem::EditHeaderItem()
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


/****************************************************************************
 *
 * AttachmentHeaderItem
 *
 */

qm::AttachmentHeaderItem::AttachmentHeaderItem(MenuManager* pMenuManager) :
	wnd_(this),
	pMenuManager_(pMenuManager),
	pParent_(0),
	pDocument_(0),
	nSecurityMode_(SECURITYMODE_NONE)
{
}

qm::AttachmentHeaderItem::~AttachmentHeaderItem()
{
}

unsigned int qm::AttachmentHeaderItem::getHeight(unsigned int nWidth,
												 unsigned int nFontHeight) const
{
	return QSMIN(ListView_GetItemCount(wnd_.getHandle())*(nFontHeight + 2), nFontHeight*4 + 7);
}

bool qm::AttachmentHeaderItem::create(WindowBase* pParent,
									  const std::pair<HFONT, HFONT>& fonts,
									  UINT nId,
									  void* pParam)
{
	assert(!wnd_.getHandle());
	
	pParent_ = pParent;
	
	DWORD dwStyle = WS_CHILD | WS_VISIBLE | LVS_SMALLICON | LVS_NOLABELWRAP |
		LVS_SHAREIMAGELISTS | LVS_ALIGNLEFT | LVS_AUTOARRANGE;
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
	return wnd_.deferWindowPos(hdwp, 0, rect.left, rect.top,
		rect.right - rect.left, rect.bottom - rect.top,
		SWP_NOZORDER | SWP_NOACTIVATE);
}

void qm::AttachmentHeaderItem::show(bool bShow)
{
	wnd_.showWindow(bShow ? SW_SHOW : SW_HIDE);
}

void qm::AttachmentHeaderItem::setMessage(const TemplateContext* pContext)
{
	HWND hwnd = wnd_.getHandle();
	
	clear();
	
	if (pContext) {
		pDocument_ = pContext->getDocument();
		nSecurityMode_ = pContext->getSecurityMode();
		
		MessageHolderBase* pmh = pContext->getMessageHolder();
		if (pmh) {
			Message* pMessage = pContext->getMessage();
			if (pmh->getMessage(Account::GETMESSAGEFLAG_TEXT,
				0, nSecurityMode_, pMessage)) {
				AttachmentParser parser(*pMessage);
				AttachmentParser::AttachmentList list;
				AttachmentParser::AttachmentListFree free(list);
				parser.getAttachments(true, &list);
				for (AttachmentParser::AttachmentList::size_type n = 0; n < list.size(); ++n) {
					W2T(list[n].first, ptszName);
					std::auto_ptr<URI> pURI(new URI(pmh->getMessageHolder(),
						pMessage, list[n].second, URIFragment::TYPE_BODY));
					
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
						reinterpret_cast<LPARAM>(pURI.get())
					};
					ListView_InsertItem(hwnd, &item);
					pURI.release();
				}
				
				wnd_.enableWindow(!parser.isAttachmentDeleted());
			}
		}
	}
}

bool qm::AttachmentHeaderItem::isEmptyValue() const
{
	return ListView_GetItemCount(wnd_.getHandle()) == 0;
}

bool qm::AttachmentHeaderItem::isActive() const
{
	return wnd_.hasFocus();
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
	assert(pDocument_);
	
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
		listURI.push_back(new URI(*reinterpret_cast<URI*>(item.lParam)));
		nItem = ListView_GetNextItem(hwnd, nItem, LVNI_ALL | LVNI_SELECTED);
	}
	
	std::auto_ptr<URIDataObject> p(new URIDataObject(
		pDocument_, nSecurityMode_, listURI));
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
	
	pDocument_ = 0;
	nSecurityMode_ = SECURITYMODE_NONE;
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
	END_MESSAGE_HANDLER()
	return DefaultWindowHandler::windowProc(uMsg, wParam, lParam);
}

LRESULT qm::AttachmentHeaderItem::AttachmentWindow::onContextMenu(HWND hwnd,
																  const POINT& pt)
{
	HMENU hmenu = pItem_->pMenuManager_->getMenu(L"attachment", false, false);
	if (hmenu) {
		UINT nFlags = TPM_LEFTALIGN | TPM_TOPALIGN;
#ifndef _WIN32_WCE
		nFlags |= TPM_LEFTBUTTON | TPM_RIGHTBUTTON;
#endif
		::TrackPopupMenu(hmenu, nFlags, pt.x, pt.y, 0, getParentFrame(), 0);
	}
	return 0;
}

LRESULT qm::AttachmentHeaderItem::AttachmentWindow::onLButtonDown(UINT nFlags,
																  const POINT& pt)
{
#if defined _WIN32_WCE && _WIN32_WCE >= 300 && _WIN32_WCE < 400 && defined _WIN32_WCE_PSPC
	if (tapAndHold(pt))
		return 0;
#endif
	return DefaultWindowHandler::onLButtonDown(nFlags, pt);
}

LRESULT qm::AttachmentHeaderItem::AttachmentWindow::onLButtonDblClk(UINT nFlags,
																	const POINT& pt)
{
	Window(getParentFrame()).postMessage(WM_COMMAND,
		MAKEWPARAM(IDM_ATTACHMENT_OPEN, 0), 0);
	return DefaultWindowHandler::onLButtonDblClk(nFlags, pt);
}


/****************************************************************************
 *
 * HeaderWindowContentHandler
 *
 */

qm::HeaderWindowContentHandler::HeaderWindowContentHandler(LineLayout* pLayout,
														   MenuManager* pMenuManager) :
	pLayout_(pLayout),
	pMenuManager_(pMenuManager),
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
		pLayout_->addLine(pHeaderLine);
		
		state_ = STATE_LINE;
	}
	else if (wcscmp(pwszLocalName, L"static") == 0 ||
		wcscmp(pwszLocalName, L"edit") == 0) {
		if (state_ != STATE_LINE)
			return false;
		
		assert(pCurrentLine_);
		
		std::auto_ptr<TextHeaderItem> pItem;
		if (wcscmp(pwszLocalName, L"static") == 0)
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
			else if (wcscmp(pwszAttrLocalName, L"showAlways") == 0) {
				if (wcscmp(attributes.getValue(n), L"true") == 0)
					pItem->setFlags(HeaderItem::FLAG_SHOWALWAYS,
						HeaderItem::FLAG_SHOWALWAYS);
			}
			else if (wcscmp(pwszAttrLocalName, L"background") == 0) {
				StringReader reader(attributes.getValue(n), false);
				std::auto_ptr<Template> pBackground(TemplateParser().parse(&reader));
				if (!pBackground.get())
					return false;
				if (!reader.close())
					return false;
				
				pItem->setBackground(pBackground);
			}
			else {
				return false;
			}
		}
		
		pCurrentItem_ = pItem.get();
		pCurrentLine_->addItem(pItem);
		
		state_ = STATE_ITEM;
	}
	else if (wcscmp(pwszLocalName, L"attachment") == 0) {
		if (state_ != STATE_LINE)
			return false;
		
		assert(pCurrentLine_);
		
		std::auto_ptr<AttachmentHeaderItem> pItem(
			new AttachmentHeaderItem(pMenuManager_));
		
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
			else {
				return false;
			}
		}
		
		pAttachmentSelectionModel_ = pItem.get();
		pCurrentItem_ = pItem.get();
		pCurrentLine_->addItem(pItem);
		
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
		
		StringReader reader(buffer_.getCharArray(), false);
		std::auto_ptr<Template> pValue(TemplateParser().parse(&reader));
		if (!pValue.get())
			return false;
		if (!reader.close())
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

