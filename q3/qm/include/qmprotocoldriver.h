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
class PasswordCallback;
class Security;


/****************************************************************************
 *
 * ProtocolDriver
 *
 */

class QMEXPORTCLASS ProtocolDriver
{
public:
	class GetMessageCallback
	{
	public:
		virtual ~GetMessageCallback();
	
	public:
		virtual bool message(const CHAR* pszMessage,
							 size_t nLen,
							 Message::Flag flag,
							 bool bMadeSeen) = 0;
	};

public:
	typedef std::vector<std::pair<Folder*, bool> > RemoteFolderList;

public:
	virtual ~ProtocolDriver();

public:
	virtual bool init() = 0;
	virtual bool save() = 0;
	virtual bool isSupport(Account::Support support) = 0;
	virtual void setOffline(bool bOffline) = 0;
	virtual void setSubAccount(SubAccount* pSubAccount) = 0;
	
	virtual std::auto_ptr<NormalFolder> createFolder(const WCHAR* pwszName,
													 Folder* pParent) = 0;
	virtual bool removeFolder(NormalFolder* pFolder) = 0;
	virtual bool renameFolder(NormalFolder* pFolder,
							  const WCHAR* pwszName) = 0;
	virtual bool moveFolder(NormalFolder* pFolder,
							NormalFolder* pParent) = 0;
	virtual bool createDefaultFolders(Account::FolderList* pList) = 0;
	virtual bool getRemoteFolders(RemoteFolderList* pList) = 0;
	virtual std::pair<const WCHAR**, size_t> getFolderParamNames() = 0;
	
	virtual bool getMessage(MessageHolder* pmh,
							unsigned int nFlags,
							GetMessageCallback* pCallback) = 0;
	virtual bool setMessagesFlags(NormalFolder* pFolder,
								  const MessageHolderList& l,
								  unsigned int nFlags,
								  unsigned int nMask) = 0;
	virtual bool appendMessage(NormalFolder* pFolder,
							   const CHAR* pszMessage,
							   unsigned int nFlags) = 0;
	virtual bool removeMessages(NormalFolder* pFolder,
								const MessageHolderList& l) = 0;
	virtual bool copyMessages(const MessageHolderList& l,
							  NormalFolder* pFolderFrom,
							  NormalFolder* pFolderTo,
							  bool bMove) = 0;
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
	static void setPasswordCallback(std::auto_ptr<PasswordCallback> pPasswordCallback);

protected:
	virtual std::auto_ptr<ProtocolDriver> createDriver(Account* pAccount,
													   PasswordCallback* pPasswordCallback,
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
