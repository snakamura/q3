/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __FOLDERCOMBOBOX_H__
#define __FOLDERCOMBOBOX_H__


namespace qm {

struct FolderComboBoxCreateContext;

class Document;
class UIManager;


/****************************************************************************
 *
 * FolderComboBoxCreateContext
 *
 */

struct FolderComboBoxCreateContext
{
	Document* pDocument_;
	UIManager* pUIManager_;
};

}

#endif // __FOLDERCOMBOBOX_H__
