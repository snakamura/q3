/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmaccount.h>
#include <qmextensions.h>
#include <qmfolder.h>
#include <qmmessageholder.h>
#include <qmprotocoldriver.h>

#include <qsconv.h>
#include <qserror.h>
#include <qsnew.h>
#include <qsosutil.h>
#include <qsprofile.h>
#include <qsstream.h>
#include <qsthread.h>

#include <algorithm>

#include "account.h"
#include "messagecache.h"
#include "messagestore.h"

#pragma warning(disable:4786)

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * AccountImpl
 *
 */

class qm::AccountImpl
{
public:
	typedef std::vector<AccountHandler*> AccountHandlerList;

public:
	QSTATUS loadFolders();
	QSTATUS saveFolders() const;
	
	QSTATUS loadSubAccounts();
	QSTATUS saveSubAccounts() const;
	
	QSTATUS fireFolderListChanged(const FolderListChangedEvent& event);

public:
	static QSTATUS createTemporaryMessage(
		MessageHolder* pmh, Message* pMessage);

private:
	QSTATUS createDefaultFolders();

public:
	Account* pThis_;
	WSTRING wstrName_;
	WSTRING wstrPath_;
	WSTRING wstrType_[Account::HOST_SIZE];
	Profile* pProfile_;
	Account::SubAccountList listSubAccount_;
	SubAccount* pCurrentSubAccount_;
	Account::FolderList listFolder_;
	MessageStore* pMessageStore_;
	MessageCache* pMessageCache_;
	ProtocolDriver* pProtocolDriver_;
	AccountHandlerList listAccountHandler_;
};

