/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmapplication.h>
#include <qmmessage.h>

#include <qsconv.h>
#include <qsosutil.h>
#include <qsstream.h>

#include "externaleditor.h"
#include "uiutil.h"

#pragma warning(disable:4786)

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * ExternalEditorManager
 *
 */

qm::ExternalEditorManager::ExternalEditorManager(Document* pDocument,
												 Profile* pProfile,
												 HWND hwnd,
												 TempFileCleaner* pTempFileCleaner,
												 FolderModel* pFolderModel,
												 SecurityModel* pSecurityModel) :
	composer_(false, pDocument, pProfile, hwnd, pFolderModel, pSecurityModel),
	pProfile_(pProfile),
	hwnd_(hwnd),
	pTempFileCleaner_(pTempFileCleaner)
{
}

qm::ExternalEditorManager::~ExternalEditorManager()
{
	if (pThread_.get())
		pThread_->stop();
	
	for (ItemList::iterator it = listItem_.begin(); it != listItem_.end(); ++it) {
		Item& item = *it;
		freeWString(item.wstrPath_);
		::CloseHandle(item.hProcess_);
	}
}

bool qm::ExternalEditorManager::open(const WCHAR* pwszMessage)
{
	wstring_ptr wstrPath(UIUtil::writeTemporaryFile(
		pwszMessage, L"q3edit", L"txt", pTempFileCleaner_));
	if (!wstrPath.get())
		return false;
	
	W2T(wstrPath.get(), ptszPath);
	WIN32_FIND_DATA fd;
	AutoFindHandle hFind(::FindFirstFile(ptszPath, &fd));
	if (!hFind.get())
		return false;
	
	wstring_ptr wstrEditor(pProfile_->getString(L"Global", L"ExternalEditor", L""));
	if (!*wstrEditor.get())
		wstrEditor = pProfile_->getString(L"Global", L"Editor", L"");
	
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
	
	wstring_ptr wstrParam(createParam(pParam, wstrPath.get()));
	
	W2T(pFile, ptszFile);
	W2T(wstrParam.get(), ptszParam);
	
	SHELLEXECUTEINFO sei = {
		sizeof(sei),
		SEE_MASK_NOCLOSEPROCESS,
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
	if (!::ShellExecuteEx(&sei))
		return false;
	
	AutoHandle hProcess(sei.hProcess);
	
	Item item = {
		wstrPath.get(),
		{
			fd.ftLastWriteTime.dwLowDateTime,
			fd.ftLastWriteTime.dwHighDateTime
		},
		hProcess.get()
	};
	{
		Lock<CriticalSection> lock(cs_);
		listItem_.push_back(item);
	}
	wstrPath.release();
	hProcess.release();
	
	if (!pThread_.get()) {
		pEvent_.reset(new Event());
		pThread_.reset(new WaitThread(this));
		if (!pThread_->start())
			return false;
	}
	else {
		pEvent_->set();
	}
	
	return true;
}

wstring_ptr qm::ExternalEditorManager::createParam(const WCHAR* pwszTemplate,
												   const WCHAR* pwszPath)
{
	assert(pwszPath);
	
	StringBuffer<WSTRING> bufParam;
	if (pwszTemplate) {
		const WCHAR* p = wcsstr(pwszTemplate, L"%f");
		if (p) {
			bufParam.append(pwszTemplate, p - pwszTemplate);
			bufParam.append(pwszPath);
			bufParam.append(p + 2);
		}
		else {
			bufParam.append(pwszTemplate);
			bufParam.append(L' ');
			bufParam.append(pwszPath);
		}
	}
	else {
		bufParam.append(pwszPath);
	}
	
	return bufParam.getString();
}

bool qm::ExternalEditorManager::createMessage(const WCHAR* pwszPath)
{
	assert(pwszPath);
	
	FileInputStream stream(pwszPath);
	if (!stream)
		return false;
	BufferedInputStream bufferedStream(&stream, false);
	InputStreamReader reader(&bufferedStream, false, getSystemEncoding());
	if (!reader)
		return false;
	
	typedef std::vector<WCHAR> Buffer;
	Buffer buffer;
	size_t nSize = 0;
	while (true) {
		buffer.resize(nSize + 1024);
		size_t nRead = reader.read(&buffer[nSize], 1024);
		if (nRead == -1)
			return false;
		else if (nRead == 0)
			break;
		nSize += nRead;
	}
	if (!reader.close())
		return false;
	
	if (nSize != 0) {
		MessageCreator creator(MessageCreator::FLAG_ADDCONTENTTYPE |
			MessageCreator::FLAG_EXPANDALIAS);
		std::auto_ptr<Message> pMessage(creator.createMessage(&buffer[0], nSize));
		
		unsigned int nFlags = 0;
		// TODO
		// Set flags
		if (!composer_.compose(0, 0, pMessage.get(), nFlags))
			return false;
	}
	
	return true;
}


/****************************************************************************
 *
 * ExternalEditorManager::WaitThread
 *
 */

qm::ExternalEditorManager::WaitThread::WaitThread(ExternalEditorManager* pManager) :
	pManager_(pManager),
	bStop_(false)
{
}

qm::ExternalEditorManager::WaitThread::~WaitThread()
{
}

void qm::ExternalEditorManager::WaitThread::stop()
{
	bStop_ = true;
	pManager_->pEvent_->set();
	join();
}

void qm::ExternalEditorManager::WaitThread::run()
{
	while (true) {
		typedef std::vector<HANDLE> HandleList;
		HandleList listHandle;
		{
			Lock<CriticalSection> lock(pManager_->cs_);
			listHandle.resize(pManager_->listItem_.size() + 1);
			listHandle[0] = pManager_->pEvent_->getHandle();
			std::transform(pManager_->listItem_.begin(),
				pManager_->listItem_.end(),
				listHandle.begin() + 1,
				mem_data_ref(&Item::hProcess_));
		}
		
		DWORD dw = ::WaitForMultipleObjects(
			listHandle.size(), &listHandle[0], FALSE, INFINITE);
		HANDLE handle = 0;
		if (WAIT_OBJECT_0 <= dw && dw < WAIT_OBJECT_0 + listHandle.size()) {
			handle = listHandle[dw - WAIT_OBJECT_0];
		}
		else if (WAIT_ABANDONED_0 <= dw && dw < WAIT_ABANDONED_0 + listHandle.size()) {
			handle = listHandle[dw - WAIT_ABANDONED_0];
		}
		else {
			// TODO
		}
		
		if (handle == pManager_->pEvent_->getHandle()) {
			if (bStop_)
				return;
		}
		else {
			wstring_ptr wstrPath;
			FILETIME ft;
			{
				Lock<CriticalSection> lock(pManager_->cs_);
				ItemList::iterator it = std::find_if(
					pManager_->listItem_.begin(), pManager_->listItem_.end(),
					std::bind2nd(
						binary_compose_f_gx_hy(
							std::equal_to<HANDLE>(),
							mem_data_ref(&Item::hProcess_),
							std::identity<HANDLE>()),
						handle));
				assert(it != pManager_->listItem_.end());
				
				Item& item = *it;
				::CloseHandle(item.hProcess_);
				wstrPath.reset(item.wstrPath_);
				ft = item.ft_;
				pManager_->listItem_.erase(it);
			}
			
			W2T(wstrPath.get(), ptszPath);
			WIN32_FIND_DATA fd;
			AutoFindHandle hFind(::FindFirstFile(ptszPath, &fd));
			if (hFind.get() && ::CompareFileTime(&ft, &fd.ftLastWriteTime) != 0) {
				if (!pManager_->createMessage(wstrPath.get())) {
					// TODO MSG
				}
				::DeleteFile(ptszPath);
			}
		}
	}
}
