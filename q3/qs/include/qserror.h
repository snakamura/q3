/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QSERROR_H__
#define __QSERROR_H__

#include <qs.h>

namespace qs {

#define DECLARE_QSTATUS() \
qs::QSTATUS status = qs::QSTATUS_SUCCESS

#define CHECK_QSTATUS() \
if (status != qs::QSTATUS_SUCCESS) \
	return status

#define CHECK_QSTATUS_VALUE(value) \
if (status != qs::QSTATUS_SUCCESS) \
	return value

#define CHECK_QSTATUS_SET(p) \
if (status != qs::QSTATUS_SUCCESS) { \
	*p = status; \
	return; \
}

static const QSTATUS QSTATUS_SUCCESS			= 0x00000000;

static const QSTATUS QSTATUS_FAIL				= 0xE0000000;
static const QSTATUS QSTATUS_OUTOFMEMORY		= 0xE0000001;
static const QSTATUS QSTATUS_NOTIMPL			= 0xE0000002;

}

#endif // __QSERROR_H__
