/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#pragma warning(disable:4786)

#define INITGUID

#include <qmmessage.h>
#include <qmsecurity.h>

#include <qsconv.h>
#include <qsinit.h>
#include <qslog.h>
#include <qsmime.h>
#include <qsosutil.h>

#include <algorithm>

#include <boost/bind.hpp>

#ifndef _WIN32_WCE
#	define USES_IID_IDistList
#	define USES_IID_IMailUser
#	define USES_IID_IMAPIAdviseSink
#	define USES_IID_IMAPIContainer
#	include <mapiguid.h>
#	include <mapi.h>
#	include <mapix.h>
#else
#	if _WIN32_WCE < 0x500
#		include <addrmapi.h>
#	endif
#	if _WIN32_WCE >= 0x420 && defined _WIN32_WCE_PSPC
#		include <pimstore.h>
#	endif
#endif
#include <tchar.h>

#include "addressbook.h"
#include "../util/confighelper.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * AddressBook
 *
 */

qm::AddressBook::AddressBook(const WCHAR* pwszPath,
							 Profile* pProfile,
							 bool bLoadExternal) :
	pProfile_(pProfile)
{
	assert(pProfile || !bLoadExternal);
	
	wstrPath_ = allocWString(pwszPath);
	
	SYSTEMTIME st;
	::GetSystemTime(&st);
	::SystemTimeToFileTime(&st, &ft_);
	
	if (bLoadExternal)
		initExternal();
	
	load();
}

qm::AddressBook::~AddressBook()
{
	clear(TYPE_BOTH);
}

const AddressBook::EntryList& qm::AddressBook::getEntries() const
{
	return listEntry_;
}

const AddressBook::CategoryList& qm::AddressBook::getCategories() const
{
	return listCategory_;
}

const AddressBookAddress* qm::AddressBook::getAddress(const WCHAR* pwszAlias) const
{
	assert(pwszAlias);
	
	for (EntryList::const_iterator itE = listEntry_.begin(); itE != listEntry_.end(); ++itE) {
		const AddressBookEntry::AddressList& l = (*itE)->getAddresses();
		for (AddressBookEntry::AddressList::const_iterator itA = l.begin(); itA != l.end(); ++itA) {
			const AddressBookAddress* pAddress = *itA;
			if (pAddress->getAlias() && wcscmp(pAddress->getAlias(), pwszAlias) == 0)
				return pAddress;
		}
	}
	
	return 0;
}

wstring_ptr qm::AddressBook::expandAlias(const WCHAR* pwszAddresses) const
{
	assert(pwszAddresses);
	
	UTF8Parser field(pwszAddresses);
	Part dummy;
	if (!dummy.setField(L"Dummy", field))
		return 0;
	AddressListParser addressList(AddressListParser::FLAG_ALLOWUTF8);
	if (dummy.getField(L"Dummy", &addressList) != Part::FIELD_EXIST)
		return 0;
	
	StringBuffer<WSTRING> buf;
	
	const AddressListParser::AddressList& l = addressList.getAddressList();
	for (AddressListParser::AddressList::const_iterator it = l.begin(); it != l.end(); ++it) {
		wstring_ptr wstrValue((*it)->getValue());
		const AddressBookAddress* pAddress = getAddress(wstrValue.get());
		
		if (buf.getLength() != 0)
			buf.append(L", ");
		if (pAddress)
			wstrValue = pAddress->getValue();
		buf.append(wstrValue.get());
	}
	
	return buf.getString();
}

const AddressBookEntry* qm::AddressBook::getEntry(const WCHAR* pwszAddress) const
{
	assert(pwszAddress);
	
	prepareEntryMap();
	
	EntryMap::value_type v(const_cast<WSTRING>(pwszAddress), 0);
	EntryMap::const_iterator it = std::lower_bound(
		mapEntry_.begin(), mapEntry_.end(), v,
		boost::bind(string_less_i<WCHAR>(),
			boost::bind(&EntryMap::value_type::first, _1),
			boost::bind(&EntryMap::value_type::first, _2)));
	if (it == mapEntry_.end() || _wcsicmp((*it).first, pwszAddress) != 0)
		return 0;
	return (*it).second;
}

bool qm::AddressBook::reload()
{
	// The primary address book needs to be locked while reloading,
	// because other thread may read it. Other methods which modify
	// an address book must not be called on the primary address book.
	// If you need to modify the primary address book, create a temporary
	// address book and modify it and save, and then reload the primary
	// address book.
	// When you use the primary address book from other threads,
	// you must call a lock/unlock pair while you hold references to
	// objects returned from the primary address book. Otherwise,
	// they may be freed when the primary address book was reloaded.
	Lock<CriticalSection> lock(cs_);
	return load();
}

void qm::AddressBook::reloadProfiles()
{
	if (pExternalManager_.get())
		initExternal();
	reload();
}

bool qm::AddressBook::save() const
{
	return ConfigSaver<const AddressBook*, AddressBookWriter>::save(this, wstrPath_.get());
}

void qm::AddressBook::lock() const
{
	cs_.lock();
}

void qm::AddressBook::unlock() const
{
	cs_.unlock();
}

void qm::AddressBook::addEntry(std::auto_ptr<AddressBookEntry> pEntry)
{
	listEntry_.push_back(pEntry.get());
	pEntry.release();
}

void qm::AddressBook::removeEntry(const AddressBookEntry* pEntry)
{
	EntryList::iterator it = std::find(
		listEntry_.begin(), listEntry_.end(), pEntry);
	assert(it != listEntry_.end());
	delete *it;
	listEntry_.erase(it);
}

const AddressBookCategory* qm::AddressBook::getCategory(const WCHAR* pwszCategory)
{
	assert(pwszCategory);
	
	CategoryList::iterator it = std::find_if(
		listCategory_.begin(), listCategory_.end(),
		boost::bind(string_equal<WCHAR>(),
			boost::bind(&AddressBookCategory::getName, _1), pwszCategory));
	if (it != listCategory_.end()) {
		return *it;
	}
	else {
		std::auto_ptr<AddressBookCategory> pCategory(
			new AddressBookCategory(pwszCategory));
		listCategory_.push_back(pCategory.get());
		return pCategory.release();
	}
}

void qm::AddressBook::initExternal()
{
	pExternalManager_.reset(new ExternalAddressBookManager(pProfile_));
}

bool qm::AddressBook::load()
{
	bool bReload = false;
	bool bClear = false;
	
	W2T(wstrPath_.get(), ptszPath);
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
		
		SYSTEMTIME st;
		::GetSystemTime(&st);
		::SystemTimeToFileTime(&st, &ft_);
	}
	
	if (bReload || bClear || (pExternalManager_.get() && pExternalManager_->isModified())) {
		clear(TYPE_BOTH);
		
		if (!bClear) {
			XMLReader reader;
			AddressBookContentHandler handler(this);
			reader.setContentHandler(&handler);
			if (!reader.parse(wstrPath_.get()))
				return false;
		}
		
		loadExternal();
	}
	
	return true;
}

bool qm::AddressBook::loadExternal()
{
	if (!pExternalManager_.get())
		return true;
	return pExternalManager_->load(this);
}

void qm::AddressBook::clear(unsigned int nType)
{
	for (EntryList::iterator it = listEntry_.begin(); it != listEntry_.end(); ) {
		bool bExternal = (*it)->isExternal();
		if ((nType & TYPE_ADDRESSBOOK && !bExternal) ||
			(nType & TYPE_EXTERNAL && bExternal)) {
			delete *it;
			it = listEntry_.erase(it);
		}
		else {
			++it;
		}
	}
	
	std::for_each(listCategory_.begin(), listCategory_.end(),
		boost::checked_deleter<AddressBookCategory>());
	listCategory_.clear();
	
	clearEntryMap();
}

