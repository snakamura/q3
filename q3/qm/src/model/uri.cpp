/*
 * $Id: uri.cpp,v 1.1.1.1 2003/04/29 08:07:31 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
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

QSTATUS qm::URI::getMessageHolder(const WCHAR* pwszURI,
	Document* pDocument, MessageHolder** ppmh)
{
	assert(pwszURI);
	assert(pDocument);
	assert(ppmh);
	
	DECLARE_QSTATUS();
	
	*ppmh = 0;
	
	string_ptr<WSTRING> wstrURI(allocWString(pwszURI));
	if (!wstrURI.get())
		return QSTATUS_OUTOFMEMORY;
	
	if (wcsncmp(wstrURI.get(), L"urn:qmail://", 12) == 0) {
		const WCHAR* pwszAccount = wstrURI.get() + 12;
		WCHAR* pwszFolder = wcschr(pwszAccount, L'/');
		if (pwszFolder) {
			*pwszFolder = L'\0';
			++pwszFolder;
			
			WCHAR* pwszId = wcsrchr(pwszFolder, L'/');
			if (pwszId) {
				*pwszId = L'\0';
				++pwszId;
				
				WCHAR* pEnd = 0;
				unsigned int nId = wcstol(pwszId, &pEnd, 10);
				if (*pEnd == L'\0') {
					WCHAR* pwszValidity = wcsrchr(pwszFolder, L'/');
					if (pwszValidity) {
						*pwszValidity = L'\0';
						++pwszValidity;
						
						WCHAR* pEnd = 0;
						unsigned int nValidity = wcstol(pwszValidity, &pEnd, 10);
						if (*pEnd == L'\0') {
							Account* pAccount = pDocument->getAccount(pwszAccount);
							if (pAccount) {
								Folder* pFolder = 0;
								status = pAccount->getFolder(pwszFolder, &pFolder);
								CHECK_QSTATUS();
								if (pFolder->getType() == Folder::TYPE_NORMAL &&
									static_cast<NormalFolder*>(pFolder)->getValidity() == nValidity) {
									status = static_cast<NormalFolder*>(
										pFolder)->getMessageById(nId, ppmh);
									CHECK_QSTATUS();
								}
							}
						}
					}
				}
			}
		}
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::URI::getURI(MessageHolder* pmh, WSTRING* pwstrURI)
{
	assert(pmh);
	assert(pwstrURI);
	
	DECLARE_QSTATUS();
	
	*pwstrURI = 0;
	
	NormalFolder* pFolder = pmh->getFolder();
	Account* pAccount = pFolder->getAccount();
	
	string_ptr<WSTRING> wstrFolderName;
	status = pFolder->getFullName(&wstrFolderName);
	CHECK_QSTATUS();
	
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
	*pwstrURI = concat(c, countof(c));
	if (!*pwstrURI)
		return QSTATUS_OUTOFMEMORY;
	
	return QSTATUS_SUCCESS;
}
