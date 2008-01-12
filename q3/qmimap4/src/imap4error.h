/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#ifndef __IMAP4ERROR_H__
#define __IMAP4ERROR_H__


namespace qmimap4 {

/****************************************************************************
 *
 * Imap4Error
 *
 */

enum Imap4Error {
	IMAP4ERROR_SUCCESS		= 0x00000000,
	
	IMAP4ERROR_APPLYRULES	= 0x00000001,
	IMAP4ERROR_MANAGEJUNK	= 0x00000002,
	IMAP4ERROR_FILTERJUNK	= 0x00000003,
	
	IMAP4ERROR_MASK			= 0x000000ff
};

}

#endif // __IMAP4ERROR_H__
