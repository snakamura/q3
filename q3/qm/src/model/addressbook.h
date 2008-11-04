/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
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
#include <qsthread.h>
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
struct AddressBookCategoryLess;
class AddressBookContentHandler;
class AddressBookWriter;
class ExternalAddressBook;
#ifndef _WIN32_WCE
	class MAPIAddressBook;
		class WindowsAddressBook;
		class OutlookAddressBook;
#else
	class POOMAddressBook;
	class PocketOutlookAddressBook;
#endif
class ExternalAddressBookManager;

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
		TYPE_EXTERNAL		= 0x02,
		TYPE_BOTH			= 0x03
	};

public:
	typedef std::vector<AddressBookEntry*> EntryList;
	typedef std::vector<AddressBookCategory*> CategoryList;
	typedef std::vector<std::pair<qs::WSTRING, AddressBookEntry*> > EntryMap;

public:
	AddressBook(const WCHAR* pwszPath,
				qs::Profile* pProfile,
				bool bLoadExternal);
	~AddressBook();

public:
	const EntryList& getEntries() const;
	const CategoryList& getCategories() const;
	const AddressBookAddress* getAddress(const WCHAR* pwszAlias) const;
	qs::wstring_ptr expandAlias(const WCHAR* pwszAddresses) const;
	const AddressBookEntry* getEntry(const WCHAR* pwszAddress) const;
	bool reload();
	void reloadProfiles();
	bool save() const;
	void lock() const;
	void unlock() const;

public:
	void addEntry(std::auto_ptr<AddressBookEntry> pEntry);
	void removeEntry(const AddressBookEntry* pEntry);
	const AddressBookCategory* getCategory(const WCHAR* pwszCategory);

private:
	void initExternal();
	bool load();
	bool loadExternal();
	void clear(unsigned int nType);
	void prepareEntryMap() const;
	void clearEntryMap();

private:
	AddressBook(const AddressBook&);
	AddressBook& operator=(const AddressBook&);

private:
	qs::wstring_ptr wstrPath_;
	FILETIME ft_;
	qs::Profile* pProfile_;
	EntryList listEntry_;
	CategoryList listCategory_;
	std::auto_ptr<ExternalAddressBookManager> pExternalManager_;
	mutable EntryMap mapEntry_;
	qs::CriticalSection cs_;
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
	AddressBookEntry(const WCHAR* pwszName,
					 const WCHAR* pwszSortKey,
					 bool bExternal);
	AddressBookEntry(const AddressBookEntry& entry);
	~AddressBookEntry();

public:
	AddressBookEntry& operator=(const AddressBookEntry& entry);

public:
	const WCHAR* getName() const;
	void setName(const WCHAR* pwszName);
	const WCHAR* getSortKey() const;
	void setSortKey(const WCHAR* pwszSortKey);
	const WCHAR* getActualSortKey() const;
	const AddressList& getAddresses() const;
	const AddressBookAddress* getAddress(const WCHAR* pwszAddress) const;
	void setAddresses(AddressList& listAddress);
	void addAddress(std::auto_ptr<AddressBookAddress> pAddress);
	bool isExternal() const;

private:
	void clearAddresses();
	void swap(AddressBookEntry& entry);

private:
	qs::wstring_ptr wstrName_;
	qs::wstring_ptr wstrSortKey_;
	AddressList listAddress_;
	bool bExternal_;
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
	AddressBookAddress(const AddressBookEntry* pEntry);
	AddressBookAddress(const AddressBookEntry* pEntry,
					   const WCHAR* pwszAddress,
					   const WCHAR* pwszAlias,
					   const CategoryList& listCategory,
					   const WCHAR* pwszComment,
					   const WCHAR* pwszCertificate,
					   bool bRFC2822);
	AddressBookAddress(const AddressBookAddress& address);
	~AddressBookAddress();

public:
	const AddressBookEntry* getEntry() const;
	const WCHAR* getAddress() const;
	void setAddress(const WCHAR* pwszAddress,
					bool bRFC2822);
	const WCHAR* getAlias() const;
	void setAlias(const WCHAR* pwszAlias);
	const CategoryList& getCategories() const;
	void setCategories(const CategoryList& listCategory);
	qs::wstring_ptr getCategoryNames() const;
	const WCHAR* getComment() const;
	void setComment(const WCHAR* pwszComment);
	const WCHAR* getCertificate() const;
	void setCertificate(const WCHAR* pwszCertificate);
	bool isRFC2822() const;
	qs::wstring_ptr getValue() const;

