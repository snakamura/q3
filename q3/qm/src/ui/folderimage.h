/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 */

#ifndef __FOLDERIMAGE_H__
#define __FOLDERIMAGE_H__

#include <qm.h>

#include <vector>

#include <commctrl.h>
#include <windows.h>


namespace qm {

class FolderImage;

class Account;
class Folder;


/****************************************************************************
 *
 * FolderImage
 *
 */

class FolderImage
{
public:
	explicit FolderImage(const WCHAR* pwszPath);
	~FolderImage();

public:
	HIMAGELIST getImageList() const;
	int getAccountImage(const Account* pAccount,
						bool bUnseen,
						bool bSelected) const;
	int getFolderImage(const Folder* pFolder,
					   bool bMessage,
					   bool bUnseen,
					   bool bSelected) const;

private:
	void loadDefaultImages(const WCHAR* pwszPath);
	void loadExtraImages(const WCHAR* pwszPath);
	int addImage(const WCHAR* pwszPath);

private:
	FolderImage(const FolderImage&);
	FolderImage& operator=(const FolderImage&);

private:
	enum {
		WIDTH	= 16,
		HEIGHT	= 16,
		INITIAL	= 64,
		GROW	= 16
	};

private:
	typedef std::vector<std::pair<qs::WSTRING, int> > ImageList;

private:
	HIMAGELIST hImageList_;
	ImageList listImage_;
};

}

#endif // __FOLDERIMAGE_H__
