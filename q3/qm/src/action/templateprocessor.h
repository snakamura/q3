/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
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

class Document;
class EditFrameWindowManager;
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
					  SecurityModel* pSecurityModel,
					  EditFrameWindowManager* pEditFrameWindowManager,
					  ExternalEditorManager* pExternalEditorManager,
					  HWND hwnd,
					  qs::Profile* pProfile,
					  bool bExternalEditor);
	~TemplateProcessor();

public:
	bool process(const WCHAR* pwszTemplateName,
				 bool bReverseExternalEditor) const;
	bool process(const WCHAR* pwszTemplateName,
				 const TemplateContext::ArgumentList& listArgument,
				 bool bReverseExternalEditor) const;

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
	SecurityModel* pSecurityModel_;
	EditFrameWindowManager* pEditFrameWindowManager_;
	ExternalEditorManager* pExternalEditorManager_;
	HWND hwnd_;
	qs::Profile* pProfile_;
	bool bExternalEditor_;
};

}

#endif // __TEMPLATEPROCESSOR_H__
