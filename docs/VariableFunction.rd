=begin
=@Variable

 Value @Variable(String name, Value value?, Boolean global?)


==説明
nameで指定された名前の変数の値を返します。変数が見つかった場合にはvalueとglobalは無視されます。指定した名前の変数が見つからない場合には、((<@Set|URL:SetFunction.html>))と同様に振舞います。


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
 # testという変数に値が入っていたらそれを返す。入っていなかったら0をセットしてそれを返す
 @Variable('test', 0)

=end
