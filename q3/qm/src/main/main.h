/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __MAIN_H__
#define __MAIN_H__

#include <qm.h>

#include <qsutil.h>


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
	const WCHAR* getGoRound() const;
	const WCHAR* getMailFolder() const;
	const WCHAR* getProfile() const;
	const WCHAR* getURL() const;

public:
	virtual qs::QSTATUS process(const WCHAR* pwszOption);

private:
	MainCommandLineHandler(const MainCommandLineHandler&);
	MainCommandLineHandler& operator=(const MainCommandLineHandler&);

private:
	enum State {
		STATE_NONE,
		STATE_GOROUND,
		STATE_MAILFOLDER,
		STATE_PROFILE,
		STATE_URL
	};

private:
	State state_;
	qs::WSTRING wstrGoRound_;
	qs::WSTRING wstrMailFolder_;
	qs::WSTRING wstrProfile_;
	qs::WSTRING wstrURL_;
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
		bool* pbContinue, qs::QSTATUS* pstatus);
	~MailFolderLock();

public:
	qs::QSTATUS setWindow(HWND hwnd);

private:
	qs::QSTATUS lock(const WCHAR* pwszMailFolder, bool* pbContinue);
	qs::QSTATUS unlock();
	qs::QSTATUS read(HANDLE hFile, HWND* phwnd, qs::WSTRING* pwstrName);

private:
	MailFolderLock(const MailFolderLock&);
	MailFolderLock& operator=(const MailFolderLock&);

private:
	qs::TSTRING tstrPath_;
	HANDLE hFile_;
	qs::Mutex* pMutex_;
};

}

#endif // __MAIN_H__
