/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#pragma warning(disable:4786)

#include <qmapplication.h>
#include <qmmessage.h>

#include <qsconv.h>
#include <qsfile.h>
#include <qsinit.h>
#include <qslog.h>
#include <qsmd5.h>
#include <qsosutil.h>
#include <qsprofile.h>
#include <qsstream.h>

#include <algorithm>

#include <boost/bind.hpp>

#include "junk.h"

using namespace qmjunk;
using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * DepotPtr
 *
 */

qmjunk::DepotPtr::DepotPtr() :
	pDepot_(0)
{
}

qmjunk::DepotPtr::DepotPtr(DEPOT* pDepot) :
	pDepot_(pDepot)
{
}

qmjunk::DepotPtr::DepotPtr(DepotPtr& ptr) :
	pDepot_(ptr.release())
{
}

qmjunk::DepotPtr::~DepotPtr()
{
	reset(0);
}

DEPOT* qmjunk::DepotPtr::operator->() const
{
	return pDepot_;
}

DepotPtr& qmjunk::DepotPtr::operator=(DepotPtr& ptr)
{
	if (&ptr != this && ptr.pDepot_ != pDepot_)
		reset(ptr.release());
	return *this;
}

DEPOT* qmjunk::DepotPtr::get() const
{
	return pDepot_;
}

DEPOT* qmjunk::DepotPtr::release()
{
	DEPOT* pDepot = pDepot_;
	pDepot_ = 0;
	return pDepot;
}

void qmjunk::DepotPtr::reset(DEPOT* pDepot)
{
	if (pDepot_)
		dpclose(pDepot_);
	pDepot_ = pDepot;
}


/****************************************************************************
 *
 * JunkFilterImpl
 *
 */

qmjunk::JunkFilterImpl::JunkFilterImpl(const WCHAR* pwszPath,
									   Profile* pProfile) :
	pProfile_(pProfile),
	nCleanCount_(-1),
	nJunkCount_(-1),
	fThresholdScore_(0.95f),
	nFlags_(FLAG_AUTOLEARN | FLAG_MANUALLEARN),
	nMaxTextLen_(32*1024),
	bModified_(false)
{
	wstrPath_ = allocWString(pwszPath);
	
	wstring_ptr wstrThresholdScore(pProfile->getString(L"JunkFilter", L"ThresholdScore"));
	WCHAR* pEnd = 0;
	double dThresholdScore = wcstod(wstrThresholdScore.get(), &pEnd);
	if (!*pEnd)
		fThresholdScore_ = static_cast<float>(dThresholdScore);
	
	nFlags_ = pProfile->getInt(L"JunkFilter", L"Flags");
	nMaxTextLen_ = pProfile->getInt(L"JunkFilter", L"MaxTextLen");
	
	pAttachmentScanner_.reset(new AttachmentScanner(pProfile));
	
	wstring_ptr wstrWhiteList(pProfile->getString(L"JunkFilter", L"WhiteList"));
	if (*wstrWhiteList.get())
		pWhiteList_.reset(new AddressList(wstrWhiteList.get()));
	
	wstring_ptr wstrBlackList(pProfile->getString(L"JunkFilter", L"BlackList"));
	if (*wstrBlackList.get())
		pBlackList_.reset(new AddressList(wstrBlackList.get()));
	
	init();
}

qmjunk::JunkFilterImpl::~JunkFilterImpl()
{
}

