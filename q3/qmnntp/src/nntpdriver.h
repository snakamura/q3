/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#ifndef __NNTPDRIVER_H__
#define __NNTPDRIVER_H__

#include <qmprotocoldriver.h>

#include <qsthread.h>

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
	virtual bool isSupport(qm::Account::Support support);
	virtual void setOffline(bool bOffline);
	virtual void setSubAccount(qm::SubAccount* pSubAccount);
	
	virtual bool createDefaultFolders(qm::Account::FolderList* pList);
	
	virtual bool getMessage(qm::MessageHolder* pmh,
							unsigned int nFlags,
							GetMessageCallback* pCallback);
	virtual bool prepareFolder(qm::NormalFolder* pFolder);

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
	qs::CriticalSection cs_;

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
	~NntpFactory();

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
