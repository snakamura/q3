/*
 * $Id$
 *
 * Copyright(C) 1998 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __LAYOUT_H__
#define __LAYOUT_H__

#include <qs.h>
#include <qswindow.h>

#include <vector>


namespace qm {

class LineLayout;
class LineLayoutLine;
class LineLayoutItem;


/****************************************************************************
 *
 * LineLayout
 *
 */

class LineLayout
{
public:
	LineLayout();
	~LineLayout();

public:
	unsigned int getLineCount() const;
	LineLayoutLine* getLine(unsigned int n) const;
	int getHeight() const;
	bool create(qs::WindowBase* pParent,
				const std::pair<HFONT, HFONT>& fonts,
				UINT* pnId,
				void* pParam) const;
	void destroy() const;
	void layout(const RECT& rect,
				unsigned int nFontHeight);
	void setFont(const std::pair<HFONT, HFONT>& fonts);

public:
	void setLineSpacing(unsigned int nLineSpacing);
	void addLine(std::auto_ptr<LineLayoutLine> pLine);

private:
	LineLayout(const LineLayout&);
	LineLayout& operator=(const LineLayout&);

private:
	typedef std::vector<LineLayoutLine*> LineList;

private:
	LineList listLine_;
	unsigned int nHeight_;
	unsigned int nLineSpacing_;
};


/****************************************************************************
 *
 * LineLayoutLine
 *
 */

class LineLayoutLine
{
public:
	enum {
		MAX_MINMAX = 9
	};

public:
	typedef std::vector<unsigned int> WidthMap;

public:
	LineLayoutLine();
	virtual ~LineLayoutLine();

public:
	unsigned int getItemCount() const;
	LineLayoutItem* getItem(unsigned int n) const;
	bool create(qs::WindowBase* pParent,
				const std::pair<HFONT, HFONT>& fonts,
				UINT* pnId,
				void* pParam) const;
	void destroy() const;
	HDWP layout(HDWP hdwp,
				const RECT& rect,
				unsigned int nFontHeight,
				const WidthMap& mapWidth,
				unsigned int* pnHeight) const;
	void show(bool bShow) const;
	void setFont(const std::pair<HFONT, HFONT>& fonts);

public:
	void addItem(std::auto_ptr<LineLayoutItem> pItem);

public:
	virtual bool isHidden() const;

private:
	LineLayoutLine(const LineLayoutLine&);
	LineLayoutLine& operator=(const LineLayoutLine&);

private:
	typedef std::vector<LineLayoutItem*> ItemList;

private:
	ItemList listItem_;
};


/****************************************************************************
 *
 * LineLayoutItem
 *
 */

class LineLayoutItem
{
public:
	enum Unit {
		UNIT_NONE,
		UNIT_PIXEL,
		UNIT_PERCENT,
		UNIT_EM,
		UNIT_AUTO,
		UNIT_MINMAX
	};

protected:
	LineLayoutItem();

public:
	virtual ~LineLayoutItem();

public:
	double getWidth() const;
	Unit getUnit() const;

public:
	void setWidth(double dWidth,
				  Unit unit);

public:
	virtual unsigned int getHeight(unsigned int nWidth,
								   unsigned int nFontHeight) const = 0;
	virtual unsigned int getPreferredWidth() const;
	virtual bool create(qs::WindowBase* pParent,
						const std::pair<HFONT, HFONT>& fonts,
						UINT nId,
						void* pParam) = 0;
	virtual void destroy() = 0;
	virtual HDWP layout(HDWP hdwp,
						const RECT& rect,
						unsigned int nFontHeight) = 0;
	virtual void show(bool bShow) = 0;
	virtual void setFont(const std::pair<HFONT, HFONT>& fonts) = 0;

public:
	static bool parseWidth(const WCHAR* pwszWidth,
						   double* pdWidth,
						   Unit* pUnit);

private:
	LineLayoutItem(const LineLayoutItem&);
	LineLayoutItem& operator=(const LineLayoutItem&);

private:
	double dWidth_;
	Unit unit_;
};

}

#endif // __LAYOUT_H__
