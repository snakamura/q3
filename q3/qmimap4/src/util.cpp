/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#pragma warning(disable:4786)

#include <qmfolder.h>
#include <qmmessageholder.h>

#include <qsencoder.h>
#include <qsstl.h>

#include <algorithm>
#include <cstdio>

#include <boost/bind.hpp>

#include "imap4error.h"
#include "main.h"
#include "resourceinc.h"
#include "util.h"

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
		wstring_ptr wstrRootFolder(pAccount->getPropertyString(L"Imap4", L"RootFolder"));
		wstring_ptr wstrRootFolderSeparator(pAccount->getPropertyString(L"Imap4", L"RootFolderSeparator"));
		if (*wstrRootFolder.get()) {
			ConcatW c[] = {
				{ wstrRootFolder.get(),				-1	},
				{ wstrRootFolderSeparator.get(),	1	},
				{ wstrName.get(),					-1	}
			};
			wstrName = concat(c, countof(c));
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

wstring_ptr qmimap4::Util::getLabelFromImap4Flags(const FetchDataFlags::FlagList& listCustomFlag)
{
	for (FetchDataFlags::FlagList::const_iterator it = listCustomFlag.begin(); it != listCustomFlag.end(); ++it) {
		const CHAR* pszFlag = *it;
		if (strncmp(pszFlag, "q3label=", 8) == 0) {
			const CHAR* pszLabel = pszFlag + 8;
			malloc_size_ptr<unsigned char> p(QuotedPrintableEncoder(true).decode(
				reinterpret_cast<const unsigned char*>(pszLabel), strlen(pszLabel)));
			if (p.get()) {
				size_t nLen = p.size();
				wxstring_size_ptr wstrLabel(UTF7Converter(true).decode(
					reinterpret_cast<const CHAR*>(p.get()), &nLen));
				if (wstrLabel.get()) {
					for (WCHAR* p = wstrLabel.get(); *p; ++p) {
						if (*p == L'\n' || *p == '\r')
							*p = L' ';
					}
					return allocWString(wstrLabel.get(), wstrLabel.size());
				}
			}
		}
	}
	return 0;
}

std::auto_ptr<Flags> qmimap4::Util::getImap4FlagsFromLabels(unsigned int nFlags,
															const WCHAR** ppwszLabel,
															size_t nCount)
{
	typedef std::vector<STRING> FlagList;
	FlagList listFlag;
	CONTAINER_DELETER(free, listFlag, &freeString);
	listFlag.reserve(nCount);
	for (size_t n = 0; n < nCount; ++n) {
		const WCHAR* pwszLabel = ppwszLabel[n];
		size_t nLen = wcslen(pwszLabel);
		xstring_size_ptr strLabel(UTF7Converter(true).encode(pwszLabel, &nLen));
		if (!strLabel.get())
			return std::auto_ptr<Flags>(0);
		
		malloc_size_ptr<unsigned char> p(QuotedPrintableEncoder(true).encode(
			reinterpret_cast<const unsigned char*>(strLabel.get()), strLabel.size()));
		if (!p.get())
			return std::auto_ptr<Flags>(0);
		
		Concat c[] = {
			{ "q3label=",								8			},
			{ reinterpret_cast<const CHAR*>(p.get()),	p.size()	}
		};
		string_ptr strFlag(concat(c, countof(c)));
		listFlag.push_back(strFlag.release());
	}
	return std::auto_ptr<Flags>(new Flags(getImap4FlagsFromMessageFlags(nFlags),
		const_cast<const CHAR**>(&listFlag[0]), listFlag.size()));
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
						// Because of BUG of courier-imap,
						// I skip invalid group terminater.
						if (pszMailbox) {
							if (bSeparator)
								buf.append(",\r\n\t");
							buf.append(pszMailbox);
							buf.append(": ");
							bSeparator = false;
							bGroup = true;
						}
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

string_ptr qmimap4::Util::getHeaderFromBodyStructure(const FetchDataBodyStructure* pBodyStructure,
													 bool bIncludeContentTypeAndDisposition)
{
	assert(pBodyStructure);
	
	typedef FetchDataBodyStructure::ParamList ParamList;
	
	StringBuffer<STRING> buf;
	
	if (bIncludeContentTypeAndDisposition) {
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
	}
	
	const CHAR* pszEncoding = pBodyStructure->getEncoding();
	appendDataToBuffer("Content-Transfer-Encoding", &pszEncoding, 1, 0, &buf);
	
	const CHAR* pszId = pBodyStructure->getId();
	appendDataToBuffer("Content-Id", &pszId, 1, 0, &buf);
	
	return buf.getString();
}

unsigned int qmimap4::Util::getTextSizeFromBodyStructure(const FetchDataBodyStructure* pBodyStructure)
{
	assert(pBodyStructure);
	
	unsigned int nSize = 0;
	
	if (isMultipart(pBodyStructure)) {
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
	
	if (isMultipart(pBodyStructure)) {
		const FetchDataBodyStructure::ChildList& l = pBodyStructure->getChildList();
		for (FetchDataBodyStructure::ChildList::size_type n = 0; n < l.size(); ++n) {
			auto_ptr_array<unsigned int> pPath(new unsigned int[nBaseLen + 2]);
			std::copy(pBasePath, pBasePath + nBaseLen, pPath.get());
			*(pPath.get() + nBaseLen) = static_cast<unsigned int>(n + 1);
			*(pPath.get() + nBaseLen + 1) = 0;
			
			if (isMultipart(l[n])) {
				getPartsFromBodyStructure(l[n], pPath.get(), pListPart);
			}
			else {
				pListPart->push_back(std::make_pair(l[n], pPath.get()));
				pPath.release();
			}
		}
	}
}

xstring_size_ptr qmimap4::Util::getContentFromBodyStructureAndBodies(const PartList& listPart,
																	 const BodyList& listBody,
																	 bool bTrustBodyStructure)
{
	BodyList::const_iterator itH = std::find_if(
		listBody.begin(), listBody.end(),
		boost::bind(&FetchDataBody::PartPath::empty,
			boost::bind(&FetchDataBody::getPartPath, _1)));
	if (itH == listBody.end())
		return xstring_size_ptr();
	
	XStringBuffer<XSTRING> buf;
	
	std::pair<const CHAR*, size_t> header((*itH)->getContent().get());
	if (!buf.append(header.first, header.second))
		return xstring_size_ptr();
	
	typedef std::vector<std::pair<const FetchDataBodyStructure*, const FetchDataBody*> > Stack;
	Stack stack;
	stack.push_back(std::make_pair(bTrustBodyStructure ? listPart.front().first : 0, *itH));
	
	for (PartList::const_iterator itP = listPart.begin() + 1; itP != listPart.end(); ++itP) {
		std::pair<FetchDataBody*, FetchDataBody*> body =
			getBodyFromBodyList(listBody, (*itP).second);
		size_t nPathLen = getPathLength((*itP).second);
		assert(nPathLen != 0);
		if (nPathLen >= stack.size()) {
			assert(nPathLen == stack.size());
			if (!appendBoundaryToBuffer(stack.back().first, stack.back().second, &buf, false))
				return xstring_size_ptr();
		}
		else {
			stack.pop_back();
			while (nPathLen < stack.size()) {
				if (!appendBoundaryToBuffer(stack.back().first, stack.back().second, &buf, true))
					return xstring_size_ptr();
				stack.pop_back();
			}
			if (!appendBoundaryToBuffer(stack.back().first, stack.back().second, &buf, false))
				return xstring_size_ptr();
		}
		stack.push_back(std::make_pair(bTrustBodyStructure ? (*itP).first : 0, body.first));
		
		if (body.first) {
			std::pair<const CHAR*, size_t> header(body.first->getContent().get());
			if (!buf.append(header.first, header.second))
				return xstring_size_ptr();
		}
		else {
			string_ptr strHeader(getHeaderFromBodyStructure((*itP).first, true));
			if (!buf.append(strHeader.get()))
				return xstring_size_ptr();
		}
		
		if (body.second) {
			std::pair<const CHAR*, size_t> content(body.second->getContent().get());
			if (!buf.append(content.first, content.second))
				return xstring_size_ptr();
		}
	}
	stack.pop_back();
	while (stack.size() > 0) {
		if (!appendBoundaryToBuffer(stack.back().first, stack.back().second, &buf, true))
			return xstring_size_ptr();
		stack.pop_back();
	}
	
	return buf.getXStringSize();
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
			const FetchDataBodyStructure* pBodyStructure = (*it).first;
			bool bFetchBody = arg == FETCHARG_TEXT ?
				isInlineTextPart(pBodyStructure) :
				isInlineHtmlPart(pBodyStructure);
			bool bFetchMime = it != listPart.begin() && (bFetchAllMime ||
				(isMultipart(pBodyStructure) && !getBoundaryFromBodyStructure(pBodyStructure)));
			if (bFetchBody || bFetchMime) {
				string_ptr strPath(formatPath((*it).second));
				
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

bool qmimap4::Util::isMultipart(const FetchDataBodyStructure* pBodyStructure)
{
	return _stricmp(pBodyStructure->getContentType(), "multipart") == 0;
}

bool qmimap4::Util::hasAttachmentPart(const FetchDataBodyStructure* pBodyStructure)
{
	if (isMultipart(pBodyStructure)) {
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

void qmimap4::Util::reportError(Imap4* pImap4,
								ErrorCallback* pCallback,
								Account* pAccount,
								SubAccount* pSubAccount,
								NormalFolder* pFolder,
								unsigned int nImap4Error,
								const WCHAR* pwszSocketErrorMessage,
								const WCHAR* pwszSSLErrorMessage)
{
	assert(pCallback);
	
	struct
	{
		unsigned int nError_;
		UINT nId_;
	} maps[][23] = {
		{
			{ IMAP4ERROR_SAVE,			IDS_ERROR_SAVE			},
			{ IMAP4ERROR_APPLYRULES,	IDS_ERROR_APPLYRULES	},
			{ IMAP4ERROR_MANAGEJUNK,	IDS_ERROR_MANAGEJUNK	},
			{ IMAP4ERROR_FILTERJUNK,	IDS_ERROR_FILTERJUNK	}
		},
		{
			{ Imap4::IMAP4_ERROR_GREETING,		IDS_ERROR_GREETING		},
			{ Imap4::IMAP4_ERROR_LOGIN,			IDS_ERROR_LOGIN			},
			{ Imap4::IMAP4_ERROR_CAPABILITY,	IDS_ERROR_CAPABILITY	},
			{ Imap4::IMAP4_ERROR_FETCH,			IDS_ERROR_FETCH			},
			{ Imap4::IMAP4_ERROR_STORE,			IDS_ERROR_STORE			},
			{ Imap4::IMAP4_ERROR_SELECT,		IDS_ERROR_SELECT		},
			{ Imap4::IMAP4_ERROR_LSUB,			IDS_ERROR_LSUB			},
			{ Imap4::IMAP4_ERROR_LIST,			IDS_ERROR_LIST			},
			{ Imap4::IMAP4_ERROR_COPY,			IDS_ERROR_COPY			},
			{ Imap4::IMAP4_ERROR_APPEND,		IDS_ERROR_APPEND		},
			{ Imap4::IMAP4_ERROR_NOOP,			IDS_ERROR_NOOP			},
			{ Imap4::IMAP4_ERROR_CREATE,		IDS_ERROR_CREATE		},
			{ Imap4::IMAP4_ERROR_DELETE,		IDS_ERROR_DELETE		},
			{ Imap4::IMAP4_ERROR_RENAME,		IDS_ERROR_RENAME		},
			{ Imap4::IMAP4_ERROR_SUBSCRIBE,		IDS_ERROR_SUBSCRIBE		},
			{ Imap4::IMAP4_ERROR_UNSUBSCRIBE,	IDS_ERROR_UNSUBSCRIBE	},
			{ Imap4::IMAP4_ERROR_CLOSE,			IDS_ERROR_CLOSE			},
			{ Imap4::IMAP4_ERROR_EXPUNGE,		IDS_ERROR_EXPUNGE		},
			{ Imap4::IMAP4_ERROR_AUTHENTICATE,	IDS_ERROR_AUTHENTICATE	},
			{ Imap4::IMAP4_ERROR_SEARCH,		IDS_ERROR_SEARCH		},
			{ Imap4::IMAP4_ERROR_NAMESPACE,		IDS_ERROR_NAMESPACE		},
			{ Imap4::IMAP4_ERROR_LOGOUT,		IDS_ERROR_LOGOUT		},
			{ Imap4::IMAP4_ERROR_STARTTLS,		IDS_ERROR_STARTTLS		}
		},
		{
			{ Imap4::IMAP4_ERROR_INITIALIZE,	IDS_ERROR_INITIALIZE	},
			{ Imap4::IMAP4_ERROR_CONNECT,		IDS_ERROR_CONNECT		},
			{ Imap4::IMAP4_ERROR_SELECTSOCKET,	IDS_ERROR_SELECTSOCKET	},
			{ Imap4::IMAP4_ERROR_TIMEOUT,		IDS_ERROR_TIMEOUT		},
			{ Imap4::IMAP4_ERROR_DISCONNECT,	IDS_ERROR_DISCONNECT	},
			{ Imap4::IMAP4_ERROR_RECEIVE,		IDS_ERROR_RECEIVE		},
			{ Imap4::IMAP4_ERROR_PARSE,			IDS_ERROR_PARSE			},
			{ Imap4::IMAP4_ERROR_OTHER,			IDS_ERROR_OTHER			},
			{ Imap4::IMAP4_ERROR_INVALIDSOCKET,	IDS_ERROR_INVALIDSOCKET	},
			{ Imap4::IMAP4_ERROR_SEND,			IDS_ERROR_SEND			},
			{ Imap4::IMAP4_ERROR_RESPONSE,		IDS_ERROR_RESPONSE		},
			{ Imap4::IMAP4_ERROR_SSL,			IDS_ERROR_SSL			}
		}
	};
	
	unsigned int nError = (pImap4 ? pImap4->getLastError() : 0) | nImap4Error;
	unsigned int nMasks[] = {
		IMAP4ERROR_MASK,
		Imap4::IMAP4_ERROR_MASK_HIGHLEVEL,
		Imap4::IMAP4_ERROR_MASK_LOWLEVEL
	};
	wstring_ptr wstrDescriptions[countof(maps)];
	for (int n = 0; n < countof(maps); ++n) {
		for (int m = 0; m < countof(maps[n]) && !wstrDescriptions[n].get(); ++m) {
			if (maps[n][m].nError_ != 0 && (nError & nMasks[n]) == maps[n][m].nError_)
				wstrDescriptions[n] = loadString(getResourceHandle(), maps[n][m].nId_);
		}
	}
	
	wstring_ptr wstrMessage(loadString(getResourceHandle(), IDS_ERROR_MESSAGE));
	wstring_ptr wstrSocketDescription(SocketBase::getErrorDescription(
		static_cast<SocketBase::Error>(nError & SocketBase::SOCKET_ERROR_MASK_SOCKET)));
	
	const WCHAR* pwszDescription[] = {
		wstrDescriptions[0].get(),
		wstrDescriptions[1].get(),
		wstrDescriptions[2].get(),
		wstrSocketDescription.get(),
		pwszSocketErrorMessage,
		pwszSSLErrorMessage,
		pImap4 ? pImap4->getLastErrorResponse() : 0
	};
	SessionErrorInfo info(pAccount, pSubAccount, pFolder, wstrMessage.get(),
		nError, pwszDescription, countof(pwszDescription));
	pCallback->addError(info);
}

Imap4::Secure qmimap4::Util::getSecure(SubAccount* pSubAccount)
{
	assert(pSubAccount);
	return getSecure(pSubAccount->getSecure(Account::HOST_RECEIVE));
}

Imap4::Secure qmimap4::Util::getSecure(SubAccount::Secure secure)
{
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

string_ptr qmimap4::Util::getBoundaryFromBodyStructureOrMime(const FetchDataBodyStructure* pBodyStructure,
															 const FetchDataBody* pMime)
{
	assert(pBodyStructure || pMime);
	
	if (pBodyStructure) {
		const CHAR* pszBoundary = getBoundaryFromBodyStructure(pBodyStructure);
		if (pszBoundary)
			return allocString(pszBoundary);
	}
	if (pMime) {
		std::pair<const CHAR*, size_t> mime(pMime->getContent().get());
		Part header;
		if (header.create(0, mime.first, mime.second)) {
			const ContentTypeParser* pContentType = header.getContentType();
			if (pContentType) {
				wstring_ptr wstrBoundary(pContentType->getParameter(L"boundary"));
				if (wstrBoundary.get())
					return wcs2mbs(wstrBoundary.get());
			}
		}
	}
	return 0;
}

const CHAR* qmimap4::Util::getBoundaryFromBodyStructure(const FetchDataBodyStructure* pBodyStructure)
{
	assert(pBodyStructure);
	
	typedef FetchDataBodyStructure::ParamList ParamList;
	const ParamList& l = pBodyStructure->getContentParams();
	ParamList::const_iterator it = std::find_if(l.begin(), l.end(),
		boost::bind(string_equal_i<CHAR>(),
			boost::bind(&ParamList::value_type::first, _1), "boundary"));
	return it != l.end() ? (*it).second : 0;
}

bool qmimap4::Util::appendBoundaryToBuffer(const FetchDataBodyStructure* pBodyStructure,
										   const FetchDataBody* pMime,
										   XStringBuffer<XSTRING>* pBuf,
										   bool bEnd)
{
	assert(pBodyStructure || pMime);
	assert(pBuf);
	
	string_ptr strBoundary(getBoundaryFromBodyStructureOrMime(pBodyStructure, pMime));
	
	if (!pBuf->append("\r\n--"))
		return false;
	if (strBoundary.get()) {
		if (!pBuf->append(strBoundary.get()))
			return false;
	}
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
		boost::bind(PathFree(),
			boost::bind(&Util::PartList::value_type::second, _1)));
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
											PasswordCallback* pPasswordCallback,
											const Security* pSecurity) :
	DefaultSSLSocketCallback(pSubAccount, Account::HOST_RECEIVE, pSecurity),
	pSubAccount_(pSubAccount),
	pPasswordCallback_(pPasswordCallback),
	state_(PASSWORDSTATE_ONETIME)
{
}

qmimap4::AbstractCallback::~AbstractCallback()
{
}


bool qmimap4::AbstractCallback::isCanceled(bool bForce) const
{
	return false;
}

void qmimap4::AbstractCallback::initialize()
{
}

void qmimap4::AbstractCallback::lookup()
{
}

void qmimap4::AbstractCallback::connecting()
{
}

void qmimap4::AbstractCallback::connected()
{
}

bool qmimap4::AbstractCallback::getUserInfo(wstring_ptr* pwstrUserName,
											wstring_ptr* pwstrPassword)
{
	assert(pwstrUserName);
	assert(pwstrPassword);
	
	*pwstrUserName = allocWString(pSubAccount_->getUserName(Account::HOST_RECEIVE));
	state_ = pPasswordCallback_->getPassword(pSubAccount_, Account::HOST_RECEIVE, pwstrPassword);
	return state_ != PASSWORDSTATE_NONE;
}

void qmimap4::AbstractCallback::setPassword(const WCHAR* pwszPassword)
{
	if (state_ == PASSWORDSTATE_SESSION || state_ == PASSWORDSTATE_SAVE)
		pPasswordCallback_->setPassword(pSubAccount_, Account::HOST_RECEIVE,
			pwszPassword, state_ == PASSWORDSTATE_SAVE);
}

wstring_ptr qmimap4::AbstractCallback::getAuthMethods()
{
	return pSubAccount_->getPropertyString(L"Imap4", L"AuthMethods");
}

void qmimap4::AbstractCallback::authenticating()
{
}

void qmimap4::AbstractCallback::setRange(size_t nMin,
										 size_t nMax)
{
}

void qmimap4::AbstractCallback::setPos(size_t nPos)
{
}
