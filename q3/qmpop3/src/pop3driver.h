/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
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
	virtual bool save();
	virtual bool isSupport(qm::Account::Support support);
	virtual void setOffline(bool bOffline);
	
	virtual std::auto_ptr<qm::NormalFolder> createFolder(qm::SubAccount* pSubAccount,
														 const WCHAR* pwszName,
														 qm::Folder* pParent);
	virtual bool removeFolder(qm::SubAccount* pSubAccount,
							  qm::NormalFolder* pFolder);
	virtual bool renameFolder(qm::SubAccount* pSubAccount,
							  qm::NormalFolder* pFolder,
							  const WCHAR* pwszName);
	virtual bool createDefaultFolders(qm::Account::FolderList* pList);
	virtual bool getRemoteFolders(qm::SubAccount* pSubAccount,
								  RemoteFolderList* pList);
	virtual std::pair<const WCHAR**, size_t> getFolderParamNames();
	
	virtual bool getMessage(qm::SubAccount* pSubAccount,
							qm::MessageHolder* pmh,
							unsigned int nFlags,
							qs::xstring_ptr* pstrMessage,
							qm::Message::Flag* pFlag,
							bool* pbMadeSeen);
	virtual bool setMessagesFlags(qm::SubAccount* pSubAccount,
								  qm::NormalFolder* pFolder,
								  const qm::MessageHolderList& l,
								  unsigned int nFlags,
								  unsigned int nMask);
	virtual bool appendMessage(qm::SubAccount* pSubAccount,
							   qm::NormalFolder* pFolder,
							   const CHAR* pszMessage,
							   unsigned int nFlags);
	virtual bool removeMessages(qm::SubAccount* pSubAccount,
								qm::NormalFolder* pFolder,
								const qm::MessageHolderList& l);
	virtual bool copyMessages(qm::SubAccount* pSubAccount,
							  const qm::MessageHolderList& l,
							  qm::NormalFolder* pFolderFrom,
							  qm::NormalFolder* pFolderTo,
							  bool bMove);
	virtual bool clearDeletedMessages(qm::SubAccount* pSubAccount,
									  qm::NormalFolder* pFolder);

private:
	Pop3Driver(const Pop3Driver&);
	Pop3Driver& operator=(const Pop3Driver&);

private:
	qm::Account* pAccount_;
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
														   const qm::Security* pSecurity);

private:
	Pop3Factory(const Pop3Factory&);
	Pop3Factory& operator=(const Pop3Factory&);

private:
	static Pop3Factory factory__;
};

}

#endif // __POP3DRIVER_H__
