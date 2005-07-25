/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#pragma warning(disable:4786)

#include <qmaccount.h>
#include <qmapplication.h>
#include <qmdocument.h>
#include <qmfilenames.h>
#include <qmrecents.h>
#include <qmsecurity.h>
#include <qmjunk.h>

#include <qsconv.h>
#include <qsosutil.h>
#include <qsstl.h>
#include <qsstring.h>

#include <vector>
#include <algorithm>

#include <tchar.h>
#include <shlobj.h>

#include "account.h"
#include "addressbook.h"
#include "fixedformtext.h"
#include "recentaddress.h"
#include "rule.h"
#include "signature.h"
#include "templatemanager.h"
#include "undo.h"
#include "uri.h"
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
	typedef std::vector<AccountManagerHandler*> AccountManagerHandlerList;
	typedef std::vector<DocumentHandler*> DocumentHandlerList;
	
	void setOffline(bool bOffline);
	void fireAccountListChanged(AccountManagerEvent::Type type,
								Account* pAccount) const;
	void fireOfflineStatusChanged();
	void fireDocumentInitialized();
	
	Document* pThis_;
	Profile* pProfile_;
	PasswordManager* pPasswordManager_;
	Document::AccountList listAccount_;
	AccountManagerHandlerList listAccountManagerHandler_;
	DocumentHandlerList listDocumentHandler_;
	std::auto_ptr<RuleManager> pRuleManager_;
	std::auto_ptr<TemplateManager> pTemplateManager_;
	std::auto_ptr<ScriptManager> pScriptManager_;
	std::auto_ptr<SignatureManager> pSignatureManager_;
	std::auto_ptr<FixedFormTextManager> pFixedFormTextManager_;
	std::auto_ptr<AddressBook> pAddressBook_;
	std::auto_ptr<RecentAddress> pRecentAddress_;
	std::auto_ptr<Security> pSecurity_;
	std::auto_ptr<Recents> pRecents_;
	std::auto_ptr<UndoManager> pUndoManager_;
	std::auto_ptr<JunkFilter> pJunkFilter_;
	unsigned int nOnline_;
};

void qm::DocumentImpl::setOffline(bool bOffline)
{
	for (Document::AccountList::iterator it = listAccount_.begin(); it != listAccount_.end(); ++it)
		(*it)->setOffline(bOffline);
	
	fireOfflineStatusChanged();
}

void qm::DocumentImpl::fireAccountListChanged(AccountManagerEvent::Type type,
											  Account* pAccount) const
{
	AccountManagerEvent event(pThis_, type, pAccount);
	for (AccountManagerHandlerList::const_iterator it = listAccountManagerHandler_.begin(); it != listAccountManagerHandler_.end(); ++it)
		(*it)->accountListChanged(event);
}

void qm::DocumentImpl::fireOfflineStatusChanged()
{
	DocumentEvent event(pThis_);
	for (DocumentHandlerList::const_iterator it = listDocumentHandler_.begin(); it != listDocumentHandler_.end(); ++it)
		(*it)->offlineStatusChanged(event);
}

void qm::DocumentImpl::fireDocumentInitialized()
{
	DocumentEvent event(pThis_);
	for (DocumentHandlerList::const_iterator it = listDocumentHandler_.begin(); it != listDocumentHandler_.end(); ++it)
		(*it)->documentInitialized(event);
}


/****************************************************************************
 *
 * Document
 *
 */

qm::Document::Document(Profile* pProfile,
					   PasswordManager* pPasswordManager) :
	pImpl_(0)
{
	const Application& app = Application::getApplication();
	const WCHAR* pwszMailFolder = app.getMailFolder();
	
	pImpl_ = new DocumentImpl();
	pImpl_->pThis_ = this;
	pImpl_->pProfile_ = pProfile;
	pImpl_->pPasswordManager_ = pPasswordManager;
	pImpl_->pRuleManager_.reset(new RuleManager(app.getProfilePath(FileNames::RULES_XML).get()));
	pImpl_->pTemplateManager_.reset(new TemplateManager(pwszMailFolder));
	pImpl_->pScriptManager_.reset(new ScriptManager(pwszMailFolder));
	pImpl_->pSignatureManager_.reset(new SignatureManager(app.getProfilePath(FileNames::SIGNATURES_XML).get()));
	pImpl_->pFixedFormTextManager_.reset(new FixedFormTextManager(app.getProfilePath(FileNames::TEXTS_XML).get()));
	pImpl_->pAddressBook_.reset(new AddressBook(app.getProfilePath(FileNames::ADDRESSBOOK_XML).get(), pProfile, true));
	pImpl_->pRecentAddress_.reset(new RecentAddress(pImpl_->pAddressBook_.get(), pProfile));
	pImpl_->pSecurity_.reset(new Security(pwszMailFolder, pProfile));
	pImpl_->pRecents_.reset(new Recents(this, pProfile));
	pImpl_->pUndoManager_.reset(new UndoManager());
	
	JunkFilterFactory* pJunkFilterFactory = JunkFilterFactory::getFactory();
	if (pJunkFilterFactory) {
		wstring_ptr wstrJunkPath(concat(pwszMailFolder, L"\\junk"));
		pImpl_->pJunkFilter_ = pJunkFilterFactory->createJunkFilter(wstrJunkPath.get(), pProfile);
	}
	
	pImpl_->nOnline_ = 0;
}