void qm::AddressBook::prepareEntryMap() const
{
	Lock<CriticalSection> lock(cs_);
	
	if (!mapEntry_.empty())
		return;
	
	for (EntryList::const_iterator itE = listEntry_.begin(); itE != listEntry_.end(); ++itE) {
		AddressBookEntry* pEntry = *itE;
		
		const AddressBookEntry::AddressList& l = pEntry->getAddresses();
		for (AddressBookEntry::AddressList::const_iterator itA = l.begin(); itA != l.end(); ++itA) {
			AddressBookAddress* pAddress = *itA;
			
			wstring_ptr wstrAddress;
			if (pAddress->isRFC2822()) {
				AddressParser address;
				if (MessageCreator::getAddress(pAddress->getAddress(), &address))
					wstrAddress = address.getAddress();
			}
			else {
				wstrAddress = allocWString(pAddress->getAddress());
			}
			if (wstrAddress.get()) {
				EntryMap::value_type v(wstrAddress.get(), pEntry);
				
				EntryMap::iterator itM = std::lower_bound(
					mapEntry_.begin(), mapEntry_.end(), v,
					boost::bind(string_less_i<WCHAR>(),
						boost::bind(&EntryMap::value_type::first, _1),
						boost::bind(&EntryMap::value_type::first, _2)));
				if (itM == mapEntry_.end() || _wcsicmp((*itM).first, v.first) != 0) {
					mapEntry_.insert(itM, v);
					wstrAddress.release();
				}
			}
		}
	}
}

void qm::AddressBook::clearEntryMap()
{
	std::for_each(mapEntry_.begin(), mapEntry_.end(),
		boost::bind(&freeWString, boost::bind(&EntryMap::value_type::first, _1)));
	mapEntry_.clear();
}


/****************************************************************************
 *
 * AddressBookEntry
 *
 */

qm::AddressBookEntry::AddressBookEntry(const WCHAR* pwszName,
									   const WCHAR* pwszSortKey,
									   bool bExternal) :
	bExternal_(bExternal)
{
	if (pwszName)
		wstrName_ = allocWString(pwszName);
	if (pwszSortKey)
		wstrSortKey_ = allocWString(pwszSortKey);
}

qm::AddressBookEntry::AddressBookEntry(const AddressBookEntry& entry)
{
	wstrName_ = allocWString(entry.wstrName_.get());
	
	if (entry.wstrSortKey_.get())
		wstrSortKey_ = allocWString(entry.wstrSortKey_.get());
	
	listAddress_.reserve(entry.listAddress_.size());
	for (AddressList::const_iterator it = entry.listAddress_.begin(); it != entry.listAddress_.end(); ++it)
		listAddress_.push_back(new AddressBookAddress(**it));
	
	bExternal_ = entry.bExternal_;
}

AddressBookEntry& qm::AddressBookEntry::operator=(const AddressBookEntry& entry)
{
	AddressBookEntry temp(entry);
	swap(temp);
	return *this;
}

qm::AddressBookEntry::~AddressBookEntry()
{
	clearAddresses();
}

const WCHAR* qm::AddressBookEntry::getName() const
{
	assert(wstrName_.get());
	return wstrName_.get();
}

void qm::AddressBookEntry::setName(const WCHAR* pwszName)
{
	assert(pwszName);
	wstrName_ = allocWString(pwszName);
}

const WCHAR* qm::AddressBookEntry::getSortKey() const
{
	return wstrSortKey_.get();
}

void qm::AddressBookEntry::setSortKey(const WCHAR* pwszSortKey)
{
	if (pwszSortKey)
		wstrSortKey_ = allocWString(pwszSortKey);
	else
		wstrSortKey_.reset(0);
}

const WCHAR* qm::AddressBookEntry::getActualSortKey() const
{
	return wstrSortKey_.get() ? wstrSortKey_.get() : wstrName_.get();
}

const AddressBookEntry::AddressList& qm::AddressBookEntry::getAddresses() const
{
	return listAddress_;
}

const AddressBookAddress* qm::AddressBookEntry::getAddress(const WCHAR* pwszAddress) const
{
	assert(pwszAddress);
	
	AddressList::const_iterator it = std::find_if(
		listAddress_.begin(), listAddress_.end(),
		boost::bind(string_equal_i<WCHAR>(),
			boost::bind(&AddressBookAddress::getAddress, _1), pwszAddress));
	return it != listAddress_.end() ? *it : 0;
}

void qm::AddressBookEntry::setAddresses(AddressList& listAddress)
{
	clearAddresses();
	listAddress_.swap(listAddress);
}

void qm::AddressBookEntry::addAddress(std::auto_ptr<AddressBookAddress> pAddress)
{
	listAddress_.push_back(pAddress.get());
	pAddress.release();
}

bool qm::AddressBookEntry::isExternal() const
{
	return bExternal_;
}

void qm::AddressBookEntry::clearAddresses()
{
	std::for_each(listAddress_.begin(), listAddress_.end(),
		boost::checked_deleter<AddressBookAddress>());
	listAddress_.clear();
}

void qm::AddressBookEntry::swap(AddressBookEntry& entry)
{
	std::swap(wstrName_, entry.wstrName_);
	std::swap(wstrSortKey_, entry.wstrSortKey_);
	listAddress_.swap(entry.listAddress_);
	std::swap(bExternal_, entry.bExternal_);
}


/****************************************************************************
 *
 * AddressBookAddress
 *
 */

qm::AddressBookAddress::AddressBookAddress(const AddressBookEntry* pEntry) :
	pEntry_(pEntry),
	bRFC2822_(false)
{
	wstrAddress_ = allocWString(L"");
}

qm::AddressBookAddress::AddressBookAddress(const AddressBookEntry* pEntry,
										   const WCHAR* pwszAddress,
										   const WCHAR* pwszAlias,
										   const CategoryList& listCategory,
										   const WCHAR* pwszComment,
										   const WCHAR* pwszCertificate,
										   bool bRFC2822) :
	pEntry_(pEntry),
	listCategory_(listCategory),
	bRFC2822_(bRFC2822)
{
	if (pwszAddress)
		setAddress(pwszAddress, bRFC2822);
	if (pwszAlias)
		wstrAlias_ = allocWString(pwszAlias);
	if (pwszComment)
		wstrComment_ = allocWString(pwszComment);
	if (pwszCertificate)
		wstrCertificate_ = allocWString(pwszCertificate);
}

qm::AddressBookAddress::AddressBookAddress(const AddressBookAddress& address) :
	pEntry_(address.pEntry_),
	listCategory_(address.listCategory_),
	bRFC2822_(address.bRFC2822_)
{
	wstrAddress_ = allocWString(address.wstrAddress_.get());
	if (address.wstrAlias_.get())
		wstrAlias_ = allocWString(address.wstrAlias_.get());
	if (address.wstrComment_.get())
		wstrComment_ = allocWString(address.wstrComment_.get());
	if (address.wstrCertificate_.get())
		wstrCertificate_ = allocWString(address.wstrCertificate_.get());
}

qm::AddressBookAddress::~AddressBookAddress()
{
}

const AddressBookEntry* qm::AddressBookAddress::getEntry() const
{
	return pEntry_;
}

const WCHAR* qm::AddressBookAddress::getAddress() const
{
	assert(wstrAddress_.get());
	return wstrAddress_.get();
}

void qm::AddressBookAddress::setAddress(const WCHAR* pwszAddress,
										bool bRFC2822)
{
	assert(pwszAddress);
	if (bRFC2822)
		wstrAddress_ = allocWString(pwszAddress);
	else
		wstrAddress_ = AddressParser(0, pwszAddress).getAddress();
	bRFC2822_ = bRFC2822;
}

const WCHAR* qm::AddressBookAddress::getAlias() const
{
	return wstrAlias_.get();
}

void qm::AddressBookAddress::setAlias(const WCHAR* pwszAlias)
{
	if (pwszAlias)
		wstrAlias_ = allocWString(pwszAlias);
	else
		wstrAlias_.reset(0);
}

const AddressBookAddress::CategoryList& qm::AddressBookAddress::getCategories() const
{
	return listCategory_;
}

void qm::AddressBookAddress::setCategories(const CategoryList& listCategory)
{
	listCategory_ = listCategory;
}

wstring_ptr qm::AddressBookAddress::getCategoryNames() const
{
	StringBuffer<WSTRING> bufCategory;
	for (CategoryList::const_iterator it = listCategory_.begin(); it != listCategory_.end(); ++it) {
		if (bufCategory.getLength() != 0)
			bufCategory.append(L',');
		bufCategory.append((*it)->getName());
	}
	return bufCategory.getString();
}

const WCHAR* qm::AddressBookAddress::getComment() const
{
	return wstrComment_.get();
}

