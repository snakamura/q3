/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __RSSRECEIVESESSION_H__
#define __RSSRECEIVESESSION_H__

#include <qmsecurity.h>
#include <qmsession.h>

#include "http.h"


namespace qmrss {

class RssReceiveSession;
class RssReceiveSessionUI;
class RssReceiveSessionFactory;

class FeedList;
class Item;


/****************************************************************************
 *
 * RssReceiveSession
 *
 */

class RssReceiveSession : public qm::ReceiveSession
{
public:
	RssReceiveSession();
	virtual ~RssReceiveSession();

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
	void clearFeeds();

private:
	static bool createItemMessage(const Item* pItem,
								  const qs::Time& timePubDate,
								  const qs::Part* pHeader,
								  const unsigned char* pBody,
								  size_t nBodyLen,
								  qm::Message* pMessage);

private:
	RssReceiveSession(const RssReceiveSession&);
	RssReceiveSession& operator=(const RssReceiveSession&);

private:
	class CallbackImpl :
		public qs::SocketCallback,
		public qm::DefaultSSLSocketCallback,
		public HttpCallback
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
	
	private:
		CallbackImpl(const CallbackImpl&);
		CallbackImpl& operator=(const CallbackImpl&);
	
	private:
		qm::SubAccount* pSubAccount_;
		qm::ReceiveSessionCallback* pSessionCallback_;
	};

private:
	qm::Document* pDocument_;
	qm::Account* pAccount_;
	qm::SubAccount* pSubAccount_;
	qm::NormalFolder* pFolder_;
	HWND hwnd_;
	qs::Profile* pProfile_;
	qs::Logger* pLogger_;
	qm::ReceiveSessionCallback* pSessionCallback_;
	std::auto_ptr<FeedList> pFeedList_;
};


/****************************************************************************
 *
 * RssReceiveSessionUI
 *
 */

class RssReceiveSessionUI : public qm::ReceiveSessionUI
{
public:
	RssReceiveSessionUI();
	virtual ~RssReceiveSessionUI();

public:
	virtual const WCHAR* getClass();
	virtual qs::wstring_ptr getDisplayName();
	virtual short getDefaultPort();
	virtual std::auto_ptr<qs::PropertyPage> createPropertyPage(qm::SubAccount* pSubAccount);

private:
	RssReceiveSessionUI(const RssReceiveSessionUI&);
	RssReceiveSessionUI& operator=(const RssReceiveSessionUI&);
};


/****************************************************************************
 *
 * RssReceiveSessionFactory
 *
 */

class RssReceiveSessionFactory : public qm::ReceiveSessionFactory
{
private:
	RssReceiveSessionFactory();

public:
	virtual ~RssReceiveSessionFactory();

protected:
	virtual std::auto_ptr<qm::ReceiveSession> createSession();
	virtual std::auto_ptr<qm::ReceiveSessionUI> createUI();

private:
	RssReceiveSessionFactory(const RssReceiveSessionFactory&);
	RssReceiveSessionFactory& operator=(const RssReceiveSessionFactory&);

private:
	static RssReceiveSessionFactory factory__;
};

}

#endif // __RSSRECEIVESESSION_H__
