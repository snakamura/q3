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
#include <qsfile.h>
#include <qsosutil.h>

#include <algorithm>

#include "confighelper.h"
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
	GoRoundImpl();
	
	bool load();
	
	GoRound* pThis_;
	GoRound::CourseList listCourse_;
	ConfigHelper<GoRound, GoRoundContentHandler, GoRoundWriter> helper_;
};

qm::GoRoundImpl::GoRoundImpl() :
	helper_(Application::getApplication().getProfilePath(FileNames::GOROUND_XML).get())
{
}

bool qm::GoRoundImpl::load()
{
	GoRoundContentHandler handler(&listCourse_);
	return helper_.load(pThis_, &handler);
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
	pImpl_->pThis_ = this;
}

qm::GoRound::~GoRound()
{
	clear();
	delete pImpl_;
}

const GoRound::CourseList& qm::GoRound::getCourses() const
{
	return getCourses(true);
}

const GoRound::CourseList& qm::GoRound::getCourses(bool bReload) const
{
	if (bReload)
		pImpl_->load();
	return pImpl_->listCourse_;
}

void qm::GoRound::setCourses(CourseList& listCourse)
{
	clear();
	pImpl_->listCourse_.swap(listCourse);
}

GoRoundCourse* qm::GoRound::getCourse(const WCHAR* pwszCourse) const
{
	pImpl_->load();
	
	CourseList::const_iterator it = std::find_if(
		pImpl_->listCourse_.begin(), pImpl_->listCourse_.end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				string_equal<WCHAR>(),
				std::mem_fun(&GoRoundCourse::getName),
				std::identity<const WCHAR*>()),
			pwszCourse));
	return it != pImpl_->listCourse_.end() ? *it : 0;
}

bool qm::GoRound::save() const
{
	return pImpl_->helper_.save(this);
}

void qm::GoRound::clear()
{
	std::for_each(pImpl_->listCourse_.begin(),
		pImpl_->listCourse_.end(), deleter<GoRoundCourse>());
	pImpl_->listCourse_.clear();
}


/****************************************************************************
 *
 * GoRoundCourse
 *
 */

qm::GoRoundCourse::GoRoundCourse() :
	nFlags_(0),
	type_(TYPE_SEQUENTIAL)
{
	wstrName_ = allocWString(L"");
}

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

qm::GoRoundCourse::GoRoundCourse(const GoRoundCourse& course) :
	nFlags_(course.nFlags_),
	type_(course.type_)
{
	wstrName_ = allocWString(course.wstrName_.get());
	
	if (course.pDialup_.get())
		pDialup_.reset(new GoRoundDialup(*course.pDialup_.get()));
	
	listEntry_.reserve(course.listEntry_.size());
	for (EntryList::const_iterator it = course.listEntry_.begin(); it != course.listEntry_.end(); ++it)
		listEntry_.push_back(new GoRoundEntry(**it));
}

GoRoundCourse& qm::GoRoundCourse::operator=(const GoRoundCourse& course)
{
	if (&course != this) {
		wstrName_ = allocWString(course.wstrName_.get());
		
		nFlags_ = course.nFlags_;
		
		if (course.pDialup_.get())
			pDialup_.reset(new GoRoundDialup(*course.pDialup_.get()));
		
		type_ = course.type_;
		
		listEntry_.reserve(course.listEntry_.size());
		for (EntryList::const_iterator it = course.listEntry_.begin(); it != course.listEntry_.end(); ++it)
			listEntry_.push_back(new GoRoundEntry(**it));
		
	}
	return *this;
}

const WCHAR* qm::GoRoundCourse::getName() const
{
	return wstrName_.get();
}

void qm::GoRoundCourse::setName(const WCHAR* pwszName)
{
	assert(pwszName);
	wstrName_ = allocWString(pwszName);
}

bool qm::GoRoundCourse::isFlag(Flag flag) const
{
	return (nFlags_ & flag) != 0;
}

void qm::GoRoundCourse::setFlags(unsigned int nFlags)
{
	nFlags_ = nFlags;
}

GoRoundCourse::Type qm::GoRoundCourse::getType() const
{
	return type_;
}

void qm::GoRoundCourse::setType(Type type)
{
	type_ = type;
}

GoRoundDialup* qm::GoRoundCourse::getDialup() const
{
	return pDialup_.get();
}

void qm::GoRoundCourse::setDialup(std::auto_ptr<GoRoundDialup> pDialup)
{
	pDialup_ = pDialup;
}

