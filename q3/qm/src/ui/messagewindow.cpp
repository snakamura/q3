/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmaccount.h>
#include <qmdocument.h>
#include <qmmessage.h>
#include <qmmessageholder.h>
#include <qmmessagewindow.h>
#include <qmtemplate.h>
#include <qmsecurity.h>

#include <qsaccelerator.h>
#include <qserror.h>
#include <qskeymap.h>
#include <qsnew.h>
#include <qsprofile.h>

#include "headerwindow.h"
#include "keymap.h"
#include "messagemodel.h"
#include "messageviewwindow.h"
#include "messagewindow.h"
#include "resourceinc.h"
#include "viewmodel.h"
#include "../model/templatemanager.h"

#pragma warning(disable:4786)

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * MessageWindowImpl
 *
 */

class qm::MessageWindowImpl :
	public MessageModelHandler
{
public:
	enum {
		ID_HEADERWINDOW	= 1001
	};
	enum {
		WM_MESSAGEMODEL_MESSAGECHANGED	= WM_APP + 1201
	};

public:
	typedef std::vector<MessageWindowHandler*> HandlerList;

public:
	QSTATUS layoutChildren();
	QSTATUS layoutChildren(int cx, int cy);
	QSTATUS setMessage(MessageHolder* pmh, bool bResetEncoding);

public:
	virtual QSTATUS messageChanged(const MessageModelEvent& event);

private:
	QSTATUS fireMessageChanged(MessageHolder* pmh,
		Message& msg, const ContentTypeParser* pContentType) const;

public:
	MessageWindow* pThis_;
	
	bool bShowHeaderWindow_;
	bool bRawMode_;
	bool bHtmlMode_;
	bool bHtmlOnlineMode_;
	bool bDecryptVerifyMode_;
	
	Profile* pProfile_;
	const WCHAR* pwszSection_;
	Accelerator* pAccelerator_;
	Document* pDocument_;
	HeaderWindow* pHeaderWindow_;
	MessageViewWindow* pMessageViewWindow_;
	bool bCreated_;
	
	MessageModel* pMessageModel_;
	MessageViewWindowFactory* pFactory_;
	
	WSTRING wstrEncoding_;
	WSTRING wstrTemplate_;
	bool bSelectMode_;
	
	HandlerList listHandler_;
};

QSTATUS qm::MessageWindowImpl::layoutChildren()
{
	RECT rect;
	pThis_->getClientRect(&rect);
	return layoutChildren(rect.right - rect.left, rect.bottom - rect.top);
}

