/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
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
	Logger(OutputStream* pStream,
		   bool bDeleteStream,
		   Level level);
	~Logger();

public:
	void log(Level level,
			 const WCHAR* pwszModule,
			 const WCHAR* pwszMessage);
	void log(Level level,
			 const WCHAR* pwszModule,
			 const WCHAR* pwszMessage,
			 const unsigned char* pData,
			 size_t nDataLen);
	void logf(Level level,
			  const WCHAR* pwszModule,
			  const WCHAR* pwszFormat,
			 ...);
	void logf(Level level,
			  const WCHAR* pwszModule,
			  const WCHAR* pwszFormat,
			  va_list args);
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
	Log(Logger* pLogger,
		const WCHAR* pwszModule);
	~Log();

public:
	void fatal(const WCHAR* pwszMessage);
	void fatal(const WCHAR* pwszMessage,
			   const unsigned char* pData,
			   size_t nDataLen);
	void fatalf(const WCHAR* pwszFormat,
				...);
	bool isFatalEnabled() const;
	
	void error(const WCHAR* pwszMessage);
	void error(const WCHAR* pwszMessage,
			   const unsigned char* pData,
			   size_t nDataLen);
	void errorf(const WCHAR* pwszFormat,
				...);
	bool isErrorEnabled() const;
	
	void warn(const WCHAR* pwszMessage);
	void warn(const WCHAR* pwszMessage,
			  const unsigned char* pData,
			  size_t nDataLen);
	void warnf(const WCHAR* pwszFormat,
			   ...);
	bool isWarnEnabled() const;
	
	void info(const WCHAR* pwszMessage);
	void info(const WCHAR* pwszMessage,
			  const unsigned char* pData,
			  size_t nDataLen);
	void infof(const WCHAR* pwszFormat,
			   ...);
	bool isInfoEnabled() const;
	
	void debug(const WCHAR* pwszMessage);
	void debug(const WCHAR* pwszMessage,
			   const unsigned char* pData,
			   size_t nDataLen);
	void debugf(const WCHAR* pwszFormat,
				...);
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
