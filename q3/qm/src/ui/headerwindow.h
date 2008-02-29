/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#ifndef __HEADERWINDOW_H__
#define __HEADERWINDOW_H__

#include <qm.h>
#include <qmsecurity.h>

#include <qsdevicecontext.h>
#include <qsdragdrop.h>
#include <qsmenu.h>
#include <qsregex.h>
#include <qssax.h>

#include "layout.h"
#include "messagewindow.h"
#include "../uimodel/attachmentselectionmodel.h"


namespace qm {

class HeaderLine;
class HeaderItem;
	class TextHeaderItem;
		class StaticHeaderItem;
		class EditHeaderItem;
	class AttachmentHeaderItem;
class TextHeaderItemCallback;
class TextHeaderItemSite;
class HeaderWindowContentHandler;
struct HeaderWindowCreateContext;

class TempFileCleaner;
class Template;
class TemplateContext;
class URIResolver;


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
	void fixup();
	MessageWindowItem* getNextFocusItem(MessageWindowItem** ppItem) const;
	MessageWindowItem* getPrevFocusItem(MessageWindowItem** ppItem) const;
	MessageWindowItem* getItemByNumber(unsigned int nNumber) const;

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
	unsigned int getFlags() const;
	unsigned int getNumber() const;

public:
	void setName(const WCHAR* pwszName);
	void setFlags(unsigned int nFlags,
				  unsigned int nMask);
	void setNumber(unsigned int nNumber);
	void setValue(std::auto_ptr<Template> pValue);

public:
	virtual void setMessage(const TemplateContext* pContext) = 0;
	virtual bool isEmptyValue() const = 0;
	virtual bool isActive() const = 0;
	virtual bool isFocusItem() const = 0;

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
	unsigned int nNumber_;
	std::auto_ptr<Template> pValue_;
};


/****************************************************************************
 *
 * TextHeaderItemCallback
 *
 */

class TextHeaderItemCallback
{
public:
	virtual ~TextHeaderItemCallback();

public:
	virtual HBRUSH getColor(qs::DeviceContext* pdc) = 0;
};


/****************************************************************************
 *
 * TextHeaderItemSite
 *
 */

class TextHeaderItemSite
{
public:
	virtual ~TextHeaderItemSite();

public:
	virtual void registerCallback(TextHeaderItem* pItem,
								  TextHeaderItemCallback* pCallback) = 0;
};


/****************************************************************************
 *
 * TextHeaderItem
 *
 */

