=begin
=@Not

 Boolean @Not(Boolean arg)


==説明
argで指定された真偽値の反対の値を返します。つまり、Trueの場合にはFalseを、Falseの場合にはTrueを返します。


==引数
:Boolean arg
  真偽値


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
