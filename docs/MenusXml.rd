=begin
=menus.xml

メニューの設定をするXMLファイルです。


==書式

===menusエレメント

 <menus>
  <!-- menubar, menu -->
 </menus>

menusエレメントがトップレベルエレメントになります。menusエレメント以下には0個以上のmenubarエレメントまたはmenuエレメントを置くことができます。


===menubarエレメント

 <menubar
  name="名前">
  <!-- menuitem, popupmenu, separator -->
 </menubar>

menubarエレメントはメニューバーを表します。name属性にはメニューバーの名前を指定します。

以下の名前のメニュバーが定義できます。

:addressbookframe
  アドレス帳ウィンドウ
:editframe
  エディットウィンドウ
:mainframe
  メインウィンドウ
:messageframe
  メッセージウィンドウ


===menuエレメント

 <menu
  name="名前">
  <!-- menuitem, popupmenu, separator -->
 </menu>

menuエレメントはコンテキストメニューを表します。name属性にはコンテキストメニューの名前を指定します。

以下の名前のメニューが定義できます。

:edit
  エディットビューのコンテキストメニュー
:folder
  フォルダビューのコンテキストメニュー
:folderlist
  フォルダリストビューのコンテキストメニュー
:list
  リストビューのコンテキストメニュー
:message
  メッセージビューのコンテキストメニュー
:attachment
  添付ファイルのコンテキストメニュー
:attachmentedit
  添付ファイル編集のコンテキストメニュー
:tab
  タブのコンテキストメニュー
:addressbooklist
  アドレスビューのコンテキストメニュー
:encoding
  ステータスバーのエンコーディング欄のコンテキストメニュー
:filter
  ステータスバーのフィルタ欄のコンテキストメニュー
:template
  ステータスバーのテンプレート欄のコンテキストメニュー
:sync
  ツールバーの同期ボタンで使われるメニュー
:goround
  ツールバーの巡回ボタンで使われるメニュー
:inserttext
  ツールバーの定型文ボタンで使われるメニュー
:recents
  新着メッセージで使われるメニュー

またこれら以外の名前のメニューを定義して、((<toolbars.xml|URL:ToolbarsXml.html>))のbuttonエレメントのdropdown属性に指定したり、((<ToolPopupMenuアクション|URL:ToolPopupMenuAction.html>))で使用することもできます。


===menuitemエレメント

 <menuitem
  text="文字列"
  action="アクション"
  param="引数"
  dynamic="動的メニュー名"/>

menuitemエレメントはメニューのアイテムを表します。メニューのアイテムには静的アイテムと動的なアイテムがあります。

静的なアイテムは定義時に決まっているアイテムで、text属性、action属性、param属性を使って指定します。

text属性には表示する文字列を指定します。

action属性にはそのアイテムが選択されたときに実行される((<アクション|URL:Action.html>))を指定します。

param属性には必要があればアクションに渡す引数を指定します。引数はスペースで区切って複数指定することができます。一つの引数にスペースを含めたい場合には""で括ります。""で括った場合には、その中に含まれる"と\を\でエスケープします。例えば、param属性に@Execute("C:\\Program Files\\QMAIL3\\q3u.exe")を指定する場合には、XML中では以下のようにエスケープされます。

 param="&quot;@Execute(\&quot;C:\\\\Program Files\\\\QMAIL3\\\\q3u.exe\&quot;)&quot;"

動的なアイテムは実行時に決まるアイテムで、通常は実行時に複数のアイテムになります。動的なアイテムは、dynamic属性とparam属性を使って指定します。dynamic属性にはアイテムが選択されたときに実行される((<アクション|URL:Action.html>))を指定します。param属性にはアクションに渡される引数を生成するマクロを指定します。

param属性に指定したマクロを評価した結果は以下のようなフォーマットになっている必要があります。

*一行に一アクション
*一行はタブで区切られていて、タブより前に表示用の文字列、後ろにアクションに渡す引数を指定する（このとき、引数はスペースで区切って複数指定することができます。一つの引数にスペースを含めたい場合には""で括ります）

例えば、

 <menuitem dynamic="MessageCreate"
           param="'新規\tnew\n返信\treply'"/>

のようにすると、MessageCreateアクションを使って新規と返信の二つのメニューが作成され、それぞれのアイテムを選択したときには、MessageCreateアクションにnewとreplyが引数として渡されます。

この例ではparamのマクロが常に同じ値を返すので動的にする意味がありませんが、状況に応じて返す文字列が返す文字列を変えるようなマクロを指定することで動的にメニューを生成することができます。

これに加えて、以下のアクションをdynamic属性に指定しparam属性を省略すると、以下の説明にあるようなメニューが動的に生成されます。

:AddressCreateMessage
  選択されているアドレス帳のエントリに登録されているアドレスをリストします
:MessageCreate
  ファイル名がcreate_から始まる((<作成用テンプレート|URL:CreateTemplate.html>))をリストします
:MessageCreateExternal
  ファイル名がcreate_から始まる((<作成用テンプレート|URL:CreateTemplate.html>))をリストします
:MessageMove
  現在のアカウントのすべてのフォルダを階層的にリストします
:MessageOpenAttachment
  現在のメッセージの添付ファイルをリストします
:MessageOpenRecent
  新着メッセージをリストします
:ToolApplyTemplate
  ファイル名がedit_から始まる((<編集用テンプレート|URL:EditTemplate.html>))をリストします
:ToolEncoding
  エンコーディングをリストします
:ToolGoround
  巡回コースをリストします
:ToolInsertText
  定型文をリストします
:ToolScript
  スクリプトをリストします
:ToolSubAccount
  サブアカウントをリストします
:ViewEncoding
  エンコーディングをリストします
:ViewFilter
  フィルタをリストします
:ViewFontGroup
  フォントグループをリストします
:ViewSort
  リストビューのカラムをリストします
:ViewTemplate
  ファイル名がview_から始まる((<表示用テンプレート|URL:ViewTemplate.html>))をリストします


===separatorエレメント

 <separator/>

separatorエレメントはセパレータを表します。


===popupmenuエレメント

 <popupmenu
  text="文字列">
  <!-- menuitem, popupmenu, separator -->
 </popupmenu>

popupmenuエレメントはポップアップメニューを表します。text属性には表示する文字列を指定します。


==スキーマ

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
