/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __RSSDRIVER_H__
#define __RSSDRIVER_H__

#include <qmprotocoldriver.h>


namespace qmrss {

/****************************************************************************
 *
 * RssDriver
 *
 */

class RssDriver : public qm::ProtocolDriver
{
public:
	explicit RssDriver(qm::Account* pAccount);
	virtual ~RssDriver();

public:
	virtual bool isSupport(qm::Account::Support support);
	virtual bool createDefaultFolders(qm::Account::FolderList* pList);
	virtual std::pair<const WCHAR**, size_t> getFolderParamNames(qm::Folder* pFolder);
	virtual void setDefaultFolderParams(qm::NormalFolder* pFolder);

private:
	RssDriver(const RssDriver&);
	RssDriver& operator=(const RssDriver&);

private:
	qm::Account* pAccount_;

private:
	static const unsigned int nSupport__;
	static const WCHAR* pwszParamNames__[];
	static const WCHAR* pwszParamValues__[];
};


/****************************************************************************
 *
 * RssFactory
 *
 */

class RssFactory : public qm::ProtocolFactory
{
private:
	RssFactory();

public:
	virtual ~RssFactory();

protected:
	virtual std::auto_ptr<qm::ProtocolDriver> createDriver(qm::Account* pAccount,
														   qm::PasswordCallback* pPasswordCallback,
														   const qm::Security* pSecurity);

private:
	RssFactory(const RssFactory&);
	RssFactory& operator=(const RssFactory&);

private:
	static RssFactory factory__;
};

}

#endif // __RSSDRIVER_H__
