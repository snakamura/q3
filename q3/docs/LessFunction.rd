=begin
=@Less

 Boolean @Less(Value v1, Value v2, Boolean case?)


==説明
v1がv2よりも小さい場合にはTrue、それ以外の場合にはFalseを返します。v1とv2がともに真偽値の場合には真偽値として、v1とv2がともに数値の場合には数値として、それ以外の場合には文字列として比較します。文字列として比較する場合には、caseにTrueを指定すると大文字と小文字を区別し、Falseの場合や省略された場合には区別しません。


==引数
:Value v1
  値
:Value v2
  文字列
:Boolean case
  大文字と小文字を区別する場合にはTrue、それ以外の場合にはFalse


==エラー
*引数の数が合っていない場合


==条件
なし


==例
 # サイズが10KBより小さい
 @Less(@Size(), 10240)
 
 # abcはdよりも小さい
 # -> True
 @Less('abc', 'd')

=end
