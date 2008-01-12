/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#ifndef __QSAPPLICATION_H__
#define __QSAPPLICATION_H__

#include <qm.h>

#include <qs.h>
#include <qsstring.h>


namespace qm {

class Application;

class Document;
class MailFolderLock;


/****************************************************************************
 *
 * Application
 *
 */

class QMEXPORTCLASS Application
{
public:
	Application(HINSTANCE hInst,
				HINSTANCE hInstResource,
				qs::wstring_ptr wstrMailFolder,
				qs::wstring_ptr wstrProfile,
				std::auto_ptr<MailFolderLock> pLock);
	~Application();

public:
	bool initialize(int nLogLevel,
					bool bQuiet);
	void uninitialize();
	void run();
	bool save(bool bForce);
	void startShutdown();
	bool isShutdown() const;
	
	HINSTANCE getResourceHandle() const;
	HINSTANCE getAtlHandle() const;
	const WCHAR* getMailFolder() const;
	const WCHAR* getTemporaryFolder() const;
	const WCHAR* getProfileName() const;
	qs::wstring_ptr getProfilePath(const WCHAR* pwszName) const;
	qs::wstring_ptr getImagePath(const WCHAR* pwszName) const;
	qs::wstring_ptr getVersion(WCHAR cSeparator,
							   bool bWithOSVersion) const;
	qs::wstring_ptr getOSVersion() const;

public:
	static Application& getApplication();

private:
	Application(const Application&);
	Application& operator=(const Application&);

private:
	struct ApplicationImpl* pImpl_;
};

}

#endif // __QSAPPLICATION_H__
