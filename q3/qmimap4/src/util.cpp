/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmfolder.h>
#include <qmmessageholder.h>

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

wstring_ptr qmimap4::Util::getFolderName(NormalFolder* pFolder)
{
	assert(pFolder);
	
	wstring_ptr wstrName(pFolder->getFullName());
	
	if (pFolder->isFlag(Folder::FLAG_CHILDOFROOT)) {
		Account* pAccount = pFolder->getAccount();
		wstring_ptr wstrRootFolder(pAccount->getProperty(L"Imap4", L"RootFolder", L""));
		
		if (*wstrRootFolder.get()) {
			WCHAR wsz[] = { pFolder->getSeparator(), L'\0' };
			wstrName = concat(wstrRootFolder.get(), wsz, wstrName.get());
		}
	}
	
	return wstrName;
}

unsigned int qmimap4::Util::getFolderFlagsFromAttributes(unsigned int nAttributes)
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

unsigned int qmimap4::Util::getMessageFlagsFromImap4Flags(unsigned int nSystemFlags,
														  const FetchDataFlags::FlagList& listCustomFlag)
{
	struct {
		Imap4::Flag imap4Flag_;
		MessageHolder::Flag messageFlag_;
	} flags[] = {
		{ Imap4::FLAG_ANSWERED,	MessageHolder::FLAG_REPLIED	},
		{ Imap4::FLAG_DELETED,	MessageHolder::FLAG_DELETED	},
		{ Imap4::FLAG_SEEN,		MessageHolder::FLAG_SEEN	},
		{ Imap4::FLAG_FLAGGED,	MessageHolder::FLAG_MARKED	},
		{ Imap4::FLAG_DRAFT,	MessageHolder::FLAG_DRAFT	}
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
		{ MessageHolder::FLAG_MARKED,	Imap4::FLAG_FLAGGED		},
		{ MessageHolder::FLAG_DRAFT,	Imap4::FLAG_DRAFT		}
	};
	
	unsigned int nImap4Flags = 0;
	for (int n = 0; n < countof(flags); ++n) {
		if (nFlags & flags[n].messageFlag_)
			nImap4Flags |= flags[n].imap4Flag_;
	}
	
	return nImap4Flags;
}

string_ptr qmimap4::Util::getMessageFromEnvelope(const FetchDataEnvelope* pEnvelope)
{
	assert(pEnvelope);
	
	StringBuffer<STRING> buf;
	
	const CHAR* pszMessageId = pEnvelope->getMessageId();
	if (pszMessageId && *pszMessageId) {
		buf.append("Message-Id: ");
		buf.append(pszMessageId);
		buf.append("\r\n");
	}
	
	const CHAR* pszInReplyTo = pEnvelope->getInReplyTo();
	if (pszInReplyTo && *pszInReplyTo) {
		buf.append("In-Reply-To: ");
		buf.append(pszInReplyTo);
		buf.append("\r\n");
	}
	
	const CHAR* pszSubject = pEnvelope->getSubject();
	if (pszSubject) {
		buf.append("Subject: ");
		buf.append(pszSubject);
		buf.append("\r\n");
	}
	
	const CHAR* pszDate = pEnvelope->getDate();
	if (pszDate) {
		buf.append("Date: ");
		buf.append(pszDate);
		buf.append("\r\n");
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
			buf.append(addresses[n].pszField_);
			
			bool bGroup = false;
			bool bSeparator = false;
			for (AddressList::const_iterator it = l.begin(); it != l.end(); ++it) {
				EnvelopeAddress* pAddress = *it;
				const CHAR* pszMailbox = pAddress->getMailbox();
				const CHAR* pszHost = pAddress->getHost();
				if (pszHost) {
					if (bSeparator)
						buf.append(",\r\n\t");
					bSeparator = true;
					
					const CHAR* pszName = pAddress->getName();
					if (pszName) {
						string_ptr strName(FieldParserUtil<STRING>::getAtomsOrQString(pszName, -1));
						buf.append(strName.get());
						buf.append(" <");
					}
					string_ptr strAddrSpec(AddressParser::getAddrSpec(pszMailbox, pszHost));
					buf.append(strAddrSpec.get());
					if (pszName)
						buf.append(">");
				}
				else {
					if (bGroup) {
						if (pszMailbox)
							return 0;
						buf.append(";");
						bSeparator = true;
						bGroup = false;
					}
					else {
						if (bSeparator)
							buf.append(",\r\n\t");
						
						if (!pszMailbox)
							return 0;
						buf.append(pszMailbox);
						buf.append(": ");
						bSeparator = false;
						bGroup = true;
					}
				}
			}
			
			if (bGroup)
				buf.append(";");
			
			buf.append("\r\n");
		}
	}
	
	return buf.getString();
}

