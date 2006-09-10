=begin
=toolbars.xml

ツールバーの設定をするXMLファイルです。


==書式

===toolbarsエレメント

 <toolbars>
  <!-- toolbar -->
 </toolbars>

toolbarsエレメントがトップレベルエレメントになります。toolbarsエレメント以下には0個以上のtoolbarエレメントを置くことができます。


===toolbarエレメント

 <toolbar
  name="名前"
  showText="true|false">
  <!-- button, separator -->
 </keymap>

toolbarエレメントはツールバーを表します。name属性にはツールバーの名前を指定します。

以下の名前のツールバーが定義できます。

:addressbookframe
  アドレス帳ウィンドウ
:editframe
  エディットウィンドウ
:mainframe
  メインウィンドウ
:messageframe
  メッセージウィンドウ

showText属性ににtrueを指定するとテキストが表示され、falseを指定すると表示されなくなります。


===buttonエレメント

 <button
  image="イメージ"
  text="テキスト"
  tooltip="ツールチップ"
  action="アクション"
  param="引数"
  dropdown="ドロップダウンメニュー"/>

buttonエレメントはツールバーのボタンを表します。

image属性には、toolbar.bmpで指定したイメージの中でのインデックスを指定します。text属性にはテキストを、tooltip属性にはツールチップを指定します。

action属性にはボタンがクリックされたときに実行されるアクションを指定します。指定できるアクションの一覧は、((<アクション|URL:Action.html>))を参照してください。param属性にはアクションのパラメータを指定します。

dropdown属性にはボタンがクリックされたときに表示するメニューを指定します。メニューは別途、((<menus.xml|URL:MenusXml.html>))で作成しておきます。

action属性とdropdown属性が両方指定されると、左側のボタン本体をクリックするとアクションが実行され、右側の下向き三角形をクリックするとメニューが表示されます。


===separatorエレメント

 <separator/>

separatorエレメントはセパレータを表します。


==サンプル

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


==スキーマ

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
