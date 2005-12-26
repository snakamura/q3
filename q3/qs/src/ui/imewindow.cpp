/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
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
	nFlags_(FLAG_NONE)
{
	assert(pProfile);
	assert(pwszSection);
	assert(pwszKeySuffix);
	
	wstring_ptr wstrKey(concat(L"Ime", pwszKeySuffix_));
	nFlags_ = pProfile_->getInt(pwszSection_, wstrKey.get(), 0);
	
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
	wstring_ptr wstrKey(concat(L"Ime", pwszKeySuffix_));
	pProfile_->setInt(pwszSection_, wstrKey.get(), nFlags_);
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
	if (UIUtil::isImeEnabled(getHandle()))
		nFlags |= FLAG_IME;
#ifdef _WIN32_WCE_PSPC
	if (UIUtil::isSipEnabled())
		nFlags |= FLAG_SIP;
#endif
	nFlags_ = nFlags;
}

void qs::ImeWindow::restore()
{
	UIUtil::setImeEnabled(getHandle(), (nFlags_ & FLAG_IME) != 0);
#ifdef _WIN32_WCE_PSPC
	UIUtil::setSipEnabled((nFlags_ & FLAG_SIP) != 0);
#endif
}
