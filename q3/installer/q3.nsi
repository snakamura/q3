; $Id$

!ifdef ANSI
  !define CODE ansi
  !define POSTFIX
!else
  !define CODE unicode
  !define POSTFIX u
!endif

!include "MUI.nsh"
  
Name "QMAIL3"

OutFile "qmail3${POSTFIX}.exe"
XPStyle on

Var STARTMENU_FOLDER
Var MAILBOX_FOLDER

InstallDir "$PROGRAMFILES\QMAIL3"
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
!define MUI_FINISHPAGE_RUN $INSTDIR\q3${POSTFIX}.exe
!insertmacro MUI_PAGE_FINISH

;UninstPage uninstConfirm
;UninstPage instfiles
!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

!insertmacro MUI_LANGUAGE "English"

Section "Core (required)" Core

  SectionIn RO
  
  SetOutPath $INSTDIR
  
  File ..\bin\win\${CODE}\release\q3${POSTFIX}.exe
  File ..\bin\win\${CODE}\release\qm${POSTFIX}.dll
  File ..\bin\win\${CODE}\release\qs${POSTFIX}.dll
  File ..\bin\win\${CODE}\release\qmpop3${POSTFIX}.dll
  File ..\bin\win\${CODE}\release\qmsmtp${POSTFIX}.dll
  
  WriteRegStr HKCU "SOFTWARE\sn\q3\Setting" "MailFolder" "$MAILBOX_FOLDER"
  
  WriteRegStr HKLM "SOFTWARE\sn\q3" "InstallDir" "$INSTDIR"
  
  WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\QMAIL3" "DisplayName" "QMAIL3"
  WriteRegStr HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\QMAIL3" "UninstallString" "$INSTDIR\uninstall.exe"
  WriteRegDWORD HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\QMAIL3" "NoModify" 1
  WriteRegDWORD HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\QMAIL3" "NoRepair" 1
  
  WriteUninstaller "$INSTDIR\uninstall.exe"
  
  !insertmacro MUI_STARTMENU_WRITE_BEGIN StartMenu
    CreateDirectory "$SMPROGRAMS\$STARTMENU_FOLDER"
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\Uninstall.lnk" "$INSTDIR\Uninstall.exe"
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\QMAIL3.lnk" "$INSTDIR\q3${POSTFIX}.exe" "" "$INSTDIR\q3${POSTFIX}.exe" 0
  !insertmacro MUI_STARTMENU_WRITE_END

SectionEnd


Section "IMAP4" Imap4

  SetOutPath $INSTDIR
  
  File ..\bin\win\${CODE}\release\qmimap4${POSTFIX}.dll
  
SectionEnd


Section "NNTP" Nntp

  SetOutPath $INSTDIR
  
  File ..\bin\win\${CODE}\release\qmnntp${POSTFIX}.dll
  
SectionEnd


Section "RSS, Atom" Rss

  SetOutPath $INSTDIR
  
  File ..\bin\win\${CODE}\release\qmrss${POSTFIX}.dll
  
SectionEnd


Section "SSL, S/MIME" Crypto

  SetOutPath $INSTDIR
  
  File ..\bin\win\${CODE}\release\qscrypto${POSTFIX}.dll
  File ..\lib\openssl\lib\win\libeay32.dll
  File ..\lib\openssl\lib\win\ssleay32.dll
  
SectionEnd


Section "PGP, GnuPG" Pgp

  SetOutPath $INSTDIR
  
  File ..\bin\win\${CODE}\release\qmpgp${POSTFIX}.dll
  
SectionEnd


Section "Junk Filter" Junk

  SetOutPath $INSTDIR
  
  File ..\bin\win\${CODE}\release\qmjunk${POSTFIX}.dll
  
SectionEnd


Section "Script" Script

  SetOutPath $INSTDIR
  
  File ..\bin\win\${CODE}\release\qmscript${POSTFIX}.dll
  
SectionEnd


