/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmaccount.h>
#include <qmfolder.h>

#include <qsfile.h>
#include <qsosutil.h>

#include "util.h"
#include "../model/dataobject.h"
#include "../model/uri.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * Util
 *
 */

wstring_ptr qm::Util::convertLFtoCRLF(const WCHAR* pwsz)
{
	StringBuffer<WSTRING> buf;
	while (*pwsz) {
		if (*pwsz == L'\n')
			buf.append(L'\r');
		buf.append(*pwsz);
		++pwsz;
	}
	return buf.getString();
}

wstring_ptr qm::Util::convertCRLFtoLF(const WCHAR* pwsz)
{
	StringBuffer<WSTRING> buf;
	while (*pwsz) {
		if (*pwsz != L'\r')
			buf.append(*pwsz);
		++pwsz;
	}
	return buf.getString();
}

wstring_ptr qm::Util::formatAccount(Account* pAccount)
{
	return concat(L"//", pAccount->getName());
}

wstring_ptr qm::Util::formatFolder(Folder* pFolder)
{
	wstring_ptr wstrName(pFolder->getFullName());
	ConcatW c[] = {
		{ L"//",							2	},
		{ pFolder->getAccount()->getName(),	-1	},
		{ L"/",								1	},
		{ wstrName.get(),					-1	}
	};
	return concat(c, countof(c));
}

wstring_ptr qm::Util::formatFolders(const Account::FolderList& l,
									const WCHAR* pwszSeparator)
{
	StringBuffer<WSTRING> buf;
	for (Account::FolderList::const_iterator it = l.begin(); it != l.end(); ++it) {
		wstring_ptr wstr(formatFolder(*it));
		if (buf.getLength() != 0)
			buf.append(pwszSeparator);
		buf.append(wstr.get());
	}
	return buf.getString();
}

std::pair<Account*, Folder*> qm::Util::getAccountOrFolder(AccountManager* pAccountManager,
														  const WCHAR* pwsz)
{
	std::pair<Account*, Folder*> p(0, 0);
	if (wcsncmp(pwsz, L"//", 2) != 0)
		return p;
	
	wstring_ptr wstrAccount;
	const WCHAR* pFolder = wcschr(pwsz + 2, L'/');
	if (pFolder) {
		wstrAccount = allocWString(pwsz + 2, pFolder - (pwsz + 2));
		++pFolder;
	}
	else {
		wstrAccount = allocWString(pwsz + 2);
	}
	
	Account* pAccount = pAccountManager->getAccount(wstrAccount.get());
	if (!pAccount)
		return p;
	
	if (pFolder)
		p.second = pAccount->getFolder(pFolder);
	else
		p.first = pAccount;
	
	return p;
}

unsigned int qm::Util::getMessageCount(Account* pAccount)
{
	unsigned int nCount = 0;
	
	const Account::FolderList& l = pAccount->getFolders();
	for (Account::FolderList::const_iterator it = l.begin(); it != l.end(); ++it) {
		Folder* pFolder = *it;
		if (pFolder->getType() == Folder::TYPE_NORMAL)
			nCount += pFolder->getCount();
	}
	
	return nCount;
}

unsigned int qm::Util::getUnseenMessageCount(Account* pAccount)
{
	unsigned int nCount = 0;
	
	const unsigned int nIgnore =
		(Folder::FLAG_BOX_MASK & ~Folder::FLAG_INBOX) |
		Folder::FLAG_IGNOREUNSEEN;
	const Account::FolderList& l = pAccount->getFolders();
	for (Account::FolderList::const_iterator it = l.begin(); it != l.end(); ++it) {
		Folder* pFolder = *it;
		if (pFolder->getType() == Folder::TYPE_NORMAL &&
			(pFolder->getFlags() & nIgnore) == 0)
			nCount += pFolder->getUnseenCount();
	}
	
	return nCount;
}

