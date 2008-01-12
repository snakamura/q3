/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#pragma warning(disable:4786)

#include <qmaccount.h>
#include <qmdocument.h>
#include <qmfolder.h>
#include <qmmessage.h>
#include <qmmessageholder.h>
#include <qmtemplate.h>

#include <qsconv.h>
#include <qsosutil.h>

#include "actionutil.h"
#include "templateprocessor.h"
#include "../model/editmessage.h"
#include "../model/messagecontext.h"
#include "../model/messageenumerator.h"
#include "../model/templatemanager.h"
#include "../model/uri.h"
#include "../ui/editframewindow.h"
#include "../ui/externaleditor.h"
#include "../uimodel/encodingmodel.h"
#include "../uimodel/foldermodel.h"
#include "../uimodel/messageselectionmodel.h"
#include "../uimodel/securitymodel.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * TemplateProcessor
 *
 */

qm::TemplateProcessor::TemplateProcessor(Document* pDocument,
										 FolderModelBase* pFolderModel,
										 MessageSelectionModel* pMessageSelectionModel,
										 EncodingModel* pEncodingModel,
										 SecurityModel* pSecurityModel,
										 EditFrameWindowManager* pEditFrameWindowManager,
										 ExternalEditorManager* pExternalEditorManager,
										 const ActionInvoker* pActionInvoker,
										 HWND hwnd,
										 Profile* pProfile,
										 bool bExternalEditor,
										 const WCHAR* pwszTempDir) :
	pDocument_(pDocument),
	pFolderModel_(pFolderModel),
	pMessageSelectionModel_(pMessageSelectionModel),
	pEncodingModel_(pEncodingModel),
	pSecurityModel_(pSecurityModel),
	pEditFrameWindowManager_(pEditFrameWindowManager),
	pExternalEditorManager_(pExternalEditorManager),
	pActionInvoker_(pActionInvoker),
	hwnd_(hwnd),
	pProfile_(pProfile),
	bExternalEditor_(bExternalEditor),
	wstrTempDir_(allocWString(pwszTempDir))
{
	assert(pDocument_);
	assert(pFolderModel);
	assert(pMessageSelectionModel);
	assert(pEditFrameWindowManager_);
	assert(hwnd_);
	assert(pProfile_);
}

qm::TemplateProcessor::~TemplateProcessor()
{
}

bool qm::TemplateProcessor::process(const WCHAR* pwszTemplateName,
									const TemplateContext::ArgumentList& listArgument,
									const MessageHolderURI* pURI,
									bool bReverseExternalEditor) const
{
	return process(pwszTemplateName, listArgument, pURI, bReverseExternalEditor, 0);
}

bool qm::TemplateProcessor::process(const WCHAR* pwszTemplateName,
									const TemplateContext::ArgumentList& listArgument,
									bool bReverseExternalEditor,
									const WCHAR* pwszClass) const
{
	assert(pwszClass);
	
	Account* pAccount = getAccount(pwszClass);
	if (!pAccount)
		return false;
	
	return process(pwszTemplateName, listArgument, 0, bReverseExternalEditor, pAccount);
}

