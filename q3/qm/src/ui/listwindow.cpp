/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
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
#include <qskeymap.h>
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
	bool createHeaderColumn();
	void layoutChildren();
	void layoutChildren(int cx, int cy);
	
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
	virtual void viewModelSelected(const ViewModelManagerEvent& event);

public:
	virtual void itemAdded(const ViewModelEvent& event);
	virtual void itemRemoved(const ViewModelEvent& event);
	virtual void itemChanged(const ViewModelEvent& event);
	virtual void itemStateChanged(const ViewModelEvent& event);
	virtual void itemAttentionPaid(const ViewModelEvent& event);
	virtual void updated(const ViewModelEvent& event);
	virtual void sorted(const ViewModelEvent& event);
	virtual void destroyed(const ViewModelEvent& event);

public:
	virtual void dragGestureRecognized(const DragGestureEvent& event);

public:
	virtual void dragDropEnd(const DragSourceDropEvent& event);

public:
	virtual void dragEnter(const DropTargetDragEvent& event);
	virtual void dragOver(const DropTargetDragEvent& event);
	virtual void dragExit(const DropTargetEvent& event);
	virtual void drop(const DropTargetDropEvent& event);

private:
	static int getMessageImage(MessageHolder* pmh,
							   unsigned int nFlags);

public:
	ListWindow* pThis_;
	
	Profile* pProfile_;
	MessageFrameWindowManager* pMessageFrameWindowManager_;
	MenuManager* pMenuManager_;
	std::auto_ptr<Accelerator> pAccelerator_;
	Document* pDocument_;
	ViewModelManager* pViewModelManager_;
	
	HFONT hfont_;
	int nLineHeight_;
	ListHeaderColumn* pHeaderColumn_;
	HIMAGELIST hImageList_;
	HIMAGELIST hImageListData_;
	HPEN hpenThreadLine_;
	HPEN hpenFocusedThreadLine_;
	std::auto_ptr<DragGestureRecognizer> pDragGestureRecognizer_;
	std::auto_ptr<DropTarget> pDropTarget_;
	bool bCanDrop_;
};

bool qm::ListWindowImpl::createHeaderColumn()
{
	std::auto_ptr<ListHeaderColumn> pHeaderColumn(
		new ListHeaderColumn(pThis_, pProfile_));
	
	RECT rect;
	pThis_->getClientRect(&rect);
	if (!pHeaderColumn->create(L"QmListHeaderColumn", 0,
		WS_CHILD, 0, 0, rect.right - rect.left, 0,
		pThis_->getHandle(), 0, 0, ID_HEADERCOLUMN, 0))
		return false;
	
	pHeaderColumn_ = pHeaderColumn.release();
	
	return true;
}

void qm::ListWindowImpl::layoutChildren()
{
	RECT rect;
	pThis_->getClientRect(&rect);
	layoutChildren(rect.right - rect.left, rect.bottom - rect.top);
}

void qm::ListWindowImpl::layoutChildren(int cx,
										int cy)
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
}

void qm::ListWindowImpl::getRect(RECT* pRect)
{
	pThis_->getClientRect(pRect);
	pRect->top += pHeaderColumn_->getHeight();
}

