/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __HEADEREDITWINDOW_H__
#define __HEADEREDITWINDOW_H__

#include <qsregex.h>
#include <qssax.h>

#include <vector>

#include "editwindow.h"
#include "layout.h"
#include "../model/editmessage.h"


namespace qm {

class HeaderEditLine;
class HeaderEditLineCallback;
class HeaderEditItem;
	class TextHeaderEditItem;
		class StaticHeaderEditItem;
		class EditHeaderEditItem;
	class AttachmentHeaderEditItem;
	class ComboBoxHeaderEditItem;
		class SignatureHeaderEditItem;
		class AccountHeaderEditItem;
class HeaderEditWindowContentHandler;

class AddressBook;


/****************************************************************************
 *
 * HeaderEditLine
 *
 */

class HeaderEditLine : public LineLayoutLine
{
public:
	enum {
		FLAG_HIDEIFNOFOCUS	= 0x01
	};

public:
	HeaderEditLine(HeaderEditLineCallback* pCallback, unsigned int nFlags,
		qs::RegexPattern* pClass, qs::QSTATUS* pstatus);
	~HeaderEditLine();

public:
	qs::QSTATUS setEditMessage(EditMessage* pEditMessage, bool bReset);
	EditWindowItem* getNextFocusItem(EditWindowItem** ppItem) const;
	EditWindowItem* getPrevFocusItem(EditWindowItem** ppItem) const;
	HeaderEditItem* getFocusedItem() const;
	HeaderEditItem* getInitialFocusItem() const;

protected:
	virtual bool isHidden() const;

private:
	HeaderEditLine(const HeaderEditLine&);
	HeaderEditLine& operator=(const HeaderEditLine&);

private:
	HeaderEditLineCallback* pCallback_;
	unsigned int nFlags_;
	qs::RegexPattern* pClass_;
	bool bHide_;
};


/****************************************************************************
 *
 * HeaderEditLineCallback
 *
 */

class HeaderEditLineCallback
{
public:
	virtual ~HeaderEditLineCallback();

public:
	virtual bool isHidden() const = 0;
};


/****************************************************************************
 *
 * HeaderEditItem
 *
 */

class HeaderEditItem : public LineLayoutItem, public EditWindowItem
{
protected:
	enum InitialFocus {
		INITIALFOCUS_NONE,
		INITIALFOCUS_TRUE,
		INITIALFOCUS_FALSE
	};

protected:
	HeaderEditItem(EditWindowFocusController* pController,
		qs::QSTATUS* pstatus);

public:
	virtual ~HeaderEditItem();

public:
	unsigned int getNumber() const;

public:
	virtual qs::QSTATUS setEditMessage(
		EditMessage* pEditMessage, bool bReset) = 0;
	virtual void releaseEditMessage(EditMessage* pEditMessage) = 0;
	virtual qs::QSTATUS updateEditMessage(EditMessage* pEditMessage) = 0;
	virtual bool hasFocus() const = 0;
	virtual bool hasInitialFocus() const = 0;
	virtual bool isFocusItem() const = 0;

public:
	qs::QSTATUS setNumber(const WCHAR* pwszNumber);
	void setInitialFocus(bool bInitialFocus);
	qs::QSTATUS addValue(const WCHAR* pwszValue, size_t nLen);

protected:
	EditWindowFocusController* getController() const;
	InitialFocus getInitialFocus() const;
	const WCHAR* getValue() const;

public:
	virtual qs::QSTATUS copy();
	virtual qs::QSTATUS canCopy(bool* pbCan);
	virtual qs::QSTATUS cut();
	virtual qs::QSTATUS canCut(bool* pbCan);
	virtual qs::QSTATUS paste();
	virtual qs::QSTATUS canPaste(bool* pbCan);
	virtual qs::QSTATUS selectAll();
	virtual qs::QSTATUS canSelectAll(bool* pbCan);
	virtual qs::QSTATUS undo();
	virtual qs::QSTATUS canUndo(bool* pbCan);
	virtual qs::QSTATUS redo();
	virtual qs::QSTATUS canRedo(bool* pbCan);

private:
	HeaderEditItem(const HeaderEditItem&);
	HeaderEditItem& operator=(const HeaderEditItem&);

private:
	EditWindowFocusController* pController_;
	unsigned int nNumber_;
	InitialFocus initialFocus_;
	qs::WSTRING wstrValue_;
};


/****************************************************************************
 *
 * TextHeaderEditItem
 *
 */

class TextHeaderEditItem :
	public HeaderEditItem,
	public DefaultEditMessageHandler
{
public:
	enum Style {
		STYLE_NORMAL	= 0x00,
		STYLE_BOLD		= 0x01,
		STYLE_ITALIC	= 0x02
	};
	
	enum Type {
		TYPE_UNSTRUCTURED,
		TYPE_ADDRESSLIST
	};

protected:
	TextHeaderEditItem(EditWindowFocusController* pController,
		qs::QSTATUS* pstatus);

public:
	virtual ~TextHeaderEditItem();

public:
	virtual qs::QSTATUS setEditMessage(
		EditMessage* pEditMessage, bool bReset);
	virtual void releaseEditMessage(EditMessage* pEditMessage);
	virtual bool hasFocus() const;

public:
	qs::QSTATUS setStyle(const WCHAR* pwszStyle);
	qs::QSTATUS setField(const WCHAR* pwszField);
	qs::QSTATUS setType(const WCHAR* pwszType);

public:
	virtual qs::QSTATUS create(qs::WindowBase* pParent,
		const std::pair<HFONT, HFONT>& fonts, UINT nId);
	virtual qs::QSTATUS destroy();
	virtual qs::QSTATUS layout(const RECT& rect, unsigned int nFontHeight);
	virtual qs::QSTATUS show(bool bShow);

public:
	virtual qs::QSTATUS fieldChanged(const EditMessageFieldEvent& event);

protected:
	HWND getHandle() const;
	const WCHAR* getField() const;
	Type getType() const;

protected:
	virtual const TCHAR* getWindowClassName() const = 0;
	virtual UINT getWindowStyle() const = 0;
	virtual UINT getWindowExStyle() const = 0;

public:
	virtual qs::QSTATUS setFocus();

private:
	TextHeaderEditItem(const TextHeaderEditItem&);
	TextHeaderEditItem& operator=(const TextHeaderEditItem&);

private:
	unsigned int nStyle_;
	qs::WSTRING wstrField_;
	Type type_;
	HWND hwnd_;
	EditWindowItemWindow* pItemWindow_;
};


/****************************************************************************
 *
 * StaticHeaderEditItem
 *
 */

class StaticHeaderEditItem : public TextHeaderEditItem
{
public:
	StaticHeaderEditItem(EditWindowFocusController* pController,
		qs::QSTATUS* pstatus);
	virtual ~StaticHeaderEditItem();

public:
	virtual qs::QSTATUS updateEditMessage(EditMessage* pEditMessage);
	virtual bool hasInitialFocus() const;
	virtual bool isFocusItem() const;

public:
	virtual unsigned int getHeight(unsigned int nFontHeight) const;

protected:
	virtual const TCHAR* getWindowClassName() const;
	virtual UINT getWindowStyle() const;
	virtual UINT getWindowExStyle() const;

private:
	StaticHeaderEditItem(const StaticHeaderEditItem&);
	StaticHeaderEditItem& operator=(const StaticHeaderEditItem&);
};


/****************************************************************************
 *
 * EditHeaderEditItem
 *
 */

class EditHeaderEditItem :
	public TextHeaderEditItem,
	public qs::DefaultCommandHandler
{
public:
	EditHeaderEditItem(EditWindowFocusController* pController,
		qs::QSTATUS* pstatus);
	virtual ~EditHeaderEditItem();

public:
	void setExpandAlias(bool bExpandAlias);

public:
	virtual qs::QSTATUS setEditMessage(
		EditMessage* pEditMessage, bool bReset);
	virtual qs::QSTATUS updateEditMessage(EditMessage* pEditMessage);
	virtual void releaseEditMessage(EditMessage* pEditMessage);
	virtual bool hasInitialFocus() const;
	virtual bool isFocusItem() const;

public:
	virtual unsigned int getHeight(unsigned int nFontHeight) const;
	virtual qs::QSTATUS create(qs::WindowBase* pParent,
		const std::pair<HFONT, HFONT>& fonts, UINT nId);
	virtual qs::QSTATUS destroy();

protected:
	virtual const TCHAR* getWindowClassName() const;
	virtual UINT getWindowStyle() const;
	virtual UINT getWindowExStyle() const;

public:
	virtual qs::QSTATUS copy();
	virtual qs::QSTATUS canCopy(bool* pbCan);
	virtual qs::QSTATUS cut();
	virtual qs::QSTATUS canCut(bool* pbCan);
	virtual qs::QSTATUS paste();
	virtual qs::QSTATUS canPaste(bool* pbCan);
	virtual qs::QSTATUS selectAll();
	virtual qs::QSTATUS canSelectAll(bool* pbCan);
	virtual qs::QSTATUS undo();
	virtual qs::QSTATUS canUndo(bool* pbCan);

public:
	virtual LRESULT onCommand(WORD nCode, WORD nId);

private:
	LRESULT onKillFocus();

private:
	EditHeaderEditItem(const EditHeaderEditItem&);
	EditHeaderEditItem& operator=(const EditHeaderEditItem&);

private:
	EditMessage* pEditMessage_;
	bool bExpandAlias_;
	AddressBook* pAddressBook_;
	qs::WindowBase* pParent_;
	unsigned int nId_;
};


/****************************************************************************
 *
 * AttachmentHeaderEditItem
 *
 */

class AttachmentHeaderEditItem :
	public HeaderEditItem,
	public DefaultEditMessageHandler
{
public:
	AttachmentHeaderEditItem(EditWindowFocusController* pController,
		qs::QSTATUS* pstatus);
	virtual ~AttachmentHeaderEditItem();

public:
	virtual qs::QSTATUS setEditMessage(
		EditMessage* pEditMessage, bool bReset);
	virtual void releaseEditMessage(EditMessage* pEditMessage);
	virtual qs::QSTATUS updateEditMessage(EditMessage* pEditMessage);
	virtual bool hasFocus() const;
	virtual bool hasInitialFocus() const;
	virtual bool isFocusItem() const;

public:
	virtual unsigned int getHeight(unsigned int nFontHeight) const;
	virtual qs::QSTATUS create(qs::WindowBase* pParent,
		const std::pair<HFONT, HFONT>& fonts, UINT nId);
	virtual qs::QSTATUS destroy();
	virtual qs::QSTATUS layout(const RECT& rect, unsigned int nFontHeight);
	virtual qs::QSTATUS show(bool bShow);

public:
	virtual qs::QSTATUS setFocus();

public:
	virtual qs::QSTATUS attachmentsChanged(const EditMessageEvent& event);

private:
	qs::QSTATUS update(EditMessage* pEditMessage);

private:
	AttachmentHeaderEditItem(const AttachmentHeaderEditItem&);
	AttachmentHeaderEditItem& operator=(const AttachmentHeaderEditItem&);

private:
	HWND hwnd_;
	EditWindowItemWindow* pItemWindow_;
	EditMessage* pEditMessage_;
};


/****************************************************************************
 *
 * ComboBoxHeaderEditItem
 *
 */

class ComboBoxHeaderEditItem :
	public HeaderEditItem,
	public qs::DefaultCommandHandler
{
public:
	ComboBoxHeaderEditItem(EditWindowFocusController* pController,
		qs::QSTATUS* pstatus);
	virtual ~ComboBoxHeaderEditItem();

public:
	virtual void releaseEditMessage(EditMessage* pEditMessage);
	virtual bool hasFocus() const;
	virtual bool hasInitialFocus() const;
	virtual bool isFocusItem() const;

public:
	virtual unsigned int getHeight(unsigned int nFontHeight) const;
	virtual qs::QSTATUS create(qs::WindowBase* pParent,
		const std::pair<HFONT, HFONT>& fonts, UINT nId);
	virtual qs::QSTATUS destroy();
	virtual qs::QSTATUS layout(const RECT& rect, unsigned int nFontHeight);
	virtual qs::QSTATUS show(bool bShow);

public:
	virtual qs::QSTATUS setFocus();

public:
	virtual LRESULT onCommand(WORD nCode, WORD nId);

protected:
	virtual LRESULT onChange();

protected:
	HWND getHandle() const;

private:
	ComboBoxHeaderEditItem(const ComboBoxHeaderEditItem&);
	ComboBoxHeaderEditItem& operator=(const ComboBoxHeaderEditItem&);

private:
	HWND hwnd_;
	EditWindowItemWindow* pItemWindow_;
	qs::WindowBase* pParent_;
	UINT nId_;
};


/****************************************************************************
 *
 * SignatureHeaderEditItem
 *
 */

class SignatureHeaderEditItem :
	public ComboBoxHeaderEditItem,
	public DefaultEditMessageHandler
{
public:
	SignatureHeaderEditItem(EditWindowFocusController* pController,
		qs::QSTATUS* pstatus);
	virtual ~SignatureHeaderEditItem();

public:
	virtual qs::QSTATUS setEditMessage(
		EditMessage* pEditMessage, bool bReset);
	virtual void releaseEditMessage(EditMessage* pEditMessage);
	virtual qs::QSTATUS updateEditMessage(EditMessage* pEditMessage);

public:
	virtual qs::QSTATUS accountChanged(const EditMessageEvent& event);
	virtual qs::QSTATUS signatureChanged(const EditMessageEvent& event);

protected:
	virtual LRESULT onChange();

private:
	qs::QSTATUS update(EditMessage* pEditMessage);

private:
	SignatureHeaderEditItem(const SignatureHeaderEditItem&);
	SignatureHeaderEditItem& operator=(const SignatureHeaderEditItem&);

private:
	EditMessage* pEditMessage_;
};


/****************************************************************************
 *
 * AccountHeaderEditItem
 *
 */

class AccountHeaderEditItem :
	public ComboBoxHeaderEditItem,
	public DefaultEditMessageHandler
{
public:
	AccountHeaderEditItem(EditWindowFocusController* pController,
		qs::QSTATUS* pstatus);
	virtual ~AccountHeaderEditItem();

public:
	virtual qs::QSTATUS setEditMessage(
		EditMessage* pEditMessage, bool bReset);
	virtual qs::QSTATUS updateEditMessage(EditMessage* pEditMessage);
	virtual void releaseEditMessage(EditMessage* pEditMessage);

public:
	virtual qs::QSTATUS accountChanged(const EditMessageEvent& event);

protected:
	virtual LRESULT onChange();

private:
	AccountHeaderEditItem(const AccountHeaderEditItem&);
	AccountHeaderEditItem& operator=(const AccountHeaderEditItem&);

private:
	EditMessage* pEditMessage_;
};


/****************************************************************************
 *
 * HeaderEditWindowContentHandler
 *
 */

class HeaderEditWindowContentHandler : public qs::DefaultHandler
{
public:
	HeaderEditWindowContentHandler(LineLayout* pLayout,
		EditWindowFocusController* pController,
		HeaderEditLineCallback* pCallback, qs::QSTATUS* pstatus);
	virtual ~HeaderEditWindowContentHandler();

public:
	virtual qs::QSTATUS startElement(const WCHAR* pwszNamespaceURI,
		const WCHAR* pwszLocalName, const WCHAR* pwszQName,
		const qs::Attributes& attributes);
	virtual qs::QSTATUS endElement(const WCHAR* pwszNamespaceURI,
		const WCHAR* pwszLocalName, const WCHAR* pwszQName);
	virtual qs::QSTATUS characters(const WCHAR* pwsz,
		size_t nStart, size_t nLength);

private:
	HeaderEditWindowContentHandler(const HeaderEditWindowContentHandler&);
	HeaderEditWindowContentHandler& operator=(const HeaderEditWindowContentHandler&);

private:
	enum State {
		STATE_ROOT,
		STATE_HEADEREDIT,
		STATE_LINE,
		STATE_ITEM
	};

private:
	LineLayout* pLayout_;
	EditWindowFocusController* pController_;
	HeaderEditLineCallback* pCallback_;
	HeaderEditLine* pCurrentLine_;
	HeaderEditItem* pCurrentItem_;
	State state_;
};


/****************************************************************************
 *
 * HeaderEditWindowCreateContext
 *
 */

struct HeaderEditWindowCreateContext
{
	EditWindowFocusController* pController_;
	HeaderEditLineCallback* pHeaderEditLineCallback_;
};

}

#endif // __HEADEREDITWINDOW_H__
