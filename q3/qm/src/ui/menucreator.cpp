/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
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
#include "messagewindow.h"
#include "resourceinc.h"
#include "uiutil.h"
#include "../model/addressbook.h"
#include "../model/messageenumerator.h"
#include "../model/filter.h"
#include "../model/fixedformtext.h"
#include "../model/goround.h"
#include "../model/templatemanager.h"
#include "../model/uri.h"
#include "../script/scriptmanager.h"
#include "../uimodel/addressbookmodel.h"
#include "../uimodel/addressbookselectionmodel.h"
#include "../uimodel/foldermodel.h"
#include "../uimodel/folderselectionmodel.h"
#include "../uimodel/messageselectionmodel.h"
#include "../uimodel/securitymodel.h"
#include "../uimodel/viewmodel.h"
#include "../util/util.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * MenuCreator
 *
 */

qm::MenuCreator::~MenuCreator()
{
}


/****************************************************************************
 *
 * ActionParamHelper
 *
 */

qm::ActionParamHelper::ActionParamHelper(ActionParamMap* pActionParamMap) :
	pActionParamMap_(pActionParamMap)
{
	assert(pActionParamMap_);
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
 * AddressBookMenuCreator
 *
 */

qm::AddressBookMenuCreator::AddressBookMenuCreator(AddressBookModel* pModel,
												   AddressBookSelectionModel* pSelectionModel,
												   ActionParamMap* pActionParamMap) :
	pModel_(pModel),
	pSelectionModel_(pSelectionModel),
	helper_(pActionParamMap)
{
}

qm::AddressBookMenuCreator::~AddressBookMenuCreator()
{
}

UINT qm::AddressBookMenuCreator::createMenu(HMENU hmenu,
											UINT nIndex,
											const DynamicMenuItem* pItem)
{
	assert(hmenu);
	
	MenuCreatorUtil::removeMenuItems(hmenu, nIndex, pItem->getId());
	helper_.clear();
	
	bool bAdded = false;
	
	unsigned int nItem = pSelectionModel_->getFocusedItem();
	if (nItem != -1) {
		const AddressBookEntry* pEntry = pModel_->getEntry(nItem);
		const AddressBookEntry::AddressList& l = pEntry->getAddresses();
		
		int nMnemonic = 1;
		for (AddressBookEntry::AddressList::const_iterator it = l.begin(); it != l.end(); ++it) {
			const AddressBookAddress* pAddress = *it;
			
			wstring_ptr wstr(pAddress->getValue());
			std::auto_ptr<ActionParam> pParam(new ActionParam(
				IDM_ADDRESS_CREATEMESSAGE, wstr.get()));
			unsigned int nId = helper_.add(MAX_ADDRESS_CREATEMESSAGE, pParam);
			if (nId != -1) {
				wstring_ptr wstrName(UIUtil::formatMenu(pAddress->getAddress(), &nMnemonic));
				MenuCreatorUtil::insertMenuItem(hmenu, nIndex++, nId, wstrName.get(), pItem->getId());
				bAdded = true;
			}
		}
	}
	if (!bAdded) {
		HINSTANCE hInst = Application::getApplication().getResourceHandle();
		wstring_ptr wstrNone(loadString(hInst, IDS_MENU_NONE));
		MenuCreatorUtil::insertMenuItem(hmenu, nIndex++, IDM_ADDRESS_CREATEMESSAGE, wstrNone.get(), pItem->getId());
	}
	
	return nIndex;
}

const WCHAR* qm::AddressBookMenuCreator::getName() const
{
	return L"AddressCreateMessage";
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
										   UINT nIndex,
										   const DynamicMenuItem* pItem)
{
	assert(hmenu);
	
	MenuCreatorUtil::removeMenuItems(hmenu, nIndex, pItem->getId());
	helper_.clear();
	
	bool bAdded = false;
	std::auto_ptr<MessageEnumerator> pEnum(pMessageSelectionModel_->getFocusedMessage());
	if (pEnum->next()) {
		Message msg;
		Message* pMessage = pEnum->getMessage(Account::GETMESSAGEFLAG_TEXT,
			0, pSecurityModel_->getSecurityMode(), &msg);
		if (pMessage) {
			AttachmentParser parser(*pMessage);
			typedef AttachmentParser::AttachmentList List;
			List list;
			AttachmentParser::AttachmentListFree free(list);
			parser.getAttachments(AttachmentParser::GAF_NONE, &list);
			
			int nMnemonic = 1;
			for (List::iterator it = list.begin(); it != list.end(); ++it) {
				std::auto_ptr<URI> pURI(pEnum->getURI(pMessage,
					(*it).second, URIFragment::TYPE_BODY));
				assert(pURI.get());
				wstring_ptr wstrURI(pURI->toString());
				std::auto_ptr<ActionParam> pParam(new ActionParam(
					IDM_MESSAGE_OPENATTACHMENT, wstrURI.get()));
				unsigned int nId = helper_.add(MAX_MESSAGE_OPENATTACHMENT, pParam);
				if (nId != -1) {
					wstring_ptr wstrName(UIUtil::formatMenu((*it).first, &nMnemonic));
					MenuCreatorUtil::insertMenuItem(hmenu, nIndex++, nId, wstrName.get(), pItem->getId());
					bAdded = true;
				}
			}
		}
	}
	
	if (!bAdded) {
		HINSTANCE hInst = Application::getApplication().getResourceHandle();
		wstring_ptr wstrNone(loadString(hInst, IDS_MENU_NONE));
		MenuCreatorUtil::insertMenuItem(hmenu, nIndex++, IDM_MESSAGE_OPENATTACHMENT, wstrNone.get(), pItem->getId());
	}
	
	return nIndex;
}

const WCHAR* qm::AttachmentMenuCreator::getName() const
{
	return L"MessageOpenAttachment";
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
										 UINT nIndex,
										 const DynamicMenuItem* pItem)
{
	MenuCreatorUtil::removeMenuItems(hmenu, nIndex, pItem->getId());
	helper_.clear();
	
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
				MenuCreatorUtil::insertMenuItem(hmenu, nIndex++, nId, wstrName.get(), pItem->getId());
			}
		}
	}
	else {
		HINSTANCE hInst = Application::getApplication().getResourceHandle();
		wstring_ptr wstrNone(loadString(hInst, IDS_MENU_NONE));
		MenuCreatorUtil::insertMenuItem(hmenu, nIndex++, nBaseId, wstrNone.get(), pItem->getId());
	}
	
	return nIndex;
}