float qmjunk::JunkFilterImpl::getScore(const Message& msg)
{
	Log log(InitThread::getInitThread().getLogger(), L"qmjunk::JunkFilterImpl");
	
	if (log.isInfoEnabled()) {
		StringBuffer<WSTRING> buf(L"Processing a message");
		
		buf.append(L", Subject: ");
		UnstructuredParser subject;
		if (msg.getField(L"Subject", &subject) == Part::FIELD_EXIST)
			buf.append(subject.getValue());
		
		buf.append(L", Message-Id: ");
		MessageIdParser messageId;
		if (msg.getField(L"Message-Id", &messageId) == Part::FIELD_EXIST) {
			buf.append(L'<');
			buf.append(messageId.getMessageId());
			buf.append(L'>');
		}
		
		log.info(buf.getCharArray());
	}
	
	{
		Lock<CriticalSection> lock(cs_);
		
		if (nCleanCount_ < 100 || nJunkCount_ == 0) {
			log.info(L"Filter a message as clean because it has not learned enough clean messages.");
			return 0.0F;
		}
		else if (nJunkCount_ == 0) {
			return 0.0F;
		}
		
		DEPOT* pDepotId = getIdDepot();
		if (!pDepotId)
			return -1.0F;
		
		string_ptr strId(getId(msg));
		int nId = 0;
		if (dpgetwb(pDepotId, strId.get(), static_cast<int>(strlen(strId.get())),
			0, sizeof(nId), reinterpret_cast<char*>(&nId)) != -1) {
			if (nId > 0) {
				log.info(L"Filter a message as clean because it has already been learned as clean.");
				return 0.0F;
			}
			else if (nId < 0) {
				log.info(L"Filter a message as junk because it has already been learned as junk.");
				return 1.0F;
			}
		}
	}
	
	if (pWhiteList_.get() && pWhiteList_->match(msg)) {
		log.info(L"A message matches against white list.");
		return 0.0F;
	}
	if (pBlackList_.get() && pBlackList_->match(msg)) {
		log.info(L"A message matches against black list.");
		return 1.0F;
	}
	
	struct TokenizerCallbackImpl : public TokenizerCallback
	{
		TokenizerCallbackImpl(DEPOT* pDepotToken,
							  const volatile unsigned int& nCleanCount,
							  const volatile unsigned int& nJunkCount,
							  CriticalSection& cs) :
			pDepotToken_(pDepotToken),
			nCleanCount_(nCleanCount),
			nJunkCount_(nJunkCount),
			cs_(cs),
			nMax_(15)
		{
			listTokenRate_.reserve(nMax_);
		}
		
		virtual ~TokenizerCallbackImpl()
		{
			std::for_each(listTokenRate_.begin(), listTokenRate_.end(),
				boost::bind(&freeWString, boost::bind(&TokenRateList::value_type::first, _1)));
		}
		
		virtual bool token(const WCHAR* pwszToken,
						   size_t nLen)
		{
			for (TokenRateList::const_iterator it = listTokenRate_.begin(); it != listTokenRate_.end(); ++it) {
				if (wcslen((*it).first) == nLen &&
					wcsncmp((*it).first, pwszToken, nLen) == 0)
					return true;
			}
			
			unsigned int nCount[2] = { 0, 0 };
			
			const char* pKey = reinterpret_cast<const char*>(pwszToken);
			size_t nKeyLen = nLen*sizeof(WCHAR);
			char* pValue = reinterpret_cast<char*>(nCount);
			size_t nValueLen = sizeof(nCount);
			
			{
				Lock<CriticalSection> lock(cs_);
				dpgetwb(pDepotToken_, pKey, static_cast<int>(nKeyLen), 0, static_cast<int>(nValueLen), pValue);
			}
			
			double dRate = 0.4;
			if (nCount[0]*2 + nCount[1] > 5) {
				if (nCount[0] == 0) {
					dRate = nCount[1] > 10 ? 0.9999 : 0.9998;
				}
				else if (nCount[1] == 0) {
					dRate = nCount[0] > 10 ? 0.0001 : 0.0002;
				}
				else {
					double dClean = static_cast<double>(nCount[0])*2/static_cast<double>(nCleanCount_);
					if (dClean > 1.0)
						dClean = 1.0;
					double dJunk = static_cast<double>(nCount[1])/static_cast<double>(nJunkCount_);
					if (dJunk > 1.0)
						dJunk = 1.0;
					dRate = dJunk/(dClean + dJunk);
					if (dRate > 0.99)
						dRate = 0.99;
					else if (dRate < 0.01)
						dRate = 0.01;
				}
			}
			
			struct RateLess
			{
				static bool comp(const std::pair<WSTRING, double>& p1,
								 const std::pair<WSTRING, double>& p2)
				{
					double d1 = p1.second;
					double d2 = p2.second;
					return (d1 > 0.5 ? d1 - 0.5 : 0.5 - d1) > (d2 > 0.5 ? d2 - 0.5 : 0.5 - d2);
				}
			};
			
			if (listTokenRate_.size() < nMax_ ||
				!RateLess::comp(listTokenRate_.back(), std::pair<WSTRING, double>(0, dRate))) {
				wstring_ptr wstrToken(allocWString(pwszToken, nLen));
				listTokenRate_.push_back(std::make_pair(wstrToken.get(), dRate));
				wstrToken.release();
				std::random_shuffle(listTokenRate_.begin(), listTokenRate_.end());
				std::sort(listTokenRate_.begin(), listTokenRate_.end(), &RateLess::comp);
				if (listTokenRate_.size() > nMax_) {
					double d = listTokenRate_[nMax_].second;
					TokenRateList::iterator itD = listTokenRate_.begin() + nMax_ + 1;
					while (itD != listTokenRate_.end() && (*itD).second == d)
						++itD;
					for (TokenRateList::iterator it = itD; it != listTokenRate_.end(); ++it)
						freeWString((*it).first);
					listTokenRate_.erase(itD, listTokenRate_.end());
				}
			}
			
			return true;
		}
		
		typedef std::vector<std::pair<WSTRING, double> > TokenRateList;
		
		DEPOT* pDepotToken_;
		const volatile unsigned int& nCleanCount_;
		const volatile unsigned int& nJunkCount_;
		CriticalSection& cs_;
		TokenRateList listTokenRate_;
		const unsigned int nMax_;
	};
	
	DEPOT* pDepotToken = getTokenDepot();
	if (!pDepotToken)
		return -1.0F;
	
	Tokenizer t(nMaxTextLen_, *pAttachmentScanner_.get());
	TokenizerCallbackImpl callback(pDepotToken, nCleanCount_, nJunkCount_, cs_);
	if (!t.getTokens(msg, &callback))
		return -1.0F;
	
	typedef TokenizerCallbackImpl::TokenRateList List;
	List& l = callback.listTokenRate_;
	if (l.empty())
		return 0.0F;
	
	if (log.isInfoEnabled()) {
		log.info(L"Rated tokens:");
		for (List::const_iterator it = l.begin(); it != l.end(); ++it)
			log.infof(L"Token: %s, Score: %f", (*it).first, (*it).second);
	}
	
	double p1 = 1.0;
	double p2 = 1.0;
	for (List::const_iterator it = l.begin(); it != l.end(); ++it) {
		p1 *= (*it).second;
		p2 *= (1 - (*it).second);
	}
	
	float fScore = static_cast<float>(p1/(p1 + p2));
	log.infof(L"Score: %f", fScore);
	return fScore;
}

