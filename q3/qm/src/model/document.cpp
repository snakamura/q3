/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmapplication.h>
#include <qmdocument.h>
#include <qmaccount.h>
#include <qmextensions.h>
#include <qmsecurity.h>

#include <qserror.h>
#include <qsnew.h>
#include <qsstl.h>
#include <qsstring.h>
#include <qsconv.h>
#include <qsosutil.h>

#include <vector>
#include <algorithm>

#include <tchar.h>
#include <shlobj.h>

#include "account.h"
#include "addressbook.h"
#include "rule.h"
#include "signature.h"
#include "templatemanager.h"
#include "../script/scriptmanager.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * DocumentImpl
 *
 */

struct qm::DocumentImpl
{
	typedef std::vector<DocumentHandler*> DocumentHandlerList;
	
	QSTATUS fireAccountListChanged(AccountListChangedEvent::Type type,
		Account* pAccount) const;
	
	Document* pThis_;
	Profile* pProfile_;
	Document::AccountList listAccount_;
	DocumentHandlerList listDocumentHandler_;
	RuleManager* pRuleManager_;
	TemplateManager* pTemplateManager_;
	ScriptManager* pScriptManager_;
	SignatureManager* pSignatureManager_;
	AddressBook* pAddressBook_;
	Security* pSecurity_;
	bool bOffline_;
	bool bCheckNewMail_;
};

