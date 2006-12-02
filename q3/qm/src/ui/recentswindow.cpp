/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifdef QMRECENTSWINDOW

#include <qmaccount.h>
#include <qmapplication.h>
#include <qmfilenames.h>
#include <qmmessageholder.h>
#include <qmrecents.h>

#include <qsaction.h>
#include <qsthread.h>
#include <qsuiutil.h>

#include <tmschema.h>

#include "actionid.h"
#include "folderimage.h"
#include "recentswindow.h"
#include "resourceinc.h"
#include "uiutil.h"
#include "../model/account.h"
#include "../model/uri.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * RecentsWindow
 *
 */

RecentsWindow::Button qm::RecentsWindow::buttons__[] = {
	{ IDM_FILE_SHOW,			IDS_RECENTS_SHOW,	L'S'	},
	{ IDM_MESSAGE_CLEARRECENTS,	IDS_RECENTS_CLEAR,	L'C'	}
};

qm::RecentsWindow::RecentsWindow(Recents* pRecents,
								 const AccountManager* pAccountManager,
								 ActionMap* pActionMap,
								 const FolderImage* pFolderImage,
								 Profile* pProfile,
								 HWND hwnd) :
	WindowBase(true),
	pRecents_(pRecents),
	pAccountManager_(pAccountManager),
	pActionMap_(pActionMap),
	pFolderImage_(pFolderImage),
	pProfile_(pProfile),
	hwnd_(hwnd),
	nSelectedItem_(-1),
	nSelectedButton_(-1),
	hfont_(0),
	hfontBold_(0),
	crForeground_(RGB(0, 0, 0)),
	crBackground_(RGB(255, 255, 255)),
	crSelectedForeground_(RGB(0, 0, 0)),
	crSelectedBackground_(RGB(255, 255, 255)),
	hImageList_(0),
	nWidth_(400),
	nLineHeight_(0),
	nHeaderLineHeight_(0),
	nMnemonicWidth_(0),
	nButtonHeight_(0),
	nHideTimeout_(20*1000),
	bImeControl_(true),
	show_(SHOW_HIDDEN),
	bMouseTracking_(false)
{
	bImeControl_ = pProfile_->getInt(L"Global", L"ImeControl") != 0;
	
	struct {
		const WCHAR* pwszKey_;
		int nIndex_;
		COLORREF* pcr_;
	} colors[] = {
		{ L"ForegroundColor",					COLOR_WINDOWTEXT,		&crForeground_					},
		{ L"BackgroundColor",					COLOR_WINDOW,			&crBackground_					},
		{ L"SelectedForegroundColor",			COLOR_HIGHLIGHTTEXT,	&crSelectedForeground_			},
		{ L"SelectedBackgroundColor",			COLOR_HIGHLIGHT,		&crSelectedBackground_			},
	};
	for (int n = 0; n < countof(colors); ++n) {
		wstring_ptr wstr(pProfile_->getString(L"RecentsWindow", colors[n].pwszKey_));
		Color color(wstr.get());
		if (color.getColor() != 0xffffffff)
			*colors[n].pcr_ = color.getColor();
		else
			*colors[n].pcr_ = ::GetSysColor(colors[n].nIndex_);
	}
	
	setWindowHandler(this, false);
}

qm::RecentsWindow::~RecentsWindow()
{
	clearItems();
}

void qm::RecentsWindow::showActive(bool bHotKey)
{
	if (show_ == SHOW_PASSIVE) {
		killTimer(TIMER_HIDE);
		killTimer(TIMER_UPDATE);
	}
	
	prepareItems(true);
	layout(!bHotKey, false);
	show_ = SHOW_ACTIVE;
	
	showWindow(SW_SHOW);
	setForegroundWindow();
}

void qm::RecentsWindow::showPassive()
{
	if (show_ == SHOW_ACTIVE)
		return;
	
	prepareItems(false);
	layout(false, true);
	show_ = SHOW_PASSIVE;
	
	showWindow(SW_SHOWNOACTIVATE);
	
	setTimer(TIMER_HIDE, nHideTimeout_);
	setTimer(TIMER_UPDATE, UPDATE_INTERVAL);
}

void qm::RecentsWindow::getWindowClass(WNDCLASS* pwc)
{
	DefaultWindowHandler::getWindowClass(pwc);
	pwc->style |= CS_HREDRAW | CS_VREDRAW;
}