bool qmjunk::JunkFilterImpl::manage(const Message& msg,
									unsigned int nOperation)
{
	assert(nOperation != 0);
	assert((nOperation & OPERATION_ADDCLEAN) == 0 || (nOperation & OPERATION_ADDJUNK) == 0);
	assert((nOperation & OPERATION_REMOVECLEAN) == 0 || (nOperation & OPERATION_REMOVEJUNK) == 0);
	assert((nOperation & OPERATION_ADDCLEAN) == 0 || (nOperation & OPERATION_REMOVECLEAN) == 0);
	assert((nOperation & OPERATION_ADDJUNK) == 0 || (nOperation & OPERATION_REMOVEJUNK) == 0);
	
	Log log(InitThread::getInitThread().getLogger(), L"qmjunk::JunkFilterImpl");
	
	{
		Lock<CriticalSection> lock(cs_);
		
		DEPOT* pDepotId = getIdDepot();
		if (!pDepotId)
			return false;
		
		bModified_ = true;
		
		string_ptr strId(getId(msg));
		int nStatus = 0;
		if (dpgetwb(pDepotId, strId.get(), static_cast<int>(strlen(strId.get())),
			0, sizeof(nStatus), reinterpret_cast<char*>(&nStatus)) == -1)
			nStatus = STATUS_NONE;
		if (nStatus > 0) {
			nOperation &= ~(JunkFilter::OPERATION_ADDCLEAN | JunkFilter::OPERATION_REMOVEJUNK);
			if (nOperation & JunkFilter::OPERATION_ADDJUNK) {
				nOperation |= JunkFilter::OPERATION_REMOVECLEAN;
				nStatus = STATUS_JUNK;
			}
			else if (nOperation & JunkFilter::OPERATION_REMOVECLEAN) {
				nStatus = STATUS_NONE;
			}
		}
		else if (nStatus < 0) {
			nOperation &= ~(JunkFilter::OPERATION_ADDJUNK | JunkFilter::OPERATION_REMOVECLEAN);
			if (nOperation & JunkFilter::OPERATION_ADDCLEAN) {
				nOperation |= JunkFilter::OPERATION_REMOVEJUNK;
				nStatus = STATUS_CLEAN;
			}
			else if (nOperation & JunkFilter::OPERATION_REMOVEJUNK) {
				nStatus = STATUS_NONE;
			}
		}
		else {
			nOperation &= ~(JunkFilter::OPERATION_REMOVECLEAN | JunkFilter::OPERATION_REMOVEJUNK);
			if (nOperation & JunkFilter::OPERATION_ADDCLEAN)
				nStatus = STATUS_CLEAN;
			else if (nOperation & JunkFilter::OPERATION_ADDJUNK)
				nStatus = STATUS_JUNK;
		}
		dpput(pDepotId, strId.get(), static_cast<int>(strlen(strId.get())),
			reinterpret_cast<char*>(&nStatus), sizeof(nStatus), DP_DOVER);
	}
	
	if (nOperation == 0) {
		log.debug(L"Ignoring a message already learned.");
		return true;
	}
	
	struct TokenizerCallbackImpl : public TokenizerCallback
	{
		TokenizerCallbackImpl(unsigned int nOperation,
							  DEPOT* pDepotToken,
							  CriticalSection& cs,
							  Log& log) :
			nOperation_(nOperation),
			pDepotToken_(pDepotToken),
			cs_(cs),
			log_(log)
		{
		}
		
		virtual bool token(const WCHAR* pwszToken,
						   size_t nLen)
		{
			unsigned int nCount[2] = { 0, 0 };
			
			const char* pKey = reinterpret_cast<const char*>(pwszToken);
			size_t nKeyLen = nLen*sizeof(WCHAR);
			char* pValue = reinterpret_cast<char*>(nCount);
			size_t nValueLen = sizeof(nCount);
			
			{
				Lock<CriticalSection> lock(cs_);
				
				dpgetwb(pDepotToken_, pKey, static_cast<int>(nKeyLen), 0, static_cast<int>(nValueLen), pValue);
				
				if (nOperation_ & JunkFilter::OPERATION_ADDCLEAN)
					++nCount[0];
				if (nOperation_ & JunkFilter::OPERATION_REMOVECLEAN && nCount[0] > 0)
					--nCount[0];
				if (nOperation_ & JunkFilter::OPERATION_ADDJUNK)
					++nCount[1];
				if (nOperation_ & JunkFilter::OPERATION_REMOVEJUNK && nCount[1] > 0)
					--nCount[1];
				
				dpput(pDepotToken_, pKey, static_cast<int>(nKeyLen), pValue, static_cast<int>(nValueLen), DP_DOVER);
			}
			
			if (log_.isDebugEnabled()) {
				wstring_ptr wstrToken(allocWString(pwszToken, nLen));
				log_.debugf(L"Token: %s, Clean: %u, Junk: %u", wstrToken.get(), nCount[0], nCount[1]);
			}
			
			return true;
		}
		
		unsigned int nOperation_;
		DEPOT* pDepotToken_;
		CriticalSection& cs_;
		Log& log_;
	};
	
	DEPOT* pDepotToken = getTokenDepot();
	if (!pDepotToken)
		return false;
	
	Tokenizer t(nMaxTextLen_, *pAttachmentScanner_.get());
	TokenizerCallbackImpl callback(nOperation, pDepotToken, cs_, log);
	if (!t.getTokens(msg, &callback))
		return false;
	
	{
		Lock<CriticalSection> lock(cs_);
		
		if (nOperation & OPERATION_ADDCLEAN)
			++nCleanCount_;
		if (nOperation & OPERATION_REMOVECLEAN && nCleanCount_ != 0)
			--nCleanCount_;
		if (nOperation & OPERATION_ADDJUNK)
			++nJunkCount_;
		if (nOperation & OPERATION_REMOVEJUNK && nJunkCount_ != 0)
			--nJunkCount_;
	}
	
	return true;
}

