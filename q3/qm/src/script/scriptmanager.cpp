/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsconv.h>
#include <qserror.h>
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

qm::ScriptManager::ScriptManager(const WCHAR* pwszPath, QSTATUS* pstatus) :
	wstrPath_(0),
	hInst_(0),
	pFactory_(0)
{
	wstrPath_ = allocWString(pwszPath);
	if (!wstrPath_) {
		*pstatus = QSTATUS_OUTOFMEMORY;
		return;
	}
	
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
	freeWString(wstrPath_);
	
	if (hInst_) {
		typedef void (*PFN_DELETE)(ScriptFactory*);
		PFN_DELETE pfnDelete = reinterpret_cast<PFN_DELETE>(
			::GetProcAddress(hInst_, WCE_T("deleteScriptFactory")));
		if (pfnDelete)
			(*pfnDelete)(pFactory_);
		
		::FreeLibrary(hInst_);
	}
}

QSTATUS qm::ScriptManager::getScript(const WCHAR* pwszName,
	Document* pDocument, Profile* pProfile, ModalHandler* pModalHandler,
	const WindowInfo& info, Script** ppScript) const
{
	assert(pwszName);
	assert(pDocument);
	assert(pModalHandler);
	assert(ppScript);
	
	DECLARE_QSTATUS();
	
	*ppScript = 0;
	
	if (!pFactory_)
		return QSTATUS_SUCCESS;
	
	ConcatW c[] = {
		{ wstrPath_,		-1 },
		{ L"\\scripts\\",	-1 },
		{ pwszName,			-1 },
		{ L".*",			-1 }
	};
	
	string_ptr<WSTRING> wstrFind(concat(c, countof(c)));
	if (!wstrFind.get())
		return QSTATUS_OUTOFMEMORY;
	
	W2T(wstrFind.get(), ptszFind);
	WIN32_FIND_DATA fd;
	AutoFindHandle hFind(::FindFirstFile(ptszFind, &fd));
	if (hFind.get()) {
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
		
		string_ptr<WSTRING> wstrPath(concat(
			wstrPath_, L"\\scripts\\", pwszFileName));
		if (!wstrPath.get())
			return QSTATUS_OUTOFMEMORY;
		
		FileInputStream stream(wstrPath.get(), &status);
		CHECK_QSTATUS();
		BufferedInputStream bufferedStream(&stream, false, &status);
		CHECK_QSTATUS();
		InputStreamReader reader(&bufferedStream, false, 0, &status);
		CHECK_QSTATUS();
		
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
		status = pFactory_->newScript(init, ppScript);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ScriptManager::getScriptNames(NameList* pList) const
{
	DECLARE_QSTATUS();
	
	if (!pFactory_)
		return QSTATUS_SUCCESS;
	
	NameList l;
	StringListFree<NameList> free(l);
	
	string_ptr<WSTRING> wstrFind(concat(wstrPath_, L"\\scripts\\*.*"));
	if (!wstrFind.get())
		return QSTATUS_OUTOFMEMORY;
	
	W2T(wstrFind.get(), ptszFind);
	WIN32_FIND_DATA fd;
	AutoFindHandle hFind(::FindFirstFile(ptszFind, &fd));
	if (hFind.get()) {
		do {
			if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
				string_ptr<WSTRING> wstrName(tcs2wcs(fd.cFileName));
				if (!wstrName.get())
					return QSTATUS_OUTOFMEMORY;
				WCHAR* p = wcsrchr(wstrName.get(), L'.');
				if (p)
					*p = L'\0';
				status = STLWrapper<NameList>(l).push_back(wstrName.get());
				CHECK_QSTATUS();
				wstrName.release();
			}
		} while (::FindNextFile(hFind.get(), &fd));
	}
	
	status = STLWrapper<NameList>(*pList).resize(l.size());
	CHECK_QSTATUS();
	std::copy(l.begin(), l.end(), pList->begin());
	free.release();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::ScriptManager::createScript(const WCHAR* pwszScript,
	const WCHAR* pwszLanguage, Document* pDocument, Profile* pProfile,
	HWND hwnd, ModalHandler* pModalHandler, Script** ppScript) const
{
	DECLARE_QSTATUS();
	
	if (!pFactory_)
		return QSTATUS_SUCCESS;
	
	StringReader reader(pwszScript, &status);
	CHECK_QSTATUS();
	ScriptFactory::Init init = {
		pwszLanguage,
		&reader,
		pDocument,
		pProfile,
		hwnd,
		pModalHandler,
		ScriptFactory::TYPE_NONE
	};
	status = pFactory_->newScript(init, ppScript);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}
