/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmaccount.h>
#include <qmapplication.h>
#include <qmgoround.h>
#include <qmmessage.h>
#include <qmmessageholder.h>
#include <qmrecents.h>

#include <qsassert.h>
#include <qsconv.h>
#include <qsstl.h>

#include <algorithm>

#include <tchar.h>

#include "foldermodel.h"
#include "menus.h"
#include "resourceinc.h"
#include "securitymodel.h"
#include "uiutil.h"
#include "viewmodel.h"
#include "../model/filter.h"
#include "../model/fixedformtext.h"
#include "../model/goround.h"
#include "../model/templatemanager.h"
#include "../model/uri.h"
#include "../script/scriptmanager.h"

#pragma warning(disable:4786)

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * AttachmentMenu
 *
 */

qm::AttachmentMenu::AttachmentMenu(SecurityModel* pSecurityModel) :
	pSecurityModel_(pSecurityModel)
{
}

qm::AttachmentMenu::~AttachmentMenu()
{
}

bool qm::AttachmentMenu::getPart(unsigned int nId,
								 Message* pMessage,
								 wstring_ptr* pwstrName,
								 const Part** ppPart) const
{
	assert(pMessage);
	assert(ppPart);
	
	List::const_iterator it = list_.begin();
	while (it != list_.end()) {
		if ((*it).first > nId)
			break;
		++it;
	}
	if (it == list_.begin())
		return false;
	--it;
	
	MessagePtrLock mpl((*it).second);
	if (!mpl)
		return false;
	if (!mpl->getMessage(Account::GETMESSAGEFLAG_ALL,
		0, pSecurityModel_->getSecurityMode(), pMessage))
		return false;
	
	AttachmentParser parser(*pMessage);
	AttachmentParser::AttachmentList l;
	AttachmentParser::AttachmentListFree free(l);
	parser.getAttachments(false, &l);
	if (l.size() < nId - (*it).first)
		return false;
	
	const AttachmentParser::AttachmentList::value_type& v = l[nId - (*it).first];
	*pwstrName = allocWString(v.first);
	*ppPart = v.second;
	
	return true;
}

bool qm::AttachmentMenu::createMenu(HMENU hmenu,
									const MessagePtr& ptr)
{
	assert(hmenu);
	
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
	
	MessagePtrLock mpl(ptr);
	if (mpl) {
		list_.push_back(List::value_type(nId, ptr));
		
		Message msg;
		if (!mpl->getMessage(Account::GETMESSAGEFLAG_TEXT,
			0, pSecurityModel_->getSecurityMode(), &msg))
			return false;
		
		AttachmentParser parser(msg);
		AttachmentParser::AttachmentList list;
		AttachmentParser::AttachmentListFree free(list);
		parser.getAttachments(false, &list);
		
		for (AttachmentParser::AttachmentList::iterator itA = list.begin();
			itA != list.end() && nId < IDM_MESSAGE_ATTACHMENT + MAX_ATTACHMENT; ++itA) {
			wstring_ptr wstrName(UIUtil::formatMenu((*itA).first));
			W2T(wstrName.get(), ptszName);
			::InsertMenu(hmenu, nIdNext, MF_BYCOMMAND | MF_STRING, nId++, ptszName);
		}
		
		if (nId != IDM_MESSAGE_ATTACHMENT)
			::InsertMenu(hmenu, nIdNext, MF_BYCOMMAND | MF_SEPARATOR, -1, 0);
	}
	
	return true;
}


/****************************************************************************
 *
 * EncodingMenu
 *
 */

