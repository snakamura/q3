/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsaction.h>
#include <qsconv.h>

#include <commctrl.h>
#include <tchar.h>

#include "toolbar.h"

using namespace qs;


/****************************************************************************
 *
 * ToolbarManagerImpl
 *
 */

struct qs::ToolbarManagerImpl
{
	typedef std::vector<Toolbar*> ToolbarList;
	
	bool load(const WCHAR* pwszPath,
			  const ActionItem* pItem,
			  size_t nItemCount);
	const Toolbar* getToolbar(const WCHAR* pwszName) const;
	
	ToolbarManager* pThis_;
	HIMAGELIST hImageList_;
	ToolbarList listToolbar_;
};

bool qs::ToolbarManagerImpl::load(const WCHAR* pwszPath,
								  const ActionItem* pItem,
								  size_t nItemCount)
{
	assert(pwszPath);
	
	XMLReader reader;
	ToolbarContentHandler handler(&listToolbar_, pItem, nItemCount);
	reader.setContentHandler(&handler);
	return reader.parse(pwszPath);
}

const Toolbar* qs::ToolbarManagerImpl::getToolbar(const WCHAR* pwszName) const
{
	ToolbarList::const_iterator it = std::find_if(
		listToolbar_.begin(), listToolbar_.end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				string_equal<WCHAR>(),
				std::mem_fun(&Toolbar::getName),
				std::identity<const WCHAR*>()),
			pwszName));
	return it != listToolbar_.end() ? *it : 0;
}


/****************************************************************************
 *
 * ToolbarManager
 *
 */

qs::ToolbarManager::ToolbarManager(const WCHAR* pwszPath,
								   HBITMAP hBitmap,
								   const ActionItem* pItem,
								   size_t nItemCount) :
	pImpl_(0)
{
	assert(pwszPath);
	assert(pItem);
	
#ifdef _WIN32_WCE
	UINT nFlags = ILC_COLOR | ILC_MASK;
#else
	UINT nFlags = ILC_COLOR32 | ILC_MASK;
#endif
	HIMAGELIST hImageList = ImageList_Create(16, 16, nFlags, 16, 16);
	ImageList_AddMasked(hImageList, hBitmap, RGB(192, 192, 192));
	
	std::auto_ptr<ToolbarManagerImpl> pImpl(new ToolbarManagerImpl());
	pImpl->pThis_ = this;
	pImpl->hImageList_ = hImageList;
	
	if (!pImpl->load(pwszPath, pItem, nItemCount))
		return;
	
	pImpl_ = pImpl.release();
}

qs::ToolbarManager::~ToolbarManager()
{
	if (pImpl_) {
		std::for_each(pImpl_->listToolbar_.begin(),
			pImpl_->listToolbar_.end(), deleter<Toolbar>());
		ImageList_Destroy(pImpl_->hImageList_);
		delete pImpl_;
	}
}

ToolbarCookie* qs::ToolbarManager::createButtons(const WCHAR* pwszName,
												 HWND hwnd,
												 WindowBase* pParent) const
{
	const Toolbar* pToolbar = pImpl_->getToolbar(pwszName);
	if (!pToolbar)
		return 0;
	return pToolbar->create(hwnd, pParent, pImpl_->hImageList_);
}

void qs::ToolbarManager::destroy(ToolbarCookie* pCookie) const
{
	const Toolbar* pToolbar = pImpl_->getToolbar(pCookie->getName());
	if (pToolbar)
		pToolbar->destroy(pCookie);
}


/****************************************************************************
 *
 * ToolbarCookie
 *
 */

#ifndef _WIN32_WCE
qs::ToolbarCookie::ToolbarCookie(const WCHAR* pwszName,
								 WindowBase* pParent,
								 std::auto_ptr<NotifyHandler> pNotifyHandler) :
	pParent_(pParent),
	pNotifyHandler_(pNotifyHandler)
{
	wstrName_ = allocWString(pwszName);
}
#else
qs::ToolbarCookie::ToolbarCookie(const WCHAR* pwszName,
								 ToolTipList& listToolTip)
{
	wstrName_ = allocWString(pwszName);
	listToolTip_.swap(listToolTip);
}
#endif

qs::ToolbarCookie::~ToolbarCookie()
{
}

const WCHAR* qs::ToolbarCookie::getName() const
{
	return wstrName_.get();
}

#ifndef _WIN32_WCE
WindowBase* qs::ToolbarCookie::getParent() const
{
	return pParent_;
}

NotifyHandler* qs::ToolbarCookie::getNotifyHandler() const
{
	return pNotifyHandler_.get();
}
#endif