LRESULT qm::RecentsWindow::windowProc(UINT uMsg,
									  WPARAM wParam,
									  LPARAM lParam)
{
	BEGIN_MESSAGE_HANDLER()
		HANDLE_ACTIVATE()
		HANDLE_CHAR()
		HANDLE_CREATE()
		HANDLE_DESTROY()
		HANDLE_KEYDOWN()
		HANDLE_LBUTTONUP()
#if !defined _WIN32_WCE && (_WIN32_WINNT >= 0x0400 || WINVER >= 0x0500)
		HANDLE_MOUSELEAVE()
#endif
		HANDLE_MOUSEMOVE()
		HANDLE_NCPAINT()
		HANDLE_PAINT()
		HANDLE_SETCURSOR()
		HANDLE_SYSKEYDOWN()
		HANDLE_THEMECHANGED()
		HANDLE_TIMER()
		HANDLE_MESSAGE(WM_RECENTSWINDOW_SHOWPASSIVE, onShowPassive)
	END_MESSAGE_HANDLER()
	return DefaultWindowHandler::windowProc(uMsg, wParam, lParam);
}

LRESULT qm::RecentsWindow::onActivate(UINT nFlags,
									  HWND hwnd,
									  bool bMinimized)
{
	DefaultWindowHandler::onActivate(nFlags, hwnd, bMinimized);
	
	if (nFlags == WA_INACTIVE) {
		if (show_ == SHOW_ACTIVE)
			close();
	}
	else {
		if (bImeControl_)
			qs::UIUtil::setImeEnabled(getHandle(), false);
	}
	
	return 0;
}

LRESULT qm::RecentsWindow::onChar(UINT nChar,
								  UINT nRepeat,
								  UINT nFlags)
{
	if (L'0' <= nChar && nChar <= L'9') {
		int nItem = nChar != L'0' ? nChar - L'1' : 9;
		if (nItem != -1)
			openItem(nItem);
	}
	else {
		int nButton = getButtonByMnemonic(nChar);
		if (nButton != -1)
			invokeAction(buttons__[nButton].nId_, 0);
	}
	return DefaultWindowHandler::onChar(nChar, nRepeat, nFlags);
}

LRESULT qm::RecentsWindow::onCreate(CREATESTRUCT* pCreateStruct)
{
	if (DefaultWindowHandler::onCreate(pCreateStruct) == -1)
		return -1;
	
	pTheme_.reset(new Theme(getHandle(), L"ToolTip"));
	
	hfont_ = qs::UIUtil::createFontFromProfile(pProfile_,
		L"RecentsWindow", qs::UIUtil::DEFAULTFONT_UI);
	if (!hfont_)
		return -1;
	
	LOGFONT lf;
	::GetObject(hfont_, sizeof(lf), &lf);
	lf.lfWeight = FW_BOLD;
	hfontBold_ = ::CreateFontIndirect(&lf);
	if (!hfontBold_)
		return -1;
	
	hImageList_ = UIUtil::createImageListFromFile(
		FileNames::LIST_BMP, 16, CLR_DEFAULT);
	if (!hImageList_)
		return -1;
	
	nWidth_ = pProfile_->getInt(L"RecentsWindow", L"Width");
	
	ClientDeviceContext dc(getHandle());
	ObjectSelector<HFONT> fontSelector(dc, hfont_);
	TEXTMETRIC tm;
	dc.getTextMetrics(&tm);
	nLineHeight_ = tm.tmHeight + tm.tmExternalLeading;
	nHeaderLineHeight_ = QSMAX(nLineHeight_, IMAGE_HEIGHT);
	nMnemonicWidth_ = static_cast<int>(tm.tmAveCharWidth*1.2);
	nButtonHeight_ = nLineHeight_ + BUTTON_PADDING*2;
	
	nHideTimeout_ = pProfile_->getInt(L"RecentsWindow", L"HideTimeout")*1000;
	
#if _WIN32_WINNT >= 0x500
	UIUtil::setWindowAlpha(getHandle(), pProfile_, L"RecentsWindow");
#endif
	
	return 0;
}

LRESULT qm::RecentsWindow::onDestroy()
{
	if (hfont_) {
		::DeleteObject(hfont_);
		hfont_ = 0;
	}
	
	if (hfontBold_) {
		::DeleteObject(hfontBold_);
		hfontBold_ = 0;
	}
	
	if (hImageList_) {
		ImageList_Destroy(hImageList_);
		hImageList_ = 0;
	}
	
	pTheme_.reset(0);
	
	return DefaultWindowHandler::onDestroy();
}

LRESULT qm::RecentsWindow::onKeyDown(UINT nKey,
									 UINT nRepeat,
									 UINT nFlags)
{
	if (nKey == VK_UP || nKey == VK_DOWN) {
		if (!listItem_.empty()) {
			ItemList::size_type nOld = nSelectedItem_;
			switch (nKey) {
			case VK_UP:
				if (nSelectedItem_ == 0 || nSelectedItem_ == -1)
					nSelectedItem_ = listItem_.size() - 1;
				else
					--nSelectedItem_;
				break;
			case VK_DOWN:
				if (nSelectedItem_ == -1 || nSelectedItem_ == listItem_.size() - 1)
					nSelectedItem_ = 0;
				else
					++nSelectedItem_;
				break;
			default:
				break;
			}
			if (nOld != nSelectedItem_) {
				invalidateItem(nOld);
				invalidateItem(nSelectedItem_);
			}
		}
	}
	else {
		switch (nKey) {
		case VK_RETURN:
			openItem(nSelectedItem_);
			break;
		case VK_ESCAPE:
			close();
			break;
		default:
			break;
		}
	}
	
	return DefaultWindowHandler::onKeyDown(nKey, nRepeat, nFlags);
}

