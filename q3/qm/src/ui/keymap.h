/*
 * $Id$
 *
 * Copyright(C) 1998-2003 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __KEYMAP_H__
#define __KEYMAP_H__

#include <qm.h>

#include <qskeymap.h>

#include "resourceinc.h"


namespace qm {

const qs::KeyNameToId mapKeyNameToId[] = {
	{ L"AttachmentOpen",				IDM_ATTACHMENT_OPEN,					0 },
	{ L"AttachmentSave",				IDM_ATTACHMENT_SAVE,					0 },
	{ L"AttachmentSaveAll",				IDM_ATTACHMENT_SAVEALL,					0 },
	{ L"EditClearDeleted",				IDM_EDIT_CLEARDELETED,					0 },
	{ L"EditCopy",						IDM_EDIT_COPY,							0 },
	{ L"EditCut",						IDM_EDIT_CUT,							0 },
	{ L"EditDelete",					IDM_EDIT_DELETE,						0 },
	{ L"EditDeleteCache",				IDM_EDIT_DELETECACHE,					0 },
	{ L"EditDeleteDirect",				IDM_EDIT_DELETEDIRECT,					0 },
	{ L"EditFind",						IDM_EDIT_FIND,							0 },
	{ L"EditFindNext",					IDM_EDIT_FINDNEXT,						0 },
	{ L"EditFindPrev",					IDM_EDIT_FINDPREV,						0 },
	{ L"EditMoveCharLeft",				IDM_EDIT_MOVECHARLEFT,					0 },
	{ L"EditMoveCharRight",				IDM_EDIT_MOVECHARRIGHT,					0 },
	{ L"EditMoveLineDown",				IDM_EDIT_MOVELINEDOWN,					0 },
	{ L"EditMoveLineEnd",				IDM_EDIT_MOVELINEEND,					0 },
	{ L"EditMoveLineStart",				IDM_EDIT_MOVELINESTART,					0 },
	{ L"EditMoveLineUp",				IDM_EDIT_MOVELINEUP,					0 },
	{ L"EditMovePageDown",				IDM_EDIT_MOVEPAGEDOWN,					0 },
	{ L"EditMovePageUp",				IDM_EDIT_MOVEPAGEUP,					0 },
	{ L"EditMoveDocEnd",				IDM_EDIT_MOVEDOCEND,					0 },
	{ L"EditMoveDocStart",				IDM_EDIT_MOVEDOCSTART,					0 },
	{ L"EditPaste",						IDM_EDIT_PASTE,							0 },
	{ L"EditPasteWithQuote",			IDM_EDIT_PASTEWITHQUOTE,				0 },
	{ L"EditRedo",						IDM_EDIT_REDO,							0 },
	{ L"EditReplace",					IDM_EDIT_REPLACE,						0 },
	{ L"EditSelectAll",					IDM_EDIT_SELECTALL,						0 },
	{ L"EditUndo",						IDM_EDIT_UNDO,							0 },
	{ L"FileClose",						IDM_FILE_CLOSE,							0 },
	{ L"FileDraft",						IDM_FILE_DRAFT,							0 },
	{ L"FileEmptyTrash",				IDM_FILE_EMPTYTRASH,					0 },
	{ L"FileExit",						IDM_FILE_EXIT,							0 },
	{ L"FileExport",					IDM_FILE_EXPORT,						0 },
	{ L"FileImport",					IDM_FILE_IMPORT,						0 },
	{ L"FileInsert",					IDM_FILE_INSERT,						0 },
	{ L"FileOffline",					IDM_FILE_OFFLINE,						0 },
	{ L"FileOpen",						IDM_FILE_OPEN,							0 },
	{ L"FilePrint",						IDM_FILE_PRINT,							0 },
	{ L"FileSave",						IDM_FILE_SAVE,							0 },
	{ L"FileSend",						IDM_FILE_SEND,							0 },
	{ L"FileSendNow",					IDM_FILE_SENDNOW,						0 },
	{ L"FolderCompact",					IDM_FOLDER_COMPACT,						0 },
	{ L"FolderCreate",					IDM_FOLDER_CREATE,						0 },
	{ L"FolderDelete",					IDM_FOLDER_DELETE,						0 },
	{ L"FolderEmpty",					IDM_FOLDER_EMPTY,						0 },
	{ L"FolderProperty",				IDM_FOLDER_PROPERTY,					0 },
	{ L"FolderUpdate",					IDM_FOLDER_UPDATE,						0 },
	{ L"HeaderEditItem0",				IDM_FOCUS_HEADEREDITITEM,				0 },
	{ L"HeaderEditItem1",				IDM_FOCUS_HEADEREDITITEM + 1,			0 },
	{ L"HeaderEditItem2",				IDM_FOCUS_HEADEREDITITEM + 2,			0 },
	{ L"HeaderEditItem3",				IDM_FOCUS_HEADEREDITITEM + 3,			0 },
	{ L"HeaderEditItem4",				IDM_FOCUS_HEADEREDITITEM + 4,			0 },
	{ L"HeaderEditItem5",				IDM_FOCUS_HEADEREDITITEM + 5,			0 },
	{ L"HeaderEditItem6",				IDM_FOCUS_HEADEREDITITEM + 6,			0 },
	{ L"HeaderEditItem7",				IDM_FOCUS_HEADEREDITITEM + 7,			0 },
	{ L"HeaderEditItem8",				IDM_FOCUS_HEADEREDITITEM + 8,			0 },
	{ L"HeaderEditItem9",				IDM_FOCUS_HEADEREDITITEM + 9,			0 },
	{ L"MessageApplyRule",				IDM_MESSAGE_APPLYRULE,					0 },
	{ L"MessageApplyRuleSelected",		IDM_MESSAGE_APPLYRULESELECTED,			0 },
	{ L"MessageApplyTemplate0",			IDM_MESSAGE_APPLYTEMPLATE,				0 },
	{ L"MessageApplyTemplate1",			IDM_MESSAGE_APPLYTEMPLATE + 1,			0 },
	{ L"MessageApplyTemplate2",			IDM_MESSAGE_APPLYTEMPLATE + 2,			0 },
	{ L"MessageApplyTemplate3",			IDM_MESSAGE_APPLYTEMPLATE + 3,			0 },
	{ L"MessageApplyTemplate4",			IDM_MESSAGE_APPLYTEMPLATE + 4,			0 },
	{ L"MessageApplyTemplate5",			IDM_MESSAGE_APPLYTEMPLATE + 5,			0 },
	{ L"MessageApplyTemplate6",			IDM_MESSAGE_APPLYTEMPLATE + 6,			0 },
	{ L"MessageApplyTemplate7",			IDM_MESSAGE_APPLYTEMPLATE + 7,			0 },
	{ L"MessageApplyTemplate8",			IDM_MESSAGE_APPLYTEMPLATE + 8,			0 },
	{ L"MessageApplyTemplate9",			IDM_MESSAGE_APPLYTEMPLATE + 9,			0 },
	{ L"MessageApplyTemplateExternal0",	IDM_MESSAGE_APPLYTEMPLATEEXTERNAL,		0 },
	{ L"MessageApplyTemplateExternal1",	IDM_MESSAGE_APPLYTEMPLATEEXTERNAL + 1,	0 },
	{ L"MessageApplyTemplateExternal2",	IDM_MESSAGE_APPLYTEMPLATEEXTERNAL + 2,	0 },
	{ L"MessageApplyTemplateExternal3",	IDM_MESSAGE_APPLYTEMPLATEEXTERNAL + 3,	0 },
	{ L"MessageApplyTemplateExternal4",	IDM_MESSAGE_APPLYTEMPLATEEXTERNAL + 4,	0 },
	{ L"MessageApplyTemplateExternal5",	IDM_MESSAGE_APPLYTEMPLATEEXTERNAL + 5,	0 },
	{ L"MessageApplyTemplateExternal6",	IDM_MESSAGE_APPLYTEMPLATEEXTERNAL + 6,	0 },
	{ L"MessageApplyTemplateExternal7",	IDM_MESSAGE_APPLYTEMPLATEEXTERNAL + 7,	0 },
	{ L"MessageApplyTemplateExternal8",	IDM_MESSAGE_APPLYTEMPLATEEXTERNAL + 8,	0 },
	{ L"MessageApplyTemplateExternal9",	IDM_MESSAGE_APPLYTEMPLATEEXTERNAL + 9,	0 },
	{ L"MessageCombine",				IDM_MESSAGE_COMBINE,					0 },
	{ L"MessageCreateFromClipboard",	IDM_MESSAGE_CREATEFROMCLIPBOARD,		0 },
	{ L"MessageDeleteAttachment",		IDM_MESSAGE_DELETEATTACHMENT,			0 },
	{ L"MessageDetach",					IDM_MESSAGE_DETACH,						0 },
	{ L"MessageDraftFromClipboard",		IDM_MESSAGE_DRAFTFROMCLIPBOARD,			0 },
	{ L"MessageEdit",					IDM_MESSAGE_EDIT,						0 },
	{ L"MessageEditExternal",			IDM_MESSAGE_EDITEXTERNAL,				0 },
	{ L"MessageExpandDigest",			IDM_MESSAGE_EXPANDDIGEST,				0 },
	{ L"MessageForward",				IDM_MESSAGE_FORWARD,					0 },
	{ L"MessageForwardExternal",		IDM_MESSAGE_FORWARDEXTERNAL,			0 },
	{ L"MessageMark",					IDM_MESSAGE_MARK,						0 },
	{ L"MessageMarkDownload",			IDM_MESSAGE_MARKDOWNLOAD,				0 },
	{ L"MessageMarkSeen",				IDM_MESSAGE_MARKSEEN,					0 },
	{ L"MessageMarkUnseen",				IDM_MESSAGE_MARKUNSEEN,					0 },
	{ L"MessageMoveOther",				IDM_MESSAGE_MOVEOTHER,					0 },
	{ L"MessageNew",					IDM_MESSAGE_NEW,						0 },
	{ L"MessageNewExternal",			IDM_MESSAGE_NEWEXTERNAL,				0 },
	{ L"MessageProperty",				IDM_MESSAGE_PROPERTY,					0 },
	{ L"MessageReply",					IDM_MESSAGE_REPLY,						0 },
	{ L"MessageReplyExternal",			IDM_MESSAGE_REPLYEXTERNAL,				0 },
	{ L"MessageReplyAll",				IDM_MESSAGE_REPLYALL,					0 },
	{ L"MessageReplyAllExternal",		IDM_MESSAGE_REPLYALLEXTERNAL,			0 },
	{ L"MessageSearch",					IDM_MESSAGE_SEARCH,						0 },
	{ L"MessageUnmark",					IDM_MESSAGE_UNMARK,						0 },
	{ L"ToolAccount",					IDM_TOOL_ACCOUNT,						0 },
	{ L"ToolAddressBook",				IDM_TOOL_ADDRESSBOOK,					0 },
	{ L"ToolAttachment",				IDM_TOOL_ATTACHMENT,					0 },
	{ L"ToolCancel",					IDM_TOOL_CANCEL,						0 },
	{ L"ToolCheckNewMail",				IDM_TOOL_CHECKNEWMAIL,					0 },
	{ L"ToolDialup",					IDM_TOOL_DIALUP,						0 },
	{ L"ToolEncrypt",					IDM_TOOL_ENCRYPT,						0 },
	{ L"ToolGoround0",					IDM_TOOL_GOROUND,						0 },
	{ L"ToolGoround1",					IDM_TOOL_GOROUND + 1,					0 },
	{ L"ToolGoround2",					IDM_TOOL_GOROUND + 2,					0 },
	{ L"ToolGoround3",					IDM_TOOL_GOROUND + 3,					0 },
	{ L"ToolGoround4",					IDM_TOOL_GOROUND + 4,					0 },
	{ L"ToolGoround5",					IDM_TOOL_GOROUND + 5,					0 },
	{ L"ToolGoround6",					IDM_TOOL_GOROUND + 6,					0 },
	{ L"ToolGoround7",					IDM_TOOL_GOROUND + 7,					0 },
	{ L"ToolGoround8",					IDM_TOOL_GOROUND + 8,					0 },
	{ L"ToolGoround9",					IDM_TOOL_GOROUND + 9,					0 },
	{ L"ToolHeaderEdit",				IDM_TOOL_HEADEREDIT,					0 },
	{ L"ToolInsertSignature",			IDM_TOOL_INSERTSIGNATURE,				0 },
	{ L"ToolInsertText",				IDM_TOOL_INSERTTEXT,					0 },
	{ L"ToolOptions",					IDM_TOOL_OPTIONS,						0 },
	{ L"ToolReceive",					IDM_TOOL_RECEIVE,						0 },
	{ L"ToolReform",					IDM_TOOL_REFORM,						0 },
	{ L"ToolReformAll",					IDM_TOOL_REFORMALL,						0 },
	{ L"ToolReformAuto",				IDM_TOOL_REFORMAUTO,					0 },
	{ L"ToolScript0",					IDM_TOOL_SCRIPT,						0 },
	{ L"ToolScript1",					IDM_TOOL_SCRIPT + 1,					0 },
	{ L"ToolScript2",					IDM_TOOL_SCRIPT + 2,					0 },
	{ L"ToolScript3",					IDM_TOOL_SCRIPT + 3,					0 },
	{ L"ToolScript4",					IDM_TOOL_SCRIPT + 4,					0 },
	{ L"ToolScript5",					IDM_TOOL_SCRIPT + 5,					0 },
	{ L"ToolScript6",					IDM_TOOL_SCRIPT + 6,					0 },
	{ L"ToolScript7",					IDM_TOOL_SCRIPT + 7,					0 },
	{ L"ToolScript8",					IDM_TOOL_SCRIPT + 8,					0 },
	{ L"ToolScript9",					IDM_TOOL_SCRIPT + 9,					0 },
	{ L"ToolSend",						IDM_TOOL_SEND,							0 },
	{ L"ToolSign",						IDM_TOOL_SIGN,							0 },
	{ L"ToolSubAccount0",				IDM_TOOL_SUBACCOUNT,					0 },
	{ L"ToolSubAccount1",				IDM_TOOL_SUBACCOUNT + 1,				0 },
	{ L"ToolSubAccount2",				IDM_TOOL_SUBACCOUNT + 2,				0 },
	{ L"ToolSubAccount3",				IDM_TOOL_SUBACCOUNT + 3,				0 },
	{ L"ToolSubAccount4",				IDM_TOOL_SUBACCOUNT + 4,				0 },
	{ L"ToolSubAccount5",				IDM_TOOL_SUBACCOUNT + 5,				0 },
	{ L"ToolSubAccount6",				IDM_TOOL_SUBACCOUNT + 6,				0 },
	{ L"ToolSubAccount7",				IDM_TOOL_SUBACCOUNT + 7,				0 },
	{ L"ToolSubAccount8",				IDM_TOOL_SUBACCOUNT + 8,				0 },
	{ L"ToolSubAccount9",				IDM_TOOL_SUBACCOUNT + 9,				0 },
	{ L"ToolSync",						IDM_TOOL_SYNC,							0 },
	{ L"ViewDecryptVerifyMode",			IDM_VIEW_DECRYPTVERIFYMODE,				0 },
	{ L"ViewEncoding0",					IDM_VIEW_ENCODING,						0 },
	{ L"ViewEncoding1",					IDM_VIEW_ENCODING + 1,					0 },
	{ L"ViewEncoding2",					IDM_VIEW_ENCODING + 2,					0 },
	{ L"ViewEncoding3",					IDM_VIEW_ENCODING + 3,					0 },
	{ L"ViewEncoding4",					IDM_VIEW_ENCODING + 4,					0 },
	{ L"ViewEncoding5",					IDM_VIEW_ENCODING + 5,					0 },
	{ L"ViewEncoding6",					IDM_VIEW_ENCODING + 6,					0 },
	{ L"ViewEncoding7",					IDM_VIEW_ENCODING + 7,					0 },
	{ L"ViewEncoding8",					IDM_VIEW_ENCODING + 8,					0 },
	{ L"ViewEncoding9",					IDM_VIEW_ENCODING + 9,					0 },
	{ L"ViewEncodingAutoDetect",		IDM_VIEW_ENCODINGAUTODETECT,			0 },
	{ L"ViewFilter0",					IDM_VIEW_FILTER,						0 },
	{ L"ViewFilter1",					IDM_VIEW_FILTER + 1,					0 },
	{ L"ViewFilter2",					IDM_VIEW_FILTER + 2,					0 },
	{ L"ViewFilter3",					IDM_VIEW_FILTER + 3,					0 },
	{ L"ViewFilter4",					IDM_VIEW_FILTER + 4,					0 },
	{ L"ViewFilter5",					IDM_VIEW_FILTER + 5,					0 },
	{ L"ViewFilter6",					IDM_VIEW_FILTER + 6,					0 },
	{ L"ViewFilter7",					IDM_VIEW_FILTER + 7,					0 },
	{ L"ViewFilter8",					IDM_VIEW_FILTER + 8,					0 },
	{ L"ViewFilter9",					IDM_VIEW_FILTER + 9,					0 },
	{ L"ViewFilterCustom",				IDM_VIEW_FILTERCUSTOM,					0 },
	{ L"ViewFilterNone",				IDM_VIEW_FILTERNONE,					0 },
	{ L"ViewFocusNext",					IDM_VIEW_FOCUSNEXT,						0 },
	{ L"ViewFocusPrev",					IDM_VIEW_FOCUSPREV,						0 },
	{ L"ViewHtmlMode",					IDM_VIEW_HTMLMODE,						0 },
	{ L"ViewLockPreview",				IDM_VIEW_LOCKPREVIEW,					0 },
	{ L"ViewNextAccount",				IDM_VIEW_NEXTACCOUNT,					0 },
	{ L"ViewNextFolder",				IDM_VIEW_NEXTFOLDER,					0 },
	{ L"ViewNextMessage",				IDM_VIEW_NEXTMESSAGE,					0 },
	{ L"ViewNextMessagePage",			IDM_VIEW_NEXTMESSAGEPAGE,				0 },
	{ L"ViewNextUnseenMessage",			IDM_VIEW_NEXTUNSEENMESSAGE,				0 },
	{ L"ViewOpenLink",					IDM_VIEW_OPENLINK,						0 },
	{ L"ViewPrevAccount",				IDM_VIEW_PREVACCOUNT,					0 },
	{ L"ViewPrevFolder",				IDM_VIEW_PREVFOLDER,					0 },
	{ L"ViewPrevMessage",				IDM_VIEW_PREVMESSAGE,					0 },
	{ L"ViewPrevMessagePage",			IDM_VIEW_PREVMESSAGEPAGE,				0 },
	{ L"ViewRawMode",					IDM_VIEW_RAWMODE,						0 },
	{ L"ViewRefresh",					IDM_VIEW_REFRESH,						0 },
	{ L"ViewScrollBottom",				IDM_VIEW_SCROLLBOTTOM,					0 },
	{ L"ViewScrollLineDown",			IDM_VIEW_SCROLLLINEDOWN,				0 },
	{ L"ViewScrollLineUp",				IDM_VIEW_SCROLLLINEUP,					0 },
	{ L"ViewScrollPageDown",			IDM_VIEW_SCROLLPAGEDOWN,				0 },
	{ L"ViewScrollPageUp",				IDM_VIEW_SCROLLPAGEUP,					0 },
	{ L"ViewScrollTop",					IDM_VIEW_SCROLLTOP,						0 },
	{ L"ViewSelectMessage",				IDM_VIEW_SELECTMESSAGE,					0 },
	{ L"ViewSelectMode",				IDM_VIEW_SELECTMODE,					0 },
	{ L"ViewShowFolder",				IDM_VIEW_SHOWFOLDER,					0 },
	{ L"ViewShowHeader",				IDM_VIEW_SHOWHEADER,					0 },
	{ L"ViewShowHeaderColumn",			IDM_VIEW_SHOWHEADERCOLUMN,				0 },
	{ L"ViewShowPreview",				IDM_VIEW_SHOWPREVIEW,					0 },
	{ L"ViewShowStatusBar",				IDM_VIEW_SHOWSTATUSBAR,					0 },
	{ L"ViewShowSyncDialog",			IDM_VIEW_SHOWSYNCDIALOG,				0 },
	{ L"ViewShowToolbar",				IDM_VIEW_SHOWTOOLBAR,					0 },
	{ L"ViewSort0",						IDM_VIEW_SORT,							0 },
	{ L"ViewSort1",						IDM_VIEW_SORT + 1,						0 },
	{ L"ViewSort2",						IDM_VIEW_SORT + 2,						0 },
	{ L"ViewSort3",						IDM_VIEW_SORT + 3,						0 },
	{ L"ViewSort4",						IDM_VIEW_SORT + 4,						0 },
	{ L"ViewSort5",						IDM_VIEW_SORT + 5,						0 },
	{ L"ViewSort6",						IDM_VIEW_SORT + 6,						0 },
	{ L"ViewSort7",						IDM_VIEW_SORT + 7,						0 },
	{ L"ViewSort8",						IDM_VIEW_SORT + 8,						0 },
	{ L"ViewSort9",						IDM_VIEW_SORT + 9,						0 },
	{ L"ViewSortAscending",				IDM_VIEW_SORTASCENDING,					0 },
	{ L"ViewSortDescending",			IDM_VIEW_SORTDESCENDING,				0 },
	{ L"ViewSortThread",				IDM_VIEW_SORTTHREAD,					0 },
	{ L"ViewTemplate0",					IDM_VIEW_TEMPLATE,						0 },
	{ L"ViewTemplate1",					IDM_VIEW_TEMPLATE + 1,					0 },
	{ L"ViewTemplate2",					IDM_VIEW_TEMPLATE + 2,					0 },
	{ L"ViewTemplate3",					IDM_VIEW_TEMPLATE + 3,					0 },
	{ L"ViewTemplate4",					IDM_VIEW_TEMPLATE + 4,					0 },
	{ L"ViewTemplate5",					IDM_VIEW_TEMPLATE + 5,					0 },
	{ L"ViewTemplate6",					IDM_VIEW_TEMPLATE + 6,					0 },
	{ L"ViewTemplate7",					IDM_VIEW_TEMPLATE + 7,					0 },
	{ L"ViewTemplate8",					IDM_VIEW_TEMPLATE + 8,					0 },
	{ L"ViewTemplate9",					IDM_VIEW_TEMPLATE + 9,					0 },
	{ L"ViewTemplateNone",				IDM_VIEW_TEMPLATENONE,					0 },
};

}

#endif // __KEYMAP_H__
