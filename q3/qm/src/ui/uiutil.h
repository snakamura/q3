/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
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
class EncodingModel;
class Folder;
class Message;
class MessageHolder;
class MessageWindow;
class PasswordManager;
class ProgressDialog;
class SecurityModel;
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

public:
	static int loadWindowPlacement(qs::Profile* pProfile,
								   const WCHAR* pwszSection,
								   CREATESTRUCT* pCreateStruct);
	static void saveWindowPlacement(HWND hwnd,
									qs::Profile* pProfile,
									const WCHAR* pwszSection);
	static void loadEncodings(qs::Profile* pProfile,
							  EncodingList* pList);
	static void parseEncodings(const WCHAR* pwszEncodings,
							   EncodingList* pList);
	
	static qs::wstring_ptr formatMenu(const WCHAR* pwszText);
	static bool openURL(const WCHAR* pwszURL,
						qs::Profile* pProfile,
						HWND hwnd);
	
	static int getFolderImage(Folder* pFolder,
							  bool bSelected);
	
	static qs::wstring_ptr writeTemporaryFile(const WCHAR* pwszValue,
											  const WCHAR* pwszPrefix,
											  const WCHAR* pwszExtension,
											  TempFileCleaner* pTempFileCleaner);
	
	static void getAttachmentInfo(const EditMessage::Attachment& attachment,
								  qs::wstring_ptr* pwstrName,
								  int* pnSysIconIndex);
	
	static bool addMessageToClipboard(HWND hwnd,
									  MessageHolder* pmh);
	static MessagePtr getMessageFromClipboard(HWND hwnd,
											  AccountManager* pAccountManager);
	
	static unsigned int getPreferredWidth(HWND hwnd);
};


/****************************************************************************
 *
 * DialogUtil
 *
 */

class DialogUtil
{
public:
	struct BoolProperty
	{
		const WCHAR* pwszKey_;
		UINT nId_;
		bool bDefault_;
	};
	
	struct IntProperty
	{
		const WCHAR* pwszKey_;
		UINT nId_;
		int nDefault_;
	};

public:
	static void loadBoolProperties(qs::Dialog* pDialog,
								   qs::Profile* pProfile,
								   const WCHAR* pwszSection,
								   const BoolProperty* pProperties,
								   size_t nCount);
	static void saveBoolProperties(qs::Dialog* pDialog,
								   qs::Profile* pProfile,
								   const WCHAR* pwszSection,
								   const BoolProperty* pProperties,
								   size_t nCount);
	static void loadIntProperties(qs::Dialog* pDialog,
								  qs::Profile* pProfile,
								  const WCHAR* pwszSection,
								  const IntProperty* pProperties,
								  size_t nCount);
	static void saveIntProperties(qs::Dialog* pDialog,
								  qs::Profile* pProfile,
								  const WCHAR* pwszSection,
								  const IntProperty* pProperties,
								  size_t nCount);
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
					   HWND hwnd,
					   UINT nTitle);
	ProgressDialogInit(ProgressDialog* pDialog,
					   HWND hwnd,
					   UINT nTitle,
					   UINT nMessage,
					   size_t nMin,
					   size_t nMax,
					   size_t nPos);
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
	virtual void setCancelable(bool bCancelable);
	virtual void setCount(size_t nCount);
	virtual void step(size_t nStep);
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
	size_t nCount_;
	size_t nPos_;
	bool bCancelable_;
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
