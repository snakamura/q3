/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
 * All rights reserved.
 *
 */

#pragma warning(disable:4786)

#include <qsassert.h>
#include <qsdragdrop.h>

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
		explicit IDropSourceImpl(DragSourceImpl* pDragSource);
		~IDropSourceImpl();
	
	public:
		STDMETHOD_(ULONG, AddRef)();
		STDMETHOD_(ULONG, Release)();
		STDMETHOD(QueryInterface)(REFIID riid,
								  void** ppv);
	
	public:
		STDMETHOD(QueryContinueDrag)(BOOL bEscapePressed,
									 DWORD dwKeyState);
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

qs::DragSourceImpl::IDropSourceImpl::IDropSourceImpl(DragSourceImpl* pDragSource) :
	nRef_(0),
	pDragSource_(pDragSource)
{
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

STDMETHODIMP qs::DragSourceImpl::IDropSourceImpl::QueryInterface(REFIID riid,
																 void** ppv)
{
	*ppv = 0;
	
	if (riid == IID_IUnknown || riid == IID_IDropSource) {
		AddRef();
		*ppv = static_cast<IDropSource*>(this);
	}
	
	return *ppv ? S_OK : E_NOINTERFACE;
}

STDMETHODIMP qs::DragSourceImpl::IDropSourceImpl::QueryContinueDrag(BOOL bEscapePressed,
																	DWORD dwKeyState)
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

qs::DragSource::DragSource() :
	pImpl_(0)
{
	pImpl_ = new DragSourceImpl();
	pImpl_->pHandler_ = 0;
	pImpl_->dwDropButton_ = 0;
	pImpl_->dwCancelButton_ = 0;
	pImpl_->pDropSource_ = 0;
	
	DragSourceImpl::IDropSourceImpl* pDropSource =
		new DragSourceImpl::IDropSourceImpl(pImpl_);
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

bool qs::DragSource::startDrag(const DragGestureEvent& event,
							   IDataObject* pDataObject,
							   DWORD dwEffect)
{
	if (::GetKeyState(VK_LBUTTON) < 0) {
		pImpl_->dwDropButton_ = MK_LBUTTON;
		pImpl_->dwCancelButton_ = MK_RBUTTON;
	}
	else if (::GetKeyState(VK_RBUTTON) < 0) {
		pImpl_->dwDropButton_ = MK_RBUTTON;
		pImpl_->dwCancelButton_ = MK_LBUTTON;
	}
	
	DragDropManager* pManager = DragDropManager::getInstance();
	DWORD dwResultEffect = DROPEFFECT_NONE;
	bool bCanceled = false;
	if (!pManager->doDragDrop(event.getWindow(), event.getPoint(),
		pDataObject, this, dwEffect, &dwResultEffect, &bCanceled))
		return false;
	if (pImpl_->pHandler_)
		pImpl_->pHandler_->dragDropEnd(DragSourceDropEvent(
			this, !bCanceled, dwResultEffect));
	
	return true;
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

qs::DragSourceDropEvent::DragSourceDropEvent(DragSource* pDragSource,
											 bool bDrop,
											 DWORD dwEffect) :
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
		IDropTargetImpl(DropTargetImpl* pDropTarget);
		~IDropTargetImpl();
	
	public:
		STDMETHOD_(ULONG, AddRef)();
		STDMETHOD_(ULONG, Release)();
		STDMETHOD(QueryInterface)(REFIID riid,
								  void** ppv);
	
	public:
		STDMETHOD(DragEnter)(IDataObject* pDataObject,
							 DWORD dwKeyState,
							 POINTL pt,
							 DWORD* pdwEffect);
		STDMETHOD(DragOver)(DWORD dwKeyState,
							POINTL pt,
							DWORD* pdwEffect);
		STDMETHOD(DragLeave)();
		STDMETHOD(Drop)(IDataObject* pDataObject,
						DWORD dwKeyState,
						POINTL pt,
						DWORD* pdwEffect);
	
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

qs::DropTargetImpl::IDropTargetImpl::IDropTargetImpl(DropTargetImpl* pDropTarget) :
	nRef_(0),
	pDropTarget_(pDropTarget),
	pDataObject_(0)
{
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

STDMETHODIMP qs::DropTargetImpl::IDropTargetImpl::QueryInterface(REFIID riid,
																 void** ppv)
{
	*ppv = 0;
	
	if (riid == IID_IUnknown || riid == IID_IDropTarget) {
		AddRef();
		*ppv = static_cast<IDropTarget*>(this);
	}
	
	return *ppv ? S_OK : E_NOINTERFACE;
}

STDMETHODIMP qs::DropTargetImpl::IDropTargetImpl::DragEnter(IDataObject* pDataObject,
															DWORD dwKeyState,
															POINTL pt,
															DWORD* pdwEffect)
{
	pDataObject_ = pDataObject;
	pDataObject_->AddRef();
	
	if (pDropTarget_->pHandler_) {
		DropTargetDragEvent event(pDropTarget_->pThis_, pDataObject,
			dwKeyState, reinterpret_cast<POINT&>(pt), *pdwEffect);
		pDropTarget_->pHandler_->dragEnter(event);
		*pdwEffect = event.getEffect();
	}
	
	return S_OK;
}

STDMETHODIMP qs::DropTargetImpl::IDropTargetImpl::DragOver(DWORD dwKeyState,
														   POINTL pt,
														   DWORD* pdwEffect)
{
	if (pDropTarget_->pHandler_) {
		DropTargetDragEvent event(pDropTarget_->pThis_, pDataObject_,
			dwKeyState, reinterpret_cast<POINT&>(pt), *pdwEffect);
		pDropTarget_->pHandler_->dragOver(event);
		*pdwEffect = event.getEffect();
	}
	
	return S_OK;
}

STDMETHODIMP qs::DropTargetImpl::IDropTargetImpl::DragLeave()
{
	if (pDropTarget_->pHandler_)
		pDropTarget_->pHandler_->dragExit(DropTargetEvent(pDropTarget_->pThis_));
	
	if (pDataObject_) {
		pDataObject_->Release();
		pDataObject_ = 0;
	}
	
	return S_OK;
}

STDMETHODIMP qs::DropTargetImpl::IDropTargetImpl::Drop(IDataObject* pDataObject,
													   DWORD dwKeyState,
													   POINTL pt,
													   DWORD* pdwEffect)
{
	if (pDropTarget_->pHandler_) {
		DropTargetDropEvent event(pDropTarget_->pThis_, pDataObject,
			dwKeyState, reinterpret_cast<POINT&>(pt), *pdwEffect);
		pDropTarget_->pHandler_->drop(event);
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

qs::DropTarget::DropTarget(HWND hwnd) :
	pImpl_(0)
{
	pImpl_ = new DropTargetImpl();
	pImpl_->pThis_ = this;
	pImpl_->pHandler_ = 0;
	pImpl_->pDropTarget_ = 0;
	pImpl_->hwnd_ = 0;
	
	DropTargetImpl::IDropTargetImpl* p =
		new DropTargetImpl::IDropTargetImpl(pImpl_);
	p->AddRef();
	pImpl_->pDropTarget_ = p;
	
	DragDropManager* pManager = DragDropManager::getInstance();
	pManager->registerDragDrop(hwnd, this);
	pImpl_->hwnd_ = hwnd;
}

qs::DropTarget::~DropTarget()
{
	if (pImpl_) {
		if (pImpl_->hwnd_) {
			DragDropManager* pManager = DragDropManager::getInstance();
			pManager->revokeDragDrop(pImpl_->hwnd_);
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
											 IDataObject* pDataObject,
											 DWORD dwKeyState,
											 const POINT& pt,
											 DWORD dwEffect) :
	DropTargetEvent(pDropTarget),
	pDataObject_(pDataObject),
	dwKeyState_(dwKeyState),
	pt_(pt),
	dwEffect_(dwEffect)
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
	std::auto_ptr<DragGestureRecognizerWindow> pWindow_;
	DragGestureHandler* pHandler_;
};


/****************************************************************************
 *
 * DragGestureRecognizer
 *
 */

qs::DragGestureRecognizer::DragGestureRecognizer(HWND hwnd) :
	pImpl_(0)
{
	std::auto_ptr<DragGestureRecognizerWindow> pWindow(
		new DragGestureRecognizerWindow(this));
	pWindow->subclassWindow(hwnd);
	
	pImpl_ = new DragGestureRecognizerImpl();
	pImpl_->pWindow_ = pWindow;
	pImpl_->pHandler_ = 0;
}

qs::DragGestureRecognizer::~DragGestureRecognizer()
{
	if (pImpl_) {
		if (pImpl_->pWindow_.get())
			pImpl_->pWindow_->unsubclassWindow();
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

qs::DragGestureEvent::DragGestureEvent(HWND hwnd,
									   const POINT& pt) :
	hwnd_(hwnd),
	pt_(pt)
{
}

qs::DragGestureEvent::~DragGestureEvent()
{
}

HWND qs::DragGestureEvent::getWindow() const
{
	return hwnd_;
}

const POINT& qs::DragGestureEvent::getPoint() const
{
	return pt_;
}


/****************************************************************************
 *
 * DragDropManager
 *
 */

DragDropManager* qs::DragDropManager::pManager__;

qs::DragDropManager::~DragDropManager()
{
}

DragDropManager* qs::DragDropManager::getInstance()
{
	assert(pManager__);
	return pManager__;
}

void qs::DragDropManager::registerManager(DragDropManager* pManager)
{
	assert(!pManager__);
	pManager__ = pManager;
}

void qs::DragDropManager::unregisterManager(DragDropManager* pManager)
{
	assert(pManager__);
	pManager__ = 0;
}


#ifndef QS_CUSTOMDRAGDROP
/****************************************************************************
 *
 * SystemDragDropManager
 *
 */

SystemDragDropManager qs::SystemDragDropManager::instance__;

qs::SystemDragDropManager::SystemDragDropManager()
{
	registerManager(this);
}

qs::SystemDragDropManager::~SystemDragDropManager()
{
	unregisterManager(this);
}

bool qs::SystemDragDropManager::doDragDrop(HWND hwnd,
										   const POINT& pt,
										   IDataObject* pDataObject,
										   DragSource* pDragSource,
										   DWORD dwEffect,
										   DWORD* pdwEffect,
										   bool* pbCanceled)
{
	assert(hwnd);
	assert(pDataObject);
	assert(pDragSource);
	assert(pdwEffect);
	assert(pbCanceled);
	
	HRESULT hr = ::DoDragDrop(pDataObject,
		pDragSource->getDropSource(), dwEffect, pdwEffect);
	*pbCanceled = hr == DRAGDROP_S_CANCEL;
	return hr == DRAGDROP_S_DROP;
}

bool qs::SystemDragDropManager::registerDragDrop(HWND hwnd,
												 DropTarget* pDropTarget)
{
	assert(hwnd);
	assert(pDropTarget);
	
	return ::RegisterDragDrop(hwnd, pDropTarget->getDropTarget()) == S_OK;
}

bool qs::SystemDragDropManager::revokeDragDrop(HWND hwnd)
{
	assert(hwnd);
	
	return ::RevokeDragDrop(hwnd) == S_OK;
}
#endif


#ifdef QS_CUSTOMDRAGDROP
/****************************************************************************
 *
 * CustomDragDropManager
 *
 */

CustomDragDropManager qs::CustomDragDropManager::instance__;

qs::CustomDragDropManager::CustomDragDropManager()
{
	registerManager(this);
}

qs::CustomDragDropManager::~CustomDragDropManager()
{
	unregisterManager(this);
}

bool qs::CustomDragDropManager::doDragDrop(HWND hwnd,
										   const POINT& pt,
										   IDataObject* pDataObject,
										   DragSource* pDragSource,
										   DWORD dwEffect,
										   DWORD* pdwEffect,
										   bool* pbCanceled)
{
	assert(hwnd);
	assert(pDataObject);
	assert(pDragSource);
	assert(pdwEffect);
	assert(pbCanceled);
	
	DragDropWindow wnd(this, hwnd, pt, pDataObject, pDragSource->getDropSource(), dwEffect);
	
	MSG msg;
	while (::GetMessage(&msg, 0, 0, 0)) {
		::TranslateMessage(&msg);
		::DispatchMessage(&msg);
		
		if (wnd.isFinished())
			break;
	}
	
	*pdwEffect = wnd.getEffect();
	*pbCanceled = wnd.isCanceled();
	
	return true;
}

bool qs::CustomDragDropManager::registerDragDrop(HWND hwnd,
												 DropTarget* pDropTarget)
{
	assert(hwnd);
	assert(pDropTarget);
	
	return mapDropTarget_.insert(DropTargetMap::value_type(hwnd, pDropTarget)).second;
}

bool qs::CustomDragDropManager::revokeDragDrop(HWND hwnd)
{
	assert(hwnd);
	
	mapDropTarget_.erase(hwnd);
	
	return true;
}

DropTarget* qs::CustomDragDropManager::getDropTarget(HWND hwnd) const
{
	DropTargetMap::const_iterator it = mapDropTarget_.find(hwnd);
	return it != mapDropTarget_.end() ? (*it).second : 0;
}


/****************************************************************************
 *
 * CustomDragDropManager::DragDropWindow
 *
 */

qs::CustomDragDropManager::DragDropWindow::DragDropWindow(const CustomDragDropManager* pManager,
														  HWND hwnd,
														  const POINT& pt,
														  IDataObject* pDataObject,
														  IDropSource* pDropSource,
														  DWORD dwAllowedEffects) :
	WindowBase(false),
	pManager_(pManager),
	pDataObject_(pDataObject),
	pDropSource_(pDropSource),
	dwAllowedEffects_(dwAllowedEffects),
	bFinished_(false),
	dwEffect_(DROPEFFECT_NONE),
	bCanceled_(false),
	bTimer_(false),
	pCurrentDropTarget_(0),
	ptCurrent_(pt)
{
	setWindowHandler(this, false);
	
	subclassWindow(hwnd);
	setCapture();
	
	bTimer_ = setTimer(TIMER_ID, TIMER_INTERVAL) != 0;
	
	setCurrentMousePosition(pt, getKeyState());
}

qs::CustomDragDropManager::DragDropWindow::~DragDropWindow()
{
	if (bTimer_)
		killTimer(TIMER_ID);
	releaseCapture();
	unsubclassWindow();
}

bool qs::CustomDragDropManager::DragDropWindow::isFinished() const
{
	return bFinished_;
}

DWORD qs::CustomDragDropManager::DragDropWindow::getEffect() const
{
	return dwEffect_;
}

bool qs::CustomDragDropManager::DragDropWindow::isCanceled() const
{
	return bCanceled_;
}

LRESULT qs::CustomDragDropManager::DragDropWindow::windowProc(UINT uMsg,
															  WPARAM wParam,
															  LPARAM lParam)
{
	BEGIN_MESSAGE_HANDLER()
		HANDLE_KEYDOWN()
		HANDLE_KEYUP()
		HANDLE_LBUTTONDOWN()
		HANDLE_LBUTTONUP()
		HANDLE_MOUSEMOVE()
		HANDLE_RBUTTONDOWN()
		HANDLE_RBUTTONUP()
		HANDLE_TIMER()
	END_MESSAGE_HANDLER()
	return DefaultWindowHandler::windowProc(uMsg, wParam, lParam);
}

LRESULT qs::CustomDragDropManager::DragDropWindow::onKeyDown(UINT nKey,
															 UINT nRepeat,
															 UINT nFlags)
{
	if (nKey == VK_ESCAPE)
		queryContinue(true, getKeyState());
	else
		updateEffect(getKeyState());
	return 0;
}

LRESULT qs::CustomDragDropManager::DragDropWindow::onKeyUp(UINT nKey,
														   UINT nRepeat,
														   UINT nFlags)
{
	updateEffect(getKeyState());
	return 0;
}

LRESULT qs::CustomDragDropManager::DragDropWindow::onLButtonDown(UINT nFlags,
																 const POINT& pt)
{
	handleMouseEvent(nFlags, pt);
	return 0;
}

LRESULT qs::CustomDragDropManager::DragDropWindow::onLButtonUp(UINT nFlags,
															   const POINT& pt)
{
	handleMouseEvent(nFlags, pt);
	return 0;
}

LRESULT qs::CustomDragDropManager::DragDropWindow::onMouseMove(UINT nFlags,
															   const POINT& pt)
{
	setCurrentMousePosition(pt, nFlags);
	return 0;
}

LRESULT qs::CustomDragDropManager::DragDropWindow::onRButtonDown(UINT nFlags,
																 const POINT& pt)
{
	handleMouseEvent(nFlags, pt);
	return 0;
}

LRESULT qs::CustomDragDropManager::DragDropWindow::onRButtonUp(UINT nFlags,
															   const POINT& pt)
{
	handleMouseEvent(nFlags, pt);
	return 0;
}

LRESULT qs::CustomDragDropManager::DragDropWindow::onTimer(UINT_PTR nId)
{
	if (nId == TIMER_ID) {
		killTimer(TIMER_ID);
		bTimer_ = false;
		updateEffect(getKeyState());
		bTimer_ = setTimer(TIMER_ID, TIMER_INTERVAL) != 0;
	}
	return DefaultWindowHandler::onTimer(nId);
}

void qs::CustomDragDropManager::DragDropWindow::setCurrentMousePosition(const POINT& pt,
																		DWORD dwKeyState)
{
	POINT ptScreen = pt;
	clientToScreen(&ptScreen);
	
	HWND hwnd = ::WindowFromPoint(ptScreen);
	DropTarget* pDropTarget = pManager_->getDropTarget(hwnd);
	if (pDropTarget == pCurrentDropTarget_)
		updateEffect(dwKeyState);
	else
		setCurrentTarget(pDropTarget, ptScreen, dwKeyState);
	
	ptCurrent_ = ptScreen;
}

void qs::CustomDragDropManager::DragDropWindow::setCurrentTarget(DropTarget* pDropTarget,
																 const POINT& ptScreen,
																 DWORD dwKeyState)
{
	if (pCurrentDropTarget_) {
		DropTargetHandler* pHandler = pCurrentDropTarget_->getDropTargetHandler();
		if (pHandler)
			pHandler->dragExit(DropTargetEvent(pCurrentDropTarget_));
	}
	if (pDropTarget) {
		DropTargetHandler* pHandler = pDropTarget->getDropTargetHandler();
		if (pHandler) {
			DropTargetDragEvent event(pDropTarget,
				pDataObject_, dwKeyState, ptScreen, dwEffect_);
			pHandler->dragEnter(event);
			dwEffect_ = event.getEffect() & dwAllowedEffects_;
		}
	}
	pCurrentDropTarget_ = pDropTarget;
}

void qs::CustomDragDropManager::DragDropWindow::updateEffect(DWORD dwKeyState)
{
	if (pCurrentDropTarget_) {
		DropTargetHandler* pHandler = pCurrentDropTarget_->getDropTargetHandler();
		if (pHandler) {
			DropTargetDragEvent event(pCurrentDropTarget_,
				pDataObject_, dwKeyState, ptCurrent_, dwEffect_);
			pHandler->dragOver(event);
			dwEffect_ = event.getEffect() & dwAllowedEffects_;
		}
	}
}

void qs::CustomDragDropManager::DragDropWindow::queryContinue(bool bEscape,
															  DWORD dwKeyState)
{
	HRESULT hr = pDropSource_->QueryContinueDrag(bEscape, getKeyState());
	if (hr == S_OK) {
		return;
	}
	else if (hr == DRAGDROP_S_DROP) {
		if (pCurrentDropTarget_) {
			DropTargetHandler* pHandler = pCurrentDropTarget_->getDropTargetHandler();
			if (pHandler) {
				DropTargetDropEvent event(pCurrentDropTarget_,
					pDataObject_, dwKeyState, ptCurrent_, dwEffect_);
				pHandler->drop(event);
				dwEffect_ = event.getEffect() & dwAllowedEffects_;
			}
		}
		
		bFinished_ = true;
		pCurrentDropTarget_ = 0;
	}
	else if (hr == DRAGDROP_S_CANCEL) {
		if (pCurrentDropTarget_) {
			DropTargetHandler* pHandler = pCurrentDropTarget_->getDropTargetHandler();
			if (pHandler)
				pHandler->dragExit(DropTargetEvent(pCurrentDropTarget_));
		}
		
		dwEffect_ = DROPEFFECT_NONE;
		bCanceled_ = true;
		bFinished_ = true;
		pCurrentDropTarget_ = 0;
	}
}

void qs::CustomDragDropManager::DragDropWindow::handleMouseEvent(UINT nFlags,
																 const POINT& pt)
{
	setCurrentMousePosition(pt, nFlags);
	queryContinue(false, nFlags);
}

DWORD qs::CustomDragDropManager::DragDropWindow::getKeyState()
{
	DWORD dwKeyState = 0;
	
	struct {
		UINT nVirtualKey_;
		DWORD dwKey_;
	} keys[] = {
		{ VK_SHIFT,		MK_SHIFT	},
		{ VK_CONTROL,	MK_CONTROL	},
		{ VK_MENU,		MK_ALT		},
		{ VK_LBUTTON,	MK_LBUTTON	},
		{ VK_MBUTTON,	MK_MBUTTON	},
		{ VK_RBUTTON,	MK_RBUTTON	}
	};
	for (int n = 0; n < countof(keys); ++n) {
		if (::GetKeyState(keys[n].nVirtualKey_) < 0)
			dwKeyState |= keys[n].dwKey_;
	}
	
	return dwKeyState;
}
#endif


/****************************************************************************
 *
 * DragGestureRecognizerWindow
 *
 */

qs::DragGestureRecognizerWindow::DragGestureRecognizerWindow(DragGestureRecognizer* pRecognizer) :
	WindowBase(false),
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

LRESULT qs::DragGestureRecognizerWindow::windowProc(UINT uMsg,
													WPARAM wParam,
													LPARAM lParam)
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

LRESULT qs::DragGestureRecognizerWindow::onLButtonDown(UINT nFlags,
													   const POINT& pt)
{
	nState_ |= STATE_LBUTTONDOWN;
	pt_ = pt;
	setCapture();
	return DefaultWindowHandler::onLButtonDown(nFlags, pt);
}

LRESULT qs::DragGestureRecognizerWindow::onLButtonUp(UINT nFlags,
													 const POINT& pt)
{
	nState_ &= ~STATE_LBUTTONDOWN;
	releaseCapture();
	return DefaultWindowHandler::onLButtonUp(nFlags, pt);
}

LRESULT qs::DragGestureRecognizerWindow::onMouseMove(UINT nFlags,
													 const POINT& pt)
{
	if (nState_ == STATE_LBUTTONDOWN ||
		nState_ == STATE_RBUTTONDOWN) {
		if (abs(pt.x - pt_.x) > delta_.cx ||
			abs(pt.y - pt_.y) > delta_.cy) {
			DragGestureHandler* pHandler = pRecognizer_->getDragGestureHandler();
			if (pHandler)
				pHandler->dragGestureRecognized(DragGestureEvent(getHandle(), pt_));
			nState_ = STATE_NONE;
			releaseCapture();
		}
	}
	
	return DefaultWindowHandler::onMouseMove(nFlags, pt);
}

LRESULT qs::DragGestureRecognizerWindow::onRButtonDown(UINT nFlags,
													   const POINT& pt)
{
	nState_ |= STATE_RBUTTONDOWN;
	pt_ = pt;
	setCapture();
	return DefaultWindowHandler::onRButtonDown(nFlags, pt);
}

LRESULT qs::DragGestureRecognizerWindow::onRButtonUp(UINT nFlags,
													 const POINT& pt)
{
	nState_ &= ~STATE_RBUTTONDOWN;
	releaseCapture();
	return DefaultWindowHandler::onRButtonUp(nFlags, pt);
}