const WCHAR* qm::EncodingMenuCreator::getName() const
{
	return bView_ ? L"ViewEncoding" : L"ToolEncoding";
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
									   UINT nIndex,
									   const DynamicMenuItem* pItem)
{
	MenuCreatorUtil::removeMenuItems(hmenu, nIndex, pItem->getId());
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
				MenuCreatorUtil::insertMenuItem(hmenu, nIndex++, nId, wstrTitle.get(), pItem->getId());
			}
		}
	}
	else {
		HINSTANCE hInst = Application::getApplication().getResourceHandle();
		wstring_ptr wstrNone(loadString(hInst, IDS_MENU_NONE));
		MenuCreatorUtil::insertMenuItem(hmenu, nIndex++, IDM_VIEW_FILTER, wstrNone.get(), pItem->getId());
	}
	
	return nIndex;
}

const WCHAR* qm::FilterMenuCreator::getName() const
{
	return L"ViewFilter";
}


/****************************************************************************
 *
 * FontGroupMenuCreator
 *
 */

qm::FontGroupMenuCreator::FontGroupMenuCreator(const MessageWindowFontManager* pFontManager,
											   ActionParamMap* pActionParamMap) :
	pFontManager_(pFontManager),
	helper_(pActionParamMap)
{
}

qm::FontGroupMenuCreator::~FontGroupMenuCreator()
{
}

