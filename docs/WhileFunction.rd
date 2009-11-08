=begin
=@While

 Value @While(Boolean condition, Value value)


==説明
conditionの値がTrueの間、valueの値を繰り返し評価します。conditionも毎回評価されます。最後に評価されたconditionが返されます。

使い方を誤ると無限ループしますので注意してください。


==引数
:Boolean condition
  繰り返しの条件
:Value value
  評価する値


==エラー
*引数の数が合っていない場合


==条件
なし


==例
 # 変数に値を代入しながら10回ループを回す
 @While(@Less(@Variable('n', 0), 10),
        @Progn(@MessageBox($n),
               @Set('n', @Add($n, 1))))

=end
