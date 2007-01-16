/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
 * All rights reserved.
 *
 */

#pragma warning(disable:4786)

#include <qmmessage.h>
#include <qmmessageholder.h>
#include <qmsecurity.h>

#include <qsassert.h>
#include <qsconv.h>
#include <qsfile.h>
#include <qsstl.h>
#include <qsstream.h>

#include <algorithm>

#include "imap4.h"
#include "offlinejob.h"
#include "util.h"

using namespace qmimap4;
using namespace qm;
using namespace qs;

#define READ(p, size) \
	do { \
		size_t nRead = pStream->read(reinterpret_cast<unsigned char*>(p), size); \
		if (nRead != size) \
			return std::auto_ptr<OfflineJob>(0); \
	} while (false)

#define READ_STRING(type, name) \
	do { \
		unsigned int nLen = 0; \
		READ(&nLen, sizeof(nLen)); \
		if (nLen != 0) { \
			name = StringTraits<type>::allocString(nLen + 1); \
			READ(name.get(), nLen*sizeof(StringTraits<type>::char_type)); \
			*(name.get() + nLen) = 0; \
		} \
	} while (false)

#define WRITE(p, size) \
	do { \
		if (pStream->write(reinterpret_cast<const unsigned char*>(p), size) == -1) \
			return false; \
	} while (false)

#define WRITE_STRING(type, str) \
	do { \
		unsigned int nLen = 0; \
		if (str) \
			nLen = static_cast<unsigned int>(CharTraits<StringTraits<type>::char_type>::getLength(str)); \
		WRITE(&nLen, sizeof(nLen)); \
		if (str) \
			WRITE(str, nLen*sizeof(StringTraits<type>::char_type)); \
	} while (false)


/****************************************************************************
 *
 * OfflineJobManager
 *
 */

const WCHAR* qmimap4::OfflineJobManager::FILENAME = L"offlinejob";

qmimap4::OfflineJobManager::OfflineJobManager(const WCHAR* pwszPath) :
	bModified_(false)
{
	assert(pwszPath);
	
	wstrPath_ = concat(pwszPath, L"\\", FILENAME);
	
	if (!load()) {
		// TODO
	}
}

qmimap4::OfflineJobManager::~OfflineJobManager()
{
	std::for_each(listJob_.begin(), listJob_.end(), deleter<OfflineJob>());
}

void qmimap4::OfflineJobManager::add(std::auto_ptr<OfflineJob> pJob)
{
	Lock<CriticalSection> lock(cs_);
	
	bModified_ = true;
	
	bool bMerged = false;
	if (!listJob_.empty())
		bMerged = listJob_.back()->merge(pJob.get());
	if (!bMerged) {
		listJob_.push_back(pJob.get());
		pJob.release();
	}
}

bool qmimap4::OfflineJobManager::apply(Account* pAccount,
									   Imap4* pImap4,
									   ReceiveSessionCallback* pCallback)
{
	assert(pImap4);
	
	Lock<CriticalSection> lock(cs_);
	
	bModified_ = true;
	
	{
		struct Deleter
		{
			typedef OfflineJobManager::JobList JobList;
			
			Deleter(JobList& l) :
				l_(l)
			{
			}
			
			~Deleter()
			{
				JobList::iterator it = std::find_if(l_.begin(), l_.end(),
					std::not1(std::bind2nd(std::equal_to<OfflineJob*>(), 0)));
				l_.erase(l_.begin(), it);
			}
			
			JobList& l_;
		} deleter(listJob_);
		
		pCallback->setRange(0, listJob_.size());
		
		Folder* pPrevFolder = 0;
		for (JobList::size_type n = 0; n < listJob_.size(); ++n) {
			pCallback->setPos(n + 1);
			
			OfflineJob*& pJob = listJob_[n];
			if (pJob->getFolder()) {
				Folder* pFolder = pAccount->getFolder(pJob->getFolder());
				if (pFolder && pFolder != pPrevFolder &&
					pFolder->getType() == Folder::TYPE_NORMAL) {
					wstring_ptr wstrName(Util::getFolderName(
						static_cast<NormalFolder*>(pFolder)));
					if (!pImap4->select(wstrName.get()))
						return false;
					pPrevFolder = pFolder;
				}
			}
			
			bool bClosed = false;
			if (!pJob->apply(pAccount, pImap4, &bClosed))
				return false;
			if (bClosed)
				pPrevFolder = 0;
			
			delete pJob;
			pJob = 0;
		}
	}
	
	if (!save()) {
		// TODO
	}
	
	return true;
}

