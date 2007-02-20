/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
 * All rights reserved.
 *
 */

#pragma warning(disable:4786)

#include <qm.h>
#include <qmapplication.h>
#include <qmmain.h>

#include <qsfile.h>
#include <qsosutil.h>
#include <qsthread.h>

#include <memory>

#include <shlobj.h>
#include <tchar.h>
#include <windows.h>

#include "main.h"
#include "../ui/actionid.h"
#include "../ui/dialogs.h"

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

HINSTANCE g_hInst = 0;
HINSTANCE g_hInstResource = 0;


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
		g_hInst = static_cast<HINSTANCE>(hInst);
		g_hInstResource = loadResourceDll(g_hInst);
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
	
	if (!wstrMailFolder.get()) {
		TCHAR tszPath[MAX_PATH + 1];
		int nLen = ::GetModuleFileName(0, tszPath, MAX_PATH);
		
		bool bLocal = nLen > 5 && _tcsicmp(tszPath + _tcslen(tszPath) - 5, _T("x.exe")) == 0;
		
		wstring_ptr wstrPath(tcs2wcs(tszPath));
		const WCHAR* p = wcsrchr(wstrPath.get(), L'\\');
		wstring_ptr wstrLocalMailFolder;
		if (p)
			wstrLocalMailFolder = concat(wstrPath.get(), p - wstrPath.get(), L"\\mail", -1);
		else
			wstrLocalMailFolder = allocWString(L"\\mail");
		if (!bLocal)
			bLocal = File::isDirectoryExisting(wstrLocalMailFolder.get());
		if (bLocal)
			wstrMailFolder = wstrLocalMailFolder;
	}
	
	if (!wstrMailFolder.get()) {
		Registry reg(HKEY_CURRENT_USER, L"Software\\sn\\q3\\Setting");
		if (!reg)
			return 1;
		
		if (!wstrMailFolder.get()) {
			bool bSelect = !reg.getValue(L"MailFolder", &wstrMailFolder);
			if (!bSelect)
				bSelect = !wstrMailFolder.get() ||
					!File::isDirectoryExisting(wstrMailFolder.get());
			if (bSelect) {
#ifndef _WIN32_WCE
				if (!wstrMailFolder.get() || !*wstrMailFolder.get()) {
					Library lib(L"shell32.dll");
					if (lib) {
						typedef BOOL (STDAPICALLTYPE* PFN)(HWND, LPWSTR, int, BOOL);
						PFN pfn = reinterpret_cast<PFN>(::GetProcAddress(
							lib, "SHGetSpecialFolderPathW"));
						if (pfn) {
							WCHAR wszAppDir[MAX_PATH];
							if ((*pfn)(0, wszAppDir, CSIDL_APPDATA, TRUE))
								wstrMailFolder = concat(wszAppDir, L"\\QMAIL3");
						}
					}
				}
#endif
				MailFolderDialog dialog(g_hInstResource, wstrMailFolder.get());
				if (dialog.doModal(0) != IDOK)
					return 1;
				wstrMailFolder = allocWString(dialog.getMailFolder());
				reg.setValue(L"MailFolder", wstrMailFolder.get());
			}
			size_t nLen = wcslen(wstrMailFolder.get());
			if (*(wstrMailFolder.get() + nLen - 1) == L'\\')
				*(wstrMailFolder.get() + nLen - 1) = L'\0';
		}
		
		if (!wstrProfile.get())
			reg.getValue(L"Profile", &wstrProfile);
	}
	if (!File::createDirectory(wstrMailFolder.get()))
		return 1;
	
	if (!wstrProfile.get())
		wstrProfile = allocWString(L"");
	
	bool bContinue = false;
	HWND hwndPrev = 0;
	std::auto_ptr<MailFolderLock> pLock(new MailFolderLock(
		wstrMailFolder.get(), &bContinue, &hwndPrev));
	if (!bContinue) {
		if (hwndPrev)
			handler.invoke(hwndPrev, true);
		return 0;
	}
	
	MailFolderLock* pLockTemp = pLock.get();
	std::auto_ptr<Application> pApplication(new Application(
		g_hInst, g_hInstResource, wstrMailFolder, wstrProfile, pLock));
	
	if (!pApplication->initialize())
		return 1;
	
	assert(getMainWindow());
	pLockTemp->setWindow(getMainWindow()->getHandle());
	handler.invoke(getMainWindow()->getHandle(), false);
	
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

