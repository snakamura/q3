/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __VIEWMODEL_H__
#define __VIEWMODEL_H__

#include <qm.h>
#include <qmaccount.h>
#include <qmfolder.h>
#include <qmmacro.h>

#include <qs.h>
#include <qsprofile.h>
#include <qssax.h>
#include <qsthread.h>
#include <qsutil.h>

#include <vector>

#include "foldermodel.h"


namespace qm {

class ViewColumn;
class ViewModel;
class ViewModelHandler;
class ViewModelEvent;
class ViewModelFolderComp;
class ViewModelHolder;
class ViewModelManager;
class ViewModelManagerHandler;
class ViewModelManagerEvent;
class ViewData;
class DefaultViewData;
class ViewDataItem;
class ViewDataContentHandler;
class ViewDataWriter;

class ColorManager;
class ColorSet;
class Document;
class Filter;
class FilterManager;
class Folder;
class MessageHolder;
class Macro;
class SecurityModel;
class UIManager;


/****************************************************************************
 *
 * ViewColumn
 *
 */

class ViewColumn
{
public:
	enum Type {
		TYPE_NONE,
		TYPE_ID,
		TYPE_DATE,
		TYPE_FROM,
		TYPE_TO,
		TYPE_FROMTO,
		TYPE_SUBJECT,
		TYPE_SIZE,
		TYPE_FLAGS,
		TYPE_OTHER
	};
	
	enum Flag {
		FLAG_INDENT			= 0x0001,
		FLAG_LINE			= 0x0002,
		FLAG_RIGHTALIGN		= 0x0004,
		FLAG_ICON			= 0x0008,
		
