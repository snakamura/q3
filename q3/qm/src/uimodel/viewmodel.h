/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
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
#include "messageviewmode.h"
#include "../model/color.h"


namespace qm {

class ViewColumn;
class ViewModel;
class ViewModelHandler;
class ViewModelEvent;
class ViewModelItem;
class ViewModelFolderComp;
class ViewModelHolder;
class ViewModelManager;
class ViewModelManagerHandler;
class ViewModelManagerEvent;
class ViewModelItemComp;
class ViewModelItemEqual;
class ViewModelParentItemComp;
class ViewData;
class DefaultViewData;
class ViewDataItem;
class ViewDataContentHandler;
class ViewDataWriter;

class Document;
class Filter;
class FilterManager;
class Folder;
class MessageHolder;
class Macro;
class SecurityModel;


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
		FLAG_CACHE			= 0x0010,
		
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
	void setCacheIndex(unsigned int n);

public:
	qs::wstring_ptr getText(const ViewModel* pViewModel,
							const ViewModelItem* pItem) const;
	unsigned int getNumber(const ViewModel* pViewModel,
						   const ViewModelItem* pItem) const;
	void getTime(const ViewModel* pViewModel,
				 const ViewModelItem* pItem,
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
	unsigned int nCacheIndex_;
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
	virtual void colorChanged(const ViewModelEvent& event) = 0;
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
	explicit ViewModelItem(MessageHolder* pmh);
	explicit ViewModelItem(unsigned int nMessageIdHash);
	~ViewModelItem();

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
	void invalidateColor();
	unsigned int getMessageFlags() const;
	void setMessageFlags(unsigned int nFlags);
	unsigned int getLevel() const;
	unsigned int getMessageIdHash() const;
	ViewModelItem* getLatestItem() const;
	bool updateLatestItem(ViewModelItem* pItem,
						  const ViewModelItemComp& comp);
	void clearLatestItem();
	const MacroValue* getCache(unsigned int n) const;
	void setCache(unsigned int n,
				  MacroValue* pValue) const;

public:
	static ViewModelItem* newItem(MessageHolder* pmh,
								  unsigned int nCacheSize);
	static void deleteItem(ViewModelItem* pItem,
						   unsigned int nCacheSize);

private:
	ViewModelItem(const ViewModelItem&);
	ViewModelItem& operator=(const ViewModelItem&);

private:
	MessageHolder* pmh_;
	ViewModelItem* pParentItem_;
	unsigned int nFlags_;
	COLORREF cr_;
	unsigned int nMessageFlags_;
	ViewModelItem* pLatestItem_;
};


/****************************************************************************
 *
 * ViewModelItemPtr
 *
 */

class ViewModelItemPtr
{
public:
	ViewModelItemPtr(MessageHolder* pmh,
					 unsigned int nCacheCount);
	~ViewModelItemPtr();

public:
	ViewModelItem* operator->() const;

public:
	ViewModelItem* get() const;
	ViewModelItem* release();

private:
	ViewModelItemPtr(const ViewModelItemPtr& ptr);
	ViewModelItemPtr& operator=(const ViewModelItemPtr& ptr);

private:
	ViewModelItem* pItem_;
	unsigned int nCacheCount_;
};


/****************************************************************************
 *
 * ViewModel
 *
 */

class ViewModel :
	public AbstractMessageViewMode,
	public DefaultFolderHandler,
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
		SORT_THREAD_MASK	= 0x00030000,
		
		SORT_FLOATTHREAD	= 0x00040000
	};