string_ptr qmimap4::Util::getHeaderFromBodyStructure(const FetchDataBodyStructure* pBodyStructure)
{
	assert(pBodyStructure);
	
	typedef FetchDataBodyStructure::ParamList ParamList;
	
	StringBuffer<STRING> buf;
	
	const CHAR* pszContentType[] = {
		pBodyStructure->getContentType(),
		"/",
		pBodyStructure->getContentSubType()
	};
	appendDataToBuffer("Content-Type", pszContentType,
		countof(pszContentType), &pBodyStructure->getContentParams(), &buf);
	
	const CHAR* pszDisposition = pBodyStructure->getDisposition();
	appendDataToBuffer("Content-Disposition", &pszDisposition,
		1, &pBodyStructure->getDispositionParams(), &buf);
	
	const CHAR* pszEncoding = pBodyStructure->getEncoding();
	appendDataToBuffer("Content-Transfer-Encoding", &pszEncoding, 1, 0, &buf);
	
	const CHAR* pszId = pBodyStructure->getId();
	appendDataToBuffer("Content-Id", &pszId, 1, 0, &buf);
	
	return buf.getString();
}

unsigned long qmimap4::Util::getTextSizeFromBodyStructure(const FetchDataBodyStructure* pBodyStructure)
{
	assert(pBodyStructure);
	
	unsigned long nSize = 0;
	
	if (_stricmp(pBodyStructure->getContentType(), "multipart") == 0) {
		const FetchDataBodyStructure::ChildList& l = pBodyStructure->getChildList();
		for (FetchDataBodyStructure::ChildList::const_iterator it = l.begin(); it != l.end(); ++it)
			nSize += getTextSizeFromBodyStructure(*it);
	}
	else if (isInlineTextPart(pBodyStructure)) {
		nSize = pBodyStructure->getSize();
	}
	
	return nSize;
}

void qmimap4::Util::getPartsFromBodyStructure(const FetchDataBodyStructure* pBodyStructure,
											  const unsigned int* pBasePath,
											  PartList* pListPart)
{
	assert(pBodyStructure);
	assert(pBasePath);
	assert(pListPart);
	
	size_t nBaseLen = getPathLength(pBasePath);
	
	auto_ptr_array<unsigned int> pPath(new unsigned int[nBaseLen + 1]);
	std::copy(pBasePath, pBasePath + nBaseLen, pPath.get());
	*(pPath.get() + nBaseLen) = 0;
	pListPart->push_back(std::make_pair(pBodyStructure, pPath.get()));
	pPath.release();
	
	if (_stricmp(pBodyStructure->getContentType(), "multipart") == 0) {
		const FetchDataBodyStructure::ChildList& l = pBodyStructure->getChildList();
		for (FetchDataBodyStructure::ChildList::size_type n = 0; n < l.size(); ++n) {
			auto_ptr_array<unsigned int> pPath(new unsigned int[nBaseLen + 2]);
			std::copy(pBasePath, pBasePath + nBaseLen, pPath.get());
			*(pPath.get() + nBaseLen) = n + 1;
			*(pPath.get() + nBaseLen + 1) = 0;
			
			if (_stricmp(l[n]->getContentType(), "multipart") == 0) {
				getPartsFromBodyStructure(l[n], pPath.get(), pListPart);
			}
			else {
				pListPart->push_back(std::make_pair(l[n], pPath.get()));
				pPath.release();
			}
		}
	}
}

xstring_ptr qmimap4::Util::getContentFromBodyStructureAndBodies(const PartList& listPart,
																const BodyList& listBody)
{
	BodyList::const_iterator it = std::find_if(
		listBody.begin(), listBody.end(),
		unary_compose_f_gx(
			std::mem_fun_ref(FetchDataBody::PartPath::empty),
			std::mem_fun(FetchDataBody::getPartPath)));
	if (it == listBody.end())
		return 0;
	
	XStringBuffer<XSTRING> buf;
	if (!buf.append((*it)->getContent()))
		return 0;
	
	size_t nDepth = 0;
	for (PartList::const_iterator itP = listPart.begin(); itP != listPart.end(); ++itP) {
		const unsigned int* pPath = (*itP).second;
		size_t n = getPathLength(pPath);
		if (n > nDepth)
			nDepth = n;
	}
	
	typedef std::vector<const FetchDataBodyStructure*> BodyStructureStack;
	BodyStructureStack stack;
	stack.reserve(nDepth);
	
	for (PartList::const_iterator itP = listPart.begin() + 1; itP != listPart.end(); ++itP) {
		std::pair<FetchDataBody*, FetchDataBody*> body =
			getBodyFromBodyList(listBody, (*itP).second);
		size_t nPathLen = getPathLength((*itP).second);
		assert(nPathLen != 0);
		if (nPathLen > stack.size()) {
			assert(nPathLen == stack.size() + 1);
			if (!appendBoundaryToBuffer((*(itP - 1)).first, &buf, false))
				return 0;
			stack.push_back((*(itP - 1)).first);
		}
		else {
			while (nPathLen < stack.size()) {
				if (!appendBoundaryToBuffer(stack.back(), &buf, true))
					return 0;
				stack.pop_back();
			}
			if (!appendBoundaryToBuffer(stack.back(), &buf, false))
				return 0;
		}
		
		if (body.first) {
			if (!buf.append(body.first->getContent()))
				return 0;
		}
		else {
			string_ptr strHeader(getHeaderFromBodyStructure((*itP).first));
			if (!buf.append(strHeader.get()))
				return 0;
		}
		
		if (body.second) {
			if (!buf.append(body.second->getContent()))
				return 0;
		}
	}
	while (stack.size() > 0) {
		if (!appendBoundaryToBuffer(stack.back(), &buf, true))
			return 0;
		stack.pop_back();
	}
	
	return buf.getXString();
}

