/*
 * $Id: qslog.inl,v 1.1.1.1 2003/04/29 08:07:34 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qserror.h>


/****************************************************************************
 *
 * Log
 *
 */

inline qs::Log::Log(qs::Logger* pLogger, const WCHAR* pwszModule) :
	pLogger_(pLogger),
	pwszModule_(pwszModule)
{
}

inline qs::Log::~Log()
{
}

inline qs::QSTATUS qs::Log::fatal(const WCHAR* pwszMessage)
{
	return fatal(pwszMessage, 0, 0);
}

inline qs::QSTATUS qs::Log::fatal(const WCHAR* pwszMessage,
	const unsigned char* pData, size_t nDataLen)
{
	return pLogger_ ?
		pLogger_->log(Logger::LEVEL_FATAL, pwszModule_, pwszMessage, pData, nDataLen) :
		qs::QSTATUS_SUCCESS;
}

inline bool qs::Log::isFatalEnabled() const
{
	return pLogger_ ? pLogger_->isEnabled(Logger::LEVEL_FATAL) : false;
}

inline qs::QSTATUS qs::Log::error(const WCHAR* pwszMessage)
{
	return error(pwszMessage, 0, 0);
}

inline qs::QSTATUS qs::Log::error(const WCHAR* pwszMessage,
	const unsigned char* pData, size_t nDataLen)
{
	return pLogger_ ?
		pLogger_->log(Logger::LEVEL_ERROR, pwszModule_, pwszMessage, pData, nDataLen) :
		qs::QSTATUS_SUCCESS;
}

inline bool qs::Log::isErrorEnabled() const
{
	return pLogger_ ? pLogger_->isEnabled(Logger::LEVEL_ERROR) : false;
}

inline qs::QSTATUS qs::Log::warn(const WCHAR* pwszMessage)
{
	return warn(pwszMessage, 0, 0);
}

inline qs::QSTATUS qs::Log::warn(const WCHAR* pwszMessage,
	const unsigned char* pData, size_t nDataLen)
{
	return pLogger_ ?
		pLogger_->log(Logger::LEVEL_WARN, pwszModule_, pwszMessage, pData, nDataLen) :
		qs::QSTATUS_SUCCESS;
}

inline bool qs::Log::isWarnEnabled() const
{
	return pLogger_ ? pLogger_->isEnabled(Logger::LEVEL_WARN) : false;
}

inline qs::QSTATUS qs::Log::info(const WCHAR* pwszMessage)
{
	return info(pwszMessage, 0, 0);
}

inline qs::QSTATUS qs::Log::info(const WCHAR* pwszMessage,
	const unsigned char* pData, size_t nDataLen)
{
	return pLogger_ ?
		pLogger_->log(Logger::LEVEL_INFO, pwszModule_, pwszMessage, pData, nDataLen) :
		qs::QSTATUS_SUCCESS;
}

inline bool qs::Log::isInfoEnabled() const
{
	return pLogger_ ? pLogger_->isEnabled(Logger::LEVEL_INFO) : false;
}

inline qs::QSTATUS qs::Log::debug(const WCHAR* pwszMessage)
{
	return debug(pwszMessage, 0, 0);
}

inline qs::QSTATUS qs::Log::debug(const WCHAR* pwszMessage,
	const unsigned char* pData, size_t nDataLen)
{
	return pLogger_ ?
		pLogger_->log(Logger::LEVEL_DEBUG, pwszModule_, pwszMessage, pData, nDataLen) :
		qs::QSTATUS_SUCCESS;
}

inline bool qs::Log::isDebugEnabled() const
{
	return pLogger_ ? pLogger_->isEnabled(Logger::LEVEL_DEBUG) : false;
}
