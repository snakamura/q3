/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmfolder.h>
#include <qmmessageholder.h>

#include <qserror.h>
#include <qsnew.h>
#include <qsstl.h>

#include <algorithm>
#include <cstdio>

#include "util.h"

#pragma warning(disable:4786)

using namespace qmimap4;
using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * Util
 *
 */

QSTATUS qmimap4::Util::getFolderName(NormalFolder* pFolder, WSTRING* pwstrName)
{
	assert(pFolder);
	assert(pwstrName);
	
	DECLARE_QSTATUS();
	
	*pwstrName = 0;
	
	string_ptr<WSTRING> wstrName;
	status = pFolder->getFullName(&wstrName);
	CHECK_QSTATUS();
	
	if (pFolder->isFlag(Folder::FLAG_CHILDOFROOT)) {
		string_ptr<WSTRING> wstrRootFolder;
		Account* pAccount = pFolder->getAccount();
		status = pAccount->getProperty(
			L"Imap4", L"RootFolder", 0, &wstrRootFolder);
		CHECK_QSTATUS();
		
		if (*wstrRootFolder.get()) {
			WCHAR wsz[] = { pFolder->getSeparator(), L'\0' };
			*pwstrName = concat(wstrRootFolder.get(), wsz, wstrName.get());
			if (!*pwstrName)
				return QSTATUS_OUTOFMEMORY;
		}
	}
	
	if (!*pwstrName)
		*pwstrName = wstrName.release();
	
	return QSTATUS_SUCCESS;
}

unsigned int qmimap4::Util::getFolderFlagsFromAttributes(
	unsigned int nAttributes)
{
	struct {
		ResponseList::Attribute attribute_;
		Folder::Flag flag_;
	} flags[] = {
		{ ResponseList::ATTRIBUTE_NOINFERIORS,	Folder::FLAG_NOINFERIORS	},
		{ ResponseList::ATTRIBUTE_NOSELECT,		Folder::FLAG_NOSELECT		}
	};
	
	unsigned int nFlags = 0;
	
	for (int n = 0; n < countof(flags); ++n) {
		if (nAttributes & flags[n].attribute_)
			nFlags |= flags[n].flag_;
	}
	
	return nFlags;
}

unsigned int qmimap4::Util::getMessageFlagsFromImap4Flags(
	unsigned int nSystemFlags, const FetchDataFlags::FlagList& listCustomFlag)
{
	struct {
		Imap4::Flag imap4Flag_;
		MessageHolder::Flag messageFlag_;
	} flags[] = {
		{ Imap4::FLAG_ANSWERED,	MessageHolder::FLAG_REPLIED	},
		{ Imap4::FLAG_DELETED,	MessageHolder::FLAG_DELETED	},
		{ Imap4::FLAG_SEEN,		MessageHolder::FLAG_SEEN	},
		{ Imap4::FLAG_FLAGGED,	MessageHolder::FLAG_MARKED	}
	};
	
	unsigned int nMessageFlags = 0;
	for (int n = 0; n < countof(flags); ++n) {
		if (nSystemFlags & flags[n].imap4Flag_)
			nMessageFlags |= flags[n].messageFlag_;
	}
	
	return nMessageFlags;
}

unsigned int qmimap4::Util::getImap4FlagsFromMessageFlags(unsigned int nFlags)
{
	struct {
		MessageHolder::Flag messageFlag_;
		Imap4::Flag imap4Flag_;
	} flags[] = {
		{ MessageHolder::FLAG_REPLIED,	Imap4::FLAG_ANSWERED	},
		{ MessageHolder::FLAG_DELETED,	Imap4::FLAG_DELETED		},
		{ MessageHolder::FLAG_SEEN,		Imap4::FLAG_SEEN		},
		{ MessageHolder::FLAG_MARKED,	Imap4::FLAG_FLAGGED		}
	};
	
	unsigned int nImap4Flags = 0;
	for (int n = 0; n < countof(flags); ++n) {
		if (nFlags & flags[n].messageFlag_)
			nImap4Flags |= flags[n].imap4Flag_;
	}
	
	return nImap4Flags;
}

