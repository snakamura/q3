/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#pragma warning(disable:4786)

#include <qmaccount.h>
#include <qmdocument.h>
#include <qmmessage.h>
#include <qmmessageholder.h>
#include <qmmessagewindow.h>
#include <qmtemplate.h>
#include <qmsecurity.h>

#include <qsaccelerator.h>
#include <qsinit.h>
#include <qsprofile.h>

#include <boost/bind.hpp>

#include "focus.h"
#include "headerwindow.h"
#include "messageviewwindow.h"
#include "messagewindow.h"
#include "resourceinc.h"
#include "uimanager.h"
#include "../model/messagecontext.h"
#include "../model/templatemanager.h"
#include "../uimodel/encodingmodel.h"
#include "../uimodel/messagemodel.h"
#include "../uimodel/messageviewmode.h"
#include "../uimodel/securitymodel.h"
#include "../uimodel/viewmodel.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * MessageWindowImpl
 *
 */

class qm::MessageWindowImpl :
	public FocusController<MessageWindowItem>,
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
		TIMER_MAKESEEN	= 20
	};

public:
	typedef std::vector<MessageWindowHandler*> HandlerList;

public:
	void layoutChildren();
	void layoutChildren(int cx,
						int cy);
	bool setMessage(MessageContext* pContext,
					bool bResetEncoding);
	void reloadProfiles(bool bInitialize);

public:
	virtual bool isPrimaryItemFocused();
	virtual bool moveFocus(Focus focus,
						   bool bCycle);
	virtual void setFocus(unsigned int nItem);
	virtual MessageWindowItem* getFocusedItem();

public:
	virtual void messageChanged(const MessageModelEvent& event);
	virtual void updateRestoreInfo(const MessageModelRestoreEvent& event);
	virtual void applyRestoreInfo(const MessageModelRestoreEvent& event);

public:
	virtual void modeChanged(const MessageViewModeEvent& event);
	virtual void zoomChanged(const MessageViewModeEvent& event);
	virtual void fitChanged(const MessageViewModeEvent& event);

public:
	virtual void messageViewModeChanged(const MessageViewModeHolderEvent& event);

public:
	virtual void encodingChanged(const EncodingModelEvent& event);

public:
	virtual void securityModeChanged(const SecurityModelEvent& event);

public:
	virtual void statusTextChanged(const WCHAR* pwszText);

public:
	static void applyModeToMessageViewWindow(MessageViewWindow* pMessageViewWindow,
											 MessageViewMode* pMode);

private:
	void fireMessageChanged(MessageHolder* pmh,
							const Message* pMessage) const;
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
	const ActionInvoker* pActionInvoker_;
	const MessageWindowFontManager* pFontManager_;
	HeaderWindow* pHeaderWindow_;
	MessageViewWindow* pMessageViewWindow_;
	bool bCreated_;
	bool bLayouting_;
	bool bSettingMessage_;
	bool bSeenTimer_;
	
	MessageModel* pMessageModel_;
	std::auto_ptr<MessageViewWindowFactory> pFactory_;
	
	wstring_ptr wstrTemplate_;
	wstring_ptr wstrCertificate_;
	unsigned int nSeenWait_;
	bool bShowHeader_;
	MessageWindowItem* pLastFocusedItem_;
	
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