UINT qm::FontGroupMenuCreator::createMenu(HMENU hmenu,
										  UINT nIndex,
										  const DynamicMenuItem* pItem)
{
	MenuCreatorUtil::removeMenuItems(hmenu, nIndex, pItem->getId());
	helper_.clear();
	
	const MessageWindowFontManager::GroupList& l = pFontManager_->getGroups();
	if (!l.empty()) {
		int nMnemonic = 1;
		for (MessageWindowFontManager::GroupList::const_iterator it = l.begin(); it != l.end(); ++it) {
			const MessageWindowFontGroup* pFontGroup = *it;
			
			std::auto_ptr<ActionParam> pParam(new ActionParam(IDM_VIEW_FONTGROUP, pFontGroup->getName()));
			unsigned int nId = helper_.add(MAX_VIEW_FONTGROUP, pParam);
			if (nId != -1) {
				wstring_ptr wstrTitle(UIUtil::formatMenu(pFontGroup->getName(), &nMnemonic));
				MenuCreatorUtil::insertMenuItem(hmenu, nIndex++, nId, wstrTitle.get(), pItem->getId());
			}
		}
	}
	else {
		HINSTANCE hInst = Application::getApplication().getResourceHandle();
		wstring_ptr wstrNone(loadString(hInst, IDS_MENU_NONE));
		MenuCreatorUtil::insertMenuItem(hmenu, nIndex++, IDM_VIEW_FONTGROUP, wstrNone.get(), pItem->getId());
	}
	
	return nIndex;
}

const WCHAR* qm::FontGroupMenuCreator::getName() const
{
	return L"ViewFontGroup";
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
										UINT nIndex,
										const DynamicMenuItem* pItem)
{
	MenuCreatorUtil::removeMenuItems(hmenu, nIndex, pItem->getId());
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
				MenuCreatorUtil::insertMenuItem(hmenu, nIndex++, nId, wstrName.get(), pItem->getId());
			}
		}
	}
	else {
		HINSTANCE hInst = Application::getApplication().getResourceHandle();
		wstring_ptr wstrName(loadString(hInst, IDS_MENU_GOROUND));
		MenuCreatorUtil::insertMenuItem(hmenu, nIndex++, IDM_TOOL_GOROUND, wstrName.get(), pItem->getId());
	}
	
	return nIndex;
}

const WCHAR* qm::GoRoundMenuCreator::getName() const
{
	return L"ToolGoround";
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
										   UINT nIndex,
										   const DynamicMenuItem* pItem)
{
	MenuCreatorUtil::removeMenuItems(hmenu, nIndex, pItem->getId());
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
				MenuCreatorUtil::insertMenuItem(hmenu, nIndex++, nId, wstrName.get(), pItem->getId());
			}
		}
	}
	else {
		HINSTANCE hInst = Application::getApplication().getResourceHandle();
		wstring_ptr wstrNone(loadString(hInst, IDS_MENU_NONE));
		MenuCreatorUtil::insertMenuItem(hmenu, nIndex++, IDM_TOOL_INSERTTEXT, wstrNone.get(), pItem->getId());
	}
	
	return nIndex;
}

const WCHAR* qm::InsertTextMenuCreator::getName() const
{
	return L"ToolInsertText";
}


/****************************************************************************
 *
 * MoveMenuCreator
 *
 */

qm::MoveMenuCreator::MoveMenuCreator(AccountSelectionModel* pAccountSelectionModel,
									 MessageSelectionModel* pMessageSelectionModel,
									 qs::ActionParamMap* pActionParamMap) :
	pAccountSelectionModel_(pAccountSelectionModel),
	pMessageSelectionModel_(pMessageSelectionModel),
	helper_(pActionParamMap)
{
}

qm::MoveMenuCreator::~MoveMenuCreator()
{
}

UINT qm::MoveMenuCreator::createMenu(HMENU hmenu,
									 UINT nIndex,
									 const DynamicMenuItem* pItem)
{
	assert(hmenu);
	
	bool bShowHidden = ::GetKeyState(VK_SHIFT) < 0;
	
	MenuCreatorUtil::removeMenuItems(hmenu, nIndex, pItem->getId());
	helper_.clear();
	
	bool bAdded = false;
	
	Account* pAccount = pAccountSelectionModel_->getAccount();
	if (pAccount) {
		bool bEnabled = pMessageSelectionModel_->hasSelectedMessageHolders();
		
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
					MenuCreatorUtil::setMenuItemData(hmenuThis, stackFolder.back().nCount_, pItem->getId());
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
						MenuCreatorUtil::setMenuItemData(hmenuThis, stackFolder.back().nCount_, pItem->getId());
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
		MenuCreatorUtil::insertMenuItem(hmenu, nIndex++, IDM_MESSAGE_MOVENONE, wstrNone.get(), pItem->getId());
	}
	
	return nIndex;
}

