/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#include <qmaction.h>

#include <qsstl.h>

#include <algorithm>

#include "action.h"
#include "../ui/resourceinc.h"

using namespace qm;
using namespace qs;


namespace qm {

struct ActionNameMap
{
	const WCHAR* pwszName_;
	unsigned int nId_;
} actionNameMap[] = {
	{ L"AttachmentOpen",				IDM_ATTACHMENT_OPEN						},
	{ L"AttachmentSave",				IDM_ATTACHMENT_SAVE						},
	{ L"AttachmentSaveAll",				IDM_ATTACHMENT_SAVEALL					},
	{ L"EditClearDeleted",				IDM_EDIT_CLEARDELETED					},
	{ L"EditCopy",						IDM_EDIT_COPY							},
	{ L"EditCut",						IDM_EDIT_CUT							},
	{ L"EditDelete",					IDM_EDIT_DELETE							},
	{ L"EditDeleteCache",				IDM_EDIT_DELETECACHE					},
	{ L"EditDeleteDirect",				IDM_EDIT_DELETEDIRECT					},
	{ L"EditFind",						IDM_EDIT_FIND							},
	{ L"EditFindNext",					IDM_EDIT_FINDNEXT						},
	{ L"EditFindPrev",					IDM_EDIT_FINDPREV						},
	{ L"EditMoveCharLeft",				IDM_EDIT_MOVECHARLEFT					},
	{ L"EditMoveCharRight",				IDM_EDIT_MOVECHARRIGHT					},
	{ L"EditMoveLineDown",				IDM_EDIT_MOVELINEDOWN					},
	{ L"EditMoveLineEnd",				IDM_EDIT_MOVELINEEND					},
	{ L"EditMoveLineStart",				IDM_EDIT_MOVELINESTART					},
	{ L"EditMoveLineUp",				IDM_EDIT_MOVELINEUP						},
	{ L"EditMovePageDown",				IDM_EDIT_MOVEPAGEDOWN					},
	{ L"EditMovePageUp",				IDM_EDIT_MOVEPAGEUP						},
	{ L"EditMoveDocEnd",				IDM_EDIT_MOVEDOCEND						},
	{ L"EditMoveDocStart",				IDM_EDIT_MOVEDOCSTART					},
	{ L"EditPaste",						IDM_EDIT_PASTE							},
	{ L"EditPasteWithQuote",			IDM_EDIT_PASTEWITHQUOTE					},
	{ L"EditRedo",						IDM_EDIT_REDO							},
	{ L"EditReplace",					IDM_EDIT_REPLACE						},
	{ L"EditSelectAll",					IDM_EDIT_SELECTALL						},
	{ L"EditUndo",						IDM_EDIT_UNDO							},
	{ L"FileClose",						IDM_FILE_CLOSE							},
	{ L"FileDraft",						IDM_FILE_DRAFT							},
	{ L"FileEmptyTrash",				IDM_FILE_EMPTYTRASH						},
	{ L"FileExit",						IDM_FILE_EXIT							},
	{ L"FileExport",					IDM_FILE_EXPORT							},
	{ L"FileImport",					IDM_FILE_IMPORT							},
	{ L"FileInsert",					IDM_FILE_INSERT							},
	{ L"FileOffline",					IDM_FILE_OFFLINE						},
	{ L"FileOpen",						IDM_FILE_OPEN							},
	{ L"FilePrint",						IDM_FILE_PRINT							},
	{ L"FileSave",						IDM_FILE_SAVE							},
	{ L"FileSend",						IDM_FILE_SEND							},
	{ L"FileSendNow",					IDM_FILE_SENDNOW						},
	{ L"FolderCompact",					IDM_FOLDER_COMPACT						},
	{ L"FolderCreate",					IDM_FOLDER_CREATE						},
	{ L"FolderDelete",					IDM_FOLDER_DELETE						},
	{ L"FolderEmpty",					IDM_FOLDER_EMPTY						},
	{ L"FolderProperty",				IDM_FOLDER_PROPERTY						},
	{ L"FolderUpdate",					IDM_FOLDER_UPDATE						},
	{ L"HeaderEditItem0",				IDM_FOCUS_HEADEREDITITEM				},
	{ L"HeaderEditItem1",				IDM_FOCUS_HEADEREDITITEM + 1			},
	{ L"HeaderEditItem2",				IDM_FOCUS_HEADEREDITITEM + 2			},
	{ L"HeaderEditItem3",				IDM_FOCUS_HEADEREDITITEM + 3			},
	{ L"HeaderEditItem4",				IDM_FOCUS_HEADEREDITITEM + 4			},
	{ L"HeaderEditItem5",				IDM_FOCUS_HEADEREDITITEM + 5			},
	{ L"HeaderEditItem6",				IDM_FOCUS_HEADEREDITITEM + 6			},
	{ L"HeaderEditItem7",				IDM_FOCUS_HEADEREDITITEM + 7			},
	{ L"HeaderEditItem8",				IDM_FOCUS_HEADEREDITITEM + 8			},
	{ L"HeaderEditItem9",				IDM_FOCUS_HEADEREDITITEM + 9			},
	{ L"MessageApplyRule",				IDM_MESSAGE_APPLYRULE					},
	{ L"MessageApplyRuleSelected",		IDM_MESSAGE_APPLYRULESELECTED			},
	{ L"MessageApplyTemplate0",			IDM_MESSAGE_APPLYTEMPLATE				},
	{ L"MessageApplyTemplate1",			IDM_MESSAGE_APPLYTEMPLATE + 1			},
	{ L"MessageApplyTemplate2",			IDM_MESSAGE_APPLYTEMPLATE + 2			},
	{ L"MessageApplyTemplate3",			IDM_MESSAGE_APPLYTEMPLATE + 3			},
	{ L"MessageApplyTemplate4",			IDM_MESSAGE_APPLYTEMPLATE + 4			},
	{ L"MessageApplyTemplate5",			IDM_MESSAGE_APPLYTEMPLATE + 5			},
	{ L"MessageApplyTemplate6",			IDM_MESSAGE_APPLYTEMPLATE + 6			},
	{ L"MessageApplyTemplate7",			IDM_MESSAGE_APPLYTEMPLATE + 7			},
	{ L"MessageApplyTemplate8",			IDM_MESSAGE_APPLYTEMPLATE + 8			},
	{ L"MessageApplyTemplate9",			IDM_MESSAGE_APPLYTEMPLATE + 9	 		},
	{ L"MessageApplyTemplateExternal0",	IDM_MESSAGE_APPLYTEMPLATEEXTERNAL	 	},
	{ L"MessageApplyTemplateExternal1",	IDM_MESSAGE_APPLYTEMPLATEEXTERNAL + 1	},
	{ L"MessageApplyTemplateExternal2",	IDM_MESSAGE_APPLYTEMPLATEEXTERNAL + 2	},
	{ L"MessageApplyTemplateExternal3",	IDM_MESSAGE_APPLYTEMPLATEEXTERNAL + 3	},
	{ L"MessageApplyTemplateExternal4",	IDM_MESSAGE_APPLYTEMPLATEEXTERNAL + 4	},
	{ L"MessageApplyTemplateExternal5",	IDM_MESSAGE_APPLYTEMPLATEEXTERNAL + 5	},
	{ L"MessageApplyTemplateExternal6",	IDM_MESSAGE_APPLYTEMPLATEEXTERNAL + 6	},
	{ L"MessageApplyTemplateExternal7",	IDM_MESSAGE_APPLYTEMPLATEEXTERNAL + 7	},
	{ L"MessageApplyTemplateExternal8",	IDM_MESSAGE_APPLYTEMPLATEEXTERNAL + 8	},
	{ L"MessageApplyTemplateExternal9",	IDM_MESSAGE_APPLYTEMPLATEEXTERNAL + 9	},
	{ L"MessageCombine",				IDM_MESSAGE_COMBINE						},
	{ L"MessageCreateFromClipboard",	IDM_MESSAGE_CREATEFROMCLIPBOARD			},
	{ L"MessageDeleteAttachment",		IDM_MESSAGE_DELETEATTACHMENT			},
	{ L"MessageDetach",					IDM_MESSAGE_DETACH						},
	{ L"MessageDraftFromClipboard",		IDM_MESSAGE_DRAFTFROMCLIPBOARD			},
	{ L"MessageEdit",					IDM_MESSAGE_EDIT						},
	{ L"MessageEditExternal",			IDM_MESSAGE_EDITEXTERNAL				},
	{ L"MessageExpandDigest",			IDM_MESSAGE_EXPANDDIGEST				},
	{ L"MessageForward",				IDM_MESSAGE_FORWARD						},
	{ L"MessageForwardExternal",		IDM_MESSAGE_FORWARDEXTERNAL				},
	{ L"MessageMark",					IDM_MESSAGE_MARK						},
	{ L"MessageMarkDownload",			IDM_MESSAGE_MARKDOWNLOAD				},
	{ L"MessageMarkSeen",				IDM_MESSAGE_MARKSEEN					},
	{ L"MessageMarkUnseen",				IDM_MESSAGE_MARKUNSEEN					},
	{ L"MessageMoveOther",				IDM_MESSAGE_MOVEOTHER					},
	{ L"MessageNew",					IDM_MESSAGE_NEW							},
	{ L"MessageNewExternal",			IDM_MESSAGE_NEWEXTERNAL					},
	{ L"MessageProperty",				IDM_MESSAGE_PROPERTY					},
	{ L"MessageReply",					IDM_MESSAGE_REPLY						},
	{ L"MessageReplyExternal",			IDM_MESSAGE_REPLYEXTERNAL				},
	{ L"MessageReplyAll",				IDM_MESSAGE_REPLYALL					},
	{ L"MessageReplyAllExternal",		IDM_MESSAGE_REPLYALLEXTERNAL			},
	{ L"MessageSearch",					IDM_MESSAGE_SEARCH						},
	{ L"MessageUnmark",					IDM_MESSAGE_UNMARK						},
	{ L"ToolAccount",					IDM_TOOL_ACCOUNT						},
	{ L"ToolAddressBook",				IDM_TOOL_ADDRESSBOOK					},
	{ L"ToolAttachment",				IDM_TOOL_ATTACHMENT						},
	{ L"ToolCancel",					IDM_TOOL_CANCEL							},
	{ L"ToolDialup",					IDM_TOOL_DIALUP							},
	{ L"ToolEncrypt",					IDM_TOOL_ENCRYPT						},
	{ L"ToolGoround0",					IDM_TOOL_GOROUND						},
	{ L"ToolGoround1",					IDM_TOOL_GOROUND + 1					},
	{ L"ToolGoround2",					IDM_TOOL_GOROUND + 2					},
	{ L"ToolGoround3",					IDM_TOOL_GOROUND + 3					},
	{ L"ToolGoround4",					IDM_TOOL_GOROUND + 4					},
	{ L"ToolGoround5",					IDM_TOOL_GOROUND + 5					},
	{ L"ToolGoround6",					IDM_TOOL_GOROUND + 6					},
	{ L"ToolGoround7",					IDM_TOOL_GOROUND + 7					},
	{ L"ToolGoround8",					IDM_TOOL_GOROUND + 8					},
	{ L"ToolGoround9",					IDM_TOOL_GOROUND + 9					},
	{ L"ToolHeaderEdit",				IDM_TOOL_HEADEREDIT						},
	{ L"ToolInsertSignature",			IDM_TOOL_INSERTSIGNATURE				},
	{ L"ToolInsertText",				IDM_TOOL_INSERTTEXT						},
	{ L"ToolOptions",					IDM_TOOL_OPTIONS						},
	{ L"ToolReceive",					IDM_TOOL_RECEIVE						},
	{ L"ToolReform",					IDM_TOOL_REFORM							},
	{ L"ToolReformAll",					IDM_TOOL_REFORMALL						},
	{ L"ToolReformAuto",				IDM_TOOL_REFORMAUTO						},
	{ L"ToolScript0",					IDM_TOOL_SCRIPT							},
	{ L"ToolScript1",					IDM_TOOL_SCRIPT + 1						},
	{ L"ToolScript2",					IDM_TOOL_SCRIPT + 2						},
	{ L"ToolScript3",					IDM_TOOL_SCRIPT + 3						},
	{ L"ToolScript4",					IDM_TOOL_SCRIPT + 4						},
	{ L"ToolScript5",					IDM_TOOL_SCRIPT + 5						},
	{ L"ToolScript6",					IDM_TOOL_SCRIPT + 6						},
	{ L"ToolScript7",					IDM_TOOL_SCRIPT + 7						},
	{ L"ToolScript8",					IDM_TOOL_SCRIPT + 8						},
	{ L"ToolScript9",					IDM_TOOL_SCRIPT + 9						},
	{ L"ToolSend",						IDM_TOOL_SEND							},
	{ L"ToolSign",						IDM_TOOL_SIGN							},
	{ L"ToolSubAccount0",				IDM_TOOL_SUBACCOUNT						},
	{ L"ToolSubAccount1",				IDM_TOOL_SUBACCOUNT + 1					},
	{ L"ToolSubAccount2",				IDM_TOOL_SUBACCOUNT + 2					},
	{ L"ToolSubAccount3",				IDM_TOOL_SUBACCOUNT + 3					},
	{ L"ToolSubAccount4",				IDM_TOOL_SUBACCOUNT + 4					},
	{ L"ToolSubAccount5",				IDM_TOOL_SUBACCOUNT + 5					},
	{ L"ToolSubAccount6",				IDM_TOOL_SUBACCOUNT + 6					},
	{ L"ToolSubAccount7",				IDM_TOOL_SUBACCOUNT + 7					},
	{ L"ToolSubAccount8",				IDM_TOOL_SUBACCOUNT + 8					},
	{ L"ToolSubAccount9",				IDM_TOOL_SUBACCOUNT + 9					},
	{ L"ToolSync",						IDM_TOOL_SYNC							},
	{ L"ViewDecryptVerifyMode",			IDM_VIEW_DECRYPTVERIFYMODE				},
	{ L"ViewEncoding0",					IDM_VIEW_ENCODING						},
	{ L"ViewEncoding1",					IDM_VIEW_ENCODING + 1					},
	{ L"ViewEncoding2",					IDM_VIEW_ENCODING + 2					},
	{ L"ViewEncoding3",					IDM_VIEW_ENCODING + 3					},
	{ L"ViewEncoding4",					IDM_VIEW_ENCODING + 4					},
	{ L"ViewEncoding5",					IDM_VIEW_ENCODING + 5					},
	{ L"ViewEncoding6",					IDM_VIEW_ENCODING + 6					},
	{ L"ViewEncoding7",					IDM_VIEW_ENCODING + 7					},
	{ L"ViewEncoding8",					IDM_VIEW_ENCODING + 8					},
	{ L"ViewEncoding9",					IDM_VIEW_ENCODING + 9					},
	{ L"ViewEncodingAutoDetect",		IDM_VIEW_ENCODINGAUTODETECT				},
	{ L"ViewFilter0",					IDM_VIEW_FILTER							},
	{ L"ViewFilter1",					IDM_VIEW_FILTER + 1						},
	{ L"ViewFilter2",					IDM_VIEW_FILTER + 2						},
	{ L"ViewFilter3",					IDM_VIEW_FILTER + 3						},
	{ L"ViewFilter4",					IDM_VIEW_FILTER + 4						},
	{ L"ViewFilter5",					IDM_VIEW_FILTER + 5						},
	{ L"ViewFilter6",					IDM_VIEW_FILTER + 6						},
	{ L"ViewFilter7",					IDM_VIEW_FILTER + 7						},
	{ L"ViewFilter8",					IDM_VIEW_FILTER + 8						},
	{ L"ViewFilter9",					IDM_VIEW_FILTER + 9						},
	{ L"ViewFilterCustom",				IDM_VIEW_FILTERCUSTOM					},
	{ L"ViewFilterNone",				IDM_VIEW_FILTERNONE						},
	{ L"ViewFocusNext",					IDM_VIEW_FOCUSNEXT						},
	{ L"ViewFocusPrev",					IDM_VIEW_FOCUSPREV						},
	{ L"ViewHtmlMode",					IDM_VIEW_HTMLMODE						},
	{ L"ViewLockPreview",				IDM_VIEW_LOCKPREVIEW					},
	{ L"ViewNextAccount",				IDM_VIEW_NEXTACCOUNT					},
	{ L"ViewNextFolder",				IDM_VIEW_NEXTFOLDER						},
	{ L"ViewNextMessage",				IDM_VIEW_NEXTMESSAGE					},
	{ L"ViewNextMessagePage",			IDM_VIEW_NEXTMESSAGEPAGE				},
	{ L"ViewNextUnseenMessage",			IDM_VIEW_NEXTUNSEENMESSAGE				},
	{ L"ViewOpenLink",					IDM_VIEW_OPENLINK						},
	{ L"ViewPrevAccount",				IDM_VIEW_PREVACCOUNT					},
	{ L"ViewPrevFolder",				IDM_VIEW_PREVFOLDER						},
	{ L"ViewPrevMessage",				IDM_VIEW_PREVMESSAGE					},
	{ L"ViewPrevMessagePage",			IDM_VIEW_PREVMESSAGEPAGE				},
	{ L"ViewRawMode",					IDM_VIEW_RAWMODE						},
	{ L"ViewRefresh",					IDM_VIEW_REFRESH						},
	{ L"ViewScrollBottom",				IDM_VIEW_SCROLLBOTTOM					},
	{ L"ViewScrollLineDown",			IDM_VIEW_SCROLLLINEDOWN					},
	{ L"ViewScrollLineUp",				IDM_VIEW_SCROLLLINEUP					},
	{ L"ViewScrollPageDown",			IDM_VIEW_SCROLLPAGEDOWN					},
	{ L"ViewScrollPageUp",				IDM_VIEW_SCROLLPAGEUP					},
	{ L"ViewScrollTop",					IDM_VIEW_SCROLLTOP						},
	{ L"ViewSelectMessage",				IDM_VIEW_SELECTMESSAGE					},
	{ L"ViewSelectMode",				IDM_VIEW_SELECTMODE						},
	{ L"ViewShowFolder",				IDM_VIEW_SHOWFOLDER						},
	{ L"ViewShowHeader",				IDM_VIEW_SHOWHEADER						},
	{ L"ViewShowHeaderColumn",			IDM_VIEW_SHOWHEADERCOLUMN				},
	{ L"ViewShowPreview",				IDM_VIEW_SHOWPREVIEW					},
	{ L"ViewShowStatusBar",				IDM_VIEW_SHOWSTATUSBAR					},
	{ L"ViewShowSyncDialog",			IDM_VIEW_SHOWSYNCDIALOG					},
	{ L"ViewShowToolbar",				IDM_VIEW_SHOWTOOLBAR					},
	{ L"ViewSort0",						IDM_VIEW_SORT							},
	{ L"ViewSort1",						IDM_VIEW_SORT + 1						},
	{ L"ViewSort2",						IDM_VIEW_SORT + 2						},
	{ L"ViewSort3",						IDM_VIEW_SORT + 3						},
	{ L"ViewSort4",						IDM_VIEW_SORT + 4						},
	{ L"ViewSort5",						IDM_VIEW_SORT + 5						},
	{ L"ViewSort6",						IDM_VIEW_SORT + 6						},
	{ L"ViewSort7",						IDM_VIEW_SORT + 7						},
	{ L"ViewSort8",						IDM_VIEW_SORT + 8						},
	{ L"ViewSort9",						IDM_VIEW_SORT + 9						},
	{ L"ViewSortAscending",				IDM_VIEW_SORTASCENDING					},
	{ L"ViewSortDescending",			IDM_VIEW_SORTDESCENDING					},
	{ L"ViewSortThread",				IDM_VIEW_SORTTHREAD						},
	{ L"ViewTemplate0",					IDM_VIEW_TEMPLATE						},
	{ L"ViewTemplate1",					IDM_VIEW_TEMPLATE + 1					},
	{ L"ViewTemplate2",					IDM_VIEW_TEMPLATE + 2					},
	{ L"ViewTemplate3",					IDM_VIEW_TEMPLATE + 3					},
	{ L"ViewTemplate4",					IDM_VIEW_TEMPLATE + 4					},
	{ L"ViewTemplate5",					IDM_VIEW_TEMPLATE + 5					},
	{ L"ViewTemplate6",					IDM_VIEW_TEMPLATE + 6					},
	{ L"ViewTemplate7",					IDM_VIEW_TEMPLATE + 7					},
	{ L"ViewTemplate8",					IDM_VIEW_TEMPLATE + 8					},
	{ L"ViewTemplate9",					IDM_VIEW_TEMPLATE + 9					},
	{ L"ViewTemplateNone",				IDM_VIEW_TEMPLATENONE					},
};

}

