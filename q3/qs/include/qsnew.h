/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QSNEW_H__
#define __QSNEW_H__

#include <qs.h>
#include <qsstl.h>
#include <qserror.h>

namespace qs {

/****************************************************************************
 *
 * newObject
 *
 */


#ifdef _CPPUNWIND

#define TRY_NEW() \
	try { \

#define CATCH_NEW() \
	} \
	catch (std::bad_alloc) { \
		return QSTATUS_OUTOFMEMORY; \
	} \

#else

#define TRY_NEW()

#define CATCH_NEW()

#endif

template<class Object>
QSTATUS newObject(Object** ppObject)
{
	assert(ppObject);
	
	TRY_NEW()
	*ppObject = new Object();
	CATCH_NEW()
	return *ppObject ? QSTATUS_SUCCESS : QSTATUS_OUTOFMEMORY;
}

template<class Object>
QSTATUS newObject(std::auto_ptr<Object>* papObject)
{
	assert(papObject);
	
	DECLARE_QSTATUS();
	
	Object* pObject = 0;
	status = newObject(&pObject);
	papObject->reset(pObject);
	
	return status;
}

template<class Object, class Arg1>
QSTATUS newObject(const Arg1& arg1, Object** ppObject)
{
	assert(ppObject);
	
	TRY_NEW()
	*ppObject = new Object(arg1);
	CATCH_NEW()
	return *ppObject ? QSTATUS_SUCCESS : QSTATUS_OUTOFMEMORY;
}

template<class Object, class Arg1>
QSTATUS newObject(const Arg1& arg1, std::auto_ptr<Object>* papObject)
{
	assert(papObject);
	
	DECLARE_QSTATUS();
	
	Object* pObject = 0;
	status = newObject(arg1, &pObject);
	papObject->reset(pObject);
	
	return status;
}

template<class Object, class Arg1, class Arg2>
QSTATUS newObject(const Arg1& arg1, const Arg2& arg2, Object** ppObject)
{
	assert(ppObject);
	
	TRY_NEW()
	*ppObject = new Object(arg1, arg2);
	CATCH_NEW()
	return *ppObject ? QSTATUS_SUCCESS : QSTATUS_OUTOFMEMORY;
}

template<class Object, class Arg1, class Arg2>
QSTATUS newObject(const Arg1& arg1, const Arg2& arg2, std::auto_ptr<Object>* papObject)
{
	assert(papObject);
	
	DECLARE_QSTATUS();
	
	Object* pObject = 0;
	status = newObject(arg1, arg2, &pObject);
	papObject->reset(pObject);
	
	return status;
}

template<class Object>
QSTATUS newQsObject(Object** ppObject)
{
	assert(ppObject);
	
	DECLARE_QSTATUS();
	
	TRY_NEW()
	*ppObject = new Object(&status);
	CATCH_NEW()
	if (!*ppObject) {
		status = QSTATUS_OUTOFMEMORY;
	}
	else if (status != QSTATUS_SUCCESS) {
		delete *ppObject;
		*ppObject = 0;
	}
	return status;
}

template<class Object>
QSTATUS newQsObject(std::auto_ptr<Object>* papObject)
{
	assert(papObject);
	
	DECLARE_QSTATUS();
	
	Object* pObject = 0;
	status = newQsObject(&pObject);
	papObject->reset(pObject);
	
	return status;
}

template<class Object, class Arg1>
QSTATUS newQsObject(const Arg1& arg1, Object** ppObject)
{
	assert(ppObject);
	
	DECLARE_QSTATUS();
	
	TRY_NEW()
	*ppObject = new Object(arg1, &status);
	CATCH_NEW()
	if (!*ppObject) {
		status = QSTATUS_OUTOFMEMORY;
	}
	else if (status != QSTATUS_SUCCESS) {
		delete *ppObject;
		*ppObject = 0;
	}
	return status;
}

template<class Object, class Arg1>
QSTATUS newQsObject(const Arg1& arg1, std::auto_ptr<Object>* papObject)
{
	assert(papObject);
	
	DECLARE_QSTATUS();
	
	Object* pObject = 0;
	status = newQsObject(arg1, &pObject);
	papObject->reset(pObject);
	
	return status;
}

template<class Object, class Arg1, class Arg2>
QSTATUS newQsObject(const Arg1& arg1,
	const Arg2& arg2, Object** ppObject)
{
	assert(ppObject);
	
	DECLARE_QSTATUS();
	
	TRY_NEW()
	*ppObject = new Object(arg1, arg2, &status);
	CATCH_NEW()
	if (!*ppObject) {
		status = QSTATUS_OUTOFMEMORY;
	}
	else if (status != QSTATUS_SUCCESS) {
		delete *ppObject;
		*ppObject = 0;
	}
	return status;
}

template<class Object, class Arg1, class Arg2>
QSTATUS newQsObject(const Arg1& arg1, const Arg2& arg2,
	std::auto_ptr<Object>* papObject)
{
	assert(papObject);
	
	DECLARE_QSTATUS();
	
	Object* pObject = 0;
	status = newQsObject(arg1, arg2, &pObject);
	papObject->reset(pObject);
	
	return status;
}

template<class Object, class Arg1, class Arg2, class Arg3>
QSTATUS newQsObject(const Arg1& arg1, const Arg2& arg2,
	const Arg3& arg3, Object** ppObject)
{
	assert(ppObject);
	
	DECLARE_QSTATUS();
	
	TRY_NEW()
	*ppObject = new Object(arg1, arg2, arg3, &status);
	CATCH_NEW()
	if (!*ppObject) {
		status = QSTATUS_OUTOFMEMORY;
	}
	else if (status != QSTATUS_SUCCESS) {
		delete *ppObject;
		*ppObject = 0;
	}
	return status;
}

template<class Object, class Arg1, class Arg2, class Arg3>
QSTATUS newQsObject(const Arg1& arg1, const Arg2& arg2,
	const Arg3& arg3, std::auto_ptr<Object>* papObject)
{
	assert(papObject);
	
	DECLARE_QSTATUS();
	
	Object* pObject = 0;
	status = newQsObject(arg1, arg2, arg3, &pObject);
	papObject->reset(pObject);
	
	return status;
}

template<class Object, class Arg1, class Arg2, class Arg3, class Arg4>
QSTATUS newQsObject(const Arg1& arg1, const Arg2& arg2,
	const Arg3& arg3, const Arg4& arg4, Object** ppObject)
{
	assert(ppObject);
	
	DECLARE_QSTATUS();
	
	TRY_NEW()
	*ppObject = new Object(arg1, arg2, arg3, arg4, &status);
	CATCH_NEW()
	if (!*ppObject) {
		status = QSTATUS_OUTOFMEMORY;
	}
	else if (status != QSTATUS_SUCCESS) {
		delete *ppObject;
		*ppObject = 0;
	}
	return status;
}

template<class Object, class Arg1, class Arg2, class Arg3, class Arg4>
QSTATUS newQsObject(const Arg1& arg1, const Arg2& arg2,
	const Arg3& arg3, const Arg4& arg4, std::auto_ptr<Object>* papObject)
{
	assert(papObject);
	
	DECLARE_QSTATUS();
	
	Object* pObject = 0;
	status = newQsObject(arg1, arg2, arg3, arg4, &pObject);
	papObject->reset(pObject);
	
	return status;
}

template<class Object, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5>
QSTATUS newQsObject(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3,
	const Arg4& arg4, const Arg5& arg5, Object** ppObject)
{
	assert(ppObject);
	
	DECLARE_QSTATUS();
	
	TRY_NEW()
	*ppObject = new Object(arg1, arg2, arg3, arg4, arg5, &status);
	CATCH_NEW()
	if (!*ppObject) {
		status = QSTATUS_OUTOFMEMORY;
	}
	else if (status != QSTATUS_SUCCESS) {
		delete *ppObject;
		*ppObject = 0;
	}
	return status;
}

template<class Object, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5>
QSTATUS newQsObject(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3,
	const Arg4& arg4, const Arg5& arg5, std::auto_ptr<Object>* papObject)
{
	assert(papObject);
	
	DECLARE_QSTATUS();
	
	Object* pObject = 0;
	status = newQsObject(arg1, arg2, arg3, arg4, arg5, &pObject);
	papObject->reset(pObject);
	
	return status;
}

template<class Object, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5, class Arg6>
QSTATUS newQsObject(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3,
	const Arg4& arg4, const Arg5& arg5, const Arg6& arg6, Object** ppObject)
{
	assert(ppObject);
	
	DECLARE_QSTATUS();
	
	TRY_NEW()
	*ppObject = new Object(arg1, arg2, arg3, arg4, arg5, arg6, &status);
	CATCH_NEW()
	if (!*ppObject) {
		status = QSTATUS_OUTOFMEMORY;
	}
	else if (status != QSTATUS_SUCCESS) {
		delete *ppObject;
		*ppObject = 0;
	}
	return status;
}

template<class Object, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5, class Arg6>
QSTATUS newQsObject(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3,
	const Arg4& arg4, const Arg5& arg5, const Arg6& arg6, std::auto_ptr<Object>* papObject)
{
	assert(papObject);
	
	DECLARE_QSTATUS();
	
	Object* pObject = 0;
	status = newQsObject(arg1, arg2, arg3, arg4, arg5, arg6, &pObject);
	papObject->reset(pObject);
	
	return status;
}

template<class Object, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5, class Arg6, class Arg7>
QSTATUS newQsObject(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3,
	const Arg4& arg4, const Arg5& arg5, const Arg6& arg6, const Arg7& arg7, Object** ppObject)
{
	assert(ppObject);
	
	DECLARE_QSTATUS();
	
	TRY_NEW()
	*ppObject = new Object(arg1, arg2, arg3, arg4, arg5, arg6, arg7, &status);
	CATCH_NEW()
	if (!*ppObject) {
		status = QSTATUS_OUTOFMEMORY;
	}
	else if (status != QSTATUS_SUCCESS) {
		delete *ppObject;
		*ppObject = 0;
	}
	return status;
}

template<class Object, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5, class Arg6, class Arg7>
QSTATUS newQsObject(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3,
	const Arg4& arg4, const Arg5& arg5, const Arg6& arg6, const Arg7& arg7,
	std::auto_ptr<Object>* papObject)
{
	assert(papObject);
	
	DECLARE_QSTATUS();
	
	Object* pObject = 0;
	status = newQsObject(arg1, arg2, arg3, arg4, arg5, arg6, arg7, &pObject);
	papObject->reset(pObject);
	
	return status;
}

template<class Object, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5, class Arg6, class Arg7, class Arg8>
QSTATUS newQsObject(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3,
	const Arg4& arg4, const Arg5& arg5, const Arg6& arg6, const Arg7& arg7,
	const Arg8& arg8, Object** ppObject)
{
	assert(ppObject);
	
	DECLARE_QSTATUS();
	
	TRY_NEW()
	*ppObject = new Object(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, &status);
	CATCH_NEW()
	if (!*ppObject) {
		status = QSTATUS_OUTOFMEMORY;
	}
	else if (status != QSTATUS_SUCCESS) {
		delete *ppObject;
		*ppObject = 0;
	}
	return status;
}

template<class Object, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5, class Arg6, class Arg7, class Arg8>
QSTATUS newQsObject(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3,
	const Arg4& arg4, const Arg5& arg5, const Arg6& arg6, const Arg7& arg7,
	const Arg8& arg8, std::auto_ptr<Object>* papObject)
{
	assert(papObject);
	
	DECLARE_QSTATUS();
	
	Object* pObject = 0;
	status = newQsObject(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, &pObject);
	papObject->reset(pObject);
	
	return status;
}

template<class Object, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5, class Arg6, class Arg7, class Arg8, class Arg9>
QSTATUS newQsObject(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3,
	const Arg4& arg4, const Arg5& arg5, const Arg6& arg6, const Arg7& arg7,
	const Arg8& arg8, const Arg9& arg9, Object** ppObject)
{
	assert(ppObject);
	
	DECLARE_QSTATUS();
	
	TRY_NEW()
	*ppObject = new Object(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, &status);
	CATCH_NEW()
	if (!*ppObject) {
		status = QSTATUS_OUTOFMEMORY;
	}
	else if (status != QSTATUS_SUCCESS) {
		delete *ppObject;
		*ppObject = 0;
	}
	return status;
}

template<class Object, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5, class Arg6, class Arg7, class Arg8, class Arg9>
QSTATUS newQsObject(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3,
	const Arg4& arg4, const Arg5& arg5, const Arg6& arg6, const Arg7& arg7,
	const Arg8& arg8, const Arg9& arg9, std::auto_ptr<Object>* papObject)
{
	assert(papObject);
	
	DECLARE_QSTATUS();
	
	Object* pObject = 0;
	status = newQsObject(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, &pObject);
	papObject->reset(pObject);
	
	return status;
}

template<class Object>
QSTATUS newArray(size_t nSize, Object** ppObject)
{
	assert(ppObject);
	
	TRY_NEW()
	*ppObject = new Object[nSize];
	CATCH_NEW()
	
	return *ppObject ? QSTATUS_SUCCESS : QSTATUS_OUTOFMEMORY;
}

template<class Object>
QSTATUS newArray(size_t nSize, auto_ptr_array<Object>* papObject)
{
	assert(papObject);
	
	DECLARE_QSTATUS();
	
	Object* pObject = 0;
	status = newArray(nSize, &pObject);
	papObject->reset(pObject);
	
	return status;
}

}

#endif // __QSNEW_H__
