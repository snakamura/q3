/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#pragma warning(disable:4786)

#include <qmaccount.h>
#include <qmapplication.h>
#include <qmgoround.h>
#include <qmmessage.h>
#include <qmmessageholder.h>
#include <qmrecents.h>

#include <qsassert.h>
#include <qsconv.h>
#include <qsstl.h>
#include <qstextutil.h>

#include <algorithm>

#include <tchar.h>

#include "actionid.h"
#include "menucreator.h"
#include "resourceinc.h"
#include "uiutil.h"
#include "../model/filter.h"
#include "../model/fixedformtext.h"
#include "../model/goround.h"
#include "../model/templatemanager.h"
#include "../model/uri.h"
#include "../script/scriptmanager.h"
#include "../uimodel/foldermodel.h"
#include "../uimodel/messageselectionmodel.h"
#include "../uimodel/securitymodel.h"
#include "../uimodel/viewmodel.h"
#include "../util/util.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * ActionParamHelper
 *
 */

qm::ActionParamHelper::ActionParamHelper(ActionParamMap* pActionParamMap) :
	pActionParamMap_(pActionParamMap)
{
}

qm::ActionParamHelper::~ActionParamHelper()
{
}

unsigned int qm::ActionParamHelper::add(unsigned int nMaxParamCount,
										std::auto_ptr<ActionParam> pParam)
{
	unsigned int nId = pActionParamMap_->addActionParam(nMaxParamCount, pParam);
	if (nId != -1)
		listId_.push_back(nId);
	return nId;
}

void qm::ActionParamHelper::clear()
{
	for (IdList::const_iterator it = listId_.begin(); it != listId_.end(); ++it)
		pActionParamMap_->removeActionParam(*it);
	listId_.clear();
}


/****************************************************************************
 *
 * AttachmentMenuCreator
 *
 */

qm::AttachmentMenuCreator::AttachmentMenuCreator(MessageSelectionModel* pMessageSelectionModel,
												 SecurityModel* pSecurityModel,
												 ActionParamMap* pActionParamMap) :
	pMessageSelectionModel_(pMessageSelectionModel),
	pSecurityModel_(pSecurityModel),
	helper_(pActionParamMap)
{
}

qm::AttachmentMenuCreator::~AttachmentMenuCreator()
{
}

UINT qm::AttachmentMenuCreator::createMenu(HMENU hmenu,
										   UINT nIndex)
{
	assert(hmenu);
	
	MenuCreatorUtil::removeMenuItems(hmenu, nIndex, DATA);
	helper_.clear();
	
	bool bAdded = false;
	MessagePtrLock mpl(pMessageSelectionModel_->getFocusedMessage());
	if (mpl) {
		Message msg;
		if (mpl->getMessage(Account::GETMESSAGEFLAG_TEXT,
			0, pSecurityModel_->getSecurityMode(), &msg)) {
			AttachmentParser parser(msg);
			typedef AttachmentParser::AttachmentList List;
			List list;
			AttachmentParser::AttachmentListFree free(list);
			parser.getAttachments(false, &list);
			
			int nMnemonic = 1;
			for (List::iterator it = list.begin(); it != list.end(); ++it) {
				URI uri(mpl, &msg, (*it).second, URIFragment::TYPE_BODY);
				wstring_ptr wstrURI(uri.toString());
				std::auto_ptr<ActionParam> pParam(new ActionParam(
					IDM_MESSAGE_ATTACHMENT, wstrURI.get()));
				unsigned int nId = helper_.add(MAX_MESSAGE_ATTACHMENT, pParam);
				if (nId != -1) {
					wstring_ptr wstrName(UIUtil::formatMenu((*it).first, &nMnemonic));
					MenuCreatorUtil::insertMenuItem(hmenu, nIndex++, nId, wstrName.get(), DATA);
					bAdded = true;
				}
			}
		}
	}
	
	if (!bAdded) {
		HINSTANCE hInst = Application::getApplication().getResourceHandle();
		wstring_ptr wstrNone(loadString(hInst, IDS_MENU_NONE));
		MenuCreatorUtil::insertMenuItem(hmenu, nIndex++, IDM_MESSAGE_ATTACHMENT, wstrNone.get(), DATA);
	}
	
	return nIndex;
}

DWORD qm::AttachmentMenuCreator::getMenuItemData() const
{
	return DATA;
}


