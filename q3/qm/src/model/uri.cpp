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

bool qm::URI::getMessageHolder(const WCHAR* pwszURI,
							   Document* pDocument,
							   MessagePtr* pptr)
{
	assert(pwszURI);
	assert(pDocument);
	assert(pptr);
	
	wstring_ptr wstrURI(allocWString(pwszURI));
	if (wcsncmp(wstrURI.get(), L"urn:qmail://", 12) != 0)
		return false;
	
	const WCHAR* pwszAccount = wstrURI.get() + 12;
	WCHAR* pwszFolder = wcschr(pwszAccount, L'/');
	if (!pwszFolder)
		return false;
	*pwszFolder = L'\0';
	++pwszFolder;
	
	WCHAR* pwszId = wcsrchr(pwszFolder, L'/');
	if (!pwszId)
		return false;
	*pwszId = L'\0';
	++pwszId;
	
	WCHAR* pEndId = 0;
	unsigned int nId = wcstol(pwszId, &pEndId, 10);
	if (*pEndId != L'\0')
		return false;
	
	WCHAR* pwszValidity = wcsrchr(pwszFolder, L'/');
	if (!pwszValidity)
		return false;
	*pwszValidity = L'\0';
	++pwszValidity;
	
	WCHAR* pEndValidity = 0;
	unsigned int nValidity = wcstol(pwszValidity, &pEndValidity, 10);
	if (*pEndValidity != L'\0')
		return false;
	
	Account* pAccount = pDocument->getAccount(pwszAccount);
	if (!pAccount)
		return false;
	
	Folder* pFolder = pAccount->getFolder(pwszFolder);
	if (!pFolder ||
		pFolder->getType() != Folder::TYPE_NORMAL ||
		static_cast<NormalFolder*>(pFolder)->getValidity() != nValidity)
		return false;
	*pptr = static_cast<NormalFolder*>(pFolder)->getMessageById(nId);
	
	return true;
}

wstring_ptr qm::URI::getURI(MessageHolder* pmh)
{
	assert(pmh);
	
	NormalFolder* pFolder = pmh->getFolder();
	Account* pAccount = pFolder->getAccount();
	
	wstring_ptr wstrFolderName(pFolder->getFullName());
	
	WCHAR wszUidValidity[32];
	swprintf(wszUidValidity, L"%u", pFolder->getValidity());
	
	WCHAR wszId[32];
	swprintf(wszId, L"%u", pmh->getId());
	
	ConcatW c[] = {
		{ L"urn:qmail://",		-1	},
		{ pAccount->getName(),	-1	},
		{ L"/",					1	},
		{ wstrFolderName.get(),	-1	},
		{ L"/",					1	},
		{ wszUidValidity,		-1	},
		{ L"/",					1	},
		{ wszId,				-1	}
	};
	return concat(c, countof(c));
}
