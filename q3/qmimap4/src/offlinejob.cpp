/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmmessage.h>
#include <qmmessageholder.h>

#include <qsassert.h>
#include <qsconv.h>
#include <qserror.h>
#include <qsfile.h>
#include <qsnew.h>
#include <qsstl.h>
#include <qsstream.h>

#include <algorithm>

#include "imap4.h"
#include "offlinejob.h"
#include "util.h"

#pragma warning(disable:4786)

using namespace qmimap4;
using namespace qm;
using namespace qs;

#define READ(p, size) \
	status = pStream->read(reinterpret_cast<unsigned char*>(p), size, &nRead); \
	CHECK_QSTATUS_SET(pstatus); \
	if (nRead != size) { \
		*pstatus = QSTATUS_FAIL; \
		return; \
	} \

#define READ_STRING(type, name) \
	size_t n##name##Len = 0; \
	READ(&n##name##Len, sizeof(size_t)); \
	string_ptr<type> name; \
	if (n##name##Len != 0) { \
		name.reset(StringTraits<type>::allocString(n##name##Len + 1)); \
		if (!name.get()) { \
			*pstatus = QSTATUS_OUTOFMEMORY; \
			return; \
		} \
		READ(name.get(), n##name##Len*sizeof(StringTraits<type>::char_type)); \
		*(name.get() + n##name##Len) = 0; \
	} \

#define WRITE(p, size) \
	status = pStream->write(reinterpret_cast<const unsigned char*>(p), size); \
	CHECK_QSTATUS() \

#define WRITE_STRING(type, str) \
	size_t n##str##Len = 0; \
	if (str) \
		n##str##Len = CharTraits<StringTraits<type>::char_type>::getLength(str); \
	WRITE(&n##str##Len, sizeof(size_t)); \
	if (str) \
		WRITE(str, n##str##Len*sizeof(StringTraits<type>::char_type)) \


/****************************************************************************
 *
 * OfflineJobManager
 *
 */

const WCHAR* qmimap4::OfflineJobManager::FILENAME = L".offlinejob";

qmimap4::OfflineJobManager::OfflineJobManager(
	const WCHAR* pwszPath, QSTATUS* pstatus)
{
	assert(pwszPath);
	assert(pstatus);
	
	DECLARE_QSTATUS();
	
	status = load(pwszPath);
	CHECK_QSTATUS_SET(pstatus);
}

qmimap4::OfflineJobManager::~OfflineJobManager()
{
	std::for_each(listJob_.begin(), listJob_.end(), deleter<OfflineJob>());
}

QSTATUS qmimap4::OfflineJobManager::add(OfflineJob* pJob)
{
	DECLARE_QSTATUS();
	
	Lock<CriticalSection> lock(cs_);
	
	bool bMerged = false;
	if (!listJob_.empty()) {
		status = listJob_.back()->merge(pJob, &bMerged);
		CHECK_QSTATUS();
	}
	if (!bMerged) {
		status = STLWrapper<JobList>(listJob_).push_back(pJob);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::OfflineJobManager::apply(Account* pAccount,
	Imap4* pImap4, ReceiveSessionCallback* pCallback)
{
	assert(pImap4);
	
	DECLARE_QSTATUS();
	
	Lock<CriticalSection> lock(cs_);
	
	struct Deleter
	{
		typedef OfflineJobManager::JobList JobList;
		Deleter(JobList& l) : l_(l) {}
		~Deleter()
		{
			JobList::iterator it = std::find_if(l_.begin(), l_.end(),
				std::not1(std::bind2nd(std::equal_to<OfflineJob*>(), 0)));
			l_.erase(l_.begin(), it);
		}
		JobList& l_;
	} deleter(listJob_);
	
	// TODO
	// Sort listJob_ not to select folder each time
	
	status = pCallback->setRange(0, listJob_.size());
	CHECK_QSTATUS();
	
	Folder* pPrevFolder = 0;
	for (JobList::size_type n = 0; n < listJob_.size(); ++n) {
		status = pCallback->setPos(n + 1);
		CHECK_QSTATUS();
		
		OfflineJob*& pJob = listJob_[n];
		if (pJob->getFolder()) {
			Folder* pFolder = 0;
			status = pAccount->getFolder(pJob->getFolder(), &pFolder);
			CHECK_QSTATUS();
			if (pFolder && pFolder != pPrevFolder &&
				pFolder->getType() == Folder::TYPE_NORMAL) {
				string_ptr<WSTRING> wstrName;
				status = Util::getFolderName(pFolder, &wstrName);
				CHECK_QSTATUS();
				status = pImap4->select(wstrName.get());
				CHECK_QSTATUS();
				pPrevFolder = pFolder;
			}
		}
		status = pJob->apply(pAccount, pImap4);
		CHECK_QSTATUS();
		delete pJob;
		pJob = 0;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::OfflineJobManager::save(const WCHAR* pwszPath) const
{
	DECLARE_QSTATUS();
	
	Lock<CriticalSection> lock(cs_);
	
	string_ptr<WSTRING> wstrPath(concat(pwszPath, L"\\", FILENAME));
	if (!wstrPath.get())
		return QSTATUS_OUTOFMEMORY;
	
	TemporaryFileRenamer renamer(wstrPath.get(), &status);
	CHECK_QSTATUS();
	
	FileOutputStream stream(renamer.getPath(), &status);
	CHECK_QSTATUS();
	BufferedOutputStream bufferedStream(&stream, false, &status);
	CHECK_QSTATUS();
	
	OfflineJobFactory factory;
	
	JobList::const_iterator it = listJob_.begin();
	while (it != listJob_.end()) {
		status = factory.writeInstance(&bufferedStream, *it);
		CHECK_QSTATUS();
		++it;
	}
	
	status = bufferedStream.close();
	CHECK_QSTATUS();
	
	status = renamer.rename();
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::OfflineJobManager::copyJobs(Folder* pFolderFrom,
	Folder* pFolderTo, const UidList& listUid, bool bMove)
{
	assert(pFolderFrom);
	assert(pFolderTo);
	
	DECLARE_QSTATUS();
	
	Lock<CriticalSection> lock(cs_);
	
	string_ptr<WSTRING> wstrFolderFrom;
	status = Util::getFolderName(pFolderFrom, &wstrFolderFrom);
	CHECK_QSTATUS();
	
	UidList::const_iterator itU = listUid.begin();
	while (itU != listUid.end()) {
		OfflineJob* pJob = getCreateMessage(
			wstrFolderFrom.get(), *itU);
		if (pJob) {
			switch (pJob->getType()) {
			case OfflineJob::TYPE_APPEND:
				if (bMove) {
					// TODO
				}
				else {
					// TODO
				}
				break;
			case OfflineJob::TYPE_COPY:
				// TODO
				break;
			default:
				assert(false);
				break;
			}
		}
		++itU;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::OfflineJobManager::load(const WCHAR* pwszPath)
{
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrPath(concat(pwszPath, L"\\", FILENAME));
	if (!wstrPath.get())
		return QSTATUS_OUTOFMEMORY;
	
	W2T(wstrPath.get(), ptszPath);
	
	if (::GetFileAttributes(ptszPath) != 0xffffffff) {
		FileInputStream stream(wstrPath.get(), &status);
		CHECK_QSTATUS();
		BufferedInputStream bufferedStream(&stream, false, &status);
		CHECK_QSTATUS();
		
		OfflineJobFactory factory;
		
		while (true) {
			OfflineJob* p = 0;
			status = factory.getInstance(&bufferedStream, &p);
			CHECK_QSTATUS();
			if (!p)
				break;
			std::auto_ptr<OfflineJob> pJob(p);
			
			status = STLWrapper<JobList>(listJob_).push_back(pJob.get());
			CHECK_QSTATUS();
			pJob.release();
		}
	}
	
	return QSTATUS_SUCCESS;
}

OfflineJob* qmimap4::OfflineJobManager::getCreateMessage(
	const WCHAR* pwszFolder, unsigned long nId) const
{
	JobList::const_iterator itJ = listJob_.begin();
	while (itJ != listJob_.end()) {
		if ((*itJ)->isCreateMessage(pwszFolder, nId))
			return *itJ;
		++itJ;
	}
	return 0;
}


/****************************************************************************
 *
 * OfflineJob
 *
 */

qmimap4::OfflineJob::OfflineJob(const WCHAR* pwszFolder, QSTATUS* pstatus) :
	wstrFolder_(0)
{
	DECLARE_QSTATUS();
	
	if (pwszFolder) {
		wstrFolder_ = allocWString(pwszFolder);
		if (!wstrFolder_) {
			*pstatus = QSTATUS_OUTOFMEMORY;
			return;
		}
	}
}

qmimap4::OfflineJob::OfflineJob(InputStream* pStream, QSTATUS* pstatus) :
	wstrFolder_(0)
{
	DECLARE_QSTATUS();
	
	size_t nRead = 0;
	
	READ_STRING(WSTRING, wstrFolder);
	
	wstrFolder_ = wstrFolder.release();
}

qmimap4::OfflineJob::~OfflineJob()
{
	freeWString(wstrFolder_);
}

QSTATUS qmimap4::OfflineJob::write(qs::OutputStream* pStream) const
{
	DECLARE_QSTATUS();
	
	WRITE_STRING(WSTRING, wstrFolder_);
	
	return QSTATUS_SUCCESS;
}

const WCHAR* qmimap4::OfflineJob::getFolder() const
{
	return wstrFolder_;
}


/****************************************************************************
 *
 * AppendOfflineJob
 *
 */

qmimap4::AppendOfflineJob::AppendOfflineJob(
	const WCHAR* pwszFolder, unsigned int nId, QSTATUS* pstatus) :
	OfflineJob(static_cast<const WCHAR*>(0), pstatus),
	wstrFolder_(0),
	nId_(nId)
{
	wstrFolder_ = allocWString(pwszFolder);
	if (!wstrFolder_) {
		*pstatus = QSTATUS_OUTOFMEMORY;
		return;
	}
}

qmimap4::AppendOfflineJob::AppendOfflineJob(
	InputStream* pStream, qs::QSTATUS* pstatus) :
	OfflineJob(pStream, pstatus),
	nId_(0)
{
	DECLARE_QSTATUS();
	
	size_t nRead = 0;
	
	READ_STRING(WSTRING, wstrFolder);
	READ(&nId_, sizeof(nId_));
}

qmimap4::AppendOfflineJob::~AppendOfflineJob()
{
	freeWString(wstrFolder_);
}

OfflineJob::Type qmimap4::AppendOfflineJob::getType() const
{
	return TYPE_APPEND;
}

QSTATUS qmimap4::AppendOfflineJob::apply(Account* pAccount, Imap4* pImap4) const
{
	DECLARE_QSTATUS();
	
	Folder* pFolder = 0;
	status = pAccount->getFolder(wstrFolder_, &pFolder);
	CHECK_QSTATUS();
	if (pFolder && pFolder->getType() == Folder::TYPE_NORMAL) {
		NormalFolder* pNormalFolder = static_cast<NormalFolder*>(pFolder);
		Lock<Folder>lock(*pFolder);
		MessageHolder* pmh = 0;
		status = pNormalFolder->getMessageById(nId_, &pmh);
		CHECK_QSTATUS();
		if (pmh->isFlag(MessageHolder::FLAG_LOCAL)) {
			string_ptr<WSTRING> wstrName;
			status = Util::getFolderName(pNormalFolder, &wstrName);
			CHECK_QSTATUS();
			
			Message msg(&status);
			CHECK_QSTATUS();
			status = pmh->getMessage(Account::GETMESSAGEFLAG_ALL, 0, &msg);
			CHECK_QSTATUS();
			
			string_ptr<STRING> strContent;
			status = msg.getContent(&strContent);
			CHECK_QSTATUS();
			
			Flags flags(Util::getImap4FlagsFromMessageFlags(pmh->getFlags()), &status);
			CHECK_QSTATUS();
			status = pImap4->append(wstrName.get(), strContent.get(), flags);
			CHECK_QSTATUS();
			
			status = pAccount->unstoreMessage(pmh);
			CHECK_QSTATUS();
		}
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::AppendOfflineJob::write(OutputStream* pStream) const
{
	DECLARE_QSTATUS();
	
	status = OfflineJob::write(pStream);
	CHECK_QSTATUS();
	
	WRITE_STRING(WSTRING, wstrFolder_);
	WRITE(&nId_, sizeof(nId_));
	
	return QSTATUS_SUCCESS;
}

bool qmimap4::AppendOfflineJob::isCreateMessage(
	const WCHAR* pwszFolder, unsigned long nId)
{
	return wcscmp(pwszFolder, wstrFolder_) == 0 && nId == nId_;
}

QSTATUS qmimap4::AppendOfflineJob::merge(OfflineJob* pOfflineJob, bool* pbMerged)
{
	assert(pOfflineJob);
	assert(pbMerged);
	
	*pbMerged = false;
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * CopyOfflineJob
 *
 */

qmimap4::CopyOfflineJob::CopyOfflineJob(const WCHAR* pwszFolderFrom,
	const WCHAR* pwszFolderTo, const UidList& listUidFrom,
	const ItemList& listItemTo, bool bMove, QSTATUS* pstatus) :
	OfflineJob(pwszFolderFrom, pstatus),
	wstrFolderTo_(0),
	bMove_(bMove)
{
	assert(!listUidFrom.empty());
	assert(listUidFrom.size() == listItemTo.size());
	
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrFolderTo(allocWString(pwszFolderTo));
	if (!wstrFolderTo.get()) {
		*pstatus = QSTATUS_OUTOFMEMORY;
		return;
	}
	
	status = STLWrapper<UidList>(listUidFrom_).resize(listUidFrom.size());
	CHECK_QSTATUS_SET(pstatus);
	status = STLWrapper<ItemList>(listItemTo_).resize(listItemTo.size());
	
	wstrFolderTo_ = wstrFolderTo.release();
	std::copy(listUidFrom.begin(), listUidFrom.end(), listUidFrom_.begin());
	std::copy(listItemTo.begin(), listItemTo.end(), listItemTo_.begin());
}

qmimap4::CopyOfflineJob::CopyOfflineJob(InputStream* pStream, QSTATUS* pstatus) :
	OfflineJob(pStream, pstatus),
	wstrFolderTo_(0),
	bMove_(false)
{
	DECLARE_QSTATUS();
	
	size_t nRead = 0;
	
	READ_STRING(WSTRING, wstrFolderTo);
	size_t nSize = 0;
	READ(&nSize, sizeof(nSize));
	if (nSize == 0) {
		*pstatus = QSTATUS_FAIL;
		return;
	}
	status = STLWrapper<UidList>(listUidFrom_).resize(nSize);
	CHECK_QSTATUS_SET(pstatus);
	status = STLWrapper<ItemList>(listItemTo_).resize(nSize);
	CHECK_QSTATUS_SET(pstatus);
	READ(&listUidFrom_[0], nSize*sizeof(UidList::value_type));
	READ(&listItemTo_[0], nSize*sizeof(ItemList::value_type));
	READ(&bMove_, sizeof(bMove_));
	
	wstrFolderTo_ = wstrFolderTo.release();
}

qmimap4::CopyOfflineJob::~CopyOfflineJob()
{
	freeWString(wstrFolderTo_);
}

OfflineJob::Type qmimap4::CopyOfflineJob::getType() const
{
	return TYPE_COPY;
}

QSTATUS qmimap4::CopyOfflineJob::apply(Account* pAccount, Imap4* pImap4) const
{
	DECLARE_QSTATUS();
	
	assert(!listUidFrom_.empty());
	assert(listUidFrom_.size() == listItemTo_.size());
	
	Folder* pFolder = 0;
	status = pAccount->getFolder(wstrFolderTo_, &pFolder);
	CHECK_QSTATUS();
	if (pFolder && pFolder->getType() == Folder::TYPE_NORMAL) {
		NormalFolder* pNormalFolder = static_cast<NormalFolder*>(pFolder);
		
		string_ptr<WSTRING> wstrName;
		status = Util::getFolderName(pNormalFolder, &wstrName);
		CHECK_QSTATUS();
		
		// TODO
		// Apply flags
		
		MultipleRange range(&listUidFrom_[0], listUidFrom_.size(), true, &status);
		CHECK_QSTATUS();
		status = pImap4->copy(range, wstrName.get());
		CHECK_QSTATUS();
		if (bMove_) {
			Flags flags(Imap4::FLAG_DELETED, &status);
			CHECK_QSTATUS();
			status = pImap4->setFlags(range, flags, flags);
			CHECK_QSTATUS();
		}
		
		Lock<Folder> lock(*pNormalFolder);
		
		ItemList::const_iterator it = listItemTo_.begin();
		while (it != listItemTo_.end()) {
			MessageHolder* pmh = 0;
			status = pNormalFolder->getMessageById((*it).nId_, &pmh);
			CHECK_QSTATUS();
			if (pmh && pmh->isFlag(MessageHolder::FLAG_LOCAL)) {
				status = pAccount->unstoreMessage(pmh);
				CHECK_QSTATUS();
			}
			++it;
		}
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::CopyOfflineJob::write(OutputStream* pStream) const
{
	DECLARE_QSTATUS();
	
	status = OfflineJob::write(pStream);
	CHECK_QSTATUS();
	
	WRITE_STRING(WSTRING, wstrFolderTo_);
	size_t nSize = listUidFrom_.size();
	WRITE(&nSize, sizeof(nSize));
	WRITE(&listUidFrom_[0], listUidFrom_.size()*sizeof(UidList::value_type));
	WRITE(&listItemTo_[0], listItemTo_.size()*sizeof(ItemList::value_type));
	WRITE(&bMove_, sizeof(bMove_));
	
	return QSTATUS_SUCCESS;
}

bool qmimap4::CopyOfflineJob::isCreateMessage(
	const WCHAR* pwszFolder, unsigned long nId)
{
	return wcscmp(pwszFolder, wstrFolderTo_) == 0 &&
		std::find_if(listItemTo_.begin(), listItemTo_.end(),
			std::bind2nd(
				binary_compose_f_gx_hy(
					std::equal_to<unsigned long>(),
					mem_data_ref(&Item::nId_),
					std::identity<unsigned long>()),
				nId)) != listItemTo_.end();
}

QSTATUS qmimap4::CopyOfflineJob::merge(OfflineJob* pOfflineJob, bool* pbMerged)
{
	assert(pOfflineJob);
	assert(pbMerged);
	
	DECLARE_QSTATUS();
	
	if (pOfflineJob->getType() == TYPE_COPY &&
		wcscmp(getFolder(), pOfflineJob->getFolder()) == 0) {
		CopyOfflineJob* p = static_cast<CopyOfflineJob*>(pOfflineJob);
		if (wcscmp(wstrFolderTo_, p->wstrFolderTo_) == 0 &&
			bMove_ == p->bMove_) {
			UidList listUidFrom;
			status = STLWrapper<UidList>(listUidFrom).reserve(
				listUidFrom_.size() + p->listUidFrom_.size());
			CHECK_QSTATUS();
			ItemList listItemTo;
			status = STLWrapper<ItemList>(listItemTo).reserve(
				listItemTo_.size() + p->listItemTo_.size());
			CHECK_QSTATUS();
			
			UidList::const_iterator itUS = listUidFrom_.begin();
			UidList::const_iterator itUO = p->listUidFrom_.begin();
			ItemList::const_iterator itIS = listItemTo_.begin();
			ItemList::const_iterator itIO = p->listItemTo_.begin();
			while (true) {
				if (itUS != listUidFrom_.end()) {
					if (itUO != p->listUidFrom_.end()) {
						if (*itUS < *itUO) {
							listUidFrom.push_back(*itUS++);
							listItemTo.push_back(*itIS++);
						}
						else {
							listUidFrom.push_back(*itUO++);
							listItemTo.push_back(*itIO++);
						}
					}
					else {
						listUidFrom.push_back(*itUS++);
						listItemTo.push_back(*itIS++);
					}
				}
				else {
					if (itUO != p->listUidFrom_.end()) {
						listUidFrom.push_back(*itUO++);
						listItemTo.push_back(*itIO++);
					}
					else {
						break;
					}
				}
			}
			
			listUidFrom_.swap(listUidFrom);
			listItemTo_.swap(listItemTo);
			
			*pbMerged = true;
		}
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * SetFlagsOfflineJob
 *
 */

qmimap4::SetFlagsOfflineJob::SetFlagsOfflineJob(const WCHAR* pwszFolder,
	const UidList& listUid, unsigned int nFlags, unsigned int nMask,
	qs::QSTATUS* pstatus) :
	OfflineJob(pwszFolder, pstatus),
	nFlags_(nFlags),
	nMask_(nMask)
{
	assert(!listUid.empty());
	
	DECLARE_QSTATUS();
	
	status = STLWrapper<UidList>(listUid_).resize(listUid.size());
	CHECK_QSTATUS_SET(pstatus);
	std::copy(listUid.begin(), listUid.end(), listUid_.begin());
}

qmimap4::SetFlagsOfflineJob::SetFlagsOfflineJob(
	InputStream* pStream, QSTATUS* pstatus) :
	OfflineJob(pStream, pstatus),
	nFlags_(0),
	nMask_(0)
{
	DECLARE_QSTATUS();
	
	size_t nRead = 0;
	
	size_t nSize = 0;
	READ(&nSize, sizeof(nSize));
	if (nSize == 0) {
		*pstatus = QSTATUS_FAIL;
		return;
	}
	status = STLWrapper<UidList>(listUid_).resize(nSize);
	CHECK_QSTATUS_SET(pstatus);
	READ(&listUid_[0], nSize*sizeof(unsigned long));
	READ(&nFlags_, sizeof(nFlags_));
	READ(&nMask_, sizeof(nMask_));
}

qmimap4::SetFlagsOfflineJob::~SetFlagsOfflineJob()
{
}

OfflineJob::Type qmimap4::SetFlagsOfflineJob::getType() const
{
	return TYPE_SETFLAGS;
}

QSTATUS qmimap4::SetFlagsOfflineJob::apply(Account* pAccount, Imap4* pImap4) const
{
	assert(!listUid_.empty());
	
	DECLARE_QSTATUS();
	
	MultipleRange range(&listUid_[0], listUid_.size(), true, &status);
	CHECK_QSTATUS();
	Flags flags(Util::getImap4FlagsFromMessageFlags(nFlags_), &status);
	CHECK_QSTATUS();
	Flags mask(Util::getImap4FlagsFromMessageFlags(nMask_), &status);
	CHECK_QSTATUS();
	status = pImap4->setFlags(range, flags, mask);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::SetFlagsOfflineJob::write(OutputStream* pStream) const
{
	DECLARE_QSTATUS();
	
	status = OfflineJob::write(pStream);
	CHECK_QSTATUS();
	
	size_t nSize = listUid_.size();
	WRITE(&nSize, sizeof(nSize));
	WRITE(&listUid_[0], listUid_.size()*sizeof(unsigned long));
	WRITE(&nFlags_, sizeof(nFlags_));
	WRITE(&nMask_, sizeof(nMask_));
	
	return QSTATUS_SUCCESS;
}

bool qmimap4::SetFlagsOfflineJob::isCreateMessage(
	const WCHAR* pwszFolder, unsigned long nId)
{
	return false;
}

QSTATUS qmimap4::SetFlagsOfflineJob::merge(OfflineJob* pOfflineJob, bool* pbMerged)
{
	assert(pOfflineJob);
	assert(pbMerged);
	
	DECLARE_QSTATUS();
	
	if (pOfflineJob->getType() == TYPE_SETFLAGS &&
		wcscmp(getFolder(), pOfflineJob->getFolder()) == 0) {
		SetFlagsOfflineJob* p = static_cast<SetFlagsOfflineJob*>(pOfflineJob);
		if (nFlags_ == p->nFlags_ && nMask_ == p->nMask_) {
			UidList l;
			status = STLWrapper<UidList>(l).reserve(
				listUid_.size() + p->listUid_.size());
			CHECK_QSTATUS();
			std::merge(listUid_.begin(), listUid_.end(),
				p->listUid_.begin(), p->listUid_.end(),
				std::back_inserter(l));
			l.erase(std::unique(l.begin(), l.end()), l.end());
			listUid_.swap(l);
			
			*pbMerged = true;
		}
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * OfflineJobFactory
 *
 */

qmimap4::OfflineJobFactory::OfflineJobFactory()
{
}

qmimap4::OfflineJobFactory::~OfflineJobFactory()
{
}

QSTATUS qmimap4::OfflineJobFactory::getInstance(
	InputStream* pStream, OfflineJob** ppJob) const
{
	assert(pStream);
	assert(ppJob);
	
	DECLARE_QSTATUS();
	
	*ppJob = 0;
	
	size_t nRead = 0;
	OfflineJob::Type type;
	status = pStream->read(reinterpret_cast<unsigned char*>(&type),
		sizeof(type), &nRead);
	CHECK_QSTATUS();
	if (nRead == -1)
		return QSTATUS_SUCCESS;
	else if (nRead != sizeof(type))
		return QSTATUS_FAIL;
	
#define BEGIN_OFFLINEJOB() \
	switch (type) { \

#define DECLARE_OFFLINEJOB(type, classname) \
	case OfflineJob::type: \
		{ \
			classname* pJob = 0; \
			status = newQsObject(pStream, &pJob); \
			CHECK_QSTATUS(); \
			*ppJob = pJob; \
		} \
		break; \

#define END_OFFLINEJOB() \
	default: \
		return QSTATUS_FAIL; \
	} \
	
	BEGIN_OFFLINEJOB()
		DECLARE_OFFLINEJOB(TYPE_APPEND, AppendOfflineJob)
		DECLARE_OFFLINEJOB(TYPE_COPY, CopyOfflineJob)
		DECLARE_OFFLINEJOB(TYPE_SETFLAGS, SetFlagsOfflineJob)
	END_OFFLINEJOB()
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::OfflineJobFactory::writeInstance(
	qs::OutputStream* pStream, OfflineJob* pJob) const
{
	DECLARE_QSTATUS();
	
	OfflineJob::Type type = pJob->getType();
	status = pStream->write(reinterpret_cast<unsigned char*>(&type), sizeof(type));
	CHECK_QSTATUS();
	
	status = pJob->write(pStream);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}
