/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmapplication.h>
#include <qmextensions.h>
#include <qmsecurity.h>

#include <qsconv.h>
#include <qserror.h>
#include <qsmime.h>
#include <qsnew.h>
#include <qsosutil.h>

#include <algorithm>

#ifndef _WIN32_WCE
#	include <mapiguid.h>
#else
#	include <addrmapi.h>
#endif

#include "addressbook.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * AddressBook
 *
 */

qm::AddressBook::AddressBook(const Security* pSecurity, QSTATUS* pstatus) :
	pSecurity_(pSecurity),
	bContactChanged_(true),
#ifndef _WIN32_WCE
	hInstWAB_(0),
	pAddrBook_(0),
	pWABObject_(0),
	nConnection_(0)
#else
	hCategoryDB_(0),
	hContactsDB_(0),
	pNotificationWindow_(0)
#endif
{
	DECLARE_QSTATUS();
	
	SYSTEMTIME st;
	::GetSystemTime(&st);
	::SystemTimeToFileTime(&st, &ft_);
	
	status = initWAB();
//	CHECK_QSTATUS_SET(pstatus);
}

qm::AddressBook::~AddressBook()
{
	clear(TYPE_BOTH);
	
#ifndef _WIN32_WCE
	if (pAddrBook_) {
		if (nConnection_ != 0)
			pAddrBook_->Unadvise(nConnection_);
		pAddrBook_->Release();
		pWABObject_->Release();
	}
	if (hInstWAB_)
		::FreeLibrary(hInstWAB_);
#else
	if (hCategoryDB_)
		::CloseHandle(hCategoryDB_);
	if (hContactsDB_)
		::CloseHandle(hContactsDB_);
	if (pNotificationWindow_)
		pNotificationWindow_->destroyWindow();
#endif
}