void qm::AddressBookAddress::setComment(const WCHAR* pwszComment)
{
	if (pwszComment)
		wstrComment_ = allocWString(pwszComment);
	else
		wstrComment_.reset(0);
}

const WCHAR* qm::AddressBookAddress::getCertificate() const
{
	return wstrCertificate_.get();
}

void qm::AddressBookAddress::setCertificate(const WCHAR* pwszCertificate)
{
	if (pwszCertificate)
		wstrCertificate_ = allocWString(pwszCertificate);
	else
		wstrCertificate_.reset(0);
}

bool qm::AddressBookAddress::isRFC2822() const
{
	return bRFC2822_;
}

wstring_ptr qm::AddressBookAddress::getValue() const
{
	assert(wstrAddress_.get());
	
	StringBuffer<WSTRING> buf;
	
	if (bRFC2822_) {
		buf.append(wstrAddress_.get());
	}
	else {
		AddressParser address(pEntry_->getName(), wstrAddress_.get());
		wstring_ptr wstrValue(address.getValue());
		buf.append(wstrValue.get());
	}
	
	return buf.getString();
}


/****************************************************************************
 *
 * AddressBookCategory
 *
 */

qm::AddressBookCategory::AddressBookCategory(const WCHAR* pwszName)
{
	wstrName_ = allocWString(pwszName);
}

qm::AddressBookCategory::~AddressBookCategory()
{
}

const WCHAR* qm::AddressBookCategory::getName() const
{
	return wstrName_.get();
}


/****************************************************************************
 *
 * AddressBookCategory
 *
 */

bool qm::AddressBookCategoryLess::operator()(const AddressBookCategory* pLhs,
											 const AddressBookCategory* pRhs)
{
	const WCHAR* pwszLhs = pLhs->getName();
	const WCHAR* pwszRhs = pRhs->getName();
	
	while (*pwszLhs && *pwszRhs) {
		if (*pwszLhs == *pwszRhs)
			;
		else if (*pwszLhs == L'/')
			return true;
		else if (*pwszRhs == L'/')
			return false;
		else if (*pwszLhs < *pwszRhs)
			return true;
		else if (*pwszLhs > *pwszRhs)
			return false;
		
		++pwszLhs;
		++pwszRhs;
	}
	
	return *pwszRhs != L'\0';
}


/****************************************************************************
 *
 * AddressBookContentHandler
 *
 */

qm::AddressBookContentHandler::AddressBookContentHandler(AddressBook* pAddressBook) :
	pAddressBook_(pAddressBook),
	state_(STATE_ROOT),
	pEntry_(0),
	pAddress_(0)
{
}

qm::AddressBookContentHandler::~AddressBookContentHandler()
{
}

bool qm::AddressBookContentHandler::startElement(const WCHAR* pwszNamespaceURI,
												 const WCHAR* pwszLocalName,
												 const WCHAR* pwszQName,
												 const Attributes& attributes)
{
	if (wcscmp(pwszLocalName, L"addressBook") == 0) {
		if (state_ != STATE_ROOT)
			return false;
		if (attributes.getLength() != 0)
			return false;
		state_ = STATE_ADDRESSBOOK;
	}
	else if (wcscmp(pwszLocalName, L"entry") == 0) {
		if (state_ != STATE_ADDRESSBOOK)
			return false;
		if (attributes.getLength() != 0)
			return false;
		
		assert(!pEntry_.get());
		pEntry_.reset(new AddressBookEntry(0, 0, false));
		
		state_ = STATE_ENTRY;
	}
	else if (wcscmp(pwszLocalName, L"name") == 0) {
		if (state_ != STATE_ENTRY)
			return false;
		if (attributes.getLength() != 0)
			return false;
		state_ = STATE_NAME;
	}
	else if (wcscmp(pwszLocalName, L"sortKey") == 0) {
		if (state_ != STATE_ENTRY)
			return false;
		if (attributes.getLength() != 0)
			return false;
		state_ = STATE_SORTKEY;
	}
	else if (wcscmp(pwszLocalName, L"addresses") == 0) {
		if (state_ != STATE_ENTRY)
			return false;
		if (attributes.getLength() != 0)
			return false;
		state_ = STATE_ADDRESSES;
	}
	else if (wcscmp(pwszLocalName, L"address") == 0) {
		if (state_ != STATE_ADDRESSES)
			return false;
		
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
				return false;
		}
		
		AddressBookAddress::CategoryList listCategory;
		if (pwszCategory) {
			wstring_ptr wstrCategory(allocWString(pwszCategory));
			const WCHAR* p = wcstok(wstrCategory.get(), L",");
			while (p) {
				listCategory.push_back(pAddressBook_->getCategory(p));
				p = wcstok(0, L",");
			}
		}
		
		assert(pEntry_.get());
		assert(!pAddress_.get());
		pAddress_.reset(new AddressBookAddress(pEntry_.get(), 0, pwszAlias,
			listCategory, pwszComment, pwszCertificate, bRFC2822));
		
		state_ = STATE_ADDRESS;
	}
	else {
		return false;
	}
	
	return true;
}

bool qm::AddressBookContentHandler::endElement(const WCHAR* pwszNamespaceURI,
											   const WCHAR* pwszLocalName,
											   const WCHAR* pwszQName)
{
	if (wcscmp(pwszLocalName, L"addressBook") == 0) {
		assert(state_ == STATE_ADDRESSBOOK);
		state_ = STATE_ROOT;
	}
	else if (wcscmp(pwszLocalName, L"entry") == 0) {
		assert(state_ == STATE_ENTRY);
		
		assert(pEntry_.get());
		if (!pEntry_->getName() || pEntry_->getAddresses().empty())
			return false;
		
		pAddressBook_->addEntry(pEntry_);
		
		state_ = STATE_ADDRESSBOOK;
	}
	else if (wcscmp(pwszLocalName, L"name") == 0) {
		assert(state_ == STATE_NAME);
		
		assert(pEntry_.get());
		pEntry_->setName(buffer_.getCharArray());
		buffer_.remove();
		
		state_ = STATE_ENTRY;
	}
	else if (wcscmp(pwszLocalName, L"sortKey") == 0) {
		assert(state_ == STATE_SORTKEY);
		
		assert(pEntry_.get());
		pEntry_->setSortKey(buffer_.getCharArray());
		buffer_.remove();
		
		state_ = STATE_ENTRY;
	}
	else if (wcscmp(pwszLocalName, L"addresses") == 0) {
		assert(state_ == STATE_ADDRESSES);
		state_ = STATE_ENTRY;
	}
	else if (wcscmp(pwszLocalName, L"address") == 0) {
		assert(state_ == STATE_ADDRESS);
		
		assert(pAddress_.get());
		pAddress_->setAddress(buffer_.getCharArray(), pAddress_->isRFC2822());
		buffer_.remove();
		pEntry_->addAddress(pAddress_);
		
		state_ = STATE_ADDRESSES;
	}
	else {
		return false;
	}
	
	return true;
}

bool qm::AddressBookContentHandler::characters(const WCHAR* pwsz,
											   size_t nStart,
											   size_t nLength)
{
	if (state_ == STATE_NAME ||
		state_ == STATE_SORTKEY ||
		state_ == STATE_ADDRESS) {
		buffer_.append(pwsz + nStart, nLength);
	}
	else {
		const WCHAR* p = pwsz + nStart;
		for (size_t n = 0; n < nLength; ++n, ++p) {
			if (*p != L' ' && *p != L'\t' && *p != '\n')
				return false;
		}
	}
	
	return true;
}


/****************************************************************************
 *
 * AddressBookWriter
 *
 */

qm::AddressBookWriter::AddressBookWriter(Writer* pWriter,
										 const WCHAR* pwszEncoding) :
	handler_(pWriter, pwszEncoding)
{
}

qm::AddressBookWriter::~AddressBookWriter()
{
}

bool qm::AddressBookWriter::write(const AddressBook* pAddressBook)
{
	if (!handler_.startDocument())
		return false;
	if (!handler_.startElement(0, 0, L"addressBook", DefaultAttributes()))
		return false;
	
	const AddressBook::EntryList& listEntry = pAddressBook->getEntries();
	for (AddressBook::EntryList::const_iterator it = listEntry.begin(); it != listEntry.end(); ++it) {
		const AddressBookEntry* pEntry = *it;
		if (!pEntry->isExternal()) {
			if (!write(pEntry))
				return false;
		}
	}
	
	if (!handler_.endElement(0, 0, L"addressBook"))
		return false;
	if (!handler_.endDocument())
		return false;
	
	return true;
}

