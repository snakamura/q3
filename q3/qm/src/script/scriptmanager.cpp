/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#include <qmeditwindow.h>
#include <qmmainwindow.h>
#include <qmmessagewindow.h>

#include <qsconv.h>
#include <qsosutil.h>
#include <qsstream.h>

#include <algorithm>

#include <tchar.h>

#include "scriptmanager.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * ScriptManager
 *
 */

qm::ScriptManager::ScriptManager(const WCHAR* pwszPath)
{
	wstrPath_ = allocWString(pwszPath);
}

qm::ScriptManager::~ScriptManager()
{
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
	
	ScriptFactory* pFactory = ScriptFactory::getFactory();
	if (!pFactory)
		return std::auto_ptr<Script>(0);
	
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
		return std::auto_ptr<Script>(0);
	T2W(fd.cFileName, pwszFileName);
	
	const WCHAR* pExt = wcsrchr(pwszFileName, L'.');
	if (!pExt)
		return std::auto_ptr<Script>(0);
	wstring_ptr wstrLang(getLanguageByExtension(pExt + 1));
	if (!wstrLang.get())
		return std::auto_ptr<Script>(0);
	
	wstring_ptr wstrPath(concat(wstrPath_.get(), L"\\scripts\\", pwszFileName));
	
	FileInputStream stream(wstrPath.get());
	if (!stream)
		return std::auto_ptr<Script>(0);
	BufferedInputStream bufferedStream(&stream, false);
	InputStreamReader reader(&bufferedStream, false, 0);
	if (!reader)
		return std::auto_ptr<Script>(0);
	
	ScriptFactory::Init init = {
		wstrLang.get(),
		&reader,
		pDocument,
		pProfile,
		0,
		pModalHandler,
		ScriptFactory::TYPE_NONE,
		0,
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
		init.type_ = ScriptFactory::TYPE_MESSAGE;
		init.window_.pMessageFrameWindow_ = info.window_.pMessageFrameWindow_;
		break;
	default:
		assert(false);
		break;
	}
	
	return pFactory->createScript(init);
}

void qm::ScriptManager::getScriptNames(NameList* pList) const
{
	ScriptFactory* pFactory = ScriptFactory::getFactory();
	if (!pFactory)
		return;
	
	NameList l;
	CONTAINER_DELETER(free, l, &freeWString);
	
	wstring_ptr wstrFind(concat(wstrPath_.get(), L"\\scripts\\*.*"));
	
	W2T(wstrFind.get(), ptszFind);
	WIN32_FIND_DATA fd;
	AutoFindHandle hFind(::FindFirstFile(ptszFind, &fd));
	if (hFind.get()) {
		do {
			if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
				wstring_ptr wstrName(tcs2wcs(fd.cFileName));
				WCHAR* pExt = wcsrchr(wstrName.get(), L'.');
				if (pExt && getLanguageByExtension(pExt + 1).get()) {
					*pExt = L'\0';
					l.push_back(wstrName.get());
					wstrName.release();
				}
			}
		} while (::FindNextFile(hFind.get(), &fd));
	}
	
	pList->assign(l.begin(), l.end());
	free.release();
}

std::auto_ptr<Script> qm::ScriptManager::createScript(const WCHAR* pwszScript,
													  const WCHAR* pwszLanguage,
													  Document* pDocument,
													  const ActionInvoker* pActionInvoker,
													  Profile* pProfile,
													  HWND hwnd,
													  ModalHandler* pModalHandler) const
{
	assert(pwszScript);
	assert(pwszLanguage);
	assert(pDocument);
	assert(pProfile);
	assert(pModalHandler);
	
	ScriptFactory* pFactory = ScriptFactory::getFactory();
	if (!pFactory)
		return std::auto_ptr<Script>(0);
	
	StringReader reader(pwszScript, false);
	if (!reader)
		return std::auto_ptr<Script>(0);
	ScriptFactory::Init init = {
		pwszLanguage,
		&reader,
		pDocument,
		pProfile,
		hwnd,
		pModalHandler,
		ScriptFactory::TYPE_NONE,
		0,
		pActionInvoker
	};
	return pFactory->createScript(init);
}

wstring_ptr qm::ScriptManager::getLanguageByExtension(const WCHAR* pwszExtension)
{
	assert(pwszExtension);
	assert(*pwszExtension);
	
	wstring_ptr wstrLang = getLanguageByExtensionFromRegistry(pwszExtension);
	if (!wstrLang.get()) {
		struct {
			const WCHAR* pwszExt_;
			const WCHAR* pwszLang_;
		} langs[] = {
			{ L"js",	L"JScript"	},
			{ L"vbs",	L"VBScript"	}
		};
		
		const WCHAR* pwszLang = 0;
		for (int n = 0; n < countof(langs) && !pwszLang; ++n) {
			if (_wcsicmp(langs[n].pwszExt_, pwszExtension) == 0)
				pwszLang = langs[n].pwszLang_;
		}
		if (!pwszLang)
			return 0;
		
		wstrLang = allocWString(pwszLang);
	}
	return wstrLang;
}

wstring_ptr qm::ScriptManager::getLanguageByExtensionFromRegistry(const WCHAR* pwszExtension)
{
	assert(pwszExtension);
	assert(*pwszExtension);
	
#ifndef _WIN32_WCE
	wstring_ptr wstrExtKey(concat(L".", pwszExtension));
	Registry regExt(HKEY_CLASSES_ROOT, wstrExtKey.get(), true);
	if (!regExt)
		return 0;
	wstring_ptr wstrName;
	if (!regExt.getValue(0, &wstrName))
		return 0;
	
	wstring_ptr wstrEngineKey(concat(wstrName.get(), L"\\ScriptEngine"));
	Registry regEngine(HKEY_CLASSES_ROOT, wstrEngineKey.get(), true);
	if (!regEngine)
		return 0;
	wstring_ptr wstrLang;
	if (!regEngine.getValue(0, &wstrLang))
		return 0;
	
	return wstrLang;
#else
	return 0;
#endif
}