JunkFilter::Status qmjunk::JunkFilterImpl::getStatus(const WCHAR* pwszId)
{
	assert(pwszId);
	
	if (!*pwszId)
		return STATUS_NONE;
	
	Lock<CriticalSection> lock(cs_);
	
	DEPOT* pDepotId = getIdDepot();
	if (!pDepotId)
		return STATUS_NONE;
	
	string_ptr strId(wcs2mbs(pwszId));
	
	int nStatus = 0;
	if (dpgetwb(pDepotId, strId.get(), static_cast<int>(strlen(strId.get())),
		0, sizeof(nStatus), reinterpret_cast<char*>(&nStatus)) == -1)
		return STATUS_NONE;
	else if (nStatus > 0)
		return STATUS_CLEAN;
	else if (nStatus < 0)
		return STATUS_JUNK;
	else
		return STATUS_NONE;
}

float qmjunk::JunkFilterImpl::getThresholdScore() const
{
	return fThresholdScore_;
}

void qmjunk::JunkFilterImpl::setThresholdScore(float fThresholdScore)
{
	fThresholdScore_ = fThresholdScore;
}

unsigned int qmjunk::JunkFilterImpl::getFlags() const
{
	return nFlags_;
}

void qmjunk::JunkFilterImpl::setFlags(unsigned int nFlags,
									  unsigned int nMask)
{
	nFlags_ = (nFlags_ & ~nMask) | (nFlags & nMask);
}

unsigned int qmjunk::JunkFilterImpl::getMaxTextLength() const
{
	return nMaxTextLen_;
}

void qmjunk::JunkFilterImpl::setMaxTextLength(unsigned int nMaxTextLength)
{
	nMaxTextLen_ = nMaxTextLength;
}

bool qmjunk::JunkFilterImpl::isScanAttachment() const
{
	return pAttachmentScanner_->isEnabled();
}

void qmjunk::JunkFilterImpl::setScanAttachment(bool bScanAttachment)
{
	pAttachmentScanner_->setEnabled(bScanAttachment);
}

unsigned int qmjunk::JunkFilterImpl::getMaxAttachmentSize() const
{
	return pAttachmentScanner_->getMaxSize();
}

void qmjunk::JunkFilterImpl::setMaxAttachmentSize(unsigned int nMaxAttachmentSize)
{
	pAttachmentScanner_->setMaxSize(nMaxAttachmentSize);
}

wstring_ptr qmjunk::JunkFilterImpl::getWhiteList(const WCHAR* pwszSeparator) const
{
	wstring_ptr wstrWhiteList(pWhiteList_.get() ? pWhiteList_->toString(pwszSeparator) : 0);
	return wstrWhiteList.get() ? wstrWhiteList : allocWString(L"");
}