bool qm::AddressBookWriter::write(const AddressBookEntry* pEntry)
{
	if (!handler_.startElement(0, 0, L"entry", DefaultAttributes()))
		return false;
	
	if (!handler_.startElement(0, 0, L"name", DefaultAttributes()) ||
		!handler_.characters(pEntry->getName(), 0, wcslen(pEntry->getName())) ||
		!handler_.endElement(0, 0, L"name"))
		return false;
	
	if (pEntry->getSortKey()) {
		if (!handler_.startElement(0, 0, L"sortKey", DefaultAttributes()) ||
			!handler_.characters(pEntry->getSortKey(), 0, wcslen(pEntry->getSortKey())) ||
			!handler_.endElement(0, 0, L"sortKey"))
			return false;
	}
	
	if (!handler_.startElement(0, 0, L"addresses", DefaultAttributes()))
		return false;
	
	const AddressBookEntry::AddressList& listAddress = pEntry->getAddresses();
	for (AddressBookEntry::AddressList::const_iterator it = listAddress.begin(); it != listAddress.end(); ++it) {
		const AddressBookAddress* pAddress = *it;
		if (!write(pAddress))
			return false;
	}
	
	if (!handler_.endElement(0, 0, L"addresses"))
		return false;
	
	if (!handler_.endElement(0, 0, L"entry"))
		return false;
	
	return true;
}

bool qm::AddressBookWriter::write(const AddressBookAddress* pAddress)
{
	wstring_ptr wstrCategory = pAddress->getCategoryNames();
	const SimpleAttributes::Item items[] = {
		{ L"alias",					pAddress->getAlias(),						!pAddress->getAlias()				},
		{ L"category",				wstrCategory.get(),							!*wstrCategory.get()				},
		{ L"comment",				pAddress->getComment(),						!pAddress->getComment()				},
		{ L"rfc2822",				pAddress->isRFC2822() ? L"true" : L"false",	!pAddress->isRFC2822()				},
		{ L"certificate",			pAddress->getCertificate(),					!pAddress->getCertificate()			}
	};
	SimpleAttributes attrs(items, countof(items));
	return handler_.startElement(0, 0, L"address", attrs) &&
		handler_.characters(pAddress->getAddress(), 0, wcslen(pAddress->getAddress())) &&
		handler_.endElement(0, 0, L"address");
}


/****************************************************************************
 *
 * ExternalAddressBook
 *
 */

qm::ExternalAddressBook::~ExternalAddressBook()
{
}


/****************************************************************************
 *
 * ExternalAddressBookManager
 *
 */

qm::ExternalAddressBookManager::ExternalAddressBookManager(Profile* pProfile) :
	bModified_(true)
{
	bool bAddressOnly = pProfile->getInt(L"AddressBook", L"AddressOnly") != 0;
	
	wstring_ptr wstrExternals(pProfile->getString(L"AddressBook", L"Externals"));
	const WCHAR* p = wcstok(wstrExternals.get(), L" ");
	while (p) {
#ifndef _WIN32_WCE
		if (wcscmp(p, L"WAB") == 0) {
			std::auto_ptr<ExternalAddressBook> p(new WindowsAddressBook());
			init(p, bAddressOnly);
		}
		else if (wcscmp(p, L"Outlook") == 0) {
			std::auto_ptr<ExternalAddressBook> p(new OutlookAddressBook());
			init(p, bAddressOnly);
		}
#else
		if (wcscmp(p, L"PocketOutlook") == 0) {
#if _WIN32_WCE >= 0x420 && defined _WIN32_WCE_PSPC
			std::auto_ptr<ExternalAddressBook> p(new POOMAddressBook());
			if (!init(p, bAddressOnly))
#endif
			{
#if _WIN32_WCE < 0x500
				std::auto_ptr<ExternalAddressBook> p(new PocketOutlookAddressBook());
				init(p, bAddressOnly);
#endif
			}
		}
#endif
		
		p = wcstok(0, L" ");
	}
}

qm::ExternalAddressBookManager::~ExternalAddressBookManager()
{
	std::for_each(listAddressBook_.begin(), listAddressBook_.end(),
		boost::checked_deleter<ExternalAddressBook>());
}

bool qm::ExternalAddressBookManager::load(AddressBook* pAddressBook)
{
	std::for_each(listAddressBook_.begin(), listAddressBook_.end(),
		std::not1(
			std::bind2nd(
				std::mem_fun(&ExternalAddressBook::load),
				pAddressBook)));
	
	bModified_ = false;
	
	return true;
}

bool qm::ExternalAddressBookManager::isModified() const
{
	if (listAddressBook_.empty())
		return bModified_;
	else
		return std::find_if(listAddressBook_.begin(), listAddressBook_.end(),
			std::mem_fun(&ExternalAddressBook::isModified)) != listAddressBook_.end();
}

bool qm::ExternalAddressBookManager::init(std::auto_ptr<ExternalAddressBook> pAddressBook,
										  bool bAddressOnly)
{
	if (!pAddressBook->init(bAddressOnly))
		return false;
	
	listAddressBook_.push_back(pAddressBook.get());
	pAddressBook.release();
	
	return true;
}

#ifndef _WIN32_WCE

/****************************************************************************
 *
 * MAPIAddressBook
 *
 */

qm::MAPIAddressBook::MAPIAddressBook() :
	pAddrBook_(0),
	nConnection_(0),
	bAddressOnly_(false),
	bModified_(true)
{
}

qm::MAPIAddressBook::~MAPIAddressBook()
{
}

void qm::MAPIAddressBook::term()
{
	if (pAddrBook_) {
		if (nConnection_ != 0)
			pAddrBook_->Unadvise(nConnection_);
		pAddrBook_->Release();
		pAddrBook_ = 0;
	}
}

