/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __TEMPLATEPROCESSOR_H__
#define __TEMPLATEPROCESSOR_H__

#include <qmmacro.h>
#include <qmtemplate.h>

#include <qs.h>
#include <qsprofile.h>
#include <qsstring.h>


namespace qm {

class TemplateProcessor;

class ActionInvoker;
class Document;
class EditFrameWindowManager;
class EncodingModel;
class ExternalEditorManager;
class FolderModelBase;
class MessageSelectionModel;
class SecurityModel;
class TemplateManager;


/****************************************************************************
 *
 * TemplateProcessor
 *
 */

class TemplateProcessor
{
public:
	TemplateProcessor(Document* pDocument,
					  FolderModelBase* pFolderModel,
					  MessageSelectionModel* pMessageSelectionModel,
					  EncodingModel* pEncodingModel,
					  SecurityModel* pSecurityModel,
					  EditFrameWindowManager* pEditFrameWindowManager,
					  ExternalEditorManager* pExternalEditorManager,
					  const ActionInvoker* pActionInvoker,
					  HWND hwnd,
					  qs::Profile* pProfile,
					  bool bExternalEditor,
					  const WCHAR* pwszTempDir);
	~TemplateProcessor();

public:
	bool process(const WCHAR* pwszTemplateName,
				 const TemplateContext::ArgumentList& listArgument,
				 const URI* pURI,
				 bool bReverseExternalEditor) const;
	bool process(const WCHAR* pwszTemplateName,
				 const TemplateContext::ArgumentList& listArgument,
				 const URI* pURI,
				 bool bReverseExternalEditor,
				 Account* pAccountForced) const;

private:
	class MacroErrorHandlerImpl : public MacroErrorHandler
	{
	public:
		MacroErrorHandlerImpl();
		virtual ~MacroErrorHandlerImpl();
	
	public:
		virtual void parseError(Code code,
							    const WCHAR* pwsz);
		virtual void processError(Code code,
								  const WCHAR* pwsz);
	
	private:
		MacroErrorHandlerImpl(const MacroErrorHandlerImpl&);
		MacroErrorHandlerImpl& operator=(const MacroErrorHandlerImpl&);
	};

private:
	TemplateProcessor(const TemplateProcessor&);
	TemplateProcessor& operator=(const TemplateProcessor&);

private:
	Document* pDocument_;
	FolderModelBase* pFolderModel_;
	MessageSelectionModel* pMessageSelectionModel_;
	EncodingModel* pEncodingModel_;
	SecurityModel* pSecurityModel_;
	EditFrameWindowManager* pEditFrameWindowManager_;
	ExternalEditorManager* pExternalEditorManager_;
	const ActionInvoker* pActionInvoker_;
	HWND hwnd_;
	qs::Profile* pProfile_;
	bool bExternalEditor_;
	qs::wstring_ptr wstrTempDir_;
};

}

#endif // __TEMPLATEPROCESSOR_H__
