/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QMSCRIPT_H__
#define __QMSCRIPT_H__

#include <qmeditwindow.h>
#include <qmdocument.h>
#include <qmmainwindow.h>
#include <qmmessagewindow.h>

#include <qs.h>
#include <qsprofile.h>
#include <qsstream.h>
#include <qswindow.h>


namespace qm {

/****************************************************************************
 *
 * Script
 *
 */

class QMEXPORTCLASS Script
{
public:
	virtual ~Script();

public:
	virtual bool run(VARIANT* pvarArgs,
					 size_t nArgCount,
					 VARIANT* pvarResult) = 0;
};


/****************************************************************************
 *
 * ScriptFactory
 *
 */

class QMEXPORTCLASS ScriptFactory
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

protected:
	ScriptFactory();

public:
	virtual ~ScriptFactory();

public:
	virtual std::auto_ptr<Script> createScript(const Init& init) = 0;

public:
	static ScriptFactory* getFactory();

protected:
	static void registerFactory(ScriptFactory* pFactory);
	static void unregisterFactory(ScriptFactory* pFactory);

private:
	ScriptFactory(const ScriptFactory&);
	ScriptFactory& operator=(const ScriptFactory&);
};

}

#endif // __QMSCRIPT_H__
