/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
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
			   const qm::Security* pSecurity);
	virtual ~NntpDriver();

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
	bool prepareSession(qm::SubAccount* pSubAccount,
						qm::NormalFolder* pFolder);
	void clearSession();
	bool isForceDisconnect() const;

private:
	NntpDriver(const NntpDriver&);
	NntpDriver& operator=(const NntpDriver&);

private:
	class CallbackImpl : public AbstractCallback
	{
	public:
		CallbackImpl(qm::SubAccount* pSubAccount,
					 const qm::Security* pSecurity);
		virtual ~CallbackImpl();
	
	public:
		virtual bool isCanceled(bool bForce) const;
		virtual void initialize();
		virtual void lookup();
		virtual void connecting();
		virtual void connected();
	
	public:
		virtual void authenticating();
		virtual void setRange(unsigned int nMin,
							  unsigned int nMax);
		virtual void setPos(unsigned int nPos);
	
	private:
		CallbackImpl(const CallbackImpl&);
		CallbackImpl& operator=(const CallbackImpl&);
	};

private:
	qm::Account* pAccount_;
	const qm::Security* pSecurity_;
	qm::SubAccount* pSubAccount_;
	std::auto_ptr<Nntp> pNntp_;
	std::auto_ptr<CallbackImpl> pCallback_;
	std::auto_ptr<qs::Logger> pLogger_;
	bool bOffline_;
	unsigned int nForceDisconnect_;
	unsigned int nLastUsedTime_;
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
														   const qm::Security* pSecurity);

private:
	NntpFactory(const NntpFactory&);
	NntpFactory& operator=(const NntpFactory&);

private:
	static NntpFactory factory__;
};

}

#endif // __NNTPDRIVER_H__
