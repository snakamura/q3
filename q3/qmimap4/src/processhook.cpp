/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsassert.h>
#include <qserror.h>
#include <qsstl.h>

#include "imap4.h"
#include "processhook.h"

#pragma warning(disable:4786)

using namespace qmimap4;
using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * ProcessHook
 *
 */

qmimap4::ProcessHook::~ProcessHook()
{
}


/****************************************************************************
 *
 * DefaultProcessHook
 *
 */

qmimap4::DefaultProcessHook::DefaultProcessHook()
{
}

qmimap4::DefaultProcessHook::~DefaultProcessHook()
{
}

QSTATUS qmimap4::DefaultProcessHook::processFetchResponse(
	ResponseFetch* pFetch, bool* pbProcessed)
{
	assert(pFetch);
	assert(pbProcessed);
	*pbProcessed = false;
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * AbstractMessageProcessHook
 *
 */

qmimap4::AbstractMessageProcessHook::AbstractMessageProcessHook()
{
}

qmimap4::AbstractMessageProcessHook::~AbstractMessageProcessHook()
{
}

QSTATUS qmimap4::AbstractMessageProcessHook::processFetchResponse(
	ResponseFetch* pFetch, bool* pbProcessed)
{
	DECLARE_QSTATUS();
	
	bool bHeader = isHeader();
	
	unsigned long nUid = 0;
	FetchDataBody* pBody = 0;
	
	int nCount = 0;
	
	const ResponseFetch::FetchDataList& l = pFetch->getFetchDataList();
	ResponseFetch::FetchDataList::const_iterator it = l.begin();
	while (it != l.end()) {
		switch ((*it)->getType()) {
		case FetchData::TYPE_BODY:
			{
				FetchDataBody* p = static_cast<FetchDataBody*>(*it);
				if (((bHeader && p->getSection() == FetchDataBody::SECTION_HEADER) ||
					(!bHeader && p->getSection() == FetchDataBody::SECTION_NONE)) &&
					p->getPartPath().empty()) {
					pBody = p;
					++nCount;
				}
			}
			break;
		case FetchData::TYPE_UID:
			nUid = static_cast<FetchDataUid*>(*it)->getUid();
			++nCount;
			break;
		default:
			break;
		}
		++it;
	}
	
	if (nCount == 2) {
		MessagePtr ptr(getMessagePtr(nUid));
		MessagePtrLock mpl(ptr);
		if (mpl) {
			status = getAccount()->updateMessage(mpl, pBody->getContent());
			CHECK_QSTATUS();
			unsigned int nMask = MessageHolder::FLAG_DOWNLOAD |
				MessageHolder::FLAG_DOWNLOADTEXT |
				MessageHolder::FLAG_PARTIAL_MASK;
			// TODO
			// Should change the flag on server?
			if (isMakeUnseen())
				nMask |= MessageHolder::FLAG_SEEN;
			mpl->setFlags(bHeader ? MessageHolder::FLAG_HEADERONLY : 0, nMask);
		}
		
		status = processed();
		CHECK_QSTATUS();
		
		*pbProcessed = true;
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * AbstractPartialMessageProcessHook
 *
 */

qmimap4::AbstractPartialMessageProcessHook::AbstractPartialMessageProcessHook()
{
}

qmimap4::AbstractPartialMessageProcessHook::~AbstractPartialMessageProcessHook()
{
}

QSTATUS qmimap4::AbstractPartialMessageProcessHook::processFetchResponse(
	ResponseFetch* pFetch, bool* pbProcessed)
{
	DECLARE_QSTATUS();
	
	const PartList& listPart = getPartList();
	
	unsigned long nUid = 0;
	BodyList listBody;
	
	const ResponseFetch::FetchDataList& l = pFetch->getFetchDataList();
	ResponseFetch::FetchDataList::const_iterator it = l.begin();
	while (it != l.end()) {
		switch ((*it)->getType()) {
		case FetchData::TYPE_BODY:
			{
				FetchDataBody* pBody = static_cast<FetchDataBody*>(*it);
				bool bAdd = false;
				if (pBody->getSection() == FetchDataBody::SECTION_HEADER) {
					bAdd = true;
				}
				else if (pBody->getSection() == FetchDataBody::SECTION_NONE ||
					pBody->getSection() == FetchDataBody::SECTION_MIME) {
					const FetchDataBody::PartPath& path = pBody->getPartPath();
					PartList::const_iterator part = std::find_if(
						listPart.begin(), listPart.end(),
						unary_compose_f_gx(
							PathEqual(&path[0], path.size()),
							std::select2nd<PartList::value_type>()));
					bAdd = part != listPart.end();
				}
				if (bAdd) {
					status = STLWrapper<BodyList>(listBody).push_back(pBody);
					CHECK_QSTATUS();
				}
			}
			break;
		case FetchData::TYPE_UID:
			nUid = static_cast<FetchDataUid*>(*it)->getUid();
			break;
		default:
			break;
		}
		++it;
	}
	
	if (listBody.size() == getPartCount() && nUid != 0) {
		MessagePtr ptr(getMessagePtr(nUid));
		MessagePtrLock mpl(ptr);
		if (mpl) {
			string_ptr<STRING> strContent;
			status = Util::getContentFromBodyStructureAndBodies(
				listPart, listBody, &strContent);
			CHECK_QSTATUS();
			status = getAccount()->updateMessage(mpl, strContent.get());
			CHECK_QSTATUS();
			unsigned int nMask = MessageHolder::FLAG_DOWNLOAD |
				MessageHolder::FLAG_DOWNLOADTEXT |
				MessageHolder::FLAG_PARTIAL_MASK;
			// TODO
			// Should change the flag on server?
			if (isMakeUnseen())
				nMask |= MessageHolder::FLAG_SEEN;
			mpl->setFlags(isAll() ? 0 : MessageHolder::FLAG_TEXTONLY, nMask);
		}
		
		*pbProcessed = true;
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * AbstractBodyStructureProcessHook
 *
 */

qmimap4::AbstractBodyStructureProcessHook::AbstractBodyStructureProcessHook()
{
}

qmimap4::AbstractBodyStructureProcessHook::~AbstractBodyStructureProcessHook()
{
}

QSTATUS qmimap4::AbstractBodyStructureProcessHook::processFetchResponse(
	ResponseFetch* pFetch, bool* pbProcessed)
{
	DECLARE_QSTATUS();
	
	unsigned long nUid = 0;
	FetchDataBodyStructure* pBodyStructure = 0;
	
	int nCount = 0;
	
	const ResponseFetch::FetchDataList& l = pFetch->getFetchDataList();
	ResponseFetch::FetchDataList::const_iterator it = l.begin();
	while (it != l.end()) {
		switch ((*it)->getType()) {
		case FetchData::TYPE_UID:
			nUid = static_cast<FetchDataUid*>(*it)->getUid();
			++nCount;
			break;
		case FetchData::TYPE_BODYSTRUCTURE:
			pBodyStructure = static_cast<FetchDataBodyStructure*>(*it);
			++nCount;
			break;
		}
		++it;
	}
	
	if (nCount == 2) {
		bool bSet = false;
		status = setBodyStructure(nUid, pBodyStructure, &bSet);
		CHECK_QSTATUS();
		if (bSet)
			pFetch->detach(pBodyStructure);
		
		status = processed();
		CHECK_QSTATUS();
		
		*pbProcessed = bSet;
	}
	
	return QSTATUS_SUCCESS;
}
