=begin
=qmail.xml

QMAIL3�S�̂Ɋւ���ݒ��ۑ�����XML�t�@�C���ł��B���̃t�@�C���Őݒ�ł��鑽���̍��ڂ�((<�I�v�V�����̐ݒ�|URL:Options.html>))�ȂǂŐݒ�ł��܂����A�ꕔ�̍��ڂ͒��ڂ��̃t�@�C����ҏW���Ȃ��Ɛݒ�ł��܂���B�ݒ�ł��鍀�ڂ̈ꗗ�͔��l���Q�Ƃ��Ă��������B

���̃t�@�C����ҏW����Ƃ��ɂ�QMAIL3���I�������Ă���ҏW���Ă��������B


==����

===profile�G�������g

 <profile>
  <!-- section -->
 </profile>

profile�G�������g���g�b�v���x���G�������g�ɂȂ�܂��Bprofile�G�������g�ȉ��ɂ�0�ȏ��section�G�������g��u�����Ƃ��ł��܂��B


===section�G�������g

 <section
  name="���O">
  <!-- key -->
 </filter>

section�G�������g�͈�̃Z�N�V������\���܂��Bname�����ɂ̓Z�N�V�����̖��O���w�肵�܂��B


===key�G�������g

 <key
  name="���O">
  �l
 </key>

