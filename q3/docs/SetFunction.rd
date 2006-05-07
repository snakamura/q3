=begin
=@Set

 Value @Set(String name, Value value, Boolean global?)


==説明
nameで指定された名前の変数にvalueを代入します。globalに:GLOBALを指定するとグローバル変数になります。指定されなかった場合にはローカル変数になります。valueをそのまま返します。


==引数
:String name
  変数名
:Value value
  値
:Boolean global
  グローバル変数かどうか


==エラー
*引数の数が合っていない場合


==条件
なし


==例
 # testという名前のローカル変数に@Address(To)の結果を代入
 @Set('test', @Address(To))
 
 # bccという名前のグローバル変数に@Profile('', 'Global', 'Bcc', '1')の結果を代入
 @Set('bcc', @Profile('', 'Global', 'Bcc', '1'), :GLOBAL)

=end
