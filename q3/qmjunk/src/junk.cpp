/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
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

qmjunk::JunkFilterImpl::JunkFilterImpl(const WCHAR* pwszPath) :
	pDepotToken_(0),
	pDepotId_(0),
	nCleanCount_(-1),
	nJunkCount_(-1),
	fThresholdScore_(0.95f),
	nFlags_(FLAG_AUTOLEARN | FLAG_MANUALLEARN)
{
	wstrPath_ = allocWString(pwszPath);
}

qmjunk::JunkFilterImpl::~JunkFilterImpl()
{
	flush();
	
	if (pDepotToken_)
		dpclose(pDepotToken_);
	if (pDepotId_)
		dpclose(pDepotId_);
}

float qmjunk::JunkFilterImpl::getScore(const Message& msg)
{
	Log log(InitThread::getInitThread().getLogger(), L"qmjunk::JunkFilterImpl");
	
	Lock<CriticalSection> lock(cs_);
	
	if (!init())
		return 0.0F;
	else if (nCleanCount_ < 100 || nJunkCount_ == 0)
		return 0.0F;
	
	string_ptr strId(getId(msg));
	int nId = 0;
	if (dpgetwb(pDepotId_, strId.get(), strlen(strId.get()),
		0, sizeof(nId), reinterpret_cast<char*>(&nId)) != -1) {
		if (nId > 0)
			return 0.0F;
		else if (nId < 0)
			return 1.0F;
	}
	
	struct TokenizerCallbackImpl : public TokenizerCallback
	{
		TokenizerCallbackImpl(DEPOT* pDepotToken,
							  unsigned int nCleanCount,
							  unsigned int nJunkCount) :
			pDepotToken_(pDepotToken),
			nCleanCount_(nCleanCount),
			nJunkCount_(nJunkCount),
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
			
			dpgetwb(pDepotToken_, pKey, nKeyLen, 0, nValueLen, pValue);
			
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
		unsigned int nCleanCount_;
		unsigned int nJunkCount_;
		TokenRateList listTokenRate_;
		const unsigned int nMax_;
	} callback(pDepotToken_, nCleanCount_, nJunkCount_);
	if (!Tokenizer().getTokens(msg, &callback))
		return 0.0F;
	
	typedef TokenizerCallbackImpl::TokenRateList List;
	List& l = callback.listTokenRate_;
	if (l.empty())
		return 0.0F;
	
	if (log.isDebugEnabled()) {
		log.debug(L"Rated tokens:");
		for (List::const_iterator it = l.begin(); it != l.end(); ++it) {
			WCHAR wszScore[64];
			swprintf(wszScore, L", Score: %f", (*it).second);
			wstring_ptr wstrLog(concat(L"Token: ", (*it).first, wszScore));
			log.debug(wstrLog.get());
		}
	}
	
	float p1 = 1;
	float p2 = 1;
	for (List::const_iterator it = l.begin(); it != l.end(); ++it) {
		p1 *= (*it).second;
		p2 *= (1 - (*it).second);
	}
	
	float fScore = p1/(p1 + p2);
	
	if (log.isDebugEnabled()) {
		WCHAR wszLog[64];
		swprintf(wszLog, L"Score: %f", fScore);
		log.debug(wszLog);
	}
	
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
	
	Lock<CriticalSection> lock(cs_);
	
	if (!init())
		return false;
	
	string_ptr strId(getId(msg));
	int nId = 0;
	if (dpgetwb(pDepotId_, strId.get(), strlen(strId.get()),
		0, sizeof(nId), reinterpret_cast<char*>(&nId)) == -1)
		nId = 0;
	if (nId > 0) {
		nOperation &= ~(JunkFilter::OPERATION_ADDCLEAN | JunkFilter::OPERATION_REMOVEJUNK);
		if (nOperation & JunkFilter::OPERATION_ADDJUNK) {
			nOperation |= JunkFilter::OPERATION_REMOVECLEAN;
			nId = -1;
		}
		else if (nOperation & JunkFilter::OPERATION_REMOVECLEAN) {
			nId = 0;
		}
	}
	else if (nId < 0) {
		nOperation &= ~(JunkFilter::OPERATION_ADDJUNK | JunkFilter::OPERATION_REMOVECLEAN);
		if (nOperation & JunkFilter::OPERATION_ADDCLEAN) {
			nOperation |= JunkFilter::OPERATION_REMOVEJUNK;
			nId = 1;
		}
		else if (nOperation & JunkFilter::OPERATION_REMOVEJUNK) {
			nId = 0;
		}
	}
	else {
		nOperation &= ~(JunkFilter::OPERATION_REMOVECLEAN | JunkFilter::OPERATION_REMOVEJUNK);
		if (nOperation & JunkFilter::OPERATION_ADDCLEAN)
			nId = 1;
		else if (nOperation & JunkFilter::OPERATION_ADDJUNK)
			nId = -1;
	}
	dpput(pDepotId_, strId.get(), strlen(strId.get()), reinterpret_cast<char*>(&nId), sizeof(nId), DP_DOVER);
	
	if (nOperation == 0)
		return true;
	
	struct TokenizerCallbackImpl : public TokenizerCallback
	{
		TokenizerCallbackImpl(unsigned int nOperation,
							  DEPOT* pDepotToken,
							  Log& log) :
			nOperation_(nOperation),
			pDepotToken_(pDepotToken),
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
			
			if (log_.isDebugEnabled()) {
				WCHAR wszCount[64];
				swprintf(wszCount, L", Clean: %u, Junk: %u", nCount[0], nCount[1]);
				wstring_ptr wstrLog(concat(L"Token: ", pwszToken, wszCount));
				log_.debug(wstrLog.get());
			}
			
			return true;
		}
		
		unsigned int nOperation_;
		DEPOT* pDepotToken_;
		Log& log_;
	} callback(nOperation, pDepotToken_, log);
	if (!Tokenizer().getTokens(msg, &callback))
		return false;
	
	if (nOperation & OPERATION_ADDCLEAN)
		++nCleanCount_;
	if (nOperation & OPERATION_REMOVECLEAN && nCleanCount_ != 0)
		--nCleanCount_;
	if (nOperation & OPERATION_ADDJUNK)
		++nJunkCount_;
	if (nOperation & OPERATION_REMOVEJUNK && nJunkCount_ != 0)
		--nJunkCount_;
	
	flush();
	
	return true;
}

