/*
 * $Id$
 *
 * Copyright(C) 1998-2008 Satoshi Nakamura
 *
 */

#ifndef __ATTACHMENTHELPER_H__
#define __ATTACHMENTHELPER_H__

#include <qm.h>
#include <qmmessage.h>
#include <qmmessageholder.h>

#include <qsmime.h>
#include <qsprofile.h>

#include "../ui/dialogs.h"


namespace qm {

class AttachmentHelper;

class MessageEnumerator;
class MessageContext;
class MessageFrameWindowManager;
class SecurityModel;
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
	AttachmentHelper(SecurityModel* pSecurityModel,
					 qs::Profile* pProfile,
					 HWND hwnd);
	AttachmentHelper(MessageFrameWindowManager* pMessageFrameWindowManager,
					 TempFileCleaner* pTempFileCleaner,
					 qs::Profile* pProfile,
					 HWND hwnd);
	~AttachmentHelper();

public:
	AttachmentParser::Result detach(MessageEnumerator* pEnum,
									const NameList* pListName);
	AttachmentParser::Result detach(MessageContext* pContext,
									const NameList* pListName);
	AttachmentParser::Result open(const qs::Part* pPart,
								  const WCHAR* pwszName,
								  const MessagePtr& parentPtr,
								  bool bOpenWithEditor);

private:
	AttachmentParser::Result openFile(const qs::Part* pPart,
									  const WCHAR* pwszName,
									  bool bOpenWithEditor);
	bool openFolder(const WCHAR* pwszFolder);
	bool isAddZoneId() const;

private:
	static void addItems(const Message& msg,
						 MessageHolder* pmh,
						 const NameList* pListName,
						 DetachDialog::List* pList);

private:
	struct DetachDialogListFree
	{
		DetachDialogListFree(DetachDialog::List& l);
		~DetachDialogListFree();
		
		DetachDialog::List& l_;
	};

private:
	AttachmentHelper(const AttachmentHelper&);
	AttachmentHelper& operator=(const AttachmentHelper&);

private:
	SecurityModel* pSecurityModel_;
	MessageFrameWindowManager* pMessageFrameWindowManager_;
	TempFileCleaner* pTempFileCleaner_;
	qs::Profile* pProfile_;
	HWND hwnd_;
};

}

#endif // __ATTACHMENTHELPER_H__
