/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsaction.h>
#include <qsconv.h>
#include <qsnew.h>

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
	
	QSTATUS load(const WCHAR* pwszPath,
		const ActionItem* pItem, size_t nItemCount);
	
	ToolbarManager* pThis_;
	HIMAGELIST hImageList_;
	ToolbarList listToolbar_;
};

QSTATUS qs::ToolbarManagerImpl::load(const WCHAR* pwszPath,
	const ActionItem* pItem, size_t nItemCount)
{
	assert(pwszPath);
	
	DECLARE_QSTATUS();
	
	XMLReader reader(&status);
	CHECK_QSTATUS();
	ToolbarContentHandler handler(&listToolbar_, pItem, nItemCount, &status);
	CHECK_QSTATUS();
	reader.setContentHandler(&handler);
	status = reader.parse(pwszPath);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * ToolbarManager
 *
 */

qs::ToolbarManager::ToolbarManager(const WCHAR* pwszPath, HBITMAP hBitmap,
	const ActionItem* pItem, size_t nItemCount, QSTATUS* pstatus) :
	pImpl_(0)
{
	assert(pwszPath);
	assert(pItem);
	assert(pstatus);
	
	DECLARE_QSTATUS();
	
#ifdef _WIN32_WCE
	UINT nFlags = ILC_COLOR | ILC_MASK;
#else
	UINT nFlags = ILC_COLOR32 | ILC_MASK;
#endif
	HIMAGELIST hImageList = ImageList_Create(16, 16, nFlags, 16, 16);
	ImageList_AddMasked(hImageList, hBitmap, RGB(192, 192, 192));
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	pImpl_->pThis_ = this;
	pImpl_->hImageList_ = hImageList;
	
	status = pImpl_->load(pwszPath, pItem, nItemCount);
	CHECK_QSTATUS_SET(pstatus);
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

QSTATUS qs::ToolbarManager::createToolbar(const WCHAR* pwszName, HWND hwnd) const
{
	DECLARE_QSTATUS();
	
	ToolbarManagerImpl::ToolbarList::const_iterator it = std::find_if(
		pImpl_->listToolbar_.begin(), pImpl_->listToolbar_.end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				string_equal<WCHAR>(),
				std::mem_fun(&Toolbar::getName),
				std::identity<const WCHAR*>()),
			pwszName));
	if (it != pImpl_->listToolbar_.end()) {
		const Toolbar* pToolbar = *it;
		status = pToolbar->create(hwnd, pImpl_->hImageList_);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * Toolbar
 *
 */

qs::Toolbar::Toolbar(const WCHAR* pwszName, bool bShowText, QSTATUS* pstatus) :
	wstrName_(0),
	bShowText_(bShowText)
{
	wstrName_ = allocWString(pwszName);
	if (!wstrName_) {
		*pstatus = QSTATUS_OUTOFMEMORY;
		return;
	}
}

qs::Toolbar::~Toolbar()
{
	freeWString(wstrName_);
	std::for_each(listItem_.begin(), listItem_.end(), deleter<ToolbarItem>());
}

const WCHAR* qs::Toolbar::getName() const
{
	return wstrName_;
}

QSTATUS qs::Toolbar::create(HWND hwnd, HIMAGELIST hImageList) const
{
	DECLARE_QSTATUS();
	
	HIMAGELIST hImageListCopy = ImageList_Duplicate(hImageList);
	if (!hImageListCopy)
		return QSTATUS_FAIL;
	::SendMessage(hwnd, TB_SETIMAGELIST, 0, reinterpret_cast<LPARAM>(hImageListCopy));
	
	ItemList::const_iterator it = listItem_.begin();
	while (it != listItem_.end()) {
		status = (*it)->create(hwnd, bShowText_);
		CHECK_QSTATUS();
		++it;
	}
	
	::SendMessage(hwnd, TB_AUTOSIZE, 0, 0);
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::Toolbar::add(ToolbarItem* pItem)
{
	return STLWrapper<ItemList>(listItem_).push_back(pItem);
}


/****************************************************************************
 *
 * ToolbarItem
 *
 */

qs::ToolbarItem::ToolbarItem(QSTATUS* pstatus)
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

qs::ToolbarButton::ToolbarButton(int nImage, const WCHAR* pwszText,
	const WCHAR* pwszToolTip, UINT nAction,
	const WCHAR* pwszDropDown, QSTATUS* pstatus) :
	ToolbarItem(pstatus),
	nImage_(nImage),
	wstrText_(0),
	wstrToolTip_(0),
	nAction_(nAction),
	wstrDropDown_(0)
{
	string_ptr<WSTRING> wstrText;
	if (pwszText) {
		wstrText.reset(allocWString(pwszText));
		if (!wstrText.get()) {
			*pstatus = QSTATUS_OUTOFMEMORY;
			return;
		}
	}
	
	string_ptr<WSTRING> wstrToolTip;
	if (pwszToolTip) {
		wstrToolTip.reset(allocWString(pwszToolTip));
		if (!wstrToolTip.get()) {
			*pstatus = QSTATUS_OUTOFMEMORY;
			return;
		}
	}
	
	string_ptr<WSTRING> wstrDropDown;
	if (pwszDropDown) {
		wstrDropDown.reset(allocWString(pwszDropDown));
		if (!wstrDropDown.get()) {
			*pstatus = QSTATUS_OUTOFMEMORY;
			return;
		}
	}
	
	wstrText_ = wstrText.release();
	wstrToolTip_ = wstrToolTip.release();
	wstrDropDown_ = wstrDropDown.release();
}

qs::ToolbarButton::~ToolbarButton()
{
	freeWString(wstrText_);
	freeWString(wstrToolTip_);
	freeWString(wstrDropDown_);
}

QSTATUS qs::ToolbarButton::create(HWND hwnd, bool bShowText)
{
	DECLARE_QSTATUS();
	
	// TODO
	// Add tooltip, handle dropdown.
	
	int nTextIndex = -1;
	if (bShowText && wstrText_) {
		W2T(wstrText_, ptszText);
		size_t nLen = _tcslen(ptszText);
		string_ptr<TSTRING> tstrText(allocTString(nLen + 2));
		if (tstrText.get()) {
			_tcscpy(tstrText.get(), ptszText);
			*(tstrText.get() + nLen + 1) = _T('\0');
			nTextIndex = ::SendMessage(hwnd, TB_ADDSTRING,
				0, reinterpret_cast<LPARAM>(tstrText.get()));
		}
	}
	
	TBBUTTON button = { 0 };
	button.iBitmap = nImage_;
	button.idCommand = nAction_;
	button.fsState = TBSTATE_ENABLED;
	button.fsStyle = TBSTYLE_BUTTON | TBSTYLE_AUTOSIZE;
	button.iString = nTextIndex;
	::SendMessage(hwnd, TB_ADDBUTTONS, 1, reinterpret_cast<LPARAM>(&button));
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * ToolbarSeparator
 *
 */

qs::ToolbarSeparator::ToolbarSeparator(QSTATUS* pstatus) :
	ToolbarItem(pstatus)
{
}

qs::ToolbarSeparator::~ToolbarSeparator()
{
}

QSTATUS qs::ToolbarSeparator::create(HWND hwnd, bool bShowText)
{
	TBBUTTON button = { 0 };
	button.iBitmap = 0;
	button.idCommand = 0;
	button.fsState = TBSTATE_ENABLED;
	button.fsStyle = TBSTYLE_SEP;
	::SendMessage(hwnd, TB_ADDBUTTONS, 1, reinterpret_cast<LPARAM>(&button));
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * ToolbarContentHandler
 *
 */

qs::ToolbarContentHandler::ToolbarContentHandler(ToolbarList* pListToolbar,
	const ActionItem* pItem, size_t nItemCount, QSTATUS* pstatus) :
	DefaultHandler(pstatus),
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

QSTATUS qs::ToolbarContentHandler::startElement(const WCHAR* pwszNamespaceURI,
	const WCHAR* pwszLocalName, const WCHAR* pwszQName, const Attributes& attributes)
{
	DECLARE_QSTATUS();
	
	if (wcscmp(pwszLocalName, L"toolbars") == 0) {
		if (state_ != STATE_ROOT)
			return QSTATUS_FAIL;
		if (attributes.getLength() != 0)
			return QSTATUS_FAIL;
		state_ = STATE_TOOLBARS;
	}
	else if (wcscmp(pwszLocalName, L"toolbar") == 0) {
		if (state_ != STATE_TOOLBARS)
			return QSTATUS_FAIL;
		
		const WCHAR* pwszName = 0;
		bool bShowText = true;
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrName, L"name") == 0)
				pwszName = attributes.getValue(n);
			else if (wcscmp(pwszAttrName, L"showText") == 0)
				bShowText = wcscmp(attributes.getValue(n), L"true") == 0;
			else
				return QSTATUS_FAIL;
		}
		if (!pwszName)
			return QSTATUS_FAIL;
		
		string_ptr<WSTRING> wstrName(allocWString(pwszName));
		if (!wstrName.get())
			return QSTATUS_OUTOFMEMORY;
		
		std::auto_ptr<Toolbar> pToolbar;
		status = newQsObject(pwszName, bShowText, &pToolbar);
		CHECK_QSTATUS();
		status = STLWrapper<ToolbarList>(*pListToolbar_).push_back(pToolbar.get());
		CHECK_QSTATUS();
		pToolbar_ = pToolbar.release();
		
		state_ = STATE_TOOLBAR;
	}
	else if (wcscmp(pwszLocalName, L"button") == 0) {
		if (state_ != STATE_TOOLBAR)
			return QSTATUS_FAIL;
		
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
					return QSTATUS_FAIL;
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
				return QSTATUS_FAIL;
			}
		}
		
		if (nImage == -1 || (!pwszAction && !pwszDropDown))
			return QSTATUS_FAIL;
		
		UINT nAction = getActionId(pwszAction);
		if (nAction != -1) {
			std::auto_ptr<ToolbarButton> pButton;
			status = newQsObject(nImage, pwszText, pwszToolTip,
				nAction, pwszDropDown, &pButton);
			CHECK_QSTATUS();
			pToolbar_->add(pButton.get());
			CHECK_QSTATUS();
			pButton.release();
		}
		
		state_ = STATE_BUTTON;
	}
	else if (wcscmp(pwszLocalName, L"separator") == 0) {
		if (state_ != STATE_TOOLBAR)
			return QSTATUS_FAIL;
		if (attributes.getLength() != 0)
			return QSTATUS_FAIL;
		
		std::auto_ptr<ToolbarSeparator> pSeparator;
		status = newQsObject(&pSeparator);
		CHECK_QSTATUS();
		pToolbar_->add(pSeparator.get());
		CHECK_QSTATUS();
		pSeparator.release();
		
		state_ = STATE_SEPARATOR;
	}
	else {
		return QSTATUS_FAIL;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::ToolbarContentHandler::endElement(const WCHAR* pwszNamespaceURI,
	const WCHAR* pwszLocalName, const WCHAR* pwszQName)
{
	DECLARE_QSTATUS();
	
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
		return QSTATUS_FAIL;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qs::ToolbarContentHandler::characters(
	const WCHAR* pwsz, size_t nStart, size_t nLength)
{
	const WCHAR* p = pwsz + nStart;
	for (size_t n = 0; n < nLength; ++n, ++p) {
		if (*p != L' ' && *p != L'\t' && *p != '\n')
			return QSTATUS_FAIL;
	}
	
	return QSTATUS_SUCCESS;
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
