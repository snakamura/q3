/*
 * $Id: dragdrop.cpp,v 1.1.1.1 2003/04/29 08:07:35 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsassert.h>
#include <qsdragdrop.h>
#include <qserror.h>
#include <qsnew.h>

#include <algorithm>

#include "dragdrop.h"

using namespace qs;


/****************************************************************************
 *
 * DragSourceImpl
 *
 */

struct qs::DragSourceImpl
{
	class IDropSourceImpl : public IDropSource
	{
	public:
		IDropSourceImpl(DragSourceImpl* pDragSource, QSTATUS* pstatus);
		~IDropSourceImpl();
	
	public:
		STDMETHOD_(ULONG, AddRef)();
		STDMETHOD_(ULONG, Release)();
		STDMETHOD(QueryInterface)(REFIID riid, void** ppv);
	
	public:
		STDMETHOD(QueryContinueDrag)(BOOL bEscapePressed, DWORD dwKeyState);
		STDMETHOD(GiveFeedback)(DWORD dwEffect);
	
	private:
		IDropSourceImpl(const IDropSourceImpl&);
		IDropSourceImpl& operator=(const IDropSourceImpl&);
	
	private:
		ULONG nRef_;
		DragSourceImpl* pDragSource_;
	};
	
	DragSourceHandler* pHandler_;
	IDropSource* pDropSource_;
	DWORD dwDropButton_;
	DWORD dwCancelButton_;
};


/****************************************************************************
 *
 * DragSourceImpl::IDropSourceImpl
 *
 */

qs::DragSourceImpl::IDropSourceImpl::IDropSourceImpl(
	DragSourceImpl* pDragSource, QSTATUS* pstatus) :
	nRef_(0),
	pDragSource_(pDragSource)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qs::DragSourceImpl::IDropSourceImpl::~IDropSourceImpl()
{
}

STDMETHODIMP_(ULONG) qs::DragSourceImpl::IDropSourceImpl::AddRef()
{
	return ::InterlockedIncrement(reinterpret_cast<LONG*>(&nRef_));
}

STDMETHODIMP_(ULONG) qs::DragSourceImpl::IDropSourceImpl::Release()
{
	ULONG nRef = ::InterlockedDecrement(reinterpret_cast<LONG*>(&nRef_));
	if (nRef == 0)
		delete this;
	return nRef;
}

STDMETHODIMP qs::DragSourceImpl::IDropSourceImpl::QueryInterface(
	REFIID riid, void** ppv)
{
	*ppv = 0;
	
	if (riid == IID_IUnknown || riid == IID_IDropSource) {
		AddRef();
		*ppv = static_cast<IDropSource*>(this);
	}
	
	return *ppv ? S_OK : E_NOINTERFACE;
}

STDMETHODIMP qs::DragSourceImpl::IDropSourceImpl::QueryContinueDrag(
	BOOL bEscapePressed, DWORD dwKeyState)
{
	if (bEscapePressed || (dwKeyState & pDragSource_->dwCancelButton_))
		return DRAGDROP_S_CANCEL;
	else if (!(dwKeyState & pDragSource_->dwDropButton_))
		return DRAGDROP_S_DROP;
	else
		return S_OK;
}

STDMETHODIMP qs::DragSourceImpl::IDropSourceImpl::GiveFeedback(DWORD dwEffect)
{
	return DRAGDROP_S_USEDEFAULTCURSORS;
}


/****************************************************************************
 *
 * DragSource
 *
 */

qs::DragSource::DragSource(QSTATUS* pstatus) :
	pImpl_(0)
{
	assert(pstatus);
	
	DECLARE_QSTATUS();
	
	*pstatus = QSTATUS_SUCCESS;
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	pImpl_->pHandler_ = 0;
	pImpl_->dwDropButton_ = 0;
	pImpl_->dwCancelButton_ = 0;
	pImpl_->pDropSource_ = 0;
	
	DragSourceImpl::IDropSourceImpl* pDropSource = 0;
	status = newQsObject(pImpl_, &pDropSource);
	CHECK_QSTATUS_SET(pstatus);
	pDropSource->AddRef();
	pImpl_->pDropSource_ = pDropSource;
}

