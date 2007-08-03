/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
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
class AddressList;
class AttachmentScanner;


/****************************************************************************
 *
 * DepotPtr
 *
 */

class DepotPtr
{
public:
	DepotPtr();
	explicit DepotPtr(DEPOT* pDepot);
	DepotPtr(DepotPtr& ptr);
	~DepotPtr();

public:
	DEPOT* operator->() const;
	DepotPtr& operator=(DepotPtr& ptr);

public:
	DEPOT* get() const;
	DEPOT* release();
	void reset(DEPOT* pDepot);

private:
	DEPOT* pDepot_;
};


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
	virtual float getThresholdScore() const;
	virtual void setThresholdScore(float fThresholdScore);
	virtual unsigned int getFlags() const;
	virtual void setFlags(unsigned int nFlags,
						  unsigned int nMask);
	virtual unsigned int getMaxTextLength() const;
	virtual void setMaxTextLength(unsigned int nMaxTextLength);
	virtual bool isScanAttachment() const;
	virtual void setScanAttachment(bool bScanAttachment);
	virtual unsigned int getMaxAttachmentSize() const;
	virtual void setMaxAttachmentSize(unsigned int nMaxAttachmentSize);
	virtual qs::wstring_ptr getWhiteList(const WCHAR* pwszSeparator) const;
	virtual void setWhiteList(const WCHAR* pwszWhiteList);
	virtual qs::wstring_ptr getBlackList(const WCHAR* pwszSeparator) const;
	virtual void setBlackList(const WCHAR* pwszBlackList);
	virtual bool repair();
	virtual bool save(bool bForce);

private:
	bool init();
	bool flush() const;
	DEPOT* getTokenDepot();
	DEPOT* getIdDepot();
	DepotPtr open(const WCHAR* pwszName) const;
	bool repair(const WCHAR* pwszName) const;

private:
	static qs::string_ptr getId(const qs::Part& part);

private:
	JunkFilterImpl(const JunkFilterImpl&);
	JunkFilterImpl& operator=(const JunkFilterImpl&);

private:
	qs::wstring_ptr wstrPath_;
	qs::Profile* pProfile_;
	DepotPtr pDepotToken_;
	DepotPtr pDepotId_;
	volatile unsigned int nCleanCount_;
	volatile unsigned int nJunkCount_;
	float fThresholdScore_;
	unsigned int nFlags_;
	unsigned int nMaxTextLen_;
	std::auto_ptr<AttachmentScanner> pAttachmentScanner_;
	std::auto_ptr<AddressList> pWhiteList_;
	std::auto_ptr<AddressList> pBlackList_;
	mutable bool bModified_;
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
	Tokenizer(size_t nMaxTextLen,
			  const AttachmentScanner& scanner);
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
	const AttachmentScanner& scanner_;
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


/****************************************************************************
 *
 * AddressList
 *
 */

class AddressList
{
public:
	AddressList(const WCHAR* pwszAddressList);
	~AddressList();

public:
	bool match(const qm::Message& msg) const;
	qs::wstring_ptr toString(const WCHAR* pwszSeparator) const;

private:
	static bool contains(const qs::AddressListParser& addresses,
						 const WCHAR* pwsz);
	static bool contains(const qs::AddressParser& address,
						 const WCHAR* pwsz);

private:
	AddressList(const AddressList&);
	AddressList& operator=(const AddressList&);

private:
	typedef std::vector<qs::WSTRING> List;

private:
	List list_;
};


/****************************************************************************
 *
 * AttachmentScanner
 *
 */

class AttachmentScanner
{
public:
	explicit AttachmentScanner(qs::Profile* pProfile);
	~AttachmentScanner();

public:
	bool isEnabled() const;
	void setEnabled(bool bEnabled);
	unsigned int getMaxSize() const;
	void setMaxSize(unsigned int nMaxSize);
	bool check(const qs::Part& part,
			   qs::wstring_ptr* pwstrExt) const;
	qs::wstring_ptr getText(const unsigned char* p,
							size_t nLen,
							const WCHAR* pwszExtension) const;
	void save() const;

private:
	AttachmentScanner(const AttachmentScanner&);
	AttachmentScanner& operator=(const AttachmentScanner&);

private:
	qs::Profile* pProfile_;
	bool bEnabled_;
	unsigned int nMaxSize_;
	qs::wstring_ptr wstrCommand_;
	qs::wstring_ptr wstrExtensions_;
};


}

#endif // __JUNK_H__
