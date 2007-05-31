/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __QMMESSAGEHOLDERLIST_H__
#define __QMMESSAGEHOLDERLIST_H__

#include <qsassert.h>

#include <vector>

#include <boost/bind.hpp>

namespace qm {

class MessageHolder;
class MessagePtr;

typedef std::vector<MessageHolder*> MessageHolderList;
typedef std::vector<MessagePtr> MessagePtrList;

class Account;


/****************************************************************************
 *
 * MessageThreadUtil
 *
 */

namespace MessageThreadUtil {

template<class List, class GetMessageHolder, class CreateItemWithMessageIdHash, class GetMessageIdHash, class GetParentItem, class SetParentItem>
void makeParentLink(Account* pAccount,
					const List& l,
					GetMessageHolder getMessageHolder,
					CreateItemWithMessageIdHash createItemWithMessageIdHash,
					GetMessageIdHash getMessageIdHash,
					GetParentItem getParentItem,
					SetParentItem setParentItem)
{
	if (l.empty())
		return;
	
	if (!pAccount->isIndexPrepared(getMessageHolder(l[l.size()/2]))) {
		MessageHolderList listMessageHolder;
		listMessageHolder.resize(l.size());
		std::transform(l.begin(), l.end(), listMessageHolder.begin(), getMessageHolder);
		pAccount->prepareIndex(listMessageHolder);
	}
	
	List listSortedByMessageIdHash(l);
	std::sort(listSortedByMessageIdHash.begin(), listSortedByMessageIdHash.end(),
		boost::bind(getMessageIdHash, _1) < boost::bind(getMessageIdHash, _2));
	
	List listSortedByPointer(l);
	std::sort(listSortedByPointer.begin(), listSortedByPointer.end());
	
	for (List::const_iterator it = l.begin(); it != l.end(); ++it)
		makeItemParentLink(listSortedByMessageIdHash, listSortedByPointer,
			*it, getMessageHolder, createItemWithMessageIdHash,
			getMessageIdHash, getParentItem, setParentItem);
}

template<class List, class GetMessageHolder, class CreateItemWithMessageIdHash, class GetMessageIdHash, class GetParentItem, class SetParentItem>
void makeItemParentLink(const List& listSortedByMessageIdHash,
						const List& listSortedByPointer,
						typename List::value_type item,
						GetMessageHolder getMessageHolder,
						CreateItemWithMessageIdHash createItemWithMessageIdHash,
						GetMessageIdHash getMessageIdHash,
						GetParentItem getParentItem,
						SetParentItem setParentItem)
{
	MessageHolder* pmh = getMessageHolder(item);
	List::value_type parentItem = getParentItem(item);
	if (!parentItem) {
		unsigned int nReferenceHash = pmh->getReferenceHash();
		if (nReferenceHash != 0) {
			CreateItemWithMessageIdHash::result_type findItem(
				createItemWithMessageIdHash(nReferenceHash));
			List::const_iterator it = std::lower_bound(
				listSortedByMessageIdHash.begin(),
				listSortedByMessageIdHash.end(), &findItem,
				boost::bind(getMessageIdHash, _1) < boost::bind(getMessageIdHash, _2));
			if  (it != listSortedByMessageIdHash.end() &&
				getMessageHolder(*it)->getMessageIdHash() == nReferenceHash) {
				bool bFound = false;
				wstring_ptr wstrReference(pmh->getReference());
				assert(*wstrReference.get());
				while  (it != listSortedByMessageIdHash.end() &&
					(*it)->getMessageHolder()->getMessageIdHash() == nReferenceHash) {
					wstring_ptr wstrMessageId(getMessageHolder(*it)->getMessageId());
					if (wcscmp(wstrReference.get(), wstrMessageId.get()) == 0) {
						bFound = true;
						break;
					}
					++it;
				}
				if (bFound && !isAncestorOf(item, *it, getParentItem))
					setParentItem(item, *it);
			}
		}
	}
	else {
		List::const_iterator it = std::lower_bound(
			listSortedByPointer.begin(),
			listSortedByPointer.end(), parentItem);
		if (it == listSortedByPointer.end() || *it != parentItem)
			setParentItem(item, 0);
	}
}

template <class Item, class GetParentItem>
bool isAncestorOf(const Item& item1,
				  const Item& item2,
				  GetParentItem getParentItem)
{
	if (item1 == item2)
		return true;
	
	Item parent = getParentItem(item2);
	if (!parent)
		return false;
	
	return isAncestorOf(item1, parent, getParentItem);
}

}

}

#endif // __QMMESSAGEHOLDERLIST_H__
