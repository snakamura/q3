/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmmacro.h>
#include <qmmessage.h>

#include <qsstl.h>

#include <algorithm>

#include "../model/uri.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * MacroValue
 *
 */

qm::MacroValue::MacroValue(Type type) :
	type_(type)
{
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

qm::MacroValueBoolean::MacroValueBoolean() :
	MacroValue(TYPE_BOOLEAN),
	b_(false)
{
}

qm::MacroValueBoolean::~MacroValueBoolean()
{
}

void qm::MacroValueBoolean::init(bool b)
{
	b_ = b;
}

void qm::MacroValueBoolean::term()
{
	b_ = false;
}

wstring_ptr qm::MacroValueBoolean::string() const
{
	return allocWString(b_ ? L"true" : L"false");
}

bool qm::MacroValueBoolean::boolean() const
{
	return b_;
}

unsigned int qm::MacroValueBoolean::number() const
{
	return b_ ? 1 : 0;
}

MacroValuePtr qm::MacroValueBoolean::clone() const
{
	return MacroValueFactory::getFactory().newBoolean(b_);
}


/****************************************************************************
 *
 * MacroValueString
 *
 */

qm::MacroValueString::MacroValueString() :
	MacroValue(TYPE_STRING)
{
}

qm::MacroValueString::~MacroValueString()
{
	assert(!wstr_.get());
}

void qm::MacroValueString::init(const WCHAR* pwsz,
								size_t nLen)
{
	assert(pwsz);
	assert(!wstr_.get());
	
	wstr_ = allocWString(pwsz, nLen);
}

void qm::MacroValueString::term()
{
	assert(wstr_.get());
	wstr_.reset(0);
}

wstring_ptr qm::MacroValueString::string() const
{
	return allocWString(wstr_.get());
}

bool qm::MacroValueString::boolean() const
{
	return *wstr_.get() != L'\0';
}

unsigned int qm::MacroValueString::number() const
{
	if (MacroParser::isNumber(wstr_.get())) {
		WCHAR* pEnd = 0;
		return wcstol(wstr_.get(), &pEnd, 10);
	}
	else {
		return 0;
	}
}

MacroValuePtr qm::MacroValueString::clone() const
{
	return MacroValueFactory::getFactory().newString(wstr_.get());
}


/****************************************************************************
 *
 * MacroValueNumber
 *
 */

qm::MacroValueNumber::MacroValueNumber() :
	MacroValue(TYPE_NUMBER),
	n_(0)
{
}

qm::MacroValueNumber::~MacroValueNumber()
{
}

void qm::MacroValueNumber::init(unsigned int n)
{
	n_ = n;
}

void qm::MacroValueNumber::term()
{
	n_ = 0;
}

wstring_ptr qm::MacroValueNumber::string() const
{
	WCHAR wsz[32];
	swprintf(wsz, L"%lu", n_);
	return allocWString(wsz);
}

bool qm::MacroValueNumber::boolean() const
{
	return n_ != 0;
}

unsigned int qm::MacroValueNumber::number() const
{
	return n_;
}

MacroValuePtr qm::MacroValueNumber::clone() const
{
	return MacroValueFactory::getFactory().newNumber(n_);
}


/****************************************************************************
 *
 * MacroValueRegex
 *
 */

qm::MacroValueRegex::MacroValueRegex() :
	MacroValue(TYPE_REGEX),
	pwszPattern_(0),
	pPattern_(0)
{
}

qm::MacroValueRegex::~MacroValueRegex()
{
}

void qm::MacroValueRegex::init(const WCHAR* pwszPattern,
							   const RegexPattern* pPattern)
{
	pwszPattern_ = pwszPattern;
	pPattern_ = pPattern;
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

wstring_ptr qm::MacroValueRegex::string() const
{
	return allocWString(pwszPattern_);
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

MacroValuePtr qm::MacroValueRegex::clone() const
{
	return MacroValueFactory::getFactory().newRegex(pwszPattern_, pPattern_);
}


/****************************************************************************
 *
 * MacroValueField
 *
 */

qm::MacroValueField::MacroValueField() :
	MacroValue(TYPE_FIELD)
{
}

qm::MacroValueField::~MacroValueField()
{
	assert(!wstrName_.get());
	assert(!strField_.get());
}

void qm::MacroValueField::init(const WCHAR* pwszName,
							   const CHAR* pszField)
{
	wstrName_ = allocWString(pwszName);
	if (pszField)
		strField_ = allocString(pszField);
}

void qm::MacroValueField::term()
{
	wstrName_.reset(0);
	strField_.reset(0);
}

const WCHAR* qm::MacroValueField::getName() const
{
	return wstrName_.get();
}

const CHAR* qm::MacroValueField::getField() const
{
	return strField_.get();
}

bool qm::MacroValueField::getAddresses(std::vector<WSTRING>* pAddresses) const
{
	assert(pAddresses);
	
	if (!isAddress())
		return false;
	else if (!strField_.get())
		return true;
	
	Part part;
	if (!part.create(0, strField_.get(), -1))
		return false;
	AddressListParser address(0);
	Part::Field field = part.getField(wstrName_.get(), &address);
	if (field == Part::FIELD_EXIST) {
		typedef AddressListParser::AddressList List;
		const List& l = address.getAddressList();
		for (List::const_iterator it = l.begin(); it != l.end(); ++it) {
			const AddressListParser* pGroup = (*it)->getGroup();
			if (pGroup) {
				const List& lg = pGroup->getAddressList();
				for (List::const_iterator itg = lg.begin(); itg != lg.end(); ++itg) {
					wstring_ptr wstr((*itg)->getAddress());
					pAddresses->push_back(wstr.get());
					wstr.release();
				}
			}
			else {
				wstring_ptr wstr((*it)->getAddress());
				pAddresses->push_back(wstr.get());
				wstr.release();
			}
		}
	}
	
	return true;
}

bool qm::MacroValueField::getNames(std::vector<WSTRING>* pNames) const
{
	assert(pNames);
	
	if (!isAddress())
		return false;
	else if (!strField_.get())
		return true;
	
	Part part;
	if (!part.create(0, strField_.get(), -1))
		return false;
	AddressListParser address(0);
	Part::Field field = part.getField(wstrName_.get(), &address);
	if (field == Part::FIELD_EXIST) {
		typedef AddressListParser::AddressList List;
		const List& l = address.getAddressList();
		for (List::const_iterator it = l.begin(); it != l.end(); ++it) {
			wstring_ptr wstr;
			if ((*it)->getPhrase())
				wstr = allocWString((*it)->getPhrase());
			else
				wstr = (*it)->getAddress();
			pNames->push_back(wstr.get());
			wstr.release();
		}
	}
	
	return true;
}

wstring_ptr qm::MacroValueField::string() const
{
	if (strField_.get()) {
		Part part;
		if (!part.create(0, strField_.get(), -1))
			return 0;
		if (isAddress()) {
			AddressListParser address(0);
			if (part.getField(wstrName_.get(), &address) == Part::FIELD_EXIST)
				return address.getValue();
		}
		else if (_wcsicmp(wstrName_.get(), L"Received") == 0) {
			NoParseParser received(L": ", 0);
			if (part.getField(wstrName_.get(), &received) == Part::FIELD_EXIST)
				return allocWString(received.getValue());
		}
		else {
			UnstructuredParser unstructured;
			if (part.getField(wstrName_.get(), &unstructured) == Part::FIELD_EXIST)
				return allocWString(unstructured.getValue());
		}
	}
	
	return allocWString(L"");
}

bool qm::MacroValueField::boolean() const
{
	return strField_.get() != 0;
}

unsigned int qm::MacroValueField::number() const
{
	wstring_ptr wstrValue(string());
	if (!wstrValue.get())
		return 0;
	
	if (MacroParser::isNumber(wstrValue.get())) {
		WCHAR* pEnd = 0;
		return wcstol(wstrValue.get(), &pEnd, 10);
	}
	else {
		return 0;
	}
}

MacroValuePtr qm::MacroValueField::clone() const
{
	return MacroValueFactory::getFactory().newField(wstrName_.get(), strField_.get());
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
		if (_wcsicmp(wstrName_.get(), pwszFields[n]) == 0)
			return true;
	}
	return false;
}


/****************************************************************************
 *
 * MacroValueAddress
 *
 */

qm::MacroValueAddress::MacroValueAddress() :
	MacroValue(TYPE_ADDRESS)
{
}

qm::MacroValueAddress::~MacroValueAddress()
{
}

void qm::MacroValueAddress::init(const AddressList& l)
{
	assert(listAddress_.empty());
	
	listAddress_.reserve(l.size());
	
	for (AddressList::const_iterator it = l.begin(); it != l.end(); ++it)
		listAddress_.push_back(allocWString(*it).release());
}

void qm::MacroValueAddress::term()
{
	std::for_each(listAddress_.begin(), listAddress_.end(), string_free<WSTRING>());
	listAddress_.clear();
}

const MacroValueAddress::AddressList& qm::MacroValueAddress::getAddress() const
{
	return listAddress_;
}

void qm::MacroValueAddress::remove(const WCHAR* pwszAddress)
{
	for (AddressList::iterator it = listAddress_.begin(); it != listAddress_.end(); ) {
		if (wcsicmp(*it, pwszAddress) == 0) {
			freeWString(*it);
			it = listAddress_.erase(it);
		}
		else {
			++it;
		}
	}
}

wstring_ptr qm::MacroValueAddress::string() const
{
	StringBuffer<WSTRING> buf;
	
	for (AddressList::const_iterator it = listAddress_.begin(); it != listAddress_.end(); ++it) {
		if (buf.getLength() != 0)
			buf.append(L", ");
		buf.append(*it);
	}
	
	return buf.getString();
}

bool qm::MacroValueAddress::boolean() const
{
	return !listAddress_.empty();
}

unsigned int qm::MacroValueAddress::number() const
{
	return listAddress_.size();
}

MacroValuePtr qm::MacroValueAddress::clone() const
{
	return MacroValueFactory::getFactory().newAddress(listAddress_);
}


/****************************************************************************
 *
 * MacroValueTime
 *
 */

qm::MacroValueTime::MacroValueTime() :
	MacroValue(TYPE_TIME)
{
}

qm::MacroValueTime::~MacroValueTime()
{
}

void qm::MacroValueTime::init(const Time& time)
{
	time_ = time;
}

void qm::MacroValueTime::term()
{
}

const Time& qm::MacroValueTime::getTime() const
{
	return time_;
}

wstring_ptr qm::MacroValueTime::string() const
{
	return time_.format(L"%Y4/%M0/%D %h:%m:%s", Time::FORMAT_ORIGINAL);
}

bool qm::MacroValueTime::boolean() const
{
	return true;
}

unsigned int qm::MacroValueTime::number() const
{
	return 0;
}

MacroValuePtr qm::MacroValueTime::clone() const
{
	return MacroValueFactory::getFactory().newTime(time_);
}


/****************************************************************************
 *
 * MacroValuePart
 *
 */

qm::MacroValuePart::MacroValuePart() :
	MacroValue(TYPE_PART),
	pPart_(0)
{
}

qm::MacroValuePart::~MacroValuePart()
{
}

void qm::MacroValuePart::init(const Part* pPart)
{
	pPart_ = pPart;
}

void qm::MacroValuePart::term()
{
	pPart_ = 0;
}

const Part* qm::MacroValuePart::getPart() const
{
	return pPart_;
}

wstring_ptr qm::MacroValuePart::string() const
{
	if (pPart_)
		return allocWString(PartUtil(*pPart_).getAllText(0, 0, false).get());
	else
		return allocWString(L"");
}

bool qm::MacroValuePart::boolean() const
{
	return pPart_ != 0;
}

unsigned int qm::MacroValuePart::number() const
{
	return pPart_ ? 1 : 0;
}

MacroValuePtr qm::MacroValuePart::clone() const
{
	return MacroValueFactory::getFactory().newPart(pPart_);
}


/****************************************************************************
 *
 * MacroValueMessageList
 *
 */

qm::MacroValueMessageList::MacroValueMessageList() :
	MacroValue(TYPE_MESSAGELIST)
{
}

qm::MacroValueMessageList::~MacroValueMessageList()
{
}

void qm::MacroValueMessageList::init(const MessageList& l)
{
	list_ = l;
}

void qm::MacroValueMessageList::term()
{
	list_.resize(0);
}

const MacroValueMessageList::MessageList& qm::MacroValueMessageList::getMessageList() const
{
	return list_;
}

wstring_ptr qm::MacroValueMessageList::string() const
{
	StringBuffer<WSTRING> buf;
	for (MessageList::const_iterator it = list_.begin(); it != list_.end(); ++it) {
		MessagePtrLock mpl(*it);
		if (mpl) {
			if (buf.getLength() != 0)
				buf.append(L", ");
			wstring_ptr wstrURI(URI::getURI(mpl));
			buf.append(wstrURI.get());
		}
	}
	return buf.getString();
}

bool qm::MacroValueMessageList::boolean() const
{
	return !list_.empty();
}

unsigned int qm::MacroValueMessageList::number() const
{
	return list_.size();
}

MacroValuePtr qm::MacroValueMessageList::clone() const
{
	return MacroValueFactory::getFactory().newMessageList(list_);
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

qm::MacroValuePtr::MacroValuePtr(MacroValuePtr& pValue) :
	pValue_(pValue.release())
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

MacroValuePtr& qm::MacroValuePtr::operator=(MacroValuePtr& pValue)
{
	if (pValue.get() != pValue_)
		reset(pValue.release());
	return *this;
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
	SpinLock lock_;
};


/****************************************************************************
 *
 * MacroValueFactory
 *
 */

template<class Value>
MacroValuePtr newValue(SpinLock& l,
					   std::vector<Value*>& v,
					   unsigned int& n)
{
	Lock<SpinLock> lock(l);

	MacroValuePtr pValue;
	if (v.empty()) {
		pValue.reset(new Value());
		v.reserve(++n);
	}
	else {
		pValue.reset(v.back());
		v.pop_back();
	}
	return pValue;
}

template<class Value>
void deleteValue(SpinLock& l,
				 std::vector<Value*>& v,
				 Value* pValue)
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
	pImpl_ = new MacroValueFactoryImpl();
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

MacroValuePtr qm::MacroValueFactory::newBoolean(bool b)
{
	MacroValuePtr pValue(newValue<MacroValueBoolean>(
		pImpl_->lock_, pImpl_->listBoolean_, pImpl_->nBoolean_));
	static_cast<MacroValueBoolean*>(pValue.get())->init(b);
	return pValue;
}

void qm::MacroValueFactory::deleteBoolean(MacroValueBoolean* pmvb)
{
	::deleteValue<MacroValueBoolean>(pImpl_->lock_, pImpl_->listBoolean_, pmvb);
}

MacroValuePtr qm::MacroValueFactory::newString(const WCHAR* pwsz)
{
	return newString(pwsz, -1);
}

MacroValuePtr qm::MacroValueFactory::newString(const WCHAR* pwsz,
											   size_t nLen)
{
	MacroValuePtr pValue(newValue<MacroValueString>(
		pImpl_->lock_, pImpl_->listString_, pImpl_->nString_));
	static_cast<MacroValueString*>(pValue.get())->init(pwsz, nLen);
	return pValue;
}

void qm::MacroValueFactory::deleteString(MacroValueString* pmvs)
{
	::deleteValue<MacroValueString>(pImpl_->lock_, pImpl_->listString_, pmvs);
}

MacroValuePtr qm::MacroValueFactory::newNumber(unsigned int n)
{
	MacroValuePtr pValue(newValue<MacroValueNumber>(
		pImpl_->lock_, pImpl_->listNumber_, pImpl_->nNumber_));
	static_cast<MacroValueNumber*>(pValue.get())->init(n);
	return pValue;
}

void qm::MacroValueFactory::deleteNumber(MacroValueNumber* pmvn)
{
	::deleteValue<MacroValueNumber>(pImpl_->lock_, pImpl_->listNumber_, pmvn);
}

MacroValuePtr qm::MacroValueFactory::newRegex(const WCHAR* pwszPattern,
											  const RegexPattern* pPattern)
{
	MacroValuePtr pValue(newValue<MacroValueRegex>(
		pImpl_->lock_, pImpl_->listRegex_, pImpl_->nRegex_));
	static_cast<MacroValueRegex*>(pValue.get())->init(pwszPattern, pPattern);
	return pValue;
}

void qm::MacroValueFactory::deleteRegex(MacroValueRegex* pmvr)
{
	::deleteValue<MacroValueRegex>(pImpl_->lock_, pImpl_->listRegex_, pmvr);
}

MacroValuePtr qm::MacroValueFactory::newField(const WCHAR* pwszName,
											  const CHAR* pszField)
{
	MacroValuePtr pValue(newValue<MacroValueField>(
		pImpl_->lock_, pImpl_->listField_, pImpl_->nField_));
	static_cast<MacroValueField*>(pValue.get())->init(pwszName, pszField);
	return pValue;
}

void qm::MacroValueFactory::deleteField(MacroValueField* pmvf)
{
	::deleteValue<MacroValueField>(pImpl_->lock_, pImpl_->listField_, pmvf);
}

MacroValuePtr qm::MacroValueFactory::newAddress(const MacroValueAddress::AddressList& l)
{
	MacroValuePtr pValue(newValue<MacroValueAddress>(
		pImpl_->lock_, pImpl_->listAddress_, pImpl_->nAddress_));
	static_cast<MacroValueAddress*>(pValue.get())->init(l);
	return pValue;
}

void qm::MacroValueFactory::deleteAddress(MacroValueAddress* pmva)
{
	::deleteValue<MacroValueAddress>(pImpl_->lock_, pImpl_->listAddress_, pmva);
}

MacroValuePtr qm::MacroValueFactory::newTime(const Time& time)
{
	MacroValuePtr pValue(newValue<MacroValueTime>(
		pImpl_->lock_, pImpl_->listTime_, pImpl_->nTime_));
	static_cast<MacroValueTime*>(pValue.get())->init(time);
	return pValue;
}

void qm::MacroValueFactory::deleteTime(MacroValueTime* pmvt)
{
	::deleteValue<MacroValueTime>(pImpl_->lock_, pImpl_->listTime_, pmvt);
}

MacroValuePtr qm::MacroValueFactory::newPart(const Part* pPart)
{
	MacroValuePtr pValue(newValue<MacroValuePart>(
		pImpl_->lock_, pImpl_->listPart_, pImpl_->nPart_));
	static_cast<MacroValuePart*>(pValue.get())->init(pPart);
	return pValue;
}

void qm::MacroValueFactory::deletePart(MacroValuePart* pmvp)
{
	::deleteValue<MacroValuePart>(pImpl_->lock_, pImpl_->listPart_, pmvp);
}

MacroValuePtr qm::MacroValueFactory::newMessageList(const MacroValueMessageList::MessageList& l)
{
	MacroValuePtr pValue(newValue<MacroValueMessageList>(
		pImpl_->lock_, pImpl_->listMessageList_, pImpl_->nMessageList_));
	static_cast<MacroValueMessageList*>(pValue.get())->init(l);
	return pValue;
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
