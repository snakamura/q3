/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QMSESSION_H__
#define __QMSESSION_H__

#include <qm.h>
#include <qmaccount.h>
#include <qmfolder.h>

#include <qs.h>
#include <qsdialog.h>
#include <qslog.h>
#include <qsprofile.h>
#include <qsssl.h>
#include <qsstring.h>

#include <memory>
#include <utility>
#include <vector>


namespace qm {

class SessionCallback;
class ReceiveSession;
class ReceiveSessionCallback;
class ReceiveSessionFactory;
class SendSession;
class SendSessionCallback;
class SendSessionFactory;
class SessionErrorInfo;
class DefaultSSLSocketCallback;

class Account;
class Document;
class SubAccount;
class NormalFolder;
class SyncFilterSet;


/****************************************************************************
 *
 * SessionCallback
 *
 */

class QMEXPORTCLASS SessionCallback
{
public:
	virtual ~SessionCallback();

public:
	virtual bool isCanceled(bool bForce) = 0;
	virtual void setPos(unsigned int n) = 0;
	virtual void setRange(unsigned int nMin,
						  unsigned int nMax) = 0;
	virtual void setSubPos(unsigned int n) = 0;
	virtual void setSubRange(unsigned int nMin,
							 unsigned int nMax) = 0;
	virtual void setMessage(const WCHAR* pwszMessage) = 0;
	virtual void addError(const SessionErrorInfo& info) = 0;
};


/****************************************************************************
 *
 * ReceiveSession
 *
 */

class QMEXPORTCLASS ReceiveSession
{
public:
	virtual ~ReceiveSession();

public:
	virtual bool init(Document* pDocument,
					  Account* pAccount,
					  SubAccount* pSubAccount,
					  HWND hwnd,
					  qs::Profile* pProfile,
					  qs::Logger* pLogger,
					  ReceiveSessionCallback* pCallback) = 0;
	virtual void term() = 0;
	virtual bool connect() = 0;
	virtual void disconnect() = 0;
	virtual bool isConnected() = 0;
	virtual bool selectFolder(NormalFolder* pFolder,
							  bool bExpunge) = 0;
	virtual bool closeFolder() = 0;
	virtual bool updateMessages() = 0;
	virtual bool downloadMessages(const SyncFilterSet* pSyncFilterSet) = 0;
	virtual bool applyOfflineJobs() = 0;
};


/****************************************************************************
 *
 * ReceiveSessionCallback
 *
 */

class QMEXPORTCLASS ReceiveSessionCallback : public SessionCallback
{
public:
	virtual ~ReceiveSessionCallback();

public:
	virtual void notifyNewMessage(MessageHolder* pmh) = 0;
};


/****************************************************************************
 *
 * ReceiveSessionUI
 *
 */

class QMEXPORTCLASS ReceiveSessionUI
{
public:
	virtual ~ReceiveSessionUI();

public:
	virtual const WCHAR* getClass() = 0;
	virtual qs::wstring_ptr getDisplayName() = 0;
	virtual short getDefaultPort() = 0;
	virtual std::auto_ptr<qs::PropertyPage> createPropertyPage(SubAccount* pSubAccount) = 0;
};


/****************************************************************************
 *
 * ReceiveSessionFactory
 *
 */

class QMEXPORTCLASS ReceiveSessionFactory
{
public:
	typedef std::vector<qs::WSTRING> NameList;

protected:
	ReceiveSessionFactory();

public:
	virtual ~ReceiveSessionFactory();

public:
	static std::auto_ptr<ReceiveSession> getSession(const WCHAR* pwszName);
	static std::auto_ptr<ReceiveSessionUI> getUI(const WCHAR* pwszName);
	static void getNames(NameList* pList);
	static void getClasses(NameList* pList);

protected:
	virtual std::auto_ptr<ReceiveSession> createSession() = 0;
	virtual std::auto_ptr<ReceiveSessionUI> createUI() = 0;

protected:
	static void registerFactory(const WCHAR* pwszName,
								ReceiveSessionFactory* pFactory);
	static void unregisterFactory(const WCHAR* pwszName);

private:
	ReceiveSessionFactory(const ReceiveSessionFactory&);
	ReceiveSessionFactory& operator=(const ReceiveSessionFactory&);
};


/****************************************************************************
 *
 * SendSession
 *
 */

class QMEXPORTCLASS SendSession
{
public:
	virtual ~SendSession();

public:
	virtual bool init(Document* pDocument,
					  Account* pAccount,
					  SubAccount* pSubAccount,
					  qs::Profile* pProfile,
					  qs::Logger* pLogger,
					  SendSessionCallback* pCallback) = 0;
	virtual void term() = 0;
	virtual bool connect() = 0;
	virtual void disconnect() = 0;
	virtual bool sendMessage(Message* pMessage) = 0;
};


/****************************************************************************
 *
 * SendSessionCallback
 *
 */

class QMEXPORTCLASS SendSessionCallback : public SessionCallback
{
public:
	virtual ~SendSessionCallback();
};


/****************************************************************************
 *
 * SendSessionUI
 *
 */

class QMEXPORTCLASS SendSessionUI
{
public:
	virtual ~SendSessionUI();

public:
	virtual const WCHAR* getClass() = 0;
	virtual qs::wstring_ptr getDisplayName() = 0;
	virtual short getDefaultPort() = 0;
	virtual std::auto_ptr<qs::PropertyPage> createPropertyPage(SubAccount* pSubAccount) = 0;
};


/****************************************************************************
 *
 * SendSessionFactory
 *
 */

class QMEXPORTCLASS SendSessionFactory
{
public:
	typedef std::vector<qs::WSTRING> NameList;

protected:
	SendSessionFactory();

public:
	virtual ~SendSessionFactory();

public:
	static std::auto_ptr<SendSession> getSession(const WCHAR* pwszName);
	static std::auto_ptr<SendSessionUI> getUI(const WCHAR* pwszName);
	static void getNames(NameList* pList);

protected:
	virtual std::auto_ptr<SendSession> createSession() = 0;
	virtual std::auto_ptr<SendSessionUI> createUI() = 0;

protected:
	static void registerFactory(const WCHAR* pwszName,
								SendSessionFactory* pFactory);
	static void unregisterFactory(const WCHAR* pwszName);

private:
	SendSessionFactory(const SendSessionFactory&);
	SendSessionFactory& operator=(const SendSessionFactory&);
};


/****************************************************************************
 *
 * SessionErrorInfo
 *
 */

class QMEXPORTCLASS SessionErrorInfo
{
public:
	SessionErrorInfo(Account* pAccount,
					 SubAccount* pSubAccount,
					 NormalFolder* pFolder,
					 const WCHAR* pwszMessage,
					 unsigned int nCode,
					 const WCHAR* pwszDescriptions[],
					 size_t nDescriptionCount);
	~SessionErrorInfo();

public:
	Account* getAccount() const;
	SubAccount* getSubAccount() const;
	NormalFolder* getFolder() const;
	const WCHAR* getMessage() const;
	unsigned int getCode() const;
	const WCHAR* getDescription(size_t n) const;
	size_t getDescriptionCount() const;

private:
	SessionErrorInfo(const SessionErrorInfo&);
	SessionErrorInfo& operator=(const SessionErrorInfo&);

private:
	Account* pAccount_;
	SubAccount* pSubAccount_;
	NormalFolder* pFolder_;
	const WCHAR* pwszMessage_;
	unsigned int nCode_;
	const WCHAR** ppwszDescription_;
	size_t nDescriptionCount_;
};


/****************************************************************************
 *
 * DefaultSSLSocketCallback
 *
 */

class QMEXPORTCLASS DefaultSSLSocketCallback : public qs::SSLSocketCallback
{
public:
	DefaultSSLSocketCallback(SubAccount* pSubAccount,
							 Account::Host host,
							 const Security* pSecurity);
	virtual ~DefaultSSLSocketCallback();

public:
	virtual const qs::Store* getCertStore();
	virtual bool checkCertificate(const qs::Certificate& cert,
								  bool bVerified);

private:
	DefaultSSLSocketCallback(const DefaultSSLSocketCallback&);
	DefaultSSLSocketCallback& operator=(const DefaultSSLSocketCallback&);

private:
	SubAccount* pSubAccount_;
	Account::Host host_;
	const Security* pSecurity_;
};

}

#endif // __QMSESSION_H__
