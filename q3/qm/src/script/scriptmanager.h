/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __SCRIPTMANAGER_H__
#define __SCRIPTMANAGER_H__

#include <qm.h>
#include <qmscript.h>

#include <qs.h>
#include <qsstring.h>

#include <vector>


namespace qm {

class ScriptManager;


/****************************************************************************
 *
 * ScriptManager
 *
 */

class ScriptManager
{
public:
	enum Type {
		TYPE_MAIN,
		TYPE_EDIT,
		TYPE_MESSAGE
	};

public:
	struct WindowInfo
	{
		Type type_;
		union {
			MainWindow* pMainWindow_;
			EditFrameWindow* pEditFrameWindow_;
			MessageFrameWindow* pMessageFrameWindow_;
		} window_;
	};

public:
	typedef std::vector<qs::WSTRING> NameList;

public:
	explicit ScriptManager(const WCHAR* pwszPath);
	~ScriptManager();

public:
	std::auto_ptr<Script> getScript(const WCHAR* pwszName,
									Document* pDocument,
									qs::Profile* pProfile,
									qs::ModalHandler* pModalHandler,
									const WindowInfo& info) const;
	void getScriptNames(NameList* pList) const;
	std::auto_ptr<Script> createScript(const WCHAR* pwszScript,
									   const WCHAR* pwszLanguage,
									   Document* pDocument,
									   const ActionInvoker* pActionInvoker,
									   qs::Profile* pProfile,
									   HWND hwnd,
									   qs::ModalHandler* pModalHandler) const;

private:
	static qs::wstring_ptr getLanguageByExtension(const WCHAR* pwszExtension);
	static qs::wstring_ptr getLanguageByExtensionFromRegistry(const WCHAR* pwszExtension);

private:
	ScriptManager(const ScriptManager&);
	ScriptManager& operator=(const ScriptManager&);

private:
	qs::wstring_ptr wstrPath_;
};

}

#endif // __SCRIPTMANAGER_H__
