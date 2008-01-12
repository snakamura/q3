/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
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
	class AbstractComboBoxHeaderEditItem;
		class ComboBoxHeaderEditItem;
		class SignatureHeaderEditItem;
		class AccountHeaderEditItem;
	class CheckBoxHeaderEditItem;
class StyledHeaderEditItem;
class FieldHeaderEditItem;
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
	EditWindowItem* getItemByNumber(unsigned int nNumber) const;
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
	
	enum {
		DEFAULT_MARGIN = 7
	};

protected:
	explicit HeaderEditItem(qs::KeyMap* pKeyMap);

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
	qs::KeyMap* getKeyMap() const;
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
	qs::KeyMap* pKeyMap_;
	unsigned int nNumber_;
	InitialFocus initialFocus_;
	qs::wstring_ptr wstrValue_;
};


/****************************************************************************
 *
 * StyledHeaderEditItem
 *
 */

class StyledHeaderEditItem
{
public:
	enum Style {
		STYLE_NORMAL	= 0x00,
		STYLE_BOLD		= 0x01,
		STYLE_ITALIC	= 0x02
	};

protected:
	StyledHeaderEditItem();
	~StyledHeaderEditItem();

public:
	unsigned int getStyle() const;
	void setStyle(unsigned int nStyle);

public:
	static unsigned int parseStyle(const WCHAR* pwszStyle);

private:
	unsigned int nStyle_;
};


/****************************************************************************
 *
 * FieldHeaderEditItem
 *
 */

class FieldHeaderEditItem : public DefaultEditMessageHandler
{
protected:
	FieldHeaderEditItem();
	virtual ~FieldHeaderEditItem();

public:
	const WCHAR* getField() const;
	void setField(const WCHAR* pwszField);

public:
	virtual void fieldChanged(const EditMessageFieldEvent& event);

protected:
	void requestNotify(EditMessage* pEditMessage);
	void revokeNotify(EditMessage* pEditMessage);

protected:
	virtual void fieldChanged(const WCHAR* pwszValue) = 0;

private:
	FieldHeaderEditItem(const FieldHeaderEditItem&);
	FieldHeaderEditItem& operator=(const FieldHeaderEditItem&);

private:
	qs::wstring_ptr wstrField_;
};


/****************************************************************************
 *
 * TextHeaderEditItem
 *
 */