qm::Document::~Document()
{
	if (pImpl_) {
		AccountList& l = pImpl_->listAccount_;
		std::for_each(l.begin(), l.end(), deleter<Account>());
		delete pImpl_;
		pImpl_ = 0;
	}
}

Account* qm::Document::getAccount(const WCHAR* pwszName) const
{
	assert(pwszName);
	
	AccountList::const_iterator it = std::find_if(
		pImpl_->listAccount_.begin(), pImpl_->listAccount_.end(),
		AccountNameEqual(pwszName));
	return it != pImpl_->listAccount_.end() ? *it : 0;
}

const Document::AccountList& qm::Document::getAccounts() const
{
	return pImpl_->listAccount_;
}

bool qm::Document::hasAccount(const WCHAR* pwszName) const
{
	assert(pwszName);
	
	return std::find_if(pImpl_->listAccount_.begin(), pImpl_->listAccount_.end(),
		AccountNameEqual(pwszName)) != pImpl_->listAccount_.end();
}

void qm::Document::addAccount(std::auto_ptr<Account> pAccount)
{
	assert(pAccount.get());
	
	const WCHAR* pwszName = pAccount->getName();
	assert(!hasAccount(pwszName));
	
	AccountList& l = pImpl_->listAccount_;
	assert(std::find(l.begin(), l.end(), pAccount.get()) == l.end());
	AccountList::iterator it = std::lower_bound(
		l.begin(), l.end(), pAccount.get(), AccountLess());
	l.insert(it, pAccount.get());
	Account* p = pAccount.release();
	
	pImpl_->fireAccountListChanged(AccountManagerEvent::TYPE_ADD, p);
}

void qm::Document::removeAccount(Account* pAccount)
{
	assert(pAccount);
	
	AccountList& l = pImpl_->listAccount_;
	AccountList::iterator it = std::find(l.begin(), l.end(), pAccount);
	assert(it != l.end());
	
	l.erase(it);
	
	pImpl_->fireAccountListChanged(AccountManagerEvent::TYPE_REMOVE, pAccount);
	
	pAccount->deletePermanent(true);
	delete pAccount;
}

bool qm::Document::renameAccount(Account* pAccount,
								 const WCHAR* pwszName)
{
	assert(pAccount);
	assert(pwszName);
	
	if (!pAccount->save(false))
		return false;
	
	wstring_ptr wstrOldPath(allocWString(pAccount->getPath()));
	
	AccountList& l = pImpl_->listAccount_;
	AccountList::iterator it = std::find(l.begin(), l.end(), pAccount);
	assert(it != l.end());
	
	pAccount->deletePermanent(false);
	l.erase(it);
	
	std::auto_ptr<Account> pOldAccount(pAccount);
	pImpl_->fireAccountListChanged(AccountManagerEvent::TYPE_REMOVE, pAccount);
	pOldAccount.reset(0);
	
	const WCHAR* p = wcsrchr(wstrOldPath.get(), L'\\');
	assert(p);
	
	wstring_ptr wstrNewPath(concat(wstrOldPath.get(),
		p - wstrOldPath.get() + 1, pwszName, -1));
	
	W2T(wstrOldPath.get(), ptszOldPath);
	W2T(wstrNewPath.get(), ptszNewPath);
	if (!::MoveFile(ptszOldPath, ptszNewPath))
		return false;
	
	std::auto_ptr<Account> pNewAccount(new Account(wstrNewPath.get(),
		pImpl_->pSecurity_.get(), pImpl_->pPasswordManager_, pImpl_->pJunkFilter_.get()));
	it = std::lower_bound(l.begin(), l.end(), pNewAccount.get(), AccountLess());
	l.insert(it, pNewAccount.get());
	pAccount = pNewAccount.release();
	
	pImpl_->fireAccountListChanged(AccountManagerEvent::TYPE_ADD, pAccount);
	
	return true;
}

