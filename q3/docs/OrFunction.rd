=begin
=@Or

 Boolean @Or(Boolean+)


==説明
引数で指定された真偽値の論理和を返します。つまり、全ての値がFalseならばFalseを返し、それ以外の場合にはTrueを返します。1個以上の任意の数の引数を渡せます。


==引数
:1-
  Boolean 真偽値


==エラー
*引数の数が合っていない場合


==条件
なし


==例
 # TrueとFalseの論理和
 # -> True
 @And(@True(), @False())
 
 # 未読またはマークされている
 @Or(@Not(@Seen()), @Marked())

=end
