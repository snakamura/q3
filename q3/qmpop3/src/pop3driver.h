/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#ifndef __POP3DRIVER_H__
#define __POP3DRIVER_H__

#include <qmprotocoldriver.h>


namespace qmpop3 {

class Pop3Driver;
class Pop3Factory;


/****************************************************************************
 *
 * Pop3Driver
 *
 */

class Pop3Driver : public qm::ProtocolDriver
{
public:
	explicit Pop3Driver(qm::Account* pAccount);
	virtual ~Pop3Driver();

public:
	virtual bool isSupport(qm::Account::Support support);
	virtual bool createDefaultFolders(qm::Account::FolderList* pList);
	virtual std::pair<const WCHAR**, size_t> getFolderParamNames(qm::Folder* pFolder);

private:
	Pop3Driver(const Pop3Driver&);
	Pop3Driver& operator=(const Pop3Driver&);

private:
	qm::Account* pAccount_;

private:
	static const unsigned int nSupport__;
	static const WCHAR* pwszParamNames__[];
};


/****************************************************************************
 *
 * Pop3Factory
 *
 */

class Pop3Factory : public qm::ProtocolFactory
{
private:
	Pop3Factory();

public:
	~Pop3Factory();

protected:
	virtual std::auto_ptr<qm::ProtocolDriver> createDriver(qm::Account* pAccount,
														   qm::PasswordCallback* pPasswordCallback,
														   const qm::Security* pSecurity);

private:
	Pop3Factory(const Pop3Factory&);
	Pop3Factory& operator=(const Pop3Factory&);

private:
	static Pop3Factory factory__;
};

}

#endif // __POP3DRIVER_H__
