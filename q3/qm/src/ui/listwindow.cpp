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
#include <qmfolder.h>
#include <qmlistwindow.h>
#include <qmmacro.h>
#include <qmmessageholder.h>

#include <qsaccelerator.h>
#include <qsconv.h>
#include <qsdevicecontext.h>
#include <qsdragdrop.h>
#include <qserror.h>
#include <qskeymap.h>
#include <qsnew.h>
#include <qsprofile.h>
#include <qsstl.h>
#include <qsstream.h>
#include <qsuiutil.h>

#include <algorithm>
#include <vector>

#include "keymap.h"
#include "listwindow.h"
#include "messageframewindow.h"
#include "resourceinc.h"
#include "viewmodel.h"
#include "../action/action.h"
#include "../model/dataobject.h"

#pragma warning(disable:4786)

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * PaintInfo
 *
 */

qm::PaintInfo::PaintInfo(DeviceContext* pdc, ViewModel* pViewModel,
	unsigned int nIndex, const RECT& rect) :
	pdc_(pdc),
	pViewModel_(pViewModel),
	nIndex_(nIndex),
	pItem_(pViewModel->getItem(nIndex)),
	rect_(rect)
{
}

qm::PaintInfo::~PaintInfo()
{
}

DeviceContext* qm::PaintInfo::getDeviceContext() const
{
	return pdc_;
}

ViewModel* qm::PaintInfo::getViewModel() const
{
	return pViewModel_;
}

unsigned int qm::PaintInfo::getIndex() const
{
	return nIndex_;
}

const ViewModelItem* qm::PaintInfo::getItem() const
{
	return pItem_;
}

const RECT& qm::PaintInfo::getRect() const
{
	return rect_;
}


/****************************************************************************
 *
 * ListWindowImpl
 *
 */

struct qm::ListWindowImpl :
	public ViewModelManagerHandler,
	public ViewModelHandler,
	public DragGestureHandler,
	public DragSourceHandler,
	public DropTargetHandler
{
public:
	enum {
		ID_HEADERCOLUMN		= 1000
	};
	enum {
		LINE_SPACING		= 4,
		HORIZONTAL_BORDER	= 2
	};
	enum {
		WM_VIEWMODEL_ITEMADDED		= WM_APP + 1001,
		WM_VIEWMODEL_ITEMREMOVED	= WM_APP + 1002
	};

public:
	QSTATUS createHeaderColumn();
	QSTATUS layoutChildren();
	QSTATUS layoutChildren(int cx, int cy);
	
	void getRect(RECT* pRect);
	void paintMessage(const PaintInfo& pi);
	void invalidateLine(unsigned int nLine);
	void invalidateSelected();
	void ensureVisible(unsigned int nLine);
	void scrollVertical(int nPos);
	void scrollHorizontal(int nPos);
	void updateScrollBar(bool bVertical);
	unsigned int getLineFromPoint(const POINT& pt) const;

public:
	virtual QSTATUS viewModelSelected(const ViewModelManagerEvent& event);

public:
	virtual QSTATUS itemAdded(const ViewModelEvent& event);
	virtual QSTATUS itemRemoved(const ViewModelEvent& event);
	virtual QSTATUS itemChanged(const ViewModelEvent& event);
	virtual QSTATUS itemStateChanged(const ViewModelEvent& event);
	virtual QSTATUS itemAttentionPaid(const ViewModelEvent& event);
	virtual QSTATUS updated(const ViewModelEvent& event);
	virtual QSTATUS sorted(const ViewModelEvent& event);

public:
	virtual QSTATUS dragGestureRecognized(const DragGestureEvent& event);

public:
	virtual QSTATUS dragDropEnd(const DragSourceDropEvent& event);

public:
	virtual QSTATUS dragEnter(const DropTargetDragEvent& event);
	virtual QSTATUS dragOver(const DropTargetDragEvent& event);
	virtual QSTATUS dragExit(const DropTargetEvent& event);
	virtual QSTATUS drop(const DropTargetDropEvent& event);

private:
	static int getMessageImage(MessageHolder* pmh, unsigned int nFlags);

public:
	ListWindow* pThis_;
	
	Profile* pProfile_;
	MessageFrameWindowManager* pMessageFrameWindowManager_;
	MenuManager* pMenuManager_;
	Accelerator* pAccelerator_;
	Document* pDocument_;
	ViewModelManager* pViewModelManager_;
	
	HFONT hfont_;
	int nLineHeight_;
	ListHeaderColumn* pHeaderColumn_;
	HIMAGELIST hImageList_;
	HIMAGELIST hImageListData_;
	HPEN hpenThreadLine_;
	HPEN hpenFocusedThreadLine_;
	DragGestureRecognizer* pDragGestureRecognizer_;
	DropTarget* pDropTarget_;
	bool bCanDrop_;
};

