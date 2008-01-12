/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#include <qs.h>


/****************************************************************************
 *
 * memory management
 *
 */

#if defined _WIN32_WCE || !defined NDEBUG
#	define USEVIRTUALALLOC
#	define MALLOC_LIMIT (2*1024*1024)
#endif

QSEXPORTPROC void* qs::allocate(size_t nSize)
{
#ifndef USEVIRTUALALLOC
	return malloc(nSize);
#else
	size_t* p = 0;
	if (nSize < MALLOC_LIMIT) {
		p = static_cast<size_t*>(malloc(nSize + sizeof(size_t)));
		if (!p)
			return 0;
	}
	else {
		void* pVirt = ::VirtualAlloc(0, nSize + sizeof(size_t), MEM_RESERVE, PAGE_NOACCESS);
		if (!pVirt)
			return 0;
		p = static_cast<size_t*>(::VirtualAlloc(pVirt,
			nSize + sizeof(size_t), MEM_COMMIT, PAGE_READWRITE));
		if (!p) {
			::VirtualFree(pVirt, 0, MEM_RELEASE);
			return 0;
		}
	}
	*p = nSize;
	return p + 1;
#endif
}

QSEXPORTPROC void qs::deallocate(void* p)
{
#ifndef USEVIRTUALALLOC
	free(p);
#else
	if (p == 0)
		return;
	
	size_t* pBase = static_cast<size_t*>(p) - 1;
	if (*pBase < MALLOC_LIMIT)
		free(pBase);
	else
		::VirtualFree(pBase, 0, MEM_RELEASE);
#endif
}

QSEXPORTPROC void* qs::reallocate(void* p,
								  size_t nSize)
{
#ifndef USEVIRTUALALLOC
	return realloc(p, nSize);
#else
	if (!p)
		return allocate(nSize);
	
	if (nSize == 0) {
		deallocate(p);
		return 0;
	}
	
	size_t* pBase = static_cast<size_t*>(p) - 1;
	if (*pBase < MALLOC_LIMIT && nSize < MALLOC_LIMIT) {
		size_t* pNew = static_cast<size_t*>(realloc(pBase, nSize + sizeof(size_t)));
		if (!pNew)
			return 0;
		*pNew = nSize;
		return pNew + 1;
	}
	else {
		void* pNew = allocate(nSize);
		if (!pNew)
			return 0;
		memcpy(pNew, p, QSMIN(*pBase, nSize));
		deallocate(p);
		return pNew;
	}
#endif
}
