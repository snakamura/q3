=begin
=@Passed

 Boolean @Passed(Number value, Number unit?)


==説明
コンテキストメッセージの日付から指定された時間が経過したかどうかを返します。

unitにはvalueで指定した数値の単位を指定します。指定できるのは以下のいずれかです。

::PASSED-DAY
  日
::PASSED-HOUR
  時間
::PASSED-MINUTE
  分
::PASSED-SECOND
  秒

指定しなかった場合には:PASSED-DAYを指定したのと同じになります。



==引数
:Number value
  経過した時間
:Number unit
  経過した時間の単位


==エラー
*引数の数が合っていない場合
*コンテキストメッセージがない場合


==条件
なし


==例
 # 7日経っているかどうか調べる
 @Passed(7)
 
 # 36時間経っているかどうか調べる
 @Passed(36, :PASSED-HOUR)

=end
