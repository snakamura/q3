/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmmessage.h>

#include <qsconv.h>
#include <qsfile.h>
#include <qsinit.h>
#include <qslog.h>
#include <qsmd5.h>
#include <qsprofile.h>

#include "junk.h"

#pragma warning(disable:4786)

using namespace qmjunk;
using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * JunkFilterImpl
 *
 */

qmjunk::JunkFilterImpl::JunkFilterImpl(const WCHAR* pwszPath,
									   Profile* pProfile) :
	pProfile_(pProfile),
	pDepotToken_(0),
	pDepotId_(0),
	nCleanCount_(-1),
	nJunkCount_(-1),
	fThresholdScore_(0.95f),
	nFlags_(FLAG_AUTOLEARN | FLAG_MANUALLEARN),
	nMaxTextLen_(32*1024),
	bModified_(false)
{
	wstrPath_ = allocWString(pwszPath);
	
	wstring_ptr wstrThresholdScore(pProfile->getString(L"JunkFilter", L"ThresholdScore", L"0.95"));
	WCHAR* pEnd = 0;
	double dThresholdScore = wcstod(wstrThresholdScore.get(), &pEnd);
	if (!*pEnd)
		fThresholdScore_ = static_cast<float>(dThresholdScore);
	
	nFlags_ = pProfile->getInt(L"JunkFilter", L"Flags", FLAG_AUTOLEARN | FLAG_MANUALLEARN);
	nMaxTextLen_ = pProfile->getInt(L"JunkFilter", L"MaxTextLen", 32*1024);
}

qmjunk::JunkFilterImpl::~JunkFilterImpl()
{
	if (pDepotToken_)
		dpclose(pDepotToken_);
	if (pDepotId_)
		dpclose(pDepotId_);
}

