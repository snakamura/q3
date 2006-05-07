=begin
=@And

 Boolean @And(Boolean arg+)


==説明
argで指定された真偽値の論理積を返します。つまり、全ての値がTrueならばTrueを返し、それ以外の場合にはFalseを返します。1個以上の任意の数の引数を渡せます。


==引数
:Boolean arg
  真偽値


==エラー
*引数の数が合っていない場合


==条件
なし


==例
 # TrueとTrueの論理積
 # -> True
 @And(@True(), @True())
 
 # 既読かつマークされている
 @And(@Seen(), @Marked())

=end
