/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsconv.h>
#include <qslog.h>
#include <qsregex.h>
#include <qsstream.h>
#include <qsutil.h>

using namespace qs;


/****************************************************************************
 *
 * LoggerImpl
 *
 */

struct qs::LoggerImpl
{
	LogHandler* pLogHandler_;
	bool bDeleteHandler_;
	Logger::Level level_;
	std::auto_ptr<RegexPattern> pFilter_;
};


/****************************************************************************
 *
 * Logger
 *
 */

qs::Logger::Logger(LogHandler* pLogHandler,
				   bool bDeleteHandler,
				   Level level,
				   const WCHAR* pwszFilter) :
	pImpl_(0)
{
	pImpl_ = new LoggerImpl();
	pImpl_->pLogHandler_ = pLogHandler;
	pImpl_->bDeleteHandler_ = bDeleteHandler;
	pImpl_->level_ = level;
	if (pwszFilter)
		pImpl_->pFilter_ = RegexCompiler().compile(pwszFilter);
}

qs::Logger::~Logger()
{
	if (pImpl_) {
		if (pImpl_->bDeleteHandler_)
			delete pImpl_->pLogHandler_;
		delete pImpl_;
	}
}

void qs::Logger::log(Level level,
					 const WCHAR* pwszModule,
					 const WCHAR* pwszMessage)
{
	log(level, pwszModule, pwszMessage, 0, 0);
}

void qs::Logger::log(Level level,
					 const WCHAR* pwszModule,
					 const WCHAR* pwszMessage,
					 const unsigned char* pData,
					 size_t nDataLen)
{
	assert(pwszModule);
	assert(pwszMessage);
	
	if (level > pImpl_->level_)
		return;
	
	if (pImpl_->pFilter_.get() && !pImpl_->pFilter_->match(pwszModule))
		return;
	
	pImpl_->pLogHandler_->log(level, pwszModule, pwszMessage, pData, nDataLen);
}

void qs::Logger::logf(Level level,
					  const WCHAR* pwszModule,
					  const WCHAR* pwszFormat,
					  ...)
{
	va_list args;
	va_start(args, pwszFormat);
	logf(level, pwszModule, pwszFormat, args);
	va_end(args);
}

void qs::Logger::logf(Level level,
					  const WCHAR* pwszModule,
					  const WCHAR* pwszFormat,
					  va_list args)
{
	WCHAR wszMessage[1024];
	_vsnwprintf(wszMessage, countof(wszMessage), pwszFormat, args);
	log(level, pwszModule, wszMessage, 0, 0);
}

bool qs::Logger::isEnabled(Level level) const
{
	return pImpl_->level_ >= level;
}


/****************************************************************************
 *
 * LogHandler
 *
 */

qs::LogHandler::~LogHandler()
{
}


/****************************************************************************
 *
 * FileLogHandlerImpl
 *
 */

struct qs::FileLogHandlerImpl
{
	bool prepareStream();
	
	wstring_ptr wstrPath_;
	wstring_ptr wstrTimeFormat_;
	std::auto_ptr<qs::OutputStream> pStream_;
};

bool qs::FileLogHandlerImpl::prepareStream()
{
	if (pStream_.get())
		return true;
	
	std::auto_ptr<FileOutputStream> pFileOutputStream(
		new FileOutputStream(wstrPath_.get()));
	if (!*pFileOutputStream.get())
		return false;
	
	pStream_.reset(new BufferedOutputStream(pFileOutputStream.get(), true));
	pFileOutputStream.release();
	
	return true;
}


/****************************************************************************
 *
 * FileLogHandler
 *
 */

qs::FileLogHandler::FileLogHandler(const WCHAR* pwszPath,
								   const WCHAR* pwszTimeFormat) :
	pImpl_(0)
{
	if (!pwszTimeFormat)
		pwszTimeFormat = L"%Y4/%M0/%D-%h:%m:%s%z";
	
	pImpl_ = new FileLogHandlerImpl();
	pImpl_->wstrPath_ = allocWString(pwszPath);
	pImpl_->wstrTimeFormat_ = allocWString(pwszTimeFormat);
}

qs::FileLogHandler::~FileLogHandler()
{
	delete pImpl_;
}

bool qs::FileLogHandler::log(Logger::Level level,
							 const WCHAR* pwszModule,
							 const WCHAR* pwszMessage,
							 const unsigned char* pData,
							 size_t nDataLen)
{
	assert(pwszModule);
	assert(pwszMessage);
	
	if (!pImpl_->prepareStream())
		return false;
	
	Time time(Time::getCurrentTime());
	wstring_ptr wstrTime(time.format(pImpl_->wstrTimeFormat_.get(), Time::FORMAT_LOCAL));
	
	const WCHAR* pwszLevels[] = {
		L"FATAL",
		L"ERROR",
		L"WARN",
		L"INFO",
		L"DEBUG"
	};
	ConcatW c[] = {
		{ L"[",					1	},
		{ pwszLevels[level],	-1	},
		{ L" ",					1	},
		{ wstrTime.get(),		-1	},
		{ L" ",					1	},
		{ pwszModule,			-1	},
		{ L"] ",				2	},
		{ pwszMessage,			-1	},
		{ L"\r\n",				2	}
	};
	wstring_ptr wstr(concat(c, countof(c)));
	
	UTF8Converter converter;
	size_t nLen = wcslen(wstr.get());
	xstring_size_ptr encoded(converter.encode(wstr.get(), &nLen));
	if (!encoded.get())
		return false;
	
	if (pImpl_->pStream_->write(
		reinterpret_cast<unsigned char*>(encoded.get()), encoded.size()) == -1)
		return false;
	
	if (pData) {
		if (pImpl_->pStream_->write(pData, nDataLen) == -1)
			return false;
		if (pImpl_->pStream_->write(reinterpret_cast<const unsigned char*>("\r\n"), 2) == -1)
			return false;
	}
	if (!pImpl_->pStream_->flush())
		return false;
	
	return true;
}
