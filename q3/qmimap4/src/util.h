/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __UTIL_H__
#define __UTIL_H__

#include <qmaccount.h>
#include <qmsecurity.h>

#include <qs.h>
#include <qssocket.h>
#include <qsstring.h>

#include "imap4.h"


namespace qmimap4 {

class Util;
class AbstractCallback;


/****************************************************************************
 *
 * Util
 *
 */

class Util
{
public:
	enum FetchArg {
		FETCHARG_TEXT,
		FETCHARG_HTML
	};

public:
	typedef std::vector<FetchDataBody*> BodyList;
	typedef std::vector<std::pair<const FetchDataBodyStructure*, unsigned int*> > PartList;
	typedef std::vector<unsigned long> UidList;

public:
	class PartListDeleter
	{
	public:
		PartListDeleter(PartList& l);
		~PartListDeleter();
	
	private:
		PartListDeleter(const PartListDeleter&);
		PartListDeleter& operator=(const PartListDeleter&);
	
	private:
		PartList& l_;
	};

public:
	static qs::QSTATUS getFolderName(
		qm::Folder* pFolder, qs::WSTRING* pwstrName);
	static unsigned int getFolderFlagsFromAttributes(unsigned int nAttributes);
	static unsigned int getMessageFlagsFromImap4Flags(unsigned int nSystemFlags,
		const FetchDataFlags::FlagList& listCustomFlag);
	static unsigned int getImap4FlagsFromMessageFlags(unsigned int nFlags);
	static qs::QSTATUS getMessageFromEnvelope(
		const FetchDataEnvelope* pEnvelope, qs::STRING* pstrMessage);
	static qs::QSTATUS getHeaderFromBodyStructure(
		const FetchDataBodyStructure* pBodyStructure, qs::STRING* pstrHeader);
	static unsigned long getTextSizeFromBodyStructure(
		const FetchDataBodyStructure* pBodyStructure);
	static qs::QSTATUS getPartsFromBodyStructure(
		const FetchDataBodyStructure* pBodyStructure,
		const unsigned int* pBasePath, PartList* pListPart);
	static qs::QSTATUS getContentFromBodyStructureAndBodies(
		const PartList& listPart, const BodyList& listBody, qs::STRING* pstrContent);
	static qs::QSTATUS getFetchArgFromPartList(const PartList& listPart,
		FetchArg arg, bool bPeek, bool bFetchAllMime, qs::STRING* pstrArg,
		unsigned int* pnCount, bool* pbAll);
	static bool isInlineTextPart(const FetchDataBodyStructure* pBodyStructure);
	static bool isInlineHtmlPart(const FetchDataBodyStructure* pBodyStructure);
	static bool hasAttachmentPart(const FetchDataBodyStructure* pBodyStructure);
	static size_t getPathLength(const unsigned int* pPath);
	static qs::QSTATUS formatPath(const unsigned int* pPath, qs::STRING* pstrPath);
	static qs::QSTATUS createUidList(
		const qm::Folder::MessageHolderList& l, UidList* pList);
	static qs::QSTATUS createRange(const qm::Folder::MessageHolderList& l,
		std::auto_ptr<MultipleRange>* apRange);
	static bool isEqualFolderName(const WCHAR* pwszLhs,
		const WCHAR* pwszRhs, WCHAR cSeparator);

private:
	static std::pair<FetchDataBody*, FetchDataBody*> getBodyFromBodyList(
		const BodyList& listBody, const unsigned int* pPath);
	static const CHAR* getBoundaryFromBodyStructure(
		const FetchDataBodyStructure* pBodyStructure);
	static qs::QSTATUS appendBoundaryToBuffer(
		const FetchDataBodyStructure* pBodyStructure,
		qs::StringBuffer<qs::STRING>* pBuf, bool bEnd);
	static qs::QSTATUS qmimap4::Util::appendDataToBuffer(
		const CHAR* pszName, const CHAR** ppsz, size_t nLen,
		const FetchDataBodyStructure::ParamList* pListParam,
		qs::StringBuffer<qs::STRING>* pBuf);
	static qs::QSTATUS appendParamsToBuffer(
		const FetchDataBodyStructure::ParamList& l,
		qs::StringBuffer<qs::STRING>* pBuf);
};


/****************************************************************************
 *
 * PathEqual
 *
 */

struct PathEqual : public std::unary_function<unsigned int*, bool>
{
	PathEqual(const unsigned int* pPath, size_t nLen);
	bool operator()(const unsigned int* pPath) const;
	const unsigned int* pPath_;
	size_t nLen_;
};


/****************************************************************************
 *
 * PathFree
 *
 */

struct PathFree :
	public std::unary_function<unsigned int*, void*>
{
	void* operator()(unsigned int* path) const;
};


/****************************************************************************
 *
 * AbstractCallback
 *
 */

class AbstractCallback :
	public qs::SocketCallback,
	public qs::SSLSocketCallback,
	public Imap4Callback
{
public:
	AbstractCallback(qm::SubAccount* pSubAccount,
		const qm::Security* pSecurity, qs::QSTATUS* pstatus);
	virtual ~AbstractCallback();

public:
	virtual qs::QSTATUS getCertStore(const qs::Store** ppStore);
	virtual qs::QSTATUS checkCertificate(
		const qs::Certificate& cert, bool bVerified);

public:
	virtual qs::QSTATUS getUserInfo(qs::WSTRING* pwstrUserName,
		qs::WSTRING* pwstrPassword);
	virtual qs::QSTATUS setPassword(const WCHAR* pwszPassword);
	virtual qs::QSTATUS getAuthMethods(qs::WSTRING* pwstrAuthMethods);

private:
	AbstractCallback(const AbstractCallback&);
	AbstractCallback& operator=(const AbstractCallback&);

private:
	qm::SubAccount* pSubAccount_;
	const qm::Security* pSecurity_;
};

}

#endif // __UTIL_H__
