/*
 * $Id: headerwindow.h,v 1.1.1.1 2003/04/29 08:07:32 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __HEADERWINDOW_H__
#define __HEADERWINDOW_H__

#include <qm.h>

#include <qssax.h>

#include "layout.h"
#include "messagewindow.h"


namespace qm {

class HeaderLine;
class HeaderItem;
	class TextHeaderItem;
		class StaticHeaderItem;
		class EditHeaderItem;
	class AttachmentHeaderItem;
class HeaderWindowContentHandler;
struct HeaderWindowCreateContext;

class Template;
class TemplateContext;


/****************************************************************************
 *
 * HeaderLine
 *
 */

class HeaderLine : public LineLayoutLine
{
public:
	typedef std::vector<std::pair<qs::WSTRING, HeaderItem*> > HideList;

public:
	explicit HeaderLine(qs::QSTATUS* pstatus);
	~HeaderLine();

public:
	qs::QSTATUS setMessage(const TemplateContext& context);

public:
	qs::QSTATUS setHideIfEmpty(const WCHAR* pwszName);
	qs::QSTATUS fixup();

protected:
	virtual bool isHidden() const;

private:
	HeaderLine(const HeaderLine&);
	HeaderLine& operator=(const HeaderLine&);

private:
	HideList listHide_;
};


/****************************************************************************
 *
 * HeaderItem
 *
 */

class HeaderItem : public LineLayoutItem, public MessageWindowItem
{
public:
	enum Flag {
		FLAG_SHOWALWAYS	= 0x01
	};

protected:
	explicit HeaderItem(qs::QSTATUS* pstatus);

public:
	virtual ~HeaderItem();

public:
	const WCHAR* getName() const;

public:
	qs::QSTATUS setName(const WCHAR* pwszName);
	void setFlags(unsigned int nFlags, unsigned int nMask);
	qs::QSTATUS addValue(const WCHAR* pwszValue, size_t nLen);
	qs::QSTATUS fixupValue();

public:
	virtual qs::QSTATUS setMessage(const TemplateContext& context) = 0;
	virtual bool isEmptyValue() const = 0;
	virtual bool isActive() const = 0;

public:
	virtual qs::QSTATUS copy();
	virtual qs::QSTATUS canCopy(bool* pbCan);
	virtual qs::QSTATUS selectAll();
	virtual qs::QSTATUS canSelectAll(bool* pbCan);

protected:
	qs::QSTATUS getValue(const TemplateContext& context,
		qs::WSTRING* pwstrValue) const;

private:
	HeaderItem(const HeaderItem&);
	HeaderItem& operator=(const HeaderItem&);

private:
	qs::WSTRING wstrName_;
	qs::WSTRING wstrValue_;
	unsigned int nFlags_;
	Template* pTemplate_;
};


/****************************************************************************
 *
 * TextHeaderItem
 *
 */

class TextHeaderItem : public HeaderItem
{
public:
	enum Style {
		STYLE_NORMAL	= 0x00,
		STYLE_BOLD		= 0x01,
		STYLE_ITALIC	= 0x02
	};

protected:
	explicit TextHeaderItem(qs::QSTATUS* pstatus);

public:
	virtual ~TextHeaderItem();

public:
	qs::QSTATUS setStyle(const WCHAR* pwszStyle);

public:
	virtual unsigned int getHeight(unsigned int nFontHeight) const;
	virtual qs::QSTATUS create(qs::WindowBase* pParent,
		const std::pair<HFONT, HFONT>& fonts, UINT nId);
	virtual qs::QSTATUS destroy();
	virtual qs::QSTATUS layout(const RECT& rect, unsigned int nFontHeight);
	virtual qs::QSTATUS show(bool bShow);

public:
	virtual qs::QSTATUS setMessage(const TemplateContext& context);
	virtual bool isEmptyValue() const;
	virtual bool isActive() const;

protected:
	virtual const TCHAR* getWindowClassName() const = 0;
	virtual UINT getWindowStyle() const = 0;

protected:
	HWND getHandle() const;

private:
	TextHeaderItem(const TextHeaderItem&);
	TextHeaderItem& operator=(const TextHeaderItem&);

private:
	unsigned int nStyle_;
	HWND hwnd_;
};


