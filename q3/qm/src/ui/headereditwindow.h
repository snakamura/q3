/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __HEADEREDITWINDOW_H__
#define __HEADEREDITWINDOW_H__

#include <qsregex.h>
#include <qssax.h>

#include <vector>

#include "autocomplete.h"
#include "editwindow.h"
#include "layout.h"
#include "../model/editmessage.h"
#include "../uimodel/attachmentselectionmodel.h"


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
class HeaderEditItemCallback;
class HeaderEditWindowContentHandler;

class AddressBook;
class AddressBookAddress;
class AddressBookEntry;
class RecentAddress;


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
	HeaderEditLine(HeaderEditLineCallback* pCallback,
				   unsigned int nFlags);
	~HeaderEditLine();

public:
	void setEditMessage(EditMessage* pEditMessage,
						bool bReset);
	EditWindowItem* getNextFocusItem(EditWindowItem** ppItem) const;
	EditWindowItem* getPrevFocusItem(EditWindowItem** ppItem) const;
	HeaderEditItem* getFocusedItem() const;
	HeaderEditItem* getInitialFocusItem() const;

public:
	virtual bool isHidden() const;

private:
	HeaderEditLine(const HeaderEditLine&);
	HeaderEditLine& operator=(const HeaderEditLine&);

private:
	HeaderEditLineCallback* pCallback_;
	unsigned int nFlags_;
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

class HeaderEditItem :
	public LineLayoutItem,
	public EditWindowItem
{
protected:
	enum InitialFocus {
		INITIALFOCUS_NONE,
		INITIALFOCUS_TRUE,
		INITIALFOCUS_FALSE
	};

protected:
	explicit HeaderEditItem(EditWindowFocusController* pController);

public:
	virtual ~HeaderEditItem();

public:
	unsigned int getNumber() const;

public:
	virtual void setEditMessage(EditMessage* pEditMessage,
								bool bReset) = 0;
	virtual void releaseEditMessage(EditMessage* pEditMessage) = 0;
	virtual void updateEditMessage(EditMessage* pEditMessage) = 0;
	virtual bool hasFocus() const = 0;
	virtual bool hasInitialFocus() const = 0;
	virtual bool isFocusItem() const = 0;

public:
	void setNumber(unsigned int nNumber);
	void setInitialFocus(bool bInitialFocus);
	void setValue(const WCHAR* pwszValue);

protected:
	EditWindowFocusController* getController() const;
	InitialFocus getInitialFocus() const;
	const WCHAR* getValue() const;

public:
	virtual void copy();
	virtual bool canCopy();
	virtual void cut();
	virtual bool canCut();
	virtual void paste();
	virtual bool canPaste();
	virtual void selectAll();
	virtual bool canSelectAll();
	virtual void undo();
	virtual bool canUndo();
	virtual void redo();
	virtual bool canRedo();

private:
	HeaderEditItem(const HeaderEditItem&);
	HeaderEditItem& operator=(const HeaderEditItem&);

private:
	EditWindowFocusController* pController_;
	unsigned int nNumber_;
	InitialFocus initialFocus_;
	qs::wstring_ptr wstrValue_;
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
	
	enum Align {
		ALIGN_LEFT,
		ALIGN_CENTER,
		ALIGN_RIGHT
	};
	
	enum Type {
		TYPE_UNSTRUCTURED,
		TYPE_ADDRESSLIST,
		TYPE_REFERENCES
	};

protected:
	explicit TextHeaderEditItem(EditWindowFocusController* pController);

public:
	virtual ~TextHeaderEditItem();

public:
	virtual void setEditMessage(EditMessage* pEditMessage,
								bool bReset);
	virtual void releaseEditMessage(EditMessage* pEditMessage);
	virtual bool hasFocus() const;

public:
	void setStyle(unsigned int nStyle);
	void setAlign(Align align);
	void setField(const WCHAR* pwszField);
	void setType(Type type);

public:
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
	virtual void fieldChanged(const EditMessageFieldEvent& event);

protected:
	HWND getHandle() const;
	Align getAlign() const;
	const WCHAR* getField() const;
	Type getType() const;

protected:
	virtual const TCHAR* getWindowClassName() const = 0;
	virtual UINT getWindowStyle() const = 0;
	virtual UINT getWindowExStyle() const = 0;

public:
	virtual void setFocus();

public:
	static unsigned int parseStyle(const WCHAR* pwszStyle);
	static Align parseAlign(const WCHAR* pwszAlign);
	static Type parseType(const WCHAR* pwszType);

private:
	TextHeaderEditItem(const TextHeaderEditItem&);
	TextHeaderEditItem& operator=(const TextHeaderEditItem&);

private:
	unsigned int nStyle_;
	Align align_;
	qs::wstring_ptr wstrField_;
	Type type_;
	HWND hwnd_;
	std::auto_ptr<EditWindowItemWindow> pItemWindow_;
};