float qmjunk::JunkFilterImpl::getScore(const Message& msg)
{
	Log log(InitThread::getInitThread().getLogger(), L"qmjunk::JunkFilterImpl");
	
	{
		Lock<CriticalSection> lock(cs_);
		
		if (!init()) {
			return -1.0F;
		}
		else if (nCleanCount_ < 100 || nJunkCount_ == 0) {
			log.debug(L"Filter a message as clean because it has not learned enough clean messages.");
			return 0.0F;
		}
		else if (nJunkCount_ == 0) {
			return 0.0F;
		}
		
		string_ptr strId(getId(msg));
		int nId = 0;
		if (dpgetwb(pDepotId_, strId.get(), strlen(strId.get()),
			0, sizeof(nId), reinterpret_cast<char*>(&nId)) != -1) {
			if (nId > 0) {
				log.debug(L"Filter a message as clean because it has already been learned as clean.");
				return 0.0F;
			}
			else if (nId < 0) {
				log.debug(L"Filter a message as junk because it has already been learned as junk.");
				return 1.0F;
			}
		}
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
				 unary_compose_f_gx(
					 string_free<WSTRING>(),
					 std::select1st<TokenRateList::value_type>()));
		}
		
		virtual bool token(const WCHAR* pwszToken)
		{
			if (std::find_if(listTokenRate_.begin(), listTokenRate_.end(),
				std::bind2nd(
					binary_compose_f_gx_hy(
						string_equal<WCHAR>(),
						std::select1st<TokenRateList::value_type>(),
						std::identity<const WCHAR*>()),
					pwszToken)) != listTokenRate_.end())
				return true;
			
			unsigned int nCount[2] = { 0, 0 };
			
			const char* pKey = reinterpret_cast<const char*>(pwszToken);
			size_t nKeyLen = wcslen(pwszToken)*sizeof(WCHAR);
			char* pValue = reinterpret_cast<char*>(nCount);
			size_t nValueLen = sizeof(nCount);
			
			{
				Lock<CriticalSection> lock(cs_);
				dpgetwb(pDepotToken_, pKey, nKeyLen, 0, nValueLen, pValue);
			}
			
			float fRate = 0.4F;
			if (nCount[0]*2 + nCount[1] > 5) {
				if (nCount[0] == 0) {
					fRate = nCount[1] > 10 ? 0.9999F : 0.9998F;
				}
				else if (nCount[1] == 0) {
					fRate = nCount[0] > 10 ? 0.0001F : 0.0002F;
				}
				else {
					float fClean = static_cast<float>(nCount[0])*2/static_cast<float>(nCleanCount_);
					if (fClean > 1.0F)
						fClean = 1.0F;
					float fJunk = static_cast<float>(nCount[1])/static_cast<float>(nJunkCount_);
					if (fJunk > 1.0F)
						fJunk = 1.0F;
					fRate = fJunk/(fClean + fJunk);
					if (fRate > 0.99F)
						fRate = 0.99F;
				}
			}
			
			float fDiff = fRate > 0.5F ? fRate - 0.5F : 0.5F - fRate;
			
			TokenRateList::iterator itR = listTokenRate_.begin();
			while (itR != listTokenRate_.end()) {
				float f = (*itR).second;
				float fD = f > 0.5F ? f - 0.5F : 0.5F - f;
				if (fDiff > fD)
					break;
				++itR;
			}
			if (itR - listTokenRate_.begin() <= static_cast<int>(nMax_)) {
				listTokenRate_.insert(itR, std::make_pair(allocWString(pwszToken).release(), fRate));
				if (listTokenRate_.size() > nMax_) {
					assert(listTokenRate_.size() == nMax_ + 1);
					freeWString(listTokenRate_.back().first);
					listTokenRate_.pop_back();
				}
			}
			
			return true;
		}
		
		typedef std::vector<std::pair<WSTRING, float> > TokenRateList;
		
		DEPOT* pDepotToken_;
		const volatile unsigned int& nCleanCount_;
		const volatile unsigned int& nJunkCount_;
		CriticalSection& cs_;
		TokenRateList listTokenRate_;
		const unsigned int nMax_;
	} callback(pDepotToken_, nCleanCount_, nJunkCount_, cs_);
	if (!Tokenizer(nMaxTextLen_).getTokens(msg, &callback))
		return -1.0F;
	
	typedef TokenizerCallbackImpl::TokenRateList List;
	List& l = callback.listTokenRate_;
	if (l.empty())
		return 0.0F;
	
	if (log.isDebugEnabled()) {
		log.debug(L"Rated tokens:");
		for (List::const_iterator it = l.begin(); it != l.end(); ++it)
			log.debugf(L"Token: %s, Score: %f", (*it).first, (*it).second);
	}
	
	float p1 = 1;
	float p2 = 1;
	for (List::const_iterator it = l.begin(); it != l.end(); ++it) {
		p1 *= (*it).second;
		p2 *= (1 - (*it).second);
	}
	
	float fScore = p1/(p1 + p2);
	log.debugf(L"Score: %f", fScore);
	return fScore;
}