/****************************************************************************
 *
 * Toolbar
 *
 */

qs::Toolbar::Toolbar(const WCHAR* pwszName,
					 bool bShowText) :
	bShowText_(bShowText)
{
	wstrName_ = allocWString(pwszName);
}

qs::Toolbar::~Toolbar()
{
	std::for_each(listItem_.begin(), listItem_.end(), deleter<ToolbarItem>());
}

const WCHAR* qs::Toolbar::getName() const
{
	return wstrName_.get();
}

ToolbarCookie* qs::Toolbar::create(HWND hwnd,
								   WindowBase* pParent,
								   HIMAGELIST hImageList) const
{
	HIMAGELIST hImageListCopy = ImageList_Duplicate(hImageList);
	if (!hImageListCopy)
		return 0;
	::SendMessage(hwnd, TB_SETIMAGELIST, 0, reinterpret_cast<LPARAM>(hImageListCopy));
	
#ifdef _WIN32_WCE
	ToolbarCookie::ToolTipList listToolTip;
	listToolTip.reserve(listItem_.size());
	for (ItemList::const_iterator it = listItem_.begin(); it != listItem_.end(); ++it) {
		const ToolbarItem* pItem = *it;
		if (!pItem->isSeparator())
			listToolTip.push_back(pItem->getToolTip());
	}
	CommandBar_AddToolTips(hwnd, listToolTip.size(), &listToolTip[0]);
#endif
	
	for (ItemList::const_iterator it = listItem_.begin(); it != listItem_.end(); ++it) {
		if (!(*it)->create(hwnd, bShowText_))
			return 0;
	}
	
	::SendMessage(hwnd, TB_AUTOSIZE, 0, 0);

#ifndef _WIN32_WCE
	UINT nId = Window(hwnd).getWindowLong(GWL_ID);
	std::auto_ptr<NotifyHandler> pNotifyHandler(new ToolbarNotifyHandler(this, nId));
	pParent->addNotifyHandler(pNotifyHandler.get());
	return new ToolbarCookie(wstrName_.get(), pParent, pNotifyHandler);
#else
	return new ToolbarCookie(wstrName_.get(), listToolTip);
#endif
}

void qs::Toolbar::destroy(ToolbarCookie* pCookie) const
{
#ifndef _WIN32_WCE
	pCookie->getParent()->removeNotifyHandler(pCookie->getNotifyHandler());
#endif
	delete pCookie;
}

void qs::Toolbar::add(std::auto_ptr<ToolbarItem> pItem)
{
	listItem_.push_back(pItem.get());
	pItem.release();
}

const ToolbarItem* qs::Toolbar::getItem(UINT nAction) const
{
	for (ItemList::const_iterator it = listItem_.begin(); it != listItem_.end(); ++it) {
		const ToolbarItem* pItem = *it;
		if (pItem->getAction() == nAction)
			return pItem;
	}
	return 0;
}


/****************************************************************************
 *
 * ToolbarItem
 *
 */

qs::ToolbarItem::ToolbarItem()
{
}

qs::ToolbarItem::~ToolbarItem()
{
}


/****************************************************************************
 *
 * ToolbarButton
 *
 */

qs::ToolbarButton::ToolbarButton(int nImage,
								 const WCHAR* pwszText,
								 const WCHAR* pwszToolTip,
								 UINT nAction,
								 const WCHAR* pwszDropDown) :
	nImage_(nImage),
	nAction_(nAction)
{
	wstring_ptr wstrText;
	if (pwszText)
		wstrText = allocWString(pwszText);
	
	wstring_ptr wstrToolTip;
	if (pwszToolTip)
		wstrToolTip = allocWString(pwszToolTip);
	
	wstring_ptr wstrDropDown;
	if (pwszDropDown)
		wstrDropDown = allocWString(pwszDropDown);
	
	wstrText_ = wstrText;
	wstrToolTip_ = wstrToolTip;
	wstrDropDown_ = wstrDropDown;
}

qs::ToolbarButton::~ToolbarButton()
{
}

