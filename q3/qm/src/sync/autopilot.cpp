/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmdocument.h>
#include <qmgoround.h>
#include <qmrecents.h>

#include <qsras.h>

#include <tchar.h>

#include "autopilot.h"
#include "syncmanager.h"
#include "../model/goround.h"
#include "../ui/syncdialog.h"
#include "../ui/syncutil.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * AutoPilot
 *
 */

qm::AutoPilot::AutoPilot(AutoPilotManager* pAutoPilotManager,
						 Profile* pProfile,
						 Document* pDocument,
						 GoRound* pGoRound,
						 SyncManager* pSyncManager,
						 SyncDialogManager* pSyncDialogManager,
						 AutoPilotCallback* pCallback) :
	pAutoPilotManager_(pAutoPilotManager),
	pProfile_(pProfile),
	pDocument_(pDocument),
	pGoRound_(pGoRound),
	pSyncManager_(pSyncManager),
	pSyncDialogManager_(pSyncDialogManager),
	hwnd_(0),
	pCallback_(pCallback),
	bOnlyWhenConnected_(false),
	nId_(0),
	bEnabled_(false),
	nCount_(0)
#ifndef _WIN32_WCE
	,
	unseenCountUpdater_(pDocument, pProfile)
#endif
{
	bEnabled_ = pProfile->getInt(L"AutoPilot", L"Enabled", 0) != 0;
	bOnlyWhenConnected_ = pProfile->getInt(L"AutoPilot", L"OnlyWhenConnected", 0) != 0;
}

qm::AutoPilot::~AutoPilot()
{
	if (pTimer_.get()) {
		if (nId_ != 0)
			pTimer_->killTimer(nId_);
	}
}

AutoPilotManager* qm::AutoPilot::getAutoPilotManager() const
{
	return pAutoPilotManager_;
}

void qm::AutoPilot::start(HWND hwnd)
{
	hwnd_ = hwnd;
	pTimer_.reset(new Timer());
	nId_ = pTimer_->setTimer(TIMER_CHECK, 60*1000, this);
}

bool qm::AutoPilot::isEnabled() const
{
	return bEnabled_;
}

void qm::AutoPilot::setEnabled(bool bEnabled)
{
	bEnabled_ = bEnabled;
}

bool qm::AutoPilot::save() const
{
	pProfile_->setInt(L"AutoPilot", L"Enabled", bEnabled_);
	return true;
}

void qm::AutoPilot::timerTimeout(unsigned int nId)
{
	if (nId == nId_) {
		bool bPilot = bEnabled_ &&
			(!bOnlyWhenConnected_ || RasConnection::isNetworkConnected()) &&
			pCallback_->canAutoPilot();
		if (bPilot) {
			const AutoPilotManager::EntryList& l = pAutoPilotManager_->getEntries();
			for (AutoPilotManager::EntryList::const_iterator it = l.begin(); it != l.end(); ++it) {
				const AutoPilotEntry* pEntry = *it;
				if (nCount_ % pEntry->getInterval() == 0) {
					const GoRoundCourse* pCourse = pGoRound_->getCourse(pEntry->getCourse());
					if (pCourse)
						SyncUtil::goRound(pSyncManager_, pDocument_, pSyncDialogManager_,
							hwnd_, SyncDialog::FLAG_NOTIFYNEWMESSAGE, pCourse);
				}
			}
		}
		
		Recents* pRecents = pDocument_->getRecents();
		pRecents->removeSeens();
		
#ifndef _WIN32_WCE
		unseenCountUpdater_.update();
#endif
		
		++nCount_;
		if (nCount_ == 100000)
			nCount_ = 0;
	}
}


#ifndef _WIN32_WCE
/****************************************************************************
 *
 * AutoPilot::UnseenCountUpdater
 *
 */

qm::AutoPilot::UnseenCountUpdater::UnseenCountUpdater(Document* pDocument,
													  Profile* pProfile) :
	pDocument_(pDocument),
	pfnSHSetUnreadMailCount_(0)
{
	if (pProfile->getInt(L"Global", L"ShowUnseenCountOnWelcome", 0)) {
		HINSTANCE hInst = ::LoadLibrary(_T("shell32.dll"));
		pfnSHSetUnreadMailCount_ = reinterpret_cast<PFN_SHSETUNREADMAILCOUNT>(
			::GetProcAddress(hInst, "SHSetUnreadMailCountW"));
		
		TCHAR tszPath[MAX_PATH];
		::GetModuleFileName(0, tszPath, countof(tszPath));
		wstrPath_ = tcs2wcs(tszPath);
	}
}

