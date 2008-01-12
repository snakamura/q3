/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __RECENTSWINDOW_H__
#define __RECENTSWINDOW_H__

#ifdef QMRECENTSWINDOW

#include <qm.h>
#include <qmrecents.h>

#include <qsdevicecontext.h>
#include <qstheme.h>
#include <qswindow.h>


namespace qm {

class RecentsWindow;
class RecentsWindowManager;

class FolderImage;
class Recents;
class URIResolver;


/****************************************************************************
 *
 * RecentsWindow
 *
 */

class RecentsWindow :
	public qs::WindowBase,
	public qs::DefaultWindowHandler
{
public:
	enum {
		WM_RECENTSWINDOW_SHOWPASSIVE	= WM_APP + 1701
	};

private:
	class Item;
	class ScanCallback;
	struct Button;

private:
	typedef std::vector<Item*> ItemList;

public:
	RecentsWindow(Recents* pRecents,
				  const URIResolver* pURIResolver,
				  qs::ActionMap* pActionMap,
				  const FolderImage* pFolderImage,
				  qs::Profile* pProfile,
				  HWND hwnd);
	virtual ~RecentsWindow();

public:
	void showActive(bool bHotKey);
	void showPassive();

public:
	virtual void getWindowClass(WNDCLASS* pwc);
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
#if !defined _WIN32_WCE && (_WIN32_WINNT >= 0x0400 || WINVER >= 0x0500)
	LRESULT onMouseLeave();
#endif
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
	LRESULT onTimer(UINT_PTR nId);
	LRESULT onShowPassive(WPARAM wParam,
						  LPARAM lParam);

private:
	void layout(bool bAtMousePosition,
				bool bTopMost);
	void paintSeparator(qs::DeviceContext& dc);
	void paintButtons(qs::DeviceContext& dc);
	void paintButton(qs::DeviceContext& dc,
					 const WCHAR* pwszText,
					 const RECT& rect,
					 bool bSelected);
	COLORREF getColor(int nIndex) const;
	
	void prepareItems(bool bActive);
	void clearItems();
	
	int calcHeight() const;
	void getItemRect(ItemList::size_type nItem,
					 RECT* pRect) const;
	ItemList::size_type getSelectedItem(int nY) const;
	void invalidateItem(ItemList::size_type nItem);
	void scanItems(ScanCallback* pCallback) const;
	
	void openItem(ItemList::size_type nItem);
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
		SEPARATOR_MARGIN	= 4,
		BUTTON_WIDTH		= 60,
		BUTTON_MARGIN		= 4,
		BUTTON_PADDING		= 4
	};
	
	enum {
		TIMER_HIDE		= 1000,
		TIMER_UPDATE	= 1001,
		UPDATE_INTERVAL	= 1000
	};

private:
	enum Show {
		SHOW_HIDDEN,
		SHOW_ACTIVE,
		SHOW_PASSIVE
	};

private:
	Recents* pRecents_;
	const URIResolver* pURIResolver_;
	qs::ActionMap* pActionMap_;
	const FolderImage* pFolderImage_;
	qs::Profile* pProfile_;
	HWND hwnd_;
	
	ItemList listItem_;
	ItemList::size_type nSelectedItem_;
	
	int nSelectedButton_;
	
	HFONT hfont_;
	HFONT hfontBold_;
	COLORREF crForeground_;
	COLORREF crBackground_;
	COLORREF crSelectedForeground_;
	COLORREF crSelectedBackground_;
	HIMAGELIST hImageList_;
	int nWidth_;
	int nLineHeight_;
	int nHeaderLineHeight_;
	int nMnemonicWidth_;
	int nButtonHeight_;
	std::auto_ptr<qs::Theme> pTheme_;
	
	int nHideTimeout_;
	bool bImeControl_;
	
	Show show_;
	bool bMouseTracking_;

private:
	static Button buttons__[];
};


/****************************************************************************
 *
 * RecentsWindowManager
 *
 */

class RecentsWindowManager : private RecentsHandler
{
public:
	RecentsWindowManager(Recents* pRecents,
						 const URIResolver* pURIResolver,
						 qs::ActionMap* pActionMap,
						 const FolderImage* pFolderImage,
						 qs::Profile* pProfile,
						 HWND hwnd_);
	~RecentsWindowManager();

public:
	bool showPopup(bool bHotKey);
	void reloadProfiles();

public:
	virtual void recentsChanged(const RecentsEvent& event);

private:
	bool createWindow();

private:
	RecentsWindowManager(const RecentsWindowManager&);
	RecentsWindowManager& operator=(const RecentsWindowManager&);

private:
	Recents* pRecents_;
	const URIResolver* pURIResolver_;
	qs::ActionMap* pActionMap_;
	const FolderImage* pFolderImage_;
	qs::Profile* pProfile_;
	HWND hwnd_;
	
	RecentsWindow* pRecentsWindow_;
	bool bShowPassive_;
};

}

#endif // QMRECENTSWINDOW

#endif // __RECENTSWINDOW_H__