private:
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
 * AddressBookCategory
 *
 */

struct AddressBookCategoryLess : public std::binary_function<AddressBookCategory*, AddressBookCategory*, bool>
{
	bool operator()(const AddressBookCategory* pLhs,
					const AddressBookCategory* pRhs);
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
	std::auto_ptr<AddressBookEntry> pEntry_;
	std::auto_ptr<AddressBookAddress> pAddress_;
};


/****************************************************************************
 *
 * AddressBookWriter
 *
 */

class AddressBookWriter
{
public:
	AddressBookWriter(qs::Writer* pWriter,
					  const WCHAR* pwszEncoding);
	~AddressBookWriter();

public:
	bool write(const AddressBook* pAddressBook);
	bool write(const AddressBookEntry* pEntry);
	bool write(const AddressBookAddress* pAddress);

private:
	AddressBookWriter(const AddressBookWriter&);
	AddressBookWriter& operator=(const AddressBookWriter&);

private:
	qs::OutputHandler handler_;
};


/****************************************************************************
 *
 * ExternalAddressBook
 *
 */

class ExternalAddressBook
{
public:
	virtual ~ExternalAddressBook();

public:
	virtual bool init(bool bAddressOnly) = 0;
	virtual void term() = 0;
	virtual bool load(AddressBook* pAddressBook) = 0;
	virtual bool isModified() = 0;
};


/****************************************************************************
 *
 * ExternalAddressBookManager
 *
 */

class ExternalAddressBookManager
{
public:
	explicit ExternalAddressBookManager(qs::Profile* pProfile);
	~ExternalAddressBookManager();

public:
	bool load(AddressBook* pAddressBook);
	bool isModified() const;

private:
	bool init(std::auto_ptr<ExternalAddressBook> pAddressBook,
			  bool bAddressOnly);

private:
	ExternalAddressBookManager(const ExternalAddressBookManager&);
	ExternalAddressBookManager& operator=(const ExternalAddressBookManager&);

private:
	typedef std::vector<ExternalAddressBook*> AddressBookList;

private:
	AddressBookList listAddressBook_;
	bool bModified_;
};


#ifndef _WIN32_WCE

/****************************************************************************
 *
 * MAPIAddressBook
 *
 */

class MAPIAddressBook : public ExternalAddressBook
{
protected:
	MAPIAddressBook();
	virtual ~MAPIAddressBook();

public:
	virtual void term();
	virtual bool load(AddressBook* pAddressBook);
	virtual bool isModified();

protected:
	bool init(IAddrBook* pAddrBook,
			  bool bAddressOnly);

protected:
	virtual void freeBuffer(void* pBuffer) const = 0;

private:
	qs::wstring_ptr expandDistList(IMAPIContainer* pContainer,
								   IDistList* pDistList) const;

private:
	class IMAPIAdviseSinkImpl : public IMAPIAdviseSink
	{
	public:
		explicit IMAPIAdviseSinkImpl(MAPIAddressBook* pAddressBook);
		~IMAPIAdviseSinkImpl();
	
	public:
		STDMETHOD(QueryInterface)(REFIID riid,
								  void** ppv);
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
		MAPIAddressBook* pAddressBook_;
	};
	friend class IMAPIAdviseSinkImpl;
	
	class RowSetDeleter
	{
	public:
		RowSetDeleter(const MAPIAddressBook* pAddressBook,
					  SRowSet* pSRowSet);
		~RowSetDeleter();
	
	private:
		RowSetDeleter(const RowSetDeleter&);
		RowSetDeleter& operator=(const RowSetDeleter&);
	
	private:
		const MAPIAddressBook* pAddressBook_;
		SRowSet* pSRowSet_;
	};
	friend class RowSetDeleter;

private:
	MAPIAddressBook(const MAPIAddressBook&);
	MAPIAddressBook& operator=(const MAPIAddressBook&);

private:
	IAddrBook* pAddrBook_;
	ULONG nConnection_;
	bool bAddressOnly_;
	bool bModified_;
};


/****************************************************************************
 *
 * WindowsAddressBook
 *
 */

