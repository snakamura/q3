/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QSINIT_H__
#define __QSINIT_H__

#include <qs.h>
#include <qslog.h>

namespace qs {

class Init;
class InitThread;

class Synchronizer;


/****************************************************************************
 *
 * Init
 *
 */

class QSEXPORTCLASS Init
{
public:
	Init(HINSTANCE hInst, const WCHAR* pwszTitle,
		unsigned int nFlags, unsigned int nThreadFlags, QSTATUS* pstatus);
	~Init();

public:
	HINSTANCE getInstanceHandle() const;
	const WCHAR* getTitle() const;
	const WCHAR* getSystemEncoding() const;
	const WCHAR* getDefaultFixedWidthFont() const;
	const WCHAR* getDefaultProportionalFont() const;
	bool isLogEnabled() const;
	const WCHAR* getLogDirectory() const;
	Logger::Level getLogLevel() const;
	QSTATUS setLogInfo(bool bEnabled,
		const WCHAR* pwszDir, Logger::Level level);

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
	InitThread(unsigned int nFlags, QSTATUS* pstatus);
	~InitThread();

public:
	Synchronizer* getSynchronizer() const;
	Logger* getLogger() const;

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
public:
	Initializer();
	virtual ~Initializer();

public:
	Initializer* getNext() const;

public:
	virtual QSTATUS init() = 0;
	virtual QSTATUS term() = 0;
	virtual QSTATUS initThread();
	virtual QSTATUS termThread();

private:
	Initializer(const Initializer&);
	Initializer& operator=(const Initializer&);

private:
	Initializer* pNext_;
};

}

#endif // __QS_H__
