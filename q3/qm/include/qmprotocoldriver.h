/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QMPROTOCOLDRIVER_H__
#define __QMPROTOCOLDRIVER_H__

#include <qm.h>
#include <qmaccount.h>
#include <qmfolder.h>
#include <qmmessage.h>

#include <qs.h>

#include <memory>


namespace qm {

class ProtocolDriver;
class ProtocolFactory;

class MessageHolder;
class Security;


/****************************************************************************
 *
 * ProtocolDriver
 *
 */

class QMEXPORTCLASS ProtocolDriver
{
public:
	typedef std::vector<std::pair<Folder*, bool> > RemoteFolderList;

public:
	virtual ~ProtocolDriver();

public:
	virtual bool isSupport(Account::Support support) = 0;
	virtual void setOffline(bool bOffline) = 0;
	virtual bool save() = 0;
	
	virtual std::auto_ptr<NormalFolder> createFolder(SubAccount* pSubAccount,
													 const WCHAR* pwszName,
													 Folder* pParent) = 0;
	virtual bool removeFolder(SubAccount* pSubAccount,
							  NormalFolder* pFolder) = 0;
	virtual bool renameFolder(SubAccount* pSubAccount,
							  NormalFolder* pFolder,
							  const WCHAR* pwszName) = 0;
	virtual bool createDefaultFolders(Account::FolderList* pList) = 0;
	virtual bool getRemoteFolders(SubAccount* pSubAccount,
								  RemoteFolderList* pList) = 0;
	
	virtual bool getMessage(SubAccount* pSubAccount,
							MessageHolder* pmh,
							unsigned int nFlags,
							qs::xstring_ptr* pstrMessage,
							Message::Flag* pFlag,
							bool* pbGet,
							bool* pbMadeSeen) = 0;
	virtual bool setMessagesFlags(SubAccount* pSubAccount,
								  NormalFolder* pFolder,
								  const MessageHolderList& l,
								  unsigned int nFlags,
								  unsigned int nMask) = 0;
	virtual bool appendMessage(SubAccount* pSubAccount,
							   NormalFolder* pFolder,
							   const CHAR* pszMessage,
							   unsigned int nFlags) = 0;
	virtual bool removeMessages(SubAccount* pSubAccount,
								NormalFolder* pFolder,
								const MessageHolderList& l) = 0;
	virtual bool copyMessages(SubAccount* pSubAccount,
							  const MessageHolderList& l,
							  NormalFolder* pFolderFrom,
							  NormalFolder* pFolderTo,
							  bool bMove) = 0;
	virtual bool clearDeletedMessages(SubAccount* pSubAccount,
									  NormalFolder* pFolder) = 0;
};


/****************************************************************************
 *
 * ProtocolFactory
 *
 */

class QMEXPORTCLASS ProtocolFactory
{
protected:
	ProtocolFactory();

public:
	virtual ~ProtocolFactory();

public:
	static std::auto_ptr<ProtocolDriver> getDriver(Account* pAccount,
												   const Security* pSecurity);

protected:
	virtual std::auto_ptr<ProtocolDriver> createDriver(Account* pAccount,
													   const Security* pSecurity) = 0;

protected:
	static void registerFactory(const WCHAR* pwszName,
								ProtocolFactory* pFactory);
	static void unregisterFactory(const WCHAR* pwszName);

private:
	ProtocolFactory(const ProtocolFactory&);
	ProtocolFactory& operator=(const ProtocolFactory&);
};

}

#endif // __QMPROTOCOLDRIVER_H__
