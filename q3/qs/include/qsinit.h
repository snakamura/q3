/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QSINIT_H__
#define __QSINIT_H__

#include <qs.h>
#include <qslog.h>
#include <qsstring.h>

namespace qs {

class Init;
class InitThread;

class ModalHandler;
class Synchronizer;


/****************************************************************************
 *
 * Init
 *
 */

class QSEXPORTCLASS Init
{
public:
	Init(HINSTANCE hInst,
		 const WCHAR* pwszTitle,
		 unsigned int nFlags,
		 unsigned int nThreadFlags);
	~Init();

public:
	HINSTANCE getInstanceHandle() const;
	const WCHAR* getTitle() const;
	const WCHAR* getSystemEncoding() const;
	const WCHAR* getMailEncoding() const;
	const WCHAR* getDefaultFixedWidthFont() const;
	const WCHAR* getDefaultProportionalFont() const;
	const WCHAR* getDefaultUIFont() const;
	
	bool isLogEnabled() const;
	void setLogEnabled(bool bEnabled);
	wstring_ptr getLogDirectory() const;
	void setLogDirectory(const WCHAR* pwszDir);
	Logger::Level getLogLevel() const;
	void setLogLevel(Logger::Level level);
	wstring_ptr getLogFilter() const;
	void setLogFilter(const WCHAR* pwszFilter);
	wstring_ptr getLogTimeFormat() const;
	void setLogTimeFormat(const WCHAR* pwszTimeFormat);

public:
	InitThread* getInitThread();
	void setInitThread(InitThread* pInitThread);

public:
	static Init& getInit();

private:
	Init(const Init&);
	Init& operator=(const Init&);

private:
	struct InitImpl* pImpl_;
};


/****************************************************************************
 *
 * InitThread
 *
 */

class QSEXPORTCLASS InitThread
{
public:
	enum Flag {
		FLAG_SYNCHRONIZER	= 0x01
	};

public:
	InitThread(unsigned int nFlags);
	~InitThread();

public:
	Synchronizer* getSynchronizer() const;
	ModalHandler* getModalHandler() const;
	void addModalHandler(ModalHandler* pModalHandler);
	void removeModalHandler(ModalHandler* pModalHandler);
	Logger* getLogger() const;
	void resetLogger();

public:
	static InitThread& getInitThread();

private:
	InitThread(const InitThread&);
	InitThread& operator=(const InitThread&);

private:
	struct InitThreadImpl* pImpl_;
};


/****************************************************************************
 *
 * Initializer
 *
 */

class QSEXPORTCLASS Initializer
{
protected:
	Initializer();

public:
	virtual ~Initializer();

public:
	Initializer* getNext() const;

public:
	virtual bool init() = 0;
	virtual void term() = 0;
	virtual bool initThread();
	virtual void termThread();

private:
	Initializer(const Initializer&);
	Initializer& operator=(const Initializer&);

private:
	Initializer* pNext_;
};

}

#endif // __QS_H__
