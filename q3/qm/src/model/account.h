/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
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
	bool operator()(const Account* pLhs, const Account* pRhs) const;
};


/****************************************************************************
 *
 * AccountLess
 *
 */

struct AccountLess : public std::binary_function<Account*, Account*, bool>
{
	bool operator()(const Account* pLhs, const Account* pRhs) const;
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
		STATE_MACRO
	};

public:
	FolderContentHandler(Account* pAccount,
		Account::FolderList* pList, qs::QSTATUS* pstatus);
	virtual ~FolderContentHandler();

public:
	virtual qs::QSTATUS startElement(const WCHAR* pwszNamespaceURI,
		const WCHAR* pwszLocalName, const WCHAR* pwszQName,
		const qs::Attributes& attributes);
	virtual qs::QSTATUS endElement(const WCHAR* pwszNamespaceURI,
		const WCHAR* pwszLocalName, const WCHAR* pwszQName);
	virtual qs::QSTATUS characters(const WCHAR* pwsz,
		size_t nStart, size_t nLength);

private:
	qs::QSTATUS processItemStartElement(unsigned int nAcceptStates,
		State state, const qs::Attributes& attributes);
	Folder* getFolder(unsigned int nId) const;
	qs::QSTATUS getNumber(unsigned int* pn);

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
	qs::WSTRING wstrName_;
	unsigned int nValidity_;
	unsigned int nDownloadCount_;
	unsigned int nDeletedCount_;
	qs::WSTRING wstrMacro_;
	qs::StringBuffer<qs::WSTRING>* pBuffer_;
};


/****************************************************************************
 *
 * FolderWriter
 *
 */

class FolderWriter
{
public:
	FolderWriter(qs::Writer* pWriter, qs::QSTATUS* pstatus);
	~FolderWriter();

public:
	qs::QSTATUS write(const Account::FolderList& l);

private:
	qs::QSTATUS writeString(const WCHAR* pwszQName,
		const WCHAR* pwsz, size_t nLen);
	qs::QSTATUS writeNumber(const WCHAR* pwszQName, unsigned int n);

private:
	FolderWriter(const FolderWriter&);
	FolderWriter& operator=(const FolderWriter&);

private:
	qs::OutputHandler handler_;
};

}

#include "account.inl"

#endif // __ACCOUNT_H__
