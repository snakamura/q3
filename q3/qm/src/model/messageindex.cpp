/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura.
 * All rights reserved.
 *
 */

#pragma warning(disable:4786)

#include "messageindex.h"
#include "messagestore.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * MessageIndex
 *
 */

qm::MessageIndex::MessageIndex(MessageStore* pMessageStore,
							   size_t nMaxSize) :
	pMessageStore_(pMessageStore),
	nMaxSize_(nMaxSize),
	nSize_(0),
	pNewFirst_(0),
	pNewLast_(0),
	pLastGotten_(0)
{
	pNewFirst_ = new MessageIndexItem(-1);
	pNewFirst_->pNewNext_ = pNewFirst_;
	pNewFirst_->pNewPrev_ = pNewFirst_;
	pNewLast_ = pNewFirst_;
}

qm::MessageIndex::~MessageIndex()
{
#if defined _WIN32_WCE && _MSC_VER == 1202 && defined MIPS
	for (ItemMap::const_iterator it = map_.begin(); it != map_.end(); ++it)
		delete (*it).second;
#else
	std::for_each(map_.begin(), map_.end(),
		unary_compose_f_gx(
			deleter<MessageIndexItem>(),
			std::select2nd<ItemMap::value_type>()));
#endif
	delete pNewLast_;
}

wstring_ptr qm::MessageIndex::get(unsigned int nKey,
								  unsigned int nLength,
								  MessageIndexName name)
{
	wstring_ptr wstrValue;
	
	MessageIndexItem* pItem = getItem(nKey);
	if (pItem) {
		const WCHAR* pwszValue = pItem->getValue(name);
		wstrValue = allocWString(pwszValue ? pwszValue : L"");
		if (pItem != pNewFirst_) {
			MessageIndexItem* pNext = pItem->pNewNext_;
			MessageIndexItem* pPrev = pItem->pNewPrev_;
			pNext->pNewPrev_ = pPrev;
			pPrev->pNewNext_ = pNext;
			
			MessageIndexItem* pFirst = pNewFirst_;
			pNewFirst_ = pItem;
			pItem->pNewPrev_ = pNewLast_;
			pItem->pNewNext_ = pFirst;
			pFirst->pNewPrev_ = pItem;
		}
	}
	else {
		malloc_ptr<unsigned char> pData(pMessageStore_->readIndex(nKey, nLength));
		if (!pData.get())
			return allocWString(L"");
		
		const WCHAR* pwszValues[NAME_MAX] = { 0 };
		parseValues(reinterpret_cast<WCHAR*>(pData.get()), nLength/sizeof(WCHAR), pwszValues);
		
		if (nMaxSize_ != 0) {
			pItem = new MessageIndexItem(nKey, pData, pwszValues);
			
			insert(pItem);
			
			if (nSize_ >= nMaxSize_)
				remove(pNewLast_->pNewPrev_->getKey());
			else
				++nSize_;
		}
		const WCHAR* pwszValue = pwszValues[name];
		wstrValue = allocWString(pwszValue ? pwszValue : L"");
	}
	if (pItem)
		pLastGotten_ = pItem;
	
	return wstrValue;
}

void qm::MessageIndex::remove(unsigned int nKey)
{
	ItemMap::iterator it = map_.find(nKey);
	if (it != map_.end())
		remove(it);
}

malloc_size_ptr<unsigned char> qm::MessageIndex::createReplacedIndex(unsigned int nKey,
																	 unsigned int nLength,
																	 MessageIndexName name,
																	 const WCHAR* pwszValue)
{
	ByteOutputStream stream;
	
	MessageIndexItem* pItem = getItem(nKey);
	
	malloc_ptr<unsigned char> pData;
	const WCHAR* pwszValues[NAME_MAX] = { 0 };
	if (!pItem) {
		pData = pMessageStore_->readIndex(nKey, nLength);
		if (!pData.get())
			return malloc_size_ptr<unsigned char>();
		parseValues(reinterpret_cast<WCHAR*>(pData.get()), nLength/sizeof(WCHAR), pwszValues);
	}
	
	for (int n = 0; n < NAME_MAX; ++n) {
		const WCHAR* p = 0;
		if (n != name)
			p = pItem ? pItem->getValue(static_cast<MessageIndexName>(n)) : pwszValues[n];
		else
			p = pwszValue;
		if (!writeToStream(&stream, p))
			return malloc_size_ptr<unsigned char>();
	}
	
	size_t nLen = stream.getLength();
	return malloc_size_ptr<unsigned char>(stream.releaseBuffer(), nLen);
}

