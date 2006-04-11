/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __UI_H__
#define __UI_H__

#include <qmaccount.h>

#include <qsdialog.h>

#include "rss.h"


namespace qmrss {

class ReceivePage;
class SendPage;
class SubscribeData;
class SubscribeURLPage;
class SubscribePropertyPage;


/****************************************************************************
 *
 * ReceivePage
 *
 */

class ReceivePage : public qs::DefaultPropertyPage
{
public:
	explicit ReceivePage(qm::SubAccount* pSubAccount);
	virtual ~ReceivePage();

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

protected:
	virtual LRESULT onOk();

private:
	LRESULT onProxy(UINT nId);

private:
	void updateState();

private:
	ReceivePage(const ReceivePage&);
	ReceivePage& operator=(const ReceivePage&);

private:
	qm::SubAccount* pSubAccount_;
};


/****************************************************************************
 *
 * SendPage
 *
 */

class SendPage : public qs::DefaultPropertyPage
{
public:
	explicit SendPage(qm::SubAccount* pSubAccount);
	virtual ~SendPage();

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

protected:
	virtual LRESULT onOk();

private:
	SendPage(const SendPage&);
	SendPage& operator=(const SendPage&);

private:
	qm::SubAccount* pSubAccount_;
};


/****************************************************************************
 *
 * SubscribeData
 *
 */

class SubscribeData
{
public:
	explicit SubscribeData(const WCHAR* pwszURL);
	~SubscribeData();

private:
	SubscribeData(const SubscribeData&);
	SubscribeData& operator=(const SubscribeData&);

public:
	qs::wstring_ptr wstrURL_;
	qs::wstring_ptr wstrName_;
	bool bMakeMultipart_;
	bool bUseDescriptionAsContent_;
	bool bUpdateIfModified_;
	qs::wstring_ptr wstrUserName_;
	qs::wstring_ptr wstrPassword_;
};


/****************************************************************************
 *
 * SubscribeURLPage
 *
 */

class SubscribeURLPage :
#if !defined _WIN32_WCE || _WIN32_WCE < 300 || !defined _WIN32_WCE_PSPC
	public qs::DefaultPropertyPage
#else
	public qs::DefaultDialog
#endif
{
private:
	enum {
		MAX_REDIRECT = 5
	};

public:
	SubscribeURLPage(qm::Document* pDocument,
					 qm::Account* pAccount,
					 SubscribeData* pData);
	virtual ~SubscribeURLPage();

#if defined _WIN32_WCE && _WIN32_WCE >= 300 && defined _WIN32_WCE_PSPC
protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

protected:
	virtual LRESULT onOk();
#endif

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

private:
	LRESULT onURLChange();

#if !defined _WIN32_WCE || _WIN32_WCE < 300 || !defined _WIN32_WCE_PSPC
public:
	virtual LRESULT onNotify(NMHDR* pnmhdr,
							 bool* pbHandled);

private:
	LRESULT onSetActive(NMHDR* pnmhdr,
						bool* pbHandled);
	LRESULT onWizNext(NMHDR* pnmhdr,
					  bool* pbHandled);
#endif

private:
	void initialize();
	bool next();
	void updateState();

private:
	std::auto_ptr<Channel> getChannel(const WCHAR* pwszURL,
									  bool bAutoDiscovery) const;

private:
	SubscribeURLPage(const SubscribeURLPage&);
	SubscribeURLPage& operator=(const SubscribeURLPage&);

private:
	qm::Document* pDocument_;
	qm::Account* pAccount_;
	SubscribeData* pData_;
};


/****************************************************************************
 *
 * SubscribePropertyPage
 *
 */

class SubscribePropertyPage :
#if !defined _WIN32_WCE || _WIN32_WCE < 300 || !defined _WIN32_WCE_PSPC
	public qs::DefaultPropertyPage
#else
	public qs::DefaultDialog
#endif
{
public:
	explicit SubscribePropertyPage(SubscribeData* pData);
	virtual ~SubscribePropertyPage();

#if defined _WIN32_WCE && _WIN32_WCE >= 300 && defined _WIN32_WCE_PSPC
protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

protected:
	virtual LRESULT onOk();
#endif

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

private:
	LRESULT onAuthenticateClicked();
	LRESULT onMakeMultipartClicked();
	LRESULT onNameChange();

#if !defined _WIN32_WCE || _WIN32_WCE < 300 || !defined _WIN32_WCE_PSPC
public:
	virtual LRESULT onNotify(NMHDR* pnmhdr,
							 bool* pbHandled);

private:
	LRESULT onSetActive(NMHDR* pnmhdr,
						bool* pbHandled);
	LRESULT onWizFinish(NMHDR* pnmhdr,
						bool* pbHandled);
#endif

private:
	void initialize();
	void finish();
	void updateState();

private:
	SubscribePropertyPage(const SubscribePropertyPage&);
	SubscribePropertyPage& operator=(const SubscribePropertyPage&);

private:
	SubscribeData* pData_;
};

}

#endif // __UI_H__
