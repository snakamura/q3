/*
 * $Id$
 *
 * Copyright(C) 1998-2006 Satoshi Nakamura
 * All rights reserved.
 *
 */

#ifndef __DEFAULTPROFILE_H__
#define __DEFAULTPROFILE_H__

#include <qsprofile.h>


namespace qm {

const qs::Profile::Default defaultProfiles[] = {
	{ L"AddressBook",	L"AddressOnly",				L"0"	},
	{ L"AddressBook",	L"Category",				L""		},
	{ L"AddressBook",	L"Externals",				L""		},
	{ L"AddressBook",	L"AddressWidth",			L"130"	},
	{ L"AddressBook",	L"CommentWidth",			L"60"	},
	{ L"AddressBook",	L"NameWidth",				L"120"	},
	{ L"AddressBook",	L"SelectedAddressWidth",	L"150"	},
#ifndef _WIN32_WCE
	{ L"AddressBook",	L"Height",					L"450"	},
	{ L"AddressBook",	L"Width",					L"620"	},
#endif
	
#ifndef _WIN32_WCE
	{ L"AddressBookFrameWindow",	L"Height",			L"0"	},
	{ L"AddressBookFrameWindow",	L"Left",			L"0"	},
	{ L"AddressBookFrameWindow",	L"Top",				L"0"	},
	{ L"AddressBookFrameWindow",	L"Width",			L"0"	},
	{ L"AddressBookFrameWindow",	L"Show",			L"1"	}, /*SW_SHOWNORMAL*/
	{ L"AddressBookFrameWindow",	L"Alpha",			L"0"	},
#endif
#ifdef _WIN32_WCE_PSPC
	{ L"AddressBookFrameWindow",	L"ShowStatusBar",	L"0"	},
#else
	{ L"AddressBookFrameWindow",	L"ShowStatusBar",	L"1"	},
#endif
	{ L"AddressBookFrameWindow",	L"ShowToolbar",		L"1"	},
	
	{ L"AddressBookListWindow",	L"FontCharset",		L"0"	},
/*	{ L"AddressBookListWindow",	L"FontFace",		L""		},*/
	{ L"AddressBookListWindow",	L"FontSize",		L"9"	},
	{ L"AddressBookListWindow",	L"FontStyle",		L"0"	},
	{ L"AddressBookListWindow",	L"AddressWidth",	L"150"	},
	{ L"AddressBookListWindow",	L"CommentWidth",	L"150"	},
	{ L"AddressBookListWindow",	L"NameWidth",		L"150"	},
	
	{ L"AutoPilot",	L"Enabled",				L"0"	},
	{ L"AutoPilot",	L"OnlyWhenConnected",	L"0"	},
	
#ifndef _WIN32_WCE
	{ L"ColorsDialog",	L"Height",	L"450"	},
	{ L"ColorsDialog",	L"Width",	L"620"	},
#endif
	
	{ L"Dialup",	L"Entry",	L""	},
	
#ifndef _WIN32_WCE
	{ L"EditFrameWindow",	L"Height",			L"0"	},
	{ L"EditFrameWindow",	L"Left",			L"0"	},
	{ L"EditFrameWindow",	L"Top",				L"0"	},
	{ L"EditFrameWindow",	L"Width",			L"0"	},
	{ L"EditFrameWindow",	L"Show",			L"1"	}, /*SW_SHOWNORMAL*/
	{ L"EditFrameWindow",	L"Alpha",			L"0"	},
#endif
#ifdef _WIN32_WCE_PSPC
	{ L"EditFrameWindow",	L"ShowStatusBar",	L"0"	},
#else
	{ L"EditFrameWindow",	L"ShowStatusBar",	L"1"	},
#endif
	{ L"EditFrameWindow",	L"ShowToolbar",		L"1"	},
	
	{ L"EditWindow",	L"FontCharset",				L"0"							},
/*	{ L"EditWindow",	L"FontFace",				L""								},*/
	{ L"EditWindow",	L"FontSize",				L"9"							},
	{ L"EditWindow",	L"FontStyle",				L"0"							},
	{ L"EditWindow",	L"AdjustExtent",			L"0"							},
	{ L"EditWindow",	L"BackgroundColor",			L"ffffff"						},
	{ L"EditWindow",	L"CharInLine",				L"0"							},
	{ L"EditWindow",	L"ClickableURL",			L"1"							},
	{ L"EditWindow",	L"DragScrollDelay",			L"300"							},
	{ L"EditWindow",	L"DragScrollInterval",		L"300"							},
	{ L"EditWindow",	L"ForegroundColor",			L"000000"						},
	{ L"EditWindow",	L"LineQuote",				L"0"							},
	{ L"EditWindow",	L"LineSpacing",				L"2"							},
	{ L"EditWindow",	L"LinkColor",				L"0000ff"						},
	{ L"EditWindow",	L"MarginBottom",			L"10"							},
	{ L"EditWindow",	L"MarginLeft",				L"10"							},
	{ L"EditWindow",	L"MarginRight",				L"10"							},
	{ L"EditWindow",	L"MarginTop",				L"10"							},
	{ L"EditWindow",	L"Quote1",					L">"							},
	{ L"EditWindow",	L"Quote2",					L"#"							},
	{ L"EditWindow",	L"QuoteColor1",				L"008000"						},
	{ L"EditWindow",	L"QuoteColor2",				L"000080"						},
	{ L"EditWindow",	L"ReformLineLength",		L"74"							},
	{ L"EditWindow",	L"ReformQuote",				L">|#"							},
	{ L"EditWindow",	L"ShowCaret",				L"1"							},
	{ L"EditWindow",	L"ShowHorizontalScrollBar",	L"0"							},
	{ L"EditWindow",	L"ShowNewLine",				L"1"							},
	{ L"EditWindow",	L"ShowRuler",				L"1"							},
	{ L"EditWindow",	L"ShowTab",					L"1"							},
	{ L"EditWindow",	L"ShowVerticalScrollBar",	L"1"							},
	{ L"EditWindow",	L"URLSchemas",				L"http https ftp file mailto"	},
	{ L"EditWindow",	L"UseSystemColor",			L"1"							},
	{ L"EditWindow",	L"WordWrap",				L"0"							},
	{ L"EditWindow",	L"TabWidth",				L"4"							},
	{ L"EditWindow",	L"Ime",						L"0"							},
#ifdef QMZIP
	{ L"EditWindow",	L"ArchiveAttachments",		L"0"							},
#endif
	{ L"EditWindow",	L"AutoReform",				L"1"							},
#ifdef _WIN32_WCE
	{ L"EditWindow",	L"HideHeaderIfNoFocus",		L"1"							},
#else
	{ L"EditWindow",	L"HideHeaderIfNoFocus",		L"0"							},
#endif
	
	{ L"Find",	L"HistorySize",	L"10"	},
	{ L"Find",	L"Ime",			L"0"	},
	{ L"Find",	L"MatchCase",	L"0"	},
	{ L"Find",	L"Regex",		L"0"	},
	
#ifndef _WIN32_WCE
	{ L"FixedFormTextDialog",	L"Height",	L"450"	},
	{ L"FixedFormTextDialog",	L"Width",	L"620"	},
#endif
	
	{ L"FolderComboBox",	L"FontCharset",		L"0"	},
/*	{ L"FolderComboBox",	L"FontFace",		L""		},*/
	{ L"FolderComboBox",	L"FontSize",		L"9"	},
	{ L"FolderComboBox",	L"FontStyle",		L"0"	},
	{ L"FolderComboBox",	L"ShowAllCount",	L"1"	},
	{ L"FolderComboBox",	L"ShowUnseenCount",	L"1"	},
	
	{ L"FolderListWindow",	L"FontCharset",			L"0"		},
/*	{ L"FolderListWindow",	L"FontFace",			L""			},*/
	{ L"FolderListWindow",	L"FontSize",			L"9"		},
	{ L"FolderListWindow",	L"FontStyle",			L"0"		},
	{ L"FolderListWindow",	L"BackgroundColor",		L"ffffff"	},
	{ L"FolderListWindow",	L"ForegroundColor",		L"000000"	},
	{ L"FolderListWindow",	L"UseSystemColor",		L"1"		},
	{ L"FolderListWindow",	L"CountWidth",			L"50"		},
	{ L"FolderListWindow",	L"IdWidth",				L"50"		},
	{ L"FolderListWindow",	L"NameWidth",			L"150"		},
	{ L"FolderListWindow",	L"SizeWidth",			L"150"		},
	{ L"FolderListWindow",	L"UnseenCountWidth",	L"50"		},
	
	{ L"FolderWindow",	L"FontCharset",				L"0"		},
/*	{ L"FolderWindow",	L"FontFace",				L""			},*/
	{ L"FolderWindow",	L"FontSize",				L"9"		},
	{ L"FolderWindow",	L"FontStyle",				L"0"		},
	{ L"FolderWindow",	L"BackgroundColor",			L"ffffff"	},
	{ L"FolderWindow",	L"ForegroundColor",			L"000000"	},
	{ L"FolderWindow",	L"UseSystemColor",			L"1"		},
	{ L"FolderWindow",	L"AccountShowAllCount",		L"1"		},
	{ L"FolderWindow",	L"AccountShowUnseenCount",	L"1"		},
	{ L"FolderWindow",	L"FolderShowAllCount",		L"1"		},
	{ L"FolderWindow",	L"FolderShowUnseenCount",	L"1"		},
	{ L"FolderWindow",	L"DragOpenWait",			L"500"		},
	{ L"FolderWindow",	L"ExpandedFolders",			L""			},
	
#ifndef _WIN32_WCE
	{ L"FullTextSearch",	L"Command",			L"namazu -l -a \"$condition\" \"$index\""	},
	{ L"FullTextSearch",	L"IndexCommand",	L"mknmz.bat -a -h -O \"$index\" \"$msg\""	},
#endif
	
	{ L"Global",	L"Action",							L""													},
	{ L"Global",	L"AddZoneId",						L"1"												},
	{ L"Global",	L"AutoUpdateCheck",					L"1"												},
	{ L"Global",	L"Bcc",								L"1"												},
	{ L"Global",	L"CharsetAliases",					L"windows-31j=shift_jis"							},
	{ L"Global",	L"ConfirmDeleteMessage",			L"0"												},
	{ L"Global",	L"ConfirmEmptyFolder",				L"1"												},
	{ L"Global",	L"ConfirmEmptyTrash",				L"1"												},
	{ L"Global",	L"CurrentFolder",					L""													},
	{ L"Global",	L"DefaultCharset",					L""													},
	{ L"Global",	L"DefaultMailAccount",				L""													},
	{ L"Global",	L"DefaultRssAccount",				L""													},
	{ L"Global",	L"DefaultTimeFormat",				L"%Y4/%M0/%D %h:%m:%s"								},
	{ L"Global",	L"DetachFolder",					L""													},
	{ L"Global",	L"DetachOpenFolder",				L"0"												},
	{ L"Global",	L"Editor",							L"notepad.exe"										},
	{ L"Global",	L"EmptyTrashOnExit",				L"0"												},
	{ L"Global",	L"Encodings",						L"iso-8859-1 iso-2022-jp shift_jis euc-jp utf-8"	},
	{ L"Global",	L"ExcludeArchive",					L"\\.(?:zip|lzh|tgz|gz)$"							},
	{ L"Global",	L"ExternalEditor",					L""													},
	{ L"Global",	L"ExternalEditorAutoCreate",		L"1"												},
#ifdef _WIN32_WCE_PSPC
	{ L"Global",	L"Filer",							L"fexplore.exe %d"									},
#else
	{ L"Global",	L"Filer",							L""													},
#endif
	{ L"Global",	L"ForwardRfc822",					L"0"												},
#ifndef _WIN32_WCE_PSPC
	{ L"Global",	L"HideWhenMinimized",				L"0"												},
#endif
	{ L"Global",	L"ImeControl",						L"1"												},
	{ L"Global",	L"IncrementalSearch",				L"0"												},
	{ L"Global",	L"NextUpdateCheck",					L""													},
	{ L"Global",	L"Libraries",						L""													},
	{ L"Global",	L"Log",								L"-1"												},
	{ L"Global",	L"LogFilter",						L""													},
	{ L"Global",	L"LogTimeFormat",					L"%Y4/%M0/%D-%h:%m:%s%z"							},
	{ L"Global",	L"Macro",							L""													},
	{ L"Global",	L"NextUnseenInOtherAccounts",		L"0"												},
	{ L"Global",	L"NextUnseenWhenScrollEnd",			L"0"												},
	{ L"Global",	L"NoBccForML",						L"0"												},
	{ L"Global",	L"Offline",							L"1"												},
	{ L"Global",	L"OpenAddressBook",					L"0"												},
	{ L"Global",	L"OpenRecentInPreview",				L"0"												},
	{ L"Global",	L"PrintExtension",					L"html"												},
	{ L"Global",	L"Quote",							L"> "												},
	{ L"Global",	L"RFC2231",							L"0"												},
	{ L"Global",	L"SaveMessageViewModePerFolder",	L"1"												},
	{ L"Global",	L"SaveOnDeactivate",				L"1"												},
	{ L"Global",	L"ShowUnseenCountOnWelcome",		L"0"												},
	{ L"Global",	L"TemporaryFolder",					L""													},
	{ L"Global",	L"UseExternalEditor",				L"0"												},
	{ L"Global",	L"WarnExtensions",					L"exe com pif bat scr htm html hta vbs js"			},
	{ L"Global",	L"XMailerWithOSVersion",			L"1"												},
	
#ifndef _WIN32_WCE
	{ L"GoRoundCourseDialog",	L"Height",	L"450"	},
	{ L"GoRoundCourseDialog",	L"Width",	L"620"	},
#endif
	
#ifndef _WIN32_WCE
	{ L"GPG",	L"Command",	L"gpg.exe"	},
#endif
	
	{ L"HeaderEditWindow",	L"FontCharset",	L"0"	},
/*	{ L"HeaderEditWindow",	L"FontFace",	L""		},*/
	{ L"HeaderEditWindow",	L"FontSize",	L"9"	},
	{ L"HeaderEditWindow",	L"FontStyle",	L"0"	},
	{ L"HeaderEditWindow",	L"ImeBcc",		L"0"	},
	{ L"HeaderEditWindow",	L"ImeCc",		L"0"	},
	{ L"HeaderEditWindow",	L"ImeSubject",	L"0"	},
	{ L"HeaderEditWindow",	L"ImeTo",		L"0"	},
	
	{ L"HeaderWindow",	L"FontCharset",	L"0"	},
/*	{ L"HeaderWindow",	L"FontFace",	L""		},*/
	{ L"HeaderWindow",	L"FontSize",	L"9"	},
	{ L"HeaderWindow",	L"FontStyle",	L"0"	},
	
	{ L"Imap4Search",	L"Command",		L"0"	},
	{ L"Imap4Search",	L"SearchBody",	L"0"	},
	
#ifndef _WIN32_WCE
	{ L"InputBoxDialog",	L"Height",	L"300"	},
	{ L"InputBoxDialog",	L"Width",	L"400"	},
#endif
	
#ifndef _WIN32_WCE
	{ L"JunkFilter",	L"BlackList",		L""			},
	{ L"JunkFilter",	L"Flags",			L"3"		}, /*JunkFilter::FLAG_AUTOLEARN | JunkFilter::FLAG_MANUALLEARN*/
	{ L"JunkFilter",	L"MaxTextLen",		L"32768"	}, /*32*1024*/
	{ L"JunkFilter",	L"ThresholdScore",	L"0.95"		},
	{ L"JunkFilter",	L"WhiteList",		L""			},
#endif
	
	{ L"Label",	L"HistorySize",	L"10"	},
	
	{ L"ListWindow",	L"FontCharset",			L"0"				},
/*	{ L"ListWindow",	L"FontFace",			L""					},*/
	{ L"ListWindow",	L"FontSize",			L"9"				},
	{ L"ListWindow",	L"FontStyle",			L"0"				},
	{ L"ListWindow",	L"BackgroundColor",		L"ffffff"			},
	{ L"ListWindow",	L"ForegroundColor",		L"000000"			},
	{ L"ListWindow",	L"UseSystemColor",		L"1"				},
	{ L"ListWindow",	L"Ellipsis",			L"1"				},
	{ L"ListWindow",	L"ShowHeaderColumn",	L"1"				},
#ifdef _WIN32_WCE_PSPC
	{ L"ListWindow",	L"SingleClickOpen",		L"1"				},
#else
	{ L"ListWindow",	L"SingleClickOpen",		L"0"				},
#endif
	{ L"ListWindow",	L"TimeFormat",			L"%Y2/%M0/%D %h:%m"	},
	
#ifndef _WIN32_WCE
	{ L"MacroDialog",	L"Height",	L"300"	},
	{ L"MacroDialog",	L"Width",	L"400"	},
#endif
	
	{ L"MacroSearch",	L"Macro",			L"0"																																			},
	{ L"MacroSearch",	L"MatchCase",		L"0"																																			},
	{ L"MacroSearch",	L"SearchHeader",	L"0"																																			},
	{ L"MacroSearch",	L"SearchBody",		L"0"																																			},
	{ L"MacroSearch",	L"SearchMacro",		L"@Or(@Contain(%Subject, $Search, $Case), @Contain(%From, $Search, $Case), @Contain(%To, $Search, $Case), @Contain(@Label(), $Search, $Case))"	},
	
#ifndef _WIN32_WCE
	{ L"MainWindow",	L"Height",	L"0"	},
	{ L"MainWindow",	L"Left",	L"0"	},
	{ L"MainWindow",	L"Top",		L"0"	},
	{ L"MainWindow",	L"Width",	L"0"	},
	{ L"MainWindow",	L"Show",	L"1"	}, /*SW_SHOWNORMAL*/
	{ L"MainWindow",	L"Alpha",	L"0"	},
#endif
#ifdef _WIN32_WCE_PSPC
	{ L"MainWindow",	L"Placement",	L"F-(L-P)"	},
#else
	{ L"MainWindow",	L"Placement",	L"F|(L-P)"	},
#endif
	{ L"MainWindow",	L"PrimaryLocation",		L"100"	},
	{ L"MainWindow",	L"SecondaryLocation",	L"200"	},
	{ L"MainWindow",	L"SecurityMode",		L"0"	},
#ifdef _WIN32_WCE_PSPC
	{ L"MainWindow",	L"ShowFolderComboBox",	L"1"	},
	{ L"MainWindow",	L"ShowFolderWindow",	L"0"	},
	{ L"MainWindow",	L"ShowPreviewWindow",	L"0"	},
	{ L"MainWindow",	L"ShowStatusBar",		L"0"	},
#else
	{ L"MainWindow",	L"ShowFolderComboBox",	L"0"	},
	{ L"MainWindow",	L"ShowFolderWindow",	L"1"	},
	{ L"MainWindow",	L"ShowPreviewWindow",	L"1"	},
	{ L"MainWindow",	L"ShowStatusBar",		L"1"	},
#endif
	{ L"MainWindow",	L"ShowToolbar",			L"1"	},
	
#ifndef _WIN32_WCE
	{ L"MessageFrameWindow",	L"Height",			L"0"	},
	{ L"MessageFrameWindow",	L"Left",			L"0"	},
	{ L"MessageFrameWindow",	L"Top",				L"0"	},
	{ L"MessageFrameWindow",	L"Width",			L"0"	},
	{ L"MessageFrameWindow",	L"Show",			L"1"	}, /*SW_SHOWNORMAL*/
	{ L"MessageFrameWindow",	L"Alpha",			L"0"	},
#endif
	{ L"MessageFrameWindow",	L"SecurityMode",	L"0"	},
#ifdef _WIN32_WCE_PSPC
	{ L"MessageFrameWindow",	L"ShowStatusBar",	L"0"	},
#else
	{ L"MessageFrameWindow",	L"ShowStatusBar",	L"1"	},
#endif
	{ L"MessageFrameWindow",	L"ShowToolbar",		L"1"	},
	
	{ L"MessageWindow",	L"FontCharset",				L"0"							},
/*	{ L"MessageWindow",	L"FontFace",				L""								},*/
	{ L"MessageWindow",	L"FontSize",				L"9"							},
	{ L"MessageWindow",	L"FontStyle",				L"0"							},
	{ L"MessageWindow",	L"AdjustExtent",			L"0"							},
	{ L"MessageWindow",	L"BackgroundColor",			L"ffffff"						},
	{ L"MessageWindow",	L"CharInLine",				L"0"							},
	{ L"MessageWindow",	L"ClickableURL",			L"1"							},
	{ L"MessageWindow",	L"DragScrollDelay",			L"300"							},
	{ L"MessageWindow",	L"DragScrollInterval",		L"300"							},
	{ L"MessageWindow",	L"ForegroundColor",			L"000000"						},
	{ L"MessageWindow",	L"LineQuote",				L"1"							},
	{ L"MessageWindow",	L"LineSpacing",				L"2"							},
	{ L"MessageWindow",	L"LinkColor",				L"0000ff"						},
	{ L"MessageWindow",	L"MarginBottom",			L"10"							},
	{ L"MessageWindow",	L"MarginLeft",				L"10"							},
	{ L"MessageWindow",	L"MarginRight",				L"10"							},
	{ L"MessageWindow",	L"MarginTop",				L"10"							},
	{ L"MessageWindow",	L"Quote1",					L">"							},
	{ L"MessageWindow",	L"Quote2",					L"#"							},
	{ L"MessageWindow",	L"QuoteColor1",				L"008000"						},
	{ L"MessageWindow",	L"QuoteColor2",				L"000080"						},
	{ L"MessageWindow",	L"ReformLineLength",		L"74"							},
	{ L"MessageWindow",	L"ReformQuote",				L">|#"							},
	{ L"MessageWindow",	L"ShowCaret",				L"0"							},
	{ L"MessageWindow",	L"ShowHorizontalScrollBar",	L"0"							},
	{ L"MessageWindow",	L"ShowNewLine",				L"0"							},
	{ L"MessageWindow",	L"ShowRuler",				L"0"							},
	{ L"MessageWindow",	L"ShowTab",					L"0"							},
	{ L"MessageWindow",	L"ShowVerticalScrollBar",	L"1"							},
	{ L"MessageWindow",	L"URLSchemas",				L"http https ftp file mailto"	},
	{ L"MessageWindow",	L"UseSystemColor",			L"1"							},
	{ L"MessageWindow",	L"WordWrap",				L"0"							},
	{ L"MessageWindow",	L"TabWidth",				L"4"							},
	{ L"MessageWindow",	L"FontGroup",				L""								},
	{ L"MessageWindow",	L"SeenWait",				L"0"							},
	{ L"MessageWindow",	L"ShowHeader",				L"1"							},
	{ L"MessageWindow",	L"ShowHeaderWindow",		L"1"							},
	{ L"MessageWindow", L"Template", 				L""								},
	{ L"MessageWindow",	L"ViewFit",					L"0"							}, /*MessageViewMode::FIT_NONE*/
	{ L"MessageWindow",	L"ViewMode",				L"32"							}, /*MessageViewMode::MODE_QUOTE*/
	{ L"MessageWindow",	L"ViewZoom",				L"-1"							}, /*MessageViewMode::ZOOM_NONE*/
	
	{ L"MoveMessageDialog",	L"ShowHidden",	L"0"	},
	
#ifndef _WIN32_WCE
	{ L"OptionDialog",	L"Height",	L"450"	},
	{ L"OptionDialog",	L"Width",	L"620"	},
#endif
	{ L"OptionDialog",	L"Panel",	L"0"	},
	
#ifndef _WIN32_WCE
	{ L"PGP",	L"Command",	L"pgp.exe"	},
	{ L"PGP",	L"UseGPG",	L"1"		},
#endif
	
	{ L"PreviewWindow",	L"FontCharset",				L"0"							},
/*	{ L"PreviewWindow",	L"FontFace",				L""								},*/
	{ L"PreviewWindow",	L"FontSize",				L"9"							},
	{ L"PreviewWindow",	L"FontStyle",				L"0"							},
	{ L"PreviewWindow",	L"AdjustExtent",			L"0"							},
	{ L"PreviewWindow",	L"BackgroundColor",			L"ffffff"						},
	{ L"PreviewWindow",	L"CharInLine",				L"0"							},
	{ L"PreviewWindow",	L"ClickableURL",			L"1"							},
	{ L"PreviewWindow",	L"DragScrollDelay",			L"300"							},
	{ L"PreviewWindow",	L"DragScrollInterval",		L"300"							},
	{ L"PreviewWindow",	L"ForegroundColor",			L"000000"						},
	{ L"PreviewWindow",	L"LineQuote",				L"1"							},
	{ L"PreviewWindow",	L"LineSpacing",				L"2"							},
	{ L"PreviewWindow",	L"LinkColor",				L"0000ff"						},
	{ L"PreviewWindow",	L"MarginBottom",			L"10"							},
	{ L"PreviewWindow",	L"MarginLeft",				L"10"							},
	{ L"PreviewWindow",	L"MarginRight",				L"10"							},
	{ L"PreviewWindow",	L"MarginTop",				L"10"							},
	{ L"PreviewWindow",	L"Quote1",					L">"							},
	{ L"PreviewWindow",	L"Quote2",					L"#"							},
	{ L"PreviewWindow",	L"QuoteColor1",				L"008000"						},
	{ L"PreviewWindow",	L"QuoteColor2",				L"000080"						},
	{ L"PreviewWindow",	L"ReformLineLength",		L"74"							},
	{ L"PreviewWindow",	L"ReformQuote",				L">|#"							},
	{ L"PreviewWindow",	L"ShowCaret",				L"0"							},
	{ L"PreviewWindow",	L"ShowHorizontalScrollBar",	L"0"							},
	{ L"PreviewWindow",	L"ShowNewLine",				L"0"							},
	{ L"PreviewWindow",	L"ShowRuler",				L"0"							},
	{ L"PreviewWindow",	L"ShowTab",					L"0"							},
	{ L"PreviewWindow",	L"ShowVerticalScrollBar",	L"1"							},
	{ L"PreviewWindow",	L"URLSchemas",				L"http https ftp file mailto"	},
	{ L"PreviewWindow",	L"UseSystemColor",			L"1"							},
	{ L"PreviewWindow",	L"WordWrap",				L"0"							},
	{ L"PreviewWindow",	L"TabWidth",				L"4"							},
	{ L"PreviewWindow",	L"Delay",					L"300"							},
	{ L"PreviewWindow",	L"FontGroup",				L""								},
	{ L"PreviewWindow",	L"SeenWait",				L"0"							},
	{ L"PreviewWindow",	L"ShowHeader",				L"1"							},
	{ L"PreviewWindow",	L"ShowHeaderWindow",		L"1"							},
	{ L"PreviewWindow",	L"Template",				L""								},
	{ L"PreviewWindow",	L"UpdateAlways",			L"0"							},
	{ L"PreviewWindow",	L"ViewFit",					L"0"							}, /*MessageViewMode::FIT_NONE*/
	{ L"PreviewWindow",	L"ViewMode",				L"32"							}, /*MessageViewMode::MODE_QUOTE*/
	{ L"PreviewWindow",	L"ViewZoom",				L"-1"							}, /*MessageViewMode::ZOOM_NONE*/
	
	{ L"RecentAddress",	L"Max",	L"10"	},
	
	{ L"Recents",	L"Filter",			L""		},
	{ L"Recents",	L"HotKey", 			L"65"	}, /*'A'*/
	{ L"Recents",	L"HotKeyModifiers",	L"5"	}, /*MOD_ALT | MOD_SHIFT*/
	{ L"Recents",	L"Max",				L"20"	},

#ifdef QMRECENTSWINDOW
	{ L"RecentsWindow",	L"Alpha",		L"224"	},
	{ L"RecentsWindow",	L"AutoPopup",	L"1"	},
	{ L"RecentsWindow",	L"HideTimeout",	L"20"	},
	{ L"RecentsWindow",	L"Width",		L"400"	},
	{ L"RecentsWindow",	L"Use",			L"1"	},
#endif
	
	{ L"Replace",	L"HistorySize",	L"10"	},
	{ L"Replace",	L"Ime",			L"0"	},
	
#ifndef _WIN32_WCE
	{ L"RulesDialog",	L"Height",	L"450"	},
	{ L"RulesDialog",	L"Width",	L"620"	},
#endif
	
	{ L"Search",	L"All",			L"0"	},
	{ L"Search",	L"Condition",	L""		},
	{ L"Search",	L"HistorySize",	L"10"	},
	{ L"Search",	L"Ime",			L"0"	},
	{ L"Search",	L"NewFolder",	L"0"	},
	{ L"Search",	L"Page",		L""		},
	{ L"Search",	L"Recursive",	L"0"	},
	
	{ L"Security",	L"DefaultMessageSecurity",	L"4112"	}, /*MESSAGESECURITY_SMIMEMULTIPARTSIGNED | MESSAGESECURITY_PGPMIME*/
	{ L"Security",	L"LoadSystemStore",			L"1"	},
	
#ifndef _WIN32_WCE
	{ L"SignatureDialog",	L"Height",	L"450"	},
	{ L"SignatureDialog",	L"Width",	L"620"	},
#endif
	
	{ L"Sync",	L"Notify",	L"0"								}, /*SyncManager::NOTIFY_ALWAYS*/
#ifdef _WIN32_WCE
	{ L"Sync",	L"Sound",	L"\\Windows\\alarm1.wav"			},
#else
	{ L"Sync",	L"Sound",	L"C:\\Windows\\Media\\notify.wav"	},
#endif
	
#ifndef _WIN32_WCE
	{ L"SyncDialog",	L"Height",	L"200"	},
	{ L"SyncDialog",	L"Left",	L"0"	},
	{ L"SyncDialog",	L"Top",		L"0"	},
	{ L"SyncDialog",	L"Width",	L"300"	},
	{ L"SyncDialog",	L"Alpha",	L"0"	},
#endif
	{ L"SyncDialog",	L"Show",	L"2"	}, /*SyncDialog::SHOW_MANUAL*/
	
#ifndef _WIN32_WCE
	{ L"SyncFiltersDialog",	L"Height",	L"450"	},
	{ L"SyncFiltersDialog",	L"Width",	L"620"	},
#endif
	
#ifdef QMTABWINDOW
	{ L"TabWindow",	L"FontCharset",		L"0"	},
/*	{ L"TabWindow",	L"FontFace",		L""		},*/
	{ L"TabWindow",	L"FontSize",		L"9"	},
	{ L"TabWindow",	L"FontStyle",		L"0"	},
	{ L"TabWindow",	L"CurrentTab",		L"0"	},
	{ L"TabWindow",	L"Multiline",		L"0"	},
	{ L"TabWindow",	L"Reuse",			L"0"	}, /*DefaultTabModel::REUSE_NONE*/
	{ L"TabWindow",	L"Show",			L"1"	},
	{ L"TabWindow",	L"ShowAllCount",	L"1"	},
	{ L"TabWindow",	L"ShowUnseenCount",	L"1"	},
#endif
};

const qs::Profile::Default defaultAccountProfiles[] = {
	{ L"Dialup",	L"DisconnectWait",	L"0"	},
	{ L"Dialup",	L"Entry",			L""		},
	{ L"Dialup",	L"ShowDialog",		L"0"	},
	{ L"Dialup",	L"Type",			L"0"	},
	
#ifndef _WIN32_WCE
	{ L"FullTextSearch",	L"Index",	L""	},
#endif
	
	{ L"Global",	L"AddMessageId",				L"1"						},
	{ L"Global",	L"AutoApplyRules",				L"0"						},
	{ L"Global",	L"BlockSize",					L"0"						},
	{ L"Global",	L"Class",						L""							},
	{ L"Global",	L"Identity",					L""							},
	{ L"Global",	L"IndexBlockSize",				L"-1"						},
	{ L"Global",	L"IndexMaxSize",				L"-1"						},
	{ L"Global",	L"LogTimeFormat",				L"%Y4/%M0/%D-%h:%m:%s%z"	},
	{ L"Global",	L"MessageStorePath",			L""							},
	{ L"Global",	L"ReplyTo",						L""							},
	{ L"Global",	L"SenderAddress",				L""							},
	{ L"Global",	L"SenderName",					L""							},
	{ L"Global", 	L"ShowUnseenCountOnWelcome",	L"1"						},
	{ L"Global",	L"SslOption",					L"0"						},
	{ L"Global",	L"StoreDecodedMessage",			L"0"						},
	{ L"Global",	L"SubAccount",					L""							},
	{ L"Global",	L"Timeout",						L"60"						},
	{ L"Global",	L"TransferEncodingFor8Bit",		L""							},
	{ L"Global",	L"TreatAsSent",					L"1"						},
	
	{ L"Http",	L"ProxyHost",			L""		},
	{ L"Http",	L"ProxyPassword",		L""		},
	{ L"Http",	L"ProxyPort",			L"8080"	},
	{ L"Http",	L"ProxyUserName",		L""		},
	{ L"Http",	L"UseInternetSetting",	L"0"	},
	{ L"Http",	L"UseProxy",			L"0"	},
	
	{ L"Imap4",	L"AdditionalFields", 	L""			},
	{ L"Imap4",	L"AuthMethods",			L""			},
	{ L"Imap4",	L"CloseFolder",			L"0"		},
	{ L"Imap4",	L"DraftboxFolder",		L"Outbox"	},
	{ L"Imap4",	L"FetchCount",			L"100"		},
	{ L"Imap4",	L"ForceDisconnect",		L"0"		},
	{ L"Imap4",	L"JunkFolder",			L"Junk"		},
	{ L"Imap4",	L"MaxSession",			L"5"		},
	{ L"Imap4",	L"Option",				L"255"		},
	{ L"Imap4",	L"OutboxFolder",		L"Outbox"	},
	{ L"Imap4",	L"Reselect",			L"1"		},
	{ L"Imap4",	L"RootFolder",			L""			},
	{ L"Imap4",	L"RootFolderSeparator",	L"/"		},
/*	{ L"Imap4",	L"SearchCharset",		L""			},*/
	{ L"Imap4",	L"SearchUseCharset",	L"1"		},
	{ L"Imap4",	L"SentboxFolder",		L"Sentbox"	},
	{ L"Imap4",	L"TrashFolder",			L"Trash"	},
	{ L"Imap4",	L"UseNamespace",		L"0"		},
	{ L"Imap4",	L"UseOthers",			L"1"		},
	{ L"Imap4",	L"UsePersonal",			L"1"		},
	{ L"Imap4",	L"UseShared",			L"1"		},
	
#ifndef _WIN32_WCE
	{ L"JunkFilter",	L"Enabled",	L"0"	},
#endif
	
	{ L"Misc",	L"IgnoreError",	L"0"	},
	
	{ L"Nntp",	L"ForceDisconnect",		L"0"	},
	{ L"Nntp",	L"InitialFetchCount",	L"300"	},
	{ L"Nntp",	L"UseXOVER",			L"1"	},
	{ L"Nntp",	L"XOVERStep",			L"100"	},
	
	{ L"Pop3",	L"Apop",				L"0"	},
	{ L"Pop3",	L"DeleteBefore",		L"0"	},
	{ L"Pop3",	L"DeleteLocal",			L"0"	},
	{ L"Pop3",	L"DeleteOnServer",		L"0"	},
	{ L"Pop3",	L"GetAll",				L"20"	},
	{ L"Pop3",	L"HandleStatus",		L"0"	},
	{ L"Pop3",	L"SkipDuplicatedUID",	L"0"	},
	
	{ L"Pop3Send",	L"Apop",	L"0"	},
	
	{ L"Receive",	L"Host",			L""		},
	{ L"Receive",	L"Log",				L"0"	},
	{ L"Receive",	L"Port",			L"0"	},
	{ L"Receive",	L"Secure",			L"0"	},
	{ L"Receive",	L"SyncFilterName",	L""		},
	{ L"Receive",	L"Type",			L""		},
	{ L"Receive",	L"UserName",		L""		},
	
	{ L"Rss",	L"KeepDay",	L"7"	},
	
	{ L"Send",	L"Host",		L""		},
	{ L"Send",	L"Log",			L"0"	},
	{ L"Send",	L"Port",		L"0"	},
	{ L"Send",	L"Secure",		L"0"	},
	{ L"Send",	L"Type",		L""		},
	{ L"Send",	L"UserName",	L""		},
	
	{ L"Smtp",	L"AuthMethods",				L""		},
	{ L"Smtp",	L"EnvelopeFrom",			L""		},
	{ L"Smtp",	L"LocalHost",				L""		},
	{ L"Smtp",	L"PopBeforeSmtp",			L"0"	},
	{ L"Smtp",	L"PopBeforeSmtpApop",		L"0"	},
	{ L"Smtp",	L"PopBeforeSmtpCustom",		L"0"	},
	{ L"Smtp",	L"PopBeforeSmtpHost",		L""		},
	{ L"Smtp",	L"PopBeforeSmtpPort",		L"110"	},
	{ L"Smtp",	L"PopBeforeSmtpProtocol",	L"pop3"	},
	{ L"Smtp",	L"PopBeforeSmtpSecure",		L"0"	},
	{ L"Smtp",	L"PopBeforeSmtpWait",		L"3"	},
	
	{ L"UI",	L"FolderTo",	L""	},
};

}

#endif // __DEFAULTPROFILE_H__
