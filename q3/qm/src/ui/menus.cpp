/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmaccount.h>
#include <qmapplication.h>
#include <qmmessage.h>
#include <qmmessageholder.h>

#include <qsassert.h>
#include <qsconv.h>
#include <qserror.h>
#include <qsstl.h>

#include <algorithm>

#include <tchar.h>

#include "menus.h"
#include "resourceinc.h"
#include "uiutil.h"
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
	status = parser.getAttachments(&l);
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

QSTATUS qm::AttachmentMenu::createMenu(HMENU hmenu, const MessagePtrList& l)
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
	MessagePtrList::const_iterator itM = l.begin();
	while (itM != l.end()) {
		MessagePtrLock mpl(*itM);
		if (mpl) {
			status = STLWrapper<List>(list_).push_back(
				List::value_type(nId, mpl));
			CHECK_QSTATUS();
			
			Message msg(&status);
			CHECK_QSTATUS();
			status = mpl->getMessage(Account::GETMESSAGEFLAG_TEXT, 0, &msg);
			CHECK_QSTATUS();
			AttachmentParser parser(msg);
			AttachmentParser::AttachmentList list;
			AttachmentParser::AttachmentListFree free(list);
			status = parser.getAttachments(&list);
			CHECK_QSTATUS();
			AttachmentParser::AttachmentList::iterator itA = list.begin();
			while (itA != list.end()) {
				string_ptr<WSTRING> wstrName;
				status = UIUtil::formatMenu((*itA).first, &wstrName);
				CHECK_QSTATUS();
				W2T(wstrName.get(), ptszName);
				::InsertMenu(hmenu, nIdNext,
					MF_BYCOMMAND | MF_STRING, nId++, ptszName);
				++itA;
			}
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
	while (p) {
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

QSTATUS qm::MoveMenu::createMenu(HMENU hmenu,
	Account* pAccount, const ActionMap& actionMap)
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
	std::remove_copy_if(l.begin(), l.end(),
		std::back_inserter(listFolder),
		std::mem_fun(&Folder::isHidden));
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
	while (it != listFolder.end()) {
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
			while (it != listFolder.end() && isDescendant(pFolder, *it))
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
	for (++first; first != last && isDescendant(pFolder, *first); ++first) {
		if ((*first)->getType() == Folder::TYPE_NORMAL &&
			!(*first)->isFlag(Folder::FLAG_NOSELECT))
			return true;
	}
	
	return false;
}

bool qm::MoveMenu::isDescendant(const Folder* pParent, const Folder* pChild)
{
	assert(pParent);
	assert(pChild);
	assert(pParent != pChild);
	
	while (pChild) {
		if (pParent == pChild)
			return true;
		pChild = pChild->getParentFolder();
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
	while (it != l.end()) {
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
	while (it != l.end()) {
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
