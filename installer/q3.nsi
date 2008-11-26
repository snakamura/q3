; $Id$

!define VC8DIR "C:\Program Files\Microsoft Visual Studio 8\VC"
!define DISTDIR "..\dist"

!ifdef ANSI
  !define CODE ansi
  !define SUFFIX
!else
  !define CODE unicode
  !define SUFFIX u
!endif
!ifdef X64
  !define CPU x64
  !define CRTBASE amd64
  !define PF $PROGRAMFILES64
!else
  !define CPU x86
  !define CRTBASE x86
  !define PF $PROGRAMFILES
!endif

!include "MUI.nsh"
  
Name "QMAIL3"

OutFile "${DISTDIR}\q3${SUFFIX}-win-${CPU}-ja.exe"
XPStyle on

Var STARTMENU_FOLDER
Var MAILBOX_FOLDER
Var REMOVE_MAILBOX

InstallDir "${PF}\QMAIL3"
InstallDirRegKey HKLM "SOFTWARE\sn\q3" "InstallDir"

;Page components
;Page directory
;Page instfiles
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
Page custom MailBoxFolder
!define MUI_STARTMENUPAGE_REGISTRY_ROOT "HKLM" 
!define MUI_STARTMENUPAGE_REGISTRY_KEY "Software\sn\q3" 
!define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "StartMenuFolder"
!insertmacro MUI_PAGE_STARTMENU StartMenu $STARTMENU_FOLDER
!insertmacro MUI_PAGE_INSTFILES
!define MUI_FINISHPAGE_RUN $INSTDIR\q3${SUFFIX}.exe
!insertmacro MUI_PAGE_FINISH

;UninstPage uninstConfirm
;UninstPage instfiles
!insertmacro MUI_UNPAGE_WELCOME
UninstPage custom un.RemoveMailBox
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

!insertmacro MUI_LANGUAGE "English"
!insertmacro MUI_LANGUAGE "Japanese"

Section "Core (required)" Core

  SectionIn RO
  
  SetOutPath $INSTDIR
  
  File ..\bin\win\${CPU}\${CODE}\release\q3${SUFFIX}.exe
  File ..\bin\win\${CPU}\${CODE}\release\qm${SUFFIX}.dll
  File ..\bin\win\${CPU}\${CODE}\release\qs${SUFFIX}.dll
  File ..\bin\win\${CPU}\${CODE}\release\qmpop3${SUFFIX}.dll
  File ..\bin\win\${CPU}\${CODE}\release\qmsmtp${SUFFIX}.dll
!ifndef ANSI
  File "${VC8DIR}\redist\${CRTBASE}\Microsoft.VC80.CRT\msvcr80.dll"
  File "${VC8DIR}\redist\${CRTBASE}\Microsoft.VC80.CRT\msvcp80.dll"
  File "${VC8DIR}\redist\${CRTBASE}\Microsoft.VC80.CRT\Microsoft.VC80.CRT.manifest"
!endif
  File ..\lib\stlport\lib\win\${CPU}\stlport.5.2.dll
  File ..\LICENSE
  File ..\misc\THIRDPARTYLICENSE
  File ..\misc\README.en.txt
  File ..\misc\README.ja.txt
  
!ifdef X64
  SetRegView 64
