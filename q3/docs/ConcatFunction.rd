=begin
=@Concat

 String @Concat(String*)


==説明
引数で指定された文字列を連結した文字列を返します。0個以上の任意の数の引数を渡せます。引数が指定されない場合には空文字列を返します。


==引数
:1-
  連結する文字列


==エラー
なし


==条件
なし


==例
 # FooとBarを連結
 # -> FooBar
 @Concat('Foo', 'Bar')

=end
