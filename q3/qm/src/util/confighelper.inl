/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#ifndef __CONFIGHELPER_INL__
#define __CONFIGHELPER_INL__

#include <qsfile.h>
#include <qsinit.h>
#include <qslog.h>
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
	qs::Log log(qs::InitThread::getInitThread().getLogger(), L"qm::ConfigHelper");
	
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
	
	WIN32_FIND_DATA fd;
	qs::AutoFindHandle hFind(::FindFirstFile(ptszPath, &fd));
	if (hFind.get()) {
		if (::CompareFileTime(&fd.ftLastWriteTime, &ft_) != 0) {
			qs::AutoHandle hFile(::CreateFile(ptszPath, GENERIC_READ, 0, 0,
				OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0));
			if (hFile.get()) {
				log.debugf(L"Loading: %s", wstrPath_.get());
				
				Lock lock(pLock_);
				
				pConfig->clear();
				
				XMLReader reader;
				reader.setContentHandler(pHandler);
				if (!reader.parse(wstrPath_.get())) {
					log.errorf(L"Failed to load: %s", wstrPath_.get());
					return false;
				}
				
				ft_ = fd.ftLastWriteTime;
			}
			else {
				log.errorf(L"Could not open file: %s", wstrPath_.get());
			}
		}
	}
	else {
		log.debugf(L"File not found: %s", wstrPath_.get());
		
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