bool qmimap4::OfflineJobManager::save() const
{
	Lock<CriticalSection> lock(cs_);
	
	if (!bModified_)
		return true;
	
	if (listJob_.empty()) {
		W2T(wstrPath_.get(), ptszPath);
		::DeleteFile(ptszPath);
	}
	else {
		TemporaryFileRenamer renamer(wstrPath_.get());
		
		FileOutputStream stream(renamer.getPath());
		if (!stream)
			return false;
		BufferedOutputStream bufferedStream(&stream, false);
		
		OfflineJobFactory factory;
		
		for (JobList::const_iterator it = listJob_.begin(); it != listJob_.end(); ++it) {
			if (!factory.writeInstance(&bufferedStream, *it))
				return false;
		}
		
		if (!bufferedStream.close())
			return false;
		
		if (!renamer.rename())
			return false;
	}
	
	bModified_ = false;
	
	return true;
}

bool qmimap4::OfflineJobManager::copyJobs(NormalFolder* pFolderFrom,
										  NormalFolder* pFolderTo,
										  const UidList& listUid,
										  bool bMove)
{
	assert(pFolderFrom);
	assert(pFolderTo);
	
	Lock<CriticalSection> lock(cs_);
	
	bModified_ = true;
	
	wstring_ptr wstrFolderFrom(Util::getFolderName(pFolderFrom));
	
	for (UidList::const_iterator itU = listUid.begin(); itU != listUid.end(); ++itU) {
		OfflineJob* pJob = getCreateMessage(wstrFolderFrom.get(), *itU);
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
	}
	
	return true;
}

bool qmimap4::OfflineJobManager::load()
{
	if (File::isFileExisting(wstrPath_.get())) {
		FileInputStream stream(wstrPath_.get());
		if (!stream)
			return false;
		BufferedInputStream bufferedStream(&stream, false);
		
		OfflineJobFactory factory;
		
		while (true) {
			std::auto_ptr<OfflineJob> pJob(factory.getInstance(&bufferedStream));
			// TODO
			if (!pJob.get())
				break;
			
			listJob_.push_back(pJob.get());
			pJob.release();
		}
	}
	
	bModified_ = false;
	
	return true;
}

OfflineJob* qmimap4::OfflineJobManager::getCreateMessage(const WCHAR* pwszFolder,
														 unsigned long nId) const
{
	for (JobList::const_iterator itJ = listJob_.begin(); itJ != listJob_.end(); ++itJ) {
		if ((*itJ)->isCreateMessage(pwszFolder, nId))
			return *itJ;
	}
	return 0;
}


/****************************************************************************
 *
 * OfflineJob
 *
 */

qmimap4::OfflineJob::OfflineJob(const WCHAR* pwszFolder)
{
	if (pwszFolder)
		wstrFolder_ = allocWString(pwszFolder);
}

qmimap4::OfflineJob::~OfflineJob()
{
}

bool qmimap4::OfflineJob::write(OutputStream* pStream) const
{
	WRITE_STRING(WSTRING, wstrFolder_.get());
	
	return true;
}

const WCHAR* qmimap4::OfflineJob::getFolder() const
{
	return wstrFolder_.get();
}


/****************************************************************************
 *
 * AppendOfflineJob
 *
 */

