/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
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
#define IDM_FILE_HIDE                   41009
#define IDM_FILE_IMPORT                 41010
#define IDM_FILE_INSERT                 41011
#define IDM_FILE_LOAD                   41012
#define IDM_FILE_OFFLINE                41013
#define IDM_FILE_OPEN                   41014
#define IDM_FILE_PRINT                  41015
#define IDM_FILE_SALVAGE                41016
#define IDM_FILE_SAVE                   41017
#define IDM_FILE_SEND                   41018
#define IDM_FILE_SENDNOW                41019
#define IDM_FILE_SHOW                   41020
#define IDM_FILE_UNINSTALL              41021
#define IDM_FOLDER_COLLAPSE             42001
#define IDM_FOLDER_CREATE               42002
#define IDM_FOLDER_DELETE               42003
#define IDM_FOLDER_EMPTYTRASH           42004
#define IDM_FOLDER_EXPAND               42005
#define IDM_FOLDER_PROPERTY             42006
#define IDM_FOLDER_RENAME               42007
#define IDM_FOLDER_SHOWSIZE             42008
#define IDM_FOLDER_SUBSCRIBE            42009
#define IDM_FOLDER_UPDATE               42010
#define IDM_FOLDER_EMPTY                42100
#define IDM_MESSAGE_ADDCLEAN            43001
#define IDM_MESSAGE_ADDJUNK             43002
#define IDM_MESSAGE_APPLYRULE           43003
#define IDM_MESSAGE_APPLYRULEALL        43004
#define IDM_MESSAGE_APPLYRULESELECTED   43005
#define IDM_MESSAGE_CERTIFICATE         43006
#define IDM_MESSAGE_CLEARRECENTS        43007
#define IDM_MESSAGE_COMBINE             43008
#define IDM_MESSAGE_CREATEFROMCLIPBOARD 43009
#define IDM_MESSAGE_CREATEFROMFILE      43010
#define IDM_MESSAGE_DELETEATTACHMENT    43011
#define IDM_MESSAGE_DETACH              43012
#define IDM_MESSAGE_DRAFTFROMCLIPBOARD  43013
#define IDM_MESSAGE_DRAFTFROMFILE       43014
#define IDM_MESSAGE_EXPANDDIGEST        43015
#define IDM_MESSAGE_MARK                43016
#define IDM_MESSAGE_MARKDELETED         43017
#define IDM_MESSAGE_MARKDOWNLOAD        43018
#define IDM_MESSAGE_MARKDOWNLOADTEXT    43019
#define IDM_MESSAGE_MARKSEEN            43020
#define IDM_MESSAGE_MARKUSER1           43021
#define IDM_MESSAGE_MARKUSER2           43022
#define IDM_MESSAGE_MARKUSER3           43023
#define IDM_MESSAGE_MARKUSER4           43024
#define IDM_MESSAGE_OPENLINK            43025
#define IDM_MESSAGE_OPENURL             43026
#define IDM_MESSAGE_PROPERTY            43027
#define IDM_MESSAGE_REMOVECLEAN         43028
#define IDM_MESSAGE_REMOVEJUNK          43029
#define IDM_MESSAGE_SEARCH              43030
#define IDM_MESSAGE_UNMARK              43031
#define IDM_MESSAGE_UNMARKDELETED       43032
#define IDM_MESSAGE_UNMARKDOWNLOAD      43033
#define IDM_MESSAGE_UNMARKDOWNLOADTEXT  43034
#define IDM_MESSAGE_UNMARKSEEN          43035
#define IDM_MESSAGE_UNMARKUSER1         43036
#define IDM_MESSAGE_UNMARKUSER2         43037
#define IDM_MESSAGE_UNMARKUSER3         43038
#define IDM_MESSAGE_UNMARKUSER4         43039
#define IDM_MESSAGE_COPY                43100
#define IDM_MESSAGE_CREATE              44100
#define IDM_MESSAGE_CREATEEXTERNAL      44200
#define IDM_MESSAGE_LABEL               44300
#define IDM_MESSAGE_MACRO               44400
#define IDM_MESSAGE_MOVE                44500
#define IDM_MESSAGE_OPENATTACHMENT      45500
#define IDM_MESSAGE_OPENRECENT          45600
#define IDM_TOOL_ACCOUNT                46001
#define IDM_TOOL_ADDADDRESS             46002
#define IDM_TOOL_ADDRESSBOOK            46003
#define IDM_TOOL_ARCHIVEATTACHMENT      46004
#define IDM_TOOL_ATTACHMENT             46005
#define IDM_TOOL_AUTOPILOT              46006
#define IDM_TOOL_DIALUP                 46007
#define IDM_TOOL_HEADEREDIT             46008
#define IDM_TOOL_INSERTSIGNATURE        46009
#define IDM_TOOL_OPTIONS                46010
#define IDM_TOOL_PGPENCRYPT             46011
#define IDM_TOOL_PGPMIME                46012
#define IDM_TOOL_PGPSIGN                46013
#define IDM_TOOL_RECEIVE                46014
#define IDM_TOOL_RECEIVEFOLDER          46015
#define IDM_TOOL_REFORM                 46016
#define IDM_TOOL_REFORMALL              46017
#define IDM_TOOL_REFORMAUTO             46018
#define IDM_TOOL_SELECTADDRESS          46019
#define IDM_TOOL_SEND                   46020
#define IDM_TOOL_SMIMEENCRYPT           46021
#define IDM_TOOL_SMIMESIGN              46022
#define IDM_TOOL_SYNC                   46023
#define IDM_TOOL_ENCODING               46100
#define IDM_TOOL_GOROUND                46200
#define IDM_TOOL_INSERTTEXT             46300
#define IDM_TOOL_INVOKEACTION           46400
#define IDM_TOOL_POPUPMENU              46500
#define IDM_TOOL_SCRIPT                 46600
#define IDM_TOOL_SUBACCOUNT             46700
#define IDM_VIEW_DROPDOWN               47001
#define IDM_VIEW_FILTERCUSTOM           47002
#define IDM_VIEW_FOCUSNEXT              47003
#define IDM_VIEW_FOCUSNEXTEDITITEM      47004
#define IDM_VIEW_FOCUSNEXTITEM          47005
#define IDM_VIEW_FOCUSPREV              47006
#define IDM_VIEW_FOCUSPREVEDITITEM      47007
#define IDM_VIEW_FOCUSPREVITEM          47008
#define IDM_VIEW_HTMLINTERNETZONEMODE   47009
#define IDM_VIEW_HTMLMODE               47010
#define IDM_VIEW_HTMLONLINEMODE         47011
#define IDM_VIEW_LOCKPREVIEW            47012
#define IDM_VIEW_NEXTACCOUNT            47013
#define IDM_VIEW_NEXTFOLDER             47014
#define IDM_VIEW_NEXTMESSAGE            47015
#define IDM_VIEW_NEXTMESSAGEPAGE        47016
#define IDM_VIEW_NEXTUNSEENMESSAGE      47017
#define IDM_VIEW_OPENLINK               47018
#define IDM_VIEW_PGPMODE                47019
#define IDM_VIEW_PREVACCOUNT            47020
#define IDM_VIEW_PREVFOLDER             47021
#define IDM_VIEW_PREVMESSAGE            47022
#define IDM_VIEW_PREVMESSAGEPAGE        47023
#define IDM_VIEW_QUOTEMODE              47024
#define IDM_VIEW_RAWMODE                47025
#define IDM_VIEW_REFRESH                47026
#define IDM_VIEW_SCROLLBOTTOM           47027
#define IDM_VIEW_SCROLLLINEDOWN         47028
#define IDM_VIEW_SCROLLLINEUP           47029
#define IDM_VIEW_SCROLLPAGEDOWN         47030
#define IDM_VIEW_SCROLLPAGEUP           47031
#define IDM_VIEW_SCROLLTOP              47032
#define IDM_VIEW_SELECTMESSAGE          47033
#define IDM_VIEW_SELECTMODE             47034
#define IDM_VIEW_SHOWFOLDER             47035
#define IDM_VIEW_SHOWHEADER             47036
#define IDM_VIEW_SHOWHEADERCOLUMN       47037
#define IDM_VIEW_SHOWPREVIEW            47038
#define IDM_VIEW_SHOWSTATUSBAR          47039
#define IDM_VIEW_SHOWSYNCDIALOG         47040
#define IDM_VIEW_SHOWTAB                47041
#define IDM_VIEW_SHOWTOOLBAR            47042
#define IDM_VIEW_SMIMEMODE              47043
#define IDM_VIEW_SORTASCENDING          47044
#define IDM_VIEW_SORTADDRESS            47045
#define IDM_VIEW_SORTCOMMENT            47046
#define IDM_VIEW_SORTDESCENDING         47047
#define IDM_VIEW_SORTFLAT               47048
#define IDM_VIEW_SORTFLOATTHREAD        47049
#define IDM_VIEW_SORTNAME               47050
#define IDM_VIEW_SORTTHREAD             47051
#define IDM_VIEW_SORTTOGGLETHREAD       47052
#define IDM_VIEW_SOURCEMODE             47053
#define IDM_VIEW_ENCODING               47100
#define IDM_VIEW_FILTER                 47200
#define IDM_VIEW_FIT                    47300
#define IDM_VIEW_FOCUS                  47400
#define IDM_VIEW_FOCUSEDITITEM          47500
#define IDM_VIEW_FOCUSITEM              47600
#define IDM_VIEW_SORT                   47700
#define IDM_VIEW_TEMPLATE               47800
#define IDM_VIEW_ZOOM                   47900
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
#define IDM_HELP_ABOUT                  54001
#define IDM_HELP_CHECKUPDATE            54002
#define IDM_HELP_OPENURL                54100
#define IDM_NONE                        55000

