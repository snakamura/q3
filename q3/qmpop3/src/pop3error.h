/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __POP3ERROR_H__
#define __POP3ERROR_H__


namespace qmpop3 {

/****************************************************************************
 *
 * Pop3Error
 *
 */

enum Pop3Error {
	POP3ERROR_SUCCESS		= 0x00000000,
	
	POP3ERROR_APPLYRULES	= 0x00000001,
	
	POP3ERROR_MASK			= 0x000000ff
};

}

#endif // __POP3ERROR_H__
