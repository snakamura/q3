/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QMSESSION_H__
#define __QMSESSION_H__

#include <qm.h>
#include <qmfolder.h>

#include <qs.h>
#include <qsdialog.h>
#include <qslog.h>
#include <qsprofile.h>
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
	virtual qs::QSTATUS setPos(unsigned int n) = 0;
	virtual qs::QSTATUS setRange(unsigned int nMin, unsigned int nMax) = 0;
	virtual qs::QSTATUS setSubPos(unsigned int n) = 0;
	virtual qs::QSTATUS setSubRange(unsigned int nMin, unsigned int nMax) = 0;
	virtual qs::QSTATUS setMessage(const WCHAR* pwszMessage) = 0;
	virtual qs::QSTATUS addError(const SessionErrorInfo& info) = 0;
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
	virtual qs::QSTATUS init(Document* pDocument, Account* pAccount,
		SubAccount* pSubAccount, HWND hwnd, qs::Profile* pProfile,
		qs::Logger* pLogger, ReceiveSessionCallback* pCallback) = 0;
	virtual qs::QSTATUS connect() = 0;
	virtual qs::QSTATUS disconnect() = 0;
	virtual qs::QSTATUS selectFolder(NormalFolder* pFolder) = 0;
	virtual qs::QSTATUS closeFolder() = 0;
	virtual qs::QSTATUS updateMessages() = 0;
	virtual qs::QSTATUS downloadMessages(
		const SyncFilterSet* pSyncFilterSet) = 0;
	virtual qs::QSTATUS applyOfflineJobs() = 0;
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
	virtual qs::QSTATUS notifyNewMessage() = 0;
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
	virtual qs::QSTATUS createPropertyPage(
		SubAccount* pSubAccount, qs::PropertyPage** ppPage) = 0;
	virtual qs::QSTATUS getDisplayName(qs::WSTRING* pwstrName) = 0;
	virtual short getDefaultPort() = 0;
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
	static qs::QSTATUS getSession(const WCHAR* pwszName,
		ReceiveSession** ppReceiveSession);
	static qs::QSTATUS getSession(const WCHAR* pwszName,
		std::auto_ptr<ReceiveSession>* papReceiveSession);
	static qs::QSTATUS getUI(const WCHAR* pwszName,
		ReceiveSessionUI** ppUI);
	static qs::QSTATUS getUI(const WCHAR* pwszName,
		std::auto_ptr<ReceiveSessionUI>* papUI);
	static qs::QSTATUS getNames(NameList* pList);

protected:
	virtual qs::QSTATUS createSession(ReceiveSession** ppReceiveSession) = 0;
	virtual qs::QSTATUS createUI(ReceiveSessionUI** ppUI) = 0;

protected:
	static qs::QSTATUS regist(const WCHAR* pwszName,
		ReceiveSessionFactory* pFactory);
	static qs::QSTATUS unregist(const WCHAR* pwszName);

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
	virtual qs::QSTATUS init(Account* pAccount,
		SubAccount* pSubAccount, qs::Profile* pProfile,
		qs::Logger* pLogger, SendSessionCallback* pCallback) = 0;
	virtual qs::QSTATUS connect() = 0;
	virtual qs::QSTATUS disconnect() = 0;
	virtual qs::QSTATUS sendMessage(Message* pMessage) = 0;
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
	virtual qs::QSTATUS createPropertyPage(
		SubAccount* pSubAccount, qs::PropertyPage** ppPage) = 0;
	virtual qs::QSTATUS getDisplayName(qs::WSTRING* pwstrName) = 0;
	virtual short getDefaultPort() = 0;
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
	static qs::QSTATUS getSession(const WCHAR* pwszName,
		SendSession** ppSendSession);
	static qs::QSTATUS getSession(const WCHAR* pwszName,
		std::auto_ptr<SendSession>* papSendSession);
	static qs::QSTATUS getUI(const WCHAR* pwszName,
		SendSessionUI** ppUI);
	static qs::QSTATUS getUI(const WCHAR* pwszName,
		std::auto_ptr<SendSessionUI>* papUI);
	static qs::QSTATUS getNames(NameList* pList);

protected:
	virtual qs::QSTATUS createSession(SendSession** ppSendSession) = 0;
	virtual qs::QSTATUS createUI(SendSessionUI** ppUI) = 0;

protected:
	static qs::QSTATUS regist(const WCHAR* pwszName,
		SendSessionFactory* pFactory);
	static qs::QSTATUS unregist(const WCHAR* pwszName);

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
	SessionErrorInfo(Account* pAccount, SubAccount* pSubAccount,
		NormalFolder* pFolder, const WCHAR* pwszMessage, unsigned int nCode,
		const WCHAR* pwszDescriptions[], size_t nDescriptionCount);
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


}

#endif // __QMSESSION_H__