qmimap4::AppendOfflineJob::AppendOfflineJob(const WCHAR* pwszFolder,
											unsigned int nId) :
	OfflineJob(static_cast<const WCHAR*>(0)),
	nId_(nId)
{
	wstrFolder_ = allocWString(pwszFolder);
}

qmimap4::AppendOfflineJob::~AppendOfflineJob()
{
}

OfflineJob::Type qmimap4::AppendOfflineJob::getType() const
{
	return TYPE_APPEND;
}

bool qmimap4::AppendOfflineJob::apply(Account* pAccount,
									  Imap4* pImap4,
									  bool* pbClosed) const
{
	assert(pAccount);
	assert(pImap4);
	assert(pbClosed);
	
	Folder* pFolder = pAccount->getFolder(wstrFolder_.get());
	if (pFolder && pFolder->getType() == Folder::TYPE_NORMAL) {
		NormalFolder* pNormalFolder = static_cast<NormalFolder*>(pFolder);
		Lock<Account> lock(*pAccount);
		MessagePtrLock mpl(pNormalFolder->getMessageById(nId_));
		if (mpl && mpl->isFlag(MessageHolder::FLAG_LOCAL)) {
			wstring_ptr wstrName(Util::getFolderName(pNormalFolder));
			
			Message msg;
			if (!mpl->getMessage(Account::GETMESSAGEFLAG_ALL,
				0, SECURITYMODE_NONE, &msg))
				return false;
			
			xstring_size_ptr strContent(msg.getContent());
			if (!strContent.get())
				return false;
			
			wstring_ptr wstrLabel(mpl->getLabel());
			const WCHAR* pwszLabel = wstrLabel.get();
			std::auto_ptr<Flags> pFlags(Util::getImap4FlagsFromLabels(mpl->getFlags(),
				&pwszLabel, pwszLabel && *pwszLabel ? 1 : 0));
			if (!pFlags.get())
				return false;
			if (!pImap4->append(wstrName.get(), strContent.get(), strContent.size(), *pFlags))
				return false;
			
			if (!pAccount->unstoreMessages(MessageHolderList(1, mpl), 0))
				return false;
		}
	}
	
	return true;
}

bool qmimap4::AppendOfflineJob::write(OutputStream* pStream) const
{
	if (!OfflineJob::write(pStream))
		return false;
	
	WRITE_STRING(WSTRING, wstrFolder_.get());
	WRITE(&nId_, sizeof(nId_));
	
	return true;
}

bool qmimap4::AppendOfflineJob::isCreateMessage(const WCHAR* pwszFolder,
												unsigned long nId)
{
	return wcscmp(pwszFolder, wstrFolder_.get()) == 0 && nId == nId_;
}

bool qmimap4::AppendOfflineJob::merge(OfflineJob* pOfflineJob)
{
	assert(pOfflineJob);
	return false;
}

std::auto_ptr<OfflineJob> qmimap4::AppendOfflineJob::create(InputStream* pStream)
{
	wstring_ptr wstrSelectFolder;
	READ_STRING(WSTRING, wstrSelectFolder);
	if (wstrSelectFolder.get())
		return std::auto_ptr<OfflineJob>(0);
	
	wstring_ptr wstrFolder;
	READ_STRING(WSTRING, wstrFolder);
	if (!wstrFolder.get())
		return std::auto_ptr<OfflineJob>(0);
	unsigned int nId = 0;
	READ(&nId, sizeof(nId));
	
	return std::auto_ptr<OfflineJob>(new AppendOfflineJob(wstrFolder.get(), nId));
}


/****************************************************************************
 *
 * CopyOfflineJob
 *
 */

qmimap4::CopyOfflineJob::CopyOfflineJob(const WCHAR* pwszFolderFrom,
										const WCHAR* pwszFolderTo,
										const UidList& listUidFrom,
										const ItemList& listItemTo,
										bool bMove) :
	OfflineJob(pwszFolderFrom),
	listUidFrom_(listUidFrom),
	listItemTo_(listItemTo),
	bMove_(bMove)
{
	assert(!listUidFrom.empty());
	assert(listUidFrom.size() == listItemTo.size());
	
	wstrFolderTo_ = allocWString(pwszFolderTo);
}