LRESULT qm::RecentsWindow::onLButtonUp(UINT nFlags,
									   const POINT& pt)
{
	ItemList::size_type nItem = getSelectedItem(pt.y);
	if (nItem != -1) {
		openItem(nItem);
	}
	else {
		int nButton = getButtonByPos(pt);
		if (nButton != -1)
			invokeAction(buttons__[nButton].nId_, 0);
		else
			close();
	}
	return 0;
}

#if !defined _WIN32_WCE && (_WIN32_WINNT >= 0x0400 || WINVER >= 0x0500)
LRESULT qm::RecentsWindow::onMouseLeave()
{
	if (nSelectedItem_ != -1) {
		invalidateItem(nSelectedItem_);
		nSelectedItem_ = -1;
	}
	
	if (nSelectedButton_ != -1) {
		invalidateButton(nSelectedButton_);
		nSelectedButton_ = -1;
	}
	
	bMouseTracking_ = false;
	
	return 0;
}
#endif
LRESULT qm::RecentsWindow::onMouseMove(UINT nFlags,
									   const POINT& pt)
{
	ItemList::size_type nItem = getSelectedItem(pt.y);
	if (nItem != nSelectedItem_) {
		invalidateItem(nSelectedItem_);
		nSelectedItem_ = nItem;
		invalidateItem(nSelectedItem_);
	}
	
	int nButton = nItem != -1 ? -1 : getButtonByPos(pt);
	if (nButton != nSelectedButton_) {
		invalidateButton(nSelectedButton_);
		nSelectedButton_ = nButton;
		invalidateButton(nSelectedButton_);
	}
	
#if !defined _WIN32_WCE && (_WIN32_WINNT >= 0x0400 || WINVER >= 0x0500)
	TRACKMOUSEEVENT tme = {
		sizeof(tme),
		TME_LEAVE,
		getHandle(),
		HOVER_DEFAULT
	};
	bMouseTracking_ = ::TrackMouseEvent(&tme) != 0;
#endif
	
	return DefaultWindowHandler::onMouseMove(nFlags, pt);
}

LRESULT qm::RecentsWindow::onNcPaint(HRGN hrgn)
{
	DefaultWindowHandler::onNcPaint(hrgn);
	
	if (pTheme_->isActive())
		qs::UIUtil::drawThemeBorder(pTheme_.get(), getHandle(),
			TTP_STANDARD, 0, getColor(COLOR_WINDOW));
	
	return 0;
}

