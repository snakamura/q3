/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
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
		const qm::Security* pSecurity, qs::QSTATUS* pstatus);
	virtual ~NntpDriver();

public:
	virtual bool isSupport(qm::Account::Support support);
	virtual qs::QSTATUS setOffline(bool bOffline);
	virtual qs::QSTATUS setForceOnline(bool bOnline);
	virtual qs::QSTATUS save();
	
	virtual qs::QSTATUS createFolder(qm::SubAccount* pSubAccount,
		const WCHAR* pwszName, qm::Folder* pParent,
		qm::NormalFolder** ppFolder);
	virtual qs::QSTATUS removeFolder(qm::SubAccount* pSubAccount,
		qm::NormalFolder* pFolder);
	virtual qs::QSTATUS renameFolder(qm::SubAccount* pSubAccount,
		qm::NormalFolder* pFolder, const WCHAR* pwszName);
	virtual qs::QSTATUS createDefaultFolders(qm::Account::FolderList* pList);
	virtual qs::QSTATUS getRemoteFolders(
		qm::SubAccount* pSubAccount, RemoteFolderList* pList);
	
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
	qs::QSTATUS prepareSession(qm::SubAccount* pSubAccount,
		qm::NormalFolder* pFolder);
	qs::QSTATUS clearSession();

private:
	NntpDriver(const NntpDriver&);
	NntpDriver& operator=(const NntpDriver&);

private:
	class CallbackImpl : public AbstractCallback
	{
	public:
		CallbackImpl(qm::SubAccount* pSubAccount,
			const qm::Security* pSecurity, qs::QSTATUS* pstatus);
		virtual ~CallbackImpl();
	
	public:
		virtual bool isCanceled(bool bForce) const;
		virtual qs::QSTATUS initialize();
		virtual qs::QSTATUS lookup();
		virtual qs::QSTATUS connecting();
		virtual qs::QSTATUS connected();
	
	public:
		virtual qs::QSTATUS authenticating();
		virtual qs::QSTATUS setRange(unsigned int nMin, unsigned int nMax);
		virtual qs::QSTATUS setPos(unsigned int nPos);
	
	private:
		CallbackImpl(const CallbackImpl&);
		CallbackImpl& operator=(const CallbackImpl&);
	};

private:
	qm::Account* pAccount_;
	const qm::Security* pSecurity_;
	qm::SubAccount* pSubAccount_;
	Nntp* pNntp_;
	CallbackImpl* pCallback_;
	qs::Logger* pLogger_;
	bool bOffline_;
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
	virtual qs::QSTATUS createDriver(qm::Account* pAccount,
		const qm::Security* pSecurity, qm::ProtocolDriver** ppProtocolDriver);

private:
	NntpFactory(const NntpFactory&);
	NntpFactory& operator=(const NntpFactory&);

private:
	static NntpFactory factory__;
};

}

#endif // __NNTPDRIVER_H__