bool qs::ToolbarButton::create(HWND hwnd,
							   bool bShowText)
{
	// TODO
	// Handle dropdown.
	
	int nTextIndex = -1;
	if (bShowText && wstrText_.get()) {
		W2T(wstrText_.get(), ptszText);
		size_t nLen = _tcslen(ptszText);
		tstring_ptr tstrText(allocTString(nLen + 2));
		_tcscpy(tstrText.get(), ptszText);
		*(tstrText.get() + nLen + 1) = _T('\0');
		nTextIndex = ::SendMessage(hwnd, TB_ADDSTRING,
			0, reinterpret_cast<LPARAM>(tstrText.get()));
	}
	
	TBBUTTON button = { 0 };
	button.iBitmap = nImage_;
	button.idCommand = nAction_;
	button.fsState = TBSTATE_ENABLED;
	button.fsStyle = TBSTYLE_BUTTON | TBSTYLE_AUTOSIZE;
	button.iString = nTextIndex;
	::SendMessage(hwnd, TB_ADDBUTTONS, 1, reinterpret_cast<LPARAM>(&button));
	
	return true;
}

UINT qs::ToolbarButton::getAction() const
{
	return nAction_;
}

const WCHAR* qs::ToolbarButton::getToolTip() const
{
	return wstrToolTip_.get() ? wstrToolTip_.get() : wstrText_.get();
}

bool qs::ToolbarButton::isSeparator() const
{
	return false;
}


/****************************************************************************
 *
 * ToolbarSeparator
 *
 */

qs::ToolbarSeparator::ToolbarSeparator()
{
}

qs::ToolbarSeparator::~ToolbarSeparator()
{
}

bool qs::ToolbarSeparator::create(HWND hwnd,
								  bool bShowText)
{
	TBBUTTON button = { 0 };
	button.iBitmap = 0;
	button.idCommand = 0;
	button.fsState = TBSTATE_ENABLED;
	button.fsStyle = TBSTYLE_SEP;
	::SendMessage(hwnd, TB_ADDBUTTONS, 1, reinterpret_cast<LPARAM>(&button));
	
	return true;
}

UINT qs::ToolbarSeparator::getAction() const
{
	return -1;
}

const WCHAR* qs::ToolbarSeparator::getToolTip() const
{
	return 0;
}

bool qs::ToolbarSeparator::isSeparator() const
{
	return true;
}


/****************************************************************************
 *
 * ToolbarContentHandler
 *
 */

qs::ToolbarContentHandler::ToolbarContentHandler(ToolbarList* pListToolbar,
												 const ActionItem* pItem,
												 size_t nItemCount) :
	pListToolbar_(pListToolbar),
	pActionItem_(pItem),
	nActionItemCount_(nItemCount),
	state_(STATE_ROOT),
	pToolbar_(0)
{
}

qs::ToolbarContentHandler::~ToolbarContentHandler()
{
}

bool qs::ToolbarContentHandler::startElement(const WCHAR* pwszNamespaceURI,
											 const WCHAR* pwszLocalName,
											 const WCHAR* pwszQName,
											 const Attributes& attributes)
{
	if (wcscmp(pwszLocalName, L"toolbars") == 0) {
		if (state_ != STATE_ROOT)
			return false;
		if (attributes.getLength() != 0)
			return false;
		state_ = STATE_TOOLBARS;
	}
	else if (wcscmp(pwszLocalName, L"toolbar") == 0) {
		if (state_ != STATE_TOOLBARS)
			return false;
		
		const WCHAR* pwszName = 0;
		bool bShowText = true;
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrName, L"name") == 0)
				pwszName = attributes.getValue(n);
			else if (wcscmp(pwszAttrName, L"showText") == 0)
				bShowText = wcscmp(attributes.getValue(n), L"true") == 0;
			else
				return false;
		}
		if (!pwszName)
			return false;
		
		wstring_ptr wstrName(allocWString(pwszName));
		
		std::auto_ptr<Toolbar> pToolbar(new Toolbar(pwszName, bShowText));
		pListToolbar_->push_back(pToolbar.get());
		pToolbar_ = pToolbar.release();
		
		state_ = STATE_TOOLBAR;
	}
	else if (wcscmp(pwszLocalName, L"button") == 0) {
		if (state_ != STATE_TOOLBAR)
			return false;
		
		int nImage = -1;
		const WCHAR* pwszText = 0;
		const WCHAR* pwszToolTip = 0;
		const WCHAR* pwszAction = 0;
		const WCHAR* pwszDropDown = 0;
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrName, L"image") == 0) {
				WCHAR* pEnd = 0;
				nImage = wcstol(attributes.getValue(n), &pEnd, 10);
				if (*pEnd)
					return false;
			}
			else if (wcscmp(pwszAttrName, L"text") == 0) {
				pwszText = attributes.getValue(n);
			}
			else if (wcscmp(pwszAttrName, L"tooltip") == 0) {
				pwszToolTip = attributes.getValue(n);
			}
			else if (wcscmp(pwszAttrName, L"action") == 0) {
				pwszAction = attributes.getValue(n);
			}
			else if (wcscmp(pwszAttrName, L"dropdown") == 0) {
				pwszDropDown = attributes.getValue(n);
			}
			else {
				return false;
			}
		}
		
		if (nImage == -1 || (!pwszAction && !pwszDropDown))
			return false;
		
		UINT nAction = getActionId(pwszAction);
		if (nAction != -1) {
			std::auto_ptr<ToolbarButton> pButton(new ToolbarButton(
				nImage, pwszText, pwszToolTip, nAction, pwszDropDown));
			pToolbar_->add(pButton);
		}
		
		state_ = STATE_BUTTON;
	}
	else if (wcscmp(pwszLocalName, L"separator") == 0) {
		if (state_ != STATE_TOOLBAR)
			return false;
		if (attributes.getLength() != 0)
			return false;
		
		std::auto_ptr<ToolbarSeparator> pSeparator(new ToolbarSeparator());
		pToolbar_->add(pSeparator);
		
		state_ = STATE_SEPARATOR;
	}
	else {
		return false;
	}
	
	return true;
}