const GoRoundCourse::EntryList& qm::GoRoundCourse::getEntries() const
{
	return listEntry_;
}

void qm::GoRoundCourse::setEntries(EntryList& listEntry)
{
	std::for_each(listEntry_.begin(), listEntry_.end(), deleter<GoRoundEntry>());
	listEntry_.clear();
	listEntry_.swap(listEntry);
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

qm::GoRoundEntry::GoRoundEntry() :
	nFlags_(FLAG_SEND | FLAG_RECEIVE),
	crbs_(CRBS_NONE)
{
	wstrAccount_ = allocWString(L"");
}

qm::GoRoundEntry::GoRoundEntry(const WCHAR* pwszAccount,
							   const WCHAR* pwszSubAccount,
							   const WCHAR* pwszFolder,
							   std::auto_ptr<RegexPattern> pFolder,
							   unsigned int nFlags,
							   const WCHAR* pwszFilter,
							   ConnectReceiveBeforeSend crbs) :
	pFolder_(pFolder),
	nFlags_(nFlags),
	crbs_(crbs)
{
	assert(pwszAccount);
	
	wstrAccount_ = allocWString(pwszAccount);
	if (pwszSubAccount)
		wstrSubAccount_ = allocWString(pwszSubAccount);
	if (pwszFolder)
		wstrFolder_ = allocWString(pwszFolder);
	if (pwszFilter)
		wstrFilter_ = allocWString(pwszFilter);
}

qm::GoRoundEntry::GoRoundEntry(const GoRoundEntry& entry) :
	nFlags_(entry.nFlags_),
	crbs_(entry.crbs_)
{
	wstrAccount_ = allocWString(entry.wstrAccount_.get());
	if (entry.wstrSubAccount_.get())
		wstrSubAccount_ = allocWString(entry.wstrSubAccount_.get());
	if (entry.wstrFolder_.get()) {
		wstrFolder_ = allocWString(entry.wstrFolder_.get());
		pFolder_ = RegexCompiler().compile(wstrFolder_.get());
		assert(pFolder_.get());
	}
	if (entry.wstrFilter_.get())
		wstrFilter_ = allocWString(entry.wstrFilter_.get());
}

qm::GoRoundEntry::~GoRoundEntry()
{
}

GoRoundEntry& qm::GoRoundEntry::operator=(const GoRoundEntry& entry)
{
	if (&entry != this) {
		wstrAccount_ = allocWString(entry.wstrAccount_.get());
		if (entry.wstrSubAccount_.get())
			wstrSubAccount_ = allocWString(entry.wstrSubAccount_.get());
		if (entry.wstrFolder_.get()) {
			wstrFolder_ = allocWString(entry.wstrFolder_.get());
			pFolder_ = RegexCompiler().compile(wstrFolder_.get());
			assert(pFolder_.get());
		}
		nFlags_ = entry.nFlags_;
		if (entry.wstrFilter_.get())
			wstrFilter_ = allocWString(entry.wstrFilter_.get());
		crbs_ = entry.crbs_;
	}
	return *this;
}

const WCHAR* qm::GoRoundEntry::getAccount() const
{
	return wstrAccount_.get();
}

void qm::GoRoundEntry::setAccount(const WCHAR* pwszAccount)
{
	assert(pwszAccount);
	wstrAccount_ = allocWString(pwszAccount);
}

const WCHAR* qm::GoRoundEntry::getSubAccount() const
{
	return wstrSubAccount_.get();
}

void qm::GoRoundEntry::setSubAccount(const WCHAR* pwszSubAccount)
{
	if (pwszSubAccount)
		wstrSubAccount_ = allocWString(pwszSubAccount);
	else
		wstrSubAccount_.reset(0);
}

const WCHAR* qm::GoRoundEntry::getFolder() const
{
	return wstrFolder_.get();
}

const RegexPattern* qm::GoRoundEntry::getFolderPattern() const
{
	return pFolder_.get();
}

void qm::GoRoundEntry::setFolder(const WCHAR* pwszFolder,
								 std::auto_ptr<RegexPattern> pFolder)
{
	assert((pwszFolder && pFolder.get()) || (!pwszFolder && !pFolder.get()));
	if (pwszFolder)
		wstrFolder_ = allocWString(pwszFolder);
	else
		wstrFolder_.reset(0);
	pFolder_ = pFolder;
}

bool qm::GoRoundEntry::isFlag(Flag flag) const
{
	return (nFlags_ & flag) != 0;
}

void qm::GoRoundEntry::setFlags(unsigned int nFlags)
{
	nFlags_ = nFlags;
}

const WCHAR* qm::GoRoundEntry::getFilter() const
{
	return wstrFilter_.get();
}

void qm::GoRoundEntry::setFilter(const WCHAR* pwszFilter)
{
	if (pwszFilter)
		wstrFilter_ = allocWString(pwszFilter);
	else
		wstrFilter_.reset(0);
}

GoRoundEntry::ConnectReceiveBeforeSend qm::GoRoundEntry::getConnectReceiveBeforeSend() const
{
	return crbs_;
}

void qm::GoRoundEntry::setConnectReceiveBeforeSend(ConnectReceiveBeforeSend crbs)
{
	crbs_ = crbs;
}


/****************************************************************************
 *
 * GoRoundDialup
 *
 */

qm::GoRoundDialup::GoRoundDialup() :
	nFlags_(0),
	nDisconnectWait_(0)
{
}

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

qm::GoRoundDialup::GoRoundDialup(const GoRoundDialup& dialup) :
	nFlags_(dialup.nFlags_),
	nDisconnectWait_(dialup.nDisconnectWait_)
{
	if (dialup.wstrName_.get())
		wstrName_ = allocWString(dialup.wstrName_.get());
	
	if (dialup.wstrDialFrom_.get())
		wstrDialFrom_ = allocWString(dialup.wstrDialFrom_.get());
}

qm::GoRoundDialup::~GoRoundDialup()
{
}

GoRoundDialup& qm::GoRoundDialup::operator=(const GoRoundDialup& dialup)
{
	if (&dialup != this) {
		if (dialup.wstrName_.get())
			wstrName_ = allocWString(dialup.wstrName_.get());
		
		nFlags_ = dialup.nFlags_;
		
		if (dialup.wstrDialFrom_.get())
			wstrDialFrom_ = allocWString(dialup.wstrDialFrom_.get());
		
		nDisconnectWait_ = dialup.nDisconnectWait_;
	}
	return *this;
}

const WCHAR* qm::GoRoundDialup::getName() const
{
	return wstrName_.get();
}

void qm::GoRoundDialup::setName(const WCHAR* pwszName)
{
	if (pwszName)
		wstrName_ = allocWString(pwszName);
	else
		wstrName_.reset(0);
}

unsigned int qm::GoRoundDialup::getFlags() const
{
	return nFlags_;
}

bool qm::GoRoundDialup::isFlag(Flag flag) const
{
	return (nFlags_ & flag) != 0;
}

void qm::GoRoundDialup::setFlags(unsigned int nFlags)
{
	nFlags_ = nFlags;
}

const WCHAR* qm::GoRoundDialup::getDialFrom() const
{
	return wstrDialFrom_.get();
}

void qm::GoRoundDialup::setDialFrom(const WCHAR* pwszDialFrom)
{
	if (pwszDialFrom)
		wstrDialFrom_ = allocWString(pwszDialFrom);
	else
		wstrDialFrom_.reset(0);
}

unsigned int qm::GoRoundDialup::getDisconnectWait() const
{
	return nDisconnectWait_;
}

void qm::GoRoundDialup::setDisconnectWait(unsigned int nDisconnectWait)
{
	nDisconnectWait_ = nDisconnectWait;
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
		
		std::auto_ptr<RegexPattern> pFolder;
		if (pwszFolder) {
			pFolder = RegexCompiler().compile(pwszFolder);
			if (!pFolder.get())
				return false;
		}
		
		std::auto_ptr<GoRoundEntry> pEntry(new GoRoundEntry(pwszAccount,
			pwszSubAccount, pwszFolder, pFolder, nFlags, pwszFilter, crbs));
		
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


/****************************************************************************
 *
 * GoRoundWriter
 *
 */

qm::GoRoundWriter::GoRoundWriter(Writer* pWriter) :
	handler_(pWriter)
{
}

qm::GoRoundWriter::~GoRoundWriter()
{
}

bool qm::GoRoundWriter::write(const GoRound* pGoRound)
{
	if (!handler_.startDocument())
		return false;
	if (!handler_.startElement(0, 0, L"goround", DefaultAttributes()))
		return false;
	
	const GoRound::CourseList& listCourse = pGoRound->getCourses(false);
	for (GoRound::CourseList::const_iterator it = listCourse.begin(); it != listCourse.end(); ++it) {
		const GoRoundCourse* pCourse = *it;
		if (!write(pCourse))
			return false;
	}
	
	if (!handler_.endElement(0, 0, L"goround"))
		return false;
	if (!handler_.endDocument())
		return false;
	
	return true;
}

bool qm::GoRoundWriter::write(const GoRoundCourse* pCourse)
{
	bool bConfirm = pCourse->isFlag(GoRoundCourse::FLAG_CONFIRM);
	const SimpleAttributes::Item items[] = {
		{ L"name",		pCourse->getName()							},
		{ L"confirm",	bConfirm ? L"true" : L"false",	!bConfirm	}
	};
	SimpleAttributes attrs(items, countof(items));
	if (!handler_.startElement(0, 0, L"course", attrs))
		return false;
	
	const GoRoundDialup* pDialup = pCourse->getDialup();
	if (pDialup) {
		if (!write(pDialup))
			return false;
	}
	
	const WCHAR* pwszType = pCourse->getType() == GoRoundCourse::TYPE_SEQUENTIAL ?
		L"sequential" : L"parallel";
	if (!handler_.startElement(0, 0, pwszType, DefaultAttributes()))
		return false;
	
	const GoRoundCourse::EntryList& listEntry = pCourse->getEntries();
	for (GoRoundCourse::EntryList::const_iterator it = listEntry.begin(); it != listEntry.end(); ++it) {
		const GoRoundEntry* pEntry = *it;
		if (!write(pEntry))
			return false;
	}
	
	if (!handler_.endElement(0, 0, pwszType))
		return false;
	
	if (!handler_.endElement(0, 0, L"course"))
		return false;
	
	return true;
}

bool qm::GoRoundWriter::write(const GoRoundDialup* pDialup)
{
	bool bShowDialog = pDialup->isFlag(GoRoundDialup::FLAG_SHOWDIALOG);
	bool bWheneverNotConnected = pDialup->isFlag(GoRoundDialup::FLAG_WHENEVERNOTCONNECTED);
	WCHAR wszDisconnectWait[32];
	swprintf(wszDisconnectWait, L"%u", pDialup->getDisconnectWait());
	const SimpleAttributes::Item items[] = {
		{ L"name",					pDialup->getName()																},
		{ L"dialFrom",				pDialup->getDialFrom(),						!pDialup->getDialFrom()				},
		{ L"showDialog",			bShowDialog ? L"true" : L"false",			!bShowDialog						},
		{ L"disconnectWait",		wszDisconnectWait,							pDialup->getDisconnectWait() == 0	},
		{ L"wheneverNotConnected",	bWheneverNotConnected ? L"true" : L"false",	!bWheneverNotConnected				}
	};
	SimpleAttributes attrs(items, countof(items));
	return handler_.startElement(0, 0, L"dialup", attrs) &&
		handler_.endElement(0, 0, L"dialup");
}

bool qm::GoRoundWriter::write(const GoRoundEntry* pEntry)
{
	bool bSend = pEntry->isFlag(GoRoundEntry::FLAG_SEND);
	bool bReceive = pEntry->isFlag(GoRoundEntry::FLAG_RECEIVE);
	bool bSelectFolder = pEntry->isFlag(GoRoundEntry::FLAG_SELECTFOLDER);
	GoRoundEntry::ConnectReceiveBeforeSend crbs = pEntry->getConnectReceiveBeforeSend();
	const SimpleAttributes::Item items[] = {
		{ L"account",					pEntry->getAccount()																			},
		{ L"subaccount",				pEntry->getSubAccount(),								!pEntry->getSubAccount()				},
		{ L"folder",					pEntry->getFolder(),									!pEntry->getFolder() || bSelectFolder	},
		{ L"filter",					pEntry->getFilter(),									!pEntry->getFilter()					},
		{ L"send",						bSend ? L"true" : L"false",								!bSend || (bSend && bReceive)			},
		{ L"receive",					bReceive ? L"true" : L"false",							!bReceive || (bSend && bReceive)		},
		{ L"selectFolder",				bSelectFolder ? L"true" : L"false",						!bSelectFolder							},
		{ L"connectReceiveBeforeSend",	crbs == GoRoundEntry::CRBS_TRUE ? L"true" : L"false",	crbs == GoRoundEntry::CRBS_NONE			}
	};
	SimpleAttributes attrs(items, countof(items));
	return handler_.startElement(0, 0, L"entry", attrs) &&
		handler_.endElement(0, 0, L"entry");
}
