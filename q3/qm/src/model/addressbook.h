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
#ifdef _WIN32_WCE
#	include <qswindow.h>
#endif

#include <vector>

#ifndef _WIN32_WCE
#	include <wab.h>
#endif

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
	enum Type {
		TYPE_ADDRESSBOOK	= 0x01,
		TYPE_WAB			= 0x02,
		TYPE_BOTH			= 0x03
	};

public:
	typedef std::vector<AddressBookEntry*> EntryList;
	typedef std::vector<qs::WSTRING> CategoryList;
	typedef std::vector<std::pair<const WCHAR*, AddressBookEntry*> > EntryMap;

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
	qs::QSTATUS getEntry(const WCHAR* pwszAddress,
		const AddressBookEntry** ppEntry);
	qs::SMIMECallback* getSMIMECallback() const;

public:
	qs::QSTATUS addEntry(AddressBookEntry* pEntry);

private:
	qs::QSTATUS initWAB();
	qs::QSTATUS load();
	qs::QSTATUS loadWAB();
	void clear(unsigned int nType);
	qs::QSTATUS prepareEntryMap();

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
#ifndef _WIN32_WCE
	class IMAPIAdviseSinkImpl : public IMAPIAdviseSink
	{
	public:
		IMAPIAdviseSinkImpl(AddressBook* pAddressBook, qs::QSTATUS* pstatus);
		~IMAPIAdviseSinkImpl();
	
	public:
		STDMETHOD(QueryInterface)(REFIID riid, void** ppv);
		STDMETHOD_(ULONG, AddRef)();
		STDMETHOD_(ULONG, Release)();
	
	public:
		STDMETHOD_(ULONG, OnNotify)(ULONG cNotif, LPNOTIFICATION lpNotifications);
	
	private:
		IMAPIAdviseSinkImpl(const IMAPIAdviseSinkImpl&);
		IMAPIAdviseSinkImpl& operator=(const IMAPIAdviseSinkImpl&);
	
	private:
		ULONG nRef_;
		AddressBook* pAddressBook_;
	};
	friend class IMAPIAdviseSinkImpl;
#else
	class NotificationWindow :
		public qs::WindowBase,
		public qs::DefaultWindowHandler
	{
	public:
		NotificationWindow(AddressBook* pAddressBook, qs::QSTATUS* pstatus);
		virtual ~NotificationWindow();
	
	public:
		virtual LRESULT windowProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
	
	protected:
		LRESULT onDBNotification(WPARAM wParam, LPARAM lParam);
	
	private:
		NotificationWindow(const NotificationWindow&);
		NotificationWindow& operator=(const NotificationWindow&);
	
	private:
		AddressBook* pAddressBook_;
	};
	friend class NotificationWindow;
#endif

private:
	const Security* pSecurity_;
	FILETIME ft_;
	EntryList listEntry_;
	CategoryList listCategory_;
	SMIMECallbackImpl* pSMIMECallback_;
	bool bContactChanged_;
#ifndef _WIN32_WCE
	HINSTANCE hInstWAB_;
	IAddrBook* pAddrBook_;
	IWABObject* pWABObject_;
	ULONG nConnection_;
#else
	HANDLE hCategoryDB_;
	HANDLE hContactsDB_;
	NotificationWindow* pNotificationWindow_;
#endif
	EntryMap mapEntry_;
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
	AddressBookEntry(bool bWAB, qs::QSTATUS* pstatus);
	~AddressBookEntry();

public:
	bool isWAB() const;
	const WCHAR* getName() const;
	const WCHAR* getSortKey() const;
	const AddressList& getAddresses() const;

public:
	void setName(qs::WSTRING wstrName);
	void setSortKey(qs::WSTRING wstrSortKey);
	qs::QSTATUS addAddress(AddressBookAddress* pAddress);

private:
	AddressBookEntry(const AddressBookEntry&);
	AddressBookEntry& operator=(const AddressBookEntry&);

private:
	bool bWAB_;
	qs::WSTRING wstrName_;
	qs::WSTRING wstrSortKey_;
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
	bool isRFC2822() const;
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
		STATE_SORTKEY,
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
