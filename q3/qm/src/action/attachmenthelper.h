/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __ATTACHMENTHELPER_H__
#define __ATTACHMENTHELPER_H__

#include <qm.h>
#include <qmmessage.h>
#include <qmmessageholder.h>

#include <qsmime.h>
#include <qsprofile.h>


namespace qm {

class AttachmentHelper;

class TempFileCleaner;


/****************************************************************************
 *
 * AttachmentHelper
 *
 */

class AttachmentHelper
{
public:
	typedef std::vector<const WCHAR*> NameList;

public:
	AttachmentHelper(qs::Profile* pProfile,
					 TempFileCleaner* pTempFileCleaner,
					 HWND hwnd);
	~AttachmentHelper();

public:
	AttachmentParser::Result detach(const MessageHolderList& listMessageHolder,
									const NameList* pListName);
	AttachmentParser::Result open(const qs::Part* pPart,
								  const WCHAR* pwszName,
								  bool bOpenWithEditor);

private:
	AttachmentHelper(const AttachmentHelper&);
	AttachmentHelper& operator=(const AttachmentHelper&);

private:
	qs::Profile* pProfile_;
	TempFileCleaner* pTempFileCleaner_;
	HWND hwnd_;
};

}

#endif // __ATTACHMENTHELPER_H__