/****************************************************************************
 *
 * EncodingMenuCreator
 *
 */

qm::EncodingMenuCreator::EncodingMenuCreator(Profile* pProfile,
											 bool bView,
											 ActionParamMap* pActionParamMap) :
	pProfile_(pProfile),
	bView_(bView),
	helper_(pActionParamMap)
{
}

qm::EncodingMenuCreator::~EncodingMenuCreator()
{
}

UINT qm::EncodingMenuCreator::createMenu(HMENU hmenu,
										 UINT nIndex)
{
	DWORD dwData = bView_ ? DATA_VIEW : DATA_TOOL;
	MenuCreatorUtil::removeMenuItems(hmenu, nIndex, dwData);
	
	typedef std::vector<WSTRING> StringList;
	StringList listEncoding;
	StringListFree<StringList> free(listEncoding);
	UIUtil::loadEncodings(pProfile_, &listEncoding);
	
	UINT nBaseId = bView_ ? IDM_VIEW_ENCODING : IDM_TOOL_ENCODING;
	UINT nMax = bView_ ? MAX_VIEW_ENCODING : MAX_TOOL_ENCODING;
	
	if (!listEncoding.empty()) {
		int nMnemonic = 1;
		for (StringList::iterator it = listEncoding.begin(); it != listEncoding.end(); ++it) {
			const WCHAR* pwszEncoding = *it;
			std::auto_ptr<ActionParam> pParam(new ActionParam(nBaseId, pwszEncoding));
			unsigned int nId = helper_.add(nMax, pParam);
			if (nId != -1) {
				wstring_ptr wstrName(UIUtil::formatMenu(pwszEncoding, &nMnemonic));
				MenuCreatorUtil::insertMenuItem(hmenu, nIndex++, nId, wstrName.get(), dwData);
			}
		}
	}
	else {
		HINSTANCE hInst = Application::getApplication().getResourceHandle();
		wstring_ptr wstrNone(loadString(hInst, IDS_MENU_NONE));
		MenuCreatorUtil::insertMenuItem(hmenu, nIndex++, nBaseId, wstrNone.get(), dwData);
	}
	
	return nIndex;
}

DWORD qm::EncodingMenuCreator::getMenuItemData() const
{
	return bView_ ? DATA_VIEW : DATA_TOOL;
}


/****************************************************************************
 *
 * FilterMenuCreator
 *
 */

qm::FilterMenuCreator::FilterMenuCreator(FilterManager* pFilterManager,
										 ActionParamMap* pActionParamMap) :
	pFilterManager_(pFilterManager),
	helper_(pActionParamMap)
{
}

qm::FilterMenuCreator::~FilterMenuCreator()
{
}

UINT qm::FilterMenuCreator::createMenu(HMENU hmenu,
									   UINT nIndex)
{
	MenuCreatorUtil::removeMenuItems(hmenu, nIndex, DATA);
	helper_.clear();
	
	const FilterManager::FilterList& l = pFilterManager_->getFilters();
	if (!l.empty()) {
		int nMnemonic = 1;
		for (FilterManager::FilterList::const_iterator it = l.begin(); it != l.end(); ++it) {
			const Filter* pFilter = *it;
			
			std::auto_ptr<ActionParam> pParam(new ActionParam(IDM_VIEW_FILTER, pFilter->getName()));
			unsigned int nId = helper_.add(MAX_VIEW_FILTER, pParam);
			if (nId != -1) {
				wstring_ptr wstrTitle(UIUtil::formatMenu(pFilter->getName(), &nMnemonic));
				MenuCreatorUtil::insertMenuItem(hmenu, nIndex++, nId, wstrTitle.get(), DATA);
			}
		}
	}
	else {
		HINSTANCE hInst = Application::getApplication().getResourceHandle();
		wstring_ptr wstrNone(loadString(hInst, IDS_MENU_NONE));
		MenuCreatorUtil::insertMenuItem(hmenu, nIndex++, IDM_VIEW_FILTER, wstrNone.get(), DATA);
	}
	
	return nIndex;
}

DWORD qm::FilterMenuCreator::getMenuItemData() const
{
	return DATA;
}


/****************************************************************************
 *
 * GoRoundMenuCreator
 *
 */

qm::GoRoundMenuCreator::GoRoundMenuCreator(GoRound* pGoRound,
										   ActionParamMap* pActionParamMap) :
	pGoRound_(pGoRound),
	helper_(pActionParamMap)
{
}