Section "Uninstall"
  
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
  
  DeleteRegKey HKLM SOFTWARE\sn\q3
  DeleteRegKey /ifempty HKLM SOFTWARE\sn
  DeleteRegKey HKCU Software\sn\q3
  DeleteRegKey /ifempty HKCU Software\sn
  
  Delete $INSTDIR\q3${POSTFIX}.exe
  Delete $INSTDIR\qm${POSTFIX}.dll
  Delete $INSTDIR\qmpop3${POSTFIX}.dll
  Delete $INSTDIR\qmimap4${POSTFIX}.dll
  Delete $INSTDIR\qmsmtp${POSTFIX}.dll
  Delete $INSTDIR\qmnntp${POSTFIX}.dll
  Delete $INSTDIR\qmrss${POSTFIX}.dll
  Delete $INSTDIR\qmpgp${POSTFIX}.dll
  Delete $INSTDIR\qmjunk${POSTFIX}.dll
  Delete $INSTDIR\qmscript${POSTFIX}.dll
  Delete $INSTDIR\qs${POSTFIX}.dll
  Delete $INSTDIR\qscrypto${POSTFIX}.dll
  Delete $INSTDIR\libeay32.dll
  Delete $INSTDIR\ssleay32.dll
  Delete $INSTDIR\uninstall.exe
  
  RMDir "$INSTDIR"

SectionEnd


Function .onInit

  !insertmacro MUI_INSTALLOPTIONS_EXTRACT "mailbox.ini"

FunctionEnd


Function MailBoxFolder

  !insertmacro MUI_HEADER_TEXT "Choose Mailbox Location" "Choose the folder in which to store mail data."
  ReadRegStr $MAILBOX_FOLDER HKCU "Software\sn\q3\Setting" "MailFolder"
  StrCmp $MAILBOX_FOLDER "" +2
    GoTo +2
    StrCpy $MAILBOX_FOLDER "$APPDATA\mail"
  !insertmacro MUI_INSTALLOPTIONS_WRITE "mailbox.ini" "Field 2" "State" "$MAILBOX_FOLDER"
  !insertmacro MUI_INSTALLOPTIONS_DISPLAY "mailbox.ini"
  !insertmacro MUI_INSTALLOPTIONS_READ $MAILBOX_FOLDER "mailbox.ini" "Field 2" "State"

FunctionEnd


LangString DESC_SecCore ${LANG_ENGLISH} "Core module includes POP3/SMTP support."
LangString DESC_SecImap4 ${LANG_ENGLISH} "IMAP4 support."
LangString DESC_SecNntp ${LANG_ENGLISH} "Netnews (NNTP) support."
LangString DESC_SecRss ${LANG_ENGLISH} "RSS and Atom support."
LangString DESC_SecCrypto ${LANG_ENGLISH} "SSL and S/MIME support."
LangString DESC_SecPgp ${LANG_ENGLISH} "PGP and GnuPG support."
LangString DESC_SecJunk ${LANG_ENGLISH} "Junk filter support."
LangString DESC_SecScript ${LANG_ENGLISH} "Script support."

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${Core} $(DESC_SecCore)
  !insertmacro MUI_DESCRIPTION_TEXT ${Imap4} $(DESC_SecImap4)
  !insertmacro MUI_DESCRIPTION_TEXT ${Nntp} $(DESC_SecNntp)
  !insertmacro MUI_DESCRIPTION_TEXT ${Rss} $(DESC_SecRss)
  !insertmacro MUI_DESCRIPTION_TEXT ${Crypto} $(DESC_SecCrypto)
  !insertmacro MUI_DESCRIPTION_TEXT ${Pgp} $(DESC_SecPgp)
  !insertmacro MUI_DESCRIPTION_TEXT ${Junk} $(DESC_SecJunk)
  !insertmacro MUI_DESCRIPTION_TEXT ${Script} $(DESC_SecScript)
!insertmacro MUI_FUNCTION_DESCRIPTION_END
