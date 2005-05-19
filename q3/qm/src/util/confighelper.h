/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __CONFIGHELPER_H__
#define __CONFIGHELPER_H__

#include <qm.h>

#include <qsstring.h>
#include <qsthread.h>


namespace qm {

/****************************************************************************
 *
 * ConfigSaver
 *
 */

template<class T, class Writer>
class ConfigSaver
{
public:
	static bool save(T t,
					 const WCHAR* pwszPath);
};


/****************************************************************************
 *
 * ConfigHelper
 *
 */

template<class Config, class Handler, class Writer, class LoadLock = qs::NoLock>
class ConfigHelper
{
public:
	explicit ConfigHelper(const WCHAR* pwszPath);
	ConfigHelper(const WCHAR* pwszPath,
				 const LoadLock& lock);
	~ConfigHelper();

public:
	bool load(Config* pConfig,
			  Handler* pHandler);
	bool save(const Config* pConfig) const;

private:
	void init(const WCHAR* pwszPath);

private:
	ConfigHelper(const ConfigHelper&);
	ConfigHelper& operator=(const ConfigHelper&);

private:
	qs::wstring_ptr wstrPath_;
	FILETIME ft_;
	const LoadLock* pLock_;
};

}

#include "confighelper.inl"

#endif // __CONFIGHELPER_H__
