/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qsconv.h>
#include <qserror.h>
#include <qsfile.h>
#include <qsnew.h>
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

qmpop3::UID::UID(const WCHAR* pwszUID, unsigned int nFlags,
	const Date& date, QSTATUS* pstatus) :
	wstrUID_(0),
	nFlags_(nFlags),
	date_(date)
{
	string_ptr<WSTRING> wstrUID(allocWString(pwszUID));
	if (!wstrUID.get()) {
		*pstatus = QSTATUS_OUTOFMEMORY;
		return;
	}
	wstrUID_ = wstrUID.release();
}

qmpop3::UID::~UID()
{
	freeWString(wstrUID_);
}

const WCHAR* qmpop3::UID::getUID() const
{
	return wstrUID_;
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
	short nYear, short nMonth, short nDay)
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

qmpop3::UIDList::UIDList(QSTATUS* pstatus) :
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

size_t qmpop3::UIDList::getIndex(const WCHAR* pwszUID, size_t nStart) const
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

QSTATUS qmpop3::UIDList::load(const WCHAR* pwszPath)
{
	assert(pwszPath);
	
	DECLARE_QSTATUS();
	
	W2T(pwszPath, ptszPath);
	if (::GetFileAttributes(ptszPath) != 0xffffffff) {
		XMLReader reader(&status);
		CHECK_QSTATUS();
		UIDListContentHandler handler(this, &status);
		CHECK_QSTATUS();
		reader.setContentHandler(&handler);
		status = reader.parse(pwszPath);
		CHECK_QSTATUS();
	}
	
	bModified_ = false;
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmpop3::UIDList::save(const WCHAR* pwszPath) const
{
	assert(pwszPath);
	
	DECLARE_QSTATUS();
	
	TemporaryFileRenamer renamer(pwszPath, &status);
	CHECK_QSTATUS();
	
	FileOutputStream stream(renamer.getPath(), &status);
	CHECK_QSTATUS();
	OutputStreamWriter writer(&stream, false, L"utf-8", &status);
	CHECK_QSTATUS();
	BufferedWriter bufferedWriter(&writer, false, &status);
	CHECK_QSTATUS();
	
	UIDListWriter w(&bufferedWriter, &status);
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

QSTATUS qmpop3::UIDList::add(UID* pUID)
{
	bModified_ = true;
	return STLWrapper<List>(list_).push_back(pUID);
}

void qmpop3::UIDList::remove(const IndexList& l)
{
	IndexList::const_iterator it = l.begin();
	while (it != l.end()) {
		assert(*it < list_.size());
		delete list_[*it];
		list_[*it] = 0;
		++it;
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

qmpop3::UIDListContentHandler::UIDListContentHandler(
	UIDList* pList, QSTATUS* pstatus) :
	DefaultHandler(pstatus),
	pList_(pList),
	state_(STATE_ROOT),
	nFlags_(0),
	pBuffer_(0)
{
	DECLARE_QSTATUS();
	
	status = newQsObject(&pBuffer_);
	CHECK_QSTATUS_SET(pstatus);
}

qmpop3::UIDListContentHandler::~UIDListContentHandler()
{
	delete pBuffer_;
}

QSTATUS qmpop3::UIDListContentHandler::startElement(
	const WCHAR* pwszNamespaceURI, const WCHAR* pwszLocalName,
	const WCHAR* pwszQName, const Attributes& attributes)
{
	DECLARE_QSTATUS();
	
	if (wcscmp(pwszLocalName, L"uidl") == 0) {
		if (state_ != STATE_ROOT)
			return QSTATUS_FAIL;
		
		if (attributes.getLength() != 0)
			return QSTATUS_FAIL;
		
		state_ = STATE_UIDL;
	}
	else if (wcscmp(pwszLocalName, L"uid") == 0) {
		if (state_ != STATE_UIDL)
			return QSTATUS_FAIL;
		
		const WCHAR* pwszFlags = 0;
		const WCHAR* pwszDate = 0;
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrName, L"flags") == 0)
				pwszFlags = attributes.getValue(n);
			else if (wcscmp(pwszAttrName, L"date") == 0)
				pwszDate = attributes.getValue(n);
			else
				return QSTATUS_FAIL;
		}
		if (!pwszDate)
			return QSTATUS_FAIL;
		
		if (pwszFlags) {
			WCHAR* p = 0;
			nFlags_ = wcstol(pwszFlags, &p, 10);
			if (*p)
				return QSTATUS_FAIL;
		}
		
		int nYear = 0;
		int nMonth = 0;
		int nDay = 0;
		if (swscanf(pwszDate, L"%04d-%02d-%02d", &nYear, &nMonth, &nDay) != 3)
			return QSTATUS_FAIL;
		date_.nYear_ = nYear;
		date_.nMonth_ = nMonth;
		date_.nDay_ = nDay;
		
		state_ = STATE_UID;
	}
	else {
		return QSTATUS_FAIL;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmpop3::UIDListContentHandler::endElement(
	const WCHAR* pwszNamespaceURI, const WCHAR* pwszLocalName,
	const WCHAR* pwszQName)
{
	DECLARE_QSTATUS();
	
	if (wcscmp(pwszLocalName, L"uidl") == 0) {
		assert(state_ == STATE_UIDL);
		state_ = STATE_ROOT;
	}
	else if (wcscmp(pwszLocalName, L"uid") == 0) {
		assert(state_ == STATE_UID);
		
		std::auto_ptr<UID> pUID;
		status = newQsObject(pBuffer_->getCharArray(), nFlags_, date_, &pUID);
		CHECK_QSTATUS();
		pBuffer_->remove();
		status = pList_->add(pUID.get());
		CHECK_QSTATUS();
		pUID.release();
		
		state_ = STATE_UIDL;
	}
	else {
		return QSTATUS_FAIL;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qmpop3::UIDListContentHandler::characters(
	const WCHAR* pwsz, size_t nStart, size_t nLength)
{
	DECLARE_QSTATUS();
	
	if (state_ == STATE_UID) {
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
 * UIDListWriter
 *
 */

qmpop3::UIDListWriter::UIDListWriter(Writer* pWriter, QSTATUS* pstatus) :
	handler_(pWriter, pstatus)
{
}

qmpop3::UIDListWriter::~UIDListWriter()
{
}

QSTATUS qmpop3::UIDListWriter::write(const UIDList& l)
{
	DECLARE_QSTATUS();
	
	status = handler_.startDocument();
	CHECK_QSTATUS();
	
	status = handler_.startElement(0, 0, L"uidl", DefaultAttributes());
	CHECK_QSTATUS();
	
	for (size_t n = 0; n < l.getCount(); ++n) {
		UID* pUID = l.getUID(n);
		
		WCHAR wszFlags[32];
		swprintf(wszFlags, L"%d", pUID->getFlags());
		
		const UID::Date& date = pUID->getDate();
		WCHAR wszDate[32];
		swprintf(wszDate, L"%04d-%02d-%02d",
			date.nYear_, date.nMonth_, date.nDay_);
		
		class Attrs : public DefaultAttributes
		{
		public:
			Attrs(const WCHAR* pwszFlags, const WCHAR* pwszDate) :
				pwszFlags_(pwszFlags),
				pwszDate_(pwszDate)
			{
			}
			virtual ~Attrs()
			{
			}
		
		public:
			virtual int getLength() const
			{
				return 2;
			}
			virtual const WCHAR* getQName(int nIndex) const
			{
				assert(nIndex == 0 || nIndex == 1);
				return nIndex == 0 ? L"flags" : L"date";
			}
			virtual const WCHAR* getValue(int nIndex) const
			{
				assert(nIndex == 0 || nIndex == 1);
				return nIndex == 0 ? pwszFlags_ : pwszDate_;
			}
		
		private:
			const WCHAR* pwszFlags_;
			const WCHAR* pwszDate_;
		} attrs(wszFlags, wszDate);
		
		status = handler_.startElement(0, 0, L"uid", attrs);
		CHECK_QSTATUS();
		const WCHAR* pwszUID = pUID->getUID();
		status = handler_.characters(pwszUID, 0, wcslen(pwszUID));
		status = handler_.endElement(0, 0, L"uid");
		CHECK_QSTATUS();
	}
	
	status = handler_.endElement(0, 0, L"uidl");
	CHECK_QSTATUS();
	
	status = handler_.endDocument();
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}
