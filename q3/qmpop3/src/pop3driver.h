/*
 * $Id: pop3driver.h,v 1.1.1.1 2003/04/29 08:07:34 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
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
	Pop3Driver(qm::Account* pAccount, qs::QSTATUS* pstatus);
	virtual ~Pop3Driver();

public:
	virtual bool isSupport(qm::Account::Support support);
	virtual qs::QSTATUS setOffline(bool bOffline);
	virtual qs::QSTATUS save();
	
	virtual qs::QSTATUS createFolder(qm::SubAccount* pSubAccount,
		const WCHAR* pwszName, qm::Folder* pParent,
		qm::NormalFolder** ppFolder);
	virtual qs::QSTATUS createDefaultFolders(
		qm::Folder*** pppFolder, size_t* pnCount);
	virtual qs::QSTATUS getRemoteFolders(qm::SubAccount* pSubAccount,
		std::pair<qm::Folder*, bool>** ppFolder, size_t* pnCount);
	
	virtual qs::QSTATUS getMessage(qm::SubAccount* pSubAccount,
		qm::MessageHolder* pmh, unsigned int nFlags,
		qm::Message* pMessage, bool* pbGet, bool* pbMadeSeen);
	virtual qs::QSTATUS setMessagesFlags(qm::SubAccount* pSubAccount,
		qm::NormalFolder* pFolder, const qm::Folder::MessageHolderList& l,
		unsigned int nFlags, unsigned int nMask);
	virtual qs::QSTATUS appendMessage(qm::SubAccount* pSubAccount,
		qm::NormalFolder* pFolder, const CHAR* pszMessage, unsigned int nFlags);
	virtual qs::QSTATUS removeMessages(qm::SubAccount* pSubAccount,
		qm::NormalFolder* pFolder, const qm::Folder::MessageHolderList& l);
	virtual qs::QSTATUS copyMessages(qm::SubAccount* pSubAccount,
		const qm::Folder::MessageHolderList& l, qm::NormalFolder* pFolderFrom,
		qm::NormalFolder* pFolderTo, bool bMove);
	virtual qs::QSTATUS clearDeletedMessages(
		qm::SubAccount* pSubAccount, qm::NormalFolder* pFolder);

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
	virtual qs::QSTATUS createDriver(qm::Account* pAccount,
		qm::ProtocolDriver** ppProtocolDriver);

private:
	Pop3Factory(const Pop3Factory&);
	Pop3Factory& operator=(const Pop3Factory&);

private:
	static Pop3Factory factory__;
};

}

#endif // __POP3DRIVER_H__