/****************************************************************************
 *
 * StaticHeaderItem
 *
 */

class StaticHeaderItem : public TextHeaderItem
{
public:
	explicit StaticHeaderItem(qs::QSTATUS* pstatus);
	virtual ~StaticHeaderItem();

protected:
	virtual const TCHAR* getWindowClassName() const;
	virtual UINT getWindowStyle() const;

private:
	StaticHeaderItem(const StaticHeaderItem&);
	StaticHeaderItem& operator=(const StaticHeaderItem&);
};


/****************************************************************************
 *
 * EditHeaderItem
 *
 */

class EditHeaderItem : public TextHeaderItem
{
public:
	explicit EditHeaderItem(qs::QSTATUS* pstatus);
	virtual ~EditHeaderItem();

protected:
	virtual const TCHAR* getWindowClassName() const;
	virtual UINT getWindowStyle() const;

public:
	virtual qs::QSTATUS copy();
	virtual qs::QSTATUS canCopy(bool* pbCan);
	virtual qs::QSTATUS selectAll();
	virtual qs::QSTATUS canSelectAll(bool* pbCan);

private:
	EditHeaderItem(const EditHeaderItem&);
	EditHeaderItem& operator=(const EditHeaderItem&);
};


/****************************************************************************
 *
 * AttachmentHeaderItem
 *
 */

class AttachmentHeaderItem : public HeaderItem
{
public:
	explicit AttachmentHeaderItem(qs::QSTATUS* pstatus);
	virtual ~AttachmentHeaderItem();

public:
	virtual unsigned int getHeight(unsigned int nFontHeight) const;
	virtual qs::QSTATUS create(qs::WindowBase* pParent,
		const std::pair<HFONT, HFONT>& fonts, UINT nId);
	virtual qs::QSTATUS destroy();
	virtual qs::QSTATUS layout(const RECT& rect, unsigned int nFontHeight);
	virtual qs::QSTATUS show(bool bShow);

public:
	virtual qs::QSTATUS setMessage(const TemplateContext& context);
	virtual bool isEmptyValue() const;
	virtual bool isActive() const;

private:
	AttachmentHeaderItem(const AttachmentHeaderItem&);
	AttachmentHeaderItem& operator=(const AttachmentHeaderItem&);

private:
	HWND hwnd_;
};


/****************************************************************************
 *
 * HeaderWindowContentHandler
 *
 */

class HeaderWindowContentHandler : public qs::DefaultHandler
{
public:
	HeaderWindowContentHandler(LineLayout* pLayout, qs::QSTATUS* pstatus);
	virtual ~HeaderWindowContentHandler();

public:
	virtual qs::QSTATUS startElement(const WCHAR* pwszNamespaceURI,
		const WCHAR* pwszLocalName, const WCHAR* pwszQName,
		const qs::Attributes& attributes);
	virtual qs::QSTATUS endElement(const WCHAR* pwszNamespaceURI,
		const WCHAR* pwszLocalName, const WCHAR* pwszQName);
	virtual qs::QSTATUS characters(const WCHAR* pwsz,
		size_t nStart, size_t nLength);

private:
	HeaderWindowContentHandler(const HeaderWindowContentHandler&);
	HeaderWindowContentHandler& operator=(const HeaderWindowContentHandler&);

private:
	enum State {
		STATE_ROOT,
		STATE_HEADER,
		STATE_LINE,
		STATE_ITEM
	};

private:
	LineLayout* pLayout_;
	HeaderLine* pCurrentLine_;
	HeaderItem* pCurrentItem_;
	State state_;
};


/****************************************************************************
 *
 * HeaderWindowCreateContext
 *
 */

struct HeaderWindowCreateContext
{
	Document* pDocument_;
};

}

#endif // __HEADERWINDOW_H__
