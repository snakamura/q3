/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#ifndef __QSDEVICECONTEXT_H__
#define __QSDEVICECONTEXT_H__

#include <qs.h>

#include <windows.h>

namespace qs {

class DeviceContext;
	class ClientDeviceContext;
	class WindowDeviceContext;
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
	int excludeClipRect(int nLeft,
						int nTop,
						int nRight,
						int nBottom);
	
	HFONT selectObject(HFONT hfont);
	HPEN selectObject(HPEN hpen);
	HBRUSH selectObject(HBRUSH hbrush);
	HBITMAP selectObject(HBITMAP hbm);
	
	bool polyline(const POINT* ppt,
				  int nPoints);
	bool rectangle(const RECT& rect);
	bool bitBlt(int x,
				int y,
				int nWidth,
				int nHeight,
				HDC hdc,
				int nSrcX,
				int nSrcY,
				DWORD dwRop);
	bool patBlt(int x,
				int y,
				int nWidth,
				int nHeight,
				DWORD dwRop);
	
	COLORREF getPixel(int x,
					  int y);
	COLORREF setPixel(int x,
					  int y,
					  COLORREF cr);
	
	int drawText(const WCHAR* pwszText,
				 int nCount,
				 RECT* pRect,
				 UINT nFormat);
	bool extTextOut(int x,
					int y,
					UINT nOptions,
					const RECT& rect,
					const WCHAR* pwszString,
					UINT nCount,
					int* pnDx);
	bool extTextOutEllipsis(int x,
							int y,
							int nWidth,
							UINT nOptions,
							const RECT& rect,
							const WCHAR* pwszString,
							UINT nCount);
	bool getTextMetrics(TEXTMETRIC* ptm) const;
#if !defined _WIN32_WCE || _WIN32_WCE >= 0x500
	UINT getOutlineTextMetrics(UINT nSize,
							   OUTLINETEXTMETRIC* potm) const;
#endif
	bool getTextExtent(const WCHAR* pwszString,
					   int nCount,
					   SIZE* pSize) const;
	bool getTextExtentEx(const WCHAR* pwszString,
						 int nCount,
						 int nMaxExtent,
						 int* pnFit,
						 int* pnDx,
						 SIZE* pSize) const;
	
	bool fillSolidRect(const RECT& rect,
					   COLORREF cr);
	bool drawFocusRect(const RECT& rect);
	bool drawFrameControl(RECT* pRect,
						  UINT nType,
						  UINT nState);
	
	int enumFontFamilies(const WCHAR* pwszFamily,
						 FONTENUMPROC pProc,
						 LPARAM lParam) const;
	
	int getDeviceCaps(int nIndex) const;
	
	bool drawIcon(int x,
				  int y,
				  HICON hIcon);

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
	explicit ClientDeviceContext(HWND hwnd);
	virtual ~ClientDeviceContext();

private:
	ClientDeviceContext(const ClientDeviceContext&);
	ClientDeviceContext& operator=(const ClientDeviceContext&);

private:
	HWND hwnd_;
};


/****************************************************************************
 *
 * WindowDeviceContext
 *
 */

class QSEXPORTCLASS WindowDeviceContext : public DeviceContext
{
public:
	explicit WindowDeviceContext(HWND hwnd);
	virtual ~WindowDeviceContext();

private:
	WindowDeviceContext(const WindowDeviceContext&);
	WindowDeviceContext& operator=(const WindowDeviceContext&);

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
	explicit PaintDeviceContext(HWND hwnd);
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
	explicit CompatibleDeviceContext(HDC hdc);
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
class ObjectSelector
{
public:
	ObjectSelector(DeviceContext& dc,
				   Object o);
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
class GdiObject
{
public:
	GdiObject(Object o);
	~GdiObject();

public:
	Object get() const;
	Object release();
	void reset(Object o);

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
	static void createLogFont(HDC hdc,
							  const WCHAR* pwszFaceName,
							  double dPointSize,
							  unsigned int nStyle,
							  unsigned int nCharset,
							  LOGFONT* plf);
};

}

#include <qsdevicecontext.inl>

#endif // __QSDEVICECONTEXT_H__
