=begin
=keymap.xml

�V���[�g�J�b�g�L�[�̐ݒ������XML�t�@�C���ł��B


==����

===keymaps�G�������g

 <keymaps>
  <!-- keymap -->
 </keymaps>

keymaps�G�������g���g�b�v���x���G�������g�ɂȂ�܂��Bkeymaps�G�������g�ȉ��ɂ�0�ȏ��keymap�G�������g��u�����Ƃ��ł��܂��B


===keymap�G�������g

 <keymap
  name="���O">
  <!-- action -->
 </keymap>

keymap�G�������g�̓L�[�}�b�v��\���܂��Bname�����ɂ̓L�[�}�b�v�̖��O���w�肵�܂��B

�ȉ��̖��O�̃L�[�}�b�v����`�ł��܂��B

:AddressBookFrameWindow
  �A�h���X���E�B���h�E
:AddressBookListWindow
  �A�h���X�r���[
:EditFrameWindow
  �G�f�B�b�g�E�B���h�E
:EditWindow
  �G�f�B�b�g�r���[
:HeaderEditWindow
  �w�b�_�G�f�B�b�g�r���[
:FolderComboBox
  �t�H���_�R���{�{�b�N�X
:FolderListWindow
  �t�H���_���X�g�r���[
:FolderWindow
  �t�H���_�r���[
:ListWindow
  ���X�g�r���[
:MainWindow
  ���C���E�B���h�E
:MessageFrameWindow
  ���b�Z�[�W�E�B���h�E
:MessageWindow
  ���b�Z�[�W�r���[
:PreviewWindow
  �v���r���[

�ł������̃E�B���h�E�Œ�`���ꂽ�ݒ肪�L���ɂȂ�܂��B�����Œ�`����Ă��Ȃ��ꍇ�ɂ͊O���̒�`���Q�Ƃ��܂��B�Ⴆ�΁AMainWindow�Őݒ肳�ꂽ���e�̓��C���E�B���h�E�ŗL���ł����A�����L�[�̐ݒ肪ListWindow�ɂ���ꍇ�ɂ́A���X�g�r���[�ł͂�����̐ݒ肪�D�悳��܂��B


===action�G�������g

 <action
  name="�A�N�V����"
  param="����">
  <!-- key -->
 </action>

action�G�������g�̓A�N�V������\���܂��Bname�����ɃA�N�V�����̖��O���w�肵�܂��B�w��ł���A�N�V�����̈ꗗ�́A((<�A�N�V����|URL:Action.html>))���Q�Ƃ��Ă��������Bparam�����ɂ̓A�N�V�����̈������w�肵�܂��Baction�G�������g�ȉ��ɂ�0�ȏ��key�G�������g��u�����Ƃ��ł��܂��B


===key�G�������g

 <key
  key="�L�["
  code="�L�[�R�[�h"
  name="�L�[��"
  shift="true|false"
  ctrl="true|false"
  alt="true|false"
  virtual="true|false"/>

key�G�������g�ŃL�[���w�肵�܂��B

key�����܂���code�����܂���name�����ŃL�[���w�肵�܂��Bkey�����ł�"A"��"1"�Ȃǂ̂悤�Ɏ��ۂ̃L�[���w�肵�܂��Bcode�����ł�16�i���ŃL�[�R�[�h���w�肵�܂��B�Ⴆ�΁A���s�̃L�[�R�[�h��0xD�Ȃ̂�"D"�Ǝw�肵�܂��Bkey�����ł�name�����ł��w��ł��Ȃ��ꍇ�Ɏg�p���܂��Bname�����ɂ̓L�[�̖��O���w�肵�܂��B�w��ł���l�͈ȉ��̒ʂ�ł��B


:escape
  Esc
:return
  Enter
:space
  �X�y�[�X
:tab
  Tab
:back
  Backspace
:delete
  Delete
:insert
  Insert
:home
  Home
:end
  End
:prior
  Page Up
:next
  Page Down
:help
  Help
:convert
  �ϊ�
:nonconvert
  ���ϊ�
:kana
  �J�^�J�i/�Ђ炪��
:kanji
  ����
:f1����f24
  F1����F24
:up
  �J�[�\���L�[�̏�
:down
  �J�[�\���L�[�̉�
:left
  �J�[�\���L�[�̍�
:right
  �J�[�\���L�[�̉E
:numpad0����numpad9
  �e���L�[��0����9
:add
  �e���L�[��+
:subtract
  �e���L�[��-
:multiply
  �e���L�[��*
:divide
  �e���L�[��/
:lwin
  ��Windows
:rwin
  �EWindows
:apps
  �A�v���P�[�V�����L�[

shift, ctrl, alt�����ɂ͂��ꂼ��AShift, Ctrl, Alt���������Ƃ��ɗL�����ǂ������w�肵�܂��Bvirtual�����ɂ͎w�肵���L�[�����z�L�[�R�[�h�łȂ��ꍇ��false���w�肵�܂��B�w�肵�Ȃ��ꍇ�ɂ�true�ɂȂ�܂��B


==�T���v��

 <?xml version="1.0" encoding="utf-8"?>
 <keymaps>
  <keymap name="MainWindow">
   <action name="EditCopy">
    <key key="C" ctrl="true"/>
    <key name="insert" ctrl="true"/>
   </action>
   <action name="EditCut">
    <key key="X" ctrl="true"/>
    <key name="delete" shift="true"/>
   </action>
   <action name="EditFind">
    <key key="F" ctrl="true"/>
   </action>
   <action name="EditFindNext">
    <key name="f3"/>
   </action>
   <action name="EditFindPrev">
    <key name="f3" shift="true"/>
   </action>
   <action name="EditPaste">
    <key key="V" ctrl="true"/>
    <key name="insert" shift="true"/>
   </action>
   <action name="EditSelectAll">
    <key key="A" ctrl="true"/>
   </action>
   <action name="EditUndo">
    <key key="Z" ctrl="true"/>
   </action>
   <action name="FileExit">
    <key key="X" alt="true"/>
   </action>
   <action name="FileOffline">
    <key key="O" ctrl="true"/>
   </action>
   <action name="MessageSearch">
    <key key="S" ctrl="true"/>
   </action>
   <action name="ViewFocusNext">
    <key name="tab"/>
   </action>
   <action name="ViewFocusPrev">
    <key name="tab" shift="true"/>
   </action>
   <action name="ViewRefresh">
    <key key="R" ctrl="true"/>
   </action>
   <action name="TabClose">
    <key key="W" ctrl="true"/>
   </action>
   <action name="TabLock">
    <key key="Z" alt="true"/>
   </action>
   <action name="TabMoveLeft">
    <key name="left" alt="true"/>
   </action>
   <action name="TabMoveRight">
    <key name="right" alt="true"/>
   </action>
   <action name="TabNavigateNext">
    <key name="tab" ctrl="true"/>
   </action>
   <action name="TabNavigatePrev">
    <key name="tab" shift="true" ctrl="true"/>
   </action>
   <action name="TabSelect" param="@0">
    <key key="1" alt="true"/>
   </action>
   <action name="TabSelect" param="@1">
    <key key="2" alt="true"/>
   </action>
   <action name="TabSelect" param="@2">
    <key key="3" alt="true"/>
   </action>
   <action name="TabSelect" param="@3">
    <key key="4" alt="true"/>
   </action>
   <action name="TabSelect" param="@4">
    <key key="5" alt="true"/>
   </action>
   <action name="TabSelect" param="@5">
    <key key="6" alt="true"/>
   </action>
   <action name="TabSelect" param="@6">
    <key key="7" alt="true"/>
   </action>
   <action name="TabSelect" param="@7">
    <key key="8" alt="true"/>
   </action>
   <action name="TabSelect" param="@8">
    <key key="9" alt="true"/>
   </action>
   <action name="TabSelect" param="@9">
    <key key="0" alt="true"/>
   </action>
   <action name="ToolGoround" param="@0">
    <key key="[" virtual="false"/>
   </action>
   <action name="ToolInvokeAction">
    <key key="X" alt="true" shift="true"/>
   </action>
   <action name="ToolReceive">
    <key key="." virtual="false"/>
   </action>
   <action name="ToolSend">
    <key key="," virtual="false"/>
   </action>
   <action name="ToolSync">
    <key key="/" virtual="false"/>
   </action>
  </keymap>
  <keymap name="FolderWindow">
   <action name="FolderCollapse">
    <key name="subtract" shift="true"/>
   </action>
   <action name="FolderExpand">
    <key name="add" shift="true"/>
   </action>
   <action name="FolderProperty">
    <key name="return" alt="true"/>
   </action>
   <action name="ViewNextAccount">
    <key name="down" shift="true"/>
   </action>
   <action name="ViewPrevAccount">
    <key name="up" shift="true"/>
   </action>
  </keymap>
  <keymap name="FolderListWindow">
   <action name="FolderProperty">
    <key name="return" alt="true"/>
   </action>
  </keymap>
  <keymap name="ListWindow">
   <action name="EditDelete">
    <key name="delete"/>
    <key key="D"/>
   </action>
   <action name="EditDeleteDirect">
    <key name="delete" shift="true"/>
    <key key="D" shift="true"/>
   </action>
   <action name="EditDeleteJunk">
    <key key="J" ctrl="true"/>
   </action>
   <action name="FileHide">
    <key key="Q"/>
   </action>
   <action name="FileSave">
    <key key="W"/>
   </action>
   <action name="MessageApplyRule">
    <key key="L"/>
   </action>
   <action name="MessageCreate" param="edit">
    <key name="return" shift="true"/>
   </action>
   <action name="MessageCreate" param="forward">
    <key key="F"/>
   </action>
   <action name="MessageCreate" param="new">
    <key key="S"/>
   </action>
   <action name="MessageCreate" param="reply">
    <key key="R"/>
   </action>
   <action name="MessageCreate" param="reply_all">
    <key key="R" shift="true"/>
   </action>
   <action name="MessageDetach">
    <key key="X"/>
   </action>
   <action name="MessageLabel">
    <key key="L" shift="true"/>
   </action>
   <action name="MessageMacro">
    <key key="X" ctrl="true" alt="true"/>
   </action>
   <action name="MessageMark">
    <key key="M" ctrl="true"/>
   </action>
   <action name="MessageMarkDeleted">
    <key key="D" ctrl="true"/>
   </action>
   <action name="MessageMarkDownload">
    <key key="O"/>
   </action>
   <action name="MessageOpenLink">
    <key key="L" ctrl="true"/>
   </action>
   <action name="MessageProperty">
    <key name="return" alt="true"/>
   </action>
   <action name="MessageUnmark">
    <key key="M" shift="true" ctrl="true"/>
   </action>
   <action name="ViewNextMessage">
    <key key="N"/>
   </action>
   <action name="ViewNextMessagePage">
    <key name="space"/>
   </action>
   <action name="ViewPrevMessage">
    <key key="P"/>
   </action>
   <action name="ViewPrevMessagePage">
    <key name="space" shift="true"/>
   </action>
   <action name="ViewNextUnseenMessage">
    <key key="M"/>
   </action>
   <action name="ViewScrollLineDown">
    <key key="N" ctrl="true"/>
   </action>
   <action name="ViewScrollLineUp">
    <key key="P" ctrl="true"/>
   </action>
   <action name="ViewScrollPageDown">
    <key key="N" alt="true"/>
   </action>
   <action name="ViewScrollPageUp">
    <key key="P" alt="true"/>
   </action>
   <action name="ViewScrollBottom">
    <key key="N" ctrl="true" alt="true"/>
   </action>
   <action name="ViewScrollTop">
    <key key="P" ctrl="true" alt="true"/>
   </action>
   <action name="ViewShowPreview">
    <key key="J"/>
   </action>
   <action name="ViewSortToggleThread">
    <key key="K"/>
   </action>
  </keymap>
  <keymap name="PreviewWindow">
   <action name="EditDelete">
    <key name="delete"/>
    <key key="D"/>
   </action>
   <action name="EditDeleteDirect">
    <key name="delete" shift="true"/>
    <key key="D" shift="true"/>
   </action>
   <action name="EditDeleteJunk">
    <key key="J" ctrl="true"/>
   </action>
   <action name="MessageCreate" param="edit">
    <key name="return" shift="true"/>
   </action>
   <action name="MessageCreate" param="forward">
    <key key="F"/>
   </action>
   <action name="MessageCreate" param="new">
    <key key="S"/>
   </action>
   <action name="MessageCreate" param="reply">
    <key key="R"/>
   </action>
   <action name="MessageCreate" param="reply_all">
    <key key="R" shift="true"/>
   </action>
   <action name="MessageDetach">
    <key key="X"/>
   </action>
   <action name="MessageLabel">
    <key key="L" shift="true"/>
   </action>
   <action name="MessageMacro">
    <key key="X" ctrl="true" alt="true"/>
   </action>
   <action name="MessageMark">
    <key key="M" ctrl="true"/>
   </action>
   <action name="MessageMarkDownload">
    <key key="O"/>
   </action>
   <action name="MessageOpenLink">
    <key key="L" ctrl="true"/>
   </action>
   <action name="MessageProperty">
    <key name="return" alt="true"/>
   </action>
   <action name="MessageUnmark">
    <key key="M" shift="true" ctrl="true"/>
   </action>
   <action name="ViewHtmlMode">
    <key key="H" shift="true"/>
   </action>
   <action name="ViewNextMessage">
    <key key="N"/>
    <key name="return"/>
   </action>
   <action name="ViewNextMessagePage">
    <key name="space"/>
   </action>
   <action name="ViewNextUnseenMessage">
    <key key="M"/>
   </action>
   <action name="ViewOpenLink">
    <key name="space" ctrl="true"/>
   </action>
   <action name="ViewPrevMessage">
    <key key="P"/>
    <key name="back"/>
   </action>
   <action name="ViewPrevMessagePage">
    <key name="space" shift="true"/>
   </action>
   <action name="ViewRawMode">
    <key key="H"/>
   </action>
   <action name="ViewSelectMode">
    <key key="V"/>
   </action>
   <action name="ViewZoom" param="-1">
    <key key="[" virtual="false" alt="true"/>
   </action>
   <action name="ViewZoom" param="+1">
    <key key="]" virtual="false" alt="true"/>
   </action>
  </keymap>
  <keymap name="MessageFrameWindow">
   <action name="EditCopy">
    <key key="C" ctrl="true"/>
    <key name="insert" ctrl="true"/>
   </action>
   <action name="EditCut">
    <key key="X" ctrl="true"/>
    <key name="delete" shift="true"/>
   </action>
   <action name="EditPaste">
    <key key="V" ctrl="true"/>
    <key name="insert" shift="true"/>
   </action>
   <action name="EditSelectAll">
    <key key="A" ctrl="true"/>
   </action>
   <action name="EditUndo">
    <key key="Z" ctrl="true"/>
   </action>
   <action name="FileClose">
    <key name="escape"/>
   </action>
  </keymap>
  <keymap name="MessageWindow">
   <action name="EditDelete">
    <key name="delete"/>
    <key key="D"/>
   </action>
   <action name="EditDeleteDirect">
    <key name="delete" shift="true"/>
    <key key="D" shift="true"/>
   </action>
   <action name="EditDeleteJunk">
    <key key="J" ctrl="true"/>
   </action>
   <action name="EditFind">
    <key key="F" ctrl="true"/>
   </action>
   <action name="EditFindNext">
    <key name="f3"/>
   </action>
   <action name="EditFindPrev">
    <key name="f3" shift="true"/>
   </action>
   <action name="MessageCreate" param="edit">
    <key name="return" shift="true"/>
   </action>
   <action name="MessageCreate" param="forward">
    <key key="F"/>
   </action>
   <action name="MessageCreate" param="new">
    <key key="S"/>
   </action>
   <action name="MessageCreate" param="reply">
    <key key="R"/>
   </action>
   <action name="MessageCreate" param="reply_all">
    <key key="R" shift="true"/>
   </action>
   <action name="MessageDetach">
    <key key="X"/>
   </action>
   <action name="MessageLabel">
    <key key="L" shift="true"/>
   </action>
   <action name="MessageMacro">
    <key key="X" ctrl="true" alt="true"/>
   </action>
   <action name="MessageMark">
    <key key="M" ctrl="true"/>
   </action>
   <action name="MessageMarkDownload">
    <key key="O"/>
   </action>
   <action name="MessageOpenLink">
    <key key="L" ctrl="true"/>
   </action>
   <action name="MessageProperty">
    <key name="return" alt="true"/>
   </action>
   <action name="MessageUnmark">
    <key key="M" shift="true" ctrl="true"/>
   </action>
   <action name="ViewHtmlMode">
    <key key="H" shift="true"/>
   </action>
   <action name="ViewNextMessage">
    <key key="N"/>
    <key name="return"/>
   </action>
   <action name="ViewNextMessagePage">
    <key name="space"/>
   </action>
   <action name="ViewNextUnseenMessage">
    <key key="M"/>
   </action>
   <action name="ViewOpenLink">
    <key name="space" ctrl="true"/>
   </action>
   <action name="ViewPrevMessage">
    <key key="P"/>
    <key name="back"/>
   </action>
   <action name="ViewPrevMessagePage">
    <key name="space" shift="true"/>
   </action>
   <action name="ViewRawMode">
    <key key="H"/>
   </action>
   <action name="ViewSelectMode">
    <key key="V"/>
   </action>
   <action name="ViewZoom" param="-1">
    <key key="[" virtual="false" alt="true"/>
   </action>
   <action name="ViewZoom" param="+1">
    <key key="]" virtual="false" alt="true"/>
   </action>
  </keymap>
  <keymap name="EditFrameWindow">
   <action name="FileClose">
    <key name="escape"/>
   </action>
  </keymap>
  <keymap name="EditWindow">
   <action name="EditCopy">
    <key key="C" ctrl="true"/>
    <key name="insert" ctrl="true"/>
   </action>
   <action name="EditCut">
    <key key="X" ctrl="true"/>
    <key name="delete" shift="true"/>
   </action>
   <action name="EditFind">
    <key key="F" ctrl="true"/>
   </action>
   <action name="EditFindNext">
    <key name="f3"/>
    <key key="L" ctrl="true"/>
   </action>
   <action name="EditFindPrev">
    <key name="f3" shift="true"/>
    <key key="L" shift="true" ctrl="true"/>
   </action>
   <action name="EditPaste">
    <key key="V" ctrl="true"/>
    <key name="insert" shift="true"/>
   </action>
   <action name="EditRedo">
    <key key="Y" ctrl="true"/>
   </action>
   <action name="EditReplace">
    <key key="H" ctrl="true"/>
   </action>
   <action name="EditSelectAll">
    <key key="A" ctrl="true"/>
   </action>
   <action name="EditUndo">
    <key key="Z" ctrl="true"/>
   </action>
   <action name="FileOpen">
    <key key="O" ctrl="true"/>
   </action>
   <action name="FileSave">
    <key key="S" ctrl="true"/>
   </action>
   <action name="FileSend">
    <key key="M" alt="true"/>
   </action>
   <action name="HeaderEditItem" param="@0">
    <key key="O" alt="true"/>
   </action>
   <action name="HeaderEditItem" param="@1">
    <key key="C" alt="true"/>
   </action>
   <action name="HeaderEditItem" param="@2">
    <key key="B" alt="true"/>
   </action>
   <action name="HeaderEditItem" param="@3">
    <key key="N" alt="true"/>
   </action>
   <action name="HeaderEditItem" param="@4">
    <key key="L" alt="true"/>
   </action>
   <action name="HeaderEditItem" param="@5">
    <key key="S" alt="true"/>
   </action>
   <action name="HeaderEditItem" param="@6">
    <key key="H" alt="true"/>
   </action>
   <action name="HeaderEditItem" param="@7">
    <key key="U" alt="true"/>
   </action>
   <action name="HeaderEditItem" param="@8">
    <key key="I" alt="true"/>
   </action>
   <action name="ToolSelectAddress">
    <key key="A" alt="true"/>
   </action>
   <action name="ToolAttachment">
    <key key="T" ctrl="true"/>
   </action>
   <action name="ToolInsertSignature">
    <key key="I" shift="true" ctrl="true"/>
   </action>
   <action name="ToolInsertText" param="@0">
    <key key="I" ctrl="true"/>
   </action>
   <action name="ToolReform">
    <key name="return" shift="true"/>
   </action>
   <action name="ViewFocusPrevEditItem">
    <key name="tab" shift="true"/>
   </action>
  </keymap>
  <keymap name="HeaderEditWindow">
   <action name="ViewFocusNextEditItem">
    <key name="tab"/>
   </action>
   <action name="ViewFocusPrevEditItem">
    <key name="tab" shift="true"/>
   </action>
  </keymap>
  <keymap name="AddressBookFrameWindow">
   <action name="FileClose">
    <key name="escape"/>
   </action>
   <action name="FileSave">
    <key key="S" ctrl="true"/>
   </action>
   <action name="ViewRefresh">
    <key key="R" ctrl="true"/>
   </action>
  </keymap>
  <keymap name="AddressBookListWindow">
   <action name="AddressDelete">
    <key key="D" ctrl="true"/>
   </action>
   <action name="AddressEdit">
    <key name="return"/>
   </action>
   <action name="AddressNew">
    <key key="N" ctrl="true"/>
   </action>
  </keymap>
 </keymaps>


==�X�L�[�}

 element keymaps {
   element keymap {
     element action {
       element key {
         empty,
         (
           attribute key {
             xsd:string
           } |
           attribute code {
             xsd:string {
               pattern = "[0-9a-fA-F]+"
             }
           } |
           attribute name {
             xsd:string
           }
         ),
         attribute shift {
           xsd:boolean
         }?,
         attribute ctrl {
           xsd:boolean
         }?,
         attribute alt {
           xsd:boolean
         }?,
         attribute virtual {
           xsd:boolean
         }?
       }+,
       attribute name {
         xsd:string
       },
       attribute param {
         xsd:string
       }?
     }*,
     attribute name {
       xsd:string
     }
   }*
 }

=end