qm::GoRoundMenuCreator::~GoRoundMenuCreator()
{
}

UINT qm::GoRoundMenuCreator::createMenu(HMENU hmenu,
										UINT nIndex)
{
	MenuCreatorUtil::removeMenuItems(hmenu, nIndex, DATA);
	helper_.clear();
	
	const GoRound::CourseList& l = pGoRound_->getCourses();
	if (!l.empty()) {
		int nMnemonic = 1;
		for (GoRound::CourseList::const_iterator it = l.begin(); it != l.end(); ++it) {
			GoRoundCourse* pCourse = *it;
			
			std::auto_ptr<ActionParam> pParam(new ActionParam(
				IDM_TOOL_GOROUND, pCourse->getName()));
			unsigned int nId = helper_.add(MAX_TOOL_GOROUND, pParam);
			if (nId != -1) {
				wstring_ptr wstrName(UIUtil::formatMenu(pCourse->getName(), &nMnemonic));
				MenuCreatorUtil::insertMenuItem(hmenu, nIndex++, nId, wstrName.get(), DATA);
			}
		}
	}
	else {
		HINSTANCE hInst = Application::getApplication().getResourceHandle();
		wstring_ptr wstrName(loadString(hInst, IDS_MENU_GOROUND));
		MenuCreatorUtil::insertMenuItem(hmenu, nIndex++, IDM_TOOL_GOROUND, wstrName.get(), DATA);
	}
	
	return nIndex;
}

DWORD qm::GoRoundMenuCreator::getMenuItemData() const
{
	return DATA;
}


/****************************************************************************
 *
 * InsertTextMenuCreator
 *
 */

qm::InsertTextMenuCreator::InsertTextMenuCreator(FixedFormTextManager* pManager,
												 ActionParamMap* pActionParamMap) :
	pManager_(pManager),
	helper_(pActionParamMap)
{
}

qm::InsertTextMenuCreator::~InsertTextMenuCreator()
{
}

UINT qm::InsertTextMenuCreator::createMenu(HMENU hmenu,
										   UINT nIndex)
{
	MenuCreatorUtil::removeMenuItems(hmenu, nIndex, DATA);
	helper_.clear();
	
	const FixedFormTextManager::TextList& l = pManager_->getTexts();
	if (!l.empty()) {
		UINT nPos = 0;
		int nMnemonic = 1;
		for (FixedFormTextManager::TextList::const_iterator it = l.begin(); it != l.end(); ++it) {
			const FixedFormText* pText = *it;
			
			std::auto_ptr<ActionParam> pParam(new ActionParam(
				IDM_TOOL_INSERTTEXT, pText->getName()));
			unsigned int nId = helper_.add(MAX_TOOL_INSERTTEXT, pParam);
			if (nId != -1) {
				wstring_ptr wstrName(UIUtil::formatMenu(pText->getName(), &nMnemonic));
				MenuCreatorUtil::insertMenuItem(hmenu, nIndex++, nId, wstrName.get(), DATA);
			}
		}
	}
	else {
		HINSTANCE hInst = Application::getApplication().getResourceHandle();
		wstring_ptr wstrNone(loadString(hInst, IDS_MENU_NONE));
		MenuCreatorUtil::insertMenuItem(hmenu, nIndex++, IDM_TOOL_INSERTTEXT, wstrNone.get(), DATA);
	}
	
	return nIndex;
}

DWORD qm::InsertTextMenuCreator::getMenuItemData() const
{
	return DATA;
}


/****************************************************************************
 *
 * MoveMenuCreator
 *
 */

qm::MoveMenuCreator::MoveMenuCreator(FolderModelBase* pFolderModel,
									 MessageSelectionModel* pMessageSelectionModel,
									 qs::ActionParamMap* pActionParamMap) :
	pFolderModel_(pFolderModel),
	pMessageSelectionModel_(pMessageSelectionModel),
	helper_(pActionParamMap)
{
}

qm::MoveMenuCreator::~MoveMenuCreator()
{
}