bool qmjunk::JunkFilterImpl::manage(const Message& msg,
									unsigned int nOperation)
{
	Log log(InitThread::getInitThread().getLogger(), L"qmjunk::JunkFilterImpl");
	
	assert(nOperation != 0);
	assert((nOperation & OPERATION_ADDCLEAN) == 0 || (nOperation & OPERATION_ADDJUNK) == 0);
	assert((nOperation & OPERATION_REMOVECLEAN) == 0 || (nOperation & OPERATION_REMOVEJUNK) == 0);
	assert((nOperation & OPERATION_ADDCLEAN) == 0 || (nOperation & OPERATION_REMOVECLEAN) == 0);
	assert((nOperation & OPERATION_ADDJUNK) == 0 || (nOperation & OPERATION_REMOVEJUNK) == 0);
	
	{
		Lock<CriticalSection> lock(cs_);
		
		if (!init())
			return false;
		
		bModified_ = true;
		
		string_ptr strId(getId(msg));
		int nStatus = 0;
		if (dpgetwb(pDepotId_, strId.get(), strlen(strId.get()),
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
		dpput(pDepotId_, strId.get(), strlen(strId.get()),
			reinterpret_cast<char*>(&nStatus), sizeof(nStatus), DP_DOVER);
	}
	
	if (nOperation == 0) {
		log.debug(L"Ignoring an message already learned.");
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
		
		virtual bool token(const WCHAR* pwszToken)
		{
			unsigned int nCount[2] = { 0, 0 };
			
			const char* pKey = reinterpret_cast<const char*>(pwszToken);
			size_t nKeyLen = wcslen(pwszToken)*sizeof(WCHAR);
			char* pValue = reinterpret_cast<char*>(nCount);
			size_t nValueLen = sizeof(nCount);
			
			{
				Lock<CriticalSection> lock(cs_);
				
				dpgetwb(pDepotToken_, pKey, nKeyLen, 0, nValueLen, pValue);
				
				if (nOperation_ & JunkFilter::OPERATION_ADDCLEAN)
					++nCount[0];
				if (nOperation_ & JunkFilter::OPERATION_REMOVECLEAN && nCount[0] > 0)
					--nCount[0];
				if (nOperation_ & JunkFilter::OPERATION_ADDJUNK)
					++nCount[1];
				if (nOperation_ & JunkFilter::OPERATION_REMOVEJUNK && nCount[1] > 0)
					--nCount[1];
				
				dpput(pDepotToken_, pKey, nKeyLen, pValue, nValueLen, DP_DOVER);
			}
			
			log_.debugf(L"Token: %s, Clean: %u, Junk: %u", pwszToken, nCount[0], nCount[1]);
			
			return true;
		}
		
		unsigned int nOperation_;
		DEPOT* pDepotToken_;
		CriticalSection& cs_;
		Log& log_;
	} callback(nOperation, pDepotToken_, cs_, log);
	if (!Tokenizer(nMaxTextLen_).getTokens(msg, &callback))
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
	
	if (!init())
		return STATUS_NONE;
	
	string_ptr strId(wcs2mbs(pwszId));
	
	int nStatus = 0;
	if (dpgetwb(pDepotId_, strId.get(), strlen(strId.get()),
		0, sizeof(nStatus), reinterpret_cast<char*>(&nStatus)) == -1)
		return STATUS_NONE;
	else if (nStatus > 0)
		return STATUS_CLEAN;
	else if (nStatus < 0)
		return STATUS_JUNK;
	else
		return STATUS_NONE;
}

float qmjunk::JunkFilterImpl::getThresholdScore()
{
	return fThresholdScore_;
}

unsigned int qmjunk::JunkFilterImpl::getFlags()
{
	return nFlags_;
}

bool qmjunk::JunkFilterImpl::save()
{
	if (!bModified_)
		return true;
	
	if (!flush())
		return false;
	
	WCHAR wszThresholdScore[64];
	swprintf(wszThresholdScore, L"%f", fThresholdScore_);
	pProfile_->setString(L"JunkFilter", L"ThresholdScore", wszThresholdScore);
	
	pProfile_->setInt(L"JunkFilter", L"Flags", nFlags_);
	pProfile_->setInt(L"JunkFilter", L"MaxTextLen", nMaxTextLen_);
	
	bModified_ = false;
	
	return true;
}

bool qmjunk::JunkFilterImpl::init()
{
	if (pDepotToken_ && pDepotId_)
		return true;
	
	Log log(InitThread::getInitThread().getLogger(), L"qmjunk::JunkFilterImpl");
	
	if (!File::createDirectory(wstrPath_.get())) {
		log.error(L"Could not create a directory for a junk filter.");
		return false;
	}
	
	wstring_ptr wstrProfilePath(concat(wstrPath_.get(), L"\\junk.xml"));
	XMLProfile profile(wstrProfilePath.get());
	if (!profile.load()) {
		log.error(L"Could not load junk.xml.");
		return false;
	}
	nCleanCount_ = profile.getInt(L"Junk", L"CleanCount", 0);
	nJunkCount_ = profile.getInt(L"Junk", L"JunkCount", 0);
	
	if (!pDepotToken_) {
		pDepotToken_ = open(L"token");
		if (!pDepotToken_)
			return false;
	}
	
	if (!pDepotId_) {
		pDepotId_ = open(L"id");
		if (!pDepotId_)
			return false;
	}
	
	return true;
}

bool qmjunk::JunkFilterImpl::flush() const
{
	Log log(InitThread::getInitThread().getLogger(), L"qmjunk::JunkFilterImpl");
	
	Lock<CriticalSection> lock(cs_);
	
	if (nCleanCount_ != -1 && nJunkCount_ != -1) {
		wstring_ptr wstrProfilePath(concat(wstrPath_.get(), L"\\junk.xml"));
		XMLProfile profile(wstrProfilePath.get());
		profile.setInt(L"Junk", L"CleanCount", nCleanCount_);
		profile.setInt(L"Junk", L"JunkCount", nJunkCount_);
		if (!profile.save()) {
			log.error(L"Counld not save junk.xml.");
			return false;
		}
	}
	
	if (pDepotToken_) {
		if (!dpsync(pDepotToken_)) {
			log.error(L"Could not sync token database.");
			return false;
		}
	}
	if (pDepotId_) {
		if (!dpsync(pDepotId_)) {
			log.error(L"Could not sync id database.");
			return false;
		}
	}
	
	return true;
}

DEPOT* qmjunk::JunkFilterImpl::open(const WCHAR* pwszName) const
{
	Log log(InitThread::getInitThread().getLogger(), L"qmjunk::JunkFilterImpl");
	
	wstring_ptr wstrPath(concat(wstrPath_.get(), L"\\", pwszName));
	string_ptr strPath(wcs2mbs(wstrPath.get()));
	DEPOT* pDepot = dpopen(strPath.get(), DP_OWRITER | DP_OCREAT, -1);
	if (!pDepot) {
		log.errorf(L"Could not open a database: %s.", pwszName);
		return 0;
	}
	
	int nBucket = dpbnum(pDepot);
	int nCount = dprnum(pDepot);
	if (nBucket != -1 && nCount != -1 && nBucket < dpprimenum(nCount*4 + 1)) {
		log.debugf(L"Optimizing a database: %s.", pwszName);
		if (!dpoptimize(pDepot, -1))
			log.errorf(L"Could not optimize the database: %s.", pwszName);
		if (!dpsync(pDepot))
			log.errorf(L"Could not sync the database: %s.", pwszName);
	}
	
	return pDepot;
}

string_ptr qmjunk::JunkFilterImpl::getId(const qs::Part& part)
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

qmjunk::Tokenizer::Tokenizer(size_t nMaxTextLen) :
	nMaxTextLen_(nMaxTextLen)
{
}

qmjunk::Tokenizer::~Tokenizer()
{
}

bool qmjunk::Tokenizer::getTokens(const qs::Part& part,
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
	else if (part.isText()) {
		wxstring_size_ptr wstrBody(part.getBodyText());
		if (!wstrBody.get())
			return false;
		if (!getTokens(wstrBody.get(), wstrBody.size(), pCallback))
			return false;
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
				const WCHAR* pBegin = p;
				do {
					++p;
				} while (p < pEnd && getToken(*p) == token);
				
				wstring_ptr wstrToken(allocWString(pBegin, p - pBegin));
				if (!pCallback->token(wstrToken.get()))
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
				
				if (!pCallback->token(buf.getCharArray()))
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
						if (!pCallback->token(wsz))
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
	else if (c < 0x200)
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


