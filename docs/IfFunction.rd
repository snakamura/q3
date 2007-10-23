=begin
=@If

 Value @If(Boolean condition, Value value, Value otherwise)


==説明
conditionを評価してTrueになったときにはvalueを返します。Falseになったときにはotherwiseを返します。3個以上の任意の奇数個の引数を渡せます。この場合、1番目の引数がTrueになった場合には2番目の引数を、3番目の引数がTrueになった場合には4番目の引数を、というように評価していき、どの条件にも合致しなかった場合には最後の引数を返します。


==引数
:Boolean condition
  条件
:Value value
  値
:Value otherwise
  どの条件にも合致しなかったときの値


==エラー
*引数の数が合っていない場合


==条件
なし


==例
 # Toがexample.orgを含んでいたら1、それ以外なら0
 @If(@Contain(To, 'example.org'), 1, 0)
 
 # Toがexample.orgを含んでいたら1、exampleを含んでいたら2、それ以外なら0
 @If(@Contain(To, 'example.org'), 1,
     @Contain(To, 'example'), 2, 0)

=end
