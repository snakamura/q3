/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QSLOG_H__
#define __QSLOG_H__

#include <qs.h>


namespace qs {

class Logger;

class OutputStream;


/****************************************************************************
 *
 * Logger
 *
 */

class QSEXPORTCLASS Logger
{
public:
	enum Level {
		LEVEL_FATAL,
		LEVEL_ERROR,
		LEVEL_WARN,
		LEVEL_INFO,
		LEVEL_DEBUG
	};

public:
	Logger(OutputStream* pStream, bool bDeleteStream,
		Level level, QSTATUS* pstatus);
	~Logger();

public:
	QSTATUS log(Level level, const WCHAR* pwszModule, const WCHAR* pwszMessage);
	QSTATUS log(Level level, const WCHAR* pwszModule, const WCHAR* pwszMessage,
		const unsigned char* pData, size_t nDataLen);
	bool isEnabled(Level level) const;

private:
	Logger(const Logger&);
	Logger& operator=(const Logger&);

private:
	struct LoggerImpl* pImpl_;
};


/****************************************************************************
 *
 * Log
 *
 */

class Log
{
public:
	Log(Logger* pLogger, const WCHAR* pwszModule);
	~Log();

public:
	QSTATUS fatal(const WCHAR* pwszMessage);
	QSTATUS fatal(const WCHAR* pwszMessage,
		const unsigned char* pData, size_t nDataLen);
	bool isFatalEnabled() const;
	QSTATUS error(const WCHAR* pwszMessage);
	QSTATUS error(const WCHAR* pwszMessage,
		const unsigned char* pData, size_t nDataLen);
	bool isErrorEnabled() const;
	QSTATUS warn(const WCHAR* pwszMessage);
	QSTATUS warn(const WCHAR* pwszMessage,
		const unsigned char* pData, size_t nDataLen);
	bool isWarnEnabled() const;
	QSTATUS info(const WCHAR* pwszMessage);
	QSTATUS info(const WCHAR* pwszMessage,
		const unsigned char* pData, size_t nDataLen);
	bool isInfoEnabled() const;
	QSTATUS debug(const WCHAR* pwszMessage);
	QSTATUS debug(const WCHAR* pwszMessage,
		const unsigned char* pData, size_t nDataLen);
	bool isDebugEnabled() const;

private:
	Log(const Log&);
	Log& operator=(const Log&);

private:
	Logger* pLogger_;
	const WCHAR* pwszModule_;
};

}

#include <qslog.inl>

#endif // __QSLOG_H__
