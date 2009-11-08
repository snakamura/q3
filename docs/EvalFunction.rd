=begin
=@Eval

 Value @Eval(String expr)


==説明
指定されたマクロを実行します。


==引数
:String expr
  実行するマクロ


==エラー
*引数の数が合っていない場合
*マクロとして不正な場合


==条件
なし


==例
 # 文字列で指定されたマクロを実行
 @Eval('@If(Reply-To, Reply-To, From)')

=end
