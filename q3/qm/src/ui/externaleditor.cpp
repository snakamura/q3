/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmapplication.h>
#include <qmmessage.h>

#include <qsconv.h>
#include <qsnew.h>
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
	Profile* pProfile, HWND hwnd, TempFileCleaner* pTempFileCleaner,
	FolderModel* pFolderModel, QSTATUS* pstatus) :
	composer_(false, pDocument, pProfile, hwnd, pFolderModel),
	pProfile_(pProfile),
	hwnd_(hwnd),
	pTempFileCleaner_(pTempFileCleaner),
	pThread_(0),
	pEvent_(0)
{
}

qm::ExternalEditorManager::~ExternalEditorManager()
{
	if (pThread_) {
		pThread_->stop();
		delete pThread_;
	}
	delete pEvent_;
	
	ItemList::iterator it = listItem_.begin();
	while (it != listItem_.end()) {
		Item& item = *it;
		freeWString(item.wstrPath_);
		::CloseHandle(item.hProcess_);
		++it;
	}
}

QSTATUS qm::ExternalEditorManager::open(const WCHAR* pwszMessage)
{
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrPath;
	status = UIUtil::writeTemporaryFile(pwszMessage,
		L"q3edit", L"txt", pTempFileCleaner_, &wstrPath);
	CHECK_QSTATUS();
	
	W2T(wstrPath.get(), ptszPath);
	WIN32_FIND_DATA fd;
	AutoFindHandle hFind(::FindFirstFile(ptszPath, &fd));
	if (!hFind.get())
		return QSTATUS_FAIL;
	
	string_ptr<WSTRING> wstrEditor;
	status = pProfile_->getString(L"Global", L"ExternalEditor", L"", &wstrEditor);
	CHECK_QSTATUS();
	if (!*wstrEditor.get()) {
		wstrEditor.reset(0);
		status = pProfile_->getString(L"Global", L"Editor", L"", &wstrEditor);
		CHECK_QSTATUS();
	}
	
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
	
	string_ptr<WSTRING> wstrParam;
	status = createParam(pParam, wstrPath.get(), &wstrParam);
	CHECK_QSTATUS();
	
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
	::ShellExecuteEx(&sei);
	
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
		status = STLWrapper<ItemList>(listItem_).push_back(item);
		CHECK_QSTATUS();
	}
	wstrPath.release();
	hProcess.release();
	
	if (!pThread_) {
		status = newQsObject(&pEvent_);
		CHECK_QSTATUS();
		
		status = newQsObject(this, &pThread_);
		CHECK_QSTATUS();
		status = pThread_->start();
		CHECK_QSTATUS();
	}
	else {
		pEvent_->set();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ExternalEditorManager::createParam(const WCHAR* pwszTemplate,
	const WCHAR* pwszPath, WSTRING* pwstrParam)
{
	assert(pwszPath);
	assert(pwstrParam);
	
	DECLARE_QSTATUS();
	
	StringBuffer<WSTRING> bufParam(&status);
	CHECK_QSTATUS();
	if (pwszTemplate) {
		const WCHAR* p = wcsstr(pwszTemplate, L"%f");
		if (p) {
			status = bufParam.append(pwszTemplate, p - pwszTemplate);
			CHECK_QSTATUS();
			status = bufParam.append(pwszPath);
			CHECK_QSTATUS();
			status = bufParam.append(p + 2);
			CHECK_QSTATUS();
		}
		else {
			status = bufParam.append(pwszTemplate);
			CHECK_QSTATUS();
			status = bufParam.append(L' ');
			CHECK_QSTATUS();
			status = bufParam.append(pwszPath);
			CHECK_QSTATUS();
		}
	}
	else {
		status = bufParam.append(pwszPath);
		CHECK_QSTATUS();
	}
	
	*pwstrParam = bufParam.getString();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ExternalEditorManager::createMessage(const WCHAR* pwszPath)
{
	assert(pwszPath);
	
	DECLARE_QSTATUS();
	
	FileInputStream stream(pwszPath, &status);
	CHECK_QSTATUS();
	BufferedInputStream bufferedStream(&stream, false, &status);
	CHECK_QSTATUS();
	InputStreamReader reader(&bufferedStream,
		false, getSystemEncoding(), &status);
	CHECK_QSTATUS();
	
	typedef std::vector<WCHAR> Buffer;
	Buffer buffer;
	size_t nSize = 0;
	while (true) {
		status = STLWrapper<Buffer>(buffer).resize(nSize + 1024);
		CHECK_QSTATUS();
		size_t nRead = 0;
		status = reader.read(&buffer[nSize], 1024, &nRead);
		CHECK_QSTATUS();
		if (nRead == -1)
			break;
		nSize += nRead;
	}
	status = reader.close();
	CHECK_QSTATUS();
	
	if (nSize != 0) {
		Message* p = 0;
		MessageCreator creator(MessageCreator::FLAG_ADDCONTENTTYPE |
			MessageCreator::FLAG_EXPANDALIAS);
		status = creator.createMessage(&buffer[0], nSize, &p);
		CHECK_QSTATUS();
		std::auto_ptr<Message> pMessage(p);
		
		unsigned int nFlags = 0;
		// TODO
		// Set flags
		status = composer_.compose(0, 0, pMessage.get(), nFlags);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * ExternalEditorManager::WaitThread
 *
 */

qm::ExternalEditorManager::WaitThread::WaitThread(
	ExternalEditorManager* pManager, QSTATUS* pstatus) :
	Thread(pstatus),
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

unsigned int qm::ExternalEditorManager::WaitThread::run()
{
	DECLARE_QSTATUS();
	
	while (true) {
		typedef std::vector<HANDLE> HandleList;
		HandleList listHandle;
		{
			Lock<CriticalSection> lock(pManager_->cs_);
			status = STLWrapper<HandleList>(listHandle).resize(
				pManager_->listItem_.size() + 1);
			listHandle[0] = pManager_->pEvent_->getHandle();
			// TODO
			// Error
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
				return 0;
		}
		else {
			string_ptr<WSTRING> wstrPath;
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
			
			W2T_STATUS(wstrPath.get(), ptszPath);
			// TODO
			// Error
			WIN32_FIND_DATA fd;
			AutoFindHandle hFind(::FindFirstFile(ptszPath, &fd));
			if (hFind.get() && ::CompareFileTime(&ft, &fd.ftLastWriteTime) != 0) {
				status = pManager_->createMessage(wstrPath.get());
				// TODO
				// Error
				
				::DeleteFile(ptszPath);
			}
		}
	}
	
	return 0;
}
