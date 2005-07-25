/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsconv.h>
#include <qsfile.h>
#include <qsstream.h>

#include <algorithm>

#include "uid.h"

using namespace qmpop3;
using namespace qs;


/****************************************************************************
 *
 * UID
 *
 */

qmpop3::UID::UID(const WCHAR* pwszUID,
				 unsigned int nFlags,
				 const Date& date) :
	nFlags_(nFlags),
	date_(date)
{
	wstrUID_ = allocWString(pwszUID);
}

qmpop3::UID::~UID()
{
}

const WCHAR* qmpop3::UID::getUID() const
{
	return wstrUID_.get();
}

unsigned int qmpop3::UID::getFlags() const
{
	return nFlags_;
}

const UID::Date& qmpop3::UID::getDate() const
{
	return date_;
}

void qmpop3::UID::update(unsigned int nFlags,
						 short nYear,
						 short nMonth,
						 short nDay)
{
	nFlags_ = nFlags;
	date_.nYear_ = nYear;
	date_.nMonth_ = nMonth;
	date_.nDay_ = nDay;
}


/****************************************************************************
 *
 * UIDList
 *
 */

qmpop3::UIDList::UIDList() :
	bModified_(false)
{
}

qmpop3::UIDList::~UIDList()
{
	std::for_each(list_.begin(), list_.end(), deleter<UID>());
}

unsigned int qmpop3::UIDList::getCount() const
{
	return static_cast<unsigned int>(list_.size());
}

UID* qmpop3::UIDList::getUID(unsigned int n) const
{
	assert(n < list_.size());
	assert(list_[n]);
	return list_[n];
}

unsigned int qmpop3::UIDList::getIndex(const WCHAR* pwszUID) const
{
	assert(pwszUID);
	
	for (List::const_iterator it = list_.begin(); it != list_.end(); ++it) {
		const UID* pUID = *it;
		if (pUID && wcscmp(pUID->getUID(), pwszUID) == 0)
			return static_cast<unsigned int>(it - list_.begin());
	}
	return -1;
}

unsigned int qmpop3::UIDList::getIndex(const WCHAR* pwszUID,
									   unsigned int nStart) const
{
	assert(pwszUID);
	assert(nStart != -1);
	
	if (nStart < list_.size()) {
		UID* pUID = list_[nStart];
		if (pUID && wcscmp(pUID->getUID(), pwszUID) == 0)
			return nStart;
	}
	
	for (unsigned int n = nStart; n > 0 && nStart - n < 10; ) {
		--n;
		UID* pUID = list_[n];
		if (pUID && wcscmp(pUID->getUID(), pwszUID) == 0)
			return n;
	}
	
	for (++nStart; nStart < list_.size(); ++nStart) {
		UID* pUID = list_[nStart];
		if (pUID && wcscmp(pUID->getUID(), pwszUID) == 0)
			return nStart;
	}
	
	return -1;
}

bool qmpop3::UIDList::load(const WCHAR* pwszPath)
{
	assert(pwszPath);
	
	W2T(pwszPath, ptszPath);
	if (::GetFileAttributes(ptszPath) != 0xffffffff) {
		XMLReader reader;
		UIDListContentHandler handler(this);
		reader.setContentHandler(&handler);
		if (!reader.parse(pwszPath))
			return false;
	}
	
	bModified_ = false;
	
	return true;
}

bool qmpop3::UIDList::save(const WCHAR* pwszPath) const
{
	assert(pwszPath);
	
	TemporaryFileRenamer renamer(pwszPath);
	
	FileOutputStream stream(renamer.getPath());
	if (!stream)
		return false;
	BufferedOutputStream bufferedStream(&stream, false);
	OutputStreamWriter writer(&bufferedStream, false, L"utf-8");
	if (!writer)
		return false;
	
	UIDListWriter w(&writer);
	if (!w.write(*this))
		return false;
	
	if (!writer.close())
		return false;
	
	if (!renamer.rename())
		return false;
	
	bModified_ = false;
	
	return true;
}

void qmpop3::UIDList::add(std::auto_ptr<UID> pUID)
{
	bModified_ = true;
	list_.push_back(pUID.get());
	pUID.release();
}

void qmpop3::UIDList::remove(const IndexList& l)
{
	for (IndexList::const_iterator it = l.begin(); it != l.end(); ++it) {
		assert(*it < list_.size());
		delete list_[*it];
		list_[*it] = 0;
	}
	
	list_.erase(std::remove(list_.begin(), list_.end(),
		static_cast<UID*>(0)), list_.end());
	
	bModified_ = true;
}