!endif
  
  WriteRegStr HKCU "SOFTWARE\sn\q3\Setting" "MailFolder" "$MAILBOX_FOLDER"
  CreateDirectory "$MAILBOX_FOLDER"
  
  WriteRegStr HKLM "SOFTWARE\sn\q3" "InstallDir" "$INSTDIR"
  
  WriteRegStr HKLM "SOFTWARE\RegisteredApplications" "QMAIL3" "Software\Clients\Mail\QMAIL3\Capabilities"
  WriteRegStr HKLM "SOFTWARE\Clients\Mail\QMAIL3" "" "QMAIL3"
  WriteRegStr HKLM "SOFTWARE\Clients\Mail\QMAIL3\Protocols\mailto" "" "URL:MailTo Protocol"
  WriteRegBin HKLM "SOFTWARE\Clients\Mail\QMAIL3\Protocols\mailto" "EditFlags" 02000000
  WriteRegStr HKLM "SOFTWARE\Clients\Mail\QMAIL3\Protocols\mailto" "URL Protocol" ""
  WriteRegStr HKLM "SOFTWARE\Clients\Mail\QMAIL3\Protocols\mailto\DefaultIcon" "" "$\"$INSTDIR\q3${SUFFIX}.exe$\",0"
  WriteRegStr HKLM "SOFTWARE\Clients\Mail\QMAIL3\Protocols\mailto\shell\open\command" "" "$\"$INSTDIR\q3${SUFFIX}.exe$\" -s $\"%1$\""
  WriteRegStr HKLM "SOFTWARE\Clients\Mail\QMAIL3\shell\open\command" "" "$\"$INSTDIR\q3${SUFFIX}.exe$\""
  WriteRegStr HKLM "SOFTWARE\Clients\Mail\QMAIL3\Capabilities" "ApplicationName" "QMAIL3"
  WriteRegStr HKLM "SOFTWARE\Clients\Mail\QMAIL3\Capabilities" "ApplicationDescription" ""
  WriteRegStr HKLM "SOFTWARE\Clients\Mail\QMAIL3\Capabilities\FileAssociations" ".eml" "QMAIL3"
  WriteRegStr HKLM "SOFTWARE\Clients\Mail\QMAIL3\Capabilities\URLAssociations" "mailto" "QMAIL3.Url.mailto"
  WriteRegStr HKLM "SOFTWARE\Clients\Mail\QMAIL3\Capabilities\StartMenu" "Mail" "QMAIL3"
  
  WriteRegStr HKCR "QMAIL3\shell\open\command" "" "$\"$INSTDIR\q3${SUFFIX}.exe$\" -o $\"%1$\""
  WriteRegStr HKCR "QMAIL3.Url.mailto" "" "URL:MailTo Protocol"
  WriteRegBin HKCR "QMAIL3.Url.mailto" "EditFlags" 02000000
  WriteRegStr HKCR "QMAIL3.Url.mailto" "URL Protocol" ""
  WriteRegStr HKCR "QMAIL3.Url.mailto\DefaultIcon" "" "$\"$INSTDIR\q3${SUFFIX}.exe$\",0"
  WriteRegStr HKCR "QMAIL3.Url.mailto\shell\open\command" "" "$\"$INSTDIR\q3${SUFFIX}.exe$\" -s $\"%1$\""
  
  WriteRegStr HKCR "Microsoft Internet Mail Message\shell\Open_with_QMAIL3" "" "$(OPEN_WITH_QMAIL3)"
  WriteRegStr HKCR "Microsoft Internet Mail Message\shell\Open_with_QMAIL3\command" "" "$\"$INSTDIR\q3${SUFFIX}.exe$\" -o $\"%1$\""
  
  WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\QMAIL3" "DisplayName" "QMAIL3"
  WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\QMAIL3" "UninstallString" "$INSTDIR\uninstall.exe"
  WriteRegDWORD HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\QMAIL3" "NoModify" 1
  WriteRegDWORD HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\QMAIL3" "NoRepair" 1
  
  WriteUninstaller "$INSTDIR\uninstall.exe"
  
  !insertmacro MUI_STARTMENU_WRITE_BEGIN StartMenu
    CreateDirectory "$SMPROGRAMS\$STARTMENU_FOLDER"
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\Uninstall.lnk" "$INSTDIR\Uninstall.exe"
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\QMAIL3.lnk" "$INSTDIR\q3${SUFFIX}.exe" "" "$INSTDIR\q3${SUFFIX}.exe" 0
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\$(DOCUMENT).lnk" "http://q3.snak.org/doc/"
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\FAQ.lnk" "http://q3.snak.org/doc/FAQ.html"
  !insertmacro MUI_STARTMENU_WRITE_END

SectionEnd


Section "IMAP4" Imap4

  SetOutPath $INSTDIR
  
  File ..\bin\win\${CPU}\${CODE}\release\qmimap4${SUFFIX}.dll
  
SectionEnd


Section "NNTP" Nntp

  SetOutPath $INSTDIR
  
  File ..\bin\win\${CPU}\${CODE}\release\qmnntp${SUFFIX}.dll
  
SectionEnd


Section "RSS, Atom" Rss

  SetOutPath $INSTDIR
  
  File ..\bin\win\${CPU}\${CODE}\release\qmrss${SUFFIX}.dll
  
SectionEnd


