=begin
=@Not

 Boolean @Not(Boolean)


==説明
引数で指定された真偽値の反対の値を返します。つまり、Trueの場合にはFalseを、Falseの場合にはTrueを返します。


==引数
:1
  Boolean 真偽値


==エラー
*引数の数が合っていない場合


==条件
なし


==例
 # Trueの反対
 # -> False
 @Not(@True())
 
 # 未読
 @Not(@Seen())

=end
