/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __HEADERWINDOW_H__
#define __HEADERWINDOW_H__

#include <qm.h>

#include <qsdragdrop.h>
#include <qsmenu.h>
#include <qsregex.h>
#include <qssax.h>

#include "attachmentselectionmodel.h"
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
	HeaderLine(const WCHAR* pwszHideIfEmpty,
			   std::auto_ptr<qs::RegexPattern> pClass);
	~HeaderLine();

public:
	void setMessage(const TemplateContext* pContext);

public:
	void fixup();

public:
	virtual bool isHidden() const;

private:
	HeaderLine(const HeaderLine&);
	HeaderLine& operator=(const HeaderLine&);

private:
	HideList listHide_;
	std::auto_ptr<qs::RegexPattern> pClass_;
	bool bHide_;
};


/****************************************************************************
 *
 * HeaderItem
 *
 */

class HeaderItem :
	public LineLayoutItem,
	public MessageWindowItem
{
public:
	enum Flag {
		FLAG_SHOWALWAYS	= 0x01
	};

protected:
	HeaderItem();

public:
	virtual ~HeaderItem();

public:
	const WCHAR* getName() const;

public:
	void setName(const WCHAR* pwszName);
	void setFlags(unsigned int nFlags,
				  unsigned int nMask);
	void setTemplate(std::auto_ptr<Template> pTemplate);

public:
	virtual void setMessage(const TemplateContext* pContext) = 0;
	virtual bool isEmptyValue() const = 0;
	virtual bool isActive() const = 0;

public:
	virtual void copy();
	virtual bool canCopy();
	virtual void selectAll();
	virtual bool canSelectAll();

protected:
	qs::wstring_ptr getValue(const TemplateContext& context) const;

private:
	HeaderItem(const HeaderItem&);
	HeaderItem& operator=(const HeaderItem&);

private:
	qs::wstring_ptr wstrName_;
	unsigned int nFlags_;
	std::auto_ptr<Template> pTemplate_;
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
	TextHeaderItem();

public:
	virtual ~TextHeaderItem();

public:
	void setStyle(unsigned int nStyle);

public:
	virtual unsigned int getHeight(unsigned int nWidth,
								   unsigned int nFontHeight) const;
	virtual bool create(qs::WindowBase* pParent,
						const std::pair<HFONT, HFONT>& fonts,
						UINT nId);
	virtual void destroy();
	virtual void layout(const RECT& rect,
						unsigned int nFontHeight);
	virtual void show(bool bShow);

public:
	virtual void setMessage(const TemplateContext* pContext);
	virtual bool isEmptyValue() const;
	virtual bool isActive() const;

protected:
	virtual const TCHAR* getWindowClassName() const = 0;
	virtual UINT getWindowStyle() const = 0;

protected:
	HWND getHandle() const;

public:
	static unsigned int parseStyle(const WCHAR* pwszStyle);

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
	StaticHeaderItem();
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
	EditHeaderItem();
	virtual ~EditHeaderItem();

protected:
	virtual const TCHAR* getWindowClassName() const;
	virtual UINT getWindowStyle() const;

public:
	virtual void copy();
	virtual bool canCopy();
	virtual void selectAll();
	virtual bool canSelectAll();

private:
	EditHeaderItem(const EditHeaderItem&);
	EditHeaderItem& operator=(const EditHeaderItem&);
};


/****************************************************************************
 *
 * AttachmentHeaderItem
 *
 */

class AttachmentHeaderItem :
	public HeaderItem,
	public AttachmentSelectionModel
#ifndef _WIN32_WCE
	,
	public qs::NotifyHandler,
	public qs::DragSourceHandler
