/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmapplication.h>
#include <qmdocument.h>
#include <qmfolder.h>
#include <qmmessage.h>
#include <qmmessageholder.h>
#include <qmtemplate.h>

#include <qsconv.h>
#include <qsnew.h>
#include <qsosutil.h>

#include "templateprocessor.h"
#include "../model/editmessage.h"
#include "../model/templatemanager.h"
#include "../ui/editframewindow.h"
#include "../ui/externaleditor.h"
#include "../ui/foldermodel.h"
#include "../ui/messageselectionmodel.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * TemplateProcessor
 *
 */

qm::TemplateProcessor::TemplateProcessor(Document* pDocument,
	FolderModelBase* pFolderModel, MessageSelectionModel* pMessageSelectionModel,
	EditFrameWindowManager* pEditFrameWindowManager,
	ExternalEditorManager* pExternalEditorManager, HWND hwnd,
	Profile* pProfile, bool bExternalEditor) :
	pDocument_(pDocument),
	pFolderModel_(pFolderModel),
	pMessageSelectionModel_(pMessageSelectionModel),
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

QSTATUS qm::TemplateProcessor::process(
	const WCHAR* pwszTemplateName, bool bReverseExternalEditor) const
{
	return process(pwszTemplateName,
		TemplateContext::ArgumentList(), bReverseExternalEditor);
}

QSTATUS qm::TemplateProcessor::process(const WCHAR* pwszTemplateName,
	const TemplateContext::ArgumentList& listArgument, bool bReverseExternalEditor) const
{
	assert(pwszTemplateName);
	
	DECLARE_QSTATUS();
	
	Account* pAccount = pFolderModel_->getCurrentAccount();
	Folder* pFolder = 0;
	if (!pAccount) {
		pFolder = pFolderModel_->getCurrentFolder();
		if (pFolder)
			pAccount = pFolder->getAccount();
	}
	if (!pAccount)
		return QSTATUS_FAIL;
	
	const Template* pTemplate = 0;
	status = pDocument_->getTemplateManager()->getTemplate(
		pAccount, pFolder, pwszTemplateName, &pTemplate);
	CHECK_QSTATUS();
	
	MessagePtr ptr;
	status = pMessageSelectionModel_->getFocusedMessage(&ptr);
	CHECK_QSTATUS();
	MessagePtrLock mpl(ptr);
	MacroErrorHandlerImpl handler;
	Message msg(&status);
	CHECK_QSTATUS();
	TemplateContext context(mpl, mpl ? &msg : 0, pAccount, pDocument_,
		hwnd_, pProfile_, &handler, listArgument, &status);
	CHECK_QSTATUS();
	
	string_ptr<WSTRING> wstrValue;
	status = pTemplate->getValue(context, &wstrValue);
	CHECK_QSTATUS();
	
	bool bExternalEditor = (bExternalEditor_ && !bReverseExternalEditor) ||
		(!bExternalEditor_ && bReverseExternalEditor);
	
	if (bExternalEditor) {
		status = pExternalEditorManager_->open(wstrValue.get());
		CHECK_QSTATUS();
	}
	else {
		MessageCreator creator;
		Message* p = 0;
		status = creator.createMessage(wstrValue.get(), -1, &p);
		CHECK_QSTATUS();
		std::auto_ptr<Message> pMessage(p);
		
		std::auto_ptr<EditMessage> pEditMessage;
		status = newQsObject(pProfile_, pDocument_, pAccount, &pEditMessage);
		CHECK_QSTATUS();
		status = pEditMessage->setMessage(pMessage.get());
		CHECK_QSTATUS();
		pMessage.release();
		
		status = pEditFrameWindowManager_->open(pEditMessage.get());
		CHECK_QSTATUS();
		pEditMessage.release();
	}
	
	return QSTATUS_SUCCESS;
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

void qm::TemplateProcessor::MacroErrorHandlerImpl::parseError(
	Code code, const WCHAR* pwsz)
{
	// TODO
}

void qm::TemplateProcessor::MacroErrorHandlerImpl::processError(
	Code code, const WCHAR* pwsz)
{
	// TODO
}
