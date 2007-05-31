/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
 * All rights reserved.
 *
 */

#pragma warning(disable:4786)

#include <qsassert.h>
#include <qsstl.h>

#include "imap4.h"
#include "option.h"
#include "processhook.h"

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
	
	unsigned long nUid = pFetch->getUid();
	FetchDataBody* pBody = 0;
	
	const ResponseFetch::FetchDataList& l = pFetch->getFetchDataList();
	for (ResponseFetch::FetchDataList::const_iterator it = l.begin(); it != l.end(); ++it) {
		switch ((*it)->getType()) {
		case FetchData::TYPE_BODY:
			{
				FetchDataBody* p = static_cast<FetchDataBody*>(*it);
				if (((bHeader && p->getSection() == FetchDataBody::SECTION_HEADER) ||
					(!bHeader && p->getSection() == FetchDataBody::SECTION_NONE)) &&
					p->getPartPath().empty())
					pBody = p;
			}
			break;
		default:
			break;
		}
	}
	
	if (nUid == -1 || !pBody)
		return RESULT_UNPROCESSED;
	
	MessagePtr ptr(getMessagePtr(nUid));
	MessagePtrLock mpl(ptr);
	if (mpl) {
		std::pair<const CHAR*, size_t> content(pBody->getContent().get());
		if (!getAccount()->updateMessage(mpl, content.first, content.second, 0))
			return RESULT_ERROR;
		mpl->setFlags(bHeader ? MessageHolder::FLAG_HEADERONLY : 0,
			MessageHolder::FLAG_DOWNLOAD |
			MessageHolder::FLAG_DOWNLOADTEXT |
			MessageHolder::FLAG_PARTIAL_MASK);
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
	
	unsigned long nUid = pFetch->getUid();
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
						boost::bind(PathEqual(&path[0], path.size()),
							boost::bind(&PartList::value_type::second, _1)));
					bAdd = part != listPart.end();
				}
				if (bAdd)
					listBody.push_back(pBody);
			}
			break;
		default:
			break;
		}
	}
	
	if (nUid == -1 || listBody.size() != getPartCount())
		return RESULT_UNPROCESSED;
	
	MessagePtr ptr(getMessagePtr(nUid));
	MessagePtrLock mpl(ptr);
	if (mpl) {
		xstring_size_ptr strContent(Util::getContentFromBodyStructureAndBodies(
			listPart, listBody, (getOption() & OPTION_TRUSTBODYSTRUCTURE) != 0));
		if (!getAccount()->updateMessage(mpl, strContent.get(), strContent.size(), 0))
			return RESULT_ERROR;
		mpl->setFlags(isAll() ? 0 : MessageHolder::FLAG_TEXTONLY,
			MessageHolder::FLAG_DOWNLOAD |
			MessageHolder::FLAG_DOWNLOADTEXT |
			MessageHolder::FLAG_PARTIAL_MASK);
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
	unsigned long nUid = pFetch->getUid();
	FetchDataBodyStructure* pBodyStructure = 0;
	
	const ResponseFetch::FetchDataList& l = pFetch->getFetchDataList();
	for (ResponseFetch::FetchDataList::const_iterator it = l.begin(); it != l.end(); ++it) {
		switch ((*it)->getType()) {
		case FetchData::TYPE_BODYSTRUCTURE:
			pBodyStructure = static_cast<FetchDataBodyStructure*>(*it);
			break;
		}
	}
	
	if (nUid == -1 || !pBodyStructure)
		return RESULT_UNPROCESSED;
	
	bool bSet = false;
	if (!setBodyStructure(nUid, pBodyStructure, &bSet))
		return RESULT_ERROR;
	if (bSet)
		pFetch->detach(pBodyStructure);
	
	processed();
	
	return bSet ? RESULT_PROCESSED : RESULT_UNPROCESSED;
}