const WCHAR* qm::MoveMenuCreator::getName() const
{
	return L"MessageMove";
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
										   const URIResolver* pURIResolver,
										   qs::ActionParamMap* pActionParamMap) :
	pRecents_(pRecents),
	pURIResolver_(pURIResolver),
	helper_(pActionParamMap)
{
}

qm::RecentsMenuCreator::~RecentsMenuCreator()
{
}

UINT qm::RecentsMenuCreator::createMenu(HMENU hmenu,
										UINT nIndex,
										const DynamicMenuItem* pItem)
{
	MenuCreatorUtil::removeMenuItems(hmenu, nIndex, pItem->getId());
	helper_.clear();
	
	pRecents_->removeSeens();
	
	typedef std::vector<MessageHolderURI*> URIList;
	URIList listURI;
	container_deleter<URIList> deleter(listURI);
	{
		Lock<Recents> lock(*pRecents_);
		
		unsigned int nCount = pRecents_->getCount();
		unsigned int nOffset = nCount > MAX_MESSAGE_OPENRECENT ?
			nCount - MAX_MESSAGE_OPENRECENT : 0;
		
		listURI.reserve(nCount - nOffset);
		for (unsigned int n = nOffset; n < nCount; ++n)
			listURI.push_back(new MessageHolderURI(*pRecents_->get(n).first));
	}
	std::sort(listURI.begin(), listURI.end(), URIComp());
	
	bool bAdded = false;
	
	Account* pAccount = 0;
	int nMnemonic = 1;
	for (URIList::iterator it = listURI.begin(); it != listURI.end(); ++it) {
		std::auto_ptr<MessageHolderURI> pURI(*it);
		*it = 0;
		
		MessagePtrLock mpl(pURI->resolveMessagePtr(pURIResolver_));
		if (mpl && !mpl->getFolder()->isHidden()) {
			if (pAccount != mpl->getAccount()) {
				if (pAccount != 0)
					MenuCreatorUtil::insertMenuItem(hmenu, nIndex++, -1, 0, pItem->getId());
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
				MenuCreatorUtil::insertMenuItem(hmenu, nIndex++, nId, wstrTitle.get(), pItem->getId());
				bAdded = true;
				++nMnemonic;
			}
		}
	}
	
	if (!bAdded) {
		HINSTANCE hInst = Application::getApplication().getResourceHandle();
		wstring_ptr wstrNone(loadString(hInst, IDS_MENU_NONE));
		MenuCreatorUtil::insertMenuItem(hmenu, nIndex++, IDM_MESSAGE_OPENRECENT, wstrNone.get(), pItem->getId());
	}
	
	return nIndex;
}

const WCHAR* qm::RecentsMenuCreator::getName() const
{
	return L"MessageOpenRecent";
}

bool RecentsMenuCreator::URIComp::operator()(const MessageHolderURI* pLhs,
											 const MessageHolderURI* pRhs)
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
									   UINT nIndex,
									   const DynamicMenuItem* pItem)
{
	assert(hmenu);
	
	MenuCreatorUtil::removeMenuItems(hmenu, nIndex, pItem->getId());
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
				MenuCreatorUtil::insertMenuItem(hmenu, nIndex++, nId, wstrMenu.get(), pItem->getId());
			}
		}
	}
	else {
		HINSTANCE hInst = Application::getApplication().getResourceHandle();
		wstring_ptr wstrNone(loadString(hInst, IDS_MENU_NONE));
		MenuCreatorUtil::insertMenuItem(hmenu, nIndex++, IDM_TOOL_SCRIPT, wstrNone.get(), pItem->getId());
	}
	
	return nIndex;
}