bool qm::TemplateProcessor::process(const WCHAR* pwszTemplateName,
									const TemplateContext::ArgumentList& listArgument,
									const MessageHolderURI* pURI,
									bool bReverseExternalEditor,
									Account* pAccountForced) const
{
	assert(pwszTemplateName);
	assert(!pURI || !pAccountForced);
	
	std::auto_ptr<MessageContext> pContext;
	std::auto_ptr<MessageEnumerator> pEnum;
	if (pURI) {
		pContext = pURI->resolve(pDocument_->getURIResolver());
		pEnum.reset(new MessageContextMessageEnumerator(pContext.get()));
	}
	else if (!pAccountForced) {
		pEnum = pMessageSelectionModel_->getFocusedMessage();
	}
	if (pEnum.get() && !pEnum->next())
		return false;
	
	Account* pAccount = 0;
	if (pEnum.get()) {
		pAccount = pEnum->getAccount();
		if (!pAccount) {
			MessagePtrLock mpl(pEnum->getOriginMessagePtr());
			if (mpl) {
				pAccount = mpl->getAccount();
			}
			else {
				pAccount = getAccount(L"mail");
				if (!pAccount)
					return false;
			}
		}
	}
	else {
		pAccount = pAccountForced;
	}
	assert(pAccount);
	
	unsigned int nSecurityMode = pSecurityModel_->getSecurityMode();
	
	MessageHolder* pmh = pEnum.get() ? pEnum->getMessageHolder() : 0;
	Message* pMessage = 0;
	Message msg;
	if (pmh) {
		pMessage = &msg;
	}
	else if (pEnum.get()) {
		pMessage = pEnum->getMessage(
			Account::GETMESSAGEFLAG_ALL, 0, nSecurityMode, &msg);
		if (!pMessage)
			return false;
	}
	
	Folder* pFolder = pFolderModel_->getCurrent().second;
	
	const Template* pTemplate = pDocument_->getTemplateManager()->getTemplate(
		pAccount, pFolder, pwszTemplateName);
	if (!pTemplate)
		return false;
	
	AccountLock lock;
	MessageHolderList listSelected;
	if (!pAccountForced && !pURI)
		pMessageSelectionModel_->getSelectedMessageHolders(&lock, 0, &listSelected);
	
	const WCHAR* pwszBodyCharset = 0;
	if (pEncodingModel_)
		pwszBodyCharset = pEncodingModel_->getEncoding();
	
	MacroErrorHandlerImpl handler;
	TemplateContext context(pmh, pMessage, listSelected, pFolder,
		pAccount, pDocument_, pActionInvoker_, hwnd_, pwszBodyCharset,
		MacroContext::FLAG_UITHREAD | MacroContext::FLAG_UI | MacroContext::FLAG_MODIFY,
		nSecurityMode, pProfile_, &handler, listArgument);
	
	wxstring_size_ptr wstrValue;
	switch (pTemplate->getValue(context, &wstrValue)) {
	case Template::RESULT_SUCCESS:
		break;
	case Template::RESULT_ERROR:
		return false;
	case Template::RESULT_CANCEL:
		return true;
	default:
		assert(false);
		return false;
	}
	
	bool bExternalEditor = bExternalEditor_;
	if (pProfile_->getInt(L"Global", L"UseExternalEditor") != 0)
		bExternalEditor = !bExternalEditor;
	if (bReverseExternalEditor)
		bExternalEditor = !bExternalEditor;
	
	if (bExternalEditor) {
		return pExternalEditorManager_->open(wstrValue.get());
	}
	else {
		MessageCreator creator(MessageCreator::FLAG_ADDNODEFAULTCONTENTTYPE |
			MessageCreator::FLAG_RECOVERHEADER, SECURITYMODE_NONE);
		std::auto_ptr<Message> pMessage(creator.createMessage(
			wstrValue.get(), wstrValue.size(), pDocument_->getURIResolver()));
		if (!pMessage.get())
			return false;
		
		std::auto_ptr<EditMessage> pEditMessage(new EditMessage(pProfile_,
			pDocument_, pAccount, pSecurityModel_->getSecurityMode(), wstrTempDir_.get()));
		if (!pEditMessage->setMessage(pMessage))
			return false;
		
		if (!pEditFrameWindowManager_->open(pEditMessage))
			return false;
	}
	
	return true;
}

Account* qm::TemplateProcessor::getAccount(const WCHAR* pwszClass) const
{
	return FolderActionUtil::getAccount(pDocument_,
		pFolderModel_, pProfile_, pwszClass);
}


/****************************************************************************
 *
 * TemplateProcessor::MacroErrorHandlerImpl
 *
 */

qm::TemplateProcessor::MacroErrorHandlerImpl::MacroErrorHandlerImpl()
{
}

qm::TemplateProcessor::MacroErrorHandlerImpl::~MacroErrorHandlerImpl()
{
}

void qm::TemplateProcessor::MacroErrorHandlerImpl::parseError(Code code,
															  const WCHAR* pwsz)
{
	// TODO
}

void qm::TemplateProcessor::MacroErrorHandlerImpl::processError(Code code,
																const WCHAR* pwsz)
{
	// TODO
}
