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
				UINT* pnId) const;
	void destroy() const;
	void layout(const RECT& rect,
				unsigned int nFontHeight);

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
	LineLayoutLine();
	virtual ~LineLayoutLine();

public:
	unsigned int getItemCount() const;
	LineLayoutItem* getItem(unsigned int n) const;
	bool create(qs::WindowBase* pParent,
				const std::pair<HFONT, HFONT>& fonts,
				UINT* pnId) const;
	void destroy() const;
	unsigned int layout(const RECT& rect,
						unsigned int nFontHeight) const;
	void show(bool bShow) const;

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
		UNIT_EM
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
	virtual bool create(qs::WindowBase* pParent,
						const std::pair<HFONT, HFONT>& fonts,
						UINT nId) = 0;
	virtual void destroy() = 0;
	virtual void layout(const RECT& rect,
						unsigned int nFontHeight) = 0;
	virtual void show(bool bShow) = 0;

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
