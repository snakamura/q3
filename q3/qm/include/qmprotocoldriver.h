/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QMPROTOCOLDRIVER_H__
#define __QMPROTOCOLDRIVER_H__

#include <qm.h>
#include <qmaccount.h>
#include <qmfolder.h>

#include <qs.h>

#include <memory>


namespace qm {

class ProtocolDriver;
class ProtocolFactory;

class Message;
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
	virtual qs::QSTATUS setOffline(bool bOffline) = 0;
	virtual qs::QSTATUS setForceOnline(bool bOnline) = 0;
	virtual qs::QSTATUS save() = 0;
	
	virtual qs::QSTATUS createFolder(SubAccount* pSubAccount,
		const WCHAR* pwszName, Folder* pParent, NormalFolder** ppFolder) = 0;
	virtual qs::QSTATUS removeFolder(
		SubAccount* pSubAccount, NormalFolder* pFolder) = 0;
	virtual qs::QSTATUS renameFolder(SubAccount* pSubAccount,
		NormalFolder* pFolder, const WCHAR* pwszName) = 0;
	virtual qs::QSTATUS createDefaultFolders(Account::FolderList* pList) = 0;
	virtual qs::QSTATUS getRemoteFolders(
		SubAccount* pSubAccount, RemoteFolderList* pList) = 0;
	
	virtual qs::QSTATUS getMessage(SubAccount* pSubAccount,
		MessageHolder* pmh, unsigned int nFlags,
		Message* pMessage, bool* pbGet, bool* pbMadeSeen) = 0;
	virtual qs::QSTATUS setMessagesFlags(SubAccount* pSubAccount,
		NormalFolder* pFolder, const Folder::MessageHolderList& l,
		unsigned int nFlags, unsigned int nMask) = 0;
	virtual qs::QSTATUS appendMessage(SubAccount* pSubAccount,
		NormalFolder* pFolder, const CHAR* pszMessage, unsigned int nFlags) = 0;
	virtual qs::QSTATUS removeMessages(SubAccount* pSubAccount,
		NormalFolder* pFolder, const Folder::MessageHolderList& l) = 0;
	virtual qs::QSTATUS copyMessages(SubAccount* pSubAccount,
		const Folder::MessageHolderList& l, NormalFolder* pFolderFrom,
		NormalFolder* pFolderTo, bool bMove) = 0;
	virtual qs::QSTATUS clearDeletedMessages(
		SubAccount* pSubAccount, NormalFolder* pFolder) = 0;
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
	static qs::QSTATUS getDriver(Account* pAccount,
		const Security* pSecurity, ProtocolDriver** ppProtocolDriver);
	static qs::QSTATUS getDriver(Account* pAccount, const Security* pSecurity,
		std::auto_ptr<ProtocolDriver>* papProtocolDriver);

protected:
	virtual qs::QSTATUS createDriver(Account* pAccount,
		const Security* pSecurity, ProtocolDriver** ppProtocolDriver) = 0;

protected:
	static qs::QSTATUS regist(const WCHAR* pwszName,
		ProtocolFactory* pFactory);
	static qs::QSTATUS unregist(const WCHAR* pwszName);

private:
	ProtocolFactory(const ProtocolFactory&);
	ProtocolFactory& operator=(const ProtocolFactory&);
};

}

#endif // __QMPROTOCOLDRIVER_H__