malloc_size_ptr<unsigned char> qm::MessageIndex::createIndex(const Message& header,
															 const WCHAR* pwszLabel)
{
	ByteOutputStream stream;
	
	const WCHAR* pwszFields[] = {
		L"From",
		L"To"
	};
	for (int n = 0; n < countof(pwszFields); ++n) {
		wstring_ptr wstrNames;
		AddressListParser address;
		if (header.getField(pwszFields[n], &address) == Part::FIELD_EXIST)
			wstrNames = address.getNames();
		if (!writeToStream(&stream, wstrNames.get()))
			return malloc_size_ptr<unsigned char>();
	}
	
	const WCHAR* pwszSubject = 0;
	UnstructuredParser subject;
	if (header.getField(L"Subject", &subject) == Part::FIELD_EXIST)
		pwszSubject = subject.getValue();
	if (!writeToStream(&stream, pwszSubject))
		return malloc_size_ptr<unsigned char>();
	
	const WCHAR* pwszMessageId = 0;
	MessageIdParser messageId;
	if (header.getField(L"Message-Id", &messageId) == Part::FIELD_EXIST)
		pwszMessageId = messageId.getMessageId();
	if (!writeToStream(&stream, pwszMessageId))
		return malloc_size_ptr<unsigned char>();
	
	wstring_ptr wstrReference(PartUtil(header).getReference());
	if (!writeToStream(&stream, wstrReference.get()))
		return malloc_size_ptr<unsigned char>();
	
	if (!writeToStream(&stream, pwszLabel))
		return malloc_size_ptr<unsigned char>();
	
	size_t nLen = stream.getLength();
	return malloc_size_ptr<unsigned char>(stream.releaseBuffer(), nLen);
}

MessageIndexItem* qm::MessageIndex::getItem(unsigned int nKey) const
{
	MessageIndexItem* pItem = 0;
	if (pLastGotten_ && pLastGotten_->getKey() == nKey)
		pItem = pLastGotten_;
	if (!pItem) {
		ItemMap::const_iterator it = map_.find(nKey);
		if (it != map_.end())
			pItem = (*it).second;
	}
	return pItem;
}

void qm::MessageIndex::insert(MessageIndexItem* pItem)
{
	map_.insert(std::make_pair(pItem->getKey(), pItem));
	
	pNewFirst_->pNewPrev_ = pItem;
	pItem->pNewNext_ = pNewFirst_;
	pNewFirst_ = pItem;
}

void qm::MessageIndex::remove(ItemMap::iterator it)
{
	assert(it != map_.end());
	
	MessageIndexItem* pItem = (*it).second;
	
#if defined _WIN32_WCE && _MSC_VER == 1202 && defined MIPS
	map_.erase((*it).first);
#else
	map_.erase(it);
#endif
	
	if (pItem == pNewFirst_)
		pNewFirst_ = pItem->pNewNext_;
	else
		pItem->pNewPrev_->pNewNext_ = pItem->pNewNext_;
	pItem->pNewNext_->pNewPrev_ = pItem->pNewPrev_;
	
	if (pItem == pLastGotten_)
		pLastGotten_ = 0;
	
	delete pItem;
}

void qm::MessageIndex::parseValues(WCHAR* p,
								   size_t nLen,
								   const WCHAR** ppwszValues)
{
	int nValue = 0;
	const WCHAR* pStart = p;
	for (size_t n = 0; n < nLen && nValue < NAME_MAX; ++n, ++p) {
		if (*p == L'\n') {
			*p = L'\0';
			ppwszValues[nValue++] = pStart;
			pStart = p + 1;
		}
	}
	while (nValue < NAME_MAX)
		ppwszValues[nValue++] = 0;
}

bool qm::MessageIndex::writeToStream(OutputStream* pStream,
									 const WCHAR* pwsz)
{
	assert(pStream);
	
	if (pwsz && *pwsz) {
		const unsigned char* p = reinterpret_cast<const unsigned char*>(pwsz);
		size_t nLen = wcslen(pwsz)*sizeof(WCHAR);
		if (pStream->write(p, nLen) == -1)
			return false;
	}
	if (pStream->write(reinterpret_cast<const unsigned char*>(L"\n"), sizeof(WCHAR)) == -1)
		return false;
	
	return true;
}


/****************************************************************************
 *
 * MessageIndexItem
 *
 */

qm::MessageIndexItem::MessageIndexItem(unsigned int nKey) :
	nKey_(nKey),
	pNewNext_(0),
	pNewPrev_(0)
{
	memset(pwszValues_, 0, sizeof(pwszValues_));
}

qm::MessageIndexItem::MessageIndexItem(unsigned int nKey,
									   malloc_ptr<unsigned char> pData,
									   const WCHAR* pwszValues[]) :
	nKey_(nKey),
	pData_(pData),
	pNewNext_(0),
	pNewPrev_(0)
{
	memcpy(pwszValues_, pwszValues, sizeof(pwszValues_));
}

qm::MessageIndexItem::~MessageIndexItem()
{
}

unsigned int qm::MessageIndexItem::getKey() const
{
	return nKey_;
}

const WCHAR* qm::MessageIndexItem::getValue(MessageIndexName name) const
{
	assert(name < NAME_MAX);
	return pwszValues_[name];
}
