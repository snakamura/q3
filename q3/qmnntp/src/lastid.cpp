/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsconv.h>
#include <qsfile.h>
#include <qsnew.h>
#include <qsstl.h>
#include <qsstream.h>

#include "lastid.h"

#pragma warning(disable:4786)

using namespace qmnntp;
using namespace qs;


/****************************************************************************
 *
 * LastIdList
 *
 */

qmnntp::LastIdList::LastIdList(const WCHAR* pwszPath, QSTATUS* pstatus) :
	wstrPath_(0),
	bModified_(false)
{
	DECLARE_QSTATUS();
	
	wstrPath_ = allocWString(pwszPath);
	if (!wstrPath_) {
		*pstatus = QSTATUS_OUTOFMEMORY;
		return;
	}
	
	status = load();
	CHECK_QSTATUS_SET(pstatus);
}

qmnntp::LastIdList::~LastIdList()
{
	freeWString(wstrPath_);
	std::for_each(listId_.begin(), listId_.end(),
		unary_compose_f_gx(
			string_free<WSTRING>(),
			std::select1st<IdList::value_type>()));
}

const LastIdList::IdList& qmnntp::LastIdList::getList() const
{
	return listId_;
}

unsigned int qmnntp::LastIdList::getLastId(const WCHAR* pwszName) const
{
	IdList::const_iterator it = std::find_if(listId_.begin(), listId_.end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				string_equal<WCHAR>(),
				std::select1st<IdList::value_type>(),
				std::identity<const WCHAR*>()),
			pwszName));
	return it != listId_.end() ? (*it).second : 0;
}

QSTATUS qmnntp::LastIdList::setLastId(const WCHAR* pwszName, unsigned int nId)
{
	DECLARE_QSTATUS();
	
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
		string_ptr<WSTRING> wstrName(allocWString(pwszName));
		if (!wstrName.get())
			return QSTATUS_OUTOFMEMORY;
		status = STLWrapper<IdList>(listId_).push_back(
			IdList::value_type(wstrName.get(), nId));
		CHECK_QSTATUS();
		wstrName.release();
	}
	
	bModified_ = true;
	
	return QSTATUS_SUCCESS;
}

bool qmnntp::LastIdList::isModified() const
{
	return bModified_;
}

