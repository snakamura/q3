/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __JUNK_H__
#define __JUNK_H__

#include <qmjunk.h>

#include <qsmime.h>
#include <qsstring.h>
#include <qsthread.h>

#include <depot.h>


namespace qmjunk {

class JunkFilterImpl;
class JunkFilterFactoryImpl;
class Tokenizer;
class TokenizerCallback;


/****************************************************************************
 *
 * JunkFilterImpl
 *
 */

class JunkFilterImpl : public qm::JunkFilter
{
public:
	JunkFilterImpl(const WCHAR* pwszPath,
				   qs::Profile* pProfile);
	virtual ~JunkFilterImpl();

public:
	virtual float getScore(const qm::Message& msg);
	virtual bool manage(const qm::Message& msg,
						unsigned int nOperation);
	virtual Status getStatus(const WCHAR* pwszId);
	virtual float getThresholdScore();
	virtual unsigned int getFlags();
	virtual bool save();

private:
	bool init();
	bool flush() const;
	DEPOT* open(const WCHAR* pwszName) const;

private:
	static qs::string_ptr getId(const qs::Part& part);

private:
	JunkFilterImpl(const JunkFilterImpl&);
	JunkFilterImpl& operator=(const JunkFilterImpl&);

private:
	qs::wstring_ptr wstrPath_;
	qs::Profile* pProfile_;
	DEPOT* pDepotToken_;
	DEPOT* pDepotId_;
	volatile unsigned int nCleanCount_;
	volatile unsigned int nJunkCount_;
	float fThresholdScore_;
	unsigned int nFlags_;
	size_t nMaxTextLen_;
	bool bModified_;
	qs::CriticalSection cs_;
};


/****************************************************************************
 *
 * JunkFilterFactoryImpl
 *
 */

class JunkFilterFactoryImpl : public qm::JunkFilterFactory
{
public:
	JunkFilterFactoryImpl();
	virtual ~JunkFilterFactoryImpl();

public:
	virtual std::auto_ptr<qm::JunkFilter> createJunkFilter(const WCHAR* pwszPath,
														   qs::Profile* pProfile);

private:
	JunkFilterFactoryImpl(const JunkFilterFactoryImpl&);
	JunkFilterFactoryImpl& operator=(const JunkFilterFactoryImpl&);

private:
	static JunkFilterFactoryImpl factory__;
};


/****************************************************************************
 *
 * Tokenizer
 *
 */

class Tokenizer
{
public:
	explicit Tokenizer(size_t nMaxTextLen);
	~Tokenizer();

public:
	bool getTokens(const qs::Part& part,
				   TokenizerCallback* pCallback) const;
	bool getTokens(const WCHAR* pwszText,
				   size_t nLen,
				   TokenizerCallback* pCallback) const;

private:
	enum Token {
		TOKEN_LATIN,
		TOKEN_IDEOGRAPHIC,
		TOKEN_SEPARATOR,
		TOKEN_KATAKANA,
		TOKEN_FULLWIDTHLATIN
	};

private:
	static Token getToken(WCHAR c);
	static bool isIgnoredToken(const WCHAR* pwsz);

private:
	Tokenizer(const Tokenizer&);
	Tokenizer& operator=(const Tokenizer&);

private:
	size_t nMaxTextLen_;
};


/****************************************************************************
 *
 * TokenizerCallback
 *
 */

class TokenizerCallback
{
public:
	virtual ~TokenizerCallback();

public:
	virtual bool token(const WCHAR* pwszToken,
					   size_t nLen) = 0;
};


}

#endif // __JUNK_H__