bool qs::ToolbarContentHandler::endElement(const WCHAR* pwszNamespaceURI,
										   const WCHAR* pwszLocalName,
										   const WCHAR* pwszQName)
{
	if (wcscmp(pwszLocalName, L"toolbars") == 0) {
		assert(state_ == STATE_TOOLBARS);
		state_ = STATE_ROOT;
	}
	else if (wcscmp(pwszLocalName, L"toolbar") == 0) {
		assert(state_ == STATE_TOOLBAR);
		pToolbar_ = 0;
		state_ = STATE_TOOLBARS;
	}
	else if (wcscmp(pwszLocalName, L"button") == 0) {
		assert(state_ == STATE_BUTTON);
		state_ = STATE_TOOLBAR;
	}
	else if (wcscmp(pwszLocalName, L"separator") == 0) {
		assert(state_ == STATE_SEPARATOR);
		state_ = STATE_TOOLBAR;
	}
	else {
		return false;
	}
	
	return true;
}

bool qs::ToolbarContentHandler::characters(const WCHAR* pwsz,
										   size_t nStart,
										   size_t nLength)
{
	const WCHAR* p = pwsz + nStart;
	for (size_t n = 0; n < nLength; ++n, ++p) {
		if (*p != L' ' && *p != L'\t' && *p != '\n')
			return false;
	}
	
	return true;
}

UINT qs::ToolbarContentHandler::getActionId(const WCHAR* pwszAction)
{
	ActionItem item = {
		pwszAction,
		0
	};
	
	const ActionItem* pItem = std::lower_bound(
		pActionItem_, pActionItem_ + nActionItemCount_, item,
		binary_compose_f_gx_hy(
			string_less<WCHAR>(),
			mem_data_ref(&ActionItem::pwszAction_),
			mem_data_ref(&ActionItem::pwszAction_)));
	if (pItem != pActionItem_ + nActionItemCount_ &&
		wcscmp(pItem->pwszAction_, pwszAction) == 0)
		return pItem->nId_;
	else
		return -1;
}


#ifndef _WIN32_WCE

/****************************************************************************
 *
 * ToolbarNotifyHandler
 *
 */

qs::ToolbarNotifyHandler::ToolbarNotifyHandler(const Toolbar* pToolbar,
											   UINT nId) :
	pToolbar_(pToolbar),
	nId_(nId)
{
}

qs::ToolbarNotifyHandler::~ToolbarNotifyHandler()
{
}

LRESULT qs::ToolbarNotifyHandler::onNotify(NMHDR* pnmhdr,
										   bool* pbHandled)
{
	BEGIN_NOTIFY_HANDLER()
		HANDLE_NOTIFY_CODE(TTN_GETDISPINFO, onGetDispInfo)
	END_NOTIFY_HANDLER()
	return 1;
}

LRESULT qs::ToolbarNotifyHandler::onGetDispInfo(NMHDR* pnmhdr,
												bool* pbHandled)
{
	NMTTDISPINFO* pDispInfo = reinterpret_cast<NMTTDISPINFO*>(pnmhdr);
	
	const ToolbarItem* pItem = pToolbar_->getItem(pnmhdr->idFrom);
	if (pItem) {
		const WCHAR* pwszToolTip = pItem->getToolTip();
		if (pwszToolTip) {
			W2T(pwszToolTip, ptszToolTip);
			_tcsncpy(pDispInfo->szText, ptszToolTip, countof(pDispInfo->szText));
		}
	}
	
	return 0;
}

#endif // _WIN32_WCE
