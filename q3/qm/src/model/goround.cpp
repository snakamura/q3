/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmapplication.h>
#include <qmfilenames.h>
#include <qmgoround.h>

#include <qsconv.h>
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
	bool load();
	
	FILETIME ft_;
	std::auto_ptr<GoRoundCourseList> pCourseList_;
};

bool qm::GoRoundImpl::load()
{
	wstring_ptr wstrPath(Application::getApplication().getProfilePath(FileNames::GOROUND_XML));
	
	W2T(wstrPath.get(), ptszPath);
	AutoHandle hFile(::CreateFile(ptszPath, GENERIC_READ, 0, 0,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0));
	if (hFile.get()) {
		FILETIME ft;
		::GetFileTime(hFile.get(), 0, 0, &ft);
		hFile.close();
		
		if (::CompareFileTime(&ft, &ft_) != 0) {
			FileInputStream fileStream(wstrPath.get());
			if (!fileStream)
				return false;
			BufferedInputStream stream(&fileStream, false);
			
			pCourseList_.reset(new GoRoundCourseList());
			if (!pCourseList_->load(&stream))
				return false;
			
			ft_ = ft;
		}
	}
	
	return true;
}


/****************************************************************************
 *
 * GoRound
 *
 */

qm::GoRound::GoRound() :
	pImpl_(0)
{
	pImpl_ = new GoRoundImpl();
	
	SYSTEMTIME st;
	::GetSystemTime(&st);
	::SystemTimeToFileTime(&st, &pImpl_->ft_);
	pImpl_->pCourseList_.reset(new GoRoundCourseList());
}

qm::GoRound::~GoRound()
{
	if (pImpl_)
		delete pImpl_;
}

const GoRoundCourseList* qm::GoRound::getCourseList()
{
	pImpl_->load();
	return pImpl_->pCourseList_.get();
}


/****************************************************************************
 *
 * GoRoundCourseList
 *
 */

