/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qm.h>
#include <qmapplication.h>
#include <qmmain.h>

#include <qsfile.h>
#include <qsosutil.h>
#include <qsthread.h>

#include <memory>

#include <tchar.h>
#include <windows.h>

#include "main.h"
#include "../ui/dialogs.h"
#include "../ui/resourceinc.h"

using namespace qm;
using namespace qs;


namespace qm{

int main(const WCHAR* pwszCommandLine);

}


/****************************************************************************
 *
 * Global Variables
 *
 */

HINSTANCE g_hInstDll = 0;


/****************************************************************************
 *
 * DllMain
 *
 */

BOOL WINAPI DllMain(HANDLE hInst,
					DWORD dwReason,
					LPVOID lpReserved)
{
	switch (dwReason) {
	case DLL_PROCESS_ATTACH:
#ifndef NDEBUG
		{
			TCHAR tsz[32];
			wsprintf(tsz, TEXT("qm: %p\n"), hInst);
			::OutputDebugString(tsz);
		}
#endif
		g_hInstDll = static_cast<HINSTANCE>(hInst);
		break;
	case DLL_PROCESS_DETACH:
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	}
	return TRUE;
}


/****************************************************************************
 *
 * Global functions
 *
 */

extern "C" QMEXPORTPROC int qmMain(const WCHAR* pwszCommandLine)
{
	return main(pwszCommandLine);
}

