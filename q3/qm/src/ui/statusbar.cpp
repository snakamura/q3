/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmapplication.h>
#include <qmmessage.h>
#include <qmmessagewindow.h>

#include "actionid.h"
#include "resourceinc.h"
#include "statusbar.h"
#include "uiutil.h"
#include "../uimodel/encodingmodel.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * StatusBar
 *
 */

qm::StatusBar::StatusBar() :
	WindowBase(true)
{
	setWindowHandler(this, false);
}

qm::StatusBar::~StatusBar()
{
}

int qm::StatusBar::getParts(int nParts,
							int* pnWidth) const
{
	return static_cast<int>(const_cast<StatusBar*>(this)->sendMessage(
		SB_GETPARTS, nParts, reinterpret_cast<LPARAM>(pnWidth)));
}

bool qm::StatusBar::setParts(int* pnWidth,
							 size_t nCount)
{
	return sendMessage(SB_SETPARTS, nCount, reinterpret_cast<LPARAM>(pnWidth)) != 0;
}

bool qm::StatusBar::setText(int nPart,
							const WCHAR* pwszText)
{
	W2T(pwszText, ptszText);
	return sendMessage(SB_SETTEXT, nPart, reinterpret_cast<LPARAM>(ptszText)) != 0;
}

#ifndef _WIN32_WCE
bool qm::StatusBar::setIcon(int nPart,
							HICON hIcon)
{
	return sendMessage(SB_SETICON, nPart, reinterpret_cast<LPARAM>(hIcon)) != 0;
}
#endif

void qm::StatusBar::setSimple(bool bSimple)
{
	sendMessage(SB_SIMPLE, bSimple);
}

bool qm::StatusBar::getRect(int nPart,
							RECT* pRect) const
{
	return const_cast<StatusBar*>(this)->sendMessage(
		SB_GETRECT, nPart, reinterpret_cast<LPARAM>(pRect)) != 0;
}


/****************************************************************************
 *
 * MessageStatusBar
 *
 */

qm::MessageStatusBar::MessageStatusBar(MessageWindow* pMessageWindow,
									   EncodingModel* pEncodingModel,
									   int nOffset,
									   MenuManager* pMenuManager) :
	pMessageWindow_(pMessageWindow),
	pEncodingModel_(pEncodingModel),
	nOffset_(nOffset),
	pMenuManager_(pMenuManager)
{
	assert(pMessageWindow);
	assert(pEncodingModel);
	assert(pMenuManager);
	
#ifdef _WIN32_WCE_PSPC
	nOffset_ -= 2;
#endif
}

qm::MessageStatusBar::~MessageStatusBar()
{
}

void qm::MessageStatusBar::updateMessageParts(MessageHolder* pmh,
											  const Message& msg)
{
	if (pmh) {
#ifndef _WIN32_WCE_PSPC
		const WCHAR* pwszEncoding = pEncodingModel_->getEncoding();
		wstring_ptr wstrCharset;
		if (!pwszEncoding) {
			wstrCharset = PartUtil(msg).getBodyTextCharset(false);
			pwszEncoding = wstrCharset.get();
		}
		setText(nOffset_ + 1, pwszEncoding);
		
		const WCHAR* pwszTemplate = pMessageWindow_->getTemplate();
		wstring_ptr wstrNone;
		if (pwszTemplate) {
			pwszTemplate += 5;
		}
		else {
			wstrNone = loadString(Application::getApplication().getResourceHandle(), IDS_STATUS_NONE);
			pwszTemplate = wstrNone.get();
		}
		setText(nOffset_ + 2, pwszTemplate);
#endif
		
		unsigned int nSecurity = msg.getSecurity();
		if (nSecurity & Message::SECURITY_DECRYPTED)
			setIconOrText(nOffset_ + 3, IDI_DECRYPTED, L"D");
		else
			setIconOrText(nOffset_ + 3, 0, L"");
		
		HICON hIconVerified = 0;
		if ((nSecurity & Message::SECURITY_VERIFICATIONFAILED) ||
			((nSecurity & Message::SECURITY_ADDRESSNOTMATCH) &&
			 !(nSecurity & Message::SECURITY_ADDRESSNOTMATCHNOERROR)))
			setIconOrText(nOffset_ + 4, IDI_UNVERIFIED, L"X");
		else if (nSecurity & Message::SECURITY_VERIFIED)
			setIconOrText(nOffset_ + 4, IDI_VERIFIED, L"V");
		else
			setIconOrText(nOffset_ + 4, 0, L"");
	}
	else {
#ifdef _WIN32_WCE_PSPC
		int nMax = 3;
#else
		int nMax = 5;
#endif
		for (int n = 1; n < nMax; ++n)
			setText(nOffset_ + n, L"");
	}
}

