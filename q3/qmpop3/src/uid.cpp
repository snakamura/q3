/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
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

size_t qmpop3::UIDList::getCount() const
{
	return list_.size();
}

UID* qmpop3::UIDList::getUID(size_t n) const
{
	assert(n < list_.size());
	assert(list_[n]);
	return list_[n];
}

size_t qmpop3::UIDList::getIndex(const WCHAR* pwszUID) const
{
	assert(pwszUID);
	
	List::const_iterator it = std::find_if(list_.begin(), list_.end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				string_equal<WCHAR>(),
				std::mem_fun(&UID::getUID),
				std::identity<const WCHAR*>()),
			pwszUID));
	return it != list_.end() ? it - list_.begin() : -1;
}

size_t qmpop3::UIDList::getIndex(const WCHAR* pwszUID,
								 size_t nStart) const
{
	assert(pwszUID);
	assert(nStart != -1);
	
	if (nStart < list_.size()) {
		UID* pUID = list_[nStart];
		if (pUID && wcscmp(pUID->getUID(), pwszUID) == 0)
			return nStart;
	}
	
	for (size_t n = nStart; n > 0 && nStart - n < 10; ) {
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
	OutputStreamWriter writer(&stream, false, L"utf-8");
	if (!writer)
		return false;
	BufferedWriter bufferedWriter(&writer, false);
	
	UIDListWriter w(&bufferedWriter);
	if (!w.write(*this))
		return false;
	
	if (!bufferedWriter.close())
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

UID* qmpop3::UIDList::remove(size_t n)
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
		UID* pUID = l.getUID(n);
		
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
