/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#pragma warning(disable:4786)

#include <qmaccount.h>
#include <qmfolder.h>
#include <qmsearch.h>

#include <qsstl.h>
#include <qsuiutil.h>

#include "../ui/uiutil.h"

using namespace qm;
using namespace qs;


namespace qm {
struct SearchDriverFactoryImpl;
}

/****************************************************************************
 *
 * SearchDriver
 *
 */

qm::SearchDriver::~SearchDriver()
{
}


/****************************************************************************
 *
 * SearchUI
 *
 */

qm::SearchUI::~SearchUI()
{
}


/****************************************************************************
 *
 * SearchPropertyDataImpl
 *
 */

struct qm::SearchPropertyDataImpl
{
	const Account* pAccount_;
	const Folder* pFolder_;
	Profile* pProfile_;
	wstring_ptr wstrCondition_;
	bool bRecursive_;
	bool bNewFolder_;
	unsigned int nImeFlags_;
};


/****************************************************************************
 *
 * SearchPropertyData
 *
 */

qm::SearchPropertyData::SearchPropertyData(const Account* pAccount,
										   const Folder* pFolder,
										   Profile* pProfile) :
	pImpl_(0)
{
	assert(pAccount);
	assert(pProfile);
	
	pImpl_ = new SearchPropertyDataImpl();
	pImpl_->pAccount_ = pAccount;
	pImpl_->pFolder_ = pFolder;
	pImpl_->pProfile_ = pProfile;
	pImpl_->wstrCondition_ = pProfile->getString(L"Search", L"Condition");
	pImpl_->bRecursive_ = pProfile->getInt(L"Search", L"Recursive") != 0;
	pImpl_->bNewFolder_ = pProfile->getInt(L"Search", L"NewFolder") != 0;
	pImpl_->nImeFlags_ = pProfile->getInt(L"Search", L"Ime");
	
	if (!pFolder) {
		pImpl_->bRecursive_ = false;
	}
	else if (pFolder->getType() == Folder::TYPE_QUERY) {
		const QueryFolder* pQueryFolder = static_cast<const QueryFolder*>(pFolder);
		const WCHAR* pwszTarget = pQueryFolder->getTargetFolder();
		pImpl_->pFolder_ = pwszTarget ? pAccount->getFolder(pwszTarget) : 0;
		if (pImpl_->pFolder_)
			pImpl_->bRecursive_ = pQueryFolder->isRecursive();
		else
			pImpl_->bRecursive_ = false;
	}
	else if (pProfile->getInt(L"Search", L"All")) {
		pImpl_->pFolder_ = 0;
		pImpl_->bRecursive_ = false;
	}
}

qm::SearchPropertyData::~SearchPropertyData()
{
	delete pImpl_;
}

const WCHAR* qm::SearchPropertyData::getCondition() const
{
	return pImpl_->wstrCondition_.get();
}

const Account* qm::SearchPropertyData::getAccount() const
{
	return pImpl_->pAccount_;
}

const Folder* qm::SearchPropertyData::getFolder() const
{
	return pImpl_->pFolder_;
}

bool qm::SearchPropertyData::isRecursive() const
{
	return pImpl_->bRecursive_;
}

bool qm::SearchPropertyData::isNewFolder() const
{
	return pImpl_->bNewFolder_;
}

unsigned int qm::SearchPropertyData::getImeFlags() const
{
	return pImpl_->nImeFlags_;
}

void qm::SearchPropertyData::set(const WCHAR* pwszCondition,
								 const Folder* pFolder,
								 bool bRecursive,
								 bool bNewFolder,
								 unsigned int nImeFlags)
{
	pImpl_->wstrCondition_ = allocWString(pwszCondition);
	pImpl_->pFolder_ = pFolder;
	pImpl_->bRecursive_ = bRecursive;
	pImpl_->bNewFolder_ = bNewFolder;
	pImpl_->nImeFlags_ = nImeFlags;
}

void qm::SearchPropertyData::save() const
{
	pImpl_->pProfile_->setString(L"Search", L"Condition", pImpl_->wstrCondition_.get());
	pImpl_->pProfile_->setInt(L"Search", L"All", pImpl_->pFolder_ == 0);
	pImpl_->pProfile_->setInt(L"Search", L"Recursive", pImpl_->bRecursive_);
	pImpl_->pProfile_->setInt(L"Search", L"NewFolder", pImpl_->bNewFolder_);
	pImpl_->pProfile_->setInt(L"Search", L"Ime", pImpl_->nImeFlags_);
}


