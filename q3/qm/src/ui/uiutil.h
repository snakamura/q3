/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __UIUTIL_H__
#define __UIUTIL_H__

#include <qm.h>
#include <qmmessageoperation.h>

#include <qs.h>
#include <qsprofile.h>
#include <qsstring.h>


namespace qm {

class UIUtil;
class ProgressDialogInit;

class Folder;
class ProgressDialog;


/****************************************************************************
 *
 * UIUtil
 *
 */

class UIUtil
{
public:
	static qs::QSTATUS loadWindowPlacement(qs::Profile* pProfile,
		const WCHAR* pwszSection, CREATESTRUCT* pCreateStruct, int* pnShow);
	static qs::QSTATUS saveWindowPlacement(HWND hwnd,
		qs::Profile* pProfile, const WCHAR* pwszSection);
	
	static qs::QSTATUS formatMenu(const WCHAR* pwszText, qs::WSTRING* pwstrText);
	static qs::QSTATUS openURL(HWND hwnd, const WCHAR* pwszURL);
	
	static int getFolderImage(Folder* pFolder, bool bSelected);
};


/****************************************************************************
 *
 * ProgressDialogInit
 *
 */

class ProgressDialogInit
{
public:
	ProgressDialogInit(ProgressDialog* pDialog, HWND hwnd, qs::QSTATUS* pstatus);
	ProgressDialogInit(ProgressDialog* pDialog, HWND hwnd,
		UINT nTitle, UINT nMessage, unsigned int nMin,
		unsigned int nMax, unsigned int nPos, qs::QSTATUS* pstatus);
	~ProgressDialogInit();

private:
	ProgressDialogInit(const ProgressDialogInit&);
	ProgressDialogInit& operator=(const ProgressDialogInit&);

private:
	ProgressDialog* pDialog_;
};


/****************************************************************************
 *
 * ProgressDialogMessageOperationCallback
 *
 */

class ProgressDialogMessageOperationCallback :
	public MessageOperationCallback
{
public:
	ProgressDialogMessageOperationCallback(
		HWND hwnd, UINT nTitle, UINT nMessage);
	virtual ~ProgressDialogMessageOperationCallback();

public:
	virtual bool isCanceled();
	virtual qs::QSTATUS setCount(unsigned int nCount);
	virtual qs::QSTATUS step(unsigned int nStep);
	virtual qs::QSTATUS show();

private:
	ProgressDialogMessageOperationCallback(ProgressDialogMessageOperationCallback&);
	ProgressDialogMessageOperationCallback& operator=(ProgressDialogMessageOperationCallback&);

private:
	HWND hwnd_;
	UINT nTitle_;
	UINT nMessage_;
	ProgressDialog* pDialog_;
	unsigned int nCount_;
	unsigned int nPos_;
};

}

#endif // __UIUTIL_H__
