/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
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
class TempFileCleaner;


/****************************************************************************
 *
 * ExternalEditorManager
 *
 */

class ExternalEditorManager
{
public:
	ExternalEditorManager(Document* pDocument,
						  qs::Profile* pProfile,
						  HWND hwnd,
						  TempFileCleaner* pTempFileCleaner,
						  FolderModel* pFolderModel);
	~ExternalEditorManager();

public:
	bool open(const WCHAR* pwszMessage);

private:
	qs::wstring_ptr createParam(const WCHAR* pwszTemplate,
								const WCHAR* pwszPath);
	bool createMessage(const WCHAR* pwszPath);

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
		WaitThread(ExternalEditorManager* pManager);
		virtual ~WaitThread();
	
	public:
		void stop();
	
	public:
		virtual void run();
	
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
	TempFileCleaner* pTempFileCleaner_;
	ItemList listItem_;
	std::auto_ptr<WaitThread> pThread_;
	std::auto_ptr<qs::Event> pEvent_;
	qs::CriticalSection cs_;
};

}

#endif // __EXTERNALEDITOR_H__
