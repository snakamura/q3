/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __ACTIONUTIL_H__
#define __ACTIONUTIL_H__

#include <qm.h>

#include <qs.h>


namespace qm {

class ProgressDialogInit;

class ProgressDialog;


/****************************************************************************
 *
 * ProgressDialogInit
 *
 */

class ProgressDialogInit
{
public:
	ProgressDialogInit(ProgressDialog* pDialog, qs::QSTATUS* pstatus);
	ProgressDialogInit(ProgressDialog* pDialog, UINT nTitle,
		UINT nMessage, unsigned int nMin, unsigned int nMax,
		unsigned int nPos, qs::QSTATUS* pstatus);
	~ProgressDialogInit();

private:
	ProgressDialogInit(const ProgressDialogInit&);
	ProgressDialogInit& operator=(const ProgressDialogInit&);

private:
	ProgressDialog* pDialog_;
};

}

#endif // __ACTIONUTIL_H__
