/*
 * $Id: log.cpp,v 1.1.1.1 2003/04/29 08:07:37 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsconv.h>
#include <qslog.h>
#include <qsnew.h>
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

qs::Logger::Logger(OutputStream* pStream, bool bDeleteStream,
	Level level, QSTATUS* pstatus) :
	pImpl_(0)
{
	DECLARE_QSTATUS();
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
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

QSTATUS qs::Logger::log(Level level, const WCHAR* pwszModule, const WCHAR* pwszMessage)
{
	return log(level, pwszModule, pwszMessage, 0, 0);
}

QSTATUS qs::Logger::log(Level level, const WCHAR* pwszModule,
	const WCHAR* pwszMessage, const unsigned char* pData, size_t nDataLen)
{
	assert(pwszModule);
	assert(pwszMessage);
	
	DECLARE_QSTATUS();
	
	if (level > pImpl_->level_)
		return QSTATUS_SUCCESS;
	
	Time time(Time::getCurrentTime());
	WCHAR wszTime[32];
	swprintf(wszTime, L"%04d/%02d/%02d-%02d:%02d:%02d",
		time.wYear, time.wMonth, time.wDay,
		time.wHour, time.wMinute, time.wSecond);
	
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
		{ wszTime,				-1	},
		{ L" ",					1	},
		{ pwszModule,			-1	},
		{ L"] ",				2	},
		{ pwszMessage,			-1	},
		{ L"\r\n",				2	}
	};
	string_ptr<WSTRING> wstr(concat(c, countof(c)));
	if (!wstr.get())
		return QSTATUS_OUTOFMEMORY;
	
	UTF8Converter converter(&status);
	CHECK_QSTATUS();
	string_ptr<STRING> str;
	size_t nLen = wcslen(wstr.get());
	size_t nResultLen = 0;
	status = converter.encode(wstr.get(), &nLen, &str, &nResultLen);
	CHECK_QSTATUS();
	
	status = pImpl_->pStream_->write(
		reinterpret_cast<unsigned char*>(str.get()), nResultLen);
	CHECK_QSTATUS();
	
	if (pData) {
		status = pImpl_->pStream_->write(pData, nDataLen);
		CHECK_QSTATUS();
		status = pImpl_->pStream_->write(
			reinterpret_cast<const unsigned char*>("\r\n"), 2);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

bool qs::Logger::isEnabled(Level level) const
{
	return pImpl_->level_ >= level;
}
