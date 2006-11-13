/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#pragma warning(disable:4786)

#include <qsconv.h>
#include <qsfile.h>
#include <qsstl.h>
#include <qsstream.h>

#include "lastid.h"

using namespace qmnntp;
using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * LastIdList
 *
 */

qmnntp::LastIdList::LastIdList(const WCHAR* pwszPath) :
	bModified_(false)
{
#ifndef NDEBUG
	nLock_ = 0;
#endif
	
	wstrPath_ = allocWString(pwszPath);
	
	if (!load()) {
		// TODO
	}
}

qmnntp::LastIdList::~LastIdList()
{
	std::for_each(listId_.begin(), listId_.end(),
		unary_compose_f_gx(
			string_free<WSTRING>(),
			std::select1st<IdList::value_type>()));
}

const LastIdList::IdList& qmnntp::LastIdList::getList() const
{
	assert(isLocked());
	return listId_;
}

unsigned int qmnntp::LastIdList::getLastId(const WCHAR* pwszName) const
{
	Lock<LastIdList> lock(*this);
	
	IdList::const_iterator it = std::find_if(listId_.begin(), listId_.end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				string_equal<WCHAR>(),
				std::select1st<IdList::value_type>(),
				std::identity<const WCHAR*>()),
			pwszName));
	return it != listId_.end() ? (*it).second : 0;
}

void qmnntp::LastIdList::setLastId(const WCHAR* pwszName,
								   unsigned int nId)
{
	Lock<LastIdList> lock(*this);
	
	IdList::iterator it = std::find_if(listId_.begin(), listId_.end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				string_equal<WCHAR>(),
				std::select1st<IdList::value_type>(),
				std::identity<const WCHAR*>()),
			pwszName));
	if (it != listId_.end()) {
		(*it).second = nId;
	}
	else {
		wstring_ptr wstrName(allocWString(pwszName));
		listId_.push_back(IdList::value_type(wstrName.get(), nId));
		wstrName.release();
	}
	
	bModified_ = true;
}

void qmnntp::LastIdList::removeLastId(const WCHAR* pwszName)
{
	assert(isLocked());
	
	IdList::iterator it = std::find_if(listId_.begin(), listId_.end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				string_equal<WCHAR>(),
				std::select1st<IdList::value_type>(),
				std::identity<const WCHAR*>()),
			pwszName));
	if (it != listId_.end()) {
		freeWString((*it).first);
		listId_.erase(it);
	}
	
	bModified_ = true;
}

bool qmnntp::LastIdList::save()
{
	Lock<LastIdList> lock(*this);
	
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
	
	LastIdWriter w(&writer, L"utf-8");
	if (!w.write(*this) ||
		!writer.close() ||
		!renamer.rename())
		return false;
	
	bModified_ = false;
	
	return true;
}

void qmnntp::LastIdList::lock() const
{
	cs_.lock();
#ifndef NDEBUG
	++nLock_;
#endif
}

void qmnntp::LastIdList::unlock() const
{
#ifndef NDEBUG
	--nLock_;
#endif
	cs_.unlock();
}

#ifndef NDEBUG
bool qmnntp::LastIdList::isLocked() const
{
	return nLock_ != 0;
}
#endif

bool qmnntp::LastIdList::load()
{
	if (File::isFileExisting(wstrPath_.get())) {
		XMLReader reader;
		LastIdContentHandler handler(this);
		reader.setContentHandler(&handler);
		if (!reader.parse(wstrPath_.get()))
			return false;
	}
	
	bModified_ = false;
	
	return true;
}


/****************************************************************************
 *
 * LastIdManager
 *
 */

qmnntp::LastIdManager::LastIdManager()
{
}

qmnntp::LastIdManager::~LastIdManager()
{
	std::for_each(map_.begin(), map_.end(),
		unary_compose_f_gx(
			qs::deleter<LastIdList>(),
			std::select2nd<Map::value_type>()));
}

