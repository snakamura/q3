=begin
=qmail.xml

QMAIL3�S�̂Ɋւ���ݒ��ۑ�����XML�t�@�C���ł��B���̃t�@�C���Őݒ�ł��鑽���̍��ڂ�((<�I�v�V�����̐ݒ�|URL:Options.html>))�ȂǂŐݒ�ł��܂����A�ꕔ�̍��ڂ͒��ڂ��̃t�@�C����ҏW���Ȃ��Ɛݒ�ł��܂���B�ݒ�ł��鍀�ڂ̈ꗗ�͔��l���Q�Ƃ��Ă��������B


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


===EditFrameWindow

+Left (0), Top (0), Width (0), Height (0), Show (1), Alpha (0 @ 0-255)
�G�f�B�b�g�E�B���h�E�̈ʒu�Ƒ傫���A�\�����@�Ɠ��ߓx�B


+ShowToolbar (1 @ 0|1), ShowStatusBar (1 @ 0|1)
�G�f�B�b�g�E�B���h�E�̃c�[���o�[�ƃX�e�[�^�X�o�[��\�����邩�ǂ����B


===EditWindow

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


// TODO

=end