UINT qm::MoveMenuCreator::createMenu(HMENU hmenu,
									 UINT nIndex)
{
	assert(hmenu);
	
	bool bShowHidden = ::GetKeyState(VK_SHIFT) < 0;
	
	MenuCreatorUtil::removeMenuItems(hmenu, nIndex, DATA);
	helper_.clear();
	
	bool bAdded = false;
	
	std::pair<Account*, Folder*> p(pFolderModel_->getCurrent());
	Account* pAccount = p.first ? p.first : p.second ? p.second->getAccount() : 0;
	if (pAccount) {
		bool bEnabled = pMessageSelectionModel_->hasSelectedMessage();
		
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
		wstring_ptr wstrThisFolder(loadString(hInst, IDS_MENU_THISFOLDER));
		W2T(wstrThisFolder.get(), ptszThisFolder);
		
		for (Account::FolderList::const_iterator it = listFolder.begin(); it != listFolder.end(); ) {
			Folder* pFolder = *it;
			
			Folder* pParent = pFolder->getParentFolder();
			while (!stackFolder.empty() && stackFolder.back().pFolder_ != pParent)
				stackFolder.pop_back();
			assert(!stackFolder.empty());
			
			HMENU hmenuThis = stackFolder.back().hmenu_;
			
			wstring_ptr wstrName(formatName(pFolder, &stackFolder.back().nMnemonic_));
			W2T(wstrName.get(), ptszName);
			
			bool bHasChild = hasSelectableChildNormalFolder(it, listFolder.end());
			if (bHasChild) {
				HMENU hmenuNew = ::CreatePopupMenu();
				bool bAddThisFolder = isMovableFolder(pFolder);
				if (bAddThisFolder) {
					wstring_ptr wstrFolder(Util::formatFolder(pFolder));
					std::auto_ptr<ActionParam> pParam(new ActionParam(IDM_MESSAGE_MOVE, wstrFolder.get()));
					unsigned int nId = helper_.add(MAX_MESSAGE_MOVE, pParam);
					if (nId != -1)
						::AppendMenu(hmenuNew, MF_STRING, nId, ptszThisFolder);
				}
				::InsertMenu(hmenuThis, stackFolder.back().nCount_, MF_POPUP | MF_BYPOSITION,
					reinterpret_cast<UINT_PTR>(hmenuNew), ptszName);
				if (stackFolder.size() == 1) {
					MenuCreatorUtil::setMenuItemData(hmenuThis, stackFolder.back().nCount_, DATA);
					++nIndex;
					bAdded = true;
				}
				if (!bEnabled)
					::EnableMenuItem(hmenuThis, stackFolder.back().nCount_,
						MF_BYPOSITION | MF_GRAYED);
				++stackFolder.back().nCount_;
				stackFolder.push_back(MenuInserter(hmenuNew, pFolder));
				if (bAddThisFolder)
					++stackFolder.back().nCount_;
			}
			else if (isMovableFolder(pFolder)) {
				wstring_ptr wstrFolder(Util::formatFolder(pFolder));
				std::auto_ptr<ActionParam> pParam(new ActionParam(IDM_MESSAGE_MOVE, wstrFolder.get()));
				unsigned int nId = helper_.add(MAX_MESSAGE_MOVE, pParam);
				if (nId != -1) {
					::InsertMenu(hmenuThis, stackFolder.back().nCount_,
						MF_STRING | MF_BYPOSITION, nId, ptszName);
					if (stackFolder.size() == 1) {
						MenuCreatorUtil::setMenuItemData(hmenuThis, stackFolder.back().nCount_, DATA);
						++nIndex;
						bAdded = true;
					}
					if (!bEnabled)
						::EnableMenuItem(hmenuThis, stackFolder.back().nCount_,
							MF_BYPOSITION | MF_GRAYED);
				}
				++stackFolder.back().nCount_;
			}
			
			++it;
			if (!bHasChild) {
				while (it != listFolder.end() && pFolder->isAncestorOf(*it))
					++it;
			}
		}
	}
	
	if (!bAdded) {
		HINSTANCE hInst = Application::getApplication().getResourceHandle();
		wstring_ptr wstrNone(loadString(hInst, IDS_MENU_NONE));
		MenuCreatorUtil::insertMenuItem(hmenu, nIndex++, IDM_MESSAGE_MOVE, wstrNone.get(), DATA);
	}
	
	return nIndex;
}

DWORD qm::MoveMenuCreator::getMenuItemData() const
{
	return DATA;
}

bool qm::MoveMenuCreator::isMovableFolder(const Folder* pFolder)
{
	return pFolder->getType() == Folder::TYPE_NORMAL &&
		!pFolder->isFlag(Folder::FLAG_NOSELECT);
}