Section "SSL, S/MIME" Crypto

  SetOutPath $INSTDIR
  
  File ..\bin\win\${CPU}\${CODE}\release\qscrypto${SUFFIX}.dll
  File ..\lib\openssl\lib\win\${CPU}\libeay32.dll
  File ..\lib\openssl\lib\win\${CPU}\ssleay32.dll
  
SectionEnd


Section "PGP, GnuPG" Pgp

  SetOutPath $INSTDIR
  
  File ..\bin\win\${CPU}\${CODE}\release\qmpgp${SUFFIX}.dll
  
SectionEnd


Section "Junk Filter" Junk

  SetOutPath $INSTDIR
  
  File ..\bin\win\${CPU}\${CODE}\release\qmjunk${SUFFIX}.dll
  File ..\lib\qdbm\lib\win\${CPU}\qdbm.dll
  
SectionEnd


Section "Script" Script

  SetOutPath $INSTDIR
  
  File ..\bin\win\${CPU}\${CODE}\release\qmscript${SUFFIX}.dll
  
SectionEnd


Section "Zip" Zip

  SetOutPath $INSTDIR
  
  File ..\lib\zip\lib\win\${CPU}\zip32.dll
  
SectionEnd


Section "Japanese" Japanese

  SetOutPath $INSTDIR
  
  File ..\bin\win\${CPU}\${CODE}\release\qm${SUFFIX}.dll.0411.mui
  File ..\bin\win\${CPU}\${CODE}\release\qs${SUFFIX}.dll.0411.mui
  File ..\bin\win\${CPU}\${CODE}\release\qmpop3${SUFFIX}.dll.0411.mui
  File ..\bin\win\${CPU}\${CODE}\release\qmsmtp${SUFFIX}.dll.0411.mui
  File ..\bin\win\${CPU}\${CODE}\release\qmimap4${SUFFIX}.dll.0411.mui
  File ..\bin\win\${CPU}\${CODE}\release\qmnntp${SUFFIX}.dll.0411.mui
  File ..\bin\win\${CPU}\${CODE}\release\qmrss${SUFFIX}.dll.0411.mui
  
SectionEnd


Section "Uninstall"
  
!ifdef X64
  SetRegView 64
!endif
  
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\QMAIL3"  
  !insertmacro MUI_STARTMENU_GETFOLDER StartMenu $R0
  Delete "$SMPROGRAMS\$R0\QMAIL3.lnk"
  Delete "$SMPROGRAMS\$R0\Uninstall.lnk"
  StrCpy $R0 "$SMPROGRAMS\$R0"
  startMenuDeleteLoop:
    ClearErrors
    RMDir $R0
    GetFullPathName $R0 "$R0\.."
    
    IfErrors startMenuDeleteLoopDone
    
    StrCmp $R0 $SMPROGRAMS startMenuDeleteLoopDone startMenuDeleteLoop
  startMenuDeleteLoopDone:
  
  ReadRegStr $MAILBOX_FOLDER HKCU "Software\sn\q3\Setting" "MailFolder"
  StrCmp $MAILBOX_FOLDER "" +3
    IntCmp $REMOVE_MAILBOX 0 +2
      RMDir /r $MAILBOX_FOLDER
  
  DeleteRegKey HKLM "SOFTWARE\sn\q3"
  DeleteRegKey /ifempty HKLM "SOFTWARE\sn"
  DeleteRegKey HKCU "Software\sn\q3"
  DeleteRegKey /ifempty HKCU "Software\sn"
  
  DeleteRegKey HKCR "QMAIL3"
  DeleteRegKey HKCR "QMAIL3.Url.mailto"
  
  DeleteRegKey HKLM "SOFTWARE\Clients\Mail\QMAIL3"
  DeleteRegValue HKLM "SOFTWARE\RegisteredApplications" "QMAIL3"
  
  Delete $INSTDIR\q3${SUFFIX}.exe
  Delete $INSTDIR\qm${SUFFIX}.dll
  Delete $INSTDIR\qmpop3${SUFFIX}.dll
  Delete $INSTDIR\qmimap4${SUFFIX}.dll
  Delete $INSTDIR\qmsmtp${SUFFIX}.dll
  Delete $INSTDIR\qmnntp${SUFFIX}.dll
  Delete $INSTDIR\qmrss${SUFFIX}.dll
  Delete $INSTDIR\qmpgp${SUFFIX}.dll
  Delete $INSTDIR\qmjunk${SUFFIX}.dll
  Delete $INSTDIR\qmscript${SUFFIX}.dll
  Delete $INSTDIR\qs${SUFFIX}.dll
  Delete $INSTDIR\qscrypto${SUFFIX}.dll
  Delete $INSTDIR\libeay32.dll
  Delete $INSTDIR\ssleay32.dll
  Delete $INSTDIR\qdbm.dll
  Delete $INSTDIR\zip32.dll
  Delete $INSTDIR\qm${SUFFIX}.dll.0411.mui
  Delete $INSTDIR\qs${SUFFIX}.dll.0411.mui
  Delete $INSTDIR\qmpop3${SUFFIX}.dll.0411.mui
  Delete $INSTDIR\qmsmtp${SUFFIX}.dll.0411.mui
  Delete $INSTDIR\qmimap4${SUFFIX}.dll.0411.mui
  Delete $INSTDIR\qmnntp${SUFFIX}.dll.0411.mui
  Delete $INSTDIR\qmrss${SUFFIX}.dll.0411.mui
