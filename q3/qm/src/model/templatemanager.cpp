/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#pragma warning(disable:4786)

#include <qmaccount.h>
#include <qmtemplate.h>

#include <qsassert.h>
#include <qsconv.h>
#include <qsosutil.h>
#include <qsstl.h>
#include <qsstream.h>

#include <algorithm>

#include "templatemanager.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * TemplateManager
 *
 */

qm::TemplateManager::TemplateManager(const WCHAR* pwszPath)
{
	wstrPath_ = allocWString(pwszPath);
}

qm::TemplateManager::~TemplateManager()
{
	std::for_each(listItem_.begin(), listItem_.end(), deleter<Item>());
}

const Template* qm::TemplateManager::getTemplate(Account* pAccount,
												 Folder* pFolder,
												 const WCHAR* pwszName) const
{
	assert(pAccount);
	assert(pwszName);
	
	wstring_ptr wstrPath;
	if (pFolder) {
		WCHAR wszFolder[16];
		swprintf(wszFolder, L"_%d", pFolder->getId());
		ConcatW c[] = {
			{ pAccount->getPath(),	-1 },
			{ L"\\templates\\",		-1 },
			{ pwszName,				-1 },
			{ wszFolder,			-1 },
			{ L".template",			-1 }
		};
		wstrPath = concat(c, countof(c));
		
		W2T(wstrPath.get(), ptszPath);
		if (::GetFileAttributes(ptszPath) == 0xffffffff)
			wstrPath.reset(0);
	}
	
	if (!wstrPath.get()) {
		ConcatW c[] = {
			{ pAccount->getPath(),	-1 },
			{ L"\\templates\\",		-1 },
			{ pwszName,				-1 },
			{ L".template",			-1 }
		};
		wstrPath = concat(c, countof(c));
		
		W2T(wstrPath.get(), ptszPath);
		if (::GetFileAttributes(ptszPath) == 0xffffffff) {
			ConcatW c[] = {
				{ wstrPath_.get(),		-1 },
				{ L"\\templates\\",		-1 },
				{ pAccount->getClass(),	-1 },
				{ L"\\",				-1 },
				{ pwszName,				-1 },
				{ L".template",			-1 }
			};
			wstrPath = concat(c, countof(c));
		}
	}
	assert(wstrPath.get());
	
	W2T(wstrPath.get(), ptszPath);
	WIN32_FIND_DATA fd;
	AutoFindHandle hFind(::FindFirstFile(ptszPath, &fd));
	
	ItemList::iterator it = std::find_if(listItem_.begin(), listItem_.end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				string_equal<WCHAR>(),
				std::mem_fun(&Item::getPath),
				std::identity<const WCHAR*>()),
			wstrPath.get()));
	if (it != listItem_.end()) {
		Item* pItem = *it;
		if (::CompareFileTime(&fd.ftLastWriteTime, &pItem->getFileTime()) == 0) {
			return pItem->getTemplate();
		}
		else {
			delete pItem;
			listItem_.erase(it);
		}
	}
	
	FileInputStream stream(wstrPath.get());
	if (!stream)
		return 0;
	BufferedInputStream bufferedStream(&stream, false);
	InputStreamReader reader(&bufferedStream, false, 0);
	if (!reader)
		return 0;
	
	std::auto_ptr<Template> pTemplate(TemplateParser().parse(&reader));
	if (!pTemplate.get())
		return 0;
	
	std::auto_ptr<Item> pItem(new Item(
		wstrPath.get(), fd.ftLastWriteTime, pTemplate));
	
	listItem_.push_back(pItem.get());
	return pItem.release()->getTemplate();
}

void qm::TemplateManager::getTemplateNames(Account* pAccount,
										   const WCHAR* pwszPrefix,
										   NameList* pList) const
{
	assert(pAccount);
	assert(pList);
	
	NameList l;
	StringListFree<NameList> free(l);
	
	StringBuffer<WSTRING> buf;
	buf.append(wstrPath_.get());
	buf.append(L"\\templates\\");
	buf.append(pAccount->getClass());
	buf.append(L"\\");
	if (pwszPrefix) {
		buf.append(pwszPrefix);
		buf.append(L'_');
	}
	buf.append(L"*.template");
	
	W2T(buf.getCharArray(), ptszFind);
	WIN32_FIND_DATA fd;
	AutoFindHandle hFind(::FindFirstFile(ptszFind, &fd));
	if (hFind.get()) {
		do {
			wstring_ptr wstrName(tcs2wcs(fd.cFileName));
			WCHAR* p = wcsrchr(wstrName.get(), L'.');
			assert(p);
			*p = L'\0';
			l.push_back(wstrName.get());
			wstrName.release();
		} while (::FindNextFile(hFind.get(), &fd));
	}
	
	pList->assign(l.begin(), l.end());
	free.release();
}


/****************************************************************************
 *
 * TemplateManager::Item
 *
 */

qm::TemplateManager::Item::Item(const WCHAR* pwszPath,
								const FILETIME& ft,
								std::auto_ptr<Template> pTemplate) :
	wstrPath_(0),
	ft_(ft),
	pTemplate_(pTemplate)
{
	wstrPath_ = allocWString(pwszPath);
}

qm::TemplateManager::Item::~Item()
{
}

const WCHAR* qm::TemplateManager::Item::getPath() const
{
	return wstrPath_.get();
}

const FILETIME& qm::TemplateManager::Item::getFileTime() const
{
	return ft_;
}

const Template* qm::TemplateManager::Item::getTemplate() const
{
	return pTemplate_.get();
}
