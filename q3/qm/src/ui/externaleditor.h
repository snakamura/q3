/*
 * $Id: $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __EXTERNALEDITOR_H__
#define __EXTERNALEDITOR_H__

#include <qm.h>

#include <qs.h>
#include <qsprofile.h>
#include <qsthread.h>

#include "messagecomposer.h"


namespace qm {

class ExternalEditorManager;

class Document;
class FolderModel;


/****************************************************************************
 *
 * ExternalEditorManager
 *
 */

class ExternalEditorManager
{
public:
	ExternalEditorManager(Document* pDocument, qs::Profile* pProfile,
		HWND hwnd, FolderModel* pFolderModel, qs::QSTATUS* pstatus);
	~ExternalEditorManager();

public:
	qs::QSTATUS open(const WCHAR* pwszMessage);

private:
	qs::QSTATUS prepareTemporaryFile(
		const WCHAR* pwszMessage, qs::WSTRING* pwstrPath);
	qs::QSTATUS createParam(const WCHAR* pwszTemplate,
		const WCHAR* pwszPath, qs::WSTRING* pwstrParam);
	qs::QSTATUS createMessage(const WCHAR* pwszPath);

private:
	ExternalEditorManager(const ExternalEditorManager&);
	ExternalEditorManager& operator=(const ExternalEditorManager&);

private:
	struct Item
	{
		qs::WSTRING wstrPath_;
		FILETIME ft_;
		HANDLE hProcess_;
	};
	
	class WaitThread : public qs::Thread
	{
	public:
		WaitThread(ExternalEditorManager* pManager, qs::QSTATUS* pstatus);
		virtual ~WaitThread();
	
	public:
		void stop();
	
	public:
		virtual unsigned int run();
	
	private:
		WaitThread(const WaitThread&);
		WaitThread& operator=(const WaitThread&);
	
	private:
		ExternalEditorManager* pManager_;
		volatile bool bStop_;
	};
	friend class WaitThread;

private:
	typedef std::vector<Item> ItemList;

private:
	MessageComposer composer_;
	qs::Profile* pProfile_;
	HWND hwnd_;
	ItemList listItem_;
	WaitThread* pThread_;
	qs::Event* pEvent_;
	qs::CriticalSection cs_;
};

}

#endif // __EXTERNALEDITOR_H__