!ifndef ANSI
  Delete $INSTDIR\msvcr80.dll
  Delete $INSTDIR\msvcp80.dll
  Delete $INSTDIR\Microsoft.VC80.CRT.manifest
!endif
  Delete $INSTDIR\stlport.5.2.dll
  Delete $INSTDIR\LICENSE
  Delete $INSTDIR\THIRDPARTYLICENSE
  Delete $INSTDIR\README.en.txt
  Delete $INSTDIR\README.ja.txt
  Delete $INSTDIR\uninstall.exe
  
  RMDir "$INSTDIR"

SectionEnd


Function .onInit

  !insertmacro MUI_INSTALLOPTIONS_EXTRACT "mailbox.ini"

FunctionEnd


Function MailBoxFolder

  !insertmacro MUI_HEADER_TEXT "$(MAILBOXFOLDER_TITLE)" "$(MAILBOXFOLDER_SUBTITLE)"
  !insertmacro MUI_INSTALLOPTIONS_WRITE "mailbox.ini" "Field 1" "Text" "$(MAILBOXFOLDER_LABEL)"
  ReadRegStr $MAILBOX_FOLDER HKCU "Software\sn\q3\Setting" "MailFolder"
  StrCmp $MAILBOX_FOLDER "" +1 +2
    StrCpy $MAILBOX_FOLDER "$APPDATA\QMAIL3"
  !insertmacro MUI_INSTALLOPTIONS_WRITE "mailbox.ini" "Field 2" "State" "$MAILBOX_FOLDER"
  !insertmacro MUI_INSTALLOPTIONS_DISPLAY "mailbox.ini"
  !insertmacro MUI_INSTALLOPTIONS_READ $MAILBOX_FOLDER "mailbox.ini" "Field 2" "State"

FunctionEnd


Function un.onInit

  !insertmacro MUI_INSTALLOPTIONS_EXTRACT "remove.ini"

FunctionEnd


Function un.RemoveMailBox

  !insertmacro MUI_HEADER_TEXT "$(REMOVEMAILBOX_TITLE)" "$(REMOVEMAILBOX_SUBTITLE)"
  !insertmacro MUI_INSTALLOPTIONS_WRITE "remove.ini" "Field 1" "Text" "$(REMOVEMAILBOX_LABEL)"
  !insertmacro MUI_INSTALLOPTIONS_DISPLAY "remove.ini"
  !insertmacro MUI_INSTALLOPTIONS_READ $REMOVE_MAILBOX "remove.ini" "Field 1" "State"

FunctionEnd


