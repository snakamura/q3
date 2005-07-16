/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QMJUNK_H__
#define __QMJUNK_H__

#include <qm.h>

#include <qs.h>
#include <qsprofile.h>

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
	
	enum Status {
		STATUS_NONE			= 0,
		STATUS_CLEAN		= 1,
		STATUS_JUNK			= -1
	};

public:
	virtual ~JunkFilter();

public:
	virtual float getScore(const Message& msg) = 0;
	virtual bool manage(const Message& msg,
						unsigned int nOperation) = 0;
	virtual Status getStatus(const WCHAR* pwszId) = 0;
	virtual float getThresholdScore() = 0;
	virtual void setThresholdScore(float fThresholdScore) = 0;
	virtual unsigned int getFlags() = 0;
	virtual void setFlags(unsigned int nFlags,
						  unsigned int nMask) = 0;
	virtual unsigned int getMaxTextLength() = 0;
	virtual void setMaxTextLength(unsigned int nMaxTextLength) = 0;
	virtual qs::wstring_ptr getWhiteList(const WCHAR* pwszSeparator) = 0;
	virtual void setWhiteList(const WCHAR* pwszWhiteList) = 0;
	virtual qs::wstring_ptr getBlackList(const WCHAR* pwszSeparator) = 0;
	virtual void setBlackList(const WCHAR* pwszBlackList) = 0;
	virtual bool save() = 0;

public:
	static std::auto_ptr<JunkFilter> getInstance(const WCHAR* pwszPath,
												 qs::Profile* pProfile);
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
	virtual std::auto_ptr<JunkFilter> createJunkFilter(const WCHAR* pwszPath,
													   qs::Profile* pProfile) = 0;

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