bool qm::MAPIAddressBook::load(AddressBook* pAddressBook)
{
	assert(pAddrBook_);
	
	Log log(InitThread::getInitThread().getLogger(), L"qm::MAPIAddressBook");
	
	HRESULT hr = S_OK;
	
	ULONG nEntrySize = 0;
	ENTRYID* pEntryId = 0;
	hr = pAddrBook_->GetPAB(&nEntrySize, &pEntryId);
	if (hr != S_OK) {
		log.errorf(L"IAddrBook::GetPAB failed: %08x.", hr);
		return false;
	}
	
	ULONG nType = 0;
	ComPtr<IMAPIContainer> pContainer;
	hr = pAddrBook_->OpenEntry(nEntrySize, pEntryId, &IID_IMAPIContainer,
		MAPI_BEST_ACCESS, &nType, reinterpret_cast<IUnknown**>(&pContainer));
	if (hr != S_OK) {
		log.errorf(L"IAddrBook::OpenEntry for PAB failed: %08x.", hr);
		return false;
	}
	
	ComPtr<IMAPITable> pTable;
	hr = pContainer->GetContentsTable(fMapiUnicode, &pTable);
	if (hr != S_OK) {
		log.errorf(L"IMAPIContainer::GetContentsTable failed: %08x.", hr);
		return false;
	}
	
	ULONG props[] = {
		PR_ENTRYID,
		PR_OBJECT_TYPE,
		PR_DISPLAY_NAME,
		PR_NORMALIZED_SUBJECT,
		PR_EMAIL_ADDRESS,
		PR_ADDRTYPE,
		PR_CONTACT_EMAIL_ADDRESSES
	};
	SizedSPropTagArray(countof(props), columns) = {
		countof(props)
	};
	memcpy(columns.aulPropTag, props, sizeof(props));
	enum {
		COLUMN_ENTRYID,
		COLUMN_OBJECT_TYPE,
		COLUMN_DISPLAY_NAME,
		COLUMN_NORMALIZED_SUBJECT,
		COLUMN_EMAIL_ADDRESS,
		COLUMN_ADDRTYPE,
		COLUMN_CONTACT_EMAIL_ADDRESSES
	};
	
	hr = pTable->SetColumns(reinterpret_cast<LPSPropTagArray>(&columns), 0);
	if (hr != S_OK) {
		log.errorf(L"IMAPITable::SetColumns failed: %08x.", hr);
		return false;
	}
	
	LONG nRowsSought = 0;
	hr = pTable->SeekRow(BOOKMARK_BEGINNING, 0, &nRowsSought);
	if (hr != S_OK) {
		log.errorf(L"IMAPITable::SeekRow failed: %08x.", hr);
		return false;
	}
	
	while (true) {
		SRowSet* pSRowSet = 0;
		if (pTable->QueryRows(1, 0, &pSRowSet) != S_OK)
			break;
		RowSetDeleter deleter(this, pSRowSet);
		if (pSRowSet->cRows == 0)
			break;
		assert(pSRowSet->cRows == 1);
		
		SRow* pRow = pSRowSet->aRow;
		
		LPCTSTR ptszName = 0;
		const SPropValue& valueSubject = pRow->lpProps[COLUMN_NORMALIZED_SUBJECT];
		if (PROP_TYPE(valueSubject.ulPropTag) == PT_TSTRING) {
			ptszName = valueSubject.Value.LPSZ;
		}
		else {
			const SPropValue& valueName = pRow->lpProps[COLUMN_DISPLAY_NAME];
			if (PROP_TYPE(valueName.ulPropTag) != PT_TSTRING) {
				log.warn(L"Skipping non-string PR_DISPLAY_NAME.");
				continue;
			}
			ptszName = valueName.Value.LPSZ;
		}
		T2W(ptszName, pwszName);
		
		std::auto_ptr<AddressBookEntry> pEntry(new AddressBookEntry(pwszName, 0, true));
		
		ULONG nEntrySize = pRow->lpProps[COLUMN_ENTRYID].Value.bin.cb;
		ENTRYID* pEntryId = reinterpret_cast<ENTRYID*>(
			pRow->lpProps[COLUMN_ENTRYID].Value.bin.lpb);
		LONG nObjectType = pRow->lpProps[COLUMN_OBJECT_TYPE].Value.l;
		if (nObjectType == MAPI_MAILUSER) {
			if (PROP_TYPE(pRow->lpProps[COLUMN_CONTACT_EMAIL_ADDRESSES].ulPropTag) == PT_MV_TSTRING) {
				const SPropValue& value = pRow->lpProps[COLUMN_CONTACT_EMAIL_ADDRESSES];
#ifdef UNICODE
				typedef SWStringArray STStringArray;
#else
				typedef SLPSTRArray STStringArray;
#endif
				const STStringArray& array = value.Value.MVSZ;
				for (unsigned int n = 0; n < array.cValues; ++n) {
					T2W(array.LPPSZ[n], pwszAddress);
					std::auto_ptr<AddressBookAddress> pAddress(
						new AddressBookAddress(pEntry.get(),
							pwszAddress,
							static_cast<const WCHAR*>(0),
							AddressBookAddress::CategoryList(),
							static_cast<const WCHAR*>(0),
							static_cast<const WCHAR*>(0), bAddressOnly_));
					pEntry->addAddress(pAddress);
				}
			}
			else if (PROP_TYPE(pRow->lpProps[COLUMN_EMAIL_ADDRESS].ulPropTag) == PT_TSTRING &&
				PROP_TYPE(pRow->lpProps[COLUMN_ADDRTYPE].ulPropTag) == PT_TSTRING) {
				const SPropValue& valueType = pRow->lpProps[COLUMN_ADDRTYPE];
				if (_tcscmp(valueType.Value.LPSZ, _T("SMTP")) == 0) {
					const SPropValue& value = pRow->lpProps[COLUMN_EMAIL_ADDRESS];
					T2W(value.Value.LPSZ, pwszAddress);
					std::auto_ptr<AddressBookAddress> pAddress(
						new AddressBookAddress(pEntry.get(),
							pwszAddress,
							static_cast<const WCHAR*>(0),
							AddressBookAddress::CategoryList(),
							static_cast<const WCHAR*>(0),
							static_cast<const WCHAR*>(0), bAddressOnly_));
					pEntry->addAddress(pAddress);
				}
			}
			else {
				log.warnf(L"Skipping unknown PR_CONTACT_EMAIL_ADDRESSES or PR_EMAIL_ADDRESS: %08x, %08x.",
					pRow->lpProps[COLUMN_CONTACT_EMAIL_ADDRESSES].ulPropTag,
					pRow->lpProps[COLUMN_EMAIL_ADDRESS].ulPropTag);
			}
		}
		else if (nObjectType == MAPI_DISTLIST) {
			ULONG nType = 0;
			ComPtr<IDistList> pDistList;
			hr = pContainer->OpenEntry(nEntrySize, pEntryId, &IID_IDistList,
				MAPI_BEST_ACCESS, &nType, reinterpret_cast<IUnknown**>(&pDistList));
			if (hr == S_OK) {
				wstring_ptr wstrDistList = expandDistList(pContainer.get(), pDistList.get());
				if (wstrDistList.get()) {
					std::auto_ptr<AddressBookAddress> pAddress(
						new AddressBookAddress(pEntry.get(),
							wstrDistList.get(),
							static_cast<const WCHAR*>(0),
							AddressBookAddress::CategoryList(),
							static_cast<const WCHAR*>(0),
							static_cast<const WCHAR*>(0), true));
					pEntry->addAddress(pAddress);
				}
			}
			else {
				log.warnf(L"Skipping distribution list which cannot be opened: %08x.", hr);
			}
		}
		
		if (!pEntry->getAddresses().empty())
			pAddressBook->addEntry(pEntry);
	}
	
	bModified_ = false;
	
	return true;
}

bool qm::MAPIAddressBook::isModified()
{
	return bModified_;
}

bool qm::MAPIAddressBook::init(IAddrBook* pAddrBook,
							   bool bAddressOnly)
{
	Log log(InitThread::getInitThread().getLogger(), L"qm::MAPIAddressBook");
	
	HRESULT hr = S_OK;
	
	ULONG nEntrySize = 0;
	ENTRYID* pEntryId = 0;
	hr = pAddrBook->GetPAB(&nEntrySize, &pEntryId);
	if (hr != S_OK) {
		log.errorf(L"IAddrBook::GetPAB failed: %08x.", hr);
		return false;
	}
	
	std::auto_ptr<IMAPIAdviseSinkImpl> pAdviseSink(new IMAPIAdviseSinkImpl(this));
	if (pAddrBook->Advise(nEntrySize, pEntryId,
		0xffffffff, pAdviseSink.get(), &nConnection_) == S_OK)
		pAdviseSink.release();
	
	pAddrBook_ = pAddrBook;
	pAddrBook_->AddRef();
	
	bAddressOnly_ = bAddressOnly;
	
	return true;
}