void qm::MainCommandLineHandler::invoke(HWND hwnd,
										bool bPrev)
{
	COPYDATASTRUCT data = { 0, 0, 0 };
	wstring_ptr wstrParams;
	switch (nAction_) {
	case IDM_TOOL_GOROUND:
		if (wstrGoRound_.get()) {
			data.dwData = nAction_;
			data.cbData = static_cast<DWORD>((wcslen(wstrGoRound_.get()) + 1)*sizeof(WCHAR));
			data.lpData = wstrGoRound_.get();
		}
		break;
	case IDM_MESSAGE_OPENURL:
		if (wstrURL_.get() || wstrAttachment_.get()) {
			data.dwData = nAction_;
			
			size_t nLen = (wstrURL_.get() ? wcslen(wstrURL_.get()) : 0) + 1 +
				(wstrAttachment_.get() ? wcslen(wstrAttachment_.get())  + 1 : 0);
			wstrParams = allocWString(nLen + 1);
			WCHAR* p = wstrParams.get();
			if (wstrURL_.get()) {
				wcscpy(p, wstrURL_.get());
				p += wcslen(wstrURL_.get()) + 1;
			}
			else {
				*p++ = L'\0';
			}
			if (wstrAttachment_.get())
				wcscpy(p, wstrAttachment_.get());
			
			data.cbData = static_cast<DWORD>(nLen*sizeof(WCHAR));
			data.lpData = wstrParams.get();
		}
		break;
	case IDM_TOOL_INVOKEACTION:
		if (wstrAction_.get()) {
			data.dwData = nAction_;
			data.cbData = static_cast<DWORD>((wcslen(wstrAction_.get()) + 1)*sizeof(WCHAR));
			data.lpData = wstrAction_.get();
		}
		break;
	case IDM_MESSAGE_CREATEFROMFILE:
	case IDM_MESSAGE_DRAFTFROMFILE:
		if (wstrPath_.get()) {
			data.dwData = nAction_;
			data.cbData = static_cast<DWORD>((wcslen(wstrPath_.get()) + 1)*sizeof(WCHAR));
			data.lpData = wstrPath_.get();
		}
		else {
			data.dwData = nAction_ == IDM_MESSAGE_CREATEFROMFILE ?
				IDM_MESSAGE_CREATEFROMCLIPBOARD : IDM_MESSAGE_DRAFTFROMCLIPBOARD;
		}
		break;
	default:
		break;
	}
	if (data.dwData != 0)
		::SendMessage(hwnd, WM_COPYDATA, 0, reinterpret_cast<LPARAM>(&data));
#ifdef _WIN32_WCE_PSPC
	else if (bPrev)
		::SetForegroundWindow(hwnd);
#endif
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
		{ L"a",	STATE_ATTACHMENT	},
		{ L"i",	STATE_ACTION		},
		{ L"c",	STATE_CREATE		},
		{ L"r",	STATE_DRAFT			}
	};
	
	wstring_ptr* pwstr[] = {
		&wstrMailFolder_,
		&wstrProfile_,
		&wstrGoRound_,
		&wstrURL_,
		&wstrAttachment_,
		&wstrAction_
	};
	
	unsigned int nActions[] = {
		IDM_TOOL_GOROUND,
		IDM_MESSAGE_OPENURL,
		IDM_MESSAGE_OPENURL,
		IDM_TOOL_INVOKEACTION,
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
	case STATE_ATTACHMENT:
	case STATE_ACTION:
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
	Registry reg(HKEY_LOCAL_MACHINE, L"Ident", true);
	if (reg)
		reg.getValue(L"Name", &wstrName);
#else
	TCHAR tszComputerName[MAX_COMPUTERNAME_LENGTH + 1];
	DWORD dwSize = countof(tszComputerName);
	::GetComputerName(tszComputerName, &dwSize);
	wstrName = tcs2wcs(tszComputerName);
#endif
	
	WCHAR wszHandle[32];
	_snwprintf(wszHandle, countof(wszHandle), L"%08x\n", reinterpret_cast<int>(hwnd));
	
	const WCHAR* pwsz[] = {
		wszHandle,
		wstrName.get()
	};
	for (int n = 0; n < countof(pwsz); ++n) {
		DWORD dwSize = static_cast<DWORD>(wcslen(pwsz[n])*sizeof(WCHAR));
		DWORD dw = 0;
		if (!::WriteFile(hFile_, pwsz[n], dwSize, &dw, 0))
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
	assert(phwnd);
	
	*pbContinue = false;
	*phwnd = 0;
	
	wstring_ptr wstrPath(concat(pwszMailFolder, L"\\lock"));
	tstring_ptr tstrPath(wcs2tcs(wstrPath.get()));
	
	bool bPrevInstance = false;
	std::auto_ptr<Mutex> pMutex(new Mutex(false, L"QMAIL3Mutex", &bPrevInstance));
	pMutex->acquire();
	
#ifndef _WIN32_WCE
	bPrevInstance = false;
#endif
	
	if (bPrevInstance) {
		HWND hwnd = ::FindWindow(_T("QmMainWindow"), 0);
		if (hwnd) {
#ifndef _WIN32_WCE_PSPC
			COPYDATASTRUCT data = { IDM_FILE_SHOW };
			::SendMessage(hwnd, WM_COPYDATA, 0, reinterpret_cast<LPARAM>(&data));
#endif
		}
		*phwnd = hwnd;
		
		pMutex->release();
	}
	else {
#ifdef _WIN32_WCE
		bool bAlreadyExists = File::isFileExisting(wstrPath.get());
#	define CHECK_ALREADY_EXISTS() (bAlreadyExists)
#else
#	define CHECK_ALREADY_EXISTS() (::GetLastError() == ERROR_ALREADY_EXISTS)
#endif
		
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
			if (read(hFileRead.get(), &hwnd, 0)) {
#ifndef _WIN32_WCE_PSPC
				COPYDATASTRUCT data = { IDM_FILE_SHOW };
				::SendMessage(hwnd, WM_COPYDATA, 0, reinterpret_cast<LPARAM>(&data));
#endif
			}
			*phwnd = hwnd;
		}
		else if (CHECK_ALREADY_EXISTS()) {
			const WCHAR* pwszName = L"Unknown";
			wstring_ptr wstrName;
			if (read(hFile.get(), 0, &wstrName))
				pwszName = wstrName.get();
			
			wstring_ptr wstrTemplate(loadString(g_hInstResource, IDS_CONFIRM_IGNORELOCK));
			const size_t nLen = wcslen(wstrTemplate.get()) + wcslen(pwszName);
			wstring_ptr wstrMessage(allocWString(nLen));
			_snwprintf(wstrMessage.get(), nLen, wstrTemplate.get(), pwszName);
			
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