LRESULT qm::RecentsWindow::onPaint()
{
	PaintDeviceContext dc(getHandle());
	
	RECT rect;
	getClientRect(&rect);
	
	struct ScanCallbackImpl : public ScanCallback
	{
		ScanCallbackImpl(DeviceContext& dc,
						 HFONT hfont,
						 HFONT hfontBold,
						 COLORREF crForeground,
						 COLORREF crBackground,
						 COLORREF crSelectedForeground,
						 COLORREF crSelectedBackground,
						 HIMAGELIST hImageList,
						 const FolderImage* pFolderImage,
						 const RECT& rect,
						 int nLineHeight,
						 int nHeaderLineHeight,
						 int nMnemonicWidth,
						 ItemList::size_type nSelectedItem) :
			dc_(dc),
			hfont_(hfont),
			hfontBold_(hfontBold),
			crForeground_(crForeground),
			crBackground_(crBackground),
			crSelectedForeground_(crSelectedForeground),
			crSelectedBackground_(crSelectedBackground),
			hImageList_(hImageList),
			pFolderImage_(pFolderImage),
			rect_(rect),
			nLineHeight_(nLineHeight),
			nMnemonicWidth_(nMnemonicWidth),
			nSelectedItem_(nSelectedItem)
		{
			rect_.left += RecentsWindow::MARGIN;
			rect_.right -= RecentsWindow::MARGIN;
			
			dc_.getClipBox(&rectClip_);
			
			nTextOffset_ = (nHeaderLineHeight - nLineHeight_)/2;
			nImageOffset_ = (nHeaderLineHeight - RecentsWindow::IMAGE_HEIGHT)/2;
		}
		
		virtual bool account(const Account* pAccount,
							 int nTop,
							 int nBottom)
		{
			rect_.top = nTop;
			rect_.bottom = nBottom;
			
			RECT r;
			if (::IntersectRect(&r, &rect_, &rectClip_)) {
				ObjectSelector<HFONT> fontSelector(dc_, hfontBold_);
				dc_.setTextColor(crForeground_);
				dc_.setBkColor(crBackground_);
				
				int nImage = pFolderImage_->getAccountImage(pAccount, false, false);
				ImageList_Draw(pFolderImage_->getImageList(), nImage,
					dc_, rect_.left, nTop + nImageOffset_, ILD_NORMAL);
				
				int x = rect_.left + RecentsWindow::IMAGE_WIDTH + RecentsWindow::IMAGE_SPACING;
				dc_.extTextOutEllipsis(x, nTop + nTextOffset_, rect_.right - x, ETO_CLIPPED,
					rect_, pAccount->getName(), static_cast<UINT>(wcslen(pAccount->getName())));
			}
			
			return true;
		}
		
		virtual bool folder(const NormalFolder* pFolder,
							int nTop,
							int nBottom)
		{
			rect_.top = nTop;
			rect_.bottom = nBottom;
			
			RECT r;
			if (::IntersectRect(&r, &rect_, &rectClip_)) {
				ObjectSelector<HFONT> fontSelector(dc_, hfontBold_);
				dc_.setTextColor(crForeground_);
				dc_.setBkColor(crBackground_);
				
				int nImage = pFolderImage_->getFolderImage(pFolder, false, false, false);
				ImageList_Draw(pFolderImage_->getImageList(), nImage, dc_,
					rect_.left + RecentsWindow::FOLDER_OFFSET, nTop + nImageOffset_, ILD_NORMAL);
				
				int x = rect_.left + RecentsWindow::FOLDER_OFFSET +
					RecentsWindow::IMAGE_WIDTH + RecentsWindow::IMAGE_SPACING;
				dc_.extTextOutEllipsis(x, nTop + nTextOffset_, rect_.right - x, ETO_CLIPPED,
					rect_, pFolder->getName(), static_cast<UINT>(wcslen(pFolder->getName())));
			}
			
			return true;
		}
		
		virtual bool item(const Item* pItem,
						  ItemList::size_type nItem,
						  int nTop,
						  int nBottom)
		{
			rect_.top = nTop;
			rect_.bottom = nBottom;
			RECT rectTemp;
			if (::IntersectRect(&rectTemp, &rect_, &rectClip_)) {
				ObjectSelector<HFONT> fontSelector(dc_, hfont_);
				
				int x = rect_.left + RecentsWindow::ITEM_OFFSET;
				
				bool bSelected  = nItem == nSelectedItem_;
				dc_.setTextColor(bSelected ? crSelectedForeground_ : crForeground_);
				dc_.setBkColor(bSelected ? crSelectedBackground_ : crBackground_);
				
				RECT r = {
					rect_.left,
					nTop - RecentsWindow::LINE_SPACING,
					rect_.right,
					nBottom + RecentsWindow::LINE_SPACING
				};
				dc_.fillSolidRect(r, bSelected ? crSelectedBackground_ : crBackground_);
				
				int nSubjectLineHeight = QSMAX(nLineHeight_, RecentsWindow::IMAGE_HEIGHT);
				
				if (nItem < 10) {
					WCHAR wsz[3];
					_snwprintf(wsz, countof(wsz), L"&%d", nItem < 9 ? nItem + 1 : 0);
					RECT r = {
						x,
						nTop + nTextOffset_,
						x + nMnemonicWidth_,
						nTop + nSubjectLineHeight,
					};
					dc_.drawText(wsz, -1, &r, DT_CENTER | DT_TOP);
				}
				x += nMnemonicWidth_ + RecentsWindow::MNEMONIC_SPACING;
				
				ImageList_Draw(hImageList_, 0, dc_, x, nTop + nImageOffset_, ILD_NORMAL);
				x += RecentsWindow::IMAGE_WIDTH + RecentsWindow::IMAGE_SPACING;
				
				const WCHAR* pwszSubject = pItem->getSubject();
				rect_.top = nTop;
				rect_.bottom = nTop + nSubjectLineHeight;
				dc_.extTextOutEllipsis(x, nTop + nTextOffset_, rect_.right - x, ETO_CLIPPED,
					rect_, pwszSubject, static_cast<UINT>(wcslen(pwszSubject)));
				nTop += nSubjectLineHeight + RecentsWindow::LINE_SPACING;
				
				const WCHAR* pwszFrom = pItem->getFrom();
				if (*pwszFrom) {
					rect_.top = nTop;
					rect_.bottom = nTop + nLineHeight_;
					dc_.extTextOutEllipsis(x, nTop, rect_.right - x, ETO_CLIPPED,
						rect_, pwszFrom, static_cast<UINT>(wcslen(pwszFrom)));
				}
			}
			
			return true;
		}
		
		DeviceContext& dc_;
		HFONT hfont_;
		HFONT hfontBold_;
		COLORREF crForeground_;
		COLORREF crBackground_;
		COLORREF crSelectedForeground_;
		COLORREF crSelectedBackground_;
		HIMAGELIST hImageList_;
		const FolderImage* pFolderImage_;
		RECT rect_;
		int nLineHeight_;
		int nMnemonicWidth_;
		RECT rectClip_;
		int nTextOffset_;
		int nImageOffset_;
		ItemList::size_type nSelectedItem_;
	} callback(dc, hfont_, hfontBold_, getColor(COLOR_WINDOWTEXT),
		getColor(COLOR_WINDOW), getColor(COLOR_HIGHLIGHTTEXT),
		getColor(COLOR_HIGHLIGHT), hImageList_, pFolderImage_, rect,
		nLineHeight_, nHeaderLineHeight_, nMnemonicWidth_, nSelectedItem_);
	scanItems(&callback);
	
	paintSeparator(dc);
	paintButtons(dc);
	
	return 0;
}

