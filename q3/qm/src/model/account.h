/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __ACCOUNT_H__
#define __ACCOUNT_H__

#include <qmaccount.h>

#include <qssax.h>
#include <qsstream.h>

#include <functional>


namespace qm {

/****************************************************************************
 *
 * AccountNameEqual
 *
 */

struct AccountNameEqual : public std::unary_function<Account*, bool>
{
	AccountNameEqual(const WCHAR* pwszName);
	bool operator()(const Account* pAccount) const;
	const WCHAR* pwszName_;
};


/****************************************************************************
 *
 * AccountEqual
 *
 */

struct AccountEqual : public std::binary_function<Account*, Account*, bool>
{
	bool operator()(const Account* pLhs,
					const Account* pRhs) const;
};


/****************************************************************************
 *
 * AccountLess
 *
 */

struct AccountLess : public std::binary_function<Account*, Account*, bool>
{
	bool operator()(const Account* pLhs,
					const Account* pRhs) const;
};


/****************************************************************************
 *
 * RemoteFolderLess
 *
 */

struct RemoteFolderLess : public std::binary_function<std::pair<Folder*, bool>, std::pair<Folder*, bool>, bool>
{
	bool operator()(const std::pair<Folder*, bool>& lhs,
					const std::pair<Folder*, bool>& rhs) const;
};


/****************************************************************************
 *
 * FolderContentHandler
 *
 */

class FolderContentHandler : public qs::DefaultHandler
{
private:
	enum State {
		STATE_ROOT,
		STATE_FOLDERS,
		STATE_NORMALFOLDER,
		STATE_QUERYFOLDER,
		STATE_ID,
		STATE_PARENT,
		STATE_FLAGS,
		STATE_COUNT,
		STATE_UNSEENCOUNT,
		STATE_SEPARATOR,
		STATE_NAME,
		STATE_VALIDITY,
		STATE_DOWNLOADCOUNT,
		STATE_DELETEDCOUNT,
		STATE_DRIVER,
		STATE_CONDITION,
		STATE_TARGETFOLDER,
		STATE_RECURSIVE,
		STATE_PARAMS,
		STATE_PARAM
	};

public:
	FolderContentHandler(Account* pAccount,
						 Account::FolderList* pList);
	virtual ~FolderContentHandler();

public:
	virtual bool startElement(const WCHAR* pwszNamespaceURI,
							  const WCHAR* pwszLocalName,
							  const WCHAR* pwszQName,
							  const qs::Attributes& attributes);
	virtual bool endElement(const WCHAR* pwszNamespaceURI,
							const WCHAR* pwszLocalName,
							const WCHAR* pwszQName);
	virtual bool characters(const WCHAR* pwsz,
							size_t nStart,
							size_t nLength);

private:
	bool processItemStartElement(unsigned int nAcceptStates,
								 State state,
								 const qs::Attributes& attributes);
	Folder* getFolder(unsigned int nId) const;
	bool getNumber(unsigned int* pn);

private:
	FolderContentHandler(const FolderContentHandler&);
	FolderContentHandler& operator=(const FolderContentHandler&);

private:
	Account* pAccount_;
	Account::FolderList* pList_;
	State state_;
	bool bNormal_;
	unsigned int nItem_;
	unsigned int nId_;
	unsigned int nParentId_;
	unsigned int nFlags_;
	unsigned int nCount_;
	unsigned int nUnseenCount_;
	WCHAR cSeparator_;
	qs::wstring_ptr wstrName_;
	unsigned int nValidity_;
	unsigned int nDownloadCount_;
	unsigned int nDeletedCount_;
	qs::wstring_ptr wstrDriver_;
	qs::wstring_ptr wstrCondition_;
	qs::wstring_ptr wstrTargetFolder_;
	bool bRecursive_;
	qs::wstring_ptr wstrParamName_;
	Folder::ParamList listParam_;
	qs::StringBuffer<qs::WSTRING> buffer_;
};


/****************************************************************************
 *
 * FolderWriter
 *
 */

class FolderWriter
{
public:
	explicit FolderWriter(qs::Writer* pWriter);
	~FolderWriter();

public:
	bool write(const Account::FolderList& l);

private:
	FolderWriter(const FolderWriter&);
	FolderWriter& operator=(const FolderWriter&);

private:
	qs::OutputHandler handler_;
};

}

#include "account.inl"

#endif // __ACCOUNT_H__
