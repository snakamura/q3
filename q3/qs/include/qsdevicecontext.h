/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QSDEVICECONTEXT_H__
#define __QSDEVICECONTEXT_H__

#include <qs.h>

#include <windows.h>

namespace qs {

class DeviceContext;
	class ClientDeviceContext;
	class PaintDeviceContext;
	class CompatibleDeviceContext;
template<class Object> class ObjectSelector;
template<class Object> class GdiObject;
class FontHelper;


/****************************************************************************
 *
 * DeviceContext
 *
 */

class QSEXPORTCLASS DeviceContext
{
public:
	explicit DeviceContext(HDC hdc);
	DeviceContext(const DeviceContext& dc);
	virtual ~DeviceContext();

public:
	operator HDC() const;
	DeviceContext& operator=(HDC hdc);
	DeviceContext& operator=(const DeviceContext& dc);
	bool operator!() const;

public:
	HDC getHandle() const;
	
	COLORREF getTextColor();
	COLORREF setTextColor(COLORREF cr);
	COLORREF getBkColor();
	COLORREF setBkColor(COLORREF cr);
	int setBkMode(int nBkMode);
	
	int getClipBox(RECT* pRect);
	
	HFONT selectObject(HFONT hfont);
	HPEN selectObject(HPEN hpen);
	HBRUSH selectObject(HBRUSH hbrush);
	HBITMAP selectObject(HBITMAP hbm);
	
	bool polyline(const POINT* ppt, int nPoints);
	bool bitBlt(int x, int y, int nWidth, int nHeight, HDC hdc,
		int nSrcX, int nSrcY, DWORD dwRop);
	bool patBlt(int x, int y, int nWidth, int nHeight, DWORD dwRop);
	
	bool extTextOut(int x, int y, UINT nOptions, const RECT& rect,
		const WCHAR* pwszString, UINT nCount, int* pnDx);
	bool getTextMetrics(TEXTMETRIC* ptm) const;
	bool getTextExtent(const WCHAR* pwszString,
		int nCount, SIZE* pSize) const;
	bool getTextExtentEx(const WCHAR* pwszString, int nCount,
		int nMaxExtent, int* pnFit, int* pnDx, SIZE* pSize) const;
	
	bool fillSolidRect(const RECT& rect, COLORREF cr);
	bool drawFocusRect(const RECT& rect);
	
	int enumFontFamilies(const WCHAR* pwszFamily,
		FONTENUMPROC pProc, LPARAM lParam) const;
	
	int getDeviceCaps(int nIndex) const;

protected:
	void setHandle(HDC hdc);

private:
	HDC hdc_;
};


/****************************************************************************
 *
 * ClientDeviceContext
 *
 */

class QSEXPORTCLASS ClientDeviceContext : public DeviceContext
{
public:
	ClientDeviceContext(HWND hwnd, QSTATUS* pstatus);
	virtual ~ClientDeviceContext();

private:
	ClientDeviceContext(const ClientDeviceContext&);
	ClientDeviceContext& operator=(const ClientDeviceContext&);

private:
	HWND hwnd_;
};


/****************************************************************************
 *
 * PaintDeviceContext
 *
 */

class QSEXPORTCLASS PaintDeviceContext : public DeviceContext
{
public:
	PaintDeviceContext(HWND hwnd, QSTATUS* pstatus);
	virtual ~PaintDeviceContext();

private:
	PaintDeviceContext(const PaintDeviceContext&);
	PaintDeviceContext& operator=(const PaintDeviceContext&);

private:
	HWND hwnd_;
	PAINTSTRUCT ps_;
};


/****************************************************************************
 *
 * CompatibleDeviceContext
 *
 */

class QSEXPORTCLASS CompatibleDeviceContext : public DeviceContext
{
public:
	CompatibleDeviceContext(HDC hdc, QSTATUS* pstatus);
	virtual ~CompatibleDeviceContext();

private:
	CompatibleDeviceContext(const CompatibleDeviceContext&);
	CompatibleDeviceContext& operator=(const CompatibleDeviceContext&);
};


/****************************************************************************
 *
 * ObjectSelector
 *
 */

template<class Object>
class QSEXPORTCLASS ObjectSelector
{
public:
	ObjectSelector(DeviceContext& dc, Object o);
	~ObjectSelector();

private:
	ObjectSelector(const ObjectSelector&);
	ObjectSelector& operator=(const ObjectSelector&);

private:
	DeviceContext& dc_;
	Object o_;
};


/****************************************************************************
 *
 * GdiObject
 *
 */

template<class Object>
class QSEXPORTCLASS GdiObject
{
public:
	GdiObject(Object o);
	~GdiObject();

public:
	Object get() const;
	Object release();

private:
	GdiObject(const GdiObject&);
	GdiObject& operator=(const GdiObject&);

private:
	Object o_;
};


/****************************************************************************
 *
 * FontHelper
 *
 */

class QSEXPORTCLASS FontHelper
{
public:
	enum Style {
		STYLE_BOLD		= 0x01,
		STYLE_ITALIC	= 0x02,
		STYLE_UNDERLINE	= 0x04,
		STYLE_STRIKEOUT	= 0x08
	};

public:
	static QSTATUS createLogFont(HDC hdc, const WCHAR* pwszFaceName,
		int nPointSize, unsigned int nStyle, unsigned int nCharset, LOGFONT* plf);
};

}

#include <qsdevicecontext.inl>

#endif // __QSDEVICECONTEXT_H__