QSTATUS qm::DocumentImpl::fireAccountListChanged(
	AccountListChangedEvent::Type type, Account* pAccount) const
{
	DECLARE_QSTATUS();
	
	AccountListChangedEvent event(pThis_, type, pAccount);
	DocumentHandlerList::const_iterator it = listDocumentHandler_.begin();
	while (it != listDocumentHandler_.end()) {
		status = (*it++)->accountListChanged(event);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * Document
 *
 */

qm::Document::Document(Profile* pProfile, QSTATUS* pstatus) :
	pImpl_(0)
{
	if (*pstatus != QSTATUS_SUCCESS)
		return;
	
	DECLARE_QSTATUS();
	
	const WCHAR* pwszMailFolder =
		Application::getApplication().getMailFolder();
	
	std::auto_ptr<RuleManager> pRuleManager;
	status = newQsObject(&pRuleManager);
	CHECK_QSTATUS_SET(pstatus);
	
	std::auto_ptr<TemplateManager> pTemplateManager;
	status = newQsObject(pwszMailFolder, &pTemplateManager);
	CHECK_QSTATUS_SET(pstatus);
	
	std::auto_ptr<ScriptManager> pScriptManager;
	status = newQsObject(pwszMailFolder, &pScriptManager);
	CHECK_QSTATUS_SET(pstatus);
	
	std::auto_ptr<SignatureManager> pSignatureManager;
	status = newQsObject(&pSignatureManager);
	CHECK_QSTATUS_SET(pstatus);
	
	std::auto_ptr<Security> pSecurity;
	status = newQsObject(pwszMailFolder, &pSecurity);
	CHECK_QSTATUS_SET(pstatus);
	
	std::auto_ptr<AddressBook> pAddressBook;
	status = newQsObject(pSecurity.get(), &pAddressBook);
	CHECK_QSTATUS_SET(pstatus);
	
	int nCheckNewMail = 0;
	status = pProfile->getInt(L"NewMailCheck", L"Enable", 0, &nCheckNewMail);
	CHECK_QSTATUS_SET(pstatus);
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	pImpl_->pThis_ = this;
	pImpl_->pProfile_ = pProfile;
	pImpl_->pRuleManager_ = pRuleManager.release();
	pImpl_->pTemplateManager_ = pTemplateManager.release();
	pImpl_->pScriptManager_ = pScriptManager.release();
	pImpl_->pSignatureManager_ = pSignatureManager.release();
	pImpl_->pAddressBook_ = pAddressBook.release();
	pImpl_->pSecurity_ = pSecurity.release();
	pImpl_->bOffline_ = true;
	pImpl_->bCheckNewMail_ = nCheckNewMail != 0;
}

qm::Document::~Document()
{
	if (pImpl_) {
		AccountList& l = pImpl_->listAccount_;
		std::for_each(l.begin(), l.end(), deleter<Account>());
		delete pImpl_->pRuleManager_;
		delete pImpl_->pTemplateManager_;
		delete pImpl_->pScriptManager_;
		delete pImpl_->pSignatureManager_;
		delete pImpl_->pAddressBook_;
		delete pImpl_->pSecurity_;
		delete pImpl_;
		pImpl_ = 0;
	}
}

Account* qm::Document::getAccount(const WCHAR* pwszName) const
{
	assert(pwszName);
	
	DECLARE_QSTATUS();
	
	AccountList& l = pImpl_->listAccount_;
	AccountList::const_iterator it = std::find_if(
		l.begin(), l.end(), AccountNameEqual(pwszName));
	return it != l.end() ? *it : 0;
}

const Document::AccountList& qm::Document::getAccounts() const
{
	return pImpl_->listAccount_;
}

bool qm::Document::hasAccount(const WCHAR* pwszName) const
{
	assert(pwszName);
	
	AccountList& l = pImpl_->listAccount_;
	return std::find_if(l.begin(), l.end(),
		AccountNameEqual(pwszName)) != l.end();
}

QSTATUS qm::Document::addAccount(Account* pAccount)
{
	assert(pAccount);
	
	DECLARE_QSTATUS();

	const WCHAR* pwszName = pAccount->getName();
	assert(!hasAccount(pwszName));
	
	AccountList& l = pImpl_->listAccount_;
	assert(std::find(l.begin(), l.end(), pAccount) == l.end());
	AccountList::iterator it = std::lower_bound(l.begin(),
		l.end(), pAccount, AccountLess());
	status = STLWrapper<AccountList>(l).insert(it, pAccount, &it);
	CHECK_QSTATUS();
	
	status = pImpl_->fireAccountListChanged(
		AccountListChangedEvent::TYPE_ADD, pAccount);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::Document::removeAccount(Account* pAccount)
{
	assert(pAccount);
	
	DECLARE_QSTATUS();
	
	AccountList& l = pImpl_->listAccount_;
	AccountList::iterator it = std::find(l.begin(), l.end(), pAccount);
	assert(it != l.end());
	
	status = pAccount->remove();
	CHECK_QSTATUS();
	l.erase(it);
	delete pAccount;
	
	status = pImpl_->fireAccountListChanged(
		AccountListChangedEvent::TYPE_REMOVE, pAccount);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::Document::renameAccount(Account* pAccount, const WCHAR* pwszName)
{
	assert(pAccount);
	assert(pwszName);
	
	DECLARE_QSTATUS();
	
	status = pAccount->rename(pwszName);
	CHECK_QSTATUS();
	
	AccountList& l = pImpl_->listAccount_;
	std::sort(l.begin(), l.end(), AccountLess());
	
	status = pImpl_->fireAccountListChanged(
		AccountListChangedEvent::TYPE_RENAME, pAccount);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::Document::loadAccounts(const WCHAR* pwszPath)
{
	assert(pwszPath);
	
	DECLARE_QSTATUS();
	
	assert(*(pwszPath + wcslen(pwszPath) - 1) != L'\\');
	string_ptr<WSTRING> wstrFind(concat(pwszPath, L"\\*.*"));
	if (!wstrFind.get())
		return QSTATUS_OUTOFMEMORY;
	W2T(wstrFind.get(), ptszFind);
	
	AccountList& l = pImpl_->listAccount_;
	
	struct AccountDestroy
	{
		AccountDestroy(AccountList& l) : p_(&l) {}
		~AccountDestroy()
		{
			if (p_) {
				std::for_each(p_->begin(), p_->end(), deleter<Account>());
				p_->clear();
			}
		};
		void release() { p_ = 0; }
		AccountList* p_;
	} accountDestroy(l);
	
	
	WIN32_FIND_DATA fd;
	AutoFindHandle hFind(::FindFirstFile(ptszFind, &fd));
	if (hFind.get()) {
		do {
			T2W(fd.cFileName, pwszName);
			string_ptr<WSTRING> wstrPath(concat(pwszPath, L"\\", pwszName));
			if (!wstrPath.get())
				return QSTATUS_OUTOFMEMORY;
			
			int nFileNameLen = _tcslen(fd.cFileName);
			if (nFileNameLen > 4 &&
				_tcscmp(fd.cFileName + nFileNameLen - 4, _T(".lnk")) == 0) {
#ifndef _WIN32_WCE
				ComPtr<IShellLink> pShellLink;
				HRESULT hr = ::CoCreateInstance(CLSID_ShellLink, 0,
					CLSCTX_INPROC_SERVER, IID_IShellLink,
					reinterpret_cast<void**>(&pShellLink));
				if (FAILED(hr))
					continue;
				
				ComPtr<IPersistFile> pPersistFile;
				hr = pShellLink->QueryInterface(IID_IPersistFile,
					reinterpret_cast<void**>(&pPersistFile));
				if (FAILED(hr))
					continue;
				
				hr = pPersistFile->Load(wstrPath.get(), STGM_READ);
				if (FAILED(hr))
					continue;
				
				hr = pShellLink->Resolve(0, SLR_NO_UI);
				if (FAILED(hr))
					continue;
				
				TCHAR tszPath[MAX_PATH];
				hr = pShellLink->GetPath(tszPath, countof(tszPath), &fd, 0);
				if (FAILED(hr))
					continue;
				
				AutoFindHandle hFindLink(::FindFirstFile(tszPath, &fd));
				if (hFindLink.get()) {
					wstrPath.reset(tcs2wcs(tszPath));
					if (!wstrPath.get())
						return QSTATUS_OUTOFMEMORY;
				}
#else
				WCHAR wszPath[MAX_PATH];
				if (::SHGetShortcutTarget(
					wstrPath.get(), wszPath, countof(wszPath))) {
					AutoFindHandle hFindLink(::FindFirstFile(wszPath, &fd));
					if (hFindLink.get()) {
						wstrPath.reset(allocWString(wszPath));
						if (!wstrPath.get())
							return QSTATUS_OUTOFMEMORY;
					}
				}
#endif
			}
			if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
				continue;
			if (_tcscmp(fd.cFileName, _T(".")) == 0 ||
				_tcscmp(fd.cFileName, _T("..")) == 0)
				continue;
			
			WSTRING wstr = concat(wstrPath.get(), L"\\", Extensions::ACCOUNT);
			if (!wstr)
				return QSTATUS_OUTOFMEMORY;
			wstrPath.reset(wstr);
			
			W2T(wstrPath.get(), ptszPath);
			DWORD dwAttributes = ::GetFileAttributes(ptszPath);
			if (dwAttributes != 0xffffffff &&
				!(dwAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
				WCHAR* p = wcsrchr(wstrPath.get(), L'\\');
				assert(p);
				*p = L'\0';
				Account* pAccount = 0;
				status = newQsObject(wstrPath.get(), pImpl_->pSecurity_, &pAccount);
				CHECK_QSTATUS();
				status = STLWrapper<AccountList>(l).push_back(pAccount);
				CHECK_QSTATUS();
			}
		} while (::FindNextFile(hFind.get(), &fd));
	}
	
	accountDestroy.release();
	std::sort(l.begin(), l.end(), AccountLess());
	
	status = pImpl_->fireAccountListChanged(
		AccountListChangedEvent::TYPE_ALL, 0);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::Document::getFolder(Account* pAccount,
	const WCHAR* pwszName, Folder** ppFolder) const
{
	assert(pAccount);
	assert(pwszName);
	assert(ppFolder);
	
	*ppFolder = 0;
	
	if (*pwszName == L'/' && *(pwszName + 1) == L'/') {
		const WCHAR* p = wcschr(pwszName + 2, L'/');
		if (p) {
			string_ptr<WSTRING> wstrAccount(
				allocWString(pwszName + 2, p - pwszName - 2));
			if (!wstrAccount.get())
				return QSTATUS_OUTOFMEMORY;
			pAccount = getAccount(wstrAccount.get());
			if (!pAccount)
				return QSTATUS_SUCCESS;
			pwszName = p + 1;
		}
	}
	
	return pAccount->getFolder(pwszName, ppFolder);
}

RuleManager* qm::Document::getRuleManager() const
{
	return pImpl_->pRuleManager_;
}

const TemplateManager* qm::Document::getTemplateManager() const
{
	return pImpl_->pTemplateManager_;
}

ScriptManager* qm::Document::getScriptManager() const
{
	return pImpl_->pScriptManager_;
}

SignatureManager* qm::Document::getSignatureManager() const
{
	return pImpl_->pSignatureManager_;
}

AddressBook* qm::Document::getAddressBook() const
{
	return pImpl_->pAddressBook_;
}

const Security* qm::Document::getSecurity() const
{
	return pImpl_->pSecurity_;
}

bool qm::Document::isOffline() const
{
	return pImpl_->bOffline_;
}

QSTATUS qm::Document::setOffline(bool bOffline)
{
	DECLARE_QSTATUS();
	
	pImpl_->bOffline_ = bOffline;
	
	AccountList::iterator it = pImpl_->listAccount_.begin();
	while (it != pImpl_->listAccount_.end()) {
		status = (*it)->setOffline(bOffline);
		CHECK_QSTATUS();
		++it;
	}
	
	return QSTATUS_SUCCESS;
}

bool qm::Document::isCheckNewMail() const
{
	return pImpl_->bCheckNewMail_;
}

void qm::Document::setCheckNewMail(bool bCheckNewMail)
{
	pImpl_->bCheckNewMail_ = bCheckNewMail;
}

QSTATUS qm::Document::save()
{
	DECLARE_QSTATUS();
	
	AccountList::iterator it = pImpl_->listAccount_.begin();
	while (it != pImpl_->listAccount_.end()) {
		status = (*it)->save();
		CHECK_QSTATUS();
		++it;
	}
	
	status = pImpl_->pProfile_->setInt(L"Global", L"Offline", isOffline());
	CHECK_QSTATUS();
	status = pImpl_->pProfile_->setInt(
		L"NewMailCheck", L"Enable", isCheckNewMail());
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::Document::addDocumentHandler(DocumentHandler* pHandler)
{
	return STLWrapper<DocumentImpl::DocumentHandlerList>(
		pImpl_->listDocumentHandler_).push_back(pHandler);
}

QSTATUS qm::Document::removeDocumentHandler(DocumentHandler* pHandler)
{
	DocumentImpl::DocumentHandlerList& l = pImpl_->listDocumentHandler_;
	DocumentImpl::DocumentHandlerList::iterator it =
		std::remove(l.begin(), l.end(), pHandler);
	l.erase(it, l.end());
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * DocumentHandler
 *
 */

qm::DocumentHandler::~DocumentHandler()
{
}


/****************************************************************************
 *
 * DefaultDocumentHandler
 *
 */

qm::DefaultDocumentHandler::DefaultDocumentHandler()
{
}

qm::DefaultDocumentHandler::~DefaultDocumentHandler()
{
}

QSTATUS qm::DefaultDocumentHandler::accountListChanged(
	const AccountListChangedEvent& event)
{
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * AccountListChangedEvent
 *
 */

qm::AccountListChangedEvent::AccountListChangedEvent(
	Document* pDocument, Type type, Account* pAccount) :
	type_(type),
	pAccount_(pAccount)
{
}

qm::AccountListChangedEvent::~AccountListChangedEvent()
{
}

qm::AccountListChangedEvent::Type qm::AccountListChangedEvent::getType() const
{
	return type_;
}

Account* qm::AccountListChangedEvent::getAccount() const
{
	return pAccount_;
}
