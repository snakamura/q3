/*
 * $Id: goround.cpp,v 1.2 2003/05/29 08:15:50 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmextensions.h>
#include <qmgoround.h>

#include <qsconv.h>
#include <qsnew.h>
#include <qsosutil.h>

#include <algorithm>

#include "goround.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * GoRoundImpl
 *
 */

struct qm::GoRoundImpl
{
	QSTATUS load();
	
	WSTRING wstrPath_;
	FILETIME ft_;
	GoRoundCourseList* pCourseList_;
};

QSTATUS qm::GoRoundImpl::load()
{
	DECLARE_QSTATUS();
	
	W2T(wstrPath_, ptszPath);
	AutoHandle hFile(::CreateFile(ptszPath, GENERIC_READ, 0, 0,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0));
	if (hFile.get()) {
		FILETIME ft;
		::GetFileTime(hFile.get(), 0, 0, &ft);
		hFile.close();
		
		if (::CompareFileTime(&ft, &ft_) != 0) {
			delete pCourseList_;
			pCourseList_ = 0;
			
			FileInputStream fileStream(wstrPath_, &status);
			CHECK_QSTATUS();
			BufferedInputStream stream(&fileStream, false, &status);
			CHECK_QSTATUS();
			
			status = newQsObject(&stream, &pCourseList_);
			CHECK_QSTATUS();
			
			ft_ = ft;
		}
	}
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * GoRound
 *
 */

qm::GoRound::GoRound(const WCHAR* pwszPath, QSTATUS* pstatus) :
	pImpl_(0)
{
	assert(pwszPath);
	assert(pstatus);
	
	DECLARE_QSTATUS();
	
	*pstatus = QSTATUS_SUCCESS;
	
	string_ptr<WSTRING> wstrPath(concat(pwszPath, L"\\", Extensions::GOROUND));
	if (!wstrPath.get()) {
		*pstatus = QSTATUS_OUTOFMEMORY;
		return;
	}
	
	status = newObject(&pImpl_);
	CHECK_QSTATUS_SET(pstatus);
	pImpl_->wstrPath_ = wstrPath.release();
	SYSTEMTIME st;
	::GetSystemTime(&st);
	::SystemTimeToFileTime(&st, &pImpl_->ft_);
	pImpl_->pCourseList_ = 0;
}

qm::GoRound::~GoRound()
{
	if (pImpl_) {
		freeWString(pImpl_->wstrPath_);
		delete pImpl_->pCourseList_;
		delete pImpl_;
	}
}

QSTATUS qm::GoRound::getCourseList(GoRoundCourseList** ppCourseList)
{
	assert(ppCourseList);
	
	DECLARE_QSTATUS();
	
	*ppCourseList = 0;
	
	status = pImpl_->load();
	CHECK_QSTATUS();
	*ppCourseList = pImpl_->pCourseList_;
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * GoRoundCourseList
 *
 */

qm::GoRoundCourseList::GoRoundCourseList(InputStream* pStream, QSTATUS* pstatus)
{
	assert(pStream);
	assert(pstatus);

	DECLARE_QSTATUS();
	
	*pstatus = QSTATUS_SUCCESS;

	status = load(pStream);
	CHECK_QSTATUS_SET(pstatus);
}

qm::GoRoundCourseList::~GoRoundCourseList()
{
	std::for_each(listCourse_.begin(), listCourse_.end(), deleter<GoRoundCourse>());
}

size_t qm::GoRoundCourseList::getCount() const
{
	return listCourse_.size();
}

GoRoundCourse* qm::GoRoundCourseList::getCourse(size_t nIndex) const
{
	assert(nIndex < getCount());
	return listCourse_[nIndex];
}

GoRoundCourse* qm::GoRoundCourseList::getCourse(const WCHAR* pwszCourse) const
{
	CourseList::const_iterator it = std::find_if(
		listCourse_.begin(), listCourse_.end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				string_equal<WCHAR>(),
				std::mem_fun(&GoRoundCourse::getName),
				std::identity<const WCHAR*>()),
			pwszCourse));
	return it != listCourse_.end() ? *it : 0;
}

