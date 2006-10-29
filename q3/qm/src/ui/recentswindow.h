/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __RECENTSWINDOW_H__
#define __RECENTSWINDOW_H__

#ifdef QMRECENTSWINDOW

#include <qm.h>

#include <qsdevicecontext.h>
#include <qstheme.h>
#include <qswindow.h>


namespace qm {

class RecentsWindow;

class AccountManager;
class FolderImage;
class Recents;


/****************************************************************************
 *
 * RecentsWindow
 *
 */

class RecentsWindow :
	public qs::WindowBase,
	public qs::DefaultWindowHandler
{
private:
	class Item;
	class ScanCallback;
	struct Button;

private:
	typedef std::vector<Item*> ItemList;

public:
	RecentsWindow(Recents* pRecents,
				  const AccountManager* pAccountManager,
				  qs::ActionMap* pActionMap,
				  const FolderImage* pFolderImage,
				  qs::Profile* pProfile);
	virtual ~RecentsWindow();

public:
	bool showPopup(HWND hwndOwner,
				   bool bHotKey);

public:
	virtual LRESULT windowProc(UINT uMsg,
							   WPARAM wParam,
							   LPARAM lParam);

protected:
	LRESULT onActivate(UINT nFlags,
					   HWND hwnd,
					   bool bMinimized);
	LRESULT onChar(UINT nChar,
				   UINT nRepeat,
				   UINT nFlags);
	LRESULT onCreate(CREATESTRUCT* pCreateStruct);
	LRESULT onDestroy();
	LRESULT onKeyDown(UINT nKey,
					  UINT nRepeat,
					  UINT nFlags);
	LRESULT onLButtonUp(UINT nFlags,
						const POINT& pt);
	LRESULT onMouseMove(UINT nFlags,
						const POINT& pt);
	LRESULT onNcPaint(HRGN hrgn);
	LRESULT onPaint();
	LRESULT onSetCursor(HWND hwnd,
						UINT nHitTest,
						UINT nMessage);
	LRESULT onSysKeyDown(UINT nKey,
						 UINT nRepeat,
						 UINT nFlags);
	LRESULT onThemeChanged();

private:
	void layout(HWND hwndOwner,
				bool bAtMousePosition);
	void paintButtons(qs::DeviceContext& dc);
	void paintButton(qs::DeviceContext& dc,
					 const WCHAR* pwszText,
					 const RECT& rect,
					 bool bSelected);
	
	void prepareItems();
	void clearItems();
	
	int calcHeight() const;
	void getItemRect(ItemList::size_type nItem,
					 RECT* pRect) const;
	ItemList::size_type getSelectedItem(int nY) const;
	void invalidateItem(ItemList::size_type nItem);
	void scanItems(ScanCallback* pCallback) const;
	
	void open(ItemList::size_type nItem);
	void invokeAction(unsigned int nId,
					  const WCHAR* pwszParam);
	
	void close();
	
	int getButtonByPos(const POINT& pt) const;
	int getButtonByMnemonic(WCHAR c) const;
	void getButtonRect(int nButton,
					   RECT* pRect) const;
	void invalidateButton(int nButton);

private:
	RecentsWindow(const RecentsWindow&);
	RecentsWindow& operator=(const RecentsWindow&);

private:
	class Item
	{
	public:
		Item(MessageHolder* pmh,
			 qs::wstring_ptr wstrSubject,
			 qs::wstring_ptr wstrFrom);
		~Item();
	
	public:
		const MessagePtr& getMessagePtr() const;
		const WCHAR* getSubject() const;
		const WCHAR* getFrom() const;
	
	private:
		Item(const Item&);
		Item& operator=(const Item&);
	
	private:
		MessagePtr ptr_;
		qs::wstring_ptr wstrURI_;
		qs::wstring_ptr wstrSubject_;
		qs::wstring_ptr wstrFrom_;
	};
	
	struct ItemComparator : public std::binary_function<Item*, Item*, bool>
	{
		bool operator()(const Item* pLhs,
						const Item* pRhs) const;
	};
	
	class ScanCallback
	{
	public:
		virtual ~ScanCallback();
	
	public:
		virtual bool account(const Account* pAccount,
							 int nTop,
							 int nBottom);
		virtual bool folder(const NormalFolder* pFolder,
							int nTop,
							int nBottom);
		virtual bool item(const Item* pItem,
						  ItemList::size_type nItem,
						  int nTop,
						  int nBottom);
	};
	
	struct Button
	{
		unsigned int nId_;
		UINT nTextId_;
		WCHAR cMnemonic_;
	};

#if _MSC_VER >= 1400
private:
#else
public:
#endif
	enum {
		MARGIN				= 4,
		ITEM_SPACING		= 4,
		LINE_SPACING		= 2,
		FOLDER_OFFSET		= 4,
		ITEM_OFFSET			= 4,
		IMAGE_WIDTH			= 16,
		IMAGE_HEIGHT		= 16,
		IMAGE_SPACING		= 4,
		MNEMONIC_SPACING	= 3,
		BUTTON_WIDTH		= 50,
		BUTTON_MARGIN		= 4,
		BUTTON_PADDING		= 4
	};

private:
	Recents* pRecents_;
	const AccountManager* pAccountManager_;
	qs::ActionMap* pActionMap_;
	const FolderImage* pFolderImage_;
	qs::Profile* pProfile_;
	
	ItemList listItem_;
	ItemList::size_type nSelectedItem_;
	
	int nSelectedButton_;
	
	HFONT hfont_;
	HFONT hfontBold_;
	HIMAGELIST hImageList_;
	int nWidth_;
	int nLineHeight_;
	int nHeaderLineHeight_;
	int nMnemonicWidth_;
	int nButtonHeight_;
	std::auto_ptr<qs::Theme> pTheme_;

private:
	static Button buttons__[];
};

}

#endif // QMRECENTSWINDOW

#endif // __RECENTSWINDOW_H__