qm::EncodingMenu::EncodingMenu(Profile* pProfile) :
	bMenuCreated_(false)
{
	load(pProfile);
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

bool qm::EncodingMenu::createMenu(HMENU hmenu)
{
	if (bMenuCreated_)
		return true;
	
	for (EncodingList::size_type n = 0; n < listEncoding_.size(); ++n) {
		W2T(listEncoding_[n], ptszEncoding);
		::AppendMenu(hmenu, MF_STRING, IDM_VIEW_ENCODING + n, ptszEncoding);
	}
	bMenuCreated_ = true;
	
	return true;
}

void qm::EncodingMenu::load(Profile* pProfile)
{
	UIUtil::loadEncodings(pProfile, &listEncoding_);
	
	if (listEncoding_.size() > MAX_ENCODING) {
		for (EncodingList::size_type n = MAX_ENCODING; n < listEncoding_.size(); ++n)
			freeWString(listEncoding_[n]);
		listEncoding_.resize(MAX_ENCODING);
	}
}


/****************************************************************************
 *
 * FilterMenu
 *
 */

qm::FilterMenu::FilterMenu(FilterManager* pFilterManager) :
	pFilterManager_(pFilterManager)
{
}

qm::FilterMenu::~FilterMenu()
{
}

const Filter* qm::FilterMenu::getFilter(unsigned int nId) const
{
	const FilterManager::FilterList& l = pFilterManager_->getFilters();
	
	if (IDM_VIEW_FILTER <= nId && nId < IDM_VIEW_FILTER + l.size())
		return l[nId - IDM_VIEW_FILTER];
	else
		return 0;
}

bool qm::FilterMenu::createMenu(HMENU hmenu)
{
	MENUITEMINFO mii = { sizeof(mii), MIIM_TYPE | MIIM_ID };
	while (true) {
		::GetMenuItemInfo(hmenu, 2, TRUE, &mii);
		if (mii.wID == IDM_VIEW_FILTERCUSTOM)
			break;
		::DeleteMenu(hmenu, 2, MF_BYPOSITION);
	}
	
	UINT nPos = 2;
	UINT nId = IDM_VIEW_FILTER;
	const FilterManager::FilterList& l = pFilterManager_->getFilters();
	for (FilterManager::FilterList::const_iterator it = l.begin();
		it != l.end() && nId < IDM_VIEW_FILTER + MAX_FILTER; ++it, ++nId, ++nPos) {
		const Filter* pFilter = *it;
		wstring_ptr wstrTitle(UIUtil::formatMenu(pFilter->getName()));
		W2T(wstrTitle.get(), ptszTitle);
		::InsertMenu(hmenu, nPos, MF_BYPOSITION | MF_STRING, nId, ptszTitle);
	}
	if (nPos != 2)
		::InsertMenu(hmenu, nPos, MF_BYPOSITION | MF_SEPARATOR, -1, 0);
	
	return true;
}


/****************************************************************************
 *
 * GoRoundMenu
 *
 */

qm::GoRoundMenu::GoRoundMenu(GoRound* pGoRound) :
	pGoRound_(pGoRound)
{
}

qm::GoRoundMenu::~GoRoundMenu()
{
}

const GoRoundCourse* qm::GoRoundMenu::getCourse(unsigned int nId) const
{
	const GoRound::CourseList& l = pGoRound_->getCourses();
	if (IDM_TOOL_GOROUND <= nId && nId < IDM_TOOL_GOROUND + l.size())
		return l[nId - IDM_TOOL_GOROUND];
	else
		return 0;
}

bool qm::GoRoundMenu::createMenu(HMENU hmenu)
{
	while (true) {
		MENUITEMINFO mii = { sizeof(mii), MIIM_TYPE };
		if (!::GetMenuItemInfo(hmenu, 0, TRUE, &mii) ||
			(mii.fType & MFT_SEPARATOR) != 0)
			break;
		::DeleteMenu(hmenu, 0, MF_BYPOSITION);
	}
	
	const GoRound::CourseList& l = pGoRound_->getCourses();
	if (!l.empty()) {
		for (GoRound::CourseList::size_type n = 0; n < l.size() && n < MAX_COURSE; ++n) {
			GoRoundCourse* pCourse = l[n];
			wstring_ptr wstrName(UIUtil::formatMenu(pCourse->getName()));
			W2T(wstrName.get(), ptszName);
			::InsertMenu(hmenu, n, MF_STRING | MF_BYPOSITION, IDM_TOOL_GOROUND + n, ptszName);
		}
	}
	else {
		HINSTANCE hInst = Application::getApplication().getResourceHandle();
		wstring_ptr wstrName(loadString(hInst, IDS_GOROUND));
		W2T(wstrName.get(), ptszName);
		::InsertMenu(hmenu, 0, MF_STRING | MF_BYPOSITION, IDM_TOOL_GOROUND, ptszName);
	}
	
	return true;
}


/****************************************************************************
 *
 * InsertTextMenu
 *
 */

qm::InsertTextMenu::InsertTextMenu(FixedFormTextManager* pManager) :
	pManager_(pManager)
{
}

qm::InsertTextMenu::~InsertTextMenu()
{
}

const FixedFormText* qm::InsertTextMenu::getText(unsigned int nId) const
{
	const FixedFormTextManager::TextList& l = pManager_->getTexts();
	if (IDM_TOOL_INSERTTEXT <= nId && nId < IDM_TOOL_INSERTTEXT + l.size())
		return l[nId - IDM_TOOL_INSERTTEXT];
	else
		return 0;
}

bool qm::InsertTextMenu::createMenu(HMENU hmenu)
{
	while (true) {
		MENUITEMINFO mii = { sizeof(mii), MIIM_TYPE };
		if (!::GetMenuItemInfo(hmenu, 0, TRUE, &mii) ||
			(mii.fType & MFT_SEPARATOR) != 0)
			break;
		::DeleteMenu(hmenu, 0, MF_BYPOSITION);
	}
	
	const FixedFormTextManager::TextList& l = pManager_->getTexts();
	if (!l.empty()) {
		for (FixedFormTextManager::TextList::size_type n = 0; n < l.size(); ++n) {
			const FixedFormText* pText = l[n];
			wstring_ptr wstrName(UIUtil::formatMenu(pText->getName()));
			W2T(wstrName.get(), ptszName);
			::InsertMenu(hmenu, n, MF_STRING | MF_BYPOSITION, IDM_TOOL_INSERTTEXT + n, ptszName);
		}
	}
	else {
		HINSTANCE hInst = Application::getApplication().getResourceHandle();
		wstring_ptr wstrNone(loadString(hInst, IDS_NONE));
		W2T(wstrNone.get(), ptszNone);
		::InsertMenu(hmenu, 0, MF_STRING | MF_BYPOSITION, IDM_TOOL_INSERTTEXTNONE, ptszNone);
		::EnableMenuItem(hmenu, IDM_TOOL_INSERTTEXTNONE, MF_BYCOMMAND | MF_GRAYED);
	}
	
	return true;
}


/****************************************************************************
 *
 * MoveMenu
 *
 */

qm::MoveMenu::MoveMenu()
{
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

bool qm::MoveMenu::createMenu(HMENU hmenu,
							  Account* pAccount,
							  bool bShowHidden,
							  const ActionMap& actionMap)
{
	assert(hmenu);
	assert(pAccount);
	
	mapMenu_.clear();
	while (true) {
		MENUITEMINFO mii = { sizeof(mii), MIIM_TYPE };
		if (!::GetMenuItemInfo(hmenu, 0, TRUE, &mii) ||
			(mii.fType & MFT_SEPARATOR) != 0)
			break;
		::DeleteMenu(hmenu, 0, MF_BYPOSITION);
	}
	
	Action* pAction = actionMap.getAction(IDM_MESSAGE_MOVE);
	bool bEnabled = pAction->isEnabled(ActionEvent(IDM_MESSAGE_MOVE, 0));
	
	const Account::FolderList& l = pAccount->getFolders();
	Account::FolderList listFolder;
	listFolder.reserve(l.size());
	if (bShowHidden)
		std::copy(l.begin(), l.end(), std::back_inserter(listFolder));
	else
		std::remove_copy_if(l.begin(), l.end(),
			std::back_inserter(listFolder), std::mem_fun(&Folder::isHidden));
	std::stable_sort(listFolder.begin(), listFolder.end(), FolderLess());
	
	typedef std::vector<MenuInserter> FolderStack;
	FolderStack stackFolder;
	stackFolder.push_back(MenuInserter(hmenu, 0));
	
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	wstring_ptr wstrThisFolder(loadString(hInst, IDS_THISFOLDER));
	W2T(wstrThisFolder.get(), ptszThisFolder);
	
	for (Account::FolderList::const_iterator it = listFolder.begin();
		it != listFolder.end() && mapMenu_.size() < MAX_FOLDER; ) {
		Folder* pFolder = *it;
		
		Folder* pParent = pFolder->getParentFolder();
		while (!stackFolder.empty() && stackFolder.back().pFolder_ != pParent)
			stackFolder.pop_back();
		assert(!stackFolder.empty());
		
		HMENU hmenuThis = stackFolder.back().hmenu_;
		
		wstring_ptr wstrName(formatName(pFolder, stackFolder.back().nCount_));
		W2T(wstrName.get(), ptszName);
		
		bool bHasChild = hasSelectableChildNormalFolder(it, listFolder.end());
		if (bHasChild) {
			HMENU hmenuNew = ::CreatePopupMenu();
			if (!hmenuNew)
				return false;
			bool bAddThisFolder = isMovableFolder(pFolder);
			if (bAddThisFolder) {
				if (!::AppendMenu(hmenuNew, MF_STRING,
					IDM_MESSAGE_MOVE + mapMenu_.size(), ptszThisFolder))
					return false;
				mapMenu_.push_back(static_cast<NormalFolder*>(pFolder));
			}
			if (!::InsertMenu(hmenuThis, stackFolder.back().nCount_,
				MF_POPUP | MF_BYPOSITION,
				reinterpret_cast<UINT_PTR>(hmenuNew), ptszName))
				return false;
			if (!bEnabled)
				::EnableMenuItem(hmenuThis, stackFolder.back().nCount_,
					MF_BYPOSITION | MF_GRAYED);
			++stackFolder.back().nCount_;
			stackFolder.push_back(MenuInserter(hmenuNew, pFolder));
			if (bAddThisFolder)
				++stackFolder.back().nCount_;
		}
		else if (isMovableFolder(pFolder)) {
			if (!::InsertMenu(hmenuThis, stackFolder.back().nCount_,
				MF_STRING | MF_BYPOSITION,
				IDM_MESSAGE_MOVE + mapMenu_.size(), ptszName))
				return false;
			if (!bEnabled)
				::EnableMenuItem(hmenuThis, stackFolder.back().nCount_,
					MF_BYPOSITION | MF_GRAYED);
			++stackFolder.back().nCount_;
			mapMenu_.push_back(static_cast<NormalFolder*>(pFolder));
		}
		
		++it;
		if (!bHasChild) {
			while (it != listFolder.end() && pFolder->isAncestorOf(*it))
				++it;
		}
	}
	
	return true;
}

bool qm::MoveMenu::isMovableFolder(const Folder* pFolder)
{
	return pFolder->getType() == Folder::TYPE_NORMAL &&
		!pFolder->isFlag(Folder::FLAG_NOSELECT);
}

bool qm::MoveMenu::hasSelectableChildNormalFolder(Account::FolderList::const_iterator first,
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

wstring_ptr qm::MoveMenu::formatName(const Folder* pFolder,
									 unsigned int n)
{
	assert(pFolder);
	return UIUtil::formatMenu(pFolder->getName());
}


/****************************************************************************
 *
 * MoveMenu::MenuInserter
 *
 */

qm::MoveMenu::MenuInserter::MenuInserter(HMENU hmenu,
										 Folder* pFolder) :
	hmenu_(hmenu),
	pFolder_(pFolder),
	nCount_(0)
{
}


/****************************************************************************
 *
 * RecentsMenu
 *
 */

qm::RecentsMenu::RecentsMenu(Document* pDocument) :
	pDocument_(pDocument)
{
}

qm::RecentsMenu::~RecentsMenu()
{
	clear();
}

const WCHAR* qm::RecentsMenu::getURI(unsigned int nId) const
{
	assert(IDM_MESSAGE_OPENRECENT <= nId && nId < IDM_MESSAGE_OPENRECENT + MAX_RECENTS);
	return listURI_[nId - IDM_MESSAGE_OPENRECENT];
}

bool qm::RecentsMenu::createMenu(HMENU hmenu)
{
	UINT nId = IDM_MESSAGE_OPENRECENT;
	while (::DeleteMenu(hmenu, nId++, MF_BYCOMMAND));
	
	clear();
	
	URIList listURI;
	StringListFree<URIList> free(listURI);
	{
		Recents* pRecents = pDocument_->getRecents();
		Lock<Recents> lock(*pRecents);
		
		unsigned int nCount = pRecents->getCount();
		unsigned int nOffset = nCount > MAX_RECENTS ? nCount - MAX_RECENTS : 0;
		
		listURI.reserve(nCount - nOffset);
		for (unsigned int n = nOffset; n < nCount; ++n)
			listURI.push_back(allocWString(pRecents->get(n)).release());
		std::sort(listURI.begin(), listURI.end(), URIComp());
	}
	
	listURI_.reserve(listURI.size());
	nId = IDM_MESSAGE_OPENRECENT;
	Account* pAccount = 0;
	for (URIList::iterator it = listURI.begin(); it != listURI.end(); ++it) {
		wstring_ptr wstrURI(*it);
		*it = 0;
		
		std::auto_ptr<URI> pURI(URI::parse(wstrURI.get()));
		if (pURI.get()) {
			MessagePtrLock mpl(pDocument_->getMessage(*pURI.get()));
			if (mpl) {
				if (pAccount != mpl->getFolder()->getAccount()) {
					if (pAccount != 0)
						::AppendMenu(hmenu, MF_SEPARATOR, -1, 0);
					pAccount = mpl->getFolder()->getAccount();
				}
				
				wstring_ptr wstrSubject(mpl->getSubject());
				wstring_ptr wstrTitle(UIUtil::formatMenu(wstrSubject.get()));
				W2T(wstrTitle.get(), ptszTitle);
				::AppendMenu(hmenu, MF_STRING, nId++, ptszTitle);
				
				listURI_.push_back(wstrURI.release());
			}
		}
	}
	if (nId != IDM_MESSAGE_OPENRECENT)
		::AppendMenu(hmenu, MF_SEPARATOR, -1, 0);
	
	HINSTANCE hInst = Application::getApplication().getResourceHandle();
	wstring_ptr wstrClear(loadString(hInst, IDS_CLEARRECENTS));
	W2T(wstrClear.get(), ptszClear);
	::AppendMenu(hmenu, MF_STRING, IDM_MESSAGE_CLEARRECENTS, ptszClear);
	
	return true;
}

void qm::RecentsMenu::clear()
{
	std::for_each(listURI_.begin(), listURI_.end(), string_free<WSTRING>());
	listURI_.clear();
}

bool RecentsMenu::URIComp::operator()(const WCHAR* pwszLhs,
									  const WCHAR* pwszRhs)
{
#ifndef NDEBUG
	wstring_ptr wstrPrefix(concat(URI::getScheme(), L"://"));
	size_t nPrefixLen = wcslen(wstrPrefix.get());
	assert(nPrefixLen == 12);
	assert(wcslen(pwszLhs) > nPrefixLen && wcsncmp(pwszLhs, wstrPrefix.get(), nPrefixLen) == 0);
	assert(wcslen(pwszRhs) > nPrefixLen && wcsncmp(pwszRhs, wstrPrefix.get(), nPrefixLen) == 0);
#endif
	const WCHAR* pLhs = pwszLhs + 12;
	const WCHAR* pLhsEnd = wcschr(pLhs, L'/');
	assert(pLhsEnd);
	size_t nLhsLen = pLhsEnd - pLhs;
	
	const WCHAR* pRhs = pwszRhs + 12;
	const WCHAR* pRhsEnd = wcschr(pRhs, L'/');
	assert(pRhsEnd);
	size_t nRhsLen = pRhsEnd - pRhs;
	
	int nComp = wcsncmp(pLhs, pRhs, QSMIN(nLhsLen, nRhsLen));
	return nComp < 0 ? true : nComp > 0 ? false : nLhsLen < nRhsLen;
}


/****************************************************************************
 *
 * ScriptMenu
 *
 */

qm::ScriptMenu::ScriptMenu(ScriptManager* pScriptManager) :
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

bool qm::ScriptMenu::createMenu(HMENU hmenu)
{
	assert(hmenu);
	
	while (::DeleteMenu(hmenu, 0, MF_BYPOSITION));
	
	clear();
	
	ScriptManager::NameList l;
	StringListFree<ScriptManager::NameList> free(l);
	pScriptManager_->getScriptNames(&l);
	
	UINT nId = IDM_TOOL_SCRIPT;
	
	for (ScriptManager::NameList::iterator it = l.begin();
		it != l.end() && list_.size() < MAX_SCRIPT; ++it, ++nId) {
		wstring_ptr wstrMenu(UIUtil::formatMenu(*it));
		W2T(wstrMenu.get(), ptszMenu);
		::AppendMenu(hmenu, MF_STRING, nId, ptszMenu);
		list_.push_back(*it);
		*it = 0;
	}
	
	if (l.empty()) {
		HINSTANCE hInst = Application::getApplication().getResourceHandle();
		wstring_ptr wstrNone(loadString(hInst, IDS_SCRIPTNONE));
		W2T(wstrNone.get(), ptszNone);
		::AppendMenu(hmenu, MF_STRING, IDM_TOOL_SCRIPTNONE, ptszNone);
	}
	
	return true;
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

qm::SubAccountMenu::SubAccountMenu(FolderModel* pFolderModel) :
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

bool qm::SubAccountMenu::createMenu(HMENU hmenu)
{
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
		for (Account::SubAccountList::size_type n = 1; n < l.size() && n < MAX_SUBACCOUNT; ++n) {
			SubAccount* pSubAccount = l[n];
			wstring_ptr wstrText(UIUtil::formatMenu(pSubAccount->getName()));
			W2T(wstrText.get(), ptszName);
			::AppendMenu(hmenu, MF_STRING, IDM_TOOL_SUBACCOUNT + n, ptszName);
		}
	}
	
	return true;
}


/****************************************************************************
 *
 * SortMenu
 *
 */

qm::SortMenu::SortMenu(ViewModelManager* pViewModelManager) :
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

bool qm::SortMenu::createMenu(HMENU hmenu)
{
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
		const ViewColumnList& l = pViewModel->getColumns();
		for (ViewColumnList::const_iterator it = l.begin();
			it != l.end() && nId < IDM_VIEW_SORT + MAX_SORT; ++it, ++nId) {
			const WCHAR* pwszTitle = (*it)->getTitle();
			if (*pwszTitle) {
				wstring_ptr wstrTitle(UIUtil::formatMenu(pwszTitle));
				W2T(wstrTitle.get(), ptszTitle);
				::InsertMenu(hmenu, nPos, MF_STRING | MF_BYPOSITION, nId, ptszTitle);
				++nPos;
			}
			if (nId - IDM_VIEW_SORT == nSortIndex)
				::CheckMenuItem(hmenu, nId, MF_BYCOMMAND | MF_CHECKED);
		}
	}
	else {
		HINSTANCE hInst = Application::getApplication().getResourceHandle();
		wstring_ptr wstrNone(loadString(hInst, IDS_NONE));
		W2T(wstrNone.get(), ptszNone);
		::InsertMenu(hmenu, 0, MF_STRING | MF_BYPOSITION, IDM_VIEW_SORT, ptszNone);
		::EnableMenuItem(hmenu, IDM_VIEW_SORT, MF_BYCOMMAND | MF_GRAYED);
	}
	
	return true;
}


/****************************************************************************
 *
 * TemplateMenu
 *
 */

qm::TemplateMenu::TemplateMenu(const TemplateManager* pTemplateManager) :
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

bool qm::TemplateMenu::createMenu(HMENU hmenu,
								  Account* pAccount)
{
	assert(hmenu);
	
	const WCHAR* pwszPrefix = getPrefix();
	
	int nBase = getBase();
	while (::DeleteMenu(hmenu, nBase, MF_BYPOSITION));
	
	clear();
	
	TemplateManager::NameList l;
	StringListFree<TemplateManager::NameList> free(l);
	pTemplateManager_->getTemplateNames(pAccount, pwszPrefix, &l);
	
	if (!l.empty()) {
		if (isAddSeparator())
			::AppendMenu(hmenu, MF_SEPARATOR, -1, 0);
		
		UINT nId = getId();
		
		for (TemplateManager::NameList::iterator it = l.begin();
			it != l.end() && list_.size() < MAX_TEMPLATE; ++it, ++nId) {
			wstring_ptr wstrMenu(UIUtil::formatMenu(*it + wcslen(pwszPrefix) + 1));
			W2T(wstrMenu.get(), ptszName);
			::AppendMenu(hmenu, MF_STRING, nId, ptszName);
			list_.push_back(*it);
			*it = 0;
		}
	}
	else if (getNoneId()) {
		UINT nId = getNoneId();
		if (nId) {
			HINSTANCE hInst = Application::getApplication().getResourceHandle();
			wstring_ptr wstrNone(loadString(hInst, IDS_TEMPLATENONE));
			W2T(wstrNone.get(), ptszNone);
			::AppendMenu(hmenu, MF_STRING, nId, ptszNone);
		}
	}
	
	return true;
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
										   bool bExternalEditor) :
	TemplateMenu(pTemplateManager),
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

bool qm::CreateTemplateMenu::isAddSeparator() const
{
	return false;
}


/****************************************************************************
 *
 * ViewTemplateMenu
 *
 */

qm::ViewTemplateMenu::ViewTemplateMenu(const TemplateManager* pTemplateManager) :
	TemplateMenu(pTemplateManager)
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
	return 1;
}

bool qm::ViewTemplateMenu::isAddSeparator() const
{
	return true;
}
