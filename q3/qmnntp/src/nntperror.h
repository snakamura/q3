/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __NNTPERROR_H__
#define __NNTPERROR_H__


namespace qmnntp {

/****************************************************************************
 *
 * NntpError
 *
 */

enum NntpError {
	NNTPERROR_SUCCESS		= 0x00000000,
	
	NNTPERROR_FILTERJUNK	= 0x00000001,
	NNTPERROR_MANAGEJUNK	= 0x00000002,
	
	NNTPERROR_MASK			= 0x000000ff
};

}

#endif // __NNTPERROR_H__
