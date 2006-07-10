/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 */

#include <qmfilenames.h>
#include <qmaccount.h>
#include <qmfolder.h>

#include <qsconv.h>
#include <qsdevicecontext.h>
#include <qsosutil.h>
#include <qsstring.h>

#include "folderimage.h"

using namespace qm;
using namespace qs;


/****************************************************************************
 *
 * FolderImage
 *
 */

qm::FolderImage::FolderImage(const WCHAR* pwszPath) :
	hImageList_(0)
{
#if !defined _WIN32_WCE || _WIN32_WCE >= 0x500
	UINT nFlags = ILC_COLOR32 | ILC_MASK;
#else
	UINT nFlags = ILC_COLOR | ILC_MASK;
#endif
	hImageList_ = ImageList_Create(WIDTH, HEIGHT, nFlags, INITIAL, GROW);
	
	loadDefaultImages(pwszPath);
	loadExtraImages(pwszPath);
}

qm::FolderImage::~FolderImage()
{
	std::for_each(listImage_.begin(), listImage_.end(),
		unary_compose_f_gx(
			string_free<WSTRING>(),
			std::select1st<ImageList::value_type>()));
	ImageList_Destroy(hImageList_);
}

HIMAGELIST qm::FolderImage::getImageList() const
{
	return hImageList_;
}

int qm::FolderImage::getAccountImage(const Account* pAccount,
									 bool bUnseen,
									 bool bSelected) const
{
	int nImage = 0;
	
	ImageList::const_iterator it = std::find_if(
		listImage_.begin(), listImage_.end(),
		std::bind2nd(
			binary_compose_f_gx_hy(
				string_equal<WCHAR>(),
				std::select1st<ImageList::value_type>(),
				std::identity<const WCHAR*>()),
			pAccount->getType(Account::HOST_RECEIVE)));
	if (it == listImage_.end())
		it = std::find_if(listImage_.begin(), listImage_.end(),
			std::bind2nd(
				binary_compose_f_gx_hy(
					string_equal<WCHAR>(),
					std::select1st<ImageList::value_type>(),
					std::identity<const WCHAR*>()),
				pAccount->getClass()));
	if (it != listImage_.end())
		nImage = (*it).second;
	
	if (bUnseen)
		nImage += 1;
	
	return nImage;
}

int qm::FolderImage::getFolderImage(const Folder* pFolder,
									bool bMessage,
									bool bUnseen,
									bool bSelected) const
{
	int nImage = 0;
	
	unsigned int nFlags = pFolder->getFlags();
	switch (pFolder->getType()) {
	case Folder::TYPE_NORMAL:
		if (nFlags & Folder::FLAG_INBOX)
			nImage = 2;
		else if (nFlags & Folder::FLAG_OUTBOX)
			nImage = 5;
		else if (nFlags & Folder::FLAG_DRAFTBOX)
			nImage = 8;
		else if (nFlags & Folder::FLAG_SENTBOX)
			nImage = 11;
		else if (nFlags & Folder::FLAG_TRASHBOX)
			nImage = 14;
		else if (nFlags & Folder::FLAG_JUNKBOX)
			nImage = 17;
		else if (nFlags & Folder::FLAG_NOSELECT)
			nImage = bSelected ? 44 : 41;
		else if ((nFlags & Folder::FLAG_LOCAL) && (nFlags & Folder::FLAG_SYNCABLE))
			nImage = bSelected ? 32 : 29;
		else if (nFlags & Folder::FLAG_LOCAL)
			nImage = bSelected ? 26 : 23;
		else
			nImage = bSelected ? 38 : 35;
		break;
	case Folder::TYPE_QUERY:
		if (nFlags & Folder::FLAG_SEARCHBOX)
			nImage = 20;
		else
			nImage = bSelected ? 50 : 47;
		break;
	default:
		assert(false);
		break;
	}
	
	if (bUnseen)
		nImage += 2;
	else if (bMessage)
		nImage += 1;
	
	return nImage;
}

void qm::FolderImage::loadDefaultImages(const WCHAR* pwszPath)
{
	wstring_ptr wstrPath(concat(pwszPath, L"\\", FileNames::FOLDER_BMP));
	addImage(wstrPath.get());
}

void qm::FolderImage::loadExtraImages(const WCHAR* pwszPath)
{
	wstring_ptr wstrFind(concat(pwszPath, L"\\account_*.bmp"));
	W2T(wstrFind.get(), ptszFind);
	WIN32_FIND_DATA fd;
	AutoFindHandle hFind(::FindFirstFile(ptszFind, &fd));
	if (!hFind.get())
		return;
	do {
		T2W(fd.cFileName, pwszName);
		wstring_ptr wstrPath(concat(pwszPath, L"\\", pwszName));
		int nIndex = addImage(wstrPath.get());
		if (nIndex != -1) {
			wstring_ptr wstrKey(allocWString(pwszName + 8, wcslen(pwszName) - 12));
			wstrKey = tolower(wstrKey.get());
			listImage_.push_back(std::make_pair(wstrKey.get(), nIndex));
			wstrKey.release();
		}
	} while (::FindNextFile(hFind.get(), &fd));
}

int qm::FolderImage::addImage(const WCHAR* pwszPath)
{
	W2T(pwszPath, ptszPath);
#ifdef _WIN32_WCE
	GdiObject<HBITMAP> hBitmap(::SHLoadDIBitmap(ptszPath));
#else
	GdiObject<HBITMAP> hBitmap(reinterpret_cast<HBITMAP>(
		::LoadImage(0, ptszPath, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE)));
#endif
	return ImageList_AddMasked(hImageList_, hBitmap.get(), CLR_DEFAULT);
}