void qmjunk::JunkFilterImpl::setWhiteList(const WCHAR* pwszWhiteList)
{
	if (pwszWhiteList && *pwszWhiteList)
		pWhiteList_.reset(new AddressList(pwszWhiteList));
	else
		pWhiteList_.reset(0);
}

wstring_ptr qmjunk::JunkFilterImpl::getBlackList(const WCHAR* pwszSeparator) const
{
	wstring_ptr wstrBlackList(pBlackList_.get() ? pBlackList_->toString(pwszSeparator) : 0);
	return wstrBlackList.get() ? wstrBlackList : allocWString(L"");
}

void qmjunk::JunkFilterImpl::setBlackList(const WCHAR* pwszBlackList)
{
	if (pwszBlackList && *pwszBlackList)
		pBlackList_.reset(new AddressList(pwszBlackList));
	else
		pBlackList_.reset(0);
}

bool qmjunk::JunkFilterImpl::repair()
{
	Lock<CriticalSection> lock(cs_);
	
	pDepotToken_.reset(0);
	pDepotId_.reset(0);
	
	bool bTokenRepaired = repair(L"token");
	bool bIdRepaired = repair(L"id");
	
	// Open depots here to ensure that depots are optimized.
	// After reparing a depot, it's highly likely that it needs to be optimized.
	getTokenDepot();
	getIdDepot();
	
	return bTokenRepaired && bIdRepaired;
}

bool qmjunk::JunkFilterImpl::save(bool bForce)
{
	if (!flush() && !bForce)
		return false;
	
	WCHAR wszThresholdScore[64];
	_snwprintf(wszThresholdScore, countof(wszThresholdScore), L"%.2f", fThresholdScore_);
	pProfile_->setString(L"JunkFilter", L"ThresholdScore", wszThresholdScore);
	
	pProfile_->setInt(L"JunkFilter", L"Flags", nFlags_);
	pProfile_->setInt(L"JunkFilter", L"MaxTextLen", nMaxTextLen_);
	
	pAttachmentScanner_->save();
	
	wstring_ptr wstrWhiteList(getWhiteList(L" "));
	pProfile_->setString(L"JunkFilter", L"WhiteList", wstrWhiteList.get());
	
	wstring_ptr wstrBlackList(getBlackList(L" "));
	pProfile_->setString(L"JunkFilter", L"BlackList", wstrBlackList.get());
	
	return true;
}

bool qmjunk::JunkFilterImpl::init()
{
	Log log(InitThread::getInitThread().getLogger(), L"qmjunk::JunkFilterImpl");
	
	if (!File::createDirectory(wstrPath_.get())) {
		log.error(L"Could not create a directory for a junk filter.");
		return false;
	}
	
	wstring_ptr wstrProfilePath(concat(wstrPath_.get(), L"\\junk.xml"));
	XMLProfile profile(wstrProfilePath.get(), 0, 0);
	if (!profile.load()) {
		log.error(L"Could not load junk.xml.");
		return false;
	}
	nCleanCount_ = profile.getInt(L"Junk", L"CleanCount", 0);
	nJunkCount_ = profile.getInt(L"Junk", L"JunkCount", 0);
	
	return true;
}

bool qmjunk::JunkFilterImpl::flush() const
{
	Log log(InitThread::getInitThread().getLogger(), L"qmjunk::JunkFilterImpl");
	
	Lock<CriticalSection> lock(cs_);
	
	if (!bModified_)
		return true;
	
	if (pDepotToken_.get())
		dpsync(pDepotToken_.get());
	if (pDepotId_.get())
		dpsync(pDepotId_.get());
	
	if (nCleanCount_ != -1 && nJunkCount_ != -1) {
		wstring_ptr wstrProfilePath(concat(wstrPath_.get(), L"\\junk.xml"));
		XMLProfile profile(wstrProfilePath.get(), 0, 0);
		profile.setInt(L"Junk", L"CleanCount", nCleanCount_);
		profile.setInt(L"Junk", L"JunkCount", nJunkCount_);
		if (!profile.save()) {
			log.error(L"Counld not save junk.xml.");
			return false;
		}
	}
	
	bModified_ = false;
	
	return true;
}

DEPOT* qmjunk::JunkFilterImpl::getTokenDepot()
{
	Lock<CriticalSection> lock(cs_);
	
	if (!pDepotToken_.get())
		pDepotToken_ = open(L"token");
	
	return pDepotToken_.get();
}

DEPOT* qmjunk::JunkFilterImpl::getIdDepot()
{
	Lock<CriticalSection> lock(cs_);
	
	if (!pDepotId_.get())
		pDepotId_ = open(L"id");
	
	return pDepotId_.get();
}

