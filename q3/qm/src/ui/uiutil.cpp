/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmapplication.h>
#include <qmfolder.h>
#include <qmmessagewindow.h>

#include <qsconv.h>
#include <qsnew.h>
#include <qswindow.h>

#include <tchar.h>

#include "dialogs.h"
#include "resourceinc.h"
#include "statusbar.h"
#include "uiutil.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * UIUtil
 *
 */

QSTATUS qm::UIUtil::loadWindowPlacement(Profile* pProfile,
	const WCHAR* pwszSection, CREATESTRUCT* pCreateStruct, int* pnShow)
{
	assert(pProfile);
	assert(pwszSection);
	assert(pCreateStruct);
	assert(pnShow);
	
	DECLARE_QSTATUS();
	
#ifndef _WIN32_WCE
	struct {
		const WCHAR* pwszKey_;
		int n_;
	} items[] = {
		{ L"Left",		0 },
		{ L"Top",		0 },
		{ L"Width",		0 },
		{ L"Height",	0 }
	};
	for (int n = 0; n < countof(items); ++n) {
		status = pProfile->getInt(pwszSection,
			items[n].pwszKey_, 0, &items[n].n_);
		CHECK_QSTATUS();
	}
	if (items[2].n_ != 0 && items[3].n_ != 0) {
		RECT rect;
		::SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);
		pCreateStruct->x = rect.left + items[0].n_;
		pCreateStruct->y = rect.top + items[1].n_;
		pCreateStruct->cx = items[2].n_;
		pCreateStruct->cy = items[3].n_;
		
		status = pProfile->getInt(pwszSection, L"Show", SW_SHOWNORMAL, pnShow);
		CHECK_QSTATUS();
		if (*pnShow != SW_MAXIMIZE &&
			*pnShow != SW_MINIMIZE &&
			*pnShow != SW_SHOWNORMAL)
			*pnShow = SW_SHOWNORMAL;
	}
	else {
		*pnShow = SW_SHOWNORMAL;
	}
