/*
 * $Id$
 *
 * Copyright(C) 1998-2005 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __ACTIONID_H__
#define __ACTIONID_H__

#define IDM_EDIT_CLEARDELETED           40001
#define IDM_EDIT_COPY                   40002
#define IDM_EDIT_CUT                    40003
#define IDM_EDIT_DELETE                 40004
#define IDM_EDIT_DELETEBACKWARDCHAR     40005
#define IDM_EDIT_DELETEBACKWARDWORD     40006
#define IDM_EDIT_DELETECACHE            40007
#define IDM_EDIT_DELETECHAR             40008
#define IDM_EDIT_DELETEDIRECT           40009
#define IDM_EDIT_DELETEJUNK             40010
#define IDM_EDIT_DELETEWORD             40011
#define IDM_EDIT_FIND                   40012
#define IDM_EDIT_FINDNEXT               40013
#define IDM_EDIT_FINDPREV               40014
#define IDM_EDIT_MOVECHARLEFT           40015
#define IDM_EDIT_MOVECHARRIGHT          40016
#define IDM_EDIT_MOVELINEDOWN           40017
#define IDM_EDIT_MOVELINEEND            40018
#define IDM_EDIT_MOVELINESTART          40019
#define IDM_EDIT_MOVELINEUP             40020
#define IDM_EDIT_MOVEPAGEDOWN           40021
#define IDM_EDIT_MOVEPAGEUP             40022
#define IDM_EDIT_MOVEDOCEND             40023
#define IDM_EDIT_MOVEDOCSTART           40024
#define IDM_EDIT_PASTE                  40025
#define IDM_EDIT_PASTEWITHQUOTE         40026
#define IDM_EDIT_REDO                   40027
#define IDM_EDIT_REPLACE                40028
#define IDM_EDIT_SELECTALL              40029
#define IDM_EDIT_UNDO                   40030
#define IDM_FILE_CHECK                  41001
#define IDM_FILE_CLOSE                  41002
#define IDM_FILE_COMPACT                41003
#define IDM_FILE_DRAFT                  41004
#define IDM_FILE_DRAFTCLOSE             41005
#define IDM_FILE_DUMP                   41006
#define IDM_FILE_EXIT                   41007
#define IDM_FILE_EXPORT                 41008
#define IDM_FILE_IMPORT                 41009
#define IDM_FILE_INSERT                 41010
#define IDM_FILE_LOAD                   41011
#define IDM_FILE_OFFLINE                41012
#define IDM_FILE_OPEN                   41013
#define IDM_FILE_PRINT                  41014
#define IDM_FILE_SALVAGE                41015
#define IDM_FILE_SAVE                   41016
#define IDM_FILE_SEND                   41017
#define IDM_FILE_SENDNOW                41018
#define IDM_FILE_UNINSTALL              41019
#define IDM_FOLDER_COLLAPSE             42001
#define IDM_FOLDER_CREATE               42002
#define IDM_FOLDER_DELETE               42003
#define IDM_FOLDER_EMPTY                42004
#define IDM_FOLDER_EMPTYTRASH           42005
#define IDM_FOLDER_EXPAND               42006
#define IDM_FOLDER_PROPERTY             42007
#define IDM_FOLDER_RENAME               42008
#define IDM_FOLDER_SHOWSIZE             42009
#define IDM_FOLDER_UPDATE               42010
#define IDM_MESSAGE_ADDCLEAN            43001
#define IDM_MESSAGE_ADDJUNK             43002
#define IDM_MESSAGE_APPLYRULE           43003
#define IDM_MESSAGE_APPLYRULEALL        43004
#define IDM_MESSAGE_APPLYRULESELECTED   43005
#define IDM_MESSAGE_APPLYTEMPLATENONE   43006
#define IDM_MESSAGE_APPLYTEMPLATENONEEXTERNAL 43007
#define IDM_MESSAGE_CLEARRECENTS        43008
#define IDM_MESSAGE_COMBINE             43009
#define IDM_MESSAGE_CREATEFROMCLIPBOARD 43010
#define IDM_MESSAGE_CREATEFROMFILE      43011
#define IDM_MESSAGE_DELETEATTACHMENT    43012
#define IDM_MESSAGE_DETACH              43013
#define IDM_MESSAGE_DRAFTFROMCLIPBOARD  43014
#define IDM_MESSAGE_DRAFTFROMFILE       43015
#define IDM_MESSAGE_EDIT                43016
#define IDM_MESSAGE_EDITEXTERNAL        43017
#define IDM_MESSAGE_EXPANDDIGEST        43018
#define IDM_MESSAGE_FORWARD             43019
#define IDM_MESSAGE_FORWARDEXTERNAL     43020
#define IDM_MESSAGE_MARK                43021
#define IDM_MESSAGE_MARKDELETED         43022
#define IDM_MESSAGE_MARKDOWNLOAD        43023
#define IDM_MESSAGE_MARKDOWNLOADTEXT    43024
#define IDM_MESSAGE_MARKSEEN            43025
#define IDM_MESSAGE_MARKUSER1           43026
#define IDM_MESSAGE_MARKUSER2           43027
#define IDM_MESSAGE_MARKUSER3           43028
#define IDM_MESSAGE_MARKUSER4           43029
#define IDM_MESSAGE_MOVEOTHER           43030
#define IDM_MESSAGE_NEW                 43031
#define IDM_MESSAGE_NEWEXTERNAL         43032
#define IDM_MESSAGE_OPENLINK            43033
#define IDM_MESSAGE_OPENURL             43034
#define IDM_MESSAGE_PROPERTY            43035
#define IDM_MESSAGE_REMOVECLEAN         43036
#define IDM_MESSAGE_REMOVEJUNK          43037
#define IDM_MESSAGE_REPLY               43038
#define IDM_MESSAGE_REPLYEXTERNAL       43039
#define IDM_MESSAGE_REPLYALL            43040
#define IDM_MESSAGE_REPLYALLEXTERNAL    43041
#define IDM_MESSAGE_SEARCH              43042
#define IDM_MESSAGE_UNMARK              43043
#define IDM_MESSAGE_UNMARKDELETED       43044
#define IDM_MESSAGE_UNMARKDOWNLOAD      43045
#define IDM_MESSAGE_UNMARKDOWNLOADTEXT  43046
#define IDM_MESSAGE_UNMARKSEEN          43047
#define IDM_MESSAGE_UNMARKUSER1         43048
#define IDM_MESSAGE_UNMARKUSER2         43049
#define IDM_MESSAGE_UNMARKUSER3         43050
#define IDM_MESSAGE_UNMARKUSER4         43051
#define IDM_MESSAGE_APPLYTEMPLATE       43100
#define IDM_MESSAGE_APPLYTEMPLATEEXTERNAL 43200
#define IDM_MESSAGE_ATTACHMENT          43300
#define IDM_MESSAGE_MOVE                43400
#define IDM_MESSAGE_OPENRECENT          44400
#define IDM_TOOL_ACCOUNT                45001
#define IDM_TOOL_ADDADDRESS             45002
#define IDM_TOOL_ADDRESSBOOK            45003
#define IDM_TOOL_ATTACHMENT             45004
#define IDM_TOOL_AUTOPILOT              45005
#define IDM_TOOL_CANCEL                 45006
#define IDM_TOOL_DIALUP                 45007
#define IDM_TOOL_ENCODINGDEFAULT        45008
#define IDM_TOOL_HEADEREDIT             45009
#define IDM_TOOL_INSERTSIGNATURE        45010
#define IDM_TOOL_INSERTTEXTNONE         45011
#define IDM_TOOL_OPTIONS                45012
#define IDM_TOOL_PGPENCRYPT             45013
#define IDM_TOOL_PGPMIME                45014
#define IDM_TOOL_PGPSIGN                45015
#define IDM_TOOL_RECEIVE                45016
#define IDM_TOOL_REFORM                 45017
#define IDM_TOOL_REFORMALL              45018
#define IDM_TOOL_REFORMAUTO             45019
#define IDM_TOOL_SCRIPTNONE             45020
#define IDM_TOOL_SELECTADDRESS          45021
#define IDM_TOOL_SEND                   45022
#define IDM_TOOL_SMIMEENCRYPT           45023
#define IDM_TOOL_SMIMESIGN              45024
#define IDM_TOOL_SYNC                   45025
#define IDM_TOOL_ENCODING               45100
#define IDM_TOOL_GOROUND                45200
#define IDM_TOOL_SCRIPT                 45300
#define IDM_TOOL_SUBACCOUNT             45400
#define IDM_TOOL_INSERTTEXT             45500
#define IDM_VIEW_DROPDOWN               46001
#define IDM_VIEW_ENCODINGAUTODETECT     46002
#define IDM_VIEW_FILTERCUSTOM           46003
#define IDM_VIEW_FILTERNONE             46004
#define IDM_VIEW_FOCUSNEXT              46005
#define IDM_VIEW_FOCUSPREV              46006
#define IDM_VIEW_HTMLMODE               46007
#define IDM_VIEW_HTMLONLINEMODE         46008
#define IDM_VIEW_LOCKPREVIEW            46009
#define IDM_VIEW_NEXTACCOUNT            46010
#define IDM_VIEW_NEXTFOLDER             46011
#define IDM_VIEW_NEXTMESSAGE            46012
#define IDM_VIEW_NEXTMESSAGEPAGE        46013
#define IDM_VIEW_NEXTUNSEENMESSAGE      46014
#define IDM_VIEW_OPENLINK               46015
#define IDM_VIEW_PGPMODE                46016
#define IDM_VIEW_PREVACCOUNT            46017
#define IDM_VIEW_PREVFOLDER             46018
#define IDM_VIEW_PREVMESSAGE            46019
#define IDM_VIEW_PREVMESSAGEPAGE        46020
#define IDM_VIEW_QUOTEMODE              46021
#define IDM_VIEW_RAWMODE                46022
#define IDM_VIEW_REFRESH                46023
#define IDM_VIEW_SCROLLBOTTOM           46024
#define IDM_VIEW_SCROLLLINEDOWN         46025
#define IDM_VIEW_SCROLLLINEUP           46026
#define IDM_VIEW_SCROLLPAGEDOWN         46027
#define IDM_VIEW_SCROLLPAGEUP           46028
#define IDM_VIEW_SCROLLTOP              46029
#define IDM_VIEW_SELECTMESSAGE          46030
#define IDM_VIEW_SELECTMODE             46031
#define IDM_VIEW_SHOWFOLDER             46032
#define IDM_VIEW_SHOWHEADER             46033
#define IDM_VIEW_SHOWHEADERCOLUMN       46034
#define IDM_VIEW_SHOWPREVIEW            46035
#define IDM_VIEW_SHOWSTATUSBAR          46036
#define IDM_VIEW_SHOWSYNCDIALOG         46037
#define IDM_VIEW_SHOWTAB                46038
#define IDM_VIEW_SHOWTOOLBAR            46039
#define IDM_VIEW_SMIMEMODE              46040
#define IDM_VIEW_SORTASCENDING          46041
#define IDM_VIEW_SORTADDRESS            46042
#define IDM_VIEW_SORTCOMMENT            46043
#define IDM_VIEW_SORTDESCENDING         46044
#define IDM_VIEW_SORTFLOATTHREAD        46045
#define IDM_VIEW_SORTNAME               46046
#define IDM_VIEW_SORTTHREAD             46047
#define IDM_VIEW_TEMPLATENONE           46048
#define IDM_VIEW_ENCODING               46100
#define IDM_VIEW_FILTER                 46200
#define IDM_VIEW_SORT                   46300
#define IDM_VIEW_TEMPLATE               46400
#define IDM_FOCUS_HEADEREDITITEM        47000
#define IDM_ADDRESSBOOK_ALLCATEGORY     48001
#define IDM_ADDRESSBOOK_CHANGEBCC       48002
#define IDM_ADDRESSBOOK_CHANGECC        48003
#define IDM_ADDRESSBOOK_CHANGETO        48004
#define IDM_ADDRESSBOOK_REMOVE          48005
#define IDM_ADDRESSBOOK_CATEGORY        48100
#define IDM_ATTACHMENT_OPEN             49001
#define IDM_ATTACHMENT_SAVE             49002
#define IDM_ATTACHMENT_SAVEALL          49003
#define IDM_ATTACHMENTEDIT_ADD          50001
#define IDM_ATTACHMENTEDIT_DELETE       50002
#define IDM_CONFIG_AUTOPILOT            51001
#define IDM_CONFIG_COLORS               51002
#define IDM_CONFIG_FILTERS              51003
#define IDM_CONFIG_GOROUND              51004
#define IDM_CONFIG_RULES                51005
#define IDM_CONFIG_SIGNATURES           51006
#define IDM_CONFIG_SYNCFILTERS          51007
#define IDM_CONFIG_TEXTS                51008
#define IDM_CONFIG_VIEWS                51009
#define IDM_TAB_CLOSE                   52001
#define IDM_TAB_CREATE                  52002
#define IDM_TAB_EDITTITLE               52003
#define IDM_TAB_LOCK                    52004
#define IDM_TAB_MOVELEFT                52005
#define IDM_TAB_MOVERIGHT               52006
#define IDM_TAB_NAVIGATENEXT            52007
#define IDM_TAB_NAVIGATEPREV            52008
#define IDM_TAB_SELECT                  52100
#define IDM_ADDRESS_DELETE              53001
#define IDM_ADDRESS_EDIT                53002
#define IDM_ADDRESS_NEW                 53003

#endif // __ACTIONID_H__