qmimap4::CopyOfflineJob::~CopyOfflineJob()
{
}

OfflineJob::Type qmimap4::CopyOfflineJob::getType() const
{
	return TYPE_COPY;
}

bool qmimap4::CopyOfflineJob::apply(Account* pAccount,
									Imap4* pImap4,
									bool* pbClosed) const
{
	assert(pAccount);
	assert(pImap4);
	assert(pbClosed);
	
	assert(!listUidFrom_.empty());
	assert(listUidFrom_.size() == listItemTo_.size());
	
	Folder* pFolder = pAccount->getFolder(wstrFolderTo_.get());
	if (pFolder && pFolder->getType() == Folder::TYPE_NORMAL) {
		NormalFolder* pNormalFolder = static_cast<NormalFolder*>(pFolder);
		if (!pNormalFolder->loadMessageHolders())
			return false;
		
		wstring_ptr wstrName(Util::getFolderName(pNormalFolder));
		
		// TODO
		// Apply flags
		
		MultipleRange range(&listUidFrom_[0], listUidFrom_.size(), true);
		if (!pImap4->copy(range, wstrName.get()))
			return false;
		if (bMove_) {
			Flags flags(Imap4::FLAG_DELETED);
			if (!pImap4->setFlags(range, flags, flags))
				return false;
		}
		
		Lock<Account> lock(*pAccount);
		
		MessageHolderList l;
		l.reserve(listItemTo_.size());
		for (ItemList::const_iterator it = listItemTo_.begin(); it != listItemTo_.end(); ++it) {
			MessageHolder* pmh = pNormalFolder->getMessageHolderById((*it).nId_);
			if (pmh && pmh->isFlag(MessageHolder::FLAG_LOCAL))
				l.push_back(pmh);
		}
		if (!pAccount->unstoreMessages(l, 0))
			return false;
	}
	
	return true;
}

bool qmimap4::CopyOfflineJob::write(OutputStream* pStream) const
{
	if (!OfflineJob::write(pStream))
		return false;
	
	WRITE_STRING(WSTRING, wstrFolderTo_.get());
	unsigned int nSize = static_cast<unsigned int>(listUidFrom_.size());
	WRITE(&nSize, sizeof(nSize));
	WRITE(&listUidFrom_[0], listUidFrom_.size()*sizeof(UidList::value_type));
	WRITE(&listItemTo_[0], listItemTo_.size()*sizeof(ItemList::value_type));
	WRITE(&bMove_, sizeof(bMove_));
	
	return true;
}

bool qmimap4::CopyOfflineJob::isCreateMessage(const WCHAR* pwszFolder,
											  unsigned long nId)
{
	return wcscmp(pwszFolder, wstrFolderTo_.get()) == 0 &&
		std::find_if(listItemTo_.begin(), listItemTo_.end(),
			std::bind2nd(
				binary_compose_f_gx_hy(
					std::equal_to<unsigned long>(),
					mem_data_ref(&Item::nId_),
					std::identity<unsigned long>()),
				nId)) != listItemTo_.end();
}

bool qmimap4::CopyOfflineJob::merge(OfflineJob* pOfflineJob)
{
	assert(pOfflineJob);
	
	if (pOfflineJob->getType() == TYPE_COPY &&
		wcscmp(getFolder(), pOfflineJob->getFolder()) == 0) {
		CopyOfflineJob* p = static_cast<CopyOfflineJob*>(pOfflineJob);
		if (wcscmp(wstrFolderTo_.get(), p->wstrFolderTo_.get()) == 0 && bMove_ == p->bMove_) {
			UidList listUidFrom;
			listUidFrom.reserve(listUidFrom_.size() + p->listUidFrom_.size());
			ItemList listItemTo;
			listItemTo.reserve(listItemTo_.size() + p->listItemTo_.size());
			
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
			
			return true;
		}
	}
	
	return false;
}