qm::Folder* qm::Document::getFolder(Account* pAccount,
									const WCHAR* pwszName) const
{
	assert(pwszName);
	
	if (*pwszName == L'/' && *(pwszName + 1) == L'/') {
		const WCHAR* p = wcschr(pwszName + 2, L'/');
		if (p) {
			wstring_ptr wstrAccount(allocWString(pwszName + 2, p - pwszName - 2));
			pAccount = getAccount(wstrAccount.get());
			pwszName = p + 1;
		}
	}
	if (!pAccount)
		return 0;
	
	return pAccount->getFolder(pwszName);
}

MessagePtr qm::Document::getMessage(const URI& uri) const
{
	Account* pAccount = getAccount(uri.getAccount());
	if (!pAccount)
		return MessagePtr();
	
	Folder* pFolder = pAccount->getFolder(uri.getFolder());
	if (!pFolder ||
		pFolder->getType() != Folder::TYPE_NORMAL ||
		static_cast<NormalFolder*>(pFolder)->getValidity() != uri.getValidity())
		return MessagePtr();
	return MessagePtr(static_cast<NormalFolder*>(pFolder)->getMessageById(uri.getId()));
}

void qm::Document::addAccountManagerHandler(AccountManagerHandler* pHandler)
{
	pImpl_->listAccountManagerHandler_.push_back(pHandler);
}

void qm::Document::removeAccountManagerHandler(AccountManagerHandler* pHandler)
{
	DocumentImpl::AccountManagerHandlerList& l = pImpl_->listAccountManagerHandler_;
	DocumentImpl::AccountManagerHandlerList::iterator it =
		std::remove(l.begin(), l.end(), pHandler);
	l.erase(it, l.end());
}

bool qm::Document::loadAccounts(const WCHAR* pwszPath)
{
	assert(pwszPath);
	
	assert(*(pwszPath + wcslen(pwszPath) - 1) != L'\\');
	wstring_ptr wstrFind(concat(pwszPath, L"\\*.*"));
	W2T(wstrFind.get(), ptszFind);
	
	AccountList& l = pImpl_->listAccount_;
	
	struct AccountDestroy
	{
		AccountDestroy(AccountList& l) :
			p_(&l)
		{
		}
		
		~AccountDestroy()
		{
			if (p_) {
				std::for_each(p_->begin(), p_->end(), deleter<Account>());
				p_->clear();
			}
		};
		
		void release()
		{
			p_ = 0;
		}
		
		AccountList* p_;
	} accountDestroy(l);
	
	
	WIN32_FIND_DATA fd;
	AutoFindHandle hFind(::FindFirstFile(ptszFind, &fd));
	if (hFind.get()) {
		do {
			T2W(fd.cFileName, pwszName);
			wstring_ptr wstrPath(concat(pwszPath, L"\\", pwszName));
			
			size_t nFileNameLen = _tcslen(fd.cFileName);
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
				if (hFindLink.get())
					wstrPath = tcs2wcs(tszPath);
#else
				WCHAR wszPath[MAX_PATH];
				if (::SHGetShortcutTarget(wstrPath.get(), wszPath, countof(wszPath))) {
					AutoFindHandle hFindLink(::FindFirstFile(wszPath, &fd));
					if (hFindLink.get())
						wstrPath = allocWString(wszPath);
				}
#endif
			}
			if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
				continue;
			if (_tcscmp(fd.cFileName, _T(".")) == 0 ||
				_tcscmp(fd.cFileName, _T("..")) == 0)
				continue;
			
			ConcatW c[] = {
				{ wstrPath.get(),		-1	},
				{ L"\\",				1	},
				{ FileNames::ACCOUNT,	-1	},
				{ FileNames::XML_EXT,	-1	}
			};
			wstrPath = concat(c, countof(c));
			
			W2T(wstrPath.get(), ptszPath);
			DWORD dwAttributes = ::GetFileAttributes(ptszPath);
			if (dwAttributes != 0xffffffff &&
				!(dwAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
				WCHAR* p = wcsrchr(wstrPath.get(), L'\\');
				assert(p);
				*p = L'\0';
				std::auto_ptr<Account> pAccount(new Account(
					wstrPath.get(), pImpl_->pSecurity_.get(),
					pImpl_->pPasswordManager_, pImpl_->pJunkFilter_.get()));
				// TODO ERROR CHECK
				l.push_back(pAccount.get());
				pAccount.release();
			}
		} while (::FindNextFile(hFind.get(), &fd));
	}
	
	accountDestroy.release();
	std::sort(l.begin(), l.end(), AccountLess());
	
	pImpl_->fireAccountListChanged(AccountManagerEvent::TYPE_ALL, 0);
	pImpl_->fireDocumentInitialized();
	
	return true;
}

