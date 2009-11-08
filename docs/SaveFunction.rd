=begin
=@Save

 Boolean @Save(String path, String content, String encoding?)


==説明
指定されたパスに指定された内容を書き込み、書き込んだ内容を返します。パスに相対パスが指定された場合にはメールボックスディレクトリからの相対パスになります。

encodingを指定すると指定されたエンコーディングで書き込みます。指定されない場合にはシステムのデフォルトのエンコーディングで書き込みます。


==引数
:String path
  ファイルパス
:String content
  内容
:String encoding
  エンコーディング


==エラー
*引数の数が合っていない場合
*書き込みに失敗した場合


==条件
なし


==例
 # 本文をファイルに書き込む
 @Save('C:\\Temp\\Test.txt', @Body())

=end