LRESULT qm::RecentsWindow::onSetCursor(HWND hwnd,
									   UINT nHitTest,
									   UINT nMessage)
{
	::SetCursor(::LoadCursor(0, IDC_ARROW));
	return TRUE;
}

LRESULT qm::RecentsWindow::onSysKeyDown(UINT nKey,
										UINT nRepeat,
										UINT nFlags)
{
	if (nKey == VK_MENU)
		close();
	return DefaultWindowHandler::onSysKeyDown(nKey, nRepeat, nFlags);
}

LRESULT qm::RecentsWindow::onThemeChanged()
{
	pTheme_.reset(new Theme(getHandle(), L"ToolTip"));
	return 0;
}

LRESULT qm::RecentsWindow::onTimer(UINT_PTR nId)
{
	switch (nId) {
	case TIMER_HIDE:
		killTimer(TIMER_HIDE);
		if (show_ == SHOW_PASSIVE)
			close();
		break;
	case TIMER_UPDATE:
		killTimer(TIMER_UPDATE);
		
		if (show_ == SHOW_PASSIVE) {
			prepareItems(false);
			if (listItem_.empty()) {
				close();
			}
			else {
				layout(false, true);
				setTimer(TIMER_UPDATE, UPDATE_INTERVAL);
			}
		}
		break;
	}
	return 0;
}

LRESULT qm::RecentsWindow::onShowPassive(WPARAM wParam,
										 LPARAM lParam)
{
	showPassive();
	return 0;
}

void qm::RecentsWindow::layout(bool bAtMousePosition,
							   bool bTopMost)
{
	int nHeight = calcHeight() + ITEM_SPACING + nButtonHeight_ + BUTTON_MARGIN*2;
	
	POINT pt = { 0, 0 };
	if (bAtMousePosition) {
		::GetCursorPos(&pt);
		
#if WINVER >= 0x500
		HMONITOR hMonitor = ::MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
		MONITORINFO info = { sizeof(info) };
		::GetMonitorInfo(hMonitor, &info);
		RECT rect = info.rcWork;
#else
		RECT rect;
		::SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);
#endif
		if (pt.x + nWidth_ > rect.right)
			pt.x = rect.right - nWidth_;
		if (pt.y + nHeight > rect.bottom)
			pt.y = rect.bottom - nHeight;
	}
	else {
#if WINVER >= 0x500
		HMONITOR hMonitor = ::MonitorFromWindow(hwnd_, MONITOR_DEFAULTTONEAREST);
		MONITORINFO info = { sizeof(info) };
		::GetMonitorInfo(hMonitor, &info);
		RECT rect = info.rcWork;
#else
		RECT rect;
		::SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);
#endif
		pt.x = rect.right - nWidth_;
		pt.y = rect.bottom - nHeight;
	}
	
	setWindowPos(bTopMost ? HWND_TOPMOST : HWND_TOP,
		pt.x, pt.y, nWidth_, nHeight, SWP_NOACTIVATE);
}

void qm::RecentsWindow::paintSeparator(qs::DeviceContext& dc)
{
	GdiObject<HPEN> hPen(::CreatePen(PS_SOLID, 1, RGB(128, 128, 128)));
	ObjectSelector<HPEN> penSelector(dc, hPen.get());
	
	RECT rect;
	getClientRect(&rect);
	
	int y = rect.bottom - (nButtonHeight_ + BUTTON_MARGIN*2);
	POINT pt[] = {
		{ rect.left + SEPARATOR_MARGIN, y },
		{ rect.right - SEPARATOR_MARGIN, y }
	};
	dc.polyline(pt, countof(pt));
}

