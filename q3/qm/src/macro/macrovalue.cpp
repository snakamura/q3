/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmmacro.h>
#include <qmmessage.h>

#include <qsstl.h>
#include <qsnew.h>

#include <algorithm>

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * MacroValue
 *
 */

qm::MacroValue::MacroValue(Type type, QSTATUS* pstatus) :
	type_(type)
{
	assert(pstatus);
	*pstatus = QSTATUS_SUCCESS;
}

qm::MacroValue::~MacroValue()
{
}

MacroValue::Type qm::MacroValue::getType() const
{
	return type_;
}


/****************************************************************************
 *
 * MacroValueBoolean
 *
 */

qm::MacroValueBoolean::MacroValueBoolean(QSTATUS* pstatus) :
	MacroValue(TYPE_BOOLEAN, pstatus),
	b_(false)
{
}

qm::MacroValueBoolean::~MacroValueBoolean()
{
}

QSTATUS qm::MacroValueBoolean::init(bool b)
{
	b_ = b;
	return QSTATUS_SUCCESS;
}

void qm::MacroValueBoolean::term()
{
	b_ = false;
}

QSTATUS qm::MacroValueBoolean::string(WSTRING* pwstr) const
{
	assert(pwstr);
	if (b_)
		*pwstr = allocWString(L"true");
	else
		*pwstr = allocWString(L"false");
	return *pwstr ? QSTATUS_SUCCESS : QSTATUS_OUTOFMEMORY;
}

bool qm::MacroValueBoolean::boolean() const
{
	return b_;
}

unsigned int qm::MacroValueBoolean::number() const
{
	return b_ ? 1 : 0;
}

QSTATUS qm::MacroValueBoolean::clone(MacroValue** ppValue) const
{
	assert(ppValue);
	return MacroValueFactory::getFactory().newBoolean(
		b_, reinterpret_cast<MacroValueBoolean**>(ppValue));
}


/****************************************************************************
 *
 * MacroValueString
 *
 */

qm::MacroValueString::MacroValueString(QSTATUS* pstatus) :
	MacroValue(TYPE_STRING, pstatus),
	wstr_(0)
{
}

qm::MacroValueString::~MacroValueString()
{
	assert(!wstr_);
}

QSTATUS qm::MacroValueString::init(const WCHAR* pwsz, size_t nLen)
{
	assert(pwsz);
	assert(!wstr_);
	
	wstr_ = allocWString(pwsz, nLen);
	return wstr_ ? QSTATUS_SUCCESS : QSTATUS_OUTOFMEMORY;
}

void qm::MacroValueString::term()
{
	assert(wstr_);
	freeWString(wstr_);
	wstr_ = 0;
}

QSTATUS qm::MacroValueString::string(WSTRING* pwstr) const
{
	assert(pwstr);
	*pwstr = allocWString(wstr_);
	return *pwstr ? QSTATUS_SUCCESS : QSTATUS_OUTOFMEMORY;
}

bool qm::MacroValueString::boolean() const
{
	return *wstr_ != L'\0';
}

unsigned int qm::MacroValueString::number() const
{
	if (MacroParser::isNumber(wstr_)) {
		WCHAR* pEnd = 0;
		return wcstol(wstr_, &pEnd, 10);
	}
	else {
		return 0;
	}
}

QSTATUS qm::MacroValueString::clone(MacroValue** ppValue) const
{
	assert(ppValue);
	return MacroValueFactory::getFactory().newString(
		wstr_, reinterpret_cast<MacroValueString**>(ppValue));
}


/****************************************************************************
 *
 * MacroValueNumber
 *
 */

qm::MacroValueNumber::MacroValueNumber(QSTATUS* pstatus) :
	MacroValue(TYPE_NUMBER, pstatus),
	n_(0)
{
}

qm::MacroValueNumber::~MacroValueNumber()
{
}

QSTATUS qm::MacroValueNumber::init(long n)
{
	n_ = n;
	return QSTATUS_SUCCESS;
}

void qm::MacroValueNumber::term()
{
	n_ = 0;
}

