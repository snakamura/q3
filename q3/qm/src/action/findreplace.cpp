/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsnew.h>

#include "findreplace.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * FindReplaceData
 *
 */

qm::FindReplaceData::FindReplaceData(const WCHAR* pwszFind,
	const WCHAR* pwszReplace, unsigned int nFlags, qs::QSTATUS* pstatus) :
	wstrFind_(0),
	wstrReplace_(0),
	nFlags_(nFlags)
{
	assert(pwszFind);
	
	string_ptr<WSTRING> wstrFind(allocWString(pwszFind));
	if (!wstrFind.get()) {
		*pstatus = QSTATUS_OUTOFMEMORY;
		return;
	}
	
	string_ptr<WSTRING> wstrReplace;
	if (pwszReplace) {
		wstrReplace.reset(allocWString(pwszReplace));
		if (!wstrReplace.get()) {
			*pstatus = QSTATUS_OUTOFMEMORY;
			return;
		}
	}
	
	wstrFind_ = wstrFind.release();
	wstrReplace_ = wstrReplace.release();
}

qm::FindReplaceData::~FindReplaceData()
{
	freeWString(wstrFind_);
	freeWString(wstrReplace_);
}

const WCHAR* qm::FindReplaceData::getFind() const
{
	return wstrFind_;
}

const WCHAR* qm::FindReplaceData::getReplace() const
{
	return wstrReplace_;
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

qm::FindReplaceManager::FindReplaceManager(QSTATUS* pstatus) :
	pData_(0)
{
}

qm::FindReplaceManager::~FindReplaceManager()
{
	delete pData_;
}

const FindReplaceData* qm::FindReplaceManager::getData() const
{
	return pData_;
}

QSTATUS qm::FindReplaceManager::setData(
	const WCHAR* pwszFind, unsigned int nFlags)
{
	return setData(pwszFind, 0, nFlags);
}

QSTATUS qm::FindReplaceManager::setData(const WCHAR* pwszFind,
	const WCHAR* pwszReplace, unsigned int nFlags)
{
	DECLARE_QSTATUS();
	
	std::auto_ptr<FindReplaceData> pData;
	status = newQsObject(pwszFind, pwszReplace, nFlags, &pData);
	CHECK_QSTATUS();
	
	delete pData_;
	pData_ = pData.release();
	
	return QSTATUS_SUCCESS;
}
