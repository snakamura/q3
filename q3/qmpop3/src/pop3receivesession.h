/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __POP3RECEIVESESSION_H__
#define __POP3RECEIVESESSION_H__

#include <qmaccount.h>
#include <qmdocument.h>
#include <qmfolder.h>
#include <qmmacro.h>
#include <qmmessage.h>
#include <qmmessageholder.h>
#include <qmsecurity.h>
#include <qmsession.h>
#include <qmsyncfilter.h>

#include <qs.h>
#include <qslog.h>

#include <vector>

#include "pop3.h"


namespace qmpop3 {

class Pop3ReceiveSession;
class Pop3ReceiveSessionFactory;
class Pop3SyncFilterCallback;
class Pop3MessageHolder;

class UIDList;


/****************************************************************************
 *
 * Pop3ReceiveSession
 *
 */

class Pop3ReceiveSession : public qm::ReceiveSession
{
public:
	enum State {
		STATE_NONE,
		STATE_HEADER,
		STATE_ALL
	};

public:
	Pop3ReceiveSession();
	virtual ~Pop3ReceiveSession();

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
							  bool bExpunge);
	virtual bool closeFolder();
	virtual bool updateMessages();
	virtual bool downloadMessages(const qm::SyncFilterSet* pSyncFilterSet);
	virtual bool applyOfflineJobs();

private:
	bool prepare();
	bool downloadReservedMessages();
	bool downloadReservedMessages(qm::NormalFolder* pFolder,
								  unsigned int* pnPos);
	std::auto_ptr<UIDList> loadUIDList() const;
	bool saveUIDList(const UIDList* pUIDList) const;
	qs::wstring_ptr getUIDListPath() const;

private:
	Pop3ReceiveSession(const Pop3ReceiveSession&);
	Pop3ReceiveSession& operator=(const Pop3ReceiveSession&);

private:
	class CallbackImpl :
		public qs::SocketCallback,
		public qm::DefaultSSLSocketCallback,
		public Pop3Callback
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
		virtual bool getUserInfo(qs::wstring_ptr* pwstrUserName,
								 qs::wstring_ptr* pwstrPassword);
		virtual void setPassword(const WCHAR* pwszPassword);
		virtual void authenticating();
		virtual void setRange(unsigned int nMin,
							  unsigned int nMax);
		virtual void setPos(unsigned int nPos);
	
	private:
		CallbackImpl(const CallbackImpl&);
		CallbackImpl& operator=(const CallbackImpl&);
	
	private:
		qm::SubAccount* pSubAccount_;
		qm::ReceiveSessionCallback* pSessionCallback_;
	};
	
	class UIDSaver
	{
	public:
		UIDSaver(Pop3ReceiveSession* pSession,
				 qs::Logger* pLogger,
				 UIDList* pList);
		~UIDSaver();
	
	private:
		Pop3ReceiveSession* pSession_;
		qs::Logger* pLogger_;
		UIDList* pUIDList_;
	};
	friend class UIDSaver;

private:
	std::auto_ptr<Pop3> pPop3_;
	std::auto_ptr<CallbackImpl> pCallback_;
	qm::Document* pDocument_;
	qm::Account* pAccount_;
	qm::SubAccount* pSubAccount_;
	qm::NormalFolder* pFolder_;
	HWND hwnd_;
	qs::Profile* pProfile_;
	qs::Logger* pLogger_;
	qm::ReceiveSessionCallback* pSessionCallback_;
	bool bReservedDownload_;
	bool bCacheAll_;
	unsigned int nStart_;
	std::auto_ptr<UIDList> pUIDList_;
	Pop3::UidList listUID_;
	Pop3::MessageSizeList listSize_;
};


/****************************************************************************
 *
 * Pop3ReceiveSessionUI
 *
 */

class Pop3ReceiveSessionUI : public qm::ReceiveSessionUI
{
public:
	Pop3ReceiveSessionUI();
	virtual ~Pop3ReceiveSessionUI();

public:
	virtual const WCHAR* getClass();
	virtual qs::wstring_ptr getDisplayName();
	virtual short getDefaultPort();
	virtual std::auto_ptr<qs::PropertyPage> createPropertyPage(qm::SubAccount* pSubAccount);

private:
	Pop3ReceiveSessionUI(const Pop3ReceiveSessionUI&);
	Pop3ReceiveSessionUI& operator=(const Pop3ReceiveSessionUI&);
};