float qmjunk::JunkFilterImpl::getThresholdScore()
{
	init();
	
	return fThresholdScore_;
}

unsigned int qmjunk::JunkFilterImpl::getFlags()
{
	init();
	
	return nFlags_;
}

bool qmjunk::JunkFilterImpl::init()
{
	if (pDepotToken_ && pDepotId_)
		return true;
	
	if (!File::createDirectory(wstrPath_.get()))
		return false;
	
	wstring_ptr wstrProfilePath(concat(wstrPath_.get(), L"\\junk.xml"));
	XMLProfile profile(wstrProfilePath.get());
	if (!profile.load())
		return false;
	nCleanCount_ = profile.getInt(L"Junk", L"CleanCount", 0);
	nJunkCount_ = profile.getInt(L"Junk", L"JunkCount", 0);
	
	wstring_ptr wstrThresholdScore(profile.getString(L"Junk", L"ThresholdScore", L"0.95"));
	WCHAR* pEnd = 0;
	double dThresholdScore = wcstod(wstrThresholdScore.get(), &pEnd);
	if (!*pEnd)
		fThresholdScore_ = static_cast<float>(dThresholdScore);
	
	nFlags_ = profile.getInt(L"Junk", L"Flags", FLAG_AUTOLEARN | FLAG_MANUALLEARN);
	
	if (!pDepotToken_) {
		wstring_ptr wstrTokenPath(concat(wstrPath_.get(), L"\\token"));
		string_ptr strTokenPath(wcs2mbs(wstrTokenPath.get()));
		pDepotToken_ = dpopen(strTokenPath.get(), DP_OWRITER | DP_OCREAT, -1);
		if (!pDepotToken_)
			return false;
	}
	
	if (!pDepotId_) {
		wstring_ptr wstrIdPath(concat(wstrPath_.get(), L"\\id"));
		string_ptr strIdPath(wcs2mbs(wstrIdPath.get()));
		pDepotId_ = dpopen(strIdPath.get(), DP_OWRITER | DP_OCREAT, -1);
		if (!pDepotId_)
			return false;
	}
	
	return true;
}

