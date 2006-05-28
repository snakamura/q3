=begin
=@Messages

 MessageList @Messages(String folder?, Number id?)


==説明
メッセージのリストを返します。folderが指定された場合にはそのフォルダ内のメッセージをすべて返します。さらにidが指定された場合にはそのIDのメッセージを返します。何も指定されなかった場合にはアカウント中のすべてのメッセージのリストを返します。

返されたメッセージリストは((<@ForEach|URL:ForEachFunction.html>))や((<@FindEach|URL:FindEachFunction.html>))などを使用して処理することができます。


==引数
:String folder
  フォルダの完全名
:Number id
  メッセージのID


==エラー
*引数の数が合っていない場合
*指定されたフォルダが存在しない場合
*指定されたフォルダが通常フォルダではない場合（idが指定された場合）


==条件
なし


==例
 # 受信箱のすべてのメッセージを既読にする
 @ForEach(@Messages('受信箱'), @Seen(@True()))
 
 # 受信箱のIDが1000のメッセージを削除する
 @ForEach(@Messages('受信箱', 1000), @Delete())

=end
