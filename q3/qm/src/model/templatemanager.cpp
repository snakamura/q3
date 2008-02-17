/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#pragma warning(disable:4786)

#include <qmaccount.h>
#include <qmtemplate.h>

#include <qsassert.h>
#include <qsconv.h>
#include <qsfile.h>
#include <qsinit.h>
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
	std::for_each(listItem_.begin(), listItem_.end(),
		boost::checked_deleter<Item>());
}

const Template* qm::TemplateManager::getTemplate(Account* pAccount,
												 Folder* pFolder,
												 const WCHAR* pwszName) const
{
	assert(pwszName);
	
	Log log(InitThread::getInitThread().getLogger(), L"qm::TemplateManager");
	
	wstring_ptr wstrPath;
	if (pFolder) {
		assert(pAccount);
		
		WCHAR wszFolder[16];
		_snwprintf(wszFolder, countof(wszFolder), L"_%d", pFolder->getId());
		ConcatW c[] = {
			{ pAccount->getPath(),	-1 },
			{ L"\\templates\\",		-1 },
			{ pwszName,				-1 },
			{ wszFolder,			-1 },
			{ L".template",			-1 }
		};
		wstrPath = concat(c, countof(c));
		
		log.debugf(L"Checking template file: %s.", wstrPath.get());
		
		if (!File::isFileExisting(wstrPath.get()))
			wstrPath.reset(0);
	}
	
	if (!wstrPath.get() && pAccount) {
		ConcatW c[] = {
			{ pAccount->getPath(),	-1 },
			{ L"\\templates\\",		-1 },
			{ pwszName,				-1 },
			{ L".template",			-1 }
		};
		wstrPath = concat(c, countof(c));
		
		log.debugf(L"Checking template file: %s.", wstrPath.get());
		
		if (!File::isFileExisting(wstrPath.get()))
			wstrPath.reset(0);
	}
	
	if (!wstrPath.get()) {
		ConcatW c[] = {
			{ wstrPath_.get(),								-1 },
			{ L"\\templates\\",								-1 },
			{ pAccount ? pAccount->getClass() : L"mail",	-1 },
			{ L"\\",										-1 },
			{ pwszName,										-1 },
			{ L".template",									-1 }
		};
		wstrPath = concat(c, countof(c));
		
		log.debugf(L"Checking template file: %s.", wstrPath.get());
	}
	
	assert(wstrPath.get());
	
	W2T(wstrPath.get(), ptszPath);
	WIN32_FIND_DATA fd;
	AutoFindHandle hFind(::FindFirstFile(ptszPath, &fd));
	if (!hFind.get()) {
		log.errorf(L"Template is not found: %s.", wstrPath.get());
		return 0;
	}
	
	ItemList::iterator it = std::find_if(listItem_.begin(), listItem_.end(),
		boost::bind(string_equal<WCHAR>(),
			boost::bind(&Item::getPath, _1), wstrPath.get()));
	if (it != listItem_.end()) {
		Item* pItem = *it;
		if (::CompareFileTime(&fd.ftLastWriteTime, &pItem->getFileTime()) == 0) {
			log.debugf(L"The template is not modified. Use cache: %s.", pwszName);
			return pItem->getTemplate();
		}
		else {
			delete pItem;
			listItem_.erase(it);
		}
	}
	
	log.debugf(L"Loading template file: %s.", wstrPath.get());
	
	FileInputStream stream(wstrPath.get());
	if (!stream) {
		log.errorf(L"Could not create a file stream: %s.", wstrPath.get());
		return 0;
	}
	BufferedInputStream bufferedStream(&stream, false);
	InputStreamReader reader(&bufferedStream, false, 0);
	if (!reader) {
		log.errorf(L"Could not create a reader: %s.", wstrPath.get());
		return 0;
	}
	
	std::auto_ptr<Template> pTemplate(TemplateParser().parse(&reader, wstrPath.get()));
	if (!pTemplate.get()) {
		log.errorf(L"Error occurred while parsing the template: %s.", pwszName);
		return 0;
	}
	
	std::auto_ptr<Item> pItem(new Item(wstrPath.get(), fd.ftLastWriteTime, pTemplate));
	listItem_.push_back(pItem.get());
	return pItem.release()->getTemplate();
}

void qm::TemplateManager::getTemplateNames(const WCHAR* pwszClass,
										   const WCHAR* pwszPrefix,
										   NameList* pList) const
{
	assert(pwszClass);
	assert(pList);
	
	NameList l;
	StringListFree<NameList> free(l);
	
	StringBuffer<WSTRING> buf;
	buf.append(wstrPath_.get());
	buf.append(L"\\templates\\");
	buf.append(pwszClass);
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
