=begin
=@Catch

 Value @Catch(Expr expr, Expr handler)


==説明
exprを評価しその結果を返します。その間にエラーが発生した場合にはhandlerを評価してその結果を返します。


==引数
:Expr expr
  評価する式
:Expr handler
  エラーが起きたときに評価される式


==エラー
*引数の数が合っていない場合
*handlerを評価中にエラーが発生した場合


==条件
なし


==例
 # ファイルを読み込んで返すが、エラーが起きたら空文字列を返す
 @Catch(@Load('C:\\Temp\\test.txt'), '')
 
 # コンテキストメッセージに既読フラグを設定するが、
 # コンテキストメッセージがなかったら何もしない
 @Catch(@Seen(@True()), @True())

=end
