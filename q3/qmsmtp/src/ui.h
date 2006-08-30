/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __UI_H__
#define __UI_H__

#include <qmaccount.h>
#include <qmconnection.h>

#include <qsdialog.h>


namespace qmsmtp {

/****************************************************************************
 *
 * PopBeforeSmtpData
 *
 */

class PopBeforeSmtpData
{
public:
	PopBeforeSmtpData();
	~PopBeforeSmtpData();

public:
	void load(const qm::SubAccount* pSubAccount);
	void save(qm::SubAccount* pSubAccount) const;

private:
	PopBeforeSmtpData(const PopBeforeSmtpData&);
	PopBeforeSmtpData& operator=(const PopBeforeSmtpData&);

public:
	bool bCustom_;
	qs::wstring_ptr wstrProtocol_;
	qs::wstring_ptr wstrHost_;
	short nPort_;
	qm::SubAccount::Secure secure_;
	bool bApop_;
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

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

protected:
	virtual LRESULT onOk();

private:
	LRESULT onCustom();
	LRESULT onPopBeforeSmtp();

private:
	void updateState();

private:
	SendPage(const SendPage&);
	SendPage& operator=(const SendPage&);

private:
	qm::SubAccount* pSubAccount_;
	PopBeforeSmtpData popBeforeSmtpData_;
};


/****************************************************************************
 *
 * PopBeforeSmtpDialog
 *
 */

class PopBeforeSmtpDialog : public qs::DefaultDialog
{
public:
	explicit PopBeforeSmtpDialog(PopBeforeSmtpData* pData);
	virtual ~PopBeforeSmtpDialog();

public:
	virtual LRESULT onCommand(WORD nCode,
							  WORD nId);

protected:
	virtual LRESULT onInitDialog(HWND hwndFocus,
								 LPARAM lParam);

protected:
	virtual LRESULT onOk();

private:
	LRESULT onProtocolSelChange();
	LRESULT onSecure(UINT nId);
	LRESULT onType();

private:
	void updateState();
	void updatePort();

private:
	PopBeforeSmtpDialog(const PopBeforeSmtpDialog&);
	PopBeforeSmtpDialog& operator=(const PopBeforeSmtpDialog&);

private:
	PopBeforeSmtpData* pData_;
	qm::ConnectionFactory::NameList listName_;
};

}

#endif // __UI_H__
