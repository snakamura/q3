/*
 * $Id$
 *
 * Copyright(C) 1998-2004 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __MENU_H__
#define __MENU_H__

#include <qsaction.h>

#include "resourceinc.h"


namespace qm {

const qs::ActionItem menuItems[] = {
	{ L"AttachmentEditAdd",			IDM_ATTACHMENTEDIT_ADD,			},
	{ L"AttachmentEditDelete",		IDM_ATTACHMENTEDIT_DELETE,		},
	{ L"AttachmentOpen",			IDM_ATTACHMENT_OPEN				},
	{ L"AttachmentSave",			IDM_ATTACHMENT_SAVE				},
	{ L"AttachmentSaveAll",			IDM_ATTACHMENT_SAVEALL			},
	{ L"ConfigViews",				IDM_CONFIG_VIEWS				},
	{ L"EditClearDeleted",			IDM_EDIT_CLEARDELETED			},
	{ L"EditCopy",					IDM_EDIT_COPY					},
	{ L"EditCut",					IDM_EDIT_CUT					},
	{ L"EditDelete",				IDM_EDIT_DELETE					},
	{ L"EditDeleteCache",			IDM_EDIT_DELETECACHE			},
	{ L"EditDeleteDirect",			IDM_EDIT_DELETEDIRECT			},
	{ L"EditFind",					IDM_EDIT_FIND					},
	{ L"EditFindNext",				IDM_EDIT_FINDNEXT				},
	{ L"EditFindPrev",				IDM_EDIT_FINDPREV				},
	{ L"EditPaste",					IDM_EDIT_PASTE					},
	{ L"EditPasteWithQuote",		IDM_EDIT_PASTEWITHQUOTE			},
	{ L"EditRedo",					IDM_EDIT_REDO					},
	{ L"EditReplace",				IDM_EDIT_REPLACE				},
	{ L"EditSelectAll",				IDM_EDIT_SELECTALL				},
	{ L"EditUndo",					IDM_EDIT_UNDO					},
	{ L"FileCheck",					IDM_FILE_CHECK					},
	{ L"FileClose",					IDM_FILE_CLOSE					},
	{ L"FileCompact",				IDM_FILE_COMPACT				},
	{ L"FileDraft",					IDM_FILE_DRAFT					},
	{ L"FileDump",					IDM_FILE_DUMP					},
	{ L"FileExit",					IDM_FILE_EXIT					},
	{ L"FileExport",				IDM_FILE_EXPORT					},
	{ L"FileImport",				IDM_FILE_IMPORT					},
	{ L"FileInsert",				IDM_FILE_INSERT					},
	{ L"FileLoad",					IDM_FILE_LOAD					},
	{ L"FileOffline",				IDM_FILE_OFFLINE				},
	{ L"FileOpen",					IDM_FILE_OPEN					},
	{ L"FilePrint",					IDM_FILE_PRINT					},
	{ L"FileSalvage",				IDM_FILE_SALVAGE,				},
	{ L"FileSave",					IDM_FILE_SAVE					},
	{ L"FileSend",					IDM_FILE_SEND					},
	{ L"FileSendNow",				IDM_FILE_SENDNOW				},
	{ L"FolderCreate",				IDM_FOLDER_CREATE				},
	{ L"FolderDelete",				IDM_FOLDER_DELETE				},
	{ L"FolderEmpty",				IDM_FOLDER_EMPTY				},
	{ L"FolderEmptyTrash",			IDM_FOLDER_EMPTYTRASH			},
	{ L"FolderProperty",			IDM_FOLDER_PROPERTY				},
	{ L"FolderRename",				IDM_FOLDER_RENAME				},
	{ L"FolderShowSize",			IDM_FOLDER_SHOWSIZE				},
	{ L"FolderUpdate",				IDM_FOLDER_UPDATE				},
	{ L"MessageApplyRule",			IDM_MESSAGE_APPLYRULE			},
	{ L"MessageApplyRuleSelected",	IDM_MESSAGE_APPLYRULESELECTED	},
	{ L"MessageCombine",			IDM_MESSAGE_COMBINE				},
	{ L"MessageCreateFromClipboard",IDM_MESSAGE_CREATEFROMCLIPBOARD	},
	{ L"MessageDraftFromClipboard",	IDM_MESSAGE_DRAFTFROMCLIPBOARD	},
	{ L"MessageEdit",				IDM_MESSAGE_EDIT				},
	{ L"MessageEditExternal",		IDM_MESSAGE_EDITEXTERNAL		},
	{ L"MessageForward",			IDM_MESSAGE_FORWARD				},
	{ L"MessageForwardExternal",	IDM_MESSAGE_FORWARDEXTERNAL		},
	{ L"MessageMark",				IDM_MESSAGE_MARK				},
	{ L"MessageMarkDeleted",		IDM_MESSAGE_MARKDELETED			},
	{ L"MessageMarkDownload",		IDM_MESSAGE_MARKDOWNLOAD		},
	{ L"MessageMarkDownloadText",	IDM_MESSAGE_MARKDOWNLOADTEXT	},
	{ L"MessageMarkSeen",			IDM_MESSAGE_MARKSEEN			},
	{ L"MessageNew",				IDM_MESSAGE_NEW					},
	{ L"MessageNewExternal",		IDM_MESSAGE_NEWEXTERNAL			},
	{ L"MessageProperty",			IDM_MESSAGE_PROPERTY			},
	{ L"MessageReply",				IDM_MESSAGE_REPLY				},
	{ L"MessageReplyAll",			IDM_MESSAGE_REPLYALL			},
	{ L"MessageReplyAllExternal",	IDM_MESSAGE_REPLYALLEXTERNAL	},
	{ L"MessageReplyExternal",		IDM_MESSAGE_REPLYEXTERNAL		},
	{ L"MessageSearch",				IDM_MESSAGE_SEARCH				},
	{ L"MessageUnmark",				IDM_MESSAGE_UNMARK				},
	{ L"MessageUnmarkDeleted",		IDM_MESSAGE_UNMARKDELETED		},
	{ L"MessageUnmarkDownload",		IDM_MESSAGE_UNMARKDOWNLOAD		},
	{ L"MessageUnmarkDownloadText",	IDM_MESSAGE_UNMARKDOWNLOADTEXT	},
	{ L"MessageUnmarkSeen",			IDM_MESSAGE_UNMARKSEEN			},
	{ L"ToolAccount",				IDM_TOOL_ACCOUNT				},
	{ L"ToolAddressBook",			IDM_TOOL_ADDRESSBOOK			},
	{ L"ToolAttachment",			IDM_TOOL_ATTACHMENT				},
	{ L"ToolCancel",				IDM_TOOL_CANCEL					},
	{ L"ToolCheckNewMail",			IDM_TOOL_CHECKNEWMAIL,			},
	{ L"ToolDialup",				IDM_TOOL_DIALUP					},
	{ L"ToolEncrypt",				IDM_TOOL_ENCRYPT				},
	{ L"ToolHeaderEdit",			IDM_TOOL_HEADEREDIT				},
	{ L"ToolInsertSignature",		IDM_TOOL_INSERTSIGNATURE		},
	{ L"ToolInsertText",			IDM_TOOL_INSERTTEXT				},
	{ L"ToolOptions",				IDM_TOOL_OPTIONS				},
	{ L"ToolReceive",				IDM_TOOL_RECEIVE				},
	{ L"ToolReform",				IDM_TOOL_REFORM					},
	{ L"ToolReformAll",				IDM_TOOL_REFORMALL				},
	{ L"ToolReformAuto",			IDM_TOOL_REFORMAUTO				},
	{ L"ToolSend",					IDM_TOOL_SEND					},
	{ L"ToolSign",					IDM_TOOL_SIGN					},
	{ L"ToolSync",					IDM_TOOL_SYNC					},
	{ L"ViewDecryptVerifyMode",		IDM_VIEW_DECRYPTVERIFYMODE		},
#ifdef QMHTMLVIEW
	{ L"ViewHtmlMode",				IDM_VIEW_HTMLMODE				},
	{ L"ViewHtmlOnlineMode",		IDM_VIEW_HTMLONLINEMODE			},
#endif
	{ L"ViewLockPreview",			IDM_VIEW_LOCKPREVIEW			},
	{ L"ViewNextMessage",			IDM_VIEW_NEXTMESSAGE			},
	{ L"ViewNextMessagePage",		IDM_VIEW_NEXTMESSAGEPAGE		},
	{ L"ViewNextUnseenMessage",		IDM_VIEW_NEXTUNSEENMESSAGE		},
	{ L"ViewOpenLink",				IDM_VIEW_OPENLINK				},
	{ L"ViewPrevMessage",			IDM_VIEW_PREVMESSAGE			},
	{ L"ViewPrevMessagePage",		IDM_VIEW_PREVMESSAGEPAGE		},
	{ L"ViewQuoteMode",				IDM_VIEW_QUOTEMODE				},
	{ L"ViewRawMode",				IDM_VIEW_RAWMODE				},
	{ L"ViewRefresh",				IDM_VIEW_REFRESH				},
	{ L"ViewSelectMessage",			IDM_VIEW_SELECTMESSAGE			},
	{ L"ViewSelectMode",			IDM_VIEW_SELECTMODE				},
	{ L"ViewShowFolder",			IDM_VIEW_SHOWFOLDER				},
	{ L"ViewShowHeader",			IDM_VIEW_SHOWHEADER				},
	{ L"ViewShowHeaderColumn",		IDM_VIEW_SHOWHEADERCOLUMN		},
	{ L"ViewShowPreview",			IDM_VIEW_SHOWPREVIEW			},
	{ L"ViewShowStatusBar",			IDM_VIEW_SHOWSTATUSBAR			},
	{ L"ViewShowSyncDialog",		IDM_VIEW_SHOWSYNCDIALOG			},
	{ L"ViewShowToolbar",			IDM_VIEW_SHOWTOOLBAR			},
};

const struct PopupMenuItem
{
	const WCHAR* pwszName_;
	UINT nId_;
} popupMenuItems[] = {
	{ L"MessageApplyTemplate",			IDR_MESSAGEAPPLYTEMPLATE			},
	{ L"MessageApplyTemplateExternal",	IDR_MESSAGEAPPLYTEMPLATEEXTERNAL	},
	{ L"MessageAttachment",				IDR_MESSAGEATTACHMENT				},
	{ L"MessageMove",					IDR_MESSAGEMOVE						},
	{ L"ToolGoRound",					IDR_TOOLGOROUND						},
	{ L"ToolScript",					IDR_TOOLSCRIPT						},
	{ L"ToolSubAccount",				IDR_TOOLSUBACCOUNT					},
	{ L"ViewEncoding",					IDR_VIEWENCODING					},
	{ L"ViewFilter",					IDR_VIEWFILTER						},
	{ L"ViewSort",						IDR_VIEWSORT						},
	{ L"ViewTemplate",					IDR_VIEWTEMPLATE					},
};

}

#endif // __MENU_H__