bool qm::MoveMenuCreator::hasSelectableChildNormalFolder(Account::FolderList::const_iterator first,
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

wstring_ptr qm::MoveMenuCreator::formatName(const Folder* pFolder,
											int* pnMnemonic)
{
	assert(pFolder);
	assert(pnMnemonic);
	return UIUtil::formatMenu(pFolder->getName(), pnMnemonic);
}


/****************************************************************************
 *
 * MoveMenuCreator::MenuInserter
 *
 */

qm::MoveMenuCreator::MenuInserter::MenuInserter(HMENU hmenu,
												Folder* pFolder) :
	hmenu_(hmenu),
	pFolder_(pFolder),
	nCount_(0),
	nMnemonic_(1)
{
}


/****************************************************************************
 *
 * RecentsMenuCreator
 *
 */

qm::RecentsMenuCreator::RecentsMenuCreator(Recents* pRecents,
										   AccountManager* pAccountManager,
										   qs::ActionParamMap* pActionParamMap) :
	pRecents_(pRecents),
	pAccountManager_(pAccountManager),
	helper_(pActionParamMap)
{
}

qm::RecentsMenuCreator::~RecentsMenuCreator()
{
}

UINT qm::RecentsMenuCreator::createMenu(HMENU hmenu,
										UINT nIndex)
{
	MenuCreatorUtil::removeMenuItems(hmenu, nIndex, DATA);
	helper_.clear();
	
	typedef std::vector<URI*> URIList;
	URIList listURI;
	struct Deleter
	{
		Deleter(URIList& l) :
			l_(l)
		{
		}
		
		~Deleter()
		{
			std::for_each(l_.begin(), l_.end(), qs::deleter<URI>());
			l_.clear();
		}
		
		URIList& l_;
	} deleter(listURI);
	{
		pRecents_->removeSeens();
		
		Lock<Recents> lock(*pRecents_);
		
		unsigned int nCount = pRecents_->getCount();
		unsigned int nOffset = nCount > MAX_MESSAGE_OPENRECENT ?
			nCount - MAX_MESSAGE_OPENRECENT : 0;
		
		listURI.reserve(nCount - nOffset);
		for (unsigned int n = nOffset; n < nCount; ++n)
			listURI.push_back(new URI(*pRecents_->get(n)));
		std::sort(listURI.begin(), listURI.end(), URIComp());
	}
	
	bool bAdded = false;
	
	Account* pAccount = 0;
	int nMnemonic = 1;
	for (URIList::iterator it = listURI.begin(); it != listURI.end(); ++it) {
		std::auto_ptr<URI> pURI(*it);
		*it = 0;
		
		MessagePtrLock mpl(pAccountManager_->getMessage(*pURI.get()));
		if (mpl) {
			if (pAccount != mpl->getAccount()) {
				if (pAccount != 0)
					MenuCreatorUtil::insertMenuItem(hmenu, nIndex++, -1, 0, DATA);
				pAccount = mpl->getAccount();
			}
			
			wstring_ptr wstrURI(pURI->toString());
			std::auto_ptr<ActionParam> pParam(new ActionParam(
				IDM_MESSAGE_OPENRECENT, wstrURI.get()));
			unsigned int nId = helper_.add(MAX_MESSAGE_OPENRECENT, pParam);
			if (nId != -1) {
				wstring_ptr wstrSubject(mpl->getSubject());
				WCHAR wszMnemonic[] = {
					L'&',
					nMnemonic < 10 ? L'1' + (nMnemonic - 1) : L'0',
					L' ',
					L'\0'
				};
				wstring_ptr wstrTitle(concat(wszMnemonic,
					TextUtil::replaceAll(wstrSubject.get(), L"&", L"&&").get()));
				MenuCreatorUtil::insertMenuItem(hmenu, nIndex++, nId, wstrTitle.get(), DATA);
				bAdded = true;
				++nMnemonic;
			}
		}
	}
	
	if (!bAdded) {
		HINSTANCE hInst = Application::getApplication().getResourceHandle();
		wstring_ptr wstrNone(loadString(hInst, IDS_MENU_NONE));
		MenuCreatorUtil::insertMenuItem(hmenu, nIndex++, IDM_MESSAGE_OPENRECENT, wstrNone.get(), DATA);
	}
	
	return nIndex;
}

DWORD qm::RecentsMenuCreator::getMenuItemData() const
{
	return DATA;
}

bool RecentsMenuCreator::URIComp::operator()(const URI* pLhs,
											 const URI* pRhs)
{
	return *pLhs < *pRhs;
}


/****************************************************************************
 *
 * ScriptMenuCreator
 *
 */

qm::ScriptMenuCreator::ScriptMenuCreator(ScriptManager* pScriptManager,
										 ActionParamMap* pActionParamMap) :
	pScriptManager_(pScriptManager),
	helper_(pActionParamMap)
{
}

qm::ScriptMenuCreator::~ScriptMenuCreator()
{
}

UINT qm::ScriptMenuCreator::createMenu(HMENU hmenu,
									   UINT nIndex)
{
	assert(hmenu);
	
	MenuCreatorUtil::removeMenuItems(hmenu, nIndex, DATA);
	helper_.clear();
	
	ScriptManager::NameList l;
	StringListFree<ScriptManager::NameList> free(l);
	pScriptManager_->getScriptNames(&l);
	
	if (!l.empty()) {
		int nMnemonic = 1;
		for (ScriptManager::NameList::const_iterator it = l.begin(); it != l.end(); ++it) {
			const WCHAR* pwszName = *it;
			
			std::auto_ptr<ActionParam> pParam(new ActionParam(IDM_TOOL_SCRIPT, pwszName));
			unsigned int nId = helper_.add(MAX_TOOL_SCRIPT, pParam);
			if (nId != -1) {
				wstring_ptr wstrMenu(UIUtil::formatMenu(pwszName, &nMnemonic));
				MenuCreatorUtil::insertMenuItem(hmenu, nIndex++, nId, wstrMenu.get(), DATA);
			}
		}
	}
	else {
		HINSTANCE hInst = Application::getApplication().getResourceHandle();
		wstring_ptr wstrNone(loadString(hInst, IDS_MENU_NONE));
		MenuCreatorUtil::insertMenuItem(hmenu, nIndex++, IDM_TOOL_SCRIPT, wstrNone.get(), DATA);
	}
	
	return nIndex;
}

DWORD qm::ScriptMenuCreator::getMenuItemData() const
{
	return DATA;
}


/****************************************************************************
 *
 * SortMenuCreator
 *
 */

qm::SortMenuCreator::SortMenuCreator(ViewModelManager* pViewModelManager,
									 ActionParamMap* pActionParamMap) :
	pViewModelManager_(pViewModelManager),
	helper_(pActionParamMap)
{
}

qm::SortMenuCreator::~SortMenuCreator()
{
}

UINT qm::SortMenuCreator::createMenu(HMENU hmenu,
									 UINT nIndex)
{
	MenuCreatorUtil::removeMenuItems(hmenu, nIndex, DATA);
	helper_.clear();
	
	ViewModel* pViewModel = pViewModelManager_->getCurrentViewModel();
	if (pViewModel) {
		unsigned int nSortIndex = pViewModel->getSort() & ViewModel::SORT_INDEX_MASK;
		UINT nId = IDM_VIEW_SORT;
		int nMnemonic = 1;
		const ViewColumnList& l = pViewModel->getColumns();
		for (ViewColumnList::size_type n = 0; n < l.size(); ++n) {
			const ViewColumn* pColumn = l[n];
			const WCHAR* pwszTitle = pColumn->getTitle();
			if (*pwszTitle) {
				WCHAR wsz[32];
				_snwprintf(wsz, countof(wsz), L"@%u", n);
				std::auto_ptr<ActionParam> pParam(new ActionParam(IDM_VIEW_SORT, wsz));
				unsigned int nId = helper_.add(MAX_VIEW_SORT, pParam);
				if (nId != -1) {
					wstring_ptr wstrTitle(UIUtil::formatMenu(pwszTitle, &nMnemonic));
					MenuCreatorUtil::insertMenuItem(hmenu, nIndex++, nId, wstrTitle.get(), DATA);
				}
			}
		}
	}
	else {
		HINSTANCE hInst = Application::getApplication().getResourceHandle();
		wstring_ptr wstrNone(loadString(hInst, IDS_MENU_NONE));
		MenuCreatorUtil::insertMenuItem(hmenu, nIndex++, IDM_VIEW_SORT, wstrNone.get(), DATA);
	}
	
	return nIndex;
}

DWORD qm::SortMenuCreator::getMenuItemData() const
{
	return DATA;
}


/****************************************************************************
 *
 * SubAccountMenuCreator
 *
 */

qm::SubAccountMenuCreator::SubAccountMenuCreator(FolderModel* pFolderModel,
												 ActionParamMap* pActionParamMap) :
	pFolderModel_(pFolderModel),
	helper_(pActionParamMap)
{
}

qm::SubAccountMenuCreator::~SubAccountMenuCreator()
{
}

UINT qm::SubAccountMenuCreator::createMenu(HMENU hmenu,
										   UINT nIndex)
{
	MenuCreatorUtil::removeMenuItems(hmenu, nIndex, DATA);
	helper_.clear();
	
	bool bAdded = false;
	
	std::pair<Account*, Folder*> p(pFolderModel_->getCurrent());
	Account* pAccount = p.first ? p.first : p.second ? p.second->getAccount() : 0;
	if (pAccount) {
		const Account::SubAccountList& l = pAccount->getSubAccounts();
		assert(!l.empty());
		int nMnemonic = 1;
		for (Account::SubAccountList::const_iterator it = l.begin() + 1; it != l.end(); ++it) {
			SubAccount* pSubAccount = *it;
			
			std::auto_ptr<ActionParam> pParam(new ActionParam(
				IDM_TOOL_SUBACCOUNT, pSubAccount->getName()));
			unsigned int nId = helper_.add(MAX_TOOL_SUBACCOUNT, pParam);
			if (nId != -1) {
				wstring_ptr wstrText(UIUtil::formatMenu(pSubAccount->getName(), &nMnemonic));
				MenuCreatorUtil::insertMenuItem(hmenu, nIndex++, nId, wstrText.get(), DATA);
				bAdded = true;
			}
		}
	}
	
	if (!bAdded) {
		HINSTANCE hInst = Application::getApplication().getResourceHandle();
		wstring_ptr wstrNone(loadString(hInst, IDS_MENU_NONE));
		MenuCreatorUtil::insertMenuItem(hmenu, nIndex++, IDM_TOOL_SUBACCOUNT, wstrNone.get(), DATA);
	}
	
	return nIndex;
}

DWORD qm::SubAccountMenuCreator::getMenuItemData() const
{
	return DATA;
}


/****************************************************************************
 *
 * TemplateMenuCreator
 *
 */

qm::TemplateMenuCreator::TemplateMenuCreator(const TemplateManager* pTemplateManager,
											 FolderModelBase* pFolderModel,
											 ActionParamMap* pActionParamMap) :
	pTemplateManager_(pTemplateManager),
	pFolderModel_(pFolderModel),
	helper_(pActionParamMap)
{
}

qm::TemplateMenuCreator::~TemplateMenuCreator()
{
}

UINT qm::TemplateMenuCreator::createMenu(HMENU hmenu,
										 UINT nIndex)
{
	assert(hmenu);
	
	const WCHAR* pwszPrefix = getPrefix();
	size_t nPrefixLen = wcslen(pwszPrefix);
	
	DWORD dwData = getMenuItemData();
	MenuCreatorUtil::removeMenuItems(hmenu, nIndex, dwData);
	helper_.clear();
	
	UINT nBaseId = getBaseId();
	bool bAdded = false;
	
	std::pair<Account*, Folder*> p(pFolderModel_->getCurrent());
	Account* pAccount = p.first ? p.first : p.second ? p.second->getAccount() : 0;
	if (pAccount) {
		TemplateManager::NameList listName;
		StringListFree<TemplateManager::NameList> free(listName);
		pTemplateManager_->getTemplateNames(pAccount->getClass(), pwszPrefix, &listName);
		
		if (!listName.empty()) {
			UINT nMax = getMax();
			int nMnemonic = 1;
			for (TemplateManager::NameList::const_iterator it = listName.begin(); it != listName.end(); ++it) {
				const WCHAR* pwszName = *it;
				
				std::auto_ptr<ActionParam> pParam(new ActionParam(nBaseId, pwszName));
				unsigned int nId = helper_.add(nMax, pParam);
				if (nId != -1) {
					wstring_ptr wstrMenu(UIUtil::formatMenu(pwszName + nPrefixLen + 1, &nMnemonic));
					MenuCreatorUtil::insertMenuItem(hmenu, nIndex++, nId, wstrMenu.get(), dwData);
					bAdded = true;
				}
			}
		}
	}
	
	if (!bAdded) {
		HINSTANCE hInst = Application::getApplication().getResourceHandle();
		wstring_ptr wstrNone(loadString(hInst, IDS_MENU_NONE));
		MenuCreatorUtil::insertMenuItem(hmenu, nIndex++, nBaseId, wstrNone.get(), dwData);
	}
	
	return nIndex;
}


/****************************************************************************
 *
 * CreateTemplateMenuCreator
 *
 */

qm::CreateTemplateMenuCreator::CreateTemplateMenuCreator(const TemplateManager* pTemplateManager,
														 FolderModelBase* pFolderModel,
														 ActionParamMap* pActionParamMap,
														 bool bExternalEditor) :
	TemplateMenuCreator(pTemplateManager, pFolderModel, pActionParamMap),
	bExternalEditor_(bExternalEditor)
{
}

qm::CreateTemplateMenuCreator::~CreateTemplateMenuCreator()
{
}

DWORD qm::CreateTemplateMenuCreator::getMenuItemData() const
{
	return !bExternalEditor_ ? DATA : DATA_EXTERNAL;
}

const WCHAR* qm::CreateTemplateMenuCreator::getPrefix() const
{
	return L"create";
}

UINT qm::CreateTemplateMenuCreator::getBaseId() const
{
	return !bExternalEditor_ ? IDM_MESSAGE_CREATE : IDM_MESSAGE_CREATEEXTERNAL;
}

UINT qm::CreateTemplateMenuCreator::getMax() const
{
	return !bExternalEditor_ ? MAX_MESSAGE_CREATE : MAX_MESSAGE_CREATEEXTERNAL;
}


/****************************************************************************
 *
 * ViewTemplateMenuCreator
 *
 */

qm::ViewTemplateMenuCreator::ViewTemplateMenuCreator(const TemplateManager* pTemplateManager,
													 FolderModelBase* pFolderModel,
													 ActionParamMap* pActionParamMap) :
	TemplateMenuCreator(pTemplateManager, pFolderModel, pActionParamMap)
{
}

qm::ViewTemplateMenuCreator::~ViewTemplateMenuCreator()
{
}

DWORD qm::ViewTemplateMenuCreator::getMenuItemData() const
{
	return DATA;
}

const WCHAR* qm::ViewTemplateMenuCreator::getPrefix() const
{
	return L"view";
}

UINT qm::ViewTemplateMenuCreator::getBaseId() const
{
	return IDM_VIEW_TEMPLATE;
}

UINT qm::ViewTemplateMenuCreator::getMax() const
{
	return MAX_VIEW_TEMPLATE;
}


/****************************************************************************
 *
 * MenuCreatorUtil
 *
 */

void qm::MenuCreatorUtil::insertMenuItem(HMENU hmenu,
										 UINT nIndex,
										 UINT nId,
										 const WCHAR* pwszText,
										 DWORD dwData)
{
	assert((nId != -1 && pwszText) || (nId == -1 && !pwszText));
	
	UINT nFlags = MF_BYPOSITION | (nId != -1 ? MF_STRING : MF_SEPARATOR);
	W2T(pwszText, ptszText);
	::InsertMenu(hmenu, nIndex, nFlags, nId, ptszText);
	setMenuItemData(hmenu, nIndex, dwData);
}

void qm::MenuCreatorUtil::removeMenuItems(HMENU hmenu,
										  UINT nIndex,
										  DWORD dwData)
{
	while (true) {
		MENUITEMINFO mii = {
			sizeof(mii),
			MIIM_DATA
		};
		if (!::GetMenuItemInfo(hmenu, nIndex, TRUE, &mii) ||
			mii.dwItemData != dwData)
			break;
		::DeleteMenu(hmenu, nIndex, MF_BYPOSITION);
	}
}

void qm::MenuCreatorUtil::setMenuItemData(HMENU hmenu,
										  UINT nIndex,
										  DWORD dwData)
{
	MENUITEMINFO mii = {
		sizeof(mii),
		MIIM_DATA,
		0,
		0,
		0,
		0,
		0,
		0,
		dwData
	};
	::SetMenuItemInfo(hmenu, nIndex, TRUE, &mii);
}
