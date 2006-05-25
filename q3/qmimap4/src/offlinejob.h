/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
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
	class SetFlagsOfflineJob;
	class SetLabelOfflineJob;
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
	OfflineJobManager(const WCHAR* pwszPath);
	~OfflineJobManager();

public:
	void add(std::auto_ptr<OfflineJob> pJob);
	bool apply(qm::Account* pAccount,
			   Imap4* pImap4,
			   qm::ReceiveSessionCallback* pCallback);
	bool save() const;
	bool copyJobs(qm::NormalFolder* pFolderFrom,
				  qm::NormalFolder* pFolderTo,
				  const UidList& listUid,
				  bool bMove);

private:
	bool load();
	OfflineJob* getCreateMessage(const WCHAR* pwszFolder,
								 unsigned long nId) const;

private:
	OfflineJobManager(const OfflineJobManager&);
	OfflineJobManager& operator=(const OfflineJobManager&);

private:
	typedef std::vector<OfflineJob*> JobList;

private:
	qs::wstring_ptr wstrPath_;
	JobList listJob_;
	mutable bool bModified_;
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
		TYPE_SETFLAGS	= 4,
		TYPE_SETLABEL	= 5
	};

protected:
	OfflineJob(const WCHAR* pwszFolder);

public:
	virtual ~OfflineJob();

public:
	virtual Type getType() const = 0;
	virtual bool apply(qm::Account* pAccount,
					   Imap4* pImap4,
					   bool* pbClosed) const = 0;
	virtual bool write(qs::OutputStream* pStream) const = 0;
	virtual bool isCreateMessage(const WCHAR* pwszFolder,
								 unsigned long nId) = 0;
	virtual bool merge(OfflineJob* pOfflineJob) = 0;

public:
	const WCHAR* getFolder() const;

private:
	qs::wstring_ptr wstrFolder_;
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
					 unsigned int nId);
	virtual ~AppendOfflineJob();

public:
	virtual Type getType() const;
	virtual bool  apply(qm::Account* pAccount,
						Imap4* pImap4,
						bool* pbClosed) const;
	virtual bool write(qs::OutputStream* pStream) const;
	virtual bool isCreateMessage(const WCHAR* pwszFolder,
								 unsigned long nId);
	virtual bool merge(OfflineJob* pOfflineJob);

public:
	static std::auto_ptr<OfflineJob> create(qs::InputStream* pStream);

private:
	AppendOfflineJob(const AppendOfflineJob&);
	AppendOfflineJob& operator=(const AppendOfflineJob&);

private:
	qs::wstring_ptr wstrFolder_;
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
	CopyOfflineJob(const WCHAR* pwszFolderFrom,
				   const WCHAR* pwszFolderTo,
				   const UidList& listUidFrom,
				   const ItemList& listItemTo,
				   bool bMove);
	virtual ~CopyOfflineJob();

public:
	virtual Type getType() const;
	virtual bool apply(qm::Account* pAccount,
					   Imap4* pImap4,
					   bool* pbClosed) const;
	virtual bool write(qs::OutputStream* pStream) const;
	virtual bool isCreateMessage(const WCHAR* pwszFolder,
								 unsigned long nId);
	virtual bool merge(OfflineJob* pOfflineJob);

public:
	static std::auto_ptr<OfflineJob> create(qs::InputStream* pStream);

private:
	CopyOfflineJob(const CopyOfflineJob&);
	CopyOfflineJob& operator=(const CopyOfflineJob&);

private:
	qs::wstring_ptr wstrFolderTo_;
	UidList listUidFrom_;
	ItemList listItemTo_;
	bool bMove_;
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
	SetFlagsOfflineJob(const WCHAR* pwszFolder,
					   const UidList& listUid,
					   unsigned int nFlags,
					   unsigned int nMask);
	virtual ~SetFlagsOfflineJob();

public:
	virtual Type getType() const;
	virtual bool apply(qm::Account* pAccount,
					   Imap4* pImap4,
					   bool* pbClosed) const;
	virtual bool write(qs::OutputStream* pStream) const;
	virtual bool isCreateMessage(const WCHAR* pwszFolder,
								 unsigned long nId);
	virtual bool merge(OfflineJob* pOfflineJob);

public:
	static std::auto_ptr<OfflineJob> create(qs::InputStream* pStream);

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
 * SetLabelOfflineJob
 *
 */

class SetLabelOfflineJob : public OfflineJob
{
public:
	typedef std::vector<unsigned long> UidList;
	typedef std::vector<qs::WSTRING> LabelList;

public:
	SetLabelOfflineJob(const WCHAR* pwszFolder,
					   const UidList& listUid,
					   const WCHAR* pwszLabel,
					   const WCHAR** ppwszMask,
					   size_t nMaskCount);
	virtual ~SetLabelOfflineJob();

public:
	virtual Type getType() const;
	virtual bool apply(qm::Account* pAccount,
					   Imap4* pImap4,
					   bool* pbClosed) const;
	virtual bool write(qs::OutputStream* pStream) const;
	virtual bool isCreateMessage(const WCHAR* pwszFolder,
								 unsigned long nId);
	virtual bool merge(OfflineJob* pOfflineJob);

public:
	static std::auto_ptr<OfflineJob> create(qs::InputStream* pStream);

private:
	SetLabelOfflineJob(const SetLabelOfflineJob&);
	SetLabelOfflineJob& operator=(const SetLabelOfflineJob&);

private:
	UidList listUid_;
	qs::wstring_ptr wstrLabel_;
	LabelList listLabel_;
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
	std::auto_ptr<OfflineJob> getInstance(qs::InputStream* pStream) const;
	bool writeInstance(qs::OutputStream* pStream,
					   OfflineJob* pJob) const;

private:
	OfflineJobFactory(const OfflineJobFactory&);
	OfflineJobFactory& operator=(const OfflineJobFactory&);
};

}

#endif // __OFFLINEJOB_H__