/****************************************************************************
 *
 * SearchPropertyPage
 *
 */

qm::SearchPropertyPage::SearchPropertyPage(HINSTANCE hInst,
										   UINT nId,
										   UINT nConditionId,
										   UINT nFolderId,
										   UINT nRecursiveId,
										   UINT nNewFolderId,
										   SearchPropertyData* pData) :
	DefaultPropertyPage(hInst, nId),
	nConditionId_(nConditionId),
	nFolderId_(nFolderId),
	nRecursiveId_(nRecursiveId),
	nNewFolderId_(nNewFolderId),
	pData_(pData)
{
}

qm::SearchPropertyPage::~SearchPropertyPage()
{
}

LRESULT qm::SearchPropertyPage::onNotify(NMHDR* pnmhdr,
										 bool* pbHandled)
{
	BEGIN_NOTIFY_HANDLER()
		HANDLE_NOTIFY_CODE(PSN_KILLACTIVE, onKillActive)
		HANDLE_NOTIFY_CODE(PSN_SETACTIVE, onSetActive)
	END_NOTIFY_HANDLER()
	return DefaultPropertyPage::onNotify(pnmhdr, pbHandled);
}

LRESULT qm::SearchPropertyPage::onInitDialog(HWND hwndFocus,
											 LPARAM lParam)
{
	FolderListComboBox(getDlgItem(nFolderId_)).addFolders(
		pData_->getAccount(), pData_->getFolder());
	Button_SetCheck(getDlgItem(nRecursiveId_),
		pData_->isRecursive() ? BST_CHECKED : BST_UNCHECKED);
	
	return TRUE;
}

void qm::SearchPropertyPage::updateData(SearchPropertyData* pData)
{
	wstring_ptr wstrCondition = getDlgItemText(nConditionId_);
	const Folder* pFolder = FolderListComboBox(getDlgItem(nFolderId_)).getSelectedFolder();
	bool bRecursive = Button_GetCheck(getDlgItem(nRecursiveId_)) == BST_CHECKED;
	bool bNewFolder = Button_GetCheck(getDlgItem(nNewFolderId_)) == BST_CHECKED;
	pData->set(wstrCondition.get(), pFolder, bRecursive, bNewFolder, getImeFlags());
}

void qm::SearchPropertyPage::updateUI(const SearchPropertyData* pData)
{
	if (pData->getCondition()) {
		setDlgItemText(nConditionId_, pData->getCondition());
		FolderListComboBox(getDlgItem(nFolderId_)).selectFolder(pData->getFolder());
		Button_SetCheck(getDlgItem(nRecursiveId_),
			pData->isRecursive() ? BST_CHECKED : BST_UNCHECKED);
		Button_SetCheck(getDlgItem(nNewFolderId_),
			pData->isNewFolder() ? BST_CHECKED : BST_UNCHECKED);
	}
	setImeFlags(pData->getImeFlags());
}

unsigned int qm::SearchPropertyPage::getImeFlags() const
{
	unsigned int nFlags = SearchPropertyData::IMEFLAG_NONE;
	if (qs::UIUtil::isImeEnabled(getHandle()))
		nFlags |= SearchPropertyData::IMEFLAG_IME;
#ifdef _WIN32_WCE_PSPC
	if (qs::UIUtil::isSipEnabled())
		nFlags |= SearchPropertyData::IMEFLAG_SIP;
#endif
	return nFlags;
}

void qm::SearchPropertyPage::setImeFlags(unsigned int nFlags)
{
	qs::UIUtil::setImeEnabled(getHandle(), (nFlags & SearchPropertyData::IMEFLAG_IME) != 0);
#ifdef _WIN32_WCE_PSPC
	qs::UIUtil::setSipEnabled((nFlags & SearchPropertyData::IMEFLAG_SIP) != 0);
#endif
}

LRESULT qm::SearchPropertyPage::onKillActive(NMHDR* pnmhdr,
											 bool* pbHandled)
{
	updateData(pData_);
	return 0;
}

LRESULT qm::SearchPropertyPage::onSetActive(NMHDR* pnmhdr,
											bool* pbHandled)
{
	updateUI(pData_);
	return 0;
}


/****************************************************************************
 *
 * SearchDriverFactoryImpl
 *
 */