		FLAG_SORT_TEXT		= 0x0100,
		FLAG_SORT_NUMBER	= 0x0200,
		FLAG_SORT_DATE		= 0x0300,
		FLAG_SORT_MASK		= 0x0f00
	};

public:
	ViewColumn(const WCHAR* pwszTitle,
			   Type type,
			   std::auto_ptr<Macro> pMacro,
			   unsigned int nFlags,
			   unsigned int nWidth);
	~ViewColumn();

public:
	const WCHAR* getTitle() const;
	Type getType() const;
	const Macro* getMacro() const;
	unsigned int getFlags() const;
	bool isFlag(Flag flag) const;
	unsigned int getWidth() const;
	void setWidth(unsigned int nWidth);
	void set(const WCHAR* pwszTitle,
			 Type type,
			 std::auto_ptr<Macro> pMacro,
			 unsigned int nFlags,
			 unsigned int nWidth);
	std::auto_ptr<ViewColumn> clone() const;

public:
	qs::wstring_ptr getText(const ViewModel* pViewModel,
							MessageHolder* pmh) const;
	unsigned int getNumber(const ViewModel* pViewModel,
						   MessageHolder* pmh) const;
	void getTime(const ViewModel* pViewModel,
				 MessageHolder* pmh,
				 qs::Time* pTime) const;

public:
	ViewColumn(const ViewColumn&);
	ViewColumn& operator=(const ViewColumn&);

private:
	qs::wstring_ptr wstrTitle_;
	Type type_;
	std::auto_ptr<Macro> pMacro_;
	unsigned int nFlags_;
	unsigned int nWidth_;
};

typedef std::vector<ViewColumn*> ViewColumnList;


/****************************************************************************
 *
 * ViewModelHandler
 *
 */

class ViewModelHandler
{
public:
	virtual ~ViewModelHandler();

public:
	virtual void itemAdded(const ViewModelEvent& event) = 0;
	virtual void itemRemoved(const ViewModelEvent& event) = 0;
	virtual void itemChanged(const ViewModelEvent& event) = 0;
	virtual void itemStateChanged(const ViewModelEvent& event) = 0;
	virtual void itemAttentionPaid(const ViewModelEvent& event) = 0;
	virtual void updated(const ViewModelEvent& event) = 0;
	virtual void sorted(const ViewModelEvent& event) = 0;
	virtual void columnChanged(const ViewModelEvent& event) = 0;
	virtual void destroyed(const ViewModelEvent& event) = 0;
};


/****************************************************************************
 *
 * ViewModelItem
 *
 */

class ViewModelItem
{
public:
	enum Flag {
		FLAG_SELECTED	= 0x01,
		FLAG_FOCUSED	= 0x02
	};

public:
	ViewModelItem(MessageHolder* pmh);
	ViewModelItem(unsigned int nMessageIdHash);
	~ViewModelItem();

public:
	void* operator new(size_t n);
	void operator delete(void* p);

public:
	MessageHolder* getMessageHolder() const;
	ViewModelItem* getParentItem() const;
	void setParentItem(ViewModelItem* pParentItem);
	bool isFlag(Flag flag) const;
	unsigned int getFlags() const;
	void setFlags(unsigned int nFlags,
				  unsigned int nMask);
	COLORREF getColor() const;
	void setColor(COLORREF cr);
	unsigned int getMessageFlags() const;
	void setMessageFlags(unsigned int nFlags);
	unsigned int getLevel() const;
	unsigned int getMessageIdHash() const;

private:
	ViewModelItem(const ViewModelItem&);
	ViewModelItem& operator=(const ViewModelItem&);

private:
	MessageHolder* pmh_;
	ViewModelItem* pParentItem_;
	unsigned int nFlags_;
	COLORREF cr_;
	unsigned int nMessageFlags_;
};


/****************************************************************************
 *
 * ViewModel
 *
 */

class ViewModel :
	public FolderHandler,
	public MessageHolderHandler
{
public:
	enum Sort {
		SORT_INDEX_MASK		= 0x000000ff,
		
		SORT_ASCENDING		= 0x00000100,
		SORT_DESCENDING		= 0x00000200,
		SORT_DIRECTION_MASK	= 0x0000ff00,
		
		SORT_THREAD			= 0x00010000,
		SORT_NOTHREAD		= 0x00020000,
		SORT_THREAD_MASK	= 0x00030000
	};

public:
	typedef std::vector<ViewModelItem*> ItemList;

public:
	ViewModel(ViewModelManager* pViewModelManager,
			  Folder* pFolder,
			  ViewDataItem* pDataItem,
			  const Filter* pFilter,
			  qs::Profile* pProfile,
			  Document* pDocument,
			  HWND hwnd,
			  SecurityModel* pSecurityModel,
			  const ColorManager* pColorManager);
	~ViewModel();

public:
	Folder* getFolder() const;
	
	const ViewColumnList& getColumns() const;
	void setColumns(const ViewColumnList& listColumn);
	unsigned int getColumnCount() const;
	const ViewColumn& getColumn(unsigned int n) const;
	ViewColumn& getColumn(unsigned int n);
	
	unsigned int getCount() const;
	unsigned int getUnseenCount() const;
	const ViewModelItem* getItem(unsigned int n);
	MessageHolder* getMessageHolder(unsigned int n) const;
	unsigned int getIndex(MessageHolder* pmh) const;
	
	void setSort(unsigned int nSort);
	unsigned int getSort() const;
	
	void setFilter(const Filter* pFilter);
	const Filter* getFilter() const;
	
	void addSelection(unsigned int n);
	void addSelection(unsigned int nStart,
					  unsigned int nEnd);
	void removeSelection(unsigned int n);
	void setSelection(unsigned int n);
	void setSelection(unsigned int nStart,
					  unsigned int nEnd);
	void clearSelection();
	void getSelection(MessageHolderList* pList) const;
	bool hasSelection() const;
	unsigned int getSelectedCount() const;
	bool isSelected(unsigned int n) const;
	unsigned int getLastSelection() const;
	void setLastSelection(unsigned int n);
	void setFocused(unsigned int n);
	unsigned int getFocused() const;
	bool isFocused(unsigned int n) const;
	unsigned int getScroll() const;
	void setScroll(unsigned int nScroll);
	
	void payAttention(unsigned int n);
	
	bool save() const;
	
	void addViewModelHandler(ViewModelHandler* pHandler);
	void removeViewModelHandler(ViewModelHandler* pHandler);
	
	void lock() const;
	void unlock() const;
#ifndef NDEBUG
	bool isLocked() const;
#endif

// These methods are intended to be called from ViewColumn.
public:
	MacroValuePtr getValue(const Macro* pMacro,
						   MessageHolder* pmh) const;

public:
	virtual void messageAdded(const FolderMessageEvent& event);
	virtual void messageRemoved(const FolderMessageEvent& event);
	virtual void messageRefreshed(const FolderEvent& event);
	virtual void unseenCountChanged(const FolderEvent& event);
	virtual void folderDestroyed(const FolderEvent& event);

public:
	virtual void messageHolderChanged(const MessageHolderEvent& event);
	virtual void messageHolderDestroyed(const MessageHolderEvent& event);

private:
	void update(bool bRestoreSelection);
	void sort(unsigned int nSort,
			  bool bRestoreSelection,
			  bool bUpdateParentLink);
	void makeParentLink();
	void makeParentLink(const ItemList& listItemSortedByMessageIdHash,
						const ItemList& listItemSortedByPointer,
						ViewModelItem* pItem);

private:
	void fireItemAdded() const;
	void fireItemRemoved() const;
	void fireItemChanged(unsigned int nItem) const;
	void fireItemStateChanged(unsigned int nItem) const;
	void fireItemAttentionPaid(unsigned int nItem) const;
	void fireUpdated() const;
	void fireSorted() const;
	void fireColumnChanged() const;
	void fireDestroyed() const;
	void fireEvent(const ViewModelEvent& event,
				   void (ViewModelHandler::*pfn)(const ViewModelEvent&)) const;

private:
	ViewModel(const ViewModel&);
	ViewModel& operator=(const ViewModel&);

private:
	class SelectionRestorer
	{
	public:
		SelectionRestorer(ViewModel* pViewModel,
						  bool bRefresh,
						  bool bIgnore);
		~SelectionRestorer();
	
	public:
		void restore();
	
	private:
		SelectionRestorer(const SelectionRestorer&);
		SelectionRestorer& operator=(const SelectionRestorer&);
	
	private:
		ViewModel* pViewModel_;
		bool bRefresh_;
		MessageHolder* pmhFocused_;
		MessageHolder* pmhLastSelection_;
	};
	friend class SelectionRestorer;
	
private:
	typedef std::vector<ViewModelHandler*> ViewModelHandlerList;

private:
	ViewModelManager* pViewModelManager_;
	Folder* pFolder_;
	ViewDataItem* pDataItem_;
	qs::Profile* pProfile_;
	Document* pDocument_;
	HWND hwnd_;
	SecurityModel* pSecurityModel_;
	const ColorSet* pColorSet_;
	ItemList listItem_;
	unsigned int nUnseenCount_;
	unsigned int nSort_;
	std::auto_ptr<Filter> pFilter_;
	unsigned int nLastSelection_;
	unsigned int nFocused_;
	unsigned int nScroll_;
	ViewModelHandlerList listHandler_;
#ifndef NDEBUG
	mutable unsigned int nLock_;
#endif
};


/****************************************************************************
 *
 * DefaultViewModelHandler
 *
 */

class DefaultViewModelHandler : public ViewModelHandler
{
public:
	DefaultViewModelHandler();
	virtual ~DefaultViewModelHandler();

public:
	virtual void itemAdded(const ViewModelEvent& event);
	virtual void itemRemoved(const ViewModelEvent& event);
	virtual void itemChanged(const ViewModelEvent& event);
	virtual void itemStateChanged(const ViewModelEvent& event);
	virtual void itemAttentionPaid(const ViewModelEvent& event);
	virtual void updated(const ViewModelEvent& event);
	virtual void sorted(const ViewModelEvent& event);
	virtual void columnChanged(const ViewModelEvent& event);
	virtual void destroyed(const ViewModelEvent& event);
};


/****************************************************************************
 *
 * ViewModelEvent
 *
 */

class ViewModelEvent
{
public:
	ViewModelEvent(const ViewModel* pViewModel);
	ViewModelEvent(const ViewModel* pViewModel,
				   unsigned int nItem);
	~ViewModelEvent();

public:
	const ViewModel* getViewModel() const;
	unsigned int getItem() const;

private:
	ViewModelEvent(const ViewModelEvent&);
	ViewModelEvent& operator=(const ViewModelEvent&);

private:
	const ViewModel* pViewModel_;
	unsigned int nItem_;
};


/****************************************************************************
 *
 * ViewModelFolderComp
 *
 */

class ViewModelFolderComp : public std::unary_function<ViewModel*, bool>
{
public:
	ViewModelFolderComp(Folder* pFolder);
	~ViewModelFolderComp();

public:
	bool operator()(ViewModel* pViewModel) const;

private:
	Folder* pFolder_;
};


/****************************************************************************
 *
 * ViewModelHolder
 *
 */

class ViewModelHolder
{
public:
	virtual ~ViewModelHolder();

public:
	virtual ViewModel* getViewModel() const = 0;
	virtual void setViewModel(ViewModel* pViewModel) = 0;
};


/****************************************************************************
 *
 * ViewModelManager
 *
 */

class ViewModelManager : public DefaultAccountHandler
{
public:
	typedef std::vector<ViewModel*> ViewModelList;

public:
	ViewModelManager(UIManager* pUIManager,
					 Document* pDocument,
					 qs::Profile* pProfile,
					 HWND hwnd,
					 SecurityModel* pSecurityModel);
	~ViewModelManager();

public:
	FilterManager* getFilterManager() const;
	