/****************************************************************************
 *
 * Pop3ReceiveSessionFactory
 *
 */

class Pop3ReceiveSessionFactory : public qm::ReceiveSessionFactory
{
private:
	Pop3ReceiveSessionFactory();

public:
	virtual ~Pop3ReceiveSessionFactory();

protected:
	virtual std::auto_ptr<qm::ReceiveSession> createSession();
	virtual std::auto_ptr<qm::ReceiveSessionUI> createUI();

private:
	Pop3ReceiveSessionFactory(const Pop3ReceiveSessionFactory&);
	Pop3ReceiveSessionFactory& operator=(const Pop3ReceiveSessionFactory&);

private:
	static Pop3ReceiveSessionFactory factory__;
};


/****************************************************************************
 *
 * Pop3SyncFilterCallback
 *
 */

class Pop3SyncFilterCallback : public qm::SyncFilterCallback
{
public:
	Pop3SyncFilterCallback(qm::Document* pDocument,
						   qm::Account* pAccount,
						   qm::NormalFolder* pFolder,
						   qm::Message* pMessage,
						   unsigned int nSize,
						   HWND hwnd,
						   qs::Profile* pProfile,
						   qm::MacroVariableHolder* pGlobalVariable,
						   Pop3* pPop3,
						   unsigned int nMessage,
						   qs::xstring_ptr* pstrMessage,
						   Pop3ReceiveSession::State* pState,
						   unsigned int* pnGetSize);
	virtual ~Pop3SyncFilterCallback();

public:
	bool getMessage(unsigned int nFlag);

public:
	virtual const qm::NormalFolder* getFolder();
	virtual std::auto_ptr<qm::MacroContext> getMacroContext();

private:
	Pop3SyncFilterCallback(const Pop3SyncFilterCallback&);
	Pop3SyncFilterCallback& operator=(const Pop3SyncFilterCallback&);

private:
	qm::Document* pDocument_;
	qm::Account* pAccount_;
	qm::NormalFolder* pFolder_;
	qm::Message* pMessage_;
	unsigned int nSize_;
	HWND hwnd_;
	qs::Profile* pProfile_;
	qm::MacroVariableHolder* pGlobalVariable_;
	Pop3* pPop3_;
	unsigned int nMessage_;
	qs::xstring_ptr* pstrMessage_;
	Pop3ReceiveSession::State* pState_;
	unsigned int* pnGetSize_;
	std::auto_ptr<Pop3MessageHolder> pmh_;
};


/****************************************************************************
 *
 * Pop3MessageHolder
 *
 */

class Pop3MessageHolder : public qm::AbstractMessageHolder
{
public:
	Pop3MessageHolder(Pop3SyncFilterCallback* pCallback,
					  qm::NormalFolder* pFolder,
					  qm::Message* pMessage,
					  unsigned int nSize);
	virtual ~Pop3MessageHolder();

public:
	virtual qs::wstring_ptr getFrom() const;
	virtual qs::wstring_ptr getTo() const;
	virtual qs::wstring_ptr getFromTo() const;
	virtual qs::wstring_ptr getSubject() const;
	virtual void getDate(qs::Time* pTime) const;
	virtual bool getMessage(unsigned int nFlags,
							const WCHAR* pwszField,
							qm::Message* pMessage);

private:
	bool getMessage(unsigned int nFlags) const;

private:
	Pop3MessageHolder(const Pop3MessageHolder&);
	Pop3MessageHolder& operator=(const Pop3MessageHolder&);

private:
	Pop3SyncFilterCallback* pCallback_;
};


/****************************************************************************
 *
 * DeleteList
 *
 */

class DeleteList
{
public:
	typedef std::vector<std::pair<bool, qm::MessagePtr> > List;

public:
	DeleteList();
	~DeleteList();

public:
	const List getList() const;
	void add(size_t n);
	void add(size_t n,
			 const qm::MessagePtr& ptr);

private:
	void resize(size_t n);

private:
	DeleteList(const DeleteList&);
	DeleteList& operator=(const DeleteList&);

private:
	List list_;
};

}

#endif // __POP3RECEIVESESSION_H__
