/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
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


/****************************************************************************
 *
 * Application
 *
 */

class QMEXPORTCLASS Application
{
public:
	Application(HINSTANCE hInst, qs::QSTATUS* pstatus);
	~Application();

public:
	qs::QSTATUS initialize();
	qs::QSTATUS uninitialize();
	qs::QSTATUS run();
	
	HINSTANCE getResourceHandle() const;
	HINSTANCE getAtlHandle() const;
	const WCHAR* getMailFolder() const;
	const WCHAR* getTemporaryFolder() const;
	const WCHAR* getProfileName() const;
	qs::QSTATUS getProfilePath(const WCHAR* pwszName,
		qs::WSTRING* pwstrPath) const;
	qs::QSTATUS getVersion(bool bWithOSVersion,
		qs::WSTRING* pwstrVersion) const;
	qs::QSTATUS getOSVersion(qs::WSTRING* pwstrOSVersion) const;

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