bool qm::MessageWindowImpl::setMessage(MessageContext* pContext,
									   bool bResetEncoding)
{
	struct SettingMessage
	{
		SettingMessage(bool& b) :
			b_(b)
		{
			b_ = true;
		}
		
		~SettingMessage()
		{
			b_ = false;
		}
		
		bool& b_;
	} settingMessage(bSettingMessage_);
	
	struct ErrorHandler
	{
		ErrorHandler(MessageWindowImpl* pImpl) :
			pImpl_(pImpl)
		{
		}
		
		~ErrorHandler()
		{
			if (pImpl_)
				pImpl_->setMessage(0, true);
		}
		
		void success() {
			pImpl_ = 0;
		}
		
		MessageWindowImpl* pImpl_;
	} errorHandler(this);
	
	bool bActive = pThis_->isActive();
	
	if (bResetEncoding)
		pEncodingModel_->setEncoding(0);
	
	wstrCertificate_.reset(0);
	
	MessagePtrLock mpl(MessagePtr(pContext ? pContext->getMessagePtr() : MessagePtr()));
	
	Account* pAccount = pMessageModel_->getCurrentAccount();
	assert(!mpl || mpl->getAccount() == pAccount);
	Folder* pFolder = pMessageModel_->getCurrentFolder();
	
	if (bSeenTimer_) {
		pThis_->killTimer(TIMER_MAKESEEN);
		bSeenTimer_ = false;
	}
	
	MessageViewMode* pMode = pMessageViewModeHolder_->getMessageViewMode();
	unsigned int nMode = pMode ? pMode->getMode() : MessageViewMode::MODE_NONE;
	bool bRawMode = (nMode & MessageViewMode::MODE_RAW) != 0;
	bool bSourceMode = (nMode & MessageViewMode::MODE_SOURCE) != 0;
	bool bHtmlMode = (nMode & MessageViewMode::MODE_HTML) != 0;
	
	Message* pMessage = 0;
	if (pContext) {
		unsigned int nFlags = Account::GMF_FALLBACK;
		if (nSeenWait_ == 0)
			nFlags |= Account::GMF_MAKESEEN;
		if (bRawMode || bSourceMode)
			nFlags |= Account::GMF_ALL;
		else if (bHtmlMode)
			nFlags |= Account::GMF_HTML;
		else
			nFlags |= Account::GMF_TEXT;
		pMessage = pContext->getMessage(nFlags, 0, pSecurityModel_->getSecurityMode());
		if (!pMessage)
			return false;
	}
	
	if (pMessage) {
		const WCHAR* pwszCertificate = pMessage->getParam(L"Certificate");
		if (pwszCertificate)
			wstrCertificate_ = allocWString(pwszCertificate);
	}
	
	if (mpl) {
		if (!mpl->isFlag(MessageHolder::FLAG_SEEN) &&
			nSeenWait_ != 0 && nSeenWait_ != -1)
			bSeenTimer_ = pThis_->setTimer(TIMER_MAKESEEN, nSeenWait_*1000) != 0;
	}
	
	const ContentTypeParser* pContentType = 0;
	MessageViewWindow* pMessageViewWindow = 0;
	if (pMessage && !bRawMode && !bSourceMode && bHtmlMode && !wstrTemplate_.get()) {
		if (pAccount && pAccount->isSupport(Account::SUPPORT_EXTERNALLINK) &&
			!pMessage->isMultipart() &&
			pMessage->hasField(L"X-QMAIL-Link")) {
			pMessageViewWindow = pFactory_->getLinkMessageViewWindow();
		}
		else {
			PartUtil util(*pMessage);
			PartUtil::ContentTypeList l;
			util.getAlternativeContentTypes(&l);
			
			PartUtil::ContentTypeList::iterator it = std::find_if(
				l.begin(), l.end(),
				std::bind1st(
					std::mem_fun(
						&MessageViewWindowFactory::isSupported),
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
		assert(pMessageViewWindow_);
		pMessageViewWindow_->getWindow().showWindow(SW_HIDE);
		pMessageViewWindow_->clearMessage();
		pMessageViewWindow->getWindow().showWindow(SW_SHOW);
		applyModeToMessageViewWindow(pMessageViewWindow, pMode);
		pMessageViewWindow_ = pMessageViewWindow;
		bLayout = true;
	}
	
	if (bShowHeaderWindow_) {
		if (pAccount || pMessage) {
			// TODO
			// Get selected
			TemplateContext context(mpl, pMessage, MessageHolderList(), pFolder,
				pAccount, pDocument_, pActionInvoker_, pThis_->getHandle(),
				pEncodingModel_->getEncoding(), MacroContext::FLAG_UITHREAD | MacroContext::FLAG_UI,
				pSecurityModel_->getSecurityMode(), pProfile_, 0, TemplateContext::ArgumentList());
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
	if (pMessage && wstrTemplate_.get())
		pTemplate = pDocument_->getTemplateManager()->getTemplate(
			pAccount, pFolder, wstrTemplate_.get());
	
	unsigned int nFlags = (bRawMode ? MessageViewWindow::FLAG_RAWMODE : 0) |
		(bSourceMode ? MessageViewWindow::FLAG_SOURCEMODE : 0) |
		(!bShowHeaderWindow_ && bShowHeader_ ? MessageViewWindow::FLAG_INCLUDEHEADER : 0) |
		(nMode & MessageViewMode::MODE_HTMLONLINE ? MessageViewWindow::FLAG_ONLINEMODE : 0) |
		(nMode & MessageViewMode::MODE_INTERNETZONE ? MessageViewWindow::FLAG_INTERNETZONE : 0);
	if (!pMessageViewWindow->setMessage(mpl, pMessage, pFolder, pTemplate,
		pEncodingModel_->getEncoding(), nFlags, pSecurityModel_->getSecurityMode()))
		return false;
	if (bActive)
		pThis_->setActive();
	
	fireMessageChanged(mpl, pMessage);
	
	errorHandler.success();
	
	return true;
}

void qm::MessageWindowImpl::reloadProfiles(bool bInitialize)
{
	nSeenWait_ = pProfile_->getInt(pwszSection_, L"SeenWait");
	bShowHeader_ = pProfile_->getInt(pwszSection_, L"ShowHeader") != 0;
	
	if (!bInitialize)
		setMessage(pMessageModel_->getCurrentMessage(), false);
}

bool qm::MessageWindowImpl::isPrimaryItemFocused()
{
	return getFocusedItem() == pMessageViewWindow_;
}

bool qm::MessageWindowImpl::moveFocus(Focus focus,
									  bool bCycle)
{
	if (!bShowHeaderWindow_)
		return false;
	
	MessageWindowItem* pItem = getFocusedItem();
	if (!pItem)
		return false;
	
	MessageWindowItem* pNewItem = 0;
	switch (focus) {
	case FOCUS_PREV:
		if (pItem == pMessageViewWindow_) {
			pNewItem = pHeaderWindow_->getPrevFocusItem(0);
		}
		else {
			pNewItem = pHeaderWindow_->getPrevFocusItem(pItem);
			if (!pNewItem && bCycle)
				pNewItem = pMessageViewWindow_;
		}
		break;
	case FOCUS_NEXT:
		if (pItem == pMessageViewWindow_) {
			if (bCycle)
				pNewItem = pHeaderWindow_->getNextFocusItem(0);
		}
		else {
			pNewItem = pHeaderWindow_->getNextFocusItem(pItem);
			if (!pNewItem)
				pNewItem = pMessageViewWindow_;
		}
		break;
	default:
		assert(false);
		break;
	}
	if (!pNewItem)
		return false;
	pNewItem->setFocus();
	
	return true;
}

void qm::MessageWindowImpl::setFocus(unsigned int nItem)
{
	if (!bShowHeaderWindow_)
		return;
	
	MessageWindowItem* pItem = pHeaderWindow_->getItemByNumber(nItem);
	if (pItem)
		pItem->setFocus();
}

MessageWindowItem* qm::MessageWindowImpl::getFocusedItem()
{
	if (pMessageViewWindow_ && pMessageViewWindow_->isActive())
		return pMessageViewWindow_;
	else
		return pHeaderWindow_->getFocusedItem();
}

void qm::MessageWindowImpl::messageChanged(const MessageModelEvent& event)
{
	assert(Init::getInit().isPrimaryThread());
	
	if (!bSettingMessage_)
		setMessage(event.getMessageContext(), true);
}

void qm::MessageWindowImpl::updateRestoreInfo(const MessageModelRestoreEvent& event)
{
	assert(Init::getInit().isPrimaryThread());
	
	ViewModel::RestoreInfo* pRestoreInfo = event.getRestoreInfo();
	pRestoreInfo->setScrollPos(pMessageViewWindow_->getScrollPos());
}

void qm::MessageWindowImpl::applyRestoreInfo(const MessageModelRestoreEvent& event)
{
	assert(Init::getInit().isPrimaryThread());
	
	ViewModel::RestoreInfo* pRestoreInfo = event.getRestoreInfo();
	pMessageViewWindow_->setScrollPos(pRestoreInfo->getScrollPos());
}

void qm::MessageWindowImpl::modeChanged(const MessageViewModeEvent& event)
{
	const unsigned int nSetMode =
		MessageViewMode::MODE_RAW |
		MessageViewMode::MODE_HTML |
		MessageViewMode::MODE_HTMLONLINE |
		MessageViewMode::MODE_INTERNETZONE |
		MessageViewMode::MODE_SOURCE;
	
	unsigned int nAdded = event.getAddedMode();
	unsigned int nRemoved = event.getRemovedMode();
	if (nAdded & nSetMode || nRemoved & nSetMode)
		setMessage(pMessageModel_->getCurrentMessage(), false);
	
	if (nAdded & MessageViewMode::MODE_SELECT)
		pMessageViewWindow_->setSelectMode(true);
	else if (nRemoved & MessageViewMode::MODE_SELECT)
		pMessageViewWindow_->setSelectMode(false);
	
	if (nAdded & MessageViewMode::MODE_QUOTE)
		pMessageViewWindow_->setQuoteMode(true);
	else if (nRemoved & MessageViewMode::MODE_QUOTE)
		pMessageViewWindow_->setQuoteMode(false);
}

void qm::MessageWindowImpl::zoomChanged(const MessageViewModeEvent& event)
{
	MessageViewMode* pMode = pMessageViewModeHolder_->getMessageViewMode();
	pMessageViewWindow_->setZoom(pMode ? pMode->getZoom() : MessageViewMode::ZOOM_NONE);
}

void qm::MessageWindowImpl::fitChanged(const MessageViewModeEvent& event)
{
	MessageViewMode* pMode = pMessageViewModeHolder_->getMessageViewMode();
	pMessageViewWindow_->setFit(pMode ? pMode->getFit() : MessageViewMode::FIT_NONE);
}

void qm::MessageWindowImpl::messageViewModeChanged(const MessageViewModeHolderEvent& event)
{
	MessageViewMode* pOld = event.getOldMessageViewMode();
	if (pOld)
		pOld->removeMessageViewModeHandler(this);
	
	MessageViewMode* pNew = event.getNewMessageViewMode();
	if (pNew)
		pNew->addMessageViewModeHandler(this);
	
	applyModeToMessageViewWindow(pMessageViewWindow_, pNew);
}

void qm::MessageWindowImpl::encodingChanged(const EncodingModelEvent& event)
{
	setMessage(pMessageModel_->getCurrentMessage(), false);
}

void qm::MessageWindowImpl::securityModeChanged(const SecurityModelEvent& event)
{
	setMessage(pMessageModel_->getCurrentMessage(), false);
}

void qm::MessageWindowImpl::statusTextChanged(const WCHAR* pwszText)
{
	fireStatusTextChanged(pwszText);
}

void qm::MessageWindowImpl::applyModeToMessageViewWindow(MessageViewWindow* pMessageViewWindow,
														 MessageViewMode* pMode)
{
	unsigned int nMode = pMode ? pMode->getMode() : MessageViewMode::MODE_NONE;
	pMessageViewWindow->setSelectMode((nMode & MessageViewMode::MODE_SELECT) != 0);
	pMessageViewWindow->setQuoteMode((nMode & MessageViewMode::MODE_QUOTE) != 0);
	pMessageViewWindow->setZoom(pMode ? pMode->getZoom() : MessageViewMode::ZOOM_NONE);
	pMessageViewWindow->setFit(pMode ? pMode->getFit() : MessageViewMode::FIT_NONE);
}

void qm::MessageWindowImpl::fireMessageChanged(MessageHolder* pmh,
											   const Message* pMessage) const
{
	MessageWindowEvent event(pmh, pMessage);
	std::for_each(listHandler_.begin(), listHandler_.end(),
		boost::bind(&MessageWindowHandler::messageChanged, _1, boost::cref(event)));
}

void qm::MessageWindowImpl::fireStatusTextChanged(const WCHAR* pwszText) const
{
	MessageWindowStatusTextEvent event(pwszText);
	std::for_each(listHandler_.begin(), listHandler_.end(),
		boost::bind(&MessageWindowHandler::statusTextChanged, _1, boost::cref(event)));
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
	wstring_ptr wstrTemplate(pProfile->getString(pwszSection, L"Template"));
	
	pImpl_ = new MessageWindowImpl();
	pImpl_->pThis_ = this;
	pImpl_->bShowHeaderWindow_ = pProfile->getInt(pwszSection, L"ShowHeaderWindow") != 0;
	pImpl_->pProfile_ = pProfile;
	pImpl_->pwszSection_ = pwszSection;
	pImpl_->pDocument_ = 0;
	pImpl_->pMessageViewModeHolder_ = 0;
	pImpl_->pEncodingModel_ = 0;
	pImpl_->pSecurityModel_ = 0;
	pImpl_->pHeaderWindow_ = 0;
	pImpl_->pMessageViewWindow_ = 0;
	pImpl_->bCreated_ = false;
	pImpl_->bLayouting_ = false;
	pImpl_->bSettingMessage_ = false;
	pImpl_->bSeenTimer_ = false;
	pImpl_->pMessageModel_ = pMessageModel;
	pImpl_->wstrTemplate_ = *wstrTemplate.get() ? wstrTemplate : 0;
	pImpl_->nSeenWait_ = 0;
	pImpl_->bShowHeader_ = true;
	pImpl_->pLastFocusedItem_ = 0;
	
	pImpl_->reloadProfiles(true);
	
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
		pImpl_->setMessage(pImpl_->pMessageModel_->getCurrentMessage(), false);
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
		pImpl_->setMessage(pImpl_->pMessageModel_->getCurrentMessage(), false);
	}
}

const WCHAR* qm::MessageWindow::getCertificate() const
{
	return pImpl_->wstrCertificate_.get();
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

std::auto_ptr<MessageWindow::Mark> qm::MessageWindow::mark() const
{
	return pImpl_->pMessageViewWindow_->mark();
}

void qm::MessageWindow::reset(const Mark& mark)
{
	pImpl_->pMessageViewWindow_->reset(mark);
}

const WCHAR* qm::MessageWindow::getFontGroup() const
{
	const MessageWindowFontGroup* pFontGroup =
		pImpl_->pFactory_->getTextMessageViewWindow()->getFontGroup();
	return pFontGroup ? pFontGroup->getName() : 0;
}

void qm::MessageWindow::setFontGroup(const WCHAR* pwszName)
{
	const MessageWindowFontGroup* pFontGroup = 0;
	if (pwszName)
		pFontGroup = pImpl_->pFontManager_->getGroup(pwszName);
	
	if (pFontGroup != pImpl_->pFactory_->getTextMessageViewWindow()->getFontGroup()) {
		pImpl_->pFactory_->getTextMessageViewWindow()->setFontGroup(
			pFontGroup, pImpl_->pwszSection_);
		pImpl_->setMessage(pImpl_->pMessageModel_->getCurrentMessage(), false);
	}
}

bool qm::MessageWindow::openLink()
{
	return pImpl_->pMessageViewWindow_->openLink();
}

FocusController<MessageWindowItem>* qm::MessageWindow::getFocusController() const
{
	return pImpl_;
}

MessageModel* qm::MessageWindow::getMessageModel() const
{
	return pImpl_->pMessageModel_;
}

AttachmentSelectionModel* qm::MessageWindow::getAttachmentSelectionModel() const
{
	return pImpl_->pHeaderWindow_->getAttachmentSelectionModel();
}

void qm::MessageWindow::saveFocusedItem()
{
	pImpl_->pLastFocusedItem_ = pImpl_->pHeaderWindow_->getFocusedItem();
	if (!pImpl_->pLastFocusedItem_)
		pImpl_->pLastFocusedItem_ = pImpl_->pMessageViewWindow_;
}

void qm::MessageWindow::restoreFocusedItem()
{
	MessageWindowItem* pItem = pImpl_->pLastFocusedItem_;
	if (!pItem)
		pItem = pImpl_->pMessageViewWindow_;
	assert(pItem);
	
	pItem->setFocus();
}

void qm::MessageWindow::layout()
{
	pImpl_->layoutChildren();
}

void qm::MessageWindow::reloadProfiles()
{
	pImpl_->pHeaderWindow_->reloadProfiles();
	pImpl_->pFactory_->reloadProfiles();
	pImpl_->reloadProfiles(false);
}

void qm::MessageWindow::save() const
{
	Profile* pProfile = pImpl_->pProfile_;
	
	pProfile->setInt(pImpl_->pwszSection_, L"ShowHeaderWindow", pImpl_->bShowHeaderWindow_);
	pProfile->setString(pImpl_->pwszSection_, L"Template",
		pImpl_->wstrTemplate_.get() ? pImpl_->wstrTemplate_.get() : L"");
	const WCHAR* pwszFontGroup = getFontGroup();
	pProfile->setString(pImpl_->pwszSection_, L"FontGroup", pwszFontGroup ? pwszFontGroup : L"");
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
	pImpl_->pActionInvoker_ = pContext->pActionInvoker_;
	pImpl_->pFontManager_ = pContext->pFontManager_;
	
	CustomAcceleratorFactory acceleratorFactory;
	pImpl_->pAccelerator_ = pContext->pUIManager_->getKeyMap()->createAccelerator(
		&acceleratorFactory, pImpl_->pwszSection_);
	if (!pImpl_->pAccelerator_.get())
		return -1;
	
	std::auto_ptr<HeaderWindow> pHeaderWindow(new HeaderWindow(pImpl_->pProfile_));
	HeaderWindowCreateContext context = {
		pContext->pUIManager_->getMenuManager()
	};
	if (!pHeaderWindow->create(L"QmHeaderWindow", 0, WS_VISIBLE | WS_CHILD,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		getHandle(), 0, 0, MessageWindowImpl::ID_HEADERWINDOW, &context))
		return -1;
	pImpl_->pHeaderWindow_ = pHeaderWindow.release();
	
	const MessageWindowFontGroup* pFontGroup = pImpl_->pFontManager_->getGroup(
		pImpl_->pProfile_->getString(pImpl_->pwszSection_, L"FontGroup").get());
	std::auto_ptr<MessageViewWindowFactory> pFactory(
		new MessageViewWindowFactory(this, pImpl_->pDocument_, pImpl_->pProfile_,
			pImpl_->pwszSection_, pImpl_->pMessageModel_, pImpl_->pActionInvoker_,
			pContext->pUIManager_->getMenuManager(), pFontGroup, pImpl_, false));
	pImpl_->pFactory_ = pFactory;
	
	if (!pImpl_->pFactory_->create())
		return -1;
	pImpl_->pMessageViewWindow_ = pImpl_->pFactory_->getTextMessageViewWindow();
	pImpl_->layoutChildren();
	pImpl_->pMessageViewWindow_->getWindow().showWindow(SW_SHOW);
	
	MessageViewMode* pMode = pImpl_->pMessageViewModeHolder_->getMessageViewMode();
	if (pMode)
		pMode->addMessageViewModeHandler(pImpl_);
	
	MessageWindowImpl::applyModeToMessageViewWindow(pImpl_->pMessageViewWindow_, pMode);
	
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

LRESULT qm::MessageWindow::onTimer(UINT_PTR nId)
{
	if (nId == MessageWindowImpl::TIMER_MAKESEEN) {
		MessageContext* pContext = pImpl_->pMessageModel_->getCurrentMessage();
		if (pContext) {
			MessagePtrLock mpl(pContext->getMessagePtr());
			if (mpl) {
				Account* pAccount = mpl->getAccount();
				pAccount->setMessagesFlags(MessageHolderList(1, mpl),
					MessageHolder::FLAG_SEEN, MessageHolder::FLAG_SEEN, 0);
			}
		}
		
		killTimer(MessageWindowImpl::TIMER_MAKESEEN);
		pImpl_->bSeenTimer_ = false;
	}
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

FocusControllerBase* qm::MessageWindow::getViewFocusController() const
{
	return pImpl_;
}


/****************************************************************************
 *
 * MessageWindow::Mark
 *
 */

qm::MessageWindow::Mark::~Mark()
{
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
										   const Message* pMessage) :
	pmh_(pmh),
	pMessage_(pMessage)
{
}

qm::MessageWindowEvent::~MessageWindowEvent()
{
}

MessageHolder* qm::MessageWindowEvent::getMessageHolder() const
{
	return pmh_;
}

const Message* qm::MessageWindowEvent::getMessage() const
{
	return pMessage_;
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


/****************************************************************************
 *
 * MessageWindowMessageViewModeHolder
 *
 */

qm::MessageWindowMessageViewModeHolder::MessageWindowMessageViewModeHolder(Profile* pProfile,
																		   const WCHAR* pwszSection) :
	pProfile_(pProfile),
	pwszSection_(pwszSection)
{
	int nZoom = pProfile->getInt(pwszSection, L"ViewZoom");
	if (nZoom != MessageViewMode::ZOOM_NONE &&
		(nZoom < MessageViewMode::ZOOM_MIN || MessageViewMode::ZOOM_MAX < nZoom))
		nZoom = MessageViewMode::ZOOM_NONE;
	int nFit = pProfile->getInt(pwszSection, L"ViewFit");
	if (nFit < MessageViewMode::FIT_NONE || MessageViewMode::FIT_SUPER < nFit)
		nFit = MessageViewMode::FIT_NONE;
	
	pMessageViewMode_.reset(new DefaultMessageViewMode(
		pProfile->getInt(pwszSection, L"ViewMode"),
		nZoom, static_cast<MessageViewMode::Fit>(nFit)));
}

void qm::MessageWindowMessageViewModeHolder::save() const
{
	pProfile_->setInt(pwszSection_, L"ViewMode", pMessageViewMode_->getMode());
	pProfile_->setInt(pwszSection_, L"ViewZoom", pMessageViewMode_->getZoom());
	pProfile_->setInt(pwszSection_, L"ViewFit", pMessageViewMode_->getFit());
}

qm::MessageWindowMessageViewModeHolder::~MessageWindowMessageViewModeHolder()
{
}

MessageViewMode* qm::MessageWindowMessageViewModeHolder::getMessageViewMode()
{
	return pMessageViewMode_.get();
}

void qm::MessageWindowMessageViewModeHolder::addMessageViewModeHolderHandler(MessageViewModeHolderHandler* pHandler)
{
}

void qm::MessageWindowMessageViewModeHolder::removeMessageViewModeHolderHandler(MessageViewModeHolderHandler* pHandler)
{
}


/****************************************************************************
 *
 * MessageWindowFontManager
 *
 */

qm::MessageWindowFontManager::MessageWindowFontManager(const WCHAR* pwszPath)
{
	load(pwszPath);
}

qm::MessageWindowFontManager::~MessageWindowFontManager()
{
	std::for_each(listGroup_.begin(), listGroup_.end(),
		qs::deleter<MessageWindowFontGroup>());
}

const MessageWindowFontGroup* qm::MessageWindowFontManager::getGroup(const WCHAR* pwszName) const
{
	GroupList::const_iterator it = std::find_if(
		listGroup_.begin(), listGroup_.end(),
		boost::bind(string_equal<WCHAR>(),
			boost::bind(&MessageWindowFontGroup::getName, _1), pwszName));
	return it != listGroup_.end() ? *it : 0;
}

const MessageWindowFontManager::GroupList& qm::MessageWindowFontManager::getGroups() const
{
	return listGroup_;
}

void qm::MessageWindowFontManager::addGroup(std::auto_ptr<MessageWindowFontGroup> pGroup)
{
	listGroup_.push_back(pGroup.get());
	pGroup.release();
}

bool qm::MessageWindowFontManager::load(const WCHAR* pwszPath)
{
	if (File::isFileExisting(pwszPath)) {
		XMLReader reader;
		MessageWindowFontContentHandler contentHandler(this);
		reader.setContentHandler(&contentHandler);
		if (!reader.parse(pwszPath))
			return false;
	}
	return true;
}


/****************************************************************************
 *
 * MessageWindowFontGroup
 *
 */

qm::MessageWindowFontGroup::MessageWindowFontGroup(const WCHAR* pwszName)
{
	assert(pwszName);
	wstrName_ = allocWString(pwszName);
}

qm::MessageWindowFontGroup::~MessageWindowFontGroup()
{
	std::for_each(listFontSet_.begin(), listFontSet_.end(),
		qs::deleter<MessageWindowFontSet>());
}

const WCHAR* qm::MessageWindowFontGroup::getName() const
{
	return wstrName_.get();
}

const MessageWindowFontSet* qm::MessageWindowFontGroup::getFontSet(MacroContext* pContext) const
{
	FontSetList::const_iterator it = std::find_if(
		listFontSet_.begin(), listFontSet_.end(),
		std::bind2nd(std::mem_fun(&MessageWindowFontSet::match), pContext));
	return it != listFontSet_.end() ? *it : 0;
}

void qm::MessageWindowFontGroup::addFontSet(std::auto_ptr<MessageWindowFontSet> pFontSet)
{
	listFontSet_.push_back(pFontSet.get());
	pFontSet.release();
}

bool qm::MessageWindowFontGroup::isSet() const
{
	// TODO
	// Check if there is font set which matches against any condition?
	return !listFontSet_.empty();
}


/****************************************************************************
 *
 * MessageWindowFontSet
 *
 */

qm::MessageWindowFontSet::MessageWindowFontSet(std::auto_ptr<Macro> pCondition,
											   unsigned int nLineSpacing) :
	pCondition_(pCondition),
	nLineSpacing_(nLineSpacing)
{
}

qm::MessageWindowFontSet::~MessageWindowFontSet()
{
}

bool qm::MessageWindowFontSet::match(MacroContext* pContext) const
{
	assert(pContext);
	assert(pContext->getMessageHolder());
	
	if (!pCondition_.get())
		return true;
	
	MacroValuePtr pValue(pCondition_->value(pContext));
	return pValue.get() && pValue->boolean();
}

const MessageWindowFontSet::Font* qm::MessageWindowFontSet::getFont() const
{
	return pFont_.get();
}

unsigned int qm::MessageWindowFontSet::getLineSpacing() const
{
	return nLineSpacing_;
}

void qm::MessageWindowFontSet::setFont(std::auto_ptr<Font> pFont)
{
	pFont_ = pFont;
}

bool qm::MessageWindowFontSet::isSet() const
{
	return pFont_.get() != 0;
}


/****************************************************************************
 *
 * MessageWindowFontSet::Font
 *
 */

qm::MessageWindowFontSet::Font::Font(const WCHAR* pwszFace,
									 double dSize,
									 unsigned int nStyle,
									 unsigned int nCharset) :
	dSize_(dSize),
	nStyle_(nStyle),
	nCharset_(nCharset)
{
	assert(pwszFace);
	wstrFace_ = allocWString(pwszFace);
}

qm::MessageWindowFontSet::Font::~Font()
{
}

HFONT qm::MessageWindowFontSet::Font::createFont() const
{
	ClientDeviceContext dc(0);
	LOGFONT lf;
	FontHelper::createLogFont(dc, wstrFace_.get(), dSize_, nStyle_, nCharset_, &lf);
	return ::CreateFontIndirect(&lf);
}


/****************************************************************************
 *
 * MessageWindowFontContentHandler
 *
 */

qm::MessageWindowFontContentHandler::MessageWindowFontContentHandler(MessageWindowFontManager* pManager) :
	pManager_(pManager),
	state_(STATE_ROOT)
{
}

qm::MessageWindowFontContentHandler::~MessageWindowFontContentHandler()
{
}

bool qm::MessageWindowFontContentHandler::startElement(const WCHAR* pwszNamespaceURI,
													   const WCHAR* pwszLocalName,
													   const WCHAR* pwszQName,
													   const Attributes& attributes)
{
	if (wcscmp(pwszLocalName, L"fonts") == 0) {
		if (state_ != STATE_ROOT)
			return false;
		state_ = STATE_FONTS;
	}
	else if (wcscmp(pwszLocalName, L"group") == 0) {
		if (state_ != STATE_FONTS)
			return false;
		
		const WCHAR* pwszName = 0;
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrName, L"name") == 0)
				pwszName = attributes.getValue(n);
			else
				return false;
		}
		if (!pwszName)
			return false;
		
		pGroup_.reset(new MessageWindowFontGroup(pwszName));
		
		state_ = STATE_GROUP;
	}
	else if (wcscmp(pwszLocalName, L"fontSet") == 0) {
		if (state_ != STATE_GROUP)
			return false;
		
		const WCHAR* pwszMatch = 0;
		const WCHAR* pwszLineSpacing = 0;
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrName, L"match") == 0)
				pwszMatch = attributes.getValue(n);
			else if (wcscmp(pwszAttrName, L"lineSpacing") == 0)
				pwszLineSpacing = attributes.getValue(n);
			else
				return false;
		}
		
		std::auto_ptr<Macro> pCondition;
		if (pwszMatch) {
			pCondition = MacroParser().parse(pwszMatch);
			if (!pCondition.get())
				return false;
		}
		
		unsigned int nLineSpacing = -1;
		if (pwszLineSpacing) {
			WCHAR* pEnd = 0;
			long n = wcstol(pwszLineSpacing, &pEnd, 10);
			if (!*pEnd)
				nLineSpacing = static_cast<unsigned int>(n);
		}
		
		pFontSet_.reset(new MessageWindowFontSet(pCondition, nLineSpacing));
		
		state_ = STATE_FONTSET;
	}
	else if (wcscmp(pwszLocalName, L"font") == 0) {
		if (state_ != STATE_FONTSET)
			return false;
		
		const WCHAR* pwszFace = 0;
		const WCHAR* pwszSize = 0;
		const WCHAR* pwszStyle = 0;
		const WCHAR* pwszCharset = 0;
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrName, L"face") == 0)
				pwszFace = attributes.getValue(n);
			else if (wcscmp(pwszAttrName, L"size") == 0)
				pwszSize = attributes.getValue(n);
			else if (wcscmp(pwszAttrName, L"style") == 0)
				pwszStyle = attributes.getValue(n);
			else if (wcscmp(pwszAttrName, L"charset") == 0)
				pwszCharset = attributes.getValue(n);
			else
				return false;
		}
		if (!pwszFace)
			return false;
		
		double dSize = 9.0;
		if (pwszSize) {
			WCHAR* pEnd = 0;
			double d = wcstod(pwszSize, &pEnd);
			if (!*pEnd)
				dSize = d;
		}
		
		unsigned int nStyle = 0;
		if (pwszStyle) {
			wstring_ptr wstrStyle(allocWString(pwszStyle));
			const WCHAR* p = wcstok(wstrStyle.get(), L" ");
			while (p) {
				if (wcscmp(p, L"bold") == 0)
					nStyle |= FontHelper::STYLE_BOLD;
				else if (wcscmp(p, L"italic") == 0)
					nStyle |= FontHelper::STYLE_ITALIC;
				else if (wcscmp(p, L"underline") == 0)
					nStyle |= FontHelper::STYLE_UNDERLINE;
				else if (wcscmp(p, L"strikeout") == 0)
					nStyle |= FontHelper::STYLE_STRIKEOUT;
				
				p = wcstok(0, L" ");
			}
		}
		
		unsigned int nCharset = 0;
		if (pwszCharset) {
			WCHAR* pEnd = 0;
			long l = wcstol(pwszCharset, &pEnd, 10);
			if (!*pEnd)
				nCharset = static_cast<int>(l);
		}
		
		assert(pFontSet_.get());
		std::auto_ptr<MessageWindowFontSet::Font> pFont(
			new MessageWindowFontSet::Font(pwszFace, dSize, nStyle, nCharset));
		pFontSet_->setFont(pFont);
		
		state_ = STATE_FONT;
	}
	else {
		return false;
	}
	
	return true;
}

