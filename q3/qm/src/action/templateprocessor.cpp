/*
 * $Id: templateprocessor.cpp,v 1.4 2003/05/10 13:15:28 snakamura Exp $
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
	FolderModel* pFolderModel, MessageSelectionModel* pMessageSelectionModel,
	EditFrameWindowManager* pEditFrameWindowManager, HWND hwnd,
	Profile* pProfile, bool bExternalEditor) :
	pDocument_(pDocument),
	pFolderModel_(pFolderModel),
	pMessageSelectionModel_(pMessageSelectionModel),
	pEditFrameWindowManager_(pEditFrameWindowManager),
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
	TemplateContext context(mpl, mpl ? &msg : 0, pAccount,
		pDocument_, hwnd_, pProfile_, &handler, &status);
	CHECK_QSTATUS();
	
	string_ptr<WSTRING> wstrValue;
	status = pTemplate->getValue(context, &wstrValue);
	CHECK_QSTATUS();
	
	bool bExternalEditor = (bExternalEditor_ && !bReverseExternalEditor) ||
		(!bExternalEditor_ && bReverseExternalEditor);
	
	if (bExternalEditor) {
		string_ptr<WSTRING> wstrEditor;
		status = pProfile_->getString(L"Global", L"ExternalEditor", L"", &wstrEditor);
		CHECK_QSTATUS();
		if (!*wstrEditor.get()) {
			wstrEditor.reset(0);
			status = pProfile_->getString(L"Global", L"Editor", L"", &wstrEditor);
			CHECK_QSTATUS();
		}
		
		if (*wstrEditor.get()) {
			string_ptr<WSTRING> wstrTempFile(concat(
				Application::getApplication().getTemporaryFolder(),
				L"\\q3edit.txt"));
			if (!wstrTempFile.get())
				return QSTATUS_OUTOFMEMORY;
			
			FileOutputStream stream(wstrTempFile.get(), &status);
			CHECK_QSTATUS();
			BufferedOutputStream bufferedStream(&stream, false, &status);
			CHECK_QSTATUS();
			OutputStreamWriter writer(&bufferedStream,
				false, getSystemEncoding(), &status);
			CHECK_QSTATUS();
			status = writer.write(wstrValue.get(), wcslen(wstrValue.get()));
			CHECK_QSTATUS();
			status = writer.close();
			CHECK_QSTATUS();
			
			const WCHAR* pFile = wstrEditor.get();
			WCHAR* pParam = 0;
			if (*wstrEditor.get() == L'\"') {
				++pFile;
				pParam = wcschr(wstrEditor.get() + 1, L'\"');
			}
			else {
				pParam = wcschr(wstrEditor.get(), L' ');
			}
			if (pParam) {
				*pParam = L'\0';
				++pParam;
			}
			
			StringBuffer<WSTRING> bufParam(&status);
			CHECK_QSTATUS();
			if (pParam) {
				const WCHAR* p = wcsstr(pParam, L"%f");
				if (p) {
					status = bufParam.append(pParam, p - pParam);
					CHECK_QSTATUS();
					status = bufParam.append(wstrTempFile.get());
					CHECK_QSTATUS();
					status = bufParam.append(p + 2);
					CHECK_QSTATUS();
				}
				else {
					status = bufParam.append(pParam);
					CHECK_QSTATUS();
					status = bufParam.append(L' ');
					CHECK_QSTATUS();
					status = bufParam.append(wstrTempFile.get());
					CHECK_QSTATUS();
				}
			}
			else {
				status = bufParam.append(wstrTempFile.get());
				CHECK_QSTATUS();
			}
			
			W2T(pFile, ptszFile);
			W2T(bufParam.getCharArray(), ptszParam);
			
			SHELLEXECUTEINFO sei = {
				sizeof(sei),
				0,
				hwnd_,
				0,
				ptszFile,
				ptszParam,
				0,
#ifdef _WIN32_WCE
				SW_SHOWNORMAL,
#else
				SW_SHOWDEFAULT,
#endif
			};
			::ShellExecuteEx(&sei);
		}
	}
	else {
		MessageCreator creator;
		Message* p = 0;
		status = creator.createMessage(wstrValue.get(), -1, &p);
		CHECK_QSTATUS();
		std::auto_ptr<Message> pMessage(p);
		
		std::auto_ptr<EditMessage> pEditMessage;
		status = newQsObject(pDocument_, pAccount, &pEditMessage);
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
