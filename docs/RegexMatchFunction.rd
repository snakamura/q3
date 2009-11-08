=begin
=@RegexMatch

 Boolean @RegexMatch(String s, Regex regex)


==説明
sで指定した文字列がregexで指定した正規表現にマッチするかどうかを返します。指定した文字列の一部がマッチする場合にもTrueを返します。

正規表現でキャプチャされた文字列は、以降のマクロで$_<数字>でアクセスできます。$_0はマッチした文字列全体をあらわし、$_1以降は各キャプチャされた文字列をあらわします。

regexには正規表現以外が指定された場合には、文字列に変換した上で正規表現としてコンパイルされます。


==引数
:String s
  文字列
:Regex regex
  正規表現


==エラー
*引数の数が合っていない場合
*正規表現が不正な場合（文字列で指定した場合）


==条件
なし


==例
 # 本文中に正規表現にマッチする文字列があるかどうかを調べる
 @RegexMatch(@Body(), /http:\/\//)
 
 # マッチした文字列を返す
 @Progn(@RegexMatch(@Body(), /^http:(.*)$/), $_1)

=end