/****************************************************************************
 *
 * ActionInvoker
 *
 */

qm::ActionInvoker::ActionInvoker(const ActionMap* pActionMap, QSTATUS* pstatus) :
	pActionMap_(pActionMap)
{
}

qm::ActionInvoker::~ActionInvoker()
{
}

QSTATUS qm::ActionInvoker::invoke(const WCHAR* pwszAction,
	VARIANT** ppvarArgs, size_t nArgs) const
{
	assert(pwszAction);
	
	DECLARE_QSTATUS();
	
	ActionNameMap map = {
		pwszAction,
		0
	};
	
	const ActionNameMap* pMap = std::lower_bound(
		actionNameMap, actionNameMap + countof(actionNameMap), map,
		binary_compose_f_gx_hy(
			string_less<WCHAR>(),
			mem_data_ref(&ActionNameMap::pwszName_),
			mem_data_ref(&ActionNameMap::pwszName_)));
	if (pMap != actionNameMap + countof(actionNameMap) &&
		wcscmp(pMap->pwszName_, pwszAction) == 0) {
		Action* pAction = pActionMap_->getAction(pMap->nId_);
		ActionParam param = { ppvarArgs, nArgs };
		ActionEvent event(pMap->nId_, 0, &param);
		bool bEnabled = false;
		status = pAction->isEnabled(event, &bEnabled);
		CHECK_QSTATUS();
		if (bEnabled) {
			status = pAction->invoke(event);
			CHECK_QSTATUS();
		}
	}
	
	return QSTATUS_SUCCESS;
}