void qm::RecentsWindow::paintButtons(DeviceContext& dc)
{
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	
	ObjectSelector<HFONT> fontSelector(dc, hfont_);
	
	for (int n = 0; n < countof(buttons__); ++n) {
		RECT r;
		getButtonRect(n, &r);
		wstring_ptr wstrText(loadString(hInst, buttons__[n].nTextId_));
		paintButton(dc, wstrText.get(), r, n == nSelectedButton_);
	}
}

void qm::RecentsWindow::paintButton(DeviceContext& dc,
									const WCHAR* pwszText,
									const RECT& rect,
									bool bSelected)
{
	COLORREF crText;
	COLORREF crBk;
	if (bSelected) {
		crText = getColor(COLOR_HIGHLIGHTTEXT);
		crBk = getColor(COLOR_HIGHLIGHT);
		dc.fillSolidRect(rect, crBk);
	}
	else {
		crText = getColor(COLOR_WINDOWTEXT);
		crBk = getColor(COLOR_WINDOW);
	}
	dc.setTextColor(crText);
	dc.setBkColor(crBk);
	
	RECT r = rect;
	dc.drawText(pwszText, -1, &r, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
}

COLORREF qm::RecentsWindow::getColor(int nIndex) const
{
	switch (nIndex) {
	case COLOR_WINDOWTEXT:
		return crForeground_;
	case COLOR_WINDOW:
		return crBackground_;
	case COLOR_HIGHLIGHTTEXT:
		return crSelectedForeground_;
	case COLOR_HIGHLIGHT:
		return crSelectedBackground_;
	default:
		break;
	}
	return ::GetSysColor(nIndex);
}

void qm::RecentsWindow::prepareItems(bool bActive)
{
	clearItems();
	
	pRecents_->removeSeens();
	
	typedef std::vector<MessagePtr> MessageList;
	MessageList listMessage;
	{
		Lock<Recents> lock(*pRecents_);
		
		Time time;
		if (!bActive) {
			time = Time::getCurrentTime();
			time.addSecond(-nHideTimeout_/1000);
		}
		
		unsigned int nCount = pRecents_->getCount();
		listMessage.reserve(nCount);
		for (unsigned int n = 0; n < nCount; ++n) {
			const std::pair<URI*, Time>& p = pRecents_->get(n);
			if (bActive || p.second > time)
				listMessage.push_back(pAccountManager_->getMessage(*p.first));
		}
	}
	
	listItem_.reserve(listMessage.size());
	for (MessageList::const_iterator it = listMessage.begin(); it != listMessage.end(); ++it) {
		MessagePtrLock mpl(*it);
		if (mpl) {
			wstring_ptr wstrSubject(mpl->getSubject());
			wstring_ptr wstrFrom(mpl->getFrom());
			listItem_.push_back(new Item(mpl, wstrSubject, wstrFrom));
		}
	}
	
	std::sort(listItem_.begin(), listItem_.end(), ItemComparator());
}

void qm::RecentsWindow::clearItems()
{
	std::for_each(listItem_.begin(), listItem_.end(), qs::deleter<Item>());
	listItem_.clear();
	nSelectedItem_ = -1;
}

int qm::RecentsWindow::calcHeight() const
{
	struct ScanCallbackImpl : public ScanCallback
	{
		ScanCallbackImpl() :
			nHeight_(0)
		{
		}
		
		virtual bool item(const Item* pItem,
						  ItemList::size_type nItem,
						  int nTop,
						  int nBottom)
		{
			nHeight_ = nBottom;
			return true;
		}
		
		int nHeight_;
	} callback;
	scanItems(&callback);
	return callback.nHeight_ + MARGIN;
}

void qm::RecentsWindow::getItemRect(ItemList::size_type nItem,
									RECT* pRect) const
{
	assert(pRect);
	
	getClientRect(pRect);
	
	struct ScanCallbackImpl : public ScanCallback
	{
		ScanCallbackImpl(ItemList::size_type nItem,
						 RECT* pRect) :
			nItem_(nItem),
			pRect_(pRect)
		{
		}
		
		virtual bool item(const Item* pItem,
						  ItemList::size_type nItem,
						  int nTop,
						  int nBottom)
		{
			if (nItem != nItem_)
				return true;
			
			pRect_->top = nTop - RecentsWindow::LINE_SPACING;
			pRect_->bottom = nBottom + RecentsWindow::LINE_SPACING;
			return false;
		}
		
		ItemList::size_type nItem_;
		RECT* pRect_;
	} callback(nItem, pRect);
	scanItems(&callback);
}

RecentsWindow::ItemList::size_type qm::RecentsWindow::getSelectedItem(int nY) const
{
	struct ScanCallbackImpl : public ScanCallback
	{
		ScanCallbackImpl(int nY) :
			nY_(nY),
			nItem_(-1)
		{
		}
		
		virtual bool item(const Item* pItem,
						  ItemList::size_type nItem,
						  int nTop,
						  int nBottom)
		{
			if (nTop <= nY_ && nY_ < nBottom) {
				nItem_ = nItem;
				return false;
			}
			else {
				return nBottom < nY_;
			}
		}
		
		int nY_;
		ItemList::size_type nItem_;
	} callback(nY);
	scanItems(&callback);
	return callback.nItem_;
}

void qm::RecentsWindow::invalidateItem(ItemList::size_type nItem)
{
	if (nItem == -1)
		return;
	
	RECT rect;
	getItemRect(nItem, &rect);
	invalidateRect(rect);
}

void qm::RecentsWindow::scanItems(ScanCallback* pCallback) const
{
	const Account* pPrevAccount = 0;
	const NormalFolder* pPrevFolder = 0;
	
	int y = MARGIN;
	for (ItemList::size_type n = 0; n < listItem_.size(); ++n) {
		const Item* pItem = listItem_[n];
		
		const NormalFolder* pFolder = pItem->getMessagePtr().getFolder();
		if (pFolder != pPrevFolder) {
			const Account* pAccount = pFolder->getAccount();
			if (pAccount != pPrevAccount) {
				if (!pCallback->account(pAccount, y, y + nHeaderLineHeight_))
					return;
				y += nHeaderLineHeight_ + LINE_SPACING;
				pPrevAccount = pAccount;
			}
			
			if (!pCallback->folder(pFolder, y, y + nHeaderLineHeight_))
				return;
			y += nHeaderLineHeight_ + LINE_SPACING;
			pPrevFolder = pFolder;
		}
		
		int nHeight = QSMAX(nLineHeight_, IMAGE_HEIGHT);
		if (*pItem->getFrom())
			nHeight += nLineHeight_ + LINE_SPACING;
		if (!pCallback->item(pItem, n, y, y + nHeight))
			return;
		y += nHeight + ITEM_SPACING;
	}
}

void qm::RecentsWindow::openItem(ItemList::size_type nItem)
{
	if (nItem == -1 || nItem >= listItem_.size())
		return;
	
	MessagePtrLock mpl(listItem_[nItem]->getMessagePtr());
	if (mpl) {
		wstring_ptr wstrURI(URI(mpl).toString());
		invokeAction(IDM_MESSAGE_OPENRECENT, wstrURI.get());
	}
}

void qm::RecentsWindow::invokeAction(unsigned int nId,
									 const WCHAR* pwszParam)
{
	Action* pAction = pActionMap_->getAction(nId);
	ActionParam param(nId, pwszParam);
	ActionEvent event(nId, ActionEvent::getSystemModifiers(), &param);
	pAction->invoke(event);
	
	close();
}

void qm::RecentsWindow::close()
{
	if (show_ == SHOW_PASSIVE) {
		killTimer(TIMER_HIDE);
		killTimer(TIMER_UPDATE);
	}
	
	clearItems();
	nSelectedButton_ = -1;
	show_ = SHOW_HIDDEN;
	showWindow(SW_HIDE);
}

int qm::RecentsWindow::getButtonByPos(const POINT& pt) const
{
	for (int n = 0; n < countof(buttons__); ++n) {
		RECT r;
		getButtonRect(n, &r);
		if (::PtInRect(&r, pt))
			return n;
	}
	return -1;
}

int qm::RecentsWindow::getButtonByMnemonic(WCHAR c) const
{
	c = toupper(c);
	for (int n = 0; n < countof(buttons__); ++n) {
		if (buttons__[n].cMnemonic_ == c)
			return n;
	}
	return -1;
}

void qm::RecentsWindow::getButtonRect(int nButton,
									  RECT* pRect) const
{
	assert(pRect);
	
	getClientRect(pRect);
	
	pRect->left = pRect->right - (BUTTON_WIDTH + BUTTON_MARGIN)*(countof(buttons__) - nButton);
	pRect->right = pRect->right - (BUTTON_WIDTH + BUTTON_MARGIN)*(countof(buttons__) - nButton - 1) - BUTTON_MARGIN;
	pRect->top = pRect->bottom - (nButtonHeight_ + BUTTON_MARGIN);
	pRect->bottom = pRect->bottom - BUTTON_MARGIN;
}

void qm::RecentsWindow::invalidateButton(int nButton)
{
	if (nButton == -1)
		return;
	
	RECT rect;
	getButtonRect(nButton, &rect);
	invalidateRect(rect);
}


/****************************************************************************
 *
 * RecentsWindow::Item
 *
 */

qm::RecentsWindow::Item::Item(MessageHolder* pmh,
							  wstring_ptr wstrSubject,
							  wstring_ptr wstrFrom) :
	ptr_(pmh),
	wstrSubject_(wstrSubject),
	wstrFrom_(wstrFrom)
{
	assert(pmh);
	assert(wstrSubject_.get());
	assert(wstrFrom_.get());
}

qm::RecentsWindow::Item::~Item()
{
}

const MessagePtr& qm::RecentsWindow::Item::getMessagePtr() const
{
	return ptr_;
}

const WCHAR* qm::RecentsWindow::Item::getSubject() const
{
	return wstrSubject_.get();
}

const WCHAR* qm::RecentsWindow::Item::getFrom() const
{
	return wstrFrom_.get();
}


/****************************************************************************
 *
 * RecentsWindow::ItemComparator
 *
 */

bool qm::RecentsWindow::ItemComparator::operator()(const Item* pLhs,
												   const Item* pRhs) const
{
	const MessagePtr& ptrLhs = pLhs->getMessagePtr();
	const MessagePtr& ptrRhs = pRhs->getMessagePtr();
	
	NormalFolder* pFolderLhs = ptrLhs.getFolder();
	NormalFolder* pFolderRhs = ptrRhs.getFolder();
	
	Account* pAccountLhs = pFolderLhs->getAccount();
	Account* pAccountRhs = pFolderRhs->getAccount();
	
	if (pAccountLhs != pAccountRhs)
		return AccountLess::compare(pAccountLhs, pAccountRhs) < 0;
	else if (pFolderLhs != pFolderRhs)
		return FolderLess::compare(pFolderLhs, pFolderRhs) < 0;
	
	MessagePtrLock mplLhs(ptrLhs);
	MessagePtrLock mplRhs(ptrRhs);
	if (!mplLhs)
		return true;
	else if (!mplRhs)
		return false;
	else
		return mplLhs->getId() < mplRhs->getId();
}


/****************************************************************************
 *
 * RecentsWindow::ScanCallback
 *
 */

qm::RecentsWindow::ScanCallback::~ScanCallback()
{
}

bool qm::RecentsWindow::ScanCallback::account(const Account* pAccount,
											  int nTop,
											  int nBottom)
{
	return true;
}

bool qm::RecentsWindow::ScanCallback::folder(const NormalFolder* pFolder,
											 int nTop,
											 int nBottom)
{
	return true;
}

bool qm::RecentsWindow::ScanCallback::item(const Item* pItem,
										   ItemList::size_type nItem,
										   int nTop,
										   int nBottom)
{
	return true;
}


/****************************************************************************
 *
 * RecentsWindowManager
 *
 */

qm::RecentsWindowManager::RecentsWindowManager(Recents* pRecents,
											   const AccountManager* pAccountManager,
											   qs::ActionMap* pActionMap,
											   const FolderImage* pFolderImage,
											   qs::Profile* pProfile,
											   HWND hwnd) :
	pRecents_(pRecents),
	pAccountManager_(pAccountManager),
	pActionMap_(pActionMap),
	pFolderImage_(pFolderImage),
	pProfile_(pProfile),
	hwnd_(hwnd),
	pRecentsWindow_(0),
	bShowPassive_(true)
{
	reloadProfiles();
	
	createWindow();
	pRecents_->addRecentsHandler(this);
}

qm::RecentsWindowManager::~RecentsWindowManager()
{
	pRecents_->removeRecentsHandler(this);
	if (pRecentsWindow_)
		pRecentsWindow_->destroyWindow();
}

void qm::RecentsWindowManager::recentsChanged(const RecentsEvent& event)
{
	if (bShowPassive_ && event.getType() == RecentsEvent::TYPE_ADDED)
		pRecentsWindow_->postMessage(RecentsWindow::WM_RECENTSWINDOW_SHOWPASSIVE);
}

bool qm::RecentsWindowManager::showPopup(bool bHotKey)
{
	if (!createWindow())
		return false;
	
	pRecentsWindow_->showActive(bHotKey);
	
	return true;
}

void qm::RecentsWindowManager::reloadProfiles()
{
	bShowPassive_ = pProfile_->getInt(L"RecentsWindow", L"AutoPopup") != 0;
}

bool qm::RecentsWindowManager::createWindow()
{
	if (!pRecentsWindow_) {
		std::auto_ptr<RecentsWindow> pRecentsWindow(new RecentsWindow(pRecents_,
			pAccountManager_, pActionMap_, pFolderImage_, pProfile_, hwnd_));
		if (!pRecentsWindow->create(L"QmRecentsWindow", 0,
			WS_POPUP | WS_BORDER, 0, 0, 0, 0, 0, WS_EX_TOOLWINDOW, 0, 0, 0))
			return false;
		pRecentsWindow_ = pRecentsWindow.release();
	}
	return true;
}

#endif // QMRECENTSWINDOW
