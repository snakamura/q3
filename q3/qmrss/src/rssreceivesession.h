/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
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
class FeedManager;
class Item;


/****************************************************************************
 *
 * RssReceiveSession
 *
 */

class RssReceiveSession : public qm::ReceiveSession
{
private:
	enum Content {
		CONTENT_NONE,
		CONTENT_CONTENTENCODED,
		CONTENT_DESCRIPTION
	};

public:
	explicit RssReceiveSession(FeedManager* pFeedManager);
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
							  unsigned int nFlags);
	virtual bool closeFolder();
	virtual bool updateMessages();
	virtual bool downloadMessages(const qm::SyncFilterSet* pSyncFilterSet);
	virtual bool applyOfflineJobs();

private:
	void clearFeeds();
	void reportError(UINT nId,
					 HttpMethod* pMethod);

private:
	static bool createItemMessage(const Channel* pChannel,
								  const Item* pItem,
								  const qs::Time& timePubDate,
								  const qs::Part* pHeader,
								  const unsigned char* pBody,
								  size_t nBodyLen,
								  Content content,
								  qm::Message* pMessage);
	static bool getInternetProxySetting(qs::wstring_ptr* pwstrProxyHost,
										unsigned short* pnProxyPort);

private:
	RssReceiveSession(const RssReceiveSession&);
	RssReceiveSession& operator=(const RssReceiveSession&);

private:
	class CallbackImpl :
		public qs::SocketCallback,
		public qm::AbstractSSLSocketCallback,
		public HttpCallback
	{
	public:
		CallbackImpl(qm::SubAccount* pSubAccount,
					 const WCHAR* pwszHost,
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
	
	protected:
		virtual unsigned int getOption();
		virtual const WCHAR* getHost();
	
	private:
		CallbackImpl(const CallbackImpl&);
		CallbackImpl& operator=(const CallbackImpl&);
	
	private:
		qm::SubAccount* pSubAccount_;
		qs::wstring_ptr wstrHost_;
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
	FeedManager* pFeedManager_;
	FeedList* pFeedList_;
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
	virtual short getDefaultPort(bool bSecure);
	virtual bool isSupported(Support support);
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
	std::auto_ptr<FeedManager> pFeedManager_;

private:
	static RssReceiveSessionFactory factory__;
};

}

#endif // __RSSRECEIVESESSION_H__
