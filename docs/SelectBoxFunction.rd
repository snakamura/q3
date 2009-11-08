=begin
=@SelectBox

 String @SelectBox(String message, String candidates, Number type?, String default?)


==説明
ユーザに選択を求めるためのダイアログを表示し、選択された文字列を返します。messageには表示するメッセージを指定します。candidatesにはリストに候補として表示する文字列を改行区切りで指定します。たとえば、foo, bar, bazを候補にする場合には、'foo\nbar\nbaz'のように指定します。

typeにはリストのタイプを指定します。指定できるのは以下のとおりです。

::SELECT-LIST
  リスト表示
  
  ((<リスト選択ダイアログ|"IMG:images/ListSelectBoxDialog.png">))

::SELECT-DROPDOWNLIST
  ドロップダウンリスト表示
  
  ((<ドロップダウンリスト選択ダイアログ|"IMG:images/DropDownListSelectBoxDialog.png">))

::SELECT-DROPDOWN
  ドロップダウン表示
  
  ((<ドロップダウン選択ダイアログ|"IMG:images/DropDownSelectBoxDialog.png">))

指定しない場合には、SELECT-LISTを指定したのと同じになります。

defaultを指定するとデフォルトでその候補が選択されます。候補にない値を指定すると、指定しないのと同じになりますが、typeが:SELECT-DROPDOWNの場合にはその値が入力された状態になります。指定しない場合には先頭の候補が選択されます。



==引数
:String message
  表示するメッセージ
:String candidates
  リストに表示する候補
:Number type
  リストのタイプ
:String default
  デフォルトの入力文字列


==エラー
*引数の数が合っていない場合
*UIがない場合

==条件
*UIが必要


==例
 # foo, bar, bazからリストで選択
 @SelectBox('選択してください', 'foo\nbar\nbaz')
 
 # us-ascii, iso-2022-jp, utf-8からドロップダウンで選択（デフォルトはiso-2022-jp）
 @SelectBox('エンコーディング', 'us-ascii\niso-2022-jp\nutf-8', :SELECT-DROPDOWN, 'iso-2022-jp')

=end