std::auto_ptr<OfflineJob> qmimap4::CopyOfflineJob::create(qs::InputStream* pStream)
{
	wstring_ptr wstrFolderFrom;
	READ_STRING(WSTRING, wstrFolderFrom);
	if (!wstrFolderFrom.get())
		return std::auto_ptr<OfflineJob>(0);
	
	wstring_ptr wstrFolderTo;
	READ_STRING(WSTRING, wstrFolderTo);
	if (!wstrFolderTo.get())
		return std::auto_ptr<OfflineJob>(0);
	
	unsigned int nSize = 0;
	READ(&nSize, sizeof(nSize));
	if (nSize == 0)
		return std::auto_ptr<OfflineJob>(0);
	UidList listUidFrom;
	listUidFrom.resize(nSize);
	ItemList listItemTo;
	listItemTo.resize(nSize);
	READ(&listUidFrom[0], nSize*sizeof(UidList::value_type));
	READ(&listItemTo[0], nSize*sizeof(ItemList::value_type));
	
	bool bMove = false;
	READ(&bMove, sizeof(bMove));
	
	return std::auto_ptr<OfflineJob>(new CopyOfflineJob(wstrFolderFrom.get(),
		wstrFolderTo.get(), listUidFrom, listItemTo, bMove));
}


/****************************************************************************
 *
 * SetFlagsOfflineJob
 *
 */

qmimap4::SetFlagsOfflineJob::SetFlagsOfflineJob(const WCHAR* pwszFolder,
												const UidList& listUid,
												unsigned int nFlags,
												unsigned int nMask) :
	OfflineJob(pwszFolder),
	listUid_(listUid),
	nFlags_(nFlags),
	nMask_(nMask)
{
	assert(!listUid.empty());
}

qmimap4::SetFlagsOfflineJob::~SetFlagsOfflineJob()
{
}

OfflineJob::Type qmimap4::SetFlagsOfflineJob::getType() const
{
	return TYPE_SETFLAGS;
}

bool qmimap4::SetFlagsOfflineJob::apply(Account* pAccount,
										Imap4* pImap4,
										bool* pbClosed) const
{
	assert(pAccount);
	assert(pImap4);
	assert(pbClosed);
	assert(!listUid_.empty());
	
	MultipleRange range(&listUid_[0], listUid_.size(), true);
	Flags flags(Util::getImap4FlagsFromMessageFlags(nFlags_));
	Flags mask(Util::getImap4FlagsFromMessageFlags(nMask_));
	if (!pImap4->setFlags(range, flags, mask)) {
		// Because some servers return NO response when I try
		// storing flags to UID that doesn't exist, I ignore this.
		return (pImap4->getLastError() & Imap4::IMAP4_ERROR_MASK_LOWLEVEL) ==
			Imap4::IMAP4_ERROR_RESPONSE;
	}
	return true;
}

bool qmimap4::SetFlagsOfflineJob::write(OutputStream* pStream) const
{
	if (!OfflineJob::write(pStream))
		return false;
	
	unsigned int nSize = static_cast<unsigned int>(listUid_.size());
	WRITE(&nSize, sizeof(nSize));
	WRITE(&listUid_[0], listUid_.size()*sizeof(UidList::value_type));
	WRITE(&nFlags_, sizeof(nFlags_));
	WRITE(&nMask_, sizeof(nMask_));
	
	return true;
}

bool qmimap4::SetFlagsOfflineJob::isCreateMessage(const WCHAR* pwszFolder,
												  unsigned long nId)
{
	return false;
}

