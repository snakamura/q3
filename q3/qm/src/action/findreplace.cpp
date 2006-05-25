/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include "findreplace.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * FindReplaceData
 *
 */

qm::FindReplaceData::FindReplaceData(const WCHAR* pwszFind,
									 const WCHAR* pwszReplace,
									 unsigned int nFlags) :
	nFlags_(nFlags)
{
	assert(pwszFind);
	
	wstrFind_ = allocWString(pwszFind);
	
	if (pwszReplace)
		wstrReplace_ = allocWString(pwszReplace);
}

qm::FindReplaceData::~FindReplaceData()
{
}

const WCHAR* qm::FindReplaceData::getFind() const
{
	return wstrFind_.get();
}

const WCHAR* qm::FindReplaceData::getReplace() const
{
	return wstrReplace_.get();
}

unsigned int qm::FindReplaceData::getFlags() const
{
	return nFlags_;
}


/****************************************************************************
 *
 * FindReplaceManager
 *
 */

qm::FindReplaceManager::FindReplaceManager() :
	pData_(0)
{
}

qm::FindReplaceManager::~FindReplaceManager()
{
}

const FindReplaceData* qm::FindReplaceManager::getData() const
{
	return pData_.get();
}

void qm::FindReplaceManager::setData(const WCHAR* pwszFind,
									 unsigned int nFlags)
{
	setData(pwszFind, 0, nFlags);
}

void qm::FindReplaceManager::setData(const WCHAR* pwszFind,
									 const WCHAR* pwszReplace,
									 unsigned int nFlags)
{
	pData_.reset(new FindReplaceData(pwszFind, pwszReplace, nFlags));
}
