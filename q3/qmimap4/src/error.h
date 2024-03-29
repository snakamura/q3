/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#ifndef __ERROR_H__
#define __ERROR_H__

#define IMAP4_ERROR(e) \
	do { \
		nError_ = e; \
		return false; \
	} while (false) \

#define IMAP4_ERROR_SOCKET(e) \
	do { \
		nError_ = e | pSocket_->getLastError(); \
		return false; \
	} while (false)

#define IMAP4_ERROR_OR(e) \
	do { \
		nError_ |= e; \
		return false; \
	} while (false) \

#endif // __ERROR_H__
