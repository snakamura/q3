/*
 * $Id: qsinit.h,v 1.1.1.1 2003/04/29 08:07:34 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QSINIT_H__
#define __QSINIT_H__

#include <qs.h>

namespace qs {

/****************************************************************************
 *
 * Init
 *
 */

class QSEXPORTCLASS Init
{
public:
	Init(HINSTANCE hInst, const WCHAR* pwszTitle, QSTATUS* pstatus);
	~Init();

public:
	HINSTANCE getInstanceHandle() const;
	const WCHAR* getTitle() const;
	const WCHAR* getSystemEncoding() const;
	const WCHAR* getDefaultFixedWidthFont() const;
	const WCHAR* getDefaultProportionalFont() const;

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
	explicit InitThread(QSTATUS* pstatus);
	~InitThread();

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

class Initializer
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