QSTATUS qmimap4::Util::getMessageFromEnvelope(
	const FetchDataEnvelope* pEnvelope, STRING* pstrMessage)
{
	assert(pEnvelope);
	assert(pstrMessage);
	
	DECLARE_QSTATUS();
	
	*pstrMessage = 0;
	
	StringBuffer<STRING> buf(&status);
	CHECK_QSTATUS();
	
	const CHAR* pszMessageId = pEnvelope->getMessageId();
	if (pszMessageId && *pszMessageId) {
		status = buf.append("Message-Id: ");
		CHECK_QSTATUS();
		status = buf.append(pszMessageId);
		CHECK_QSTATUS();
		status = buf.append("\r\n");
		CHECK_QSTATUS();
	}
	
	const CHAR* pszInReplyTo = pEnvelope->getInReplyTo();
	if (pszInReplyTo && *pszInReplyTo) {
		status = buf.append("In-Reply-To: ");
		CHECK_QSTATUS();
		status = buf.append(pszInReplyTo);
		CHECK_QSTATUS();
		status = buf.append("\r\n");
		CHECK_QSTATUS();
	}
	
	const CHAR* pszSubject = pEnvelope->getSubject();
	if (pszSubject) {
		status = buf.append("Subject: ");
		CHECK_QSTATUS();
		status = buf.append(pszSubject);
		CHECK_QSTATUS();
		status = buf.append("\r\n");
		CHECK_QSTATUS();
	}
	
	const CHAR* pszDate = pEnvelope->getDate();
	if (pszDate) {
		status = buf.append("Date: ");
		CHECK_QSTATUS();
		status = buf.append(pszDate);
		CHECK_QSTATUS();
		status = buf.append("\r\n");
		CHECK_QSTATUS();
	}
	
	struct {
		FetchDataEnvelope::Address address_;
		const CHAR* pszField_;
	} addresses[] = {
		{ FetchDataEnvelope::ADDRESS_FROM		, "From: "		},
		{ FetchDataEnvelope::ADDRESS_SENDER		, "Sender: "	},
		{ FetchDataEnvelope::ADDRESS_REPLYTO	, "Reply-To: "	},
		{ FetchDataEnvelope::ADDRESS_TO			, "To: "		},
		{ FetchDataEnvelope::ADDRESS_CC			, "Cc: "		},
		{ FetchDataEnvelope::ADDRESS_BCC		, "Bcc: "		}
	};
	for (int n = 0; n < countof(addresses); ++n) {
		typedef FetchDataEnvelope::AddressList AddressList;
		const AddressList& l = pEnvelope->getAddresses(addresses[n].address_);
		if (!l.empty()) {
			status = buf.append(addresses[n].pszField_);
			CHECK_QSTATUS();
			
			bool bGroup = false;
			bool bSeparator = false;
			AddressList::const_iterator it = l.begin();
			while (it != l.end()) {
				EnvelopeAddress* pAddress = *it;
				const CHAR* pszMailbox = pAddress->getMailbox();
				const CHAR* pszHost = pAddress->getHost();
				if (pszHost) {
					if (bSeparator) {
						status = buf.append(",\r\n\t");
						CHECK_QSTATUS();
					}
					bSeparator = true;
					
					const CHAR* pszName = pAddress->getName();
					if (pszName) {
						string_ptr<STRING> strName;
						status = FieldParser::getAtomsOrQString(pszName,
							static_cast<size_t>(-1), &strName);
						CHECK_QSTATUS();
						status = buf.append(strName.get());
						CHECK_QSTATUS();
						status = buf.append(" <");
						CHECK_QSTATUS();
					}
					string_ptr<STRING> strAddrSpec;
					status = AddressParser::getAddrSpec(pszMailbox,
						pszHost, &strAddrSpec);
					CHECK_QSTATUS();
					status = buf.append(strAddrSpec.get());
					CHECK_QSTATUS();
					if (pszName) {
						status = buf.append(">");
						CHECK_QSTATUS();
					}
				}
				else {
					if (bGroup) {
						if (pszMailbox)
							return QSTATUS_FAIL;
						status = buf.append(";");
						CHECK_QSTATUS();
						bSeparator = true;
						bGroup = false;
					}
					else {
						if (bSeparator) {
							status = buf.append(",\r\n\t");
							CHECK_QSTATUS();
						}
						
						if (!pszMailbox)
							return QSTATUS_FAIL;
						status = buf.append(pszMailbox);
						CHECK_QSTATUS();
						status = buf.append(": ");
						CHECK_QSTATUS();
						bSeparator = false;
						bGroup = true;
					}
				}
				++it;
			}
			
			if (bGroup) {
				status = buf.append(";");
				CHECK_QSTATUS();
			}
			
			status = buf.append("\r\n");
			CHECK_QSTATUS();
		}
	}
	
	*pstrMessage = buf.getString();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Util::getHeaderFromBodyStructure(
	const FetchDataBodyStructure* pBodyStructure, STRING* pstrHeader)
{
	assert(pBodyStructure);
	assert(pstrHeader);
	
	DECLARE_QSTATUS();
	
	typedef FetchDataBodyStructure::ParamList ParamList;
	
	StringBuffer<STRING> buf(&status);
	CHECK_QSTATUS();
	
	const CHAR* pszContentType[] = {
		pBodyStructure->getContentType(),
		"/",
		pBodyStructure->getContentSubType()
	};
	status = appendDataToBuffer("Content-Type", pszContentType,
		countof(pszContentType), &pBodyStructure->getContentParams(), &buf);
	CHECK_QSTATUS();
	
	const CHAR* pszDisposition = pBodyStructure->getDisposition();
	status = appendDataToBuffer("Content-Disposition",
		&pszDisposition, 1, &pBodyStructure->getDispositionParams(), &buf);
	CHECK_QSTATUS();
	
	const CHAR* pszEncoding = pBodyStructure->getEncoding();
	status = appendDataToBuffer("Content-Transfer-Encoding", &pszEncoding, 1, 0, &buf);
	
	const CHAR* pszId = pBodyStructure->getId();
	status = appendDataToBuffer("Content-Id", &pszId, 1, 0, &buf);
	
	*pstrHeader = buf.getString();
	
	return QSTATUS_SUCCESS;
}

unsigned long qmimap4::Util::getTextSizeFromBodyStructure(
	const FetchDataBodyStructure* pBodyStructure)
{
	assert(pBodyStructure);
	
	unsigned long nSize = 0;
	
	if (_stricmp(pBodyStructure->getContentType(), "multipart") == 0) {
		const FetchDataBodyStructure::ChildList& l = pBodyStructure->getChildList();
		FetchDataBodyStructure::ChildList::const_iterator it = l.begin();
		while (it != l.end())
			nSize += getTextSizeFromBodyStructure(*it++);
	}
	else if (isInlineTextPart(pBodyStructure)) {
		nSize = pBodyStructure->getSize();
	}
	
	return nSize;
}

QSTATUS qmimap4::Util::getPartsFromBodyStructure(
	const FetchDataBodyStructure* pBodyStructure,
	const unsigned int* pBasePath, PartList* pListPart)
{
	assert(pBodyStructure);
	assert(pBasePath);
	assert(pListPart);
	
	DECLARE_QSTATUS();
	
	size_t nBaseLen = getPathLength(pBasePath);
	
	malloc_ptr<unsigned int> pPath(static_cast<unsigned int*>(
		malloc((nBaseLen + 1)*sizeof(unsigned int))));
	if (!pPath.get())
		return QSTATUS_OUTOFMEMORY;
	std::copy(pBasePath, pBasePath + nBaseLen, pPath.get());
	*(pPath.get() + nBaseLen) = 0;
	status = STLWrapper<PartList>(*pListPart).push_back(
		std::make_pair(pBodyStructure, pPath.get()));
	CHECK_QSTATUS();
	pPath.release();
	
	if (_stricmp(pBodyStructure->getContentType(), "multipart") == 0) {
		const FetchDataBodyStructure::ChildList& l = pBodyStructure->getChildList();
		FetchDataBodyStructure::ChildList::size_type n = 0;
		while (n < l.size()) {
			malloc_ptr<unsigned int> pPath(static_cast<unsigned int*>(
				malloc((nBaseLen + 2)*sizeof(unsigned int))));
			if (!pPath.get())
				return QSTATUS_OUTOFMEMORY;
			std::copy(pBasePath, pBasePath + nBaseLen, pPath.get());
			*(pPath.get() + nBaseLen) = n + 1;
			*(pPath.get() + nBaseLen + 1) = 0;
			
			if (_stricmp(l[n]->getContentType(), "multipart") == 0) {
				status = getPartsFromBodyStructure(l[n], pPath.get(), pListPart);
				CHECK_QSTATUS();
			}
			else {
				status = STLWrapper<PartList>(*pListPart).push_back(
					std::make_pair(l[n], pPath.get()));
				CHECK_QSTATUS();
				pPath.release();
			}
			++n;
		}
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Util::getContentFromBodyStructureAndBodies(
	const PartList& listPart, const BodyList& listBody, qs::STRING* pstrContent)
{
	assert(pstrContent);
	
	DECLARE_QSTATUS();
	
	BodyList::const_iterator it = std::find_if(
		listBody.begin(), listBody.end(),
		unary_compose_f_gx(
			std::mem_fun_ref(FetchDataBody::PartPath::empty),
			std::mem_fun(FetchDataBody::getPartPath)));
	if (it == listBody.end())
		return QSTATUS_FAIL;
	
	StringBuffer<STRING> buf((*it)->getContent(), &status);
	CHECK_QSTATUS();
	
	size_t nDepth = 0;
	PartList::const_iterator itP = listPart.begin();
	while (itP != listPart.end()) {
		const unsigned int* pPath = (*itP).second;
		size_t n = getPathLength(pPath);
		if (n > nDepth)
			nDepth = n;
		++itP;
	}
	
	typedef std::vector<const FetchDataBodyStructure*> BodyStructureStack;
	BodyStructureStack stack;
	status = STLWrapper<BodyStructureStack>(stack).reserve(nDepth);
	CHECK_QSTATUS();
	
	itP = listPart.begin();
	++itP;
	while (itP != listPart.end()) {
		std::pair<FetchDataBody*, FetchDataBody*> body =
			getBodyFromBodyList(listBody, (*itP).second);
		size_t nPathLen = getPathLength((*itP).second);
		assert(nPathLen != 0);
		if (nPathLen > stack.size()) {
			assert(nPathLen == stack.size() + 1);
			status = appendBoundaryToBuffer((*(itP - 1)).first, &buf, false);
			CHECK_QSTATUS();
			stack.push_back((*(itP - 1)).first);
		}
		else {
			while (nPathLen < stack.size()) {
				status = appendBoundaryToBuffer(stack.back(), &buf, true);
				CHECK_QSTATUS();
				stack.pop_back();
			}
			status = appendBoundaryToBuffer(stack.back(), &buf, false);
			CHECK_QSTATUS();
		}
		
		if (body.first) {
			status = buf.append(body.first->getContent());
			CHECK_QSTATUS();
		}
		else {
			string_ptr<STRING> strHeader;
			status = getHeaderFromBodyStructure((*itP).first, &strHeader);
			CHECK_QSTATUS();
			status = buf.append(strHeader.get());
			CHECK_QSTATUS();
		}
		
		if (body.second) {
			status = buf.append(body.second->getContent());
			CHECK_QSTATUS();
		}
		
		++itP;
	}
	while (stack.size() > 0) {
		status = appendBoundaryToBuffer(stack.back(), &buf, true);
		CHECK_QSTATUS();
		stack.pop_back();
	}
	
	*pstrContent = buf.getString();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Util::getFetchArgFromPartList(const PartList& listPart,
	FetchArg arg, bool bPeek, bool bFetchAllMime, STRING* pstrArg,
	unsigned int* pnCount, bool* pbAll)
{
	assert(!listPart.empty());
	assert(pstrArg);
	assert(pnCount);
	assert(pbAll);
	
	DECLARE_QSTATUS();
	
	*pstrArg = 0;
	*pnCount = 0;
	*pbAll = false;
	
	const CHAR* pszBody = bPeek ? "BODY.PEEK" : "BODY";
	
	unsigned int nPartCount = 1;
	
	StringBuffer<STRING> buf("(", &status);
	CHECK_QSTATUS();
	status = buf.append(pszBody);
	CHECK_QSTATUS();
	if (listPart.size() == 1 &&
		Util::isInlineTextPart(listPart.front().first)) {
		status = buf.append("[]");
		CHECK_QSTATUS();
		*pbAll = true;
	}
	else {
		status = buf.append("[HEADER]");
		CHECK_QSTATUS();
		
		PartList::const_iterator it = listPart.begin();
		while (it != listPart.end()) {
			bool bFetchBody = arg == FETCHARG_TEXT ?
				Util::isInlineTextPart((*it).first) :
				Util::isInlineHtmlPart((*it).first);
			if (bFetchBody || (bFetchAllMime && it != listPart.begin())) {
				string_ptr<STRING> strPath;
				status = Util::formatPath((*it).second, &strPath);
				CHECK_QSTATUS();
				
				status = buf.append(" ");
				CHECK_QSTATUS();
				if (bFetchBody) {
					status = buf.append(pszBody);
					CHECK_QSTATUS();
					status = buf.append("[");
					CHECK_QSTATUS();
					status = buf.append(strPath.get());
					CHECK_QSTATUS();
					status = buf.append("] ");
					CHECK_QSTATUS();
					++nPartCount;
				}
				status = buf.append(pszBody);
				CHECK_QSTATUS();
				status = buf.append("[");
				CHECK_QSTATUS();
				status = buf.append(strPath.get());
				CHECK_QSTATUS();
				status = buf.append(".MIME]");
				CHECK_QSTATUS();
				++nPartCount;
			}
			++it;
		}
	}
	status = buf.append(")");
	CHECK_QSTATUS();
	
	*pstrArg = buf.getString();
	*pnCount = nPartCount;
	
	return QSTATUS_SUCCESS;
}

bool qmimap4::Util::isInlineTextPart(const FetchDataBodyStructure* pBodyStructure)
{
	const CHAR* pszDisposition = pBodyStructure->getDisposition();
	const CHAR* pszContentType = pBodyStructure->getContentType();
	const CHAR* pszSubType = pBodyStructure->getContentSubType();
	return (!pszDisposition || _stricmp(pszDisposition, "attachment") != 0) &&
		(!pszContentType || _stricmp(pszContentType, "text") == 0 ||
			(_stricmp(pszContentType, "message") == 0 &&
				pszSubType && _stricmp(pszSubType, "rfc822") == 0));
}

bool qmimap4::Util::isInlineHtmlPart(const FetchDataBodyStructure* pBodyStructure)
{
	return isInlineTextPart(pBodyStructure) || pBodyStructure->getId();
}

bool qmimap4::Util::hasAttachmentPart(const FetchDataBodyStructure* pBodyStructure)
{
	const CHAR* pszContentType = pBodyStructure->getContentType();
	if (_stricmp(pszContentType, "multipart") == 0) {
		const FetchDataBodyStructure::ChildList& l = pBodyStructure->getChildList();
		FetchDataBodyStructure::ChildList::const_iterator it = l.begin();
		while (it != l.end()) {
			if (hasAttachmentPart(*it))
				return true;
			++it;
		}
		return false;
	}
	else {
		return !isInlineTextPart(pBodyStructure);
	}
}

size_t qmimap4::Util::getPathLength(const unsigned int* pPath)
{
	size_t nLen = 0;
	while (*pPath) {
		++pPath;
		++nLen;
	}
	return nLen;
}

QSTATUS qmimap4::Util::formatPath(const unsigned int* pPath, qs::STRING* pstrPath)
{
	assert(pPath);
	assert(pstrPath);
	
	DECLARE_QSTATUS();
	
	StringBuffer<STRING> buf(&status);
	CHECK_QSTATUS();
	
	if (*pPath) {
		CHAR sz[32];
		const unsigned int* p = pPath;
		while (*p) {
			sprintf(sz, "%u.", *p);
			status = buf.append(sz);
			CHECK_QSTATUS();
			++p;
		}
		size_t n = buf.getLength();
		*pstrPath = buf.getString();
		*(*pstrPath + n - 1) = '\0';
	}
	else {
		*pstrPath = buf.getString();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Util::createUidList(const MessageHolderList& l, UidList* pList)
{
	assert(std::find_if(l.begin(), l.end(),
		std::bind2nd(
			std::mem_fun(&MessageHolder::isFlag),
			MessageHolder::FLAG_LOCAL)) == l.end());
	
	DECLARE_QSTATUS();
	
	status = STLWrapper<UidList>(*pList).resize(l.size());
	CHECK_QSTATUS();
	std::transform(l.begin(), l.end(), pList->begin(),
		std::mem_fun(&MessageHolder::getId));
	std::sort(pList->begin(), pList->end());
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Util::createRange(const MessageHolderList& l,
	std::auto_ptr<MultipleRange>* apRange)
{
	DECLARE_QSTATUS();
	
	UidList listUid;
	status = createUidList(l, &listUid);
	CHECK_QSTATUS();
	
	status = newQsObject(&listUid[0], listUid.size(), true, apRange);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

bool qmimap4::Util::isEqualFolderName(const WCHAR* pwszLhs,
	const WCHAR* pwszRhs, WCHAR cSeparator)
{
	if (wcscmp(pwszLhs, pwszRhs) == 0) {
		return true;
	}
	else if (_wcsnicmp(pwszLhs, L"INBOX", 5) == 0 &&
		_wcsnicmp(pwszRhs, L"INBOX", 5) == 0) {
		if (*(pwszLhs + 5) == L'\0' && *(pwszRhs + 5) == L'\0')
			return true;
		else if (*(pwszLhs + 5) == cSeparator && *(pwszRhs + 5) == cSeparator)
			return wcscmp(pwszLhs + 6, pwszRhs + 6) == 0;
		else
			return false;
	}
	else {
		return false;
	}
}

QSTATUS qmimap4::Util::getSsl(SubAccount* pSubAccount, Imap4::Ssl* pSsl)
{
	assert(pSubAccount);
	assert(pSsl);
	
	DECLARE_QSTATUS();
	
	Imap4::Ssl ssl = Imap4::SSL_NONE;
	if (pSubAccount->isSsl(Account::HOST_RECEIVE)) {
		ssl = Imap4::SSL_SSL;
	}
	else {
		int nStartTls = 0;
		status = pSubAccount->getProperty(L"Imap4", L"STARTTLS", 0, &nStartTls);
		CHECK_QSTATUS();
		if (nStartTls)
			ssl = Imap4::SSL_STARTTLS;
	}
	
	*pSsl = ssl;
	
	return QSTATUS_SUCCESS;
}

std::pair<FetchDataBody*, FetchDataBody*> qmimap4::Util::getBodyFromBodyList(
	const BodyList& listBody, const unsigned int* pPath)
{
	std::pair<FetchDataBody*, FetchDataBody*> body(0, 0);
	
	BodyList::const_iterator it = listBody.begin();
	while (it != listBody.end() && (!body.first || !body.second)) {
		const FetchDataBody::PartPath& path = (*it)->getPartPath();
		if (PathEqual(&path[0], path.size())(pPath)) {
			switch ((*it)->getSection()) {
			case FetchDataBody::SECTION_MIME:
				body.first = *it;
				break;
			case FetchDataBody::SECTION_NONE:
				body.second = *it;
				break;
			default:
				break;
			}
		}
		++it;
	}
	
	return body;
}

const CHAR* qmimap4::Util::getBoundaryFromBodyStructure(
	const FetchDataBodyStructure* pBodyStructure)
{
	typedef FetchDataBodyStructure::ParamList ParamList;
	const ParamList& l = pBodyStructure->getContentParams();
	ParamList::const_iterator it = std::find_if(
		l.begin(), l.end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				string_equal_i<CHAR>(),
				std::select1st<ParamList::value_type>(),
				std::identity<const CHAR*>()),
			"boundary"));
	if (it != l.end())
		return (*it).second;
	else
		return "";
}

QSTATUS qmimap4::Util::appendBoundaryToBuffer(
	const FetchDataBodyStructure* pBodyStructure,
	StringBuffer<STRING>* pBuf, bool bEnd)
{
	// TODO
	// If the result of BODYSTRUCTURE is not trusted
	// (OPTION_TRUSTBODYSTRUCTURE is not set),
	// I need to get the boundary from
	// the result of BODY[] instead of BODYSTRUCTURE
	
	assert(pBodyStructure);
	assert(pBuf);
	
	DECLARE_QSTATUS();
	
	status = pBuf->append("\r\n--");
	CHECK_QSTATUS();
	status = pBuf->append(getBoundaryFromBodyStructure(pBodyStructure));
	CHECK_QSTATUS();
	if (bEnd) {
		status = pBuf->append("--");
		CHECK_QSTATUS();
	}
	status = pBuf->append("\r\n");
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Util::appendDataToBuffer(const CHAR* pszName, const CHAR** ppsz, size_t nLen,
	const FetchDataBodyStructure::ParamList* pListParam, StringBuffer<STRING>* pBuf)
{
	assert(pszName);
	assert(ppsz);
	assert(pBuf);
	
	DECLARE_QSTATUS();
	
	if (*ppsz && **ppsz) {
		status = pBuf->append(pszName);
		CHECK_QSTATUS();
		status = pBuf->append(": ");
		CHECK_QSTATUS();
		for (size_t n = 0; n < nLen; ++n) {
			status = pBuf->append(*(ppsz + n));
			CHECK_QSTATUS();
		}
		if (pListParam) {
			status = appendParamsToBuffer(*pListParam, pBuf);
			CHECK_QSTATUS();
		}
		status = pBuf->append("\r\n");
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::Util::appendParamsToBuffer(
	const FetchDataBodyStructure::ParamList& l, StringBuffer<STRING>* pBuf)
{
	assert(pBuf);
	
	DECLARE_QSTATUS();
	
	FetchDataBodyStructure::ParamList::const_iterator it = l.begin();
	while (it != l.end()) {
		status = pBuf->append(";\r\n\t");
		CHECK_QSTATUS();
		status = pBuf->append((*it).first);
		CHECK_QSTATUS();
		status = pBuf->append("=");
		CHECK_QSTATUS();
		string_ptr<STRING> strValue;
		status = FieldParser::getAtomOrQString((*it).second,
			static_cast<size_t>(-1), &strValue);
		CHECK_QSTATUS();
		status = pBuf->append(strValue.get());
		CHECK_QSTATUS();
		++it;
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * Util::PartListDeleter
 *
 */

qmimap4::Util::PartListDeleter::PartListDeleter(Util::PartList& l) :
	l_(l)
{
}

qmimap4::Util::PartListDeleter::~PartListDeleter()
{
	std::for_each(l_.begin(), l_.end(),
		unary_compose_f_gx(
			PathFree()/*std::ptr_fun(free)*/,
			std::select2nd<Util::PartList::value_type>()));
	l_.clear();
}


/****************************************************************************
 *
 * PathEqual
 *
 */

qmimap4::PathEqual::PathEqual(const unsigned int* pPath, size_t nLen) :
	pPath_(pPath),
	nLen_(nLen)
{
}

bool qmimap4::PathEqual::operator()(const unsigned int* pPath) const
{
	const unsigned int* p = pPath_;
	while (static_cast<size_t>(p - pPath_) < nLen_) {
		if (*p != *pPath)
			return false;
		++pPath;
		++p;
	}
	return *pPath == 0;
}


/****************************************************************************
 *
 * PathFree
 *
 */

void* qmimap4::PathFree::operator()(unsigned int* path) const
{
	free(path);
	return 0;
}


/****************************************************************************
 *
 * AbstractCallback
 *
 */

qmimap4::AbstractCallback::AbstractCallback(SubAccount* pSubAccount,
	const Security* pSecurity, QSTATUS* pstatus) :
	DefaultSSLSocketCallback(pSubAccount, Account::HOST_RECEIVE, pSecurity),
	pSubAccount_(pSubAccount)
{
	*pstatus = QSTATUS_SUCCESS;
}

qmimap4::AbstractCallback::~AbstractCallback()
{
}

QSTATUS qmimap4::AbstractCallback::getUserInfo(
	WSTRING* pwstrUserName, WSTRING* pwstrPassword)
{
	assert(pwstrUserName);
	assert(pwstrPassword);
	
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrUserName(
		allocWString(pSubAccount_->getUserName(Account::HOST_RECEIVE)));
	if (!wstrUserName.get())
		return QSTATUS_OUTOFMEMORY;
	string_ptr<WSTRING> wstrPassword(
		allocWString(pSubAccount_->getPassword(Account::HOST_RECEIVE)));
	if (!wstrPassword.get())
		return QSTATUS_OUTOFMEMORY;
	
	*pwstrUserName = wstrUserName.release();
	*pwstrPassword = wstrPassword.release();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::AbstractCallback::setPassword(const WCHAR* pwszPassword)
{
	// TODO
	return QSTATUS_SUCCESS;
}

QSTATUS qmimap4::AbstractCallback::getAuthMethods(qs::WSTRING* pwstrAuthMethods)
{
	assert(pwstrAuthMethods);
	return pSubAccount_->getProperty(L"Imap4",
		L"AuthMethods", L"", pwstrAuthMethods);
}