bool qmimap4::SetFlagsOfflineJob::merge(OfflineJob* pOfflineJob)
{
	assert(pOfflineJob);
	
	if (pOfflineJob->getType() == TYPE_SETFLAGS &&
		wcscmp(getFolder(), pOfflineJob->getFolder()) == 0) {
		SetFlagsOfflineJob* p = static_cast<SetFlagsOfflineJob*>(pOfflineJob);
		if (nFlags_ == p->nFlags_ && nMask_ == p->nMask_) {
			UidList l;
			l.reserve(listUid_.size() + p->listUid_.size());
			std::merge(listUid_.begin(), listUid_.end(),
				p->listUid_.begin(), p->listUid_.end(),
				std::back_inserter(l));
			l.erase(std::unique(l.begin(), l.end()), l.end());
			listUid_.swap(l);
			
			return true;
		}
	}
	
	return false;
}

std::auto_ptr<OfflineJob> qmimap4::SetFlagsOfflineJob::create(qs::InputStream* pStream)
{
	wstring_ptr wstrFolder;
	READ_STRING(WSTRING, wstrFolder);
	if (!wstrFolder.get())
		return std::auto_ptr<OfflineJob>(0);
	
	unsigned int nSize = 0;
	READ(&nSize, sizeof(nSize));
	if (nSize == 0)
		return std::auto_ptr<OfflineJob>(0);
	UidList listUid;
	listUid.resize(nSize);
	READ(&listUid[0], nSize*sizeof(UidList::value_type));
	
	unsigned int nFlags = 0;
	READ(&nFlags, sizeof(nFlags));
	
	unsigned int nMask = 0;
	READ(&nMask, sizeof(nMask));
	
	return std::auto_ptr<OfflineJob>(new SetFlagsOfflineJob(
		wstrFolder.get(), listUid, nFlags, nMask));
}


/****************************************************************************
 *
 * SetLabelOfflineJob
 *
 */

qmimap4::SetLabelOfflineJob::SetLabelOfflineJob(const WCHAR* pwszFolder,
												const UidList& listUid,
												const WCHAR* pwszLabel,
												const WCHAR** ppwszMask,
												size_t nMaskCount) :
	OfflineJob(pwszFolder),
	listUid_(listUid)
{
	if (pwszLabel)
		wstrLabel_ = allocWString(pwszLabel);
	
	listLabel_.reserve(nMaskCount);
	for (size_t n = 0; n < nMaskCount; ++n)
		listLabel_.push_back(allocWString(ppwszMask[n]).release());
}

qmimap4::SetLabelOfflineJob::~SetLabelOfflineJob()
{
	std::for_each(listLabel_.begin(), listLabel_.end(), qs::string_free<WSTRING>());
}

OfflineJob::Type qmimap4::SetLabelOfflineJob::getType() const
{
	return TYPE_SETLABEL;
}

bool qmimap4::SetLabelOfflineJob::apply(Account* pAccount,
										Imap4* pImap4,
										bool* pbClosed) const
{
	assert(pAccount);
	assert(pImap4);
	assert(pbClosed);
	assert(!listUid_.empty());
	
	MultipleRange range(&listUid_[0], listUid_.size(), true);
	const WCHAR* pwszLabel = wstrLabel_.get();
	std::auto_ptr<Flags> pFlags(Util::getImap4FlagsFromLabels(
		0, &pwszLabel, pwszLabel && *pwszLabel ? 1 : 0));
	if (!pFlags.get())
		return false;
	std::auto_ptr<Flags> pMask(Util::getImap4FlagsFromLabels(0,
		const_cast<const WCHAR**>(&listLabel_[0]), listLabel_.size()));
	if (!pMask.get())
		return false;
	if (!pImap4->setFlags(range, *pFlags, *pMask)) {
		// Because some servers return NO response when I try
		// storing flags to UID that doesn't exist, I ignore this.
		return (pImap4->getLastError() & Imap4::IMAP4_ERROR_MASK_LOWLEVEL) ==
			Imap4::IMAP4_ERROR_RESPONSE;
	}
	return true;
}