QSTATUS qm::ListWindowImpl::createHeaderColumn()
{
	DECLARE_QSTATUS();
	
	std::auto_ptr<ListHeaderColumn> pHeaderColumn;
	status = newQsObject(pThis_, pProfile_, &pHeaderColumn);
	CHECK_QSTATUS();
	
	RECT rect;
	pThis_->getClientRect(&rect);
	status = pHeaderColumn->create(L"QmListHeaderColumn", 0,
		WS_CHILD, 0, 0, rect.right - rect.left, 0,
		pThis_->getHandle(), 0, 0, ID_HEADERCOLUMN, 0);
	CHECK_QSTATUS();
	
	pHeaderColumn_ = pHeaderColumn.release();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ListWindowImpl::layoutChildren()
{
	RECT rect;
	pThis_->getClientRect(&rect);
	return layoutChildren(rect.right - rect.left, rect.bottom - rect.top);
}

QSTATUS qm::ListWindowImpl::layoutChildren(int cx, int cy)
{
	if (pHeaderColumn_) {
		RECT rect;
		pHeaderColumn_->getWindowRect(&rect);
		pHeaderColumn_->setWindowPos(0, 0, 0,
			cx + pThis_->getScrollPos(SB_HORZ),
			rect.bottom - rect.top, SWP_NOMOVE | SWP_NOZORDER);
	}
	updateScrollBar(true);
	updateScrollBar(false);
	pThis_->invalidate();
	
	return QSTATUS_SUCCESS;
}

void qm::ListWindowImpl::getRect(RECT* pRect)
{
	pThis_->getClientRect(pRect);
	pRect->top += pHeaderColumn_->getHeight();
}

void qm::ListWindowImpl::paintMessage(const PaintInfo& pi)
{
	DECLARE_QSTATUS();
	
	DeviceContext* pdc = pi.getDeviceContext();
	ViewModel* pViewModel = pi.getViewModel();
	const ViewModelItem* pItem = pi.getItem();
	MessageHolder* pmh = pItem->getMessageHolder();
	const RECT& rect = pi.getRect();
	
	bool bHasFocus = pThis_->hasFocus();
	bool bSelected = (pItem->getFlags() & ViewModel::FLAG_SELECTED) != 0;
	if (bSelected) {
		pdc->setTextColor(::GetSysColor(bHasFocus ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT));
		pdc->setBkColor(::GetSysColor(bHasFocus ? COLOR_HIGHLIGHT : COLOR_INACTIVEBORDER));
	}
	else {
		COLORREF cr = pItem->getColor();
		if (cr == 0xff000000)
			cr = ::GetSysColor(COLOR_WINDOWTEXT);
		pdc->setTextColor(cr);
		pdc->setBkColor(::GetSysColor(COLOR_WINDOW));
	}
	
	RECT r = { rect.left, rect.top, rect.left, rect.bottom };
#if defined _WIN32_WCE && _WIN32_WCE >= 300 && defined _WIN32_WCE_PSPC
	RECT rectFocus = { 0, rect.top, 0, rect.bottom };
#else
	RECT rectFocus = { -1, rect.top, 0, rect.bottom };
#endif
	
	unsigned int nLevel = 0;
	if ((pViewModel->getSort() & ViewModel::SORT_THREAD_MASK) == ViewModel::SORT_THREAD)
		nLevel = pItem->getLevel();
	
	int nThreadLeft = -1;
	int nThreadRight = -1;
	const ViewModel::ColumnList& listColumn = pViewModel->getColumns();
	ViewModel::ColumnList::const_iterator it = listColumn.begin();
	while (it != listColumn.end()) {
		ViewColumn* pColumn = *it;
		
		r.right += pColumn->getWidth();
		
		int nOffset = 0;
		if (pColumn->isFlag(ViewColumn::FLAG_INDENT) &&
			((pViewModel->getSort() & ViewModel::SORT_THREAD_MASK) == ViewModel::SORT_THREAD)) {
			nOffset = nLevel*15;
			if (pColumn->isFlag(ViewColumn::FLAG_LINE)) {
				nThreadLeft = r.left;
				nThreadRight = r.right;
			}
		}
		
		ViewColumn::Type type = pColumn->getType();
		if (!pColumn->isFlag(ViewColumn::FLAG_ICON)) {
			string_ptr<WSTRING> wstrText;
			status = pColumn->getText(pmh, &wstrText);
			// TODO
			// Check error
			
			size_t nLen = wcslen(wstrText.get());
			POINT pt = { r.left + HORIZONTAL_BORDER, r.top + LINE_SPACING/2 };
			if (pColumn->isFlag(ViewColumn::FLAG_RIGHTALIGN)) {
				SIZE size;
				pdc->getTextExtent(wstrText.get(), nLen, &size);
				pt.x = r.right - size.cx - HORIZONTAL_BORDER;
			}
			else {
				pt.x += nOffset;
			}
			
			pdc->extTextOut(pt.x, pt.y, ETO_CLIPPED | ETO_OPAQUE,
				r, wstrText.get(), nLen, 0);
			
			if (rectFocus.left == -1)
				rectFocus.left = r.left;
			rectFocus.right = r.right;
		}
		else {
			unsigned int nValue = pColumn->getNumber(pmh);
			
			UINT nBkColorId = COLOR_WINDOW;
#if defined _WIN32_WCE && _WIN32_WCE >= 300 && defined _WIN32_WCE_PSPC
			if (bSelected)
				nBkColorId = pThis_->hasFocus() ? COLOR_HIGHLIGHT : COLOR_INACTIVEBORDER;
#endif
			COLORREF crOld = pdc->getBkColor();
			pdc->fillSolidRect(r, ::GetSysColor(nBkColorId));
			pdc->setBkColor(crOld);
			
#if defined _WIN32_WCE && _WIN32_WCE >= 300 && defined _WIN32_WCE_PSPC
			UINT nFlags = bSelected ? ILD_TRANSPARENT : ILD_NORMAL;
#else
			UINT nFlags = bSelected ? ILD_SELECTED : ILD_NORMAL;
#endif
			int y = r.top + (r.bottom - r.top - 16)/2;
			ImageList_Draw(hImageList_, getMessageImage(pmh, nValue),
				pdc->getHandle(), r.left + 7, y, nFlags);
			if (nValue & MessageHolder::FLAG_MULTIPART)
				ImageList_Draw(hImageListData_, 0, pdc->getHandle(),
					r.left, y, nFlags);
			if (nValue & MessageHolder::FLAG_REPLIED)
				ImageList_Draw(hImageListData_, 1, pdc->getHandle(),
					r.left + 15, y, nFlags);
			else if (nValue & MessageHolder::FLAG_FORWARDED)
				ImageList_Draw(hImageListData_, 2, pdc->getHandle(),
					r.left + 15, y, nFlags);
			if (nValue & MessageHolder::FLAG_DOWNLOAD ||
				nValue & MessageHolder::FLAG_DOWNLOADTEXT)
				ImageList_Draw(hImageListData_, 3, pdc->getHandle(),
					r.left + 7, y, nFlags);
			if (nValue & MessageHolder::FLAG_DELETED)
				ImageList_Draw(hImageListData_, 4, pdc->getHandle(),
					r.left + 7, y, nFlags);
			if (nValue & MessageHolder::FLAG_MARKED)
				ImageList_Draw(hImageListData_, 5, pdc->getHandle(),
					r.left + 15, y, nFlags);
			
#if defined _WIN32_WCE && _WIN32_WCE >= 300 && defined _WIN32_WCE_PSPC
			rectFocus.right = r.right;
#endif
		}
		
		r.left = r.right;
		
		++it;
	}
	r.right = rect.right;
	pdc->fillSolidRect(r, ::GetSysColor(COLOR_WINDOW));
	
	if (((pViewModel->getSort() & ViewModel::SORT_THREAD_MASK) == ViewModel::SORT_THREAD) &&
		nThreadLeft != -1) {
		if (nLevel > 0) {
			typedef std::vector<int> Line;
			Line line;
			status = STLWrapper<Line>(line).resize(nLevel);
			if (status == QSTATUS_SUCCESS) {
				for (unsigned int n = pi.getIndex() + 1; n < pViewModel->getCount(); ++n) {
					const ViewModelItem* pNextItem = pViewModel->getItem(n);
					unsigned int nNextLevel = pNextItem->getLevel();
					if (nNextLevel == 0) {
						break;
					}
					else if (nNextLevel <= nLevel) {
						line[nNextLevel - 1] = 1;
						nLevel = nNextLevel;
					}
				}
				
				HPEN hpen = (bSelected && bHasFocus) ?
					hpenFocusedThreadLine_ : hpenThreadLine_;
				ObjectSelector<HPEN> selector(*pdc, hpen);
				int nOffset = 7;
				POINT points[4];
				for (Line::iterator it = line.begin(); it != line.end(); ++it) {
					int nPoint = 2;
					if (*it || it + 1 == line.end()) {
						points[0].x = nThreadLeft + nOffset;
						points[0].y = rect.top;
						points[1].x = points[0].x;
						points[1].y = *it ? rect.bottom : (rect.top + rect.bottom)/2;
						if (it + 1 == line.end()) {
							if (*it) {
								points[nPoint].x = points[1].x;
								points[nPoint].y = (rect.top + rect.bottom)/2;
								++nPoint;
							}
							points[nPoint].x = points[nPoint - 1].x + 5;
							points[nPoint].y = points[nPoint - 1].y;
							++nPoint;
						}
						pdc->polyline(points, nPoint);
					}
					nOffset += 15;
					if (nThreadLeft + nOffset > nThreadRight)
						break;
				}
			}
		}
	}
	
#if !defined _WIN32_WCE || _WIN32_WCE < 300 || !defined _WIN32_WCE_PSPC
	if ((pItem->getFlags() & ViewModel::FLAG_FOCUSED) && bHasFocus) {
		pdc->setTextColor(COLOR_WINDOWTEXT);
		pdc->drawFocusRect(rectFocus);
	}
#endif
}

void qm::ListWindowImpl::invalidateLine(unsigned int nLine)
{
	RECT rect;
	getRect(&rect);
	
	int nPos = pThis_->getScrollPos(SB_VERT);
	rect.top += (nLine - nPos)*nLineHeight_;
	rect.bottom = rect.top + nLineHeight_;
	
	pThis_->invalidateRect(rect);
}

void qm::ListWindowImpl::invalidateSelected()
{
	ViewModel* pViewModel = pViewModelManager_->getCurrentViewModel();
	if (pViewModel) {
		Lock<ViewModel> lock(*pViewModel);
		
		RECT rect;
		getRect(&rect);
		int nTop = rect.top;
		
		SCROLLINFO si = {
			sizeof(si),
			SIF_PAGE | SIF_POS
		};
		pThis_->getScrollInfo(SB_VERT, &si);
		
		unsigned int nCount = pViewModel->getCount();
		unsigned int nFocused = pViewModel->getFocused();
		for (unsigned int n = si.nPos; n < nCount && n <= si.nPos + si.nPage; ++n) {
			if (pViewModel->isSelected(n) || n == nFocused) {
				rect.top = nTop + (n - si.nPos)*nLineHeight_;
				rect.bottom = rect.top + nLineHeight_;
				pThis_->invalidateRect(rect);
			}
		}
	}
}

void qm::ListWindowImpl::ensureVisible(unsigned int nLine)
{
	SCROLLINFO si = { sizeof(si), SIF_POS | SIF_PAGE };
	pThis_->getScrollInfo(SB_VERT, &si);
	
	if (nLine < static_cast<unsigned int>(si.nPos))
		scrollVertical(nLine);
	else if (nLine >= static_cast<unsigned int>(si.nPos + si.nPage) - 1)
		scrollVertical(nLine + 1 - si.nPage);
}

void qm::ListWindowImpl::scrollVertical(int nPos)
{
	SCROLLINFO si = {
		sizeof(si),
		SIF_RANGE | SIF_POS | SIF_PAGE
	};
	pThis_->getScrollInfo(SB_VERT, &si);
	
	if (nPos < 0)
		nPos = 0;
	else if (nPos > si.nMax - static_cast<int>(si.nPage) + 1)
		nPos = si.nMax - si.nPage + 1;
	
	if (nPos == si.nPos)
		return;
	
	RECT rect;
	getRect(&rect);
	
	int nOldPos = si.nPos;
	pThis_->setScrollPos(SB_VERT, nPos);
	pThis_->scrollWindow(0, (nOldPos - nPos)*nLineHeight_,
		&rect, &rect, 0, 0, SW_INVALIDATE);
}

void qm::ListWindowImpl::scrollHorizontal(int nPos)
{
	SCROLLINFO si = {
		sizeof(si),
		SIF_RANGE | SIF_POS | SIF_PAGE
	};
	pThis_->getScrollInfo(SB_HORZ, &si);
	
	if (nPos < 0)
		nPos = 0;
	else if (nPos > si.nMax - static_cast<int>(si.nPage) + 1)
		nPos = si.nMax - si.nPage;
	
	if (nPos == si.nPos)
		return;
	
	RECT rect;
	pThis_->getClientRect(&rect);
	
	int nOldPos = si.nPos;
	pThis_->setScrollPos(SB_HORZ, nPos);
	pThis_->scrollWindow(nOldPos - nPos, 0,
		&rect, &rect, 0, 0, SW_INVALIDATE/* | SW_SCROLLCHILDREN*/);
	
	pHeaderColumn_->setWindowPos(0, -nPos, 0,
		(rect.right - rect.left) + nPos, pHeaderColumn_->getHeight(),
		SWP_NOZORDER | SWP_NOACTIVATE);
}

void qm::ListWindowImpl::updateScrollBar(bool bVertical)
{
	RECT rect;
	getRect(&rect);
	
	if (bVertical) {
		ViewModel* pViewModel = pViewModelManager_->getCurrentViewModel();
		unsigned int nCount = 0;
		if (pViewModel) {
			Lock<ViewModel> lock(*pViewModel);
			nCount = pViewModel->getCount();
		}
		
		SCROLLINFO si = {
			sizeof(si),
			SIF_PAGE | SIF_RANGE,
			0,
			nCount - 1,
			(rect.bottom - rect.top)/nLineHeight_,
			0,
		};
		pThis_->setScrollInfo(SB_VERT, si);
	}
	else {
		unsigned int nWidth = pHeaderColumn_->getWidth();
		SCROLLINFO si = {
			sizeof(si),
			SIF_PAGE | SIF_RANGE,
			0,
			nWidth,
			rect.right - rect.left,
			0,
		};
		pThis_->setScrollInfo(SB_HORZ, si);
		
		pHeaderColumn_->setWindowPos(0, -pThis_->getScrollPos(SB_HORZ),
			0, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
	}
}

unsigned int qm::ListWindowImpl::getLineFromPoint(const POINT& pt) const
{
	ViewModel* pViewModel = pViewModelManager_->getCurrentViewModel();
	assert(pViewModel);
	assert(pViewModel->isLocked());
	
	unsigned int nLine = (pt.y - pHeaderColumn_->getHeight())/
		nLineHeight_ + pThis_->getScrollPos(SB_VERT);
	return nLine < pViewModel->getCount() ? nLine : static_cast<unsigned int>(-1);
}

QSTATUS qm::ListWindowImpl::viewModelSelected(const ViewModelManagerEvent& event)
{
	DECLARE_QSTATUS();
	
	ViewModel* pOldViewModel = event.getOldViewModel();
	if (pOldViewModel) {
		status = pOldViewModel->removeViewModelHandler(this);
		CHECK_QSTATUS();
	}
	
	ViewModel* pNewViewModel = event.getNewViewModel();
	if (pNewViewModel) {
		status = pNewViewModel->addViewModelHandler(this);
		CHECK_QSTATUS();
	}
	
	status = pHeaderColumn_->setViewModel(pNewViewModel);
	CHECK_QSTATUS();
	
	pThis_->refresh();
	
	if (pNewViewModel)
		ensureVisible(pNewViewModel->getFocused());
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ListWindowImpl::itemAdded(const ViewModelEvent& event)
{
	assert(event.getViewModel() == pViewModelManager_->getCurrentViewModel());
	pThis_->postMessage(WM_VIEWMODEL_ITEMADDED, event.getItem(), 0);
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ListWindowImpl::itemRemoved(const ViewModelEvent& event)
{
	assert(event.getViewModel() == pViewModelManager_->getCurrentViewModel());
	pThis_->postMessage(WM_VIEWMODEL_ITEMREMOVED, event.getItem(), 0);
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ListWindowImpl::itemChanged(const ViewModelEvent& event)
{
	assert(event.getViewModel() == pViewModelManager_->getCurrentViewModel());
	invalidateLine(event.getItem());
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ListWindowImpl::itemStateChanged(const ViewModelEvent& event)
{
	assert(event.getViewModel() == pViewModelManager_->getCurrentViewModel());
	invalidateLine(event.getItem());
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ListWindowImpl::itemAttentionPaid(const ViewModelEvent& event)
{
	assert(event.getViewModel() == pViewModelManager_->getCurrentViewModel());
	ensureVisible(event.getItem());
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ListWindowImpl::updated(const ViewModelEvent& event)
{
	assert(event.getViewModel() == pViewModelManager_->getCurrentViewModel());
	pThis_->refresh();
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ListWindowImpl::sorted(const ViewModelEvent& event)
{
	assert(event.getViewModel() == pViewModelManager_->getCurrentViewModel());
	ensureVisible(event.getViewModel()->getFocused());
	pThis_->invalidate();
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ListWindowImpl::dragGestureRecognized(const DragGestureEvent& event)
{
	DECLARE_QSTATUS();
	
	ViewModel* pViewModel = pViewModelManager_->getCurrentViewModel();
	if (!pViewModel)
		return QSTATUS_SUCCESS;
	
	Lock<ViewModel> lock(*pViewModel);
	
	Folder::MessageHolderList l;
	status = pViewModel->getSelection(&l);
	CHECK_QSTATUS();
	if (l.empty())
		return QSTATUS_SUCCESS;
	
	MessagePtrList listMessagePtr;
	status = STLWrapper<MessagePtrList>(listMessagePtr).resize(l.size());
	CHECK_QSTATUS();
	for (Folder::MessageHolderList::size_type n = 0; n < l.size(); ++n)
		listMessagePtr[n] = MessagePtr(l[n]);
	
	MessageDataObject* p = 0;
	status = newQsObject(pViewModel->getFolder()->getAccount(),
		listMessagePtr, MessageDataObject::FLAG_NONE, &p);
	CHECK_QSTATUS();
	p->AddRef();
	ComPtr<IDataObject> pDataObject(p);
	
	DragSource source(&status);
	CHECK_QSTATUS();
	source.setDragSourceHandler(this);
	status = source.startDrag(pDataObject.get(),
		DROPEFFECT_COPY | DROPEFFECT_MOVE);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ListWindowImpl::dragDropEnd(const DragSourceDropEvent& event)
{
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ListWindowImpl::dragEnter(const DropTargetDragEvent& event)
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

QSTATUS qm::ListWindowImpl::dragOver(const DropTargetDragEvent& event)
{
	if (bCanDrop_) 
		event.setEffect(DROPEFFECT_COPY);
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ListWindowImpl::dragExit(const DropTargetEvent& event)
{
	bCanDrop_ = false;
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ListWindowImpl::drop(const DropTargetDropEvent& event)
{
	DECLARE_QSTATUS();
	
	ViewModel* pViewModel = pViewModelManager_->getCurrentViewModel();
	if (pViewModel) {
		Folder* pFolder = pViewModel->getFolder();
		if (pFolder->getType() == Folder::TYPE_NORMAL) {
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
					int nCount = ::DragQueryFile(hDrop, 0xffffffff, 0, 0);
					for (int n = 0; n < nCount; ++n) {
						TCHAR tszPath[MAX_PATH];
						::DragQueryFile(hDrop, n, tszPath, countof(tszPath));
						DWORD dwFileAttributes = ::GetFileAttributes(tszPath);
						if (dwFileAttributes != 0xffffffff &&
							!(dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
							T2W(tszPath, pwszPath);
							
							FileInputStream stream(pwszPath, &status);
							CHECK_QSTATUS();
							BufferedInputStream bufferedStream(&stream, false, &status);
							CHECK_QSTATUS();
							
							status = FileImportAction::readMessage(
								static_cast<NormalFolder*>(pFolder), &bufferedStream,
								false, Account::IMPORTFLAG_NORMALFLAGS);
							CHECK_QSTATUS();
						}
					}
				}
			}
#endif
		}
	}
	
	return QSTATUS_SUCCESS;
}

int qm::ListWindowImpl::getMessageImage(MessageHolder* pmh, unsigned int nFlags)
{
	int nImage = 0;
	
	NormalFolder* pFolder = pmh->getFolder();
	if (pFolder && pFolder->isFlag(NormalFolder::FLAG_OUTBOX))
		nImage = nFlags & MessageHolder::FLAG_DRAFT ? 4 : 1;
	else if (nFlags & MessageHolder::FLAG_SENT)
		nImage = 2;
	else if (nFlags & MessageHolder::FLAG_SEEN ||
		nFlags & MessageHolder::FLAG_DELETED)
		nImage = 3;
	
	if (nFlags & MessageHolder::FLAG_PARTIAL_MASK)
		nImage += 5;
	
	return nImage;
}


/****************************************************************************
 *
 * ListWindow
 *
 */

qm::ListWindow::ListWindow(ViewModelManager* pViewModelManager, Profile* pProfile,
	MessageFrameWindowManager* pMessageFrameWindowManager, QSTATUS* pstatus) :
	WindowBase(true, pstatus),
	DefaultWindowHandler(pstatus),
	pImpl_(0)
{
	if (*pstatus != QSTATUS_SUCCESS)
		return;
	
	DECLARE_QSTATUS();
	
	int nShowHeaderColumn = 1;
	status = pProfile->getInt(L"ListWindow",
		L"ShowHeaderColumn", 1, &nShowHeaderColumn);
	CHECK_QSTATUS_SET(pstatus);
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	pImpl_->pThis_ = this;
	pImpl_->pProfile_ = pProfile;
	pImpl_->pMessageFrameWindowManager_ = pMessageFrameWindowManager;
	pImpl_->pMenuManager_ = 0;
	pImpl_->pAccelerator_ = 0;
	pImpl_->pDocument_ = 0;
	pImpl_->pViewModelManager_ = pViewModelManager;
	pImpl_->hfont_ = 0;
	pImpl_->nLineHeight_ = 0;
	pImpl_->pHeaderColumn_ = 0;
	pImpl_->hImageList_ = 0;
	pImpl_->hImageListData_ = 0;
	pImpl_->hpenThreadLine_ = 0;
	pImpl_->hpenFocusedThreadLine_ = 0;
	pImpl_->pDragGestureRecognizer_ = 0;
	pImpl_->pDropTarget_ = 0;
	pImpl_->bCanDrop_ = false;
	
	status = pImpl_->pViewModelManager_->addViewModelManagerHandler(pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	
	setWindowHandler(this, false);
}

qm::ListWindow::~ListWindow()
{
	if (pImpl_) {
		delete pImpl_->pAccelerator_;
		delete pImpl_;
		pImpl_ = 0;
	}
}

QSTATUS qm::ListWindow::refresh()
{
	pImpl_->updateScrollBar(true);
	pImpl_->updateScrollBar(false);
	invalidate();
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ListWindow::moveSelection(MoveSelection m, bool bShift, bool bCtrl)
{
	DECLARE_QSTATUS();
	
	ViewModel* pViewModel = pImpl_->pViewModelManager_->getCurrentViewModel();
	if (!pViewModel)
		return QSTATUS_SUCCESS;
	
	Lock<ViewModel> lock(*pViewModel);
	
	unsigned int nCount = pViewModel->getCount();
	if (nCount == 0)
		return QSTATUS_SUCCESS;
	
	unsigned int nLine = pViewModel->getFocused();
	unsigned int nNewLine = nLine;
	
	switch (m) {
	case MOVESELECTION_LINEUP:
		if (nNewLine != 0)
			--nNewLine;
		break;
	case MOVESELECTION_LINEDOWN:
		if (nNewLine != nCount - 1)
			++nNewLine;
		break;
	case MOVESELECTION_PAGEUP:
		{
			SCROLLINFO si = { sizeof(si), SIF_POS | SIF_PAGE };
			getScrollInfo(SB_VERT, &si);
			unsigned int nTop = si.nPos;
			if (nNewLine <= nTop)
				nNewLine = nNewLine > si.nPage ? nNewLine - si.nPage : 0;
			else
				nNewLine = nTop;
		}
		break;
	case MOVESELECTION_PAGEDOWN:
		{
			SCROLLINFO si = { sizeof(si), SIF_POS | SIF_PAGE };
			getScrollInfo(SB_VERT, &si);
			unsigned int nBottom = si.nPos + si.nPage - 1;
			if (nNewLine >= nBottom)
				nNewLine = nNewLine + si.nPage >= nCount ?
					nCount - 1 : nNewLine + si.nPage;
			else
				nNewLine = nBottom;
		}
		break;
	case MOVESELECTION_TOP:
		nNewLine = 0;
		break;
	case MOVESELECTION_BOTTOM:
		nNewLine = nCount - 1;
		break;
	case MOVESELECTION_CURRENT:
		if (bShift) {
			status = pViewModel->setSelection(nNewLine,
				pViewModel->getLastSelection());
			CHECK_QSTATUS_VALUE(0);
		}
		else {
			if (pViewModel->isSelected(nNewLine)) {
				if (bCtrl) {
					status = pViewModel->removeSelection(nNewLine);
					CHECK_QSTATUS();
				}
			}
			else {
				status = pViewModel->addSelection(nNewLine);
				CHECK_QSTATUS();
			}
			if (bCtrl)
				pViewModel->setLastSelection(nNewLine);
		}
		break;
	default:
		assert(false);
		break;
	}
	
	if (nNewLine != nLine) {
		status = pViewModel->setFocused(nNewLine);
		CHECK_QSTATUS();
		if (bShift) {
			status = pViewModel->setSelection(nNewLine,
				pViewModel->getLastSelection());
			CHECK_QSTATUS();
		}
		else {
			if (!bCtrl) {
				status = pViewModel->setSelection(nNewLine);
				CHECK_QSTATUS();
				pViewModel->setLastSelection(nNewLine);
			}
		}
		pImpl_->ensureVisible(nNewLine);
	}
	
	return QSTATUS_SUCCESS;
}

HFONT qm::ListWindow::getFont() const
{
	return pImpl_->hfont_;
}

bool qm::ListWindow::isShowHeaderColumn() const
{
	return pImpl_->pHeaderColumn_->isShow();
}

QSTATUS qm::ListWindow::setShowHeaderColumn(bool bShow)
{
	if (bShow != pImpl_->pHeaderColumn_->isShow()) {
		pImpl_->pHeaderColumn_->setShow(bShow);
		pImpl_->layoutChildren();
	}
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ListWindow::save() const
{
	return pImpl_->pHeaderColumn_->save();
}

QSTATUS qm::ListWindow::preCreateWindow(CREATESTRUCT* pCreateStruct)
{
	DECLARE_QSTATUS();
	
	status = DefaultWindowHandler::preCreateWindow(pCreateStruct);
	CHECK_QSTATUS();
	
	pCreateStruct->style |= WS_HSCROLL | WS_VSCROLL;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ListWindow::getAccelerator(Accelerator** ppAccelerator)
{
	assert(ppAccelerator);
	*ppAccelerator = pImpl_->pAccelerator_;
	return QSTATUS_SUCCESS;
}

LRESULT qm::ListWindow::windowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	BEGIN_MESSAGE_HANDLER()
		HANDLE_CONTEXTMENU()
		HANDLE_CREATE()
		HANDLE_DESTROY()
		HANDLE_ERASEBKGND()
		HANDLE_HSCROLL()
		HANDLE_KEYDOWN()
		HANDLE_KILLFOCUS()
		HANDLE_LBUTTONDBLCLK()
		HANDLE_LBUTTONDOWN()
		HANDLE_LBUTTONUP()
#if !defined _WIN32_WCE || _WIN32_WCE >= 211
		HANDLE_MOUSEWHEEL()
#endif
		HANDLE_PAINT()
		HANDLE_SETFOCUS()
		HANDLE_SIZE()
		HANDLE_VSCROLL()
		HANDLE_MESSAGE(ListWindowImpl::WM_VIEWMODEL_ITEMADDED, onViewModelItemAdded)
		HANDLE_MESSAGE(ListWindowImpl::WM_VIEWMODEL_ITEMREMOVED, onViewModelItemRemoved)
	END_MESSAGE_HANDLER()
	return DefaultWindowHandler::windowProc(uMsg, wParam, lParam);
}

LRESULT qm::ListWindow::onContextMenu(HWND hwnd, const POINT& pt)
{
	DECLARE_QSTATUS();
	
	setFocus();
	
	HMENU hmenu = 0;
	status = pImpl_->pMenuManager_->getMenu(L"list", false, false, &hmenu);
	if (status == QSTATUS_SUCCESS) {
		UINT nFlags = TPM_LEFTALIGN | TPM_TOPALIGN;
#ifndef _WIN32_WCE
		nFlags |= TPM_LEFTBUTTON | TPM_RIGHTBUTTON;
#endif
		::TrackPopupMenu(hmenu, nFlags, pt.x, pt.y, 0, getParentFrame(), 0);
	}
	
	return DefaultWindowHandler::onContextMenu(hwnd, pt);
}

LRESULT qm::ListWindow::onCreate(CREATESTRUCT* pCreateStruct)
{
	if (DefaultWindowHandler::onCreate(pCreateStruct) == -1)
		return -1;
	
	DECLARE_QSTATUS();
	
	ListWindowCreateContext* pContext =
		static_cast<ListWindowCreateContext*>(pCreateStruct->lpCreateParams);
	pImpl_->pDocument_ = pContext->pDocument_;
	pImpl_->pMenuManager_ = pContext->pMenuManager_;
	
	status = pContext->pKeyMap_->createAccelerator(
		CustomAcceleratorFactory(), L"ListWindow",
		mapKeyNameToId, countof(mapKeyNameToId), &pImpl_->pAccelerator_);
	CHECK_QSTATUS_VALUE(-1);
	
	status = UIUtil::createFontFromProfile(pImpl_->pProfile_,
		L"ListWindow", false, &pImpl_->hfont_);
	CHECK_QSTATUS_VALUE(-1);
	
	ClientDeviceContext dc(getHandle(), &status);
	CHECK_QSTATUS_VALUE(-1);
	ObjectSelector<HFONT> selector(dc, pImpl_->hfont_);
	TEXTMETRIC tm;
	dc.getTextMetrics(&tm);
	pImpl_->nLineHeight_ = tm.tmHeight +
		tm.tmExternalLeading + ListWindowImpl::LINE_SPACING;
	
	status = pImpl_->createHeaderColumn();
	CHECK_QSTATUS_VALUE(-1);
	
	pImpl_->hImageList_ = ImageList_LoadImage(
		Application::getApplication().getResourceHandle(),
		MAKEINTRESOURCE(IDB_LIST), 16, 0, CLR_DEFAULT, IMAGE_BITMAP, 0);
#ifdef _WIN32_WCE_PSPC
	ImageList_SetBkColor(pImpl_->hImageList_, CLR_NONE);
#endif
	pImpl_->hImageListData_ = ImageList_LoadBitmap(
		Application::getApplication().getResourceHandle(),
		MAKEINTRESOURCE(IDB_LISTDATA), 8, 0, RGB(255, 255, 255));
	
	pImpl_->hpenThreadLine_ = ::CreatePen(PS_SOLID, 1, RGB(0, 0x80, 0));
	pImpl_->hpenFocusedThreadLine_ = ::CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
	
	refresh();
	
	status = newQsObject(getHandle(), &pImpl_->pDragGestureRecognizer_);
	CHECK_QSTATUS();
	pImpl_->pDragGestureRecognizer_->setDragGestureHandler(pImpl_);
	
	status = newQsObject(getHandle(), &pImpl_->pDropTarget_);
	CHECK_QSTATUS_VALUE(-1);
	pImpl_->pDropTarget_->setDropTargetHandler(pImpl_);
	
	return 0;
}

LRESULT qm::ListWindow::onDestroy()
{
	if (pImpl_->hfont_) {
		::DeleteObject(pImpl_->hfont_);
		pImpl_->hfont_ = 0;
	}
	
	if (pImpl_->hImageList_) {
		ImageList_Destroy(pImpl_->hImageList_);
		pImpl_->hImageList_ = 0;
	}
	if (pImpl_->hImageListData_) {
		ImageList_Destroy(pImpl_->hImageListData_);
		pImpl_->hImageListData_ = 0;
	}
	
	if (pImpl_->hpenThreadLine_) {
		::DeleteObject(pImpl_->hpenThreadLine_);
		pImpl_->hpenThreadLine_ = 0;
	}
	if (pImpl_->hpenFocusedThreadLine_) {
		::DeleteObject(pImpl_->hpenFocusedThreadLine_);
		pImpl_->hpenFocusedThreadLine_ = 0;
	}
	
	delete pImpl_->pDragGestureRecognizer_;
	delete pImpl_->pDropTarget_;
	
	return DefaultWindowHandler::onDestroy();
}

LRESULT qm::ListWindow::onEraseBkgnd(HDC hdc)
{
	return 1;
}

LRESULT qm::ListWindow::onHScroll(UINT nCode, UINT nPos, HWND hwnd)
{
	SCROLLINFO si = {
		sizeof(si),
		SIF_RANGE | SIF_POS | SIF_PAGE
	};
	getScrollInfo(SB_HORZ, &si);
	
	int nNewPos = si.nPos;
	bool bScroll = true;
	switch (nCode) {
	case SB_LINELEFT:
		nNewPos -= 10;
		break;
	case SB_LINERIGHT:
		nNewPos += 10;
		break;
	case SB_PAGELEFT:
		nNewPos -= si.nPage;
		break;
	case SB_PAGERIGHT:
		nNewPos += si.nPage;
		break;
	case SB_LEFT:
		nNewPos = 0;
		break;
	case SB_RIGHT:
		nNewPos = si.nMax;
		break;
	case SB_THUMBPOSITION:
	case SB_THUMBTRACK:
		nNewPos = nPos;
		break;
	default:
		bScroll = false;
		break;
	}
	if (bScroll)
		pImpl_->scrollHorizontal(nNewPos);
	
	return 0;
}

LRESULT qm::ListWindow::onKeyDown(UINT nKey, UINT nRepeat, UINT nFlags)
{
	DECLARE_QSTATUS();
	
	switch (nKey) {
	case VK_UP:
		status = moveSelection(MOVESELECTION_LINEUP,
			::GetKeyState(VK_SHIFT) < 0, ::GetKeyState(VK_CONTROL) < 0);
		CHECK_QSTATUS_VALUE(0);
		break;
	case VK_DOWN:
		status = moveSelection(MOVESELECTION_LINEDOWN,
			::GetKeyState(VK_SHIFT) < 0, ::GetKeyState(VK_CONTROL) < 0);
		CHECK_QSTATUS_VALUE(0);
		break;
	case VK_LEFT:
		status = moveSelection(MOVESELECTION_PAGEUP,
			::GetKeyState(VK_SHIFT) < 0, ::GetKeyState(VK_CONTROL) < 0);
		CHECK_QSTATUS_VALUE(0);
		break;
	case VK_RIGHT:
		status = moveSelection(MOVESELECTION_PAGEDOWN,
			::GetKeyState(VK_SHIFT) < 0, ::GetKeyState(VK_CONTROL) < 0);
		CHECK_QSTATUS_VALUE(0);
		break;
	case VK_PRIOR:
		status = moveSelection(MOVESELECTION_PAGEUP,
			::GetKeyState(VK_SHIFT) < 0, ::GetKeyState(VK_CONTROL) < 0);
		CHECK_QSTATUS_VALUE(0);
		break;
	case VK_NEXT:
		status = moveSelection(MOVESELECTION_PAGEDOWN,
			::GetKeyState(VK_SHIFT) < 0, ::GetKeyState(VK_CONTROL) < 0);
		CHECK_QSTATUS_VALUE(0);
		break;
	case VK_HOME:
		status = moveSelection(MOVESELECTION_TOP,
			::GetKeyState(VK_SHIFT) < 0, ::GetKeyState(VK_CONTROL) < 0);
		CHECK_QSTATUS_VALUE(0);
		break;
	case VK_END:
		status = moveSelection(MOVESELECTION_BOTTOM,
			::GetKeyState(VK_SHIFT) < 0, ::GetKeyState(VK_CONTROL) < 0);
		CHECK_QSTATUS_VALUE(0);
		break;
	case VK_SPACE:
		status = moveSelection(MOVESELECTION_CURRENT,
			::GetKeyState(VK_SHIFT) < 0, ::GetKeyState(VK_CONTROL) < 0);
		CHECK_QSTATUS_VALUE(0);
		break;
	case VK_RETURN:
		{
			ViewModel* pViewModel = pImpl_->pViewModelManager_->getCurrentViewModel();
			if (pViewModel) {
				Lock<ViewModel> lock(*pViewModel);
				unsigned int nItem = pViewModel->getFocused();
				if (nItem < pViewModel->getCount()) {
					status = pImpl_->pMessageFrameWindowManager_->open(
						pViewModel, pViewModel->getMessageHolder(nItem));
					CHECK_QSTATUS();
				}
			}
		}
		break;
	default:
		break;
	}
	
	return 0;
}

LRESULT qm::ListWindow::onKillFocus(HWND hwnd)
{
	pImpl_->invalidateSelected();
	return 0;
}

LRESULT qm::ListWindow::onLButtonDblClk(UINT nFlags, const POINT& pt)
{
	DECLARE_QSTATUS();
	
	ViewModel* pViewModel = pImpl_->pViewModelManager_->getCurrentViewModel();
	if (pViewModel) {
		Lock<ViewModel> lock(*pViewModel);
		
		unsigned int nLine = pImpl_->getLineFromPoint(pt);
		if (nLine != static_cast<unsigned int>(-1)) {
			status = pImpl_->pMessageFrameWindowManager_->open(
				pViewModel, pViewModel->getMessageHolder(nLine));
			CHECK_QSTATUS();
		}
	}
	
	return 0;
}

LRESULT qm::ListWindow::onLButtonDown(UINT nFlags, const POINT& pt)
{
	DECLARE_QSTATUS();
	
	setFocus();
	
#if defined _WIN32_WCE && _WIN32_WCE >= 300 && _WIN32_WCE_PSPC
	bool bTapAndHold = false;
#endif
	
	ViewModel* pViewModel = pImpl_->pViewModelManager_->getCurrentViewModel();
	if (pViewModel) {
		Lock<ViewModel> lock(*pViewModel);
		
		unsigned int nLine = pImpl_->getLineFromPoint(pt);
		if (nLine != static_cast<unsigned int>(-1)) {
			bool bSelected = pViewModel->isSelected(nLine);
			
			status = pViewModel->setFocused(nLine);
			CHECK_QSTATUS_VALUE(0);
			
			if (!(nFlags & MK_CONTROL) && !bSelected) {
				status = pViewModel->clearSelection();
				CHECK_QSTATUS_VALUE(0);
			}
			
			if (nFlags & MK_SHIFT) {
				unsigned int nLast = pViewModel->getLastSelection();
				if (nLast < nLine) {
					while (nLast <= nLine) {
						status = pViewModel->addSelection(nLast);
						CHECK_QSTATUS_VALUE(0);
						++nLast;
					}
				}
				else {
					while (nLast >= nLine) {
						status = pViewModel->addSelection(nLine);
						CHECK_QSTATUS_VALUE(0);
						++nLine;
					}
				}
			}
			else {
				if (nFlags & MK_CONTROL) {
					if (pViewModel->isSelected(nLine))
						status = pViewModel->removeSelection(nLine);
					else
						status = pViewModel->addSelection(nLine);
					CHECK_QSTATUS_VALUE(0);
				}
				else {
					if (!bSelected) {
						status = pViewModel->addSelection(nLine);
						CHECK_QSTATUS_VALUE(0);
					}
				}
				pViewModel->setLastSelection(nLine);
			}
			
			pImpl_->ensureVisible(nLine);
			
#if defined _WIN32_WCE && _WIN32_WCE >= 300 && _WIN32_WCE_PSPC
			if (tapAndHold(pt))
				return 0;
			bTapAndHold = true;
			
			status = pImpl_->pMessageFrameWindowManager_->open(
				pViewModel, pViewModel->getMessageHolder(nLine));
			CHECK_QSTATUS();
#endif
		}
	}
	
#if defined _WIN32_WCE && _WIN32_WCE >= 300 && _WIN32_WCE_PSPC
	if (!bTapAndHold) {
		if (tapAndHold(pt))
			return 0;
	}
#endif
	
	return 0;
}

LRESULT qm::ListWindow::onLButtonUp(UINT nFlags, const POINT& pt)
{
	DECLARE_QSTATUS();
	
	ViewModel* pViewModel = pImpl_->pViewModelManager_->getCurrentViewModel();
	if (pViewModel) {
		Lock<ViewModel> lock(*pViewModel);
		
		unsigned int nLine = pImpl_->getLineFromPoint(pt);
		if (nLine != static_cast<unsigned int>(-1)) {
			bool bSelected = pViewModel->isSelected(nLine);
			
			if (bSelected && !(nFlags & MK_SHIFT) && !(nFlags & MK_CONTROL)) {
				status = pViewModel->setSelection(nLine);
				CHECK_QSTATUS();
			}
		}
	}
	
	return 0;
}

#if !defined _WIN32_WCE || _WIN32_WCE >= 211
LRESULT qm::ListWindow::onMouseWheel(UINT nFlags, short nDelta, const POINT& pt)
{
#ifdef _WIN32_WCE
#	define WHEEL_DELTA 120
#endif
	pImpl_->scrollVertical(getScrollPos(SB_VERT) - nDelta/WHEEL_DELTA*3);
	
	return 0;
}
#endif

LRESULT qm::ListWindow::onPaint()
{
	DECLARE_QSTATUS();
	
	PaintDeviceContext dc(getHandle(), &status);
	// TODO
	// Error handling
	
	ObjectSelector<HFONT> fontSelector(dc, pImpl_->hfont_);
	
	ViewModel* pViewModel = pImpl_->pViewModelManager_->getCurrentViewModel();
	if (pViewModel) {
		Lock<ViewModel> lock(*pViewModel);
		
		RECT rectClip;
		dc.getClipBox(&rectClip);
		
		RECT rect;
		pImpl_->getRect(&rect);
		int nBottom = rect.bottom;
		rect.bottom = rect.top + pImpl_->nLineHeight_;
		
		SCROLLINFO si = {
			sizeof(si),
			SIF_PAGE | SIF_POS
		};
		getScrollInfo(SB_VERT, &si);
		
		int nHorzPos = getScrollPos(SB_HORZ);
		
		unsigned int nCount = pViewModel->getCount();
		for (unsigned int n = si.nPos; n < nCount && n <= si.nPos + si.nPage; ++n) {
			RECT rectTemp;
			if (::IntersectRect(&rectTemp, &rect, &rectClip)) {
				RECT r = {
					rect.left - nHorzPos,
					rect.top,
					rect.right,
					rect.bottom
				};
				PaintInfo pi(&dc, pViewModel, n, r);
				pImpl_->paintMessage(pi);
			}
			rect.top = rect.bottom;
			rect.bottom += pImpl_->nLineHeight_;
		}
		
		rect.bottom = nBottom;
		dc.fillSolidRect(rect, ::GetSysColor(COLOR_WINDOW));
	}
	else {
		RECT rect;
		getClientRect(&rect);
		dc.fillSolidRect(rect, ::GetSysColor(COLOR_WINDOW));
	}
	
	return 0;
}

LRESULT qm::ListWindow::onSetFocus(HWND hwnd)
{
	pImpl_->invalidateSelected();
	return 0;
}

LRESULT qm::ListWindow::onSize(UINT nFlags, int cx, int cy)
{
	if (pImpl_->pHeaderColumn_)
		pImpl_->layoutChildren(cx, cy);
	
	return DefaultWindowHandler::onSize(nFlags, cx, cy);
}

LRESULT qm::ListWindow::onVScroll(UINT nCode, UINT nPos, HWND hwnd)
{
	SCROLLINFO si = {
		sizeof(si),
		SIF_RANGE | SIF_POS | SIF_PAGE
	};
	getScrollInfo(SB_VERT, &si);
	
	int nNewPos = si.nPos;
	bool bScroll = true;
	switch (nCode) {
	case SB_LINEUP:
		--nNewPos;
		break;
	case SB_LINEDOWN:
		++nNewPos;
		break;
	case SB_PAGEUP:
		nNewPos -= si.nPage;
		break;
	case SB_PAGEDOWN:
		nNewPos += si.nPage;
		break;
	case SB_TOP:
		nNewPos = 0;
		break;
	case SB_BOTTOM:
		nNewPos = si.nMax;
		break;
	case SB_THUMBPOSITION:
	case SB_THUMBTRACK:
		nNewPos = nPos;
		break;
	default:
		bScroll = false;
		break;
	}
	if (bScroll)
		pImpl_->scrollVertical(nNewPos);
	
	return 0;
}

LRESULT qm::ListWindow::onViewModelItemAdded(WPARAM wParam, LPARAM lParam)
{
	// TODO
	// Performance up
	refresh();
	return 0;
}

LRESULT qm::ListWindow::onViewModelItemRemoved(WPARAM wParam, LPARAM lParam)
{
	// TODO
	// Performance up
	refresh();
	return 0;
}

bool qm::ListWindow::isShow() const
{
	return isVisible();
}

bool qm::ListWindow::isActive() const
{
	return hasFocus();
}

QSTATUS qm::ListWindow::setActive()
{
	setFocus();
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * ListHeaderColumnImpl
 *
 */

struct qm::ListHeaderColumnImpl :
	public NotifyHandler,
	public DefaultViewModelHandler
{
public:
	void updateSortIcon();

public:
	virtual LRESULT onNotify(NMHDR* pnmhdr, bool* pbHandled);

private:
	LRESULT onItemChanging(NMHDR* pnmhdr, bool* pbHandled);
	LRESULT onItemClick(NMHDR* pnmhdr, bool* pbHandled);

public:
	virtual QSTATUS sorted(const ViewModelEvent& event);

public:
	ListHeaderColumn* pThis_;
	ListWindow* pListWindow_;
	UINT nId_;
	Profile* pProfile_;
	ViewModel* pViewModel_;
	
	bool bShow_;
};

void qm::ListHeaderColumnImpl::updateSortIcon()
{
	assert(pViewModel_);
	
	HDITEM item = { HDI_IMAGE };
	for (int n = 0; n < Header_GetItemCount(pThis_->getHandle()); ++n)
		Header_SetItem(pThis_->getHandle(), n, &item);
	
	unsigned int nSort = pViewModel_->getSort();
	item.iImage = (nSort & ViewModel::SORT_DIRECTION_MASK) == ViewModel::SORT_ASCENDING ? 2 : 1;
	Header_SetItem(pThis_->getHandle(), nSort & ViewModel::SORT_INDEX_MASK, &item);
}

LRESULT qm::ListHeaderColumnImpl::onNotify(NMHDR* pnmhdr, bool* pbHandled)
{
	BEGIN_NOTIFY_HANDLER()
		HANDLE_NOTIFY(HDN_ITEMCHANGING, nId_, onItemChanging)
		HANDLE_NOTIFY(HDN_ITEMCLICK, nId_, onItemClick)
	END_NOTIFY_HANDLER()
	
	return 0;
}

LRESULT qm::ListHeaderColumnImpl::onItemChanging(NMHDR* pnmhdr, bool* pbHandled)
{
	NMHEADER* pHeader = reinterpret_cast<NMHEADER*>(pnmhdr);
	if (pHeader->pitem->mask & HDI_WIDTH) {
		pViewModel_->getColumn(pHeader->iItem).setWidth(pHeader->pitem->cxy);
		pListWindow_->refresh();
	}
	
	*pbHandled = true;
	
	return 0;
}

LRESULT qm::ListHeaderColumnImpl::onItemClick(NMHDR* pnmhdr, bool* pbHandled)
{
	NMHEADER* pHeader = reinterpret_cast<NMHEADER*>(pnmhdr);
	pViewModel_->setSort(pHeader->iItem);
	return 0;
}

QSTATUS qm::ListHeaderColumnImpl::sorted(const ViewModelEvent& event)
{
	assert(event.getViewModel() == pViewModel_);
	updateSortIcon();
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * ListHeaderColumn
 *
 */

qm::ListHeaderColumn::ListHeaderColumn(ListWindow* pListWindow,
	Profile* pProfile, QSTATUS* pstatus) :
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
	pImpl_->pListWindow_ = pListWindow;
	pImpl_->nId_ = 0;
	pImpl_->pProfile_ = pProfile;
	pImpl_->pViewModel_ = 0;
	
	int nShow = 0;
	status = pProfile->getInt(L"ListWindow", L"ShowHeaderColumn", 1, &nShow);
	CHECK_QSTATUS_SET(pstatus);
	pImpl_->bShow_ = nShow != 0;
	
	setWindowHandler(this, false);
	
	pListWindow->addNotifyHandler(pImpl_);
}

qm::ListHeaderColumn::~ListHeaderColumn()
{
	if (pImpl_) {
		delete pImpl_;
		pImpl_ = 0;
	}
}

int qm::ListHeaderColumn::getWidth() const
{
	int nWidth = 0;
	if (pImpl_->pViewModel_) {
		for (unsigned int n = 0; n < pImpl_->pViewModel_->getColumnCount(); ++n)
			nWidth += pImpl_->pViewModel_->getColumn(n).getWidth();
	}
	return nWidth;
}

int qm::ListHeaderColumn::getHeight() const
{
	if (isVisible()) {
		RECT rect;
		getWindowRect(&rect);
		return rect.bottom - rect.top;
	}
	else {
		return 0;
	}
}

QSTATUS qm::ListHeaderColumn::setViewModel(ViewModel* pViewModel)
{
	for (int n = Header_GetItemCount(getHandle()) - 1; n >= 0; --n)
		Header_DeleteItem(getHandle(), n);
	if (pImpl_->pViewModel_)
		pImpl_->pViewModel_->removeViewModelHandler(pImpl_);
	
	if (pViewModel) {
		unsigned int nColumn = 0;
		while (nColumn < pViewModel->getColumnCount()) {
			ViewColumn& column = pViewModel->getColumn(nColumn);
			W2T(column.getTitle(), ptszTitle);
			HDITEM item = {
				HDI_TEXT | HDI_WIDTH | HDI_FORMAT | HDI_IMAGE,
				column.getWidth(),
				const_cast<LPTSTR>(ptszTitle),
				0,
				0,
				HDF_STRING | HDF_IMAGE | (column.isFlag(ViewColumn::FLAG_RIGHTALIGN) ? HDF_RIGHT : HDF_LEFT),
				0,
				0
			};
			Header_InsertItem(getHandle(), nColumn, &item);
			
			++nColumn;
		}
		if (pImpl_->bShow_)
			showWindow(SW_SHOW);
	}
	else {
		showWindow(SW_HIDE);
	}
	
	pImpl_->pViewModel_ = pViewModel;
	
	if (pImpl_->pViewModel_) {
		pImpl_->pViewModel_->addViewModelHandler(pImpl_);
		pImpl_->updateSortIcon();
	}
	
	return QSTATUS_SUCCESS;
}

bool qm::ListHeaderColumn::isShow() const
{
	return pImpl_->bShow_;
}

QSTATUS qm::ListHeaderColumn::setShow(bool bShow)
{
	if (bShow) {
		if (pImpl_->pViewModel_)
			showWindow(SW_SHOW);
	}
	else {
		showWindow(SW_HIDE);
	}
	pImpl_->bShow_ = bShow;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ListHeaderColumn::save() const
{
	DECLARE_QSTATUS();
	
	status = pImpl_->pProfile_->setInt(L"ListWindow",
		L"ShowHeaderColumn", pImpl_->bShow_ ? 1 : 0);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ListHeaderColumn::getSuperClass(WSTRING* pwstrSuperClass)
{
	assert(pwstrSuperClass);
	*pwstrSuperClass = allocWString(WC_HEADERW);
	return *pwstrSuperClass ? QSTATUS_SUCCESS : QSTATUS_OUTOFMEMORY;
}

QSTATUS qm::ListHeaderColumn::preCreateWindow(CREATESTRUCT* pCreateStruct)
{
	DECLARE_QSTATUS();
	
	ClientDeviceContext dc(0, &status);
	CHECK_QSTATUS();
	ObjectSelector<HFONT> selector(dc, pImpl_->pListWindow_->getFont());
	TEXTMETRIC tm;
	dc.getTextMetrics(&tm);
	pCreateStruct->cy = tm.tmHeight + tm.tmExternalLeading + 8;
	pCreateStruct->style |= HDS_FULLDRAG | HDS_BUTTONS | HDS_HORZ;
	
	return DefaultWindowHandler::preCreateWindow(pCreateStruct);
}

LRESULT qm::ListHeaderColumn::windowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	BEGIN_MESSAGE_HANDLER()
		HANDLE_CREATE()
		HANDLE_DESTROY()
	END_MESSAGE_HANDLER()
	return DefaultWindowHandler::windowProc(uMsg, wParam, lParam);
}

LRESULT qm::ListHeaderColumn::onCreate(CREATESTRUCT* pCreateStruct)
{
	if (DefaultWindowHandler::onCreate(pCreateStruct) == -1)
		return -1;
	
	ListWindow* pListWindow = pImpl_->pListWindow_;
	setFont(pListWindow->getFont());
	pImpl_->nId_ = getWindowLong(GWL_ID);
	
	HIMAGELIST hImageList = ImageList_LoadImage(
		Application::getApplication().getResourceHandle(),
		MAKEINTRESOURCE(IDB_HEADER), 16, 0, CLR_DEFAULT, IMAGE_BITMAP, 0);
	Header_SetImageList(getHandle(), hImageList);
	
	return 0;
}

LRESULT qm::ListHeaderColumn::onDestroy()
{
	HIMAGELIST hImageList = Header_SetImageList(getHandle(), 0);
	ImageList_Destroy(hImageList);
	
	pImpl_->pListWindow_->removeNotifyHandler(pImpl_);
	
	return DefaultWindowHandler::onDestroy();
}
