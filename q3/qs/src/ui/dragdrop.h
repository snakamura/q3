/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __DRAGDROP_H__
#define __DRAGDROP_H__

#include <qs.h>
#include <qswindow.h>


namespace qs {

class DragDropManager;
class DragGestureRecognizerWindow;


/****************************************************************************
 *
 * DragDropManager
 *
 */

class DragDropManager
{
private:
	DragDropManager();

public:
	~DragDropManager();

public:
	bool doDragDrop(IDataObject* pDataObject,
					DragSource* pDragSource,
					DWORD dwEffect,
					DWORD* pdwEffect,
					bool* pbCanceled);
	bool registerDragDrop(HWND hwnd,
						  DropTarget* pDropTarget);
	bool revokeDragDrop(HWND hwnd);

public:
	static DragDropManager& getInstance();

private:
	DragDropManager(const DragDropManager&);
	DragDropManager& operator=(const DragDropManager&);

private:
	static DragDropManager instance__;
};


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
