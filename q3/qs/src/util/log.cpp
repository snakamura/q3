/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsconv.h>
#include <qslog.h>
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
	OutputStream* pStream_;
	bool bDeleteStream_;
	Logger::Level level_;
};


/****************************************************************************
 *
 * Logger
 *
 */

qs::Logger::Logger(OutputStream* pStream,
				   bool bDeleteStream,
				   Level level) :
	pImpl_(0)
{
	pImpl_ = new LoggerImpl();
	pImpl_->pStream_ = pStream;
	pImpl_->bDeleteStream_ = bDeleteStream;
	pImpl_->level_ = level;
}

qs::Logger::~Logger()
{
	if (pImpl_) {
		if (pImpl_->bDeleteStream_)
			delete pImpl_->pStream_;
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
	
	Time time(Time::getCurrentTime());
	wstring_ptr wstrTime(time.format(L"%Y4/%M0/%D-%h:%m:%s%z", Time::FORMAT_LOCAL));
	
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
		return;
	
	if (pImpl_->pStream_->write(
		reinterpret_cast<unsigned char*>(encoded.get()), encoded.size()) == -1)
		return;
	
	if (pData) {
		if (pImpl_->pStream_->write(pData, nDataLen) == -1)
			return;
		if (pImpl_->pStream_->write(reinterpret_cast<const unsigned char*>("\r\n"), 2) == -1)
			return;
	}
	pImpl_->pStream_->flush();
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
	WCHAR wszMessage[256];
	_vsnwprintf(wszMessage, countof(wszMessage), pwszFormat, args);
	log(level, pwszModule, wszMessage, 0, 0);
}

bool qs::Logger::isEnabled(Level level) const
{
	return pImpl_->level_ >= level;
}
