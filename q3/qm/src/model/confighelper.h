/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __CONFIGHELPER_H__
#define __CONFIGHELPER_H__

#include <qm.h>

#include <qsstring.h>


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

template<class Config, class Handler, class Writer>
class ConfigHelper
{
public:
	ConfigHelper(const WCHAR* pwszPath);
	~ConfigHelper();

public:
	bool load(Config* pConfig,
			  Handler* pHandler);
	bool save(const Config* pConfig) const;

private:
	ConfigHelper(const ConfigHelper&);
	ConfigHelper& operator=(const ConfigHelper&);

private:
	qs::wstring_ptr wstrPath_;
	FILETIME ft_;
};

}

#include "confighelper.inl"

#endif // __CONFIGHELPER_H__
