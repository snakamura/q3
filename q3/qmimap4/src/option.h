/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __OPTION_H__
#define __OPTION_H__

namespace qmimap4 {

/****************************************************************************
 *
 * Option
 *
 */

enum Option
{
	OPTION_USEENVELOPE				= 0x01,
	OPTION_USEBODYSTRUCTUREALWAYS	= 0x02,
	OPTION_TRUSTBODYSTRUCTURE		= 0x04
};

}

#endif // __OPTION_H__
