/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */


/****************************************************************************
 *
 * Log
 *
 */

inline qs::Log::Log(qs::Logger* pLogger,
					const WCHAR* pwszModule) :
	pLogger_(pLogger),
	pwszModule_(pwszModule)
{
}

inline qs::Log::~Log()
{
}

inline void qs::Log::fatal(const WCHAR* pwszMessage)
{
	fatal(pwszMessage, 0, 0);
}

inline void qs::Log::fatal(const WCHAR* pwszMessage,
						   const unsigned char* pData,
						   size_t nDataLen)
{
	if (pLogger_)
		pLogger_->log(Logger::LEVEL_FATAL, pwszModule_, pwszMessage, pData, nDataLen);
}

inline void qs::Log::fatalf(const WCHAR* pwszFormat,
							...)
{
	if (pLogger_) {
		va_list args;
		va_start(args, pwszFormat);
		pLogger_->logf(Logger::LEVEL_FATAL, pwszModule_, pwszFormat, args);
		va_end(args);
	}
}

inline bool qs::Log::isFatalEnabled() const
{
	return pLogger_ ? pLogger_->isEnabled(Logger::LEVEL_FATAL) : false;
}

inline void qs::Log::error(const WCHAR* pwszMessage)
{
	error(pwszMessage, 0, 0);
}

inline void qs::Log::error(const WCHAR* pwszMessage,
						   const unsigned char* pData,
						   size_t nDataLen)
{
	if (pLogger_)
		pLogger_->log(Logger::LEVEL_ERROR, pwszModule_, pwszMessage, pData, nDataLen);
}

inline void qs::Log::errorf(const WCHAR* pwszFormat,
							...)
{
	if (pLogger_) {
		va_list args;
		va_start(args, pwszFormat);
		pLogger_->logf(Logger::LEVEL_ERROR, pwszModule_, pwszFormat, args);
		va_end(args);
	}
}

inline bool qs::Log::isErrorEnabled() const
{
	return pLogger_ ? pLogger_->isEnabled(Logger::LEVEL_ERROR) : false;
}

inline void qs::Log::warn(const WCHAR* pwszMessage)
{
	warn(pwszMessage, 0, 0);
}

inline void qs::Log::warn(const WCHAR* pwszMessage,
						  const unsigned char* pData,
						  size_t nDataLen)
{
	if (pLogger_)
		pLogger_->log(Logger::LEVEL_WARN, pwszModule_, pwszMessage, pData, nDataLen);
}

inline void qs::Log::warnf(const WCHAR* pwszFormat,
						   ...)
{
	if (pLogger_) {
		va_list args;
		va_start(args, pwszFormat);
		pLogger_->logf(Logger::LEVEL_WARN, pwszModule_, pwszFormat, args);
		va_end(args);
	}
}

inline bool qs::Log::isWarnEnabled() const
{
	return pLogger_ ? pLogger_->isEnabled(Logger::LEVEL_WARN) : false;
}

inline void qs::Log::info(const WCHAR* pwszMessage)
{
	info(pwszMessage, 0, 0);
}

inline void qs::Log::info(const WCHAR* pwszMessage,
						  const unsigned char* pData,
						  size_t nDataLen)
{
	if (pLogger_)
		pLogger_->log(Logger::LEVEL_INFO, pwszModule_, pwszMessage, pData, nDataLen);
}

inline void qs::Log::infof(const WCHAR* pwszFormat,
						   ...)
{
	if (pLogger_) {
		va_list args;
		va_start(args, pwszFormat);
		pLogger_->logf(Logger::LEVEL_INFO, pwszModule_, pwszFormat, args);
		va_end(args);
	}
}

inline bool qs::Log::isInfoEnabled() const
{
	return pLogger_ ? pLogger_->isEnabled(Logger::LEVEL_INFO) : false;
}

inline void qs::Log::debug(const WCHAR* pwszMessage)
{
	debug(pwszMessage, 0, 0);
}

inline void qs::Log::debug(const WCHAR* pwszMessage,
						   const unsigned char* pData,
						   size_t nDataLen)
{
	if (pLogger_)
		pLogger_->log(Logger::LEVEL_DEBUG, pwszModule_, pwszMessage, pData, nDataLen);
}

inline void qs::Log::debugf(const WCHAR* pwszFormat,
							...)
{
	if (pLogger_) {
		va_list args;
		va_start(args, pwszFormat);
		pLogger_->logf(Logger::LEVEL_DEBUG, pwszModule_, pwszFormat, args);
		va_end(args);
	}
}

inline bool qs::Log::isDebugEnabled() const
{
	return pLogger_ ? pLogger_->isEnabled(Logger::LEVEL_DEBUG) : false;
}
