=begin
=keymap.xml

ショートカットキーの設定をするXMLファイルです。


==書式

===keymapsエレメント

 <keymaps>
  <!-- keymap -->
 </keymaps>

keymapsエレメントがトップレベルエレメントになります。keymapsエレメント以下には0個以上のkeymapエレメントを置くことができます。


===keymapエレメント

 <keymap
  name="名前">
  <!-- action -->
 </keymap>

keymapエレメントはキーマップを表します。name属性にはキーマップの名前を指定します。

以下の名前のキーマップが定義できます。

:AddressBookFrameWindow
  アドレス帳ウィンドウ
:AddressBookListWindow
  アドレスビュー
:EditFrameWindow
  エディットウィンドウ
:EditWindow
  エディットビュー
:HeaderEditWindow
  ヘッダエディットビュー
:FolderComboBox
  フォルダコンボボックス
:FolderListWindow
  フォルダリストビュー
:FolderWindow
  フォルダビュー
:ListWindow
  リストビュー
:MainWindow
  メインウィンドウ
:MessageFrameWindow
  メッセージウィンドウ
:MessageWindow
  メッセージビュー
:PreviewWindow
  プレビュー

最も内側のウィンドウで定義された設定が有効になります。内側で定義されていない場合には外側の定義を参照します。例えば、MainWindowで設定された内容はメインウィンドウで有効ですが、同じキーの設定がListWindowにある場合には、リストビューではそちらの設定が優先されます。


===actionエレメント

 <action
  name="アクション"
  param="引数">
  <!-- key -->
 </action>

actionエレメントはアクションを表します。name属性にアクションの名前を指定します。指定できるアクションの一覧は、((<アクション|URL:Action.html>))を参照してください。param属性にはアクションの引数を指定します。actionエレメント以下には0個以上のkeyエレメントを置くことができます。


===keyエレメント

 <key
  key="キー"
  code="キーコード"
  name="キー名"
  shift="true|false"
  ctrl="true|false"
  alt="true|false"
  virtual="true|false"/>

keyエレメントでキーを指定します。

key属性またはcode属性またはname属性でキーを指定します。key属性では"A"や"1"などのように実際のキーを指定します。code属性では16進数でキーコードを指定します。例えば、改行のキーコードは0xDなので"D"と指定します。key属性でもname属性でも指定できない場合に使用します。name属性にはキーの名前を指定します。指定できる値は以下の通りです。


:escape
  Esc
:return
  Enter
:space
  スペース
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
  変換
:nonconvert
  無変換
:kana
  カタカナ/ひらがな
:kanji
  漢字
:f1からf24
  F1からF24
:up
  カーソルキーの上
:down
  カーソルキーの下
:left
  カーソルキーの左
:right
  カーソルキーの右
:numpad0からnumpad9
  テンキーの0から9
:add
  テンキーの+
:subtract
  テンキーの-
:multiply
  テンキーの*
:divide
  テンキーの/
:lwin
  左Windows
:rwin
  右Windows
:apps
  アプリケーションキー

shift, ctrl, alt属性にはそれぞれ、Shift, Ctrl, Altを押したときに有効かどうかを指定します。virtual属性には指定したキーが仮想キーコードでない場合にfalseを指定します。指定しない場合にはtrueになります。


==サンプル

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


==スキーマ

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
