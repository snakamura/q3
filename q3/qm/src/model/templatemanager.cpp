/*
 * $Id: templatemanager.cpp,v 1.1.1.1 2003/04/29 08:07:31 snakamura Exp $
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmaccount.h>
#include <qmtemplate.h>

#include <qsassert.h>
#include <qsconv.h>
#include <qserror.h>
#include <qsnew.h>
#include <qsosutil.h>
#include <qsstl.h>
#include <qsstream.h>

#include <algorithm>

#include "templatemanager.h"

#pragma warning(disable:4786)

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * TemplateManager
 *
 */

qm::TemplateManager::TemplateManager(const WCHAR* pwszPath, QSTATUS* pstatus) :
	wstrPath_(0)
{
	DECLARE_QSTATUS();
	
	wstrPath_ = allocWString(pwszPath);
	if (!wstrPath_) {
		*pstatus = QSTATUS_OUTOFMEMORY;
		return;
	}
}

qm::TemplateManager::~TemplateManager()
{
	freeWString(wstrPath_);
	std::for_each(listItem_.begin(), listItem_.end(), deleter<Item>());
}

QSTATUS qm::TemplateManager::getTemplate(Account* pAccount,
	Folder* pFolder, const WCHAR* pwszName, const Template** ppTemplate) const
{
	assert(pAccount);
	assert(pwszName);
	assert(ppTemplate);
	
	DECLARE_QSTATUS();
	
	*ppTemplate = 0;
	
	string_ptr<WSTRING> wstrPath;
	if (pFolder) {
		WCHAR wszFolder[16];
		swprintf(wszFolder, L"_%d", pFolder->getId());
		ConcatW c[] = {
			{ pAccount->getPath(),	-1 },
			{ L"\\",				-1 },
			{ pwszName,				-1 },
			{ wszFolder,			-1 },
			{ L".template",			-1 }
		};
		wstrPath.reset(concat(c, countof(c)));
		
		W2T(wstrPath.get(), ptszPath);
		if (::GetFileAttributes(ptszPath) == 0xffffffff)
			wstrPath.reset(0);
	}
	
	if (!wstrPath.get()) {
		ConcatW c[] = {
			{ pAccount->getPath(),	-1 },
			{ L"\\",				-1 },
			{ pwszName,				-1 },
			{ L".template",			-1 }
		};
		wstrPath.reset(concat(c, countof(c)));
		if (!wstrPath.get())
			return QSTATUS_OUTOFMEMORY;
		W2T(wstrPath.get(), ptszPath);
		if (::GetFileAttributes(ptszPath) == 0xffffffff) {
			ConcatW c[] = {
				{ wstrPath_,		-1 },
				{ L"\\templates\\",	-1 },
				{ pwszName,			-1 },
				{ L".template",		-1 }
			};
			wstrPath.reset(concat(c, countof(c)));
			if (!wstrPath.get())
				return QSTATUS_OUTOFMEMORY;
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
			*ppTemplate = pItem->getTemplate();
		}
		else {
			delete pItem;
			listItem_.erase(it);
		}
	}
	if (!*ppTemplate) {
		FileInputStream stream(wstrPath.get(), &status);
		CHECK_QSTATUS();
		BufferedInputStream bufferedStream(&stream, false, &status);
		CHECK_QSTATUS();
		InputStreamReader reader(&bufferedStream, false, 0, &status);
		CHECK_QSTATUS();
		
		Template* p = 0;
		TemplateParser parser(&status);
		CHECK_QSTATUS();
		status = parser.parse(&reader, &p);
		CHECK_QSTATUS();
		std::auto_ptr<Template> pTemplate(p);
		
		std::auto_ptr<Item> pItem;
		status = newQsObject(wstrPath.get(), fd.ftLastWriteTime,
			pTemplate.get(), &pItem);
		CHECK_QSTATUS();
		
		status = STLWrapper<ItemList>(listItem_).push_back(pItem.get());
		CHECK_QSTATUS();
		pItem.release();
		
		*ppTemplate = pTemplate.release();
	}
	
	return QSTATUS_SUCCESS;
}

QSTATUS qm::TemplateManager::getTemplateNames(
	const WCHAR* pwszPrefix, NameList* pList) const
{
	assert(pList);
	
	DECLARE_QSTATUS();
	
	NameList l;
	StringListFree<NameList> free(l);
	
	StringBuffer<WSTRING> buf(&status);
	CHECK_QSTATUS();
	status = buf.append(wstrPath_);
	CHECK_QSTATUS();
	status = buf.append(L"\\templates\\");
	CHECK_QSTATUS();
	if (pwszPrefix) {
		status = buf.append(pwszPrefix);
		CHECK_QSTATUS();
		status = buf.append(L'_');
		CHECK_QSTATUS();
	}
	status = buf.append(L"*.template");
	CHECK_QSTATUS();
	
	W2T(buf.getCharArray(), ptszFind);
	WIN32_FIND_DATA fd;
	AutoFindHandle hFind(::FindFirstFile(ptszFind, &fd));
	if (hFind.get()) {
		do {
			string_ptr<WSTRING> wstrName(tcs2wcs(fd.cFileName));
			if (!wstrName.get())
				return QSTATUS_OUTOFMEMORY;
			WCHAR* p = wcsrchr(wstrName.get(), L'.');
			assert(p);
			*p = L'\0';
			status = STLWrapper<NameList>(l).push_back(wstrName.get());
			CHECK_QSTATUS();
			wstrName.release();
		} while (::FindNextFile(hFind.get(), &fd));
	}
	
	status = STLWrapper<NameList>(*pList).resize(l.size());
	CHECK_QSTATUS();
	std::copy(l.begin(), l.end(), pList->begin());
	free.release();
	
	return QSTATUS_SUCCESS;
}


/****************************************************************************
 *
 * TemplateManager::Item
 *
 */

qm::TemplateManager::Item::Item(const WCHAR* pwszPath,
	const FILETIME& ft, Template* pTemplate, QSTATUS* pstatus) :
	wstrPath_(0),
	ft_(ft),
	pTemplate_(0)
{
	wstrPath_ = allocWString(pwszPath);
	if (!wstrPath_) {
		*pstatus = QSTATUS_OUTOFMEMORY;
		return;
	}
	pTemplate_ = pTemplate;
	
	*pstatus = QSTATUS_SUCCESS;
}

qm::TemplateManager::Item::~Item()
{
	freeWString(wstrPath_);
	delete pTemplate_;
}

const WCHAR* qm::TemplateManager::Item::getPath() const
{
	return wstrPath_;
}

const FILETIME& qm::TemplateManager::Item::getFileTime() const
{
	return ft_;
}

Template* qm::TemplateManager::Item::getTemplate() const
{
	return pTemplate_;
}