class TextHeaderItem :
	public HeaderItem,
	public TextHeaderItemCallback
{
public:
	enum Style {
		STYLE_NORMAL	= 0x00,
		STYLE_BOLD		= 0x01,
		STYLE_ITALIC	= 0x02
	};
	
	enum Align {
		ALIGN_LEFT,
		ALIGN_CENTER,
		ALIGN_RIGHT
	};

protected:
	TextHeaderItem();

public:
	virtual ~TextHeaderItem();

public:
	HWND getHandle() const;

public:
	void setStyle(unsigned int nStyle);
	void setAlign(Align align);
	void setBackground(std::auto_ptr<Template> pBackground);

public:
	virtual unsigned int getHeight(unsigned int nWidth,
								   unsigned int nFontHeight) const;
	virtual bool create(qs::WindowBase* pParent,
						const std::pair<HFONT, HFONT>& fonts,
						UINT nId,
						void* pParam);
	virtual void destroy();
	virtual HDWP layout(HDWP hdwp,
						const RECT& rect,
						unsigned int nFontHeight);
	virtual void show(bool bShow);
	virtual void setFont(const std::pair<HFONT, HFONT>& fonts);

public:
	virtual void setMessage(const TemplateContext* pContext);
	virtual bool isEmptyValue() const;
	virtual bool isActive() const;

protected:
	Align getAlign() const;

protected:
	virtual const TCHAR* getWindowClassName() const = 0;
	virtual UINT getWindowStyle() const = 0;
	virtual void postLayout();

public:
	virtual HBRUSH getColor(qs::DeviceContext* pdc);

public:
	static unsigned int parseStyle(const WCHAR* pwszStyle);
	static Align parseAlign(const WCHAR* pwszAlign);

private:
	void updateColor(const TemplateContext* pContext);

private:
	TextHeaderItem(const TextHeaderItem&);
	TextHeaderItem& operator=(const TextHeaderItem&);

private:
	unsigned int nStyle_;
	Align align_;
	std::auto_ptr<Template> pBackground_;
	HWND hwnd_;
	COLORREF crBackground_;
	HBRUSH hbrBackground_;
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

public:
	virtual unsigned int getPreferredWidth() const;

public:
	virtual bool isFocusItem() const;

protected:
	virtual const TCHAR* getWindowClassName() const;
	virtual UINT getWindowStyle() const;

public:
	virtual void setFocus();

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

public:
	void setMultiline(unsigned int nMultiline);
	void setWrap(bool bWrap);

public:
	virtual unsigned int getHeight(unsigned int nWidth,
								   unsigned int nFontHeight) const;

public:
	virtual bool isFocusItem() const;

protected:
	virtual const TCHAR* getWindowClassName() const;
	virtual UINT getWindowStyle() const;
	virtual void postLayout();

public:
	virtual void copy();
	virtual bool canCopy();
	virtual void selectAll();
	virtual bool canSelectAll();
	virtual void setFocus();

private:
	unsigned int getLineCount(unsigned int nWidth,
							  unsigned int nFontHeight) const;

public:
	static unsigned int parseMultiline(const WCHAR* pwszMultiline);

private:
	EditHeaderItem(const EditHeaderItem&);
	EditHeaderItem& operator=(const EditHeaderItem&);

private:
	unsigned int nMultiline_;
	bool bWrap_;
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
	AttachmentHeaderItem(qs::MenuManager* pMenuManager,
						 TempFileCleaner* pTempFileCleaner);
	virtual ~AttachmentHeaderItem();

public:
	void setBackground(std::auto_ptr<Template> pBackground);

public:
	virtual unsigned int getHeight(unsigned int nWidth,
								   unsigned int nFontHeight) const;
	virtual bool create(qs::WindowBase* pParent,
						const std::pair<HFONT, HFONT>& fonts,
						UINT nId,
						void* pParam);
	virtual void destroy();
	virtual HDWP layout(HDWP hdwp,
						const RECT& rect,
						unsigned int nFontHeight);
	virtual void show(bool bShow);
	virtual void setFont(const std::pair<HFONT, HFONT>& fonts);

public:
	virtual void setMessage(const TemplateContext* pContext);
	virtual bool isEmptyValue() const;
	virtual bool isActive() const;
	virtual bool isFocusItem() const;

public:
	virtual void setFocus();

public:
	virtual bool hasAttachment();
	virtual bool hasSelectedAttachment();
	virtual void getSelectedAttachment(NameList* pList);
	virtual bool isAttachmentDeleted();

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
	void updateColor(const TemplateContext* pContext);

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
		LRESULT onSize(UINT nFlags,
					   int cx,
					   int cy);
	
	private:
		AttachmentWindow(const AttachmentWindow&);
		AttachmentWindow& operator=(const AttachmentWindow&);
	
	private:
		AttachmentHeaderItem* pItem_;
	};
	friend class AttachmentWindow;

private:
	std::auto_ptr<Template> pBackground_;
	AttachmentWindow wnd_;
	qs::MenuManager* pMenuManager_;
	TempFileCleaner* pTempFileCleaner_;
	qs::WindowBase* pParent_;
	const URIResolver* pURIResolver_;
	unsigned int nSecurityMode_;
	bool bAttachmentDeleted_;
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
							   qs::MenuManager* pMenuManager,
							   TempFileCleaner* pTempFileCleaner);
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
	static void setNumber(HeaderItem* pItem,
						  const WCHAR* pwszNumber);
	static std::auto_ptr<Template> parseTemplate(const WCHAR* pwszTemplate);

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
	qs::MenuManager* pMenuManager_;
	TempFileCleaner* pTempFileCleaner_;
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
	qs::MenuManager* pMenuManager_;
	TempFileCleaner* pTempFileCleaner_;
};

}

#endif // __HEADERWINDOW_H__