#endif
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::UIUtil::saveWindowPlacement(HWND hwnd,
	Profile* pProfile, const WCHAR* pwszSection)
{
	assert(hwnd);
	assert(pProfile);
	assert(pwszSection);
	
	DECLARE_QSTATUS();
	
#ifndef _WIN32_WCE
	WINDOWPLACEMENT wp = { sizeof(wp) };
	Window(hwnd).getWindowPlacement(&wp);
	
	const RECT& rect = wp.rcNormalPosition;
	struct {
		const WCHAR* pwszKey_;
		int n_;
	} items[] = {
		{ L"Left",		rect.left				},
		{ L"Top",		rect.top				},
		{ L"Width",		rect.right - rect.left	},
		{ L"Height",	rect.bottom - rect.top	}
	};
	for (int n = 0; n < countof(items); ++n) {
		status = pProfile->setInt(pwszSection, items[n].pwszKey_, items[n].n_);
		CHECK_QSTATUS();
	}
	
	int nShow = SW_SHOWNORMAL;
	switch (wp.showCmd) {
	case SW_MAXIMIZE:
//	case SW_SHOWMAXIMIZED:
		nShow = SW_MAXIMIZE;
		break;
	case SW_MINIMIZE:
	case SW_SHOWMINIMIZED:
		nShow = SW_MINIMIZE;
		break;
	}
	status = pProfile->setInt(pwszSection, L"Show", nShow);
	CHECK_QSTATUS();
#endif
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::UIUtil::formatMenu(const WCHAR* pwszText, WSTRING* pwstrText)
{
	assert(pwszText);
	assert(pwstrText);
	
	DECLARE_QSTATUS();
	
	StringBuffer<WSTRING> buf(&status);
	CHECK_QSTATUS();
	status = buf.append(L'&');
	CHECK_QSTATUS();
	while (*pwszText) {
		if (*pwszText == L'&') {
			status = buf.append(L'&');
			CHECK_QSTATUS();
		}
		status = buf.append(*pwszText);
		CHECK_QSTATUS();
		++pwszText;
	}
	
	*pwstrText = buf.getString();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::UIUtil::openURL(HWND hwnd, const WCHAR* pwszURL)
{
	assert(pwszURL);
	
	DECLARE_QSTATUS();
	
	W2T(pwszURL, ptszURL);
	
	SHELLEXECUTEINFO info = {
		sizeof(info),
		0,
		hwnd,
		_T("open"),
		ptszURL,
		0,
		0,
		SW_SHOW
	};
	::ShellExecuteEx(&info);
	
	return QSTATUS_SUCCESS;
}

int qm::UIUtil::getFolderImage(Folder* pFolder, bool bSelected)
{
	int nImage = 0;
	unsigned int nFlags = pFolder->getFlags();
	if (nFlags & Folder::FLAG_INBOX)
		nImage = 6;
	else if ((nFlags & Folder::FLAG_OUTBOX) ||
		(nFlags & Folder::FLAG_SENTBOX) ||
		(nFlags & Folder::FLAG_DRAFTBOX))
		nImage = 8;
	else if (nFlags & Folder::FLAG_TRASHBOX)
		nImage = 10;
	else if (bSelected)
		nImage = 4;
	else
		nImage = 2;
	return nImage;
}

QSTATUS qm::UIUtil::updateStatusBar(MessageWindow* pMessageWindow,
	StatusBar* pStatusBar, int nOffset, MessageHolder* pmh, Message& msg)
{
	DECLARE_QSTATUS();
	
	if (pmh) {
		const WCHAR* pwszEncoding = pMessageWindow->getEncoding();
		string_ptr<WSTRING> wstrCharset;
		if (!pwszEncoding) {
			const ContentTypeParser* pContentType = msg.getContentType();
			if (pContentType) {
				status = pContentType->getParameter(L"charset", &wstrCharset);
				CHECK_QSTATUS();
			}
			pwszEncoding = wstrCharset.get();
		}
		if (!pwszEncoding)
			pwszEncoding = L"us-ascii";
		status = pStatusBar->setText(nOffset + 1, pwszEncoding);
		CHECK_QSTATUS();
		
		const WCHAR* pwszTemplate = pMessageWindow->getTemplate();
		string_ptr<WSTRING> wstrNone;
		if (pwszTemplate) {
			pwszTemplate += 5;
		}
		else {
			status = loadString(Application::getApplication().getResourceHandle(),
				IDS_NONE, &wstrNone);
			CHECK_QSTATUS();
			pwszTemplate = wstrNone.get();
		}
		status = pStatusBar->setText(nOffset + 2, pwszTemplate);
		CHECK_QSTATUS();
		
		unsigned int nSecurity = msg.getSecurity();
		if (nSecurity & Message::SECURITY_DECRYPTED) {
			// TODO
			// Use icon
			status = pStatusBar->setText(nOffset + 3, L"D");
			CHECK_QSTATUS();
		}
		if (nSecurity & Message::SECURITY_VERIFIED) {
			// TODO
			// Use icon
			status = pStatusBar->setText(nOffset + 4, L"V");
			CHECK_QSTATUS();
		}
	}
	else {
		for (int n = 1; n < 5; ++n) {
			status = pStatusBar->setText(nOffset + n, L"");
			CHECK_QSTATUS();
		}
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * ProgressDialogInit
 *
 */

qm::ProgressDialogInit::ProgressDialogInit(
	ProgressDialog* pDialog, HWND hwnd, QSTATUS* pstatus) :
	pDialog_(0)
{
	DECLARE_QSTATUS();
	
	status = pDialog->init(hwnd);
	CHECK_QSTATUS_SET(pstatus);
	pDialog_ = pDialog;
}

qm::ProgressDialogInit::ProgressDialogInit(ProgressDialog* pDialog,
	HWND hwnd, UINT nTitle, UINT nMessage, unsigned int nMin,
	unsigned int nMax, unsigned int nPos, qs::QSTATUS* pstatus) :
	pDialog_(0)
{
	DECLARE_QSTATUS();
	
	status = pDialog->init(hwnd);
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


/****************************************************************************
 *
 * ProgressDialogMessageOperationCallback
 *
 */

qm::ProgressDialogMessageOperationCallback::ProgressDialogMessageOperationCallback(
	HWND hwnd, UINT nTitle, UINT nMessage) :
	hwnd_(hwnd),
	nTitle_(nTitle),
	nMessage_(nMessage),
	pDialog_(0),
	nCount_(-1),
	nPos_(0)
{
}

qm::ProgressDialogMessageOperationCallback::~ProgressDialogMessageOperationCallback()
{
	if (pDialog_) {
		pDialog_->term();
		delete pDialog_;
	}
}

bool qm::ProgressDialogMessageOperationCallback::isCanceled()
{
	if (pDialog_)
		return pDialog_->isCanceled();
	else
		return false;
}

QSTATUS qm::ProgressDialogMessageOperationCallback::setCount(unsigned int nCount)
{
	DECLARE_QSTATUS();
	
	if (nCount_ == static_cast<unsigned int>(-1)) {
		nCount_ = nCount;
		
		if (pDialog_) {
			status = pDialog_->setRange(0, nCount);
			CHECK_QSTATUS();
		}
	}
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ProgressDialogMessageOperationCallback::step(unsigned int nStep)
{
	DECLARE_QSTATUS();
	
	nPos_ += nStep;
	
	if (pDialog_) {
		status = pDialog_->setPos(nPos_);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ProgressDialogMessageOperationCallback::show()
{
	assert(!pDialog_);
	
	DECLARE_QSTATUS();
	
	std::auto_ptr<ProgressDialog> pDialog;
	status = newQsObject(nTitle_, &pDialog);
	CHECK_QSTATUS();
	status = pDialog->init(hwnd_);
	CHECK_QSTATUS();
	pDialog_ = pDialog.release();
	
	status = pDialog_->setMessage(nMessage_);
	CHECK_QSTATUS();
	
	if (nCount_ != static_cast<unsigned int>(-1)) {
		status = pDialog_->setRange(0, nCount_);
		CHECK_QSTATUS();
		status = pDialog_->setPos(nPos_);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}
