=begin
=@BodyCharset

 String @BodyCharset(Number type?, Part part?)


==説明
コンテキストメッセージの本文のエンコーディングを返します。partが指定された場合にはそのパートの本文のエンコーディングを返します。この関数は、@Bodyを使用したときに使用されるエンコーディングを返します。エンコーディングを直接指定している場合にはそのエンコーディングが、それ以外の場合には自動判定されたエンコーディングを返します。マルチパートの場合に各パートのエンコーディングが異なる場合にはutf-8を返します。

typeには本文のフォーマット方法を指定します。((<@Body|URL:BodyFunction.html>))のtype引数と同じ値が指定できます。


==引数
:Number type
  フォーマット方法
:Part part
  パート


==エラー
*引数の数が合っていない場合
*コンテキストメッセージがない場合
*メッセージの取得に失敗した場合
*指定したパートがない場合（partを指定した場合）


==条件
なし


==例
 # 本文を全て取得
 @Body()
 
 # インラインの本文を「> 」で引用して取得
 @Body('> ', :BODY-INLINE)
 
 # インラインの本文を取得（ただし、message/rfc822のパートは必ず取得）
 @Body('', :BODY-RFC822INLINE)
 
 # マルチパートメッセージではじめのパートの本文を取得
 @Body('', :BODY-ALL, @Part(0))

=end