wstring_ptr qm::MAPIAddressBook::expandDistList(IMAPIContainer* pContainer,
												IDistList* pDistList) const
{
	Log log(InitThread::getInitThread().getLogger(), L"qm::MAPIAddressBook");
	
	HRESULT hr = S_OK;
	
	ComPtr<IMAPITable> pTable;
	hr = pDistList->GetContentsTable(fMapiUnicode, &pTable);
	if (hr != S_OK)
		return 0;
	
	StringBuffer<WSTRING> buf;
	
	ULONG props[] = {
		PR_ENTRYID,
		PR_DISPLAY_NAME,
		PR_NORMALIZED_SUBJECT,
		PR_EMAIL_ADDRESS,
		PR_ADDRTYPE
	};
	SizedSPropTagArray(countof(props), columns) = {
		countof(props)
	};
	memcpy(columns.aulPropTag, props, sizeof(props));
	enum {
		COLUMN_ENTRYID,
		COLUMN_DISPLAY_NAME,
		COLUMN_NORMALIZED_SUBJECT,
		COLUMN_EMAIL_ADDRESS,
		COLUMN_ADDRTYPE
	};
	
	hr = pTable->SetColumns(reinterpret_cast<LPSPropTagArray>(&columns), 0);
	if (hr != S_OK)
		return 0;
	
	LONG nRowsSought = 0;
	hr = pTable->SeekRow(BOOKMARK_BEGINNING, 0, &nRowsSought);
	if (hr != S_OK)
		return 0;
	
	while (true) {
		SRowSet* pSRowSet = 0;
		if (pTable->QueryRows(1, 0, &pSRowSet) != S_OK)
			break;
		RowSetDeleter deleter(this, pSRowSet);
		if (pSRowSet->cRows == 0)
			break;
		assert(pSRowSet->cRows == 1);
		
		SRow* pRow = pSRowSet->aRow;
		
		const SPropValue& valueType = pRow->lpProps[COLUMN_ADDRTYPE];
		if (PROP_TYPE(valueType.ulPropTag) != PT_TSTRING ||
			_tcscmp(valueType.Value.LPSZ, _T("SMTP")) != 0)
			continue;
		
		wstring_ptr wstrName;
		if (!bAddressOnly_) {
			ULONG nEntrySize = pRow->lpProps[COLUMN_ENTRYID].Value.bin.cb;
			ENTRYID* pEntryId = reinterpret_cast<ENTRYID*>(
				pRow->lpProps[COLUMN_ENTRYID].Value.bin.lpb);
			ComPtr<IMailUser> pMailUser;
			ULONG nType = 0;
			hr = pContainer->OpenEntry(nEntrySize, pEntryId, &IID_IMailUser,
				MAPI_BEST_ACCESS, &nType, reinterpret_cast<IUnknown**>(&pMailUser));
			if (hr == S_OK) {
				SPropValue* pValue = 0;
				ULONG nValues = 0;
				hr = pMailUser->GetProps(reinterpret_cast<LPSPropTagArray>(&columns),
					fMapiUnicode, &nValues, &pValue);
				if (hr == S_OK) {
					const SPropValue& valueSubject = pValue[COLUMN_NORMALIZED_SUBJECT];
					if (PROP_TYPE(valueSubject.ulPropTag) == PT_TSTRING)
						wstrName = tcs2wcs(valueSubject.Value.LPSZ);
				}
				freeBuffer(pValue);
			}
			
			if (!wstrName.get()) {
				const SPropValue& valueName = pRow->lpProps[COLUMN_DISPLAY_NAME];
				if (PROP_TYPE(valueName.ulPropTag) != PT_TSTRING)
					continue;
				wstrName = tcs2wcs(valueName.Value.LPSZ);
			}
		}
		
		const SPropValue& valueAddress = pRow->lpProps[COLUMN_EMAIL_ADDRESS];
		if (PROP_TYPE(valueAddress.ulPropTag) != PT_TSTRING)
			continue;
		T2W(valueAddress.Value.LPSZ, pwszAddress);
		
		if (buf.getLength() != 0)
			buf.append(L", ");
		
		if (bAddressOnly_)
			buf.append(pwszAddress);
		else
			buf.append(AddressParser(wstrName.get(), pwszAddress).getValue().get());
	}
	
	if (buf.getLength() == 0)
		return 0;
	
	return buf.getString();
}


/****************************************************************************
 *
 * MAPIAddressBook::IMAPIAdviseSinkImpl
 *
 */

qm::MAPIAddressBook::IMAPIAdviseSinkImpl::IMAPIAdviseSinkImpl(MAPIAddressBook* pAddressBook) :
	nRef_(0),
	pAddressBook_(pAddressBook)
{
}

qm::MAPIAddressBook::IMAPIAdviseSinkImpl::~IMAPIAdviseSinkImpl()
{
}

STDMETHODIMP qm::MAPIAddressBook::IMAPIAdviseSinkImpl::QueryInterface(REFIID riid,
																	  void** ppv)
{
	if (riid == IID_IUnknown || riid == IID_IMAPIAdviseSink)
		*ppv = static_cast<IMAPIAdviseSink*>(this);
	else
		return E_NOINTERFACE;
	
	AddRef();
	
	return S_OK;
}

STDMETHODIMP_(ULONG) qm::MAPIAddressBook::IMAPIAdviseSinkImpl::AddRef()
{
	return ++nRef_;
}

STDMETHODIMP_(ULONG) qm::MAPIAddressBook::IMAPIAdviseSinkImpl::Release()
{
	if (--nRef_ == 0) {
		delete this;
		return 0;
	}
	return nRef_;
}

STDMETHODIMP_(ULONG) qm::MAPIAddressBook::IMAPIAdviseSinkImpl::OnNotify(ULONG cNotif,
																		LPNOTIFICATION lpNotifications)
{
	pAddressBook_->bModified_ = true;
	return S_OK;
}


/****************************************************************************
 *
 * MAPIAddressBook::RowSetDeleter
 *
 */

qm::MAPIAddressBook::RowSetDeleter::RowSetDeleter(const MAPIAddressBook* pAddressBook,
												  SRowSet* pSRowSet) :
	pAddressBook_(pAddressBook),
	pSRowSet_(pSRowSet)
{
}

qm::MAPIAddressBook::RowSetDeleter::~RowSetDeleter()
{
	for (ULONG n = 0; n < pSRowSet_->cRows; ++n)
		pAddressBook_->freeBuffer(pSRowSet_->aRow[n].lpProps);
	pAddressBook_->freeBuffer(pSRowSet_);
}


/****************************************************************************
 *
 * WindowsAddressBook
 *
 */

qm::WindowsAddressBook::WindowsAddressBook() :
	hInstWAB_(0),
	pWABObject_(0)
{
}

qm::WindowsAddressBook::~WindowsAddressBook()
{
	term();
}

