/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __MAIN_H__
#define __MAIN_H__

#include <qm.h>

#include <qsthread.h>
#include <qsutil.h>
#include <qswindow.h>


namespace qm {

/****************************************************************************
 *
 * MainCommandLineHandler
 *
 */

class MainCommandLineHandler : public qs::CommandLineHandler
{
public:
	MainCommandLineHandler();
	virtual ~MainCommandLineHandler();

public:
	const WCHAR* getMailFolder() const;
	const WCHAR* getProfile() const;
	void invoke(HWND hwnd);

public:
	virtual bool process(const WCHAR* pwszOption);

private:
	MainCommandLineHandler(const MainCommandLineHandler&);
	MainCommandLineHandler& operator=(const MainCommandLineHandler&);

private:
	enum State {
		STATE_NONE,
		STATE_MAILFOLDER,
		STATE_PROFILE,
		STATE_GOROUND,
		STATE_URL,
		STATE_CREATE,
		STATE_DRAFT
	};

private:
	State state_;
	qs::wstring_ptr wstrMailFolder_;
	qs::wstring_ptr wstrProfile_;
	qs::wstring_ptr wstrGoRound_;
	qs::wstring_ptr wstrURL_;
	qs::wstring_ptr wstrPath_;
	unsigned int nAction_;
};


/****************************************************************************
 *
 * MailFolderLock
 *
 */

class MailFolderLock
{
public:
	MailFolderLock(const WCHAR* pwszMailFolder,
				   bool* pbContinue,
				   HWND* phwnd);
	~MailFolderLock();

public:
	bool setWindow(HWND hwnd);
	void unsetWindow();

private:
	void lock(const WCHAR* pwszMailFolder,
			  bool* pbContinue,
			  HWND* phwnd);
	void unlock();
	bool read(HANDLE hFile,
			  HWND* phwnd,
			  qs::wstring_ptr* pwstrName);

private:
	MailFolderLock(const MailFolderLock&);
	MailFolderLock& operator=(const MailFolderLock&);

private:
	qs::tstring_ptr tstrPath_;
	HANDLE hFile_;
	std::auto_ptr<qs::Mutex> pMutex_;
};

}

#endif // __MAIN_H__