key�G�������g�͈�̃L�[��\���܂��Bname�����ɂ̓L�[�̖��O���w�肵�܂��B�q�m�[�h�Ƃ��Ă��̃L�[�̒l���w�肵�܂��B


==�T���v��

 <?xml version="1.0" encoding="utf-8"?>
 <profile>
  <section name="AddressBookFrameWindow">
   <key name="Height">552</key>
   <key name="Left">856</key>
   <key name="Top">97</key>
   <key name="Width">611</key>
  </section>
  <section name="EditFrameWindow">
   <key name="Height">565</key>
   <key name="Left">175</key>
   <key name="Top">558</key>
   <key name="Width">627</key>
  </section>
  <section name="Find">
   <key name="History0">Test</key>
  </section>
  <section name="FolderWindow">
   <key name="ExpandedFolders">//Main //News //RSS //Sub //Sub/Inbox</key>
  </section>
  <section name="Global">
   <key name="CurrentFolder">//Main/�e�X�g</key>
   <key name="DetachFolder">C:\Temp</key>
   <key name="LastUpdateCheck">2006-08-13T20:40:26+09:00</key>
   <key name="Offline">0</key>
  </section>
  <section name="HeaderEditWindow">
   <key name="ImeFollowup-To">0</key>
   <key name="ImeNewsgroups">0</key>
  </section>
  <section name="MainWindow">
   <key name="Height">849</key>
   <key name="Left">159</key>
   <key name="PrimaryLocation">187</key>
   <key name="Top">79</key>
   <key name="Width">764</key>
  </section>
  <section name="MessageFrameWindow">
   <key name="Height">711</key>
   <key name="Left">682</key>
   <key name="Top">426</key>
   <key name="Width">811</key>
  </section>
  <section name="OptionDialog">
   <key name="Panel">20</key>
  </section>
  <section name="RecentAddress">
   <key name="Address0">Taro Yamada &lt;taro@example.org></key>
  </section>
  <section name="SyncDialog">
   <key name="Left">609</key>
   <key name="Top">845</key>
  </section>
 </profile>


==�X�L�[�}

 element profile {
   element section {
     element key {
       ## �l
       xsd:string,
       ## �L�[�̖��O
       attribute name {
         xsd:string
       }
     }*,
     ## �Z�N�V�����̖��O
     attribute name {
       xsd:string
     }
   }*
 }


==���l
���̃t�@�C���ł̓Z�N�V�����ƃL�[�Œl���w�肵�܂��B�Ⴆ�΁A��̗�ł�Global�Z�N�V������DetachFolder�L�[��C:\Temp�Ƃ����l���w�肳��Ă��܂��B���̃h�L�������g���ł͂����Global/DetachFolder�̂悤�ɋL�q���Ă��邱�Ƃ�����܂��B�Ⴆ�΁A�u�L������A�h���X�̌���((<qmail.xml|URL:QmailXml.html>))��RecentAddress/Max�Ŏw��ł��܂��B�v�̂悤�ɏ�����Ă�����ARecentAddress�Z�N�V������Max�L�[�Ŏw�肷��Ƃ������ƂɂȂ�܂��B

���ꂼ��̃L�[�̓f�t�H���g�̒l�������Ă��āA�w�肳��Ă��Ȃ��ꍇ�ɂ͂��̒l���g�p����܂��B�܂��A�l���f�t�H���g�̒l�Ɠ����ꍇ�ɂ̓t�@�C���ɂ͏����o����܂���B���݂��Ȃ��L�[�̒l���w�肷��ꍇ�ɂ́A�V�����Z�N�V������L�[��ǉ����Ă��������B

�w��ł���Z�N�V�����ƃL�[�͈ȉ��̒ʂ�ł��B


===AddressBook�Z�N�V����

+AddressOnly (0 @ 0|1)
�O���A�h���X������A�h���X����荞�ނƂ��ɖ��O����荞�܂����[���A�h���X��������荞�ނ��ǂ����B


+Category
�A�h���X�I���_�C�A���O�Ō��ݑI������Ă���J�e�S���B


+Externals ( @ WAB, Outlook, PocketOutlook)
��荞�ފO���A�h���X���B


+AddressWidth (130), NameWidth (120), CommentWidth (60), SelectedAddressWidth (150)
�A�h���X�I���_�C�A���O�̃A�h���X���A���O���A�R�����g���A�I�����ꂽ�A�h���X���̕��B


+Width (620), Height (450)
�A�h���X�I���_�C�A���O�̑傫���B


===AddressBookFrameWindow�Z�N�V����

+Left (0), Top (0), Width (0), Height (0), Show (1), Alpha (0 @ 0-255)
�A�h���X���E�B���h�E�̈ʒu�Ƒ傫���A�\�����@�Ɠ��ߓx�B


+ShowToolbar (1 @ 0|1), ShowStatusBar (1 @ 0|1)
�A�h���X���E�B���h�E�̃c�[���o�[�ƃX�e�[�^�X�o�[��\�����邩�ǂ����B


===AddressBookListWindow�Z�N�V����

+AddressWidth (150), NameWidth (150), CommentWidth (150)
�A�h���X�r���[�̃A�h���X���A���O���A�R�����g���̕��B


+FontFace, FontSize (9), FontStyle (0), FontCharset (0)
�A�h���X�r���[�̃t�H���g�B�t�H���g�T�C�Y�̓|�C���g�B


===AutoPilot�Z�N�V����

+Enabled (0 @ 0|1)
�������񂪗L�����ǂ���


+OnlyWhenConnected
�l�b�g���[�N�ڑ�����Ă���Ƃ��̂ݎ������񂷂邩�ǂ����B


===ColorsDialog�Z�N�V����

+Width (620), Height (450)
�F�̐ݒ�_�C�A���O�̑傫���B


===Dialup�Z�N�V����

+Entry
�Ō�Ɏw�肵���_�C�A���A�b�v�̃G���g�����B


===EditFrameWindow�Z�N�V����

+Left (0), Top (0), Width (0), Height (0), Show (1), Alpha (0 @ 0-255)
�G�f�B�b�g�E�B���h�E�̈ʒu�Ƒ傫���A�\�����@�Ɠ��ߓx�B


+ShowToolbar (1 @ 0|1), ShowStatusBar (1 @ 0|1)
�G�f�B�b�g�E�B���h�E�̃c�[���o�[�ƃX�e�[�^�X�o�[��\�����邩�ǂ����B


===EditWindow�Z�N�V����

+FontFace, FontSize (9), FontStyle (0), FontCharset (0)
�G�f�B�b�g�r���[�̃t�H���g�B�t�H���g�T�C�Y�̓|�C���g�B


+AdjustExtent (0 @ 0|1)
�������𒲐߂��邩�ǂ����B


+UseSystemColor (1 @ 0|1)
�V�X�e���̔z�F���g�����ǂ����B


+ForegroundColor (000000), BackgroundColor (ffffff), LinkColor (0000ff), QuoteColor1 (008000), QuoteColor2 (000080)
�����F�A�w�i�F�A�����N�̐F�A���p�̐F1, 2�B�`����RRGGBB�B


+CharInLine (0)
��s�̕������B0�̏ꍇ�ɂ̓E�B���h�E�̐܂�Ԃ��ʒu�B�w�肵���l�~x�̕������̈ʒu�Ő܂�Ԃ����B


+ClickableURL (1 @ 0|1)
�N���b�J�u��URL���L�����ǂ����B


+DragScrollDelay (300), DragScrollInterval (300)
�h���b�O�őI�𒆂ɃX�N���[������Ƃ��̒x���ƊԊu�B�P�ʂ̓~���b�B


+LineQuote (0 @ 0|1)
���p����ŕ\�����邩�ǂ����B


+LineSpacing (2)
�s�Ԃ̍����B�P�ʂ̓s�N�Z���B


+MarginLeft (10), MarginTop (10), MarginRight (10), MarginBottom (10)
�}�[�W���B�P�ʂ̓s�N�Z���B


+Quote1 (>), Quote2 (#)
���p����1, 2�B���𕶎���Ŏw��B

Quote1�Ŏw�肵�������̂����ꂩ����n�܂�s��QuoteColor1�Ŏw�肵���F�ɁAQuote2�Ŏw�肵�������̂����ꂩ����n�܂�s��QuoteColor2�Ŏw�肵���F�ɂȂ�BLineQuote��1�̏ꍇ�ɂ́AQuote1�Ŏw�肵�������̂����ꂩ����n�܂�s�͐��ŕ\�������B


+ReformLineLength (74)
���`����Ƃ��̈�s�̕������B


+ReformQuote (>|#)
���`����Ƃ��Ɉ��p�Ƃ��Ĉ����镶���B


+ShowCaret (1 @ 0|1), ShowNewLine (1 @ 0|1), ShowTab (1 @ 0|1), ShowRuler (1 @ 0|1), ShowHorizontalScrollBar (0 @ 0|1), ShowVerticalScrollBar (1 @ 0|1)
�L�����b�g�A���s�����A�^�u�A���[���A�����X�N���[���o�[�A�����X�N���[���o�[��\�����邩�ǂ����B


+URLSchemas (http https ftp file mailto)
�����N�ɂ���X�L�[�}�B�X�y�[�X�ŋ�؂��Ďw��B


+WordWrap
���[�h���b�v�Ƌ֑����L�����ǂ����B


+TabWidth
�^�u�̕��B�w�肵���l�~x�̕������̈ʒu���^�u�ʒu�ɂȂ�B


+Ime (0)
Ime�̏�ԁB


+ArchiveAttachments (0 @ 0|1)
�f�t�H���g�œY�t�t�@�C���̈��k���L�����ǂ����B


+AutoReform (1 @ 0|1)
�f�t�H���g�Ŏ������`���L�����ǂ����B


+HideHeaderIfNoFocus (0 @ 0|1)
�w�b�_�G�f�B�b�g�r���[���t�H�[�J�X���������Ƃ��Ƀw�b�_�G�f�B�b�g�r���[���B�����ǂ����B


===Find�Z�N�V����

+Histroy?
�������������B?��0����n�܂鐔���B


+HistorySize (10)
�ۑ����闚���̍ő吔�B


+Ime (0)
Ime�̏�ԁB


+MatchCase (0 @ 0|1)
�啶���Ə���������ʂ��邩�ǂ����B


+Regex (0 @ 0|1)
���K�\�����g�����ǂ����B


===FixedFormTextDialog�Z�N�V����

+Width (620), Height (450)
��^���_�C�A���O�̃T�C�Y�B


===FolderComboBox�Z�N�V����

+FontFace, FontSize (9), FontStyle (0), FontCharset (0)
�t�H���_�R���{�{�b�N�X�̃t�H���g�B�t�H���g�T�C�Y�̓|�C���g�B


+ShowAllCount (1 @ 0|1)
���b�Z�[�W����\�����邩�ǂ����B


+ShowUnseenCount (1 @ 0|1)
���ǃ��b�Z�[�W����\�����邩�ǂ����B


===FolderListWindow�Z�N�V����

+FontFace, FontSize (9), FontStyle (0), FontCharset (0)
�t�H���_���X�g�r���[�̃t�H���g�B�t�H���g�T�C�Y�̓|�C���g�B


+UseSystemColor (1 @ 0|1)
�V�X�e���̔z�F���g�����ǂ����B


+ForegroundColor (000000), BackgroundColor (ffffff)
�����F�A�w�i�F�B�`����RRGGBB�B


+NameWidth (150), IdWidth (50), CountWidth (50), UnseenCountWidth (50), SizeWidth (150)
���O���AID���A���b�Z�[�W�����A���ǃ��b�Z�[�W�����A�T�C�Y���̕��B



===FolderWindow�Z�N�V����

+FontFace, FontSize (9), FontStyle (0), FontCharset (0)
�t�H���_�r���[�̃t�H���g�B�t�H���g�T�C�Y�̓|�C���g�B


+UseSystemColor (1 @ 0|1)
�V�X�e���̔z�F���g�����ǂ����B


+ForegroundColor (000000), BackgroundColor (ffffff)
�����F�A�w�i�F�B�`����RRGGBB�B


+AccountShowAllCount (1 @ 0|1)
�A�J�E���g�Ƀ��b�Z�[�W����\�����邩�ǂ����B


+AccountShowUnseenCount (1 @ 0|1)
�A�J�E���g�ɖ��ǃ��b�Z�[�W����\�����邩�ǂ����B


+FolderShowAllCount (1 @ 0|1)
�t�H���_�Ƀ��b�Z�[�W����\�����邩�ǂ����B


+FolderShowUnseenCount (1 @ 0|1)
�t�H���_�ɖ��ǃ��b�Z�[�W����\�����邩�ǂ����B


+DragOpenWait (500)
�h���b�O�A���h�h���b�v�Ńt�H���_�̏�Ƀh���b�O�����Ƃ��Ƀt�H���_���J���܂ł̑҂����ԁB�P�ʂ̓~���b�B


+ExpandedFolders
�W�J����Ă���A�J�E���g�ƃt�H���_�B


===FullTextSearch�Z�N�V����

+Command (namazu -l -a "$condition" "$index")
�S�������Ŏg�p����R�}���h�B


+IndexCommand (mknmz.bat -a -h -O \"$index\" \"$msg\")
�S�������̃C���f�b�N�X�X�V�Ŏg�p����R�}���h�B


===Global�Z�N�V����

+Action
ToolInvokeAction�A�N�V�����ōŌ�ɋN�������A�N�V�����B


+AddZoneId (1 @ 0|1)
�Y�t�t�@�C����ۑ�����Ƃ���ZoneId��t�����邩�ǂ����B


+AutoUpdateCheck (1 @ 0|1)
�����o�[�W�����`�F�b�N���L�����ǂ����B


+Bcc (1 @ 0|1)
�f�t�H���g�Ŏ����̃A�h���X��Bcc�ɓ���邩�ǂ����B


+CharsetAliases (windows-31j=shift_jis)
�����R�[�h���̃G�C���A�X�B

�G�C���A�X��=�G���R�[�f�B���O���̌`�ŏ������Ŏw��B�����w�肷��ꍇ�ɂ͋󔒂ŋ�؂�B


+ConfirmDeleteMessage (0 @ 0|1)
���b�Z�[�W���폜����Ƃ��Ɋm�F���邩�ǂ����B


+ConfirmEmptyFolder (1 @ 0|1)
�t�H���_����ɂ���Ƃ��Ɋm�F���邩�ǂ����B


+ConfirmEmptyTrash (1 @ 0|1)
�S�~������ɂ���Ƃ��Ɋm�F���邩�ǂ����B


+CurrentFolder
�I������Ă���t�H���_�B


+DefaultCharset
�f�t�H���g�̕����R�[�h�B�w�肳��Ă��Ȃ��ꍇ�ɂ̓v���b�g�t�H�[�����玩���擾�B


+DefaultMailAccount
�R�}���h���C������-s���g����mailto URL���w�肵�ċN�����ꂽ�Ƃ��Ɏg�p�����A�J�E���g�B


+DefaultRssAccount
�R�}���h���C������-s���g����feed URL���w�肵�ċN�����ꂽ�Ƃ��Ɏg�p�����A�J�E���g�B


+DefaultTimeFormat (%Y4/%M0/%D %h:%m:%s)
�f�t�H���g�̎��Ԃ̃t�H�[�}�b�g�B�w����@�́A((<@FormatDate|URL:FormatDateFunction.html>))���Q�ƁB


+DetachFolder
�f�t�H���g�̓Y�t�t�@�C����ۑ�����t�H���_�B


+DetachOpenFolder (0 @ 0|1)
�Y�t�t�@�C����ۑ�������ŁA�ۑ���̃t�H���_���J�����ǂ����B


+Editor (notepad.exe)
�O���G�f�B�^�B


+EmptyTrashOnExit (0 @ 0|1)
�I�����ɃS�~������ɂ��邩�ǂ����B


+Encodings (iso-8859-1 iso-2022-jp shift_jis euc-jp utf-8)
�I���\�ȕ����R�[�h�B�����w�肷��Ƃ��ɂ͋󔒋�؂�B


+ExcludeArchive (\.(?:zip|lzh|tgz|gz)$)
�Y�t�t�@�C�������k����Ƃ��ɏ��O����t�@�C�����w�肷�鐳�K�\���B


+ExternalEditor
�O���G�f�B�^�BEditor�Ŏw�肵�����̂����D�悳���B

Editor�Ŏw�肵���G�f�B�^�́A�Y�t�t�@�C�����G�f�B�^�ŊJ�����肷��Ƃ��Ȃǂɂ��g�p����܂��B���[���ҏW�p�̊O���G�f�B�^�Ƃ��đ��̃G�f�B�^���g�p�������ꍇ�ɂ͂�����Ɏw�肵�܂��B


+ExternalEditorAutoCreate (1 @ 0|1)
�O���G�f�B�^�Ń��b�Z�[�W���쐬����Ƃ��ɁA�O���G�f�B�^���I�������玩���Ń��b�Z�[�W���쐬���邩�ǂ����B


+Filer
�Y�t�t�@�C����ۑ�������Ńt�H���_���J���Ƃ��Ɏg�p����G�f�B�^�B�w�肵�Ȃ��ꍇ�ɂ͊֘A�t���ŊJ���B


+ForwardRfc822 (0 @ 0|1)
�]������Ƃ���message/rfc822�`���œ]�����邩�ǂ����B


+HideWhenMinimized (0 @ 0|1)
�ŏ������ꂽ�Ƃ��ɉB�����ǂ����B


+ImeControl (1 @ 0|1)
IME�������Ő��䂷�邩�ǂ����B


+IncrementalSearch (0 @ 0|1)
���b�Z�[�W��������G�f�B�b�g�r���[�̌����ŃC���N�������^���T�[�`���g�����ǂ����B


+LastUpdateCheck
�Ō�Ƀo�[�W�����`�F�b�N�����������B


+Libraries
���[�h����O�����C�u�����B�����w�肷��Ƃ��ɂ͋󔒋�؂�B


+Log (-1 @ -1|0|1|2|3|4)
�V�X�e�����O�̃��O���x���B

:-1
  None
:0
  Fatal
:1
  Error
:2
  Warn
:3
  Info
:4
  Debug


+LogFilter
�V�X�e�����O�����W���[�����Ńt�B���^���邽�߂̐��K�\���B


+LogTimeFormat (%Y4/%M0/%D-%h:%m:%s%z)
�V�X�e�����O�̓��t�t�H�[�}�b�g�B�w����@�́A((<@FormatDate|URL:FormatDateFunction.html>))���Q�ƁB


+Macro
MessageMacro�A�N�V�����ōŌ�Ɏw�肵���}�N���B


+NextUnseenInOtherAccounts (0 @ 0|1)
ViewNextUnseenMessage�A�N�V�����ő��̃A�J�E���g�̖��ǃ��b�Z�[�W�ɃW�����v���邩�ǂ����B


+NextUnseenWhenScrollEnd (0 @ 0|1)
ViewNextMessagePage�A�N�V�����ōŌ�܂ŃX�N���[�������Ƃ��Ɏ��̖��ǃ��b�Z�[�W�ɃW�����v���邩�ǂ����B


+NoBccForML (0 @ 0|1)
������Bcc�Ɋ܂߂�ꍇ��ML����̃��b�Z�[�W�炵���Ƃ��ɂ�Bcc��t�����Ȃ����ǂ����B


+Offline (1 @ 0|1)
�I�t���C�����ǂ����B


+OpenAddressBook (0 @ 0|1)
���b�Z�[�W�쐬���Ɏ����ŃA�h���X�I���_�C�A���O���J�����ǂ����B


+OpenRecentInPreview (0 @ 0|1)
�V�����b�Z�[�W���X�g���烁�b�Z�[�W���J���Ƃ��Ƀv���r���[�ŊJ�����ǂ����B


+PrintExtension (html)
�������Ƃ��ɏ����o���t�@�C���̊g���q�B


+Quote (> )
EditPasteWithQuote�A�N�V�����ȂǂŎg������p���B


+RFC2231 (0 @ 0|1)
�Y�t�t�@�C���̃t�@�C�����Ȃǂ�RFC2231�`���ŃG���R�[�h���邩�ǂ����B


+SaveMessageViewModePerFolder (1 @ 0|1)
���b�Z�[�W���[�h���t�H���_���Ƃɕۑ����邩�ǂ����B


+SaveOnDeactivate (1 @ 0|1)
��A�N�e�B�u�ɂȂ����Ƃ��ɕۑ����邩�ǂ����B


+ShowUnseenCountOnWelcome (0 @ 0|1)
Windows XP�̂悤������ʂɖ��ǃ��b�Z�[�W����\�����邩�ǂ����B


+TemporaryFolder
�ꎞ�t�@�C����u���t�H���_�B


+UseExternalEditor (0 @ 0|1)
�O���G�f�B�^���g�p���邩�ǂ����B


+WarnExtensions (exe com pif bat scr htm html hta vbs js)
�Y�t�t�@�C�����J���Ƃ��Ɍx������g���q�B


+XMailerWithOSVersion (1 @ 0|1)
X-Mailer��OS�̃o�[�W�������܂߂邩�ǂ����B


===GoRoundCourseDialog�Z�N�V����

+Width (620), Height(450)
����R�[�X�_�C�A���O�̃T�C�Y�B



===GPG�Z�N�V����

+Command (gpg.exe)
GPG���N������Ƃ��̃R�}���h�B


===HeaderEditWindow�Z�N�V����

+FontFace, FontSize (9), FontStyle (0), FontCharset (0)
�w�b�_�G�f�B�b�g�r���[�̃t�H���g�B�t�H���g�T�C�Y�̓|�C���g�B


+ImeTo, ImeCc, ImeBcc, ImeSubject
To, Cc, Bcc, Subject����Ime�̏�ԁB


===HeaderWindow�Z�N�V����

+FontFace, FontSize (9), FontStyle (0), FontCharset (0)
�w�b�_�r���[�̃t�H���g�B�t�H���g�T�C�Y�̓|�C���g�B


===Imap4Search�Z�N�V����

+Command (0 @ 0|1)
IMAP4�����ŃR�}���h���w�肷�邩�ǂ����B


+SearchBody (0 @ 0|1)
IMAP4�����Ŗ{�����������邩�ǂ����B


===InputBoxDialog�Z�N�V����

+Width (400), Height (300)
((<@InputBox|URL:InputBoxFunction.html>))�̕����s�_�C�A���O�̃T�C�Y�B


===JunkFilter�Z�N�V����

+BlackList, WhiteList
�u���b�N���X�g�ƃz���C�g���X�g�B


+Flags (3)
�t���O�B�ȉ��̑g�ݍ��킹��10�i�Ŏw��B

:0x01
  �����Ŋw�K
:0x02
  �蓮�Ŋw�K


+MaxTextLen (32768)
����Ώۂɂ���e�L�X�g�̍ő�T�C�Y�B�P�ʂ̓o�C�g�B


+ThresholdScore (0.95)
�X�p���Ɣ��肷��臒l�B


===Label�Z�N�V����

+Histroy?
���x���̗����B?��0����n�܂鐔���B


+HistorySize (10)
�ۑ����郉�x���̍ő吔�B


===ListWindow�Z�N�V����

+FontFace, FontSize (9), FontStyle (0), FontCharset (0)
���X�g�r���[�̃t�H���g�B�t�H���g�T�C�Y�̓|�C���g�B


+UseSystemColor (1 @ 0|1)
�V�X�e���̔z�F���g�����ǂ����B


+ForegroundColor (000000), BackgroundColor (ffffff)
�����F�A�w�i�F�B�`����RRGGBB�B


+Ellipsis (1 @ 0|1)
�J�����̕��Ɏ��܂�Ȃ�������̏I�[��...�ɂ��邩�ǂ����B


+ShowHeaderColumn (1 @ 0|1)
�w�b�_�J������\�����邩�ǂ����B


+SingleClickOpen (0 @ 0|1)
�V���O���N���b�N�Ń��b�Z�[�W�E�B���h�E���J�����ǂ����B


+TimeFormat (%Y2/%M0/%D %h:%m)
���t�t�H�[�}�b�g�B�w����@�́A((<@FormatDate|URL:FormatDateFunction.html>))���Q�ƁB


===MacroDialog�Z�N�V����

+Width (620), Height(450)
�}�N���_�C�A���O�̃T�C�Y�B


===MacroSearch�Z�N�V����

+Macro (0 @ 0|1)
�����������}�N�����ǂ����B


+MatchCase (0 @ 0|1)
�啶���Ə���������ʂ��邩�ǂ����B


+SearchHeader (0 @ 0|1)
�w�b�_���������邩�ǂ����B


+SearchBody (0 @ 0|1)
�{�����������邩�ǂ����B


+SearchMacro (@Or(@Contain(%Subject, $Search, $Case), @Contain(%From, $Search, $Case), @Contain(%To, $Search, $Case), @Contain(@Label(), $Search, $Case)))
�����Ɏg�p����}�N���B�ڍׂ́A((<��{����|URL:MacroSearch.html>))���Q�ƁB

// TODO

=end
