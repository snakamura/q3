/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsconv.h>
#include <qsstl.h>
#include <qsstream.h>

#include <algorithm>

#include <tchar.h>

#include "scriptmanager.h"

using namespace qm;
using namespace qmscript;
using namespace qs;


/****************************************************************************
 *
 * ScriptManager
 *
 */

qm::ScriptManager::ScriptManager(const WCHAR* pwszPath) :
	hInst_(0),
	pFactory_(0)
{
	wstrPath_ = allocWString(pwszPath);
	
#ifdef NDEBUG
#	ifdef UNICODE
#		define SUFFIX _T("u")
#	else
#		define SUFFIX _T("")
#	endif
#else
#	ifdef UNICODE
#		define SUFFIX _T("ud")
#	else
#		define SUFFIX _T("d")
#	endif
#endif
	hInst_ = ::LoadLibrary(_T("qmscript") SUFFIX _T(".dll"));
	if (hInst_) {
		typedef ScriptFactory* (*PFN_NEW)();
		PFN_NEW pfnNew = reinterpret_cast<PFN_NEW>(
			::GetProcAddress(hInst_, WCE_T("newScriptFactory")));
		if (pfnNew)
			pFactory_ = (*pfnNew)();
	}
}

qm::ScriptManager::~ScriptManager()
{
	if (hInst_) {
		typedef void (*PFN_DELETE)(ScriptFactory*);
		PFN_DELETE pfnDelete = reinterpret_cast<PFN_DELETE>(
			::GetProcAddress(hInst_, WCE_T("deleteScriptFactory")));
		if (pfnDelete)
			(*pfnDelete)(pFactory_);
		
		::FreeLibrary(hInst_);
	}
}

std::auto_ptr<Script> qm::ScriptManager::getScript(const WCHAR* pwszName,
												   Document* pDocument,
												   Profile* pProfile,
												   ModalHandler* pModalHandler,
												   const WindowInfo& info) const
{
	assert(pwszName);
	assert(pDocument);
	assert(pModalHandler);
	
	if (!pFactory_)
		return 0;
	
	ConcatW c[] = {
		{ wstrPath_.get(),	-1 },
		{ L"\\scripts\\",	-1 },
		{ pwszName,			-1 },
		{ L".*",			-1 }
	};
	
	wstring_ptr wstrFind(concat(c, countof(c)));
	
	W2T(wstrFind.get(), ptszFind);
	WIN32_FIND_DATA fd;
	AutoFindHandle hFind(::FindFirstFile(ptszFind, &fd));
	if (!hFind.get())
		return 0;
	T2W(fd.cFileName, pwszFileName);
	
	struct {
		const WCHAR* pwszExt_;
		const WCHAR* pwszLang_;
	} langs[] = {
		{ L"js",	L"JScript"	},
		{ L"vbs",	L"VBScript"	}
	};
	
	const WCHAR* pwszLang = 0;
	const WCHAR* pExt = wcsrchr(pwszFileName, L'.');
	if (pExt) {
		for (int n = 0; n < countof(langs) && !pwszLang; ++n) {
			if (wcsicmp(langs[n].pwszExt_, pExt + 1) == 0)
				pwszLang = langs[n].pwszLang_;
		}
	}
	if (!pwszLang)
		pwszLang = L"JScript";
	
	wstring_ptr wstrPath(concat(wstrPath_.get(), L"\\scripts\\", pwszFileName));
	
	FileInputStream stream(wstrPath.get());
	if (!stream)
		return 0;
	BufferedInputStream bufferedStream(&stream, false);
	InputStreamReader reader(&bufferedStream, false, 0);
	if (!reader)
		return 0;
	
	ScriptFactory::Init init = {
		pwszLang,
		&reader,
		pDocument,
		pProfile,
		0,
		pModalHandler,
		ScriptFactory::TYPE_NONE,
		0
	};
	
	switch (info.type_) {
	case TYPE_MAIN:
		init.hwnd_ = info.window_.pMainWindow_->getHandle();
		init.type_ = ScriptFactory::TYPE_MAIN;
		init.window_.pMainWindow_ = info.window_.pMainWindow_;
		break;
	case TYPE_EDIT:
		init.hwnd_ = info.window_.pEditFrameWindow_->getHandle();
		init.type_ = ScriptFactory::TYPE_EDIT;
		init.window_.pEditFrameWindow_ = info.window_.pEditFrameWindow_;
		break;
	case TYPE_MESSAGE:
		init.hwnd_ = info.window_.pMessageFrameWindow_->getHandle();
		init.type_ = ScriptFactory::TYPE_EDIT;
		init.window_.pMessageFrameWindow_ = info.window_.pMessageFrameWindow_;
		break;
	default:
		assert(false);
		break;
	}
	
	return pFactory_->newScript(init);
}

void qm::ScriptManager::getScriptNames(NameList* pList) const
{
	if (!pFactory_)
		return;
	
	NameList l;
	StringListFree<NameList> free(l);
	
	wstring_ptr wstrFind(concat(wstrPath_.get(), L"\\scripts\\*.*"));
	
	W2T(wstrFind.get(), ptszFind);
	WIN32_FIND_DATA fd;
	AutoFindHandle hFind(::FindFirstFile(ptszFind, &fd));
	if (hFind.get()) {
		do {
			if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
				wstring_ptr wstrName(tcs2wcs(fd.cFileName));
				WCHAR* p = wcsrchr(wstrName.get(), L'.');
				if (p)
					*p = L'\0';
				l.push_back(wstrName.get());
				wstrName.release();
			}
		} while (::FindNextFile(hFind.get(), &fd));
	}
	
	pList->assign(l.begin(), l.end());
	free.release();
}

std::auto_ptr<Script> qm::ScriptManager::createScript(const WCHAR* pwszScript,
													  const WCHAR* pwszLanguage,
													  Document* pDocument,
													  Profile* pProfile,
													  HWND hwnd,
													  ModalHandler* pModalHandler) const
{
	if (!pFactory_)
		return 0;
	
	StringReader reader(pwszScript, false);
	if (!reader)
		return 0;
	ScriptFactory::Init init = {
		pwszLanguage,
		&reader,
		pDocument,
		pProfile,
		hwnd,
		pModalHandler,
		ScriptFactory::TYPE_NONE
	};
	return pFactory_->newScript(init);
}
