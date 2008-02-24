/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
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
class ProcessHookHolder;
class Hook;

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
	enum Result {
		RESULT_PROCESSED,
		RESULT_UNPROCESSED,
		RESULT_ERROR
	};
	
public:
	virtual ~ProcessHook();

public:
	virtual Result processFetchResponse(ResponseFetch* pFetch) = 0;
	virtual Result processListResponse(ResponseList* pList) = 0;
	virtual Result processSearchResponse(ResponseSearch* pSearch) = 0;
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
	virtual Result processFetchResponse(ResponseFetch* pFetch);
	virtual Result processListResponse(ResponseList* pList);
	virtual Result processSearchResponse(ResponseSearch* pSearch);
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
	virtual Result processFetchResponse(ResponseFetch* pFetch);

protected:
	virtual qm::Account* getAccount() = 0;
	virtual bool isHeader() = 0;
	virtual qm::MessagePtr getMessagePtr(unsigned int nUid) = 0;
	virtual void processed() = 0;

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
	virtual Result processFetchResponse(ResponseFetch* pFetch);

protected:
	virtual qm::Account* getAccount() = 0;
	virtual bool isAll() = 0;
	virtual const PartList& getPartList() = 0;
	virtual unsigned int getPartCount() = 0;
	virtual qm::MessagePtr getMessagePtr(unsigned int nUid) = 0;
	virtual unsigned int getOption() = 0;
	virtual void processed() = 0;

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
	virtual Result processFetchResponse(ResponseFetch* pFetch);

protected:
	virtual bool setBodyStructure(unsigned int nUid,
								  FetchDataBodyStructure* pBodyStructure,
								  bool* pbSet) = 0;
	virtual void processed() = 0;

private:
	AbstractBodyStructureProcessHook(const AbstractBodyStructureProcessHook&);
	AbstractBodyStructureProcessHook& operator=(const AbstractBodyStructureProcessHook&);
};


/****************************************************************************
 *
 * ProcessHookHolder
 *
 */

class ProcessHookHolder
{
public:
	virtual ~ProcessHookHolder();

public:
	virtual void setProcessHook(ProcessHook* pProcessHook) = 0;
};


/****************************************************************************
 *
 * Hook
 *
 */

class Hook
{
public:
	Hook(ProcessHookHolder* pProcessHookHolder,
		 ProcessHook* pProcessHook);
	~Hook();

private:
	Hook(const Hook&);
	Hook& operator=(const Hook&);

private:
	ProcessHookHolder* pProcessHookHolder_;
};

}

#endif // __PROCESSHOOK_H__
