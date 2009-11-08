=begin
=@Quote

 String @Quote(String s, String quote)


==説明
指定された文字列を指定された引用符で引用します。各行の先頭に引用符が挿入されます。


==引数
:String s
  文字列
:String quote
  引用符


==エラー
*引数の数が合っていない場合


==条件
なし


==例
 # 本文を引用してクリップボードにコピー
 # @Clipboard(@Quote(@Body(), '> '))

=end