#define MAX_ADDRESSBOOK_CATEGORY        100
#define MAX_FOLDER_EMPTY                100
#define MAX_HELP_OPENURL                100
#define MAX_MESSAGE_COPY                1000
#define MAX_MESSAGE_CREATE              100
#define MAX_MESSAGE_CREATEEXTERNAL      100
#define MAX_MESSAGE_LABEL               100
#define MAX_MESSAGE_MACRO               100
#define MAX_MESSAGE_MOVE                1000
#define MAX_MESSAGE_OPENATTACHMENT      100
#define MAX_MESSAGE_OPENRECENT          100
#define MAX_TAB_SELECT                  100
#define MAX_TOOL_ENCODING               100
#define MAX_TOOL_GOROUND                100
#define MAX_TOOL_INSERTTEXT             100
#define MAX_TOOL_INVOKEACTION           100
#define MAX_TOOL_POPUPMENU              100
#define MAX_TOOL_SCRIPT                 100
#define MAX_TOOL_SUBACCOUNT             100
#define MAX_VIEW_ENCODING               100
#define MAX_VIEW_FILTER                 100
#define MAX_VIEW_FIT                    100
#define MAX_VIEW_FOCUS                  100
#define MAX_VIEW_FOCUSEDITITEM          100
#define MAX_VIEW_FOCUSITEM              100
#define MAX_VIEW_SORT                   100
#define MAX_VIEW_TEMPLATE               100
#define MAX_VIEW_ZOOM                   100

#endif // __ACTIONID_H__
