/*
 * $Id$
 *
 * Copyright(C) 1998-2007 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __ACTIONITEM_H__
#define __ACTIONITEM_H__

#include <qsaction.h>

#include "actionid.h"


namespace qm {

const qs::ActionItem actionItems[] = {
	{ L"AddressCreateMessage",			IDM_ADDRESS_CREATEMESSAGE,			MAX_ADDRESS_CREATEMESSAGE,	0 },
	{ L"AddressDelete",					IDM_ADDRESS_DELETE,					1,							0 },
	{ L"AddressEdit",					IDM_ADDRESS_EDIT,					1,							0 },
	{ L"AddressNew",					IDM_ADDRESS_NEW,					1,							0 },
	{ L"AttachmentEditAdd",				IDM_ATTACHMENTEDIT_ADD,				1,							0 },
	{ L"AttachmentEditDelete",			IDM_ATTACHMENTEDIT_DELETE,			1,							0 },
	{ L"AttachmentOpen",				IDM_ATTACHMENT_OPEN,				1,							0 },
	{ L"AttachmentSave",				IDM_ATTACHMENT_SAVE,				1,							0 },
	{ L"AttachmentSaveAll",				IDM_ATTACHMENT_SAVEALL,				1,							0 },
	{ L"ConfigAutoPilot",				IDM_CONFIG_AUTOPILOT,				1,							0 },
	{ L"ConfigColors",					IDM_CONFIG_COLORS,					1,							0 },
	{ L"ConfigFilters",					IDM_CONFIG_FILTERS,					1,							0 },
	{ L"ConfigGoRound",					IDM_CONFIG_GOROUND,					1,							0 },
	{ L"ConfigRules",					IDM_CONFIG_RULES,					1,							0 },
	{ L"ConfigSignatures",				IDM_CONFIG_SIGNATURES,				1,							0 },
	{ L"ConfigSyncFilters",				IDM_CONFIG_SYNCFILTERS,				1,							0 },
	{ L"ConfigTexts",					IDM_CONFIG_TEXTS,					1,							0 },
	{ L"ConfigViews",					IDM_CONFIG_VIEWS,					1,							0 },
	{ L"EditClearDeleted",				IDM_EDIT_CLEARDELETED,				1,							0 },
	{ L"EditCopy",						IDM_EDIT_COPY,						1,							0 },
	{ L"EditCut",						IDM_EDIT_CUT,						1,							0 },
	{ L"EditDelete",					IDM_EDIT_DELETE,					1,							0 },
	{ L"EditDeleteBackwardChar",		IDM_EDIT_DELETEBACKWARDCHAR,		1,							0 },
	{ L"EditDeleteBackwardWord",		IDM_EDIT_DELETEBACKWARDWORD,		1,							0 },
	{ L"EditDeleteCache",				IDM_EDIT_DELETECACHE,				1,							0 },
	{ L"EditDeleteChar",				IDM_EDIT_DELETECHAR,				1,							0 },
	{ L"EditDeleteDirect",				IDM_EDIT_DELETEDIRECT,				1,							0 },
	{ L"EditDeleteJunk",				IDM_EDIT_DELETEJUNK,				1,							0 },
	{ L"EditDeleteWord",				IDM_EDIT_DELETEWORD,				1,							0 },
	{ L"EditFind",						IDM_EDIT_FIND,						1,							0 },
	{ L"EditFindNext",					IDM_EDIT_FINDNEXT,					1,							0 },
	{ L"EditFindPrev",					IDM_EDIT_FINDPREV,					1,							0 },
	{ L"EditMoveCharLeft",				IDM_EDIT_MOVECHARLEFT,				1,							0 },
	{ L"EditMoveCharRight",				IDM_EDIT_MOVECHARRIGHT,				1,							0 },
	{ L"EditMoveDocEnd",				IDM_EDIT_MOVEDOCEND,				1,							0 },
	{ L"EditMoveDocStart",				IDM_EDIT_MOVEDOCSTART,				1,							0 },
	{ L"EditMoveLineDown",				IDM_EDIT_MOVELINEDOWN,				1,							0 },
	{ L"EditMoveLineEnd",				IDM_EDIT_MOVELINEEND,				1,							0 },
	{ L"EditMoveLineStart",				IDM_EDIT_MOVELINESTART,				1,							0 },
	{ L"EditMoveLineUp",				IDM_EDIT_MOVELINEUP,				1,							0 },
	{ L"EditMovePageDown",				IDM_EDIT_MOVEPAGEDOWN,				1,							0 },
	{ L"EditMovePageUp",				IDM_EDIT_MOVEPAGEUP,				1,							0 },
	{ L"EditPaste",						IDM_EDIT_PASTE,						1,							0 },
	{ L"EditPasteWithQuote",			IDM_EDIT_PASTEWITHQUOTE,			1,							0 },
	{ L"EditRedo",						IDM_EDIT_REDO,						1,							0 },
	{ L"EditReplace",					IDM_EDIT_REPLACE,					1,							0 },
	{ L"EditSelectAll",					IDM_EDIT_SELECTALL,					1,							0 },
	{ L"EditUndo",						IDM_EDIT_UNDO,						1,							0 },
	{ L"FileCheck",						IDM_FILE_CHECK,						1,							0 },
	{ L"FileClose",						IDM_FILE_CLOSE,						1,							0 },
	{ L"FileCompact",					IDM_FILE_COMPACT,					1,							0 },
	{ L"FileDraft",						IDM_FILE_DRAFT,						1,							0 },
	{ L"FileDraftClose",				IDM_FILE_DRAFTCLOSE,				1,							0 },
	{ L"FileDump",						IDM_FILE_DUMP,						1,							0 },
	{ L"FileExit",						IDM_FILE_EXIT,						1,							0 },
	{ L"FileExport",					IDM_FILE_EXPORT,					1,							0 },
	{ L"FileHide",						IDM_FILE_HIDE,						1,							0 },
	{ L"FileImport",					IDM_FILE_IMPORT,					1,							0 },
	{ L"FileInsert",					IDM_FILE_INSERT,					1,							0 },
	{ L"FileLoad",						IDM_FILE_LOAD,						1,							0 },
	{ L"FileOffline",					IDM_FILE_OFFLINE,					1,							0 },
	{ L"FileOpen",						IDM_FILE_OPEN,						1,							0 },
	{ L"FilePrint",						IDM_FILE_PRINT,						1,							0 },
	{ L"FileSalvage",					IDM_FILE_SALVAGE,					1,							0 },
	{ L"FileSave",						IDM_FILE_SAVE,						1,							0 },
	{ L"FileSend",						IDM_FILE_SEND,						1,							0 },
	{ L"FileSendNow",					IDM_FILE_SENDNOW,					1,							0 },
	{ L"FileShow",						IDM_FILE_SHOW,						1,							0 },
	{ L"FileUninstall",					IDM_FILE_UNINSTALL,					1,							0 },
	{ L"FolderCollapse",				IDM_FOLDER_COLLAPSE,				1,							0 },
	{ L"FolderCreate",					IDM_FOLDER_CREATE,					1,							0 },
	{ L"FolderDelete",					IDM_FOLDER_DELETE,					1,							0 },
	{ L"FolderEmpty",					IDM_FOLDER_EMPTY,					MAX_FOLDER_EMPTY,			0 },
	{ L"FolderEmptyTrash",				IDM_FOLDER_EMPTYTRASH,				1,							0 },
	{ L"FolderExpand",					IDM_FOLDER_EXPAND,					1,							0 },
	{ L"FolderProperty",				IDM_FOLDER_PROPERTY,				1,							0 },
	{ L"FolderRename",					IDM_FOLDER_RENAME,					1,							0 },
	{ L"FolderShowSize",				IDM_FOLDER_SHOWSIZE,				1,							0 },
	{ L"FolderSubscribe",				IDM_FOLDER_SUBSCRIBE,				1,							0 },
	{ L"FolderUpdate",					IDM_FOLDER_UPDATE,					1,							0 },
	{ L"HeaderEditItem",				IDM_VIEW_FOCUSEDITITEM,				MAX_VIEW_FOCUSEDITITEM,		0 },
	{ L"HelpAbout",						IDM_HELP_ABOUT,						1,							0 },
	{ L"HelpCheckUpdate",				IDM_HELP_CHECKUPDATE,				1,							0 },
	{ L"HelpOpenURL",					IDM_HELP_OPENURL,					MAX_HELP_OPENURL,			0 },
	{ L"MessageAddClean",				IDM_MESSAGE_ADDCLEAN,				1,							0 },
	{ L"MessageAddJunk",				IDM_MESSAGE_ADDJUNK,				1,							0 },
	{ L"MessageApplyRule",				IDM_MESSAGE_APPLYRULE,				1,							0 },
	{ L"MessageApplyRuleAll",			IDM_MESSAGE_APPLYRULEALL,			1,							0 },
	{ L"MessageApplyRuleSelected",		IDM_MESSAGE_APPLYRULESELECTED,		1,							0 },
	{ L"MessageCertificate",			IDM_MESSAGE_CERTIFICATE,			1,							0 },
	{ L"MessageClearRecents",			IDM_MESSAGE_CLEARRECENTS,			1,							0 },
	{ L"MessageCombine",				IDM_MESSAGE_COMBINE,				1,							0 },
	{ L"MessageCopy",					IDM_MESSAGE_COPY,					MAX_MESSAGE_COPY,			0 },
	{ L"MessageCreate",					IDM_MESSAGE_CREATE,					MAX_MESSAGE_CREATE,			0 },
	{ L"MessageCreateExternal",			IDM_MESSAGE_CREATEEXTERNAL,			MAX_MESSAGE_CREATEEXTERNAL,	0 },
	{ L"MessageCreateFromClipboard",	IDM_MESSAGE_CREATEFROMCLIPBOARD,	1,							0 },
	{ L"MessageDeleteAttachment",		IDM_MESSAGE_DELETEATTACHMENT,		1,							0 },
	{ L"MessageDetach",					IDM_MESSAGE_DETACH,					1,							0 },
	{ L"MessageDraftFromClipboard",		IDM_MESSAGE_DRAFTFROMCLIPBOARD,		1,							0 },
	{ L"MessageExpandDigest",			IDM_MESSAGE_EXPANDDIGEST,			1,							0 },
	{ L"MessageLabel",					IDM_MESSAGE_LABEL,					MAX_MESSAGE_LABEL,			0 },
	{ L"MessageMacro",					IDM_MESSAGE_MACRO,					MAX_MESSAGE_MACRO,			0 },
	{ L"MessageMark",					IDM_MESSAGE_MARK,					1,							0 },
	{ L"MessageMarkDeleted",			IDM_MESSAGE_MARKDELETED,			1,							0 },
	{ L"MessageMarkDownload",			IDM_MESSAGE_MARKDOWNLOAD,			1,							0 },
	{ L"MessageMarkDownloadText",		IDM_MESSAGE_MARKDOWNLOADTEXT,		1,							0 },
	{ L"MessageMarkSeen",				IDM_MESSAGE_MARKSEEN,				1,							0 },
	{ L"MessageMarkUser1",				IDM_MESSAGE_MARKUSER1,				1,							0 },
	{ L"MessageMarkUser2",				IDM_MESSAGE_MARKUSER2,				1,							0 },
	{ L"MessageMarkUser3",				IDM_MESSAGE_MARKUSER3,				1,							0 },
	{ L"MessageMarkUser4",				IDM_MESSAGE_MARKUSER4,				1,							0 },
	{ L"MessageMove",					IDM_MESSAGE_MOVE,					MAX_MESSAGE_MOVE,			0 },
	{ L"MessageOpen",					IDM_MESSAGE_OPEN,					MAX_MESSAGE_OPEN,			0 },
	{ L"MessageOpenAttachment",			IDM_MESSAGE_OPENATTACHMENT,			MAX_MESSAGE_OPENATTACHMENT,	0 },
	{ L"MessageOpenLink",				IDM_MESSAGE_OPENLINK,				1,							0 },
	{ L"MessageOpenRecent",				IDM_MESSAGE_OPENRECENT,				MAX_MESSAGE_OPENRECENT,		0 },
	{ L"MessageOpenURL",				IDM_MESSAGE_OPENURL,				MAX_MESSAGE_OPENURL,		0 },
	{ L"MessageProperty",				IDM_MESSAGE_PROPERTY,				1,							0 },
	{ L"MessageRemoveClean",			IDM_MESSAGE_REMOVECLEAN,			1,							0 },
	{ L"MessageRemoveJunk",				IDM_MESSAGE_REMOVEJUNK,				1,							0 },
	{ L"MessageSearch",					IDM_MESSAGE_SEARCH,					1,							0 },
	{ L"MessageUnmark",					IDM_MESSAGE_UNMARK,					1,							0 },
	{ L"MessageUnmarkDeleted",			IDM_MESSAGE_UNMARKDELETED,			1,							0 },
	{ L"MessageUnmarkDownload",			IDM_MESSAGE_UNMARKDOWNLOAD,			1,							0 },
	{ L"MessageUnmarkDownloadText",		IDM_MESSAGE_UNMARKDOWNLOADTEXT,		1,							0 },
	{ L"MessageUnmarkSeen",				IDM_MESSAGE_UNMARKSEEN,				1,							0 },
	{ L"MessageUnmarkUser1",			IDM_MESSAGE_UNMARKUSER1,			1,							0 },
	{ L"MessageUnmarkUser2",			IDM_MESSAGE_UNMARKUSER2,			1,							0 },
	{ L"MessageUnmarkUser3",			IDM_MESSAGE_UNMARKUSER3,			1,							0 },
	{ L"MessageUnmarkUser4",			IDM_MESSAGE_UNMARKUSER4,			1,							0 },
#ifdef QMTABWINDOW
	{ L"TabClose",						IDM_TAB_CLOSE,						1,							0 },
	{ L"TabCreate",						IDM_TAB_CREATE,						1,							0 },
	{ L"TabEditTitle",					IDM_TAB_EDITTITLE,					1,							0 },
	{ L"TabLock",						IDM_TAB_LOCK,						1,							0 },
	{ L"TabMoveLeft",					IDM_TAB_MOVELEFT,					1,							0 },
	{ L"TabMoveRight",					IDM_TAB_MOVERIGHT,					1,							0 },
	{ L"TabNavigateNext",				IDM_TAB_NAVIGATENEXT,				1,							0 },
	{ L"TabNavigatePrev",				IDM_TAB_NAVIGATEPREV,				1,							0 },
	{ L"TabSelect",						IDM_TAB_SELECT,						MAX_TAB_SELECT,				0 },
#endif
	{ L"ToolAccount",					IDM_TOOL_ACCOUNT,					1,							0 },
	{ L"ToolAddAddress",				IDM_TOOL_ADDADDRESS,				1,							0 },
	{ L"ToolAddressBook",				IDM_TOOL_ADDRESSBOOK,				1,							0 },
	{ L"ToolArchiveAttachment",			IDM_TOOL_ARCHIVEATTACHMENT,			1,							0 },
	{ L"ToolAttachment",				IDM_TOOL_ATTACHMENT,				1,							0 },
	{ L"ToolAutoPilot",					IDM_TOOL_AUTOPILOT,					1,							0 },
	{ L"ToolDialup",					IDM_TOOL_DIALUP,					1,							0 },
	{ L"ToolEncoding",					IDM_TOOL_ENCODING,					MAX_TOOL_ENCODING,			0 },
	{ L"ToolGoround",					IDM_TOOL_GOROUND,					MAX_TOOL_GOROUND,			0 },
	{ L"ToolHeaderEdit",				IDM_TOOL_HEADEREDIT,				1,							0 },
	{ L"ToolInsertSignature",			IDM_TOOL_INSERTSIGNATURE,			1,							0 },
	{ L"ToolInsertText",				IDM_TOOL_INSERTTEXT,				MAX_TOOL_INSERTTEXT,		0 },
	{ L"ToolInvokeAction",				IDM_TOOL_INVOKEACTION,				MAX_TOOL_INVOKEACTION,		0 },
	{ L"ToolOptions",					IDM_TOOL_OPTIONS,					1,							0 },
	{ L"ToolPGPEncrypt",				IDM_TOOL_PGPENCRYPT,				1,							0 },
	{ L"ToolPGPMime",					IDM_TOOL_PGPMIME,					1,							0 },
	{ L"ToolPGPSign",					IDM_TOOL_PGPSIGN,					1,							0 },
	{ L"ToolPopupMenu",					IDM_TOOL_POPUPMENU,					MAX_TOOL_POPUPMENU,			0 },
	{ L"ToolReceive",					IDM_TOOL_RECEIVE,					1,							0 },
	{ L"ToolReceiveFolder",				IDM_TOOL_RECEIVEFOLDER,				1,							0 },
	{ L"ToolReform",					IDM_TOOL_REFORM,					1,							0 },
	{ L"ToolReformAll",					IDM_TOOL_REFORMALL,					1,							0 },
	{ L"ToolReformAuto",				IDM_TOOL_REFORMAUTO,				1,							0 },
	{ L"ToolSMIMEEncrypt",				IDM_TOOL_SMIMEENCRYPT,				1,							0 },
	{ L"ToolSMIMESign",					IDM_TOOL_SMIMESIGN,					1,							0 },
	{ L"ToolScript",					IDM_TOOL_SCRIPT,					MAX_TOOL_SCRIPT,			0 },
	{ L"ToolSelectAddress",				IDM_TOOL_SELECTADDRESS,				1,							0 },
	{ L"ToolSend",						IDM_TOOL_SEND,						1,							0 },
	{ L"ToolSubAccount",				IDM_TOOL_SUBACCOUNT,				MAX_TOOL_SUBACCOUNT,		0 },
	{ L"ToolSync",						IDM_TOOL_SYNC,						1,							0 },
	{ L"ViewDropDown",					IDM_VIEW_DROPDOWN,					1,							0 },
	{ L"ViewEncoding",					IDM_VIEW_ENCODING,					MAX_VIEW_ENCODING,			0 },
	{ L"ViewFilter",					IDM_VIEW_FILTER,					MAX_VIEW_FILTER,			0 },
	{ L"ViewFilterCustom",				IDM_VIEW_FILTERCUSTOM,				1,							0 },
	{ L"ViewFit",						IDM_VIEW_FIT,						MAX_VIEW_FIT,				0 },
	{ L"ViewFocus",						IDM_VIEW_FOCUS,						MAX_VIEW_FOCUS,				0 },
	{ L"ViewFocusEditItem",				IDM_VIEW_FOCUSEDITITEM,				MAX_VIEW_FOCUSEDITITEM,		0 },
	{ L"ViewFocusItem",					IDM_VIEW_FOCUSITEM,					MAX_VIEW_FOCUSITEM,			0 },
	{ L"ViewFocusNext",					IDM_VIEW_FOCUSNEXT,					1,							0 },
	{ L"ViewFocusNextEditItem",			IDM_VIEW_FOCUSNEXTEDITITEM,			1,							0 },
	{ L"ViewFocusNextItem",				IDM_VIEW_FOCUSNEXTITEM,				1,							0 },
	{ L"ViewFocusPrev",					IDM_VIEW_FOCUSPREV,					1,							0 },
	{ L"ViewFocusPrevEditItem",			IDM_VIEW_FOCUSPREVEDITITEM,			1,							0 },
	{ L"ViewFocusPrevItem",				IDM_VIEW_FOCUSPREVITEM,				1,							0 },
#ifdef QMHTMLVIEW
	{ L"ViewHtmlInternetZoneMode",		IDM_VIEW_HTMLINTERNETZONEMODE,		1,							0 },
	{ L"ViewHtmlMode",					IDM_VIEW_HTMLMODE,					1,							0 },
	{ L"ViewHtmlOnlineMode",			IDM_VIEW_HTMLONLINEMODE,			1,							0 },
#endif
	{ L"ViewLockPreview",				IDM_VIEW_LOCKPREVIEW,				1,							0 },
	{ L"ViewNextAccount",				IDM_VIEW_NEXTACCOUNT,				1,							0 },
	{ L"ViewNextFolder",				IDM_VIEW_NEXTFOLDER,				1,							0 },
	{ L"ViewNextMessage",				IDM_VIEW_NEXTMESSAGE,				1,							0 },
	{ L"ViewNextMessagePage",			IDM_VIEW_NEXTMESSAGEPAGE,			1,							0 },
	{ L"ViewNextUnseenMessage",			IDM_VIEW_NEXTUNSEENMESSAGE,			1,							0 },
	{ L"ViewOpenLink",					IDM_VIEW_OPENLINK,					1,							0 },
	{ L"ViewPGPMode",					IDM_VIEW_PGPMODE,					1,							0 },
	{ L"ViewPrevAccount",				IDM_VIEW_PREVACCOUNT,				1,							0 },
	{ L"ViewPrevFolder",				IDM_VIEW_PREVFOLDER,				1,							0 },
	{ L"ViewPrevMessage",				IDM_VIEW_PREVMESSAGE,				1,							0 },
	{ L"ViewPrevMessagePage",			IDM_VIEW_PREVMESSAGEPAGE,			1,							0 },
	{ L"ViewQuoteMode",					IDM_VIEW_QUOTEMODE,					1,							0 },
	{ L"ViewRawMode",					IDM_VIEW_RAWMODE,					1,							0 },
	{ L"ViewRefresh",					IDM_VIEW_REFRESH,					1,							0 },
	{ L"ViewSMIMEMode",					IDM_VIEW_SMIMEMODE,					1,							0 },
	{ L"ViewScrollBottom",				IDM_VIEW_SCROLLBOTTOM,				1,							0 },
	{ L"ViewScrollLineDown",			IDM_VIEW_SCROLLLINEDOWN,			1,							0 },
	{ L"ViewScrollLineUp",				IDM_VIEW_SCROLLLINEUP,				1,							0 },
	{ L"ViewScrollMessagePageDown",		IDM_VIEW_SCROLLMESSAGEPAGEDOWN,		1,							0 },
	{ L"ViewScrollMessagePageUp",		IDM_VIEW_SCROLLMESSAGEPAGEUP,		1,							0 },
	{ L"ViewScrollPageDown",			IDM_VIEW_SCROLLPAGEDOWN,			1,							0 },
	{ L"ViewScrollPageUp",				IDM_VIEW_SCROLLPAGEUP,				1,							0 },
	{ L"ViewScrollTop",					IDM_VIEW_SCROLLTOP,					1,							0 },
	{ L"ViewSelectMessage",				IDM_VIEW_SELECTMESSAGE,				1,							0 },
	{ L"ViewSelectMode",				IDM_VIEW_SELECTMODE,				1,							0 },
	{ L"ViewShowFolder",				IDM_VIEW_SHOWFOLDER,				1,							0 },
	{ L"ViewShowHeader",				IDM_VIEW_SHOWHEADER,				1,							0 },
	{ L"ViewShowHeaderColumn",			IDM_VIEW_SHOWHEADERCOLUMN,			1,							0 },
	{ L"ViewShowPreview",				IDM_VIEW_SHOWPREVIEW,				1,							0 },
	{ L"ViewShowStatusBar",				IDM_VIEW_SHOWSTATUSBAR,				1,							0 },
	{ L"ViewShowSyncDialog",			IDM_VIEW_SHOWSYNCDIALOG,			1,							0 },
	{ L"ViewShowTab",					IDM_VIEW_SHOWTAB,					1,							0 },
	{ L"ViewShowToolbar",				IDM_VIEW_SHOWTOOLBAR,				1,							0 },
	{ L"ViewSort",						IDM_VIEW_SORT,						MAX_VIEW_SORT,				0 },
	{ L"ViewSortAddress",				IDM_VIEW_SORTADDRESS,				1,							0 },
	{ L"ViewSortAscending",				IDM_VIEW_SORTASCENDING,				1,							0 },
	{ L"ViewSortComment",				IDM_VIEW_SORTCOMMENT,				1,							0 },
	{ L"ViewSortDescending",			IDM_VIEW_SORTDESCENDING,			1,							0 },
	{ L"ViewSortFlat",					IDM_VIEW_SORTFLAT,					1,							0 },
	{ L"ViewSortFloatThread",			IDM_VIEW_SORTFLOATTHREAD,			1,							0 },
	{ L"ViewSortName",					IDM_VIEW_SORTNAME,					1,							0 },
	{ L"ViewSortThread",				IDM_VIEW_SORTTHREAD,				1,							0 },
	{ L"ViewSortToggleThread",			IDM_VIEW_SORTTOGGLETHREAD,			1,							0 },
	{ L"ViewSourceMode",				IDM_VIEW_SOURCEMODE,				1,							0 },
	{ L"ViewTemplate",					IDM_VIEW_TEMPLATE,					MAX_VIEW_TEMPLATE,			0 },
	{ L"ViewZoom",						IDM_VIEW_ZOOM,						MAX_VIEW_ZOOM,				0 },
};

}

#endif // __ACTIONITEM_H__
