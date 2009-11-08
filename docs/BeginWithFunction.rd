=begin
=@BeginWith

 Boolean @BeginWith(String s1, String s2, Boolean case?)


==説明
s1がs2から始まる場合にはTrue、それ以外の場合にはFalseを返します。caseにTrueを指定すると大文字と小文字を区別し、Falseの場合や省略された場合には区別しません。


==引数
:String s1
  文字列
:String s2
  文字列
:Boolean case
  大文字と小文字を区別する場合にはTrue、それ以外の場合にはFalse


==エラー
*引数の数が合っていない場合


==条件
なし


==例
 # %Subjectに[Qs:が含まれる（大文字と小文字を区別）
 @Begin(%Subject, '[Qs:', @True())

=end