qm::AutoPilot::UnseenCountUpdater::~UnseenCountUpdater()
{
}

void qm::AutoPilot::UnseenCountUpdater::update()
{
	if (!pfnSHSetUnreadMailCount_)
		return;
	
	const Document::AccountList& l = pDocument_->getAccounts();
	std::for_each(l.begin(), l.end(),
		std::bind1st(
			std::mem_fun(&UnseenCountUpdater::updateAccount),
			this));
}

bool qm::AutoPilot::UnseenCountUpdater::updateAccount(Account* pAccount)
{
	if (wcscmp(pAccount->getClass(), L"mail") != 0)
		return false;
	
	SubAccount* pSubAccount = pAccount->getCurrentSubAccount();
	const WCHAR* pwszAddress = pSubAccount->getSenderAddress();
	if (!pwszAddress || !*pwszAddress)
		return false;
	
	unsigned int nCount = 0;
	if (pSubAccount->getProperty(L"Global", L"ShowUnseenCountOnWelcome", 1)) {
		Folder* pFolder = pAccount->getFolderByBoxFlag(Folder::FLAG_INBOX);
		if (pFolder)
			nCount = pFolder->getUnseenCount();
	}
	(*pfnSHSetUnreadMailCount_)(pwszAddress, nCount, wstrPath_.get());
	
	return true;
}
#endif

/****************************************************************************
 *
 * AutoPilotCallback
 *
 */

qm::AutoPilotCallback::~AutoPilotCallback()
{
}


/****************************************************************************
 *
 * AutoPilotManager
 *
 */

qm::AutoPilotManager::AutoPilotManager(const WCHAR* pwszPath) :
	helper_(pwszPath)
{
}

qm::AutoPilotManager::~AutoPilotManager()
{
	clear();
}

const AutoPilotManager::EntryList& qm::AutoPilotManager::getEntries()
{
	return getEntries(true);
}

const AutoPilotManager::EntryList& qm::AutoPilotManager::getEntries(bool bReload)
{
	if (bReload)
		load();
	return listEntry_;
}

void qm::AutoPilotManager::setEntries(EntryList& listEntry)
{
	clear();
	listEntry_.swap(listEntry);
}

bool qm::AutoPilotManager::save() const
{
	return helper_.save(this);
}

void qm::AutoPilotManager::clear()
{
	std::for_each(listEntry_.begin(), listEntry_.end(), qs::deleter<AutoPilotEntry>());
	listEntry_.clear();
}

void qm::AutoPilotManager::addEntry(std::auto_ptr<AutoPilotEntry> pEntry)
{
	listEntry_.push_back(pEntry.get());
	pEntry.release();
}

bool qm::AutoPilotManager::load()
{
	AutoPilotContentHandler handler(this);
	return helper_.load(this, &handler);
}


/****************************************************************************
 *
 * AutoPilotEntry
 *
 */

qm::AutoPilotEntry::AutoPilotEntry() :
	nInterval_(5)
{
	wstrCourse_ = allocWString(L"");
}

qm::AutoPilotEntry::AutoPilotEntry(const WCHAR* pwszCourse,
								   int nInterval) :
	nInterval_(nInterval)
{
	wstrCourse_ = allocWString(pwszCourse);
}

qm::AutoPilotEntry::AutoPilotEntry(const AutoPilotEntry& entry) :
	nInterval_(entry.nInterval_)
{
	wstrCourse_ = allocWString(entry.wstrCourse_.get());
}

qm::AutoPilotEntry::~AutoPilotEntry()
{
}

const WCHAR* qm::AutoPilotEntry::getCourse() const
{
	return wstrCourse_.get();
}

void qm::AutoPilotEntry::setCourse(const WCHAR* pwszCourse)
{
	assert(pwszCourse);
	wstrCourse_ = allocWString(pwszCourse);
}

int qm::AutoPilotEntry::getInterval() const
{
	return nInterval_;
}

void qm::AutoPilotEntry::setInterval(int nInterval)
{
	nInterval_ = nInterval;
}


/****************************************************************************
 *
 * AutoPilotContentHandler
 *
 */

qm::AutoPilotContentHandler::AutoPilotContentHandler(AutoPilotManager* pManager) :
	pManager_(pManager),
	state_(STATE_ROOT),
	nInterval_(-1)
{
}

qm::AutoPilotContentHandler::~AutoPilotContentHandler()
{
}