const WCHAR* qm::ScriptMenuCreator::getName() const
{
	return L"ToolScript";
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
									 UINT nIndex,
									 const DynamicMenuItem* pItem)
{
	MenuCreatorUtil::removeMenuItems(hmenu, nIndex, pItem->getId());
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
					MenuCreatorUtil::insertMenuItem(hmenu, nIndex++, nId, wstrTitle.get(), pItem->getId());
				}
			}
		}
	}
	else {
		HINSTANCE hInst = Application::getApplication().getResourceHandle();
		wstring_ptr wstrNone(loadString(hInst, IDS_MENU_NONE));
		MenuCreatorUtil::insertMenuItem(hmenu, nIndex++, IDM_VIEW_SORT, wstrNone.get(), pItem->getId());
	}
	
	return nIndex;
}

const WCHAR* qm::SortMenuCreator::getName() const
{
	return L"ViewSort";
}


/****************************************************************************
 *
 * SubAccountMenuCreator
 *
 */

qm::SubAccountMenuCreator::SubAccountMenuCreator(AccountSelectionModel* pAccountSelectionModel,
												 ActionParamMap* pActionParamMap) :
	pAccountSelectionModel_(pAccountSelectionModel),
	helper_(pActionParamMap)
{
}

qm::SubAccountMenuCreator::~SubAccountMenuCreator()
{
}

UINT qm::SubAccountMenuCreator::createMenu(HMENU hmenu,
										   UINT nIndex,
										   const DynamicMenuItem* pItem)
{
	MenuCreatorUtil::removeMenuItems(hmenu, nIndex, pItem->getId());
	helper_.clear();
	
	bool bAdded = false;
	
	Account* pAccount = pAccountSelectionModel_->getAccount();
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
				MenuCreatorUtil::insertMenuItem(hmenu, nIndex++, nId, wstrText.get(), pItem->getId());
				bAdded = true;
			}
		}
	}
	
	if (!bAdded) {
		HINSTANCE hInst = Application::getApplication().getResourceHandle();
		wstring_ptr wstrNone(loadString(hInst, IDS_MENU_NONE));
		MenuCreatorUtil::insertMenuItem(hmenu, nIndex++, IDM_TOOL_SUBACCOUNT, wstrNone.get(), pItem->getId());
	}
	
	return nIndex;
}

const WCHAR* qm::SubAccountMenuCreator::getName() const
{
	return L"ToolSubAccount";
}


/****************************************************************************
 *
 * TemplateMenuCreator
 *
 */

qm::TemplateMenuCreator::TemplateMenuCreator(const TemplateManager* pTemplateManager,
											 AccountSelectionModel* pAccountSelectionModel,
											 ActionParamMap* pActionParamMap) :
	pTemplateManager_(pTemplateManager),
	pAccountSelectionModel_(pAccountSelectionModel),
	helper_(pActionParamMap)
{
}

qm::TemplateMenuCreator::~TemplateMenuCreator()
{
}

UINT qm::TemplateMenuCreator::createMenu(HMENU hmenu,
										 UINT nIndex,
										 const DynamicMenuItem* pItem)
{
	assert(hmenu);
	
	const WCHAR* pwszPrefix = getPrefix();
	size_t nPrefixLen = wcslen(pwszPrefix);
	
	MenuCreatorUtil::removeMenuItems(hmenu, nIndex, pItem->getId());
	helper_.clear();
	
	UINT nBaseId = getBaseId();
	bool bAdded = false;
	
	Account* pAccount = pAccountSelectionModel_->getAccount();
	if (pAccount) {
		TemplateManager::NameList listName;
		StringListFree<TemplateManager::NameList> free(listName);
		pTemplateManager_->getTemplateNames(pAccount->getClass(), pwszPrefix, &listName);
		
		if (!listName.empty()) {
			unsigned int nMax = getMax();
			int nMnemonic = 1;
			for (TemplateManager::NameList::const_iterator it = listName.begin(); it != listName.end(); ++it) {
				const WCHAR* pwszName = *it;
				
				std::auto_ptr<ActionParam> pParam(new ActionParam(nBaseId, pwszName));
				unsigned int nId = helper_.add(nMax, pParam);
				if (nId != -1) {
					wstring_ptr wstrMenu(UIUtil::formatMenu(pwszName + nPrefixLen + 1, &nMnemonic));
					MenuCreatorUtil::insertMenuItem(hmenu, nIndex++, nId, wstrMenu.get(), pItem->getId());
					bAdded = true;
				}
			}
		}
	}
	
	if (!bAdded) {
		HINSTANCE hInst = Application::getApplication().getResourceHandle();
		wstring_ptr wstrNone(loadString(hInst, IDS_MENU_NONE));
		MenuCreatorUtil::insertMenuItem(hmenu, nIndex++, nBaseId, wstrNone.get(), pItem->getId());
	}
	
	return nIndex;
}


