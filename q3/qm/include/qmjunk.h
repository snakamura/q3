/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QMJUNK_H__
#define __QMJUNK_H__

#include <qm.h>

#include <qs.h>

#include <memory>


namespace qm {

class JunkFilter;
class JunkFilterFactory;

class Message;


/****************************************************************************
 *
 * JunkFilter
 *
 */

class QMEXPORTCLASS JunkFilter
{
public:
	enum Operation {
		OPERATION_ADDCLEAN		= 0x01,
		OPERATION_REMOVECLEAN	= 0x02,
		OPERATION_ADDJUNK		= 0x10,
		OPERATION_REMOVEJUNK	= 0x20
	};
	
	enum Flag {
		FLAG_AUTOLEARN		= 0x01,
		FLAG_MANUALLEARN	= 0x02
	};

public:
	virtual ~JunkFilter();

public:
	virtual float getScore(const Message& msg) = 0;
	virtual bool manage(const Message& msg,
						unsigned int nOperation) = 0;
	virtual float getThresholdScore() = 0;
	virtual unsigned int getFlags() = 0;

public:
	static std::auto_ptr<JunkFilter> getInstance(const WCHAR* pwszPath);
};


/****************************************************************************
 *
 * JunkFilterFactory
 *
 */

class QMEXPORTCLASS JunkFilterFactory
{
protected:
	JunkFilterFactory();

public:
	virtual ~JunkFilterFactory();

public:
	virtual std::auto_ptr<JunkFilter> createJunkFilter(const WCHAR* pwszPath) = 0;

public:
	static JunkFilterFactory* getFactory();

protected:
	static void registerFactory(JunkFilterFactory* pFactory);
	static void unregisterFactory(JunkFilterFactory* pFactory);

private:
	JunkFilterFactory(const JunkFilterFactory&);
	JunkFilterFactory& operator=(const JunkFilterFactory&);
};

}

#endif // __QMJUNK_H__