QSTATUS qm::MacroValueNumber::string(WSTRING* pwstr) const
{
	assert(pwstr);
	WCHAR wsz[32];
	swprintf(wsz, L"%lu", n_);
	*pwstr = allocWString(wsz);
	return *pwstr ? QSTATUS_SUCCESS : QSTATUS_OUTOFMEMORY;
}

bool qm::MacroValueNumber::boolean() const
{
	return n_ != 0;
}

unsigned int qm::MacroValueNumber::number() const
{
	return n_;
}

QSTATUS qm::MacroValueNumber::clone(MacroValue** ppValue) const
{
	assert(ppValue);
	return MacroValueFactory::getFactory().newNumber(
		n_, reinterpret_cast<MacroValueNumber**>(ppValue));
}


/****************************************************************************
 *
 * MacroValueRegex
 *
 */

qm::MacroValueRegex::MacroValueRegex(QSTATUS* pstatus) :
	MacroValue(TYPE_REGEX, pstatus),
	pwszPattern_(0),
	pPattern_(0)
{
}

qm::MacroValueRegex::~MacroValueRegex()
{
}

QSTATUS qm::MacroValueRegex::init(const WCHAR* pwszPattern,
	const RegexPattern* pPattern)
{
	pwszPattern_ = pwszPattern;
	pPattern_ = pPattern;
	return QSTATUS_SUCCESS;
}

void qm::MacroValueRegex::term()
{
	pwszPattern_ = 0;
	pPattern_ = 0;
}

const RegexPattern* qm::MacroValueRegex::getPattern() const
{
	return pPattern_;
}

QSTATUS qm::MacroValueRegex::string(WSTRING* pwstr) const
{
	assert(pwstr);
	*pwstr = allocWString(pwszPattern_);
	return *pwstr ? QSTATUS_SUCCESS : QSTATUS_OUTOFMEMORY;
}

bool qm::MacroValueRegex::boolean() const
{
	return *pwszPattern_ != L'\0';
}

unsigned int qm::MacroValueRegex::number() const
{
	if (MacroParser::isNumber(pwszPattern_)) {
		WCHAR* pEnd = 0;
		return wcstol(pwszPattern_, &pEnd, 10);
	}
	else {
		return 0;
	}
}

QSTATUS qm::MacroValueRegex::clone(MacroValue** ppValue) const
{
	assert(ppValue);
	return MacroValueFactory::getFactory().newRegex(pwszPattern_,
		pPattern_, reinterpret_cast<MacroValueRegex**>(ppValue));
}


/****************************************************************************
 *
 * MacroValueField
 *
 */

qm::MacroValueField::MacroValueField(QSTATUS* pstatus) :
	MacroValue(TYPE_FIELD, pstatus),
	wstrName_(0),
	strField_(0)
{
}

qm::MacroValueField::~MacroValueField()
{
	assert(!wstrName_);
	assert(!strField_);
}

QSTATUS qm::MacroValueField::init(const WCHAR* pwszName, const CHAR* pszField)
{
	wstrName_ = allocWString(pwszName);
	if (!wstrName_)
		return QSTATUS_OUTOFMEMORY;
	
	if (pszField) {
		strField_ = allocString(pszField);
		if (!strField_)
			return QSTATUS_OUTOFMEMORY;
	}
	return QSTATUS_SUCCESS;
}

void qm::MacroValueField::term()
{
	freeWString(wstrName_);
	wstrName_ = 0;
	
	freeString(strField_);
	strField_ = 0;
}

const WCHAR* qm::MacroValueField::getName() const
{
	return wstrName_;
}

const CHAR* qm::MacroValueField::getField() const
{
	return strField_;
}