LastIdList* qmnntp::LastIdManager::get(Account* pAccount)
{
	Lock<CriticalSection> lock(cs_);
	
	Map::iterator it = std::find_if(map_.begin(), map_.end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				std::equal_to<Account*>(),
				std::select1st<Map::value_type>(),
				std::identity<Account*>()),
			pAccount));
	if (it != map_.end())
		return (*it).second;
	
	wstring_ptr wstrPath(concat(pAccount->getPath(), L"\\lastid.xml"));
	std::auto_ptr<LastIdList> pLastIdList(new LastIdList(wstrPath.get()));
	map_.push_back(std::make_pair(pAccount, pLastIdList.get()));
	return pLastIdList.release();
}


/****************************************************************************
 *
 * LastIdContentHandler
 *
 */

qmnntp::LastIdContentHandler::LastIdContentHandler(LastIdList* pList) :
	pList_(pList),
	state_(STATE_ROOT)
{
}

qmnntp::LastIdContentHandler::~LastIdContentHandler()
{
}

bool qmnntp::LastIdContentHandler::startElement(const WCHAR* pwszNamespaceURI,
												const WCHAR* pwszLocalName,
												const WCHAR* pwszQName,
												const Attributes& attributes)
{
	if (wcscmp(pwszLocalName, L"lastIdList") == 0) {
		if (state_ != STATE_ROOT)
			return false;
		state_ = STATE_LASTIDLIST;
	}
	else if (wcscmp(pwszLocalName, L"lastId") == 0) {
		if (state_ != STATE_LASTIDLIST)
			return false;
		
		const WCHAR* pwszName = 0;
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrName, L"name") == 0)
				pwszName = attributes.getValue(n);
			else
				return false;
		}
		if (!pwszName)
			return false;
		
		assert(!wstrName_.get());
		wstrName_ = allocWString(pwszName);
		
		state_ = STATE_LASTID;
	}
	else {
		return false;
	}
	
	return true;
}

bool qmnntp::LastIdContentHandler::endElement(const WCHAR* pwszNamespaceURI,
											  const WCHAR* pwszLocalName,
											  const WCHAR* pwszQName)
{
	if (wcscmp(pwszLocalName, L"lastIdList") == 0) {
		assert(state_ == STATE_LASTIDLIST);
		state_ = STATE_ROOT;
	}
	else if (wcscmp(pwszLocalName, L"lastId") == 0) {
		assert(state_ == STATE_LASTID);
		
		if (buffer_.getLength() == 0)
			return false;
		
		WCHAR* p = 0;
		unsigned int nId = wcstol(buffer_.getCharArray(), &p, 10);
		if (*p)
			return false;
		
		pList_->setLastId(wstrName_.get(), nId);
		
		buffer_.remove();
		wstrName_.reset(0);
		
		state_ = STATE_LASTIDLIST;
	}
	else {
		return false;
	}
	
	return true;
}

bool qmnntp::LastIdContentHandler::characters(const WCHAR* pwsz,
											  size_t nStart,
											  size_t nLength)
{
	if (state_ == STATE_LASTID) {
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
 * LastIdWriter
 *
 */

qmnntp::LastIdWriter::LastIdWriter(Writer* pWriter,
								   const WCHAR* pwszEncoding) :
	handler_(pWriter, pwszEncoding)
{
}

qmnntp::LastIdWriter::~LastIdWriter()
{
}

bool qmnntp::LastIdWriter::write(const LastIdList& l)
{
	if (!handler_.startDocument() ||
		!handler_.startElement(0, 0, L"lastIdList", DefaultAttributes()))
		return false;
	
	const LastIdList::IdList& listId = l.getList();
	for (LastIdList::IdList::const_iterator it = listId.begin(); it != listId.end(); ++it) {
		SimpleAttributes attrs(L"name", (*it).first);
		WCHAR wszId[32];
		_snwprintf(wszId, countof(wszId), L"%u", (*it).second);
		if (!handler_.startElement(0, 0, L"lastId", attrs) ||
			!handler_.characters(wszId, 0, wcslen(wszId)) ||
			!handler_.endElement(0, 0, L"lastId"))
			return false;
	}
	
	if (!handler_.endElement(0, 0, L"lastIdList") ||
		!handler_.endDocument())
		return false;
	
	return true;
}
