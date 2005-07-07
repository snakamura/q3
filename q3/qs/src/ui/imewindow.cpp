/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsprofile.h>
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
	bIme_(false)
{
	assert(pProfile);
	assert(pwszSection);
	assert(pwszKeySuffix);
	
	wstring_ptr wstrKey(concat(L"Ime", pwszKeySuffix_));
	bIme_ = pProfile_->getInt(pwszSection_, wstrKey.get(), 0) != 0;
	
	setWindowHandler(this, false);
}

qs::ImeWindow::~ImeWindow()
{
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
	wstring_ptr wstrKey(concat(L"Ime", pwszKeySuffix_));
	pProfile_->setInt(pwszSection_, wstrKey.get(), bIme_);
	return DefaultWindowHandler::onDestroy();
}

LRESULT qs::ImeWindow::onKillFocus(HWND hwnd)
{
	HIMC hImc = ::ImmGetContext(getHandle());
	bIme_ = ::ImmGetOpenStatus(hImc) != 0;
	::ImmReleaseContext(getHandle(), hImc);
	return DefaultWindowHandler::onKillFocus(hwnd);
}

LRESULT qs::ImeWindow::onSetFocus(HWND hwnd)
{
	HIMC hImc = ::ImmGetContext(getHandle());
	::ImmSetOpenStatus(hImc, bIme_);
	::ImmReleaseContext(getHandle(), hImc);
	return DefaultWindowHandler::onSetFocus(hwnd);
}