public:
	class RestoreInfo
	{
	public:
		RestoreInfo();
		RestoreInfo(MessageHolder* pmh);
	
	public:
		MessageHolder* getMessageHolder() const;
	
	private:
		MessageHolder* pmh_;
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
	
	void setSort(unsigned int nSort,
				 unsigned int nMask);
	unsigned int getSort() const;
	
	void setFilter(const Filter* pFilter);
	const Filter* getFilter() const;
	
	void setMode(unsigned int nMode,
				 unsigned int nMask);
	unsigned int getMode() const;
	
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
	void setFocused(unsigned int n,
					bool bDelay);
	unsigned int getFocused() const;
	bool isFocused(unsigned int n) const;
	unsigned int getScroll() const;
	void setScroll(unsigned int nScroll);
	
	void payAttention(unsigned int n);
	
	RestoreInfo getRestoreInfo() const;
	void setRestoreInfo(const RestoreInfo& info);
	
	void invalidateColors(const ColorManager* pColorManager);
	void save() const;
	
	void destroy();
	
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
	virtual bool isMode(Mode mode) const;
	virtual void setMode(Mode mode,
						 bool b);

public:
	virtual void messageAdded(const FolderMessageEvent& event);
	virtual void messageRemoved(const FolderMessageEvent& event);
	virtual void messageRefreshed(const FolderEvent& event);
	virtual void folderDestroyed(const FolderEvent& event);

public:
	virtual void messageHolderChanged(const MessageHolderEvent& event);
	virtual void messageHolderDestroyed(const MessageHolderEvent& event);

private:
	void update(bool bRestoreSelection,
				unsigned int nOldCacheCount);
	void sort(unsigned int nSort,
			  bool bRestoreSelection,
			  bool bUpdateParentLink);
	ViewModelItemComp getComparator(unsigned int nSort) const;
	bool isFloatThread(unsigned int nSort) const;
	void makeParentLink(bool bUpdateLatest);
	void makeParentLink(const ItemList& listItemSortedByMessageIdHash,
						const ItemList& listItemSortedByPointer,
						ViewModelItem* pItem);
	void updateCacheCount();

private:
	void fireItemAdded() const;
	void fireItemRemoved() const;
	void fireItemChanged(unsigned int nItem) const;
	void fireItemStateChanged(unsigned int nItem,
							  bool bDelay) const;
	void fireItemAttentionPaid(unsigned int nItem) const;
	void fireUpdated() const;
	void fireSorted() const;
	void fireColorChanged() const;
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
	SecurityModel* pSecurityModel_;
	std::auto_ptr<ColorList> pColorList_;
	ItemList listItem_;
	unsigned int nUnseenCount_;
	unsigned int nSort_;
	std::auto_ptr<Filter> pFilter_;
	unsigned int nLastSelection_;
	unsigned int nFocused_;
	unsigned int nScroll_;
	unsigned int nMode_;
	unsigned int nCacheCount_;
	RestoreInfo restoreInfo_;
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
	virtual void colorChanged(const ViewModelEvent& event);
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
	ViewModelEvent(const ViewModel* pViewModel,
				   unsigned int nItem,
				   bool bDelay);
	~ViewModelEvent();

public:
	const ViewModel* getViewModel() const;
	unsigned int getItem() const;
	bool isDelay() const;

private:
	ViewModelEvent(const ViewModelEvent&);
	ViewModelEvent& operator=(const ViewModelEvent&);

private:
	const ViewModel* pViewModel_;
	unsigned int nItem_;
	bool bDelay_;
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

class ViewModelManager :
	public DefaultAccountHandler,
	public ColorManagerHandler
{
public:
	typedef std::vector<ViewModel*> ViewModelList;

public:
	ViewModelManager(Document* pDocument,
					 qs::Profile* pProfile,
					 SecurityModel* pSecurityModel);
	~ViewModelManager();

public:
	DefaultViewData* getDefaultViewData() const;
	ColorManager* getColorManager() const;
	FilterManager* getFilterManager() const;
	
	Account* getCurrentAccount() const;
	void setCurrentAccount(Account* pAccount);
	void setCurrentFolder(Folder* pFolder);
	ViewModel* getCurrentViewModel() const;
	ViewModel* getViewModel(Folder* pFolder);
	
	bool save(bool bForce) const;
	
	void addViewModelManagerHandler(ViewModelManagerHandler* pHandler);
	void removeViewModelManagerHandler(ViewModelManagerHandler* pHandler);

// These methods are intended to be called from ViewModel class.
public:
	void removeViewModel(ViewModel* pViewModel);

public:
	virtual void accountDestroyed(const AccountEvent& event);

public:
	virtual void colorSetsChanged(const ColorManagerEvent& event);

private:
	void setCurrentFolder(Account* pAccount,
						  Folder* pFolder);
	void setCurrentViewModel(ViewModel* pViewModel);
	ViewDataItem* getViewDataItem(Folder* pFolder);
	qs::wstring_ptr getViewsPath(Account* pAccount);
	void invalidateColors();

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
	Document* pDocument_;
	qs::Profile* pProfile_;
	SecurityModel* pSecurityModel_;
	Account* pCurrentAccount_;
	ViewModelList listViewModel_;
	ViewModel* pCurrentViewModel_;
	ViewDataMap mapViewData_;
	std::auto_ptr<DefaultViewData> pDefaultViewData_;
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
					  bool bThread,
					  bool bFloat);
	~ViewModelItemComp();

public:
	bool operator()(const ViewModelItem* pLhs,
					const ViewModelItem* pRhs) const;
	int compare(const ViewModelItem* pLhs,
				const ViewModelItem* pRhs) const;

private:
	const ViewModel* pViewModel_;
	const ViewColumn& column_;
	bool bAscending_;
	bool bThread_;
	bool bFloat_;
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
	ViewData(DefaultViewData* pDefaultViewData,
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
	DefaultViewData* pDefaultViewData_;
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
	unsigned int getMode() const;
	void setMode(unsigned int nMode);

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
	unsigned int nMode_;
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
		STATE_FILTER,
		STATE_MODE
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
