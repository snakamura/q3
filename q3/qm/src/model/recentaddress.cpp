/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#pragma warning(disable:4786)

#include <qmmessage.h>

#include <algorithm>

#include <boost/bind.hpp>

#include "addressbook.h"
#include "recentaddress.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * RecentAddress
 *
 */

qm::RecentAddress::RecentAddress(AddressBook* pAddressBook,
								 Profile* pProfile) :
	pAddressBook_(pAddressBook),
	pProfile_(pProfile),
	bModified_(false)
{
	nMax_ = pProfile_->getInt(L"RecentAddress", L"Max");
	if (nMax_ == 0)
		nMax_ = 10;
	
	load();
}

qm::RecentAddress::~RecentAddress()
{
	std::for_each(listAddress_.begin(), listAddress_.end(), qs::deleter<AddressParser>());
	listAddress_.clear();
}

const RecentAddress::AddressList& qm::RecentAddress::getAddresses() const
{
	return listAddress_;
}

void qm::RecentAddress::add(const Message& msg)
{
	const WCHAR* pwszFields[] = {
		L"To",
		L"Cc",
		L"Bcc"
	};
	for (int n = 0; n < countof(pwszFields); ++n) {
		AddressListParser addressList;
		if (msg.getField(pwszFields[n], &addressList) == Part::FIELD_EXIST)
			add(addressList);
	}
}

void qm::RecentAddress::remove(const WCHAR* pwsz)
{
	AddressList::iterator it = listAddress_.begin();
	while (it != listAddress_.end() && wcscmp((*it)->getValue().get(), pwsz) != 0)
		++it;
	if (it != listAddress_.end()) {
		listAddress_.erase(it);
		bModified_ = true;
	}
}

void qm::RecentAddress::save() const
{
	if (!bModified_)
		return;
	
	for (unsigned int n = 0; n < nMax_; ++n) {
		WCHAR wszKey[32];
		_snwprintf(wszKey, countof(wszKey), L"Address%u", n);
		if (n < listAddress_.size()) {
			wstring_ptr wstrValue(listAddress_[n]->getValue());
			pProfile_->setString(L"RecentAddress", wszKey, wstrValue.get());
		}
		else if (n == listAddress_.size()) {
			pProfile_->setString(L"RecentAddress", wszKey, L"");
		}
		else {
			break;
		}
	}
	
	bModified_ = false;
}

void qm::RecentAddress::load()
{
	listAddress_.reserve(nMax_);
	
	for (unsigned int n = 0; n < nMax_; ++n) {
		WCHAR wszKey[32];
		_snwprintf(wszKey, countof(wszKey), L"Address%u", n);
		wstring_ptr wstrAddress(pProfile_->getString(L"RecentAddress", wszKey));
		if (!*wstrAddress.get())
			break;
		
		std::auto_ptr<AddressParser> pAddress(new AddressParser());
		if (MessageCreator::getAddress(wstrAddress.get(), pAddress.get()))
			listAddress_.push_back(pAddress.release());
	}
	
	bModified_ = false;
}

void qm::RecentAddress::add(const qs::AddressListParser& addressList)
{
	const AddressListParser::AddressList& l = addressList.getAddressList();
	for (AddressListParser::AddressList::const_iterator it = l.begin(); it != l.end(); ++it)
		add(**it);
}

void qm::RecentAddress::add(const AddressParser& address)
{
	const AddressListParser* pGroup = address.getGroup();
	if (pGroup) {
		add(*pGroup);
	}
	else {
		wstring_ptr wstrAddress(address.getAddress());
		if (pAddressBook_->getEntry(wstrAddress.get()))
			return;
		
		AddressList::iterator it = std::find_if(
			listAddress_.begin(), listAddress_.end(),
			boost::bind(std::logical_and<bool>(),
				boost::bind(string_equal_i<WCHAR>(),
					boost::bind(&AddressParser::getMailbox, _1), address.getMailbox()),
				boost::bind(string_equal_i<WCHAR>(),
					boost::bind(&AddressParser::getHost, _1), address.getHost())));
		if (it != listAddress_.end()) {
			delete *it;
			listAddress_.erase(it);
		}
		
		std::auto_ptr<AddressParser> pAddress(new AddressParser(
			address.getPhrase(), address.getMailbox(), address.getHost()));
		if (listAddress_.size() >= nMax_) {
			std::for_each(listAddress_.begin() + nMax_ - 1,
				listAddress_.end(), qs::deleter<AddressParser>());
			listAddress_.resize(nMax_ - 1);
		}
		listAddress_.insert(listAddress_.begin(), pAddress.release());
		
		bModified_ = true;
	}
}