void qm::ListWindowImpl::paintMessage(const PaintInfo& pi)
{
	DeviceContext* pdc = pi.getDeviceContext();
	ViewModel* pViewModel = pi.getViewModel();
	const ViewModelItem* pItem = pi.getItem();
	MessageHolder* pmh = pItem->getMessageHolder();
	const RECT& rect = pi.getRect();
	
	bool bHasFocus = pThis_->hasFocus();
	bool bSelected = (pItem->getFlags() & ViewModelItem::FLAG_SELECTED) != 0;
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
			wstring_ptr wstrText(pColumn->getText(pmh));
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
			Line line(nLevel, 0);
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
	
#if !defined _WIN32_WCE || _WIN32_WCE < 300 || !defined _WIN32_WCE_PSPC
	if ((pItem->getFlags() & ViewModelItem::FLAG_FOCUSED) && bHasFocus) {
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
	SCROLLINFO si = {
		sizeof(si),
		SIF_POS | SIF_PAGE | SIF_RANGE
	};
	pThis_->getScrollInfo(SB_VERT, &si);
	if (si.nMax - si.nMin < static_cast<int>(si.nPage))
		return;
	
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
		
		unsigned int nPage = (rect.bottom - rect.top)/nLineHeight_;
		if (nPage > nCount)
			nPage = nCount;
		SCROLLINFO si = {
			sizeof(si),
			SIF_PAGE | SIF_RANGE,
			0,
			nCount == 0 ? 0 : nCount - 1,
			nPage,
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

void qm::ListWindowImpl::viewModelSelected(const ViewModelManagerEvent& event)
{
	ViewModel* pOldViewModel = event.getOldViewModel();
	if (pOldViewModel)
		pOldViewModel->removeViewModelHandler(this);
	
	ViewModel* pNewViewModel = event.getNewViewModel();
	if (pNewViewModel)
		pNewViewModel->addViewModelHandler(this);
	
	pHeaderColumn_->setViewModel(pNewViewModel);
	
	pThis_->refresh();
	
	if (pNewViewModel)
		ensureVisible(pNewViewModel->getFocused());
}

void qm::ListWindowImpl::itemAdded(const ViewModelEvent& event)
{
	assert(event.getViewModel() == pViewModelManager_->getCurrentViewModel());
	pThis_->postMessage(WM_VIEWMODEL_ITEMADDED, event.getItem(), 0);
}

void qm::ListWindowImpl::itemRemoved(const ViewModelEvent& event)
{
	assert(event.getViewModel() == pViewModelManager_->getCurrentViewModel());
	pThis_->postMessage(WM_VIEWMODEL_ITEMREMOVED, event.getItem(), 0);
}

void qm::ListWindowImpl::itemChanged(const ViewModelEvent& event)
{
	assert(event.getViewModel() == pViewModelManager_->getCurrentViewModel());
	invalidateLine(event.getItem());
}

void qm::ListWindowImpl::itemStateChanged(const ViewModelEvent& event)
{
	assert(event.getViewModel() == pViewModelManager_->getCurrentViewModel());
	invalidateLine(event.getItem());
}

void qm::ListWindowImpl::itemAttentionPaid(const ViewModelEvent& event)
{
	assert(event.getViewModel() == pViewModelManager_->getCurrentViewModel());
	ensureVisible(event.getItem());
}

void qm::ListWindowImpl::updated(const ViewModelEvent& event)
{
	assert(event.getViewModel() == pViewModelManager_->getCurrentViewModel());
	pThis_->refresh();
}

void qm::ListWindowImpl::sorted(const ViewModelEvent& event)
{
	assert(event.getViewModel() == pViewModelManager_->getCurrentViewModel());
	ensureVisible(event.getViewModel()->getFocused());
	pThis_->invalidate();
}

void qm::ListWindowImpl::destroyed(const ViewModelEvent& event)
{
	assert(false);
}

void qm::ListWindowImpl::dragGestureRecognized(const DragGestureEvent& event)
{
	ViewModel* pViewModel = pViewModelManager_->getCurrentViewModel();
	if (!pViewModel)
		return;
	
	Lock<ViewModel> lock(*pViewModel);
	
	MessageHolderList l;
	pViewModel->getSelection(&l);
	if (l.empty())
		return;
	
	std::auto_ptr<MessageDataObject> p(new MessageDataObject(pDocument_,
		pViewModel->getFolder()->getAccount(), l, MessageDataObject::FLAG_NONE));
	p->AddRef();
	ComPtr<IDataObject> pDataObject(p.release());
	
	DragSource source;
	source.setDragSourceHandler(this);
	source.startDrag(pDataObject.get(), DROPEFFECT_COPY | DROPEFFECT_MOVE);
}

void qm::ListWindowImpl::dragDropEnd(const DragSourceDropEvent& event)
{
}

void qm::ListWindowImpl::dragEnter(const DropTargetDragEvent& event)
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

void qm::ListWindowImpl::dragOver(const DropTargetDragEvent& event)
{
	if (bCanDrop_) 
		event.setEffect(DROPEFFECT_COPY);
}

void qm::ListWindowImpl::dragExit(const DropTargetEvent& event)
{
	bCanDrop_ = false;
}

void qm::ListWindowImpl::drop(const DropTargetDropEvent& event)
{
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
							
							FileInputStream stream(pwszPath);
							if (!stream) {
								// TODO MSG
								return;
							}
							BufferedInputStream bufferedStream(&stream, false);
							
							if (!FileImportAction::readMessage(
								static_cast<NormalFolder*>(pFolder), &bufferedStream,
								false, Account::IMPORTFLAG_NORMALFLAGS, 0, 0, 0)) {
								// TODO MSG
							}
						}
					}
				}
			}