bool qm::WindowsAddressBook::init(bool bAddressOnly)
{
	Registry reg(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\WAB\\DLLPath", true);
	if (!reg)
		return false;
	
	wstring_ptr wstrPath;
	if (!reg.getValue(L"", &wstrPath))
		return false;
	
	W2T(wstrPath.get(), ptszPath);
	hInstWAB_ = ::LoadLibrary(ptszPath);
	if (!hInstWAB_)
		return false;
	
	LPWABOPEN pfnWABOpen = reinterpret_cast<LPWABOPEN>(
		::GetProcAddress(hInstWAB_, "WABOpen"));
	if (!pfnWABOpen)
		return false;
	
	ComPtr<IAddrBook> pAddrBook;
	ComPtr<IWABObject> pWABObject;
	WAB_PARAM param = { sizeof(param) };
	if ((*pfnWABOpen)(&pAddrBook, &pWABObject, &param, 0) != S_OK)
		return false;
	
	if (!MAPIAddressBook::init(pAddrBook.get(), bAddressOnly))
		return false;
	
	pWABObject_ = pWABObject.release();
	
	return true;
}

void qm::WindowsAddressBook::term()
{
	MAPIAddressBook::term();
	if (pWABObject_) {
		pWABObject_->Release();
		pWABObject_ = 0;
	}
	if (hInstWAB_) {
		::FreeLibrary(hInstWAB_);
		hInstWAB_ = 0;
	}
}

void qm::WindowsAddressBook::freeBuffer(void* pBuffer) const
{
	pWABObject_->FreeBuffer(pBuffer);
}


/****************************************************************************
 *
 * OutlookAddressBook
 *
 */

qm::OutlookAddressBook::OutlookAddressBook() :
	hInst_(0),
	pfnMAPIFreeBuffer_(0)
{
}

qm::OutlookAddressBook::~OutlookAddressBook()
{
	term();
}

bool qm::OutlookAddressBook::init(bool bAddressOnly)
{
	Log log(InitThread::getInitThread().getLogger(), L"qm::OutlookAddressBook");
	
	Registry reg(HKEY_LOCAL_MACHINE, L"Software\\Clients\\Mail\\Microsoft Outlook", true);
	if (!reg) {
		log.info(L"Registry key for Outlook is not found.");
		return false;
	}
	
	wstring_ptr wstrPath;
	if (!reg.getValue(L"DLLPathEx", &wstrPath)) {
		log.error(L"Failed to get DLL path.");
		return false;
	}
	log.debugf(L"DLL path: %s", wstrPath.get());
	
	W2T(wstrPath.get(), ptszPath);
	hInst_ = ::LoadLibrary(ptszPath);
	if (!hInst_) {
		log.error(L"Failed to load DLL.");
		return false;
	}
	
	HRESULT hr = S_OK;
	
	LPMAPIINITIALIZE pfnMAPIInitialize = reinterpret_cast<LPMAPIINITIALIZE>(
		::GetProcAddress(hInst_, "MAPIInitialize"));
	if (!pfnMAPIInitialize)
		return false;
	hr = (*pfnMAPIInitialize)(0);
	if (hr != S_OK) {
		log.errorf(L"MAPIInitialize failed: %08x.", hr);
		return false;
	}
	
	LPMAPILOGONEX pfnMAPILogonEx = reinterpret_cast<LPMAPILOGONEX>(
		::GetProcAddress(hInst_, "MAPILogonEx"));
	if (!pfnMAPILogonEx)
		return false;
	
	ComPtr<IMAPISession> pSession;
	FLAGS flags = MAPI_EXTENDED | MAPI_USE_DEFAULT | MAPI_LOGON_UI | MAPI_NO_MAIL | fMapiUnicode;
	hr = (*pfnMAPILogonEx)(0, 0, 0, flags, &pSession);
	if (hr != S_OK) {
		log.errorf(L"MAPILogonEx failed: %08x.", hr);
		return false;
	}
	
	ComPtr<IAddrBook> pAddrBook;
	hr = pSession->OpenAddressBook(0, 0, 0, &pAddrBook);
	if (hr != S_OK) {
		log.errorf(L"IMAPISession::OpenAddressBook failed: %08x.", hr);
		return false;
	}
	
	pfnMAPIFreeBuffer_ = reinterpret_cast<LPMAPIFREEBUFFER>(
		::GetProcAddress(hInst_, "MAPIFreeBuffer"));
	if (!pfnMAPIFreeBuffer_)
		return false;
	
	if (!MAPIAddressBook::init(pAddrBook.get(), bAddressOnly))
		return false;
	
	return true;
}

void qm::OutlookAddressBook::term()
{
	MAPIAddressBook::term();
	if (hInst_) {
		LPMAPIUNINITIALIZE pfnMAPIUninitialize = reinterpret_cast<LPMAPIUNINITIALIZE>(
			::GetProcAddress(hInst_, "MAPIUninitialize"));
		if (pfnMAPIUninitialize)
			(*pfnMAPIUninitialize)();
		::FreeLibrary(hInst_);
		hInst_ = 0;
	}
}

void qm::OutlookAddressBook::freeBuffer(void* pBuffer) const
{
	(*pfnMAPIFreeBuffer_)(pBuffer);
}

#else // _WIN32_WCE

#if _WIN32_WCE >= 0x420 && defined _WIN32_WCE_PSPC

/****************************************************************************
 *
 * POOMAddressBook
 *
 */

qm::POOMAddressBook::POOMAddressBook() :
	pPOutlookApp_(0),
#if _WIN32_WCE >= 0x500
	pNotificationWindow_(0),
#endif
	bAddressOnly_(false),
	bModified_(true)
{
}

qm::POOMAddressBook::~POOMAddressBook()
{
	term();
}

bool qm::POOMAddressBook::init(bool bAddressOnly)
{
	HRESULT hr = ::CoCreateInstance(CLSID_Application, 0, CLSCTX_INPROC_SERVER,
		IID_IPOutlookApp, reinterpret_cast<void**>(&pPOutlookApp_));
	if (FAILED(hr))
		return false;
	
	HWND hwnd = 0;
#if _WIN32_WCE >= 0x500
	std::auto_ptr<NotificationWindow> pWindow(new NotificationWindow(this));
	if (!pWindow->create(L"QmPOOMAddressBookNotificationWindow",
		0, WS_POPUP, 0, 0, 0, 0, 0, 0, 0, 0, 0))
		return false;
	pNotificationWindow_ = pWindow.release();
	hwnd = pNotificationWindow_->getHandle();
#endif
	
	hr = pPOutlookApp_->Logon(reinterpret_cast<long>(hwnd));
	if (FAILED(hr))
		return false;
	
#if _WIN32_WCE >= 0x500
	registerNotification();
#endif
	
	bAddressOnly_ = bAddressOnly;
	
	return true;
}

void qm::POOMAddressBook::term()
{
	if (pPOutlookApp_) {
		pPOutlookApp_->Logoff();
		pPOutlookApp_->Release();
		pPOutlookApp_ = 0;
	}
	
#if _WIN32_WCE >= 0x500
	if (pNotificationWindow_) {
		pNotificationWindow_->destroyWindow();
		pNotificationWindow_ = 0;
	}
#endif
}

bool qm::POOMAddressBook::load(AddressBook* pAddressBook)
{
	ComPtr<IFolder> pFolder;
	HRESULT hr = pPOutlookApp_->GetDefaultFolder(olFolderContacts, &pFolder);
	if (FAILED(hr))
		return false;
	
	ComPtr<IPOutlookItemCollection> pItems;
	hr = pFolder->get_Items(&pItems);
	if (FAILED(hr))
		return false;
	
	int nCount = 0;
	hr = pItems->get_Count(&nCount);
	if (FAILED(hr))
		return false;
	
	for (int n = 1; n <= nCount; ++n) {
		ComPtr<IDispatch> pItem;
		hr = pItems->Item(n, &pItem);
		if (SUCCEEDED(hr)) {
			ComPtr<IContact> pContact;
			hr = pItem->QueryInterface(IID_IContact, reinterpret_cast<void**>(&pContact));
			if (SUCCEEDED(hr)) {
				typedef HRESULT (WINAPI IContact::*PFN)(BSTR*);
				
				PFN pfnProps[] = {
					&IContact::get_FirstName,
					&IContact::get_LastName,
					&IContact::get_YomiFirstName,
					&IContact::get_YomiLastName
				};
				BSTRPtr bstrProps[countof(pfnProps)];
				for (int n = 0; n < countof(pfnProps) && SUCCEEDED(hr); ++n) {
					hr = (pContact.get()->*pfnProps[n])(&bstrProps[n]);
					if (n > 1 && hr == E_NOTIMPL)
						hr = S_OK;
				}
				if (SUCCEEDED(hr)) {
					const WCHAR* pwszYomi[2] = { 0 };
					for (int n = 0; n < countof(pwszYomi); ++n) {
						pwszYomi[n] = bstrProps[n + 2].get();
						if (!pwszYomi[n])
							pwszYomi[n] = bstrProps[n].get();
					}
#ifdef JAPAN
					wstring_ptr wstrName(concat(bstrProps[1].get(), L" ", bstrProps[0].get()));
					wstring_ptr wstrSortKey(concat(pwszYomi[1], L" ", pwszYomi[0]));
#else
					wstring_ptr wstrName(concat(bstrProps[0].get(), L" ", bstrProps[1].get()));
					wstring_ptr wstrSortKey(concat(pwszYomi[0], L" ", pwszYomi[1]));
#endif
					
					AddressBookAddress::CategoryList listCategory;
					getCategories(pAddressBook, pContact.get(), &listCategory);
					
					std::auto_ptr<AddressBookEntry> pEntry(new AddressBookEntry(
						wstrName.get(), wstrSortKey.get(), true));
					
					PFN pfn[] = {
						&IContact::get_Email1Address,
						&IContact::get_Email2Address,
						&IContact::get_Email3Address
					};
					for (int n = 0; n < countof(pfn); ++n) {
						BSTRPtr bstrAddress;
						hr = (pContact.get()->*pfn[n])(&bstrAddress);
						if (SUCCEEDED(hr) && *bstrAddress.get()) {
							std::auto_ptr<AddressBookAddress> pAddress(
								new AddressBookAddress(pEntry.get(),
									bstrAddress.get(),
									static_cast<const WCHAR*>(0),
									listCategory,
									static_cast<const WCHAR*>(0),
									static_cast<const WCHAR*>(0), bAddressOnly_));
							pEntry->addAddress(pAddress);
						}
					}
					
					if (!pEntry->getAddresses().empty())
						pAddressBook->addEntry(pEntry);
				}
			}
		}
	}
	
	bModified_ = false;
	
	return true;
}

bool qm::POOMAddressBook::isModified()
{
	return bModified_;
}

#if _WIN32_WCE >= 0x500
bool qm::POOMAddressBook::registerNotification()
{
	ComPtr<IFolder> pFolder;
	HRESULT hr = pPOutlookApp_->GetDefaultFolder(olFolderContacts, &pFolder);
	if (FAILED(hr))
		return false;
	
	ComPtr<IItem> pItem;
	hr = pFolder->QueryInterface(IID_IItem, reinterpret_cast<void**>(&pItem));
	if (FAILED(hr))
		return false;
	
	CEPROPVAL val = { PIMPR_FOLDERNOTIFICATIONS };
	val.val.ulVal = PIMFOLDERNOTIFICATION_ALL;
	hr = pItem->SetProps(0, 1, &val);
	if (FAILED(hr))
		return false;
	
	return true;
}
#endif

void qm::POOMAddressBook::getCategories(AddressBook* pAddressBook,
										IContact* pContact,
										AddressBookAddress::CategoryList* pList)
{
	assert(pAddressBook);
	assert(pContact);
	assert(pList);
	
	BSTRPtr bstrCategories;
	HRESULT hr = pContact->get_Categories(&bstrCategories);
	if (FAILED(hr) || !*bstrCategories.get())
		return;
	
	const WCHAR* p = wcstok(bstrCategories.get(), L",");
	while (p) {
		wstring_ptr wstr(trim(p));
		if (*wstr.get())
			pList->push_back(pAddressBook->getCategory(wstr.get()));
		p = wcstok(0, L",");
	}
}


#if _WIN32_WCE >= 0x500
/****************************************************************************
 *
 * POOMAddressBook::NotificationWindow
 *
 */

qm::POOMAddressBook::NotificationWindow::NotificationWindow(POOMAddressBook* pAddressBook) :
	WindowBase(true),
	pAddressBook_(pAddressBook)
{
	setWindowHandler(this, false);
}

POOMAddressBook::NotificationWindow::~NotificationWindow()
{
}

LRESULT POOMAddressBook::NotificationWindow::windowProc(UINT uMsg,
														WPARAM wParam,
														LPARAM lParam)
{
	BEGIN_MESSAGE_HANDLER()
		HANDLE_MESSAGE(PIM_ITEM_CREATED_LOCAL, onPimFolderNotification)
		HANDLE_MESSAGE(PIM_ITEM_DELETED_LOCAL, onPimFolderNotification)
		HANDLE_MESSAGE(PIM_ITEM_CHANGED_LOCAL, onPimFolderNotification)
		HANDLE_MESSAGE(PIM_ITEM_CREATED_REMOTE, onPimFolderNotification)
		HANDLE_MESSAGE(PIM_ITEM_DELETED_REMOTE, onPimFolderNotification)
		HANDLE_MESSAGE(PIM_ITEM_CHANGED_REMOTE, onPimFolderNotification)
	END_MESSAGE_HANDLER()
	return DefaultWindowHandler::windowProc(uMsg, wParam, lParam);
}

LRESULT POOMAddressBook::NotificationWindow::onPimFolderNotification(WPARAM wParam,
																	 LPARAM lParam)
{
	pAddressBook_->bModified_ = true;
	return 0;
}
#endif

#endif // _WIN32_WCE >= 0x420 && defined _WIN32_WCE_PSPC


#if _WIN32_WCE < 0x500
/****************************************************************************
 *
 * PocketOutlookAddressBook
 *
 */

qm::PocketOutlookAddressBook::PocketOutlookAddressBook() :
	hCategoryDB_(0),
	hContactsDB_(0),
	pNotificationWindow_(0),
	bAddressOnly_(false),
	bModified_(true)
{
}

qm::PocketOutlookAddressBook::~PocketOutlookAddressBook()
{
	term();
}

bool qm::PocketOutlookAddressBook::init(bool bAddressOnly)
{
	std::auto_ptr<NotificationWindow> pWindow(new NotificationWindow(this));
	if (!pWindow->create(L"QmPocketOutlookAddressBookNotificationWindow",
		0, WS_POPUP, 0, 0, 0, 0, 0, 0, 0, 0, 0))
		return false;
	pNotificationWindow_ = pWindow.release();
	
	CEOID oidCategory = 0;
	hCategoryDB_ = ::CeOpenDatabase(&oidCategory,
		L"\\Categories Database", 0, CEDB_AUTOINCREMENT,
		pNotificationWindow_->getHandle());
	
	CEOID oidContact = 0;
	hContactsDB_ = ::CeOpenDatabase(&oidContact,
		L"Contacts Database", 0, CEDB_AUTOINCREMENT,
		pNotificationWindow_->getHandle());
	if (!hContactsDB_)
		return false;
	
	bAddressOnly_ = bAddressOnly;
	
	return true;
}

void qm::PocketOutlookAddressBook::term()
{
	if (hCategoryDB_) {
		::CloseHandle(hCategoryDB_);
		hCategoryDB_ = 0;
	}
	if (hContactsDB_) {
		::CloseHandle(hContactsDB_);
		hContactsDB_ = 0;
	}
	if (pNotificationWindow_) {
		pNotificationWindow_->destroyWindow();
		pNotificationWindow_ = 0;
	}
}

bool qm::PocketOutlookAddressBook::load(AddressBook* pAddressBook)
{
	assert(hContactsDB_);
	
	typedef std::vector<std::pair<unsigned int, WSTRING> > CategoryMap;
	CategoryMap mapCategory;
	CONTAINER_DELETER(deleter, mapCategory,
		boost::bind(&freeWString, boost::bind(&Map::value_type::second, _1)));
	
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
				wstring_ptr wstrCategory(allocWString(pVal[1].val.lpwstr));
				mapCategory.push_back(CategoryMap::value_type(nId, wstrCategory.get()));
				wstrCategory.release();
			}
		}
	}
	
	std::sort(mapCategory.begin(), mapCategory.end(),
		boost::bind(&CategoryMap::value_type::first, _1) <
		boost::bind(&CategoryMap::value_type::first, _2));
	
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
		wstring_ptr wstrSortKey(concat(pwszYomiLastName, L" ", pwszYomiFirstName));