/****************************************************************************
 *
 * CreateTemplateMenuCreator
 *
 */

qm::CreateTemplateMenuCreator::CreateTemplateMenuCreator(const TemplateManager* pTemplateManager,
														 AccountSelectionModel* pAccountSelectionModel,
														 bool bExternalEditor,
														 ActionParamMap* pActionParamMap) :
	TemplateMenuCreator(pTemplateManager, pAccountSelectionModel, pActionParamMap),
	bExternalEditor_(bExternalEditor)
{
}

qm::CreateTemplateMenuCreator::~CreateTemplateMenuCreator()
{
}

const WCHAR* qm::CreateTemplateMenuCreator::getName() const
{
	return !bExternalEditor_ ? L"MessageCreate" : L"MessageCreateExternal";
}

const WCHAR* qm::CreateTemplateMenuCreator::getPrefix() const
{
	return L"create";
}

UINT qm::CreateTemplateMenuCreator::getBaseId() const
{
	return !bExternalEditor_ ? IDM_MESSAGE_CREATE : IDM_MESSAGE_CREATEEXTERNAL;
}

unsigned int qm::CreateTemplateMenuCreator::getMax() const
{
	return !bExternalEditor_ ? MAX_MESSAGE_CREATE : MAX_MESSAGE_CREATEEXTERNAL;
}


/****************************************************************************
 *
 * ViewTemplateMenuCreator
 *
 */

qm::ViewTemplateMenuCreator::ViewTemplateMenuCreator(const TemplateManager* pTemplateManager,
													 AccountSelectionModel* pAccountSelectionModel,
													 ActionParamMap* pActionParamMap) :
	TemplateMenuCreator(pTemplateManager, pAccountSelectionModel, pActionParamMap)
{
}

qm::ViewTemplateMenuCreator::~ViewTemplateMenuCreator()
{
}

const WCHAR* qm::ViewTemplateMenuCreator::getName() const
{
	return L"ViewTemplate";
}

const WCHAR* qm::ViewTemplateMenuCreator::getPrefix() const
{
	return L"view";
}

UINT qm::ViewTemplateMenuCreator::getBaseId() const
{
	return IDM_VIEW_TEMPLATE;
}

unsigned int qm::ViewTemplateMenuCreator::getMax() const
{
	return MAX_VIEW_TEMPLATE;
}


/****************************************************************************
 *
 * MacroMenuCreator
 *
 */

qm::MacroMenuCreator::MacroMenuCreator(Document* pDocument,
									   MessageSelectionModel* pMessageSelectionModel,
									   SecurityModel* pSecurityModel,
									   qs::Profile* pProfile,
									   const qs::ActionItem* pActionItem,
									   size_t nActionItemCount,
									   ActionParamMap* pActionParamMap) :
	pDocument_(pDocument),
	pMessageSelectionModel_(pMessageSelectionModel),
	pAccountSelectionModel_(0),
	pSecurityModel_(pSecurityModel),
	pProfile_(pProfile),
	pActionItem_(pActionItem),
	nActionItemCount_(nActionItemCount),
	helper_(pActionParamMap)
{
	assert(pDocument);
	assert(pMessageSelectionModel);
	assert(pSecurityModel);
	assert(pProfile);
	assert(pActionItem);
}