QSTATUS qm::MacroValueField::getAddresses(std::vector<WSTRING>* pAddresses) const
{
	assert(pAddresses);
	
	DECLARE_QSTATUS();
	
	if (!isAddress())
		return QSTATUS_FAIL;
	else if (!strField_)
		return QSTATUS_SUCCESS;
	
	STLWrapper<std::vector<WSTRING> > wrapper(*pAddresses);
	
	Part part(0, strField_, static_cast<size_t>(-1), &status);
	CHECK_QSTATUS();
	AddressListParser address(0, &status);
	CHECK_QSTATUS();
	Part::Field field;
	status = part.getField(wstrName_, &address, &field);
	CHECK_QSTATUS();
	if (field == Part::FIELD_EXIST) {
		typedef AddressListParser::AddressList List;
		const List& l = address.getAddressList();
		List::const_iterator it = l.begin();
		while (it != l.end()) {
			const AddressListParser* pGroup = (*it)->getGroup();
			if (pGroup) {
				const List& lg = pGroup->getAddressList();
				List::const_iterator itg = lg.begin();
				while (itg != lg.end()) {
					string_ptr<WSTRING> wstr;
					status = (*itg)->getAddress(&wstr);
					CHECK_QSTATUS();
					status = wrapper.push_back(wstr.get());
					CHECK_QSTATUS();
					wstr.release();
					++itg;
				}
			}
			else {
				string_ptr<WSTRING> wstr;
				status = (*it)->getAddress(&wstr);
				CHECK_QSTATUS();
				status = wrapper.push_back(wstr.get());
				CHECK_QSTATUS();
				wstr.release();
			}
			++it;
		}
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MacroValueField::getNames(std::vector<WSTRING>* pNames) const
{
	assert(pNames);
	
	DECLARE_QSTATUS();
	
	if (!isAddress())
		return QSTATUS_FAIL;
	else if (!strField_)
		return QSTATUS_SUCCESS;
	
	STLWrapper<std::vector<WSTRING> > wrapper(*pNames);
	
	Part part(0, strField_, static_cast<size_t>(-1), &status);
	CHECK_QSTATUS();
	AddressListParser address(0, &status);
	CHECK_QSTATUS();
	Part::Field field;
	status = part.getField(wstrName_, &address, &field);
	CHECK_QSTATUS();
	if (field == Part::FIELD_EXIST) {
		typedef AddressListParser::AddressList List;
		const List& l = address.getAddressList();
		List::const_iterator it = l.begin();
		while (it != l.end()) {
			string_ptr<WSTRING> wstr;
			if ((*it)->getPhrase()) {
				wstr.reset(allocWString((*it)->getPhrase()));
				if (!wstr.get())
					return QSTATUS_OUTOFMEMORY;
			}
			else {
				status = (*it)->getAddress(&wstr);
				CHECK_QSTATUS();
			}
			status = wrapper.push_back(wstr.get());
			CHECK_QSTATUS();
			wstr.release();
			
			++it;
		}
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::MacroValueField::string(WSTRING* pwstr) const
{
	assert(pwstr);
	
	DECLARE_QSTATUS();
	
	*pwstr = 0;
	
	if (strField_) {
		Part part(0, strField_, static_cast<size_t>(-1), &status);
		CHECK_QSTATUS();
		Part::Field field;
		if (isAddress()) {
			AddressListParser address(0, &status);
			CHECK_QSTATUS();
			status = part.getField(wstrName_, &address, &field);
			CHECK_QSTATUS();
			if (field == Part::FIELD_EXIST) {
				status = address.getValue(pwstr);
				CHECK_QSTATUS();
			}
		}
		else if (_wcsicmp(wstrName_, L"Received") == 0) {
			NoParseParser received(L": ", 0, &status);
			CHECK_QSTATUS();
			status = part.getField(wstrName_, &received, &field);
			CHECK_QSTATUS();
			if (field == Part::FIELD_EXIST) {
				*pwstr = allocWString(received.getValue());
				if (!*pwstr)
					return QSTATUS_OUTOFMEMORY;
			}
		}
		else {
			UnstructuredParser unstructured(&status);
			CHECK_QSTATUS();
			status = part.getField(wstrName_, &unstructured, &field);
			CHECK_QSTATUS();
			if (field == Part::FIELD_EXIST) {
				*pwstr = allocWString(unstructured.getValue());
				if (!*pwstr)
					return QSTATUS_OUTOFMEMORY;
			}
		}
	}
	
	if (!*pwstr) {
		*pwstr = allocWString(L"");
		if (!*pwstr)
			return QSTATUS_OUTOFMEMORY;
	}
	
	return QSTATUS_SUCCESS;
}

bool qm::MacroValueField::boolean() const
{
	return strField_ != 0;
}

unsigned int qm::MacroValueField::number() const
{
	DECLARE_QSTATUS();
	
	string_ptr<WSTRING> wstrValue;
	status = string(&wstrValue);
	if (status != QSTATUS_SUCCESS)
		return 0;
	
	if (MacroParser::isNumber(wstrValue.get())) {
		WCHAR* pEnd = 0;
		return wcstol(wstrValue.get(), &pEnd, 10);
	}
	else {
		return 0;
	}
}

QSTATUS qm::MacroValueField::clone(MacroValue** ppValue) const
{
	return MacroValueFactory::getFactory().newField(
		wstrName_, strField_, reinterpret_cast<MacroValueField**>(ppValue));
}

bool qm::MacroValueField::isAddress() const
{
	const WCHAR* pwszFields[] = {
		L"i",
		L"to",
		L"cc",
		L"bcc",
		L"from",
		L"reply-to",
		L"sender",
		L"errors-to",
		L"resent-to",
		L"resent-cc",
		L"resent-bcc",
		L"resent-from",
		L"resent-reply-to",
		L"resent-sender"
	};
	for (int n = 0; n < countof(pwszFields); ++n) {
		if (_wcsicmp(wstrName_, pwszFields[n]) == 0)
			return true;
	}
	return false;
}


/****************************************************************************
 *
 * MacroValueAddress
 *
 */

qm::MacroValueAddress::MacroValueAddress(QSTATUS* pstatus) :
	MacroValue(TYPE_ADDRESS, pstatus)
{
}

qm::MacroValueAddress::~MacroValueAddress()
{
}

QSTATUS qm::MacroValueAddress::init(const AddressList& l)
{
	assert(listAddress_.empty());
	
	DECLARE_QSTATUS();
	
	status = STLWrapper<AddressList>(listAddress_).reserve(l.size());
	CHECK_QSTATUS();
	
	AddressList::const_iterator it = l.begin();
	while (it != l.end()) {
		WSTRING wstr = allocWString(*it);
		if (!wstr) {
			term();
			return QSTATUS_OUTOFMEMORY;
		}
		listAddress_.push_back(wstr);
		++it;
	}
	
	return QSTATUS_SUCCESS;
}

void qm::MacroValueAddress::term()
{
	std::for_each(listAddress_.begin(), listAddress_.end(),
		string_free<WSTRING>());
	listAddress_.clear();
}

const MacroValueAddress::AddressList& qm::MacroValueAddress::getAddress() const
{
	return listAddress_;
}

void qm::MacroValueAddress::remove(const WCHAR* pwszAddress)
{
	AddressList::iterator it = listAddress_.begin();
	while (it != listAddress_.end()) {
		if (wcsicmp(*it, pwszAddress) == 0) {
			freeWString(*it);
			it = listAddress_.erase(it);
		}
		else {
			++it;
		}
	}
}

QSTATUS qm::MacroValueAddress::string(WSTRING* pwstr) const
{
	assert(pwstr);
	
	DECLARE_QSTATUS();
	
	StringBuffer<WSTRING> buf(&status);
	CHECK_QSTATUS();
	
	AddressList::const_iterator it = listAddress_.begin();
	while (it != listAddress_.end()) {
		if (buf.getLength() != 0) {
			status = buf.append(L", ");
			CHECK_QSTATUS();
		}
		status = buf.append(*it);
		CHECK_QSTATUS();
		
		++it;
	}
	
	*pwstr = buf.getString();
	
	return QSTATUS_SUCCESS;
}

bool qm::MacroValueAddress::boolean() const
{
	return !listAddress_.empty();
}

unsigned int qm::MacroValueAddress::number() const
{
	return listAddress_.size();
}

QSTATUS qm::MacroValueAddress::clone(MacroValue** ppValue) const
{
	assert(ppValue);
	return MacroValueFactory::getFactory().newAddress(
		listAddress_, reinterpret_cast<MacroValueAddress**>(ppValue));
}


/****************************************************************************
 *
 * MacroValueTime
 *
 */

qm::MacroValueTime::MacroValueTime(QSTATUS* pstatus) :
	MacroValue(TYPE_TIME, pstatus)
{
}

qm::MacroValueTime::~MacroValueTime()
{
}

QSTATUS qm::MacroValueTime::init(const Time& time)
{
	time_ = time;
	return QSTATUS_SUCCESS;
}

void qm::MacroValueTime::term()
{
}

const Time& qm::MacroValueTime::getTime() const
{
	return time_;
}

QSTATUS qm::MacroValueTime::string(WSTRING* pwstr) const
{
	assert(pwstr);
	return time_.format(L"%Y4/%M0/%D %h:%m:%s",
		Time::FORMAT_ORIGINAL, pwstr);
}

bool qm::MacroValueTime::boolean() const
{
	return true;
}

unsigned int qm::MacroValueTime::number() const
{
	return 0;
}

QSTATUS qm::MacroValueTime::clone(MacroValue** ppValue) const
{
	assert(ppValue);
	return MacroValueFactory::getFactory().newTime(
		time_, reinterpret_cast<MacroValueTime**>(ppValue));
}


/****************************************************************************
 *
 * MacroValuePart
 *
 */

qm::MacroValuePart::MacroValuePart(QSTATUS* pstatus) :
	MacroValue(TYPE_PART, pstatus),
	pPart_(0)
{
}

qm::MacroValuePart::~MacroValuePart()
{
}

QSTATUS qm::MacroValuePart::init(const Part* pPart)
{
	pPart_ = pPart;
	return QSTATUS_SUCCESS;
}

void qm::MacroValuePart::term()
{
	pPart_ = 0;
}

const Part* qm::MacroValuePart::getPart() const
{
	return pPart_;
}

QSTATUS qm::MacroValuePart::string(WSTRING* pwstr) const
{
	DECLARE_QSTATUS();
	
	if (pPart_) {
		status = PartUtil(*pPart_).getAllText(0, 0, false, pwstr);
		CHECK_QSTATUS();
	}
	else {
		*pwstr = allocWString(L"");
		if (!*pwstr)
			return QSTATUS_OUTOFMEMORY;
	}
	return QSTATUS_SUCCESS;
}

bool qm::MacroValuePart::boolean() const
{
	return pPart_ != 0;
}

unsigned int qm::MacroValuePart::number() const
{
	return pPart_ ? 1 : 0;
}

QSTATUS qm::MacroValuePart::clone(MacroValue** ppValue) const
{
	assert(ppValue);
	return MacroValueFactory::getFactory().newPart(
		pPart_, reinterpret_cast<MacroValuePart**>(ppValue));
}


/****************************************************************************
 *
 * MacroValueMessageList
 *
 */

qm::MacroValueMessageList::MacroValueMessageList(QSTATUS* pstatus) :
	MacroValue(TYPE_MESSAGELIST, pstatus)
{
}

qm::MacroValueMessageList::~MacroValueMessageList()
{
}

QSTATUS qm::MacroValueMessageList::init(const MessageList& l)
{
	DECLARE_QSTATUS();
	
	status = STLWrapper<MessageList>(list_).resize(l.size());
	CHECK_QSTATUS();
	std::copy(l.begin(), l.end(), list_.begin());
	
	return QSTATUS_SUCCESS;
}

void qm::MacroValueMessageList::term()
{
	list_.resize(0);
}

const MacroValueMessageList::MessageList&
	qm::MacroValueMessageList::getMessageList() const
{
	return list_;
}

QSTATUS qm::MacroValueMessageList::string(WSTRING* pwstr) const
{
	assert(pwstr);
	*pwstr = allocWString(L"");
	return *pwstr ? QSTATUS_SUCCESS : QSTATUS_OUTOFMEMORY;
}

bool qm::MacroValueMessageList::boolean() const
{
	return !list_.empty();
}

unsigned int qm::MacroValueMessageList::number() const
{
	return list_.size();
}

QSTATUS qm::MacroValueMessageList::clone(MacroValue** ppValue) const
{
	assert(ppValue);
	return MacroValueFactory::getFactory().newMessageList(list_,
		reinterpret_cast<MacroValueMessageList**>(ppValue));
}


/****************************************************************************
 *
 * MacroValuePtr
 *
 */

qm::MacroValuePtr::MacroValuePtr() :
	pValue_(0)
{
}

qm::MacroValuePtr::MacroValuePtr(MacroValue* pValue) :
	pValue_(pValue)
{
}

qm::MacroValuePtr::~MacroValuePtr()
{
	reset(0);
}

MacroValue* qm::MacroValuePtr::operator->() const
{
	return pValue_;
}

MacroValue** qm::MacroValuePtr::operator&()
{
	assert(!pValue_);
	return &pValue_;
}

MacroValue* qm::MacroValuePtr::get() const
{
	return pValue_;
}

MacroValue* qm::MacroValuePtr::release()
{
	MacroValue* pValue = pValue_;
	pValue_ = 0;
	return pValue;
}

void qm::MacroValuePtr::reset(MacroValue* pValue)
{
	if (pValue_)
		MacroValueFactory::getFactory().deleteValue(pValue_);
	pValue_ = pValue;
}


/****************************************************************************
 *
 * MacroValueFactoryImpl
 *
 */

struct qm::MacroValueFactoryImpl
{
public:
	typedef std::vector<MacroValueBoolean*> BooleanList;
	typedef std::vector<MacroValueString*> StringList;
	typedef std::vector<MacroValueNumber*> NumberList;
	typedef std::vector<MacroValueRegex*> RegexList;
	typedef std::vector<MacroValueField*> FieldList;
	typedef std::vector<MacroValueAddress*> AddressList;
	typedef std::vector<MacroValueTime*> TimeList;
	typedef std::vector<MacroValuePart*> PartList;
	typedef std::vector<MacroValueMessageList*> MessageListList;

public:
	BooleanList listBoolean_;
	unsigned int nBoolean_;
	StringList listString_;
	unsigned int nString_;
	NumberList listNumber_;
	unsigned int nNumber_;
	RegexList listRegex_;
	unsigned int nRegex_;
	FieldList listField_;
	unsigned int nField_;
	AddressList listAddress_;
	unsigned int nAddress_;
	TimeList listTime_;
	unsigned int nTime_;
	PartList listPart_;
	unsigned int nPart_;
	MessageListList listMessageList_;
	unsigned int nMessageList_;
	qs::SpinLock lock_;
};


/****************************************************************************
 *
 * MacroValueFactory
 *
 */

template<class Value>
QSTATUS newValue(SpinLock& l, std::vector<Value*>& v,
	unsigned int& n, Value** ppValue)
{
	assert(ppValue);
	
	DECLARE_QSTATUS();
	
	Lock<SpinLock> lock(l);
	
	if (v.empty()) {
		status = newQsObject(ppValue);
		CHECK_QSTATUS();
		status = STLWrapper<std::vector<Value*> >(v).reserve(++n);
		CHECK_QSTATUS();
	}
	else {
		*ppValue = v.back();
		v.pop_back();
	}
	return QSTATUS_SUCCESS;
}

template<class Value>
void deleteValue(SpinLock& l, std::vector<Value*>& v, Value* pValue)
{
	assert(pValue);
	
	Lock<SpinLock> lock(l);
	
	pValue->term();
	assert(v.capacity() >= v.size() + 1);
	v.push_back(pValue);
}

MacroValueFactory qm::MacroValueFactory::factory__;

qm::MacroValueFactory::MacroValueFactory() :
	pImpl_(0)
{
	DECLARE_QSTATUS();
	
	status = newObject(&pImpl_);
	
	pImpl_->nBoolean_ = 0;
	pImpl_->nString_ = 0;
	pImpl_->nNumber_ = 0;
	pImpl_->nRegex_ = 0;
	pImpl_->nField_ = 0;
	pImpl_->nAddress_ = 0;
	pImpl_->nTime_ = 0;
	pImpl_->nPart_ = 0;
	pImpl_->nMessageList_ = 0;
}

qm::MacroValueFactory::~MacroValueFactory()
{
	if (pImpl_) {
		std::for_each(pImpl_->listBoolean_.begin(),
			pImpl_->listBoolean_.end(), deleter<MacroValueBoolean>());
		std::for_each(pImpl_->listString_.begin(),
			pImpl_->listString_.end(), deleter<MacroValueString>());
		std::for_each(pImpl_->listNumber_.begin(),
			pImpl_->listNumber_.end(), deleter<MacroValueNumber>());
		std::for_each(pImpl_->listRegex_.begin(),
			pImpl_->listRegex_.end(), deleter<MacroValueRegex>());
		std::for_each(pImpl_->listField_.begin(),
			pImpl_->listField_.end(), deleter<MacroValueField>());
		std::for_each(pImpl_->listAddress_.begin(),
			pImpl_->listAddress_.end(), deleter<MacroValueAddress>());
		std::for_each(pImpl_->listTime_.begin(),
			pImpl_->listTime_.end(), deleter<MacroValueTime>());
		std::for_each(pImpl_->listPart_.begin(),
			pImpl_->listPart_.end(), deleter<MacroValuePart>());
		std::for_each(pImpl_->listMessageList_.begin(),
			pImpl_->listMessageList_.end(), deleter<MacroValueMessageList>());
		delete pImpl_;
	}
}

QSTATUS qm::MacroValueFactory::newBoolean(bool b, MacroValueBoolean** ppmvb)
{
	DECLARE_QSTATUS();
	
	status = newValue<MacroValueBoolean>(pImpl_->lock_,
		pImpl_->listBoolean_, pImpl_->nBoolean_, ppmvb);
	CHECK_QSTATUS();
	
	return (*ppmvb)->init(b);
}

void qm::MacroValueFactory::deleteBoolean(MacroValueBoolean* pmvb)
{
	::deleteValue<MacroValueBoolean>(pImpl_->lock_, pImpl_->listBoolean_, pmvb);
}

QSTATUS qm::MacroValueFactory::newString(const WCHAR* pwsz, MacroValueString** ppmvs)
{
	return newString(pwsz, static_cast<size_t>(-1), ppmvs);
}

QSTATUS qm::MacroValueFactory::newString(const WCHAR* pwsz, size_t nLen, MacroValueString** ppmvs)
{
	DECLARE_QSTATUS();
	
	status = newValue<MacroValueString>(pImpl_->lock_,
		pImpl_->listString_, pImpl_->nString_, ppmvs);
	CHECK_QSTATUS();
	
	return (*ppmvs)->init(pwsz, nLen);
}

void qm::MacroValueFactory::deleteString(MacroValueString* pmvs)
{
	::deleteValue<MacroValueString>(pImpl_->lock_, pImpl_->listString_, pmvs);
}

QSTATUS qm::MacroValueFactory::newNumber(unsigned int n, MacroValueNumber** ppmvn)
{
	DECLARE_QSTATUS();
	
	status = newValue<MacroValueNumber>(pImpl_->lock_,
		pImpl_->listNumber_, pImpl_->nNumber_, ppmvn);
	CHECK_QSTATUS();
	
	return (*ppmvn)->init(n);
}

void qm::MacroValueFactory::deleteNumber(MacroValueNumber* pmvn)
{
	::deleteValue<MacroValueNumber>(pImpl_->lock_, pImpl_->listNumber_, pmvn);
}

QSTATUS qm::MacroValueFactory::newRegex(const WCHAR* pwszPattern,
	const qs::RegexPattern* pPattern, MacroValueRegex** ppmvr)
{
	DECLARE_QSTATUS();
	
	status = newValue<MacroValueRegex>(pImpl_->lock_,
		pImpl_->listRegex_, pImpl_->nRegex_, ppmvr);
	CHECK_QSTATUS();
	
	return (*ppmvr)->init(pwszPattern, pPattern);
}

void qm::MacroValueFactory::deleteRegex(MacroValueRegex* pmvr)
{
	::deleteValue<MacroValueRegex>(pImpl_->lock_, pImpl_->listRegex_, pmvr);
}

QSTATUS qm::MacroValueFactory::newField(const WCHAR* pwszName,
	const CHAR* pszField, MacroValueField** ppmvf)
{
	DECLARE_QSTATUS();
	
	status = newValue<MacroValueField>(pImpl_->lock_,
		pImpl_->listField_, pImpl_->nField_, ppmvf);
	CHECK_QSTATUS();
	
	return (*ppmvf)->init(pwszName, pszField);
}

void qm::MacroValueFactory::deleteField(MacroValueField* pmvf)
{
	::deleteValue<MacroValueField>(pImpl_->lock_, pImpl_->listField_, pmvf);
}

QSTATUS qm::MacroValueFactory::newAddress(
	const MacroValueAddress::AddressList& l, MacroValueAddress** ppmva)
{
	DECLARE_QSTATUS();
	
	status = newValue<MacroValueAddress>(pImpl_->lock_,
		pImpl_->listAddress_, pImpl_->nAddress_, ppmva);
	CHECK_QSTATUS();
	
	return (*ppmva)->init(l);
}

void qm::MacroValueFactory::deleteAddress(MacroValueAddress* pmva)
{
	::deleteValue<MacroValueAddress>(pImpl_->lock_, pImpl_->listAddress_, pmva);
}

QSTATUS qm::MacroValueFactory::newTime(const Time& time, MacroValueTime** ppmvt)
{
	DECLARE_QSTATUS();
	
	status = newValue<MacroValueTime>(pImpl_->lock_,
		pImpl_->listTime_, pImpl_->nTime_, ppmvt);
	CHECK_QSTATUS();
	
	return (*ppmvt)->init(time);
}

void qm::MacroValueFactory::deleteTime(MacroValueTime* pmvt)
{
	::deleteValue<MacroValueTime>(pImpl_->lock_, pImpl_->listTime_, pmvt);
}

QSTATUS qm::MacroValueFactory::newPart(const Part* pPart, MacroValuePart** ppmvp)
{
	DECLARE_QSTATUS();
	
	status = newValue<MacroValuePart>(pImpl_->lock_,
		pImpl_->listPart_, pImpl_->nPart_, ppmvp);
	CHECK_QSTATUS();
	
	return (*ppmvp)->init(pPart);
}

void qm::MacroValueFactory::deletePart(MacroValuePart* pmvp)
{
	::deleteValue<MacroValuePart>(pImpl_->lock_, pImpl_->listPart_, pmvp);
}

QSTATUS qm::MacroValueFactory::newMessageList(
	const MacroValueMessageList::MessageList& l,
	MacroValueMessageList** ppmvml)
{
	DECLARE_QSTATUS();
	
	status = newValue<MacroValueMessageList>(pImpl_->lock_,
		pImpl_->listMessageList_, pImpl_->nMessageList_, ppmvml);
	CHECK_QSTATUS();
	
	return (*ppmvml)->init(l);
}

void qm::MacroValueFactory::deleteMessageList(MacroValueMessageList* pmvml)
{
	::deleteValue<MacroValueMessageList>(
		pImpl_->lock_, pImpl_->listMessageList_, pmvml);
}

void qm::MacroValueFactory::deleteValue(MacroValue* pmv)
{
	switch (pmv->getType()) {
	case MacroValue::TYPE_BOOLEAN:
		deleteBoolean(static_cast<MacroValueBoolean*>(pmv));
		break;
	case MacroValue::TYPE_STRING:
		deleteString(static_cast<MacroValueString*>(pmv));
		break;
	case MacroValue::TYPE_NUMBER:
		deleteNumber(static_cast<MacroValueNumber*>(pmv));
		break;
	case MacroValue::TYPE_REGEX:
		deleteRegex(static_cast<MacroValueRegex*>(pmv));
		break;
	case MacroValue::TYPE_FIELD:
		deleteField(static_cast<MacroValueField*>(pmv));
		break;
	case MacroValue::TYPE_ADDRESS:
		deleteAddress(static_cast<MacroValueAddress*>(pmv));
		break;
	case MacroValue::TYPE_TIME:
		deleteTime(static_cast<MacroValueTime*>(pmv));
		break;
	case MacroValue::TYPE_PART:
		deletePart(static_cast<MacroValuePart*>(pmv));
		break;
	case MacroValue::TYPE_MESSAGELIST:
		deleteMessageList(static_cast<MacroValueMessageList*>(pmv));
		break;
	}
}

MacroValueFactory& qm::MacroValueFactory::getFactory()
{
	return factory__;
}