/****************************************************************************
 *
 * StaticHeaderEditItem
 *
 */

class StaticHeaderEditItem : public TextHeaderEditItem
{
public:
	explicit StaticHeaderEditItem(EditWindowFocusController* pController);
	virtual ~StaticHeaderEditItem();

public:
	virtual void updateEditMessage(EditMessage* pEditMessage);
	virtual bool hasInitialFocus() const;
	virtual bool isFocusItem() const;

public:
	virtual unsigned int getHeight(unsigned int nWidth,
								   unsigned int nFontHeight) const;
	virtual unsigned int getPreferredWidth() const;

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
	public qs::CommandHandler
{
public:
	EditHeaderEditItem(EditWindowFocusController* pController,
					   qs::Profile* pProfile);
	virtual ~EditHeaderEditItem();

public:
	virtual void setEditMessage(EditMessage* pEditMessage,
								bool bReset);
	virtual void updateEditMessage(EditMessage* pEditMessage);
	virtual void releaseEditMessage(EditMessage* pEditMessage);
	virtual bool hasInitialFocus() const;
	virtual bool isFocusItem() const;

public:
	virtual unsigned int getHeight(unsigned int nWidth,
								   unsigned int nFontHeight) const;
	virtual bool create(qs::WindowBase* pParent,
						const std::pair<HFONT, HFONT>& fonts,
						UINT nId,
						void* pParam);
	virtual void destroy();

protected:
	virtual const TCHAR* getWindowClassName() const;
	virtual UINT getWindowStyle() const;
	virtual UINT getWindowExStyle() const;

public:
	virtual void copy();
	virtual bool canCopy();
	virtual void cut();
	virtual bool canCut();
	virtual void paste();
	virtual bool canPaste();
	virtual void selectAll();
	virtual bool canSelectAll();
	virtual void undo();
	virtual bool canUndo();

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

protected:
	virtual LRESULT onKillFocus();

private:
	EditHeaderEditItem(const EditHeaderEditItem&);
	EditHeaderEditItem& operator=(const EditHeaderEditItem&);

private:
	qs::Profile* pProfile_;
	EditMessage* pEditMessage_;
	qs::WindowBase* pParent_;
	unsigned int nId_;
	std::auto_ptr<qs::ImeWindow> pImeWindow_;
};


/****************************************************************************
 *
 * AddressHeaderEditItem
 *
 */

class AddressHeaderEditItem :
	public EditHeaderEditItem,
	private AutoCompleteCallback
{
public:
	enum Flag {
		FLAG_EXPANDALIAS	= 0x01,
		FLAG_AUTOCOMPLETE	= 0x02
	};

public:
	AddressHeaderEditItem(EditWindowFocusController* pController,
						  qs::Profile* pProfile);
	virtual ~AddressHeaderEditItem();

public:
	void setExpandAlias(bool bExpandAlias);
	void setAutoComplete(bool bAutoComplete);

public:
	virtual void setEditMessage(EditMessage* pEditMessage,
								bool bReset);
	virtual void releaseEditMessage(EditMessage* pEditMessage);

public:
	virtual bool create(qs::WindowBase* pParent,
						const std::pair<HFONT, HFONT>& fonts,
						UINT nId,
						void* pParam);
	virtual void destroy();

protected:
	virtual LRESULT onKillFocus();

public:
	virtual std::pair<size_t, size_t> getInput(const WCHAR* pwszText,
											   size_t nCaret);
	virtual void getCandidates(const WCHAR* pwszInput,
							   CandidateList* pList);

private:
	static void getCandidates(const WCHAR* pwszInput,
							  const AddressBook* pAddressBook,
							  CandidateList* pList);
	static void getCandidates(const WCHAR* pwszInput,
							  const AddressBookEntry* pEntry,
							  CandidateList* pList);
	static bool isMatch(const WCHAR* pwszInput,
						const AddressBookAddress* pAddress);
	static void getCandidates(const WCHAR* pwszInput,
							  const RecentAddress* pRecentAddress,
							  const AddressBook* pAddressBook,
							  CandidateList* pList);
	static bool match(const WCHAR* pwszInput,
					  size_t nLen,
					  const AddressBookAddress* pAddress);
	static bool match(const WCHAR* pwszInput,
					  size_t nLen,
					  const qs::AddressListParser& addressList);
	static bool match(const WCHAR* pwszInput,
					  size_t nLen,
					  const qs::AddressParser& address);
	static bool matchName(const WCHAR* pwszName,
						  const WCHAR* pwszInput,
						  size_t nInputLen);

private:
	AddressHeaderEditItem(const AddressHeaderEditItem&);
	AddressHeaderEditItem& operator=(const AddressHeaderEditItem&);

private:
	unsigned int nFlags_;
	AddressBook* pAddressBook_;
	RecentAddress* pRecentAddress_;
	std::auto_ptr<AutoComplete> pAutoComplete_;
};


/****************************************************************************
 *
 * AttachmentHeaderEditItem
 *
 */

class AttachmentHeaderEditItem :
	public HeaderEditItem,
	public DefaultEditMessageHandler,
	public AttachmentSelectionModel
{
public:
	AttachmentHeaderEditItem(EditWindowFocusController* pController,
							 qs::MenuManager* pMenuManager,
							 HeaderEditItemCallback* pCallback);
	virtual ~AttachmentHeaderEditItem();

public:
	virtual void setEditMessage(EditMessage* pEditMessage,
								bool bReset);
	virtual void releaseEditMessage(EditMessage* pEditMessage);
	virtual void updateEditMessage(EditMessage* pEditMessage);
	virtual bool hasFocus() const;
	virtual bool hasInitialFocus() const;
	virtual bool isFocusItem() const;

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
	virtual void setFocus();

public:
	virtual void paste();
	virtual bool canPaste();
	virtual void selectAll();
	virtual bool canSelectAll();

public:
	virtual void attachmentsChanged(const EditMessageEvent& event);

public:
	virtual bool hasAttachment();
	virtual bool hasSelectedAttachment();
	virtual void getSelectedAttachment(NameList* pList);

private:
	void update(EditMessage* pEditMessage);
	void clear();

private:
	AttachmentHeaderEditItem(const AttachmentHeaderEditItem&);
	AttachmentHeaderEditItem& operator=(const AttachmentHeaderEditItem&);

private:
	class AttachmentEditWindow :
		public qs::WindowBase,
		public qs::DefaultWindowHandler
	{
	public:
		explicit AttachmentEditWindow(AttachmentHeaderEditItem* pItem);
		virtual ~AttachmentEditWindow();
	
	public:
		virtual LRESULT windowProc(UINT uMsg,
								   WPARAM wParam,
								   LPARAM lParam);
	
	protected:
		LRESULT onContextMenu(HWND hwnd,
							  const POINT& pt);
		LRESULT onLButtonDown(UINT nFlags,
							  const POINT& pt);
	
	private:
		AttachmentEditWindow(const AttachmentEditWindow&);
		AttachmentEditWindow& operator=(const AttachmentEditWindow&);
	
	private:
		AttachmentHeaderEditItem* pItem_;
	};
	friend class AttachmentEditWindow;

private:
	AttachmentEditWindow wnd_;
	qs::MenuManager* pMenuManager_;
	HeaderEditItemCallback* pCallback_;
	std::auto_ptr<EditWindowItemWindow> pItemWindow_;
	EditMessage* pEditMessage_;
};


/****************************************************************************
 *
 * ComboBoxHeaderEditItem
 *
 */

class ComboBoxHeaderEditItem :
	public HeaderEditItem,
	public qs::CommandHandler
{
public:
	explicit ComboBoxHeaderEditItem(EditWindowFocusController* pController);
	virtual ~ComboBoxHeaderEditItem();

public:
	virtual void releaseEditMessage(EditMessage* pEditMessage);
	virtual bool hasFocus() const;
	virtual bool hasInitialFocus() const;
	virtual bool isFocusItem() const;

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
	virtual void setFocus();

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

protected:
	virtual LRESULT onChange();

protected:
	HWND getHandle() const;

#ifdef _WIN32_WCE
private:
	int calcItemHeight(HFONT hfont);
#endif

#ifdef _WIN32_WCE
private:
	class ComboBoxEditWindow :
		public qs::WindowBase,
		public qs::DefaultWindowHandler
	{
	public:
		ComboBoxEditWindow(HWND hwnd,
						   int nItemHeight);
		virtual ~ComboBoxEditWindow();
	
	public:
		void setItemHeight(int nItemHeight);
	
	public:
		virtual LRESULT windowProc(UINT uMsg,
								   WPARAM wParam,
								   LPARAM lParam);
	
	protected:
		LRESULT onWindowPosChanged(WINDOWPOS* pWindowPos);
	
	private:
		ComboBoxEditWindow(const ComboBoxEditWindow&);
		ComboBoxEditWindow& operator=(const ComboBoxEditWindow&);
	
	private:
		int nItemHeight_;
	};
#endif

private:
	ComboBoxHeaderEditItem(const ComboBoxHeaderEditItem&);
	ComboBoxHeaderEditItem& operator=(const ComboBoxHeaderEditItem&);

private:
	HWND hwnd_;
	std::auto_ptr<EditWindowItemWindow> pItemWindow_;
	qs::WindowBase* pParent_;
	UINT nId_;
#ifdef _WIN32_WCE
	ComboBoxEditWindow* pComboBoxEditWindow_;
#endif
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
	explicit SignatureHeaderEditItem(EditWindowFocusController* pController);
	virtual ~SignatureHeaderEditItem();

public:
	virtual void setEditMessage(EditMessage* pEditMessage,
								bool bReset);
	virtual void releaseEditMessage(EditMessage* pEditMessage);
	virtual void updateEditMessage(EditMessage* pEditMessage);

public:
	virtual void accountChanged(const EditMessageEvent& event);
	virtual void signatureChanged(const EditMessageEvent& event);

protected:
	virtual LRESULT onChange();

private:
	void update(EditMessage* pEditMessage);

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
	explicit AccountHeaderEditItem(EditWindowFocusController* pController);
	virtual ~AccountHeaderEditItem();

public:
	virtual void setEditMessage(EditMessage* pEditMessage,
								bool bReset);
	virtual void updateEditMessage(EditMessage* pEditMessage);
	virtual void releaseEditMessage(EditMessage* pEditMessage);

public:
	void setShowFrom(bool bShow);

public:
	virtual void accountChanged(const EditMessageEvent& event);

protected:
	virtual LRESULT onChange();

private:
	AccountHeaderEditItem(const AccountHeaderEditItem&);
	AccountHeaderEditItem& operator=(const AccountHeaderEditItem&);

private:
	bool bShowFrom_;
	EditMessage* pEditMessage_;
};


/****************************************************************************
 *
 * HeaderEditItemCallback
 *
 */

class HeaderEditItemCallback
{
public:
	virtual ~HeaderEditItemCallback();

public:
	virtual void itemSizeChanged() = 0;
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
								   const WCHAR* pwszClass,
								   EditWindowFocusController* pController,
								   qs::MenuManager* pMenuManager,
								   qs::Profile* pProfile,
								   HeaderEditLineCallback* pLineCallback,
								   HeaderEditItemCallback* pItemCallback);
	virtual ~HeaderEditWindowContentHandler();

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
	static void setNumber(HeaderEditItem* pItem,
						  const WCHAR* pwszNumber);
	
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
	const WCHAR* pwszClass_;
	EditWindowFocusController* pController_;
	qs::MenuManager* pMenuManager_;
	qs::Profile* pProfile_;
	HeaderEditLineCallback* pLineCallback_;
	HeaderEditItemCallback* pItemCallback_;
	HeaderEditLine* pCurrentLine_;
	HeaderEditItem* pCurrentItem_;
	State state_;
	bool bIgnore_;
	AttachmentSelectionModel* pAttachmentSelectionModel_;
	qs::StringBuffer<qs::WSTRING> buffer_;
};


/****************************************************************************
 *
 * HeaderEditWindowCreateContext
 *
 */

struct HeaderEditWindowCreateContext
{
	EditWindowFocusController* pController_;
	const WCHAR* pwszClass_;
	qs::MenuManager* pMenuManager_;
	HeaderEditLineCallback* pHeaderEditLineCallback_;
	HeaderEditItemCallback* pHeaderEditItemCallback_;
};

}

#endif // __HEADEREDITWINDOW_H__
