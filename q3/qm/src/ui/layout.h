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
	explicit LineLayout(qs::QSTATUS* pstatus);
	~LineLayout();

public:
	unsigned int getLineCount() const;
	LineLayoutLine* getLine(unsigned int n) const;
	int getHeight() const;
	qs::QSTATUS create(qs::WindowBase* pParent,
		const std::pair<HFONT, HFONT>& fonts, UINT* pnId) const;
	qs::QSTATUS destroy() const;
	qs::QSTATUS layout(const RECT& rect, unsigned int nFontHeight);

public:
	void setLineSpacing(unsigned int nLineSpacing);
	qs::QSTATUS addLine(LineLayoutLine* pLine);

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
	explicit LineLayoutLine(qs::QSTATUS* pstatus);
	virtual ~LineLayoutLine();

public:
	unsigned int getItemCount() const;
	LineLayoutItem* getItem(unsigned int n) const;
	unsigned int getHeight(unsigned int nFontHeight) const;
	qs::QSTATUS create(qs::WindowBase* pParent,
		const std::pair<HFONT, HFONT>& fonts, UINT* pnId) const;
	qs::QSTATUS destroy() const;
	qs::QSTATUS layout(const RECT& rect, unsigned int nFontHeight) const;
	qs::QSTATUS show(bool bShow) const;

public:
	qs::QSTATUS addItem(LineLayoutItem* pItem);

protected:
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
	explicit LineLayoutItem(qs::QSTATUS* pstatus);

public:
	virtual ~LineLayoutItem();

public:
	double getWidth() const;
	Unit getUnit() const;

public:
	void setWidth(double dWidth, Unit unit);
	qs::QSTATUS setWidth(const WCHAR* pwszWidth);

public:
	virtual unsigned int getHeight(unsigned int nFontHeight) const = 0;
	virtual qs::QSTATUS create(qs::WindowBase* pParent,
		const std::pair<HFONT, HFONT>& fonts, UINT nId) = 0;
	virtual qs::QSTATUS destroy() = 0;
	virtual qs::QSTATUS layout(const RECT& rect, unsigned int nFontHeight) = 0;
	virtual qs::QSTATUS show(bool bShow) = 0;

private:
	LineLayoutItem(const LineLayoutItem&);
	LineLayoutItem& operator=(const LineLayoutItem&);

private:
	double dWidth_;
	Unit unit_;
};

}

#endif // __LAYOUT_H__
