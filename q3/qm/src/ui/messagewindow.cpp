/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
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
#include <qskeymap.h>
#include <qsprofile.h>

#include "encodingmodel.h"
#include "headerwindow.h"
#include "keymap.h"
#include "messagemodel.h"
#include "messageviewmode.h"
#include "messageviewwindow.h"
#include "messagewindow.h"
#include "resourceinc.h"
#include "securitymodel.h"
#include "uimanager.h"
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
	public AbstractMessageViewMode,
	public MessageViewModeHolder,
	public MessageModelHandler,
	public MessageViewModeHandler,
	public MessageViewModeHolderHandler,
	public EncodingModelHandler,
	public SecurityModelHandler,
	public MessageViewWindowCallback
{
public:
	enum {
		ID_HEADERWINDOW	= 1001
	};
	enum {
		WM_MESSAGEMODEL_MESSAGECHANGED	= WM_APP + 1201
	};
	enum {
		TIMER_MAKESEEN	= 20
	};

public:
	typedef std::vector<MessageWindowHandler*> HandlerList;

public:
	void layoutChildren();
	void layoutChildren(int cx,
						int cy);
	bool setMessage(MessageHolder* pmh,
					bool bResetEncoding);

public:
	virtual bool isMode(Mode mode) const;
	virtual void setMode(Mode mode,
						 bool b);

public:
	virtual MessageViewMode* getMessageViewMode();
	virtual void addMessageViewModeHolderHandler(MessageViewModeHolderHandler* pHandler);
	virtual void removeMessageViewModeHolderHandler(MessageViewModeHolderHandler* pHandler);

public:
	virtual void messageChanged(const MessageModelEvent& event);

public:
	virtual void messageViewModeChanged(const MessageViewModeEvent& event);

public:
	virtual void messageViewModeChanged(const MessageViewModeHolderEvent& event);

public:
	virtual void encodingChanged(const EncodingModelEvent& event);

public:
	virtual void securityModeChanged(const SecurityModelEvent& event);

public:
	virtual void statusTextChanged(const WCHAR* pwszText);

private:
	void fireMessageChanged(MessageHolder* pmh,
							Message& msg,
							const ContentTypeParser* pContentType) const;
	void fireStatusTextChanged(const WCHAR* pwszText) const;

public:
	MessageWindow* pThis_;
	
	bool bShowHeaderWindow_;
	
	Profile* pProfile_;
	const WCHAR* pwszSection_;
	std::auto_ptr<Accelerator> pAccelerator_;
	Document* pDocument_;
	MessageViewModeHolder* pMessageViewModeHolder_;
	EncodingModel* pEncodingModel_;
	SecurityModel* pSecurityModel_;
	HeaderWindow* pHeaderWindow_;
	MessageViewWindow* pMessageViewWindow_;
	bool bCreated_;
	bool bLayouting_;
	unsigned int nSeenTimerId_;
	
	MessageModel* pMessageModel_;
	std::auto_ptr<MessageViewWindowFactory> pFactory_;
	
	unsigned int nMode_;
	wstring_ptr wstrTemplate_;
	unsigned int nSeenWait_;
	
	HandlerList listHandler_;
};

void qm::MessageWindowImpl::layoutChildren()
{
	RECT rect;
	pThis_->getClientRect(&rect);
	layoutChildren(rect.right - rect.left, rect.bottom - rect.top);
}

