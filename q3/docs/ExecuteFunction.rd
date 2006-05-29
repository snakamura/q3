=begin
=@Execute

 String @Execute(String command, String input?)


==説明
commandで指定されたコマンドを実行します。

commandが関連付けられたファイルだった場合には関連付けでファイルを開きます。

inputが指定された場合には、指定された文字列をシステムのエンコーディングでバイト列に変換した結果をコマンドの標準入力に渡します。この場合、標準出力から出力されたバイト列をシステムのエンコーディングで文字列に変換した結果を返します。inputの指定はWindows版でのみ可能です。

inputが指定されなかった場合には、コマンドを実行して空文字列を返します。


==引数
:String command
  実行するコマンド
:String input
  コマンドの標準入力に渡す文字列


==エラー
*引数の数が合っていない場合
*コマンドの実行に失敗した場合


==条件
なし


==例
 # メモ帳を起動
 @Execute('notepad.exe')
 
 # 空白文字を含むときには""で括る
 @Execute('"C:\\Program Files\\Test\\test.exe"')
 
 # 引数を渡す
 @Execute('notepad.exe "C:\\Temp\\test.txt"')
 
 # 関連付け
 @Execute('C:/Temp/Test.doc')
 
 # フィルタ
 @Execute('sed.exe -e "s/foo/bar/"', 'foo')

=end
