/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
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
	virtual bool init();
	virtual bool save();
	virtual bool isSupport(qm::Account::Support support);
	virtual void setOffline(bool bOffline);
	
	virtual std::auto_ptr<qm::NormalFolder> createFolder(qm::SubAccount* pSubAccount,
														 const WCHAR* pwszName,
														 qm::Folder* pParent);
	virtual bool removeFolder(qm::SubAccount* pSubAccount,
							  qm::NormalFolder* pFolder);
	virtual bool renameFolder(qm::SubAccount* pSubAccount,
							  qm::NormalFolder* pFolder,
							  const WCHAR* pwszName);
	virtual bool createDefaultFolders(qm::Account::FolderList* pList);
	virtual bool getRemoteFolders(qm::SubAccount* pSubAccount,
								  RemoteFolderList* pList);
	virtual std::pair<const WCHAR**, size_t> getFolderParamNames();
	
	virtual bool getMessage(qm::SubAccount* pSubAccount,
							qm::MessageHolder* pmh,
							unsigned int nFlags,
							qs::xstring_ptr* pstrMessage,
							qm::Message::Flag* pFlag,
							bool* pbMadeSeen);
	virtual bool setMessagesFlags(qm::SubAccount* pSubAccount,
								  qm::NormalFolder* pFolder,
								  const qm::MessageHolderList& l,
								  unsigned int nFlags,
								  unsigned int nMask);
	virtual bool appendMessage(qm::SubAccount* pSubAccount,
							   qm::NormalFolder* pFolder,
							   const CHAR* pszMessage,
							   unsigned int nFlags);
	virtual bool removeMessages(qm::SubAccount* pSubAccount,
								qm::NormalFolder* pFolder,
								const qm::MessageHolderList& l);
	virtual bool copyMessages(qm::SubAccount* pSubAccount,
							  const qm::MessageHolderList& l,
							  qm::NormalFolder* pFolderFrom,
							  qm::NormalFolder* pFolderTo,
							  bool bMove);

private:
	RssDriver(const RssDriver&);
	RssDriver& operator=(const RssDriver&);

private:
	qm::Account* pAccount_;

private:
	static const unsigned int nSupport__;
	static const WCHAR* pwszParams__[];
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
														   const qm::Security* pSecurity);

private:
	RssFactory(const RssFactory&);
	RssFactory& operator=(const RssFactory&);

private:
	static RssFactory factory__;
};

}

#endif // __RSSDRIVER_H__