void qm::MessageWindowImpl::layoutChildren(int cx,
										   int cy)
{
	bLayouting_ = true;
	
	int nHeaderHeight = 0;
	if (bShowHeaderWindow_) {
		pHeaderWindow_->layout(Rect(0, 0, cx, cy));
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
	
	bLayouting_ = false;
}

bool qm::MessageWindowImpl::setMessage(MessageHolder* pmh,
									   bool bResetEncoding)
{
	bool bActive = pThis_->isActive();
	
	if (bResetEncoding)
		pEncodingModel_->setEncoding(0);
	
	Account* pAccount = pMessageModel_->getCurrentAccount();
	assert(!pmh || pmh->getFolder()->getAccount() == pAccount);
	
	if (nSeenTimerId_ != 0)
		pThis_->killTimer(nSeenTimerId_);
	
	MessageViewMode* pMode = pMessageViewModeHolder_->getMessageViewMode();
	bool bRawMode = pMode && pMode->isMode(MessageViewMode::MODE_RAW);
	bool bHtmlMode = pMode && pMode->isMode(MessageViewMode::MODE_HTML);
	
	Message msg;
	if (pmh) {
		unsigned int nFlags = 0;
		if (nSeenWait_ == 0)
			nFlags |= Account::GETMESSAGEFLAG_MAKESEEN;
		if (bRawMode)
			nFlags |= Account::GETMESSAGEFLAG_ALL;
		else if (bHtmlMode)
			nFlags |= Account::GETMESSAGEFLAG_HTML;
		else
			nFlags |= Account::GETMESSAGEFLAG_TEXT;
		if (!pmh->getMessage(nFlags, 0, pSecurityModel_->getSecurityMode(), &msg))
			return false;
		
		if (nSeenWait_ != 0 && nSeenWait_ != -1)
			nSeenTimerId_ = pThis_->setTimer(TIMER_MAKESEEN, nSeenWait_*1000);
	}
	
	const ContentTypeParser* pContentType = 0;
	MessageViewWindow* pMessageViewWindow = 0;
	if (pmh && !bRawMode && bHtmlMode && !wstrTemplate_.get()) {
		if (pAccount->isSupport(Account::SUPPORT_EXTERNALLINK) &&
			!msg.isMultipart() &&
			msg.hasField(L"X-QMAIL-Link")) {
			pMessageViewWindow = pFactory_->getLinkMessageViewWindow();
		}
		else {
			PartUtil util(msg);
			PartUtil::ContentTypeList l;
			util.getAlternativeContentTypes(&l);
			
			PartUtil::ContentTypeList::iterator it = std::find_if(
				l.begin(), l.end(),
				std::bind1st(
					std::mem_fun(
						MessageViewWindowFactory::isSupported),
					pFactory_.get()));
			pContentType = it != l.end() ? *it : 0;
			
			pMessageViewWindow = pFactory_->getMessageViewWindow(pContentType);
		}
	}
	else {
		pMessageViewWindow = pFactory_->getTextMessageViewWindow();
	}
	
	bool bLayout = false;
	if (pMessageViewWindow_ != pMessageViewWindow) {
		if (pMessageViewWindow_)
			pMessageViewWindow_->getWindow().showWindow(SW_HIDE);
		pMessageViewWindow->getWindow().showWindow(SW_SHOW);
		pMessageViewWindow->setSelectMode(pMode && pMode->isMode(MessageViewMode::MODE_SELECT));
		pMessageViewWindow_ = pMessageViewWindow;
		bLayout = true;
	}
	
	if (bShowHeaderWindow_) {
		if (pAccount) {
			// TODO
			// Get selected
			TemplateContext context(pmh, &msg, MessageHolderList(), pAccount,
				pDocument_, pThis_->getHandle(), pEncodingModel_->getEncoding(),
				pSecurityModel_->getSecurityMode(),
				pProfile_, 0, TemplateContext::ArgumentList());
			pHeaderWindow_->setMessage(&context);
		}
		else {
			pHeaderWindow_->setMessage(0);
		}
		bLayout = true;
	}
	
	if (bLayout)
		layoutChildren();
	
	const Template* pTemplate = 0;
	if (pmh && wstrTemplate_.get())
		pTemplate = pDocument_->getTemplateManager()->getTemplate(
			pAccount, pmh->getFolder(), wstrTemplate_.get());
	
	unsigned int nFlags = (bRawMode ? MessageViewWindow::FLAG_RAWMODE : 0) |
		(!bShowHeaderWindow_ ? MessageViewWindow::FLAG_INCLUDEHEADER : 0) |
		(pMode && pMode->isMode(MessageViewMode::MODE_HTMLONLINE) ? MessageViewWindow::FLAG_ONLINEMODE : 0);
	if (!pMessageViewWindow->setMessage(pmh, pmh ? &msg : 0,
		pTemplate, pEncodingModel_->getEncoding(), nFlags, pSecurityModel_->getSecurityMode()))
		return false;
	if (bActive)
		pThis_->setActive();
	
	fireMessageChanged(pmh, msg, pContentType);
	
	return true;
}

bool qm::MessageWindowImpl::isMode(Mode mode) const
{
	return (nMode_ & mode) != 0;
}

void qm::MessageWindowImpl::setMode(Mode mode,
									bool b)
{
	unsigned int nMode = nMode_;
	if (b)
		nMode_ |= mode;
	else
		nMode_ &= ~mode;
	
	if (nMode_ != nMode)
		fireMessageViewModeChanged(mode, b);
}

MessageViewMode* qm::MessageWindowImpl::getMessageViewMode()
{
	return this;
}

void qm::MessageWindowImpl::addMessageViewModeHolderHandler(MessageViewModeHolderHandler* pHandler)
{
}

void qm::MessageWindowImpl::removeMessageViewModeHolderHandler(MessageViewModeHolderHandler* pHandler)
{
}

void qm::MessageWindowImpl::messageChanged(const MessageModelEvent& event)
{
	MessageHolder* pmh = event.getMessageHolder();
	if (pmh)
		setMessage(pmh, true);
	else
		pThis_->postMessage(WM_MESSAGEMODEL_MESSAGECHANGED, 0,
			reinterpret_cast<LPARAM>(event.getMessageHolder()));
}

void qm::MessageWindowImpl::messageViewModeChanged(const MessageViewModeEvent& event)
{
	MessageViewMode::Mode mode = event.getMode();
	bool b = event.isSet();
	switch (mode) {
	case MessageViewMode::MODE_RAW:
	case MessageViewMode::MODE_HTML:
	case MessageViewMode::MODE_HTMLONLINE:
		{
			MessagePtrLock mpl(pMessageModel_->getCurrentMessage());
			setMessage(mpl, false);
		}
		break;
	case MessageViewMode::MODE_SELECT:
		pMessageViewWindow_->setSelectMode(b);
		break;
	case MessageViewMode::MODE_QUOTE:
		pMessageViewWindow_->setQuoteMode(b);
		break;
	default:
		break;
	}
}

void qm::MessageWindowImpl::messageViewModeChanged(const MessageViewModeHolderEvent& event)
{
	MessageViewMode* pOld = event.getOldMessageViewMode();
	if (pOld)
		pOld->removeMessageViewModeHandler(this);
	
	MessageViewMode* pNew = event.getNewMessageViewMode();
	if (pNew)
		pNew->addMessageViewModeHandler(this);
	
	pMessageViewWindow_->setSelectMode(pNew && pNew->isMode(MessageViewMode::MODE_SELECT));
	pMessageViewWindow_->setQuoteMode(pNew && pNew->isMode(MessageViewMode::MODE_QUOTE));
}

void qm::MessageWindowImpl::encodingChanged(const EncodingModelEvent& event)
{
	MessagePtrLock mpl(pMessageModel_->getCurrentMessage());
	setMessage(mpl, false);
}

void qm::MessageWindowImpl::securityModeChanged(const SecurityModelEvent& event)
{
	MessagePtrLock mpl(pMessageModel_->getCurrentMessage());
	setMessage(mpl, false);
}

void qm::MessageWindowImpl::statusTextChanged(const WCHAR* pwszText)
{
	fireStatusTextChanged(pwszText);
}

void qm::MessageWindowImpl::fireMessageChanged(MessageHolder* pmh,
											   Message& msg,
											   const ContentTypeParser* pContentType) const
{
	MessageWindowEvent event(pmh, msg, pContentType);
	for (HandlerList::const_iterator it = listHandler_.begin(); it != listHandler_.end(); ++it)
		(*it)->messageChanged(event);
}

void qm::MessageWindowImpl::fireStatusTextChanged(const WCHAR* pwszText) const
{
	MessageWindowStatusTextEvent event(pwszText);
	for (HandlerList::const_iterator it = listHandler_.begin(); it != listHandler_.end(); ++it)
		(*it)->statusTextChanged(event);
}


/****************************************************************************
 *
 * MessageWindow
 *
 */

qm::MessageWindow::MessageWindow(MessageModel* pMessageModel,
								 Profile* pProfile,
								 const WCHAR* pwszSection) :
	WindowBase(true),
	pImpl_(0)
{
	wstring_ptr wstrTemplate(pProfile->getString(pwszSection, L"Template", L""));
	
	pImpl_ = new MessageWindowImpl();
	pImpl_->pThis_ = this;
	pImpl_->bShowHeaderWindow_ = pProfile->getInt(pwszSection, L"ShowHeaderWindow", 1) != 0;
	pImpl_->pProfile_ = pProfile;
	pImpl_->pwszSection_ = pwszSection;
	pImpl_->pDocument_ = 0;
	pImpl_->pHeaderWindow_ = 0;
	pImpl_->pMessageViewWindow_ = 0;
	pImpl_->bCreated_ = false;
	pImpl_->bLayouting_ = false;
	pImpl_->nSeenTimerId_ = 0;
	pImpl_->pMessageModel_ = pMessageModel;
	pImpl_->nMode_ = pProfile->getInt(pwszSection, L"ViewMode", MessageViewMode::MODE_QUOTE);
	pImpl_->wstrTemplate_ = *wstrTemplate.get() ? wstrTemplate : 0;
	pImpl_->nSeenWait_ = pProfile->getInt(pwszSection, L"SeenWait", 0);
	
	pImpl_->pMessageModel_->addMessageModelHandler(pImpl_);
	
	setWindowHandler(this, false);
}

qm::MessageWindow::~MessageWindow()
{
	delete pImpl_;
}

bool qm::MessageWindow::isShowHeaderWindow() const
{
	return pImpl_->bShowHeaderWindow_;
}

void qm::MessageWindow::setShowHeaderWindow(bool bShow)
{
	if (bShow != pImpl_->bShowHeaderWindow_) {
		pImpl_->bShowHeaderWindow_ = bShow;
		pImpl_->layoutChildren();
	}
}

const WCHAR* qm::MessageWindow::getTemplate() const
{
	return pImpl_->wstrTemplate_.get();
}

void qm::MessageWindow::setTemplate(const WCHAR* pwszTemplate)
{
	if (!((pwszTemplate && pImpl_->wstrTemplate_.get() &&
		wcscmp(pwszTemplate, pImpl_->wstrTemplate_.get()) == 0) ||
		(!pwszTemplate && !pImpl_->wstrTemplate_.get()))) {
		if (pwszTemplate)
			pImpl_->wstrTemplate_ = allocWString(pwszTemplate);
		else
			pImpl_->wstrTemplate_.reset(0);
		
		MessagePtrLock mpl(pImpl_->pMessageModel_->getCurrentMessage());
		pImpl_->setMessage(mpl, false);
	}
}

bool qm::MessageWindow::scrollPage(bool bPrev)
{
	return pImpl_->pMessageViewWindow_->scrollPage(bPrev);
}

bool qm::MessageWindow::find(const WCHAR* pwszFind,
							 unsigned int nFlags)
{
	return pImpl_->pMessageViewWindow_->find(pwszFind, nFlags);
}

unsigned int qm::MessageWindow::getSupportedFindFlags() const
{
	return pImpl_->pMessageViewWindow_->getSupportedFindFlags();
}

bool qm::MessageWindow::openLink()
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

MessageViewModeHolder* qm::MessageWindow::getMessageViewModeHolder() const
{
	return pImpl_;
}

AttachmentSelectionModel* qm::MessageWindow::getAttachmentSelectionModel() const
{
	return pImpl_->pHeaderWindow_->getAttachmentSelectionModel();
}

bool qm::MessageWindow::save()
{
	Profile* pProfile = pImpl_->pProfile_;
	
	pProfile->setInt(pImpl_->pwszSection_, L"ShowHeaderWindow", pImpl_->bShowHeaderWindow_);
	pProfile->setInt(pImpl_->pwszSection_, L"ViewMode", pImpl_->nMode_);
	pProfile->setString(pImpl_->pwszSection_, L"Template",
		pImpl_->wstrTemplate_.get() ? pImpl_->wstrTemplate_.get() : L"");
	
	return true;
}

void qm::MessageWindow::addMessageWindowHandler(MessageWindowHandler* pHandler)
{
	pImpl_->listHandler_.push_back(pHandler);
}

void qm::MessageWindow::removeMessageWindowHandler(MessageWindowHandler* pHandler)
{
	MessageWindowImpl::HandlerList::iterator it = std::remove(
		pImpl_->listHandler_.begin(), pImpl_->listHandler_.end(), pHandler);
	pImpl_->listHandler_.erase(it, pImpl_->listHandler_.end());
}

Accelerator* qm::MessageWindow::getAccelerator()
{
	return pImpl_->pAccelerator_.get();
}

LRESULT qm::MessageWindow::windowProc(UINT uMsg,
									  WPARAM wParam,
									  LPARAM lParam)
{
	BEGIN_MESSAGE_HANDLER()
		HANDLE_CREATE()
		HANDLE_DESTROY()
		HANDLE_LBUTTONDOWN()
		HANDLE_SIZE()
		HANDLE_TIMER()
		HANDLE_MESSAGE(MessageWindowImpl::WM_MESSAGEMODEL_MESSAGECHANGED, onMessageModelMessageChanged)
	END_MESSAGE_HANDLER()
	return DefaultWindowHandler::windowProc(uMsg, wParam, lParam);
}

LRESULT qm::MessageWindow::onCreate(CREATESTRUCT* pCreateStruct)
{
	if (DefaultWindowHandler::onCreate(pCreateStruct) == -1)
		return -1;
	
	MessageWindowCreateContext* pContext =
		static_cast<MessageWindowCreateContext*>(pCreateStruct->lpCreateParams);
	pImpl_->pDocument_ = pContext->pDocument_;
	pImpl_->pMessageViewModeHolder_ = pContext->pMessageViewModeHolder_;
	pImpl_->pMessageViewModeHolder_->addMessageViewModeHolderHandler(pImpl_);
	pImpl_->pEncodingModel_ = pContext->pEncodingModel_;
	pImpl_->pEncodingModel_->addEncodingModelHandler(pImpl_);
	pImpl_->pSecurityModel_ = pContext->pSecurityModel_;
	pImpl_->pSecurityModel_->addSecurityModelHandler(pImpl_);
	
	CustomAcceleratorFactory acceleratorFactory;
	pImpl_->pAccelerator_ = pContext->pUIManager_->getKeyMap()->createAccelerator(
		&acceleratorFactory, pImpl_->pwszSection_, mapKeyNameToId, countof(mapKeyNameToId));
	if (!pImpl_->pAccelerator_.get())
		return -1;
	
	std::auto_ptr<HeaderWindow> pHeaderWindow(new HeaderWindow(pImpl_->pProfile_));
	HeaderWindowCreateContext context = {
		pContext->pDocument_,
		pContext->pUIManager_->getMenuManager()
	};
	if (!pHeaderWindow->create(L"QmHeaderWindow", 0, WS_VISIBLE | WS_CHILD,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		getHandle(), 0, 0, MessageWindowImpl::ID_HEADERWINDOW, &context))
		return -1;
	pImpl_->pHeaderWindow_ = pHeaderWindow.release();
	
	std::auto_ptr<MessageViewWindowFactory> pFactory(
		new MessageViewWindowFactory(pImpl_->pDocument_, pImpl_->pProfile_,
			pImpl_->pwszSection_, pImpl_->pMessageModel_,
			pContext->pUIManager_->getMenuManager(), pImpl_, false));
	pImpl_->pFactory_ = pFactory;
	
	if (!pImpl_->pFactory_->create(getHandle()))
		return -1;
	pImpl_->pMessageViewWindow_ = pImpl_->pFactory_->getTextMessageViewWindow();
	pImpl_->layoutChildren();
	pImpl_->pMessageViewWindow_->getWindow().showWindow(SW_SHOW);
	
	MessageViewMode* pMode = pImpl_->pMessageViewModeHolder_->getMessageViewMode();
	if (pMode)
		pMode->addMessageViewModeHandler(pImpl_);
	
	pImpl_->bCreated_ = true;
	pImpl_->bLayouting_ = false;
	
	return 0;
}

LRESULT qm::MessageWindow::onDestroy()
{
	MessageViewMode* pMode = pImpl_->pMessageViewModeHolder_->getMessageViewMode();
	if (pMode)
		pMode->removeMessageViewModeHandler(pImpl_);
	
	pImpl_->pMessageViewModeHolder_->removeMessageViewModeHolderHandler(pImpl_);
	pImpl_->pEncodingModel_->removeEncodingModelHandler(pImpl_);
	pImpl_->pSecurityModel_->removeSecurityModelHandler(pImpl_);
	pImpl_->pMessageModel_->removeMessageModelHandler(pImpl_);
	
	return DefaultWindowHandler::onDestroy();
}

LRESULT qm::MessageWindow::onLButtonDown(UINT nFlags,
										 const POINT& pt)
{
	setFocus();
	return DefaultWindowHandler::onLButtonDown(nFlags, pt);
}

LRESULT qm::MessageWindow::onSize(UINT nFlags,
								  int cx,
								  int cy)
{
	if (pImpl_->bCreated_ && !pImpl_->bLayouting_)
		pImpl_->layoutChildren(cx, cy);
	return DefaultWindowHandler::onSize(nFlags, cx, cy);
}

LRESULT qm::MessageWindow::onTimer(UINT nId)
{
	if (nId == pImpl_->nSeenTimerId_) {
		MessagePtrLock mpl(pImpl_->pMessageModel_->getCurrentMessage());
		if (mpl) {
			Account* pAccount = mpl->getFolder()->getAccount();
			pAccount->setMessagesFlags(MessageHolderList(1, mpl),
				MessageHolder::FLAG_SEEN, MessageHolder::FLAG_SEEN);
		}
	}
	return 0;
}

LRESULT qm::MessageWindow::onMessageModelMessageChanged(WPARAM wParam,
														LPARAM lParam)
{
	pImpl_->setMessage(reinterpret_cast<MessageHolder*>(lParam), true);
	return 0;
}

bool qm::MessageWindow::isShow() const
{
	return (getStyle() & WS_VISIBLE) != 0;
}

bool qm::MessageWindow::isActive() const
{
	return hasFocus() ||
		(pImpl_->pMessageViewWindow_ &&
			pImpl_->pMessageViewWindow_->isActive()) ||
		pImpl_->pHeaderWindow_->isActive();
}

void qm::MessageWindow::setActive()
{
	if (pImpl_->pMessageViewWindow_)
		pImpl_->pMessageViewWindow_->setActive();
	else
		setFocus();
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
										   Message& msg,
										   const ContentTypeParser* pContentType) :
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
 * MessageWindowStatusTextEvent
 *
 */

qm::MessageWindowStatusTextEvent::MessageWindowStatusTextEvent(const WCHAR* pwszText) :
	pwszText_(pwszText)
{
}

qm::MessageWindowStatusTextEvent::~MessageWindowStatusTextEvent()
{
}

const WCHAR* qm::MessageWindowStatusTextEvent::getText() const
{
	return pwszText_;
}


/****************************************************************************
 *
 * MessageWindowItem
 *
 */

qm::MessageWindowItem::~MessageWindowItem()
{
}
