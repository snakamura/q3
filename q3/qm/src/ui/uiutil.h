/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __UIUTIL_H__
#define __UIUTIL_H__

#include <qm.h>
#include <qmmessageoperation.h>
#include <qmsession.h>

#include <qs.h>
#include <qsprofile.h>
#include <qsstring.h>

#include "../model/editmessage.h"


namespace qm {

class UIUtil;
class ProgressDialogInit;
template<class Callback> class ProgressDialogMessageOperationCallbackBase;
class DefaultPasswordManagerCallback;
class DefaultPasswordCallback;

class Account;
class Folder;
class Message;
class MessageHolder;
class MessageWindow;
class PasswordManager;
class ProgressDialog;
class SecurityModel;
class StatusBar;
class TempFileCleaner;


/****************************************************************************
 *
 * UIUtil
 *
 */

class UIUtil
{
public:
	typedef std::vector<qs::WSTRING> EncodingList;
	typedef std::vector<qs::WSTRING> PathList;

public:
	static int loadWindowPlacement(qs::Profile* pProfile,
								   const WCHAR* pwszSection,
								   CREATESTRUCT* pCreateStruct);
	static void saveWindowPlacement(HWND hwnd,
									qs::Profile* pProfile,
									const WCHAR* pwszSection);
	static void loadEncodings(qs::Profile* pProfile,
							  EncodingList* pList);
	
	static qs::wstring_ptr formatMenu(const WCHAR* pwszText);
	static bool openURL(HWND hwnd,
						const WCHAR* pwszURL);
	
	static int getFolderImage(Folder* pFolder,
							  bool bSelected);
	
	static void updateStatusBar(MessageWindow* pMessageWindow,
								StatusBar* pStatusBar,
								int nOffset,
								MessageHolder* pmh,
								Message& msg,
								const qs::ContentTypeParser* pContentType);
	
	static qs::wstring_ptr writeTemporaryFile(const WCHAR* pwszValue,
											  const WCHAR* pwszPrefix,
											  const WCHAR* pwszExtension,
											  TempFileCleaner* pTempFileCleaner);
	
	static void getAttachmentInfo(const EditMessage::Attachment& attachment,
								  qs::wstring_ptr* pwstrName,
								  int* pnSysIconIndex);
	
	static qs::wstring_ptr convertLFtoCRLF(const WCHAR* pwsz);
	static qs::wstring_ptr convertCRLFtoLF(const WCHAR* pwsz);
	
	static qs::wstring_ptr formatAccount(Account* pAccount);
	static qs::wstring_ptr formatFolder(Folder* pFolder);
	static std::pair<Account*, Folder*> getAccountOrFolder(Document* pDocument,
														   const WCHAR* pwsz);
	
	static bool addMessageToClipboard(HWND hwnd,
									  MessageHolder* pmh);
	static MessagePtr getMessageFromClipboard(HWND hwnd,
											  Document* pDocument);
	
	static bool hasFilesOrURIs(IDataObject* pDataObject);
	static void getFilesOrURIs(IDataObject* pDataObject,
							   PathList* pList);
};


/****************************************************************************
 *
 * ProgressDialogInit
 *
 */

class ProgressDialogInit
{
public:
	ProgressDialogInit(ProgressDialog* pDialog,
					   HWND hwnd);
	ProgressDialogInit(ProgressDialog* pDialog,
					   HWND hwnd,
					   UINT nTitle,
					   UINT nMessage,
					   unsigned int nMin,
					   unsigned int nMax,
					   unsigned int nPos);
	~ProgressDialogInit();

private:
	ProgressDialogInit(const ProgressDialogInit&);
	ProgressDialogInit& operator=(const ProgressDialogInit&);

private:
	ProgressDialog* pDialog_;
};


/****************************************************************************
 *
 * ProgressDialogMessageOperationCallbackBase
 *
 */

template<class Callback>
class ProgressDialogMessageOperationCallbackBase : public Callback
{
public:
	ProgressDialogMessageOperationCallbackBase(HWND hwnd,
											   UINT nTitle,
											   UINT nMessage);
	virtual ~ProgressDialogMessageOperationCallbackBase();

public:
	virtual bool isCanceled();
	virtual void setCount(unsigned int nCount);
	virtual void step(unsigned int nStep);
	virtual void show();

protected:
	ProgressDialog* getDialog() const;

private:
	ProgressDialogMessageOperationCallbackBase(ProgressDialogMessageOperationCallbackBase&);
	ProgressDialogMessageOperationCallbackBase& operator=(ProgressDialogMessageOperationCallbackBase&);

private:
	HWND hwnd_;
	UINT nTitle_;
	UINT nMessage_;
	std::auto_ptr<ProgressDialog> pDialog_;
	unsigned int nCount_;
	unsigned int nPos_;
};

typedef ProgressDialogMessageOperationCallbackBase<MessageOperationCallback> ProgressDialogMessageOperationCallback;


/****************************************************************************
 *
 * DefaultPasswordManagerCallback
 *
 */

class DefaultPasswordManagerCallback : public PasswordManagerCallback
{
public:
	DefaultPasswordManagerCallback();
	virtual ~DefaultPasswordManagerCallback();

public:
	virtual PasswordState getPassword(const PasswordCondition& condition,
									  qs::wstring_ptr* pwstrPassword);

private:
	DefaultPasswordManagerCallback(const DefaultPasswordManagerCallback&);
	DefaultPasswordManagerCallback& operator=(const DefaultPasswordManagerCallback&);
};


/****************************************************************************
 *
 * DefaultPasswordCallback
 *
 */

class DefaultPasswordCallback : public PasswordCallback
{
public:
	explicit DefaultPasswordCallback(PasswordManager* pPasswordManager);
	virtual ~DefaultPasswordCallback();

public:
	virtual PasswordState getPassword(SubAccount* pSubAccount,
									  Account::Host host,
									  qs::wstring_ptr* pwstrPassword);
	virtual void setPassword(SubAccount* pSubAccount,
							 Account::Host host,
							 const WCHAR* pwszPassword,
							 bool bPermanent);

private:
	DefaultPasswordCallback(const DefaultPasswordCallback&);
	DefaultPasswordCallback& operator=(const DefaultPasswordCallback&);

private:
	PasswordManager* pPasswordManager_;
};

}

#include "uiutil.inl"

#endif // __UIUTIL_H__
