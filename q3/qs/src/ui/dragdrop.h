/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __DRAGDROP_H__
#define __DRAGDROP_H__

#include <qs.h>
#include <qswindow.h>

#include "uiutil.h"

#ifdef _WIN32_WCE
#	define	QS_CUSTOMDRAGDROP
#endif

namespace qs {

class DragDropManager;
	class SystemDragDropManager;
	class CustomDragDropManager;
class DragGestureRecognizerWindow;


/****************************************************************************
 *
 * DragDropManager
 *
 */

class DragDropManager
{
public:
	virtual ~DragDropManager();

public:
	virtual bool doDragDrop(HWND hwnd,
							const POINT& pt,
							IDataObject* pDataObject,
							DragSource* pDragSource,
							DWORD dwEffect,
							DWORD* pdwEffect,
							bool* pbCanceled) = 0;
	virtual bool registerDragDrop(HWND hwnd,
								  DropTarget* pDropTarget) = 0;
	virtual bool revokeDragDrop(HWND hwnd) = 0;

public:
	static DragDropManager* getInstance();

protected:
	static void registerManager(DragDropManager* pManager);
	static void unregisterManager(DragDropManager* pManager);

private:
	static DragDropManager* pManager__;
};


#ifndef QS_CUSTOMDRAGDROP
/****************************************************************************
 *
 * SystemDragDropManager
 *
 */

class SystemDragDropManager : public DragDropManager
{
private:
	SystemDragDropManager();

public:
	virtual ~SystemDragDropManager();

public:
	virtual bool doDragDrop(HWND hwnd,
							const POINT& pt,
							IDataObject* pDataObject,
							DragSource* pDragSource,
							DWORD dwEffect,
							DWORD* pdwEffect,
							bool* pbCanceled);
	virtual bool registerDragDrop(HWND hwnd,
								  DropTarget* pDropTarget);
	virtual bool revokeDragDrop(HWND hwnd);

private:
	SystemDragDropManager(const SystemDragDropManager&);
	SystemDragDropManager& operator=(const SystemDragDropManager&);

private:
	static SystemDragDropManager instance__;
};
#endif


#ifdef QS_CUSTOMDRAGDROP
/****************************************************************************
 *
 * CustomDragDropManager
 *
 */

class CustomDragDropManager : public DragDropManager
{
private:
	CustomDragDropManager();

public:
	virtual ~CustomDragDropManager();

public:
	virtual bool doDragDrop(HWND hwnd,
							const POINT& pt,
							IDataObject* pDataObject,
							DragSource* pDragSource,
							DWORD dwEffect,
							DWORD* pdwEffect,
							bool* pbCanceled);
	virtual bool registerDragDrop(HWND hwnd,
								  DropTarget* pDropTarget);
	virtual bool revokeDragDrop(HWND hwnd);

private:
	DropTarget* getDropTarget(HWND hwnd) const;

private:
	CustomDragDropManager(const CustomDragDropManager&);
	CustomDragDropManager& operator=(const CustomDragDropManager&);

private:
	class DragDropWindow :
		public WindowBase,
		public DefaultWindowHandler
	{
	public:
		DragDropWindow(const CustomDragDropManager* pManager,
					   HWND hwnd,
					   const POINT& pt,
					   IDataObject* pDataObject,
					   IDropSource* pDropSource,
					   DWORD dwAllowedEffects);
		virtual ~DragDropWindow();
	
	public:
		bool isFinished() const;
		DWORD getEffect() const;
		bool isCanceled() const;
	
	public:
		virtual LRESULT windowProc(UINT uMsg,
								   WPARAM wParam,
								   LPARAM lParam);
	
	protected:
		LRESULT onKeyDown(UINT nKey,
						  UINT nRepeat,
						  UINT nFlags);
		LRESULT onKeyUp(UINT nKey,
						UINT nRepeat,
						UINT nFlags);
		LRESULT onLButtonDown(UINT nFlags,
							  const POINT& pt);
		LRESULT onLButtonUp(UINT nFlags,
							const POINT& pt);
		LRESULT onMouseMove(UINT nFlags,
							const POINT& pt);
		LRESULT onRButtonDown(UINT nFlags,
							  const POINT& pt);
		LRESULT onRButtonUp(UINT nFlags,
							const POINT& pt);
		LRESULT onTimer(UINT_PTR nId);
	
	private:
		void setCurrentMousePosition(const POINT& pt,
									 DWORD dwKeyState);
		void setCurrentTarget(DropTarget* pDropTarget,
							  const POINT& ptScreen,
							  DWORD dwKeyState);
		void updateEffect(DWORD dwKeyState);
		void queryContinue(bool bEscape,
						   DWORD dwKeyState);
		void handleMouseEvent(UINT nFlags,
							  const POINT& pt);
	
	private:
		static DWORD getKeyState();
	
	private:
		DragDropWindow(const DragDropWindow&);
		DragDropWindow& operator=(const DragDropWindow&);
	
	private:
		enum {
			TIMER_ID		= 1001,
			TIMER_INTERVAL	= 200
		};
	
	private:
		const CustomDragDropManager* pManager_;
		IDataObject* pDataObject_;
		IDropSource* pDropSource_;
		DWORD dwAllowedEffects_;
		bool bFinished_;
		DWORD dwEffect_;
		bool bCanceled_;
		bool bTimer_;
		DropTarget* pCurrentDropTarget_;
		POINT ptCurrent_;
	};
	friend class DragDropWindow;

private:
	typedef std::hash_map<HWND, DropTarget*, hash_hwnd> DropTargetMap;

private:
	DropTargetMap mapDropTarget_;

private:
	static CustomDragDropManager instance__;
};
#endif


/****************************************************************************
 *
 * DragGestureRecognizerWindow
 *
 */

class DragGestureRecognizerWindow :
	public WindowBase,
	public DefaultWindowHandler
{
public:
	explicit DragGestureRecognizerWindow(DragGestureRecognizer* pRecognizer);
	virtual ~DragGestureRecognizerWindow();

public:
	virtual LRESULT windowProc(UINT uMsg,
							   WPARAM wParam,
							   LPARAM lParam);

protected:
	LRESULT onDestroy();
	LRESULT onLButtonDown(UINT nFlags,
						  const POINT& pt);
	LRESULT onLButtonUp(UINT nFlags,
						const POINT& pt);
	LRESULT onMouseMove(UINT nFlags,
						const POINT& pt);
	LRESULT onRButtonDown(UINT nFlags,
						  const POINT& pt);
	LRESULT onRButtonUp(UINT nFlags,
						const POINT& pt);

private:
	DragGestureRecognizerWindow(const DragGestureRecognizerWindow&);
	DragGestureRecognizerWindow& operator=(const DragGestureRecognizerWindow&);

private:
	enum State {
		STATE_NONE			= 0x00,
		STATE_LBUTTONDOWN	= 0x01,
		STATE_RBUTTONDOWN	= 0x02
	};

private:
	DragGestureRecognizer* pRecognizer_;
	unsigned int nState_;
	POINT pt_;
	SIZE delta_;
};

}

#endif // __DRAGDROP_H__