QSTATUS qm::AddressBook::getEntries(const EntryList** ppList)
{
	assert(ppList);
	
	DECLARE_QSTATUS();
	
	*ppList = 0;
	
	status = load();
	CHECK_QSTATUS();
	
	*ppList = &listEntry_;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::AddressBook::getCategories(CategoryList* pList)
{
	assert(pList);
	
	DECLARE_QSTATUS();
	
	status = load();
	CHECK_QSTATUS();
	
	status = STLWrapper<CategoryList>(*pList).resize(listCategory_.size());
	CHECK_QSTATUS();
	std::copy(listCategory_.begin(), listCategory_.end(), pList->begin());
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::AddressBook::getAddress(const WCHAR* pwszAlias,
	const AddressBookAddress** ppAddress)
{
	assert(pwszAlias);
	assert(ppAddress);
	
	DECLARE_QSTATUS();
	
	*ppAddress = 0;
	
	status = load();
	CHECK_QSTATUS();
	
	EntryList::iterator itE = listEntry_.begin();
	while (itE != listEntry_.end() && !*ppAddress) {
		const AddressBookEntry::AddressList& l = (*itE)->getAddresses();
		AddressBookEntry::AddressList::const_iterator itA = l.begin();
		while (itA != l.end() && !*ppAddress) {
			if ((*itA)->getAlias() &&
				wcscmp((*itA)->getAlias(), pwszAlias) == 0)
				*ppAddress = *itA;
			++itA;
		}
		++itE;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::AddressBook::expandAlias(
	const WCHAR* pwszAddresses, qs::WSTRING* pwstrAddresses)
{
	assert(pwszAddresses);
	assert(pwstrAddresses);
	
	DECLARE_QSTATUS();
	
	*pwstrAddresses = 0;
	
	status = load();
	CHECK_QSTATUS();
	
	DummyParser field(pwszAddresses, 0, &status);
	CHECK_QSTATUS();
	Part dummy(&status);
	CHECK_QSTATUS();
	status = dummy.setField(L"Dummy", field);
	CHECK_QSTATUS();
	AddressListParser addressList(0, &status);
	CHECK_QSTATUS();
	Part::Field f;
	status = dummy.getField(L"Dummy", &addressList, &f);
	CHECK_QSTATUS();
	if (f != Part::FIELD_EXIST)
		return QSTATUS_FAIL;
	
	StringBuffer<WSTRING> buf(&status);
	
	const AddressListParser::AddressList& l = addressList.getAddressList();
	AddressListParser::AddressList::const_iterator it = l.begin();
	while (it != l.end()) {
		string_ptr<WSTRING> wstrValue;
		status = (*it)->getValue(&wstrValue);
		CHECK_QSTATUS();
		const AddressBookAddress* pAddress = 0;
		status = getAddress(wstrValue.get(), &pAddress);
		CHECK_QSTATUS();
		
		if (buf.getLength() != 0) {
			status = buf.append(L", ");
			CHECK_QSTATUS();
		}
		if (pAddress) {
			wstrValue.reset(0);
			status = pAddress->getValue(&wstrValue);
			CHECK_QSTATUS();
		}
		status = buf.append(wstrValue.get());
		CHECK_QSTATUS();
		
		++it;
	}

	*pwstrAddresses = buf.getString();
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::AddressBook::getEntry(const WCHAR* pwszAddress,
	const AddressBookEntry** ppEntry)
{
	assert(pwszAddress);
	assert(ppEntry);
	
	DECLARE_QSTATUS();
	
	*ppEntry = 0;
	
	status = load();
	CHECK_QSTATUS();
	status = prepareEntryMap();
	CHECK_QSTATUS();
	
	EntryMap::value_type v(pwszAddress, 0);
	EntryMap::const_iterator it = std::lower_bound(
		mapEntry_.begin(), mapEntry_.end(), v,
		binary_compose_f_gx_hy(
			string_less_i<WCHAR>(),
			std::select1st<EntryMap::value_type>(),
			std::select1st<EntryMap::value_type>()));
	if (it != mapEntry_.end() && _wcsicmp((*it).first, pwszAddress) == 0)
		*ppEntry = (*it).second;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::AddressBook::addEntry(AddressBookEntry* pEntry)
{
	return STLWrapper<EntryList>(listEntry_).push_back(pEntry);
}

QSTATUS qm::AddressBook::getCategory(const WCHAR* pwszCategory,
	const AddressBookCategory** ppCategory)
{
	assert(pwszCategory);
	assert(ppCategory);
	
	DECLARE_QSTATUS();
	
	*ppCategory = 0;
	
	CategoryList::iterator it = std::find_if(
		listCategory_.begin(), listCategory_.end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				string_equal<WCHAR>(),
				std::mem_fun(&AddressBookCategory::getName),
				std::identity<const WCHAR*>()),
			pwszCategory));
	if (it != listCategory_.end()) {
		*ppCategory = *it;
	}
	else {
		std::auto_ptr<AddressBookCategory> pCategory;
		status = newQsObject(pwszCategory, &pCategory);
		CHECK_QSTATUS();
		status = STLWrapper<CategoryList>(listCategory_).push_back(pCategory.get());
		CHECK_QSTATUS();
		*ppCategory = pCategory.release();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::AddressBook::initWAB()
{
	DECLARE_QSTATUS();
	
#ifndef _WIN32_WCE
	Registry reg(HKEY_LOCAL_MACHINE,
		L"Software\\Microsoft\\WAB\\DLLPath", &status);
	CHECK_QSTATUS();
	if (!reg)
		return QSTATUS_FAIL;
	
	string_ptr<WSTRING> wstrPath;
	status = reg.getValue(L"", &wstrPath);
	CHECK_QSTATUS();
	
	W2T(wstrPath.get(), ptszPath);
	hInstWAB_ = ::LoadLibrary(ptszPath);
	if (!hInstWAB_)
		return QSTATUS_FAIL;
	
	LPWABOPEN pfnWABOpen = reinterpret_cast<LPWABOPEN>(
		::GetProcAddress(hInstWAB_, "WABOpen"));
	if (!pfnWABOpen)
		return QSTATUS_FAIL;
	
	ComPtr<IAddrBook> pAddrBook;
	ComPtr<IWABObject> pWABObject;
	WAB_PARAM param = { sizeof(param) };
	if ((*pfnWABOpen)(&pAddrBook, &pWABObject, &param, 0) != S_OK)
		return QSTATUS_FAIL;
	
	ULONG nSize = 0;
	ENTRYID* pEntryId = 0;
	if (pAddrBook->GetPAB(&nSize, &pEntryId) != S_OK)
		return QSTATUS_FAIL;
	
	std::auto_ptr<IMAPIAdviseSinkImpl> pAdviseSink;
	status = newQsObject(this, &pAdviseSink);
	CHECK_QSTATUS();
	if (pAddrBook->Advise(sizeof(ENTRYID), pEntryId,
		0xffffffff, pAdviseSink.get(), &nConnection_) != S_OK)
		return QSTATUS_FAIL;
	pAdviseSink.release();
	
	pAddrBook_ = pAddrBook.release();
	pWABObject_ = pWABObject.release();
#else
	std::auto_ptr<NotificationWindow> pWindow;
	status = newQsObject(this, &pWindow);
	CHECK_QSTATUS();
	status = pWindow->create(L"QmAddressBookNotificationWindow",
		0, WS_POPUP, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	CHECK_QSTATUS();
	pNotificationWindow_ = pWindow.release();
	
	CEOID oidCategory = 0;
	hCategoryDB_ = ::CeOpenDatabase(&oidCategory,
		L"\\Categories Database", 0, CEDB_AUTOINCREMENT,
		pNotificationWindow_->getHandle());
	
	CEOID oidContact = 0;
	hContactsDB_ = ::CeOpenDatabase(&oidContact,
		L"Contacts Database", 0, CEDB_AUTOINCREMENT,
		pNotificationWindow_->getHandle());
#endif
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::AddressBook::load()
{
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrPath;
	status = Application::getApplication().getProfilePath(
		Extensions::ADDRESSBOOK, &wstrPath);
	CHECK_QSTATUS();
	
	bool bReload = false;
	bool bClear = false;
	
	W2T(wstrPath.get(), ptszPath);
	AutoHandle hFile(::CreateFile(ptszPath, GENERIC_READ, 0, 0,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0));
	if (hFile.get()) {
		FILETIME ft;
		::GetFileTime(hFile.get(), 0, 0, &ft);
		hFile.close();
		
		if (::CompareFileTime(&ft, &ft_) != 0) {
			bReload = true;
			ft_ = ft;
		}
	}
	else {
		bClear = true;
	}
	
	if (bReload || bClear || bContactChanged_) {
		clear(TYPE_BOTH);
		
		if (bReload) {
			XMLReader reader(&status);
			CHECK_QSTATUS();
			AddressBookContentHandler handler(this, &status);
			CHECK_QSTATUS();
			reader.setContentHandler(&handler);
			status = reader.parse(wstrPath.get());
			CHECK_QSTATUS();
			
		}
		if (bClear || bContactChanged_) {
			status = loadWAB();
//			CHECK_QSTATUS();
		}
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::AddressBook::loadWAB()
{
	DECLARE_QSTATUS();
	
#ifndef _WIN32_WCE
	if (!pAddrBook_)
		return QSTATUS_FAIL;
	
	ULONG nSize = 0;
	ENTRYID* pEntryId = 0;
	if (pAddrBook_->GetPAB(&nSize, &pEntryId) != S_OK)
		return QSTATUS_FAIL;
	
	ULONG nType = 0;
	ComPtr<IABContainer> pABC;
	if (pAddrBook_->OpenEntry(nSize, pEntryId,
		0, 0, &nType, reinterpret_cast<IUnknown**>(&pABC)) != S_OK)
		return QSTATUS_FAIL;
	
	ComPtr<IMAPITable> pTable;
	if (pABC->GetContentsTable(0, &pTable) != S_OK)
		return QSTATUS_FAIL;
	
	ULONG props[] = {
		PR_ENTRYID,
		PR_DISPLAY_NAME,
		PR_EMAIL_ADDRESS
	};
	SizedSPropTagArray(countof(props), columns) = {
		countof(props)
	};
	memcpy(columns.aulPropTag, props, sizeof(props));
	
	if (pTable->SetColumns(reinterpret_cast<LPSPropTagArray>(&columns), 0) == S_OK) {
		LONG nRowsSought = 0;
		if (pTable->SeekRow(BOOKMARK_BEGINNING, 0, &nRowsSought) == S_OK) {
			while (true) {
				SRowSet* pSRowSet = 0;
				if (pTable->QueryRows(1, 0, &pSRowSet) != S_OK)
					break;
				struct Deleter
				{
					Deleter(IWABObject* pWABObject, SRowSet* pSRowSet) :
						pWABObject_(pWABObject),
						pSRowSet_(pSRowSet)
					{
					}
					
					~Deleter()
					{
						for (ULONG n = 0; n < pSRowSet_->cRows; ++n)
							pWABObject_->FreeBuffer(pSRowSet_->aRow[n].lpProps);
						pWABObject_->FreeBuffer(pSRowSet_);
					}
					
					IWABObject* pWABObject_;
					SRowSet* pSRowSet_;
				} deleter(pWABObject_, pSRowSet);
				if (pSRowSet->cRows == 0)
					break;
				
				for (ULONG nRow = 0; nRow < pSRowSet->cRows; ++nRow) {
					SRow* pRow = pSRowSet->aRow + nRow;
					
					std::auto_ptr<AddressBookEntry> pEntry;
					status = newQsObject(true, &pEntry);
					CHECK_QSTATUS();
					
					for (ULONG nValue = 0; nValue < pRow->cValues; ++nValue) {
						SPropValue* pValue = pRow->lpProps + nValue;
						ULONG nTag = pValue->ulPropTag;
						switch (PROP_ID(nTag)) {
						case PROP_ID(PR_DISPLAY_NAME):
							{
								string_ptr<WSTRING> wstrName;
								if (PROP_TYPE(nTag) == PT_STRING8)
									wstrName.reset(mbs2wcs(pValue->Value.lpszA));
								else if (PROP_TYPE(nTag) == PT_UNICODE)
									wstrName.reset(allocWString(pValue->Value.lpszW));
								if (wstrName.get())
									pEntry->setName(wstrName.release());
							}
							break;
						case PROP_ID(PR_EMAIL_ADDRESS):
							{
								string_ptr<WSTRING> wstrAddress;
								if (PROP_TYPE(nTag) == PT_STRING8)
									wstrAddress.reset(mbs2wcs(pValue->Value.lpszA));
								else if (PROP_TYPE(nTag) == PT_UNICODE)
									wstrAddress.reset(allocWString(pValue->Value.lpszW));
								if (wstrAddress.get()) {
									std::auto_ptr<AddressBookAddress> pAddress;
									status = newQsObject(pEntry.get(),
										static_cast<const WCHAR*>(0),
										AddressBookAddress::CategoryList(),
										static_cast<const WCHAR*>(0),
										static_cast<const WCHAR*>(0),
										false, &pAddress);
									CHECK_QSTATUS();
									pAddress->setAddress(wstrAddress.release());
									status = pEntry->addAddress(pAddress.get());
									CHECK_QSTATUS();
									pAddress.release();
								}
							}
							break;
						default:
							break;
						}
					}
					
					if (pEntry->getName() && !pEntry->getAddresses().empty()) {
						status = addEntry(pEntry.get());
						CHECK_QSTATUS();
						pEntry.release();
					}
				}
			}
		}
	}
#else
	if (!hContactsDB_)
		return QSTATUS_FAIL;
	
	typedef std::vector<std::pair<unsigned int, WSTRING> > CategoryMap;
	CategoryMap mapCategory;
	struct Deleter
	{
		typedef CategoryMap Map;
		Deleter(Map& m) : m_(m) {}
		~Deleter()
		{
			std::for_each(m_.begin(), m_.end(),
				unary_compose_f_gx(
					string_free<WSTRING>(),
					std::select2nd<Map::value_type>()));
		}
		Map& m_;
	} deleter(mapCategory);
	
	struct LocalFreeCaller
	{
		LocalFreeCaller(LPBYTE p) : p_(p) {}
		~LocalFreeCaller() { ::LocalFree(p_); }
		LPBYTE p_;
	};
	
	if (hCategoryDB_) {
		DWORD dwIndex = 0;
		::CeSeekDatabase(hCategoryDB_, CEDB_SEEK_BEGINNING, 0, &dwIndex);
		
		static CEPROPID cepropid[] = {
			0x40020002,		// ID
			0x4001001f,		// Name
			0x40040002		// Count
		};
		while (true) {
			WORD wProps = sizeof(cepropid)/sizeof(cepropid[0]);
			LPBYTE pBuffer = 0;
			DWORD dwSize = 0;
			CEOID oid = ::CeReadRecordProps(hCategoryDB_,
				CEDB_ALLOWREALLOC, &wProps, cepropid, &pBuffer, &dwSize);
			if (!oid)
				break;
			if (!pBuffer)
				continue;
			
			LocalFreeCaller free(pBuffer);
			
			CEPROPVAL* pVal = reinterpret_cast<CEPROPVAL*>(pBuffer);
			if (pVal[0].wFlags != CEDB_PROPNOTFOUND &&
				pVal[1].wFlags != CEDB_PROPNOTFOUND &&
				pVal[2].wFlags != CEDB_PROPNOTFOUND &&
				pVal[2].val.iVal != 0) {
				unsigned int nId = pVal[0].val.iVal;
				string_ptr<WSTRING> wstrCategory(
					allocWString(pVal[1].val.lpwstr));
				if (!wstrCategory.get())
					return QSTATUS_OUTOFMEMORY;
				
				status = STLWrapper<CategoryMap>(mapCategory).push_back(
					CategoryMap::value_type(nId, wstrCategory.get()));
				CHECK_QSTATUS();
				wstrCategory.release();
			}
		}
	}
	
	std::sort(mapCategory.begin(), mapCategory.end(),
		binary_compose_f_gx_hy(
			std::less<unsigned int>(),
			std::select1st<CategoryMap::value_type>(),
			std::select1st<CategoryMap::value_type>()));
	
	DWORD dwIndex = 0;
	::CeSeekDatabase(hContactsDB_, CEDB_SEEK_BEGINNING, 0, &dwIndex);
	
	static CEPROPID cepropid[] = {
		HHPR_SURNAME,
		HHPR_GIVEN_NAME,
		HHPR_EMAIL1_EMAIL_ADDRESS,
		HHPR_EMAIL2_EMAIL_ADDRESS,
		HHPR_EMAIL3_EMAIL_ADDRESS,
//		HHPR_CATEGORY,
		0x160041,
#ifdef JAPAN
		HHPR_YOMI_FIRSTNAME,
		HHPR_YOMI_LASTNAME
#endif // JAPAN
	};
	while (true) {
		WORD wProps = sizeof(cepropid)/sizeof(cepropid[0]);
		LPBYTE pBuffer = 0;
		DWORD dwSize = 0;
		CEOID oid = ::CeReadRecordProps(hContactsDB_,
			CEDB_ALLOWREALLOC, &wProps, cepropid, &pBuffer, &dwSize);
		if (!oid)
			break;
		if (!pBuffer)
			continue;
		
		LocalFreeCaller free(pBuffer);
		
		CEPROPVAL* pVal = reinterpret_cast<CEPROPVAL*>(pBuffer);
		
		const WCHAR* pwszGivenName = L"";
		if (pVal[1].wFlags != CEDB_PROPNOTFOUND)
			pwszGivenName = pVal[1].val.lpwstr;
		const WCHAR* pwszSurName = L"";
		if (pVal[0].wFlags != CEDB_PROPNOTFOUND)
			pwszSurName = pVal[0].val.lpwstr;
		
#ifdef JAPAN
		const WCHAR* pwszYomiLastName = L"";
		if (pVal[7].wFlags != CEDB_PROPNOTFOUND)
			pwszYomiLastName = pVal[7].val.lpwstr;
		const WCHAR* pwszYomiFirstName = L"";
		if (pVal[6].wFlags != CEDB_PROPNOTFOUND)
			pwszYomiFirstName = pVal[6].val.lpwstr;
		string_ptr<WSTRING> wstrSortKey(concat(
			pwszYomiLastName, L" ", pwszYomiFirstName));
#else
		string_ptr<WSTRING> wstrSortKey(concat(
			wstrSurName.get(), L", ", wstrGivenName()));
		if (!wstrSortKey.get())
			return QSTATUS_OUTOFMEMORY;
#endif
		
#ifdef JAPAN
		string_ptr<WSTRING> wstrName(concat(
			pwszSurName, L" ", pwszGivenName));
#else
		string_ptr<WSTRING> wstrName(concat(
			pwszGivenName, L" ", pwszSurName));
#endif
		if (!wstrName.get())
			return QSTATUS_OUTOFMEMORY;
		
		AddressBookAddress::CategoryList listCategory;
		if (pVal[5].wFlags != CEDB_PROPNOTFOUND) {
			BYTE* pCategory = pVal[5].val.blob.lpb;
			for (DWORD dwCat = 0; dwCat < pVal[5].val.blob.dwCount; ++dwCat) {
				BYTE b = *(pCategory + dwCat);
				if (b) {
					for (int n = 0; n < 8; ++n) {
						if ((b >> n) & 0x01) {
							unsigned int nId = dwCat*8 + n;
							CategoryMap::const_iterator it = std::lower_bound(
								mapCategory.begin(), mapCategory.end(),
								CategoryMap::value_type(nId, 0),
								binary_compose_f_gx_hy(
									std::less<unsigned int>(),
									std::select1st<CategoryMap::value_type>(),
									std::select1st<CategoryMap::value_type>()));
							if (it != mapCategory.end() && (*it).first == nId) {
								const AddressBookCategory* pCategory = 0;
								status = getCategory((*it).second, &pCategory);
								CHECK_QSTATUS();
								status = STLWrapper<AddressBookAddress::CategoryList>(
									listCategory).push_back(pCategory);
								CHECK_QSTATUS();
							}
						}
					}
				}
			}
		}
		
		std::auto_ptr<AddressBookEntry> pEntry;
		status = newQsObject(true, &pEntry);
		CHECK_QSTATUS();
		pEntry->setName(wstrName.release());
		pEntry->setSortKey(wstrSortKey.release());
		
		for (int nAddress = 2; nAddress < 5; ++nAddress) {
			if (pVal[nAddress].wFlags != CEDB_PROPNOTFOUND) {
				std::auto_ptr<AddressBookAddress> pAddress;
				status = newQsObject(pEntry.get(),
					static_cast<const WCHAR*>(0), listCategory,
					static_cast<const WCHAR*>(0),
					static_cast<const WCHAR*>(0), false, &pAddress);
				CHECK_QSTATUS();
				string_ptr<WSTRING> wstrAddress(
					allocWString(pVal[nAddress].val.lpwstr));
				if (!wstrAddress.get())
					return QSTATUS_OUTOFMEMORY;
				pAddress->setAddress(wstrAddress.release());
				status = pEntry->addAddress(pAddress.get());
				CHECK_QSTATUS();
				pAddress.release();
			}
		}
		
		status = addEntry(pEntry.get());
		CHECK_QSTATUS();
		pEntry.release();
	}
#endif
	
	bContactChanged_ = false;
	
	return QSTATUS_SUCCESS;
}

void qm::AddressBook::clear(unsigned int nType)
{
	EntryList::iterator it = listEntry_.begin();
	while (it != listEntry_.end()) {
		bool bWAB = (*it)->isWAB();
		if ((nType & TYPE_ADDRESSBOOK && !bWAB) ||
			(nType & TYPE_WAB && bWAB)) {
			delete *it;
			it = listEntry_.erase(it);
		}
		else {
			++it;
		}
	}
	
	std::for_each(listCategory_.begin(), listCategory_.end(),
		deleter<AddressBookCategory>());
	listCategory_.clear();
	
	mapEntry_.clear();
}

QSTATUS qm::AddressBook::prepareEntryMap()
{
	DECLARE_QSTATUS();
	
	if (mapEntry_.empty()) {
		EntryList::const_iterator itE = listEntry_.begin();
		while (itE != listEntry_.end()) {
			AddressBookEntry* pEntry = *itE;
			
			const AddressBookEntry::AddressList& l = pEntry->getAddresses();
			AddressBookEntry::AddressList::const_iterator itA = l.begin();
			while (itA != l.end()) {
				AddressBookAddress* pAddress = *itA;
				if (!pAddress->isRFC2822()) {
					EntryMap::value_type v(pAddress->getAddress(), pEntry);
					
					EntryMap::iterator itM = std::lower_bound(
						mapEntry_.begin(), mapEntry_.end(), v,
						binary_compose_f_gx_hy(
							string_less_i<WCHAR>(),
							std::select1st<EntryMap::value_type>(),
							std::select1st<EntryMap::value_type>()));
					if (itM == mapEntry_.end() || _wcsicmp((*itM).first, v.first) != 0) {
						EntryMap::iterator p;
						status = STLWrapper<EntryMap>(mapEntry_).insert(itM, v, &p);
						CHECK_QSTATUS();
					}
				}
				++itA;
			}
			
			++itE;
		}
	}
	
	return QSTATUS_SUCCESS;
}


#ifndef _WIN32_WCE

/****************************************************************************
 *
 * AddressBook::IMAPIAdviseSinkImpl
 *
 */

qm::AddressBook::IMAPIAdviseSinkImpl::IMAPIAdviseSinkImpl(
	AddressBook* pAddressBook, qs::QSTATUS* pstatus) :
	nRef_(0),
	pAddressBook_(pAddressBook)
{
}

qm::AddressBook::IMAPIAdviseSinkImpl::~IMAPIAdviseSinkImpl()
{
}

STDMETHODIMP qm::AddressBook::IMAPIAdviseSinkImpl::QueryInterface(
	REFIID riid, void** ppv)
{
	if (riid == IID_IUnknown || riid == IID_IMAPIAdviseSink)
		*ppv = static_cast<IMAPIAdviseSink*>(this);
	else
		return E_NOINTERFACE;
	
	AddRef();
	
	return S_OK;
}

STDMETHODIMP_(ULONG) qm::AddressBook::IMAPIAdviseSinkImpl::AddRef()
{
	return ++nRef_;
}

STDMETHODIMP_(ULONG) qm::AddressBook::IMAPIAdviseSinkImpl::Release()
{
	if (--nRef_ == 0) {
		delete this;
		return 0;
	}
	return nRef_;
}

STDMETHODIMP_(ULONG) qm::AddressBook::IMAPIAdviseSinkImpl::OnNotify(
	ULONG cNotif, LPNOTIFICATION lpNotifications)
{
	pAddressBook_->bContactChanged_ = true;
	return S_OK;
}

#else

/****************************************************************************
 *
 * AddressBook::NotificationWindow
 *
 */

qm:: AddressBook::NotificationWindow::NotificationWindow(
	AddressBook* pAddressBook, qs::QSTATUS* pstatus) :
	WindowBase(true, pstatus),
	DefaultWindowHandler(pstatus),
	pAddressBook_(pAddressBook)
{
	setWindowHandler(this, false);
}

qm:: AddressBook::NotificationWindow::~NotificationWindow()
{
}

LRESULT qm:: AddressBook::NotificationWindow::windowProc(
	UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	BEGIN_MESSAGE_HANDLER()
		HANDLE_MESSAGE(DB_CEOID_CREATED, onDBNotification)
		HANDLE_MESSAGE(DB_CEOID_RECORD_DELETED, onDBNotification)
		HANDLE_MESSAGE(DB_CEOID_CHANGED, onDBNotification)
	END_MESSAGE_HANDLER()
	return DefaultWindowHandler::windowProc(uMsg, wParam, lParam);
}

LRESULT qm:: AddressBook::NotificationWindow::onDBNotification(
	WPARAM wParam, LPARAM lParam)
{
	pAddressBook_->bContactChanged_ = true;
	return 0;
}

#endif


/****************************************************************************
 *
 * AddressBookEntry
 *
 */

qm::AddressBookEntry::AddressBookEntry(bool bWAB, QSTATUS* pstatus) :
	bWAB_(bWAB),
	wstrName_(0),
	wstrSortKey_(0)
{
}

qm::AddressBookEntry::~AddressBookEntry()
{
	freeWString(wstrName_);
	freeWString(wstrSortKey_);
	std::for_each(listAddress_.begin(), listAddress_.end(),
		deleter<AddressBookAddress>());
}

bool qm::AddressBookEntry::isWAB() const
{
	return bWAB_;
}

const WCHAR* qm::AddressBookEntry::getName() const
{
	return wstrName_;
}

const WCHAR* qm::AddressBookEntry::getSortKey() const
{
	return wstrSortKey_ ? wstrSortKey_ : wstrName_;
}

const AddressBookEntry::AddressList& qm::AddressBookEntry::getAddresses() const
{
	return listAddress_;
}

void qm::AddressBookEntry::setName(qs::WSTRING wstrName)
{
	wstrName_ = wstrName;
}

void qm::AddressBookEntry::setSortKey(qs::WSTRING wstrSortKey)
{
	wstrSortKey_ = wstrSortKey;
}

QSTATUS qm::AddressBookEntry::addAddress(AddressBookAddress* pAddress)
{
	return STLWrapper<AddressList>(listAddress_).push_back(pAddress);
}


/****************************************************************************
 *
 * AddressBookAddress
 *
 */

qm::AddressBookAddress::AddressBookAddress(const AddressBookEntry* pEntry,
	const WCHAR* pwszAlias, const CategoryList& listCategory,
	const WCHAR* pwszComment, const WCHAR* pwszCertificate,
	bool bRFC2822, QSTATUS* pstatus) :
	pEntry_(pEntry),
	wstrAddress_(0),
	wstrAlias_(0),
	wstrComment_(0),
	wstrCertificate_(0),
	bRFC2822_(bRFC2822)
{
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrAlias;
	if (pwszAlias) {
		wstrAlias.reset(allocWString(pwszAlias));
		if (!wstrAlias.get()) {
			*pstatus = QSTATUS_OUTOFMEMORY;
			return;
		}
	}
	
	status = STLWrapper<CategoryList>(listCategory_).resize(listCategory.size());
	CHECK_QSTATUS_SET(pstatus);
	std::copy(listCategory.begin(), listCategory.end(), listCategory_.begin());
	
	string_ptr<WSTRING> wstrComment;
	if (pwszComment) {
		wstrComment.reset(allocWString(pwszComment));
		if (!wstrComment.get()) {
			*pstatus = QSTATUS_OUTOFMEMORY;
			return;
		}
	}
	
	string_ptr<WSTRING> wstrCertificate;
	if (pwszCertificate) {
		wstrCertificate.reset(allocWString(pwszCertificate));
		if (!wstrCertificate.get()) {
			*pstatus = QSTATUS_OUTOFMEMORY;
			return;
		}
	}
	
	wstrAlias_ = wstrAlias.release();
	wstrComment_ = wstrComment.release();
	wstrCertificate_ = wstrCertificate.release();
}

qm::AddressBookAddress::~AddressBookAddress()
{
	freeWString(wstrAddress_);
	freeWString(wstrAlias_);
	freeWString(wstrComment_);
	freeWString(wstrCertificate_);
}

const AddressBookEntry* qm::AddressBookAddress::getEntry() const
{
	return pEntry_;
}

const WCHAR* qm::AddressBookAddress::getAddress() const
{
	return wstrAddress_;
}

const WCHAR* qm::AddressBookAddress::getAlias() const
{
	return wstrAlias_;
}

const AddressBookAddress::CategoryList& qm::AddressBookAddress::getCategories() const
{
	return listCategory_;
}

const WCHAR* qm::AddressBookAddress::getComment() const
{
	return wstrComment_;
}

const WCHAR* qm::AddressBookAddress::getCertificate() const
{
	return wstrCertificate_;
}

bool qm::AddressBookAddress::isRFC2822() const
{
	return bRFC2822_;
}

QSTATUS qm::AddressBookAddress::getValue(WSTRING* pwstrValue) const
{
	assert(pwstrValue);
	
	DECLARE_QSTATUS();
	
	*pwstrValue = 0;
	
	StringBuffer<WSTRING> buf(&status);
	CHECK_QSTATUS();
	if (bRFC2822_) {
		status = buf.append(wstrAddress_);
		CHECK_QSTATUS();
	}
	else {
		AddressParser address(pEntry_->getName(),
			wstrAddress_, &status);
		CHECK_QSTATUS();
		string_ptr<WSTRING> wstrValue;
		status = address.getValue(&wstrValue);
		CHECK_QSTATUS();
		status = buf.append(wstrValue.get());
		CHECK_QSTATUS();
	}
	
	*pwstrValue = buf.getString();
	
	return QSTATUS_SUCCESS;
}

void qm::AddressBookAddress::setAddress(qs::WSTRING wstrAddress)
{
	wstrAddress_ = wstrAddress;
}


/****************************************************************************
 *
 * AddressBookCategory
 *
 */

qm::AddressBookCategory::AddressBookCategory(
	const WCHAR* pwszName, QSTATUS* pstatus) :
	wstrName_(0)
{
	string_ptr<WSTRING> wstrName(allocWString(pwszName));
	if (!wstrName.get()) {
		*pstatus = QSTATUS_OUTOFMEMORY;
		return;
	}
	wstrName_ = wstrName.release();
}

qm::AddressBookCategory::~AddressBookCategory()
{
	freeWString(wstrName_);
}

const WCHAR* qm::AddressBookCategory::getName() const
{
	return wstrName_;
}


/****************************************************************************
 *
 * AddressBookContentHandler
 *
 */

qm::AddressBookContentHandler::AddressBookContentHandler(
	AddressBook* pAddressBook, QSTATUS* pstatus) :
	DefaultHandler(pstatus),
	pAddressBook_(pAddressBook),
	state_(STATE_ROOT),
	pEntry_(0),
	pAddress_(0)
{
	DECLARE_QSTATUS();
	
	status = newQsObject(&pBuffer_);
	CHECK_QSTATUS_SET(pstatus);
}

qm::AddressBookContentHandler::~AddressBookContentHandler()
{
	delete pBuffer_;
}

QSTATUS qm::AddressBookContentHandler::startElement(
	const WCHAR* pwszNamespaceURI, const WCHAR* pwszLocalName,
	const WCHAR* pwszQName, const qs::Attributes& attributes)
{
	DECLARE_QSTATUS();
	
	if (wcscmp(pwszLocalName, L"addressBook") == 0) {
		if (state_ != STATE_ROOT)
			return QSTATUS_FAIL;
		
		if (attributes.getLength() != 0)
			return QSTATUS_FAIL;
		
		state_ = STATE_ADDRESSBOOK;
	}
	else if (wcscmp(pwszLocalName, L"entry") == 0) {
		if (state_ != STATE_ADDRESSBOOK)
			return QSTATUS_FAIL;
		
		if (attributes.getLength() != 0)
			return QSTATUS_FAIL;
		
		std::auto_ptr<AddressBookEntry> pEntry;
		status = newQsObject(false, &pEntry);
		CHECK_QSTATUS();
		status = pAddressBook_->addEntry(pEntry.get());
		CHECK_QSTATUS();
		pEntry_ = pEntry.release();
		
		state_ = STATE_ENTRY;
	}
	else if (wcscmp(pwszLocalName, L"name") == 0) {
		if (state_ != STATE_ENTRY)
			return QSTATUS_FAIL;
		
		if (attributes.getLength() != 0)
			return QSTATUS_FAIL;
		
		state_ = STATE_NAME;
	}
	else if (wcscmp(pwszLocalName, L"sortKey") == 0) {
		if (state_ != STATE_ENTRY)
			return QSTATUS_FAIL;
		
		if (attributes.getLength() != 0)
			return QSTATUS_FAIL;
		
		state_ = STATE_SORTKEY;
	}
	else if (wcscmp(pwszLocalName, L"addresses") == 0) {
		if (state_ != STATE_ENTRY)
			return QSTATUS_FAIL;
		
		if (attributes.getLength() != 0)
			return QSTATUS_FAIL;
		
		state_ = STATE_ADDRESSES;
	}
	else if (wcscmp(pwszLocalName, L"address") == 0) {
		if (state_ != STATE_ADDRESSES)
			return QSTATUS_FAIL;
		
		const WCHAR* pwszAlias = 0;
		const WCHAR* pwszCategory = 0;
		const WCHAR* pwszComment = 0;
		const WCHAR* pwszCertificate = 0;
		bool bRFC2822 = false;
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrName, L"alias") == 0)
				pwszAlias = attributes.getValue(n);
			else if (wcscmp(pwszAttrName, L"category") == 0)
				pwszCategory = attributes.getValue(n);
			else if (wcscmp(pwszAttrName, L"comment") == 0)
				pwszComment = attributes.getValue(n);
			else if (wcscmp(pwszAttrName, L"certificate") == 0)
				pwszCertificate = attributes.getValue(n);
			else if (wcscmp(pwszAttrName, L"rfc2822") == 0)
				bRFC2822 = wcscmp(attributes.getValue(n), L"true") == 0;
			else
				return QSTATUS_FAIL;
		}
		
		AddressBookAddress::CategoryList listCategory;
		if (pwszCategory) {
			string_ptr<WSTRING> wstrCategory(allocWString(pwszCategory));
			if (!wstrCategory.get())
				return QSTATUS_OUTOFMEMORY;
			const WCHAR* p = wcstok(wstrCategory.get(), L",");
			while (p) {
				const AddressBookCategory* pCategory = 0;
				status = pAddressBook_->getCategory(p, &pCategory);
				CHECK_QSTATUS();
				status = STLWrapper<AddressBookAddress::CategoryList>(
					listCategory).push_back(pCategory);
				CHECK_QSTATUS();
				p = wcstok(0, L",");
			}
		}
		
		assert(pEntry_);
		std::auto_ptr<AddressBookAddress> pAddress;
		status = newQsObject(pEntry_, pwszAlias, listCategory,
			pwszComment, pwszCertificate, bRFC2822, &pAddress);
		CHECK_QSTATUS();
		status = pEntry_->addAddress(pAddress.get());
		CHECK_QSTATUS();
		pAddress_ = pAddress.release();
		
		state_ = STATE_ADDRESS;
	}
	else {
		return QSTATUS_FAIL;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::AddressBookContentHandler::endElement(
	const WCHAR* pwszNamespaceURI, const WCHAR* pwszLocalName,
	const WCHAR* pwszQName)
{
	DECLARE_QSTATUS();
	
	if (wcscmp(pwszLocalName, L"addressBook") == 0) {
		assert(state_ == STATE_ADDRESSBOOK);
		state_ = STATE_ROOT;
	}
	else if (wcscmp(pwszLocalName, L"entry") == 0) {
		assert(state_ == STATE_ENTRY);
		
		assert(pEntry_);
		if (!pEntry_->getName() || pEntry_->getAddresses().empty())
			return QSTATUS_FAIL;
		pEntry_ = 0;
		
		state_ = STATE_ADDRESSBOOK;
	}
	else if (wcscmp(pwszLocalName, L"name") == 0) {
		assert(state_ == STATE_NAME);
		
		assert(pEntry_);
		pEntry_->setName(pBuffer_->getString());
		
		state_ = STATE_ENTRY;
	}
	else if (wcscmp(pwszLocalName, L"sortKey") == 0) {
		assert(state_ == STATE_SORTKEY);
		
		assert(pEntry_);
		pEntry_->setSortKey(pBuffer_->getString());
		
		state_ = STATE_ENTRY;
	}
	else if (wcscmp(pwszLocalName, L"addresses") == 0) {
		assert(state_ == STATE_ADDRESSES);
		state_ = STATE_ENTRY;
	}
	else if (wcscmp(pwszLocalName, L"address") == 0) {
		assert(state_ == STATE_ADDRESS);
		
		assert(pAddress_);
		pAddress_->setAddress(pBuffer_->getString());
		
		state_ = STATE_ADDRESSES;
	}
	else {
		return QSTATUS_FAIL;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::AddressBookContentHandler::characters(
	const WCHAR* pwsz, size_t nStart, size_t nLength)
{
	DECLARE_QSTATUS();
	
	if (state_ == STATE_NAME ||
		state_ == STATE_SORTKEY ||
		state_ == STATE_ADDRESS) {
		status = pBuffer_->append(pwsz + nStart, nLength);
		CHECK_QSTATUS();
	}
	else {
		const WCHAR* p = pwsz + nStart;
		for (size_t n = 0; n < nLength; ++n, ++p) {
			if (*p != L' ' && *p != L'\t' && *p != '\n')
				return QSTATUS_FAIL;
		}
	}
	
	return QSTATUS_SUCCESS;
}
