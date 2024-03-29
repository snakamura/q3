/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#pragma warning(disable:4786)

#include <qsconv.h>
#include <qsfile.h>

#include <boost/bind.hpp>

#include "groups.h"
#include "nntp.h"

using namespace qmnntp;
using namespace qs;


/****************************************************************************
 *
 * Groups
 *
 */

qmnntp::Groups::Groups(const WCHAR* pwszPath) :
	bModified_(false)
{
	wstrPath_ = allocWString(pwszPath);
	
	listGroup_.reserve(10240);
	
	if (!load())
		clear();
}

qmnntp::Groups::~Groups()
{
	clear();
}

const Groups::GroupList& qmnntp::Groups::getGroupList() const
{
	return listGroup_;
}

const WCHAR* qmnntp::Groups::getDate() const
{
	return wstrDate_.get();
}

const WCHAR* qmnntp::Groups::getTime() const
{
	return wstrTime_.get();
}

void qmnntp::Groups::add(const GroupsData* pData,
						 const WCHAR* pwszDate,
						 const WCHAR* pwszTime)
{
	setDateTime(pwszDate, pwszTime);
	
	size_t nCount = pData->getCount();
	if (nCount == 0)
		return;
	
	listGroup_.reserve(listGroup_.size() + nCount);
	
	for (size_t n = 0; n < nCount; ++n) {
		const GroupsData::Item& item = pData->getItem(n);
		wstring_ptr wstrName(mbs2wcs(item.pszGroup_));
		std::auto_ptr<Group> pGroup(new Group(wstrName.get()));
		add(pGroup);
	}
}

bool qmnntp::Groups::save() const
{
	if (!bModified_)
		return true;
	
	TemporaryFileRenamer renamer(wstrPath_.get());
	
	FileOutputStream stream(renamer.getPath());
	if (!stream)
		return false;
	BufferedOutputStream bufferedStream(&stream, false);
	OutputStreamWriter writer(&bufferedStream, false, L"utf-8");
	if (!writer)
		return false;
	
	GroupsWriter w(&writer, L"utf-8");
	if (!w.write(this) ||
		!writer.close() ||
		!renamer.rename())
		return false;
	
	bModified_ = false;
	
	return true;
}

void qmnntp::Groups::add(std::auto_ptr<Group> pGroup)
{
	GroupList::iterator it = std::lower_bound(
		listGroup_.begin(), listGroup_.end(), pGroup.get(),
		boost::bind(string_less<WCHAR>(),
			boost::bind(&Group::getName, _1),
			boost::bind(&Group::getName, _2)));
	if (it != listGroup_.end() && wcscmp((*it)->getName(), pGroup->getName()) == 0)
		return;
	
	listGroup_.insert(it, pGroup.get());
	pGroup.release();
	
	bModified_ = true;
}

void qmnntp::Groups::setDateTime(const WCHAR* pwszDate,
								 const WCHAR* pwszTime)
{
	assert(pwszDate);
	assert(pwszTime);
	
	wstrDate_ = allocWString(pwszDate);
	wstrTime_ = allocWString(pwszTime);
	
	bModified_ = true;
}

bool qmnntp::Groups::load()
{
	if (File::isFileExisting(wstrPath_.get())) {
		XMLReader reader;
		GroupsContentHandler handler(this);
		reader.setContentHandler(&handler);
		if (!reader.parse(wstrPath_.get()))
			return false;
	}
	
	bModified_ = false;
	
	return true;
}

void qmnntp::Groups::clear()
{
	std::for_each(listGroup_.begin(), listGroup_.end(),
		boost::checked_deleter<Group>());
}


/****************************************************************************
 *
 * Group
 *
 */

qmnntp::Group::Group(const WCHAR* pwszName)
{
	wstrName_ = allocWString(pwszName);
}

qmnntp::Group::~Group()
{
}

const WCHAR* qmnntp::Group::getName() const
{
	return wstrName_.get();
}


/****************************************************************************
 *
 * GroupsContentHandler
 *
 */

qmnntp::GroupsContentHandler::GroupsContentHandler(Groups* pGroups) :
	pGroups_(pGroups),
	state_(STATE_ROOT)
{
}

qmnntp::GroupsContentHandler::~GroupsContentHandler()
{
}

bool qmnntp::GroupsContentHandler::startElement(const WCHAR* pwszNamespaceURI,
												const WCHAR* pwszLocalName,
												const WCHAR* pwszQName,
												const Attributes& attributes)
{
	if (wcscmp(pwszLocalName, L"groups") == 0) {
		if (state_ != STATE_ROOT)
			return false;
		
		const WCHAR* pwszDate = 0;
		const WCHAR* pwszTime = 0;
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrName, L"date") == 0)
				pwszDate = attributes.getValue(n);
			else if (wcscmp(pwszAttrName, L"time") == 0)
				pwszTime = attributes.getValue(n);
			else
				return false;
		}
		
		if (pwszDate && pwszTime)
			pGroups_->setDateTime(pwszDate, pwszTime);
		
		state_ = STATE_GROUPS;
	}
	else if (wcscmp(pwszLocalName, L"group") == 0) {
		if (state_ != STATE_GROUPS)
			return false;
		state_ = STATE_GROUP;
	}
	else {
		return false;
	}
	
	return true;
}

bool qmnntp::GroupsContentHandler::endElement(const WCHAR* pwszNamespaceURI,
											  const WCHAR* pwszLocalName,
											  const WCHAR* pwszQName)
{
	if (wcscmp(pwszLocalName, L"groups") == 0) {
		assert(state_ == STATE_GROUPS);
		state_ = STATE_ROOT;
	}
	else if (wcscmp(pwszLocalName, L"group") == 0) {
		assert(state_ == STATE_GROUP);
		
		if (buffer_.getLength() == 0)
			return false;
		
		std::auto_ptr<Group> pGroup(new Group(buffer_.getCharArray()));
		pGroups_->add(pGroup);
		buffer_.remove();
		
		state_ = STATE_GROUPS;
	}
	else {
		return false;
	}
	
	return true;
}

bool qmnntp::GroupsContentHandler::characters(const WCHAR* pwsz,
											  size_t nStart,
											  size_t nLength)
{
	if (state_ == STATE_GROUP) {
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
 * GroupsWriter
 *
 */

qmnntp::GroupsWriter::GroupsWriter(Writer* pWriter,
								   const WCHAR* pwszEncoding) :
	handler_(pWriter, pwszEncoding)
{
}

qmnntp::GroupsWriter::~GroupsWriter()
{
}

bool qmnntp::GroupsWriter::write(const Groups* pGroups)
{
	const SimpleAttributes::Item items[] = {
		{ L"date",	pGroups->getDate(),	!pGroups->getDate()	},
		{ L"time",	pGroups->getTime(),	!pGroups->getTime()	}
	};
	SimpleAttributes attrs(items, countof(items));
	if (!handler_.startDocument() ||
		!handler_.startElement(0, 0, L"groups", attrs))
		return false;
	
	const Groups::GroupList& l = pGroups->getGroupList();
	for (Groups::GroupList::const_iterator it = l.begin(); it != l.end(); ++it) {
		const Group* pGroup = *it;
		const WCHAR* pwszName = pGroup->getName();
		if (!handler_.startElement(0, 0, L"group", DefaultAttributes()) ||
			!handler_.characters(pwszName, 0, wcslen(pwszName)) ||
			!handler_.endElement(0, 0, L"group"))
			return false;
	}
	
	if (!handler_.endElement(0, 0, L"groups") ||
		!handler_.endDocument())
		return false;
	
	return true;
}
