/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmaccount.h>
#include <qmdocument.h>
#include <qmfolder.h>
#include <qmmessageholder.h>

#include <qsthread.h>

#include "uri.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * URI
 *
 */

qm::URI::URI(const WCHAR* pwszAccount,
			 const WCHAR* pwszFolder,
			 unsigned int nValidity,
			 unsigned int nId) :
	nValidity_(nValidity),
	nId_(nId)
{
	wstrAccount_ = allocWString(pwszAccount);
	wstrFolder_ = allocWString(pwszFolder);
}

qm::URI::URI(MessageHolder* pmh) :
	nValidity_(-1),
	nId_(-1)
{
	NormalFolder* pFolder = pmh->getFolder();
	wstrAccount_ = allocWString(pFolder->getAccount()->getName());
	wstrFolder_ = pFolder->getFullName();
	nValidity_ = pFolder->getValidity();
	nId_ = pmh->getId();
}

qm::URI::~URI()
{
}

const WCHAR* qm::URI::getAccount() const
{
	return wstrAccount_.get();
}

const WCHAR* qm::URI::getFolder() const
{
	return wstrFolder_.get();
}

unsigned int qm::URI::getValidity() const
{
	return nValidity_;
}

unsigned int qm::URI::getId() const
{
	return nId_;
}

wstring_ptr qm::URI::toString() const
{
	WCHAR wszUidValidity[32];
	swprintf(wszUidValidity, L"%u", nValidity_);
	
	WCHAR wszId[32];
	swprintf(wszId, L"%u", nId_);
	
	ConcatW c[] = {
		{ L"urn:qmail://",		-1	},
		{ wstrAccount_.get(),	-1	},
		{ L"/",					1	},
		{ wstrFolder_.get(),	-1	},
		{ L"/",					1	},
		{ wszUidValidity,		-1	},
		{ L"/",					1	},
		{ wszId,				-1	}
	};
	return concat(c, countof(c));
}

const WCHAR* qm::URI::getScheme()
{
	return L"urn:qmail";
}

std::auto_ptr<URI> qm::URI::parse(const WCHAR* pwszURI)
{
	assert(pwszURI);
	
	std::auto_ptr<URI> pURI;
	
	wstring_ptr wstrURI(allocWString(pwszURI));
	if (wcsncmp(wstrURI.get(), L"urn:qmail://", 12) != 0)
		return pURI;
	
	const WCHAR* pwszAccount = wstrURI.get() + 12;
	WCHAR* pwszFolder = wcschr(pwszAccount, L'/');
	if (!pwszFolder)
		return pURI;
	*pwszFolder = L'\0';
	++pwszFolder;
	
	WCHAR* pwszId = wcsrchr(pwszFolder, L'/');
	if (!pwszId)
		return pURI;
	*pwszId = L'\0';
	++pwszId;
	
	WCHAR* pEndId = 0;
	unsigned int nId = wcstol(pwszId, &pEndId, 10);
	if (*pEndId != L'\0')
		return pURI;
	
	WCHAR* pwszValidity = wcsrchr(pwszFolder, L'/');
	if (!pwszValidity)
		return pURI;
	*pwszValidity = L'\0';
	++pwszValidity;
	
	WCHAR* pEndValidity = 0;
	unsigned int nValidity = wcstol(pwszValidity, &pEndValidity, 10);
	if (*pEndValidity != L'\0')
		return pURI;
	
	pURI.reset(new URI(pwszAccount, pwszFolder, nValidity, nId));
	
	return pURI;
}
