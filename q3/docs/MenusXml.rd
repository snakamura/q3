=begin
=menus.xml

���j���[�̐ݒ������XML�t�@�C���ł��B


==����

===menus�G�������g

 <menus>
  <!-- menubar, menu -->
 </menus>

menus�G�������g���g�b�v���x���G�������g�ɂȂ�܂��Bmenus�G�������g�ȉ��ɂ�0�ȏ��menubar�G�������g�܂���menu�G�������g��u�����Ƃ��ł��܂��B


===menubar�G�������g

 <menubar
  name="���O">
  <!-- menuitem, popupmenu, separator -->
 </menubar>

menubar�G�������g�̓��j���[�o�[��\���܂��Bname�����ɂ̓��j���[�o�[�̖��O���w�肵�܂��B

�ȉ��̖��O�̃��j���o�[����`�ł��܂��B

:addressbookframe
  �A�h���X���E�B���h�E
:editframe
  �G�f�B�b�g�E�B���h�E
:mainframe
  ���C���E�B���h�E
:messageframe
  ���b�Z�[�W�E�B���h�E


===menu�G�������g

 <menu
  name="���O">
  <!-- menuitem, popupmenu, separator -->
 </menu>

menu�G�������g�̓R���e�L�X�g���j���[��\���܂��Bname�����ɂ̓R���e�L�X�g���j���[�̖��O���w�肵�܂��B

�ȉ��̖��O�̃��j���[����`�ł��܂��B

:edit
  �G�f�B�b�g�r���[�̃R���e�L�X�g���j���[
:folder
  �t�H���_�r���[�̃R���e�L�X�g���j���[
:folderlist
  �t�H���_���X�g�r���[�̃R���e�L�X�g���j���[
:list
  ���X�g�r���[�̃R���e�L�X�g���j���[
:message
  ���b�Z�[�W�r���[�̃R���e�L�X�g���j���[
:attachment
  �Y�t�t�@�C���̃R���e�L�X�g���j���[
:attachmentedit
  �Y�t�t�@�C���ҏW�̃R���e�L�X�g���j���[
:tab
  �^�u�̃R���e�L�X�g���j���[
:addressbooklist
  �A�h���X�r���[�̃R���e�L�X�g���j���[
:encoding
  �X�e�[�^�X�o�[�̃G���R�[�f�B���O���̃R���e�L�X�g���j���[
:filter
  �X�e�[�^�X�o�[�̃t�B���^���̃R���e�L�X�g���j���[
:template
  �X�e�[�^�X�o�[�̃e���v���[�g���̃R���e�L�X�g���j���[
:sync
  �c�[���o�[�̓����{�^���Ŏg���郁�j���[
:goround
  �c�[���o�[�̏���{�^���Ŏg���郁�j���[
:inserttext
  �c�[���o�[�̒�^���{�^���Ŏg���郁�j���[
:recents
  �V�����b�Z�[�W�Ŏg���郁�j���[

�܂������ȊO�̖��O�̃��j���[���`���āA((<toolbars.xml|URL:ToolbarsXml.html>))��button�G�������g��dropdown�����Ɏw�肵����A((<ToolPopupMenu�A�N�V����|URL:ToolPopupMenuAction.html>))�Ŏg�p���邱�Ƃ��ł��܂��B


===menuitem�G�������g

 <menuitem
  text="������"
  action="�A�N�V����"
  param="����"
  dynamic="���I���j���[��"/>

menuitem�G�������g�̓��j���[�̃A�C�e����\���܂��B���j���[�̃A�C�e���ɂ͐ÓI�A�C�e���Ɠ��I�ȃA�C�e��������܂��B

�ÓI�ȃA�C�e���͒�`���Ɍ��܂��Ă���A�C�e���ŁAtext�����Aaction�����Aparam�������g���Ďw�肵�܂��B

text�����ɂ͕\�����镶������w�肵�܂��B

action�����ɂ͂��̃A�C�e�����I�����ꂽ�Ƃ��Ɏ��s�����((<�A�N�V����|URL:Action.html>))���w�肵�܂��B

param�����ɂ͕K�v������΃A�N�V�����ɓn���������w�肵�܂��B�����̓X�y�[�X�ŋ�؂��ĕ����w�肷�邱�Ƃ��ł��܂��B��̈����ɃX�y�[�X���܂߂����ꍇ�ɂ�""�Ŋ���܂��B""�Ŋ������ꍇ�ɂ́A���̒��Ɋ܂܂��"��\��\�ŃG�X�P�[�v���܂��B�Ⴆ�΁Aparam������@Execute("C:\\Program Files\\QMAIL3\\q3u.exe")���w�肷��ꍇ�ɂ́AXML���ł͈ȉ��̂悤�ɃG�X�P�[�v����܂��B

 param="&quot;@Execute(\&quot;C:\\\\Program Files\\\\QMAIL3\\\\q3u.exe&quot;)&quot;"

���I�ȃA�C�e���͎��s���Ɍ��܂�A�C�e���ŁA�ʏ�͎��s���ɕ����̃A�C�e���ɂȂ�܂��B���I�ȃA�C�e���́Adynamic������param�������g���Ďw�肵�܂��Bdynamic�����ɂ̓A�C�e�����I�����ꂽ�Ƃ��Ɏ��s�����((<�A�N�V����|URL:Action.html>))���w�肵�܂��Bparam�����ɂ̓A�N�V�����ɓn���������𐶐�����}�N�����w�肵�܂��B

param�����Ɏw�肵���}�N����]���������ʂ͈ȉ��̂悤�ȃt�H�[�}�b�g�ɂȂ��Ă���K�v������܂��B

*��s�Ɉ�A�N�V����
*��s�̓^�u�ŋ�؂��Ă��āA�^�u���O�ɕ\���p�̕�����A���ɃA�N�V�����ɓn���������w�肷��i���̂Ƃ��A�����̓X�y�[�X�ŋ�؂��ĕ����w�肷�邱�Ƃ��ł��܂��B��̈����ɃX�y�[�X���܂߂����ꍇ�ɂ�""�Ŋ���܂��j

�Ⴆ�΁A

 <menuitem dynamic="MessageCreate"
           param="'�V�K\tnew\n�ԐM\treply'"/>

�̂悤�ɂ���ƁAMessageCreate�A�N�V�������g���ĐV�K�ƕԐM�̓�̃��j���[���쐬����A���ꂼ��̃A�C�e����I�������Ƃ��ɂ́AMessageCreate�A�N�V������new��reply�������Ƃ��ēn����܂��B

���̗�ł�param�̃}�N������ɓ����l��Ԃ��̂œ��I�ɂ���Ӗ�������܂��񂪁A�󋵂ɉ����ĕԂ������񂪕Ԃ��������ς���悤�ȃ}�N�����w�肷�邱�Ƃœ��I�Ƀ��j���[�𐶐����邱�Ƃ��ł��܂��B

����ɉ����āA�ȉ��̃A�N�V������dynamic�����Ɏw�肵�Aparam�������ȗ�����Ɛ����ɂ���悤�ȃ��j���[�����I�ɐ�������܂��B

:AddressCreateMessage
  �I������Ă���A�h���X���̃G���g���ɓo�^����Ă���A�h���X�����X�g���܂�
:MessageCreate
  �t�@�C������create_����n�܂�쐬�p�e���v���[�g�����X�g���܂�
:MessageCreateExternal
  �t�@�C������create_����n�܂�쐬�p�e���v���[�g�����X�g���܂�
:MessageMove
  ���݂̃A�J�E���g�̂��ׂẴt�H���_���K�w�I�Ƀ��X�g���܂�
:MessageOpenAttachment
  ���݂̃��b�Z�[�W�̓Y�t�t�@�C�������X�g���܂�
:MessageOpenRecent
  �V�����b�Z�[�W�����X�g���܂�
:ToolEncoding
  �G���R�[�f�B���O�����X�g���܂�
:ToolGoround
  ����R�[�X�����X�g���܂�
:ToolInsertText
  ��^�������X�g���܂�
:ToolScript
  �X�N���v�g�����X�g���܂�
:ToolSubAccount
  �T�u�A�J�E���g�����X�g���܂�
:ViewEncoding
  �G���R�[�f�B���O�����X�g���܂�
:ViewFilter
  �t�B���^�����X�g���܂�
:ViewFontGroup
  �t�H���g�O���[�v�����X�g���܂�
:ViewSort
  ���X�g�r���[�̃J���������X�g���܂�
:ViewTemplate
  �t�@�C������view_����n�܂�\���p�e���v���[�g�����X�g���܂�


===separator�G�������g

 <separator/>

separator�G�������g�̓Z�p���[�^��\���܂��B


===popupmenu�G�������g

 <popupmenu
  text="������">
  <!-- menuitem, popupmenu, separator -->
 </popupmenu>

popupmenu�G�������g�̓|�b�v�A�b�v���j���[��\���܂��Btext�����ɂ͕\�����镶������w�肵�܂��B


==�X�L�[�}

 start = element menus {
   (
     element menubar {
       item*,
       attribute name {
         xsd:string
       }
     } |
     element menu {
       item*,
       attribute name {
         xsd:string
       }
     }
   )*
 }
 
 item = element menuitem {
   (
     attribute text {
       xsd:string
     },
     attribute action {
       xsd:string
     },
     attribute param {
       xsd:string
     }?
   ) |
   (
     attribute dynamic {
       xsd:string
     },
     attribute param {
       xsd:string
     }?
   )
 } |
 element separator {
   empty
 } |
 element popupmenu {
   attribute text {
     xsd:string
   },
   item*
 }

=end
