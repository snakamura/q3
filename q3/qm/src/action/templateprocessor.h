/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __TEMPLATEPROCESSOR_H__
#define __TEMPLATEPROCESSOR_H__

#include <qmmacro.h>

#include <qs.h>
#include <qsprofile.h>
#include <qsstring.h>


namespace qm {

class TemplateProcessor;

class Document;
class EditFrameWindowManager;
class ExternalEditorManager;
class FolderModel;
class MessageSelectionModel;
class TemplateManager;


/****************************************************************************
 *
 * TemplateProcessor
 *
 */

class TemplateProcessor
{
public:
	TemplateProcessor(Document* pDocument, FolderModel* pFolderModel,
		MessageSelectionModel* pMessageSelectionModel,
		EditFrameWindowManager* pEditFrameWindowManager,
		ExternalEditorManager* pExternalEditorManager,
		HWND hwnd, qs::Profile* pProfile, bool bExternalEditor);
	~TemplateProcessor();

public:
	qs::QSTATUS process(const WCHAR* pwszTemplateName,
		bool bReverseExternalEditor) const;

private:
	class MacroErrorHandlerImpl : public MacroErrorHandler
	{
	public:
		MacroErrorHandlerImpl();
		virtual ~MacroErrorHandlerImpl();
	
	public:
		virtual void parseError(Code code, const WCHAR* pwsz);
		virtual void processError(Code code, const WCHAR* pwsz);
	
	private:
		MacroErrorHandlerImpl(const MacroErrorHandlerImpl&);
		MacroErrorHandlerImpl& operator=(const MacroErrorHandlerImpl&);
	};

private:
	TemplateProcessor(const TemplateProcessor&);
	TemplateProcessor& operator=(const TemplateProcessor&);

private:
	Document* pDocument_;
	FolderModel* pFolderModel_;
	MessageSelectionModel* pMessageSelectionModel_;
	EditFrameWindowManager* pEditFrameWindowManager_;
	ExternalEditorManager* pExternalEditorManager_;
	HWND hwnd_;
	qs::Profile* pProfile_;
	bool bExternalEditor_;
};

}

#endif // __TEMPLATEPROCESSOR_H__
