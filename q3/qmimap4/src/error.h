/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
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