LRESULT qm::MessageStatusBar::windowProc(UINT uMsg,
										 WPARAM wParam,
										 LPARAM lParam)
{
	BEGIN_MESSAGE_HANDLER()
		HANDLE_CONTEXTMENU()
		HANDLE_LBUTTONDOWN()
	END_MESSAGE_HANDLER()
	return StatusBar::windowProc(uMsg, wParam, lParam);
}

LRESULT qm::MessageStatusBar::onContextMenu(HWND hwnd,
											const POINT& pt)
{
#ifndef _WIN32_WCE_PSPC
	POINT ptMenu = UIUtil::getContextMenuPosition(getHandle(), pt);
	POINT ptClient = pt;
	screenToClient(&ptClient);
	int nPart = getPart(ptClient);
	if (nPart != -1) {
		HMENU hmenu = pMenuManager_->getMenu(getMenuName(nPart), false, false);
		if (hmenu) {
			UINT nFlags = TPM_LEFTALIGN | TPM_TOPALIGN;
#ifndef _WIN32_WCE
			nFlags |= TPM_LEFTBUTTON | TPM_RIGHTBUTTON;
#endif
			::TrackPopupMenu(hmenu, nFlags, ptMenu.x, ptMenu.y, 0, getParentFrame(), 0);
		}
	}
#endif
	
	return 0;
}

LRESULT qm::MessageStatusBar::onLButtonDown(UINT nFlags,
											const POINT& pt)
{
	int nPart = getPart(pt);
	if (nPart != -1) {
		UINT nId = getClickActionId(nPart);
		if (nId != -1)
			Window(getParentFrame()).postMessage(WM_COMMAND, MAKEWPARAM(nId, 0), 0);
	}
	
	return StatusBar::onLButtonDown(nFlags, pt);
}

int qm::MessageStatusBar::getPart(const POINT& pt) const
{
	typedef std::vector<int> PartList;
	PartList listPart;
	listPart.resize(10);
	int nParts = getParts(static_cast<int>(listPart.size()), &listPart[0]);
	for (int n = 0; n < nParts; ++n) {
		RECT rect;
		getRect(n, &rect);
		if (::PtInRect(&rect, pt))
			return n;
	}
	return -1;
}

void qm::MessageStatusBar::setIconOrText(int nPart,
										 UINT nIcon,
										 const WCHAR* pwszText)
{
#ifndef _WIN32_WCE
	setIconId(nPart, nIcon);
#else
	setText(nPart, pwszText);
#endif
}

#ifndef _WIN32_WCE
void qm::MessageStatusBar::setIconId(int nPart,
									 UINT nIcon)
{
	HICON hIcon = 0;
	if (nIcon != 0)
		hIcon = reinterpret_cast<HICON>(::LoadImage(
			Application::getApplication().getResourceHandle(),
			MAKEINTRESOURCE(nIcon), IMAGE_ICON, 16, 16, LR_SHARED));
	setIcon(nPart, hIcon);
}
#endif

const WCHAR* qm::MessageStatusBar::getMenuName(int nPart)
{
	if (nPart == nOffset_ + 1)
		return L"encoding";
	else if (nPart == nOffset_ + 2)
		return L"template";
	else
		return 0;
}

UINT qm::MessageStatusBar::getClickActionId(int nPart)
{
	if (nPart == nOffset_ + 4)
		return IDM_MESSAGE_CERTIFICATE;
	else
		return -1;
}
