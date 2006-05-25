/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
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

#include "templateprocessor.h"
#include "../model/editmessage.h"
#include "../model/templatemanager.h"
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
									bool bReverseExternalEditor) const
{
	TemplateContext::ArgumentList l;
	return process(pwszTemplateName, l, bReverseExternalEditor, 0);
}

bool qm::TemplateProcessor::process(const WCHAR* pwszTemplateName,
									const TemplateContext::ArgumentList& listArgument,
									bool bReverseExternalEditor,
									Account* pAccountForced) const
{
	assert(pwszTemplateName);
	
	Account* pAccount = pAccountForced;
	Folder* pFolder = 0;
	if (!pAccount) {
		std::pair<Account*, Folder*> p(pFolderModel_->getCurrent());
		pAccount = p.first ? p.first : p.second ? p.second->getAccount() : 0;
		if (!pAccount)
			return false;
		pFolder = p.second;
	}
	
	const Template* pTemplate = pDocument_->getTemplateManager()->getTemplate(
		pAccount, pFolder, pwszTemplateName);
	if (!pTemplate)
		return false;
	
	MessagePtrLock mpl(pMessageSelectionModel_->getFocusedMessage());
	MessageHolder* pmh = 0;
	if (!pAccountForced)
		pmh = mpl;
	
	AccountLock lock;
	MessageHolderList listSelected;
	if (!pAccountForced)
		pMessageSelectionModel_->getSelectedMessages(&lock, 0, &listSelected);
	
	const WCHAR* pwszBodyCharset = 0;
	if (pEncodingModel_)
		pwszBodyCharset = pEncodingModel_->getEncoding();
	
	MacroErrorHandlerImpl handler;
	Message msg;
	TemplateContext context(pmh, pmh ? &msg : 0, listSelected,
		pFolder, pAccount, pDocument_, hwnd_, pwszBodyCharset,
		MacroContext::FLAG_UITHREAD | MacroContext::FLAG_UI | MacroContext::FLAG_MODIFY,
		pSecurityModel_->getSecurityMode(), pProfile_, &handler, listArgument);
	
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
			pDocument_, wstrValue.get(), wstrValue.size()));
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