QSTATUS qmnntp::LastIdList::save()
{
	DECLARE_QSTATUS();
	
	TemporaryFileRenamer renamer(wstrPath_, &status);
	CHECK_QSTATUS();
	
	FileOutputStream stream(renamer.getPath(), &status);
	CHECK_QSTATUS();
	OutputStreamWriter writer(&stream, false, L"utf-8", &status);
	CHECK_QSTATUS();
	BufferedWriter bufferedWriter(&writer, false, &status);
	CHECK_QSTATUS();
	
	LastIdWriter w(&bufferedWriter, &status);
	CHECK_QSTATUS();
	status = w.write(*this);
	CHECK_QSTATUS();
	
	status = bufferedWriter.close();
	CHECK_QSTATUS();
	
	status = renamer.rename();
	CHECK_QSTATUS();
	
	bModified_ = false;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmnntp::LastIdList::load()
{
	DECLARE_QSTATUS();
	
	W2T(wstrPath_, ptszPath);
	if (::GetFileAttributes(ptszPath) != 0xffffffff) {
		XMLReader reader(&status);
		CHECK_QSTATUS();
		LastIdContentHandler handler(this, &status);
		CHECK_QSTATUS();
		reader.setContentHandler(&handler);
		status = reader.parse(wstrPath_);
		CHECK_QSTATUS();
	}
	
	bModified_ = false;
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * LastIdContentHandler
 *
 */

qmnntp::LastIdContentHandler::LastIdContentHandler(
	LastIdList* pList, QSTATUS* pstatus) :
	DefaultHandler(pstatus),
	pList_(pList),
	state_(STATE_ROOT),
	wstrName_(0),
	pBuffer_(0)
{
	DECLARE_QSTATUS();
	
	status = newQsObject(&pBuffer_);
	CHECK_QSTATUS_SET(pstatus);
}

qmnntp::LastIdContentHandler::~LastIdContentHandler()
{
	freeWString(wstrName_);
	delete pBuffer_;
}

QSTATUS qmnntp::LastIdContentHandler::startElement(
	const WCHAR* pwszNamespaceURI, const WCHAR* pwszLocalName,
	const WCHAR* pwszQName, const Attributes& attributes)
{
	DECLARE_QSTATUS();
	
	if (wcscmp(pwszLocalName, L"lastIdList") == 0) {
		if (state_ != STATE_ROOT)
			return QSTATUS_FAIL;
		state_ = STATE_LASTIDLIST;
	}
	else if (wcscmp(pwszLocalName, L"lastId") == 0) {
		if (state_ != STATE_LASTIDLIST)
			return QSTATUS_FAIL;
		
		const WCHAR* pwszName = 0;
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrName, L"name") == 0)
				pwszName = attributes.getValue(n);
			else
				return QSTATUS_FAIL;
		}
		if (!pwszName)
			return QSTATUS_FAIL;
		
		assert(!wstrName_);
		wstrName_ = allocWString(pwszName);
		if (!wstrName_)
			return QSTATUS_OUTOFMEMORY;
		
		state_ = STATE_LASTID;
	}
	else {
		return QSTATUS_FAIL;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmnntp::LastIdContentHandler::endElement(const WCHAR* pwszNamespaceURI,
	const WCHAR* pwszLocalName, const WCHAR* pwszQName)
{
	DECLARE_QSTATUS();
	
	if (wcscmp(pwszLocalName, L"lastIdList") == 0) {
		assert(state_ == STATE_LASTIDLIST);
		state_ = STATE_ROOT;
	}
	else if (wcscmp(pwszLocalName, L"lastId") == 0) {
		assert(state_ == STATE_LASTID);
		
		if (pBuffer_->getLength() == 0)
			return QSTATUS_OUTOFMEMORY;
		
		WCHAR* p = 0;
		unsigned int nId = wcstol(pBuffer_->getCharArray(), &p, 10);
		if (*p)
			return QSTATUS_OUTOFMEMORY;
		
		status = pList_->setLastId(wstrName_, nId);
		CHECK_QSTATUS();
		
		pBuffer_->remove();
		freeWString(wstrName_);
		wstrName_ = 0;
		
		state_ = STATE_LASTIDLIST;
	}
	else {
		return QSTATUS_FAIL;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmnntp::LastIdContentHandler::characters(
	const WCHAR* pwsz, size_t nStart, size_t nLength)
{
	DECLARE_QSTATUS();
	
	if (state_ == STATE_LASTID) {
		status = pBuffer_->append(pwsz + nStart, nLength);
		CHECK_QSTATUS();
	}
	else {
		const WCHAR* p = pwsz + nStart;
		for (size_t n = 0; n < nLength; ++n, ++p) {
			if (*p != L' ' && *p != L'\t' && *p != '\n')
				return QSTATUS_FAIL;
		}
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * LastIdWriter
 *
 */

qmnntp::LastIdWriter::LastIdWriter(Writer* pWriter, QSTATUS* pstatus) :
	handler_(pWriter, pstatus)
{
}

qmnntp::LastIdWriter::~LastIdWriter()
{
}

QSTATUS qmnntp::LastIdWriter::write(const LastIdList& l)
{
	DECLARE_QSTATUS();
	
	status = handler_.startDocument();
	CHECK_QSTATUS();
	
	status = handler_.startElement(0, 0, L"lastIdList", DefaultAttributes());
	CHECK_QSTATUS();
	
	const LastIdList::IdList& listId = l.getList();
	LastIdList::IdList::const_iterator it = listId.begin();
	while (it != listId.end()) {
		class Attrs : public DefaultAttributes
		{
		public:
			Attrs(const WCHAR* pwszName) :
				pwszName_(pwszName)
			{
			}
			
			~Attrs()
			{
			}
		
		public:
			virtual int getLength() const
			{
				return 1;
			}
			
			virtual const WCHAR* getQName(int nIndex) const
			{
				assert(nIndex == 0);
				return L"name";
			}
			
			virtual const WCHAR* getValue(int nIndex) const
			{
				assert(nIndex == 0);
				return pwszName_;
			}
		
		private:
			const WCHAR* pwszName_;
		} attrs((*it).first);
		
		status = handler_.startElement(0, 0, L"lastId", attrs);
		CHECK_QSTATUS();
		
		WCHAR wszId[32];
		swprintf(wszId, L"%u", (*it).second);
		status = handler_.characters(wszId, 0, wcslen(wszId));
		
		status = handler_.endElement(0, 0, L"lastId");
		CHECK_QSTATUS();
		
		++it;
	}
	
	status = handler_.endElement(0, 0, L"lastIdList");
	CHECK_QSTATUS();
	
	status = handler_.endDocument();
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}
