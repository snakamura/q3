/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmapplication.h>
#include <qmextensions.h>

#include <qsconv.h>
#include <qserror.h>
#include <qsmime.h>
#include <qsosutil.h>
#include <qsnew.h>

#include <algorithm>

#include "addressbook.h"
#include "security.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * AddressBook
 *
 */

qm::AddressBook::AddressBook(const Security* pSecurity, QSTATUS* pstatus) :
	pSecurity_(pSecurity),
	pSMIMECallback_(0)
{
	DECLARE_QSTATUS();
	
	SYSTEMTIME st;
	::GetSystemTime(&st);
	::SystemTimeToFileTime(&st, &ft_);
	
	status = newQsObject(this, &pSMIMECallback_);
	CHECK_QSTATUS_SET(pstatus);
}

qm::AddressBook::~AddressBook()
{
	clear();
	delete pSMIMECallback_;
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

QSTATUS qm::AddressBook::getCategories(const CategoryList** ppList)
{
	assert(ppList);
	
	DECLARE_QSTATUS();
	
	*ppList = 0;
	
	status = load();
	CHECK_QSTATUS();
	
	*ppList = &listCategory_;
	
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

SMIMECallback* qm::AddressBook::getSMIMECallback() const
{
	return pSMIMECallback_;
}

QSTATUS qm::AddressBook::addEntry(AddressBookEntry* pEntry)
{
	return STLWrapper<EntryList>(listEntry_).push_back(pEntry);
}

QSTATUS qm::AddressBook::load()
{
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrPath;
	status = Application::getApplication().getProfilePath(
		Extensions::ADDRESSBOOK, &wstrPath);
	CHECK_QSTATUS();
	
	W2T(wstrPath.get(), ptszPath);
	AutoHandle hFile(::CreateFile(ptszPath, GENERIC_READ, 0, 0,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0));
	if (hFile.get()) {
		FILETIME ft;
		::GetFileTime(hFile.get(), 0, 0, &ft);
		hFile.close();
		
		if (::CompareFileTime(&ft, &ft_) != 0) {
			clear();
			
			XMLReader reader(&status);
			CHECK_QSTATUS();
			AddressBookContentHandler handler(this, &status);
			CHECK_QSTATUS();
			reader.setContentHandler(&handler);
			status = reader.parse(wstrPath.get());
			CHECK_QSTATUS();
			
			ft_ = ft;
		}
	}
	else {
		clear();
	}
	
	EntryList::iterator itE = listEntry_.begin();
	while (itE != listEntry_.end()) {
		const AddressBookEntry::AddressList& l = (*itE)->getAddresses();
		AddressBookEntry::AddressList::const_iterator itA = l.begin();
		while (itA != l.end()) {
			const WCHAR* pwszCategory = (*itA)->getCategory();
			if (pwszCategory) {
				CategoryList::iterator itC = std::lower_bound(
					listCategory_.begin(), listCategory_.end(),
					pwszCategory, string_less<WCHAR>());
				if (itC == listCategory_.end() || wcscmp(*itC, pwszCategory) != 0) {
					string_ptr<WSTRING> wstrCategory(allocWString(pwszCategory));
					if (!wstrCategory.get())
						return QSTATUS_OUTOFMEMORY;
					CategoryList::iterator p;
					status = STLWrapper<CategoryList>(listCategory_).insert(
						itC, wstrCategory.get(), &p);
					CHECK_QSTATUS();
					wstrCategory.release();
				}
			}
			++itA;
		}
		++itE;
	}
	
	return QSTATUS_SUCCESS;
}

void qm::AddressBook::clear()
{
	std::for_each(listEntry_.begin(),
		listEntry_.end(), deleter<AddressBookEntry>());
	listEntry_.clear();
	
	std::for_each(listCategory_.begin(),
		listCategory_.end(), string_free<WSTRING>());
	listCategory_.clear();
}


/****************************************************************************
 *
 * AddressBook::SMIMECallbackImpl
 *
 */

qm::AddressBook::SMIMECallbackImpl::SMIMECallbackImpl(
	AddressBook* pAddressBook, QSTATUS* pstatus) :
	pAddressBook_(pAddressBook)
{
}

qm::AddressBook::SMIMECallbackImpl::~SMIMECallbackImpl()
{
}

QSTATUS qm::AddressBook::SMIMECallbackImpl::getCertificate(
	const WCHAR* pwszAddress, Certificate** ppCertificate)
{
	assert(pwszAddress);
	assert(ppCertificate);
	
	DECLARE_QSTATUS();
	
	*ppCertificate = 0;
	
	const AddressBook::EntryList* pList = 0;
	status = pAddressBook_->getEntries(&pList);
	CHECK_QSTATUS();
	
	const WCHAR* pwszCertificate = 0;
	
	bool bEnd = false;
	AddressBook::EntryList::const_iterator itE = pList->begin();
	while (itE != pList->end() && !bEnd) {
		const AddressBookEntry* pEntry = *itE;
		const AddressBookEntry::AddressList& l = pEntry->getAddresses();
		AddressBookEntry::AddressList::const_iterator itA = l.begin();
		while (itA != l.end() && !bEnd) {
			const AddressBookAddress* pAddress = *itA;
			if (wcscmp(pAddress->getAddress(), pwszAddress) == 0) {
				pwszCertificate = pAddress->getCertificate();
				bEnd = true;
			}
			++itA;
		}
		++itE;
	}
	
	if (pwszCertificate) {
		status = pAddressBook_->pSecurity_->getCertificate(
			pwszCertificate, ppCertificate);
		CHECK_QSTATUS();
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * AddressBookEntry
 *
 */

qm::AddressBookEntry::AddressBookEntry(QSTATUS* pstatus) :
	wstrName_(0)
{
}

qm::AddressBookEntry::~AddressBookEntry()
{
	freeWString(wstrName_);
	std::for_each(listAddress_.begin(), listAddress_.end(),
		deleter<AddressBookAddress>());
}

const WCHAR* qm::AddressBookEntry::getName() const
{
	return wstrName_;
}

const AddressBookEntry::AddressList& qm::AddressBookEntry::getAddresses() const
{
	return listAddress_;
}

void qm::AddressBookEntry::setName(qs::WSTRING wstrName)
{
	wstrName_ = wstrName;
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
	const WCHAR* pwszAlias, const WCHAR* pwszCategory, const WCHAR* pwszComment,
	const WCHAR* pwszCertificate, bool bRFC2822, QSTATUS* pstatus) :
	pEntry_(pEntry),
	wstrAddress_(0),
	wstrAlias_(0),
	wstrCategory_(0),
	wstrComment_(0),
	wstrCertificate_(0),
	bRFC2822_(bRFC2822)
{
	string_ptr<WSTRING> wstrAlias;
	if (pwszAlias) {
		wstrAlias.reset(allocWString(pwszAlias));
		if (!wstrAlias.get()) {
			*pstatus = QSTATUS_OUTOFMEMORY;
			return;
		}
	}
	string_ptr<WSTRING> wstrCategory;
	if (pwszCategory) {
		wstrCategory.reset(allocWString(pwszCategory));
		if (!wstrCategory.get()) {
			*pstatus = QSTATUS_OUTOFMEMORY;
			return;
		}
	}
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
	wstrCategory_ = wstrCategory.release();
	wstrComment_ = wstrComment.release();
	wstrCertificate_ = wstrCertificate.release();
}

qm::AddressBookAddress::~AddressBookAddress()
{
	freeWString(wstrAddress_);
	freeWString(wstrAlias_);
	freeWString(wstrCategory_);
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

const WCHAR* qm::AddressBookAddress::getCategory() const
{
	return wstrCategory_;
}

const WCHAR* qm::AddressBookAddress::getComment() const
{
	return wstrComment_;
}

const WCHAR* qm::AddressBookAddress::getCertificate() const
{
	return wstrCertificate_;
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
		status = newQsObject(&pEntry);
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
		
		assert(pEntry_);
		std::auto_ptr<AddressBookAddress> pAddress;
		status = newQsObject(pEntry_, pwszAlias, pwszCategory,
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
	
	if (state_ == STATE_NAME || state_ == STATE_ADDRESS) {
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
