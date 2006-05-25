/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __ATTACHMENTSELECTIONMODEL_H__
#define __ATTACHMENTSELECTIONMODEL_H__

#include <qm.h>

#include <qs.h>
#include <qsstring.h>

#include <vector>


namespace qm {

/****************************************************************************
 *
 * AttachmentSelectionModel
 *
 */

class AttachmentSelectionModel
{
public:
	typedef std::vector<qs::WSTRING> NameList;

public:
	virtual ~AttachmentSelectionModel();

public:
	virtual bool hasAttachment() = 0;
	virtual bool hasSelectedAttachment() = 0;
	virtual void getSelectedAttachment(NameList* pList) = 0;
	virtual bool isAttachmentDeleted() = 0;
};

}

#endif // __ATTACHMENTSELECTIONMODEL_H__