#endif
		}
	}
}

int qm::ListWindowImpl::getMessageImage(MessageHolder* pmh,
										unsigned int nFlags)
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

qm::ListWindow::ListWindow(ViewModelManager* pViewModelManager,
						   Profile* pProfile,
						   MessageFrameWindowManager* pMessageFrameWindowManager) :
	WindowBase(true),
	pImpl_(0)
{
	pImpl_ = new ListWindowImpl();
	pImpl_->pThis_ = this;
	pImpl_->pProfile_ = pProfile;
	pImpl_->pMessageFrameWindowManager_ = pMessageFrameWindowManager;
	pImpl_->pMenuManager_ = 0;
	pImpl_->pDocument_ = 0;
	pImpl_->pViewModelManager_ = pViewModelManager;
	pImpl_->hfont_ = 0;
	pImpl_->nLineHeight_ = 0;
	pImpl_->pHeaderColumn_ = 0;
	pImpl_->hImageList_ = 0;
	pImpl_->hImageListData_ = 0;
	pImpl_->hpenThreadLine_ = 0;
	pImpl_->hpenFocusedThreadLine_ = 0;
	pImpl_->bCanDrop_ = false;
	
	pImpl_->pViewModelManager_->addViewModelManagerHandler(pImpl_);
	
	setWindowHandler(this, false);
}

qm::ListWindow::~ListWindow()
{
	delete pImpl_;
}

void qm::ListWindow::refresh()
{
	pImpl_->updateScrollBar(true);
	pImpl_->updateScrollBar(false);
	invalidate();
}

void qm::ListWindow::moveSelection(MoveSelection m,
								   bool bShift,
								   bool bCtrl)
{
	ViewModel* pViewModel = pImpl_->pViewModelManager_->getCurrentViewModel();
	if (!pViewModel)
		return;
	
	Lock<ViewModel> lock(*pViewModel);
	
	unsigned int nCount = pViewModel->getCount();
	if (nCount == 0)
		return;
	
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
			pViewModel->setSelection(nNewLine, pViewModel->getLastSelection());
		}
		else {
			if (pViewModel->isSelected(nNewLine)) {
				if (bCtrl)
					pViewModel->removeSelection(nNewLine);
			}
			else {
				pViewModel->addSelection(nNewLine);
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
		pViewModel->setFocused(nNewLine);
		if (bShift) {
			pViewModel->setSelection(nNewLine, pViewModel->getLastSelection());
		}
		else {
			if (!bCtrl) {
				pViewModel->setSelection(nNewLine);
				pViewModel->setLastSelection(nNewLine);
			}
		}
		pImpl_->ensureVisible(nNewLine);
	}
}

HFONT qm::ListWindow::getFont() const
{
	return pImpl_->hfont_;
}

bool qm::ListWindow::isShowHeaderColumn() const
{
	return pImpl_->pHeaderColumn_->isShow();
}

void qm::ListWindow::setShowHeaderColumn(bool bShow)
{
	if (bShow != pImpl_->pHeaderColumn_->isShow()) {
		pImpl_->pHeaderColumn_->setShow(bShow);
		pImpl_->layoutChildren();
	}
}

bool qm::ListWindow::save()
{
	return pImpl_->pHeaderColumn_->save();
}

bool qm::ListWindow::preCreateWindow(CREATESTRUCT* pCreateStruct)
{
	if (!DefaultWindowHandler::preCreateWindow(pCreateStruct))
		return false;
	pCreateStruct->style |= WS_HSCROLL | WS_VSCROLL;
	return true;
}

Accelerator* qm::ListWindow::getAccelerator()
{
	return pImpl_->pAccelerator_.get();
}

LRESULT qm::ListWindow::windowProc(UINT uMsg,
								   WPARAM wParam,
								   LPARAM lParam)
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

