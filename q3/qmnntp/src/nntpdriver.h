/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __NNTPDRIVER_H__
#define __NNTPDRIVER_H__

#include <qmprotocoldriver.h>

#include "util.h"


namespace qmnntp {

class NntpDriver;
class NntpFactory;

class Nntp;


/****************************************************************************
 *
 * NntpDriver
 *
 */

class NntpDriver : public qm::ProtocolDriver
{
public:
	NntpDriver(qm::Account* pAccount,
			   qm::PasswordCallback* pPasswordCallback,
			   const qm::Security* pSecurity);
	virtual ~NntpDriver();

public:
	virtual bool init();
	virtual bool save(bool bForce);
	virtual bool isSupport(qm::Account::Support support);
	virtual void setOffline(bool bOffline);
	virtual void setSubAccount(qm::SubAccount* pSubAccount);
	
	virtual std::auto_ptr<qm::NormalFolder> createFolder(const WCHAR* pwszName,
														 qm::Folder* pParent);
	virtual bool removeFolder(qm::NormalFolder* pFolder);
	virtual bool renameFolder(qm::NormalFolder* pFolder,
							  const WCHAR* pwszName);
	virtual bool moveFolder(qm::NormalFolder* pFolder,
							qm::NormalFolder* pParent,
							const WCHAR* pwszName);
	virtual bool createDefaultFolders(qm::Account::FolderList* pList);
	virtual bool getRemoteFolders(RemoteFolderList* pList);
	virtual std::pair<const WCHAR**, size_t> getFolderParamNames(bool bSyncable);
	virtual void setDefaultFolderParams(qm::NormalFolder* pFolder);
	
	virtual bool getMessage(qm::MessageHolder* pmh,
							unsigned int nFlags,
							GetMessageCallback* pCallback);
	virtual bool setMessagesFlags(qm::NormalFolder* pFolder,
								  const qm::MessageHolderList& l,
								  unsigned int nFlags,
								  unsigned int nMask);
	virtual bool setMessagesLabel(qm::NormalFolder* pFolder,
								  const qm::MessageHolderList& l,
								  const WCHAR* pwszLabel);
	virtual bool appendMessage(qm::NormalFolder* pFolder,
							   const CHAR* pszMessage,
							   size_t nLen,
							   unsigned int nFlags,
							   const WCHAR* pwszLabel);
	virtual bool removeMessages(qm::NormalFolder* pFolder,
								const qm::MessageHolderList& l);
	virtual bool copyMessages(const qm::MessageHolderList& l,
							  qm::NormalFolder* pFolderFrom,
							  qm::NormalFolder* pFolderTo,
							  bool bMove);

private:
	bool prepareSession(qm::NormalFolder* pFolder);
	void clearSession();
	bool isForceDisconnect() const;

private:
	NntpDriver(const NntpDriver&);
	NntpDriver& operator=(const NntpDriver&);

private:
	qm::Account* pAccount_;
	qm::PasswordCallback* pPasswordCallback_;
	const qm::Security* pSecurity_;
	std::auto_ptr<Nntp> pNntp_;
	std::auto_ptr<DefaultCallback> pCallback_;
	std::auto_ptr<qs::Logger> pLogger_;
	qm::SubAccount* pSubAccount_;
	bool bOffline_;
	unsigned int nForceDisconnect_;
	unsigned int nLastUsedTime_;

private:
	static const unsigned int nSupport__;
};


/****************************************************************************
 *
 * NntpFactory
 *
 */

class NntpFactory : public qm::ProtocolFactory
{
private:
	NntpFactory();

public:
	virtual ~NntpFactory();

protected:
	virtual std::auto_ptr<qm::ProtocolDriver> createDriver(qm::Account* pAccount,
														   qm::PasswordCallback* pPasswordCallback,
														   const qm::Security* pSecurity);

private:
	NntpFactory(const NntpFactory&);
	NntpFactory& operator=(const NntpFactory&);

private:
	static NntpFactory factory__;
};

}

#endif // __NNTPDRIVER_H__