class WindowsAddressBook : public MAPIAddressBook
{
public:
	WindowsAddressBook();
	virtual ~WindowsAddressBook();

public:
	virtual bool init(bool bAddressOnly);
	virtual void term();

protected:
	virtual void freeBuffer(void* pBuffer) const;

private:
	WindowsAddressBook(const WindowsAddressBook&);
	WindowsAddressBook& operator=(const WindowsAddressBook&);

private:
	HINSTANCE hInstWAB_;
	IWABObject* pWABObject_;
};


/****************************************************************************
 *
 * OutlookAddressBook
 *
 */

class OutlookAddressBook : public MAPIAddressBook
{
public:
	OutlookAddressBook();
	virtual ~OutlookAddressBook();

public:
	virtual bool init(bool bAddressOnly);
	virtual void term();

protected:
	virtual void freeBuffer(void* pBuffer) const;

private:
	OutlookAddressBook(const OutlookAddressBook&);
	OutlookAddressBook& operator=(const OutlookAddressBook&);

private:
	HINSTANCE hInst_;
	LPMAPIFREEBUFFER pfnMAPIFreeBuffer_;
};

#else // _WIN32_WCE

#if _WIN32_WCE >= 0x420 && defined _WIN32_WCE_PSPC

/****************************************************************************
 *
 * POOMAddressBook
 *
 */

class POOMAddressBook : public ExternalAddressBook
{
public:
	POOMAddressBook();
	virtual ~POOMAddressBook();

public:
	virtual bool init(bool bAddressOnly);
	virtual void term();
	virtual bool load(AddressBook* pAddressBook);
	virtual bool isModified();

private:
#if _WIN32_WCE >= 0x500
	bool registerNotification();
#endif
	void getCategories(AddressBook* pAddressBook,
					   struct IContact* pContact,
					   AddressBookAddress::CategoryList* pList);

#if _WIN32_WCE >= 0x500
private:
	class NotificationWindow :
		public qs::WindowBase,
		public qs::DefaultWindowHandler
	{
	public:
		explicit NotificationWindow(POOMAddressBook* pAddressBook);
		virtual ~NotificationWindow();
	
	public:
		virtual LRESULT windowProc(UINT uMsg,
								   WPARAM wParam,
								   LPARAM lParam);
	
	protected:
		LRESULT onPimFolderNotification(WPARAM wParam,
										LPARAM lParam);
	
	private:
		NotificationWindow(const NotificationWindow&);
		NotificationWindow& operator=(const NotificationWindow&);
	
	private:
		POOMAddressBook* pAddressBook_;
	};
	friend class NotificationWindow;
#endif

private:
	POOMAddressBook(const POOMAddressBook&);
	POOMAddressBook& operator=(const POOMAddressBook&);

private:
	struct IPOutlookApp* pPOutlookApp_;
#if _WIN32_WCE >= 0x500
	NotificationWindow* pNotificationWindow_;
#endif
	bool bAddressOnly_;
	bool bModified_;
};

#endif // _WIN32_WCE >= 0x420 && defined _WIN32_WCE_PSPC

#if _WIN32_WCE < 0x500

/****************************************************************************
 *
 * PocketOutlookAddressBook
 *
 */

class PocketOutlookAddressBook : public ExternalAddressBook
{
public:
	PocketOutlookAddressBook();
	virtual ~PocketOutlookAddressBook();

public:
	virtual bool init(bool bAddressOnly);
	virtual void term();
	virtual bool load(AddressBook* pAddressBook);
	virtual bool isModified();

private:
	class NotificationWindow :
		public qs::WindowBase,
		public qs::DefaultWindowHandler
	{
	public:
		explicit NotificationWindow(PocketOutlookAddressBook* pAddressBook);
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
		PocketOutlookAddressBook* pAddressBook_;
	};
	friend class NotificationWindow;

private:
	PocketOutlookAddressBook(const PocketOutlookAddressBook&);
	PocketOutlookAddressBook& operator=(const PocketOutlookAddressBook&);

private:
	HANDLE hCategoryDB_;
	HANDLE hContactsDB_;
	NotificationWindow* pNotificationWindow_;
	bool bAddressOnly_;
	bool bModified_;
};

#endif

#endif // _WIN32_WCE

}


#endif // __ADDRESSBOOK_H__
