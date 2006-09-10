=begin
=toolbars.xml

�c�[���o�[�̐ݒ������XML�t�@�C���ł��B


==����

===toolbars�G�������g

 <toolbars>
  <!-- toolbar -->
 </toolbars>

toolbars�G�������g���g�b�v���x���G�������g�ɂȂ�܂��Btoolbars�G�������g�ȉ��ɂ�0�ȏ��toolbar�G�������g��u�����Ƃ��ł��܂��B


===toolbar�G�������g

 <toolbar
  name="���O"
  showText="true|false">
  <!-- button, separator -->
 </keymap>

toolbar�G�������g�̓c�[���o�[��\���܂��Bname�����ɂ̓c�[���o�[�̖��O���w�肵�܂��B

�ȉ��̖��O�̃c�[���o�[����`�ł��܂��B

:addressbookframe
  �A�h���X���E�B���h�E
:editframe
  �G�f�B�b�g�E�B���h�E
:mainframe
  ���C���E�B���h�E
:messageframe
  ���b�Z�[�W�E�B���h�E

showText�����ɂ�true���w�肷��ƃe�L�X�g���\������Afalse���w�肷��ƕ\������Ȃ��Ȃ�܂��B


===button�G�������g

 <button
  image="�C���[�W"
  text="�e�L�X�g"
  tooltip="�c�[���`�b�v"
  action="�A�N�V����"
  param="����"
  dropdown="�h���b�v�_�E�����j���["/>

button�G�������g�̓c�[���o�[�̃{�^����\���܂��B

image�����ɂ́Atoolbar.bmp�Ŏw�肵���C���[�W�̒��ł̃C���f�b�N�X���w�肵�܂��Btext�����ɂ̓e�L�X�g���Atooltip�����ɂ̓c�[���`�b�v���w�肵�܂��B

action�����ɂ̓{�^�����N���b�N���ꂽ�Ƃ��Ɏ��s�����A�N�V�������w�肵�܂��B�w��ł���A�N�V�����̈ꗗ�́A((<�A�N�V����|URL:Action.html>))���Q�Ƃ��Ă��������Bparam�����ɂ̓A�N�V�����̃p�����[�^���w�肵�܂��B

dropdown�����ɂ̓{�^�����N���b�N���ꂽ�Ƃ��ɕ\�����郁�j���[���w�肵�܂��B���j���[�͕ʓr�A((<menus.xml|URL:MenusXml.html>))�ō쐬���Ă����܂��B

action������dropdown�����������w�肳���ƁA�����̃{�^���{�̂��N���b�N����ƃA�N�V���������s����A�E���̉������O�p�`���N���b�N����ƃ��j���[���\������܂��B


===separator�G�������g

 <separator/>

separator�G�������g�̓Z�p���[�^��\���܂��B


==�T���v��

 <?xml version="1.0" encoding="utf-8"?>
 <toolbars>
  <toolbar name="mainframe" showText="true">
   <button image="0" action="MessageCreate" param="new" text="New"/>
   <button image="1" action="MessageCreate" param="reply" text="Reply"/>
   <button image="2" action="MessageCreate" param="reply_all" text="Reply All"/>
   <button image="3" action="MessageCreate" param="forward" text="Forward"/>
   <separator/>
   <button image="5" action="EditDelete" text="Delete"/>
   <separator/>
   <button image="20" action="ViewPrevMessage" text="Prev"/>
   <button image="21" action="ViewNextMessage" text="Next"/>
   <button image="22" action="ViewNextUnseenMessage" text="Unseen"/>
   <separator/>
   <button image="6" action="MessageSearch" text="Search"/>
   <separator/>
   <button image="7" action="ToolSync" text="Sync" dropdown="sync"/>
   <button image="8" action="ToolGoround" param="@0" text="Goround" dropdown="goround"/>
  </toolbar>
  <toolbar name="messageframe" showText="true">
   <button image="0" action="MessageCreate" param="new" text="New"/>
   <button image="1" action="MessageCreate" param="reply" text="Reply"/>
   <button image="2" action="MessageCreate" param="reply_all" text="Reply All"/>
   <button image="3" action="MessageCreate" param="forward" text="Forward"/>
   <separator/>
   <button image="5" action="EditDelete" text="Delete"/>
   <separator/>
   <button image="20" action="ViewPrevMessage" text="Prev"/>
   <button image="21" action="ViewNextMessage" text="Next"/>
   <button image="22" action="ViewNextUnseenMessage" text="Unseen"/>
  </toolbar>
  <toolbar name="editframe" showText="true">
   <button image="15" action="FileSend" text="Send"/>
   <button image="16" action="FileDraft" text="Draft"/>
   <separator/>
   <button image="10" action="EditCut" text="Cut"/>
   <button image="11" action="EditCopy" text="Copy"/>
   <button image="12" action="EditPaste" text="Paste"/>
   <button image="13" action="EditUndo" text="Undo"/>
   <button image="14" action="EditRedo" text="Redo"/>
   <separator/>
   <button image="18" action="ToolSelectAddress" text="Address Book"/>
   <button image="17" action="ToolInsertText" param="@0" text="Insert Text" dropdown="inserttext"/>
   <button image="19" action="ToolAttachment" text="Attachment"/>
  </toolbar>
  <toolbar name="addressbookframe" showText="true">
   <button image="23" action="AddressNew" text="New"/>
   <button image="24" action="AddressEdit" text="Edit"/>
   <button image="5" action="AddressDelete" text="Delete"/>
  </toolbar>
 </toolbars>


==�X�L�[�}

 start = element toolbars {
   element toolbar {
     (
       element button {
         empty,
         attribute image {
           xsd:nonNegativeInteger
         },
         attribute text {
           xsd:string
         }?,
         attribute tooltip {
           xsd:string
         }?,
         (action | (action, dropdown) | dropdown)
       } |
       element separator {
         empty
       }
     )*,
     attribute name {
       xsd:string
     },
     attribute showText {
       xsd:boolean
     }?
   }*
 }
 
 action = attribute action {
   xsd:string
 },
 attribute param {
   xsd:string
 }?
 
 dropdown = attribute dropdown {
   xsd:string
 }

=end