QSTATUS qm::MessageWindowImpl::layoutChildren(int cx, int cy)
{
	DECLARE_QSTATUS();
	
	int nHeaderHeight = 0;
	if (bShowHeaderWindow_) {
		status = pHeaderWindow_->layout();
		CHECK_QSTATUS();
		nHeaderHeight = pHeaderWindow_->getHeight();
	}
	
	pHeaderWindow_->setWindowPos(0, 0, 0, cx, nHeaderHeight, SWP_NOZORDER);
	pHeaderWindow_->showWindow(bShowHeaderWindow_ ? SW_SHOW : SW_HIDE);
	
	if (pMessageViewWindow_) {
		int nY = bShowHeaderWindow_ ? nHeaderHeight : 0;
		int nHeight = cy > nY ? cy - nY : 0;
		pMessageViewWindow_->getWindow().setWindowPos(
			HWND_TOP, 0, nY, cx, nHeight, 0);
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MessageWindowImpl::setMessage(MessageHolder* pmh, bool bResetEncoding)
{
	DECLARE_QSTATUS();
	
	bool bActive = pThis_->isActive();
	
	if (bResetEncoding) {
		freeWString(wstrEncoding_);
		wstrEncoding_ = 0;
	}
	
	Account* pAccount = pMessageModel_->getCurrentAccount();
	assert(!pmh || pmh->getFolder()->getAccount() == pAccount);
	
	Message msg(&status);
	CHECK_QSTATUS();
	if (pmh) {
		unsigned int nFlags = Account::GETMESSAGEFLAG_MAKESEEN |
			(bRawMode_ ? Account::GETMESSAGEFLAG_ALL :
				bHtmlMode_ ? Account::GETMESSAGEFLAG_HTML :
				Account::GETMESSAGEFLAG_TEXT);
		status = pmh->getMessage(nFlags, 0, &msg);
		CHECK_QSTATUS();
		
		unsigned int nSecurity = Message::SECURITY_NONE;
		const Security* pSecurity = pDocument_->getSecurity();
		if (Security::isEnabled() && bDecryptVerifyMode_) {
			const SMIMEUtility* pSMIMEUtility = pSecurity->getSMIMEUtility();
			SMIMEUtility::Type type = pSMIMEUtility->getType(msg);
			if  (type != SMIMEUtility::TYPE_NONE) {
				if ((nFlags & Account::GETMESSAGEFLAG_METHOD_MASK) !=
					Account::GETMESSAGEFLAG_ALL) {
					msg.clear();
					status = pmh->getMessage(Account::GETMESSAGEFLAG_ALL, 0, &msg);
					CHECK_QSTATUS();
				}
			}
			
			while  (type != SMIMEUtility::TYPE_NONE) {
				string_ptr<STRING> strMessage;
				switch (type) {
				case SMIMEUtility::TYPE_SIGNED:
				case SMIMEUtility::TYPE_MULTIPARTSIGNED:
					status = pSMIMEUtility->verify(msg,
						pSecurity->getCA(), &strMessage);
					CHECK_QSTATUS();
					nSecurity |= Message::SECURITY_VERIFIED;
					break;
				case SMIMEUtility::TYPE_ENVELOPED:
					{
						SubAccount* pSubAccount = pAccount->getCurrentSubAccount();
						PrivateKey* pPrivateKey = pSubAccount->getPrivateKey();
						Certificate* pCertificate = pSubAccount->getCertificate();
						if (pPrivateKey && pCertificate) {
							status = pSMIMEUtility->decrypt(msg,
								pPrivateKey, pCertificate, &strMessage);
							CHECK_QSTATUS();
							nSecurity |= Message::SECURITY_DECRYPTED;
						}
					}
					break;
				default:
					break;
				}
				
				if (!strMessage.get())
					break;
				
				status = msg.create(strMessage.get(), -1,
					Message::FLAG_NONE, nSecurity);
				CHECK_QSTATUS();
				type = pSMIMEUtility->getType(msg);
			}
		}
	}
	
	const ContentTypeParser* pContentType = 0;
	MessageViewWindow* pMessageViewWindow = 0;
	if (pmh && !bRawMode_ && bHtmlMode_ && !wstrTemplate_) {
		PartUtil util(msg);
		PartUtil::ContentTypeList l;
		status = util.getAlternativeContentTypes(&l);
		CHECK_QSTATUS();
		
		PartUtil::ContentTypeList::iterator it = std::find_if(
			l.begin(), l.end(),
			std::bind1st(
				std::mem_fun(
					MessageViewWindowFactory::isSupported),
				pFactory_));
		pContentType = it != l.end() ? *it : 0;
		
		status = pFactory_->getMessageViewWindow(
			pContentType, &pMessageViewWindow);
		CHECK_QSTATUS();
	}
	else {
		pMessageViewWindow = pFactory_->getTextMessageViewWindow();
	}
	
	bool bLayout = false;
	if (pMessageViewWindow_ != pMessageViewWindow) {
		if (pMessageViewWindow_)
			pMessageViewWindow_->getWindow().showWindow(SW_HIDE);
		pMessageViewWindow->getWindow().showWindow(SW_SHOW);
		pMessageViewWindow_ = pMessageViewWindow;
		bLayout = true;
	}
	
	if (bShowHeaderWindow_) {
		if (pAccount) {
			TemplateContext context(pmh, &msg, pAccount,
				pDocument_, pThis_->getHandle(), pProfile_, 0,
				TemplateContext::ArgumentList(), &status);
			CHECK_QSTATUS();
			status = pHeaderWindow_->setMessage(&context);
			CHECK_QSTATUS();
		}
		else {
			status = pHeaderWindow_->setMessage(0);
			CHECK_QSTATUS();
		}
		bLayout = true;
	}
	
	if (bLayout) {
		status = layoutChildren();
		CHECK_QSTATUS();
	}
	
	const Template* pTemplate = 0;
	if (pmh && wstrTemplate_) {
		status = pDocument_->getTemplateManager()->getTemplate(
			pAccount, pmh->getFolder(), wstrTemplate_, &pTemplate);
		CHECK_QSTATUS();
	}
	
	unsigned int nFlags = (bRawMode_ ? MessageViewWindow::FLAG_RAWMODE : 0) |
		(!bShowHeaderWindow_ ? MessageViewWindow::FLAG_INCLUDEHEADER : 0) |
		(bHtmlOnlineMode_ ? MessageViewWindow::FLAG_ONLINEMODE : 0);
	status = pMessageViewWindow->setMessage(pmh,
		pmh ? &msg : 0, pTemplate, wstrEncoding_, nFlags);
	CHECK_QSTATUS();
	if (bActive) {
		status = pThis_->setActive();
		CHECK_QSTATUS();
	}
	
	status = fireMessageChanged(pmh, msg, pContentType);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MessageWindowImpl::messageChanged(const MessageModelEvent& event)
{
	MessageHolder* pmh = event.getMessageHolder();
	if (pmh) {
		return setMessage(pmh, true);
	}
	else {
		pThis_->postMessage(WM_MESSAGEMODEL_MESSAGECHANGED, 0,
			reinterpret_cast<LPARAM>(event.getMessageHolder()));
		return QSTATUS_SUCCESS;
	}
}

QSTATUS qm::MessageWindowImpl::fireMessageChanged(MessageHolder* pmh,
	Message& msg, const ContentTypeParser* pContentType) const
{
	DECLARE_QSTATUS();
	
	MessageWindowEvent event(pmh, msg, pContentType);
	
	HandlerList::const_iterator it = listHandler_.begin();
	while (it != listHandler_.end()) {
		status = (*it)->messageChanged(event);
		CHECK_QSTATUS();
		++it;
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * MessageWindow
 *
 */

qm::MessageWindow::MessageWindow(MessageModel* pMessageModel,
	Profile* pProfile, const WCHAR* pwszSection, QSTATUS* pstatus) :
	WindowBase(true, pstatus),
	DefaultWindowHandler(pstatus),
	pImpl_(0)
{
	if (*pstatus != QSTATUS_SUCCESS)
		return;
	
	DECLARE_QSTATUS();
	
	int nShowHeaderWindow = 0;
	status = pProfile->getInt(pwszSection, L"ShowHeaderWindow",
		1, &nShowHeaderWindow);
	CHECK_QSTATUS_SET(pstatus);
	
	int nHtmlMode = 0;
	status = pProfile->getInt(pwszSection, L"HtmlMode", 0, &nHtmlMode);
	CHECK_QSTATUS_SET(pstatus);
	int nHtmlOnlineMode = 0;
	status = pProfile->getInt(pwszSection, L"HtmlOnlineMode", 0, &nHtmlOnlineMode);
	CHECK_QSTATUS_SET(pstatus);
	int nDecryptVerifyMode = 0;
	status = pProfile->getInt(pwszSection,
		L"DecryptVerifyMode", 0, &nDecryptVerifyMode);
	CHECK_QSTATUS_SET(pstatus);
	string_ptr<WSTRING> wstrTemplate;
	status = pProfile->getString(pwszSection, L"Template", L"", &wstrTemplate);
	CHECK_QSTATUS_SET(pstatus);
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	pImpl_->pThis_ = this;
	pImpl_->bShowHeaderWindow_ = nShowHeaderWindow != 0;
	pImpl_->bRawMode_ = false;
	pImpl_->bHtmlMode_ = nHtmlMode != 0;
	pImpl_->bHtmlOnlineMode_ = nHtmlOnlineMode != 0;
	pImpl_->bDecryptVerifyMode_ = nDecryptVerifyMode != 0;
	pImpl_->pProfile_ = pProfile;
	pImpl_->pwszSection_ = pwszSection;
	pImpl_->pAccelerator_ = 0;
	pImpl_->pDocument_ = 0;
	pImpl_->pHeaderWindow_ = 0;
	pImpl_->pMessageViewWindow_ = 0;
	pImpl_->bCreated_ = false;
	pImpl_->pMessageModel_ = pMessageModel;
	pImpl_->pFactory_ = 0;
	pImpl_->wstrEncoding_ = 0;
	pImpl_->wstrTemplate_ = *wstrTemplate.get() ? wstrTemplate.release() : 0;
	pImpl_->bSelectMode_ = false;
	
	status = pImpl_->pMessageModel_->addMessageModelHandler(pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	
	setWindowHandler(this, false);
}

qm::MessageWindow::~MessageWindow()
{
	if (pImpl_) {
		delete pImpl_->pFactory_;
		delete pImpl_->pAccelerator_;
		freeWString(pImpl_->wstrEncoding_);
		freeWString(pImpl_->wstrTemplate_);
		delete pImpl_;
		pImpl_ = 0;
	}
}

bool qm::MessageWindow::isShowHeaderWindow() const
{
	return pImpl_->bShowHeaderWindow_;
}

QSTATUS qm::MessageWindow::setShowHeaderWindow(bool bShow)
{
	DECLARE_QSTATUS();
	if (bShow != pImpl_->bShowHeaderWindow_) {
		pImpl_->bShowHeaderWindow_ = bShow;
		status = pImpl_->layoutChildren();
	}
	return status;
}

bool qm::MessageWindow::isRawMode() const
{
	return pImpl_->bRawMode_;
}

QSTATUS qm::MessageWindow::setRawMode(bool bRawMode)
{
	DECLARE_QSTATUS();
	
	if (bRawMode != pImpl_->bRawMode_) {
		pImpl_->bRawMode_ = bRawMode;
		MessagePtrLock mpl(pImpl_->pMessageModel_->getCurrentMessage());
		status = pImpl_->setMessage(mpl, false);
		CHECK_QSTATUS();
	}
	return QSTATUS_SUCCESS;
}

bool qm::MessageWindow::isHtmlMode() const
{
	return pImpl_->bHtmlMode_;
}

QSTATUS qm::MessageWindow::setHtmlMode(bool bHtmlMode)
{
	DECLARE_QSTATUS();
	
	if (bHtmlMode != pImpl_->bHtmlMode_) {
		pImpl_->bHtmlMode_ = bHtmlMode;
		MessagePtrLock mpl(pImpl_->pMessageModel_->getCurrentMessage());
		status = pImpl_->setMessage(mpl, false);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

bool qm::MessageWindow::isHtmlOnlineMode() const
{
	return pImpl_->bHtmlOnlineMode_;
}

QSTATUS qm::MessageWindow::setHtmlOnlineMode(bool bHtmlOnlineMode)
{
	DECLARE_QSTATUS();
	
	if (bHtmlOnlineMode != pImpl_->bHtmlOnlineMode_) {
		pImpl_->bHtmlOnlineMode_ = bHtmlOnlineMode;
		MessagePtrLock mpl(pImpl_->pMessageModel_->getCurrentMessage());
		status = pImpl_->setMessage(mpl, false);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

bool qm::MessageWindow::isDecryptVerifyMode() const
{
	return pImpl_->bDecryptVerifyMode_;
}

QSTATUS qm::MessageWindow::setDecryptVerifyMode(bool bDecryptVerifyMode)
{
	DECLARE_QSTATUS();
	
	if (bDecryptVerifyMode != pImpl_->bDecryptVerifyMode_) {
		pImpl_->bDecryptVerifyMode_ = bDecryptVerifyMode;
		MessagePtrLock mpl(pImpl_->pMessageModel_->getCurrentMessage());
		status = pImpl_->setMessage(mpl, false);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

const WCHAR* qm::MessageWindow::getEncoding() const
{
	return pImpl_->wstrEncoding_;
}

QSTATUS qm::MessageWindow::setEncoding(const WCHAR* pwszEncoding)
{
	DECLARE_QSTATUS();
	
	if (!((pwszEncoding && pImpl_->wstrEncoding_ &&
		wcscmp(pwszEncoding, pImpl_->wstrEncoding_) == 0) ||
		(!pwszEncoding && !pImpl_->wstrEncoding_))) {
		string_ptr<WSTRING> wstrEncoding;
		if (pwszEncoding) {
			wstrEncoding.reset(allocWString(pwszEncoding));
			if (!wstrEncoding.get())
				return QSTATUS_OUTOFMEMORY;
		}
		
		freeWString(pImpl_->wstrEncoding_);
		pImpl_->wstrEncoding_ = wstrEncoding.release();
		
		MessagePtrLock mpl(pImpl_->pMessageModel_->getCurrentMessage());
		status = pImpl_->setMessage(mpl, false);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

const WCHAR* qm::MessageWindow::getTemplate() const
{
	return pImpl_->wstrTemplate_;
}

QSTATUS qm::MessageWindow::setTemplate(const WCHAR* pwszTemplate)
{
	DECLARE_QSTATUS();
	
	if (!((pwszTemplate && pImpl_->wstrTemplate_ &&
		wcscmp(pwszTemplate, pImpl_->wstrTemplate_) == 0) ||
		(!pwszTemplate && !pImpl_->wstrTemplate_))) {
		string_ptr<WSTRING> wstrTemplate;
		if (pwszTemplate) {
			wstrTemplate.reset(allocWString(pwszTemplate));
			if (!wstrTemplate.get())
				return QSTATUS_OUTOFMEMORY;
		}
		
		freeWString(pImpl_->wstrTemplate_);
		pImpl_->wstrTemplate_ = wstrTemplate.release();
		
		MessagePtrLock mpl(pImpl_->pMessageModel_->getCurrentMessage());
		status = pImpl_->setMessage(mpl, false);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MessageWindow::scrollPage(bool bPrev, bool* pbScrolled)
{
	return pImpl_->pMessageViewWindow_->scrollPage(bPrev, pbScrolled);
}

bool qm::MessageWindow::isSelectMode() const
{
	return pImpl_->bSelectMode_;
}

QSTATUS qm::MessageWindow::setSelectMode(bool bSelectMode)
{
	DECLARE_QSTATUS();
	
	if (pImpl_->bSelectMode_ != bSelectMode) {
		status = pImpl_->pMessageViewWindow_->setSelectMode(bSelectMode);
		CHECK_QSTATUS();
		pImpl_->bSelectMode_ = bSelectMode;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MessageWindow::find(const WCHAR* pwszFind,
	bool bMatchCase, bool bPrev, bool* pbFound)
{
	return pImpl_->pMessageViewWindow_->find(
		pwszFind, bMatchCase, bPrev, pbFound);
}

QSTATUS qm::MessageWindow::openLink()
{
	return pImpl_->pMessageViewWindow_->openLink();
}

MessageWindowItem* qm::MessageWindow::getFocusedItem() const
{
	if (pImpl_->pMessageViewWindow_ &&
		pImpl_->pMessageViewWindow_->isActive())
		return pImpl_->pMessageViewWindow_;
	else
		return pImpl_->pHeaderWindow_->getFocusedItem();
}

MessageModel* qm::MessageWindow::getMessageModel() const
{
	return pImpl_->pMessageModel_;
}

AttachmentSelectionModel* qm::MessageWindow::getAttachmentSelectionModel() const
{
	return pImpl_->pHeaderWindow_->getAttachmentSelectionModel();
}

QSTATUS qm::MessageWindow::save()
{
	DECLARE_QSTATUS();
	
	Profile* pProfile = pImpl_->pProfile_;
	
	status = pProfile->setInt(pImpl_->pwszSection_,
		L"ShowHeaderWindow", pImpl_->bShowHeaderWindow_);
	CHECK_QSTATUS();
	status = pProfile->setInt(pImpl_->pwszSection_,
		L"HtmlMode", pImpl_->bHtmlMode_);
	CHECK_QSTATUS();
	status = pProfile->setInt(pImpl_->pwszSection_,
		L"HtmlOnlineMode", pImpl_->bHtmlOnlineMode_);
	CHECK_QSTATUS();
	status = pProfile->setInt(pImpl_->pwszSection_,
		L"DecryptVerifyMode", pImpl_->bDecryptVerifyMode_);
	CHECK_QSTATUS();
	status = pProfile->setString(pImpl_->pwszSection_,
		L"Template", pImpl_->wstrTemplate_ ? pImpl_->wstrTemplate_ : L"");
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MessageWindow::addMessageWindowHandler(MessageWindowHandler* pHandler)
{
	return STLWrapper<MessageWindowImpl::HandlerList>(
		pImpl_->listHandler_).push_back(pHandler);
}

QSTATUS qm::MessageWindow::removeMessageWindowHandler(MessageWindowHandler* pHandler)
{
	MessageWindowImpl::HandlerList::iterator it = std::remove(
		pImpl_->listHandler_.begin(), pImpl_->listHandler_.end(), pHandler);
	pImpl_->listHandler_.erase(it, pImpl_->listHandler_.end());
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MessageWindow::getAccelerator(Accelerator** ppAccelerator)
{
	assert(ppAccelerator);
	*ppAccelerator = pImpl_->pAccelerator_;
	return QSTATUS_SUCCESS;
}

LRESULT qm::MessageWindow::windowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	BEGIN_MESSAGE_HANDLER()
		HANDLE_CREATE()
		HANDLE_DESTROY()
		HANDLE_LBUTTONDOWN()
		HANDLE_SIZE()
		HANDLE_MESSAGE(MessageWindowImpl::WM_MESSAGEMODEL_MESSAGECHANGED, onMessageModelMessageChanged)
	END_MESSAGE_HANDLER()
	return DefaultWindowHandler::windowProc(uMsg, wParam, lParam);
}

LRESULT qm::MessageWindow::onCreate(CREATESTRUCT* pCreateStruct)
{
	if (DefaultWindowHandler::onCreate(pCreateStruct) == -1)
		return -1;
	
	DECLARE_QSTATUS();
	
	MessageWindowCreateContext* pContext =
		static_cast<MessageWindowCreateContext*>(pCreateStruct->lpCreateParams);
	pImpl_->pDocument_ = pContext->pDocument_;
	
	status = pContext->pKeyMap_->createAccelerator(
		CustomAcceleratorFactory(), pImpl_->pwszSection_,
		mapKeyNameToId, countof(mapKeyNameToId), &pImpl_->pAccelerator_);
	CHECK_QSTATUS_VALUE(-1);
	
	std::auto_ptr<HeaderWindow> pHeaderWindow;
	status = newQsObject(pImpl_->pProfile_, &pHeaderWindow);
	CHECK_QSTATUS_VALUE(-1);
	HeaderWindowCreateContext context = {
		pContext->pDocument_,
		pContext->pMenuManager_,
	};
	status = pHeaderWindow->create(L"QmHeaderWindow", 0, WS_VISIBLE | WS_CHILD,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		getHandle(), 0, 0, MessageWindowImpl::ID_HEADERWINDOW, &context);
	CHECK_QSTATUS_VALUE(-1);
	pImpl_->pHeaderWindow_ = pHeaderWindow.release();
	
	std::auto_ptr<MessageViewWindowFactory> pFactory;
	status = newQsObject(pImpl_->pDocument_, pImpl_->pProfile_,
		pImpl_->pwszSection_, pContext->pMenuManager_, false, &pFactory);
	CHECK_QSTATUS_VALUE(-1);
	pImpl_->pFactory_ = pFactory.release();
	
	status = pImpl_->pFactory_->create(getHandle());
	CHECK_QSTATUS_VALUE(-1);
	pImpl_->pMessageViewWindow_ = pImpl_->pFactory_->getTextMessageViewWindow();
	status = pImpl_->layoutChildren();
	CHECK_QSTATUS_VALUE(-1);
	pImpl_->pMessageViewWindow_->getWindow().showWindow(SW_SHOW);
	
	pImpl_->bCreated_ = true;
	
	return 0;
}

LRESULT qm::MessageWindow::onDestroy()
{
	pImpl_->pMessageModel_->removeMessageModelHandler(pImpl_);
	return DefaultWindowHandler::onDestroy();
}

LRESULT qm::MessageWindow::onLButtonDown(UINT nFlags, const POINT& pt)
{
	setFocus();
	return DefaultWindowHandler::onLButtonDown(nFlags, pt);
}

LRESULT qm::MessageWindow::onSize(UINT nFlags, int cx, int cy)
{
	if (pImpl_->bCreated_)
		pImpl_->layoutChildren(cx, cy);
	return DefaultWindowHandler::onSize(nFlags, cx, cy);
}

LRESULT qm::MessageWindow::onMessageModelMessageChanged(
	WPARAM wParam, LPARAM lParam)
{
	pImpl_->setMessage(reinterpret_cast<MessageHolder*>(lParam), true);
	return 0;
}

bool qm::MessageWindow::isShow() const
{
	return isVisible();
}

bool qm::MessageWindow::isActive() const
{
	return hasFocus() ||
		(pImpl_->pMessageViewWindow_ &&
			pImpl_->pMessageViewWindow_->isActive()) ||
		pImpl_->pHeaderWindow_->isActive();
}

QSTATUS qm::MessageWindow::setActive()
{
	if (pImpl_->pMessageViewWindow_)
		pImpl_->pMessageViewWindow_->setActive();
	else
		setFocus();
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * MessageWindowHandler
 *
 */

qm::MessageWindowHandler::~MessageWindowHandler()
{
}


/****************************************************************************
 *
 * MessageWindowEvent
 *
 */

qm::MessageWindowEvent::MessageWindowEvent(MessageHolder* pmh,
	Message& msg, const ContentTypeParser* pContentType) :
	pmh_(pmh),
	msg_(msg),
	pContentType_(pContentType)
{
}

qm::MessageWindowEvent::~MessageWindowEvent()
{
}

MessageHolder* qm::MessageWindowEvent::getMessageHolder() const
{
	return pmh_;
}

Message& qm::MessageWindowEvent::getMessage() const
{
	return msg_;
}

const ContentTypeParser* qm::MessageWindowEvent::getContentType() const
{
	return pContentType_;
}


/****************************************************************************
 *
 * MessageWindowItem
 *
 */

qm::MessageWindowItem::~MessageWindowItem()
{
}