DepotPtr qmjunk::JunkFilterImpl::open(const WCHAR* pwszName) const
{
	Log log(InitThread::getInitThread().getLogger(), L"qmjunk::JunkFilterImpl");
	
	wstring_ptr wstrPath(concat(wstrPath_.get(), L"\\", pwszName));
	string_ptr strPath(wcs2mbs(wstrPath.get()));
	DepotPtr pDepot(dpopen(strPath.get(), DP_OWRITER | DP_OCREAT, -1));
	if (!pDepot.get()) {
		if (log.isErrorEnabled()) {
			wstring_ptr wstrError(mbs2wcs(dperrmsg(dpecode)));
			log.errorf(L"Could not open a database: %s : %s.", pwszName, wstrError.get());
		}
		return DepotPtr(0);
	}
	
	int nBucket = dpbnum(pDepot.get());
	int nCount = dprnum(pDepot.get());
	if (nBucket != -1 && nCount != -1 && nBucket < dpprimenum(nCount*4 + 1)) {
		log.debugf(L"Optimizing a database: %s.", pwszName);
		if (!dpoptimize(pDepot.get(), -1))
			log.errorf(L"Could not optimize the database: %s.", pwszName);
		if (!dpsync(pDepot.get()))
			log.errorf(L"Could not sync the database: %s.", pwszName);
	}
	
	return pDepot;
}

bool qmjunk::JunkFilterImpl::repair(const WCHAR* pwszName) const
{
	Log log(InitThread::getInitThread().getLogger(), L"qmjunk::JunkFilterImpl");
	
	wstring_ptr wstrPath(concat(wstrPath_.get(), L"\\", pwszName));
	if (!File::isFileExisting(wstrPath.get()))
		return true;
	
	string_ptr strPath(wcs2mbs(wstrPath.get()));
	if (!dprepair(strPath.get())) {
		log.errorf(L"Could not repair the database: %s", pwszName);
		return false;
	}
	return true;
}

string_ptr qmjunk::JunkFilterImpl::getId(const Part& part)
{
	MessageIdParser messageId;
	if (part.getField(L"Message-Id", &messageId) == Part::FIELD_EXIST) {
		return wcs2mbs(messageId.getMessageId());
	}
	else {
		const CHAR* pszHeader = part.getHeader();
		string_ptr strId(allocString(128/8*2 + 1));
		MD5::md5ToString(reinterpret_cast<const unsigned char*>(pszHeader),
			strlen(pszHeader), strId.get());
		*(strId.get() + 128/8*2) = '\0';
		return strId;
	}
}


/****************************************************************************
 *
 * JunkFilterFactoryImpl
 *
 */

JunkFilterFactoryImpl qmjunk::JunkFilterFactoryImpl::factory__;

qmjunk::JunkFilterFactoryImpl::JunkFilterFactoryImpl()
{
	registerFactory(this);
}

qmjunk::JunkFilterFactoryImpl::~JunkFilterFactoryImpl()
{
	unregisterFactory(this);
}

std::auto_ptr<JunkFilter> qmjunk::JunkFilterFactoryImpl::createJunkFilter(const WCHAR* pwszPath,
																		  Profile* pProfile)
{
	return std::auto_ptr<JunkFilter>(new JunkFilterImpl(pwszPath, pProfile));
}


/****************************************************************************
 *
 * Tokenizer
 *
 */

qmjunk::Tokenizer::Tokenizer(size_t nMaxTextLen,
							 const AttachmentScanner& scanner) :
	nMaxTextLen_(nMaxTextLen),
	scanner_(scanner)
{
}

qmjunk::Tokenizer::~Tokenizer()
{
}

bool qmjunk::Tokenizer::getTokens(const Part& part,
								  TokenizerCallback* pCallback) const
{
	Part::FieldList listField;
	Part::FieldListFree free(listField);
	part.getFields(&listField);
	for (Part::FieldList::const_iterator it = listField.begin(); it != listField.end(); ++it) {
		wstring_ptr wstrName(mbs2wcs((*it).first));
		UnstructuredParser field;
		if (part.getField(wstrName.get(), &field) == Part::FIELD_EXIST) {
			if (!getTokens(field.getValue(), -1, pCallback))
				return false;
		}
	}
	
	if (part.isMultipart()) {
		const Part::PartList& listPart = part.getPartList();
		for (Part::PartList::const_iterator it = listPart.begin(); it != listPart.end(); ++it) {
			if (!getTokens(**it, pCallback))
				return false;
		}
	}
	else if (part.getEnclosedPart()) {
		if (!getTokens(*part.getEnclosedPart(), pCallback))
			return false;
	}
	else if (part.isText()) {
		wxstring_size_ptr wstrBody(part.getBodyText());
		if (!wstrBody.get())
			return false;
		if (!getTokens(wstrBody.get(), wstrBody.size(), pCallback))
			return false;
	}
	else {
		wstring_ptr wstrExt;
		if (scanner_.check(part, &wstrExt)) {
			malloc_size_ptr<unsigned char> pData(part.getBodyData());
			if (pData.get()) {
				wstring_ptr wstr(scanner_.getText(pData.get(), pData.size(), wstrExt.get()));
				if (wstr.get()) {
					if (!getTokens(wstr.get(), -1, pCallback))
						return false;
				}
			}
		}
	}
	
	return true;
}