LangString DESC_SecCore ${LANG_ENGLISH} "Core module includes POP3/SMTP support."
LangString DESC_SecImap4 ${LANG_ENGLISH} "IMAP4 support."
LangString DESC_SecNntp ${LANG_ENGLISH} "Netnews (NNTP) support."
LangString DESC_SecRss ${LANG_ENGLISH} "RSS and Atom support."
LangString DESC_SecCrypto ${LANG_ENGLISH} "SSL and S/MIME support."
LangString DESC_SecPgp ${LANG_ENGLISH} "PGP and GnuPG support."
LangString DESC_SecJunk ${LANG_ENGLISH} "Junk filter support."
LangString DESC_SecScript ${LANG_ENGLISH} "Script support."
LangString DESC_SecZip ${LANG_ENGLISH} "Enable archiving attachments."
LangString DESC_SecJapanese ${LANG_ENGLISH} "Japanese UI support."
LangString DOCUMENT ${LANG_ENGLISH} "Document"
LangString MAILBOXFOLDER_TITLE ${LANG_ENGLISH} "Choose Mailbox Location"
LangString MAILBOXFOLDER_SUBTITLE ${LANG_ENGLISH} "Choose the folder in which to store mail data."
LangString MAILBOXFOLDER_LABEL ${LANG_ENGLISH} "Mailbox folder"
LangString REMOVEMAILBOX_TITLE ${LANG_ENGLISH} "Remove Mailbox"
LangString REMOVEMAILBOX_SUBTITLE ${LANG_ENGLISH} "Choose whether you want to remove the folder in which mail data are stored."
LangString REMOVEMAILBOX_LABEL ${LANG_ENGLISH} "Remove mail data"
LangString OPEN_WITH_QMAIL3 ${LANG_ENGLISH} "Open with &QMAIL3"

LangString DESC_SecCore ${LANG_JAPANESE} "POP3/SMTPのサポートを含むコアモジュール"
LangString DESC_SecImap4 ${LANG_JAPANESE} "IMAP4のサポート"
LangString DESC_SecNntp ${LANG_JAPANESE} "ネットニュース(NNTP)のサポート"
LangString DESC_SecRss ${LANG_JAPANESE} "RSSとAtomのサポート"
LangString DESC_SecCrypto ${LANG_JAPANESE} "SSLとS/MIMEのサポート"
LangString DESC_SecPgp ${LANG_JAPANESE} "PGPとGnuPGのサポート"
LangString DESC_SecJunk ${LANG_JAPANESE} "スパムフィルタのサポート"
LangString DESC_SecScript ${LANG_JAPANESE} "スクリプトのサポート"
LangString DESC_SecZip ${LANG_JAPANESE} "添付ファイルの圧縮のサポート"
LangString DESC_SecJapanese ${LANG_JAPANESE} "日本語UIのサポート"
LangString DOCUMENT ${LANG_JAPANESE} "ドキュメント"
LangString MAILBOXFOLDER_TITLE ${LANG_JAPANESE} "メールボックスの場所を選んでください"
LangString MAILBOXFOLDER_SUBTITLE ${LANG_JAPANESE} "メールのデータを置くフォルダを選択してください。"
LangString MAILBOXFOLDER_LABEL ${LANG_JAPANESE} "メールボックスフォルダ"
LangString REMOVEMAILBOX_TITLE ${LANG_JAPANESE} "メールボックスの削除"
LangString REMOVEMAILBOX_SUBTITLE ${LANG_JAPANESE} "メールのデータが置いてあるフォルダを削除するかどうかを指定してください。"
LangString REMOVEMAILBOX_LABEL ${LANG_JAPANESE} "メールボックスを削除"
LangString OPEN_WITH_QMAIL3 ${LANG_JAPANESE} "&QMAIL3で開く"

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${Core} $(DESC_SecCore)
  !insertmacro MUI_DESCRIPTION_TEXT ${Imap4} $(DESC_SecImap4)
  !insertmacro MUI_DESCRIPTION_TEXT ${Nntp} $(DESC_SecNntp)
  !insertmacro MUI_DESCRIPTION_TEXT ${Rss} $(DESC_SecRss)
  !insertmacro MUI_DESCRIPTION_TEXT ${Crypto} $(DESC_SecCrypto)
  !insertmacro MUI_DESCRIPTION_TEXT ${Pgp} $(DESC_SecPgp)
  !insertmacro MUI_DESCRIPTION_TEXT ${Junk} $(DESC_SecJunk)
  !insertmacro MUI_DESCRIPTION_TEXT ${Script} $(DESC_SecScript)
  !insertmacro MUI_DESCRIPTION_TEXT ${Zip} $(DESC_SecZip)
  !insertmacro MUI_DESCRIPTION_TEXT ${Japanese} $(DESC_SecJapanese)
!insertmacro MUI_FUNCTION_DESCRIPTION_END
