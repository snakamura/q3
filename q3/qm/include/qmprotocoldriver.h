/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
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
	virtual bool init();
	virtual bool save(bool bForce);
	virtual bool isSupport(Account::Support support) = 0;
	virtual void setOffline(bool bOffline);
	virtual void setSubAccount(SubAccount* pSubAccount);
	
	virtual std::auto_ptr<NormalFolder> createFolder(const WCHAR* pwszName,
													 Folder* pParent);
	virtual bool removeFolder(NormalFolder* pFolder);
	virtual bool renameFolder(NormalFolder* pFolder,
							  const WCHAR* pwszName);
	virtual bool moveFolder(NormalFolder* pFolder,
							NormalFolder* pParent,
							const WCHAR* pwszName);
	virtual bool createDefaultFolders(Account::FolderList* pList);
	virtual bool getRemoteFolders(RemoteFolderList* pList);
	virtual std::pair<const WCHAR**, size_t> getFolderParamNames(Folder* pFolder);
	virtual void setDefaultFolderParams(NormalFolder* pFolder);
	
	virtual bool getMessage(MessageHolder* pmh,
							unsigned int nFlags,
							GetMessageCallback* pCallback);
	virtual bool setMessagesFlags(NormalFolder* pFolder,
								  const MessageHolderList& l,
								  unsigned int nFlags,
								  unsigned int nMask);
	virtual bool setMessagesLabel(NormalFolder* pFolder,
								  const MessageHolderList& l,
								  const WCHAR* pwszLabel);
	virtual bool appendMessage(NormalFolder* pFolder,
							   const CHAR* pszMessage,
							   size_t nLen,
							   unsigned int nFlags,
							   const WCHAR* pwszLabel);
	virtual bool removeMessages(NormalFolder* pFolder,
								const MessageHolderList& l);
	virtual bool copyMessages(const MessageHolderList& l,
							  NormalFolder* pFolderFrom,
							  NormalFolder* pFolderTo,
							  bool bMove);
	virtual bool prepareFolder(NormalFolder* pFolder);
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