LRESULT qm::ListWindow::onContextMenu(HWND hwnd,
									  const POINT& pt)
{
	setFocus();
	
	HMENU hmenu = pImpl_->pMenuManager_->getMenu(L"list", false, false);
	if (hmenu) {
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
	
	ListWindowCreateContext* pContext =
		static_cast<ListWindowCreateContext*>(pCreateStruct->lpCreateParams);
	pImpl_->pDocument_ = pContext->pDocument_;
	pImpl_->pMenuManager_ = pContext->pMenuManager_;
	
	CustomAcceleratorFactory acceleratorFactory;
	pImpl_->pAccelerator_ = pContext->pKeyMap_->createAccelerator(
		&acceleratorFactory, L"ListWindow", mapKeyNameToId, countof(mapKeyNameToId));
	if (!pImpl_->pAccelerator_.get())
		return -1;
	
	pImpl_->hfont_ = UIUtil::createFontFromProfile(
		pImpl_->pProfile_, L"ListWindow", false);
	
	ClientDeviceContext dc(getHandle());
	ObjectSelector<HFONT> selector(dc, pImpl_->hfont_);
	TEXTMETRIC tm;
	dc.getTextMetrics(&tm);
	pImpl_->nLineHeight_ = tm.tmHeight +
		tm.tmExternalLeading + ListWindowImpl::LINE_SPACING;
	
	if (!pImpl_->createHeaderColumn())
		return -1;
	
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
	
	pImpl_->pDragGestureRecognizer_.reset(new DragGestureRecognizer(getHandle()));
	pImpl_->pDragGestureRecognizer_->setDragGestureHandler(pImpl_);
	pImpl_->pDropTarget_.reset(new DropTarget(getHandle()));
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
	
	pImpl_->pDragGestureRecognizer_.reset(0);
	pImpl_->pDropTarget_.reset(0);
	
	return DefaultWindowHandler::onDestroy();
}

LRESULT qm::ListWindow::onEraseBkgnd(HDC hdc)
{
	return 1;
}

LRESULT qm::ListWindow::onHScroll(UINT nCode,
								  UINT nPos,
								  HWND hwnd)
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

LRESULT qm::ListWindow::onKeyDown(UINT nKey,
								  UINT nRepeat,
								  UINT nFlags)
{
	switch (nKey) {
	case VK_UP:
		moveSelection(MOVESELECTION_LINEUP,
			::GetKeyState(VK_SHIFT) < 0, ::GetKeyState(VK_CONTROL) < 0);
		break;
	case VK_DOWN:
		moveSelection(MOVESELECTION_LINEDOWN,
			::GetKeyState(VK_SHIFT) < 0, ::GetKeyState(VK_CONTROL) < 0);
		break;
	case VK_LEFT:
		moveSelection(MOVESELECTION_PAGEUP,
			::GetKeyState(VK_SHIFT) < 0, ::GetKeyState(VK_CONTROL) < 0);
		break;
	case VK_RIGHT:
		moveSelection(MOVESELECTION_PAGEDOWN,
			::GetKeyState(VK_SHIFT) < 0, ::GetKeyState(VK_CONTROL) < 0);
		break;
	case VK_PRIOR:
		moveSelection(MOVESELECTION_PAGEUP,
			::GetKeyState(VK_SHIFT) < 0, ::GetKeyState(VK_CONTROL) < 0);
		break;
	case VK_NEXT:
		moveSelection(MOVESELECTION_PAGEDOWN,
			::GetKeyState(VK_SHIFT) < 0, ::GetKeyState(VK_CONTROL) < 0);
		break;
	case VK_HOME:
		moveSelection(MOVESELECTION_TOP,
			::GetKeyState(VK_SHIFT) < 0, ::GetKeyState(VK_CONTROL) < 0);
		break;
	case VK_END:
		moveSelection(MOVESELECTION_BOTTOM,
			::GetKeyState(VK_SHIFT) < 0, ::GetKeyState(VK_CONTROL) < 0);
		break;
	case VK_SPACE:
		moveSelection(MOVESELECTION_CURRENT,
			::GetKeyState(VK_SHIFT) < 0, ::GetKeyState(VK_CONTROL) < 0);
		break;
	case VK_RETURN:
		{
			ViewModel* pViewModel = pImpl_->pViewModelManager_->getCurrentViewModel();
			if (pViewModel) {
				Lock<ViewModel> lock(*pViewModel);
				unsigned int nItem = pViewModel->getFocused();
				if (nItem < pViewModel->getCount()) {
					if (!pImpl_->pMessageFrameWindowManager_->open(
						pViewModel, pViewModel->getMessageHolder(nItem))) {
						// TODO MSG
					}
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

LRESULT qm::ListWindow::onLButtonDblClk(UINT nFlags,
										const POINT& pt)
{
	ViewModel* pViewModel = pImpl_->pViewModelManager_->getCurrentViewModel();
	if (pViewModel) {
		Lock<ViewModel> lock(*pViewModel);
		
		unsigned int nLine = pImpl_->getLineFromPoint(pt);
		if (nLine != static_cast<unsigned int>(-1)) {
			if (!pImpl_->pMessageFrameWindowManager_->open(
				pViewModel, pViewModel->getMessageHolder(nLine))) {
				// TODO MSG
			}
		}
	}
	
	return 0;
}

LRESULT qm::ListWindow::onLButtonDown(UINT nFlags,
									  const POINT& pt)
{
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
			
			pViewModel->setFocused(nLine);
			
			if (!(nFlags & MK_CONTROL) && !bSelected)
				pViewModel->clearSelection();
			
			if (nFlags & MK_SHIFT) {
				unsigned int nLast = pViewModel->getLastSelection();
				if (nLast < nLine) {
					for (unsigned int n = nLast; n <= nLine; ++n)
						pViewModel->addSelection(n);
				}
				else {
					for (unsigned int n = nLine; n <= nLast; ++n)
						pViewModel->addSelection(n);
				}
			}
			else {
				if (nFlags & MK_CONTROL) {
					if (pViewModel->isSelected(nLine))
						pViewModel->removeSelection(nLine);
					else
						pViewModel->addSelection(nLine);
				}
				else {
					if (!bSelected)
						pViewModel->addSelection(nLine);
				}
				pViewModel->setLastSelection(nLine);
			}
			
			pImpl_->ensureVisible(nLine);
			
#if defined _WIN32_WCE && _WIN32_WCE >= 300 && _WIN32_WCE_PSPC
			if (tapAndHold(pt))
				return 0;
			bTapAndHold = true;
			
			if (!pImpl_->pMessageFrameWindowManager_->open(
				pViewModel, pViewModel->getMessageHolder(nLine))) {
				// TODO MSG
			}
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

LRESULT qm::ListWindow::onLButtonUp(UINT nFlags,
									const POINT& pt)
{
	ViewModel* pViewModel = pImpl_->pViewModelManager_->getCurrentViewModel();
	if (pViewModel) {
		Lock<ViewModel> lock(*pViewModel);
		
		unsigned int nLine = pImpl_->getLineFromPoint(pt);
		if (nLine != static_cast<unsigned int>(-1)) {
			bool bSelected = pViewModel->isSelected(nLine);
			
			if (bSelected && !(nFlags & MK_SHIFT) && !(nFlags & MK_CONTROL))
				pViewModel->setSelection(nLine);
		}
	}
	
	return 0;
}

#if !defined _WIN32_WCE || _WIN32_WCE >= 211
LRESULT qm::ListWindow::onMouseWheel(UINT nFlags,
									 short nDelta,
									 const POINT& pt)
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
	PaintDeviceContext dc(getHandle());
	
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

LRESULT qm::ListWindow::onSize(UINT nFlags,
							   int cx,
							   int cy)
{
	if (pImpl_->pHeaderColumn_)
		pImpl_->layoutChildren(cx, cy);
	
	return DefaultWindowHandler::onSize(nFlags, cx, cy);
}

LRESULT qm::ListWindow::onVScroll(UINT nCode,
								  UINT nPos,
								  HWND hwnd)
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

LRESULT qm::ListWindow::onViewModelItemAdded(WPARAM wParam,
											 LPARAM lParam)
{
	// TODO
	// Performance up
	refresh();
	return 0;
}

LRESULT qm::ListWindow::onViewModelItemRemoved(WPARAM wParam,
											   LPARAM lParam)
{
	// TODO
	// Performance up
	refresh();
	return 0;
}

bool qm::ListWindow::isShow() const
{
	return (getStyle() & WS_VISIBLE) != 0;
}

bool qm::ListWindow::isActive() const
{
	return hasFocus();
}

void qm::ListWindow::setActive()
{
	setFocus();
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
	virtual LRESULT onNotify(NMHDR* pnmhdr,
							 bool* pbHandled);

private:
	LRESULT onItemChanging(NMHDR* pnmhdr,
						   bool* pbHandled);
	LRESULT onItemClick(NMHDR* pnmhdr,
						bool* pbHandled);

public:
	virtual void sorted(const ViewModelEvent& event);

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

LRESULT qm::ListHeaderColumnImpl::onNotify(NMHDR* pnmhdr,
										   bool* pbHandled)
{
	BEGIN_NOTIFY_HANDLER()
		HANDLE_NOTIFY(HDN_ITEMCHANGING, nId_, onItemChanging)
		HANDLE_NOTIFY(HDN_ITEMCLICK, nId_, onItemClick)
	END_NOTIFY_HANDLER()
	
	return 0;
}

LRESULT qm::ListHeaderColumnImpl::onItemChanging(NMHDR* pnmhdr,
												 bool* pbHandled)
{
	NMHEADER* pHeader = reinterpret_cast<NMHEADER*>(pnmhdr);
	if (pHeader->pitem->mask & HDI_WIDTH) {
		pViewModel_->getColumn(pHeader->iItem).setWidth(pHeader->pitem->cxy);
		pListWindow_->refresh();
	}
	
	*pbHandled = true;
	
	return 0;
}

LRESULT qm::ListHeaderColumnImpl::onItemClick(NMHDR* pnmhdr,
											  bool* pbHandled)
{
	NMHEADER* pHeader = reinterpret_cast<NMHEADER*>(pnmhdr);
	pViewModel_->setSort(pHeader->iItem);
	return 0;
}

void qm::ListHeaderColumnImpl::sorted(const ViewModelEvent& event)
{
	assert(event.getViewModel() == pViewModel_);
	updateSortIcon();
}


/****************************************************************************
 *
 * ListHeaderColumn
 *
 */

qm::ListHeaderColumn::ListHeaderColumn(ListWindow* pListWindow,
									   Profile* pProfile) :
	WindowBase(true),
	pImpl_(0)
{
	pImpl_ = new ListHeaderColumnImpl();
	pImpl_->pThis_ = this;
	pImpl_->pListWindow_ = pListWindow;
	pImpl_->nId_ = 0;
	pImpl_->pProfile_ = pProfile;
	pImpl_->pViewModel_ = 0;
	pImpl_->bShow_ = pProfile->getInt(L"ListWindow", L"ShowHeaderColumn", 1) != 0;
	
	setWindowHandler(this, false);
	
	pListWindow->addNotifyHandler(pImpl_);
}

qm::ListHeaderColumn::~ListHeaderColumn()
{
	delete pImpl_;
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
	if (getStyle() & WS_VISIBLE) {
		RECT rect;
		getWindowRect(&rect);
		return rect.bottom - rect.top;
	}
	else {
		return 0;
	}
}

void qm::ListHeaderColumn::setViewModel(ViewModel* pViewModel)
{
	for (int n = Header_GetItemCount(getHandle()) - 1; n >= 0; --n)
		Header_DeleteItem(getHandle(), n);
	if (pImpl_->pViewModel_)
		pImpl_->pViewModel_->removeViewModelHandler(pImpl_);
	
	if (pViewModel) {
		for (unsigned int nColumn = 0; nColumn < pViewModel->getColumnCount(); ++nColumn) {
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
}

bool qm::ListHeaderColumn::isShow() const
{
	return pImpl_->bShow_;
}

void qm::ListHeaderColumn::setShow(bool bShow)
{
	if (bShow) {
		if (pImpl_->pViewModel_)
			showWindow(SW_SHOW);
	}
	else {
		showWindow(SW_HIDE);
	}
	pImpl_->bShow_ = bShow;
}

bool qm::ListHeaderColumn::save()
{
	pImpl_->pProfile_->setInt(L"ListWindow", L"ShowHeaderColumn", pImpl_->bShow_ ? 1 : 0);
	return true;
}

wstring_ptr qm::ListHeaderColumn::getSuperClass()
{
	return allocWString(WC_HEADERW);
}

bool qm::ListHeaderColumn::preCreateWindow(CREATESTRUCT* pCreateStruct)
{
	ClientDeviceContext dc(0);
	ObjectSelector<HFONT> selector(dc, pImpl_->pListWindow_->getFont());
	TEXTMETRIC tm;
	dc.getTextMetrics(&tm);
	pCreateStruct->cy = tm.tmHeight + tm.tmExternalLeading + 8;
	pCreateStruct->style |= HDS_FULLDRAG | HDS_BUTTONS | HDS_HORZ;
	
	return DefaultWindowHandler::preCreateWindow(pCreateStruct);
}

LRESULT qm::ListHeaderColumn::windowProc(UINT uMsg,
										 WPARAM wParam,
										 LPARAM lParam)
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
