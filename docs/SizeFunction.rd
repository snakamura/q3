=begin
=@Size

 Number @Size(Boolean text?)


==説明
コンテキストメッセージのサイズを返します。textにTrueが指定された場合には、取得できれば本文部分のみサイズを返します。取得できない場合にはメッセージ全体のサイズを返します。textにFalseが指定された場合や引数が省略された場合にはメッセージ全体のサイズを返します。


==引数
:Boolean text
  本文部分のみのサイズを返すかどうか


==エラー
*引数の数が合っていない場合
*コンテキストメッセージホルダがない場合


==条件
なし


==例
 # サイズを取得
 @Size()
 
 # 本文部分のサイズを取得
 @Size(@True())

=end