int qm::main(const WCHAR* pwszCommandLine)
{
	MainCommandLineHandler handler;
	CommandLine commandLine(&handler);
	if (!commandLine.parse(pwszCommandLine))
		return 1;
	
	wstring_ptr wstrMailFolder;
	const WCHAR* pwszMailFolder = handler.getMailFolder();
	if (pwszMailFolder && *pwszMailFolder)
		wstrMailFolder = allocWString(pwszMailFolder);
	
	wstring_ptr wstrProfile;
	const WCHAR* pwszProfile = handler.getProfile();
	if (pwszProfile)
		wstrProfile = allocWString(pwszProfile);
	
	if (!wstrMailFolder.get() || !wstrProfile.get()) {
		TCHAR tszPath[MAX_PATH + 1];
		if (::GetModuleFileName(0, tszPath, MAX_PATH) > 5 &&
			_tcsicmp(tszPath + _tcslen(tszPath) - 5, _T("x.exe")) == 0) {
			if (!wstrMailFolder.get()) {
				wstring_ptr wstrPath(tcs2wcs(tszPath));
				const WCHAR* p = wcsrchr(wstrPath.get(), L'\\');
				if (p)
					wstrMailFolder = concat(wstrPath.get(), p - wstrPath.get(), L"\\mail", -1);
				else
					wstrMailFolder = allocWString(L"\\mail");
				if (!File::createDirectory(wstrMailFolder.get()))
					return 1;
			}
			if (!wstrProfile.get())
				wstrProfile = allocWString(L"");
		}
	}
	
	if (!wstrMailFolder.get() || !wstrProfile.get()) {
		Registry reg(HKEY_CURRENT_USER, L"Software\\sn\\q3\\Setting");
		if (!reg)
			return 1;
		
		if (!wstrMailFolder.get()) {
			bool bSelect = !reg.getValue(L"MailFolder", &wstrMailFolder);
			if (!bSelect)
				bSelect = !wstrMailFolder.get();
			if (!bSelect) {
				W2T(wstrMailFolder.get(), ptszMailFolder);
				DWORD dwAttributes = ::GetFileAttributes(ptszMailFolder);
				bSelect = dwAttributes == -1 ||
					(dwAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
			}
			if (bSelect) {
				// TODO
				// Use resource handle
				MailFolderDialog dialog(g_hInstDll);
				if (dialog.doModal(0) != IDOK)
					return 1;
				wstrMailFolder = allocWString(dialog.getMailFolder());
				reg.setValue(L"MailFolder", wstrMailFolder.get());
			}
			int nLen = wcslen(wstrMailFolder.get());
			if (*(wstrMailFolder.get() + nLen - 1) == L'\\')
				*(wstrMailFolder.get() + nLen - 1) = L'\0';
		}
		
		if (!wstrProfile.get()) {
			if (!reg.getValue(L"Profile", &wstrProfile) || !wstrProfile.get())
				wstrProfile = allocWString(L"");
		}
	}
	
	bool bContinue = false;
	HWND hwndPrev = 0;
	std::auto_ptr<MailFolderLock> pLock(new MailFolderLock(
		wstrMailFolder.get(), &bContinue, &hwndPrev));
	if (!bContinue) {
		if (hwndPrev)
			handler.invoke(hwndPrev);
		return 0;
	}
	
	MailFolderLock* pLockTemp = pLock.get();
	std::auto_ptr<Application> pApplication(new Application(
		g_hInstDll, wstrMailFolder, wstrProfile, pLock));
	
	if (!pApplication->initialize())
		return 1;
	
	assert(getMainWindow());
	pLockTemp->setWindow(getMainWindow()->getHandle());
	handler.invoke(getMainWindow()->getHandle());
	
	pApplication->run();
	
	pApplication->uninitialize();
	
	return 0;
}


/****************************************************************************
 *
 * MainCommandLineHandler
 *
 */

qm::MainCommandLineHandler::MainCommandLineHandler() :
	state_(STATE_NONE),
	nAction_(0)
{
}

qm::MainCommandLineHandler::~MainCommandLineHandler()
{
}

const WCHAR* qm::MainCommandLineHandler::getMailFolder() const
{
	return wstrMailFolder_.get();
}

const WCHAR* qm::MainCommandLineHandler::getProfile() const
{
	return wstrProfile_.get();
}

void qm::MainCommandLineHandler::invoke(HWND hwnd)
{
	struct {
		unsigned int nAction_;
		unsigned int nActionNoParam_;
		WCHAR* pwsz_;
	} commands[] = {
		{ IDM_TOOL_GOROUND,				0,									wstrGoRound_.get()	},
		{ IDM_MESSAGE_OPENURL,			0,									wstrURL_.get()		},
		{ IDM_MESSAGE_CREATEFROMFILE,	IDM_MESSAGE_CREATEFROMCLIPBOARD,	wstrPath_.get()		},
		{ IDM_MESSAGE_DRAFTFROMFILE,	IDM_MESSAGE_DRAFTFROMCLIPBOARD,		wstrPath_.get()		}
	};
	
	COPYDATASTRUCT data = {
		0,
		0,
		0
	};
	for (int n = 0; n < countof(commands) && data.dwData == 0; ++n) {
		if (commands[n].nAction_ == nAction_) {
			if (commands[n].pwsz_) {
				data.dwData = commands[n].nAction_;
				data.cbData = (wcslen(commands[n].pwsz_) + 1)*sizeof(WCHAR);
				data.lpData = commands[n].pwsz_;
			}
			else {
				data.dwData = commands[n].nActionNoParam_;
			}
		}
	}
	if (data.dwData != 0)
		::SendMessage(hwnd, WM_COPYDATA, 0, reinterpret_cast<LPARAM>(&data));
}

bool qm::MainCommandLineHandler::process(const WCHAR* pwszOption)
{
	struct {
		const WCHAR* pwsz_;
		State state_;
	} options[] = {
		{ L"d",	STATE_MAILFOLDER	},
		{ L"p",	STATE_PROFILE		},
		{ L"g",	STATE_GOROUND		},
		{ L"s",	STATE_URL			},
		{ L"c",	STATE_CREATE		},
		{ L"r",	STATE_DRAFT			}
	};
	
	wstring_ptr* pwstr[] = {
		&wstrMailFolder_,
		&wstrProfile_,
		&wstrGoRound_,
		&wstrURL_
	};
	
	unsigned int nActions[] = {
		IDM_TOOL_GOROUND,
		IDM_MESSAGE_OPENURL,
		IDM_MESSAGE_CREATEFROMFILE,
		IDM_MESSAGE_DRAFTFROMFILE
	};
	
	if (state_ == STATE_CREATE || state_ == STATE_DRAFT) {
		if (*pwszOption == L'-' || *pwszOption == L'/')
			state_ = STATE_NONE;
	}
	
	switch (state_) {
	case STATE_NONE:
		if (*pwszOption == L'-' || *pwszOption == L'/') {
			for (int n = 0; n < countof(options) && state_ == STATE_NONE; ++n) {
				if (wcscmp(pwszOption + 1, options[n].pwsz_) == 0)
					state_ = options[n].state_;
			}
			if (STATE_GOROUND <= state_ && state_ <= STATE_DRAFT)
				nAction_ = nActions[state_ - STATE_GOROUND];
		}
		break;
	case STATE_MAILFOLDER:
	case STATE_PROFILE:
	case STATE_GOROUND:
	case STATE_URL:
		*pwstr[state_ - STATE_MAILFOLDER] = allocWString(pwszOption);
		state_ = STATE_NONE;
		break;
	case STATE_CREATE:
	case STATE_DRAFT:
		wstrPath_ = allocWString(pwszOption);
		break;
	default:
		break;
	}
	
	return true;
}


/****************************************************************************
 *
 * MailFolderLock
 *
 */

qm::MailFolderLock::MailFolderLock(const WCHAR* pwszMailFolder,
								   bool* pbContinue,
								   HWND* phwnd) :
	hFile_(0)
{
	lock(pwszMailFolder, pbContinue, phwnd);
}

qm::MailFolderLock::~MailFolderLock()
{
	unlock();
}

bool qm::MailFolderLock::setWindow(HWND hwnd)
{
	assert(hwnd);
	assert(hFile_);
	assert(pMutex_.get());
	
	wstring_ptr wstrName;
#ifdef _WIN32_WCE
	Registry reg(HKEY_LOCAL_MACHINE, L"Ident");
	if (reg)
		reg.getValue(L"Name", &wstrName);
#else
	TCHAR tszComputerName[MAX_COMPUTERNAME_LENGTH + 1];
	DWORD dwSize = countof(tszComputerName);
	::GetComputerName(tszComputerName, &dwSize);
	wstrName = tcs2wcs(tszComputerName);
#endif
	
	WCHAR wszHandle[32];
	swprintf(wszHandle, L"%08x\n", reinterpret_cast<int>(hwnd));
	
	const WCHAR* pwsz[] = {
		wszHandle,
		wstrName.get()
	};
	for (int n = 0; n < countof(pwsz); ++n) {
		DWORD dw = 0;
		if (!::WriteFile(hFile_, pwsz[n], wcslen(pwsz[n])*sizeof(WCHAR), &dw, 0))
			return false;
	}
	if (!::FlushFileBuffers(hFile_))
		return false;
	
	pMutex_->release();
	
	return true;
}

void qm::MailFolderLock::unsetWindow()
{
	assert(pMutex_.get());
	pMutex_->acquire();
}

void qm::MailFolderLock::lock(const WCHAR* pwszMailFolder,
							  bool* pbContinue,
							  HWND* phwnd)
{
	assert(pwszMailFolder);
	assert(pbContinue);
	
	*pbContinue = false;
	
	wstring_ptr wstrPath(concat(pwszMailFolder, L"\\lock"));
	tstring_ptr tstrPath(wcs2tcs(wstrPath.get()));
	
	std::auto_ptr<Mutex> pMutex(new Mutex(false, L"QMAIL3Mutex"));
	pMutex->acquire();
	
	AutoHandle hFile(::CreateFile(tstrPath.get(),
		GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, 0,
		OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0));
	if (!hFile.get()) {
		AutoHandle hFileRead(::CreateFile(tstrPath.get(),
			GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
			0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0));
		if (!hFileRead.get())
			return;
		
		HWND hwnd = 0;
		if (read(hFileRead.get(), &hwnd, 0))
			::SetForegroundWindow(hwnd);
		*phwnd = hwnd;
	}
	else if (::GetLastError() == ERROR_ALREADY_EXISTS) {
		const WCHAR* pwszName = L"Unknown";
		wstring_ptr wstrName;
		if (read(hFile.get(), 0, &wstrName))
			pwszName = wstrName.get();
		
		wstring_ptr wstrTemplate(loadString(g_hInstDll, IDS_CONFIRMIGNORELOCK));
		wstring_ptr wstrMessage(allocWString(
			wcslen(wstrTemplate.get()) + wcslen(pwszName)));
		swprintf(wstrMessage.get(), wstrTemplate.get(), pwszName);
		
		int nRet= messageBox(wstrMessage.get(),
			MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);
		*pbContinue = nRet == IDYES;
	}
	else {
		*pbContinue = true;
	}
	
	if (*pbContinue) {
		hFile_ = hFile.release();
		tstrPath_ = tstrPath;
		pMutex_ = pMutex;
	}
	else {
		pMutex->release();
	}
}

void qm::MailFolderLock::unlock()
{
	if (hFile_) {
		assert(tstrPath_.get());
		assert(pMutex_.get());
		
		::CloseHandle(hFile_);
		::DeleteFile(tstrPath_.get());
		
		pMutex_->release();
		
		hFile_ = 0;
		pMutex_.reset(0);
		tstrPath_.reset(0);
	}
	assert(!hFile_);
	assert(!pMutex_.get());
	assert(!tstrPath_.get());
}

bool qm::MailFolderLock::read(HANDLE hFile,
							  HWND* phwnd,
							  wstring_ptr* pwstrName)
{
	assert(hFile);
	
	WCHAR wsz[1024];
	DWORD dw = 0;
	if (!::ReadFile(hFile, wsz, sizeof(wsz) - sizeof(WCHAR), &dw, 0))
		return false;
	wsz[dw/sizeof(WCHAR)] = L'\0';
	::SetFilePointer(hFile, 0, 0, FILE_BEGIN);
	
	WCHAR* p = wcschr(wsz, L'\n');
	if (!p)
		return false;
	*p = L'\0';
	
	if (phwnd) {
		int hwnd = 0;
		swscanf(wsz, L"%08x", &hwnd);
		*phwnd = reinterpret_cast<HWND>(hwnd);
	}
	
	if (pwstrName)
		*pwstrName = allocWString(p + 1);
	
	return true;
}