#endif
{
public:
	AttachmentHeaderItem(SecurityModel* pSecurityModel,
						 qs::MenuManager* pMenuManager);
	virtual ~AttachmentHeaderItem();

public:
	virtual unsigned int getHeight(unsigned int nWidth,
								   unsigned int nFontHeight) const;
	virtual bool create(qs::WindowBase* pParent,
						const std::pair<HFONT, HFONT>& fonts,
						UINT nId);
	virtual void destroy();
	virtual void layout(const RECT& rect,
						unsigned int nFontHeight);
	virtual void show(bool bShow);

public:
	virtual void setMessage(const TemplateContext* pContext);
	virtual bool isEmptyValue() const;
	virtual bool isActive() const;

public:
	virtual bool hasAttachment();
	virtual bool hasSelectedAttachment();
	virtual void getSelectedAttachment(NameList* pList);

#ifndef _WIN32_WCE
public:
	virtual LRESULT onNotify(NMHDR* pnmhdr,
							 bool* pbHandled);

private:
	LRESULT onBeginDrag(NMHDR* pnmhdr,
						bool* pbHandled);

public:
	virtual void dragDropEnd(const qs::DragSourceDropEvent& event);
#endif

private:
	void clear();

private:
	AttachmentHeaderItem(const AttachmentHeaderItem&);
	AttachmentHeaderItem& operator=(const AttachmentHeaderItem&);

private:
	class AttachmentWindow :
		public qs::WindowBase,
		public qs::DefaultWindowHandler
	{
	public:
		explicit AttachmentWindow(AttachmentHeaderItem* pItem);
		virtual ~AttachmentWindow();
	
	public:
		virtual LRESULT windowProc(UINT uMsg,
								   WPARAM wParam,
								   LPARAM lParam);
	
	protected:
		LRESULT onContextMenu(HWND hwnd,
							  const POINT& pt);
		LRESULT onLButtonDblClk(UINT nFlags,
								const POINT& pt);
		LRESULT onLButtonDown(UINT nFlags,
							  const POINT& pt);
	
	private:
		AttachmentWindow(const AttachmentWindow&);
		AttachmentWindow& operator=(const AttachmentWindow&);
	
	private:
		AttachmentHeaderItem* pItem_;
	};
	friend class AttachmentWindow;

private:
	AttachmentWindow wnd_;
	SecurityModel* pSecurityModel_;
	qs::MenuManager* pMenuManager_;
	Document* pDocument_;
	qs::WindowBase* pParent_;
};


/****************************************************************************
 *
 * HeaderWindowContentHandler
 *
 */

class HeaderWindowContentHandler : public qs::DefaultHandler
{
public:
	HeaderWindowContentHandler(LineLayout* pLayout,
							   SecurityModel* pSecurityModel,
							   qs::MenuManager* pMenuManager);
	virtual ~HeaderWindowContentHandler();

public:
	AttachmentSelectionModel* getAttachmentSelectionModel() const;

public:
	virtual bool startElement(const WCHAR* pwszNamespaceURI,
							  const WCHAR* pwszLocalName,
							  const WCHAR* pwszQName,
							  const qs::Attributes& attributes);
	virtual bool endElement(const WCHAR* pwszNamespaceURI,
							const WCHAR* pwszLocalName,
							const WCHAR* pwszQName);
	virtual bool characters(const WCHAR* pwsz,
							size_t nStart,
							size_t nLength);

private:
	static void setWidth(LineLayoutItem* pItem,
						 const WCHAR* pwszWidth);

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
	SecurityModel* pSecurityModel_;
	qs::MenuManager* pMenuManager_;
	HeaderLine* pCurrentLine_;
	HeaderItem* pCurrentItem_;
	State state_;
	AttachmentSelectionModel* pAttachmentSelectionModel_;
	qs::StringBuffer<qs::WSTRING> buffer_;
};


/****************************************************************************
 *
 * HeaderWindowCreateContext
 *
 */

struct HeaderWindowCreateContext
{
	Document* pDocument_;
	qs::MenuManager* pMenuManager_;
	SecurityModel* pSecurityModel_;
};

}

#endif // __HEADERWINDOW_H__
