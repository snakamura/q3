/*
 * $Id: scriptmanager.h,v 1.1 2003/05/19 07:13:31 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
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
	ScriptManager(const WCHAR* pwszPath, qs::QSTATUS* pstatus);
	~ScriptManager();

public:
	qs::QSTATUS getScript(const WCHAR* pwszName, Document* pDocument,
		qs::Profile* pProfile, qs::ModalHandler* pModalHandler,
		const WindowInfo& info, qmscript::Script** ppScript) const;
	qs::QSTATUS getScriptNames(NameList* pList) const;
	qs::QSTATUS createScript(const WCHAR* pwszScript, const WCHAR* pwszLanguage,
		Document* pDocument, qs::Profile* pProfile, HWND hwnd,
		qs::ModalHandler* pModalHandler, qmscript::Script** ppScript) const;

private:
	ScriptManager(const ScriptManager&);
	ScriptManager& operator=(const ScriptManager&);

private:
	qs::WSTRING wstrPath_;
	HINSTANCE hInst_;
	qmscript::ScriptFactory* pFactory_;
};

}

#endif // __SCRIPTMANAGER_H__
