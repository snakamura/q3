/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __OFFLINEJOB_H__
#define __OFFLINEJOB_H__

#include <qmaccount.h>
#include <qmsession.h>

#include <qs.h>
#include <qsthread.h>

#include <vector>


namespace qmimap4 {

class OfflineJobManager;
class OfflineJob;
	class AppendOfflineJob;
	class CopyOfflineJob;
	class ExpungeOfflineJob;
	class SetFlagsOfflineJob;
class OfflineJobFactory;

class Imap4;
class Range;


/****************************************************************************
 *
 * OfflineJobManager
 *
 */

class OfflineJobManager
{
public:
	typedef std::vector<unsigned long> UidList;

public:
	OfflineJobManager(const WCHAR* pwszPath, qs::QSTATUS* pstatus);
	~OfflineJobManager();

public:
	qs::QSTATUS add(OfflineJob* pJob);
	qs::QSTATUS apply(qm::Account* pAccount, Imap4* pImap4,
		qm::ReceiveSessionCallback* pCallback);
	qs::QSTATUS save(const WCHAR* pwszPath) const;
	qs::QSTATUS copyJobs(qm::NormalFolder* pFolderFrom,
		qm::NormalFolder* pFolderTo, const UidList& listUid, bool bMove);

private:
	qs::QSTATUS load(const WCHAR* pwszPath);
	OfflineJob* getCreateMessage(const WCHAR* pwszFolder,
		unsigned long nId) const;

private:
	OfflineJobManager(const OfflineJobManager&);
	OfflineJobManager& operator=(const OfflineJobManager&);

private:
	typedef std::vector<OfflineJob*> JobList;

private:
	JobList listJob_;
	qs::CriticalSection cs_;

private:
	static const WCHAR* FILENAME;
};


/****************************************************************************
 *
 * OfflineJob
 *
 */

class OfflineJob
{
public:
	enum Type {
		TYPE_APPEND		= 1,
		TYPE_COPY		= 2,
		TYPE_EXPUNGE	= 3,
		TYPE_SETFLAGS	= 4
	};

protected:
	OfflineJob(const WCHAR* pwszFolder, qs::QSTATUS* pstatus);
	OfflineJob(qs::InputStream* pStream, qs::QSTATUS* pstatus);

public:
	virtual ~OfflineJob();

public:
	virtual Type getType() const = 0;
	virtual qs::QSTATUS apply(qm::Account* pAccount,
		Imap4* pImap4, bool* pbClosed) const = 0;
	virtual qs::QSTATUS write(qs::OutputStream* pStream) const = 0;
	virtual bool isCreateMessage(const WCHAR* pwszFolder, unsigned long nId) = 0;
	virtual qs::QSTATUS merge(OfflineJob* pOfflineJob, bool* pbMerged) = 0;

public:
	const WCHAR* getFolder() const;

private:
	qs::WSTRING wstrFolder_;
};


/****************************************************************************
 *
 * AppendOfflineJob
 *
 */

class AppendOfflineJob : public OfflineJob
{
public:
	AppendOfflineJob(const WCHAR* pwszFolder,
		unsigned int nId, qs::QSTATUS* pstatus);
	AppendOfflineJob(qs::InputStream* pStream, qs::QSTATUS* pstatus);
	virtual ~AppendOfflineJob();

public:
	virtual Type getType() const;
	virtual qs::QSTATUS apply(qm::Account* pAccount,
		Imap4* pImap4, bool* pbClosed) const;
	virtual qs::QSTATUS write(qs::OutputStream* pStream) const;
	virtual bool isCreateMessage(const WCHAR* pwszFolder, unsigned long nId);
	virtual qs::QSTATUS merge(OfflineJob* pOfflineJob, bool* pbMerged);

private:
	AppendOfflineJob(const AppendOfflineJob&);
	AppendOfflineJob& operator=(const AppendOfflineJob&);

private:
	qs::WSTRING wstrFolder_;
	unsigned int nId_;
};


/****************************************************************************
 *
 * CopyOfflineJob
 *
 */

class CopyOfflineJob : public OfflineJob
{
public:
	struct Item
	{
		unsigned long nId_;
		unsigned int nFlags_;
	};

public:
	typedef std::vector<unsigned long> UidList;
	typedef std::vector<Item> ItemList;

public:
	CopyOfflineJob(const WCHAR* pwszFolderFrom, const WCHAR* pwszFolderTo,
		const UidList& listUidFrom, const ItemList& listItemTo,
		bool bMove, qs::QSTATUS* pstatus);
	CopyOfflineJob(qs::InputStream* pStream, qs::QSTATUS* pstatus);
	virtual ~CopyOfflineJob();

public:
	virtual Type getType() const;
	virtual qs::QSTATUS apply(qm::Account* pAccount,
		Imap4* pImap4, bool* pbClosed) const;
	virtual qs::QSTATUS write(qs::OutputStream* pStream) const;
	virtual bool isCreateMessage(const WCHAR* pwszFolder, unsigned long nId);
	virtual qs::QSTATUS merge(OfflineJob* pOfflineJob, bool* pbMerged);

private:
	CopyOfflineJob(const CopyOfflineJob&);
	CopyOfflineJob& operator=(const CopyOfflineJob&);

private:
	qs::WSTRING wstrFolderTo_;
	UidList listUidFrom_;
	ItemList listItemTo_;
	bool bMove_;
};


/****************************************************************************
 *
 * ExpungeOfflineJob
 *
 */

class ExpungeOfflineJob : public OfflineJob
{
public:
	ExpungeOfflineJob(const WCHAR* pwszFolder, qs::QSTATUS* pstatus);
	ExpungeOfflineJob(qs::InputStream* pStream, qs::QSTATUS* pstatus);
	virtual ~ExpungeOfflineJob();

public:
	virtual Type getType() const;
	virtual qs::QSTATUS apply(qm::Account* pAccount,
		Imap4* pImap4, bool* pbClosed) const;
	virtual qs::QSTATUS write(qs::OutputStream* pStream) const;
	virtual bool isCreateMessage(const WCHAR* pwszFolder, unsigned long nId);
	virtual qs::QSTATUS merge(OfflineJob* pOfflineJob, bool* pbMerged);

private:
	ExpungeOfflineJob(const ExpungeOfflineJob&);
	ExpungeOfflineJob& operator=(const ExpungeOfflineJob&);
};


/****************************************************************************
 *
 * SetFlagsOfflineJob
 *
 */

class SetFlagsOfflineJob : public OfflineJob
{
public:
	typedef std::vector<unsigned long> UidList;

public:
	SetFlagsOfflineJob(const WCHAR* pwszFolder, const UidList& listUid,
		unsigned int nFlags, unsigned int nMask, qs::QSTATUS* pstatus);
	SetFlagsOfflineJob(qs::InputStream* pStream, qs::QSTATUS* pstatus);
	virtual ~SetFlagsOfflineJob();

public:
	virtual Type getType() const;
	virtual qs::QSTATUS apply(qm::Account* pAccount,
		Imap4* pImap4, bool* pbClosed) const;
	virtual qs::QSTATUS write(qs::OutputStream* pStream) const;
	virtual bool isCreateMessage(const WCHAR* pwszFolder, unsigned long nId);
	virtual qs::QSTATUS merge(OfflineJob* pOfflineJob, bool* pbMerged);

private:
	SetFlagsOfflineJob(const SetFlagsOfflineJob&);
	SetFlagsOfflineJob& operator=(const SetFlagsOfflineJob&);

private:
	UidList listUid_;
	unsigned int nFlags_;
	unsigned int nMask_;
};


/****************************************************************************
 *
 * OfflineJobFactory
 *
 */

class OfflineJobFactory
{
public:
	OfflineJobFactory();
	~OfflineJobFactory();

public:
	qs::QSTATUS getInstance(qs::InputStream* pStream, OfflineJob** ppJob) const;
	qs::QSTATUS writeInstance(qs::OutputStream* pStream, OfflineJob* pJob) const;

private:
	OfflineJobFactory(const OfflineJobFactory&);
	OfflineJobFactory& operator=(const OfflineJobFactory&);
};

}

#endif // __OFFLINEJOB_H__