RuleManager* qm::Document::getRuleManager() const
{
	return pImpl_->pRuleManager_.get();
}

const TemplateManager* qm::Document::getTemplateManager() const
{
	return pImpl_->pTemplateManager_.get();
}

ScriptManager* qm::Document::getScriptManager() const
{
	return pImpl_->pScriptManager_.get();
}

SignatureManager* qm::Document::getSignatureManager() const
{
	return pImpl_->pSignatureManager_.get();
}

FixedFormTextManager* qm::Document::getFixedFormTextManager() const
{
	return pImpl_->pFixedFormTextManager_.get();
}

AddressBook* qm::Document::getAddressBook() const
{
	return pImpl_->pAddressBook_.get();
}

RecentAddress* qm::Document::getRecentAddress() const
{
	return pImpl_->pRecentAddress_.get();
}

Security* qm::Document::getSecurity() const
{
	return pImpl_->pSecurity_.get();
}

Recents* qm::Document::getRecents() const
{
	return pImpl_->pRecents_.get();
}

UndoManager* qm::Document::getUndoManager() const
{
	return pImpl_->pUndoManager_.get();
}

JunkFilter* qm::Document::getJunkFilter() const
{
	return pImpl_->pJunkFilter_.get();
}

bool qm::Document::isOffline() const
{
	return pImpl_->nOnline_ == 0;
}

void qm::Document::setOffline(bool bOffline)
{
	assert((pImpl_->nOnline_ & 0x7fffffff) == 0);
	
	if (bOffline)
		pImpl_->nOnline_ &= 0x7fffffff;
	else
		pImpl_->nOnline_ |= 0x80000000;
	
	pImpl_->setOffline(bOffline);
}

void qm::Document::incrementInternalOnline()
{
	if (pImpl_->nOnline_++ == 0)
		pImpl_->setOffline(false);
}

void qm::Document::decrementInternalOnline()
{
	if (--pImpl_->nOnline_ == 0)
		pImpl_->setOffline(true);
}

bool qm::Document::save(bool bForce)
{
	const AccountList& l = pImpl_->listAccount_;
	for (AccountList::const_iterator it = l.begin(); it != l.end(); ++it) {
		if (!(*it)->save(bForce))
			return false;
	}
	
	pImpl_->pRecents_->save();
	pImpl_->pRecentAddress_->save();
	
	if (pImpl_->pJunkFilter_.get()) {
		if (!pImpl_->pJunkFilter_->save(bForce))
			return false;
	}
	
	pImpl_->pProfile_->setInt(L"Global", L"Offline", isOffline());
	
	return true;
}

void qm::Document::addDocumentHandler(DocumentHandler* pHandler)
{
	pImpl_->listDocumentHandler_.push_back(pHandler);
}

void qm::Document::removeDocumentHandler(DocumentHandler* pHandler)
{
	DocumentImpl::DocumentHandlerList& l = pImpl_->listDocumentHandler_;
	DocumentImpl::DocumentHandlerList::iterator it =
		std::remove(l.begin(), l.end(), pHandler);
	l.erase(it, l.end());
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

void qm::DefaultDocumentHandler::offlineStatusChanged(const DocumentEvent& event)
{
}

void qm::DefaultDocumentHandler::documentInitialized(const DocumentEvent& event)
{
}


/****************************************************************************
 *
 * DocumentEvent
 *
 */

qm::DocumentEvent::DocumentEvent(Document* pDocument) :
	pDocument_(pDocument)
{
}

qm::DocumentEvent::~DocumentEvent()
{
}

Document* qm::DocumentEvent::getDocument() const
{
	return pDocument_;
}
