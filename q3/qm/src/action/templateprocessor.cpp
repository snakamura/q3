/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmaccount.h>
#include <qmapplication.h>
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
#include "../ui/foldermodel.h"
#include "../ui/messageselectionmodel.h"
#include "../ui/securitymodel.h"

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
										 SecurityModel* pSecurityModel,
										 EditFrameWindowManager* pEditFrameWindowManager,
										 ExternalEditorManager* pExternalEditorManager,
										 HWND hwnd,
										 Profile* pProfile,
										 bool bExternalEditor) :
	pDocument_(pDocument),
	pFolderModel_(pFolderModel),
	pMessageSelectionModel_(pMessageSelectionModel),
	pSecurityModel_(pSecurityModel),
	pEditFrameWindowManager_(pEditFrameWindowManager),
	pExternalEditorManager_(pExternalEditorManager),
	hwnd_(hwnd),
	pProfile_(pProfile),
	bExternalEditor_(bExternalEditor)
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
	return process(pwszTemplateName,
		TemplateContext::ArgumentList(), bReverseExternalEditor);
}

bool qm::TemplateProcessor::process(const WCHAR* pwszTemplateName,
									const TemplateContext::ArgumentList& listArgument,
									bool bReverseExternalEditor) const
{
	assert(pwszTemplateName);
	
	Account* pAccount = pFolderModel_->getCurrentAccount();
	Folder* pFolder = 0;
	if (!pAccount) {
		pFolder = pFolderModel_->getCurrentFolder();
		if (pFolder)
			pAccount = pFolder->getAccount();
	}
	if (!pAccount)
		return false;
	
	const Template* pTemplate = pDocument_->getTemplateManager()->getTemplate(
		pAccount, pFolder, pwszTemplateName);
	if (!pTemplate)
		return false;
	
	MessagePtrLock mpl(pMessageSelectionModel_->getFocusedMessage());
	
	AccountLock lock;
	MessageHolderList listSelected;
	pMessageSelectionModel_->getSelectedMessages(&lock, 0, &listSelected);
	
	MacroErrorHandlerImpl handler;
	Message msg;
	TemplateContext context(mpl, mpl ? &msg : 0, listSelected, pAccount, pDocument_,
		hwnd_, pSecurityModel_->isDecryptVerify(), pProfile_, &handler, listArgument);
	
	wstring_ptr wstrValue;
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
	if (pProfile_->getInt(L"Global", L"UseExternalEditor", 0) != 0)
		bExternalEditor = !bExternalEditor;
	if (bReverseExternalEditor)
		bExternalEditor = !bExternalEditor;
	
	if (bExternalEditor) {
		return pExternalEditorManager_->open(wstrValue.get());
	}
	else {
		MessageCreator creator;
		std::auto_ptr<Message> pMessage(creator.createMessage(
			pDocument_, wstrValue.get(), -1));
		if (!pMessage.get())
			return false;
		
		std::auto_ptr<EditMessage> pEditMessage(new EditMessage(pProfile_,
			pDocument_, pAccount, pSecurityModel_->isDecryptVerify()));
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