	Account* getCurrentAccount() const;
	void setCurrentAccount(Account* pAccount);
	void setCurrentFolder(Folder* pFolder);
	ViewModel* getCurrentViewModel() const;
	ViewModel* getViewModel(Folder* pFolder);
	
	bool save() const;
	
	void addViewModelManagerHandler(ViewModelManagerHandler* pHandler);
	void removeViewModelManagerHandler(ViewModelManagerHandler* pHandler);

// These methods are intended to be called from ViewModel class.
public:
	void removeViewModel(ViewModel* pViewModel);

public:
	virtual void accountDestroyed(const AccountEvent& event);

private:
	void setCurrentFolder(Account* pAccount,
						  Folder* pFolder);
	void setCurrentViewModel(ViewModel* pViewModel);
	ViewDataItem* getViewDataItem(Folder* pFolder);
	qs::wstring_ptr getViewsPath(Account* pAccount);

private:
	void fireViewModelSelected(ViewModel* pNewViewModel,
							   ViewModel* pOldViewModel) const;

private:
	ViewModelManager(const ViewModelManager&);
	ViewModelManager& operator=(const ViewModelManager&);

private:
	typedef std::vector<ViewModelManagerHandler*> HandlerList;
	typedef std::vector<std::pair<Account*, ViewData*> > ViewDataMap;

private:
	UIManager* pUIManager_;
	Document* pDocument_;
	qs::Profile* pProfile_;
	HWND hwnd_;
	SecurityModel* pSecurityModel_;
	Account* pCurrentAccount_;
	ViewModelList listViewModel_;
	ViewModel* pCurrentViewModel_;
	ViewDataMap mapViewData_;
	std::auto_ptr<FilterManager> pFilterManager_;
	std::auto_ptr<ColorManager> pColorManager_;
	HandlerList listHandler_;
};


/****************************************************************************
 *
 * ViewModelManagerHandler
 *
 */

class ViewModelManagerHandler
{
public:
	virtual ~ViewModelManagerHandler();

public:
	virtual void viewModelSelected(const ViewModelManagerEvent& event) = 0;
};


/****************************************************************************
 *
 * ViewModelManagerEvent
 *
 */

class ViewModelManagerEvent
{
public:
	ViewModelManagerEvent(const ViewModelManager* pViewModelManager,
						  ViewModel* pNewViewModel,
						  ViewModel* pOldViewModel);
	~ViewModelManagerEvent();

public:
	const ViewModelManager* getViewModelManager() const;
	ViewModel* getNewViewModel() const;
	ViewModel* getOldViewModel() const;

private:
	ViewModelManagerEvent(const ViewModelManagerEvent&);
	ViewModelManagerEvent& operator=(const ViewModelManagerEvent&);

private:
	const ViewModelManager* pViewModelManager_;
	ViewModel* pNewViewModel_;
	ViewModel* pOldViewModel_;
};


/****************************************************************************
 *
 * ViewModelItemComp
 *
 */

class ViewModelItemComp :
	public std::binary_function<ViewModelItem*, ViewModelItem*, bool>
{
public:
	ViewModelItemComp(const ViewModel* pViewModel,
					  const ViewColumn& column,
					  bool bAscending,
					  bool bThread);
	~ViewModelItemComp();

public:
	bool operator()(const ViewModelItem* pLhs,
					const ViewModelItem* pRhs) const;

private:
	const ViewModel* pViewModel_;
	const ViewColumn& column_;
	bool bAscending_;
	bool bThread_;
};


/****************************************************************************
 *
 * ViewModelItemEqual
 *
 */

class ViewModelItemEqual :
	public std::binary_function<ViewModelItem*, ViewModelItem*, bool>
{
public:
	bool operator()(const ViewModelItem* pLhs,
					const ViewModelItem* pRhs) const;
};


/****************************************************************************
 *
 * ViewModelParentItemComp
 *
 */

class ViewModelParentItemComp : public std::unary_function<ViewModelItem*, bool>
{
public:
	ViewModelParentItemComp(unsigned int nReferenceHash,
							const WCHAR* pwszReference);

public:
	bool operator()(const ViewModelItem* pItem) const;

private:
	unsigned int nReferenceHash_;
	const WCHAR* pwszReference_;
};


/****************************************************************************
 *
 * ViewData
 *
 */

class ViewData
{
public:
	typedef std::vector<ViewDataItem*> ItemList;

public:
	ViewData(UIManager* pUIManager,
			 const WCHAR* pwszPath);
	~ViewData();

public:
	const ItemList& getItems() const;
	ViewDataItem* getItem(const Folder* pFolder);
	bool save() const;

public:
	void addItem(std::auto_ptr<ViewDataItem> pItem);
	void removeItem(unsigned int nFolderId);

private:
	ViewData(const ViewData&);
	ViewData& operator=(const ViewData&);

private:
	UIManager* pUIManager_;
	qs::wstring_ptr wstrPath_;
	ItemList listItem_;
};


/****************************************************************************
 *
 * DefaultViewData
 *
 */

class DefaultViewData
{
public:
	typedef std::vector<std::pair<qs::WSTRING, ViewDataItem*> > ItemList;

public:
	explicit DefaultViewData(const WCHAR* pwszPath);
	~DefaultViewData();

public:
	const ItemList& getItems() const;
	ViewDataItem* getItem(const WCHAR* pwszClass);
	void setItem(const WCHAR* pwszClass,
				 std::auto_ptr<ViewDataItem> pItem);
	bool save() const;

private:
	static std::auto_ptr<ViewDataItem> createDefaultItem();

private:
	DefaultViewData(const DefaultViewData&);
	DefaultViewData& operator=(const DefaultViewData&);

private:
	qs::wstring_ptr wstrPath_;
	ItemList listItem_;
};


/****************************************************************************
 *
 * ViewDataItem
 *
 */

class ViewDataItem
{
public:
	explicit ViewDataItem(unsigned int nFolderId);
	~ViewDataItem();

public:
	unsigned int getFolderId() const;
	const ViewColumnList& getColumns() const;
	void setColumns(const ViewColumnList& listColumn);
	void addColumn(std::auto_ptr<ViewColumn> pColumn);
	unsigned int getFocus() const;
	void setFocus(unsigned int nFocus);
	unsigned int getSort() const;
	void setSort(unsigned int nSort);
	const WCHAR* getFilter() const;
	void setFilter(const WCHAR* pwszFilter);

public:
	std::auto_ptr<ViewDataItem> clone(unsigned int nFolderId) const;

private:
	ViewDataItem(const ViewDataItem&);
	ViewDataItem& operator=(const ViewDataItem&);

private:
	unsigned int nFolderId_;
	ViewColumnList listColumn_;
	unsigned int nFocus_;
	unsigned int nSort_;
	qs::wstring_ptr wstrFilter_;
};


/****************************************************************************
 *
 * ViewDataContentHandler
 *
 */

class ViewDataContentHandler : public qs::DefaultHandler
{
public:
	ViewDataContentHandler(ViewData* pData);
	ViewDataContentHandler(DefaultViewData* pDefaultData);
	virtual ~ViewDataContentHandler();

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
	ViewDataContentHandler(const ViewDataContentHandler&);
	ViewDataContentHandler& operator=(const ViewDataContentHandler&);

private:
	enum State {
		STATE_ROOT,
		STATE_VIEWS,
		STATE_VIEW,
		STATE_COLUMNS,
		STATE_COLUMN,
		STATE_TITLE,
		STATE_MACRO,
		STATE_WIDTH,
		STATE_FOCUS,
		STATE_SORT,
		STATE_FILTER
	};

private:
	ViewData* pData_;
	DefaultViewData* pDefaultData_;
	State state_;
	qs::wstring_ptr wstrClass_;
	std::auto_ptr<ViewDataItem> pItem_;
	qs::wstring_ptr wstrTitle_;
	ViewColumn::Type type_;
	std::auto_ptr<Macro> pMacro_;
	unsigned int nFlags_;
	unsigned int nWidth_;
	unsigned int nSort_;
	qs::StringBuffer<qs::WSTRING> buffer_;
	MacroParser parser_;
};


/****************************************************************************
 *
 * ViewDataWriter
 *
 */

class ViewDataWriter
{
public:
	explicit ViewDataWriter(qs::Writer* pWriter);
	~ViewDataWriter();

public:
	bool write(const ViewData* pData);
	bool write(const DefaultViewData* pData);

private:
	bool write(const ViewDataItem* pItem,
			   const WCHAR* pwszClass);

private:
	ViewDataWriter(const ViewDataWriter&);
	ViewDataWriter& operator=(const ViewDataWriter&);

private:
	qs::OutputHandler handler_;
};

}

#include "viewmodel.inl"

#endif // __VIEWMODEL_H__