qs::DragSource::~DragSource()
{
	if (pImpl_) {
		pImpl_->pDropSource_->Release();
		delete pImpl_;
	}
}

QSTATUS qs::DragSource::startDrag(IDataObject* pDataObject, DWORD dwEffect)
{
	DECLARE_QSTATUS();
	
	if (::GetKeyState(VK_LBUTTON) < 0) {
		pImpl_->dwDropButton_ = MK_LBUTTON;
		pImpl_->dwCancelButton_ = MK_RBUTTON;
	}
	else if (::GetKeyState(VK_RBUTTON) < 0) {
		pImpl_->dwDropButton_ = MK_RBUTTON;
		pImpl_->dwCancelButton_ = MK_LBUTTON;
	}
	
	DWORD dwResultEffect = DROPEFFECT_NONE;
	bool bCanceled = false;
	DragDropManager& manager = DragDropManager::getInstance();
	status = manager.doDragDrop(pDataObject, this,
		dwEffect, &dwResultEffect, &bCanceled);
	CHECK_QSTATUS();
	if (pImpl_->pHandler_) {
		DragSourceDropEvent event(this, !bCanceled, dwResultEffect);
		status = pImpl_->pHandler_->dragDropEnd(event);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

DragSourceHandler* qs::DragSource::getDragSourceHandler() const
{
	return pImpl_->pHandler_;
}

void qs::DragSource::setDragSourceHandler(DragSourceHandler* pHandler)
{
	pImpl_->pHandler_ = pHandler;
}

IDropSource* qs::DragSource::getDropSource() const
{
	return pImpl_->pDropSource_;
}


/****************************************************************************
 *
 * DragSourceHandler
 *
 */

qs::DragSourceHandler::~DragSourceHandler()
{
}


/****************************************************************************
 *
 * DragSourceEvent
 *
 */

qs::DragSourceEvent::DragSourceEvent(DragSource* pDragSource) :
	pDragSource_(pDragSource)
{
}

qs::DragSourceEvent::~DragSourceEvent()
{
}

DragSource* qs::DragSourceEvent::getDragSource() const
{
	return pDragSource_;
}


/****************************************************************************
 *
 * DragSourceDropEvent
 *
 */

qs::DragSourceDropEvent::DragSourceDropEvent(
	DragSource* pDragSource, bool bDrop, DWORD dwEffect) :
	DragSourceEvent(pDragSource),
	bDrop_(bDrop),
	dwEffect_(dwEffect)
{
}

qs::DragSourceDropEvent::~DragSourceDropEvent()
{
}

bool qs::DragSourceDropEvent::isDrop() const
{
	return bDrop_;
}

DWORD qs::DragSourceDropEvent::getEffect() const
{
	return dwEffect_;
}


/****************************************************************************
 *
 * DropTargetImpl
 *
 */

struct qs::DropTargetImpl
{
	class IDropTargetImpl : public IDropTarget
	{
	public:
		IDropTargetImpl(DropTargetImpl* pDropTarget, QSTATUS* pstatus);
		~IDropTargetImpl();
	
	public:
		STDMETHOD_(ULONG, AddRef)();
		STDMETHOD_(ULONG, Release)();
		STDMETHOD(QueryInterface)(REFIID riid, void** ppv);
	
	public:
		STDMETHOD(DragEnter)(IDataObject* pDataObject,
			DWORD dwKeyState, POINTL pt, DWORD* pdwEffect);
		STDMETHOD(DragOver)(DWORD dwKeyState, POINTL pt, DWORD* pdwEffect);
		STDMETHOD(DragLeave)();
		STDMETHOD(Drop)(IDataObject* pDataObject,
			DWORD dwKeyState, POINTL pt, DWORD* pdwEffect);
	
	private:
		IDropTargetImpl(const IDropTargetImpl&);
		IDropTargetImpl& operator=(const IDropTargetImpl&);
	
	private:
		ULONG nRef_;
		DropTargetImpl* pDropTarget_;
		IDataObject* pDataObject_;
	};
	
	DropTarget* pThis_;
	DropTargetHandler* pHandler_;
	IDropTarget* pDropTarget_;
	HWND hwnd_;
};


/****************************************************************************
 *
 * DropTargetImpl::IDropTargetImpl
 *
 */

qs::DropTargetImpl::IDropTargetImpl::IDropTargetImpl(
	DropTargetImpl* pDropTarget, QSTATUS* pstatus) :
	nRef_(0),
	pDropTarget_(pDropTarget),
	pDataObject_(0)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qs::DropTargetImpl::IDropTargetImpl::~IDropTargetImpl()
{
	if (pDataObject_)
		pDataObject_->Release();
}

STDMETHODIMP_(ULONG) qs::DropTargetImpl::IDropTargetImpl::AddRef()
{
	return ::InterlockedIncrement(reinterpret_cast<LONG*>(&nRef_));
}

STDMETHODIMP_(ULONG) qs::DropTargetImpl::IDropTargetImpl::Release()
{
	ULONG nRef = ::InterlockedDecrement(reinterpret_cast<LONG*>(&nRef_));
	if (nRef == 0)
		delete this;
	return nRef;
}

STDMETHODIMP qs::DropTargetImpl::IDropTargetImpl::QueryInterface(
	REFIID riid, void** ppv)
{
	*ppv = 0;
	
	if (riid == IID_IUnknown || riid == IID_IDropTarget) {
		AddRef();
		*ppv = static_cast<IDropTarget*>(this);
	}
	
	return *ppv ? S_OK : E_NOINTERFACE;
}

STDMETHODIMP qs::DropTargetImpl::IDropTargetImpl::DragEnter(
	IDataObject* pDataObject, DWORD dwKeyState, POINTL pt, DWORD* pdwEffect)
{
	DECLARE_QSTATUS();
	
	pDataObject_ = pDataObject;
	pDataObject_->AddRef();
	
	if (pDropTarget_->pHandler_) {
		DropTargetDragEvent event(pDropTarget_->pThis_,
			pDataObject, dwKeyState, reinterpret_cast<POINT&>(pt));
		status = pDropTarget_->pHandler_->dragEnter(event);
		if (status == QSTATUS_OUTOFMEMORY)
			return E_OUTOFMEMORY;
		else if (status != QSTATUS_SUCCESS)
			return E_UNEXPECTED;
		*pdwEffect = event.getEffect();
	}
	
	return S_OK;
}

STDMETHODIMP qs::DropTargetImpl::IDropTargetImpl::DragOver(
	DWORD dwKeyState, POINTL pt, DWORD* pdwEffect)
{
	DECLARE_QSTATUS();
	
	if (pDropTarget_->pHandler_) {
		DropTargetDragEvent event(pDropTarget_->pThis_,
			pDataObject_, dwKeyState, reinterpret_cast<POINT&>(pt));
		status = pDropTarget_->pHandler_->dragOver(event);
		if (status == QSTATUS_OUTOFMEMORY)
			return E_OUTOFMEMORY;
		else if (status != QSTATUS_SUCCESS)
			return E_UNEXPECTED;
		*pdwEffect = event.getEffect();
	}
	
	return S_OK;
}

STDMETHODIMP qs::DropTargetImpl::IDropTargetImpl::DragLeave()
{
	DECLARE_QSTATUS();
	
	if (pDropTarget_->pHandler_) {
		DropTargetEvent event(pDropTarget_->pThis_);
		status = pDropTarget_->pHandler_->dragExit(event);
		if (status == QSTATUS_OUTOFMEMORY)
			return E_OUTOFMEMORY;
		else if (status != QSTATUS_SUCCESS)
			return E_UNEXPECTED;
	}
	
	if (pDataObject_) {
		pDataObject_->Release();
		pDataObject_ = 0;
	}
	
	return S_OK;
}

STDMETHODIMP qs::DropTargetImpl::IDropTargetImpl::Drop(
	IDataObject* pDataObject, DWORD dwKeyState, POINTL pt, DWORD* pdwEffect)
{
	DECLARE_QSTATUS();
	
	if (pDropTarget_->pHandler_) {
		DropTargetDropEvent event(pDropTarget_->pThis_,
			pDataObject, dwKeyState, reinterpret_cast<POINT&>(pt));
		status = pDropTarget_->pHandler_->drop(event);
		if (status == QSTATUS_OUTOFMEMORY)
			return E_OUTOFMEMORY;
		else if (status != QSTATUS_SUCCESS)
			return E_UNEXPECTED;
		*pdwEffect = event.getEffect();
	}
	
	if (pDataObject_) {
		pDataObject_->Release();
		pDataObject_ = 0;
	}
	
	return S_OK;
}


/****************************************************************************
 *
 * DropTarget
 *
 */

qs::DropTarget::DropTarget(HWND hwnd, QSTATUS* pstatus) :
	pImpl_(0)
{
	assert(pstatus);
	
	DECLARE_QSTATUS();
	
	*pstatus = QSTATUS_SUCCESS;
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	pImpl_->pThis_ = this;
	pImpl_->pHandler_ = 0;
	pImpl_->pDropTarget_ = 0;
	pImpl_->hwnd_ = 0;
	
	DropTargetImpl::IDropTargetImpl* p = 0;
	status = newQsObject(pImpl_, &p);
	CHECK_QSTATUS_SET(pstatus);
	p->AddRef();
	pImpl_->pDropTarget_ = p;
	
	DragDropManager& manager = DragDropManager::getInstance();
	status = manager.registerDragDrop(hwnd, this);
	CHECK_QSTATUS_SET(pstatus);
	pImpl_->hwnd_ = hwnd;
}

qs::DropTarget::~DropTarget()
{
	if (pImpl_) {
		if (pImpl_->hwnd_) {
			DragDropManager& manager = DragDropManager::getInstance();
			manager.revokeDragDrop(pImpl_->hwnd_);
		}
		
		if (pImpl_->pDropTarget_)
			pImpl_->pDropTarget_->Release();
		delete pImpl_;
	}
}

DropTargetHandler* qs::DropTarget::getDropTargetHandler() const
{
	return pImpl_->pHandler_;
}

void qs::DropTarget::setDropTargetHandler(DropTargetHandler* pHandler)
{
	pImpl_->pHandler_ = pHandler;
}

IDropTarget* qs::DropTarget::getDropTarget() const
{
	return pImpl_->pDropTarget_;
}


/****************************************************************************
 *
 * DropTargetHandler
 *
 */

qs::DropTargetHandler::~DropTargetHandler()
{
}


/****************************************************************************
 *
 * DropTargetEvent
 *
 */

qs::DropTargetEvent::DropTargetEvent(DropTarget* pDropTarget) :
	pDropTarget_(pDropTarget)
{
}

qs::DropTargetEvent::~DropTargetEvent()
{
}

DropTarget* qs::DropTargetEvent::getDropTarget() const
{
	return pDropTarget_;
}


/****************************************************************************
 *
 * DropTargetDragEvent
 *
 */

qs::DropTargetDragEvent::DropTargetDragEvent(DropTarget* pDropTarget,
	IDataObject* pDataObject, DWORD dwKeyState, const POINT& pt) :
	DropTargetEvent(pDropTarget),
	pDataObject_(pDataObject),
	dwKeyState_(dwKeyState),
	pt_(pt),
	dwEffect_(DROPEFFECT_NONE)
{
	if (pDataObject_)
		pDataObject_->AddRef();
}

qs::DropTargetDragEvent::~DropTargetDragEvent()
{
	if (pDataObject_)
		pDataObject_->Release();
}

IDataObject* qs::DropTargetDragEvent::getDataObject() const
{
	return pDataObject_;
}

DWORD qs::DropTargetDragEvent::getKeyState() const
{
	return dwKeyState_;
}

const POINT& qs::DropTargetDragEvent::getPoint() const
{
	return pt_;
}

DWORD qs::DropTargetDragEvent::getEffect() const
{
	return dwEffect_;
}

void qs::DropTargetDragEvent::setEffect(DWORD dwEffect) const
{
	dwEffect_ = dwEffect;
}


/****************************************************************************
 *
 * DragGestureRecognizerImpl
 *
 */

struct qs::DragGestureRecognizerImpl
{
	DragGestureRecognizerWindow* pWindow_;
	DragGestureHandler* pHandler_;
};


/****************************************************************************
 *
 * DragGestureRecognizer
 *
 */

qs::DragGestureRecognizer::DragGestureRecognizer(HWND hwnd, QSTATUS* pstatus) :
	pImpl_(0)
{
	assert(pstatus);
	
	DECLARE_QSTATUS();
	
	*pstatus = QSTATUS_SUCCESS;
	
	std::auto_ptr<DragGestureRecognizerWindow> pWindow;
	status = newQsObject(this, &pWindow);
	CHECK_QSTATUS_SET(pstatus);
	status = pWindow->subclassWindow(hwnd);
	CHECK_QSTATUS_SET(pstatus);
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	pImpl_->pWindow_ = pWindow.release();
	pImpl_->pHandler_ = 0;
}

qs::DragGestureRecognizer::~DragGestureRecognizer()
{
	if (pImpl_) {
		if (pImpl_->pWindow_) {
			pImpl_->pWindow_->unsubclassWindow();
			delete pImpl_->pWindow_;
		}
		delete pImpl_;
	}
}

DragGestureHandler* qs::DragGestureRecognizer::getDragGestureHandler() const
{
	return pImpl_->pHandler_;
}

void qs::DragGestureRecognizer::setDragGestureHandler(DragGestureHandler* pHandler)
{
	pImpl_->pHandler_ = pHandler;
}


/****************************************************************************
 *
 * DragGestureHandler
 *
 */

qs::DragGestureHandler::~DragGestureHandler()
{
}


/****************************************************************************
 *
 * DragGestureEvent
 *
 */

qs::DragGestureEvent::DragGestureEvent(const POINT& pt) :
	pt_(pt)
{
}

qs::DragGestureEvent::~DragGestureEvent()
{
}


/****************************************************************************
 *
 * DragDropManager
 *
 */

DragDropManager qs::DragDropManager::instance__;

qs::DragDropManager::DragDropManager()
{
}

qs::DragDropManager::~DragDropManager()
{
}

QSTATUS qs::DragDropManager::doDragDrop(IDataObject* pDataObject,
	DragSource* pDragSource, DWORD dwEffect, DWORD* pdwEffect, bool* pbCanceled)
{
	assert(pDataObject);
	assert(pDragSource);
	assert(pdwEffect);
	assert(pbCanceled);
	
	HRESULT hr = ::DoDragDrop(pDataObject,
		pDragSource->getDropSource(), dwEffect, pdwEffect);
	*pbCanceled = hr == DRAGDROP_S_CANCEL;
	return hr == S_OK ? QSTATUS_SUCCESS :
		hr == E_OUTOFMEMORY ? QSTATUS_OUTOFMEMORY : QSTATUS_FAIL;
}

QSTATUS qs::DragDropManager::registerDragDrop(HWND hwnd, DropTarget* pDropTarget)
{
	assert(hwnd);
	assert(pDropTarget);

	HRESULT hr = ::RegisterDragDrop(hwnd, pDropTarget->getDropTarget());
	return hr == S_OK ? QSTATUS_SUCCESS :
		hr == E_OUTOFMEMORY ? QSTATUS_OUTOFMEMORY : QSTATUS_FAIL;
}

QSTATUS qs::DragDropManager::revokeDragDrop(HWND hwnd)
{
	assert(hwnd);
	
	HRESULT hr = ::RevokeDragDrop(hwnd);
	return hr == S_OK ? QSTATUS_SUCCESS :
		hr == E_OUTOFMEMORY ? QSTATUS_OUTOFMEMORY : QSTATUS_FAIL;
}

DragDropManager& qs::DragDropManager::getInstance()
{
	return instance__;
}


/****************************************************************************
 *
 * DragGestureRecognizerWindow
 *
 */

qs::DragGestureRecognizerWindow::DragGestureRecognizerWindow(
	DragGestureRecognizer* pRecognizer, QSTATUS* pstatus) :
	WindowBase(false, pstatus),
	DefaultWindowHandler(pstatus),
	pRecognizer_(pRecognizer),
	nState_(0)
{
	pt_.x = 0;
	pt_.y = 0;
	
#ifdef _WIN32_WCE
	delta_.cx = 5;
	delta_.cy = 5;
#else
	delta_.cx = ::GetSystemMetrics(SM_CXDRAG);
	delta_.cy = ::GetSystemMetrics(SM_CYDRAG);
#endif
	
	setWindowHandler(this, false);
}

qs::DragGestureRecognizerWindow::~DragGestureRecognizerWindow()
{
}

LRESULT qs::DragGestureRecognizerWindow::windowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	BEGIN_MESSAGE_HANDLER()
		HANDLE_DESTROY()
		HANDLE_LBUTTONDOWN()
		HANDLE_LBUTTONUP()
		HANDLE_MOUSEMOVE()
		HANDLE_RBUTTONDOWN()
		HANDLE_RBUTTONUP()
	END_MESSAGE_HANDLER()
	return DefaultWindowHandler::windowProc(uMsg, wParam, lParam);
}

LRESULT qs::DragGestureRecognizerWindow::onDestroy()
{
	unsubclassWindow();
	return DefaultWindowHandler::onDestroy();
}

LRESULT qs::DragGestureRecognizerWindow::onLButtonDown(
	UINT nFlags, const POINT& pt)
{
	nState_ |= STATE_LBUTTONDOWN;
	pt_ = pt;
	setCapture();
	return DefaultWindowHandler::onLButtonDown(nFlags, pt);
}

LRESULT qs::DragGestureRecognizerWindow::onLButtonUp(
	UINT nFlags, const POINT& pt)
{
	nState_ &= ~STATE_LBUTTONDOWN;
	releaseCapture();
	return DefaultWindowHandler::onLButtonUp(nFlags, pt);
}

LRESULT qs::DragGestureRecognizerWindow::onMouseMove(
	UINT nFlags, const POINT& pt)
{
	if (nState_ == STATE_LBUTTONDOWN ||
		nState_ == STATE_RBUTTONDOWN) {
		if (abs(pt.x - pt_.x) > delta_.cx ||
			abs(pt.y - pt_.y) > delta_.cy) {
			DragGestureHandler* pHandler =
				pRecognizer_->getDragGestureHandler();
			if (pHandler) {
				DragGestureEvent event(pt_);
				pHandler->dragGestureRecognized(event);
			}
			nState_ = STATE_NONE;
			releaseCapture();
		}
	}
	
	return DefaultWindowHandler::onMouseMove(nFlags, pt);
}

LRESULT qs::DragGestureRecognizerWindow::onRButtonDown(
	UINT nFlags, const POINT& pt)
{
	nState_ |= STATE_RBUTTONDOWN;
	pt_ = pt;
	setCapture();
	return DefaultWindowHandler::onRButtonDown(nFlags, pt);
}

LRESULT qs::DragGestureRecognizerWindow::onRButtonUp(
	UINT nFlags, const POINT& pt)
{
	nState_ &= ~STATE_RBUTTONDOWN;
	releaseCapture();
	return DefaultWindowHandler::onRButtonUp(nFlags, pt);
}
