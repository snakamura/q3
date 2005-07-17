/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __RECENTADDRESS_H__
#define __RECENTADDRESS_H__

#include <qm.h>

#include <qsmime.h>
#include <qsprofile.h>

#include <vector>


namespace qm {

class RecentAddress;

class AddressBook;
class Message;


/****************************************************************************
 *
 * RecentAddress
 *
 */

class RecentAddress
{
public:
	typedef std::vector<qs::AddressParser*> AddressList;

public:
	RecentAddress(AddressBook* pAddressBook,
				  qs::Profile* pProfile);
	~RecentAddress();

public:
	const AddressList& getAddresses() const;
	void add(const Message& msg);
	void save() const;

private:
	void load();
	void add(const qs::AddressListParser& addressList);
	void add(const qs::AddressParser& address);

private:
	RecentAddress(const RecentAddress&);
	RecentAddress& operator=(const RecentAddress&);

private:
	AddressBook* pAddressBook_;
	qs::Profile* pProfile_;
	unsigned int nMax_;
	AddressList listAddress_;
};

}

#endif // __RECENTADDRESS_H__
