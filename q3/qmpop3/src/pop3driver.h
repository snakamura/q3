/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __POP3DRIVER_H__
#define __POP3DRIVER_H__

#include <qmprotocoldriver.h>


namespace qmpop3 {

class Pop3Driver;
class Pop3Factory;


/****************************************************************************
 *
 * Pop3Driver
 *
 */

class Pop3Driver : public qm::ProtocolDriver
{
public:
	explicit Pop3Driver(qm::Account* pAccount);
	virtual ~Pop3Driver();

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
	virtual std::pair<const WCHAR**, size_t> getFolderParamNames();
	
	virtual bool getMessage(qm::MessageHolder* pmh,
							unsigned int nFlags,
							GetMessageCallback* pCallback);
	virtual bool setMessagesFlags(qm::NormalFolder* pFolder,
								  const qm::MessageHolderList& l,
								  unsigned int nFlags,
								  unsigned int nMask);
	virtual bool appendMessage(qm::NormalFolder* pFolder,
							   const CHAR* pszMessage,
							   size_t nLen,
							   unsigned int nFlags);
	virtual bool removeMessages(qm::NormalFolder* pFolder,
								const qm::MessageHolderList& l);
	virtual bool copyMessages(const qm::MessageHolderList& l,
							  qm::NormalFolder* pFolderFrom,
							  qm::NormalFolder* pFolderTo,
							  bool bMove);

private:
	Pop3Driver(const Pop3Driver&);
	Pop3Driver& operator=(const Pop3Driver&);

private:
	qm::Account* pAccount_;

private:
	static const unsigned int nSupport__;
};


/****************************************************************************
 *
 * Pop3Factory
 *
 */

class Pop3Factory : public qm::ProtocolFactory
{
private:
	Pop3Factory();

public:
	virtual ~Pop3Factory();

protected:
	virtual std::auto_ptr<qm::ProtocolDriver> createDriver(qm::Account* pAccount,
														   qm::PasswordCallback* pPasswordCallback,
														   const qm::Security* pSecurity);

private:
	Pop3Factory(const Pop3Factory&);
	Pop3Factory& operator=(const Pop3Factory&);

private:
	static Pop3Factory factory__;
};

}

#endif // __POP3DRIVER_H__
