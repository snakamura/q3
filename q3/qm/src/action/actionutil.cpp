/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qserror.h>

#include "actionutil.h"
#include "../ui/dialogs.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * ProgressDialogInit
 *
 */

qm::ProgressDialogInit::ProgressDialogInit(
	ProgressDialog* pDialog, QSTATUS* pstatus) :
	pDialog_(0)
{
	DECLARE_QSTATUS();
	
	status = pDialog->init();
	CHECK_QSTATUS_SET(pstatus);
	pDialog_ = pDialog;
}

qm::ProgressDialogInit::ProgressDialogInit(ProgressDialog* pDialog,
	UINT nTitle, UINT nMessage, unsigned int nMin, unsigned int nMax,
	unsigned int nPos, qs::QSTATUS* pstatus) :
	pDialog_(0)
{
	DECLARE_QSTATUS();
	
	status = pDialog->init();
	CHECK_QSTATUS_SET(pstatus);
	pDialog_ = pDialog;
	
	status = pDialog->setTitle(nTitle);
	CHECK_QSTATUS_SET(pstatus);
	status = pDialog->setMessage(nMessage);
	CHECK_QSTATUS_SET(pstatus);
	status = pDialog->setRange(nMin, nMax);
	CHECK_QSTATUS_SET(pstatus);
	status = pDialog->setPos(nPos);
	CHECK_QSTATUS_SET(pstatus);
}

qm::ProgressDialogInit::~ProgressDialogInit()
{
	if (pDialog_)
		pDialog_->term();
}