qm::GoRoundCourseList::GoRoundCourseList()
{
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

bool qm::GoRoundCourseList::load(InputStream* pStream)
{
	XMLReader reader;
	GoRoundContentHandler handler(&listCourse_);
	reader.setContentHandler(&handler);
	InputSource source(pStream);
	return reader.parse(&source);
}


/****************************************************************************
 *
 * GoRoundCourse
 *
 */

qm::GoRoundCourse::GoRoundCourse(const WCHAR* pwszName,
								 unsigned int nFlags) :
	nFlags_(nFlags),
	type_(TYPE_SEQUENTIAL)
{
	assert(pwszName);
	
	wstrName_ = allocWString(pwszName);
}

qm::GoRoundCourse::~GoRoundCourse()
{
	std::for_each(listEntry_.begin(), listEntry_.end(), deleter<GoRoundEntry>());
}

const WCHAR* qm::GoRoundCourse::getName() const
{
	return wstrName_.get();
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
	return pDialup_.get();
}

const GoRoundCourse::EntryList& qm::GoRoundCourse::getEntries() const
{
	return listEntry_;
}

void qm::GoRoundCourse::setDialup(std::auto_ptr<GoRoundDialup> pDialup)
{
	pDialup_ = pDialup;
}

void qm::GoRoundCourse::setType(Type type)
{
	type_ = type;
}

void qm::GoRoundCourse::addEntry(std::auto_ptr<GoRoundEntry> pEntry)
{
	listEntry_.push_back(pEntry.get());
	pEntry.release();
}


/****************************************************************************
 *
 * GoRoundEntry
 *
 */

qm::GoRoundEntry::GoRoundEntry(const WCHAR* pwszAccount,
							  const WCHAR* pwszSubAccount,
							  std::auto_ptr<RegexPattern> pFolderName,
							  unsigned int nFlags,
							  const WCHAR* pwszFilterName,
							  ConnectReceiveBeforeSend crbs) :
	pFolderName_(pFolderName),
	nFlags_(nFlags),
	crbs_(crbs)
{
	assert(pwszAccount);
	
	wstrAccount_ = allocWString(pwszAccount);
	if (pwszSubAccount)
		wstrSubAccount_ = allocWString(pwszSubAccount);
	if (pwszFilterName)
		wstrFilterName_ = allocWString(pwszFilterName);
}

qm::GoRoundEntry::~GoRoundEntry()
{
}

const WCHAR* qm::GoRoundEntry::getAccount() const
{
	return wstrAccount_.get();
}

const WCHAR* qm::GoRoundEntry::getSubAccount() const
{
	return wstrSubAccount_.get();
}

const RegexPattern* qm::GoRoundEntry::getFolderNamePattern() const
{
	return pFolderName_.get();
}

bool qm::GoRoundEntry::isFlag(Flag flag) const
{
	return (nFlags_ & flag) != 0;
}

const WCHAR* qm::GoRoundEntry::getFilterName() const
{
	return wstrFilterName_.get();
}

GoRoundEntry::ConnectReceiveBeforeSend qm::GoRoundEntry::getConnectReceiveBeforeSend() const
{
	return crbs_;
}


/****************************************************************************
 *
 * GoRoundDialup
 *
 */

qm::GoRoundDialup::GoRoundDialup(const WCHAR* pwszName,
								 unsigned int nFlags,
								 const WCHAR* pwszDialFrom,
								 unsigned int nDisconnectWait) :
	nFlags_(nFlags),
	nDisconnectWait_(nDisconnectWait)
{
	if (pwszName)
		wstrName_ = allocWString(pwszName);
	if (pwszDialFrom)
		wstrDialFrom_ = allocWString(pwszDialFrom);
}

qm::GoRoundDialup::~GoRoundDialup()
{
}

const WCHAR* qm::GoRoundDialup::getName() const
{
	return wstrName_.get();
}

unsigned int qm::GoRoundDialup::getFlags() const
{
	return nFlags_;
}

const WCHAR* qm::GoRoundDialup::getDialFrom() const
{
	return wstrDialFrom_.get();
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

qm::GoRoundContentHandler::GoRoundContentHandler(CourseList* pListCourse) :
	pListCourse_(pListCourse),
	state_(STATE_ROOT),
	pCurrentCourse_(0),
	pCurrentEntry_(0)
{
}

qm::GoRoundContentHandler::~GoRoundContentHandler()
{
}

bool qm::GoRoundContentHandler::startElement(const WCHAR* pwszNamespaceURI,
											 const WCHAR* pwszLocalName,
											 const WCHAR* pwszQName,
											 const Attributes& attributes)
{
	if (wcscmp(pwszLocalName, L"goround") == 0) {
		if (state_ != STATE_ROOT)
			return false;
		if (attributes.getLength() != 0)
			return false;
		state_ = STATE_GOROUND;
	}
	else if (wcscmp(pwszLocalName, L"course") == 0) {
		if (state_ != STATE_GOROUND)
			return false;
		
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
				return false;
			}
		}
		if (!pwszName)
			return false;
		
		std::auto_ptr<GoRoundCourse> pCourse(new GoRoundCourse(pwszName, nFlags));
		
		pListCourse_->push_back(pCourse.get());
		pCurrentCourse_ = pCourse.release();
		
		state_ = STATE_COURSE;
	}
	else if (wcscmp(pwszLocalName, L"dialup") == 0) {
		if (state_ != STATE_COURSE)
			return false;
		
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
					return false;
			}
			else if (wcscmp(pwszAttrName, L"wheneverNotConnected") == 0) {
				if (wcscmp(attributes.getValue(n), L"true") == 0)
					nFlags |= GoRoundDialup::FLAG_WHENEVERNOTCONNECTED;
			}
			else {
				return false;
			}
		}
		
		std::auto_ptr<GoRoundDialup> pDialup(new GoRoundDialup(
			pwszName, nFlags, pwszDialFrom, nDisconnectWait));
		pCurrentCourse_->setDialup(pDialup);
		
		state_ = STATE_DIALUP;
	}
	else if (wcscmp(pwszLocalName, L"sequential") == 0) {
		if (state_ != STATE_COURSE)
			return false;
		
		assert(pCurrentCourse_);
		pCurrentCourse_->setType(GoRoundCourse::TYPE_SEQUENTIAL);
		
		state_ = STATE_TYPE;
	}
	else if (wcscmp(pwszLocalName, L"parallel") == 0) {
		if (state_ != STATE_COURSE)
			return false;
		
		assert(pCurrentCourse_);
		pCurrentCourse_->setType(GoRoundCourse::TYPE_PARALLEL);
		
		state_ = STATE_TYPE;
	}
	else if (wcscmp(pwszLocalName, L"entry") == 0) {
		if (state_ != STATE_TYPE)
			return false;
		
		assert(pCurrentCourse_);
		
		const WCHAR* pwszAccount = 0;
		const WCHAR* pwszSubAccount = 0;
		const WCHAR* pwszFolder = 0;
		const WCHAR* pwszFilter = 0;
		unsigned int nFlags = 0;
		GoRoundEntry::ConnectReceiveBeforeSend crbs = GoRoundEntry::CRBS_NONE;
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
			else if (wcscmp(pwszAttrName, L"connectReceiveBeforeSend") == 0) {
				crbs = wcscmp(attributes.getValue(n), L"true") == 0 ?
					GoRoundEntry::CRBS_TRUE : GoRoundEntry::CRBS_FALSE;
			}
			else {
				return false;
			}
		}
		if (!pwszAccount)
			return false;
		
		if (!(nFlags & GoRoundEntry::FLAG_SEND) &&
			!(nFlags & GoRoundEntry::FLAG_RECEIVE))
			nFlags |= GoRoundEntry::FLAG_SEND | GoRoundEntry::FLAG_RECEIVE;
		
		std::auto_ptr<RegexPattern> pFolderName;
		if (pwszFolder) {
			RegexCompiler compiler;
			pFolderName = compiler.compile(pwszFolder);
			if (!pFolderName.get())
				return false;
		}
		
		std::auto_ptr<GoRoundEntry> pEntry(new GoRoundEntry(pwszAccount,
			pwszSubAccount, pFolderName, nFlags, pwszFilter, crbs));
		
		pCurrentEntry_ = pEntry.get();
		pCurrentCourse_->addEntry(pEntry);
		
		state_ = STATE_ENTRY;
	}
	else {
		return false;
	}
	
	return true;
}

bool qm::GoRoundContentHandler::endElement(const WCHAR* pwszNamespaceURI,
										   const WCHAR* pwszLocalName,
										   const WCHAR* pwszQName)
{
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
		return false;
	}
	
	return true;
}

bool qm::GoRoundContentHandler::characters(const WCHAR* pwsz,
										   size_t nStart,
										   size_t nLength)
{
	const WCHAR* p = pwsz + nStart;
	for (size_t n = 0; n < nLength; ++n, ++p) {
		if (*p != L' ' && *p != L'\t' && *p != '\n')
			return false;
	}
	
	return true;
}
