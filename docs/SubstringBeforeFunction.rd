=begin
=@SubstringBefore

 String @SubstringBefore(String s, String separator, Boolean case?)


==説明
sで指定された文字列内のseparatorで指定した文字列より前の文字列を返します。そのような文字列が見つからない場合には空文字列を返します。caseにTrueを指定すると大文字と小文字を区別し、Falseの場合や省略された場合には区別しません。


==引数
:String s
  文字列
:String separator
  検索する文字列
:Boolean case
  大文字と小文字を区別する場合にはTrue、それ以外の場合にはFalse


==エラー
*引数の数が合っていない場合


==条件
なし


==例
 # cより前を取得
 # -> ab
 @SubstringBefore('abcdef', 'c')
 
 # 大文字小文字を区別してXYZより前を取得
 # -> wxyz
 @SubstringBefore('wxyzXYZZZ', 'XYZ', @True())

=end
