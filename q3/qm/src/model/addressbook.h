/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __ADDRESSBOOK_H__
#define __ADDRESSBOOK_H__

#include <qm.h>

#include <qs.h>
#include <qscrypto.h>
#include <qsprofile.h>
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
class AddressBookCategory;
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
	typedef std::vector<AddressBookCategory*> CategoryList;
	typedef std::vector<std::pair<const WCHAR*, AddressBookEntry*> > EntryMap;

public:
	AddressBook(qs::Profile* pProfile);
	~AddressBook();

public:
	const EntryList& getEntries();
	void getCategories(CategoryList* pList);
	const AddressBookAddress* getAddress(const WCHAR* pwszAlias);
	qs::wstring_ptr expandAlias(const WCHAR* pwszAddresses);
	const AddressBookEntry* getEntry(const WCHAR* pwszAddress);
	void setEnableReload(bool bEnable);

public:
	void addEntry(std::auto_ptr<AddressBookEntry> pEntry);
	const AddressBookCategory* getCategory(const WCHAR* pwszCategory);

private:
	bool initWAB();
	bool load();
	bool loadWAB();
	void clear(unsigned int nType);
	void prepareEntryMap();

private:
	AddressBook(const AddressBook&);
	AddressBook& operator=(const AddressBook&);

private:
#ifndef _WIN32_WCE
	class IMAPIAdviseSinkImpl : public IMAPIAdviseSink
	{
	public:
		explicit IMAPIAdviseSinkImpl(AddressBook* pAddressBook);
		~IMAPIAdviseSinkImpl();
	
	public:
		STDMETHOD(QueryInterface)(REFIID riid, void** ppv);
		STDMETHOD_(ULONG, AddRef)();
		STDMETHOD_(ULONG, Release)();
	
	public:
		STDMETHOD_(ULONG, OnNotify)(ULONG cNotif,
									LPNOTIFICATION lpNotifications);
	
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
		explicit NotificationWindow(AddressBook* pAddressBook);
		virtual ~NotificationWindow();
	
	public:
		virtual LRESULT windowProc(UINT uMsg,
								   WPARAM wParam,
								   LPARAM lParam);
	
	protected:
		LRESULT onDBNotification(WPARAM wParam,
								 LPARAM lParam);
	
	private:
		NotificationWindow(const NotificationWindow&);
		NotificationWindow& operator=(const NotificationWindow&);
	
	private:
		AddressBook* pAddressBook_;
	};
	friend class NotificationWindow;
#endif

private:
	FILETIME ft_;
	EntryList listEntry_;
	CategoryList listCategory_;
	bool bEnableReload_;
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
	explicit AddressBookEntry(bool bWAB);
	~AddressBookEntry();

public:
	bool isWAB() const;
	const WCHAR* getName() const;
	const WCHAR* getSortKey() const;
	const AddressList& getAddresses() const;

public:
	void setName(qs::wstring_ptr wstrName);
	void setSortKey(qs::wstring_ptr wstrSortKey);
	void addAddress(std::auto_ptr<AddressBookAddress> pAddress);

private:
	AddressBookEntry(const AddressBookEntry&);
	AddressBookEntry& operator=(const AddressBookEntry&);

private:
	bool bWAB_;
	qs::wstring_ptr wstrName_;
	qs::wstring_ptr wstrSortKey_;
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
	typedef std::vector<const AddressBookCategory*> CategoryList;

public:
	AddressBookAddress(const AddressBookEntry* pEntry,
					   const WCHAR* pwszAlias,
					   const CategoryList& listCategory,
					   const WCHAR* pwszComment,
					   const WCHAR* pwszCertificate,
					   bool bRFC2822);
	~AddressBookAddress();

public:
	const AddressBookEntry* getEntry() const;
	const WCHAR* getAddress() const;
	const WCHAR* getAlias() const;
	const CategoryList& getCategories() const;
	const WCHAR* getComment() const;
	const WCHAR* getCertificate() const;
	bool isRFC2822() const;
	qs::wstring_ptr getValue() const;

public:
	void setAddress(qs::wstring_ptr wstrAddress);

private:
	AddressBookAddress(const AddressBookAddress&);
	AddressBookAddress& operator=(const AddressBookAddress&);

private:
	const AddressBookEntry* pEntry_;
	qs::wstring_ptr wstrAddress_;
	qs::wstring_ptr wstrAlias_;
	CategoryList listCategory_;
	qs::wstring_ptr wstrComment_;
	qs::wstring_ptr wstrCertificate_;
	bool bRFC2822_;
};


/****************************************************************************
 *
 * AddressBookCategory
 *
 */

class AddressBookCategory
{
public:
	explicit AddressBookCategory(const WCHAR* pwszName);
	~AddressBookCategory();

public:
	const WCHAR* getName() const;

private:
	AddressBookCategory(const AddressBookCategory&);
	AddressBookCategory& operator=(const AddressBookCategory&);

private:
	qs::wstring_ptr wstrName_;
};


/****************************************************************************
 *
 * AddressBookContentHandler
 *
 */

class AddressBookContentHandler : public qs::DefaultHandler
{
public:
	explicit AddressBookContentHandler(AddressBook* pAddressBook);
	virtual ~AddressBookContentHandler();

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
	qs::StringBuffer<qs::WSTRING> buffer_;
	AddressBookEntry* pEntry_;
	AddressBookAddress* pAddress_;
};

}

#endif // __ADDRESSBOOK_H__