UID* qmpop3::UIDList::remove(unsigned int n)
{
	assert(n < list_.size());
	
	UID* pUID = list_[n];
	list_[n] = 0;
	
	bModified_ = true;
	
	return pUID;
}

void qmpop3::UIDList::setModified(bool bModified)
{
	bModified_ = bModified;
}

bool qmpop3::UIDList::isModified() const
{
	return bModified_;
}


/****************************************************************************
 *
 * UIDListContentHandler
 *
 */

qmpop3::UIDListContentHandler::UIDListContentHandler(UIDList* pList) :
	pList_(pList),
	state_(STATE_ROOT),
	nFlags_(0)
{
}

qmpop3::UIDListContentHandler::~UIDListContentHandler()
{
}

bool qmpop3::UIDListContentHandler::startElement(const WCHAR* pwszNamespaceURI,
												 const WCHAR* pwszLocalName,
												 const WCHAR* pwszQName,
												 const Attributes& attributes)
{
	if (wcscmp(pwszLocalName, L"uidl") == 0) {
		if (state_ != STATE_ROOT)
			return false;
		if (attributes.getLength() != 0)
			return false;
		state_ = STATE_UIDL;
	}
	else if (wcscmp(pwszLocalName, L"uid") == 0) {
		if (state_ != STATE_UIDL)
			return false;
		
		const WCHAR* pwszFlags = 0;
		const WCHAR* pwszDate = 0;
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrName, L"flags") == 0)
				pwszFlags = attributes.getValue(n);
			else if (wcscmp(pwszAttrName, L"date") == 0)
				pwszDate = attributes.getValue(n);
			else
				return false;
		}
		if (!pwszDate)
			return false;
		
		if (pwszFlags) {
			WCHAR* p = 0;
			nFlags_ = wcstol(pwszFlags, &p, 10);
			if (*p)
				return false;
		}
		
		int nYear = 0;
		int nMonth = 0;
		int nDay = 0;
		if (swscanf(pwszDate, L"%04d-%02d-%02d", &nYear, &nMonth, &nDay) != 3)
			return false;
		date_.nYear_ = nYear;
		date_.nMonth_ = nMonth;
		date_.nDay_ = nDay;
		
		state_ = STATE_UID;
	}
	else {
		return false;
	}
	
	return true;
}

bool qmpop3::UIDListContentHandler::endElement(const WCHAR* pwszNamespaceURI,
											   const WCHAR* pwszLocalName,
											   const WCHAR* pwszQName)
{
	if (wcscmp(pwszLocalName, L"uidl") == 0) {
		assert(state_ == STATE_UIDL);
		state_ = STATE_ROOT;
	}
	else if (wcscmp(pwszLocalName, L"uid") == 0) {
		assert(state_ == STATE_UID);
		
		std::auto_ptr<UID> pUID(new UID(buffer_.getCharArray(), nFlags_, date_));
		buffer_.remove();
		pList_->add(pUID);
		
		state_ = STATE_UIDL;
	}
	else {
		return false;
	}
	
	return true;
}

bool qmpop3::UIDListContentHandler::characters(const WCHAR* pwsz,
											   size_t nStart,
											   size_t nLength)
{
	if (state_ == STATE_UID) {
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
 * UIDListWriter
 *
 */

qmpop3::UIDListWriter::UIDListWriter(Writer* pWriter) :
	handler_(pWriter)
{
}

qmpop3::UIDListWriter::~UIDListWriter()
{
}

bool qmpop3::UIDListWriter::write(const UIDList& l)
{
	if (!handler_.startDocument())
		return false;
	
	if (!handler_.startElement(0, 0, L"uidl", DefaultAttributes()))
		return false;
	
	for (size_t n = 0; n < l.getCount(); ++n) {
		UID* pUID = l.getUID(static_cast<unsigned int>(n));
		
		WCHAR wszFlags[32];
		swprintf(wszFlags, L"%d", pUID->getFlags());
		
		const UID::Date& date = pUID->getDate();
		WCHAR wszDate[32];
		swprintf(wszDate, L"%04d-%02d-%02d",
			date.nYear_, date.nMonth_, date.nDay_);
		
		SimpleAttributes::Item items[] = {
			{ L"flags",	wszFlags	},
			{ L"date",	wszDate		}
		};
		SimpleAttributes attrs(items, countof(items));
		const WCHAR* pwszUID = pUID->getUID();
		if (!handler_.startElement(0, 0, L"uid", attrs) ||
			!handler_.characters(pwszUID, 0, wcslen(pwszUID)) ||
			!handler_.endElement(0, 0, L"uid"))
			return false;
	}
	
	if (!handler_.endElement(0, 0, L"uidl"))
		return false;
	
	if (!handler_.endDocument())
		return false;
	
	return true;
}
