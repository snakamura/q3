/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QSDRAGDROP_H__
#define __QSDRAGDROP_H__

#include <qs.h>


namespace qs {

class DragSource;
class DragSourceHandler;
class DragSourceEvent;
	class DragSourceDropEvent;
class DropTarget;
class DropTargetHandler;
class DropTargetEvent;
	class DropTargetDragEvent;
class DragGestureRecognizer;
class DragGestureHandler;
class DragGestureEvent;

typedef DropTargetDragEvent DropTargetDropEvent;


/****************************************************************************
 *
 * DragSource
 *
 */

class QSEXPORTCLASS DragSource
{
public:
	DragSource();
	~DragSource();

public:
	bool startDrag(IDataObject* pDataObject,
				   DWORD dwEffect);
	DragSourceHandler* getDragSourceHandler() const;
	void setDragSourceHandler(DragSourceHandler* pHandler);

public:
	IDropSource* getDropSource() const;

private:
	DragSource(const DragSource&);
	DragSource& operator=(const DragSource&);

private:
	struct DragSourceImpl* pImpl_;
};


/****************************************************************************
 *
 * DragSourceHandler
 *
 */

class QSEXPORTCLASS DragSourceHandler
{
public:
	virtual ~DragSourceHandler();

public:
	virtual void dragDropEnd(const DragSourceDropEvent& event) = 0;
};


/****************************************************************************
 *
 * DragSourceEvent
 *
 */

class QSEXPORTCLASS DragSourceEvent
{
public:
	explicit DragSourceEvent(DragSource* pDragSource);
	~DragSourceEvent();

public:
	DragSource* getDragSource() const;

private:
	DragSourceEvent(const DragSourceEvent&);
	DragSourceEvent& operator=(const DragSourceEvent&);

private:
	DragSource* pDragSource_;
};


/****************************************************************************
 *
 * DragSourceDropEvent
 *
 */

class QSEXPORTCLASS DragSourceDropEvent : public DragSourceEvent
{
public:
	DragSourceDropEvent(DragSource* pDragSource,
						bool bDrop,
						DWORD dwEffect);
	~DragSourceDropEvent();

public:
	bool isDrop() const;
	DWORD getEffect() const;

private:
	DragSourceDropEvent(const DragSourceDropEvent&);
	DragSourceDropEvent& operator=(const DragSourceDropEvent&);

private:
	bool bDrop_;
	DWORD dwEffect_;
};


/****************************************************************************
 *
 * DropTarget
 *
 */

class QSEXPORTCLASS DropTarget
{
public:
	explicit DropTarget(HWND hwnd);
	~DropTarget();

public:
	DropTargetHandler* getDropTargetHandler() const;
	void setDropTargetHandler(DropTargetHandler* pHandler);

public:
	IDropTarget* getDropTarget() const;

private:
	DropTarget(const DropTarget&);
	DropTarget& operator=(const DropTarget&);

private:
	struct DropTargetImpl* pImpl_;
};


/****************************************************************************
 *
 * DropTargetHandler
 *
 */

class QSEXPORTCLASS DropTargetHandler
{
public:
	virtual ~DropTargetHandler();

public:
	virtual void dragEnter(const DropTargetDragEvent& event) = 0;
	virtual void dragOver(const DropTargetDragEvent& event) = 0;
	virtual void dragExit(const DropTargetEvent& event) = 0;
	virtual void drop(const DropTargetDropEvent& event) = 0;
};


/****************************************************************************
 *
 * DropTargetEvent
 *
 */

class QSEXPORTCLASS DropTargetEvent
{
public:
	explicit DropTargetEvent(DropTarget* pDropTarget);
	~DropTargetEvent();

public:
	DropTarget* getDropTarget() const;

private:
	DropTargetEvent(const DropTargetEvent&);
	DropTargetEvent& operator=(const DropTargetEvent&);

private:
	DropTarget* pDropTarget_;
};


/****************************************************************************
 *
 * DropTargetDragEvent
 *
 */

class QSEXPORTCLASS DropTargetDragEvent : public DropTargetEvent
{
public:
	DropTargetDragEvent(DropTarget* pDropTarget,
						IDataObject* pDataObject,
						DWORD dwKeyState,
						const POINT& pt);
	~DropTargetDragEvent();

public:
	IDataObject* getDataObject() const;
	DWORD getKeyState() const;
	const POINT& getPoint() const;
	DWORD getEffect() const;
	void setEffect(DWORD dwEffect) const;

private:
	DropTargetDragEvent(const DropTargetDragEvent&);
	DropTargetDragEvent& operator=(const DropTargetDragEvent&);

private:
	IDataObject* pDataObject_;
	DWORD dwKeyState_;
	POINT pt_;
	mutable DWORD dwEffect_;
};


/****************************************************************************
 *
 * DragGestureRecognizer
 *
 */

class QSEXPORTCLASS DragGestureRecognizer
{
public:
	explicit DragGestureRecognizer(HWND hwnd);
	virtual ~DragGestureRecognizer();

public:
	DragGestureHandler* getDragGestureHandler() const;
	void setDragGestureHandler(DragGestureHandler* pHandler);

private:
	DragGestureRecognizer(const DragGestureRecognizer&);
	DragGestureRecognizer& operator=(const DragGestureRecognizer&);

private:
	struct DragGestureRecognizerImpl* pImpl_;
};


/****************************************************************************
 *
 * DragGestureHandler
 *
 */

class QSEXPORTCLASS DragGestureHandler
{
public:
	virtual ~DragGestureHandler();

public:
	virtual void dragGestureRecognized(const DragGestureEvent& event) = 0;
};


/****************************************************************************
 *
 * DragGestureEvent
 *
 */

class QSEXPORTCLASS DragGestureEvent
{
public:
	explicit DragGestureEvent(const POINT& pt);
	~DragGestureEvent();

private:
	DragGestureEvent(const DragGestureEvent&);
	DragGestureEvent& operator=(const DragGestureEvent&);

private:
	POINT pt_;
};

}

#endif // __QSDRAGDROP_H__
