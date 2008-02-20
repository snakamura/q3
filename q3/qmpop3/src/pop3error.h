/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
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
	
	POP3ERROR_SAVE			= 0x00000001,
	POP3ERROR_APPLYRULES	= 0x00000002,
	POP3ERROR_MANAGEJUNK	= 0x00000003,
	POP3ERROR_FILTERJUNK	= 0x00000004,
	
	POP3ERROR_MASK			= 0x000000ff
};

}

#endif // __POP3ERROR_H__