#else
		wstring_ptr wstrSortKey(concat(wstrSurName.get(), L", ", wstrGivenName()));
#endif
		
#ifdef JAPAN
		wstring_ptr wstrName(concat(pwszSurName, L" ", pwszGivenName));
#else
		wstring_ptr wstrName(concat(pwszGivenName, L" ", pwszSurName));
#endif
		
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
								boost::bind(&CategoryMap::value_type::first, _1) <
								boost::bind(&CategoryMap::value_type::first, _2));
							if (it != mapCategory.end() && (*it).first == nId)
								listCategory.push_back(pAddressBook->getCategory((*it).second));
						}
					}
				}
			}
		}
		
		std::auto_ptr<AddressBookEntry> pEntry(new AddressBookEntry(
			wstrName.get(), wstrSortKey.get(), true));
		
		for (int nAddress = 2; nAddress < 5; ++nAddress) {
			if (pVal[nAddress].wFlags != CEDB_PROPNOTFOUND) {
				std::auto_ptr<AddressBookAddress> pAddress(
					new AddressBookAddress(pEntry.get(), pVal[nAddress].val.lpwstr,
						0, listCategory, 0, 0, bAddressOnly_));
				pEntry->addAddress(pAddress);
			}
		}
		
		pAddressBook->addEntry(pEntry);
	}
	
	bModified_ = false;
	
	return true;
}

bool qm::PocketOutlookAddressBook::isModified()
{
	return bModified_;
}


/****************************************************************************
 *
 * PocketOutlookAddressBook::NotificationWindow
 *
 */

qm::PocketOutlookAddressBook::NotificationWindow::NotificationWindow(PocketOutlookAddressBook* pAddressBook) :
	WindowBase(true),
	pAddressBook_(pAddressBook)
{
	setWindowHandler(this, false);
}

PocketOutlookAddressBook::NotificationWindow::~NotificationWindow()
{
}

LRESULT PocketOutlookAddressBook::NotificationWindow::windowProc(UINT uMsg,
																 WPARAM wParam,
																 LPARAM lParam)
{
	BEGIN_MESSAGE_HANDLER()
		HANDLE_MESSAGE(DB_CEOID_CREATED, onDBNotification)
		HANDLE_MESSAGE(DB_CEOID_RECORD_DELETED, onDBNotification)
		HANDLE_MESSAGE(DB_CEOID_CHANGED, onDBNotification)
	END_MESSAGE_HANDLER()
	return DefaultWindowHandler::windowProc(uMsg, wParam, lParam);
}

LRESULT PocketOutlookAddressBook::NotificationWindow::onDBNotification(WPARAM wParam,
																	   LPARAM lParam)
{
	pAddressBook_->bModified_ = true;
	return 0;
}
#endif // _WIN32_WCE <= 0x500

#endif // _WIN32_WCE
