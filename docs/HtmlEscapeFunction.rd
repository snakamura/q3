=begin
=@HtmlEscape

 String @HtmlEscape(String s)


==説明
sをHTMLとしてエスケープした文字列を返します。

以下の置き換えを行います。

*<を&lt;
*>を&gt;
*&を&amp;
*"を&quot;


==引数
:String s
  エスケープする文字列


==エラー
*引数の数が合っていない場合


==条件
なし


==例
 # 本文をHTMLとしてエスケープ
 @HtmlEscape(@Body())

=end
