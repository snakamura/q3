/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __NNTPRECEIVESESSION_H__
#define __NNTPRECEIVESESSION_H__

#include <qmmacro.h>
#include <qmsession.h>
#include <qmsyncfilter.h>

#include <qssocket.h>

#include "nntp.h"
#include "util.h"


namespace qmnntp {

class NntpReceiveSession;
class NntpReceiveSessionUI;
class NntpReceiveSessionFactory;
class NntpSyncFilterCallback;
class NntpMessageHolder;

class LastIdList;


/****************************************************************************
 *
 * NntpReceiveSession
 *
 */

class NntpReceiveSession : public qm::ReceiveSession
{
public:
	enum State {
		STATE_NONE,
		STATE_HEADER,
		STATE_ALL
	};

public:
	NntpReceiveSession();
	virtual ~NntpReceiveSession();

public:
	virtual bool init(qm::Document* pDocument,
					  qm::Account* pAccount,
					  qm::SubAccount* pSubAccount,
					  HWND hwnd,
					  qs::Profile* pProfile,
					  qs::Logger* pLogger,
					  qm::ReceiveSessionCallback* pCallback);
	virtual void term();
	virtual bool connect();
	virtual void disconnect();
	virtual bool isConnected();
	virtual bool selectFolder(qm::NormalFolder* pFolder,
							  unsigned int nFlags);
	virtual bool closeFolder();
	virtual bool updateMessages();
	virtual bool downloadMessages(const qm::SyncFilterSet* pSyncFilterSet);
	virtual bool applyOfflineJobs();

private:
	void clearLastIds();

private:
	bool downloadReservedMessages();
	bool downloadReservedMessages(qm::NormalFolder* pFolder,
								  unsigned int* pnPos);

private:
	NntpReceiveSession(const NntpReceiveSession&);
	NntpReceiveSession& operator=(const NntpReceiveSession&);

private:
	class CallbackImpl : public AbstractCallback
	{
	public:
		CallbackImpl(qm::SubAccount* pSubAccount,
					 const qm::Security* pSecurity,
					 qm::ReceiveSessionCallback* pSessionCallback);
		virtual ~CallbackImpl();
	
	public:
		void setMessage(UINT nId);
	
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
	
	private:
		qm::ReceiveSessionCallback* pSessionCallback_;
	};

private:
	std::auto_ptr<Nntp> pNntp_;
	std::auto_ptr<CallbackImpl> pCallback_;
	qm::Document* pDocument_;
	qm::Account* pAccount_;
	qm::SubAccount* pSubAccount_;
	qm::NormalFolder* pFolder_;
	HWND hwnd_;
	qs::Profile* pProfile_;
	qs::Logger* pLogger_;
	qm::ReceiveSessionCallback* pSessionCallback_;
	std::auto_ptr<LastIdList> pLastIdList_;
};


/****************************************************************************
 *
 * NntpReceiveSessionUI
 *
 */

class NntpReceiveSessionUI : public qm::ReceiveSessionUI
{
public:
	NntpReceiveSessionUI();
	virtual ~NntpReceiveSessionUI();

public:
	virtual const WCHAR* getClass();
	virtual qs::wstring_ptr getDisplayName();
	virtual short getDefaultPort();
	virtual std::auto_ptr<qs::PropertyPage> createPropertyPage(qm::SubAccount* pSubAccount);

private:
	NntpReceiveSessionUI(const NntpReceiveSessionUI&);
	NntpReceiveSessionUI& operator=(const NntpReceiveSessionUI&);
};


/****************************************************************************
 *
 * NntpReceiveSessionFactory
 *
 */

class NntpReceiveSessionFactory : public qm::ReceiveSessionFactory
{
private:
	NntpReceiveSessionFactory();

public:
	virtual ~NntpReceiveSessionFactory();

protected:
	virtual std::auto_ptr<qm::ReceiveSession> createSession();
	virtual std::auto_ptr<qm::ReceiveSessionUI> createUI();

private:
	NntpReceiveSessionFactory(const NntpReceiveSessionFactory&);
	NntpReceiveSessionFactory& operator=(const NntpReceiveSessionFactory&);

private:
	static NntpReceiveSessionFactory factory__;
};


/****************************************************************************
 *
 * NntpSyncFilterCallback
 *
 */

class NntpSyncFilterCallback : public qm::SyncFilterCallback
{
public:
	NntpSyncFilterCallback(qm::Document* pDocument,
						   qm::Account* pAccount,
						   qm::NormalFolder* pFolder,
						   qm::Message* pMessage,
						   unsigned int nSize,
						   HWND hwnd,
						   qs::Profile* pProfile,
						   qm::MacroVariableHolder* pGlobalVariable,
						   Nntp* pNntp,
						   unsigned int nMessage,
						   qs::xstring_ptr* pstrMessage,
						   NntpReceiveSession::State* pState);
	virtual ~NntpSyncFilterCallback();

public:
	bool getMessage(unsigned int nFlag);

public:
	virtual const qm::NormalFolder* getFolder();
	virtual std::auto_ptr<qm::MacroContext> getMacroContext();

private:
	NntpSyncFilterCallback(const NntpSyncFilterCallback&);
	NntpSyncFilterCallback& operator=(const NntpSyncFilterCallback&);

private:
	qm::Document* pDocument_;
	qm::Account* pAccount_;
	qm::NormalFolder* pFolder_;
	qm::Message* pMessage_;
	unsigned int nSize_;
	HWND hwnd_;
	qs::Profile* pProfile_;
	qm::MacroVariableHolder* pGlobalVariable_;
	Nntp* pNntp_;
	unsigned int nMessage_;
	qs::xstring_ptr* pstrMessage_;
	NntpReceiveSession::State* pState_;
	unsigned int* pnGetSize_;
	std::auto_ptr<NntpMessageHolder> pmh_;
};


/****************************************************************************
 *
 * NntpMessageHolder
 *
 */

class NntpMessageHolder : public qm::AbstractMessageHolder
{
public:
	NntpMessageHolder(NntpSyncFilterCallback* pCallback,
					  qm::NormalFolder* pFolder,
					  qm::Message* pMessage,
					  unsigned int nSize);
	virtual ~NntpMessageHolder();

public:
	virtual bool getMessage(unsigned int nFlags,
							const WCHAR* pwszField,
							qm::Message* pMessage);

private:
	NntpMessageHolder(const NntpMessageHolder&);
	NntpMessageHolder& operator=(const NntpMessageHolder&);

private:
	NntpSyncFilterCallback* pCallback_;
};

}

#endif // __NNTPRECEIVESESSION_H__
