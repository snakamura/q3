/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmaccount.h>
#include <qmapplication.h>
#include <qmgoround.h>
#include <qmmessage.h>
#include <qmmessageholder.h>

#include <qsassert.h>
#include <qsconv.h>
#include <qserror.h>
#include <qsstl.h>

#include <algorithm>

#include <tchar.h>

#include "foldermodel.h"
#include "menus.h"
#include "resourceinc.h"
#include "uiutil.h"
#include "viewmodel.h"
#include "../model/filter.h"
#include "../model/goround.h"
#include "../model/templatemanager.h"
#include "../script/scriptmanager.h"

#pragma warning(disable:4786)

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * AttachmentMenu
 *
 */

qm::AttachmentMenu::AttachmentMenu(QSTATUS* pstatus)
{
}

qm::AttachmentMenu::~AttachmentMenu()
{
}

QSTATUS qm::AttachmentMenu::getPart(unsigned int nId, Message* pMessage,
	qs::WSTRING* pwstrName, const Part** ppPart) const
{
	assert(pMessage);
	assert(ppPart);
	
	DECLARE_QSTATUS();
	
	List::const_iterator it = list_.begin();
	while (it != list_.end()) {
		if ((*it).first > nId)
			break;
		++it;
	}
	if (it == list_.begin())
		return QSTATUS_FAIL;
	--it;
	
	status = (*it).second->getMessage(
		Account::GETMESSAGEFLAG_ALL, 0, pMessage);
	CHECK_QSTATUS();
	
	AttachmentParser parser(*pMessage);
	AttachmentParser::AttachmentList l;
	AttachmentParser::AttachmentListFree free(l);
	status = parser.getAttachments(false, &l);
	CHECK_QSTATUS();
	if (l.size() < nId - (*it).first)
		return QSTATUS_FAIL;
	
	const AttachmentParser::AttachmentList::value_type& v = l[nId - (*it).first];
	*pwstrName = allocWString(v.first);
	if (!*pwstrName)
		return QSTATUS_OUTOFMEMORY;
	*ppPart = v.second;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::AttachmentMenu::createMenu(HMENU hmenu, const MessageHolderList& l)
{
	assert(hmenu);
	
	DECLARE_QSTATUS();
	
	const UINT nIdNext = IDM_MESSAGE_EXPANDDIGEST;
	
	list_.clear();
	
	while (true) {
		MENUITEMINFO mii = { sizeof(mii), MIIM_TYPE | MIIM_ID };
		if (!::GetMenuItemInfo(hmenu, 3, TRUE, &mii) ||
			mii.wID == nIdNext)
			break;
		::DeleteMenu(hmenu, 3, MF_BYPOSITION);
	}
	
	UINT nId = IDM_MESSAGE_ATTACHMENT;
	MessageHolderList::const_iterator itM = l.begin();
	while (itM != l.end() && nId < IDM_MESSAGE_ATTACHMENT + MAX_ATTACHMENT) {
		MessageHolder* pmh = *itM;
		
		status = STLWrapper<List>(list_).push_back(
			List::value_type(nId, pmh));
		CHECK_QSTATUS();
		
		Message msg(&status);
		CHECK_QSTATUS();
		status = pmh->getMessage(Account::GETMESSAGEFLAG_TEXT, 0, &msg);
		CHECK_QSTATUS();
		
		AttachmentParser parser(msg);
		AttachmentParser::AttachmentList list;
		AttachmentParser::AttachmentListFree free(list);
		status = parser.getAttachments(false, &list);
		CHECK_QSTATUS();
		AttachmentParser::AttachmentList::iterator itA = list.begin();
		while (itA != list.end() &&
			nId < IDM_MESSAGE_ATTACHMENT + MAX_ATTACHMENT) {
			string_ptr<WSTRING> wstrName;
			status = UIUtil::formatMenu((*itA).first, &wstrName);
			CHECK_QSTATUS();
			W2T(wstrName.get(), ptszName);
			::InsertMenu(hmenu, nIdNext,
				MF_BYCOMMAND | MF_STRING, nId++, ptszName);
			++itA;
		}
		
		++itM;
	}
	if (nId != IDM_MESSAGE_ATTACHMENT)
		::InsertMenu(hmenu, nIdNext, MF_BYCOMMAND | MF_SEPARATOR, -1, 0);
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * EncodingMenu
 *
 */

qm::EncodingMenu::EncodingMenu(Profile* pProfile, QSTATUS* pstatus) :
	bMenuCreated_(false)
{
	DECLARE_QSTATUS();
	
	status = load(pProfile);
	CHECK_QSTATUS_SET(pstatus);
}

qm::EncodingMenu::~EncodingMenu()
{
	std::for_each(listEncoding_.begin(),
		listEncoding_.end(), string_free<WSTRING>());
}

const WCHAR* qm::EncodingMenu::getEncoding(unsigned int nId) const
{
	if (nId >= IDM_VIEW_ENCODING) {
		size_t n = nId - IDM_VIEW_ENCODING;
		if (n < listEncoding_.size())
			return listEncoding_[n];
	}
	return 0;
}

QSTATUS qm::EncodingMenu::createMenu(HMENU hmenu)
{
	DECLARE_QSTATUS();
	
	if (bMenuCreated_)
		return QSTATUS_SUCCESS;
	
	for (EncodingList::size_type n = 0; n < listEncoding_.size(); ++n) {
		W2T(listEncoding_[n], ptszEncoding);
		::AppendMenu(hmenu, MF_STRING, IDM_VIEW_ENCODING + n, ptszEncoding);
	}
	bMenuCreated_ = true;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::EncodingMenu::load(Profile* pProfile)
{
	assert(pProfile);
	
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrEncodings;
	status = pProfile->getString(L"Global", L"Encodings",
		L"iso-8859-1 iso-2022-jp shift_jis euc-jp utf-8", &wstrEncodings);
	CHECK_QSTATUS();
	
	WCHAR* p = wcstok(wstrEncodings.get(), L" ");
	while (p && listEncoding_.size() < MAX_ENCODING) {
		string_ptr<WSTRING> wstrEncoding(allocWString(p));
		if (!wstrEncoding.get())
			return QSTATUS_OUTOFMEMORY;
		status = STLWrapper<EncodingList>(
			listEncoding_).push_back(wstrEncoding.get());
		CHECK_QSTATUS();
		wstrEncoding.release();
		
		p = wcstok(0, L" ");
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * FilterMenu
 *
 */

qm::FilterMenu::FilterMenu(FilterManager* pFilterManager, QSTATUS* pstatus) :
	pFilterManager_(pFilterManager)
{
}

qm::FilterMenu::~FilterMenu()
{
}

QSTATUS qm::FilterMenu::getFilter(unsigned int nId, const Filter** ppFilter) const
{
	assert(ppFilter);
	
	DECLARE_QSTATUS();
	
	*ppFilter = 0;
	
	const FilterManager::FilterList* pList = 0;
	status = pFilterManager_->getFilters(&pList);
	CHECK_QSTATUS();
	
	if (IDM_VIEW_FILTER <= nId && nId < IDM_VIEW_FILTER + pList->size())
		*ppFilter = (*pList)[nId - IDM_VIEW_FILTER];
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::FilterMenu::createMenu(HMENU hmenu)
{
	DECLARE_QSTATUS();
	
	MENUITEMINFO mii = { sizeof(mii), MIIM_TYPE | MIIM_ID };
	while (true) {
		::GetMenuItemInfo(hmenu, 2, TRUE, &mii);
		if (mii.wID == IDM_VIEW_FILTERCUSTOM)
			break;
		::DeleteMenu(hmenu, 2, MF_BYPOSITION);
	}
	
	UINT nPos = 2;
	UINT nId = IDM_VIEW_FILTER;
	const FilterManager::FilterList* pList = 0;
	status = pFilterManager_->getFilters(&pList);
	CHECK_QSTATUS();
	FilterManager::FilterList::const_iterator it = pList->begin();
	while (it != pList->end() && nId < IDM_VIEW_FILTER + MAX_FILTER) {
		const Filter* pFilter = *it;
		string_ptr<WSTRING> wstrTitle;
		status = UIUtil::formatMenu(pFilter->getName(), &wstrTitle);
		CHECK_QSTATUS();
		W2T(wstrTitle.get(), ptszTitle);
		::InsertMenu(hmenu, nPos, MF_BYPOSITION, nId, ptszTitle);
		++nId;
		++nPos;
		++it;
	}
	if (nPos != 2)
		::InsertMenu(hmenu, nPos, MF_BYPOSITION | MF_SEPARATOR, -1, 0);
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * GoRoundMenu
 *
 */

qm::GoRoundMenu::GoRoundMenu(GoRound* pGoRound, QSTATUS* pstatus) :
	pGoRound_(pGoRound)
{
}

qm::GoRoundMenu::~GoRoundMenu()
{
}

QSTATUS qm::GoRoundMenu::getCourse(unsigned int nId,
	const GoRoundCourse** ppCourse) const
{
	assert(ppCourse);
	
	DECLARE_QSTATUS();
	
	*ppCourse = 0;
	
	GoRoundCourseList* pCourseList = 0;
	status = pGoRound_->getCourseList(&pCourseList);
	CHECK_QSTATUS();
	if (pCourseList && pCourseList->getCount() > nId - IDM_TOOL_GOROUND)
		*ppCourse = pCourseList->getCourse(nId - IDM_TOOL_GOROUND);
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::GoRoundMenu::createMenu(HMENU hmenu)
{
	DECLARE_QSTATUS();
	
	UINT nId = IDM_TOOL_GOROUND;
	while (::DeleteMenu(hmenu, nId++, MF_BYCOMMAND));
	
	GoRoundCourseList* pList = 0;
	status = pGoRound_->getCourseList(&pList);
	CHECK_QSTATUS();
	
	if (pList && pList->getCount() > 0) {
		for (size_t n = 0; n < pList->getCount() && n < MAX_COURSE; ++n) {
			GoRoundCourse* pCourse = pList->getCourse(n);
			string_ptr<WSTRING> wstrName;
			status = UIUtil::formatMenu(pCourse->getName(), &wstrName);
			CHECK_QSTATUS();
			W2T(wstrName.get(), ptszName);
			::AppendMenu(hmenu, MF_STRING, IDM_TOOL_GOROUND + n, ptszName);
		}
	}
	else {
		string_ptr<WSTRING> wstrName;
		status = loadString(Application::getApplication().getResourceHandle(),
			IDS_GOROUND, &wstrName);
		CHECK_QSTATUS();
		W2T(wstrName.get(), ptszName);
		::AppendMenu(hmenu, MF_STRING, IDM_TOOL_GOROUND, ptszName);
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * MoveMenu
 *
 */

qm::MoveMenu::MoveMenu(QSTATUS* pstatus)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qm::MoveMenu::~MoveMenu()
{
}

NormalFolder* qm::MoveMenu::getFolder(unsigned int nId) const
{
	if (nId >= IDM_MESSAGE_MOVE + mapMenu_.size())
		return 0;
	else
		return mapMenu_[nId - IDM_MESSAGE_MOVE];
}

QSTATUS qm::MoveMenu::createMenu(HMENU hmenu, Account* pAccount,
	bool bShowHidden, const ActionMap& actionMap)
{
	assert(hmenu);
	assert(pAccount);
	
	DECLARE_QSTATUS();
	
	mapMenu_.clear();
	while (true) {
		MENUITEMINFO mii = { sizeof(mii), MIIM_TYPE };
		if (!::GetMenuItemInfo(hmenu, 0, TRUE, &mii) ||
			(mii.fType & MFT_SEPARATOR) != 0)
			break;
		::DeleteMenu(hmenu, 0, MF_BYPOSITION);
	}
	
	Action* pAction = actionMap.getAction(IDM_MESSAGE_MOVE);
	bool bEnabled = false;
	status = pAction->isEnabled(ActionEvent(IDM_MESSAGE_MOVE, 0), &bEnabled);
	CHECK_QSTATUS();
	
	const Account::FolderList& l = pAccount->getFolders();
	Account::FolderList listFolder;
	status = STLWrapper<Account::FolderList>(listFolder).reserve(l.size());
	CHECK_QSTATUS();
	if (bShowHidden)
		std::copy(l.begin(), l.end(), std::back_inserter(listFolder));
	else
		std::remove_copy_if(l.begin(), l.end(),
			std::back_inserter(listFolder), std::mem_fun(&Folder::isHidden));
	std::sort(listFolder.begin(), listFolder.end(), FolderLess());
	
	typedef std::vector<MenuInserter> FolderStack;
	FolderStack stackFolder;
	status = STLWrapper<FolderStack>(stackFolder).push_back(
		MenuInserter(hmenu, 0));
	CHECK_QSTATUS();
	
	string_ptr<WSTRING> wstrThisFolder;
	status = loadString(Application::getApplication().getResourceHandle(),
		IDS_THISFOLDER, &wstrThisFolder);
	CHECK_QSTATUS();
	W2T(wstrThisFolder.get(), ptszThisFolder);
	
	Account::FolderList::const_iterator it = listFolder.begin();
	while (it != listFolder.end() && mapMenu_.size() < MAX_FOLDER) {
		Folder* pFolder = *it;
		
		Folder* pParent = pFolder->getParentFolder();
		while (!stackFolder.empty() && stackFolder.back().pFolder_ != pParent)
			stackFolder.pop_back();
		assert(!stackFolder.empty());
		
		HMENU hmenuThis = stackFolder.back().hmenu_;
		
		string_ptr<WSTRING> wstrName;
		status = formatName(pFolder, stackFolder.back().nCount_, &wstrName);
		W2T(wstrName.get(), ptszName);
		
		bool bHasChild = hasSelectableChildNormalFolder(it, listFolder.end());
		if (bHasChild) {
			HMENU hmenuNew = ::CreatePopupMenu();
			if (!hmenuNew)
				return QSTATUS_FAIL;
			bool bAddThisFolder = isMovableFolder(pFolder);
			if (bAddThisFolder) {
				if (!::AppendMenu(hmenuNew, MF_STRING,
					IDM_MESSAGE_MOVE + mapMenu_.size(), ptszThisFolder))
					return QSTATUS_FAIL;
				status = STLWrapper<MenuMap>(mapMenu_).push_back(
					static_cast<NormalFolder*>(pFolder));
				CHECK_QSTATUS();
			}
			if (!::InsertMenu(hmenuThis, stackFolder.back().nCount_,
				MF_POPUP | MF_BYPOSITION,
				reinterpret_cast<UINT_PTR>(hmenuNew), ptszName))
				return QSTATUS_FAIL;
			if (!bEnabled)
				::EnableMenuItem(hmenuThis, stackFolder.back().nCount_,
					MF_BYPOSITION | MF_GRAYED);
			++stackFolder.back().nCount_;
			status = STLWrapper<FolderStack>(stackFolder).push_back(
				MenuInserter(hmenuNew, pFolder));
			CHECK_QSTATUS();
			if (bAddThisFolder)
				++stackFolder.back().nCount_;
		}
		else if (isMovableFolder(pFolder)) {
			if (!::InsertMenu(hmenuThis, stackFolder.back().nCount_,
				MF_STRING | MF_BYPOSITION,
				IDM_MESSAGE_MOVE + mapMenu_.size(), ptszName))
				return QSTATUS_FAIL;
			if (!bEnabled)
				::EnableMenuItem(hmenuThis, stackFolder.back().nCount_,
					MF_BYPOSITION | MF_GRAYED);
			++stackFolder.back().nCount_;
			status = STLWrapper<MenuMap>(mapMenu_).push_back(
				static_cast<NormalFolder*>(pFolder));
			CHECK_QSTATUS();
		}
		
		++it;
		if (!bHasChild) {
			while (it != listFolder.end() && pFolder->isAncestorOf(*it))
				++it;
		}
	}
	
	return QSTATUS_SUCCESS;
}

bool qm::MoveMenu::isMovableFolder(const Folder* pFolder)
{
	return pFolder->getType() == Folder::TYPE_NORMAL &&
		!pFolder->isFlag(Folder::FLAG_NOSELECT);
}

bool qm::MoveMenu::hasSelectableChildNormalFolder(
	Account::FolderList::const_iterator first,
	Account::FolderList::const_iterator last)
{
	assert(first != last);
	
	const Folder* pFolder = *first;
	for (++first; first != last && pFolder->isAncestorOf(*first); ++first) {
		if ((*first)->getType() == Folder::TYPE_NORMAL &&
			!(*first)->isFlag(Folder::FLAG_NOSELECT))
			return true;
	}
	
	return false;
}

QSTATUS qm::MoveMenu::formatName(const Folder* pFolder,
	unsigned int n, WSTRING* pwstrName)
{
	assert(pFolder);
	assert(pwstrName);
	
	return UIUtil::formatMenu(pFolder->getName(), pwstrName);
}


/****************************************************************************
 *
 * MoveMenu::MenuInserter
 *
 */

qm::MoveMenu::MenuInserter::MenuInserter(HMENU hmenu, Folder* pFolder) :
	hmenu_(hmenu), pFolder_(pFolder), nCount_(0)
{
}


/****************************************************************************
 *
 * ScriptMenu
 *
 */

qm::ScriptMenu::ScriptMenu(ScriptManager* pScriptManager, QSTATUS* pstatus) :
	pScriptManager_(pScriptManager)
{
}

qm::ScriptMenu::~ScriptMenu()
{
	clear();
}

const WCHAR* qm::ScriptMenu::getScript(unsigned int nId) const
{
	if (nId >= IDM_TOOL_SCRIPT + list_.size())
		return 0;
	else
		return list_[nId - IDM_TOOL_SCRIPT];
}

ScriptManager* qm::ScriptMenu::getScriptManager() const
{
	return pScriptManager_;
}

QSTATUS qm::ScriptMenu::createMenu(HMENU hmenu)
{
	assert(hmenu);
	
	DECLARE_QSTATUS();
	
	while (::DeleteMenu(hmenu, 0, MF_BYPOSITION));
	
	clear();
	
	ScriptManager::NameList l;
	StringListFree<ScriptManager::NameList> free(l);
	status = pScriptManager_->getScriptNames(&l);
	CHECK_QSTATUS();
	
	UINT nId = IDM_TOOL_SCRIPT;
	
	ScriptManager::NameList::iterator it = l.begin();
	while (it != l.end() && list_.size() < MAX_SCRIPT) {
		string_ptr<WSTRING> wstrMenu;
		status = UIUtil::formatMenu(*it, &wstrMenu);
		CHECK_QSTATUS();
		W2T(wstrMenu.get(), ptszMenu);
		::AppendMenu(hmenu, MF_STRING, nId, ptszMenu);
		status = STLWrapper<List>(list_).push_back(*it);
		CHECK_QSTATUS();
		*it = 0;
		++nId;
		++it;
	}
	
	if (l.empty()) {
		string_ptr<WSTRING> wstrNone;
		status = loadString(Application::getApplication().getResourceHandle(),
			IDS_SCRIPTNONE, &wstrNone);
		CHECK_QSTATUS();
		W2T(wstrNone.get(), ptszNone);
		::AppendMenu(hmenu, MF_STRING, IDM_TOOL_SCRIPTNONE, ptszNone);
	}
	
	return QSTATUS_SUCCESS;
}

void qm::ScriptMenu::clear()
{
	std::for_each(list_.begin(), list_.end(), string_free<WSTRING>());
	list_.clear();
}


/****************************************************************************
 *
 * SubAccountMenu
 *
 */

qm::SubAccountMenu::SubAccountMenu(FolderModel* pFolderModel, QSTATUS* pstatus) :
	pFolderModel_(pFolderModel)
{
}

qm::SubAccountMenu::~SubAccountMenu()
{
}

const WCHAR* qm::SubAccountMenu::getName(unsigned int nId) const
{
	const WCHAR* pwszName = 0;
	
	Account* pAccount = pFolderModel_->getCurrentAccount();
	if (!pAccount) {
		Folder* pFolder = pFolderModel_->getCurrentFolder();
		if (pFolder)
			pAccount = pFolder->getAccount();
	}
	if (pAccount) {
		const Account::SubAccountList& listSubAccount = pAccount->getSubAccounts();
		if (nId - IDM_TOOL_SUBACCOUNT < listSubAccount.size()) {
			SubAccount* pSubAccount = listSubAccount[nId - IDM_TOOL_SUBACCOUNT];
			pwszName = pSubAccount->getName();
		}
	}
	
	return pwszName;
}

QSTATUS qm::SubAccountMenu::createMenu(HMENU hmenu)
{
	DECLARE_QSTATUS();
	
	UINT nId = IDM_TOOL_SUBACCOUNT + 1;
	while (::DeleteMenu(hmenu, nId++, MF_BYCOMMAND));
	
	Account* pAccount = pFolderModel_->getCurrentAccount();
	if (!pAccount) {
		Folder* pFolder = pFolderModel_->getCurrentFolder();
		if (pFolder)
			pAccount = pFolder->getAccount();
	}
	
	if (pAccount) {
		const Account::SubAccountList& l = pAccount->getSubAccounts();
		assert(!l.empty());
		Account::SubAccountList::size_type n = 1;
		while (n < l.size() && n < MAX_SUBACCOUNT) {
			SubAccount* pSubAccount = l[n];
			string_ptr<WSTRING> wstrText;
			status = UIUtil::formatMenu(pSubAccount->getName(), &wstrText);
			W2T(wstrText.get(), ptszName);
			::AppendMenu(hmenu, MF_STRING, IDM_TOOL_SUBACCOUNT + n, ptszName);
			++n;
		}
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * SortMenu
 *
 */

qm::SortMenu::SortMenu(ViewModelManager* pViewModelManager, QSTATUS* pstatus) :
	pViewModelManager_(pViewModelManager)
{
}

qm::SortMenu::~SortMenu()
{
}

unsigned int qm::SortMenu::getSort(unsigned int nId) const
{
	unsigned int nSort = 0;
	
	ViewModel* pViewModel = pViewModelManager_->getCurrentViewModel();
	if (pViewModel) {
		nSort = pViewModel->getSort();
		nSort &= ~ViewModel::SORT_INDEX_MASK;
		nSort &= ~ViewModel::SORT_DIRECTION_MASK;
		nSort |= nId - IDM_VIEW_SORT;
	}
	
	return nSort;
}

QSTATUS qm::SortMenu::createMenu(HMENU hmenu)
{
	DECLARE_QSTATUS();
	
	MENUITEMINFO mii = { sizeof(mii), MIIM_TYPE | MIIM_ID };
	while (true) {
		::GetMenuItemInfo(hmenu, 0, TRUE, &mii);
		if (mii.fType & MFT_SEPARATOR)
			break;
		::DeleteMenu(hmenu, 0, MF_BYPOSITION);
	}
	
	ViewModel* pViewModel = pViewModelManager_->getCurrentViewModel();
	if (pViewModel) {
		unsigned int nSortIndex = pViewModel->getSort() & ViewModel::SORT_INDEX_MASK;
		UINT nPos = 0;
		UINT nId = IDM_VIEW_SORT;
		const ViewModel::ColumnList& l = pViewModel->getColumns();
		ViewModel::ColumnList::const_iterator it = l.begin();
		while (it != l.end() && nId < IDM_VIEW_SORT + MAX_SORT) {
			const WCHAR* pwszTitle = (*it)->getTitle();
			if (*pwszTitle) {
				string_ptr<WSTRING> wstrTitle;
				status = UIUtil::formatMenu(pwszTitle, &wstrTitle);
				CHECK_QSTATUS();
				W2T(wstrTitle.get(), ptszTitle);
				::InsertMenu(hmenu, nPos, MF_BYPOSITION, nId, ptszTitle);
				++nPos;
			}
			if (nId - IDM_VIEW_SORT == nSortIndex)
				::CheckMenuItem(hmenu, nId, MF_BYCOMMAND | MF_CHECKED);
			++nId;
			++it;
		}
	}
	else {
		string_ptr<WSTRING> wstrNone;
		status = loadString(Application::getApplication().getResourceHandle(),
			IDS_NONE, &wstrNone);
		CHECK_QSTATUS();
		W2T(wstrNone.get(), ptszNone);
		::InsertMenu(hmenu, 0, MF_BYPOSITION, IDM_VIEW_SORT, ptszNone);
		::EnableMenuItem(hmenu, IDM_VIEW_SORT, MF_BYCOMMAND | MF_GRAYED);
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * TemplateMenu
 *
 */

qm::TemplateMenu::TemplateMenu(const TemplateManager* pTemplateManager,
	QSTATUS* pstatus) :
	pTemplateManager_(pTemplateManager)
{
}

qm::TemplateMenu::~TemplateMenu()
{
	clear();
}

const WCHAR* qm::TemplateMenu::getTemplate(unsigned int nId) const
{
	if (nId >= getId() + list_.size())
		return 0;
	else
		return list_[nId - getId()];
}

QSTATUS qm::TemplateMenu::createMenu(HMENU hmenu, Account* pAccount)
{
	assert(hmenu);
	
	DECLARE_QSTATUS();
	
	const WCHAR* pwszPrefix = getPrefix();
	
	int nBase = getBase();
	while (::DeleteMenu(hmenu, nBase, MF_BYPOSITION));
	
	clear();
	
	TemplateManager::NameList l;
	StringListFree<TemplateManager::NameList> free(l);
	status = pTemplateManager_->getTemplateNames(pAccount, pwszPrefix, &l);
	CHECK_QSTATUS();
	
	UINT nId = getId();
	
	TemplateManager::NameList::iterator it = l.begin();
	while (it != l.end() && list_.size() < MAX_TEMPLATE) {
		string_ptr<WSTRING> wstrMenu;
		status = UIUtil::formatMenu(*it + wcslen(pwszPrefix) + 1, &wstrMenu);
		CHECK_QSTATUS();
		W2T(wstrMenu.get(), ptszName);
		::AppendMenu(hmenu, MF_STRING, nId, ptszName);
		status = STLWrapper<List>(list_).push_back(*it);
		CHECK_QSTATUS();
		*it = 0;
		++nId;
		++it;
	}
	
	if (l.empty() && getNoneId()) {
		UINT nId = getNoneId();
		if (nId) {
			string_ptr<WSTRING> wstrNone;
			status = loadString(Application::getApplication().getResourceHandle(),
				IDS_TEMPLATENONE, &wstrNone);
			CHECK_QSTATUS();
			W2T(wstrNone.get(), ptszNone);
			::AppendMenu(hmenu, MF_STRING, nId, ptszNone);
		}
	}
	
	return QSTATUS_SUCCESS;
}

void qm::TemplateMenu::clear()
{
	std::for_each(list_.begin(), list_.end(), string_free<WSTRING>());
	list_.clear();
}


/****************************************************************************
 *
 * CreateTemplateMenu
 *
 */

qm::CreateTemplateMenu::CreateTemplateMenu(const TemplateManager* pTemplateManager,
	bool bExternalEditor, QSTATUS* pstatus) :
	TemplateMenu(pTemplateManager, pstatus),
	bExternalEditor_(bExternalEditor)
{
}

qm::CreateTemplateMenu::~CreateTemplateMenu()
{
}

const WCHAR* qm::CreateTemplateMenu::getPrefix() const
{
	return L"create";
}

UINT qm::CreateTemplateMenu::getId() const
{
	return !bExternalEditor_ ? IDM_MESSAGE_APPLYTEMPLATE :
		IDM_MESSAGE_APPLYTEMPLATEEXTERNAL;
}

UINT qm::CreateTemplateMenu::getNoneId() const
{
	return !bExternalEditor_ ? IDM_MESSAGE_APPLYTEMPLATENONE :
		IDM_MESSAGE_APPLYTEMPLATENONEEXTERNAL;
}

int qm::CreateTemplateMenu::getBase() const
{
	return 0;
}


/****************************************************************************
 *
 * ViewTemplateMenu
 *
 */

qm::ViewTemplateMenu::ViewTemplateMenu(
	const TemplateManager* pTemplateManager, QSTATUS* pstatus) :
	TemplateMenu(pTemplateManager, pstatus)
{
}

qm::ViewTemplateMenu::~ViewTemplateMenu()
{
}

const WCHAR* qm::ViewTemplateMenu::getPrefix() const
{
	return L"view";
}

UINT qm::ViewTemplateMenu::getId() const
{
	return IDM_VIEW_TEMPLATE;
}

UINT qm::ViewTemplateMenu::getNoneId() const
{
	return 0;
}

int qm::ViewTemplateMenu::getBase() const
{
	return 2;
}