bool qmjunk::Tokenizer::getTokens(const WCHAR* pwszText,
								  size_t nLen,
								  TokenizerCallback* pCallback) const
{
	assert(pwszText);
	assert(pCallback);
	
	if (nLen == -1)
		nLen = wcslen(pwszText);
	if (nLen > nMaxTextLen_)
		nLen = nMaxTextLen_;
	
	const WCHAR* p = pwszText;
	const WCHAR* pEnd = p + nLen;
	while (p < pEnd) {
		Token token = getToken(*p);
		switch (token) {
		case TOKEN_LATIN:
			{
				bool bLower = false;
				const WCHAR* pBegin = p;
				do {
					if (!bLower && 'a' <= *p && *p < 'z')
						bLower = true;
					++p;
				} while (p < pEnd && getToken(*p) == token && (!bLower || *p < 'A' || 'Z' < *p));
				
				if (!pCallback->token(pBegin, p - pBegin))
					return false;
			}
			break;
		case TOKEN_KATAKANA:
		case TOKEN_FULLWIDTHLATIN:
			{
				StringBuffer<WSTRING> buf;
				do {
					if (*p != L'\r' && *p != L'\n')
						buf.append(*p);
					++p;
				} while (p < pEnd && (*p == L'\r' || *p == L'\n' || getToken(*p) == token));
				
				if (!pCallback->token(buf.getCharArray(), buf.getLength()))
					return false;
			}
			break;
		case TOKEN_IDEOGRAPHIC:
			{
				WCHAR wsz[3] = { *p, L'\0', L'\0' };
				++p;
				while (p < pEnd && getToken(*p) == TOKEN_IDEOGRAPHIC) {
					wsz[1] = *p;
					if (!isIgnoredToken(wsz)) {
						if (!pCallback->token(wsz, 2))
							return false;
					}
					wsz[0] = *p;
					++p;
				}
			}
			break;
		case TOKEN_SEPARATOR:
			do {
				++p;
			} while (p < pEnd && getToken(*p) == TOKEN_SEPARATOR);
			break;
		default:
			assert(false);
			break;
		}
	}
	
	return true;
}

qmjunk::Tokenizer::Token qmjunk::Tokenizer::getToken(WCHAR c)
{
	if ((L'a' <= c && c <= L'z') ||
		(L'A' <= c && c <= L'Z') ||
		(L'0' <= c && c <= L'9'))
		return TOKEN_LATIN;
	else if (c < 0x7f)
		return TOKEN_SEPARATOR;
	else if (c < 0x500)
		return TOKEN_LATIN;
	else if (0x30a1 <= c && c <= 0x30fe)		// Katakana
		return TOKEN_KATAKANA;
	else if ((0xff10 <= c && c <= 0xff19) ||	// Fullwidth Digit
		(0xff21 <= c && c <= 0xff3a) ||			// Fullwidth Latin Capital
		(0xff41 <= c && c <= 0xff5a))			// Fullwidth Latin Small
		return TOKEN_FULLWIDTHLATIN;
	else
		return TOKEN_IDEOGRAPHIC;
}

bool qmjunk::Tokenizer::isIgnoredToken(const WCHAR* pwsz)
{
	while (*pwsz) {
		if ((0x3041 <= *pwsz && *pwsz <= 0x309e) ||	// Hiragana
			*pwsz == 0x3000 ||						// Ideographic Space
			*pwsz == 0x3001 ||						// Ideographic Comma
			*pwsz == 0x3002)						// Ideographic Full Stop
			return true;
		++pwsz;
	}
	return false;
}


/****************************************************************************
 *
 * TokenizerCallback
 *
 */

qmjunk::TokenizerCallback::~TokenizerCallback()
{
}


/****************************************************************************
 *
 * AddressList
 *
 */

qmjunk::AddressList::AddressList(const WCHAR* pwszAddressList)
{
	if (pwszAddressList && *pwszAddressList) {
		wstring_ptr wstrAddressList(tolower(pwszAddressList));
		WCHAR* p = wcstok(wstrAddressList.get(), L" \r\n\t");
		while (p) {
			wstring_ptr wstr(allocWString(p));
			list_.push_back(wstr.get());
			wstr.release();
			p = wcstok(0, L" \r\n\t");
		}
	}
}

qmjunk::AddressList::~AddressList()
{
	std::for_each(list_.begin(), list_.end(), &freeWString);
}

bool qmjunk::AddressList::match(const qm::Message& msg) const
{
	if (list_.empty())
		return false;
	
	AddressListParser from;
	if (msg.getField(L"From", &from) != Part::FIELD_EXIST)
		return false;
	
	for (List::const_iterator it = list_.begin(); it != list_.end(); ++it) {
		if (contains(from, *it))
			return true;
	}
	
	return false;
}