bool qm::Util::setMessagesLabel(Account* pAccount,
								const MessageHolderList& l,
								LabelType type,
								const WCHAR* pwszLabel,
								UndoItemList* pUndoItemList)
{
	switch (type) {
	case LABELTYPE_SET:
		if (!pAccount->setMessagesLabel(l, pwszLabel, pUndoItemList))
			return false;
		break;
	case LABELTYPE_ADD:
		for (MessageHolderList::const_iterator it = l.begin(); it != l.end(); ++it) {
			MessageHolder* pmh = *it;
			wstring_ptr wstrLabel(pmh->getLabel());
			if (*wstrLabel.get()) {
				const WCHAR* p = wcstok(wstrLabel.get(), L" ");
				while (p) {
					if (wcscmp(p, pwszLabel) == 0)
						return true;
					p = wcstok(0, L" ");
				}
				wstrLabel = concat(pmh->getLabel().get(), L" ", pwszLabel);
				if (!pAccount->setMessagesLabel(MessageHolderList(1, pmh), wstrLabel.get(), pUndoItemList))
					return false;
			}
			else {
				if (!pAccount->setMessagesLabel(MessageHolderList(1, pmh), pwszLabel, pUndoItemList))
					return false;
			}
		}
		break;
	case LABELTYPE_REMOVE:
		for (MessageHolderList::const_iterator it = l.begin(); it != l.end(); ++it) {
			MessageHolder* pmh = *it;
			wstring_ptr wstrLabel(pmh->getLabel());
			if (*wstrLabel.get()) {
				StringBuffer<WSTRING> buf;
				const WCHAR* p = wcstok(wstrLabel.get(), L" ");
				while (p) {
					if (wcscmp(p, pwszLabel) != 0) {
						if (buf.getLength() != 0)
							buf.append(L' ');
						buf.append(p);
					}
					p = wcstok(0, L" ");
				}
				if (!pAccount->setMessagesLabel(MessageHolderList(1, pmh), buf.getCharArray(), pUndoItemList))
					return false;
			}
		}
		break;
	default:
		assert(false);
		break;
	}
	return true;
}

bool qm::Util::hasFilesOrURIs(IDataObject* pDataObject)
{
#ifndef _WIN32_WCE
	FORMATETC fe = {
		CF_HDROP,
		0,
		DVASPECT_CONTENT,
		-1,
		TYMED_HGLOBAL
	};
	StgMedium stm;
	if (pDataObject->GetData(&fe, &stm) == S_OK) {
		if (stm.tymed == TYMED_HGLOBAL) {
			HDROP hDrop = reinterpret_cast<HDROP>(stm.hGlobal);
			UINT nCount = ::DragQueryFile(hDrop, 0xffffffff, 0, 0);
			for (UINT n = 0; n < nCount; ++n) {
				TCHAR tszPath[MAX_PATH];
				::DragQueryFile(hDrop, n, tszPath, countof(tszPath));
				T2W(tszPath, pwszPath);
				if (File::isFileExisting(pwszPath))
					return true;
			}
		}
	}
#endif
	
	return MessageDataObject::canPasteMessage(pDataObject);
}

void qm::Util::getFilesOrURIs(IDataObject* pDataObject,
							  PathList* pList)
{
#ifndef _WIN32_WCE
	FORMATETC fe = {
		CF_HDROP,
		0,
		DVASPECT_CONTENT,
		-1,
		TYMED_HGLOBAL
	};
	StgMedium stm;
	if (pDataObject->GetData(&fe, &stm) == S_OK && stm.tymed == TYMED_HGLOBAL) {
		HDROP hDrop = reinterpret_cast<HDROP>(stm.hGlobal);
		UINT nCount = ::DragQueryFile(hDrop, 0xffffffff, 0, 0);
		for (UINT n = 0; n < nCount; ++n) {
			TCHAR tszPath[MAX_PATH];
			::DragQueryFile(hDrop, n, tszPath, countof(tszPath));
			wstring_ptr wstrPath(tcs2wcs(tszPath));
			if (File::isFileExisting(wstrPath.get())) {
				pList->push_back(wstrPath.get());
				wstrPath.release();
			}
		}
	}
#endif
	
	MessageDataObject::URIList listURI;
	struct Deleter
	{
		Deleter(MessageDataObject::URIList& l) : l_(l) {}
		~Deleter() { std::for_each(l_.begin(), l_.end(), qs::deleter<URI>()); }
		MessageDataObject::URIList& l_;
	} deleter(listURI);
	if (MessageDataObject::getURIs(pDataObject, &listURI)) {
		for (MessageDataObject::URIList::const_iterator it = listURI.begin(); it != listURI.end(); ++it) {
			wstring_ptr wstrURI((*it)->toString());
			pList->push_back(wstrURI.get());
			wstrURI.release();
		}
	}
}
