/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
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
	virtual qs::QSTATUS hasAttachment(bool* pbHas) = 0;
	virtual qs::QSTATUS hasSelectedAttachment(bool* pbHas) = 0;
	virtual qs::QSTATUS getSelectedAttachment(NameList* pList) = 0;
};

}

#endif // __ATTACHMENTSELECTIONMODEL_H__