wstring_ptr qmjunk::AddressList::toString(const WCHAR* pwszSeparator) const
{
	if (list_.empty())
		return 0;
	
	if (!pwszSeparator)
		pwszSeparator = L" ";
	
	StringBuffer<WSTRING> buf;
	for (List::const_iterator it = list_.begin(); it != list_.end(); ++it) {
		if (buf.getLength() != 0)
			buf.append(pwszSeparator);
		buf.append(*it);
	}
	return buf.getString();
}

bool qmjunk::AddressList::contains(const AddressListParser& addresses,
								 const WCHAR* pwsz)
{
	typedef AddressListParser::AddressList List;
	const List& l = addresses.getAddressList();
	for (List::const_iterator it = l.begin(); it != l.end(); ++it) {
		if (contains(**it, pwsz))
			return true;
	}
	return false;
}

bool qmjunk::AddressList::contains(const AddressParser& address,
								 const WCHAR* pwsz)
{
	const AddressListParser* pGroup = address.getGroup();
	if (pGroup)
		return contains(*pGroup, pwsz);
	else
		return wcsstr(tolower(address.getAddress().get()).get(), pwsz) != 0;
}


/****************************************************************************
 *
 * AttachmentScanner
 *
 */

qmjunk::AttachmentScanner::AttachmentScanner(Profile* pProfile) :
	pProfile_(pProfile),
	bEnabled_(false),
	nMaxSize_(32*1024)
{
	bEnabled_ = pProfile->getInt(L"JunkFilter", L"ScanAttachment") != 0;
	nMaxSize_ = pProfile_->getInt(L"JunkFilter", L"MaxAttachmentSize");
	wstrCommand_ = pProfile_->getString(L"JunkFilter", L"AttachmentScanCommand");
	wstring_ptr wstrExtensions = pProfile_->getString(L"JunkFilter", L"AttachmentExtensions");
	wstrExtensions_ = concat(L" ", wstrExtensions.get(), L" ");
}

qmjunk::AttachmentScanner::~AttachmentScanner()
{
}

bool qmjunk::AttachmentScanner::isEnabled() const
{
	return bEnabled_;
}

void qmjunk::AttachmentScanner::setEnabled(bool bEnabled)
{
	bEnabled_ = bEnabled;
}

unsigned int qmjunk::AttachmentScanner::getMaxSize() const
{
	return nMaxSize_;
}

void qmjunk::AttachmentScanner::setMaxSize(unsigned int nMaxSize)
{
	nMaxSize_ = nMaxSize;
}

bool qmjunk::AttachmentScanner::check(const Part& part,
									  wstring_ptr* pwstrExt) const
{
	assert(pwstrExt);
	
	if (!bEnabled_ ||
		!part.isAttachment() ||
		part.getEnclosedPart() ||
		strlen(part.getBody()) > nMaxSize_)
		return false;
	
	wstring_ptr wstrName(AttachmentParser(part).getName());
	if (!wstrName.get())
		return false;
	
	const WCHAR* pExt = wcsrchr(wstrName.get(), '.');
	if (!pExt)
		return false;
	++pExt;
	
	wstring_ptr wstrExt(concat(L" ", tolower(pExt).get(), L" "));
	if (!wcsstr(wstrExtensions_.get(), wstrExt.get()))
		return false;
	
	*pwstrExt = allocWString(pExt);
	
	return true;
}

wstring_ptr qmjunk::AttachmentScanner::getText(const unsigned char* p,
											   size_t nLen,
											   const WCHAR* pwszExtension) const
{
	assert(p);
	assert(pwszExtension);
	
	const WCHAR* pwszDir = Application::getApplication().getTemporaryFolder();
	WCHAR wszName[128];
	Time time(Time::getCurrentTime());
	_snwprintf(wszName, countof(wszName),
		L"%04d%02d%02d%02d%02d%02d%03d%04d",
		time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute,
		time.wSecond, time.wMilliseconds, ::GetCurrentThreadId());
	ConcatW c[] = {
		{ pwszDir,			-1	},
		{ L"\\",			1	},
		{ wszName,			-1	},
		{ L".",				1	},
		{ pwszExtension,	-1	}
	};
	wstring_ptr wstrPath(concat(c, countof(c)));
	
	FileOutputStream stream(wstrPath.get());
	if (!stream)
		return 0;
	stream.write(p, nLen);
	stream.close();
	
	StringBuffer<WSTRING> command;
	command.append(wstrCommand_.get());
	command.append(L" \"");
	command.append(wstrPath.get());
	command.append(L'\"');
	
	wstring_ptr wstr(Process::exec(command.getCharArray(), 0));
	
	W2T(wstrPath.get(), ptszPath);
	::DeleteFile(ptszPath);
	
	return wstr;
}

void qmjunk::AttachmentScanner::save() const
{
	pProfile_->setInt(L"JunkFilter", L"ScanAttachment", bEnabled_);
	pProfile_->setInt(L"JunkFilter", L"MaxAttachmentSize", nMaxSize_);
}
