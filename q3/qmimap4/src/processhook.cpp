/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsassert.h>
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

ProcessHook::Result qmimap4::DefaultProcessHook::processFetchResponse(ResponseFetch* pFetch)
{
	assert(pFetch);
	return RESULT_UNPROCESSED;
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

ProcessHook::Result qmimap4::AbstractMessageProcessHook::processFetchResponse(ResponseFetch* pFetch)
{
	bool bHeader = isHeader();
	
	unsigned long nUid = 0;
	FetchDataBody* pBody = 0;
	
	int nCount = 0;
	
	const ResponseFetch::FetchDataList& l = pFetch->getFetchDataList();
	for (ResponseFetch::FetchDataList::const_iterator it = l.begin(); it != l.end(); ++it) {
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
	}
	
	if (nCount != 2)
		return RESULT_UNPROCESSED;
	
	MessagePtr ptr(getMessagePtr(nUid));
	MessagePtrLock mpl(ptr);
	if (mpl) {
		if (!getAccount()->updateMessage(mpl, pBody->getContent()))
			return RESULT_ERROR;
		unsigned int nMask = MessageHolder::FLAG_DOWNLOAD |
			MessageHolder::FLAG_DOWNLOADTEXT |
			MessageHolder::FLAG_PARTIAL_MASK;
		// TODO
		// Should change the flag on server?
		if (isMakeUnseen())
			nMask |= MessageHolder::FLAG_SEEN;
		mpl->setFlags(bHeader ? MessageHolder::FLAG_HEADERONLY : 0, nMask);
	}
	
	processed();
	
	return RESULT_PROCESSED;
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

ProcessHook::Result qmimap4::AbstractPartialMessageProcessHook::processFetchResponse(ResponseFetch* pFetch)
{
	const PartList& listPart = getPartList();
	
	unsigned long nUid = 0;
	BodyList listBody;
	
	const ResponseFetch::FetchDataList& l = pFetch->getFetchDataList();
	for (ResponseFetch::FetchDataList::const_iterator it = l.begin(); it != l.end(); ++it) {
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
				if (bAdd)
					listBody.push_back(pBody);
			}
			break;
		case FetchData::TYPE_UID:
			nUid = static_cast<FetchDataUid*>(*it)->getUid();
			break;
		default:
			break;
		}
	}
	
	if (listBody.size() != getPartCount() || nUid == 0)
		return RESULT_UNPROCESSED;
	
	MessagePtr ptr(getMessagePtr(nUid));
	MessagePtrLock mpl(ptr);
	if (mpl) {
		xstring_ptr strContent(Util::getContentFromBodyStructureAndBodies(listPart, listBody));
		if (!getAccount()->updateMessage(mpl, strContent.get()))
			return RESULT_ERROR;
		unsigned int nMask = MessageHolder::FLAG_DOWNLOAD |
			MessageHolder::FLAG_DOWNLOADTEXT |
			MessageHolder::FLAG_PARTIAL_MASK;
		// TODO
		// Should change the flag on server?
		if (isMakeUnseen())
			nMask |= MessageHolder::FLAG_SEEN;
		mpl->setFlags(isAll() ? 0 : MessageHolder::FLAG_TEXTONLY, nMask);
	}
	
	return RESULT_PROCESSED;
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

ProcessHook::Result qmimap4::AbstractBodyStructureProcessHook::processFetchResponse(ResponseFetch* pFetch)
{
	unsigned long nUid = 0;
	FetchDataBodyStructure* pBodyStructure = 0;
	
	int nCount = 0;
	
	const ResponseFetch::FetchDataList& l = pFetch->getFetchDataList();
	for (ResponseFetch::FetchDataList::const_iterator it = l.begin(); it != l.end(); ++it) {
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
	}
	
	if (nCount != 2)
		return RESULT_UNPROCESSED;
	
	bool bSet = false;
	if (!setBodyStructure(nUid, pBodyStructure, &bSet))
		return RESULT_ERROR;
	if (bSet)
		pFetch->detach(pBodyStructure);
	
	processed();
	
	return bSet ? RESULT_PROCESSED : RESULT_UNPROCESSED;
}