QSTATUS qm::GoRoundCourseList::load(InputStream* pStream)
{
	DECLARE_QSTATUS();
	
	XMLReader reader(&status);
	CHECK_QSTATUS();
	GoRoundContentHandler handler(&listCourse_, &status);
	CHECK_QSTATUS();
	reader.setContentHandler(&handler);
	InputSource source(pStream, &status);
	CHECK_QSTATUS();
	status = reader.parse(&source);
	CHECK_QSTATUS();
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * GoRoundCourse
 *
 */

qm::GoRoundCourse::GoRoundCourse(const WCHAR* pwszName,
	unsigned int nFlags, QSTATUS* pstatus) :
	wstrName_(0),
	nFlags_(nFlags),
	pDialup_(0),
	type_(TYPE_SEQUENTIAL)
{
	assert(pwszName);
	assert(pstatus);
	
	*pstatus = QSTATUS_SUCCESS;
	
	wstrName_ = allocWString(pwszName);
	if (!wstrName_) {
		*pstatus = QSTATUS_OUTOFMEMORY;
		return;
	}
}

qm::GoRoundCourse::~GoRoundCourse()
{
	freeWString(wstrName_);
	delete pDialup_;
	std::for_each(listEntry_.begin(), listEntry_.end(),
		deleter<GoRoundEntry>());
}

const WCHAR* qm::GoRoundCourse::getName() const
{
	return wstrName_;
}

bool qm::GoRoundCourse::isFlag(Flag flag) const
{
	return (nFlags_ & flag) != 0;
}

GoRoundCourse::Type qm::GoRoundCourse::getType() const
{
	return type_;
}

const GoRoundDialup* qm::GoRoundCourse::getDialup() const
{
	return pDialup_;
}

const GoRoundCourse::EntryList& qm::GoRoundCourse::getEntries() const
{
	return listEntry_;
}

void qm::GoRoundCourse::setDialup(GoRoundDialup* pDialup)
{
	pDialup_ = pDialup;
}

void qm::GoRoundCourse::setType(Type type)
{
	type_ = type;
}

QSTATUS qm::GoRoundCourse::addEntry(GoRoundEntry* pEntry)
{
	return STLWrapper<EntryList>(listEntry_).push_back(pEntry);
}


/****************************************************************************
 *
 * GoRoundEntry
 *
 */

qm::GoRoundEntry::GoRoundEntry(const WCHAR* pwszAccount,
	const WCHAR* pwszSubAccount, const WCHAR* pwszFolder,
	unsigned int nFlags, const WCHAR* pwszFilterName, QSTATUS* pstatus) :
	wstrAccount_(0),
	wstrSubAccount_(0),
	pFolderName_(0),
	nFlags_(nFlags),
	wstrFilterName_(0)
{
	assert(pwszAccount);
	assert(pstatus);
	
	DECLARE_QSTATUS();
	
	*pstatus = QSTATUS_SUCCESS;
	
	wstrAccount_ = allocWString(pwszAccount);
	if (!wstrAccount_) {
		*pstatus = QSTATUS_OUTOFMEMORY;
		return;
	}
	if (pwszSubAccount) {
		wstrSubAccount_ = allocWString(pwszSubAccount);
		if (!wstrSubAccount_) {
			*pstatus = QSTATUS_OUTOFMEMORY;
			return;
		}
	}
	if (pwszFolder) {
		RegexCompiler compiler;
		status = compiler.compile(pwszFolder, &pFolderName_);
		CHECK_QSTATUS_SET(pstatus);
	}
	if (pwszFilterName) {
		wstrFilterName_ = allocWString(pwszFilterName);
		if (!wstrFilterName_) {
			*pstatus = QSTATUS_OUTOFMEMORY;
			return;
		}
	}
}

qm::GoRoundEntry::~GoRoundEntry()
{
	freeWString(wstrAccount_);
	freeWString(wstrSubAccount_);
	delete pFolderName_;
	freeWString(wstrFilterName_);
}

const WCHAR* qm::GoRoundEntry::getAccount() const
{
	return wstrAccount_;
}

const WCHAR* qm::GoRoundEntry::getSubAccount() const
{
	return wstrSubAccount_;
}

const RegexPattern* qm::GoRoundEntry::getFolderNamePattern() const
{
	return pFolderName_;
}

bool qm::GoRoundEntry::isFlag(Flag flag) const
{
	return (nFlags_ & flag) != 0;
}

const WCHAR* qm::GoRoundEntry::getFilterName() const
{
	return wstrFilterName_;
}


/****************************************************************************
 *
 * GoRoundDialup
 *
 */

qm::GoRoundDialup::GoRoundDialup(const WCHAR* pwszName, unsigned int nFlags,
	const WCHAR* pwszDialFrom, unsigned int nDisconnectWait, QSTATUS* pstatus) :
	wstrName_(0),
	nFlags_(nFlags),
	wstrDialFrom_(0),
	nDisconnectWait_(nDisconnectWait)
{
	assert(pstatus);
	
	*pstatus = QSTATUS_SUCCESS;
	
	if (pwszName) {
		wstrName_ = allocWString(pwszName);
		if (!wstrName_) {
			*pstatus = QSTATUS_OUTOFMEMORY;
			return;
		}
	}
	if (pwszDialFrom) {
		wstrDialFrom_ = allocWString(pwszDialFrom);
		if (!wstrDialFrom_) {
			*pstatus = QSTATUS_OUTOFMEMORY;
			return;
		}
	}
}

qm::GoRoundDialup::~GoRoundDialup()
{
	freeWString(wstrName_);
	freeWString(wstrDialFrom_);
}

const WCHAR* qm::GoRoundDialup::getName() const
{
	return wstrName_;
}

unsigned int qm::GoRoundDialup::getFlags() const
{
	return nFlags_;
}

const WCHAR* qm::GoRoundDialup::getDialFrom() const
{
	return wstrDialFrom_;
}

unsigned int qm::GoRoundDialup::getDisconnectWait() const
{
	return nDisconnectWait_;
}


/****************************************************************************
 *
 * GoRoundContentHandler
 *
 */

qm::GoRoundContentHandler::GoRoundContentHandler(
	CourseList* pListCourse, QSTATUS* pstatus) :
	DefaultHandler(pstatus),
	pListCourse_(pListCourse),
	state_(STATE_ROOT),
	pCurrentCourse_(0),
	pCurrentEntry_(0)
{
	assert(pstatus);
}

qm::GoRoundContentHandler::~GoRoundContentHandler()
{
}

QSTATUS qm::GoRoundContentHandler::startElement(
	const WCHAR* pwszNamespaceURI, const WCHAR* pwszLocalName,
	const WCHAR* pwszQName, const Attributes& attributes)
{
	DECLARE_QSTATUS();
	
	if (wcscmp(pwszLocalName, L"goround") == 0) {
		if (state_ != STATE_ROOT)
			return QSTATUS_FAIL;
		if (attributes.getLength() != 0)
			return QSTATUS_FAIL;
		state_ = STATE_GOROUND;
	}
	else if (wcscmp(pwszLocalName, L"course") == 0) {
		if (state_ != STATE_GOROUND)
			return QSTATUS_FAIL;
		
		const WCHAR* pwszName = 0;
		unsigned int nFlags = 0;
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrName, L"name") == 0) {
				pwszName = attributes.getValue(n);
			}
			else if (wcscmp(pwszAttrName, L"confirm") == 0) {
				if (wcscmp(attributes.getValue(n), L"true"))
					nFlags |= GoRoundCourse::FLAG_CONFIRM;
			}
			else {
				return QSTATUS_FAIL;
			}
		}
		if (!pwszName)
			return QSTATUS_FAIL;
		
		std::auto_ptr<GoRoundCourse> pCourse;
		status = newQsObject(pwszName, nFlags, &pCourse);
		CHECK_QSTATUS();
		
		STLWrapper<CourseList>(*pListCourse_).push_back(pCourse.get());
		CHECK_QSTATUS();
		pCurrentCourse_ = pCourse.release();
		
		state_ = STATE_COURSE;
	}
	else if (wcscmp(pwszLocalName, L"dialup") == 0) {
		if (state_ != STATE_COURSE)
			return QSTATUS_FAIL;
		
		assert(pCurrentCourse_);
		
		const WCHAR* pwszName = 0;
		unsigned int nFlags = 0;
		const WCHAR* pwszDialFrom = 0;
		unsigned int nDisconnectWait = 0;
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrName, L"name") == 0) {
				pwszName = attributes.getValue(n);
			}
			else if (wcscmp(pwszAttrName, L"dialFrom") == 0) {
				pwszDialFrom = attributes.getValue(n);
			}
			else if (wcscmp(pwszAttrName, L"showDialog") == 0) {
				if (wcscmp(attributes.getValue(n), L"true") == 0)
					nFlags |= GoRoundDialup::FLAG_SHOWDIALOG;
			}
			else if (wcscmp(pwszAttrName, L"disconnectWait") == 0) {
				WCHAR* pEnd = 0;
				nDisconnectWait = wcstol(attributes.getValue(n), &pEnd, 10);
				if (*pEnd)
					return QSTATUS_FAIL;
			}
			else if (wcscmp(pwszAttrName, L"wheneverNotConnected") == 0) {
				if (wcscmp(attributes.getValue(n), L"true") == 0)
					nFlags |= GoRoundDialup::FLAG_WHENEVERNOTCONNECTED;
			}
			else {
				return QSTATUS_FAIL;
			}
		}
		
		std::auto_ptr<GoRoundDialup> pDialup;
		status = newQsObject(pwszName, nFlags,
			pwszDialFrom, nDisconnectWait, &pDialup);
		CHECK_QSTATUS();
		
		pCurrentCourse_->setDialup(pDialup.release());
		
		state_ = STATE_DIALUP;
	}
	else if (wcscmp(pwszLocalName, L"sequential") == 0) {
		if (state_ != STATE_COURSE)
			return QSTATUS_FAIL;
		
		assert(pCurrentCourse_);
		
		pCurrentCourse_->setType(GoRoundCourse::TYPE_SEQUENTIAL);
		
		state_ = STATE_TYPE;
	}
	else if (wcscmp(pwszLocalName, L"parallel") == 0) {
		if (state_ != STATE_COURSE)
			return QSTATUS_FAIL;
		
		assert(pCurrentCourse_);
		
		pCurrentCourse_->setType(GoRoundCourse::TYPE_PARALLEL);
		
		state_ = STATE_TYPE;
	}
	else if (wcscmp(pwszLocalName, L"entry") == 0) {
		if (state_ != STATE_TYPE)
			return QSTATUS_FAIL;
		
		assert(pCurrentCourse_);
		
		const WCHAR* pwszAccount = 0;
		const WCHAR* pwszSubAccount = 0;
		const WCHAR* pwszFolder = 0;
		const WCHAR* pwszFilter = 0;
		unsigned int nFlags = 0;
		for (int n = 0; n < attributes.getLength(); ++n) {
			const WCHAR* pwszAttrName = attributes.getLocalName(n);
			if (wcscmp(pwszAttrName, L"account") == 0) {
				pwszAccount = attributes.getValue(n);
			}
			else if (wcscmp(pwszAttrName, L"subaccount") == 0) {
				pwszSubAccount = attributes.getValue(n);
			}
			else if (wcscmp(pwszAttrName, L"send") == 0) {
				if (wcscmp(attributes.getValue(n), L"true") == 0)
					nFlags |= GoRoundEntry::FLAG_SEND;
			}
			else if (wcscmp(pwszAttrName, L"receive") == 0) {
				if (wcscmp(attributes.getValue(n), L"true") == 0)
					nFlags |= GoRoundEntry::FLAG_RECEIVE;
			}
			else if (wcscmp(pwszAttrName, L"folder") == 0) {
				pwszFolder = attributes.getValue(n);
			}
			else if (wcscmp(pwszAttrName, L"selectFolder") == 0) {
				if (wcscmp(attributes.getValue(n), L"true") == 0)
					nFlags |= GoRoundEntry::FLAG_SELECTFOLDER;
			}
			else if (wcscmp(pwszAttrName, L"filter") == 0) {
				pwszFilter = attributes.getValue(n);
			}
			else {
				return QSTATUS_FAIL;
			}
		}
		if (!pwszAccount)
			return QSTATUS_FAIL;
		
		if (!(nFlags & GoRoundEntry::FLAG_SEND) &&
			!(nFlags & GoRoundEntry::FLAG_RECEIVE))
			nFlags |= GoRoundEntry::FLAG_SEND | GoRoundEntry::FLAG_RECEIVE;
		
		std::auto_ptr<GoRoundEntry> pEntry;
		status = newQsObject(pwszAccount, pwszSubAccount,
			pwszFolder, nFlags, pwszFilter, &pEntry);
		CHECK_QSTATUS();
		
		status = pCurrentCourse_->addEntry(pEntry.get());
		CHECK_QSTATUS();
		pCurrentEntry_ = pEntry.release();
		
		state_ = STATE_ENTRY;
	}
	else {
		return QSTATUS_FAIL;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::GoRoundContentHandler::endElement(
	const WCHAR* pwszNamespaceURI, const WCHAR* pwszLocalName,
	const WCHAR* pwszQName)
{
	DECLARE_QSTATUS();
	
	if (wcscmp(pwszLocalName, L"goround") == 0) {
		assert(state_ == STATE_GOROUND);
		state_ = STATE_ROOT;
	}
	else if (wcscmp(pwszLocalName, L"course") == 0) {
		assert(state_ == STATE_COURSE);
		assert(pCurrentCourse_);
		pCurrentCourse_ = 0;
		state_ = STATE_GOROUND;
	}
	else if (wcscmp(pwszLocalName, L"dialup") == 0) {
		assert(state_ == STATE_DIALUP);
		state_ = STATE_COURSE;
	}
	else if (wcscmp(pwszLocalName, L"sequential") == 0) {
		assert(state_ == STATE_TYPE);
		state_ = STATE_COURSE;
	}
	else if (wcscmp(pwszLocalName, L"parallel") == 0) {
		assert(state_ == STATE_TYPE);
		state_ = STATE_COURSE;
	}
	else if (wcscmp(pwszLocalName, L"entry") == 0) {
		assert(state_ == STATE_ENTRY);
		assert(pCurrentEntry_);
		pCurrentEntry_ = 0;
		state_ = STATE_TYPE;
	}
	else {
		assert(false);
		return QSTATUS_FAIL;
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::GoRoundContentHandler::characters(
	const WCHAR* pwsz, size_t nStart, size_t nLength)
{
	DECLARE_QSTATUS();
	
	const WCHAR* p = pwsz + nStart;
	for (size_t n = 0; n < nLength; ++n, ++p) {
		if (*p != L' ' && *p != L'\t' && *p != '\n')
			return QSTATUS_FAIL;
	}
	
	return QSTATUS_SUCCESS;
}