void qmimap4::Util::getFetchArgFromPartList(const PartList& listPart,
											FetchArg arg,
											bool bPeek,
											bool bFetchAllMime,
											string_ptr* pstrArg,
											unsigned int* pnCount,
											bool* pbAll)
{
	assert(!listPart.empty());
	assert(pstrArg);
	assert(pnCount);
	assert(pbAll);
	
	pstrArg->reset(0);
	*pnCount = 0;
	*pbAll = false;
	
	const CHAR* pszBody = bPeek ? "BODY.PEEK" : "BODY";
	
	unsigned int nPartCount = 1;
	
	StringBuffer<STRING> buf("(");
	buf.append(pszBody);
	if (listPart.size() == 1 &&
		Util::isInlineTextPart(listPart.front().first)) {
		buf.append("[]");
		*pbAll = true;
	}
	else {
		buf.append("[HEADER]");
		
		for (PartList::const_iterator it = listPart.begin(); it != listPart.end(); ++it) {
			bool bFetchBody = arg == FETCHARG_TEXT ?
				Util::isInlineTextPart((*it).first) :
				Util::isInlineHtmlPart((*it).first);
			if (bFetchBody || (bFetchAllMime && it != listPart.begin())) {
				string_ptr strPath(Util::formatPath((*it).second));
				
				buf.append(" ");
				if (bFetchBody) {
					buf.append(pszBody);
					buf.append("[");
					buf.append(strPath.get());
					buf.append("] ");
					++nPartCount;
				}
				buf.append(pszBody);
				buf.append("[");
				buf.append(strPath.get());
				buf.append(".MIME]");
				++nPartCount;
			}
		}
	}
	buf.append(")");
	
	*pstrArg = buf.getString();
	*pnCount = nPartCount;
}