qm::MacroMenuCreator::MacroMenuCreator(Document* pDocument,
									   AccountSelectionModel* pAccountSelectionModel,
									   SecurityModel* pSecurityModel,
									   qs::Profile* pProfile,
									   const qs::ActionItem* pActionItem,
									   size_t nActionItemCount,
									   ActionParamMap* pActionParamMap) :
	pDocument_(pDocument),
	pMessageSelectionModel_(0),
	pAccountSelectionModel_(pAccountSelectionModel),
	pSecurityModel_(pSecurityModel),
	pProfile_(pProfile),
	pActionItem_(pActionItem),
	nActionItemCount_(nActionItemCount),
	helper_(pActionParamMap)
{
	assert(pDocument);
	assert(pAccountSelectionModel);
	assert(pSecurityModel);
	assert(pProfile);
	assert(pActionItem);
}

qm::MacroMenuCreator::~MacroMenuCreator()
{
}

UINT qm::MacroMenuCreator::createMenu(HMENU hmenu,
									  UINT nIndex,
									  const DynamicMenuItem* pItem)
{
	MenuCreatorUtil::removeMenuItems(hmenu, nIndex, pItem->getId());
	helper_.clear();
	
	bool bAdded = false;
	
	const ActionItem* pActionItem = getActionItem(pItem->getName());
	assert(pItem);
	
	ItemList listItem;
	wstring_ptr wstrItems(evalMacro(static_cast<const MacroDynamicMenuItem*>(pItem)->getMacro()));
	if (wstrItems.get())
		parseItems(wstrItems.get(), &listItem);
	
	int nMnemonic = 1;
	for (ItemList::const_iterator it = listItem.begin(); it != listItem.end(); ++it) {
		const WCHAR* pwszName = (*it).first;
		const WCHAR* pwszParam = (*it).second;
		std::auto_ptr<ActionParam> pParam(new ActionParam(pActionItem->nId_, pwszParam, true));
		unsigned int nId = helper_.add(pActionItem->nMaxParamCount_, pParam);
		if (nId != -1) {
			wstring_ptr wstrText(UIUtil::formatMenu(pwszName, &nMnemonic));
			MenuCreatorUtil::insertMenuItem(hmenu, nIndex++, nId, wstrText.get(), pItem->getId());
			bAdded = true;
		}
	}
	
	if (!bAdded) {
		HINSTANCE hInst = Application::getApplication().getResourceHandle();
		wstring_ptr wstrNone(loadString(hInst, IDS_MENU_NONE));
		MenuCreatorUtil::insertMenuItem(hmenu, nIndex++, IDM_NONE, wstrNone.get(), pItem->getId());
	}
	
	return nIndex;
}

wstring_ptr qm::MacroMenuCreator::evalMacro(const Macro* pMacro) const
{
	if (!pMacro)
		return 0;
	
	Account* pAccount = 0;
	AccountLock lock;
	Folder* pFolder = 0;
	MessageHolderList l;
	std::auto_ptr<MessageEnumerator> pEnum;
	if (pMessageSelectionModel_) {
		pMessageSelectionModel_->getSelectedMessageHolders(&lock, &pFolder, &l);
		if (!l.empty())
			pAccount = lock.get();
		pEnum = pMessageSelectionModel_->getFocusedMessage();
	}
	else {
		pAccount = pAccountSelectionModel_->getAccount();
	}
	
	unsigned int nSecurityMode = pSecurityModel_->getSecurityMode();
	MessageHolder* pmh = 0;
	Message msg;
	Message* pMessage = 0;
	if (pEnum.get() && pEnum->next()) {
		pmh = pEnum->getMessageHolder();
		if (pmh) {
			pMessage = &msg;
		}
		else {
			pMessage = pEnum->getMessage(
				Account::GETMESSAGEFLAG_ALL, 0, nSecurityMode, &msg);
			if (!pMessage)
				return 0;
		}
	}
	
	MacroVariableHolder globalVariable;
	MacroContext context(pmh, pMessage, pAccount, l, pFolder,
		pDocument_, 0, 0, pProfile_, 0, MacroContext::FLAG_UITHREAD,
		nSecurityMode, 0, &globalVariable);
	MacroValuePtr pValue(pMacro->value(&context));
	if (!pValue.get())
		return 0;
	
	return pValue->string().release();
}