class TextHeaderEditItem :
	public HeaderEditItem,
	public StyledHeaderEditItem,
	public FieldHeaderEditItem
{
public:
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
	explicit TextHeaderEditItem(qs::KeyMap* pKeyMap);

public:
	virtual ~TextHeaderEditItem();

public:
	virtual void setEditMessage(EditMessage* pEditMessage,
								bool bReset);
	virtual void releaseEditMessage(EditMessage* pEditMessage);
	virtual bool hasFocus() const;

public:
	void setAlign(Align align);
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

protected:
	virtual void fieldChanged(const WCHAR* pwszValue);

protected:
	HWND getHandle() const;
	Align getAlign() const;
	Type getType() const;

protected:
	virtual const TCHAR* getWindowClassName() const = 0;
	virtual UINT getWindowStyle() const = 0;
	virtual UINT getWindowExStyle() const = 0;
	virtual int getTopOffset(const RECT& rect,
							 unsigned int nFontHeight) const = 0;

public:
	virtual void setFocus();

public:
	static Align parseAlign(const WCHAR* pwszAlign);
	static Type parseType(const WCHAR* pwszType);

private:
	TextHeaderEditItem(const TextHeaderEditItem&);
	TextHeaderEditItem& operator=(const TextHeaderEditItem&);

private:
	Align align_;
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
	explicit StaticHeaderEditItem(qs::KeyMap* pKeyMap);
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
	virtual int getTopOffset(const RECT& rect,
							 unsigned int nFontHeight) const;

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
	EditHeaderEditItem(qs::KeyMap* pKeyMap,
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
	virtual int getTopOffset(const RECT& rect,
							 unsigned int nFontHeight) const;

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
	AddressHeaderEditItem(qs::KeyMap* pKeyMap,
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
	virtual void removeCandidate(const WCHAR* pwszCandidate);

private:
	static void getCandidates(const WCHAR* pwszInput,
							  size_t nInputLen,
							  const WCHAR* pwszDomain,
							  size_t nDomainLen,
							  const AddressBook* pAddressBook,
							  CandidateList* pList);
	static void getCandidates(const WCHAR* pwszInput,
							  size_t nInputLen,
							  const WCHAR* pwszDomain,
							  size_t nDomainLen,
							  const AddressBookEntry* pEntry,
							  CandidateList* pList);
	static void getCandidates(const WCHAR* pwszInput,
							  size_t nInputLen,
							  const WCHAR* pwszDomain,
							  size_t nDomainLen,
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
	static bool matchAddress(const WCHAR* pwszAddress,
							 const WCHAR* pwszInput,
							 size_t nInputLen);
	static const WCHAR* getDomain(const WCHAR* pwszInput);
	static void addDomainCandidate(const WCHAR* pwszInput,
								   const WCHAR* pwszDomain,
								   size_t nDomainLen,
								   const WCHAR* pwszHost,
								   CandidateList* pList);

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
	AttachmentHeaderEditItem(qs::KeyMap* pKeyMap,
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
	virtual bool isAttachmentDeleted();

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
		LRESULT onSize(UINT nFlags,
					   int cx,
					   int cy);
	
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
 * AbstractComboBoxHeaderEditItem
 *
 */

class AbstractComboBoxHeaderEditItem :
	public HeaderEditItem,
	public qs::CommandHandler
{
public:
	explicit AbstractComboBoxHeaderEditItem(qs::KeyMap* pKeyMap);
	virtual ~AbstractComboBoxHeaderEditItem();

public:
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
	virtual void postCreate();

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
	AbstractComboBoxHeaderEditItem(const AbstractComboBoxHeaderEditItem&);
	AbstractComboBoxHeaderEditItem& operator=(const AbstractComboBoxHeaderEditItem&);

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
 * ComboBoxHeaderEditItem
 *
 */

class ComboBoxHeaderEditItem :
	public AbstractComboBoxHeaderEditItem,
	public FieldHeaderEditItem
{
public:
	explicit ComboBoxHeaderEditItem(qs::KeyMap* pKeyMap);
	virtual ~ComboBoxHeaderEditItem();

public:
	virtual void setEditMessage(EditMessage* pEditMessage,
								bool bReset);
	virtual void releaseEditMessage(EditMessage* pEditMessage);
	virtual void updateEditMessage(EditMessage* pEditMessage);

public:
	void addOption(const WCHAR* pwszOption);

protected:
	virtual void fieldChanged(const WCHAR* pwszValue);

protected:
	virtual void postCreate();

private:
	ComboBoxHeaderEditItem(const ComboBoxHeaderEditItem&);
	ComboBoxHeaderEditItem& operator=(const ComboBoxHeaderEditItem&);

private:
	typedef std::vector<qs::WSTRING> OptionList;

private:
	OptionList listOption_;
};


/****************************************************************************
 *
 * SignatureHeaderEditItem
 *
 */

class SignatureHeaderEditItem :
	public AbstractComboBoxHeaderEditItem,
	public DefaultEditMessageHandler
{
public:
	explicit SignatureHeaderEditItem(qs::KeyMap* pKeyMap);
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
	public AbstractComboBoxHeaderEditItem,
	public DefaultEditMessageHandler
{
public:
	explicit AccountHeaderEditItem(qs::KeyMap* pKeyMap);
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
 * CheckBoxHeaderEditItem
 *
 */

class CheckBoxHeaderEditItem :
	public HeaderEditItem,
	public StyledHeaderEditItem,
	public FieldHeaderEditItem
{
public:
	explicit CheckBoxHeaderEditItem(qs::KeyMap* pKeyMap);
	virtual ~CheckBoxHeaderEditItem();

public:
	virtual void setEditMessage(EditMessage* pEditMessage,
								bool bReset);
	virtual void releaseEditMessage(EditMessage* pEditMessage);
	virtual void updateEditMessage(EditMessage* pEditMessage);
	virtual bool hasFocus() const;
	virtual bool hasInitialFocus() const;
	virtual bool isFocusItem() const;

public:
	void setValue(const WCHAR* pwszValue);

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

protected:
	virtual void fieldChanged(const WCHAR* pwszValue);

public:
	virtual void setFocus();

private:
	CheckBoxHeaderEditItem(const CheckBoxHeaderEditItem&);
	CheckBoxHeaderEditItem& operator=(const CheckBoxHeaderEditItem&);

private:
	qs::wstring_ptr wstrValue_;
	HWND hwnd_;
	std::auto_ptr<EditWindowItemWindow> pItemWindow_;
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
								   UIManager* pUIManager,
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
		STATE_ITEM,
		STATE_COMBOBOX,
		STATE_OPTION
	};

private:
	LineLayout* pLayout_;
	const WCHAR* pwszClass_;
	UIManager* pUIManager_;
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
	const WCHAR* pwszClass_;
	UIManager* pUIManager_;
	HeaderEditLineCallback* pHeaderEditLineCallback_;
	HeaderEditItemCallback* pHeaderEditItemCallback_;
};

}

#endif // __HEADEREDITWINDOW_H__
