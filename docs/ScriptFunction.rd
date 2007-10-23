=begin
=@Script

 Value @Script(String script, String lang, Value args+)


==説明
指定されたスクリプトを実行します。


==引数
:String script
  実行するスクリプト
:String lang
  スクリプト言語の名前
:Value args
  スクリプトに渡す引数


==エラー
*引数の数が合っていない場合
*スクリプトのパース・実行に失敗した場合


==条件
なし


==例
 # スクリプトを実行
 @Script('result.value = 1 + 2', 'JScript')
 
 # スクリプトに引数を渡して実行
 @Script('result.value = arguments(0) + arguments(1)', 'JScript', 1, 2)

=end
