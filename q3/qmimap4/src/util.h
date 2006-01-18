/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __UTIL_H__
#define __UTIL_H__

#include <qmaccount.h>
#include <qmsecurity.h>
#include <qmsession.h>

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
	static qs::wstring_ptr getFolderName(qm::NormalFolder* pFolder);
	static unsigned int getFolderFlagsFromAttributes(unsigned int nAttributes);
	static unsigned int getMessageFlagsFromImap4Flags(unsigned int nSystemFlags,
													  const FetchDataFlags::FlagList& listCustomFlag);
	static unsigned int getImap4FlagsFromMessageFlags(unsigned int nFlags);
	static qs::wstring_ptr getLabelFromImap4Flags(const FetchDataFlags::FlagList& listCustomFlag);
	static std::auto_ptr<Flags> getImap4FlagsFromLabels(unsigned int nFlags,
														const WCHAR** ppwszLabel,
														size_t nCount);
	static qs::string_ptr getMessageFromEnvelope(const FetchDataEnvelope* pEnvelope);
	static qs::string_ptr getHeaderFromBodyStructure(const FetchDataBodyStructure* pBodyStructure,
													 bool bIncludeContentTypeAndDisposition);
	static unsigned long getTextSizeFromBodyStructure(const FetchDataBodyStructure* pBodyStructure);
	static void getPartsFromBodyStructure(const FetchDataBodyStructure* pBodyStructure,
										  const unsigned int* pBasePath,
										  PartList* pListPart);
	static qs::xstring_size_ptr getContentFromBodyStructureAndBodies(const PartList& listPart,
																	 const BodyList& listBody,
																	 bool bTrustBodyStructure);
	static void getFetchArgFromPartList(const PartList& listPart,
										FetchArg arg,
										bool bPeek,
										bool bFetchAllMime,
										qs::string_ptr* pstrArg,
										unsigned int* pnCount,
										bool* pbAll);
	static bool isInlineTextPart(const FetchDataBodyStructure* pBodyStructure);
	static bool isInlineHtmlPart(const FetchDataBodyStructure* pBodyStructure);
	static bool isMultipart(const FetchDataBodyStructure* pBodyStructure);
	static bool hasAttachmentPart(const FetchDataBodyStructure* pBodyStructure);
	static size_t getPathLength(const unsigned int* pPath);
	static qs::string_ptr formatPath(const unsigned int* pPath);
	static void createUidList(const qm::MessageHolderList& l,
							  UidList* pList);
	static std::auto_ptr<MultipleRange> createRange(const qm::MessageHolderList& l);
	static bool isEqualFolderName(const WCHAR* pwszLhs,
								  const WCHAR* pwszRhs,
								  WCHAR cSeparator);
	static Imap4::Secure getSecure(qm::SubAccount* pSubAccount);

private:
	static std::pair<FetchDataBody*, FetchDataBody*> getBodyFromBodyList(const BodyList& listBody,
																		 const unsigned int* pPath);
	static qs::string_ptr getBoundaryFromBodyStructureOrMime(const FetchDataBodyStructure* pBodyStructure,
															 const FetchDataBody* pMime);
	static const CHAR* getBoundaryFromBodyStructure(const FetchDataBodyStructure* pBodyStructure);
	static bool appendBoundaryToBuffer(const FetchDataBodyStructure* pBodyStructure,
									   const FetchDataBody* pMime,
									   qs::XStringBuffer<qs::XSTRING>* pBuf,
									   bool bEnd);
	static void qmimap4::Util::appendDataToBuffer(const CHAR* pszName,
												  const CHAR** ppsz,
												  size_t nLen,
												  const FetchDataBodyStructure::ParamList* pListParam,
												  qs::StringBuffer<qs::STRING>* pBuf);
	static void appendParamsToBuffer(const FetchDataBodyStructure::ParamList& l,
									 qs::StringBuffer<qs::STRING>* pBuf);
};


/****************************************************************************
 *
 * PathEqual
 *
 */

struct PathEqual : public std::unary_function<unsigned int*, bool>
{
	PathEqual(const unsigned int* pPath,
			  size_t nLen);
	bool operator()(const unsigned int* pPath) const;
	const unsigned int* pPath_;
	size_t nLen_;
};


/****************************************************************************
 *
 * PathFree
 *
 */

struct PathFree : public std::unary_function<unsigned int*, void*>
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
	public qm::DefaultSSLSocketCallback,
	public Imap4Callback
{
public:
	AbstractCallback(qm::SubAccount* pSubAccount,
					 qm::PasswordCallback* pPasswordCallback,
					 const qm::Security* pSecurity);
	virtual ~AbstractCallback();

public:
	virtual bool getUserInfo(qs::wstring_ptr* pwstrUserName,
							 qs::wstring_ptr* pwstrPassword);
	virtual void setPassword(const WCHAR* pwszPassword);
	virtual qs::wstring_ptr getAuthMethods();

private:
	AbstractCallback(const AbstractCallback&);
	AbstractCallback& operator=(const AbstractCallback&);

private:
	qm::SubAccount* pSubAccount_;
	qm::PasswordCallback* pPasswordCallback_;
	qm::PasswordState state_;
};

}

#endif // __UTIL_H__
