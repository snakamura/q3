/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsprofile.h>
#include <qsuiutil.h>
#include <qswindow.h>


/****************************************************************************
 *
 * ImeWindow
 *
 */

qs::ImeWindow::ImeWindow(Profile* pProfile,
						 const WCHAR* pwszSection,
						 const WCHAR* pwszKeySuffix,
						 bool bDeleteThis) :
	WindowBase(bDeleteThis),
	pProfile_(pProfile),
	pwszSection_(pwszSection),
	pwszKeySuffix_(pwszKeySuffix),
	nFlags_(FLAG_NONE),
	dwConversionStatus_(-1)
{
	assert(pProfile);
	assert(pwszSection);
	assert(pwszKeySuffix);
	
	wstring_ptr wstrFlagsKey(concat(L"Ime", pwszKeySuffix_));
	nFlags_ = pProfile_->getInt(pwszSection_, wstrFlagsKey.get(), FLAG_NONE);
	
#ifdef _WIN32_WCE_PSPC
	wstring_ptr wstrStatusKey(concat(L"ImeStatus", pwszKeySuffix_));
	dwConversionStatus_ = pProfile_->getInt(pwszSection_, wstrStatusKey.get(), -1);
#endif
	
	setWindowHandler(this, false);
}

qs::ImeWindow::~ImeWindow()
{
}

void qs::ImeWindow::postSubclassWindow()
{
	if (hasFocus())
		restore();
}

LRESULT qs::ImeWindow::windowProc(UINT uMsg,
								  WPARAM wParam,
								  LPARAM lParam)
{
	BEGIN_MESSAGE_HANDLER()
		HANDLE_DESTROY()
		HANDLE_KILLFOCUS()
		HANDLE_SETFOCUS()
	END_MESSAGE_HANDLER()
	return DefaultWindowHandler::windowProc(uMsg, wParam, lParam);
}

LRESULT qs::ImeWindow::onDestroy()
{
	wstring_ptr wstrFlagsKey(concat(L"Ime", pwszKeySuffix_));
	pProfile_->setInt(pwszSection_, wstrFlagsKey.get(), nFlags_);
	
#ifdef _WIN32_WCE_PSPC
	wstring_ptr wstrStatusKey(concat(L"ImeStatus", pwszKeySuffix_));
	pProfile_->setInt(pwszSection_, wstrStatusKey.get(), dwConversionStatus_);
#endif
	
	return DefaultWindowHandler::onDestroy();
}

LRESULT qs::ImeWindow::onKillFocus(HWND hwnd)
{
	save();
	return DefaultWindowHandler::onKillFocus(hwnd);
}

LRESULT qs::ImeWindow::onSetFocus(HWND hwnd)
{
	restore();
	return DefaultWindowHandler::onSetFocus(hwnd);
}

void qs::ImeWindow::save()
{
	unsigned int nFlags = FLAG_NONE;
#ifndef _WIN32_WCE_PSPC
	if (UIUtil::isImeEnabled(getHandle()))
		nFlags |= FLAG_IME;
#else
	dwConversionStatus_ = UIUtil::getImeStatus(getHandle());
	if (UIUtil::isSipEnabled())
		nFlags |= FLAG_SIP;
#endif
	nFlags_ = nFlags;
}

void qs::ImeWindow::restore()
{
#ifndef _WIN32_WCE_PSPC
	UIUtil::setImeEnabled(getHandle(), (nFlags_ & FLAG_IME) != 0);
#else
	if (dwConversionStatus_ != -1)
		UIUtil::setImeStatus(getHandle(), dwConversionStatus_);
	UIUtil::setSipEnabled((nFlags_ & FLAG_SIP) != 0);
#endif
}