QSTATUS qm::AccountImpl::loadFolders()
{
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrPath(
		concat(wstrPath_, L"\\", Extensions::FOLDERS));
	if (!wstrPath.get())
		return QSTATUS_OUTOFMEMORY;
	
	W2T(wstrPath.get(), ptszPath);
	if (::GetFileAttributes(ptszPath) != 0xffffffff) {
		FileInputStream stream(wstrPath.get(), &status);
		CHECK_QSTATUS();
		BufferedInputStream bufferedStream(&stream, false, &status);
		CHECK_QSTATUS();
		XMLReader reader(&status);
		CHECK_QSTATUS();
		FolderContentHandler handler(pThis_, &listFolder_, &status);
		CHECK_QSTATUS();
		reader.setContentHandler(&handler);
		InputSource source(&bufferedStream, &status);
		CHECK_QSTATUS();
		status = reader.parse(&source);
		CHECK_QSTATUS();
	}
	else {
		status = createDefaultFolders();
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::AccountImpl::saveFolders() const
{
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrTempPath(
		concat(wstrPath_, L"\\_", Extensions::FOLDERS));
	if (!wstrTempPath.get())
		return QSTATUS_OUTOFMEMORY;
	
	string_ptr<WSTRING> wstrPath(
		concat(wstrPath_, L"\\", Extensions::FOLDERS));
	if (!wstrPath.get())
		return QSTATUS_OUTOFMEMORY;
	
	struct Move
	{
		Move(const WCHAR* pwszPath, const WCHAR* pwszTempPath, QSTATUS* pstatus) :
			tstrPath_(0),
			tstrTempPath_(0),
			bSuccess_(false)
		{
			*pstatus = QSTATUS_OUTOFMEMORY;
			
			string_ptr<TSTRING> tstrPath(wcs2tcs(pwszPath));
			if (!tstrPath.get())
				return;
			string_ptr<TSTRING> tstrTempPath(wcs2tcs(pwszTempPath));
			if (!tstrTempPath.get())
				return;
			tstrPath_ = tstrPath.release();
			tstrTempPath_ = tstrTempPath.release();
			
			*pstatus = QSTATUS_SUCCESS;
		}
		~Move()
		{
			if (bSuccess_) {
				::DeleteFile(tstrPath_);
				::MoveFile(tstrTempPath_, tstrPath_);
			}
			else {
				::DeleteFile(tstrTempPath_);
			}
			freeTString(tstrPath_);
			freeTString(tstrTempPath_);
		}
		void success() { bSuccess_ = true; }
		TSTRING tstrPath_;
		TSTRING tstrTempPath_;
		bool bSuccess_;
	} move(wstrPath.get(), wstrTempPath.get(), &status);
	CHECK_QSTATUS();
	
	FileOutputStream os(wstrTempPath.get(), &status);
	CHECK_QSTATUS();
	OutputStreamWriter writer(&os, false, L"utf-8", &status);
	CHECK_QSTATUS();
	BufferedWriter bufferedWriter(&writer, false, &status);
	CHECK_QSTATUS();
	
	FolderWriter folderWriter(&bufferedWriter, &status);
	CHECK_QSTATUS();
	status = folderWriter.write(listFolder_);
	CHECK_QSTATUS();
	
	status = bufferedWriter.close();
	CHECK_QSTATUS();
	
	move.success();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::AccountImpl::loadSubAccounts()
{
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrPath(
		concat(wstrPath_, L"\\", Extensions::ACCOUNT));
	if (!wstrPath.get())
		return QSTATUS_OUTOFMEMORY;
	
	std::auto_ptr<XMLProfile> pAccountProfile;
	status = newQsObject(wstrPath.get(), &pAccountProfile);
	CHECK_QSTATUS();
	status = pAccountProfile->load();
	CHECK_QSTATUS();
	
	std::auto_ptr<SubAccount> pDefaultSubAccount;
	status = newQsObject(pThis_, pAccountProfile.get(), L"", &pDefaultSubAccount);
	CHECK_QSTATUS();
	pProfile_ = pAccountProfile.release();
	
	STLWrapper<Account::SubAccountList> wrapper(listSubAccount_);
	status = wrapper.push_back(pDefaultSubAccount.get());
	CHECK_QSTATUS();
	pDefaultSubAccount.release();
	
	string_ptr<WSTRING> wstrFind(
		concat(wstrPath_, L"\\*", Extensions::ACCOUNT));
	if (!wstrFind.get())
		return QSTATUS_OUTOFMEMORY;
	W2T(wstrFind.get(), ptszFind);
	
	WIN32_FIND_DATA fd;
	AutoFindHandle hFind(::FindFirstFile(ptszFind, &fd));
	if (hFind.get()) {
		do {
			T2W(fd.cFileName, pwszFileName);
			if (wcscmp(pwszFileName, Extensions::ACCOUNT) == 0)
				continue;
			
			const WCHAR* p = wcschr(pwszFileName, L'.');
			assert(p);
			string_ptr<WSTRING> wstrName(
				allocWString(pwszFileName, p - pwszFileName));
			if (!wstrName.get())
				return QSTATUS_OUTOFMEMORY;
			
			string_ptr<WSTRING> wstrPath(
				concat(wstrPath_, L"\\", pwszFileName));
			if (!wstrPath.get())
				return QSTATUS_OUTOFMEMORY;
			
			std::auto_ptr<XMLProfile> pProfile;
			status = newQsObject(wstrPath.get(), &pProfile);
			CHECK_QSTATUS();
			status = pProfile->load();
			CHECK_QSTATUS();
			
			std::auto_ptr<SubAccount> pSubAccount;
			status = newQsObject(pThis_, pProfile.get(),
				wstrName.get(), &pSubAccount);
			CHECK_QSTATUS();
			pProfile.release();
			
			status = wrapper.push_back(pSubAccount.get());
			CHECK_QSTATUS();
			pSubAccount.release();
		} while (::FindNextFile(hFind.get(), &fd));
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::AccountImpl::saveSubAccounts() const
{
	DECLARE_QSTATUS();
	
	Account::SubAccountList::const_iterator it = listSubAccount_.begin();
	while (it != listSubAccount_.end()) {
		status = (*it)->save();
		CHECK_QSTATUS();
		++it;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::AccountImpl::fireFolderListChanged(
	const FolderListChangedEvent& event)
{
	DECLARE_QSTATUS();
	
	AccountHandlerList::const_iterator it = listAccountHandler_.begin();
	while (it != listAccountHandler_.end()) {
		status = (*it++)->folderListChanged(event);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::AccountImpl::createTemporaryMessage(
	MessageHolder* pmh, Message* pMessage)
{
	DECLARE_QSTATUS();
	
	StringBuffer<WSTRING> buf(&status);
	CHECK_QSTATUS();
	
	Time time;
	status = pmh->getDate(&time);
	CHECK_QSTATUS();
	string_ptr<WSTRING> wstrDate;
	status = time.format(L"Date: %W, %D %M1 %Y4 %h:%m:%s %z\n",
		Time::FORMAT_ORIGINAL, &wstrDate);
	CHECK_QSTATUS();
	status = buf.append(wstrDate.get());
	CHECK_QSTATUS();
	
	status = buf.append(L"Subject: ");
	CHECK_QSTATUS();
	string_ptr<WSTRING> wstrSubject;
	status = pmh->getSubject(&wstrSubject);
	CHECK_QSTATUS();
	status = buf.append(wstrSubject.get());
	CHECK_QSTATUS();
	status = buf.append(L"\n");
	CHECK_QSTATUS();
	
	MessageCreator creator;
	status = creator.createHeader(pMessage,
		buf.getCharArray(), buf.getLength());
	CHECK_QSTATUS();
	pMessage->setFlag(Message::FLAG_TEMPORARY);
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::AccountImpl::createDefaultFolders()
{
	DECLARE_QSTATUS();
	
	Folder** ppFolder = 0;
	size_t nCount = 0;
	status = pProtocolDriver_->createDefaultFolders(&ppFolder, &nCount);
	CHECK_QSTATUS();
	malloc_ptr<Folder*> p(ppFolder);
	
	STLWrapper<Account::FolderList> wrapper(listFolder_);
	for (size_t n = 0; n < nCount; ++n) {
		status = wrapper.push_back(*(ppFolder + n));
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * Account
 *
 */

qm::Account::Account(const WCHAR* pwszPath, QSTATUS* pstatus) :
	pImpl_(0)
{
	assert(pwszPath);
	assert(*pwszPath);
	assert(pwszPath[wcslen(pwszPath) - 1] != L'\\');
	assert(pstatus);
	
	*pstatus = QSTATUS_SUCCESS;
	
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrPath(allocWString(pwszPath));
	if (!wstrPath.get()) {
		*pstatus = QSTATUS_OUTOFMEMORY;
		return;
	}
	
	const WCHAR* pName = wcsrchr(pwszPath, L'\\');
	assert(pName);
	string_ptr<WSTRING> wstrName(allocWString(pName + 1));
	if (!wstrName.get()) {
		*pstatus = QSTATUS_OUTOFMEMORY;
		return;
	}
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	pImpl_->pThis_ = this;
	pImpl_->wstrName_ = wstrName.release();
	pImpl_->wstrPath_ = wstrPath.release();
	pImpl_->wstrType_[HOST_SEND] = 0;
	pImpl_->wstrType_[HOST_RECEIVE] = 0;
	pImpl_->pProfile_ = 0;
	pImpl_->pCurrentSubAccount_ = 0;
	pImpl_->pMessageStore_ = 0;
	pImpl_->pMessageCache_ = 0;
	pImpl_->pProtocolDriver_ = 0;
	
	status = pImpl_->loadSubAccounts();
	CHECK_QSTATUS_SET(pstatus);
	assert(!pImpl_->listSubAccount_.empty());
	
	int nBlockSize = 0;
	status = pImpl_->pProfile_->getInt(L"Global",
		L"BlockSize", -1, &nBlockSize);
	CHECK_QSTATUS_SET(pstatus);
	if (nBlockSize != -1)
		nBlockSize *= 1024*1024;
	
	int nCacheBlockSize = 0;
	status = pImpl_->pProfile_->getInt(L"Global",
		L"CacheBlockSize", -1, &nCacheBlockSize);
	CHECK_QSTATUS_SET(pstatus);
	if (nCacheBlockSize == 0)
		nCacheBlockSize = -1;
	else if (nCacheBlockSize != -1)
		nCacheBlockSize *= 1024*1024;
	
	if (nBlockSize == 0) {
		std::auto_ptr<MultiMessageStore> pMessageStore;
		status = newQsObject(pwszPath, nCacheBlockSize, &pMessageStore);
		CHECK_QSTATUS_SET(pstatus);
		pImpl_->pMessageStore_ = pMessageStore.release();
	}
	else {
		std::auto_ptr<SingleMessageStore> pMessageStore;
		status = newQsObject(pwszPath, nBlockSize,
			nCacheBlockSize, &pMessageStore);
		CHECK_QSTATUS_SET(pstatus);
		pImpl_->pMessageStore_ = pMessageStore.release();
	}
	
	status = newQsObject(pImpl_->pMessageStore_, &pImpl_->pMessageCache_);
	CHECK_QSTATUS_SET(pstatus);
	
	status = pImpl_->pProfile_->getString(L"Send", L"Type", L"smtp",
		&pImpl_->wstrType_[HOST_SEND]);
	CHECK_QSTATUS_SET(pstatus);
	status = pImpl_->pProfile_->getString(L"Receive", L"Type", L"pop3",
		&pImpl_->wstrType_[HOST_RECEIVE]);
	CHECK_QSTATUS_SET(pstatus);
	
	std::auto_ptr<ProtocolDriver> pProtocolDriver;
	status = ProtocolFactory::getDriver(
		this, pImpl_->wstrType_[HOST_RECEIVE], &pProtocolDriver);
	CHECK_QSTATUS_SET(pstatus);
	pImpl_->pProtocolDriver_ = pProtocolDriver.release();
	
	status = pImpl_->loadFolders();
	CHECK_QSTATUS_SET(pstatus);
	
	string_ptr<WSTRING> wstrSubAccount;
	status = pImpl_->pProfile_->getString(L"Global",
		L"SubAccount", L"", &wstrSubAccount);
	CHECK_QSTATUS_SET(pstatus);
	pImpl_->pCurrentSubAccount_ = getSubAccount(wstrSubAccount.get());
	if (!pImpl_->pCurrentSubAccount_)
		pImpl_->pCurrentSubAccount_ = pImpl_->listSubAccount_.front();
}

qm::Account::~Account()
{
	if (pImpl_) {
		WSTRING* pwstrs[] = {
			&pImpl_->wstrName_,
			&pImpl_->wstrPath_,
			&pImpl_->wstrType_[HOST_SEND],
			&pImpl_->wstrType_[HOST_RECEIVE],
		};
		for (int n = 0; n < countof(pwstrs); ++n)
			freeWString(*pwstrs[n]);
		std::for_each(pImpl_->listSubAccount_.begin(),
			pImpl_->listSubAccount_.end(), deleter<SubAccount>());
		std::for_each(pImpl_->listFolder_.begin(),
			pImpl_->listFolder_.end(), deleter<Folder>());
		delete pImpl_->pMessageCache_;
		delete pImpl_->pMessageStore_;
		delete pImpl_->pProtocolDriver_;
		delete pImpl_;
		pImpl_ = 0;
	}
}

const WCHAR* qm::Account::getName() const
{
	return pImpl_->wstrName_;
}

const WCHAR* qm::Account::getPath() const
{
	return pImpl_->wstrPath_;
}

const WCHAR* qm::Account::getType(Host host) const
{
	return pImpl_->wstrType_[host];
}

bool qm::Account::isSupport(Support support) const
{
	return pImpl_->pProtocolDriver_->isSupport(support);
}

QSTATUS qm::Account::getProperty(const WCHAR* pwszSection,
	const WCHAR* pwszKey, int nDefault, int* pnValue) const
{
	assert(pwszSection);
	assert(pwszKey);
	assert(pnValue);
	assert(!pImpl_->listSubAccount_.empty());
	
	return pImpl_->listSubAccount_.front()->getProperty(
		pwszSection, pwszKey, nDefault, pnValue);
}

QSTATUS qm::Account::getProperty(const WCHAR* pwszSection,
	const WCHAR* pwszName, const WCHAR* pwszDefault, WSTRING* pwstrValue) const
{
	assert(pwszSection);
	assert(pwszName);
	assert(pwstrValue);
	assert(!pImpl_->listSubAccount_.empty());
	
	return pImpl_->listSubAccount_.front()->getProperty(
		pwszSection, pwszName, pwszDefault, pwstrValue);
}

SubAccount* qm::Account::getSubAccount(const WCHAR* pwszName) const
{
	SubAccountList::const_iterator it = std::find_if(
		pImpl_->listSubAccount_.begin(), pImpl_->listSubAccount_.end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				string_equal<WCHAR>(),
				std::mem_fun(&SubAccount::getName),
				std::identity<const WCHAR*>()),
			pwszName));
	return it != pImpl_->listSubAccount_.end() ? *it : 0;
}

SubAccount* qm::Account::getSubAccountByIdentity(const WCHAR* pwszIdentity) const
{
	SubAccountList::const_iterator it = std::find_if(
		pImpl_->listSubAccount_.begin(), pImpl_->listSubAccount_.end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				string_equal<WCHAR>(),
				std::mem_fun(&SubAccount::getIdentity),
				std::identity<const WCHAR*>()),
			pwszIdentity));
	return it != pImpl_->listSubAccount_.end() ? *it : 0;
}

const Account::SubAccountList& qm::Account::getSubAccounts() const
{
	return pImpl_->listSubAccount_;
}

QSTATUS qm::Account::addSubAccount(SubAccount* pSubAccount)
{
	assert(!getSubAccount(pSubAccount->getName()));
	return STLWrapper<SubAccountList>(
		pImpl_->listSubAccount_).push_back(pSubAccount);
}

QSTATUS qm::Account::removeSubAccount(SubAccount* pSubAccount)
{
	// TODO
	return QSTATUS_SUCCESS;
}

QSTATUS qm::Account::renameSubAccount(
	SubAccount* pSubAccount, const WCHAR* pwszName)
{
	// TODO
	return QSTATUS_SUCCESS;
}

SubAccount* qm::Account::getCurrentSubAccount() const
{
	return pImpl_->pCurrentSubAccount_;
}

void qm::Account::setCurrentSubAccount(SubAccount* pSubAccount)
{
	pImpl_->pCurrentSubAccount_ = pSubAccount;
}

QSTATUS qm::Account::getFolder(const WCHAR* pwszName, Folder** ppFolder) const
{
	assert(pwszName);
	
	DECLARE_QSTATUS();
	
	const FolderList& l = pImpl_->listFolder_;
	FolderList::const_iterator it = l.begin();
	while (it != l.end()) {
		string_ptr<WSTRING> wstrName;
		status = (*it)->getFullName(&wstrName);
		CHECK_QSTATUS();
		if (wcscmp(wstrName.get(), pwszName) == 0)
			break;
		++it;
	}
	*ppFolder = it != l.end() ? *it : 0;
	
	return QSTATUS_SUCCESS;
}

Folder* qm::Account::getFolder(Folder* pParent, const WCHAR* pwszName) const
{
	const FolderList& l = pImpl_->listFolder_;
//	FolderList::const_iterator it = l.begin();
//	while (it != l.end()) {
//		if ((*it)->getParentFolder() == pParent &&
//			wcscmp((*it)->getName(), pwszName) == 0)
//			break;
//		++it;
//	}
	FolderList::const_iterator it = std::find_if(
		l.begin(), l.end(),
		unary_compose_f_gx_hy(
			binary_and_t<bool>(),
			std::bind2nd(
				binary_compose_f_gx_hy(
					std::equal_to<Folder*>(),
					std::mem_fun(&Folder::getParentFolder),
					std::identity<Folder*>()),
				pParent),
			std::bind2nd(
				binary_compose_f_gx_hy(
					string_equal<WCHAR>(),
					std::mem_fun(&Folder::getName),
					std::identity<const WCHAR*>()),
				pwszName)));
	return it != l.end() ? *it : 0;
}

Folder* qm::Account::getFolderById(unsigned int nId) const
{
	const FolderList& l = pImpl_->listFolder_;
	FolderList::const_iterator it = std::find_if(
		l.begin(), l.end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				std::equal_to<unsigned int>(),
				std::mem_fun(&Folder::getId),
				std::identity<unsigned int>()),
			nId));
	return it != l.end() ? *it : 0;
}

Folder* qm::Account::getFolderByFlag(unsigned int nFlag) const
{
	assert((nFlag & ~Folder::FLAG_BOX_MASK) == 0);
	
	const FolderList& l = pImpl_->listFolder_;
	FolderList::const_iterator it = std::find_if(
		l.begin(), l.end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				contains<unsigned int>(),
				std::mem_fun(&Folder::getFlags),
				std::identity<unsigned int>()),
			nFlag));
	return it != l.end() ? *it : 0;
}

const Account::FolderList& qm::Account::getFolders() const
{
	return pImpl_->listFolder_;
}

QSTATUS qm::Account::createNormalFolder(
	const WCHAR* pwszName, Folder* pParent, bool bRemote)
{
	DECLARE_QSTATUS();
	
	if (getFolder(pParent, pwszName))
		return QSTATUS_FAIL;
	else if (pParent && pParent->isFlag(Folder::FLAG_NOINFERIORS))
		return QSTATUS_FAIL;
	
	std::auto_ptr<NormalFolder> pFolder;
	if (bRemote) {
		NormalFolder* p = 0;
		status = pImpl_->pProtocolDriver_->createFolder(
			getCurrentSubAccount(), pwszName, pParent, &p);
		CHECK_QSTATUS();
		pFolder.reset(p);
	}
	else {
		NormalFolder::Init init;
		init.nId_ = generateFolderId();
		init.pwszName_ = pwszName;
		init.cSeparator_ = pParent ? pParent->getSeparator() : L'/';
		init.nFlags_ = Folder::FLAG_LOCAL;
		init.nCount_ = 0;
		init.nUnseenCount_ = 0;
		init.pParentFolder_ = pParent;
		init.pAccount_ = this;
		status = newQsObject(init, &pFolder);
		CHECK_QSTATUS();
	}
	
	status = STLWrapper<FolderList>(
		pImpl_->listFolder_).push_back(pFolder.get());
	CHECK_QSTATUS();
	NormalFolder* p = pFolder.release();
	
	FolderListChangedEvent event(this, FolderListChangedEvent::TYPE_ADD, p);
	status = pImpl_->fireFolderListChanged(event);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::Account::createQueryFolder(const WCHAR* pwszName,
	Folder* pParent, const WCHAR* pwszMacro)
{
	// TODO
	return QSTATUS_SUCCESS;
}

QSTATUS qm::Account::removeFolder(Folder* pFolder)
{
	// TODO
	return QSTATUS_SUCCESS;
}

QSTATUS qm::Account::renameFolder(Folder* pFolder, const WCHAR* pwszName)
{
	// TODO
	return QSTATUS_SUCCESS;
}

QSTATUS qm::Account::showFolder(Folder* pFolder, bool bShow)
{
	DECLARE_QSTATUS();
	
	if (pFolder->isFlag(Folder::FLAG_HIDE) == bShow) {
		pFolder->setFlags(bShow ? 0 : Folder::FLAG_HIDE, Folder::FLAG_HIDE);
		
		FolderListChangedEvent::Type type = bShow ?
			FolderListChangedEvent::TYPE_SHOW : FolderListChangedEvent::TYPE_HIDE;
		FolderListChangedEvent event(this, type, pFolder);
		status = pImpl_->fireFolderListChanged(event);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::Account::updateFolders()
{
	DECLARE_QSTATUS();
	
	std::pair<Folder*, bool>* pFolder = 0;
	size_t nCount = 0;
	status = pImpl_->pProtocolDriver_->getRemoteFolders(
		getCurrentSubAccount(), &pFolder, &nCount);
	CHECK_QSTATUS();
	struct Deleter
	{
		Deleter(std::pair<Folder*, bool>* pFolder, size_t nCount) :
			pFolder_(pFolder),
			nCount_(nCount),
			bReleased_(false)
		{
		}
		~Deleter()
		{
			if (!bReleased_) {
				for (size_t n = 0; n < nCount_; ++n) {
					if (pFolder_[n].second)
						delete pFolder_[n].first;
				}
			}
			free(pFolder_);
		}
		void release() { bReleased_ = true; }
		std::pair<Folder*, bool>* pFolder_;
		size_t nCount_;
		bool bReleased_;
	} deleter(pFolder, nCount);
	
	size_t nNewCount = 0;
	for (size_t n = 0; n < nCount; ++n)
		nNewCount += (*(pFolder + n)).second ? 1 : 0;
	status = STLWrapper<FolderList>(pImpl_->listFolder_).reserve(
		pImpl_->listFolder_.size() + nNewCount);
	CHECK_QSTATUS();
	
	for (n = 0; n < nCount; ++n) {
		const std::pair<Folder*, bool>& f = *(pFolder + n);
		if (f.second)
			pImpl_->listFolder_.push_back(f.first);
	}
	deleter.release();
	
	FolderList listDelete;
	struct Deleter2
	{
		Deleter2(const FolderList& l) : l_(l) {}
		~Deleter2() { std::for_each(l_.begin(), l_.end(), deleter<Folder>()); }
		const FolderList& l_;
	} deleter2(listDelete);
	
	FolderList::iterator it = pImpl_->listFolder_.begin();
	while (it != pImpl_->listFolder_.end()) {
		std::pair<Folder*, bool>* p = std::find_if(
			&pFolder[0], &pFolder[nCount],
			std::bind2nd(
				binary_compose_f_gx_hy(
					std::equal_to<Folder*>(),
					std::select1st<std::pair<Folder*, bool> >(),
					std::identity<Folder*>()),
				*it));
		if (p != &pFolder[nCount]) {
			++it;
		}
		else if ((*it)->getType() == Folder::TYPE_NORMAL &&
			!((*it)->getFlags() & Folder::FLAG_LOCAL)) {
			status = (*it)->deletePermanent();
			CHECK_QSTATUS();
			status = STLWrapper<FolderList>(listDelete).push_back(*it);
			CHECK_QSTATUS();
			it = pImpl_->listFolder_.erase(it);
		}
	}
	
	FolderListChangedEvent event(this, FolderListChangedEvent::TYPE_ALL, 0);
	status = pImpl_->fireFolderListChanged(event);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::Account::setOffline(bool bOffline)
{
	return pImpl_->pProtocolDriver_->setOffline(bOffline);
}

QSTATUS qm::Account::compact()
{
	DECLARE_QSTATUS();
	
	// TODO
	// Complete compaction
	
	Account::FolderList::iterator it = pImpl_->listFolder_.begin();
	while (it != pImpl_->listFolder_.end()) {
		Folder* pFolder = *it;
		if (pFolder->getType() == Folder::TYPE_NORMAL) {
			Lock<Folder> lock(*pFolder);
			status = pFolder->loadMessageHolders();
			CHECK_QSTATUS();
			unsigned int nCount = pFolder->getCount();
			for (unsigned n = 0; n < nCount; ++n) {
				MessageHolder* pmh = pFolder->getMessage(n);
				MessageHolder::MessageBoxKey boxKey = pmh->getMessageBoxKey();
				MessageCacheKey cacheKey = 0;
				status = pImpl_->pMessageStore_->compact(boxKey.nOffset_,
					boxKey.nLength_, pmh->getMessageCacheKey(), 0,
					&boxKey.nOffset_, &cacheKey);
				CHECK_QSTATUS();
				pmh->setKeys(cacheKey, boxKey);
			}
		}
		++it;
	}
	status = pImpl_->pMessageStore_->freeUnused();
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::Account::save() const
{
	DECLARE_QSTATUS();
	
	status = pImpl_->pProfile_->setString(L"Global", L"SubAccount",
		pImpl_->pCurrentSubAccount_->getName());
	CHECK_QSTATUS();
	
	status = pImpl_->pMessageStore_->flush();
	CHECK_QSTATUS();
	
	FolderList::const_iterator it = pImpl_->listFolder_.begin();
	while (it != pImpl_->listFolder_.end()) {
		status = (*it)->saveMessageHolders();
		CHECK_QSTATUS();
		++it;
	}
	
	status = pImpl_->saveFolders();
	CHECK_QSTATUS();
	
	status = pImpl_->saveSubAccounts();
	CHECK_QSTATUS();
	
	status = pImpl_->pProtocolDriver_->save();
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::Account::importMessage(NormalFolder* pFolder,
	const CHAR* pszMessage, unsigned int nFlags)
{
	assert(pFolder);
	assert(pszMessage);
	
	DECLARE_QSTATUS();
	
	size_t nHeaderLen = static_cast<size_t>(-1);
	const CHAR* p = strstr(pszMessage, "\r\n\r\n");
	if (p)
		nHeaderLen = p - pszMessage + 4;
	
	Message msgHeader(pszMessage, nHeaderLen,
		Message::FLAG_HEADERONLY, &status);
	CHECK_QSTATUS();
	
	unsigned int nMessageFlags = 0;
	NumberParser flags(NumberParser::FLAG_HEX, &status);
	CHECK_QSTATUS();
	Part::Field field;
	status = msgHeader.getField(L"X-QMAIL-Flags", &flags, &field);
	CHECK_QSTATUS();
	if (field == Part::FIELD_EXIST) {
		switch (nFlags) {
		case Account::IMPORTFLAG_NORMALFLAGS:
			nMessageFlags = flags.getValue() & MessageHolder::FLAG_USER_MASK;
			break;
		case Account::IMPORTFLAG_IGNOREFLAGS:
			break;
		case Account::IMPORTFLAG_QMAIL20FLAGS:
			// TODO
			break;
		}
	}
	if (field != Part::FIELD_NOTEXIST) {
		status = msgHeader.removeField(L"X-QMAIL-Flags");
		CHECK_QSTATUS();
	}
	
	return appendMessage(pFolder, pszMessage, msgHeader,
		nMessageFlags, static_cast<size_t>(-1));
}

QSTATUS qm::Account::addAccountHandler(AccountHandler* pHandler)
{
	assert(std::find(pImpl_->listAccountHandler_.begin(),
		pImpl_->listAccountHandler_.end(), pHandler) == pImpl_->listAccountHandler_.end());
	return STLWrapper<AccountImpl::AccountHandlerList>(
		pImpl_->listAccountHandler_).push_back(pHandler);
}

QSTATUS qm::Account::removeAccountHandler(AccountHandler* pHandler)
{
	AccountImpl::AccountHandlerList& l = pImpl_->listAccountHandler_;
	AccountImpl::AccountHandlerList::iterator it =
		std::remove(l.begin(), l.end(), pHandler);
	assert(it != l.end());
	l.erase(it, l.end());
	return QSTATUS_SUCCESS;
}

QSTATUS qm::Account::remove()
{
	// TODO
	return QSTATUS_SUCCESS;
}

QSTATUS qm::Account::rename(const WCHAR* pwszName)
{
	// TODO
	return QSTATUS_SUCCESS;
}

QSTATUS qm::Account::getData(MessageCacheKey key,
	MessageCacheItem item, WSTRING* pwstrData) const
{
	return pImpl_->pMessageCache_->getData(key, item, pwstrData);
}

QSTATUS qm::Account::getMessage(MessageHolder* pmh,
	unsigned int nFlags, Message* pMessage) const
{
	assert(pmh);
	assert(pMessage);
	assert((nFlags & GETMESSAGEFLAG_METHOD_MASK) != 0);
	
#ifndef NDEBUG
	Message::Flag flag = pMessage->getFlag();
	switch (nFlags & GETMESSAGEFLAG_METHOD_MASK) {
	case GETMESSAGEFLAG_ALL:
		assert(flag != Message::FLAG_NONE);
		break;
	case GETMESSAGEFLAG_HEADER:
		assert(flag != Message::FLAG_NONE &&
			flag != Message::FLAG_HEADERONLY &&
			flag != Message::FLAG_TEXTONLY &&
			flag != Message::FLAG_HTMLONLY);
		break;
	case GETMESSAGEFLAG_TEXT:
		assert(flag != Message::FLAG_NONE &&
			flag != Message::FLAG_TEXTONLY &&
			flag != Message::FLAG_HTMLONLY);
		break;
	case GETMESSAGEFLAG_HTML:
		assert(flag != Message::FLAG_NONE &&
			flag != Message::FLAG_HTMLONLY);
		break;
	case GETMESSAGEFLAG_POSSIBLE:
		break;
	default:
		assert(false);
		break;
	}
#endif
	
	DECLARE_QSTATUS();
	
	bool bLoadFromStore = false;
	Message::Flag msgFlag = Message::FLAG_NONE;
	unsigned int nPartialMask = pmh->getFlags() & MessageHolder::FLAG_PARTIAL_MASK;
	switch (nFlags & GETMESSAGEFLAG_METHOD_MASK) {
	case GETMESSAGEFLAG_ALL:
		bLoadFromStore = nPartialMask == 0;
		msgFlag = Message::FLAG_NONE;
		break;
	case GETMESSAGEFLAG_HEADER:
		bLoadFromStore = nPartialMask == 0 ||
			nPartialMask == MessageHolder::FLAG_HTMLONLY ||
			nPartialMask == MessageHolder::FLAG_TEXTONLY ||
			nPartialMask == MessageHolder::FLAG_HEADERONLY;
		msgFlag = Message::FLAG_HEADERONLY;
		break;
	case GETMESSAGEFLAG_TEXT:
		bLoadFromStore = nPartialMask == 0 ||
			nPartialMask == MessageHolder::FLAG_HTMLONLY ||
			nPartialMask == MessageHolder::FLAG_TEXTONLY;
		msgFlag = nPartialMask == 0 ? Message::FLAG_NONE :
			nPartialMask == MessageHolder::FLAG_HTMLONLY ?
			Message::FLAG_HTMLONLY : Message::FLAG_TEXTONLY;
		break;
	case GETMESSAGEFLAG_HTML:
		bLoadFromStore = nPartialMask == 0 ||
			nPartialMask == MessageHolder::FLAG_HTMLONLY;
		msgFlag = nPartialMask == 0 ? Message::FLAG_NONE : Message::FLAG_HTMLONLY;
		break;
	case GETMESSAGEFLAG_POSSIBLE:
		bLoadFromStore = true;
		msgFlag = nPartialMask == 0 ? Message::FLAG_NONE :
			nPartialMask == MessageHolder::FLAG_INDEXONLY ? Message::FLAG_TEMPORARY :
			nPartialMask == MessageHolder::FLAG_HEADERONLY ? Message::FLAG_HEADERONLY :
			nPartialMask == MessageHolder::FLAG_TEXTONLY ? Message::FLAG_TEXTONLY :
			/*nPartialMask == MessageHolder::FLAG_HTMLONLY ?*/ Message::FLAG_HTMLONLY/* : 0*/;
		break;
	default:
		assert(false);
		break;
	}
	if (pmh->getFolder()->isFlag(Folder::FLAG_LOCAL) ||
		pmh->isFlag(MessageHolder::FLAG_LOCAL))
		bLoadFromStore = true;
	
	bool bGet = false;
	bool bMadeSeen = false;
	if (!bLoadFromStore) {
		status = pImpl_->pProtocolDriver_->getMessage(getCurrentSubAccount(),
			pmh, nFlags, pMessage, &bGet, &bMadeSeen);
		CHECK_QSTATUS();
	}
	
	if (!bGet) {
		const MessageHolder::MessageBoxKey& key = pmh->getMessageBoxKey();
		if (key.nOffset_ != -1) {
			unsigned int nLength =
				(nFlags & GETMESSAGEFLAG_METHOD_MASK) == GETMESSAGEFLAG_HEADER ?
				key.nHeaderLength_ : key.nLength_;
			status = pImpl_->pMessageStore_->load(key.nOffset_, nLength, pMessage);
			CHECK_QSTATUS();
		}
		else {
			status = AccountImpl::createTemporaryMessage(pmh, pMessage);
			CHECK_QSTATUS();
		}
	}
	
	pMessage->setFlag(msgFlag);
	
	if ((nFlags & GETMESSAGEFLAG_MAKESEEN) &&
		!bMadeSeen &&
		!pmh->isFlag(MessageHolder::FLAG_SEEN)) {
		Folder::MessageHolderList l;
		status = STLWrapper<Folder::MessageHolderList>(l).push_back(pmh);
		CHECK_QSTATUS();
		status = setMessagesFlags(pmh->getFolder(), l,
			MessageHolder::FLAG_SEEN, MessageHolder::FLAG_SEEN);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::Account::setMessagesFlags(NormalFolder* pFolder,
	const Folder::MessageHolderList& l,
	unsigned int nFlags, unsigned int nMask) const
{
	assert(pFolder);
	assert(pFolder->isLocked());
	assert(std::find_if(l.begin(), l.end(),
		std::not1(
			std::bind2nd(
				binary_compose_f_gx_hy(
					std::equal_to<Folder*>(),
					std::mem_fun(&MessageHolder::getFolder),
					std::identity<Folder*>()),
				pFolder))) == l.end());
	
	DECLARE_QSTATUS();
	
	if (l.empty())
		return QSTATUS_SUCCESS;
	
	if (pFolder->isFlag(Folder::FLAG_LOCAL)) {
		Folder::MessageHolderList::const_iterator it = l.begin();
		while (it != l.end()) {
			(*it)->setFlags(nFlags, nMask);
			++it;
		}
	}
	else {
		status = pImpl_->pProtocolDriver_->setMessagesFlags(
			getCurrentSubAccount(), l.front()->getFolder(), l, nFlags, nMask);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::Account::appendMessage(NormalFolder* pFolder,
	const CHAR* pszMessage, const Message& msgHeader,
	unsigned int nFlags, unsigned int nSize)
{
	assert(pFolder);
	assert(pszMessage);
	assert((nFlags & ~MessageHolder::FLAG_USER_MASK) == 0);
	
	DECLARE_QSTATUS();
	
	if (pFolder->isFlag(Folder::FLAG_LOCAL)) {
		Lock<Folder> lock(*pFolder);
		MessageHolder* pmh = 0;
		status = storeMessage(pFolder, pszMessage,
			&msgHeader, static_cast<unsigned int>(-1),
			nFlags, nSize, false, &pmh);
		CHECK_QSTATUS();
	}
	else {
		status = pImpl_->pProtocolDriver_->appendMessage(
			getCurrentSubAccount(), pFolder, pszMessage, nFlags);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::Account::removeMessages(NormalFolder* pFolder,
	const Folder::MessageHolderList& l, bool bDirect) const
{
	assert(pFolder);
	assert(pFolder->isLocked());
	assert(std::find_if(l.begin(), l.end(),
		std::not1(
			std::bind2nd(
				binary_compose_f_gx_hy(
					std::equal_to<Folder*>(),
					std::mem_fun(&MessageHolder::getFolder),
					std::identity<Folder*>()),
				pFolder))) == l.end());
	
	DECLARE_QSTATUS();
	
	if (l.empty())
		return QSTATUS_SUCCESS;
	
	NormalFolder* pTrash = 0;
	if (!bDirect)
		pTrash = static_cast<NormalFolder*>(
			getFolderByFlag(Folder::FLAG_TRASHBOX));
	
	if (pTrash && pFolder != pTrash) {
		status = copyMessages(l, pFolder, pTrash, true);
		CHECK_QSTATUS();
	}
	else {
		if (pFolder->isFlag(Folder::FLAG_LOCAL)) {
			status = pFolder->deleteMessages(l);
			CHECK_QSTATUS();
		}
		else {
			status = pImpl_->pProtocolDriver_->removeMessages(
				getCurrentSubAccount(), pFolder, l);
			CHECK_QSTATUS();
		}
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::Account::copyMessages(const Folder::MessageHolderList& l,
	NormalFolder* pFolderFrom, NormalFolder* pFolderTo, bool bMove) const
{
	assert(pFolderFrom);
	assert(pFolderFrom->isLocked());
	assert(pFolderTo);
	assert(std::find_if(l.begin(), l.end(),
		std::not1(
			std::bind2nd(
				binary_compose_f_gx_hy(
					std::equal_to<NormalFolder*>(),
					std::mem_fun(&MessageHolder::getFolder),
					std::identity<NormalFolder*>()),
				pFolderFrom))) == l.end());
	assert(pFolderFrom->getAccount() == this);
	
	DECLARE_QSTATUS();
	
	if (l.empty() || pFolderFrom == pFolderTo)
		return QSTATUS_SUCCESS;
	
	// TODO
	// Take care of local messages in remote folder
	
	bool bLocalCopy = true;
	if (pFolderTo->getAccount() == this &&
		(bMove &&
		pFolderFrom->isFlag(Folder::FLAG_LOCAL) &&
		pFolderTo->isFlag(Folder::FLAG_LOCAL)) ||
		(!pFolderFrom->isFlag(Folder::FLAG_LOCAL) &&
		!pFolderTo->isFlag(Folder::FLAG_LOCAL)))
		bLocalCopy = false;
	
	if (bLocalCopy) {
		Folder::MessageHolderList::const_iterator it = l.begin();
		while (it != l.end()) {
			Message msg(&status);
			CHECK_QSTATUS();
			status = (*it)->getMessage(Account::GETMESSAGEFLAG_ALL, 0, &msg);
			CHECK_QSTATUS();
			status = pFolderTo->appendMessage(msg,
				(*it)->getFlags() & MessageHolder::FLAG_USER_MASK);
			CHECK_QSTATUS();
			if (bMove) {
				status = pFolderFrom->removeMessage(*it);
				CHECK_QSTATUS();
			}
			++it;
		}
	}
	else {
		if (pFolderFrom->isFlag(Folder::FLAG_LOCAL)) {
			assert(bMove);
			status = pFolderFrom->moveMessages(l, pFolderTo);
			CHECK_QSTATUS();
		}
		else {
			status = pImpl_->pProtocolDriver_->copyMessages(
				getCurrentSubAccount(), l, pFolderFrom, pFolderTo, bMove);
			CHECK_QSTATUS();
		}
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::Account::clearDeletedMessages(NormalFolder* pFolder) const
{
	assert(pFolder);
	return pImpl_->pProtocolDriver_->clearDeletedMessages(
		getCurrentSubAccount(), pFolder);
}

ProtocolDriver* qm::Account::getProtocolDriver() const
{
	return pImpl_->pProtocolDriver_;
}

QSTATUS qm::Account::storeMessage(NormalFolder* pFolder,
	const CHAR* pszMessage, const Message* pHeader, unsigned int nId,
	unsigned int nFlags, unsigned int nSize, bool bIndexOnly, MessageHolder** ppmh)
{
	assert(pFolder);
	assert(pFolder->isLocked());
	assert(pszMessage);
	assert((nFlags & ~(MessageHolder::FLAG_USER_MASK | MessageHolder::FLAG_PARTIAL_MASK | MessageHolder::FLAG_LOCAL)) == 0);
	assert(ppmh);
	
	DECLARE_QSTATUS();
	
	if (nSize == static_cast<size_t>(-1))
		nSize = strlen(pszMessage);
	
	Message header(&status);
	CHECK_QSTATUS();
	
	if (!pHeader) {
		const CHAR* p = strstr(pszMessage, "\r\n\r\n");
		status = header.create(pszMessage,
			p ? p - pszMessage + 4 : static_cast<size_t>(-1),
			Message::FLAG_HEADERONLY);
		pHeader = &header;
	}
	assert(pHeader);
	
	if (pHeader->isMultipart())
		nFlags |= MessageHolder::FLAG_MULTIPART;
	
	unsigned int nOffset = -1;
	unsigned int nLength = 0;
	unsigned int nHeaderLength = 0;
	MessageCacheKey key;
	status = pImpl_->pMessageStore_->save(pszMessage, *pHeader,
		pImpl_->pMessageCache_, bIndexOnly, &nOffset,
		&nLength, &nHeaderLength, &key);
	CHECK_QSTATUS();
	
	SubAccount* pSubAccount = getCurrentSubAccount();
	const WCHAR* pwszFields[] = {
		L"To",
		L"Cc"
	};
	MessageHolder::Flag flags[] = {
		MessageHolder::FLAG_TOME,
		MessageHolder::FLAG_CCME
	};
	for (int n = 0; n < countof(pwszFields); ++n) {
		AddressListParser address(0, &status);
		CHECK_QSTATUS();
		Part::Field field;
		status = pHeader->getField(pwszFields[n], &address, &field);
		CHECK_QSTATUS();
		if (field == Part::FIELD_EXIST) {
			if (pSubAccount->isMyAddress(address))
				nFlags |= flags[n];
		}
	}
	
	const Time* pTime = 0;
	Time time;
	DateParser date(&status);
	CHECK_QSTATUS();
	Part::Field f;
	status = pHeader->getField(L"Date", &date, &f);
	CHECK_QSTATUS();
	if (f == Part::FIELD_EXIST) {
		pTime = &date.getTime();
	}
	else {
		time = Time::getCurrentTime();
		pTime = &time;
	}
	
	if (nId == static_cast<unsigned int>(-1)) {
		status = pFolder->generateId(&nId);
		CHECK_QSTATUS();
	}
	
	MessageHolder::Init init = {
		nId,
		nFlags,
		MessageHolder::getDate(*pTime),
		MessageHolder::getTime(*pTime),
		nSize,
		key,
		nOffset,
		nLength,
		nHeaderLength
	};
	
	std::auto_ptr<MessageHolder> pmh;
	status = newQsObject(pFolder, init, &pmh);
	CHECK_QSTATUS();
	
	status = pFolder->appendMessage(pmh.get());
	CHECK_QSTATUS();
	*ppmh = pmh.release();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::Account::unstoreMessage(MessageHolder* pmh)
{
	assert(pmh);
	
	DECLARE_QSTATUS();
	
	const MessageHolder::MessageBoxKey& key = pmh->getMessageBoxKey();
	if (key.nOffset_ != -1) {
		status = pImpl_->pMessageStore_->free(key.nOffset_,
			key.nLength_, pmh->getMessageCacheKey());
		CHECK_QSTATUS();
	}
	
	pImpl_->pMessageCache_->removeData(pmh->getMessageCacheKey());
	
	NormalFolder* pFolder = pmh->getFolder();
	status = pFolder->removeMessage(pmh);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::Account::cloneMessage(MessageHolder* pmh,
	NormalFolder* pFolderTo, MessageHolder** ppmh)
{
	assert(pmh);
	assert(pFolderTo);
	assert(pFolderTo->isLocked());
	assert(ppmh);
	
	DECLARE_QSTATUS();
	
	const MessageHolder::MessageBoxKey& key = pmh->getMessageBoxKey();
	if (key.nOffset_ == -1) {
		MessageHolder::Init init;
		pmh->getInit(&init);
		status = pFolderTo->generateId(&init.nId_);
		CHECK_QSTATUS();
		std::auto_ptr<MessageHolder> pmhNew;
		status = newQsObject(pFolderTo, init, &pmhNew);
		CHECK_QSTATUS();
		
		status = pFolderTo->appendMessage(pmhNew.get());
		CHECK_QSTATUS();
		*ppmh = pmhNew.release();
	}
	else {
		Message msg(&status);
		CHECK_QSTATUS();
		status = pmh->getMessage(GETMESSAGEFLAG_POSSIBLE, 0, &msg);
		CHECK_QSTATUS();
		string_ptr<STRING> strContent;
		status = msg.getContent(&strContent);
		CHECK_QSTATUS();
		unsigned int nId = 0;
		status = pFolderTo->generateId(&nId);
		CHECK_QSTATUS();
		status = storeMessage(pFolderTo, strContent.get(), &msg,
			nId, pmh->getFlags(), pmh->getSize(), false, ppmh);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::Account::updateMessage(MessageHolder* pmh, const CHAR* pszMessage)
{
	assert(pmh);
	assert(pszMessage);
	
	DECLARE_QSTATUS();
	
	Message header(pszMessage, static_cast<size_t>(-1),
		Message::FLAG_NONE, &status);
	CHECK_QSTATUS();
	
	MessageHolder::MessageBoxKey boxKey = pmh->getMessageBoxKey();
	unsigned int nOldOffset = boxKey.nOffset_;
	unsigned int nOldLength = boxKey.nLength_;
	
	MessageCacheKey key;
	status = pImpl_->pMessageStore_->save(pszMessage, header,
		pImpl_->pMessageCache_, false, &boxKey.nOffset_,
		&boxKey.nLength_, &boxKey.nHeaderLength_, &key);
	CHECK_QSTATUS();
	
	MessageCacheKey keyOld = pmh->getMessageCacheKey();
	if (key != keyOld)
		pImpl_->pMessageCache_->removeData(keyOld);
	pmh->setKeys(key, boxKey);
	
	if (nOldOffset != -1) {
		status = pImpl_->pMessageStore_->free(nOldOffset, nOldLength, keyOld);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

unsigned int qm::Account::generateFolderId() const
{
	unsigned int nId = 0;
	
	FolderList::const_iterator it = pImpl_->listFolder_.begin();
	while (it != pImpl_->listFolder_.end()) {
		nId = QSMAX(nId, (*it)->getId());
		++it;
	}
	
	return nId + 1;
}

QSTATUS qm::Account::openLogger(Host host, Logger** ppLogger) const
{
	assert(ppLogger);
	
	DECLARE_QSTATUS();
	
	*ppLogger = 0;
	
	Time time(Time::getCurrentTime());
	WCHAR wszName[128];
	swprintf(wszName, L"%s-%04d%02d%02d%02d%02d%02d%03d-%u.log",
		getType(host), time.wYear, time.wMonth, time.wDay,
		time.wHour, time.wMinute, time.wSecond, time.wMilliseconds,
		::GetCurrentThreadId());
	
	string_ptr<WSTRING> wstrDir(concat(getPath(), L"\\log"));
	if (!wstrDir.get())
		return QSTATUS_OUTOFMEMORY;
	W2T(wstrDir.get(), ptszDir);
	if (::GetFileAttributes(ptszDir) == 0xffffffff) {
		if (!::CreateDirectory(ptszDir, 0))
			return QSTATUS_FAIL;
	}
	
	string_ptr<WSTRING> wstrPath(concat(wstrDir.get(), L"\\", wszName));
	if (!wstrPath.get())
		return QSTATUS_OUTOFMEMORY;
	
	std::auto_ptr<FileOutputStream> pStream;
	status = newQsObject(wstrPath.get(), &pStream);
	CHECK_QSTATUS();
	
	std::auto_ptr<Logger> pLogger;
	status = newQsObject(pStream.get(), true, Logger::LEVEL_DEBUG, &pLogger);
	CHECK_QSTATUS();
	pStream.release();
	
	*ppLogger = pLogger.release();
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * AccountHandler
 *
 */

qm::AccountHandler::~AccountHandler()
{
}


/****************************************************************************
 *
 * FolderListChangedEvent
 *
 */

qm::FolderListChangedEvent::FolderListChangedEvent(
	Account* pAccount, Type type, Folder* pFolder) :
	pAccount_(pAccount),
	type_(type),
	pFolder_(pFolder)
{
}

qm::FolderListChangedEvent::~FolderListChangedEvent()
{
}

Account* qm::FolderListChangedEvent::getAccount() const
{
	return pAccount_;
}

FolderListChangedEvent::Type qm::FolderListChangedEvent::getType() const
{
	return type_;
}

Folder* qm::FolderListChangedEvent::getFolder() const
{
	return pFolder_;
}


/****************************************************************************
 *
 * FolderContentHandler
 *
 */

qm::FolderContentHandler::FolderContentHandler(Account* pAccount,
	Account::FolderList* pList, QSTATUS* pstatus) :
	DefaultHandler(pstatus),
	pAccount_(pAccount),
	pList_(pList),
	state_(STATE_ROOT),
	bNormal_(true),
	nItem_(0),
	nId_(0),
	nParentId_(0),
	nFlags_(0),
	nCount_(0),
	nUnseenCount_(0),
	cSeparator_(L'\0'),
	wstrName_(0),
	nValidity_(0),
	nDownloadCount_(0),
	wstrMacro_(0),
	pBuffer_(0)
{
	DECLARE_QSTATUS();
	
	status = newQsObject(&pBuffer_);
	CHECK_QSTATUS_SET(pstatus);
}

qm::FolderContentHandler::~FolderContentHandler()
{
	freeWString(wstrMacro_);
	freeWString(wstrName_);
	delete pBuffer_;
}

QSTATUS qm::FolderContentHandler::startElement(
	const WCHAR* pwszNamespaceURI, const WCHAR* pwszLocalName,
	const WCHAR* pwszQName, const Attributes& attributes)
{
	DECLARE_QSTATUS();
	
	struct {
		const WCHAR* pwszName_;
		unsigned int nAcceptStates_;
		State state_;
	} items[] = {
		{ L"id",			STATE_NORMALFOLDER | STATE_QUERYFOLDER,	STATE_ID			},
		{ L"parent",		STATE_NORMALFOLDER | STATE_QUERYFOLDER,	STATE_PARENT		},
		{ L"flags",			STATE_NORMALFOLDER | STATE_QUERYFOLDER,	STATE_FLAGS			},
		{ L"count",			STATE_NORMALFOLDER | STATE_QUERYFOLDER,	STATE_COUNT			},
		{ L"unseenCount",	STATE_NORMALFOLDER | STATE_QUERYFOLDER,	STATE_UNSEENCOUNT	},
		{ L"separator",		STATE_NORMALFOLDER | STATE_QUERYFOLDER,	STATE_SEPARATOR		},
		{ L"name",			STATE_NORMALFOLDER | STATE_QUERYFOLDER,	STATE_NAME			},
		{ L"validity",		STATE_NORMALFOLDER,						STATE_VALIDITY		},
		{ L"downloadCount",	STATE_NORMALFOLDER,						STATE_DOWNLOADCOUNT	},
		{ L"macro",			STATE_QUERYFOLDER,						STATE_MACRO			}
	};
	
	if (wcscmp(pwszLocalName, L"folders") == 0) {
		if (state_ != STATE_ROOT)
			return QSTATUS_FAIL;
		
		if (attributes.getLength() != 0)
			return QSTATUS_FAIL;
		
		state_ = STATE_FOLDERS;
	}
	else if (wcscmp(pwszLocalName, L"normalFolder") == 0) {
		if (state_ != STATE_FOLDERS)
			return QSTATUS_FAIL;
		
		if (attributes.getLength() != 0)
			return QSTATUS_FAIL;
		
		bNormal_ = true;
		nItem_ = 0;
		state_ = STATE_NORMALFOLDER;
	}
	else if (wcscmp(pwszLocalName, L"queryFolder") == 0) {
		if (state_ != STATE_FOLDERS)
			return QSTATUS_FAIL;
		
		if (attributes.getLength() != 0)
			return QSTATUS_FAIL;
		
		bNormal_ = false;
		nItem_ = 0;
		state_ = STATE_QUERYFOLDER;
	}
	else {
		for (int n = 0; n < countof(items); ++n) {
			if (wcscmp(pwszLocalName, items[n].pwszName_) == 0) {
				status = processItemStartElement(items[n].nAcceptStates_,
					items[n].state_, attributes);
				CHECK_QSTATUS();
				break;
			}
		}
		if (n == countof(items))
			return QSTATUS_FAIL;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::FolderContentHandler::endElement(const WCHAR* pwszNamespaceURI,
	const WCHAR* pwszLocalName, const WCHAR* pwszQName)
{
	DECLARE_QSTATUS();
	
	if (wcscmp(pwszLocalName, L"folders") == 0) {
		assert(state_ == STATE_FOLDERS);
		state_ = STATE_ROOT;
	}
	else if (wcscmp(pwszLocalName, L"normalFolder") == 0) {
		assert(state_ == STATE_NORMALFOLDER);
		
		if (nItem_ != 9)
			return QSTATUS_FAIL;
		
		NormalFolder::Init init;
		init.nId_ = nId_;
		init.pwszName_ = wstrName_;
		init.cSeparator_ = cSeparator_;
		init.nFlags_ = nFlags_;
		init.nCount_ = nCount_;
		init.nUnseenCount_ = nUnseenCount_;
		init.pParentFolder_ = 0;
		init.pAccount_ = pAccount_;
		if (nParentId_ != 0)
			init.pParentFolder_ = getFolder(nParentId_);
		init.nValidity_ = nValidity_;
		init.nDownloadCount_ = nDownloadCount_;
		
		std::auto_ptr<NormalFolder> pFolder;
		status = newQsObject(init, &pFolder);
		CHECK_QSTATUS();
		status = STLWrapper<Account::FolderList>(*pList_).push_back(pFolder.get());
		CHECK_QSTATUS();
		pFolder.release();
		
		freeWString(wstrName_);
		wstrName_ = 0;
		
		state_ = STATE_FOLDERS;
	}
	else if (wcscmp(pwszLocalName, L"queryFolder") == 0) {
		assert(state_ == STATE_QUERYFOLDER);
		
		if (nItem_ != 8)
			return QSTATUS_FAIL;
		
		QueryFolder::Init init;
		init.nId_ = nId_;
		init.pwszName_ = wstrName_;
		init.cSeparator_ = cSeparator_;
		init.nFlags_ = nFlags_;
		init.nCount_ = nCount_;
		init.nUnseenCount_ = nUnseenCount_;
		init.pParentFolder_ = 0;
		init.pAccount_ = pAccount_;
		if (nParentId_ != 0)
			init.pParentFolder_ = getFolder(nParentId_);
		init.pwszMacro_ = wstrMacro_;
		
		std::auto_ptr<QueryFolder> pFolder;
		status = newQsObject(init, &pFolder);
		CHECK_QSTATUS();
		status = STLWrapper<Account::FolderList>(*pList_).push_back(pFolder.get());
		CHECK_QSTATUS();
		pFolder.release();
		
		freeWString(wstrName_);
		wstrName_ = 0;
		freeWString(wstrMacro_);
		wstrMacro_ = 0;
		
		state_ = STATE_FOLDERS;
	}
	else if (wcscmp(pwszLocalName, L"id") == 0) {
		assert(state_ == STATE_ID);
		
		status = getNumber(&nId_);
		CHECK_QSTATUS();
		
		state_ = bNormal_ ? STATE_NORMALFOLDER : STATE_QUERYFOLDER;
	}
	else if (wcscmp(pwszLocalName, L"parent") == 0) {
		assert(state_ == STATE_PARENT);
		
		status = getNumber(&nParentId_);
		CHECK_QSTATUS();
		
		state_ = bNormal_ ? STATE_NORMALFOLDER : STATE_QUERYFOLDER;
	}
	else if (wcscmp(pwszLocalName, L"flags") == 0) {
		assert(state_ == STATE_FLAGS);
		
		status = getNumber(&nFlags_);
		CHECK_QSTATUS();
		
		state_ = bNormal_ ? STATE_NORMALFOLDER : STATE_QUERYFOLDER;
	}
	else if (wcscmp(pwszLocalName, L"count") == 0) {
		assert(state_ == STATE_COUNT);
		
		status = getNumber(&nCount_);
		CHECK_QSTATUS();
		
		state_ = bNormal_ ? STATE_NORMALFOLDER : STATE_QUERYFOLDER;
	}
	else if (wcscmp(pwszLocalName, L"unseenCount") == 0) {
		assert(state_ == STATE_UNSEENCOUNT);
		
		status = getNumber(&nUnseenCount_);
		CHECK_QSTATUS();
		
		state_ = bNormal_ ? STATE_NORMALFOLDER : STATE_QUERYFOLDER;
	}
	else if (wcscmp(pwszLocalName, L"separator") == 0) {
		assert(state_ == STATE_SEPARATOR);
		
		if (pBuffer_->getLength() > 1)
			return QSTATUS_FAIL;
		
		cSeparator_ = *pBuffer_->getCharArray();
		pBuffer_->remove();
		
		state_ = bNormal_ ? STATE_NORMALFOLDER : STATE_QUERYFOLDER;
	}
	else if (wcscmp(pwszLocalName, L"name") == 0) {
		assert(state_ == STATE_NAME);
		
		if (pBuffer_->getLength() == 0)
			return QSTATUS_FAIL;
		
		assert(!wstrName_);
		wstrName_ = pBuffer_->getString();
		
		state_ = bNormal_ ? STATE_NORMALFOLDER : STATE_QUERYFOLDER;
	}
	else if (wcscmp(pwszLocalName, L"validity") == 0) {
		assert(state_ == STATE_VALIDITY);
		assert(bNormal_);
		
		status = getNumber(&nValidity_);
		CHECK_QSTATUS();
		
		state_ = STATE_NORMALFOLDER;
	}
	else if (wcscmp(pwszLocalName, L"downloadCount") == 0) {
		assert(state_ == STATE_DOWNLOADCOUNT);
		assert(bNormal_);
		
		status = getNumber(&nDownloadCount_);
		CHECK_QSTATUS();
		
		state_ = STATE_NORMALFOLDER;
	}
	else if (wcscmp(pwszLocalName, L"macro") == 0) {
		assert(state_ == STATE_MACRO);
		assert(!bNormal_);
		
		assert(!wstrMacro_);
		wstrMacro_ = pBuffer_->getString();
		
		state_ = STATE_QUERYFOLDER;
	}
	else {
		return QSTATUS_FAIL;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::FolderContentHandler::characters(
	const WCHAR* pwsz, size_t nStart, size_t nLength)
{
	DECLARE_QSTATUS();
	
	if (state_ == STATE_ID ||
		state_ == STATE_PARENT ||
		state_ == STATE_FLAGS ||
		state_ == STATE_COUNT ||
		state_ == STATE_UNSEENCOUNT ||
		state_ == STATE_SEPARATOR ||
		state_ == STATE_NAME ||
		state_ == STATE_VALIDITY ||
		state_ == STATE_DOWNLOADCOUNT ||
		state_ == STATE_MACRO) {
		status = pBuffer_->append(pwsz + nStart, nLength);
		CHECK_QSTATUS();
	}
	else {
		const WCHAR* p = pwsz + nStart;
		for (size_t n = 0; n < nLength; ++n, ++p) {
			if (*p != L' ' && *p != L'\t' && *p != '\n')
				return QSTATUS_FAIL;
		}
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::FolderContentHandler::processItemStartElement(
	unsigned int nAcceptStates, State state, const qs::Attributes& attributes)
{
	if ((state_ & nAcceptStates) == 0)
		return QSTATUS_FAIL;
	
	if (attributes.getLength() != 0)
		return QSTATUS_FAIL;
	
	++nItem_;
	state_ = state;
	
	return QSTATUS_SUCCESS;
}

Folder* qm::FolderContentHandler::getFolder(unsigned int nId) const
{
	Account::FolderList::const_iterator it = std::find_if(
		pList_->begin(), pList_->end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				std::equal_to<unsigned int>(),
				std::mem_fun(&Folder::getId),
				std::identity<unsigned int>()),
			nId));
	return it != pList_->end() ? *it : 0;
}

QSTATUS qm::FolderContentHandler::getNumber(unsigned int* pn)
{
	DECLARE_QSTATUS();
	
	if (pBuffer_->getLength() == 0)
		return QSTATUS_FAIL;
	
	WCHAR* pEnd = 0;
	*pn = wcstol(pBuffer_->getCharArray(), &pEnd, 10);
	pBuffer_->remove();
	if (*pEnd)
		return QSTATUS_FAIL;
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * FolderWriter
 *
 */

qm::FolderWriter::FolderWriter(Writer* pWriter, QSTATUS* pstatus) :
	handler_(pWriter, pstatus)
{
}

qm::FolderWriter::~FolderWriter()
{
}

QSTATUS qm::FolderWriter::write(const Account::FolderList& l)
{
	DECLARE_QSTATUS();
	
	DefaultAttributes attrs;
	
	status = handler_.startDocument();
	CHECK_QSTATUS();
	status = handler_.startElement(0, 0, L"folders", attrs);
	CHECK_QSTATUS();
	
	Account::FolderList::const_iterator it = l.begin();
	while (it != l.end()) {
		Folder* pFolder = *it;
		const WCHAR* pwszQName = 0;
		switch (pFolder->getType()) {
		case Folder::TYPE_NORMAL:
			pwszQName = L"normalFolder";
			break;
		case Folder::TYPE_QUERY:
			pwszQName = L"queryFolder";
			break;
		default:
			assert(false);
			break;
		}
		status = handler_.startElement(0, 0, pwszQName, attrs);
		CHECK_QSTATUS();
		
		status = writeNumber(L"id", pFolder->getId());
		CHECK_QSTATUS();
		Folder* pParent = pFolder->getParentFolder();
		status = writeNumber(L"parent", pParent ? pParent->getId() : 0);
		CHECK_QSTATUS();
		status = writeNumber(L"flags", pFolder->getFlags());
		CHECK_QSTATUS();
		status = writeNumber(L"count", pFolder->getCount());
		CHECK_QSTATUS();
		status = writeNumber(L"unseenCount", pFolder->getUnseenCount());
		CHECK_QSTATUS();
		WCHAR cSeparator = pFolder->getSeparator();
		status = writeString(L"separator", &cSeparator,
			cSeparator == L'\0' ? 0 : 1);
		CHECK_QSTATUS();
		status = writeString(L"name", pFolder->getName(), -1);
		CHECK_QSTATUS();
		switch (pFolder->getType()) {
		case Folder::TYPE_NORMAL:
			status = writeNumber(L"validity",
				static_cast<NormalFolder*>(pFolder)->getValidity());
			CHECK_QSTATUS();
			status = writeNumber(L"downloadCount",
				static_cast<NormalFolder*>(pFolder)->getDownloadCount());
			break;
		case Folder::TYPE_QUERY:
			status = writeString(L"macro",
				static_cast<QueryFolder*>(pFolder)->getMacro(), -1);
			CHECK_QSTATUS();
			break;
		default:
			assert(false);
			break;
		}
		
		status = handler_.endElement(0, 0, pwszQName);
		CHECK_QSTATUS();
		
		++it;
	}
	
	status = handler_.endElement(0, 0, L"folders");
	CHECK_QSTATUS();
	status = handler_.endDocument();
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::FolderWriter::writeString(
	const WCHAR* pwszQName, const WCHAR* pwsz, size_t nLen)
{
	DECLARE_QSTATUS();
	
	if (nLen == -1)
		nLen = wcslen(pwsz);
	
	status = handler_.startElement(0, 0, pwszQName, DefaultAttributes());
	CHECK_QSTATUS();
	status = handler_.characters(pwsz, 0, nLen);
	CHECK_QSTATUS();
	status = handler_.endElement(0, 0, pwszQName);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::FolderWriter::writeNumber(const WCHAR* pwszQName, unsigned int n)
{
	WCHAR wsz[32];
	swprintf(wsz, L"%u", n);
	return writeString(pwszQName, wsz, -1);
}