bool qmimap4::Util::isInlineTextPart(const FetchDataBodyStructure* pBodyStructure)
{
	const CHAR* pszDisposition = pBodyStructure->getDisposition();
	const CHAR* pszContentType = pBodyStructure->getContentType();
	const CHAR* pszSubType = pBodyStructure->getContentSubType();
	if (!pszContentType || _stricmp(pszContentType, "text") == 0)
		return !pszDisposition || _stricmp(pszDisposition, "inline") == 0;
	else if (_stricmp(pszContentType, "message") == 0 &&
		_stricmp(pszSubType, "rfc822") == 0)
		return true;
	else
		return false;
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

string_ptr qmimap4::Util::formatPath(const unsigned int* pPath)
{
	assert(pPath);
	
	StringBuffer<STRING> buf;
	
	if (*pPath) {
		CHAR sz[32];
		const unsigned int* p = pPath;
		while (*p) {
			sprintf(sz, "%u.", *p);
			buf.append(sz);
			++p;
		}
		size_t n = buf.getLength();
		string_ptr strPath = buf.getString();
		*(strPath.get() + n - 1) = '\0';
		return strPath;
	}
	else {
		return buf.getString();
	}
}

void qmimap4::Util::createUidList(const MessageHolderList& l,
								  UidList* pList)
{
	assert(std::find_if(l.begin(), l.end(),
		std::bind2nd(
			std::mem_fun(&MessageHolder::isFlag),
			MessageHolder::FLAG_LOCAL)) == l.end());
	
	pList->resize(l.size());
	std::transform(l.begin(), l.end(), pList->begin(),
		std::mem_fun(&MessageHolder::getId));
	std::sort(pList->begin(), pList->end());
}

std::auto_ptr<MultipleRange> qmimap4::Util::createRange(const MessageHolderList& l)
{
	UidList listUid;
	createUidList(l, &listUid);
	return std::auto_ptr<MultipleRange>(new MultipleRange(
		&listUid[0], listUid.size(), true));
}

bool qmimap4::Util::isEqualFolderName(const WCHAR* pwszLhs,
									  const WCHAR* pwszRhs,
									  WCHAR cSeparator)
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

Imap4::Secure qmimap4::Util::getSecure(SubAccount* pSubAccount)
{
	assert(pSubAccount);
	
	SubAccount::Secure secure = pSubAccount->getSecure(Account::HOST_RECEIVE);
	switch (secure) {
	case SubAccount::SECURE_SSL:
		return Imap4::SECURE_SSL;
	case SubAccount::SECURE_STARTTLS:
		return Imap4::SECURE_STARTTLS;
	default:
		return Imap4::SECURE_NONE;
	}
}

std::pair<FetchDataBody*, FetchDataBody*> qmimap4::Util::getBodyFromBodyList(const BodyList& listBody,
																			 const unsigned int* pPath)
{
	std::pair<FetchDataBody*, FetchDataBody*> body(0, 0);
	
	for (BodyList::const_iterator it = listBody.begin(); it != listBody.end() && (!body.first || !body.second); ++it) {
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
	}
	
	return body;
}

const CHAR* qmimap4::Util::getBoundaryFromBodyStructure(const FetchDataBodyStructure* pBodyStructure)
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

bool qmimap4::Util::appendBoundaryToBuffer(const FetchDataBodyStructure* pBodyStructure,
										   XStringBuffer<XSTRING>* pBuf,
										   bool bEnd)
{
	// TODO
	// If the result of BODYSTRUCTURE is not trusted
	// (OPTION_TRUSTBODYSTRUCTURE is not set),
	// I need to get the boundary from
	// the result of BODY[] instead of BODYSTRUCTURE
	
	assert(pBodyStructure);
	assert(pBuf);
	
	if (!pBuf->append("\r\n--"))
		return false;
	if (!pBuf->append(getBoundaryFromBodyStructure(pBodyStructure)))
		return false;
	if (bEnd) {
		if (!pBuf->append("--"))
			return false;
	}
	if (!pBuf->append("\r\n"))
		return false;
	
	return true;
}

void qmimap4::Util::appendDataToBuffer(const CHAR* pszName,
									   const CHAR** ppsz,
									   size_t nLen,
									   const FetchDataBodyStructure::ParamList* pListParam,
									   StringBuffer<STRING>* pBuf)
{
	assert(pszName);
	assert(ppsz);
	assert(pBuf);
	
	if (*ppsz && **ppsz) {
		pBuf->append(pszName);
		pBuf->append(": ");
		for (size_t n = 0; n < nLen; ++n)
			pBuf->append(*(ppsz + n));
		if (pListParam)
			appendParamsToBuffer(*pListParam, pBuf);
		pBuf->append("\r\n");
	}
}

void qmimap4::Util::appendParamsToBuffer(const FetchDataBodyStructure::ParamList& l,
										 StringBuffer<STRING>* pBuf)
{
	assert(pBuf);
	
	for (FetchDataBodyStructure::ParamList::const_iterator it = l.begin(); it != l.end(); ++it) {
		pBuf->append(";\r\n\t");
		pBuf->append((*it).first);
		pBuf->append("=");
		string_ptr strValue(FieldParserUtil<STRING>::getAtomOrQString((*it).second, -1));
		pBuf->append(strValue.get());
	}
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

qmimap4::PathEqual::PathEqual(const unsigned int* pPath,
							  size_t nLen) :
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
	delete[] path;
	return 0;
}


/****************************************************************************
 *
 * AbstractCallback
 *
 */

qmimap4::AbstractCallback::AbstractCallback(SubAccount* pSubAccount,
											const Security* pSecurity) :
	DefaultSSLSocketCallback(pSubAccount, Account::HOST_RECEIVE, pSecurity),
	pSubAccount_(pSubAccount)
{
}

qmimap4::AbstractCallback::~AbstractCallback()
{
}

bool qmimap4::AbstractCallback::getUserInfo(wstring_ptr* pwstrUserName,
											wstring_ptr* pwstrPassword)
{
	assert(pwstrUserName);
	assert(pwstrPassword);
	
	*pwstrUserName = allocWString(pSubAccount_->getUserName(Account::HOST_RECEIVE));
	*pwstrPassword = allocWString(pSubAccount_->getPassword(Account::HOST_RECEIVE));
	
	return true;
}

void qmimap4::AbstractCallback::setPassword(const WCHAR* pwszPassword)
{
	// TODO
}

wstring_ptr qmimap4::AbstractCallback::getAuthMethods()
{
	return pSubAccount_->getProperty(L"Imap4", L"AuthMethods", L"");
}
