/*
 * $Id: processhook.h,v 1.1.1.1 2003/04/29 08:07:34 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __PROCESSHOOK_H__
#define __PROCESSHOOK_H__

#include <qmaccount.h>
#include <qmmessageholder.h>

#include <qs.h>

#include "util.h"


namespace qmimap4 {

class ProcessHook;
	class DefaultProcessHook;
		class AbstractMessageProcessHook;
		class AbstractPartialMessageProcessHook;
		class AbstractBodyStructureProcessHook;

class FetchDataBodyStructure;
class ResponseFetch;


/****************************************************************************
 *
 * ProcessHook
 *
 */

class ProcessHook
{
public:
	virtual ~ProcessHook();

public:
	virtual qs::QSTATUS processFetchResponse(
		ResponseFetch* pFetch, bool* pbProcessed) = 0;
};


/****************************************************************************
 *
 * DefaultProcessHook
 *
 */

class DefaultProcessHook : public ProcessHook
{
public:
	DefaultProcessHook();
	virtual ~DefaultProcessHook();

public:
	virtual qs::QSTATUS processFetchResponse(
		ResponseFetch* pFetch, bool* pbProcessed);
};


/****************************************************************************
 *
 * AbstractMessageProcessHook
 *
 */

class AbstractMessageProcessHook : public DefaultProcessHook
{
public:
	AbstractMessageProcessHook();
	virtual ~AbstractMessageProcessHook();

public:
	virtual qs::QSTATUS processFetchResponse(
		ResponseFetch* pFetch, bool* pbProcessed);

protected:
	virtual qm::Account* getAccount() = 0;
	virtual bool isHeader() = 0;
	virtual bool isMakeUnseen() = 0;
	virtual qm::MessagePtr getMessagePtr(unsigned long nUid) = 0;
	virtual qs::QSTATUS processed() = 0;

private:
	AbstractMessageProcessHook(const AbstractMessageProcessHook&);
	AbstractMessageProcessHook& operator=(const AbstractMessageProcessHook&);
};


/****************************************************************************
 *
 * AbstractPartialMessageProcessHook
 *
 */

class AbstractPartialMessageProcessHook : public DefaultProcessHook
{
public:
	typedef Util::BodyList BodyList;
	typedef Util::PartList PartList;

public:
	AbstractPartialMessageProcessHook();
	virtual ~AbstractPartialMessageProcessHook();

public:
	virtual qs::QSTATUS processFetchResponse(
		ResponseFetch* pFetch, bool* pbProcessed);

protected:
	virtual qm::Account* getAccount() = 0;
	virtual bool isAll() = 0;
	virtual const PartList& getPartList() = 0;
	virtual unsigned int getPartCount() = 0;
	virtual bool isMakeUnseen() = 0;
	virtual qm::MessagePtr getMessagePtr(unsigned long nUid) = 0;
	virtual qs::QSTATUS processed() = 0;

private:
	AbstractPartialMessageProcessHook(const AbstractPartialMessageProcessHook&);
	AbstractPartialMessageProcessHook& operator=(const AbstractPartialMessageProcessHook&);
};


/****************************************************************************
 *
 * AbstractBodyStructureProcessHook
 *
 */

class AbstractBodyStructureProcessHook : public DefaultProcessHook
{
public:
	AbstractBodyStructureProcessHook();
	virtual ~AbstractBodyStructureProcessHook();

public:
	virtual qs::QSTATUS processFetchResponse(
		ResponseFetch* pFetch, bool* pbProcessed);

protected:
	virtual qs::QSTATUS setBodyStructure(unsigned long nUid,
		FetchDataBodyStructure* pBodyStructure, bool* pbSet) = 0;
	virtual qs::QSTATUS processed() = 0;

private:
	AbstractBodyStructureProcessHook(const AbstractBodyStructureProcessHook&);
	AbstractBodyStructureProcessHook& operator=(const AbstractBodyStructureProcessHook&);
};

}

#endif // __PROCESSHOOK_H__
