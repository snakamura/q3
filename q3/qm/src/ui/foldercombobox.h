/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __FOLDERCOMBOBOX_H__
#define __FOLDERCOMBOBOX_H__

#include <qskeymap.h>

namespace qm {

class Document;


/****************************************************************************
 *
 * FolderComboBoxCreateContext
 *
 */

struct FolderComboBoxCreateContext
{
	Document* pDocument_;
	qs::MenuManager* pMenuManager_;
	qs::KeyMap* pKeyMap_;
};

}

#endif // __FOLDERCOMBOBOX_H__
