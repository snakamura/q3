/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmmessage.h>

#include <algorithm>

#include "addressbook.h"
#include "recentaddress.h"

#pragma warning(disable:4786)

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
	pProfile_(pProfile)
{
	nMax_ = pProfile_->getInt(L"RecentAddress", L"Max", 10);
	if (nMax_ == 0 || nMax_ > 100)
		nMax_ = 10;
	
	load();
}

qm::RecentAddress::~RecentAddress()
{
	std::for_each(listAddress_.begin(), listAddress_.end(), qs::deleter<AddressParser>());
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
		AddressListParser addressList(0);
		if (msg.getField(pwszFields[n], &addressList) == Part::FIELD_EXIST)
			add(addressList);
	}
}

bool qm::RecentAddress::save() const
{
	for (unsigned int n = 0; n < nMax_; ++n) {
		WCHAR wszKey[32];
		swprintf(wszKey, L"Address%u", n);
		const WCHAR* pwszValue = L"";
		wstring_ptr wstrValue;
		if (n < listAddress_.size()) {
			wstrValue = listAddress_[n]->getValue();
			pwszValue = wstrValue.get();
		}
		pProfile_->setString(L"RecentAddress", wszKey, pwszValue);
	}
	return true;
}

void qm::RecentAddress::load()
{
	listAddress_.reserve(nMax_);
	for (unsigned int n = 0; n < nMax_; ++n) {
		WCHAR wszKey[32];
		swprintf(wszKey, L"Address%u", n);
		wstring_ptr wstrAddress(pProfile_->getString(L"RecentAddress", wszKey, L""));
		if (*wstrAddress.get()) {
			std::auto_ptr<AddressParser> pAddress(new AddressParser(0));
			if (MessageCreator::getAddress(wstrAddress.get(), pAddress.get()))
				listAddress_.push_back(pAddress.release());
		}
	}
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
		
		for (AddressList::const_iterator it = listAddress_.begin(); it != listAddress_.end(); ++it) {
			if (_wcsicmp(address.getMailbox(), (*it)->getMailbox()) == 0 &&
				_wcsicmp(address.getHost(), (*it)->getHost()) == 0)
				return;
		}
		
		std::auto_ptr<AddressParser> pAddress(new AddressParser(
			address.getPhrase(), address.getMailbox(), address.getHost()));
		if (listAddress_.size() >= nMax_) {
			std::for_each(listAddress_.begin() + nMax_ - 1,
				listAddress_.end(), qs::deleter<AddressParser>());
			listAddress_.resize(nMax_ - 1);
		}
		listAddress_.insert(listAddress_.begin(), pAddress.release());
	}
}
