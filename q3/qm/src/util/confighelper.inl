/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
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
	
	qs::FileOutputStream stream(renamer.getPath());
	if (!stream)
		return false;
	qs::BufferedOutputStream bufferedStream(&stream, false);
	qs::OutputStreamWriter writer(&bufferedStream, false, L"utf-8");
	if (!writer)
		return false;
	
	Writer w(&writer, L"utf-8");
	if (!w.write(t))
		return false;
	
	if (!writer.close())
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

template<class Config, class Handler, class Writer, class LoadLock>
qm::ConfigHelper<Config, Handler, Writer, LoadLock>::ConfigHelper(const WCHAR* pwszPath) :
	pLock_(0)
{
	init(pwszPath);
}

template<class Config, class Handler, class Writer, class LoadLock>
qm::ConfigHelper<Config, Handler, Writer, LoadLock>::ConfigHelper(const WCHAR* pwszPath,
																  const LoadLock& lock) :
	pLock_(&lock)
{
	init(pwszPath);
}

template<class Config, class Handler, class Writer, class LoadLock>
qm::ConfigHelper<Config, Handler, Writer, LoadLock>::~ConfigHelper()
{
}

template<class Config, class Handler, class Writer, class LoadLock>
bool qm::ConfigHelper<Config, Handler, Writer, LoadLock>::load(Config* pConfig,
															   Handler* pHandler)
{
	struct Lock
	{
		Lock(const LoadLock* pLock) :
			pLock_(pLock)
		{
			if (pLock_)
				pLock_->lock();
		}
		
		~Lock()
		{
			if (pLock_)
				pLock_->unlock();
		}
		
		const LoadLock* pLock_;
	};
	
	W2T(wstrPath_.get(), ptszPath);
	qs::AutoHandle hFile(::CreateFile(ptszPath, GENERIC_READ, 0, 0,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0));
	if (hFile.get()) {
		FILETIME ft;
		::GetFileTime(hFile.get(), 0, 0, &ft);
		hFile.close();
		
		if (::CompareFileTime(&ft, &ft_) != 0) {
			Lock lock(pLock_);
			
			pConfig->clear();
			
			XMLReader reader;
			reader.setContentHandler(pHandler);
			if (!reader.parse(wstrPath_.get()))
				return false;
			
			ft_ = ft;
		}
	}
	else {
		Lock lock(pLock_);
		
		pConfig->clear();
		
		SYSTEMTIME st;
		::GetSystemTime(&st);
		::SystemTimeToFileTime(&st, &ft_);
	}
	
	return true;
}

template<class Config, class Handler, class Writer, class LoadLock>
bool qm::ConfigHelper<Config, Handler, Writer, LoadLock>::save(const Config* pConfig) const
{
	return ConfigSaver<const Config*, Writer>::save(pConfig, wstrPath_.get());
}

template<class Config, class Handler, class Writer, class LoadLock>
void qm::ConfigHelper<Config, Handler, Writer, LoadLock>::init(const WCHAR* pwszPath)
{
	wstrPath_ = allocWString(pwszPath);
	
	SYSTEMTIME st;
	::GetSystemTime(&st);
	::SystemTimeToFileTime(&st, &ft_);
}

#endif // __CONFIGHELPER_INL__