bool qmimap4::SetLabelOfflineJob::write(OutputStream* pStream) const
{
	if (!OfflineJob::write(pStream))
		return false;
	
	unsigned int nSize = static_cast<unsigned int>(listUid_.size());
	WRITE(&nSize, sizeof(nSize));
	WRITE(&listUid_[0], listUid_.size()*sizeof(UidList::value_type));
	WRITE_STRING(WSTRING, wstrLabel_.get());
	unsigned int nLabelSize = static_cast<unsigned int>(listLabel_.size());
	WRITE(&nLabelSize, sizeof(nLabelSize));
	for (LabelList::const_iterator it = listLabel_.begin(); it != listLabel_.end(); ++it)
		WRITE_STRING(WSTRING, *it);
	
	return true;
}

bool qmimap4::SetLabelOfflineJob::isCreateMessage(const WCHAR* pwszFolder,
												  unsigned long nId)
{
	return false;
}

bool qmimap4::SetLabelOfflineJob::merge(OfflineJob* pOfflineJob)
{
	return false;
}

std::auto_ptr<OfflineJob> qmimap4::SetLabelOfflineJob::create(InputStream* pStream)
{
	wstring_ptr wstrFolder;
	READ_STRING(WSTRING, wstrFolder);
	if (!wstrFolder.get())
		return std::auto_ptr<OfflineJob>(0);
	
	unsigned int nSize = 0;
	READ(&nSize, sizeof(nSize));
	if (nSize == 0)
		return std::auto_ptr<OfflineJob>(0);
	UidList listUid;
	listUid.resize(nSize);
	READ(&listUid[0], nSize*sizeof(UidList::value_type));
	
	wstring_ptr wstrLabel;
	READ_STRING(WSTRING, wstrLabel);
	
	unsigned int nLabelSize = 0;
	READ(&nLabelSize, sizeof(nLabelSize));
	LabelList listLabel;
	listLabel.reserve(nLabelSize);
	StringListFree<LabelList> free(listLabel);
	for (unsigned int n = 0; n < nLabelSize; ++n) {
		wstring_ptr wstrLabel;
		READ_STRING(WSTRING, wstrLabel);
		listLabel.push_back(wstrLabel.release());
	}
	
	return std::auto_ptr<OfflineJob>(new SetLabelOfflineJob(wstrFolder.get(), listUid,
		wstrLabel.get(), const_cast<const WCHAR**>(&listLabel[0]), listLabel.size()));
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

std::auto_ptr<OfflineJob> qmimap4::OfflineJobFactory::getInstance(InputStream* pStream) const
{
	assert(pStream);
	
	size_t nRead = 0;
	OfflineJob::Type type;
	nRead = pStream->read(reinterpret_cast<unsigned char*>(&type), sizeof(type));
	if (nRead == -1)
		return std::auto_ptr<OfflineJob>(0);
	else if (nRead != sizeof(type))
		return std::auto_ptr<OfflineJob>(0);	// TODO Handle EOF
	
#define BEGIN_OFFLINEJOB() \
	switch (type) { \

#define DECLARE_OFFLINEJOB(type, classname) \
	case OfflineJob::type: \
		return classname::create(pStream); \

#define END_OFFLINEJOB() \
	default: \
		return std::auto_ptr<OfflineJob>(0); \
	} \
	
	BEGIN_OFFLINEJOB()
		DECLARE_OFFLINEJOB(TYPE_APPEND, AppendOfflineJob)
		DECLARE_OFFLINEJOB(TYPE_COPY, CopyOfflineJob)
		DECLARE_OFFLINEJOB(TYPE_SETFLAGS, SetFlagsOfflineJob)
		DECLARE_OFFLINEJOB(TYPE_SETLABEL, SetLabelOfflineJob)
	END_OFFLINEJOB()
	
	return std::auto_ptr<OfflineJob>(0);
}

bool qmimap4::OfflineJobFactory::writeInstance(OutputStream* pStream,
											   OfflineJob* pJob) const
{
	OfflineJob::Type type = pJob->getType();
	if (pStream->write(reinterpret_cast<unsigned char*>(&type), sizeof(type)) == -1)
		return false;
	
	if (!pJob->write(pStream))
		return false;
	
	return true;
}