bool qm::AutoPilotContentHandler::startElement(const WCHAR* pwszNamespaceURI,
											   const WCHAR* pwszLocalName,
											   const WCHAR* pwszQName,
											   const Attributes& attributes)
{
	struct {
		const WCHAR* pwszName_;
		State stateBefore_;
		State stateAfter_;
	} states[] = {
		{ L"autoPilot",	STATE_ROOT,			STATE_AUTOPILOT	},
		{ L"entry",		STATE_AUTOPILOT,	STATE_ENTRY		},
		{ L"course",	STATE_ENTRY,		STATE_COURSE	},
		{ L"interval",	STATE_ENTRY,		STATE_INTERVAL	}
	};
	
	int n = 0;
	for (n = 0; n < countof(states); ++n) {
		if (wcscmp(pwszLocalName, states[n].pwszName_) == 0) {
			if (state_ != states[n].stateBefore_)
				return false;
			if (attributes.getLength() != 0)
				return false;
			state_ = states[n].stateAfter_;
			break;
		}
	}
	if (n == countof(states))
		return false;
	
	return true;
}

bool qm::AutoPilotContentHandler::endElement(const WCHAR* pwszNamespaceURI,
											 const WCHAR* pwszLocalName,
											 const WCHAR* pwszQName)
{
	if (wcscmp(pwszLocalName, L"autoPilot") == 0) {
		assert(state_ == STATE_AUTOPILOT);
		state_ = STATE_ROOT;
	}
	else if (wcscmp(pwszLocalName, L"entry") == 0) {
		assert(state_ == STATE_ENTRY);
		
		if (!wstrCourse_.get() || nInterval_ == -1)
			return false;
		
		std::auto_ptr<AutoPilotEntry> pEntry(
			new AutoPilotEntry(wstrCourse_.get(), nInterval_));
		pManager_->addEntry(pEntry);
		
		wstrCourse_.reset(0);
		nInterval_ = -1;
		
		state_ = STATE_AUTOPILOT;
	}
	else if (wcscmp(pwszLocalName, L"course") == 0) {
		assert(state_ == STATE_COURSE);
		
		if (buffer_.getLength() == 0)
			return false;
		wstrCourse_ = buffer_.getString();
		
		state_ = STATE_ENTRY;
	}
	else if (wcscmp(pwszLocalName, L"interval") == 0) {
		assert(state_ == STATE_INTERVAL);
		
		if (buffer_.getLength() == 0)
			return false;
		
		WCHAR* pEnd = 0;
		long nInterval = wcstol(buffer_.getCharArray(), &pEnd, 10);
		if (*pEnd || nInterval <= 0)
			return false;
		nInterval_ = nInterval;
		
		buffer_.remove();
		
		state_ = STATE_ENTRY;
	}
	else {
		return false;
	}
	return true;
}

bool qm::AutoPilotContentHandler::characters(const WCHAR* pwsz,
											 size_t nStart,
											 size_t nLength)
{
	if (state_ == STATE_COURSE ||
		state_ == STATE_INTERVAL) {
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
 * AutoPilotWriter
 *
 */

qm::AutoPilotWriter::AutoPilotWriter(Writer* pWriter) :
	handler_(pWriter)
{
}

qm::AutoPilotWriter::~AutoPilotWriter()
{
}

bool qm::AutoPilotWriter::write(const AutoPilotManager* pManager)
{
	if (!handler_.startDocument())
		return false;
	if (!handler_.startElement(0, 0, L"autoPilot", DefaultAttributes()))
		return false;
	
	const AutoPilotManager::EntryList& l = const_cast<AutoPilotManager*>(pManager)->getEntries(false);
	for (AutoPilotManager::EntryList::const_iterator it = l.begin(); it != l.end(); ++it) {
		const AutoPilotEntry* pEntry = *it;
		if (!write(pEntry))
			return false;
	}
	
	if (!handler_.endElement(0, 0, L"autoPilot"))
		return false;
	if (!handler_.endDocument())
		return false;
	
	return true;
}

bool qm::AutoPilotWriter::write(const AutoPilotEntry* pEntry)
{
	WCHAR wszInterval[32];
	swprintf(wszInterval, L"%d", pEntry->getInterval());
	
	return handler_.startElement(0, 0, L"entry", DefaultAttributes()) &&
		handler_.startElement(0, 0, L"course", DefaultAttributes()) &&
		handler_.characters(pEntry->getCourse(), 0, wcslen(pEntry->getCourse())) &&
		handler_.endElement(0, 0, L"course") &&
		handler_.startElement(0, 0, L"interval", DefaultAttributes()) &&
		handler_.characters(wszInterval, 0, wcslen(wszInterval)) &&
		handler_.endElement(0, 0, L"interval") &&
		handler_.endElement(0, 0, L"entry");
}