bool qm::MessageWindowFontContentHandler::endElement(const WCHAR* pwszNamespaceURI,
													 const WCHAR* pwszLocalName,
													 const WCHAR* pwszQName)
{
	if (wcscmp(pwszLocalName, L"fonts") == 0) {
		assert(state_ == STATE_FONTS);
		state_ = STATE_ROOT;
	}
	else if (wcscmp(pwszLocalName, L"group") == 0) {
		assert(state_ == STATE_GROUP);
		
		assert(pGroup_.get());
		if (pGroup_->isSet())
			pManager_->addGroup(pGroup_);
		pGroup_.reset(0);
		
		state_ = STATE_FONTS;
	}
	else if (wcscmp(pwszLocalName, L"fontSet") == 0) {
		assert(state_ == STATE_FONTSET);
		
		assert(pGroup_.get());
		assert(pFontSet_.get());
		if (pFontSet_->isSet())
			pGroup_->addFontSet(pFontSet_);
		pFontSet_.reset(0);
		
		state_ = STATE_GROUP;
	}
	else if (wcscmp(pwszLocalName, L"font") == 0) {
		assert(state_ == STATE_FONT);
		state_ = STATE_FONTSET;
	}
	else {
		return false;
	}
	
	return true;
}

bool qm::MessageWindowFontContentHandler::characters(const WCHAR* pwsz,
													 size_t nStart,
													 size_t nLength)
{
	const WCHAR* p = pwsz + nStart;
	for (size_t n = 0; n < nLength; ++n, ++p) {
		if (*p != L' ' && *p != L'\t' && *p != L'\n')
			return false;
	}
	return true;
}
