/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __CONFIGHELPER_INL__
#define __CONFIGHELPER_INL__

#include <qsfile.h>
#include <qsosutil.h>
#include <qsstream.h>


/****************************************************************************
 *
 * ConfigSaver
 *
 */

template<class T, class Writer>
bool qm::ConfigSaver<T, Writer>::save(T t,
									  const WCHAR* pwszPath)
{
	qs::TemporaryFileRenamer renamer(pwszPath);
	
	qs::FileOutputStream os(renamer.getPath());
	if (!os)
		return false;
	qs::OutputStreamWriter streamWriter(&os, false, L"utf-8");
	if (!streamWriter)
		return false;
	qs::BufferedWriter bufferedWriter(&streamWriter, false);
	
	Writer writer(&bufferedWriter);
	if (!writer.write(t))
		return false;
	
	if (!bufferedWriter.close())
		return false;
	
	if (!renamer.rename())
		return false;
	
	return true;
}


/****************************************************************************
 *
 * ConfigHelper
 *
 */

template<class Config, class Handler, class Writer>
qm::ConfigHelper<Config, Handler, Writer>::ConfigHelper(const WCHAR* pwszPath)
{
	wstrPath_ = allocWString(pwszPath);
	
	SYSTEMTIME st;
	::GetSystemTime(&st);
	::SystemTimeToFileTime(&st, &ft_);
}

template<class Config, class Handler, class Writer>
qm::ConfigHelper<Config, Handler, Writer>::~ConfigHelper()
{
}

template<class Config, class Handler, class Writer>
bool qm::ConfigHelper<Config, Handler, Writer>::load(Config* pConfig,
													 Handler* pHandler)
{
	W2T(wstrPath_.get(), ptszPath);
	qs::AutoHandle hFile(::CreateFile(ptszPath, GENERIC_READ, 0, 0,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0));
	if (hFile.get()) {
		FILETIME ft;
		::GetFileTime(hFile.get(), 0, 0, &ft);
		hFile.close();
		
		if (::CompareFileTime(&ft, &ft_) != 0) {
			pConfig->clear();
			
			XMLReader reader;
			reader.setContentHandler(pHandler);
			if (!reader.parse(wstrPath_.get()))
				return false;
			
			ft_ = ft;
		}
	}
	else {
		pConfig->clear();
		
		SYSTEMTIME st;
		::GetSystemTime(&st);
		::SystemTimeToFileTime(&st, &ft_);
	}
	
	return true;
}

template<class Config, class Handler, class Writer>
bool qm::ConfigHelper<Config, Handler, Writer>::save(const Config* pConfig) const
{
	return ConfigSaver<const Config*, Writer>::save(pConfig, wstrPath_.get());
}

#endif // __CONFIGHELPER_INL__