const ActionItem* qm::MacroMenuCreator::getActionItem(const WCHAR* pwszAction) const
{
	assert(pwszAction);
	
	ActionItem item = {
		pwszAction,
		0
	};
	
	const ActionItem* pItem = std::lower_bound(
		pActionItem_, pActionItem_ + nActionItemCount_, item,
		boost::bind(string_less<WCHAR>(),
			boost::bind(&ActionItem::pwszAction_, _1),
			boost::bind(&ActionItem::pwszAction_, _2)));
	if (pItem == pActionItem_ + nActionItemCount_ ||
		wcscmp(pItem->pwszAction_, pwszAction) != 0 ||
		(pItem->nFlags_ != 0 && !(pItem->nFlags_ & ActionItem::FLAG_MENU)))
		return 0;
	return pItem;
}

void qm::MacroMenuCreator::parseItems(WCHAR* pwsz,
									  ItemList* pList)
{
	assert(pwsz);
	assert(pList);
	
	WCHAR* p = wcstok(pwsz, L"\n");
	while (p) {
		WCHAR* pParam = wcschr(p, L'\t');
		if (pParam) {
			*pParam = L'\0';
			pList->push_back(std::make_pair(p, pParam + 1));
		}
		p = wcstok(0, L"\n");
	}
}


/****************************************************************************
 *
 * MacroDynamicMenuItem
 *
 */

qm::MacroDynamicMenuItem::MacroDynamicMenuItem(unsigned int nId,
											   const WCHAR* pwszName,
											   const WCHAR* pwszParam) :
	DynamicMenuItem(nId, pwszName, pwszParam)
{
	if (pwszParam)
		pMacro_ = MacroParser().parse(pwszParam);
}

qm::MacroDynamicMenuItem::~MacroDynamicMenuItem()
{
}

const Macro* qm::MacroDynamicMenuItem::getMacro() const
{
	return pMacro_.get();
}


/****************************************************************************
 *
 * MacroDynamicMenuItem
 *
 */

qm::MacroDynamicMenuMap::MacroDynamicMenuMap()
{
}

qm::MacroDynamicMenuMap::~MacroDynamicMenuMap()
{
}

std::auto_ptr<DynamicMenuItem> qm::MacroDynamicMenuMap::createItem(unsigned int nId,
																   const WCHAR* pwszName,
																   const WCHAR* pwszParam) const
{
	return std::auto_ptr<DynamicMenuItem>(new MacroDynamicMenuItem(nId, pwszName, pwszParam));
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


/****************************************************************************
 *
 * MenuCreatorList
 *
 */

qm::MenuCreatorList::MenuCreatorList(MenuCreatorListCallback* pCallback) :
	pCallback_(pCallback)
{
}

qm::MenuCreatorList::~MenuCreatorList()
{
	std::for_each(list_.begin(), list_.end(), qs::deleter<MenuCreator>());
}

void qm::MenuCreatorList::add(std::auto_ptr<MenuCreator> pMenuCreator)
{
	list_.push_back(pMenuCreator.get());
	pMenuCreator.release();
}

DynamicMenuCreator* qm::MenuCreatorList::get(const qs::DynamicMenuItem* pItem) const
{
	if (pItem->getParam()) {
		if (!pMacroMenuCreator_.get() && pCallback_)
			pMacroMenuCreator_ = pCallback_->createMacroMenuCreator();
		return pMacroMenuCreator_.get();
	}
	else {
		List::const_iterator it = std::find_if(list_.begin(), list_.end(),
			boost::bind(string_equal<WCHAR>(),
				boost::bind(&MenuCreator::getName, _1), pItem->getName()));
		return it != list_.end() ? *it : 0;
	}
}


/****************************************************************************
 *
 * MenuCreatorListCallback
 *
 */

qm::MenuCreatorListCallback::~MenuCreatorListCallback()
{
}