struct qm::SearchDriverFactoryImpl
{
	typedef std::vector<std::pair<const WCHAR*, SearchDriverFactory*> > FactoryList;
	
	static FactoryList::iterator getIterator(const WCHAR* pwszName);
	
	static FactoryList listFactory__;
};

qm::SearchDriverFactoryImpl::FactoryList qm::SearchDriverFactoryImpl::listFactory__;

qm::SearchDriverFactoryImpl::FactoryList::iterator qm::SearchDriverFactoryImpl::getIterator(const WCHAR* pwszName)
{
	return std::find_if(listFactory__.begin(), listFactory__.end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				string_equal<WCHAR>(),
				std::select1st<FactoryList::value_type>(),
				std::identity<const WCHAR*>()),
			pwszName));
}


/****************************************************************************
 *
 * SearchDriverFactory
 *
 */

qm::SearchDriverFactory::SearchDriverFactory()
{
}

qm::SearchDriverFactory::~SearchDriverFactory()
{
}

std::auto_ptr<SearchDriver> qm::SearchDriverFactory::getDriver(const WCHAR* pwszName,
															   Document* pDocument,
															   Account* pAccount,
															   HWND hwnd,
															   Profile* pProfile)
{
	SearchDriverFactoryImpl::FactoryList::iterator it =
		SearchDriverFactoryImpl::getIterator(pwszName);
	if (it == SearchDriverFactoryImpl::listFactory__.end())
		return std::auto_ptr<SearchDriver>(0);
	else
		return (*it).second->createDriver(pDocument, pAccount, hwnd, pProfile);
}

std::auto_ptr<SearchUI> qm::SearchDriverFactory::getUI(const WCHAR* pwszName,
													   Account* pAccount,
													   Profile* pProfile)
{
	SearchDriverFactoryImpl::FactoryList::iterator it =
		SearchDriverFactoryImpl::getIterator(pwszName);
	if (it == SearchDriverFactoryImpl::listFactory__.end())
		return std::auto_ptr<SearchUI>(0);
	else
		return (*it).second->createUI(pAccount, pProfile);
}

void qm::SearchDriverFactory::getNames(NameList* pList)
{
	assert(pList);
	
	pList->resize(SearchDriverFactoryImpl::listFactory__.size());
	std::transform(SearchDriverFactoryImpl::listFactory__.begin(),
		SearchDriverFactoryImpl::listFactory__.end(),
		pList->begin(),
		std::select1st<SearchDriverFactoryImpl::FactoryList::value_type>());
}

void qm::SearchDriverFactory::registerFactory(const WCHAR* pwszName,
											  SearchDriverFactory* pFactory)
{
	assert(pwszName);
	assert(pFactory);
	
	SearchDriverFactoryImpl::listFactory__.push_back(std::make_pair(pwszName, pFactory));
}

void qm::SearchDriverFactory::unregisterFactory(const WCHAR* pwszName)
{
	assert(pwszName);
	
	SearchDriverFactoryImpl::FactoryList::iterator it =
		SearchDriverFactoryImpl::getIterator(pwszName);
	if (it != SearchDriverFactoryImpl::listFactory__.end())
		SearchDriverFactoryImpl::listFactory__.erase(it);
}


/****************************************************************************
 *
 * SearchContext
 *
 */

qm::SearchContext::SearchContext(const WCHAR* pwszCondition,
								 const WCHAR* pwszTargetFolder,
								 bool bRecursive,
								 unsigned int nSecurityMode) :
	bRecursive_(bRecursive),
	nSecurityMode_(nSecurityMode)
{
	wstrCondition_ = (allocWString(pwszCondition));
	if (pwszTargetFolder)
		wstrTargetFolder_ = allocWString(pwszTargetFolder);
}

qm::SearchContext::~SearchContext()
{
}

const WCHAR* qm::SearchContext::getCondition() const
{
	return wstrCondition_.get();
}

const WCHAR* qm::SearchContext::getTargetFolder() const
{
	return wstrTargetFolder_.get();
}

bool qm::SearchContext::isRecursive() const
{
	return bRecursive_;
}

unsigned int qm::SearchContext::getSecurityMode() const
{
	return nSecurityMode_;
}

void qm::SearchContext::getTargetFolders(Account* pAccount,
										 FolderList* pList) const
{
	pAccount->getNormalFolders(wstrTargetFolder_.get(), bRecursive_, pList);
}
