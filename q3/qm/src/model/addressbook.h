/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __ADDRESSBOOK_H__
#define __ADDRESSBOOK_H__

#include <qm.h>

#include <qs.h>
#include <qscrypto.h>
#include <qssax.h>
#include <qsstring.h>

#include <vector>


namespace qm {

class AddressBook;
class AddressBookEntry;
class AddressBookAddress;
class AddressBookContentHandler;

class Security;


/****************************************************************************
 *
 * AddressBook
 *
 */

class AddressBook
{
public:
	typedef std::vector<AddressBookEntry*> EntryList;
	typedef std::vector<qs::WSTRING> CategoryList;

public:
	AddressBook(const Security* pSecurity, qs::QSTATUS* pstatus);
	~AddressBook();

public:
	qs::QSTATUS getEntries(const EntryList** ppList);
	qs::QSTATUS getCategories(const CategoryList** ppList);
	qs::QSTATUS getAddress(const WCHAR* pwszAlias,
		const AddressBookAddress** ppAddress);
	qs::QSTATUS expandAlias(const WCHAR* pwszAddresses,
		qs::WSTRING* pwstrAddresses);
	qs::SMIMECallback* getSMIMECallback() const;

public:
	qs::QSTATUS addEntry(AddressBookEntry* pEntry);

private:
	qs::QSTATUS load();
	void clear();

private:
	AddressBook(const AddressBook&);
	AddressBook& operator=(const AddressBook&);

private:
	class SMIMECallbackImpl : public qs::SMIMECallback
	{
	public:
		SMIMECallbackImpl(AddressBook* pAddressBook, qs::QSTATUS* pstatus);
		~SMIMECallbackImpl();
	
	public:
		virtual qs::QSTATUS getCertificate(const WCHAR* pwszAddress,
			qs::Certificate** ppCertificate);
	
	private:
		SMIMECallbackImpl(const SMIMECallbackImpl&);
		SMIMECallbackImpl& operator=(const SMIMECallbackImpl&);
	
	private:
		AddressBook* pAddressBook_;
	};
	friend class SMIMECallbackImpl;

private:
	const Security* pSecurity_;
	FILETIME ft_;
	EntryList listEntry_;
	CategoryList listCategory_;
	SMIMECallbackImpl* pSMIMECallback_;
};


/****************************************************************************
 *
 * AddressBookEntry
 *
 */

class AddressBookEntry
{
public:
	typedef std::vector<AddressBookAddress*> AddressList;

public:
	explicit AddressBookEntry(qs::QSTATUS* pstatus);
	~AddressBookEntry();

public:
	const WCHAR* getName() const;
	const AddressList& getAddresses() const;

public:
	void setName(qs::WSTRING wstrName);
	qs::QSTATUS addAddress(AddressBookAddress* pAddress);

private:
	AddressBookEntry(const AddressBookEntry&);
	AddressBookEntry& operator=(const AddressBookEntry&);

private:
	qs::WSTRING wstrName_;
	AddressList listAddress_;
};


/****************************************************************************
 *
 * AddressBookAddress
 *
 */

class AddressBookAddress
{
public:
	AddressBookAddress(const AddressBookEntry* pEntry,
		const WCHAR* pwszAlias, const WCHAR* pwszCategory,
		const WCHAR* pwszComment, const WCHAR* pwszCertificate,
		bool bRFC2822, qs::QSTATUS* pstatus);
	~AddressBookAddress();

public:
	const AddressBookEntry* getEntry() const;
	const WCHAR* getAddress() const;
	const WCHAR* getAlias() const;
	const WCHAR* getCategory() const;
	const WCHAR* getComment() const;
	const WCHAR* getCertificate() const;
	qs::QSTATUS getValue(qs::WSTRING* pwstrValue) const;

public:
	void setAddress(qs::WSTRING wstrAddress);

private:
	AddressBookAddress(const AddressBookAddress&);
	AddressBookAddress& operator=(const AddressBookAddress&);

private:
	const AddressBookEntry* pEntry_;
	qs::WSTRING wstrAddress_;
	qs::WSTRING wstrAlias_;
	qs::WSTRING wstrCategory_;
	qs::WSTRING wstrComment_;
	qs::WSTRING wstrCertificate_;
	bool bRFC2822_;
};


/****************************************************************************
 *
 * AddressBookContentHandler
 *
 */

class AddressBookContentHandler : public qs::DefaultHandler
{
public:
	AddressBookContentHandler(AddressBook* pAddressBook, qs::QSTATUS* pstatus);
	virtual ~AddressBookContentHandler();

public:
	virtual qs::QSTATUS startElement(const WCHAR* pwszNamespaceURI,
		const WCHAR* pwszLocalName, const WCHAR* pwszQName,
		const qs::Attributes& attributes);
	virtual qs::QSTATUS endElement(const WCHAR* pwszNamespaceURI,
		const WCHAR* pwszLocalName, const WCHAR* pwszQName);
	virtual qs::QSTATUS characters(const WCHAR* pwsz,
		size_t nStart, size_t nLength);

private:
	enum State {
		STATE_ROOT,
		STATE_ADDRESSBOOK,
		STATE_ENTRY,
		STATE_NAME,
		STATE_ADDRESSES,
		STATE_ADDRESS
	};

private:
	AddressBookContentHandler(const AddressBookContentHandler&);
	AddressBookContentHandler& operator=(const AddressBookContentHandler&);

private:
	AddressBook* pAddressBook_;
	State state_;
	qs::StringBuffer<qs::WSTRING>* pBuffer_;
	AddressBookEntry* pEntry_;
	AddressBookAddress* pAddress_;
};

}

#endif // __ADDRESSBOOK_H__
