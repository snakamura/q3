=begin
=@Substring

 String @Substring(String s, Number offset, Number length?)


==説明
sで指定された文字列の、offsetで指定された文字目からlengthで指定された長さの部分文字列を取得します。offsetは0ベースで指定します。lengthを指定しない場合や長さが文字列の最後を超える場合には文字列の最後までを取得します。offsetがsの長さよりも大きい場合には空文字列を返します。


==引数
:String s
  文字列
:Number offset
  0ベースのオフセット
:Number length
  長さ


==エラー
*引数の数が合っていない場合


==条件
なし


==例
 # 3文字目から4文字取得
 # -> defg
 @Substring('abcdefghij', 3, 4)
 
 # 5文字目以降を取得
 # -> fghij
 @Substring('abcdefghij', 5)

=end