bool qmjunk::JunkFilterImpl::flush() const
{
	if (nCleanCount_ != -1 && nJunkCount_ != -1) {
		wstring_ptr wstrProfilePath(concat(wstrPath_.get(), L"\\junk.xml"));
		XMLProfile profile(wstrProfilePath.get());
		profile.setInt(L"Junk", L"CleanCount", nCleanCount_);
		profile.setInt(L"Junk", L"JunkCount", nJunkCount_);
		
		WCHAR wszThresholdScore[64];
		swprintf(wszThresholdScore, L"%f", fThresholdScore_);
		profile.setString(L"Junk", L"ThresholdScore", wszThresholdScore);
		
		profile.setInt(L"Junk", L"Flags", nFlags_);
		
		if (!profile.save())
			return false;
	}
	
	if (pDepotToken_) {
		if (!dpsync(pDepotToken_))
			return false;
	}
	if (pDepotId_) {
		if (!dpsync(pDepotId_))
			return false;
	}
	
	return true;
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

std::auto_ptr<JunkFilter> qmjunk::JunkFilterFactoryImpl::createJunkFilter(const WCHAR* pwszPath)
{
	return std::auto_ptr<JunkFilter>(new JunkFilterImpl(pwszPath));
}


/****************************************************************************
 *
 * Tokenizer
 *
 */

qmjunk::Tokenizer::Tokenizer()
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
			if (!getTokens(field.getValue(), pCallback))
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
		wxstring_ptr wstrBody(part.getBodyText());
		if (!wstrBody.get())
			return false;
		if (!getTokens(wstrBody.get(), pCallback))
			return false;
	}
	
	return true;
}

bool qmjunk::Tokenizer::getTokens(const WCHAR* pwszText,
								  TokenizerCallback* pCallback) const
{
	const WCHAR* p = pwszText;
	
	while (*p) {
		Token token = getToken(*p);
		switch (token) {
		case TOKEN_LATEN:
			{
				const WCHAR* pBegin = p;
				do {
					++p;
				} while (*p && getToken(*p) == TOKEN_LATEN);
				
				wstring_ptr wstrToken(allocWString(pBegin, p - pBegin));
				if (!pCallback->token(wstrToken.get()))
					return false;
			}
			break;
		case TOKEN_IDEOGRAPHIC:
			{
				WCHAR wsz[3] = { *p, L'\0', L'\0' };
				++p;
				while (*p && getToken(*p) == TOKEN_IDEOGRAPHIC) {
					wsz[1] = *p;
					if (!pCallback->token(wsz))
						return false;
					wsz[0] = *p;
					++p;
				}
			}
			break;
		case TOKEN_SEPARATOR:
			do {
				++p;
			} while (*p && getToken(*p) == TOKEN_SEPARATOR);
			break;
		default:
			assert(false);
			break;
		}
	}
	
	return true;
}

qmjunk::Tokenizer::Token qmjunk::Tokenizer::getToken(WCHAR c) const
{
	if ((L'a' <= c && c <= L'z') ||
		(L'A' <= c && c <= L'Z') ||
		(L'0' <= c && c <= L'9'))
		return TOKEN_LATEN;
	else if (c < 0x7f)
		return TOKEN_SEPARATOR;
	else if (c < 0x200)
		return TOKEN_LATEN;
	else
		return TOKEN_IDEOGRAPHIC;
}


/****************************************************************************
 *
 * TokenizerCallback
 *
 */

qmjunk::TokenizerCallback::~TokenizerCallback()
{
}


