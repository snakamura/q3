/*
 * $Id: qmscript.h,v 1.3 2003/05/13 17:38:22 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QMSCRIPT_H__
#define __QMSCRIPT_H__

#include <qmscriptconfig.h>

#include <qmeditwindow.h>
#include <qmdocument.h>
#include <qmmainwindow.h>
#include <qmmessagewindow.h>

#include <qs.h>
#include <qsprofile.h>
#include <qsstream.h>
#include <qswindow.h>


namespace qmscript {

/****************************************************************************
 *
 * Script
 *
 */

class Script
{
public:
	virtual ~Script();

public:
	virtual qs::QSTATUS run(VARIANT* pvarArgs,
		size_t nArgCount, VARIANT* pvarResult) = 0;
};


/****************************************************************************
 *
 * ScriptFactory
 *
 */

class ScriptFactory
{
public:
	enum Type {
		TYPE_NONE,
		TYPE_MAIN,
		TYPE_EDIT,
		TYPE_MESSAGE
	};

public:
	struct Init
	{
		const WCHAR* pwszLanguage_;
		qs::Reader* pReader_;
		qm::Document* pDocument_;
		qs::Profile* pProfile_;
		HWND hwnd_;
		qs::ModalHandler* pModalHandler_;
		Type type_;
		union {
			qm::MainWindow* pMainWindow_;
			qm::EditFrameWindow* pEditFrameWindow_;
			qm::MessageFrameWindow* pMessageFrameWindow_;
		} window_;
	};

public:
	virtual ~ScriptFactory();

public:
	virtual qs::QSTATUS newScript(const Init& init, Script** ppScript) = 0;
	virtual void deleteScript(Script* pScript) = 0;
};

}


/****************************************************************************
 *
 * Global functions
 *
 */

extern "C" qmscript::ScriptFactory* newScriptFactory();
extern "C" void deleteScriptFactory(
	qmscript::ScriptFactory* pFactory);

#endif // __QMSCRIPT_H__
